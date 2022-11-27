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
#include <limits.h>
#include <flags.h>
#include <extdcon.h>

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

//CDP LIB REPLACEMENTS
static int setup_mchiter_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_mchiter_param_ranges_and_defaults(dataptr dz);
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
static int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz);
static int get_the_mode_from_cmdline(char *str,dataptr dz);
static int setup_and_init_input_brktable_constants(dataptr dz,int brkcnt);

static int  read_the_input_snd(dataptr dz);
static void scale_input(dataptr dz);
static int get_next_writestart(int write_start,dataptr dz);

static int iterate(int cnt,int pass,double *gain,double *pshift,
				int write_end,int local_write_start,int inmsampsize,double level,double *maxsamp,int pstep,int iterating,int thischan,dataptr dz);
static int iter(int cnt,int passno, double *gain,int local_write_start,int inmsampsize,double level,double *maxsamp,int iterating,int thischan,dataptr dz);
static int iter_shift_interp(int cnt,int passno, double *gain,double *pshift,int local_write_start,int inmsampsize,double level,double *maxsamp,
				int pstep,int iterating,int thischan,dataptr dz);
static int fixa_iter(int cnt,int passno,double *gain,int local_write_start,int inmsampsize,double level,double *maxsamp,int iterating,int thischan,dataptr dz);
static int fixa_iter_shift_interp(int cnt,int passno,double *gain,double *pshift,int local_write_start,int inmsampsize,double level,
				double *maxsamp,int pstep,int iterating,int thischan,dataptr dz);
static double get_gain(dataptr dz);
static double get_pshift(dataptr dz);
static int get_maxvalue_of_rand(double *maxrand,dataptr dz);
static int get_maxvalue_of_pscat(double *maxpscat,dataptr dz);
static int get_minvalue_of_delay(double *mindelay,dataptr dz);
static void set_default_gain(int mindelay_samps,dataptr dz);
static void set_default_delays(dataptr dz);
static void reverse_fadevals(dataptr dz);
static void setup_iter_process_type(int is_unity_gain,dataptr dz);

static int create_mchiterbufs(double maxpscat,dataptr dz);

static void shuflupch(int k,int outchans,dataptr dz);
static void prefixch(int n,int outchans,dataptr dz);
static void insertch(int n,int t,int outchans,dataptr dz);
static void permute_chans(int outchans,dataptr dz);

static int mi_setup_internal_arrays_and_array_pointers(dataptr dz);
static int do_iteration(dataptr dz);

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
	int exit_status;
	dataptr dz = NULL;
	char **cmdline;
	int  cmdlinecnt;
	int n;
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
		if((exit_status = setup_mchiter_application(dz))<0) {
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
	if((exit_status = setup_mchiter_param_ranges_and_defaults(dz))<0) {
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
//	handle_special_data()		redundant
 
	if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {		// CDP LIB
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
//	check_param_validity_and_consistency()	redundant
	is_launched = TRUE;
	dz->bufcnt = 3;
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

	if((exit_status = open_the_outfile(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	//param_preprocess  .... includes ... create_sndbufs
	if((exit_status = iterate_preprocess(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	//spec_process_file =
	if((exit_status = do_iteration(dz))<0) {
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
	if((exit_status = mi_setup_internal_arrays_and_array_pointers(dz))<0)	 
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
	dz->infile->channels = dz->iparam[MITER_OCHANS];
	if((exit_status = create_sized_outfile(dz->outfilename,dz))<0)
		return(exit_status);
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

/************************* SETUP_MCHITER_APPLICATION *******************/

int setup_mchiter_application(dataptr dz)
{
	int exit_status;
	aplptr ap;
	if((exit_status = establish_application(dz))<0)		// GLOBAL
		return(FAILED);
	ap = dz->application;
	switch(dz->mode) {
	case(0):
		if((exit_status = set_param_data(ap,0,2,2,"id"))<0)
			return(FAILED);
		break;
	case(1):
		if((exit_status = set_param_data(ap,0,2,2,"ii"))<0)
			return(FAILED);		
		break;
	}
	if((exit_status = set_vflgs(ap,"",0,"","drpafgs" ,7,7,"DDDDDdi"))<0)
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
			sprintf(errstr,"File %s is not of correct type (must be mono)\n",cmdline[0]);
			return(DATA_ERROR);
		} else if((exit_status = copy_parse_info_to_main_structure(infile_info,dz))<0) {
			sprintf(errstr,"Failed to copy file parsing information\n");
			return(PROGRAM_ERROR);
		}
		free(infile_info);
	}
	return(FINISHED);
}

/************************* SETUP_MCHITER_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_mchiter_param_ranges_and_defaults(dataptr dz)
{
	int exit_status;
	aplptr ap = dz->application;
	// set_param_ranges()
	ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
	// NB total_input_param_cnt is > 0 !!!
	if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
		return(FAILED);
	// get_param_ranges()

	ap->lo[MITER_OCHANS]  = 2;
	ap->hi[MITER_OCHANS]  = 16;
	ap->default_val[MITER_OCHANS] = 8;
	switch(dz->mode) {
	case(0):
		ap->lo[MITER_DUR] = dz->duration;
		ap->hi[MITER_DUR] = BIG_TIME;
		ap->default_val[MITER_DUR] = dz->duration * 2.0; 	
		break;
	case(1):
		ap->lo[MITER_REPEATS] 	= 1.0;
		ap->hi[MITER_REPEATS] 	= BIG_VALUE;
		ap->default_val[MITER_REPEATS] = 2.0;
		break;
	}
	ap->lo[MITER_DELAY]	= FLTERR;
	ap->hi[MITER_DELAY]	= ITER_MAX_DELAY;
	ap->default_val[MITER_DELAY]= dz->duration;
	ap->lo[MITER_RANDOM]= 0.0;
	ap->hi[MITER_RANDOM]= 1.0;
	ap->default_val[MITER_RANDOM]= 0.0;
	ap->lo[MITER_PSCAT]	= 0.0;
	ap->hi[MITER_PSCAT]	= ITER_MAXPSHIFT;	
	ap->default_val[MITER_PSCAT] = 0.0;
	ap->lo[MITER_ASCAT]	= 0.0;
	ap->hi[MITER_ASCAT]	= 1.0;
	ap->default_val[MITER_ASCAT] = 0.0;
	ap->lo[MITER_FADE]	= 0.0;
	ap->hi[MITER_FADE]	= 1.0;
	ap->default_val[MITER_FADE]  = 0.0;
	ap->lo[MITER_GAIN]	= 0.0;
	ap->hi[MITER_GAIN]	= 1.0;
	ap->default_val[MITER_GAIN]  = 1.0; /* 0.0 */ 
	ap->lo[MITER_RSEED]	= 0.0;
	ap->hi[MITER_RSEED]	= MAXSHORT;
	ap->default_val[MITER_RSEED] = 0.0;
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
			if((exit_status = setup_mchiter_application(dz))<0)
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

int set_legal_internalparam_structure(int process,int mode,aplptr ap)
{
	return(FINISHED);
}

int setup_internal_arrays_and_array_pointers(dataptr dz)
{
	return(FINISHED);
}

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


/******************************** USAGE1 ********************************/

int usage1(void)
{
	usage2("iter");
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
	if(!strcmp(prog_identifier_from_cmdline,"iter"))				dz->process = MCHITER;
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

/****************************** DO_ITERATION *************************
 *
 * (1) First event is always copy of original.
 */

//TW COMPLETELY UPDATED FUNCTION : (flt-converted)
#define ACCEPTABLE_LEVEL 0.75

int do_iteration(dataptr dz)
{
	int    exit_status, iterating;
	int   write_end, tail, cnt, arraysize = BIGARRAY;
	float *tailend;
	int    bufs_written, finished;
	double level, thistime;
	int   out_sampdur = 0, inmsampsize;
	int   write_start, local_write_start;
	double one_over_sr = 1.0/(double)dz->infile->srate, maxsamp = 0.0;
	int    passno, is_penult = 0, pstep;
	int   k;
	double *gain, *pshift, gaingain = -1.0;
	int   *wstart;
	char   *permstore;
	int permstorecnt = 0;
	float	*orig_inbuf = dz->sampbuf[0];
	int permno = 0,  thischan, endchan;
	int outchans = dz->iparam[MITER_OCHANS], *perm = dz->iparray[0];
	permute_chans(outchans,dz);
	thischan = perm[permno];

	pstep = MITER_STEP;
	iterating = 1;
	if ((gain = (double *)malloc(arraysize * sizeof(double)))==NULL) {
		sprintf(errstr,"Insufficient memory to store gain values\n");
		return(MEMORY_ERROR);
	}
	if ((pshift = (double *)malloc(arraysize * sizeof(double)))==NULL) {
		sprintf(errstr,"Insufficient memory to store pitch shift values\n");
		return(MEMORY_ERROR);
	}
	if ((wstart = (int *)malloc(arraysize * sizeof(int)))==NULL) {
		sprintf(errstr,"Insufficient memory to store pitch shift values\n");
		return(MEMORY_ERROR);
	}
	if ((permstore = (char *)malloc(arraysize * outchans * sizeof(char)))==NULL) {
		sprintf(errstr,"Insufficient memory to store pitch shift values\n");
		return(MEMORY_ERROR);
	}
	for(k=0;k<outchans;k++)
		permstore[permstorecnt++] = (char)(perm[k]);

	if(dz->mode==ITERATE_DUR)
		out_sampdur  = round(dz->param[MITER_DUR] * (double)dz->infile->srate);
	if(sloom) {
		switch(dz->mode) {
		case(ITERATE_DUR):		
			dz->tempsize = out_sampdur;		
			break;
		case(ITERATE_REPEATS):	
			dz->tempsize = dz->insams[0] * (dz->iparam[MITER_REPEATS]+1);	/* approx */
			break; 
		}
	}
	for(passno=0;passno<2;passno++) {
		if(passno > 0) {
			permno = 0;
			thischan = permstore[permno];
		}
		is_penult = 0;
		cnt = 0;
		bufs_written = 0;
		write_start = 0;
		maxsamp = 0.0;
		memset((char *)dz->sampbuf[1],0,dz->buflen * outchans * sizeof(float));
		level = dz->param[MITER_FADE];
		sndseekEx(dz->ifd[0],0L,0);
		display_virtual_time(0L,dz);
		fflush(stdout);
		dz->sampbuf[0] = orig_inbuf;
		if(passno > 0) {
			print_outmessage_flush("Second pass, level adjusted\n");
			dz->tempsize = dz->total_samps_written/outchans;
			dz->total_samps_written = 0;
			memset((char *)dz->sampbuf[0],0,(dz->sampbuf[3] - dz->sampbuf[0]) * sizeof(float));
		}
		if((exit_status = read_the_input_snd(dz))<0)
			return(exit_status);
		if(dz->iparam[MITER_DO_SCALE])								
			scale_input(dz);
		inmsampsize = dz->insams[0];
		/* 1 */
		local_write_start = 0;
		switch(dz->iparam[MITER_PROCESS]) {
		case(MONO):		      		
			iter(0,passno,gain,local_write_start,inmsampsize,level,&maxsamp,iterating,thischan,dz);		
			break;
		case(MN_INTP_SHIFT):      	
			iter_shift_interp(0,passno,gain,pshift,local_write_start,inmsampsize,level,&maxsamp,pstep,iterating,thischan,dz);	 		
			break;
		case(FIXA_MONO):	      	
			fixa_iter(0,passno,gain,local_write_start,inmsampsize,level,&maxsamp,iterating,thischan,dz);			 		
			break;
		case(FIXA_MN_INTP_SHIFT): 	
			fixa_iter_shift_interp(0,passno,gain,pshift,local_write_start,inmsampsize,level,&maxsamp,pstep,iterating,thischan,dz); 		
			break;
		}
		permno++;
		if(passno == 0)
			thischan = perm[permno]; 
		else
			thischan = permstore[permno]; 
		write_end   = dz->insams[0];
		thistime = 0.0;
		if((exit_status = read_values_from_all_existing_brktables(thistime,dz))<0)
			return(exit_status);
		if(dz->brksize[MITER_DELAY])
			dz->iparam[MITER_MSAMPDEL] = round(dz->param[MITER_DELAY] * (double)dz->infile->srate);
		if(passno==0)
			wstart[cnt] = get_next_writestart(write_start,dz);
		write_start = wstart[cnt];
		local_write_start = write_start;
		finished = FALSE;
		for(;;) {
			switch(dz->mode) {
			case(ITERATE_DUR):
				if(write_start >= out_sampdur)
					finished = TRUE;
				break;
			case(ITERATE_REPEATS):
				if(cnt >= dz->iparam[MITER_REPEATS])
					finished = TRUE;
				break;
			}
			if(finished)
				break;
			while(local_write_start >= dz->buflen) {
				if(passno > 0) {
					if((exit_status = write_samps(dz->sampbuf[1],dz->buflen * outchans,dz))<0)
						return(exit_status);
				}
				bufs_written++;
				tail = write_end - dz->buflen;
				memset((char *)dz->sampbuf[1],0,dz->buflen * outchans * sizeof(float));
				if(tail > 0) {
					memmove((char *)dz->sampbuf[1],(char *)dz->sampbuf[2],tail * outchans * sizeof(float));
					tailend = dz->sampbuf[1] + (tail * outchans);
				} else
					tailend = dz->sampbuf[2]; // dz->sampbuf[2] must be set to something big enough to accomodate multichan out
				memset((char *)tailend,0,(dz->sampbuf[3] - tailend) * sizeof(float)); // dz->sampbuf[3] simil
				local_write_start -= dz->buflen;
				write_end         -= dz->buflen;
			}
			cnt++;
			if((passno == 0) && (cnt >= arraysize)) {
				arraysize += BIGARRAY;
				if ((gain = (double *)realloc((char *)gain,arraysize * sizeof(double)))==NULL) {
					sprintf(errstr,"Insufficient memory to store gain values (2)\n");
					return(MEMORY_ERROR);
				}
				if ((pshift = (double *)realloc((char *)pshift,arraysize * sizeof(double)))==NULL) {
					sprintf(errstr,"Insufficient memory to store pshift values (2)\n");
					return(MEMORY_ERROR);
				}
				if ((wstart = (int *)realloc((char *)wstart,arraysize * sizeof(int)))==NULL) {
					sprintf(errstr,"Insufficient memory to store wstart values (2)\n");
					return(MEMORY_ERROR);
				}
				if ((permstore = (char *)realloc((char *)wstart,arraysize * outchans * sizeof(char)))==NULL) {
					sprintf(errstr,"Insufficient memory to store wstart values (2)\n");
					return(MEMORY_ERROR);
				}
			}
			thistime = ((dz->buflen * bufs_written) + local_write_start) * one_over_sr;
			
			if((exit_status = read_values_from_all_existing_brktables(thistime,dz))<0)
				return(exit_status);
			if(is_penult) {
				dz->param[MITER_PSCAT] = 0.0;
				dz->param[MITER_ASCAT] = 0.0;
			}
			if(dz->brksize[MITER_DELAY])
				dz->iparam[MITER_MSAMPDEL] = round(dz->param[MITER_DELAY] * (double)dz->infile->srate);
			write_end = iterate(cnt,passno,gain,pshift,write_end,local_write_start,inmsampsize,level,&maxsamp,pstep,iterating,thischan,dz);
			permno++;
			if(passno == 0) {
				if(permno >= outchans) {
					endchan = perm[permno-1];
					do {
						permute_chans(dz->iparam[MITER_OCHANS],dz);
					} while(perm[0] == endchan);
					permno = 0;
					for(k=0;k<outchans;k++)
						permstore[permstorecnt++] = (char)(perm[k]);
				}
			}
			if(passno == 0)
				thischan = perm[permno]; 
			else
				thischan = permstore[permno]; 
			level *= dz->param[MITER_FADE];
			if(passno==0)
				wstart[cnt] = get_next_writestart(write_start,dz);
			write_start = wstart[cnt];
			local_write_start = write_start - (bufs_written * dz->buflen);
		}
		if(passno > 0) {
			if(write_end > 0) {
				if((exit_status = write_samps(dz->sampbuf[1],write_end * outchans,dz))<0)
					return(exit_status);
			}
		} else {
		if(maxsamp <= 0.0) {
			sprintf(errstr,"No significant signal level found");
			return(DATA_ERROR);
		}
		if(maxsamp < ACCEPTABLE_LEVEL || maxsamp > 0.99)
			gaingain = ACCEPTABLE_LEVEL/maxsamp;
		else
			gaingain = 1.0;
			switch(dz->iparam[MITER_PROCESS]) {
			case(MONO):		      		
			case(MN_INTP_SHIFT):      	
				for(k=0;k<=cnt;k++)
					gain[k] *= gaingain;
				break;
			case(FIXA_MONO):	      	
			case(FIXA_MN_INTP_SHIFT): 	
				for(k=0;k<=cnt;k++)
					gain[k] = gaingain;
				break;
			}
		}
	}
	return FINISHED;
}

/*************************** READ_THE_INPUT_SND **************************/

int read_the_input_snd(dataptr dz)
{
	int samps_read;
	if((samps_read = fgetfbufEx(dz->sampbuf[0], dz->insams[0],dz->ifd[0],0)) <= 0) {
		sprintf(errstr,"Can't read samps from input soundfile\n");
		if(samps_read<0)
			return(SYSTEM_ERROR);
		return(DATA_ERROR);
	}
	if(samps_read!=dz->insams[0]) {
		sprintf(errstr, "Failed to read all of source file. read_the_input_snd()\n");
		return(PROGRAM_ERROR);
	}
	if(dz->vflag[IS_ITER_PSCAT])
		dz->sampbuf[0][samps_read] = (float)0; /* GUARD POINT FOR INTERPOLATION */
	return(FINISHED);
}

/******************************* SCALE_INPUT ****************************/

void scale_input(dataptr dz)
{
	int n;
	int end = dz->insams[0];
	if(dz->iparam[MITER_PROCESS]!=FIXA_MONO && dz->iparam[MITER_PROCESS]!=FIXA_STEREO)
		end = dz->insams[0] + 1;	/* ALLOW FOR GUARD POINTS */
	for(n=0; n < end; n++)
		dz->sampbuf[0][n] = (float)(dz->sampbuf[0][n] * dz->param[MITER_GAIN]);
}

/*************************** GET_NEXT_WRITESTART ****************************/

int get_next_writestart(int write_start,dataptr dz)
{
	int this_step;
	double d;  
	if(dz->vflag[IS_ITER_RAND]) {
		d = ((drand48() * 2.0) - 1.0) * dz->param[MITER_RANDOM];
		d += 1.0;
		this_step = (int)round((double)dz->iparam[MITER_MSAMPDEL] * d);
		write_start += this_step;
	} else
		write_start += dz->iparam[MITER_MSAMPDEL];
	return(write_start);
}    

/******************************* ITERATE *****************************/

//TW COMPLETELY UPDATED FUNCTION : (flt-converted)
int iterate
(int cnt,int pass,double *gain,double *pshift,int write_end,int local_write_start,
	int inmsampsize,double level,double *maxsamp,int pstep,int iterating,int thischan,dataptr dz)
{
	int wr_end = 0;
	switch(dz->iparam[MITER_PROCESS]) {
	case(MONO):		      		
		wr_end = iter(cnt,pass,gain,local_write_start,inmsampsize,level,maxsamp,iterating,thischan,dz);							
		break;
	case(MN_INTP_SHIFT):      	
		wr_end = iter_shift_interp(cnt,pass,gain,pshift,local_write_start,inmsampsize,level,maxsamp,pstep,iterating,thischan,dz);				
		break;
	case(FIXA_MONO):	      	
		wr_end = fixa_iter(cnt,pass,gain,local_write_start,inmsampsize,level,maxsamp,iterating,thischan,dz);			 			
		break;
	case(FIXA_MN_INTP_SHIFT): 	
		wr_end = fixa_iter_shift_interp(cnt,pass,gain,pshift,local_write_start,inmsampsize,level,maxsamp,pstep,iterating,thischan,dz); 		
		break;
	}
	return max(wr_end,write_end);
}

/**************************** ITER ***************************/

//TW COMPLETELY UPDATED FUNCTION : (flt-converted)
int iter(int cnt,int passno, double *gain,int local_write_start,int inmsampsize,double level,double *maxsamp,int iterating,int thischan,dataptr dz)
{
	int outchans = dz->iparam[MITER_OCHANS];
	register int i, j = (local_write_start * outchans) + thischan;
	float *outbuf = dz->sampbuf[1];
	float *inbuf  = dz->sampbuf[0];
	double z;
	double thisgain;
	if(passno == 0) {
		gain[cnt] = get_gain(dz);
		thisgain = gain[cnt];

		if(dz->vflag[IS_ITER_FADE]) {
			for(i=0; i < inmsampsize; i++) {
				z = outbuf[j] + (inbuf[i] * thisgain * level);
				*maxsamp = max(*maxsamp,fabs(z));
				outbuf[j] = (float)z;
				j += outchans;
				local_write_start++;
			}
		} else {
			for(i=0; i < inmsampsize; i++) {
				if(iterating)
					z = outbuf[j] + (inbuf[i] * thisgain);
				else
					z = outbuf[j] + inbuf[i];
				*maxsamp = max(*maxsamp,fabs(z));
				outbuf[j] = (float)z;
				j += outchans;
				local_write_start++;
			}
		}
	} else {
		thisgain = gain[cnt];

		if(dz->vflag[IS_ITER_FADE]) {
			for(i=0; i < inmsampsize; i++) {
				z = outbuf[j] + (inbuf[i] * thisgain * level);
				outbuf[j] = (float)z;
				j += outchans;
				local_write_start++;
			}
		} else {
			for(i=0; i < inmsampsize; i++) {
			if(iterating)
				z = outbuf[j] + (inbuf[i] * thisgain);
			else
				z = outbuf[j] + inbuf[i];
				outbuf[j] = (float)z;
				j += outchans;
				local_write_start++;
			}
		}
	}
	return(local_write_start);
}

/**************************** ITER_SHIFT_INTERP ***************************/

//TW COMPLETELY UPDATED FUNCTION : (flt-converted)
int iter_shift_interp(int cnt,int passno, double *gain,double *pshift,int local_write_start,
		int inmsampsize,double level,double *maxsamp,int pstep,int iterating,int thischan,dataptr dz)
{
	int outchans = dz->iparam[MITER_OCHANS];
	register int i = 0, j = (local_write_start * outchans) + thischan;
	double d = 0.0, part = 0.0;
	float val, nextval, diff;
	float *outbuf = dz->sampbuf[1];
	float *inbuf  = dz->sampbuf[0];
	double z;
	double thisgain;
	if(passno == 0) {
		gain[cnt] = get_gain(dz);
		thisgain = gain[cnt];

		if(dz->vflag[IS_ITER_FADE]) {
			while(i < inmsampsize) {
				val     = inbuf[i++];
				nextval = inbuf[i];
				diff    = nextval - val;
				z = val + ((double)diff * part);
				z = (z * thisgain * level);
				z += outbuf[j];
//				*maxsamp = max(*maxsamp,abs(z));
				*maxsamp = max(*maxsamp,fabs(z));
				outbuf[j] = (float)z;
				j += outchans;
				local_write_start++;
				d      += dz->param[pstep];
				i      = (int)d; 			/* TRUNCATE */
				part   = d - (double)i; 
			}
		} else {
			while(i < inmsampsize) {
				val     = inbuf[i++];
				nextval = inbuf[i];
				diff    = nextval - val;
				z = val + ((double)diff * part);
				if(iterating)
					z = (z * thisgain);
				z += outbuf[j];
//				*maxsamp = max(*maxsamp,abs(z));
				*maxsamp = max(*maxsamp,fabs(z));
				outbuf[j] = (float)z;
				j += outchans;
				local_write_start++;
				d      += dz->param[pstep];
				i      = (int)d; 			/* TRUNCATE */
				part   = d - (double)i; 
			}
		}
		pshift[cnt] = get_pshift(dz);
		dz->param[pstep] = pshift[cnt];
	} else {
		thisgain = gain[cnt];

		if(dz->vflag[IS_ITER_FADE]) {
			while(i < inmsampsize) {
				val     = inbuf[i++];
				nextval = inbuf[i];
				diff    = nextval - val;
				z = val + ((double)diff * part);
				z = (z * thisgain * level);
				z += outbuf[j];
				outbuf[j] = (float)z;
				j += outchans;
				local_write_start++;
				d      += dz->param[pstep];
				i      = (int)d; 			/* TRUNCATE */
				part   = d - (double)i; 
			}
		} else {
			while(i < inmsampsize) {
				val     = inbuf[i++];
				nextval = inbuf[i];
				diff    = nextval - val;
				z = val + ((double)diff * part);
				if(iterating)
					z = (z * thisgain);
				z += outbuf[j];
				local_write_start++;
				outbuf[j] = (float)z;
				j += outchans;
				d      += dz->param[pstep];
				i      = (int)d; 			/* TRUNCATE */
				part   = d - (double)i; 
			}
		}
		dz->param[pstep] = pshift[cnt];
	}
	return(local_write_start);
}

/**************************** FIXA_ITER ***************************/

//TW COMPLETELY UPDATED FUNCTION : (flt-converted)
int fixa_iter(int cnt,int passno,double *gain,int local_write_start,int inmsampsize,double level,double *maxsamp,int iterating,int thischan,dataptr dz)
{
	int outchans = dz->iparam[MITER_OCHANS];
	register int i, j = (local_write_start * outchans) + thischan;
	float *outbuf = dz->sampbuf[1];
	float *inbuf  = dz->sampbuf[0];
	double z;

	if(passno ==0) {
		if(dz->vflag[IS_ITER_FADE]) {
			for(i=0; i < inmsampsize; i++) {
				z = outbuf[j] + (inbuf[i] * level);
				*maxsamp = max(*maxsamp,fabs(z));
				outbuf[j]  = (float)z;
				j += outchans;
				local_write_start++;
			}
		} else {
			for(i=0; i < inmsampsize; i++) {
				z = outbuf[j] + inbuf[i];
				*maxsamp = max(*maxsamp,fabs(z));
				outbuf[j]  = (float)z;
				j += outchans;
				local_write_start++;
			}
		}
	} else {
		if(dz->vflag[IS_ITER_FADE]) {
			for(i=0; i < inmsampsize; i++) {
				z = outbuf[j] + (inbuf[i] * level * gain[cnt]);
				outbuf[j]  = (float)z;
				j += outchans;
				local_write_start++;
			}
		} else {
			for(i=0; i < inmsampsize; i++) {
				if(iterating)
					z = outbuf[j] + (inbuf[i] * gain[cnt]);
				else
				z = outbuf[j] + inbuf[i];
				outbuf[j]  = (float)z;
				j += outchans;
				local_write_start++;
			}
		}
	}
	return(local_write_start);
}

/**************************** FIXA_ITER_SHIFT_INTERP ***************************/

//TW COMPLETELY UPDATED FUNCTION : (flt-converted)
int fixa_iter_shift_interp(int cnt,int passno,double *gain,double *pshift,int local_write_start,
		int inmsampsize,double level,double *maxsamp,int pstep,int iterating,int thischan,dataptr dz)
{
	int outchans = dz->iparam[MITER_OCHANS];
	register int i = 0, j = (local_write_start * outchans) + thischan;
 	double d = 0.0, part = 0.0;
 	float val, nextval, diff;
 	float *outbuf = dz->sampbuf[1];
	float *inbuf  = dz->sampbuf[0];
	double z;

	if(passno == 0) {
		if(dz->vflag[IS_ITER_FADE]) {
		 	while(i < inmsampsize) {
		 		val     = inbuf[i++];
		 		nextval = inbuf[i];
		 		diff    = nextval - val;
				z = val + ((double)diff * part);
				z = (z * level);
				z += outbuf[j];
				*maxsamp = max(*maxsamp,fabs(z));
				outbuf[j] = (float)z;
				j += outchans;
				local_write_start++;
		 		d      += dz->param[pstep];
		 		i      = (int)d; 			/* TRUNCATE */
		 		part   = d - (double)i; 
		 	}
		} else {
		 	while(i < inmsampsize) {
		 		val     = inbuf[i++];
		 		nextval = inbuf[i];
		 		diff    = nextval - val;
				z = val + ((double)diff * part);
				z += outbuf[j];
				*maxsamp = max(*maxsamp,fabs(z));
		 		outbuf[j] = (float)z;
				j += outchans;
				local_write_start++;
		 		d      += dz->param[pstep];
		 		i      = (int)d; 			/* TRUNCATE */
		 		part   = d - (double)i; 
		 	}
		}
		pshift[cnt] = get_pshift(dz);
	} else {
		if(dz->vflag[IS_ITER_FADE]) {
		 	while(i < inmsampsize) {
		 		val     = inbuf[i++];
		 		nextval = inbuf[i];
		 		diff    = nextval - val;
				z = val + ((double)diff * part);
				z = (z * level * gain[cnt]);
				z += outbuf[j];
		 		outbuf[j] = (float)z;
				j += outchans;
				local_write_start++;
		 		d      += dz->param[pstep];
		 		i      = (int)d; 			/* TRUNCATE */
		 		part   = d - (double)i; 
		 	}
		} else {
			while(i < inmsampsize) {
		 		val     = inbuf[i++];
		 		nextval = inbuf[i];
		 		diff    = nextval - val;
				z = val + ((double)diff * part);
				if(iterating)
					z = (z * gain[cnt]);
				z += outbuf[j];
		 		outbuf[j] = (float)z;
				j += outchans;
				local_write_start++;
		 		d      += dz->param[pstep];
		 		i      = (int)d; 			/* TRUNCATE */
		 		part   = d - (double)i; 
		 	}
		}
	}
	dz->param[pstep] = pshift[cnt];
 	return(local_write_start);
}

/******************************** GET_GAIN *****************************/

double get_gain(dataptr dz)
{
	double scatter;
	double newlgain;
	newlgain = dz->param[MITER_GAIN];
	if(dz->vflag[IS_ITER_ASCAT]) {
		scatter  = drand48() * dz->param[MITER_ASCAT];
		scatter  = 1.0 - scatter;
		newlgain = scatter * (double)dz->param[MITER_GAIN];
	}
	return(newlgain);
}

/******************************** GET_PSHIFT *****************************/

double get_pshift(dataptr dz)
{
	double scatter;
	scatter = (drand48() * 2.0) - 1.0;
	scatter *= dz->param[MITER_PSCAT];
	return(pow(2.0,scatter * OCTAVES_PER_SEMITONE));
}

/*************************** CREATE_ITERBUFS **************************
 *
 * (1)	Create extra spaces for interpolation guard points at end of infile.
 *
 * (2) 	Allow for any cumulative addition errors in interpolation.
 *
 * (3)	Output buffer must be at least as big as the overflow buffer.
 *	Output buffer must be big enough for the whole of any possible
 *	data overflow (in overflow_size buff) to be copied back into it.
 *	This is because the overflow buffer is ZEROED after such a copy
 *	and if a 2nd copy of the overflow back into the main buffer
 *	were necessary , we would be copying zeroes rather than true data.
 *
 *
 *		true buffer	    			overflow
 *  |-----------------------------|------------------------------------------|
 *			    worst			  ^					    					 ^
 *		    possible case		  ^					    					 ^
 *		                 	      ^----->-delayed by maxdelay_size to ->-----^
 *			         			  ^<-restored by -buffer_size into truebuf-<-^
 *  |<-------- BUFFER_SIZE------->
 *
 */

int create_mchiterbufs(double maxpscat,dataptr dz)
{
	size_t bigbufsize, seccnt, outchans = dz->iparam[MITER_OCHANS];
	double k;
	size_t extra_space, infile_space = dz->insams[0], big_buffer_size;
	size_t overflowsize;
	size_t framesize = F_SECSIZE * sizeof(float);
	size_t bigchunk, min_bufsize;

	if(dz->vflag[IS_ITER_PSCAT]) {
		infile_space++;			/* 1 */
		k = pow(2.0,maxpscat * OCTAVES_PER_SEMITONE);
		overflowsize = round((double)dz->insams[0] * k) + 1;
		overflowsize += ITER_SAFETY; 						/* 2 */	
	} else
		overflowsize = dz->insams[0];

	overflowsize *= outchans;
	if((seccnt = infile_space/F_SECSIZE) * F_SECSIZE < infile_space)
		seccnt++;
	infile_space = F_SECSIZE * seccnt;

	extra_space = (infile_space * outchans) + overflowsize;
	min_bufsize = (extra_space * sizeof(float)) + framesize;
	bigchunk = (size_t) Malloc(-1);
	if(bigchunk < min_bufsize)
		bigbufsize = framesize;
	else {
		bigbufsize = bigchunk - extra_space*sizeof(float);
		bigbufsize = (bigbufsize/framesize) * framesize;
		if(bigbufsize <= 0)
			bigbufsize = framesize;
	}
	dz->buflen     = (int)(bigbufsize/sizeof(float));
	big_buffer_size = dz->buflen + extra_space;
	if((dz->bigbuf = (float *) Malloc(big_buffer_size * sizeof(float)))==NULL) {
		sprintf(errstr, "INSUFFICIENT MEMORY to create sound buffers.\n");
		return(MEMORY_ERROR);
	}
	dz->sbufptr[0] = dz->sampbuf[0] = dz->bigbuf;
	dz->sbufptr[1] = dz->sampbuf[1] = dz->sampbuf[0] + infile_space;
	dz->sbufptr[2] = dz->sampbuf[2] = dz->sampbuf[1] + (dz->buflen * outchans);
	dz->sbufptr[3] = dz->sampbuf[3] = dz->sampbuf[2] + overflowsize;
	memset((char *)dz->sampbuf[0],0,(size_t)(infile_space * sizeof(float)));
	memset((char *)dz->sampbuf[1],0,(size_t)(dz->buflen * outchans * sizeof(float)));
	memset((char *)dz->sampbuf[2],0,(size_t)(overflowsize * sizeof(float)));
	return(FINISHED);
}

/**************************** ITERATE_PREPROCESS ******************************/

int iterate_preprocess(dataptr dz)
{
	int exit_status;
	double maxrand, maxpscat, mindelay;
	int mindelay_samps, is_unity_gain = FALSE;
	if(dz->iparam[MITER_RSEED] > 0)
		srand((int)dz->iparam[MITER_RSEED]);
	else
		initrand48();
	if((exit_status = get_maxvalue_of_rand(&maxrand,dz))<0)
			return(exit_status);
	if((exit_status = get_maxvalue_of_pscat(&maxpscat,dz))<0)
			return(exit_status);
	if((exit_status = get_minvalue_of_delay(&mindelay,dz))<0)
			return(exit_status);
 	mindelay_samps = round(mindelay * (double)dz->infile->srate);
	if(dz->param[MITER_GAIN]==DEFAULT_ITER_GAIN)
		set_default_gain(mindelay_samps,dz);
	set_default_delays(dz);
	reverse_fadevals(dz);
	dz->param[MITER_STEP] = 1.0; /* 1st sound is exact copy of orig */
	if(flteq(dz->param[MITER_GAIN],1.0))
		is_unity_gain = TRUE;
	setup_iter_process_type(is_unity_gain,dz);
	if((dz->iparray[0] = (int *)malloc(dz->iparam[MITER_OCHANS] * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to make channel perm array.\n");
		return(MEMORY_ERROR);
	}
	if(sloom) {
		dz->vflag[IS_ITER_DELAY] = 1;
		if(flteq(dz->param[MITER_RANDOM],0.0))
			dz->vflag[IS_ITER_RAND] = 0;
		else
			dz->vflag[IS_ITER_RAND] = 01;

		if(flteq(dz->param[MITER_PSCAT],0.0))
			dz->vflag[IS_ITER_PSCAT] = 0;
		else
			dz->vflag[IS_ITER_PSCAT] = 1;

		if(flteq(dz->param[MITER_ASCAT],0.0))
			dz->vflag[IS_ITER_ASCAT] = 0;
		else
			dz->vflag[IS_ITER_ASCAT] = 1;

		if(flteq(dz->param[MITER_FADE],0.0) || flteq(dz->param[MITER_FADE],1.0))
			dz->vflag[IS_ITER_FADE] = 0;
		else
			dz->vflag[IS_ITER_FADE] = 1;

		if(flteq(dz->param[MITER_GAIN],0.0))
			dz->vflag[IS_ITER_GAIN] = 0;
		else
			dz->vflag[IS_ITER_GAIN] = 1;

		if(flteq(dz->iparam[MITER_RSEED],0))
			dz->vflag[IS_ITER_RSEED] = 0;
		else
			dz->vflag[IS_ITER_RSEED] = 1;
	}
	return create_mchiterbufs(maxpscat,dz);
}

/*************************** GET_MAXVALUE_OF_RAND ****************************/

int get_maxvalue_of_rand(double *maxrand,dataptr dz)
{
	int exit_status;
	if(dz->brksize[MITER_RANDOM]) {
		if((exit_status = get_maxvalue_in_brktable(maxrand,MITER_RANDOM,dz))<0)
			return(exit_status);
	} else
		*maxrand = dz->param[MITER_RANDOM];
	return(FINISHED);
}

/************************* GET_MAXVALUE_OF_PSCAT ****************************/

int get_maxvalue_of_pscat(double *maxpscat,dataptr dz)
{
	int exit_status;
	if(dz->brksize[MITER_PSCAT]) {
		if((exit_status = get_maxvalue_in_brktable(maxpscat,MITER_PSCAT,dz))<0)
			return(exit_status);
	} else
		*maxpscat = dz->param[MITER_PSCAT];
	return(FINISHED);
}

/************************* GET_MINVALUE_OF_DELAY ****************************/

int get_minvalue_of_delay(double *mindelay,dataptr dz)
{
	int exit_status;
	if(dz->brksize[MITER_DELAY]) {
		if((exit_status = get_minvalue_in_brktable(mindelay,MITER_DELAY,dz))<0)
			return(exit_status);
	} else
		*mindelay = dz->param[MITER_DELAY];
	return(FINISHED);
}

/************************* REVERSE_FADEVALS ****************************/

void reverse_fadevals(dataptr dz)
{
	double *p, *pend;

	if(dz->brksize[ITER_FADE]==0)
		dz->param[ITER_FADE] = 1.0 - dz->param[ITER_FADE];
	else {
		p    = dz->brk[ITER_FADE] + 1;
		pend = dz->brk[ITER_FADE] + (dz->brksize[ITER_FADE] * 2);
		while(p < pend) {
			*p = 1.0 - *p;
			p += 2;
		}
	}
}

/************************** SET_DEFAULT_GAIN ****************************/

void set_default_gain(int mindelay_samps,dataptr dz)
{
	int maxoverlay_cnt;
	maxoverlay_cnt = round(((double)dz->insams[0]/(double)mindelay_samps)+1.0);
	if(dz->vflag[IS_ITER_RAND])
		maxoverlay_cnt++;
	dz->param[MITER_GAIN]   = 1.0/(double)maxoverlay_cnt;
}

/*********************** SET_DEFAULT_DELAYS ****************************/

void set_default_delays(dataptr dz)
{
	if(dz->vflag[IS_ITER_DELAY]) {
		if(!dz->brksize[MITER_DELAY])
			dz->iparam[MITER_MSAMPDEL] = round(dz->param[MITER_DELAY] * (double)dz->infile->srate);
	} else
		dz->iparam[MITER_MSAMPDEL] = dz->insams[0];	/* default */
}

/********************* SETUP_ITER_PROCESS_TYPE ********************************/

void setup_iter_process_type(int is_unity_gain,dataptr dz)
{
	dz->iparam[MITER_DO_SCALE] = TRUE;
	if(dz->vflag[IS_ITER_PSCAT])
		dz->iparam[MITER_PROCESS] = MN_INTP_SHIFT;
	else
		dz->iparam[MITER_PROCESS] = MONO;
	if(!dz->vflag[IS_ITER_ASCAT]) {
		if(is_unity_gain)
			dz->iparam[MITER_DO_SCALE] = FALSE;
		dz->iparam[MITER_PROCESS] += FIXED_AMP;
	}
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
	if(!strcmp(str,"iter")) {		
	    fprintf(stdout,
		"ITERATE INPUT SOUND IN A FLUID MANNER, SCATTERING TO MULTICHANNEL SPACE\n\n"
		"USAGE: mchiter iter 1 infil outfil outchans outduration\n"
		"     [-ddelay] [-rrand] [-ppshift] [-aampcut] [-ffade] [-ggain] [-sseed]\n"
		"OR:    mchiter iter 2 infil outfil outchans repetitions\n"
		"     [-ddelay] [-rrand] [-ppshift] [-aampcut] [-ffade] [-ggain] [-sseed]\n\n"
		"outduration  duration of output file.\n"
		"repetitions  number of iterations of the source.\n"
		"delay   (average) delay between iterations. Default: infile duration.\n"
		"rand    delaytime-randomisation: Range 0 - 1: Default 0\n"
		"pshift  max of random pitchshift of each iter: Range 0 - %.0lf semitones\n"
		"        e.g.  2.5 =  2.5 semitones up or down.\n"
		"ampcut  max of random amp-reduction on each iter: Range 0-1: default 0\n"
		"fade    (average) amplitude fade beween iters (Range 0 - 1: default 0)\n"
		"gain    Overall Gain: Range 0 - 1:\n"
		"        special val 0 (default), gives best guess for no distortion.\n"
		"seed	 the same seed-number will produce identical output on rerun,\n"
		"        (Default: (0) random sequence is different every time).\n",ITER_MAXPSHIFT);
	} else
		fprintf(stdout,"Unknown option '%s'\n",str);
	return(USAGE_ONLY);
}

/******************************** USAGE3 ********************************/

int usage3(char *str1,char *str2)
{
	sprintf(errstr,"Insufficient parameters on command line.\n");
	return(USAGE_ONLY);
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
	dz->iparray[0][t+1] = n;
}

/****************************** PREFIX ****************************/

void prefixch(int n,int outchans,dataptr dz)
{   
	shuflupch(0,outchans,dz);
	dz->iparray[0][0] = n;
}

/****************************** SHUFLUPCH ****************************/

void shuflupch(int k,int outchans,dataptr dz)
{   
	int n;
	for(n = outchans - 1; n > k; n--)
		dz->iparray[0][n] = dz->iparray[0][n-1];
}

/***************************** MI_SETUP_INTERNAL_ARRAYS_AND_ARRAY_POINTERS **************************/

int mi_setup_internal_arrays_and_array_pointers(dataptr dz)
{
	int n;		 
	dz->iarray_cnt = 1;
	if((dz->iparray = (int **)malloc(dz->iarray_cnt * sizeof(int *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for internal int arrays.\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<dz->iarray_cnt;n++)
		dz->iparray[n] = NULL;
	return(FINISHED);
}

