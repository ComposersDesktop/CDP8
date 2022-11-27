#include <stdio.h>
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

#include <stdlib.h>
#include <structures.h>
#include <tkglobals.h>
#include <pnames.h>
#include <filetype.h>
#include <processno.h>
#include <modeno.h>
#include <logic.h>
#include <flags.h>
#include <globcon.h>
#include <speccon.h>
#include <txtucon.h>
#include <cdpmain.h>
#include <math.h>
#include <mixxcon.h>
#include <osbind.h>
#include <standalone.h>
#include <ctype.h>
#include <sfsys.h>
#include <string.h>
#include <srates.h>
#include <modicon.h>
#include <arrays.h>

#ifdef unix
#define round(x) lround((x))
#endif

char errstr[2400];

int anal_infiles = 1;
int	sloom = 0;
int sloombatch = 0;

const char* cdp_version = "7.1.0";

#define MININC 	(0.002)		/* rwd - black hole avoidance */

//CDP LIB REPLACEMENTS
//static int check_gate_param_validity_and_consistency(dataptr dz);
static int setup_strans_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_strans_param_ranges_and_defaults(dataptr dz);
static int handle_the_outfile(int *cmdlinecnt,char ***cmdline,dataptr dz);
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
static int setup_and_init_input_brktable_constants(dataptr dz,int brkcnt);
static int strans_preprocess(dataptr dz);
static int process_varispeed(dataptr dz);
static int convert_brkpnts(dataptr dz);
static int change_frame(dataptr dz);
static long inv_cntevents(long dur,double s0,double s1);
static int force_value_at_end_time(int paramno,dataptr dz);
static int read_samps_for_strans(dataptr dz);
static int samesize(long *obufcnt,long intime0,double insize0,double insize1,long duration,dataptr dz);
static int getetime(long *obufcnt,long t0,long t1,double s0,double s1,long number,dataptr dz);
static double cntevents(long dur,double s0,double s1);
static int timevents(long *obufcnt,long intime0,long intime1,double insize0,double insize1,dataptr dz);
static int write_samps_with_intime_display(float *buf,int samps_to_write,dataptr dz);
static void splice_end(long obufcnt,dataptr dz);		/* A kludge to avoid clicks at end */
static int sizeq(double f1,double f2);
static int putval(long *obufcnt,double pos,dataptr dz);
static void dz2props(dataptr dz, SFPROPS* props);
static int create_stransbufs(dataptr dz);
static double refinesize(double hibound,double lobound,double fnumber,long duration,double error,double insize0);
static int integral_times(long *obufcnt,long intime0,long isize,long number,dataptr dz);
static int unvarying_times(long *obufcnt,long intime0,double size,long number,dataptr dz);
static int putintval(long *obufcnt,long i,dataptr dz);
static int do_acceleration(dataptr dz);
static int do_vibrato(dataptr dz);
static double interp_read_sintable(double *sintab,double sfindex);
static int get_the_mode_from_cmdline(char *str,dataptr dz);

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
	int exit_status;
	dataptr dz = NULL;
	char **cmdline;
	int  cmdlinecnt;
	long n;
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
	dz->maxmode = 4;
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
		if((exit_status = get_the_mode_from_cmdline(cmdline[0],dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(exit_status);
		}
		cmdline++;
		cmdlinecnt--;
		// setup_particular_application =
		if((exit_status = setup_strans_application(dz))<0) {
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
	if((exit_status = setup_strans_param_ranges_and_defaults(dz))<0) {
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
//	check_param_validity_and_consistency() 			redundant
	is_launched = TRUE;
	dz->bufcnt = 2;
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

	if((exit_status = create_stransbufs(dz))<0) {							// CDP LIB
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = strans_preprocess(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	switch(dz->mode) {
	case(0):
	case(1): exit_status = process_varispeed(dz); break;
	case(2): exit_status = do_acceleration(dz);	  break;
	case(3): exit_status = do_vibrato(dz);		  break;
	}
	if(exit_status<0) {
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
	if((exit_status = setup_internal_arrays_and_array_pointers(dz))<0)	 
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
	int stype, n;
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
	if(dz->floatsam_output!=1)
		stype = SAMP_SHORT;
	else
		stype = dz->infile->stype;
	dz->true_outfile_stype = stype;
	dz->outfilesize = -1;
	if(dz->infile->channels > 2 || stype > SAMP_FLOAT) {
		SFPROPS props, inprops;
		dz2props(dz,&props);
		props.chans = dz->infile->channels;
		props.srate = dz->infile->srate;
		if(dz->ifd && dz->ifd[0] >=0) {
			if(snd_headread(dz->ifd[0], &inprops))	  /* snd_getchanformat not working ...*/
				props.chformat = inprops.chformat;
		}
		dz->ofd = sndcreat_ex(filename,dz->outfilesize,&props,SFILE_CDP,CDP_CREATE_NORMAL); 
		if(dz->ofd < 0){
			sprintf(errstr,"Cannot open output file %s\n", filename);
			return(DATA_ERROR);
		}
	} else {
		if((dz->ofd = sndcreat_formatted(filename,dz->outfilesize,stype,
				dz->infile->channels,dz->infile->srate,CDP_CREATE_NORMAL)) < 0) {
			sprintf(errstr,"Cannot open output file %s\n", filename);
			return(DATA_ERROR);
		}
	}
	dz->outchans = dz->infile->channels;
	dz->needpeaks = 1;
	dz->outpeaks = (CHPEAK *) malloc(sizeof(CHPEAK) * dz->outchans);
	if(dz->outpeaks==NULL)
		return MEMORY_ERROR;
	dz->outpeakpos = (unsigned int *) malloc(sizeof(unsigned int) * dz->outchans);
	if(dz->outpeakpos==NULL)
		return MEMORY_ERROR;
	for(n=0;n < dz->outchans;n++){
		dz->outpeaks[n].value = 0.0f;
		dz->outpeaks[n].position = 0;
		dz->outpeakpos[n] = 0;
	}
	dz->outfilesize = sndsizeEx(dz->ofd);
	(*cmdline)++;
	(*cmdlinecnt)--;
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

/************************* SETUP_STRANS_APPLICATION *******************/

int setup_strans_application(dataptr dz)
{
	int exit_status;
	aplptr ap;
	if((exit_status = establish_application(dz))<0)		// GLOBAL
		return(FAILED);
	ap = dz->application;
	// SEE parstruct FOR EXPLANATION of next 2 functions
	switch(dz->mode) {
	case(0):
	case(1):
		if((exit_status = set_param_data(ap,0   ,1,1,"D"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","o",1,0,"0"))<0)
			return(FAILED);
		break;
	case(2):
		if((exit_status = set_param_data(ap,0  ,2,2, "dd"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"s",1,"d","",0,0,""))<0)	
			return(FAILED);
		break;
	case(3):
		if((exit_status = set_param_data(ap,0  ,2,2, "DD"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","",0,0,""))<0)	
			return(FAILED);
		break;
	}
// TW April 2015 -->
	if((exit_status = set_legal_internalparam_structure(dz->process,dz->mode,ap))<0)
		return(exit_status);										/* LIBRARY */
// <-- TW April 2015
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
		} else if((exit_status = copy_parse_info_to_main_structure(infile_info,dz))<0) {
			sprintf(errstr,"Failed to copy file parsing information\n");
			return(PROGRAM_ERROR);
		}
		free(infile_info);
	}
	return(FINISHED);
}

/************************* SETUP_STRANS_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_strans_param_ranges_and_defaults(dataptr dz)
{
	int exit_status;
	double  duration;
	aplptr ap = dz->application;
	// set_param_ranges()
	ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
	// NB total_input_param_cnt is > 0 !!!
	if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
		return(FAILED);
	// get_param_ranges()
	duration = (double)(dz->insams[0]/dz->infile->channels)/(double)dz->infile->srate;

	switch(dz->mode) {
		case(MOD_TRANSPOS):
			ap->lo[VTRANS_TRANS] = MIN_TRANSPOS;
			ap->hi[VTRANS_TRANS] = MAX_TRANSPOS;
			ap->default_val[VTRANS_TRANS] = 1.0;
			break;
		case(MOD_TRANSPOS_SEMIT):		   
			ap->lo[VTRANS_TRANS] = EIGHT_8VA_DOWN;
			ap->hi[VTRANS_TRANS] = EIGHT_8VA_UP;
			ap->default_val[VTRANS_TRANS] = 0.0;
			break;
		case(2):
			ap->lo[ACCEL_ACCEL]		= MIN_ACCEL;
			ap->hi[ACCEL_ACCEL]		= MAX_ACCEL;
			ap->default_val[ACCEL_ACCEL] = 1.0;
			ap->lo[ACCEL_GOALTIME]	= MINTIME_ACCEL;
			ap->hi[ACCEL_GOALTIME]	= duration;
			ap->default_val[ACCEL_GOALTIME] = min(1.0,duration);
			ap->lo[ACCEL_STARTTIME]	= 0.0;
			ap->hi[ACCEL_STARTTIME]	= duration - MINTIME_ACCEL;
			ap->default_val[ACCEL_STARTTIME]= 0.0;
			break;
		case(3):
			ap->lo[VIB_FRQ]		= 0.0;
			ap->hi[VIB_FRQ]		= MAX_VIB_FRQ;
			ap->default_val[VIB_FRQ]	= DEFAULT_VIB_FRQ;
			ap->lo[VIB_DEPTH] 	= 0.0;
			ap->hi[VIB_DEPTH] 	= EIGHT_8VA_UP;
			ap->default_val[VIB_DEPTH] 	= DEFAULT_VIB_DEPTH;
			break;
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
			if((exit_status = setup_strans_application(dz))<0)
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

/************************* SETUP_INTERNAL_ARRAYS_AND_ARRAY_POINTERS *******************/

int setup_internal_arrays_and_array_pointers(dataptr dz)
{
	dz->array_cnt  = 1; 
	if(dz->mode == 3) { 
		if((dz->parray  = (double **)malloc(sizeof(double *)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for internal double arrays.\n");
			return(MEMORY_ERROR);
		}
		dz->parray[0] = NULL;
	}
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


/******************************** USAGE1 ********************************/

int usage1(void)
{
	usage2("multi");
	return(USAGE_ONLY);
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	if(!strcmp(prog_identifier_from_cmdline,"multi"))				dz->process = STRANS;
	else {
		sprintf(errstr,"Unknown program identification string '%s'\n",prog_identifier_from_cmdline);
		return(USAGE_ONLY);
	}
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
	if(!strcmp(str,"multi")) {
		fprintf(stderr,
		"CHANGE SPEED & PITCH OF (MULTICHANNEL) SRC SOUND, OR ADD VIBRATO.\n\n"
		"USAGE: strans multi 1 infile outfile     speed             [-o]\n"
    	"OR:    strans multi 2 infile outfile     semitone-transpos [-o]\n"
    	"OR:    strans multi 3 infile outfile accel goaltime [-sstarttime]\n"
    	"OR:    strans multi 4 infile outfile vibfrq vibdepth\n"
		"\n"
		"SPEED             is a speed-multiplier\n"
		"SEMITONE-TRANSPOS is transposition in semitones\n"
		"    -o  brkpnt times read as outfile times (default: as infile times).\n"
		"\n"
		"ACCEL     (a speed multiplier) is the speed reached at the goaltime.\n"
		"GOALTIME  is the time at which the full acceleration is achieved.\n"
		"STARTTIME is the time at which acceleration begins (default 0.0).\n"
		"\n"
		"VIBFRQ    is the vibrato frequency in Hz (can vary over time).\n"
		"VIBDEPTH  is pitch-depth of vibrato (in semitones) (can vary over time).\n");
	} else
		fprintf(stdout,"Unknown option '%s'\n",str);
	return(USAGE_ONLY);
}

int usage3(char *str1,char *str2)
{
	fprintf(stderr,"Insufficient parameters on command line.\n");
	return(USAGE_ONLY);
}

/****************************** STRANS_PREPROCESS *********************************/

int strans_preprocess(dataptr dz)
{
	double acceltime, tabratio, *p, *pend;
	long n;
	switch(dz->mode) {
	case(0):
	case(1):
		dz->iparam[UNITSIZE] = dz->insams[0]/dz->infile->channels;
		dz->param[VTRANS_SR] = (double)dz->infile->srate;
		dz->iparam[TOTAL_UNITS_READ] = 0;
		break;
	case(2):
		acceltime = dz->param[ACCEL_GOALTIME] - dz->param[ACCEL_STARTTIME];		
    	if(acceltime < MINTIME_ACCEL) {
    		sprintf(errstr,"time for acceleration (%lf) must be greater than  %.3fsecs\n",acceltime,MINTIME_ACCEL);
			return(DATA_ERROR);
		}
	    dz->param[ACCEL_ACCEL] = pow(dz->param[ACCEL_ACCEL],(1.0/(acceltime *(double)dz->infile->srate)));
		dz->iparam[ACCEL_STARTTIME] = 	/* convert to time-in-samples */
		round(dz->param[ACCEL_STARTTIME] * (double)dz->infile->srate) * dz->infile->channels;
		break;
	case(3):
// TW : Mar 2015 NB (integer) iparam[1 = UNITSIZE] but (in mode 3) (float) param[1 = VIB_DEPTH] : NO CONFLICT
		dz->iparam[UNITSIZE] = dz->insams[0]/dz->infile->channels;
		dz->param[VTRANS_SR] = (double)dz->infile->srate;
// <-- TW : Mar 2015 
		tabratio = (double)VIB_TABSIZE/(double)dz->infile->srate;  	/* converts frq to sintable step */
		if(dz->brksize[VIB_FRQ]) {
			p    = dz->brk[VIB_FRQ] + 1;
			pend = dz->brk[VIB_FRQ] + (dz->brksize[VIB_FRQ] * 2);
			while(p < pend) {
				*p *= tabratio;
				p += 2;
			}
		} else
			dz->param[VIB_FRQ] *= tabratio;
		if(dz->brksize[VIB_DEPTH]) {								   /* converts semitones to 8vas */
			p    = dz->brk[VIB_DEPTH] + 1;
			pend = dz->brk[VIB_DEPTH] + (dz->brksize[VIB_DEPTH] * 2);
			while(p < pend) {
				*p *= OCTAVES_PER_SEMITONE;
				p += 2;
			}
		} else
			dz->param[VIB_DEPTH] *= OCTAVES_PER_SEMITONE;
		if((dz->parray[VIB_SINTAB] = (double *)malloc((VIB_TABSIZE + 1) * sizeof(double)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for vibrato sintable.\n");
			return(MEMORY_ERROR);
		}
		tabratio = TWOPI/(double)VIB_TABSIZE;			   /* converts table-step to fraction of 2PI */
		for(n=0;n<VIB_TABSIZE;n++)
			dz->parray[VIB_SINTAB][n] = sin((double)n * tabratio);
		dz->parray[VIB_SINTAB][n] = 0.0;		
		break;
	}
	return FINISHED;
}

/****************************** SET_LEGAL_INTERNALPARAM_STRUCTURE *********************************/

int set_legal_internalparam_structure(int process,int mode,aplptr ap)
{
	int exit_status = FINISHED;
	switch(mode) {
	case(2):	exit_status = set_internalparam_data(  "iiid",ap);	break;
	case(3):	exit_status = set_internalparam_data( "iiiid",ap);	break;
	default:
		exit_status = set_internalparam_data("iiiiid",ap);
		break;
	}
	return exit_status;
}

/****************************** VTRANS_PROCESS *************************/

int process_varispeed(dataptr dz)
{
	int exit_status, chans = dz->infile->channels;
	long n, m, chunksread, place, thispos;
	double *dbrk, flplace = 0.0, fracsamp, step, interp;
	float *ibuf = dz->sampbuf[0], *obuf = dz->sampbuf[1], *obufend = dz->sampbuf[2], *obufptr;
	long obufcnt = 0;
	if(dz->brksize[VTRANS_TRANS]) {
		if((exit_status = convert_brkpnts(dz))<0)
			return(exit_status);
		if(dz->vflag[VTRANS_OUTTIMES]) {	/* convert to intimes */
			if((exit_status = change_frame(dz))<0)
				return(exit_status);
		}
	} else if(dz->mode == MOD_TRANSPOS_SEMIT)
		dz->param[VTRANS_TRANS] = pow(2.0,(dz->param[VTRANS_TRANS] * OCTAVES_PER_SEMITONE));

	display_virtual_time(0L,dz);
	if((exit_status = read_samps_for_strans(dz))<0)
		return(exit_status);
	if(dz->brksize[VTRANS_TRANS]) {
		dbrk = dz->brk[VTRANS_TRANS];
		for(n=1,m=2;n<dz->brksize[VTRANS_TRANS];n++,m+=2) {
			if((exit_status = 
				timevents(&obufcnt,(long)round(dbrk[m-2]),(long)round(dbrk[m]),dbrk[m-1],dbrk[m+1],dz))!=CONTINUE)
	    		break;
		}
		if(exit_status < 0)
			return(exit_status);
		return write_samps_with_intime_display(dz->sampbuf[1],obufcnt,dz);
	} else {
		obufptr = obuf;
		while(dz->ssampsread > 0) {
			chunksread = dz->ssampsread/chans;
			while(flplace < (double)chunksread) {
				place    = (int)flplace;
				fracsamp = flplace - (double)place;
				thispos = place * chans;
				for(n=0;n<chans;n++)  {
					step = ibuf[thispos + chans] - ibuf[thispos];
					interp   = fracsamp * step;
					*obufptr = (float)(ibuf[thispos] + interp);
					obufptr++;
					thispos++;
				}
				if(obufptr >= obufend) {
					if((exit_status = write_samps_with_intime_display(obuf,dz->buflen,dz))<0)
						return(exit_status);
					obufptr = obuf;
				}
				flplace   += dz->param[VTRANS_TRANS];
			}
			if((exit_status = read_samps_for_strans(dz))<0)
				return(exit_status);
			chunksread = dz->ssampsread/chans;
			flplace -= (double)(dz->buflen/chans);
		}
		if(obufptr!=obuf)					/* 12 */
			return write_samps_with_intime_display(obuf,obufptr - obuf,dz);
	}
	return FINISHED;
}

/*************************** READ_SAMPS_FOR_STRANS **************************/

int read_samps_for_strans(dataptr dz)
{
//TW MUST READ EXTRA SAMPLE FOR EACH CHANNEL to get wrap-around value
	float *buffer = dz->sampbuf[0];
	long samps_read, samps_to_read = dz->buflen + dz->infile->channels;
	dz->iparam[UNITS_RD_PRE_THISBUF] = dz->iparam[TOTAL_UNITS_READ];
	if((samps_read = fgetfbufEx(buffer, samps_to_read,dz->ifd[0],0)) < 0) {
		sprintf(errstr, "Can't read from input soundfile.\n");
		return(SYSTEM_ERROR);
	}
	dz->samps_left -= samps_read;
	if(samps_read == samps_to_read) {
		if((sndseekEx(dz->ifd[0],-(long)dz->infile->channels,1))<0) { /* WE'VE READ EXTRA SECTOR,for wrap-around */
			sprintf(errstr,"sndseekEx() failed.\n");
			return(SYSTEM_ERROR);
		}
		samps_read -= dz->infile->channels;
		dz->samps_left += dz->infile->channels;
	}
	dz->ssampsread = samps_read;
	dz->total_samps_read 		 += samps_read;
	dz->iparam[UNITS_READ]        = samps_read/dz->infile->channels;
	dz->iparam[TOTAL_UNITS_READ] += dz->iparam[UNITS_READ];
	return(FINISHED);
}

/************************ CONVERT_BRKPNTS ************************/

int convert_brkpnts(dataptr dz)
{
	int exit_status;
	double *p, *pend;
	if((exit_status= force_value_at_zero_time(VTRANS_TRANS,dz))<0)
		return(exit_status);
	if((exit_status= force_value_at_end_time(VTRANS_TRANS,dz))<0)
		return(exit_status);
	p    = dz->brk[VTRANS_TRANS];
	pend = p + (dz->brksize[VTRANS_TRANS] * 2);

	while(p < pend) {
		*p = (double)round((*p) * dz->param[VTRANS_SR]);	 /* convert to (mono) sample frame */
		p += 2;
	}
	if(dz->mode==MOD_TRANSPOS_SEMIT) {
		p  = dz->brk[VTRANS_TRANS] + 1;		
		while(p < pend) {
			*p = pow(2.0,(*p) * OCTAVES_PER_SEMITONE);	/* convert to ratios */
			p += 2;
		}
	}
	return(FINISHED);
}

/************************ CHANGE_FRAME ************************/

int change_frame(dataptr dz)
{
	long n, m;
	long *newtime;
	double *dbrk, lasttime, lastval, thistime, thisval; 
	dbrk = dz->brk[VTRANS_TRANS];
	if((newtime = (long *)malloc(dz->brksize[VTRANS_TRANS] * 2 * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create transformed brktable.\n");
		return(MEMORY_ERROR);
	}
	newtime[0] = 0;
	lasttime = dbrk[0];
	lastval  = dbrk[1];
	for(n=1,m=2;n<dz->brksize[VTRANS_TRANS];n++,m+=2) {
		thistime = dbrk[m]; 
		thisval  = dbrk[m+1]; 
		newtime[n] = newtime[n-1] + 
				 inv_cntevents((long)round(thistime - lasttime),lastval,thisval);
		lasttime = thistime;
		lastval  = thisval;
	}
	for(n=1,m=2;n<dz->brksize[VTRANS_TRANS];n++,m+=2)
		dbrk[m] = (double)newtime[n];
	free(newtime);
	return(FINISHED);
}

/*************************** INV_CNTEVENTS *****************************
 *
 *	no. of event in input (N) corresponding to output (T1-T0) given by :-
 *
 *		N = (T1-T0) log S1
 *		    -------    e__
 *		    (S1-S0)     S0
 *
 *      Hence, 
 *	no. of event in output (T1-T0) corresponding to input (N) given by :-
 *
 *		(S1-S0)N = (T1-T0)
 *		--------
 *		 log S1
 *		    e__
 *		     S0
 *
 *	Except where S1==S0, when (T1-T0) = S * N
 */

long inv_cntevents(long dur,double s0,double s1)
{   
	double ftime;
	long time;
	if(sizeq(s1,s0))
		ftime = (double)dur * s0;
	else
		ftime = ((double)dur * (s1-s0))/log(s1/s0);
	time = round(ftime);
	if(time==0)
		time = 1;
	return(time);
}

/*************************** FORCE_VALUE_AT_END_TIME *****************************/

int force_value_at_end_time(int paramno,dataptr dz)
{
	double duration = (double)dz->iparam[UNITSIZE]/dz->param[VTRANS_SR];
	double *p = dz->brk[paramno] + ((dz->brksize[paramno]-1) * 2);
	double timegap, newgap, step, newstep, timeratio;
	if(dz->brksize[paramno] < 2) {
		sprintf(errstr,"Brktable too small (< 2 pairs).\n");
		return(DATA_ERROR);
	}
	if(flteq(*p,duration)) {
		*p = duration;
		return(FINISHED);
	} else if(*p <  duration) {
		dz->brksize[paramno]++;
		if((dz->brk[paramno] = realloc((char *)dz->brk[paramno],dz->brksize[paramno] * 2 * sizeof(double)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY TO GROW BRKTABLE.\n");
			return(MEMORY_ERROR);
		}
		p = dz->brk[paramno] + ((dz->brksize[paramno]-1) * 2);
		*p = duration;
		*(p+1) = *(p-1);
		return(FINISHED);
	} else {  /* *p >  duration) */
		while(p > dz->brk[paramno]) {
			p -= 2;
			if(flteq(*p,duration)) {
				*p = duration;
				dz->brksize[paramno] = (p - dz->brk[paramno] + 2)/2;
				return(FINISHED);
			} else if(*p < duration) {
				timegap = *(p+2) - *p;
				newgap = duration - *p;
				timeratio = newgap/timegap;
				switch(dz->mode) {
				case(MOD_TRANSPOS_SEMIT):
					step = *(p+3) - *(p+1);
					newstep = step * timeratio;
					*(p+3) = *(p+1) + newstep;
					break;
				case(MOD_TRANSPOS):
					step    = LOG2(*(p+3)) - LOG2(*(p+1));
					newstep = pow(2.0,step * timeratio);
					*(p+3) = *(p+1) * newstep;
					break;										
				}
				*(p+2) = duration;
				dz->brksize[paramno] = (p - dz->brk[paramno] + 4)/2;
				return(FINISHED);
			}
		}
	}
	sprintf(errstr,"Failed to place brkpnt at snd endtime.\n");
	return(PROGRAM_ERROR);
}

/****************************** PUTVAL ******************************/

int putval(long *obufcnt,double pos,dataptr dz)
{
	int exit_status;
	long i = (long)pos;	/* TRUNCATE */
	double frac = pos - (double)i, diff; 
	float val1, val2;
	float *buffer = dz->sampbuf[0];
	float *obuf   = dz->sampbuf[1];
	int chans = dz->infile->channels;
	int n;
	i *= chans;
	for(n=0;n<chans;n++) {
		val1 = buffer[i];
		val2 = buffer[i+chans];
		diff = (double)(val2-val1);
//TW	obuf[(*obufcnt)++] = val1 + (float)/*round*/(frac * diff);
		obuf[(*obufcnt)++] = (float)(val1 + (frac * diff));
		if(*obufcnt >= dz->buflen) {
			if((exit_status = write_samps_with_intime_display(obuf,dz->buflen,dz))<0)
				return(exit_status);
			*obufcnt = 0;
		}
		i++;	
	}
	return(CONTINUE);
}

/**************************** SIZEQ *******************************/

#define CALCLIM 0.02

int sizeq(double f1,double f2)
{   
	double upperbnd, lowerbnd;
	upperbnd = f2 + CALCLIM;
	lowerbnd = f2 - CALCLIM;
	if((f1>upperbnd) || (f1<lowerbnd))
		return(FALSE);
	return(TRUE);
}

/*************************** SPLICE_END *****************************/

void splice_end(long obufcnt,dataptr dz)		/* A kludge to avoid clicks at end */
{

#define VTRANS_SPLICELEN (5.0)

	float *buf = dz->sampbuf[1];
	int chans = dz->infile->channels;
	int splicelen = round(VTRANS_SPLICELEN * MS_TO_SECS * dz->infile->srate) * dz->infile->channels;
	int n, m, k = min(obufcnt,splicelen);
	long startsamp = obufcnt - k;
	double inv_k;
	k /= chans;
	inv_k = 1.0/(double)k;
	for(n = k-1;n >= 0;n--) {
		for(m=0;m<chans;m++) {
			buf[startsamp] = (float)(buf[startsamp] * n * inv_k);
			startsamp++;
		}
	}
}
	
/*************************** WRITE_SAMPS_WITH_INTIME_DISPLAY ***********************/

int write_samps_with_intime_display(float *buf,int samps_to_write,dataptr dz)
{   
	int exit_status;
	int samps_written;
	if(samps_to_write > 0) {
		if((exit_status = write_samps_no_report(buf,samps_to_write,&samps_written,dz))<0) {
			sprintf(errstr, "Can't write to output soundfile.\n");
			return(SYSTEM_ERROR);
		}
		display_virtual_time(dz->total_samps_read,dz);
	}
	return(FINISHED);
}

/************************** TIMEVENTS *******************************
 *
 * 	Generates event positions from startsize and end size
 *
 *	Let start-time be T0 and end time be T1
 *	Let start size be S0 and end size be S1
 *
 *	number of event is given by :-
 *
 *		N = (T1-T0) log S1
 *		    -------    e__
 *		    (S1-S0)     S0
 *
 *	In general this will be non-integral, and we should
 *	round N to an integer, and recalculate S1 by successive 
 *	approximation.
 *
 *	Then positions of events are given by
 *	
 *	    for X = 0 to N		 (S1-S0)
 *					  ----- X
 *	T = (S1T0 - S0T1)   S0(T1 - T0)  (T1-T0)
 *	 s   -----------  + ----------  e 
 *	      (S1 - S0)      (S1 - S0)
 *
 * 	If difference in eventsizes input to the main program is very small
 *
 * This function calculates the number of events within the time-interval,
 * and generates the times of these events.
 */

int timevents(long *obufcnt,long intime0,long intime1,double insize0,double insize1,dataptr dz)
{
	int exit_status;
	long   number;
	double fnum, fnumber, error, pos;
	double lobound, hibound;
	long duration;
	if(flteq(insize0,0.0) || flteq(insize1,0.0)) {
		sprintf(errstr,"Event size of zero encountered in pair %lf %lf at time %lf\n",
		insize0,insize1,(double)intime0/dz->param[VTRANS_SR]);
		return(DATA_ERROR);
	}
	duration = intime1-intime0;
	if(duration<=0) {
		sprintf(errstr,"Inconsistent input times [ %lf : %lf ] to timevents()\n",
		(double)intime0/dz->param[VTRANS_SR],(double)intime1/dz->param[VTRANS_SR]);
		return(DATA_ERROR);
	}
	if(sizeq(insize1,insize0))				/* 1 */
		return(samesize(obufcnt,intime0,insize0,insize1,duration,dz));

	fnum =cntevents(duration,insize0,insize1);		/* 2 */
	number = round(fnum);
	if(number<2) {					/* 3 */
		pos = (double)(intime0 - dz->iparam[UNITS_RD_PRE_THISBUF]);
		if(pos >= (double)dz->iparam[UNITS_READ]) {
			if(dz->samps_left <= 0)
				return(FINISHED);
			if((exit_status = read_samps_for_strans(dz))<0)
				return(exit_status);
			pos -= (double)dz->iparam[UNIT_BUFLEN];
		}
		if((exit_status = putval(obufcnt,pos,dz))<0)
			return(exit_status);
	} else {
		fnumber = (double)number;			/* 4 */
		error   = fabs(fnum - fnumber);
/* HIBOUND is a SMALLER value of SIZE to give BIGGER value of NUM-OF-SEGS */
/* LOBOUND is a LARGER value of SIZE to give SMALLER value of NUM-OF-SEGS */
		lobound = max(insize1,insize0);			/* 5 */
		hibound = min(insize1,insize0);
		if(error > FLTERR)				/* 6 */
			insize1 = refinesize(hibound,lobound,fnumber,duration,error,insize0);
		else
			insize1 = (hibound+lobound)/2;
		return(getetime(obufcnt,intime0,intime1,insize0,insize1,number,dz));
    }							/* 7 */
	return(CONTINUE);
}

/*************************** CNTEVENTS *****************************/

double cntevents(long dur,double s0,double s1)
{   
	double f1 = (double)dur,f2;
	if(sizeq(s1,s0))
		return((f1*2.0)/(s1+s0));
	f2  = s1-s0;
	f1 /= f2;
	f2  = s1/s0;
	f2  = log(f2);
	f1 *= f2;
	return(fabs(f1));
}

/******************************* SAMESIZE(): *******************************
 *
 * get event positions, if eventsize approx same throughout segment.
 *
 *(1)	Get average size, find number of events and round to nearest int.
 *(2)	If too many or zero events, do exception.
 *(3)	Recalculate size, and thence event times.
 */

int samesize(long *obufcnt,long intime0,double insize0,double insize1,long duration,dataptr dz)
{   
	int exit_status;
	long number, isize;
	double size, pos;					/* 1 */
	size     = (insize0+insize1)/2;
	number   = round((double)duration/size);
	if(number==0) {					/* 2 */
		pos = (double)(intime0 - dz->iparam[UNITS_RD_PRE_THISBUF]);
		if(pos >= (double)dz->iparam[UNITS_READ]) {
			if(dz->samps_left <= 0)
				return(FINISHED);
			if((exit_status = read_samps_for_strans(dz))<0)
				return(exit_status);
			pos -= (double)dz->iparam[UNIT_BUFLEN];
		}
		if((exit_status = putval(obufcnt,pos,dz))<0)
			return(exit_status);
		return(CONTINUE);
	}
	size = (double)duration/(double)number;
	if(flteq((double)(isize = (long)round(size)),size))
		return integral_times(obufcnt,intime0,isize,number,dz);
	return unvarying_times(obufcnt,intime0,size,number,dz);
}

/******************************* GETETIME ********************************
 *
 * Calculate time-positions of events that vary in size between s0 and s1.
 * NB We need to invert the time order, if s1 < s0.
 */

int getetime(long *obufcnt,long t0,long t1,double s0,double s1,long number,dataptr dz)
{   
	int exit_status;
	long n;
	double sdiff = s1-s0, tdiff = (double)(t1-t0), d1, d2, d3, pos;
 /***** TW n=0 NOT 1 ********/
	for(n=1;n<number;n++)   {
/***** TW ********/
		d1    = sdiff/tdiff;
		d1   *= (double)n;
		d1    = exp(d1);
		d2    = s0*tdiff;
		d2   /= sdiff;
		d1   *= d2;
		d2    = s1*t0;
		d3    = s0*t1;
		d2   -= d3;
		d2   /= sdiff;
		pos   = d1 + d2;
		pos -= (double)dz->iparam[UNITS_RD_PRE_THISBUF];
		if(pos >= (double)dz->iparam[UNITS_READ]) {
			if(dz->samps_left <= 0)
				return(FINISHED);
			if((exit_status = read_samps_for_strans(dz))<0)
				return(exit_status);
			pos -= (double)dz->iparam[UNIT_BUFLEN];
		}
		if((exit_status = putval(obufcnt,pos,dz))<0)
			return(exit_status);
	}
	return(CONTINUE);
}

/******************************* DZ2PROPS ********************************/

void dz2props(dataptr dz, SFPROPS* props)
{
	if(dz->outfiletype == SNDFILE_OUT){
		switch(dz->true_outfile_stype){
		case SAMP_SHORT:
			props->samptype = SHORT16;
			break;
		case SAMP_FLOAT:
			props->samptype = FLOAT32;
			break;
		case SAMP_BYTE:
			props->samptype = SHORT8;
			break;
		case SAMP_LONG:
			props->samptype = INT_32;
			break;
		case SAMP_2424:
			props->samptype = INT2424;
			break;
		case SAMP_2432:
			props->samptype = INT2432;
			break;
		case SAMP_2024:
			props->samptype = INT2024;
			break;
		default:
			props->samptype = INT_MASKED;
			break;
		}
		props->srate = dz->outfile->srate;
		props->chans = dz->outchans; //or outfile->channels?
		props->type = wt_wave;
		props->format = WAVE_EX;
		props->chformat = MC_STD;
		
	}
}

/*************************** CREATE_STRANSBUFS **************************/

int create_stransbufs(dataptr dz)
{
	long bigbufsize, xs;
	if(dz->sbufptr == 0 || dz->sampbuf==0) {
		sprintf(errstr,"buffer pointers not allocated: create_sndbufs()\n");
		return(PROGRAM_ERROR);
	}
	bigbufsize = (long)Malloc(-1);
	bigbufsize /= dz->bufcnt;
	if(bigbufsize <=0) {
		bigbufsize  =  F_SECSIZE * sizeof(float);	  	  /* RWD keep ths for now */

	}
	dz->buflen = bigbufsize / sizeof(float);	
	/*RWD also cover n-channels usage */
	dz->buflen = (dz->buflen / dz->infile->channels)  * dz->infile->channels;
	bigbufsize = dz->buflen * sizeof(float);
	xs = dz->infile->channels * sizeof(float); /* wraparound points */
	if((dz->bigbuf = (float *)malloc((bigbufsize * dz->bufcnt) + xs)) == NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
		return(PROGRAM_ERROR);
	}
	dz->sampbuf[0] = dz->sbufptr[0] = dz->bigbuf;
	dz->sampbuf[1] = dz->sbufptr[1] = dz->bigbuf + dz->buflen + dz->infile->channels;
	dz->sampbuf[2] = dz->sampbuf[1] + dz->buflen;
// TW : Mar 2015: iparam[2 = UNIT_BUFLEN] SET BELOW, but in Mode 2, iparam[2 = ACCEL_STARTTIME in samples] : CONFLICT
// TW : Mar 2015: BUT: UNIT_BUFLEN not used in Mode 2 SO THIS RESOLVES CONFLICT
	if(dz->mode != 2)
		dz->iparam[UNIT_BUFLEN] = dz->buflen/dz->infile->channels;
	return(FINISHED);
}

/***************************** REFINESIZE(): ******************************
 *
 * refine size of final event to reduce error within bounds.
 */

double refinesize(double hibound,double lobound,double fnumber,long duration,double error,double insize0)
{   
	double size, fnum, lasterror;
	double flterr_squared = FLTERR * FLTERR;
	do {
		lasterror = error;
		size  = (hibound+lobound)/2.0;
		fnum  = cntevents(duration,insize0,size);
		error = fabs(fnumber-fnum);
		if(error>FLTERR)  {
			if(fnum<fnumber)
				lobound = size;
			else
				hibound = size;
		}
		if(fabs(lasterror - error) < flterr_squared)
			break;	/* In case error cannot be reduced */
	} while(error > FLTERR);
	return(size);
}

/************************ INTEGRAL_TIMES ***************************
 *
 * GENERATE vals for equally spaced events, spaced by whole nos. of samps.
 */

int integral_times(long *obufcnt,long intime0,long isize,long number,dataptr dz)
{
	int exit_status;
	long k, pos = intime0 - dz->iparam[UNITS_RD_PRE_THISBUF];
	for(k=0;k<number;k++) {
		if(pos >= dz->iparam[UNITS_READ]) {
			if(dz->samps_left <= 0)
				return(FINISHED);
			if((exit_status = read_samps_for_strans(dz))<0)
				return(exit_status);
			pos -= dz->iparam[UNIT_BUFLEN];
		}
		if((exit_status = putintval(obufcnt,pos,dz))<0)
			return(exit_status);
		pos += isize;
	}
	return(CONTINUE);
}

/****************************** PUTINTVAL ******************************/

int putintval(long *obufcnt,long i,dataptr dz)
{  
	int exit_status;
	int n;
	float *buffer = dz->sampbuf[0];
	float *obuf   = dz->sampbuf[1];
	i *= dz->infile->channels;
	for(n=0;n<dz->infile->channels;n++) {
		obuf[(*obufcnt)++] = buffer[i];
		if(*obufcnt >= dz->buflen) {
			if((exit_status = write_samps_with_intime_display(obuf,dz->buflen,dz))<0)
				return(exit_status);
			*obufcnt = 0;
		}
		i++;	
	}
	return(FINISHED);
}

/************************ UNVARYING_TIMES ***************************
 *
 * GENERATE vals for equally spaced events.
 */

int unvarying_times(long *obufcnt,long intime0,double size,long number,dataptr dz)
{   
	int exit_status;
	long k; 
	double pos = (double)(intime0 - dz->iparam[UNITS_RD_PRE_THISBUF]);
	for(k=0;k<number;k++) {
		if(pos >= (double)dz->iparam[UNITS_READ]) {
			if(dz->samps_left <= 0)
				return(FINISHED);
			if((exit_status = read_samps_for_strans(dz))<0)
				return(exit_status);
			pos -= (double)dz->iparam[UNIT_BUFLEN];
		}
		if((exit_status = putval(obufcnt,pos,dz))<0)
			return(exit_status);
		pos += size;
	}
    return(CONTINUE);
}

/******************************* DO_ACCELERATION ***************************/

int do_acceleration(dataptr dz)
{	
	int exit_status;
	long place, chunksread, n, thisplace, nextplace;
	double flplace, fracsamp, step;
	int OK = 1, chans = dz->infile->channels;
	double interp;
	float *ibuf = dz->sampbuf[0];
	float *obuf = dz->sampbuf[1], *obufptr, *obufend = obuf + dz->buflen;
	double place_inc = 1.0;
	long   startsamp = dz->iparam[ACCEL_STARTTIME];
	long   previous_total_ssampsread = 0;
	double accel     = dz->param[ACCEL_ACCEL];
	dz->total_samps_read = 0;
	display_virtual_time(0L,dz);
	if((exit_status = read_samps_for_strans(dz))<0)
		return(exit_status);

	while(dz->total_samps_read <= startsamp) {
		if((exit_status = write_samps_with_intime_display(ibuf,dz->ssampsread,dz))<0)
			return(exit_status);
		previous_total_ssampsread = dz->total_samps_read;
		if((exit_status = read_samps_for_strans(dz))<0)
			return(exit_status);
	}
	startsamp -= previous_total_ssampsread;
	if(startsamp > 0)
		memcpy((char *)obuf,(char *)ibuf,startsamp * sizeof(float));
	obufptr = obuf + startsamp;
	chunksread = dz->ssampsread/chans;
	place = startsamp/chans;
	flplace = place;
	for(;;) {
		while(chunksread > place) {
			while(place < chunksread) {
				fracsamp   = flplace - (double)place;
				for(n=0;n < chans; n++) {
					thisplace = (place * chans) + n;
					nextplace = ((place+1) * chans) + n;
					step = ibuf[nextplace] - ibuf[thisplace];
					interp     = fracsamp * step;
					*obufptr++ = (float)(ibuf[thisplace] + interp);
				}
				if(obufptr >= obufend) {
					if((exit_status = write_samps_with_intime_display(obuf,dz->buflen,dz))<0)
						return(exit_status);
					obufptr = obuf;
				}
				flplace   += place_inc;
				place_inc *= accel;
				if(place_inc <= MININC) {
					fprintf(stdout, "INFO: Acceleration reached black hole! - finishing\n");
					fflush(stdout);
					OK = 0;
					break;
				}
				place = (int)flplace;
			}
			if(!OK)
				break;
		}
		if(dz->samps_left > 0) {
			flplace -= (double)chunksread;
			place   -= chunksread;
			if((exit_status = read_samps_for_strans(dz))<0)
				return(exit_status);
			if((chunksread = dz->ssampsread/chans) <= 0)
				break;
		} else
			break;
	}
	splice_end(obufptr - obuf,dz);
	return write_samps_with_intime_display(obuf,obufptr - obuf ,dz);  
}

/******************************* DO_VIBRATO ********************************/

int do_vibrato(dataptr dz)
{
	int    exit_status;
	float  *ibuf      = dz->sampbuf[0];
	float  *obuf      = dz->sampbuf[1];
	float  *obufend   = dz->sampbuf[1] + dz->buflen;
	double *sintab = dz->parray[VIB_SINTAB];
	int    chans    = dz->infile->channels;
	double sr       = (double)dz->infile->srate;
	double tabsize  = (double)VIB_TABSIZE;
	double bloksize = (double)VIB_BLOKSIZE;
	double inv_bloksize = 1.0/bloksize;
	double thistime = 0.0;
	double time_incr = bloksize/sr;	/* timestep  between param reads */
	int    effective_buflen = dz->buflen/chans;
	long   place   = 0;				/* integer position in sndfile   */
	double flplace = 0.0;			/* float position in sndfile     */
	double incr;					/* incr of position in sndfile   */
	double sfindex = 0.0;			/* float position in sintable    */
	double sinval;					/* current sintable value        */
	double current_depth, lastdepth, depth_incr;
	double current_frq,   lastfrq,   frq_incr;
	double fracsamp;				/* fraction of samptime to interp*/
	double step;					/* step between snd samples      */
	double interp;					/* part of sampstep to use   	 */
	int    blokcnt = 0;				/* distance through blokd params */
	float  *obufptr = obuf;
	long chunksread, thisplace, nextplace, n;
	if(dz->brksize[VIB_FRQ]) {
		if((exit_status = read_value_from_brktable(thistime,VIB_FRQ,dz))<0)
			return(exit_status);
	} else
		frq_incr = 0.0;	
	if(dz->brksize[VIB_DEPTH]) {
		if((exit_status = read_value_from_brktable(thistime,VIB_DEPTH,dz))<0)
			return(exit_status);
	} else
		depth_incr = 0.0;	
	current_frq   = lastfrq   = dz->param[VIB_FRQ];
	current_depth = lastdepth = dz->param[VIB_DEPTH];
	thistime += time_incr;						  /* now time is 1 bloklength ahead */

	if((exit_status = read_samps_for_strans(dz))<0)
		return(exit_status);

	if((exit_status = read_values_from_all_existing_brktables(thistime,dz))<0)
		return(exit_status);						/* read value 1 bloklength ahead */
	frq_incr   = (dz->param[VIB_FRQ]   - lastfrq)   * inv_bloksize;
	depth_incr = (dz->param[VIB_DEPTH] - lastdepth) * inv_bloksize;
	lastfrq   = dz->param[VIB_FRQ];
	lastdepth = dz->param[VIB_DEPTH];
	blokcnt = VIB_BLOKSIZE - 1;		  /* -1 as we've read 1 (chancnt) sample already. */
	thistime += time_incr;						  /* Now time is 2 bloklengths ahead */
								/* but we won't read till another 1 blok has passed. */
	chunksread = dz->ssampsread / chans;
	memset((char *)obuf,0,dz->buflen * sizeof(float));
	for(;;) {
		if(--blokcnt <= 0) {	/* look at params only every BLOKCNT(*chans)samples */
			if((exit_status = read_values_from_all_existing_brktables(thistime,dz))<0)
				return(exit_status); /* & read vals 1 bloklen ahead,to calc par incr */
			frq_incr   = (dz->param[VIB_FRQ]   - lastfrq)   * inv_bloksize;
			depth_incr = (dz->param[VIB_DEPTH] - lastdepth) * inv_bloksize;
			lastfrq   = dz->param[VIB_FRQ];
			lastdepth = dz->param[VIB_DEPTH];
			blokcnt = VIB_BLOKSIZE;
			thistime += time_incr;
		}
		current_depth += depth_incr;	 	  /* increment params sample by (stereo) sample */
		current_frq   += frq_incr;
		if((sfindex  += current_frq) >= tabsize)
			sfindex -= tabsize;		 		 /* advance in sintable,wrapping around if ness */
		sinval = interp_read_sintable(sintab,sfindex);		/* read sintab by interpolation */
		incr = pow(2.0,sinval * current_depth); /* convert speed & depth from 8vas to ratio */
		if((flplace += incr) >= (double)chunksread) {			/* advance through src sound */
			if(dz->samps_left <=0)							    /* if at end of src, finish */
				break;						 /* else if at end of buffer, get more sound in */
			if((exit_status = read_samps_for_strans(dz))<0)
				return(exit_status);
			flplace -= effective_buflen;
		}
		place 	   = (long)flplace; 	/* TRUNCATE */	   /* read sndfile by interpolation */
		fracsamp   = flplace - (double)place;
		for(n=0;n<chans;n++) {
			thisplace = (place * chans) + n;
			nextplace = ((place+1) * chans) + n;
			step       = ibuf[nextplace] - ibuf[thisplace];
			interp     = fracsamp * step;
			*obufptr++ = (float)(ibuf[thisplace] + interp);
		}
		if(obufptr >= obufend) {					 /* if at end of outbuf, write a buffer */
			if((exit_status = write_samps_with_intime_display(obuf,dz->buflen,dz))<0)
				return(exit_status);
			memset((char *)obuf,0,dz->buflen * sizeof(float));
			obufptr = obuf;
		}
	}
	if(obufptr > obuf)								/* if samples remain in outbuf, write them */
		return write_samps_with_intime_display(obuf,obufptr - obuf,dz);
	return(FINISHED);
}

/******************************* INTERP_READ_SINTABLE ********************************/

double interp_read_sintable(double *sintab,double sfindex)
{
	int    sindex0  = (int)sfindex;	/* TRUNCATE */			
	int    sindex1  = sindex0 + 1;
	double frac = sfindex - (double)sindex0;
	double val0 = sintab[sindex0];
	double val1 = sintab[sindex1];
	double valdiff = val1 - val0;
	double valfrac = valdiff * frac;
	return(val0 + valfrac);
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
