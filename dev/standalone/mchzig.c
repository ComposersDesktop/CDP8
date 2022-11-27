/*
 * Copyright (c) 1983-2013 Trevor Wishart and Composers Desktop Project Ltd
 * http://www.trevorwishart.co.uk
 * http://www.composersdesktop.com
 *
 This file is part of the CDP System.

    The CDP System is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    The CDP System is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with the CDP System; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
 *
 */



/* MCHZIG  zigzag with mchan output
 */

#include <stdio.h>
#include <stdlib.h>
#include <structures.h>
#include <tkglobals.h>
#include <pnames.h>
#include <filetype.h>
#include <processno.h>
#include <modeno.h>
#include <logic.h>
#include <globcon.h>
#include <cdpmain.h>
#include <math.h>
#include <mixxcon.h>
#include <osbind.h>
#include <standalone.h>
#include <ctype.h>
#include <sfsys.h>
#include <string.h>
#include <srates.h>
#include <extdcon.h>
#include <arrays.h>
#include <flags.h>
//#include <memory.h>
//#include <string.h>

#if defined unix || defined __GNUC__
#define round(x) lround((x))
#endif
#ifndef HUGE
#define HUGE 3.40282347e+38F
#endif

char errstr[2400];

int anal_infiles = 1;
int	sloom = 0;
int sloombatch = 0;

const char* cdp_version = "7.1.0";

/* internal */
#define MZIG_SPLICECNT (8)
#define MZIG_SPLSAMPS  (9)
#define MZIG_RUNSTOEND (10)

#define MZIG_PERMCH	(0)

#define	ZIG		(1)
#define UNKNOWN	(0)
#define ZAG		(-1)

#define NORMAL	(0)
#define REVERSE	(1)

#define	FORWARDS	(1)
#define	BACKWARDS	(-1)

#define ROOT2		(1.4142136)

#define SECMARGIN  (256) 

static int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz);
static int get_the_mode_from_cmdline(char *str,dataptr dz);
static int setup_mchzig_application(dataptr dz);
static int setup_mchzig_param_ranges_and_defaults(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int handle_the_outfile(int *cmdlinecnt,char ***cmdline,dataptr dz);
static int open_the_outfile(dataptr dz);

static int setup_and_init_input_param_activity(dataptr dz,int tipc);
static int setup_input_param_defaultval_stores(int tipc,aplptr ap);
static int establish_application(dataptr dz);
static int initialise_vflags(dataptr dz);
static int setup_parameter_storage_and_constants(int storage_cnt,dataptr dz);
static int initialise_is_int_and_no_brk_constants(int storage_cnt,dataptr dz);
static int mark_parameter_types(dataptr dz,aplptr ap);
static int assign_file_data_storage(int infilecnt,dataptr dz);
static int get_tk_cmdline_word(int *cmdlinecnt,char ***cmdline,char *q);
static int setup_and_init_input_brktable_constants(dataptr dz,int brkcnt);

static int check_zigzag_consistency(dataptr dz);
static int mzigzag_preprocess(int *maxzig,dataptr dz);

static int handle_the_special_data(int *cmdlinecnt,char ***cmdline,dataptr dz);
static int setup_the_special_data_ranges(int mode,int srate,double duration,double nyquist,int wlength,int channels,aplptr ap);
static int read_ziginfo(char *filename,dataptr dz);

static int setup_zigzag_splice(int paramno,dataptr dz);
static int make_zigsplice(int paramno,dataptr dz);
static int create_zigzag_splicebuffer(dataptr dz);
static int generate_zigzag_table(dataptr dz);
static int sort_zigs(int *maxzig,dataptr dz);
static int eliminate_n_steps(int *this_zigtime,int *next_zigtime,int **ziglistend,int *cnt,dataptr dz);
static int eliminate_step(int *next_zigtime,int **ziglistend,int *cnt,dataptr dz);
static int zigzag(dataptr dz);
static int zig_or_zag(int *thisstart,int *lastend,int *outbuf_space,int obufno,int obufendno,int startchan,int endchan,dataptr dz);
static int find_zzchunk(int *thisstart,int *ziglistend, int *minsamp, dataptr dz);
static int reverse_it(int incnt,dataptr dz);
static int copy_with_spatial_scatter(float *outptr,float *tobufend,int startchan,int endchan,int sampcnt,int chancnt,dataptr dz);
static void permute_chans(int outchans,dataptr dz);
static void insertch(int n,int t,int outchans,dataptr dz);
static void prefixch(int n,int outchans,dataptr dz);
static void shuflupch(int k,int outchans,dataptr dz);
static int mz_setup_internal_arrays_and_array_pointers(dataptr dz);
static void pancalc(double position,double *leftgain,double *rightgain);
static void get_bufsize_needed(int *maxzig,dataptr dz);
static int create_mzig_sndbufs(int maxzig,dataptr dz);
static void do_start_splice(float *buf,dataptr dz);
static void do_end_splice(float *buf,int incnt,dataptr dz);

static int adjacence(int endchan,dataptr dz);

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
	int exit_status;
	dataptr dz = NULL;
	char **cmdline;
	int  cmdlinecnt;
	int n, maxzig;
	aplptr ap;
	int is_launched = FALSE;
	if(argc==2 && (strcmp(argv[1],"--version") == 0)) {
		fprintf(stdout,"%s\n",cdp_version);
		fflush(stdout);
		return 0;
	}
						/* CHECK FOR SOUNDLOOM */
	if((sloom = sound_loom_in_use(&argc,&argv)) > 1) {
		sloom = 0;
		sloombatch = 1;
	}
	if(sflinit("cdp")){
		sfperror("cdp: initialisation\n");
		return(FAILED);
	}
						  /* SET UP THE PRINCIPLE DATASTRUCTURE */
	if((exit_status = establish_datastructure(&dz))<0) {					// CDP LIB
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if(!sloom) {
		if(argc == 1) {
			usage1();	
			return(FAILED);
		} else if(argc == 2) {
			usage2(argv[1]);	
			return(FAILED);
		}
	}
	if(!sloom) {
		if((exit_status = make_initial_cmdline_check(&argc,&argv))<0) {		// CDP LIB
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		cmdline    = argv;
		cmdlinecnt = argc;
		if((get_the_process_no(argv[0],dz))<0)
			return(FAILED);
		cmdline++;
		cmdlinecnt--;
		dz->maxmode = 2;
		if((exit_status = get_the_mode_from_cmdline(cmdline[0],dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(exit_status);
		}
		cmdline++;
		cmdlinecnt--;
		// setup_particular_application =
		if((exit_status = setup_mchzig_application(dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		if((exit_status = count_and_allocate_for_infiles(cmdlinecnt,cmdline,dz))<0) {		// CDP LIB
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
	} else {
		//parse_TK_data() =
		if((exit_status = parse_sloom_data(argc,argv,&cmdline,&cmdlinecnt,dz))<0) {
			exit_status = print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(exit_status);		 
		}
	}
	ap = dz->application;

	// parse_infile_and_hone_type() = 
	if((exit_status = parse_infile_and_check_type(cmdline,dz))<0) {
		exit_status = print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	// setup_param_ranges_and_defaults() =
	if((exit_status = setup_mchzig_param_ranges_and_defaults(dz))<0) {
		exit_status = print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	// open_first_infile		CDP LIB
	if((exit_status = open_first_infile(cmdline[0],dz))<0) {	
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);	
		return(FAILED);
	}
	cmdlinecnt--;
	cmdline++;

//	handle_extra_infiles() : redundant
	// handle_outfile() = 
	if((exit_status = handle_the_outfile(&cmdlinecnt,&cmdline,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}

//	handle_formants()			redundant
//	handle_formant_quiksearch()	redundant
//	handle_special_data() =
	if(dz->mode == 1) {
		if((exit_status = handle_the_special_data(&cmdlinecnt,&cmdline,dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
	}
	if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {		// CDP LIB
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
//	check_param_validity_and_consistency....
	if((exit_status = check_zigzag_consistency(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	is_launched = TRUE;
	dz->bufcnt = 2 + dz->iparam[MZIG_OCHANS];
	dz->extra_bufcnt = 1;
	if((dz->sampbuf = (float **)malloc(sizeof(float *) * (dz->bufcnt+1)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffers.\n");
		return(MEMORY_ERROR);
	}
	if((dz->sbufptr = (float **)malloc(sizeof(float *) * dz->bufcnt))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffer pointers.\n");
		return(MEMORY_ERROR);
	}
	for(n = 0;n <dz->bufcnt; n++)
		dz->sampbuf[n] = dz->sbufptr[n] = (float *)0;
	dz->sampbuf[n] = (float *)0;
	if((dz->extrabuf = (float **)malloc(sizeof(float *) * (dz->extra_bufcnt+1)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY establishing extra buffers.\n");
		return(MEMORY_ERROR);
	}
	if((dz->extrabufptr = (float **)malloc(sizeof(float *) * (dz->extra_bufcnt+1)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY establishing extrabuffer pointers.\n");
		return(MEMORY_ERROR);
	}
	for(n = 0;n <dz->extra_bufcnt; n++)
		dz->extrabuf[n] = dz->extrabufptr[n] = (float *)0;
	dz->extrabuf[n] = (float *)0;
	//param_preprocess() ...........
	if((exit_status = mzigzag_preprocess(&maxzig,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = create_mzig_sndbufs(maxzig,dz))<0) {										// CDP LIB
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = open_the_outfile(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	//groucho_process_file =
	if((exit_status = zigzag(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = complete_output(dz))<0) {										// CDP LIB
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	exit_status = print_messages_and_close_sndfiles(FINISHED,is_launched,dz);		// CDP LIB
	free(dz);
	return(SUCCEEDED);
}

/**********************************************
		REPLACED CDP LIB FUNCTIONS
**********************************************/


/****************************** SET_PARAM_DATA *********************************/

int set_param_data(aplptr ap, int special_data,int maxparamcnt,int paramcnt,char *paramlist)
{
	ap->special_data   = (char)special_data;	   
	ap->param_cnt      = (char)paramcnt;
	ap->max_param_cnt  = (char)maxparamcnt;
	if(ap->max_param_cnt>0) {
		if((ap->param_list = (char *)malloc((size_t)(ap->max_param_cnt+1)))==NULL) {	
			sprintf(errstr,"INSUFFICIENT MEMORY: for param_list\n");
			return(MEMORY_ERROR);
		}
		strcpy(ap->param_list,paramlist); 
	}
	return(FINISHED);
}

/****************************** SET_VFLGS *********************************/

int set_vflgs
(aplptr ap,char *optflags,int optcnt,char *optlist,char *varflags,int vflagcnt, int vparamcnt,char *varlist)
{
	ap->option_cnt 	 = (char) optcnt;			/*RWD added cast */
	if(optcnt) {
		if((ap->option_list = (char *)malloc((size_t)(optcnt+1)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY: for option_list\n");
			return(MEMORY_ERROR);
		}
		strcpy(ap->option_list,optlist);
		if((ap->option_flags = (char *)malloc((size_t)(optcnt+1)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY: for option_flags\n");
			return(MEMORY_ERROR);
		}
		strcpy(ap->option_flags,optflags); 
	}
	ap->vflag_cnt = (char) vflagcnt;		   
	ap->variant_param_cnt = (char) vparamcnt;
	if(vflagcnt) {
		if((ap->variant_list  = (char *)malloc((size_t)(vflagcnt+1)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY: for variant_list\n");
			return(MEMORY_ERROR);
		}
		strcpy(ap->variant_list,varlist);		
		if((ap->variant_flags = (char *)malloc((size_t)(vflagcnt+1)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY: for variant_flags\n");
			return(MEMORY_ERROR);
		}
		strcpy(ap->variant_flags,varflags);

	}
	return(FINISHED);
}

/***************************** APPLICATION_INIT **************************/

int application_init(dataptr dz)
{
	int exit_status;
	int storage_cnt;
	int tipc, brkcnt;
	aplptr ap = dz->application;
	if(ap->vflag_cnt>0)
		initialise_vflags(dz);	  
	tipc  = ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt;
	ap->total_input_param_cnt = (char)tipc;
	if(tipc>0) {
		if((exit_status = setup_input_param_range_stores(tipc,ap))<0)			  
			return(exit_status);
		if((exit_status = setup_input_param_defaultval_stores(tipc,ap))<0)		  
			return(exit_status);
		if((exit_status = setup_and_init_input_param_activity(dz,tipc))<0)	  
			return(exit_status);
	}
	brkcnt = tipc;
	//THERE ARE NO INPUTFILE brktables USED IN THIS PROCESS
	if(brkcnt>0) {
		if((exit_status = setup_and_init_input_brktable_constants(dz,brkcnt))<0)			  
			return(exit_status);
	}
	if((storage_cnt = tipc + ap->internal_param_cnt)>0) {		  
		if((exit_status = setup_parameter_storage_and_constants(storage_cnt,dz))<0)	  
			return(exit_status);
		if((exit_status = initialise_is_int_and_no_brk_constants(storage_cnt,dz))<0)	  
			return(exit_status);
	}													   
 	if((exit_status = mark_parameter_types(dz,ap))<0)	  
		return(exit_status);
	
	// establish_infile_constants() replaced by
	dz->infilecnt = 1;
	//establish_bufptrs_and_extra_buffers():
	if((exit_status = mz_setup_internal_arrays_and_array_pointers(dz))<0)	 
		return(exit_status);
	return(FINISHED);
}

/********************** SETUP_PARAMETER_STORAGE_AND_CONSTANTS ********************/
/* RWD mallo changed to calloc; helps debug verison run as release! */

int setup_parameter_storage_and_constants(int storage_cnt,dataptr dz)
{
	if((dz->param       = (double *)calloc(storage_cnt, sizeof(double)))==NULL) {
		sprintf(errstr,"setup_parameter_storage_and_constants(): 1\n");
		return(MEMORY_ERROR);
	}
	if((dz->iparam      = (int    *)calloc(storage_cnt, sizeof(int)   ))==NULL) {
		sprintf(errstr,"setup_parameter_storage_and_constants(): 2\n");
		return(MEMORY_ERROR);
	}
	if((dz->is_int      = (char   *)calloc(storage_cnt, sizeof(char)))==NULL) {
		sprintf(errstr,"setup_parameter_storage_and_constants(): 3\n");
		return(MEMORY_ERROR);
	}
	if((dz->no_brk      = (char   *)calloc(storage_cnt, sizeof(char)))==NULL) {
		sprintf(errstr,"setup_parameter_storage_and_constants(): 5\n");
		return(MEMORY_ERROR);
	}
	return(FINISHED);
}

/************** INITIALISE_IS_INT_AND_NO_BRK_CONSTANTS *****************/

int initialise_is_int_and_no_brk_constants(int storage_cnt,dataptr dz)
{
	int n;
	for(n=0;n<storage_cnt;n++) {
		dz->is_int[n] = (char)0;
		dz->no_brk[n] = (char)0;
	}
	return(FINISHED);
}

/***************************** MARK_PARAMETER_TYPES **************************/

int mark_parameter_types(dataptr dz,aplptr ap)
{
	int n, m;							/* PARAMS */
	for(n=0;n<ap->max_param_cnt;n++) {
		switch(ap->param_list[n]) {
		case('0'):	break; /* dz->is_active[n] = 0 is default */
		case('i'):	dz->is_active[n] = (char)1; dz->is_int[n] = (char)1;dz->no_brk[n] = (char)1; break;
		case('I'):	dz->is_active[n] = (char)1;	dz->is_int[n] = (char)1; 						 break;
		case('d'):	dz->is_active[n] = (char)1;							dz->no_brk[n] = (char)1; break;
		case('D'):	dz->is_active[n] = (char)1;	/* normal case: double val or brkpnt file */	 break;
		default:
			sprintf(errstr,"Programming error: invalid parameter type in mark_parameter_types()\n");
			return(PROGRAM_ERROR);
		}
	}						 		/* OPTIONS */
	for(n=0,m=ap->max_param_cnt;n<ap->option_cnt;n++,m++) {
		switch(ap->option_list[n]) {
		case('i'): dz->is_active[m] = (char)1; dz->is_int[m] = (char)1;	dz->no_brk[m] = (char)1; break;
		case('I'): dz->is_active[m] = (char)1; dz->is_int[m] = (char)1; 						 break;
		case('d'): dz->is_active[m] = (char)1; 							dz->no_brk[m] = (char)1; break;
		case('D'): dz->is_active[m] = (char)1;	/* normal case: double val or brkpnt file */	 break;
		default:
			sprintf(errstr,"Programming error: invalid option type in mark_parameter_types()\n");
			return(PROGRAM_ERROR);
		}
	}								/* VARIANTS */
	for(n=0,m=ap->max_param_cnt + ap->option_cnt;n < ap->variant_param_cnt; n++, m++) {
		switch(ap->variant_list[n]) {
		case('0'): break;
		case('i'): dz->is_active[m] = (char)1; dz->is_int[m] = (char)1;	dz->no_brk[m] = (char)1; break;
		case('I'): dz->is_active[m] = (char)1; dz->is_int[m] = (char)1;	 						 break;
		case('d'): dz->is_active[m] = (char)1; 							dz->no_brk[m] = (char)1; break;
		case('D'): dz->is_active[m] = (char)1; /* normal case: double val or brkpnt file */		 break;
		default:
			sprintf(errstr,"Programming error: invalid variant type in mark_parameter_types()\n");
			return(PROGRAM_ERROR);
		}
	}								/* INTERNAL */
	for(n=0,
	m=ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt; n<ap->internal_param_cnt; n++,m++) {
		switch(ap->internal_param_list[n]) {
		case('0'):  break;	 /* dummy variables: variables not used: but important for internal paream numbering!! */
		case('i'):	dz->is_int[m] = (char)1;	dz->no_brk[m] = (char)1;	break;
		case('d'):								dz->no_brk[m] = (char)1;	break;
		default:
			sprintf(errstr,"Programming error: invalid internal param type in mark_parameter_types()\n");
			return(PROGRAM_ERROR);
		}
	}
	return(FINISHED);
}

/************************ HANDLE_THE_OUTFILE *********************/

int handle_the_outfile(int *cmdlinecnt,char ***cmdline,dataptr dz)
{
	char *filename = (*cmdline)[0];
	if(filename[0]=='-' && filename[1]=='f') {
		dz->floatsam_output = 1;
		dz->true_outfile_stype = SAMP_FLOAT;
		filename+= 2;
	}
	if(!sloom) {
		if(file_has_invalid_startchar(filename) || value_is_numeric(filename)) {
			sprintf(errstr,"Outfile name %s has invalid start character(s) or looks too much like a number.\n",filename);
			return(DATA_ERROR);
		}
	}
	strcpy(dz->outfilename,filename);	   
	(*cmdline)++;
	(*cmdlinecnt)--;
	return(FINISHED);
}

/************************ OPEN_THE_OUTFILE *********************/

int open_the_outfile(dataptr dz)
{	
	int exit_status;
	dz->infile->channels  = dz->iparam[MZIG_OCHANS];
	dz->outfile->channels = dz->iparam[MZIG_OCHANS];
	if((exit_status = create_sized_outfile(dz->outfilename,dz))<0)
		return(exit_status);
	dz->infile->channels = 1;
	return(FINISHED);
}

/***************************** ESTABLISH_APPLICATION **************************/

int establish_application(dataptr dz)
{
	aplptr ap;
	if((dz->application = (aplptr)malloc(sizeof (struct applic)))==NULL) {
		sprintf(errstr,"establish_application()\n");
		return(MEMORY_ERROR);
	}
	ap = dz->application;
	memset((char *)ap,0,sizeof(struct applic));
	return(FINISHED);
}

/************************* INITIALISE_VFLAGS *************************/

int initialise_vflags(dataptr dz)
{
	int n;
	if((dz->vflag  = (char *)malloc(dz->application->vflag_cnt * sizeof(char)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY: vflag store,\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<dz->application->vflag_cnt;n++)
		dz->vflag[n]  = FALSE;
	return FINISHED;
}

/************************* SETUP_INPUT_PARAM_DEFAULTVALS *************************/

int setup_input_param_defaultval_stores(int tipc,aplptr ap)
{
	int n;
	if((ap->default_val = (double *)malloc(tipc * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for application default values store\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<tipc;n++)
		ap->default_val[n] = 0.0;
	return(FINISHED);
}

/***************************** SETUP_AND_INIT_INPUT_PARAM_ACTIVITY **************************/

int setup_and_init_input_param_activity(dataptr dz,int tipc)
{
	int n;
	if((dz->is_active = (char   *)malloc((size_t)tipc))==NULL) {
		sprintf(errstr,"setup_and_init_input_param_activity()\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<tipc;n++)
		dz->is_active[n] = (char)0;
	return(FINISHED);
}

/************************* SETUP_MCHZIG_APPLICATION *******************/

int setup_mchzig_application(dataptr dz)
{
	int exit_status;
	aplptr ap;
	if((exit_status = establish_application(dz))<0)		// GLOBAL
		return(FAILED);
	ap = dz->application;
	// SEE parstruct FOR EXPLANATION of next 2 functions
	switch(dz->mode) {
	case(0): exit_status = set_param_data(ap,0	    , 5,5,"ddddi"); break;
	case(1): exit_status = set_param_data(ap,MZIGDATA,5,1,"0000i"); break;
	}
	if(exit_status <0)
		return(FAILED);
	switch(dz->mode) {
	case(0): exit_status = set_vflgs(ap,"smr",3,"ddi","a",1,0,"0");	break;
	case(1): exit_status = set_vflgs(ap,"s"  ,1,"d"  ,"a",1,0,"0");	break;
	}
	if(exit_status <0)
		return(FAILED);
	// set_legal_infile_structure -->
	dz->has_otherfile = FALSE;
	// assign_process_logic -->
	dz->input_data_type = SNDFILES_ONLY;
	dz->process_type	= UNEQUAL_SNDFILE;	
	dz->outfiletype  	= SNDFILE_OUT;
	return application_init(dz);	//GLOBAL
}

/************************* PARSE_INFILE_AND_CHECK_TYPE *******************/

int parse_infile_and_check_type(char **cmdline,dataptr dz)
{
	int exit_status;
	infileptr infile_info;
	if(!sloom) {
		if((infile_info = (infileptr)malloc(sizeof(struct filedata)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for infile structure to test file data.");
			return(MEMORY_ERROR);
		} else if((exit_status = cdparse(cmdline[0],infile_info))<0) {
			sprintf(errstr,"Failed to parse input file %s\n",cmdline[0]);
			return(PROGRAM_ERROR);
		} else if(infile_info->filetype != SNDFILE)  {
			sprintf(errstr,"File %s is not of correct type\n",cmdline[0]);
			return(DATA_ERROR);
		} else if(infile_info->channels != 1)  {
			sprintf(errstr,"Mono files only\n");
			return(DATA_ERROR);
		} else if((exit_status = copy_parse_info_to_main_structure(infile_info,dz))<0) {
			sprintf(errstr,"Failed to copy file parsing information\n");
			return(PROGRAM_ERROR);
		}
		free(infile_info);
	}
	return(FINISHED);
}

/************************* SETUP_MCHZIG_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_mchzig_param_ranges_and_defaults(dataptr dz)
{
	int exit_status;
	aplptr ap = dz->application;
	// set_param_ranges()
	double duration = (double)dz->insams[0]/(double)dz->infile->srate;
	
	ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
	// NB total_input_param_cnt is > 0 !!!
	if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
		return(FAILED);
	// get_param_ranges()
	ap->lo[MZIG_START] 	= 0.0;
	ap->hi[MZIG_START] 	= duration - (MZIG_SPLICELEN * MS_TO_SECS);
	ap->default_val[MZIG_START] = 0.0;
	ap->lo[MZIG_END]   	= ((MZIG_SPLICELEN * 2) + MZIG_MIN_UNSPLICED) * MS_TO_SECS;
	ap->hi[MZIG_END]	= duration;
	ap->default_val[MZIG_END]  = duration;
	ap->lo[MZIG_DUR]	= duration + FLTERR;
	ap->hi[MZIG_DUR]	= BIG_TIME;
	ap->default_val[MZIG_DUR] = duration * 2.0;
	ap->lo[MZIG_MIN]	= ((MZIG_SPLICELEN * 2) + MZIG_MIN_UNSPLICED) * MS_TO_SECS;
	ap->hi[MZIG_MIN]	= duration - (2 * MZIG_SPLICELEN * MS_TO_SECS);
	ap->default_val[MZIG_MIN] = ((MZIG_SPLICELEN * 2) + MZIG_MIN_UNSPLICED) * MS_TO_SECS;
	ap->lo[MZIG_OCHANS] = 2;
	ap->hi[MZIG_OCHANS] = 16;
	ap->default_val[MZIG_OCHANS]	  = 8;
	ap->lo[MZIG_SPLEN]  = MMIN_ZIGSPLICE;
	ap->hi[MZIG_SPLEN]  = MMAX_ZIGSPLICE;
	ap->default_val[MZIG_SPLEN] = MZIG_SPLICELEN;
	if(dz->mode==ZIGZAG_SELF) {
		ap->lo[MZIG_MAX] = ((MZIG_SPLICELEN * 2) + MZIG_MIN_UNSPLICED) * MS_TO_SECS;
		ap->hi[MZIG_MAX] = duration - (2 * MZIG_SPLICELEN * MS_TO_SECS);
		ap->default_val[MZIG_MAX] = min(2.0,duration - (2 * MZIG_SPLICELEN * MS_TO_SECS));
		ap->lo[MZIG_RSEED] = 0.0;
		ap->hi[MZIG_RSEED] = MAXSHORT;
		ap->default_val[MZIG_RSEED] = 0.0;
	}
	if(!sloom)
		put_default_vals_in_all_params(dz);
	return(FINISHED);
}

/********************************* PARSE_SLOOM_DATA *********************************/

int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz)
{
	int exit_status;
	int cnt = 1, infilecnt;
	int filesize, insams, inbrksize;
	double dummy;
	int true_cnt = 0;
	aplptr ap;

	while(cnt<=PRE_CMDLINE_DATACNT) {
		if(cnt > argc) {
			sprintf(errstr,"Insufficient data sent from TK\n");
			return(DATA_ERROR);
		}
		switch(cnt) {
		case(1):	
			if(sscanf(argv[cnt],"%d",&dz->process)!=1) {
				sprintf(errstr,"Cannot read process no. sent from TK\n");
				return(DATA_ERROR);
			}
			break;

		case(2):	
			if(sscanf(argv[cnt],"%d",&dz->mode)!=1) {
				sprintf(errstr,"Cannot read mode no. sent from TK\n");
				return(DATA_ERROR);
			}
			if(dz->mode > 0)
				dz->mode--;
			//setup_particular_application() =
			if((exit_status = setup_mchzig_application(dz))<0)
				return(exit_status);
			ap = dz->application;
			break;

		case(3):	
			if(sscanf(argv[cnt],"%d",&infilecnt)!=1) {
				sprintf(errstr,"Cannot read infilecnt sent from TK\n");
				return(DATA_ERROR);
			}
			if(infilecnt < 1) {
				true_cnt = cnt + 1;
				cnt = PRE_CMDLINE_DATACNT;	/* force exit from loop after assign_file_data_storage */
			}
			if((exit_status = assign_file_data_storage(infilecnt,dz))<0)
				return(exit_status);
			break;
		case(INPUT_FILETYPE+4):	
			if(sscanf(argv[cnt],"%d",&dz->infile->filetype)!=1) {
				sprintf(errstr,"Cannot read filetype sent from TK (%s)\n",argv[cnt]);
				return(DATA_ERROR);
			}
			break;
		case(INPUT_FILESIZE+4):	
			if(sscanf(argv[cnt],"%d",&filesize)!=1) {
				sprintf(errstr,"Cannot read infilesize sent from TK\n");
				return(DATA_ERROR);
			}
			dz->insams[0] = filesize;	
			break;
		case(INPUT_INSAMS+4):	
			if(sscanf(argv[cnt],"%d",&insams)!=1) {
				sprintf(errstr,"Cannot read insams sent from TK\n");
				return(DATA_ERROR);
			}
			dz->insams[0] = insams;	
			break;
		case(INPUT_SRATE+4):	
			if(sscanf(argv[cnt],"%d",&dz->infile->srate)!=1) {
				sprintf(errstr,"Cannot read srate sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_CHANNELS+4):	
			if(sscanf(argv[cnt],"%d",&dz->infile->channels)!=1) {
				sprintf(errstr,"Cannot read channels sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_STYPE+4):	
			if(sscanf(argv[cnt],"%d",&dz->infile->stype)!=1) {
				sprintf(errstr,"Cannot read stype sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_ORIGSTYPE+4):	
			if(sscanf(argv[cnt],"%d",&dz->infile->origstype)!=1) {
				sprintf(errstr,"Cannot read origstype sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_ORIGRATE+4):	
			if(sscanf(argv[cnt],"%d",&dz->infile->origrate)!=1) {
				sprintf(errstr,"Cannot read origrate sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_MLEN+4):	
			if(sscanf(argv[cnt],"%d",&dz->infile->Mlen)!=1) {
				sprintf(errstr,"Cannot read Mlen sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_DFAC+4):	
			if(sscanf(argv[cnt],"%d",&dz->infile->Dfac)!=1) {
				sprintf(errstr,"Cannot read Dfac sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_ORIGCHANS+4):	
			if(sscanf(argv[cnt],"%d",&dz->infile->origchans)!=1) {
				sprintf(errstr,"Cannot read origchans sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_SPECENVCNT+4):	
			if(sscanf(argv[cnt],"%d",&dz->infile->specenvcnt)!=1) {
				sprintf(errstr,"Cannot read specenvcnt sent from TK\n");
				return(DATA_ERROR);
			}
			dz->specenvcnt = dz->infile->specenvcnt;
			break;
		case(INPUT_WANTED+4):	
			if(sscanf(argv[cnt],"%d",&dz->wanted)!=1) {
				sprintf(errstr,"Cannot read wanted sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_WLENGTH+4):	
			if(sscanf(argv[cnt],"%d",&dz->wlength)!=1) {
				sprintf(errstr,"Cannot read wlength sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_OUT_CHANS+4):	
			if(sscanf(argv[cnt],"%d",&dz->out_chans)!=1) {
				sprintf(errstr,"Cannot read out_chans sent from TK\n");
				return(DATA_ERROR);
			}
			break;
			/* RWD these chanegs to samps - tk will have to deal with that! */
		case(INPUT_DESCRIPTOR_BYTES+4):	
			if(sscanf(argv[cnt],"%d",&dz->descriptor_samps)!=1) {
				sprintf(errstr,"Cannot read descriptor_samps sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_IS_TRANSPOS+4):	
			if(sscanf(argv[cnt],"%d",&dz->is_transpos)!=1) {
				sprintf(errstr,"Cannot read is_transpos sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_COULD_BE_TRANSPOS+4):	
			if(sscanf(argv[cnt],"%d",&dz->could_be_transpos)!=1) {
				sprintf(errstr,"Cannot read could_be_transpos sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_COULD_BE_PITCH+4):	
			if(sscanf(argv[cnt],"%d",&dz->could_be_pitch)!=1) {
				sprintf(errstr,"Cannot read could_be_pitch sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_DIFFERENT_SRATES+4):	
			if(sscanf(argv[cnt],"%d",&dz->different_srates)!=1) {
				sprintf(errstr,"Cannot read different_srates sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_DUPLICATE_SNDS+4):	
			if(sscanf(argv[cnt],"%d",&dz->duplicate_snds)!=1) {
				sprintf(errstr,"Cannot read duplicate_snds sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_BRKSIZE+4):	
			if(sscanf(argv[cnt],"%d",&inbrksize)!=1) {
				sprintf(errstr,"Cannot read brksize sent from TK\n");
				return(DATA_ERROR);
			}
			if(inbrksize > 0) {
				switch(dz->input_data_type) {
				case(WORDLIST_ONLY):
					break;
				case(PITCH_AND_PITCH):
				case(PITCH_AND_TRANSPOS):
				case(TRANSPOS_AND_TRANSPOS):
					dz->tempsize = inbrksize;
					break;
				case(BRKFILES_ONLY):
				case(UNRANGED_BRKFILE_ONLY):
				case(DB_BRKFILES_ONLY):
				case(ALL_FILES):
				case(ANY_NUMBER_OF_ANY_FILES):
					if(dz->extrabrkno < 0) {
						sprintf(errstr,"Storage location number for brktable not established by CDP.\n");
						return(DATA_ERROR);
					}
					if(dz->brksize == NULL) {
						sprintf(errstr,"CDP has not established storage space for input brktable.\n");
						return(PROGRAM_ERROR);
					}
					dz->brksize[dz->extrabrkno]	= inbrksize;
					break;
				default:
					sprintf(errstr,"TK sent brktablesize > 0 for input_data_type [%d] not using brktables.\n",
					dz->input_data_type);
					return(PROGRAM_ERROR);
				}
				break;
			}
			break;
		case(INPUT_NUMSIZE+4):	
			if(sscanf(argv[cnt],"%d",&dz->numsize)!=1) {
				sprintf(errstr,"Cannot read numsize sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_LINECNT+4):	
			if(sscanf(argv[cnt],"%d",&dz->linecnt)!=1) {
				sprintf(errstr,"Cannot read linecnt sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_ALL_WORDS+4):	
			if(sscanf(argv[cnt],"%d",&dz->all_words)!=1) {
				sprintf(errstr,"Cannot read all_words sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_ARATE+4):	
			if(sscanf(argv[cnt],"%f",&dz->infile->arate)!=1) {
				sprintf(errstr,"Cannot read arate sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_FRAMETIME+4):	
			if(sscanf(argv[cnt],"%lf",&dummy)!=1) {
				sprintf(errstr,"Cannot read frametime sent from TK\n");
				return(DATA_ERROR);
			}
			dz->frametime = (float)dummy;
			break;
		case(INPUT_WINDOW_SIZE+4):	
			if(sscanf(argv[cnt],"%f",&dz->infile->window_size)!=1) {
				sprintf(errstr,"Cannot read window_size sent from TK\n");
					return(DATA_ERROR);
			}
			break;
		case(INPUT_NYQUIST+4):	
			if(sscanf(argv[cnt],"%lf",&dz->nyquist)!=1) {
				sprintf(errstr,"Cannot read nyquist sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_DURATION+4):	
			if(sscanf(argv[cnt],"%lf",&dz->duration)!=1) {
				sprintf(errstr,"Cannot read duration sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_MINBRK+4):	
			if(sscanf(argv[cnt],"%lf",&dz->minbrk)!=1) {
				sprintf(errstr,"Cannot read minbrk sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_MAXBRK+4):	
			if(sscanf(argv[cnt],"%lf",&dz->maxbrk)!=1) {
				sprintf(errstr,"Cannot read maxbrk sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_MINNUM+4):	
			if(sscanf(argv[cnt],"%lf",&dz->minnum)!=1) {
				sprintf(errstr,"Cannot read minnum sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_MAXNUM+4):	
			if(sscanf(argv[cnt],"%lf",&dz->maxnum)!=1) {
				sprintf(errstr,"Cannot read maxnum sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		default:
			sprintf(errstr,"case switch item missing: parse_sloom_data()\n");
			return(PROGRAM_ERROR);
		}
		cnt++;
	}
	if(cnt!=PRE_CMDLINE_DATACNT+1) {
		sprintf(errstr,"Insufficient pre-cmdline params sent from TK\n");
		return(DATA_ERROR);
	}

	if(true_cnt)
		cnt = true_cnt;
	*cmdlinecnt = 0;		

	while(cnt < argc) {
		if((exit_status = get_tk_cmdline_word(cmdlinecnt,cmdline,argv[cnt]))<0)
			return(exit_status);
		cnt++;
	}
	return(FINISHED);
}

/********************************* GET_TK_CMDLINE_WORD *********************************/

int get_tk_cmdline_word(int *cmdlinecnt,char ***cmdline,char *q)
{
	if(*cmdlinecnt==0) {
		if((*cmdline = (char **)malloc(sizeof(char *)))==NULL)	{
			sprintf(errstr,"INSUFFICIENT MEMORY for TK cmdline array.\n");
			return(MEMORY_ERROR);
		}
	} else {
		if((*cmdline = (char **)realloc(*cmdline,((*cmdlinecnt)+1) * sizeof(char *)))==NULL)	{
			sprintf(errstr,"INSUFFICIENT MEMORY for TK cmdline array.\n");
			return(MEMORY_ERROR);
		}
	}
	if(((*cmdline)[*cmdlinecnt] = (char *)malloc((strlen(q) + 1) * sizeof(char)))==NULL)	{
		sprintf(errstr,"INSUFFICIENT MEMORY for TK cmdline item %d.\n",(*cmdlinecnt)+1);
		return(MEMORY_ERROR);
	}
	strcpy((*cmdline)[*cmdlinecnt],q);
	(*cmdlinecnt)++;
	return(FINISHED);
}


/****************************** ASSIGN_FILE_DATA_STORAGE *********************************/

int assign_file_data_storage(int infilecnt,dataptr dz)
{
	int exit_status;
	int no_sndfile_system_files = FALSE;
	dz->infilecnt = infilecnt;
	if((exit_status = allocate_filespace(dz))<0)
		return(exit_status);
	if(no_sndfile_system_files)
		dz->infilecnt = 0;
	return(FINISHED);
}



/************************* redundant functions: to ensure libs compile OK *******************/

int assign_process_logic(dataptr dz)
{
	return(FINISHED);
}

void set_legal_infile_structure(dataptr dz)
{}

int establish_bufptrs_and_extra_buffers(dataptr dz)
{
	return(FINISHED);
}

int read_special_data(char *str,dataptr dz)	
{
	return(FINISHED);
}

int inner_loop
(int *peakscore,int *descnt,int *in_start_portion,int *least,int *pitchcnt,int windows_in_buf,dataptr dz)
{
	return(FINISHED);
}

int get_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	return(FINISHED);
}

int setup_internal_arrays_and_array_pointers(dataptr dz)
{
	return(FINISHED);
}

/******************************** USAGE1 ********************************/

int usage1(void)
{
	usage2("zag");
	return(USAGE_ONLY);
}

/******************************** DBTOLEVEL ***********************/

double dbtolevel(double val)
{
	int isneg = 0;
	if(flteq(val,0.0))
		return(1.0);
	if(val < 0.0) {
		val = -val;
		isneg = 1;
	}
	val /= 20.0;
	val = pow(10.0,val);
	if(isneg)
		val = 1.0/val;
	return(val);
}	

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	if(!strcmp(prog_identifier_from_cmdline,"zag"))				dz->process = MCHZIG;
	else {
		sprintf(errstr,"Unknown program identification string '%s'\n",prog_identifier_from_cmdline);
		return(USAGE_ONLY);
	}
	return(FINISHED);
}

/****************************** GET_MODE *********************************/

int get_the_mode_from_cmdline(char *str,dataptr dz)
{
	if(sscanf(str,"%d",&dz->mode)!=1) {
		sprintf(errstr,"Cannot read mode of program.\n");
		return(USAGE_ONLY);
	}
	if(dz->mode <= 0 || dz->mode > dz->maxmode) {
		sprintf(errstr,"Program mode value [%d] is out of range [1 - %d].\n",dz->mode,dz->maxmode);
		return(USAGE_ONLY);
	}
	dz->mode--;		/* CHANGE TO INTERNAL REPRESENTATION OF MODE NO */
	return(FINISHED);
}

/******************************** SETUP_AND_INIT_INPUT_BRKTABLE_CONSTANTS ********************************/

int setup_and_init_input_brktable_constants(dataptr dz,int brkcnt)
{	
	int n;
	if((dz->brk      = (double **)malloc(brkcnt * sizeof(double *)))==NULL) {
		sprintf(errstr,"setup_and_init_input_brktable_constants(): 1\n");
		return(MEMORY_ERROR);
	}
	if((dz->brkptr   = (double **)malloc(brkcnt * sizeof(double *)))==NULL) {
		sprintf(errstr,"setup_and_init_input_brktable_constants(): 6\n");
		return(MEMORY_ERROR);
	}
	if((dz->brksize  = (int    *)malloc(brkcnt * sizeof(int)))==NULL) {
		sprintf(errstr,"setup_and_init_input_brktable_constants(): 2\n");
		return(MEMORY_ERROR);
	}
	if((dz->firstval = (double  *)malloc(brkcnt * sizeof(double)))==NULL) {
		sprintf(errstr,"setup_and_init_input_brktable_constants(): 3\n");
		return(MEMORY_ERROR);												  
	}
	if((dz->lastind  = (double  *)malloc(brkcnt * sizeof(double)))==NULL) {
		sprintf(errstr,"setup_and_init_input_brktable_constants(): 4\n");
		return(MEMORY_ERROR);
	}
	if((dz->lastval  = (double  *)malloc(brkcnt * sizeof(double)))==NULL) {
		sprintf(errstr,"setup_and_init_input_brktable_constants(): 5\n");
		return(MEMORY_ERROR);
	}
	if((dz->brkinit  = (int     *)malloc(brkcnt * sizeof(int)))==NULL) {
		sprintf(errstr,"setup_and_init_input_brktable_constants(): 7\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<brkcnt;n++) {
		dz->brk[n]     = NULL;
		dz->brkptr[n]  = NULL;
		dz->brkinit[n] = 0;
		dz->brksize[n] = 0;
	}
	return(FINISHED);
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
	if(!strcmp(str,"zag")) {
	    fprintf(stderr,
		"USAGE: mchzig zag 1 infile outfile start end dur minzig outchans\n"
		"     [-ssplicelen] [-mmaxzig] [-rseed] [-a]\n"
		"OR:    mchzig zag 2 infile outfile timefile outchans [-ssplicelen] [-a]\n\n"
	    "READ SOUNDFILE BACK AND FORTH, PANNING RANDOMLY BETWEEN OUTPUT CHANS\n\n"
		"MODES\n"
		"1: random zigzags: starts at file start, ends at file end.\n"
		"2: zigzagging follows times supplied by user.\n"
		"\n"
		"PARAMETERS\n"
		"start:     together with...\n"
		"end:       define interval in which times zigzag.\n"
		"dur:       is total duration of output sound required.\n"
		"minzig:    is min acceptable time between successive zigzag timepoints.\n"
		"outchans:  number of channels in output file.\n"
		"splicelen: in MILLIsecs (Default 25ms).\n"
		"maxzig:    is max acceptable time between successive zigzag timepoints\n"
		"seed:      number to generate a replicable random sequence. (>0)\n"
		"           entering same number on next program run, generates same sequence.\n"
		"           (Default: (0) random sequence is different every time).\n"
		"timefile:  text file containing sequence of times to zigzag between.\n"
		"           Each step-between-times must be > (3 * splicelen).\n"
		"           zigsteps moving in the same (time-)direction will be concatenated.\n"
		"-a         Avoid zigs between adjacent channels (5 or more outchans only).\n");
	} else
		fprintf(stdout,"Unknown option '%s'\n",str);
	return(USAGE_ONLY);
}

int usage3(char *str1,char *str2)
{
	fprintf(stderr,"Insufficient parameters on command line.\n");
	return(USAGE_ONLY);
}

/********************************** CHECK_ZIGZAG_CONSISTENCY **********************************/

int check_zigzag_consistency(dataptr dz)
{
	double diff;
	if(dz->mode==ZIGZAG_SELF) {
		if(dz->infile->channels != 1) {
			sprintf(errstr,"mchzig only works with mono files.\n");
			return(DATA_ERROR);
		}
		if(dz->param[MZIG_MAX] <= dz->param[MZIG_MIN]) {
			sprintf(errstr,"maximum zig duration <= minimum zig duration\n");
			return(DATA_ERROR);
		}
		if(dz->param[MZIG_MIN] < (((dz->param[MZIG_SPLEN] * 2) + ZIG_MIN_UNSPLICED) * MS_TO_SECS)) {
			sprintf(errstr,"minimum ziglength must be > %.3lf: cannot proceed\n",
			((dz->param[MZIG_SPLEN] * 2) + ZIG_MIN_UNSPLICED) * MS_TO_SECS);
			return(DATA_ERROR);
		}
		diff = dz->param[MZIG_END] - dz->param[MZIG_START];
		if(diff<=0.0) {
			sprintf(errstr,"Zig start and end times incompatible.\n");
			return(DATA_ERROR);
		}
		if(round(diff/dz->param[MZIG_MIN])<1) {
			sprintf(errstr,"Zigzagging sector too short for zig-zag minlength specified.\n");
			return(DATA_ERROR);
		}
	}
	if(dz->vflag[0] && (dz->iparam[MZIG_OCHANS] < 5)) {
		sprintf(errstr,"Can't avoid adjacent chans + use all chans, with < 5 outchans.\n");
		return(DATA_ERROR);
	}
	dz->outchans = dz->iparam[MZIG_OCHANS];
	return FINISHED;
}

/********************************** ZIGZAG_PREPROCESS **********************************/

int mzigzag_preprocess(int *maxzig,dataptr dz)
{
	int exit_status;
	int n = 0;
	if(dz->mode==ZIGZAG_SELF)
		initialise_random_sequence(IS_ZIG_RSEED,MZIG_RSEED,dz);
	if((exit_status = setup_zigzag_splice(MZIG_SPLEN,dz))<0)
		return(exit_status);
	if(dz->insams[0] <= dz->iparam[MZIG_SPLSAMPS] * 2) {
		sprintf(errstr,"Infile too short for splices.\n");
		return(DATA_ERROR);
	}
	if(dz->mode == ZIGZAG_SELF) {
		if((exit_status = generate_zigzag_table(dz))<0)
			return(exit_status);
	}
	if((exit_status = sort_zigs(maxzig,dz))<0)
		return(exit_status);

	if(sloom) {
		dz->tempsize = 0L;
		for(n=1;n<dz->itemcnt;n++)
			/*RWD treat tempszie as in samps */
			dz->tempsize += abs(dz->lparray[ZIGZAG_TIMES][n] - dz->lparray[ZIGZAG_TIMES][n-1]);
	}
	if((dz->iparray[MZIG_PERMCH] = (int *)malloc(dz->iparam[MZIG_OCHANS] * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to make channel perm array.\n");
		return(MEMORY_ERROR);
	}
	return(FINISHED);
}

/*********************** SETUP_ZIGZAG_SPLICE ***************************/

int setup_zigzag_splice(int paramno,dataptr dz)
{
	int exit_status;
	if((exit_status = make_zigsplice(paramno,dz))<0)
		return(exit_status);
	return create_zigzag_splicebuffer(dz);
}

/*********************** MAKE_ZIGSPLICE ***************************/

int make_zigsplice(int paramno,dataptr dz)
{
	int n;
	dz->iparam[MZIG_SPLICECNT] = (int)round(dz->param[paramno] * MS_TO_SECS * dz->infile->srate);
	dz->iparam[MZIG_SPLSAMPS]  = dz->iparam[MZIG_SPLICECNT];

	if((dz->parray[ZIGZAG_SPLICE] = (double *)malloc(dz->iparam[MZIG_SPLICECNT] * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to make splicer buffer.\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<dz->iparam[MZIG_SPLICECNT];n++)
		dz->parray[ZIGZAG_SPLICE][n] = (double)n/(double)dz->iparam[MZIG_SPLICECNT];
	return(FINISHED);
}

/*********************** CREATE_ZIGZAG_SPLICEBUFFER ***************************/

int create_zigzag_splicebuffer(dataptr dz)
{
	if(dz->extrabuf == (float **)0) {
		sprintf(errstr,"extrabuf has not been created: create_zigzag_splicebuffer()\n");
		return(PROGRAM_ERROR);
	}

    if((dz->extrabuf[ZIGZAG_SPLBUF] = (float *)malloc(sizeof(float) * dz->iparam[MZIG_SPLSAMPS]))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to make splicing buffer.\n");
		return(MEMORY_ERROR);
	}
    memset((char *)dz->extrabuf[ZIGZAG_SPLBUF],0,sizeof(float) * dz->iparam[MZIG_SPLSAMPS]);
	return(FINISHED);
}

/***************************** GENERATE_ZIGZAG_TABLE ***************************/

int generate_zigzag_table(dataptr dz)
{
	int    OK;
	int   arraysize = BIGARRAY;
	double infiledur = (double)(dz->insams[0]/dz->infile->channels)/(double)(dz->infile->srate);
	double totaltime = dz->param[MZIG_START];
	double goaltime  = dz->param[MZIG_DUR] - (infiledur - dz->param[MZIG_END]);
	double diff, randlen = 0.0, here  = dz->param[MZIG_START];
	int direction = FORWARDS;
	if((dz->lparray[ZIGZAG_TIMES] = (int *)malloc(arraysize * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store times.\n");
		return(MEMORY_ERROR);
	}
	dz->lparray[ZIGZAG_TIMES][0] = 0;
	dz->itemcnt = 1;
	do {
		OK = TRUE;
		switch(direction) {
		case(FORWARDS):
			diff = min(dz->param[MZIG_MAX],dz->param[MZIG_END] - here);
			if(diff<=dz->param[MZIG_MIN])
				OK = FALSE;
			else {
				randlen = drand48();						/* generate segment length at random */
				randlen *= (diff - dz->param[MZIG_MIN]);	/* scale it to range required */
				randlen += dz->param[MZIG_MIN];				/* and add mindur */
				here = (here + randlen);
			}
			break;
		case(BACKWARDS):
			diff = min(dz->param[MZIG_MAX],here - dz->param[MZIG_START]);
			if(diff<=dz->param[MZIG_MIN])
				OK = FALSE;
			else {
				randlen = drand48();						/* generate segment length at random */
				randlen *= (diff - dz->param[MZIG_MIN]);	/* scale it to range required */
				randlen += dz->param[MZIG_MIN];			/* and add mindur */
				here = (here - randlen);
			}
			break;
		}
		direction = -direction; /* invert time-direction */
		if(!OK)
			continue;
		totaltime += randlen;
		dz->lparray[ZIGZAG_TIMES][dz->itemcnt] = round(here * (double)dz->infile->srate) * dz->infile->channels;
		if(++dz->itemcnt >= arraysize) {
			arraysize += BIGARRAY;
			if((dz->lparray[ZIGZAG_TIMES] = 
			(int *)realloc((char *)dz->lparray[ZIGZAG_TIMES],arraysize*sizeof(int)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY to reallocate times store.\n");
				return(MEMORY_ERROR);
			}
		}
	} while(totaltime<goaltime);
	dz->lparray[ZIGZAG_TIMES][dz->itemcnt] = dz->insams[0];
	dz->itemcnt++;
	if(dz->itemcnt < arraysize) {
		if((dz->lparray[ZIGZAG_TIMES] = 
		(int *)realloc((char *)dz->lparray[ZIGZAG_TIMES],dz->itemcnt*sizeof(int)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to reallocate times store.\n");
			return(MEMORY_ERROR);
		}
	}
	return(FINISHED);
}

/****************************** SORT_ZIGS ************************************/

int sort_zigs(int *maxzig,dataptr dz)
{
	int exit_status;
	int zigsize;
	int safety = round(ZIG_MIN_UNSPLICED * MS_TO_SECS * (double)dz->infile->srate) * dz->infile->channels;
	int cnt = 0, firstime = TRUE, direction, previous_direction = FORWARDS;
	int *this_zigtime = dz->lparray[ZIGZAG_TIMES];
	int *next_zigtime = dz->lparray[ZIGZAG_TIMES] + 1;
	int *ziglistend = dz->lparray[ZIGZAG_TIMES] + dz->itemcnt;
	int minzig = (dz->iparam[MZIG_SPLSAMPS] * 2) + safety;
    int file_samplen = dz->insams[0];
	double convert_to_time = 1.0/(double)dz->infile->channels/(double)dz->infile->srate;
	if(*this_zigtime < 0 || *this_zigtime > file_samplen) {
		sprintf(errstr,"Invalid 1st zigtime %lf\n",(*this_zigtime) * convert_to_time);
		return(DATA_ERROR);
	}
	if(*(ziglistend-1) >= file_samplen) {
		*(ziglistend-1) = file_samplen;
		dz->iparam[ZIG_RUNSTOEND] = 1;
	} else
		dz->iparam[ZIG_RUNSTOEND] = 0;
	while(next_zigtime < ziglistend - dz->iparam[ZIG_RUNSTOEND]) {
		if(*next_zigtime < 0 || *next_zigtime > file_samplen) {
			sprintf(errstr,"Invalid zigtime %lf\n",(*next_zigtime) * convert_to_time);
			return(DATA_ERROR);
		}
		while((zigsize = abs(*next_zigtime - *this_zigtime)) < minzig) {
			if(++next_zigtime == ziglistend - dz->iparam[ZIG_RUNSTOEND])
				break;
		}
		if(next_zigtime - this_zigtime > 1) {
			if(dz->mode == ZIGZAG_USER) {
				sprintf(errstr,"Some zigs too short to use with specified splicelen.\n");
				return(DATA_ERROR);
			}
			eliminate_n_steps(this_zigtime,next_zigtime,&ziglistend,&cnt,dz);
			next_zigtime = this_zigtime + 1;
		}
		if(*next_zigtime > *this_zigtime)
			direction = FORWARDS;
		else
			direction = BACKWARDS;
		if(!firstime && (direction == previous_direction)) {
			if((exit_status = eliminate_step(next_zigtime,&ziglistend,&cnt,dz))<0)
				return(exit_status);
			continue;
		}
		previous_direction = direction;
		firstime = FALSE;
		this_zigtime++;
		next_zigtime++;
	}
	if(cnt>0) {
		fprintf(stdout,"WARNING: %d steps eliminated (too small relative to spliclength\n",cnt);
//TW : CAN'T SPLIT LINES SENT TO SLOOM - 'WARNING' is a flag to SLOOM - possibly my error, since updated
		fprintf(stdout,"WARNING: or moving in same direction as previous step)\n");
		fflush(stdout);
		if((dz->lparray[ZIGZAG_TIMES] =
		(int *)realloc((char *)dz->lparray[ZIGZAG_TIMES],dz->itemcnt * sizeof(int)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to reallocate times store.\n");
			return(MEMORY_ERROR);
		}
	}
	if(dz->iparam[ZIG_RUNSTOEND]) {
		if(*(ziglistend-1) - *(ziglistend-2) < dz->iparam[MZIG_SPLSAMPS] + safety) {
			sprintf(errstr,"Final zig too short for splicelen.\n");
			return(GOAL_FAILED);
		}
	}
	get_bufsize_needed(maxzig,dz);
	return FINISHED;
}

/**************************** ELIMINATE_N_STEPS ***************************/

int eliminate_n_steps(int *this_zigtime,int *next_zigtime,int **ziglistend,int *cnt,dataptr dz)
{
	int *here  = this_zigtime + 1;
	int *there = next_zigtime;
	int elimination_cnt = next_zigtime - this_zigtime - 1;
	while(there < *ziglistend) {
		*here = *there;
		here++;
		there++;
	}
	if((dz->itemcnt -= elimination_cnt) < 2) {
		sprintf(errstr,"All zigsteps either too small for splices: or moving in same direction.\n");
		return(DATA_ERROR);
	}
	*ziglistend -= elimination_cnt;
	(*cnt) += elimination_cnt;
	return(FINISHED);
}

/***************************** ELIMINATE_STEP ***************************/

int eliminate_step(int *next_zigtime,int **ziglistend,int *cnt,dataptr dz)
{
	int *here = next_zigtime;

	while(here < *ziglistend) {
		*(here-1) = *here;
		here++;
	}
	if(--dz->itemcnt < 2) {
		sprintf(errstr,"All zigsteps either too small for splices: or moving in same direction.\n");
		return(DATA_ERROR);
	}
	(*ziglistend)--;
	(*cnt)++;
	return(FINISHED);
}

/***************************** INSERT_EXTRA_ZIG ***************************/

int insert_extra_zig(int direction,int **this_zigtime,int **next_zigtime,int **ziglistend,int len,dataptr dz)
{
	int *here;
	int zthis = *this_zigtime - dz->lparray[ZIGZAG_TIMES];
	int znext = *next_zigtime - dz->lparray[ZIGZAG_TIMES];
	dz->itemcnt++;
	if((dz->lparray[ZIGZAG_TIMES] =
	(int *)realloc((char *)dz->lparray[ZIGZAG_TIMES],dz->itemcnt * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to reallocate times store.\n");
		return(MEMORY_ERROR);
	}
	*this_zigtime = dz->lparray[ZIGZAG_TIMES] + zthis;
	*next_zigtime = dz->lparray[ZIGZAG_TIMES] + znext;
	*ziglistend  =  dz->lparray[ZIGZAG_TIMES] + dz->itemcnt;
	here = *ziglistend - 1;
	while(here > *next_zigtime) {
		*here = *(here-1);
		here--;
	}
	switch(direction) {
	case(FORWARDS):
		*here = **this_zigtime + len;		
		break;
	case(BACKWARDS):	
		*here = **this_zigtime - len;		
		break;
	}
	if(*here < 0.0 || (int)*here > dz->insams[0]) {
		sprintf(errstr,"Error in logic of sample arithmetic: insert_extra_zig()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/**************************** GET_BUFSIZE_NEEDED ***************************/

void get_bufsize_needed(int *maxzig,dataptr dz)
{
	int *zigtime = dz->lparray[ZIGZAG_TIMES], n, ziglen;
	*maxzig = 0;
	for(n = 1; n < dz->itemcnt;n++) {
		ziglen = abs(zigtime[n] - zigtime[n-1]);
		if(ziglen > *maxzig)
			*maxzig = ziglen;
	}
}

/***************************** ZIGZAG **********************************/

int zigzag(dataptr dz)
{
	int exit_status;
	int *perm = dz->iparray[MZIG_PERMCH];
	int *thisstart, *lastend = dz->lparray[ZIGZAG_TIMES], minsamp;
	int permno, chancnt = dz->iparam[MZIG_OCHANS],startchan,endchan;
    int *ziglistend = dz->lparray[ZIGZAG_TIMES] + dz->itemcnt;
	int outbuf_space = dz->buflen, samps_left;
	int obufno, obufendno;
	obufno = 2;					//	The last chancnt bufs are the multichan outbuf
	obufendno = 2 + chancnt;	//	which starts at dz->sampbuf[2];
								//	dz->sampbuf[2+chancnt] marks the outbuf end
	thisstart = lastend;
	thisstart++;
	if((exit_status = find_zzchunk(thisstart,ziglistend,&minsamp,dz))!=CONTINUE) {
		if(exit_status == FINISHED)
			exit_status = GOAL_FAILED;
		sprintf(errstr,"WARNING: No valid zigzag found\n");
		return(exit_status);
	}
	lastend = thisstart;
	thisstart++;
	if(sndseekEx(dz->ifd[0],minsamp,0)<0) {		
		sprintf(errstr,"Seek Anomaly 1()\n");
		return(PROGRAM_ERROR);
	}
	if((exit_status = read_samps(dz->sampbuf[0],dz))<0)
		return(exit_status);
	endchan = -1;
	do {
		permute_chans(chancnt,dz);
	} while(adjacence(endchan,dz));
	startchan = perm[0];
	endchan   = perm[1];
	permno = 2;
	while(thisstart < ziglistend-1) {
		if((exit_status = zig_or_zag(thisstart,lastend,&outbuf_space,obufno,obufendno,startchan,endchan,dz))<0)
			return(exit_status);
		startchan = endchan;
		endchan = perm[permno];
		permno++;
		if(permno >= dz->iparam[MZIG_OCHANS]) {
			do {
				permute_chans(dz->iparam[MZIG_OCHANS],dz);
			} while((perm[0] == endchan) || adjacence(endchan,dz));
			permno = 0;
		}
		if((exit_status = find_zzchunk(thisstart,ziglistend,&minsamp,dz))!=CONTINUE) {
			if(exit_status==FINISHED)
				break;
			else
				return(exit_status);
		}
		if(sndseekEx(dz->ifd[0],minsamp,0)<0) {		
			sprintf(errstr,"Seek Anomaly 2()\n");
			return(PROGRAM_ERROR);
		}
		if((exit_status = read_samps(dz->sampbuf[0],dz))<0)
			return(exit_status);
		lastend = thisstart;
		thisstart++;
	}
	samps_left = dz->sbufptr[obufno] - dz->sampbuf[obufno];
	if(samps_left > 0)
		return write_samps(dz->sampbuf[obufno],samps_left,dz);
	return FINISHED;
}

/************************* DO_ZIGZAGS ************************************/

int zig_or_zag(int *thisstart,int *lastend,int *outbuf_space,int obufno,int obufendno,int startchan,int endchan,dataptr dz)
{
	int exit_status;
	int *here  = thisstart;
	int *next  = thisstart + 1;
	int  chancnt = dz->iparam[MZIG_OCHANS];
	int incnt  = abs(*next - *here);
	float *write_limit;
	int wraplen = dz->iparam[MZIG_SPLICECNT] * chancnt;
	if(*here < *next) {
		// direction = ZIG;
		memcpy((char *)dz->sampbuf[1],(char *)dz->sampbuf[0],incnt * sizeof(float));
	} else {
		//direction = ZAG;
		if((exit_status = reverse_it(incnt,dz))<0)
			return(exit_status);
	}
	do_start_splice(dz->sampbuf[1],dz);
	do_end_splice(dz->sampbuf[1],incnt,dz);
	write_limit = dz->sampbuf[obufendno] + wraplen;	//	Allows for wrap-around-area at end of obuf
	if((exit_status = copy_with_spatial_scatter(dz->sbufptr[obufno],write_limit,startchan,endchan,incnt,chancnt,dz))<0)
		return(exit_status);
	dz->sbufptr[obufno] -= wraplen;
	return(FINISHED);
}

/*************************** FIND_ZZCHUNK ************************************/

int find_zzchunk(int *thisstart,int *ziglistend, int *minsamp, dataptr dz)
{
	int *p, min_samp, max_samp;
	p = thisstart;
							/* WE'RE SEARCHING FOR THE earliest and latest times among successive zigtimes */
	min_samp = *p;								/* so preset both of these to the current first zigtime */
	max_samp = *p;
	p++;
	if(p == ziglistend)
		return FINISHED;
	if(*p < min_samp)						/* if this zigtime earlier than any encountered so far */
		min_samp = *p;
	else if(*p > max_samp)					/* if this zigtime later than any encountered so far */
		max_samp = *p;
	if(max_samp - min_samp < 0) {
		sprintf(errstr,"Anomaly in find_zzchunk()\n");
		return(PROGRAM_ERROR);
	}
	*minsamp = min_samp;
	return(CONTINUE);
}

/********************** REVERSE_IT ***************************/

int reverse_it(int incnt,dataptr dz)
{
	int n;
	float *s0ptr = dz->sampbuf[0] + incnt - 1;
	float *s1ptr = dz->sampbuf[1];
	for(n=0;n<incnt;n++)	
		*s1ptr++ = *s0ptr--;
	return(FINISHED);
}

/********************** COPY_WITH_SPATIAL_SCATTER ***************************/

int copy_with_spatial_scatter(float *outptr,float *tobufend,int startchan,int endchan,int sampcnt,int chancnt,dataptr dz)
{
	int exit_status;
	int panpos, n;
	double pos, leftgain, rightgain;
	int outchans = dz->iparam[MZIG_OCHANS];
	int obufsamps = dz->buflen * chancnt;					 //	Samples in a full outbuffer
	int  wrapsamps = dz->iparam[MZIG_SPLICECNT] * outchans; //	Samples in wraparound seg after obufend
	float *toptr = outptr;									 // Location of next write in obuffer
	float *fromptr = dz->sampbuf[1];						 // Location of first read in input buffer
	float *obuf_wrap = dz->sampbuf[2 + outchans];			 //	Start of wrap-around segment
	if(sampcnt < 0) {
		sprintf(errstr,"Attempted to copy -ve number of samps: copy_with_spatial_scatter()\n");
		return(PROGRAM_ERROR);
	}
	panpos = 0;
	for(n = 0;n<sampcnt;n++) {
		if(toptr >= tobufend) {
			if((exit_status = write_samps(dz->sampbuf[2],obufsamps,dz))<0)
				return exit_status;
			memset((char *)dz->sampbuf[2],0,obufsamps * sizeof(float));					// clear the outbuf,
			memcpy((char *)dz->sampbuf[2],(char *)obuf_wrap,wrapsamps * sizeof(float)); // wrap_around any buffer overflow,
			memset((char *)obuf_wrap,0,wrapsamps * sizeof(float));						// THEN clear the wrap-around segment.
			toptr -= obufsamps;															// Baktrak output pointer by a full buflen.
		}
		pos = (double)panpos/(double)sampcnt;	//	Fraction of time-distance into zig
		pos = (pos * 2.0) - 1.0;				//	Change to -1 to + 1 frame
		pancalc(pos,&leftgain,&rightgain);
		toptr[startchan] = (float)(toptr[startchan] + (fromptr[n] * leftgain));
		toptr[endchan]   = (float)(toptr[endchan]   + (fromptr[n] * rightgain));
		toptr += outchans;
		panpos++;
	}
	dz->sbufptr[2] = toptr;
	return FINISHED;
}

/********************** DO_START_SPLICE ***************************/

void do_start_splice(float *buf,dataptr dz)
{
	int  n;
	for(n=0;n < dz->iparam[MZIG_SPLICECNT];n++)
		buf[n] = (float)(buf[n] * dz->parray[ZIGZAG_SPLICE][n]);
}

/********************** DO_END_SPLICE ***************************/

void do_end_splice(float *buf,int incnt,dataptr dz)
{
	int  n, m;
	int splicecnt = dz->iparam[MZIG_SPLICECNT];
	buf += incnt - splicecnt;
	for(n=0,m = splicecnt - 1;n < splicecnt;n++,m--)
		buf[n] = (float)(buf[n] * dz->parray[ZIGZAG_SPLICE][m]);
}

/*************************** PERMUTE_CHANS ***************************/

void permute_chans(int outchans,dataptr dz)
{   
	int n, t;
	for(n=0;n<outchans;n++) {
		t = (int)(drand48() * (double)(n+1));	 /* TRUNCATE */
		if(t==n)
			prefixch(n,outchans,dz);
		else
			insertch(n,t,outchans,dz);
	}
}

/****************************** INSERTCH ****************************/

void insertch(int n,int t,int outchans,dataptr dz)
{   
	shuflupch(t+1,outchans,dz);
	dz->iparray[MZIG_PERMCH][t+1] = n;
}

/****************************** PREFIX ****************************/

void prefixch(int n,int outchans,dataptr dz)
{   
	shuflupch(0,outchans,dz);
	dz->iparray[MZIG_PERMCH][0] = n;
}

/****************************** SHUFLUPCH ****************************/

void shuflupch(int k,int outchans,dataptr dz)
{   
	int n;
	for(n = outchans - 1; n > k; n--)
		dz->iparray[MZIG_PERMCH][n] = dz->iparray[MZIG_PERMCH][n-1];
}

/***************************** SETUP_INTERNAL_ARRAYS_AND_ARRAY_POINTERS **************************/

int mz_setup_internal_arrays_and_array_pointers(dataptr dz)
{
	int n;		 
	dz->array_cnt = 1;
	dz->iarray_cnt = 1;
	dz->larray_cnt = 1;
	if((dz->parray  = (double **)malloc(dz->array_cnt * sizeof(double *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for internal double arrays.\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<dz->array_cnt;n++)
		dz->parray[n] = NULL;

	if((dz->iparray = (int     **)malloc(dz->iarray_cnt * sizeof(int *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for internal int arrays.\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<dz->iarray_cnt;n++)
		dz->iparray[n] = NULL;

	if((dz->lparray = (int    **)malloc(dz->larray_cnt * sizeof(int *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for internal long arrays.\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<dz->larray_cnt;n++)
		dz->lparray[n] = NULL;

	return(FINISHED);
}

/************************************ PANCALC *******************************/

void pancalc(double position,double *leftgain,double *rightgain)
{
	int signal_to_left = 0;
	double temp;
	double relpos;
	double reldist, invsquare;

	if(position < 0.0)
		signal_to_left = 1;		/* signal on left */

	if(position < 0) 
		relpos = -position;
	else 
		relpos = position;
	if(relpos <= 1.0){		/* between the speakers */
		temp = 1.0 + (relpos * relpos);
		reldist = ROOT2 / sqrt(temp);
		temp = (position + 1.0) / 2.0;
		*rightgain = temp * reldist;
		*leftgain = (1.0 - temp ) * reldist;
	} else {				/* outside the speakers */
		temp = (relpos * relpos) + 1.0;
		reldist  = sqrt(temp) / ROOT2;   /* relative distance to source */
		invsquare = 1.0 / (reldist * reldist);
		if(signal_to_left){
			*leftgain = invsquare;
			*rightgain = 0.0;
		} else {   /* SIGNAL_TO_RIGHT */
			*rightgain = invsquare;
			*leftgain = 0;
		}
	}
}

/************************ HANDLE_THE_SPECIAL_DATA *********************/

int handle_the_special_data(int *cmdlinecnt,char ***cmdline,dataptr dz)
{
	int exit_status;
	aplptr ap = dz->application;

	if(ap->special_data) {
		if(!sloom) {
			if(*cmdlinecnt <= 0) {
				sprintf(errstr,"Insufficient parameters on command line.\n");
				return(USAGE_ONLY);
			}
		}
		if((exit_status = setup_the_special_data_ranges
		(dz->mode,dz->infile->srate,dz->duration,dz->nyquist,dz->wlength,dz->infile->channels,ap))<0)
			return(exit_status);

		if((exit_status = read_ziginfo((*cmdline)[0],dz))<0)
			return(exit_status);
		(*cmdline)++;		
		(*cmdlinecnt)--;
	}
	return(FINISHED);
}


/************************ SETUP_SPECIAL_DATA_RANGES *********************/

int setup_the_special_data_ranges(int mode,int srate,double duration,double nyquist,int wlength,int channels,aplptr ap)
{
	ap->data_in_file_only 	= TRUE;
	ap->special_range 		= TRUE;
	ap->min_special 		= MZIG_SPLICELEN * MS_TO_SECS * 3;
	ap->max_special 		= duration;
	return(FINISHED);
}

/***************************** READ_ZIGINFO ***************************/

int read_ziginfo(char *filename,dataptr dz)
{
	FILE   *fp;
	double  p;
	char  temp[200], *q;
	int  arraysize = BIGARRAY, sampcnt;
	int maxlong = /*getmaxlong()*/0x7fffffff;
	dz->itemcnt = 0;
	if((fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,"Can't open text file %s to read.\n",filename);
		return(DATA_ERROR);
	}
	if((dz->lparray[ZIGZAG_TIMES] = (int *)malloc(arraysize * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store zigzag times.\n");
		return(MEMORY_ERROR);
	}
	while(fgets(temp,200,fp)!=NULL) {
		q = temp;
		while(get_float_from_within_string(&q,&p)){
			if(p < 0.0) {
				sprintf(errstr,"Invalid zigzag time, less than zero\n");
				return(DATA_ERROR);
			}
			if((sampcnt = round(p * (double)dz->infile->srate) * dz->infile->channels) < 0) /* overflow */
				dz->lparray[ZIGZAG_TIMES][dz->itemcnt] = (maxlong/dz->infile->channels) * dz->infile->channels;
			else
				dz->lparray[ZIGZAG_TIMES][dz->itemcnt] = sampcnt;
 			if(++dz->itemcnt >= arraysize) {
				arraysize += BIGARRAY;
				if((dz->lparray[ZIGZAG_TIMES] = 
				(int *)realloc((char *)dz->lparray[ZIGZAG_TIMES],arraysize*sizeof(int)))==NULL) {
					sprintf(errstr,"INSUFFICIENT MEMORY to reallocate zigzag times.\n");
					return(MEMORY_ERROR);
				}
			}
		}
	}	    
	if(!dz->itemcnt) {
		sprintf(errstr,"No data in file %s\n",filename);
		return(DATA_ERROR);
	}
	if((dz->lparray[ZIGZAG_TIMES] =
	(int *)realloc((char *)dz->lparray[ZIGZAG_TIMES],dz->itemcnt * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to reallocate zigzag times.\n");
		return(MEMORY_ERROR);
	}
	if(fclose(fp)<0) {
		fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
		fflush(stdout);
	}
	if(dz->itemcnt < 2) {
		sprintf(errstr,"Not enough zig information found in file %s\n",filename);
		return(DATA_ERROR);
	}
	return(FINISHED);
}

int set_legal_internalparam_structure(int process,int mode,aplptr ap)
{
	int exit_status;
	switch(mode) {
		case(0): exit_status = set_internalparam_data("iii"  ,ap);	break;
		case(1): exit_status = set_internalparam_data("diiii",ap);	break;
		default:
			sprintf(errstr,"Unknown mode for zigzag: set_legal_internalparam_structure()\n");
			return(PROGRAM_ERROR);
	}
	return(exit_status);		
}

/*************************** CREATE_MZIG_SNDBUFS **************************/

/* 2009 MULTICHANNEL */

int create_mzig_sndbufs(int maxzig,dataptr dz)
{
	int n;
	int bigbufsize, totalbufsize;
	int framesize, safety = 20;
	int outchans = dz->iparam[MZIG_OCHANS];
	int wraplen = dz->iparam[MZIG_SPLICECNT] * outchans;
	framesize = F_SECSIZE * dz->infile->channels;
	if(dz->sbufptr == 0 || dz->sampbuf==0) {
		sprintf(errstr,"buffer pointers not allocated: create_sndbufs()\n");
		return(PROGRAM_ERROR);
	}
	bigbufsize = ((maxzig + safety)* dz->bufcnt) * sizeof(float);
	if(bigbufsize <= 0) {
		sprintf(errstr,"maximum zig too large for available moeory\n");
		return(DATA_ERROR);
	}
	bigbufsize /= dz->bufcnt;
	dz->buflen = bigbufsize / sizeof(float);	
	totalbufsize = (bigbufsize  * dz->bufcnt);
	totalbufsize += (wraplen * sizeof(float)) + safety;						//	NB This creates a splicelen(+) segment of buffer beyond outbuf end
	if((dz->bigbuf = (float *)malloc(totalbufsize)) == NULL) {				//	Which is used to wrap-around samples, into outbuf, at write_samps
		sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");	//	This is needed because there is a splicelen backtrack
		return(PROGRAM_ERROR);												//	before every zig write to the outbuf.
	}
	for(n=0;n<dz->bufcnt;n++)
		dz->sbufptr[n] = dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
	dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
	return(FINISHED);
}

/*************************** ADJACENCE **************************/

int adjacence(int endchan,dataptr dz)
{
	int *perm = dz->iparray[MZIG_PERMCH];
	int i, j, ochans = dz->iparam[MZIG_OCHANS];
	int lastchan = ochans - 1;
	if(dz->vflag[0] == 0)
		return 0;
	if(endchan >= 0) {
		if ((perm[0] == 0) && (endchan == lastchan))
			return 1;
		else if((endchan == 0) && (perm[0] == lastchan))
			return 1;
		else if(perm[0] - endchan == 1)
			return 1;
		else if(endchan - perm[0] == 1)
			return 1;
	}
	for(i=0;i < ochans;i++) {
		j = (i + 1) % ochans;
		if ((perm[i] == 0) && (perm[j] == lastchan))
			return 1;
		else if((perm[j] == 0) && (perm[i] == lastchan))
			return 1;
		else if(perm[j] - perm[i] == 1)
			return 1;
		else if(perm[i] - perm[j] == 1)
			return 1;
	}
	return 0;
}
