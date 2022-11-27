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
#include <extdcon.h>
#include <flags.h>

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
static int setup_freeze_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_freeze_param_ranges_and_defaults(dataptr dz);
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
static int  read_the_input_snd(dataptr dz);
static int get_next_writestart(int write_start,dataptr dz);

static int iterate(int cnt,int pass,double *gain,double *pshift,
				int write_end,int local_write_start,int inmsampsize,double level,double *maxsamp,int iterating,dataptr dz);
static int iter(int cnt,int passno, double *gain,int local_write_start,int inmsampsize,double level,double *maxsamp,int iterating,dataptr dz);
static int iter_stereo(int cnt,int passno, double *gain,int local_write_start,int inmsampsize,double level,double *maxsamp,int iterating,dataptr dz);
static int iter_shift_interp(int cnt,int passno, double *gain,double *pshift,int local_write_start,int inmsampsize,double level,double *maxsamp,int iterating,dataptr dz);
static int iter_shift_interp_stereo(int cnt,int passno, double *gain,double *pshift,int local_write_start,int inmsampsize,double level,double *maxsamp,int iterating,dataptr dz);
static int fixa_iter(int cnt,int passno,double *gain,int local_write_start,int inmsampsize,double level,double *maxsamp,int iterating,dataptr dz);
static int fixa_iter_stereo(int cnt,int passno,double *gain,int local_write_start,int inmsampsize,double level,double *maxsamp,int iterating,dataptr dz);
static int fixa_iter_shift_interp(int cnt,int passno,double *gain,double *pshift,int local_write_start,int inmsampsize,double level,double *maxsamp,int iterating,dataptr dz);
static int fixa_iter_shift_interp_stereo(int cnt,int passno,double *gain,double *pshift,int local_write_start,int inmsampsize,double level,double *maxsamp,int iterating,dataptr dz);

static double get_gain(dataptr dz);
static double get_pshift(dataptr dz);

static void setup_iter_process_type(int is_unity_gain,dataptr dz);
static void set_default_delays(dataptr dz);
static int get_minvalue_of_delay(double *mindelay,dataptr dz);
static int get_maxvalue_of_pscat(double *maxpscat,dataptr dz);
static int get_maxvalue_of_rand(double *maxrand,dataptr dz);
static int friterate_preprocess(dataptr dz);
static int create_friterbufs(double maxpscat,dataptr dz);
static double get_pshift(dataptr dz);
static double get_gain(dataptr dz);
static int fixa_iter_shift_interp_stereo(int cnt,int passno,double *gain,double *pshift,int local_write_start,int inmsampsize,double level,double *maxsamp,int iterating,dataptr dz);
static int fixa_iter_shift_interp(int cnt,int passno,double *gain,double *pshift,int local_write_start,int inmsampsize,double level,double *maxsamp,int iterating,dataptr dz);
static int fixa_iter_stereo(int cnt,int passno,double *gain,int local_write_start,int inmsampsize,double level,double *maxsamp,int iterating,dataptr dz);
static int fixa_iter(int cnt,int passno,double *gain,int local_write_start,int inmsampsize,double level,double *maxsamp,int iterating,dataptr dz);
static int iter_shift_interp_stereo(int cnt,int passno, double *gain,double *pshift,int local_write_start,int inmsampsize,double level,double *maxsamp,int iterating,dataptr dz);
static int iter_shift_interp(int cnt,int passno, double *gain,double *pshift,int local_write_start,int inmsampsize,double level,double *maxsamp,int iterating,dataptr dz);
static int iter_stereo(int cnt,int passno, double *gain,int local_write_start,int inmsampsize,double level,double *maxsamp,int iterating,dataptr dz);
static int iter(int cnt,int passno, double *gain,int local_write_start,int inmsampsize,double level,double *maxsamp,int iterating,dataptr dz);
static int iterate
(int cnt,int pass,double *gain,double *pshift,int write_end,int local_write_start,int inmsampsize,double level,double *maxsamp,int iterating,dataptr dz);
static int get_next_writestart(int write_start,dataptr dz);
static int read_the_input_snd(dataptr dz);
static int do_friteration(dataptr dz);


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
		// setup_particular_application =
		if((exit_status = setup_freeze_application(dz))<0) {
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
	if((exit_status = setup_freeze_param_ranges_and_defaults(dz))<0) {
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
//	check_param_validity_and_consistency....
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

	//create bufs later
	//param_preprocess()						redundant
	if((exit_status = friterate_preprocess(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = do_friteration(dz))<0) {
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
	int exit_status;
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
	if((exit_status = create_sized_outfile(filename,dz))<0)
		return(exit_status);
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

/************************* SETUP_GATE_APPLICATION *******************/

int setup_freeze_application(dataptr dz)
{
	int exit_status;
	aplptr ap;
	if((exit_status = establish_application(dz))<0)		// GLOBAL
		return(FAILED);
	ap = dz->application;
	// SEE parstruct FOR EXPLANATION of next 2 functions

	switch(dz->mode) {
	case(ITERATE_DUR):
		if((exit_status = set_param_data(ap,0   ,8,8,"dDDDDddd"))<0)
			return(FAILED);
		break;
	case(ITERATE_REPEATS):
		if((exit_status = set_param_data(ap,0	,8,8,"iDDDDddD"))<0)	
		break;
	}
	if((exit_status = set_vflgs(ap,""      ,0,"" ,"s" ,1,1,"i"))<0)
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

/************************* SETUP_FREEZE_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_freeze_param_ranges_and_defaults(dataptr dz)
{
	int exit_status;
	aplptr ap = dz->application;
	// set_param_ranges()
	ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
	// NB total_input_param_cnt is > 0 !!!
	if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
		return(FAILED);
	// get_param_ranges()
	switch(dz->mode) {
	case(ITERATE_DUR):
		ap->lo[0] 					= dz->duration;
		ap->hi[0] 					= BIG_TIME;
		ap->default_val[ITER_DUR]	= dz->duration * 2.0; 	
		break;
	case(ITERATE_REPEATS):
		ap->lo[ITER_REPEATS] 			= 1.0;
		ap->hi[ITER_REPEATS] 			= BIG_VALUE;
		ap->default_val[ITER_REPEATS] 	= 2.0;
		break;
	default:
		sprintf(errstr,"Unknown mode for ITERFREEZE: in setup_freeze_param_ranges_and_defaults()\n");
		return(PROGRAM_ERROR);
	}
	ap->lo[ITER_DELAY]			 = FLTERR;
	ap->hi[ITER_DELAY]			 = ITER_MAX_DELAY;
	ap->default_val[ITER_DELAY]  = dz->duration;
	ap->lo[ITER_RANDOM]			 = 0.0;
	ap->hi[ITER_RANDOM]			 = 1.0;
	ap->default_val[ITER_RANDOM] = 0.0;
	ap->lo[ITER_PSCAT]			 = 0.0;
	ap->hi[ITER_PSCAT]			 = ITER_MAXPSHIFT;	
	ap->default_val[ITER_PSCAT]  = 0.0;
	ap->lo[ITER_ASCAT]			 = 0.0;
	ap->hi[ITER_ASCAT]			 = 1.0;
	ap->default_val[ITER_ASCAT]  = 0.0;
	ap->lo[CHUNKSTART]			 = 0.0;
	ap->hi[CHUNKSTART]			 = dz->duration;
	ap->default_val[CHUNKSTART]  = 0.0;
	ap->lo[CHUNKEND]			 = 0.0;
	ap->hi[CHUNKEND]			 = dz->duration;
	ap->default_val[CHUNKEND]	 = dz->duration;
	ap->lo[ITER_RRSEED]			 = 0.0;
	ap->hi[ITER_RRSEED]			 = MAXSHORT;
	ap->default_val[ITER_RRSEED] = 0.0;
	ap->lo[ITER_LGAIN]			 = 0.25;
	ap->hi[ITER_LGAIN]			 = 4.0;
	ap->default_val[ITER_LGAIN] = 1.0;
	dz->maxmode = 2;
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
			if((exit_status = setup_freeze_application(dz))<0)
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

/****************************** SET_LEGAL_INTERNALPARAM_STRUCTURE *********************************/

int set_legal_internalparam_structure(int process,int mode,aplptr ap)
{
	return set_internalparam_data("diiii", ap);
}

/************************* redundant functions: to ensure libs compile OK *******************/

int assign_process_logic(dataptr dz)
{
	return(FINISHED);
}

void set_legal_infile_structure(dataptr dz)
{}

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
	usage2("freeze");
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
	if(!strcmp(prog_identifier_from_cmdline,"freeze"))				dz->process = ITERATE_EXTEND;
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
	if(!strcmp(str,"freeze")) {		
	    fprintf(stdout,
		"FREEZE A SEGMENT OF A SOUND BY ITERATION IN A FLUID MANNER\n\n"
		"USAGE: freeze freeze 1 infil outfil outduration\n"
		"     delay rand pscat ampcut start end adjust [-sseed]\n"
		"OR:    freeze freeze 2 infil outfil repetitions\n"
		"     delay rand pscat ampcut start end adjust [-sseed]\n"
		"delay   (average) delay between iterations: <= length of frozen segement.\n"
		"rand    delaytime-randomisation: Range 0 - 1\n"
		"pscat   max of random pitchshift of each iter: Range 0 - %.0lf semitones\n"
		"        e.g.  2.5 =  2.5 semitones up or down.\n"
		"ampcut  max of random amp-reduction on each iter: Range 0-1\n"
		"start   Time where frozen segment begins in original sound.\n"
		"end     Time where frozen segment ends in original sound.\n"
		"adjust  Adjust gain of frozen segment.\n"
		"seed	 the same seed-number will produce identical output on rerun,\n"
		"        (Default: (0) random sequence is different every time).\n",ITER_MAXPSHIFT);
	} else
		fprintf(stdout,"Unknown option '%s'\n",str);
	return(USAGE_ONLY);
}

int usage3(char *str1,char *str2)
{
	fprintf(stderr,"Insufficient parameters on command line.\n");
	return(USAGE_ONLY);
}

/****************************** DO_ITERATION *************************
 *
 * (1) First event is always copy of original.
 */

//TW COMPLETELY UPDATED FUNCTION : (flt-converted)
#define ACCEPTABLE_LEVEL 0.75

int do_friteration(dataptr dz)
{
	int    exit_status, iterating = 0;
	int   write_end, tail, cnt, arraysize = BIGARRAY;
	float *tailend;
	int    bufs_written, finished;
	double level, thistime, localmax = 0.0;
	int   out_sampdur = 0, inmsampsize;
	int   write_start, local_write_start;
	double one_over_sr = 1.0/(double)dz->infile->srate, maxsamp = 0.0;
	int    passno, is_penult = 0;
	int   k, orig_sampdel = 0, ii, jj, kk;
	double *gain, *pshift, gaingain;
	int   *wstart, chunkmsamps = 0, chunksampsize = 0;
	float	*orig_inbuf = dz->sampbuf[0];
	int splicelen = ITX_SPLICELEN;
	double spliceincr = 1.0/(double)ITX_SPLICELEN, splicer;

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
	chunksampsize = dz->iparam[CHUNKEND] - dz->iparam[CHUNKSTART];
	chunkmsamps = chunksampsize/dz->infile->channels;
	if(dz->mode==ITERATE_DUR)
		out_sampdur  = round(dz->param[ITER_DUR] * (double)dz->infile->srate) * dz->infile->channels;
	if(sloom) {
		switch(dz->mode) {
		case(ITERATE_DUR):		
			dz->tempsize = out_sampdur;		
			break;
		case(ITERATE_REPEATS):	
			dz->tempsize = dz->insams[0] + (chunksampsize * dz->iparam[ITER_REPEATS]);	/* approx */
			break; 
		}
	}
	for(passno=0;passno<2;passno++) {
		is_penult = 0;
		cnt = 0;
		bufs_written = 0;
		write_start = 0;
		maxsamp = 0.0;
		memset((char *)dz->sampbuf[1],0,dz->buflen * sizeof(float));
		level = 1.0;
		sndseekEx(dz->ifd[0],0L,0);
		display_virtual_time(0L,dz);
		fflush(stdout);
		dz->sampbuf[0] = orig_inbuf;
		if(passno > 0) {
			print_outmessage_flush("Second pass, for greater level\n");
			dz->tempsize = dz->total_samps_written;
			dz->total_samps_written = 0;
			memset((char *)dz->sampbuf[0],0,(dz->sampbuf[3] - dz->sampbuf[0]) * sizeof(float));
		}
		if((exit_status = read_the_input_snd(dz))<0)
			return(exit_status);
		/* Make bakup copy of infile, with splice at start of iter section */
		if(passno == 0) {
			for(jj = dz->iparam[CHUNKSTART]; jj < dz->iparam[CHUNKEND];jj++)
				localmax = max(localmax,dz->sampbuf[0][jj]);
			memcpy((char *)dz->sampbuf[3],(char *)dz->sampbuf[0],(dz->insams[0] * sizeof(float)));
			jj = dz->iparam[CHUNKSTART];
			splicer = 0.0;
			for(kk = 0; kk < splicelen; kk++) {
				for(ii=0; ii <dz->infile->channels;ii++) {
					dz->sampbuf[3][jj] = (float)(dz->sampbuf[3][jj] * splicer);
					jj++;
				}
				splicer += spliceincr;
			}
		}
		/*Put splice on END of iterated chunk in TRUE buffer */
		jj = dz->iparam[CHUNKEND] + dz->infile->channels - 1;
		for(ii=0; ii <dz->infile->channels;ii++) {
			dz->sampbuf[0][jj] = (float)0;		/* These are the wrap-around points */
			jj--;
		}
		splicer = 0.0;
		for(kk = 0; kk < splicelen; kk++) {
			for(ii=0; ii <dz->infile->channels;ii++) {
				dz->sampbuf[0][jj] = (float)(dz->sampbuf[0][jj] * splicer);
				jj--;
			}
			splicer += spliceincr;
		}
		inmsampsize = dz->iparam[CHUNKEND]/dz->infile->channels;
		/* 1 */
		local_write_start = 0;
		switch(dz->iparam[ITER_PROCESS]) {
		case(MONO):		      		
			iter(0,passno,gain,local_write_start,inmsampsize,level,&maxsamp,iterating,dz);		
			break;
		case(STEREO):	      		
			iter_stereo(0,passno,gain,local_write_start,inmsampsize,level,&maxsamp,iterating,dz);			 		
			break;
		case(MN_INTP_SHIFT):      	
			iter_shift_interp(0,passno,gain,pshift,local_write_start,inmsampsize,level,&maxsamp,iterating,dz);	 		
			break;
		case(ST_INTP_SHIFT):      	
			iter_shift_interp_stereo(0,passno,gain,pshift,local_write_start,inmsampsize,level,&maxsamp,iterating,dz);		
			break;
		case(FIXA_MONO):	      	
			fixa_iter(0,passno,gain,local_write_start,inmsampsize,level,&maxsamp,iterating,dz);			 		
			break;
		case(FIXA_STEREO):	      	
			fixa_iter_stereo(0,passno,gain,local_write_start,inmsampsize,level,&maxsamp,iterating,dz);		 		
			break;
		case(FIXA_MN_INTP_SHIFT): 	
			fixa_iter_shift_interp(0,passno,gain,pshift,local_write_start,inmsampsize,level,&maxsamp,iterating,dz); 		
			break;
		case(FIXA_ST_INTP_SHIFT): 	
			fixa_iter_shift_interp_stereo(0,passno,gain,pshift,local_write_start,inmsampsize,level,&maxsamp,iterating,dz); 
			break;
		}
		write_end   = dz->iparam[CHUNKEND];
		jj = dz->iparam[CHUNKSTART];	 /*Put splice on START of iterarted chunk */
		splicer = 0.0;
		for(kk = 0; kk < splicelen; kk++) {
			for(ii=0; ii <dz->infile->channels;ii++) {
				dz->sampbuf[0][jj] = (float)(dz->sampbuf[0][jj] * splicer);
				jj++;
			}
			splicer += spliceincr;
		}
		thistime = 0.0;
		if((exit_status = read_values_from_all_existing_brktables(thistime,dz))<0)
			return(exit_status);
		orig_sampdel = round(dz->param[ITER_DELAY] * (double)dz->infile->srate);
		dz->iparam[ITER_MSAMPDEL] = dz->iparam[CHUNKSTART]/dz->infile->channels + orig_sampdel;
		dz->iparam[ITER_MSAMPDEL] -= ITX_SPLICELEN * 2;
		if(passno==0)
			wstart[cnt] = dz->iparam[ITER_MSAMPDEL];
		write_start = wstart[cnt];
		local_write_start = write_start;
		finished = FALSE;
		for(;;) {
			if(is_penult) {			/* If previously got to end, break */
				finished = 1;
				break;
			}
			if(cnt==0) {			/* If 1st iteration, set to read from start of iter-chunk */
				dz->sampbuf[0] += dz->iparam[CHUNKSTART];
				inmsampsize = chunkmsamps;		/* and read just the iterated chunk */
				/* And set to normal iteration delay  for 2nd Iteration: */
				if(!dz->brksize[ITER_DELAY])
					dz->iparam[ITER_MSAMPDEL]= orig_sampdel;
				iterating = 0;
			} else {
				iterating = 1;
			}
			switch(dz->mode) {
			case(ITERATE_DUR):
				if(write_start >= out_sampdur)
					is_penult = 1;
				break;
			case(ITERATE_REPEATS):
				if(cnt >= dz->iparam[ITER_REPEATS])
					is_penult = 1;
				break;
			}
			if(is_penult) {	/* If last repeat, set to read to end of infile */
				inmsampsize = (dz->insams[0] - dz->iparam[CHUNKSTART])/dz->infile->channels;
				dz->sampbuf[0] -= dz->iparam[CHUNKSTART];
				memcpy((char *)dz->sampbuf[0],(char *)dz->sampbuf[3],(dz->insams[0] * sizeof(float)));
				dz->sampbuf[0] += dz->iparam[CHUNKSTART];
				iterating = 0;
			}
			if(finished)
				break;
			while(local_write_start >= dz->buflen) {
				if(passno > 0) {
					if((exit_status = write_samps(dz->sampbuf[1],dz->buflen,dz))<0)
						return(exit_status);
				}
				bufs_written++;
				tail = write_end - dz->buflen;
				memset((char *)dz->sampbuf[1],0,dz->buflen * sizeof(float));
				if(tail > 0) {
					memmove((char *)dz->sampbuf[1],(char *)dz->sampbuf[2],tail * sizeof(float));
					tailend = dz->sampbuf[1] + tail;
				} else
					tailend = dz->sampbuf[2];
				memset((char *)tailend,0,(dz->sampbuf[3] - tailend) * sizeof(float));
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
			}
			thistime = ((dz->buflen * bufs_written) + local_write_start) * one_over_sr;
			
			if((exit_status = read_values_from_all_existing_brktables(thistime,dz))<0)
				return(exit_status);
			if(is_penult) {
				dz->param[ITER_PSCAT] = 0.0;
				dz->param[ITER_ASCAT] = 0.0;
			}
			if(dz->brksize[ITER_DELAY])
				dz->iparam[ITER_MSAMPDEL] = round(dz->param[ITER_DELAY] * (double)dz->infile->srate);
			write_end = iterate(cnt,passno,gain,pshift,write_end,local_write_start,inmsampsize,level,&maxsamp,iterating,dz);
			if(passno==0)
				wstart[cnt] = get_next_writestart(write_start,dz);
			write_start = wstart[cnt];
			local_write_start = write_start - (bufs_written * dz->buflen);
		}
		if(passno > 0) {
			if(write_end > 0) {
				if((exit_status = write_samps(dz->sampbuf[1],write_end,dz))<0)
					return(exit_status);
			}
		} else {
			gaingain = (localmax/maxsamp) * dz->param[ITER_LGAIN];
			switch(dz->iparam[ITER_PROCESS]) {
			case(MONO):		      		
			case(STEREO):	      		
			case(MN_INTP_SHIFT):      	
			case(ST_INTP_SHIFT):      	
				for(k=0;k<=cnt;k++)
					gain[k] *= gaingain;
				break;
			case(FIXA_MONO):	      	
			case(FIXA_STEREO):	      	
			case(FIXA_MN_INTP_SHIFT): 	
			case(FIXA_ST_INTP_SHIFT): 	
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
	int samps, k, samps_read;
	int n;
	if((samps_read = fgetfbufEx(dz->sampbuf[0], dz->insams[0]/* + SECSIZE*/,dz->ifd[0],0)) <= 0) {
		sprintf(errstr,"Can't read samps from input soundfile\n");
		if(samps_read<0)
			return(SYSTEM_ERROR);
		return(DATA_ERROR);
	}
	if(samps_read!=dz->insams[0]) {
		sprintf(errstr, "Failed to read all of source file. read_the_input_snd()\n");
		return(PROGRAM_ERROR);
	}
	samps = samps_read / dz->infile->channels;

	if(dz->param[ITER_PSCAT] > 0.0) {
		k = samps * dz->infile->channels;
		for(n=0;n<dz->infile->channels;n++) {
			dz->sampbuf[0][k] = (float)0;
			k++;		/* GUARD POINTS FOR INTERPOLATION */
		}
	}
	return(FINISHED);
}

/*************************** GET_NEXT_WRITESTART ****************************/

int get_next_writestart(int write_start,dataptr dz)
{
	int this_step;
	double d;  
	int mwrite_start = write_start/dz->infile->channels;
	if(dz->param[ITER_RANDOM] > 0.0) {
		d = ((drand48() * 2.0) - 1.0) * dz->param[ITER_RANDOM];
		d += 1.0;
		this_step = (int)round((double)dz->iparam[ITER_MSAMPDEL] * d);
		mwrite_start += this_step;
	} else
		mwrite_start += dz->iparam[ITER_MSAMPDEL];
	write_start = mwrite_start * dz->infile->channels;
	return(write_start);
}    

/******************************* ITERATE *****************************/

//TW COMPLETELY UPDATED FUNCTION : (flt-converted)
int iterate
(int cnt,int pass,double *gain,double *pshift,int write_end,int local_write_start,int inmsampsize,double level,double *maxsamp,int iterating,dataptr dz)
{
	int wr_end = 0;
	switch(dz->iparam[ITER_PROCESS]) {
	case(MONO):		      		
		wr_end = iter(cnt,pass,gain,local_write_start,inmsampsize,level,maxsamp,iterating,dz);							
		break;
	case(STEREO):	      		
		wr_end = iter_stereo(cnt,pass,gain,local_write_start,inmsampsize,level,maxsamp,iterating,dz);			 		
		break;
	case(MN_INTP_SHIFT):      	
		wr_end = iter_shift_interp(cnt,pass,gain,pshift,local_write_start,inmsampsize,level,maxsamp,iterating,dz);				
		break;
	case(ST_INTP_SHIFT):      	
		wr_end = iter_shift_interp_stereo(cnt,pass,gain,pshift,local_write_start,inmsampsize,level,maxsamp,iterating,dz);		
		break;
	case(FIXA_MONO):	      	
		wr_end = fixa_iter(cnt,pass,gain,local_write_start,inmsampsize,level,maxsamp,iterating,dz);			 			
		break;
	case(FIXA_STEREO):	      	
		wr_end = fixa_iter_stereo(cnt,pass,gain,local_write_start,inmsampsize,level,maxsamp,iterating,dz);		 		
		break;
	case(FIXA_MN_INTP_SHIFT): 	
		wr_end = fixa_iter_shift_interp(cnt,pass,gain,pshift,local_write_start,inmsampsize,level,maxsamp,iterating,dz); 		
		break;
	case(FIXA_ST_INTP_SHIFT): 	
		wr_end = fixa_iter_shift_interp_stereo(cnt,pass,gain,pshift,local_write_start,inmsampsize,level,maxsamp,iterating,dz); 
		break;
	}
	return max(wr_end,write_end);
}

/**************************** ITER ***************************/

//TW COMPLETELY UPDATED FUNCTION : (flt-converted)
int iter(int cnt,int passno, double *gain,int local_write_start,int inmsampsize,double level,double *maxsamp,int iterating,dataptr dz)
{
	register int i, j = local_write_start;
	float *outbuf = dz->sampbuf[1];
	float *inbuf  = dz->sampbuf[0];
	double z;
	double thisgain;
	if(passno == 0) {
		gain[cnt] = get_gain(dz);
		thisgain = gain[cnt];

		for(i=0; i < inmsampsize; i++) {
			z = outbuf[j] + (inbuf[i] * thisgain);
			*maxsamp = max(*maxsamp,fabs(z));
			outbuf[j++] = (float)z;
		}
	} else {
		thisgain = gain[cnt];
		for(i=0; i < inmsampsize; i++) {
			if(iterating)
				z = outbuf[j] + (inbuf[i] * thisgain);
			else
				z = outbuf[j] + inbuf[i];
			outbuf[j++] = (float)z;
		}
	}
	return(j);
}

/**************************** ITER_STEREO ***************************/

//TW COMPLETELY UPDATED FUNCTION : (flt-converted)
int iter_stereo(int cnt,int passno, double *gain,int local_write_start,int inmsampsize,double level,double *maxsamp,int iterating,dataptr dz)
{
	register int i, j = local_write_start, k;
	int n;
	float *outbuf = dz->sampbuf[1];
	float *inbuf  = dz->sampbuf[0];
	double z;
	double thisgain;
	if(passno == 0) {
		gain[cnt] = get_gain(dz);
		thisgain = gain[cnt];
		for(i=0; i < inmsampsize; i++) {
			k = i*dz->infile->channels;
			for(n=0;n<dz->infile->channels;n++) {
				z = outbuf[j] + (inbuf[k++] * thisgain);
//					*maxsamp = max(*maxsamp,abs(z));
				*maxsamp = max(*maxsamp,fabs(z));
				outbuf[j++]  = (float)z;
			}
		}
	} else {
		thisgain = gain[cnt];

		for(i=0; i < inmsampsize; i++) {
			k = i*dz->infile->channels;
			for(n=0;n<dz->infile->channels;n++) {
				if(iterating)
					z = outbuf[j] + (inbuf[k++] * thisgain);
				else
					z = outbuf[j] + inbuf[k++];
			}
		}
	}
	return(j);
}

/**************************** ITER_SHIFT_INTERP ***************************/

//TW COMPLETELY UPDATED FUNCTION : (flt-converted)
int iter_shift_interp(int cnt,int passno, double *gain,double *pshift,int local_write_start,int inmsampsize,double level,double *maxsamp,int iterating,dataptr dz)
{
	register int i = 0, j = local_write_start;
	double d = 0.0, part = 0.0;
	float val, nextval, diff;
	float *outbuf = dz->sampbuf[1];
	float *inbuf  = dz->sampbuf[0];
	double z;
	double thisgain;
	if(passno == 0) {
		gain[cnt] = get_gain(dz);
		thisgain = gain[cnt];

		while(i < inmsampsize) {
			val     = inbuf[i++];
			nextval = inbuf[i];
			diff    = nextval - val;
			z = val + ((double)diff * part);
			z = (z * thisgain);
			z += outbuf[j];
//				*maxsamp = max(*maxsamp,abs(z));
			*maxsamp = max(*maxsamp,fabs(z));
			outbuf[j++] = (float)z;
			d      += dz->param[ITER_SSTEP];
			i      = (int)d; 			/* TRUNCATE */
			part   = d - (double)i; 
		}
		pshift[cnt] = get_pshift(dz);
		dz->param[ITER_SSTEP] = pshift[cnt];
	} else {
		thisgain = gain[cnt];

		while(i < inmsampsize) {
			val     = inbuf[i++];
			nextval = inbuf[i];
			diff    = nextval - val;
			z = val + ((double)diff * part);
			if(iterating)
				z = (z * thisgain);
				z += outbuf[j];
			outbuf[j++] = (float)z;
			d      += dz->param[ITER_SSTEP];
			i      = (int)d; 			/* TRUNCATE */
			part   = d - (double)i; 
		}
		dz->param[ITER_SSTEP] = pshift[cnt];
	}
	return(j);
}

/*********************** ITER_SHIFT_INTERP_STEREO *************************/

//TW COMPLETELY UPDATED FUNCTION : (flt-converted)
int iter_shift_interp_stereo(int cnt,int passno, double *gain,double *pshift,int local_write_start,int inmsampsize,double level,double *maxsamp,int iterating,dataptr dz)
{
	register int i = 0, j = local_write_start, k;
	int n;
	double d = 0.0, part = 0.0;
	float val, nextval, diff;
	float *outbuf = dz->sampbuf[1];
	float *inbuf  = dz->sampbuf[0];
	double z;
	double thisgain;
	if(passno == 0) {
		gain[cnt] = get_gain(dz);
		thisgain = gain[cnt];

		while(i < inmsampsize) {
			k = i*dz->infile->channels;
			for(n=0;n<dz->infile->channels;n++) {
				val     = inbuf[k];
				nextval = inbuf[k+dz->infile->channels];
				diff    = nextval - val;
				z = val + ((double)diff * part);
				z = (z * thisgain);
				z += outbuf[j];
//					*maxsamp = max(*maxsamp,abs(z));
				*maxsamp = max(*maxsamp,fabs(z));
				outbuf[j++] = (float)z;
				k++;
			}
			d   += dz->param[ITER_SSTEP];
			i    = (int)d; 			/* TRUNCATE */
			part = d - (double)i; 
		}
		pshift[cnt] = get_pshift(dz);
		dz->param[ITER_SSTEP] = pshift[cnt];
	} else {
		thisgain = gain[cnt];

		while(i < inmsampsize) {
			k = i*dz->infile->channels;
			for(n=0;n<dz->infile->channels;n++) {
				val     = inbuf[k];
				nextval = inbuf[k+dz->infile->channels];
				diff    = nextval - val;
				z = val + ((double)diff * part);
				if(iterating)
					z = (z * thisgain);
				z += outbuf[j];
				outbuf[j++] = (float)z;
				k++;
			}
			d   += dz->param[ITER_SSTEP];
			i    = (int)d; 			/* TRUNCATE */
			part = d - (double)i; 
		}
		dz->param[ITER_SSTEP] = pshift[cnt];
	}
	return(j);
}

/**************************** FIXA_ITER ***************************/

//TW COMPLETELY UPDATED FUNCTION : (flt-converted)
int fixa_iter(int cnt,int passno,double *gain,int local_write_start,int inmsampsize,double level,double *maxsamp,int iterating,dataptr dz)
{
	register int i, j = local_write_start;
	float *outbuf = dz->sampbuf[1];
	float *inbuf  = dz->sampbuf[0];
	double z;

	if(passno ==0) {
		for(i=0; i < inmsampsize; i++) {
			z = outbuf[j] + inbuf[i];
			*maxsamp = max(*maxsamp,fabs(z));
			outbuf[j]  = (float)z;
			j++;
		}
	} else {
		for(i=0; i < inmsampsize; i++) {
			if(iterating)
				z = outbuf[j] + (inbuf[i] * gain[cnt]);
			else
				z = outbuf[j] + inbuf[i];
			outbuf[j]  = (float)z;
			j++;
		}
	}
	return(j);
}

/**************************** FIXA_ITER_STEREO ***************************/

//TW COMPLETELY UPDATED FUNCTION : (flt-converted)
int fixa_iter_stereo(int cnt,int passno,double *gain,int local_write_start,int inmsampsize,double level,double *maxsamp,int iterating,dataptr dz)
{
	register int i, j = local_write_start, k;
	int n;
	float *outbuf = dz->sampbuf[1];
	float *inbuf  = dz->sampbuf[0];
	double z;

	if(passno == 0) {
		for(i=0; i < inmsampsize; i++) {
			k = i*dz->infile->channels;
			for(n=0;n<dz->infile->channels;n++) {
				z = outbuf[j] + inbuf[k++];
				*maxsamp = max(*maxsamp,fabs(z));
				outbuf[j]  = (float)z;
				j++;
			}
		}
 	} else {
		for(i=0; i < inmsampsize; i++) {
			k = i*dz->infile->channels;
			for(n=0;n<dz->infile->channels;n++) {
				if(iterating)
					z = outbuf[j] + (inbuf[k++] * gain[cnt]);
				else
					z = outbuf[j] + inbuf[k++];
				outbuf[j]  = (float)z;
				j++;
			}
		}
	}
 	return(j);
}

/**************************** FIXA_ITER_SHIFT_INTERP ***************************/

//TW COMPLETELY UPDATED FUNCTION : (flt-converted)
int fixa_iter_shift_interp(int cnt,int passno,double *gain,double *pshift,int local_write_start,int inmsampsize,double level,double *maxsamp,int iterating,dataptr dz)
{
	register int i = 0, j = local_write_start;
 	double d = 0.0, part = 0.0;
 	float val, nextval, diff;
 	float *outbuf = dz->sampbuf[1];
	float *inbuf  = dz->sampbuf[0];
	double z;

	if(passno == 0) {
		while(i < inmsampsize) {
		 	val     = inbuf[i++];
		 	nextval = inbuf[i];
		 	diff    = nextval - val;
			z = val + ((double)diff * part);
			z += outbuf[j];
			*maxsamp = max(*maxsamp,fabs(z));
		 	outbuf[j++] = (float)z;
		 	d      += dz->param[ITER_SSTEP];
		 	i      = (int)d; 			/* TRUNCATE */
		 	part   = d - (double)i; 
		}
		pshift[cnt] = get_pshift(dz);
	} else {
		while(i < inmsampsize) {
		 	val     = inbuf[i++];
		 	nextval = inbuf[i];
		 	diff    = nextval - val;
			z = val + ((double)diff * part);
			if(iterating)
				z = (z * gain[cnt]);
			z += outbuf[j];
		 	outbuf[j++] = (float)z;
		 	d      += dz->param[ITER_SSTEP];
		 	i      = (int)d; 			/* TRUNCATE */
		 	part   = d - (double)i; 
		}
	}
	dz->param[ITER_SSTEP] = pshift[cnt];
 	return(j);
}

/*********************** FIXA_ITER_SHIFT_INTERP_STEREO *************************/

//TW COMPLETELY UPDATED FUNCTION : (flt-converted)
int fixa_iter_shift_interp_stereo(int cnt,int passno,double *gain,double *pshift,int local_write_start,int inmsampsize,double level,double *maxsamp,int iterating,dataptr dz)
{
	register int i = 0, j = local_write_start, k;
	int n;
	double d = 0.0, part = 0.0;
	float val, nextval, diff;
	float *outbuf = dz->sampbuf[1];
	float *inbuf  = dz->sampbuf[0];
	double z;

	if(passno == 0) {
		while(i < inmsampsize) {
			k = i*dz->infile->channels;
			for(n=0;n<dz->infile->channels;n++) {
				val     = inbuf[k];
				nextval = inbuf[k+dz->infile->channels];
				diff    = nextval - val;
				z = val + ((double)diff * part);
				z += outbuf[j];
				*maxsamp = max(*maxsamp,fabs(z));
				outbuf[j++] = (float)z;
				k++;
			}
			d   += dz->param[ITER_SSTEP];
			i    = (int)d; 			/* TRUNCATE */
			part = d - (double)i; 
		}
		pshift[cnt] = get_pshift(dz);
	} else {
		while(i < inmsampsize) {
			k = i*dz->infile->channels;
			for(n=0;n<dz->infile->channels;n++) {
				val     = inbuf[k];
				nextval = inbuf[k+dz->infile->channels];
				diff    = nextval - val;
				z = val + ((double)diff * part);
				if(iterating)
					z = (z * gain[cnt]);
				z += outbuf[j];
				outbuf[j++] = (float)z;
				k++;
			}
			d   += dz->param[ITER_SSTEP];
			i    = (int)d; 			/* TRUNCATE */
			part = d - (double)i; 
		}
	}
	dz->param[ITER_SSTEP] = pshift[cnt];
	return(j);
}

/******************************** GET_GAIN *****************************/

double get_gain(dataptr dz)
{
	double scatter;
	double newlgain = dz->param[ITER_GAIN];
	if (dz->param[ITER_ASCAT] > 0.0) {
		scatter  = drand48() * dz->param[ITER_ASCAT];
		scatter  = 1.0 - scatter;
		newlgain = scatter * (double)dz->param[ITER_GAIN];
	}
	return(newlgain);
}

/******************************** GET_PSHIFT *****************************/

double get_pshift(dataptr dz)
{
	double scatter;
	scatter = (drand48() * 2.0) - 1.0;
	scatter *= dz->param[ITER_PSCAT];
	return(pow(2.0,scatter * OCTAVES_PER_SEMITONE));
}

/*************************** CREATE_FRITERBUFS ***************************/

int create_friterbufs(double maxpscat,dataptr dz)
{
	size_t bigbufsize, seccnt;
	double k;
	int extra_space, infile_space = dz->insams[0], big_buffer_size;
	int overflowsize /*, seccnt*/;

	size_t framesize = F_SECSIZE * sizeof(float) * dz->infile->channels;

	size_t bigchunk, min_bufsize;

	if(dz->param[ITER_PSCAT] > 0.0) {
		infile_space += dz->infile->channels;			/* 1 */
		k = pow(2.0,maxpscat * OCTAVES_PER_SEMITONE);
		overflowsize = (round((double)(dz->insams[0] * k)+1.0));
		overflowsize += ITER_SAFETY; 						/* 2 */	
	} else
		overflowsize = dz->insams[0];

	if((seccnt = infile_space/F_SECSIZE) * F_SECSIZE < infile_space)
		seccnt++;
	infile_space = F_SECSIZE * seccnt;

	extra_space = infile_space + overflowsize;
	extra_space += infile_space;
	min_bufsize = (extra_space * sizeof(float)) + framesize;
	bigchunk = (size_t)Malloc(-1);
	if(bigchunk < min_bufsize)
		bigbufsize = framesize;
	else {
		bigbufsize = bigchunk - extra_space*sizeof(float);
		bigbufsize = (bigbufsize/framesize) * framesize;
	}

	dz->buflen     = (int)(bigbufsize/sizeof(float));

	big_buffer_size = (size_t)(dz->buflen + extra_space);

	if((dz->bigbuf = (float *)Malloc(big_buffer_size * sizeof(float)))==NULL) {
		sprintf(errstr, "INSUFFICIENT MEMORY to create sound buffers.\n");
		return(MEMORY_ERROR);
	}
	dz->sbufptr[0] = dz->sampbuf[0] = dz->bigbuf;
	dz->sbufptr[1] = dz->sampbuf[1] = dz->sampbuf[0] + infile_space;
	dz->sbufptr[2] = dz->sampbuf[2] = dz->sampbuf[1] + dz->buflen;
	dz->sbufptr[3] = dz->sampbuf[3] = dz->sampbuf[2] + overflowsize;
	memset((char *)dz->sampbuf[0],0,(size_t)(infile_space * sizeof(float)));
	memset((char *)dz->sampbuf[1],0,(size_t)(dz->buflen * sizeof(float)));
	memset((char *)dz->sampbuf[2],0,(size_t)(overflowsize * sizeof(float)));
	memset((char *)dz->sampbuf[3],0,(size_t)infile_space * sizeof(float));
	return(FINISHED);
}

/**************************** FRITERATE_PREPROCESS ******************************/

int friterate_preprocess(dataptr dz)
{
	int exit_status;
	double maxrand, maxpscat, mindelay, temp;
	int is_unity_gain = FALSE;
	int mindelay_samps;
	if(dz->iparam[ITER_RRSEED] > 0)
		srand((int)dz->iparam[ITER_RRSEED]);
	else
		initrand48();
	if((exit_status = get_maxvalue_of_rand(&maxrand,dz))<0)
			return(exit_status);
	if((exit_status = get_maxvalue_of_pscat(&maxpscat,dz))<0)
			return(exit_status);
	if((exit_status = get_minvalue_of_delay(&mindelay,dz))<0)
			return(exit_status);
 	mindelay_samps = round(mindelay * (double)dz->infile->srate);
	dz->iparam[CHUNKSTART] = (int)round(dz->param[CHUNKSTART] * dz->infile->srate) * dz->infile->channels;
	dz->iparam[CHUNKEND]   = (int)round(dz->param[CHUNKEND]   * dz->infile->srate) * dz->infile->channels;
	if(dz->param[CHUNKSTART] > dz->param[CHUNKEND]) {
		temp = dz->param[CHUNKSTART];
		dz->param[CHUNKSTART] = dz->param[CHUNKEND];
		dz->param[CHUNKEND] = temp;
	}
	if(dz->iparam[CHUNKEND] - dz->iparam[CHUNKSTART] <= ITX_SPLICELEN * dz->infile->channels * 2) {
		sprintf(errstr,"FROZEN SEGMENT (%d samples) TOO SHORT FOR SPLICING (needs %d samples)\n",
			dz->iparam[CHUNKEND] - dz->iparam[CHUNKSTART],ITX_SPLICELEN * dz->infile->channels * 2);
		return(DATA_ERROR);
	}
	if(dz->iparam[CHUNKEND] - dz->iparam[CHUNKSTART] < mindelay_samps * dz->infile->channels) {
		sprintf(errstr,"FROZEN SEGMENT (%d samples) TOO SHORT FOR (MINIMUM) DELAY TIME SPECIFIED (%d samples)\n",
			dz->iparam[CHUNKEND] - dz->iparam[CHUNKSTART],mindelay_samps * dz->infile->channels);
		return(DATA_ERROR);
	}
	set_default_delays(dz);
	dz->param[ITER_SSTEP] = 1.0; /* 1st sound is exact copy of orig */
	is_unity_gain = TRUE;
	setup_iter_process_type(is_unity_gain,dz);
	return create_friterbufs(maxpscat,dz);
}

/*************************** GET_MAXVALUE_OF_RAND ****************************/

int get_maxvalue_of_rand(double *maxrand,dataptr dz)
{
	int exit_status;
	if(dz->brksize[ITER_RANDOM]) {
		if((exit_status = get_maxvalue_in_brktable(maxrand,ITER_RANDOM,dz))<0)
			return(exit_status);
	} else
		*maxrand = dz->param[ITER_RANDOM];
	return(FINISHED);
}

/************************* GET_MAXVALUE_OF_PSCAT ****************************/

int get_maxvalue_of_pscat(double *maxpscat,dataptr dz)
{
	int exit_status;
	if(dz->brksize[ITER_PSCAT]) {
		if((exit_status = get_maxvalue_in_brktable(maxpscat,ITER_PSCAT,dz))<0)
			return(exit_status);
	} else
		*maxpscat = dz->param[ITER_PSCAT];
	return(FINISHED);
}

/************************* GET_MINVALUE_OF_DELAY ****************************/

int get_minvalue_of_delay(double *mindelay,dataptr dz)
{
	int exit_status;
	if(dz->brksize[ITER_DELAY]) {
		if((exit_status = get_minvalue_in_brktable(mindelay,ITER_DELAY,dz))<0)
			return(exit_status);
	} else
		*mindelay = dz->param[ITER_DELAY];
	return(FINISHED);
}

/*********************** SET_DEFAULT_DELAYS ****************************/

void set_default_delays(dataptr dz)
{
	if(!dz->brksize[ITER_DELAY])
		dz->iparam[ITER_MSAMPDEL] = round(dz->param[ITER_DELAY] * (double)dz->infile->srate);
}

/********************* SETUP_ITER_PROCESS_TYPE ********************************/

void setup_iter_process_type(int is_unity_gain,dataptr dz)
{
	if(dz->param[ITER_PSCAT] > 0.0) {
		if(dz->infile->channels>1)
			dz->iparam[ITER_PROCESS] = ST_INTP_SHIFT;
		else
			dz->iparam[ITER_PROCESS] = MN_INTP_SHIFT;
	} else {
		if(dz->infile->channels>1)
			dz->iparam[ITER_PROCESS] = STEREO;
		else
			dz->iparam[ITER_PROCESS] = MONO;
	}
	if(flteq(dz->param[ITER_ASCAT],0.0))
		dz->iparam[ITER_PROCESS] += FIXED_AMP;
	return;
}


#ifndef round
int round(double a) {
	return (int)floor(a + 0.5);
}
#endif
