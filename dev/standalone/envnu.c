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
static int check_the_param_validity_and_consistency(dataptr dz);
static int setup_envnu_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_envnu_param_ranges_and_defaults(dataptr dz);
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
static int expdecay(dataptr dz);
static void getenv_of_buffer(int samps_to_process,int envwindow_sampsize,double convertor,double time_convertor,float **envptr,float *buffer,dataptr dz);
static int windows_in_sndfile(dataptr dz);
static int buffers_in_sndfile(int buffer_size,dataptr dz);
static int peakchop(dataptr dz);
static int extract_env_from_sndfile(int *envcnt,dataptr dz);
static void extract_peaks_from_envelope(int *envcnt,dataptr dz);
static int eliminate_too_low_peaks(int *envcnt,dataptr dz);
static int find_exact_peaktimes(int envcnt,dataptr dz);
static int calculate_event_outtimes(int envcnt,int *outcnt,dataptr dz);
static int generate_enveloped_output(int envcnt,int outcnt,dataptr dz);
static double getmaxsamp(int startsamp, int sampcnt,float *buffer);
static int create_peakchop_sndbufs(dataptr dz);
static int peakchop_param_preprocess(dataptr dz);
static int generate_envelope_as_output(int envcnt,dataptr dz);	
static int get_the_mode_from_cmdline(char *str,dataptr dz);
static int beyond_endtime(char **cmdline,int cmdlinecnt,dataptr dz);
static int this_value_is_numeric(char *str);

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
		if(dz->process == PEAKCHOP) {
			dz->maxmode = 2;
			if((exit_status = get_the_mode_from_cmdline(cmdline[0],dz))<0) {
				print_messages_and_close_sndfiles(exit_status,is_launched,dz);
				return(FAILED);
			}
			cmdline++;
			cmdlinecnt--;
		}
		else
			dz->maxmode = 0;
		// setup_particular_application =
		if((exit_status = setup_envnu_application(dz))<0) {
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
	if((exit_status = setup_envnu_param_ranges_and_defaults(dz))<0) {
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

	is_launched = TRUE;
	switch(dz->process) {
	case(EXPDECAY):
		dz->bufcnt = 1;
		break;
	case(PEAKCHOP):
		if(dz->mode == 0)
			dz->bufcnt = 3;
		else
			dz->bufcnt = 1;
		break;
	}
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

	if(!sloom && (dz->process == EXPDECAY)) {
		if(beyond_endtime(cmdline,cmdlinecnt,dz))	//	In cmdline version, endtime vals beyond end of file are trapped
			sprintf(cmdline[1],"%f",0.0);			//	and (temporarily) set to 0 here
			
	}
	if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = check_the_param_validity_and_consistency(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	switch(dz->process) {
	case(EXPDECAY):
		if((exit_status = create_sndbufs(dz)) < 0)
			return(exit_status);
		break;
	case(PEAKCHOP):
		if(dz->mode == 0) {
			if((exit_status = create_peakchop_sndbufs(dz)) < 0)
				return(exit_status);
		} else {
			if((exit_status = create_sndbufs(dz)) < 0)
				return(exit_status);
		}
		if((exit_status = peakchop_param_preprocess(dz)) < 0)
			return(exit_status);
		break;
	}
	switch(dz->process) {
	case(EXPDECAY):
		if((exit_status = expdecay(dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		break;
	case(PEAKCHOP):
		if((exit_status = peakchop(dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		break;
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
	if(dz->process == PEAKCHOP) {
		if((exit_status = setup_internal_arrays_and_array_pointers(dz))<0)	 
			return(exit_status);
	}
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
	char *filename = (*cmdline)[0], *p;
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
	if(sloom) {
		p = filename + strlen(filename);
		p--;
		while(*p != '.') {
			p--;
			if(p < filename)
				break;
		}
		if(p >= filename)
			*p = ENDOFSTR;
	}
	if(dz->process != PEAKCHOP || dz->mode != 1)
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

/************************* SETUP_ENVU_APPLICATION *******************/

int setup_envnu_application(dataptr dz)
{
	int exit_status;
	aplptr ap;
	if((exit_status = establish_application(dz))<0)		// GLOBAL
		return(FAILED);
	ap = dz->application;
	// SEE parstruct FOR EXPLANATION of next 2 functions
	switch(dz->process) {
	case(EXPDECAY):
		if((exit_status = set_param_data(ap,0   ,2,2,"dd"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","",0,0,""))<0)
			return(FAILED);
		// set_legal_infile_structure -->
		dz->has_otherfile = FALSE;
		// assign_process_logic -->
		dz->input_data_type = SNDFILES_ONLY;
		dz->process_type	= UNEQUAL_SNDFILE;	
		dz->outfiletype  	= SNDFILE_OUT;
		dz->array_cnt	= 0;
		break;
	case(PEAKCHOP):
		if(dz->mode == 0) {
			if((exit_status = set_param_data(ap,0   ,5,5,"dDDDD"))<0)
				return(FAILED);
			if((exit_status = set_vflgs(ap,"",0,"","gqsnrm",6,6,"ddDDII"))<0)
				return(FAILED);
			// assign_process_logic -->
			dz->input_data_type = SNDFILES_ONLY;
			dz->process_type	= UNEQUAL_SNDFILE;	
			dz->outfiletype  	= SNDFILE_OUT;
			dz->array_cnt	= 1;
		} else {
			if((exit_status = set_param_data(ap,0   ,5,3,"dDD00"))<0)
			return(FAILED);
			if((exit_status = set_vflgs(ap,"",0,"","gq",2,2,"dd"))<0)
				return(FAILED);
			// assign_process_logic -->
			dz->input_data_type = SNDFILES_ONLY;
			dz->process_type	= TO_TEXTFILE;	
			dz->outfiletype  	= TEXTFILE_OUT;
			dz->array_cnt	= 0;
		}
		break;
	}
	// set_legal_infile_structure -->
	dz->has_otherfile = FALSE;
	dz->iarray_cnt	= 0;
	dz->larray_cnt	= 0;
	dz->ptr_cnt		= 0;
	dz->fptr_cnt	= 0;
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

/************************* SETUP_ENVNU_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_envnu_param_ranges_and_defaults(dataptr dz)
{
	int exit_status;
	aplptr ap = dz->application;
	// set_param_ranges()
	ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
	// NB total_input_param_cnt is > 0 !!!
	if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
		return(FAILED);
	// get_param_ranges()
	switch(dz->process) {
	case(EXPDECAY):
		ap->lo[0]			= 0.0;
		ap->hi[0]			= dz->duration;
		ap->default_val[0]	= 0.0;
		ap->lo[1]			= 0.0;
		ap->hi[1]			= dz->duration;
		ap->default_val[1]	= dz->duration;
		break;
	case(PEAKCHOP):	
		ap->lo[PKCH_WSIZE]	= 1.0;	//wsize for envelope extraction
		ap->hi[PKCH_WSIZE]	= min(dz->duration,1.0) * SECS_TO_MS;
		ap->default_val[PKCH_WSIZE]	= 50.0;
		ap->lo[PKCH_WIDTH]	= 0.0;	// peakwidth (mS)
		ap->hi[PKCH_WIDTH]	= 1000.0;
		ap->default_val[PKCH_WIDTH]	= 2.0;
		ap->lo[PKCH_SPLICE]	= 1.0;	// risetime (mS)
		ap->hi[PKCH_SPLICE]	= 200;
		ap->default_val[PKCH_SPLICE]	= 10.0;
		ap->lo[PKCH_GATE]	= 0.0;	// gate (0-1)
		ap->hi[PKCH_GATE]	= 1.0;
		ap->default_val[PKCH_GATE]	= 0.0;
		ap->lo[PKCH_SKEW]	= 0.0;	// skew (0-1)
		ap->hi[PKCH_SKEW]	= 1.0;
		ap->default_val[PKCH_SKEW]	= 0.25;
		if(dz->mode == 0) {
			ap->lo[PKCH_TEMPO]	= 20;	// tempo (MM)
			ap->hi[PKCH_TEMPO]	= 3000;
			ap->default_val[PKCH_TEMPO]	= 120;
			ap->lo[PKCH_GAIN]	 = 0.0;	// overall gain
			ap->hi[PKCH_GAIN]	 = 1.0;
			ap->default_val[PKCH_GAIN]	= 1.0;
			ap->lo[PKCH_SCAT]	= 0.0;	// scatter (0-1)
			ap->hi[PKCH_SCAT]	= 1.0;
			ap->default_val[PKCH_SCAT]	= 0.0;
			ap->lo[PKCH_NORM]	= 0.0;	// normalise (0-1)
			ap->hi[PKCH_NORM]	= 1.0;
			ap->default_val[PKCH_NORM]	= 0.0;
			ap->lo[PKCH_REPET]	= 0;	// repeat attacks
			ap->hi[PKCH_REPET]	= 256;
			ap->default_val[PKCH_REPET]	= 0;
			ap->lo[PKCH_MISS]	= 0;	// skip attacks (i.e. take every n+1th attack only)
			ap->hi[PKCH_MISS]	= 64;
			ap->default_val[PKCH_MISS]	= 0;
		}
		break;
	}
	if(!sloom)
		put_default_vals_in_all_params(dz);
	dz->maxmode = 0;
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
			if((exit_status = setup_envnu_application(dz))<0)
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
	int n;
	if(dz->array_cnt > 0) {  
		if((dz->parray  = (double **)malloc(dz->array_cnt * sizeof(double *)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for internal double arrays.\n");
			return(MEMORY_ERROR);
		}
		for(n=0;n<dz->array_cnt;n++)
			dz->parray[n] = NULL;
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

int set_legal_internalparam_structure(int process,int mode,aplptr ap)
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

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	if     (!strcmp(prog_identifier_from_cmdline,"expdecay"))	dz->process = EXPDECAY;
	else if(!strcmp(prog_identifier_from_cmdline,"peakchop"))	dz->process = PEAKCHOP;
	else {
		sprintf(errstr,"Unknown program identification string '%s'\n",prog_identifier_from_cmdline);
		return(USAGE_ONLY);
	}
	return(FINISHED);
}

/****************************** GET_MODE *********************************/

int get_the_mode_from_cmdline(char *str,dataptr dz)
{
	char temp[200], *p;
	if(sscanf(str,"%s",temp)!=1) {
		sprintf(errstr,"Cannot read mode of program.\n");
		return(USAGE_ONLY);
	}
	p = temp + strlen(temp) - 1;
	while(p >= temp) {
		if(!isdigit(*p)) {
			sprintf(errstr,"Invalid mode of program entered.\n");
			return(USAGE_ONLY);
		}
		p--;
	}
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

/******************************** USAGE1 ********************************/

int usage1(void)
{
	fprintf(stderr,
	"\nNEW ENVELOPING OPERATIONS\n\n"
	"USAGE: envnu NAME (mode) infile outfile parameters: \n"
	"\n"
	"where NAME can be any one of\n"
	"\n"
	"expdecay  peakchop\n"
	"Type 'envnu expdecay' for more info on envnu expdecay..ETC.\n");
	return(USAGE_ONLY);
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
	if(!strcmp(str,"expdecay")) {
		fprintf(stderr,
	    "USAGE:\n"
	    "envnu expdecay infile outfile starttime endtime\n"
		"\n"
		"Impose exponential decay (to zero) on a sound, from starttime to endtime.\n"
		"\n"
		"To force decay-end to be at end of file,\n"
		"enter an endtime at OR beyond file duration.\n");
	} else if(!strcmp(str,"peakchop")) {
		fprintf(stderr,
	    "USAGE:\n"
		"envnu peakchop 1 infil outfil wsize pkwidth risetime tempo gain\n"
		"              [-ggate -qskew -sscatter -norm -rrepeat -mmiss]\n"
		"OR:\n"
		"envnu peakchop 2 infil outfil wsize pkwidth risetime [-ggate -qskew]\n"
		"\n"
		"Isolate peaks in source, and (Mode 1) play back at specified tempo,\n"
		"or (Mode 2) output peak-isolating envelope.\n"
		"\n"
		"WSIZE    windowsize (in mS) for extracting envelope (1-64 dflt 50)\n"
		"PKWIDTH  width of retained peaks (in mS) (dflt 20mS).\n"
		"         (Must be less than minimum distance between data peaks).\n"
		"RISETIME risetime from zero to peak (in mS) (dflt 10mS)\n"
		"TEMPO    tempo of resulting output, MM = events per minute.\n"
		"GAIN     lower this if rapid tempo causes peaks to overlap (0-1, dflt 1)\n"
		"GATE     level (relative to max) below which peaks ignored (0-1 dflt 0)\n"
		"SKEW     envelope centring on peak (0 to 1: dflt 0.25). 0.5 = centred,\n"
		"         0 = peak at envelope start,  1 = peak at envelope end.\n"
		"SCATTER  randomisation of output times (0-1 dflt 0)\n"
		"NORM     force peakevent levels towards that of loudest (0-1 dflt 0)\n"
		"REPEAT   number of times peakevents are repeated (0-256 dflt 0)\n"
		"MISS     use peakevent, skip next 'miss' peakevents (0-64 default 0).\n"
		"\n"
		"\nAll parameters may vary in time, EXCEPT wsize, gate and skew.\n"
		"\nTimes in breakpoint files are times in the output.\n"
		"\n");
	} else
		fprintf(stdout,"Unknown option '%s'\n",str);
	return(USAGE_ONLY);
}

int usage3(char *str1,char *str2)
{
	fprintf(stderr,"Insufficient parameters on command line.\n");
	return(USAGE_ONLY);
}

/******************************** EXPDECAY ********************************/

int expdecay(dataptr dz)
{
	int exit_status;
	double b, val, nextval, div;
    double incr, mlt, dur, srate;
    int chans,j, k;
    int  local_frames, local_frame_cnt = 0;
	float *buf = dz->sampbuf[0];
	int startframe, endframe, decayframes, frame_cnt;

 	srate = (double)dz->infile->srate;
	dur = (double)dz->duration;
	chans = dz->infile->channels;
	startframe = (int)round(dz->param[0] * srate);
	endframe   = (int)round(dz->param[1] * srate);
	decayframes = endframe - startframe;
    val = 1.0;
    b = 0.00001;
	div = 1.0/(double)decayframes;
    mlt = pow(b,div);
	dz->ssampsread = 1;
	frame_cnt = 0;
	while(frame_cnt < endframe) {
		if((exit_status = read_samps(buf,dz))<0)
				return(exit_status);
		local_frames = dz->ssampsread / chans;
		local_frame_cnt = 0;
		while (local_frame_cnt < local_frames) {
			if(frame_cnt >= startframe) {
				if(frame_cnt >= endframe) {
					if(local_frame_cnt > 0) {
						if((exit_status = write_samps(buf,local_frame_cnt * chans,dz))<0)
							return(exit_status);
					}
					return FINISHED;
				}
				for(j = 0;j < chans;j++) {
					k = (local_frame_cnt * chans) + j;
					buf[k] = (float)(buf[k] * val);
				}
				nextval = val * mlt;
				incr = nextval - val;
				val += incr;
			}
			local_frame_cnt++;
			frame_cnt++;
		}
		if(local_frame_cnt > 0) {
			if((exit_status = write_samps(buf,local_frame_cnt * chans,dz))<0)
				return(exit_status);
		}
	}
	return FINISHED;
}

/*********************** CHECK_THE_PARAM_VALIDITY_AND_CONSISTENCY *********************/

int check_the_param_validity_and_consistency(dataptr dz)
{
	int  channels = dz->infile->channels;
	int srate = dz->infile->srate;
	switch(dz->process) {
	case(EXPDECAY):
		if(!sloom && (dz->param[1] == 0.0))		//	In cmdline case, endtime vals beyond end of file are p[reviously trapped and set to 0
			dz->param[1] = dz->duration;		//	Then readjusted to file-duration here
		if(dz->param[0] >= dz->param[1]) {
			sprintf(errstr,"Endtime of decay must be greater than starttime. %lf  %lf",dz->param[0],dz->param[1]);
			return(USER_ERROR);
		}
		break;
	case(PEAKCHOP):												//	Convert time parameters to samples
		if(dz->brksize[PKCH_WIDTH] == 0)
			dz->iparam[PKCH_WIDTH]  = round(dz->param[PKCH_WIDTH]  * MS_TO_SECS * (double)srate) * channels;
		if(dz->brksize[PKCH_SPLICE] == 0)
			dz->iparam[PKCH_SPLICE] = round(dz->param[PKCH_SPLICE] * MS_TO_SECS * (double)srate) * channels;

															//	Convert tempo parameter to event-distance
		if(dz->mode == 0) {
			if(dz->brksize[PKCH_TEMPO] == 0)
				dz->param[PKCH_TEMPO] = 60.0 / dz->param[PKCH_TEMPO];
																//	Convert miss to step (miss 0 = step 1, miss 1 = step 2 etc.
			if(dz->brksize[PKCH_MISS] == 0)
				dz->iparam[PKCH_MISS]++;
		}
		break;
	}
	return FINISHED;
}

/*********************** PEAKCHOP_PARAM_PREPROCESS *********************/

int peakchop_param_preprocess(dataptr dz)
{
	int exit_status, chans = dz->infile->channels;
	int n, j, k, unadjusted_envwindow_sampsize;
	float *buf;
	double maxsamp, thisval, srate = (double)dz->infile->srate;
			// Trim envelopesize to fit exactly into buffers (which are a pow of two in length)
	unadjusted_envwindow_sampsize = round(dz->param[PKCH_WSIZE] * MS_TO_SECS * srate) * chans;
	if(unadjusted_envwindow_sampsize < dz->buflen) {
		k = dz->buflen;
		while(unadjusted_envwindow_sampsize<k)
			k /= 2;
		j = k * 2;
		if(j - unadjusted_envwindow_sampsize > unadjusted_envwindow_sampsize - k)
			dz->iparam[PKCH_WSIZE] = (int)k;
		else
			dz->iparam[PKCH_WSIZE] = (int)j;
	} else {
		k = round((double)unadjusted_envwindow_sampsize/(double)dz->buflen);
		dz->iparam[PKCH_WSIZE] = (int)(dz->buflen * k);
	}
			// Get and store maximum source sample, if required
	if(dz->param[PKCH_GATE] > 0.0 || (dz->mode == 0 && (dz->brksize[PKCH_NORM] > 0 || dz->param[PKCH_NORM] > 0.0))) {
		if(dz->param[PKCH_GATE] > 0.0)
			fprintf(stdout,"INFO: Finding maximum sample, for gating.\n");
		else
			fprintf(stdout,"INFO: Finding maximum sample, for normalisation.\n");
		fflush(stdout);
		buf = dz->sampbuf[0];
		if((exit_status = read_samps(buf,dz))<0)
			return(exit_status);
		maxsamp = 0.0;
		while(dz->ssampsread > 0) {
			n = 0;
			while(n < dz->ssampsread) {
				thisval = fabs(buf[n]);
				if(thisval > maxsamp)
					maxsamp = thisval;
				n++;
			}
			if((exit_status = read_samps(buf,dz))<0)
				return(exit_status);
		}
		if(maxsamp <= 0.0) {
			sprintf(errstr,"NO SIGNIFICANT SIGNAL FOUND IN SOURCE\n");
			return(DATA_ERROR);
		}
		if(dz->param[PKCH_GATE] > 0.0)
			dz->param[PKCH_GATE] *= maxsamp;
		dz->scalefact = maxsamp;
		if((sndseekEx(dz->ifd[0],0,0)) < 0) {
			sprintf(errstr,"sndseek() 1 failed.\n");
			return(SYSTEM_ERROR);
		}
		reset_filedata_counters(dz);
		if(sloom)
			display_virtual_time(dz->total_samps_read,dz);		
	}
	return FINISHED;
}

/********************************************** PEAKCHOP ********************************************/

int peakchop(dataptr dz)
{
	int exit_status;
	int envcnt,outcnt;
	if((exit_status = extract_env_from_sndfile(&envcnt,dz))<0)
		return(exit_status);
	extract_peaks_from_envelope(&envcnt,dz);		// ENVELOPE to 'env' as time-val pairs
	if(envcnt == 0) {
		sprintf(errstr,"No peaks found. Silent file.\n");
		return(DATA_ERROR);
	}
	if(dz->param[PKCH_GATE] > 0.0) {
		if((exit_status = eliminate_too_low_peaks(&envcnt,dz))<0)
			return(exit_status);
	}
	if((sndseekEx(dz->ifd[0],0,0)) < 0) {
		sprintf(errstr,"sndseek() 2 failed\n");
		return(SYSTEM_ERROR);
	}
	reset_filedata_counters(dz);
	if(sloom)
		display_virtual_time(dz->total_samps_read,dz);		
	if((exit_status = find_exact_peaktimes(envcnt,dz))<0)
		return(exit_status);
	switch(dz->mode) {
	case(0):
		if((exit_status = calculate_event_outtimes(envcnt,&outcnt,dz))<0)
			return(exit_status);
		if((sndseekEx(dz->ifd[0],0,0)) < 0) {
			sprintf(errstr,"sndseek() 4 failed\n");
			return(SYSTEM_ERROR);
		}
		reset_filedata_counters(dz);
		if(sloom)
			display_virtual_time(dz->total_samps_read,dz);		
		if((exit_status = generate_enveloped_output(envcnt,outcnt,dz))<0)
			return(exit_status);
		break;
	case(1):
		if((exit_status = generate_envelope_as_output(envcnt,dz))<0)
			return(exit_status);
		break;
	}
	return FINISHED;
}

/****************************** EXTRACT_ENV_FROM_SNDFILE ******************************/

int extract_env_from_sndfile(int *envcnt,dataptr dz)
{
	int exit_status, safety = 100;
	int n, bufcnt, totmem;
	double convertor = 1.0/F_ABSMAXSAMP;
	double time_convertor = 1.0/(dz->infile->channels * dz->infile->srate);
	float *envptr;
	float *buffer = dz->sampbuf[0];
	fprintf(stdout,"INFO: Finding envelope of source.\n");
	fflush(stdout);
	bufcnt = buffers_in_sndfile(dz->buflen,dz);
	*envcnt = windows_in_sndfile(dz);
	totmem = (*envcnt * 2) + safety;
	if((dz->env=(float *)calloc(totmem,sizeof(float)))==NULL) { // *2 -> accomodates time,val PAIRS
		sprintf(errstr,"INSUFFICIENT MEMORY for envelope array.\n");
		return(MEMORY_ERROR);
	}
	envptr = dz->env;
	for(n = 0; n < bufcnt; n++)	{
		if((exit_status = read_samps(dz->sampbuf[0],dz))<0)
			return(exit_status);
		if(sloom)
			display_virtual_time(dz->total_samps_read,dz);		
		getenv_of_buffer(dz->ssampsread,dz->iparam[PKCH_WSIZE],convertor,time_convertor,&envptr,buffer,dz);
	}
	return(FINISHED);
}

/****************************** BUFFERS_IN_SNDFILE ******************************/

int buffers_in_sndfile(int buffer_size,dataptr dz)
{
	int bufcnt;
	if(((bufcnt = dz->insams[0]/buffer_size)*buffer_size)!=dz->insams[0])
		bufcnt++;
	return(bufcnt);
}

/****************************** WINDOWS_IN_SNDFILE [GET_ENVSIZE] ******************************/

int windows_in_sndfile(dataptr dz)
{
	int envsize, winsize = dz->iparam[PKCH_WSIZE];
	if(((envsize = dz->insams[0]/winsize) * winsize)!=dz->insams[0])
		envsize++;
	return(envsize);
}

/************************* GETENV_OF_BUFFER *******************************/

void getenv_of_buffer(int samps_to_process,int envwindow_sampsize,double convertor,double time_convertor,float **envptr,float *buffer,dataptr dz)
{
	int  start_samp = 0;
	int bufstart = dz->total_samps_read - dz->ssampsread;
	float *env = *envptr;
	while(samps_to_process >= envwindow_sampsize) {
		*env++ = (float)((start_samp + bufstart) * time_convertor);
		*env++ = (float)(getmaxsamp(start_samp,envwindow_sampsize,buffer) * convertor);
		start_samp  += envwindow_sampsize;
		samps_to_process -= envwindow_sampsize;
	}
	if(samps_to_process) {	/* Handle any final short buffer */
		*env++ = (float)((start_samp + bufstart) * time_convertor);
		*env++ = (float)(getmaxsamp(start_samp,samps_to_process,buffer) * convertor);
	}
	*envptr = env;
}

/************************* EXTRACT_PEAKS_FROM_ENVELOPE *******************************/

void extract_peaks_from_envelope(int *envcnt,dataptr dz)
{
	int orig_env_size = *envcnt, n, j=0, k=0;
	int islocalmax = 0;
	float lastenv = 0.0;
	int envloc = 0;
	float *env = dz->env;
	fprintf(stdout,"INFO: Finding peaks.\n");
	fflush(stdout);
	for(n=1;n < orig_env_size; n++) {
		j = n * 2;	// indexes time
		k = j + 1;	// indexes value
		if(env[k] <= lastenv) {
			if(islocalmax) {
				env[envloc] = env[j-2];		//	overwrites original envelope, with env peaks
				envloc++;
				env[envloc] = env[k-2];		//	overwrites original envelope, with env peaks
				envloc++;
			}
			islocalmax = 0;
		} else
			islocalmax = 1;
		lastenv = env[k];
	}
	if(islocalmax) {						//	Capture last peak, if a max
		env[envloc] = env[j-2];
		envloc++;
		env[envloc] = env[k-2];
		envloc++;
	}
	*envcnt = envloc/2;
}

/************************* ELIMINATE_TOO_LOW_PEAKS *******************************/

int eliminate_too_low_peaks(int *envcnt,dataptr dz)
{
	int new_envcnt = *envcnt;
	int tabend = new_envcnt * 2;
	int n = 0, j, k, jj, gated;
	fprintf(stdout,"INFO: Gating low-level peaks.\n");
	fflush(stdout);
	while(n < new_envcnt) {
		j = n * 2;
		k = j + 1;
		if(dz->env[k] < dz->param[PKCH_GATE]) {
			new_envcnt--;
			tabend = new_envcnt * 2;
			jj = j;
			while(jj < tabend) {
				dz->env[jj] = dz->env[jj + 2];
				jj++;
			}
		} else {
			n++;
		}
	}
	if(new_envcnt == 0) {
		sprintf(errstr,"No peaks retained at this gate level.\n");
		return(DATA_ERROR);
	}
	if((gated = *envcnt - new_envcnt) > 0) {
		fprintf(stdout,"INFO: %d peaks removed by gate: %d peaks remain\n",gated,new_envcnt);
		fflush(stdout);
		*envcnt = new_envcnt;
	}
	return FINISHED;
}

/************************* FIND_EXACT_PEAKTIMES *******************************/

int find_exact_peaktimes(int envcnt,dataptr dz)
{
	int exit_status;
	double inv_srate = 1.0 / dz->infile->srate, maxsamp, val;
	int n, j, k, maxtime, samptime, searchend, ibufstart_in_file;
	float *env = dz->env;
	float *buf = dz->sampbuf[0];
	if((exit_status = read_samps(buf,dz))<0)
		return(exit_status);
	ibufstart_in_file = 0;	
	for(n=0;n < envcnt; n++) {
		j = n * 2;	// indexes time
		k = j + 1;	// indexes value
		samptime = (int)round(env[j] * (double)dz->infile->srate) * dz->infile->channels;
		if((searchend = samptime + dz->iparam[PKCH_WSIZE]) >= dz->total_samps_read) {
			if(dz->ssampsread < dz->buflen) {
				samptime -= ibufstart_in_file;
				searchend = dz->ssampsread;
			} else {
				ibufstart_in_file = (samptime/F_SECSIZE) * F_SECSIZE;
				if((sndseekEx(dz->ifd[0],ibufstart_in_file,0)) < 0) {
					sprintf(errstr,"sndseek() 3 failed\n");
					return(SYSTEM_ERROR);
				}
				dz->total_samps_read = ibufstart_in_file;
				if((exit_status = read_samps(buf,dz))<0)
					return(exit_status);
				if(dz->ssampsread == 0)
					break;
				if(sloom)
					display_virtual_time(dz->total_samps_read,dz);		
				samptime -= ibufstart_in_file;
				searchend = min(samptime + dz->iparam[PKCH_WSIZE],dz->ssampsread);
			}
		} else {
			samptime  -= ibufstart_in_file;
			searchend -= ibufstart_in_file;
		}
		maxtime = 0;
		maxsamp = fabs(buf[samptime++]);
		while(samptime < searchend) {
			val = fabs(buf[samptime]);
			if(val > maxsamp) {
				maxtime = samptime;
				maxsamp = val;
			}
			samptime++;
		}
		maxtime += ibufstart_in_file;
		maxtime /= dz->infile->channels;
		env[j] = (float)((double)maxtime * inv_srate);
	}
	return FINISHED;
}

/**************************** CALCULATE_EVENT_OUTTIMES *****************************/

int calculate_event_outtimes(int envcnt,int *outcnt,dataptr dz)
{
	int exit_status;
	double *outtime;
	int n;
	double lastnewpos, newpos, lastgap, nextgap, scatter;
	double rpet;
	int rpetmax;
	if(dz->brksize[PKCH_REPET] > 0) {
		if((exit_status = get_maxvalue_in_brktable(&rpet,PKCH_REPET,dz)) < 0)
			return(exit_status);
		rpetmax = (int)ceil(rpet);
		rpetmax++;
	} else
		rpetmax = dz->iparam[PKCH_REPET] + 1;
	*outcnt = envcnt * rpetmax;
	if((dz->parray[0] = (double *)malloc(sizeof(double) * *outcnt))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY establishing peak-outtimes array.\n");
		return(MEMORY_ERROR);
	}
	fprintf(stdout,"INFO: Generating output event times.\n");
	fflush(stdout);
	outtime = dz->parray[0];
	outtime[0] = 0.0;
	n = 1;
	if(dz->brksize[PKCH_TEMPO] == 0) {
		while(n < *outcnt) {
			outtime[n] = outtime[n-1] + dz->param[PKCH_TEMPO];
			n++;
		}
	} else {
		while(n < *outcnt) {
			read_value_from_brktable(outtime[n-1],PKCH_TEMPO,dz);
			dz->param[PKCH_TEMPO] = 60.0 / dz->param[PKCH_TEMPO];
			outtime[n] = outtime[n-1] + dz->param[PKCH_TEMPO];
			n++;
		}
	}
	if(dz->brksize[PKCH_SCAT] > 0 || dz->param[PKCH_SCAT] > 0.0) {
		newpos = outtime[0];
		for(n=2;n<*outcnt;n++) {				// scatter events about there exact positions
			lastnewpos = newpos;
			lastgap = outtime[n-1] - outtime[n-2];
			nextgap = outtime[n] - outtime[n-1];
			scatter = (drand48() * 2.0) - 1.0;
			if(dz->brksize[PKCH_SCAT] > 0)
				read_value_from_brktable(outtime[n-1],PKCH_SCAT,dz);
			scatter *= dz->param[PKCH_SCAT];
			if(scatter > 0.0) {
				nextgap *= scatter;
				newpos = outtime[n-1] + nextgap;
			} else {
				lastgap *= scatter;
				newpos = outtime[n-1] - lastgap;
			}
			outtime[n-2] = lastnewpos;
		}
	}
	return FINISHED;
}

/**************************** GENERATE_ENVELOPED_OUTPUT *****************************/

int generate_enveloped_output(int envcnt,int outcnt,dataptr dz)
{
	int exit_status, j, k, kk, finished = 0, mstep;
	float *ibuf = dz->sampbuf[0], *obuf1 = dz->sampbuf[1], *obuf2 = dz->sampbuf[2];
	int obufpos = 0, obufstart_in_file, obufend_in_file, n, m, outsamptime, seekback, chwidth, halfchwidth, centring, peakadj = 0;
	int startupsplice, endupsplice, startdownsplice, enddownsplice, splice_steps, envlen;
	int ibufpos, ibufstart_in_file, insamptime, readend_of_file, readlen;
	double srate = (double)dz->infile->srate;
	int chans = dz->infile->channels, repeater = 0;
	double *outtimes = dz->parray[0], outtime, splice_incr, spliceval;
	double this_norm, thisamp, normalisation, normalisation_incr, thisgain_incr, outmax = 0.0;

	fprintf(stdout,"INFO: Generating output sound.\n");
	fflush(stdout);

	memset((char *)obuf1,0,2 * dz->buflen * sizeof(float));	// Set double buffer to zero
	if((exit_status = read_samps(ibuf,dz))<0)
		return(exit_status);
	ibufstart_in_file = 0;
	obufstart_in_file  = 0;
	obufend_in_file    = dz->buflen;
	envlen = envcnt * 2;
	if(dz->brksize[PKCH_MISS] > 0) {
		read_value_from_brktable(outtimes[0],PKCH_MISS,dz);
		dz->iparam[PKCH_MISS]++;
	}
	if(dz->brksize[PKCH_WIDTH] == 0) {
		chwidth = dz->iparam[PKCH_WIDTH]/chans;
		halfchwidth = chwidth/2;
		centring = (int)round(dz->param[PKCH_SKEW] * chwidth);
		peakadj = (centring - halfchwidth) * chans;
	}
	mstep = dz->iparam[PKCH_MISS] * 2;
	for(n=0,m = 0;m<envlen;n++, m += mstep) {
		if(n >= outcnt)	// SAFETY
			break;
		outtime = outtimes[n];
		if(dz->brksize[PKCH_GAIN] > 0)
			read_value_from_brktable(outtime,PKCH_GAIN,dz);
		if(dz->brksize[PKCH_WIDTH] > 0) {
			read_value_from_brktable(outtime,PKCH_WIDTH,dz);
			dz->iparam[PKCH_WIDTH]  = (int)round(dz->param[PKCH_WIDTH] * MS_TO_SECS * srate) * chans;
			chwidth = dz->iparam[PKCH_WIDTH]/chans;
			halfchwidth = chwidth/2;
			centring = (int)round(dz->param[PKCH_SKEW] * chwidth);
			peakadj = (centring - halfchwidth) * chans;
		}
		if(dz->brksize[PKCH_SPLICE] > 0) {
			read_value_from_brktable(outtime,PKCH_SPLICE,dz);
			dz->iparam[PKCH_SPLICE] = (int)round(dz->param[PKCH_SPLICE] * MS_TO_SECS * srate) * chans;
		}
		if(dz->brksize[PKCH_MISS] > 0) {
			read_value_from_brktable(outtime,PKCH_MISS,dz);
			dz->iparam[PKCH_MISS]++;
			mstep = dz->iparam[PKCH_MISS] * 2;
		}
		if(dz->brksize[PKCH_REPET] > 0)
			read_value_from_brktable(outtime,PKCH_REPET,dz);
		if(dz->brksize[PKCH_NORM] > 0) 
			read_value_from_brktable(outtime,PKCH_NORM,dz);
		if(dz->param[PKCH_NORM] > 0.0)  {
			thisamp = dz->env[m+1];
			normalisation = min(dz->scalefact/thisamp,100.0);	// scalefact stores maxamplitude of source
			normalisation_incr = normalisation - 1.0;
			thisgain_incr = normalisation_incr * dz->param[PKCH_NORM];
			this_norm = 1.0 + thisgain_incr;
		} else
			this_norm  = 1.0;
		outsamptime = (int)round(outtime * (double)dz->infile->srate) * chans;
		while(outsamptime >= obufend_in_file) {
			for(kk=0;kk<dz->buflen;kk++)
				outmax = max(outmax,fabs((double)obuf1[kk]));
			if((exit_status = write_samps(obuf1,dz->buflen,dz))<0)
				return(exit_status);
			memcpy((char *)obuf1,(char *)obuf2,dz->buflen * sizeof(float));
			memset((char *)obuf2,0,dz->buflen * sizeof(float));
			obufend_in_file   += dz->buflen;
			obufstart_in_file += dz->buflen;
		}
		obufpos = outsamptime % dz->buflen;
		if((insamptime = (int)round(dz->env[m] * srate) * chans) >= dz->insams[0]) {
			break;
		}
		insamptime += peakadj;
		startupsplice = max(insamptime - dz->iparam[PKCH_SPLICE],0);
		endupsplice	  = insamptime;
		startdownsplice	= endupsplice + dz->iparam[PKCH_WIDTH];
		readend_of_file = 0;
		if((enddownsplice = startdownsplice + dz->iparam[PKCH_SPLICE]) >= dz->insams[0])
			readend_of_file = dz->insams[0] - insamptime;
		if(startupsplice < ibufstart_in_file) {			// Possible if segments are repeated, and they cross a buffer boundary
			seekback = dz->total_samps_read - dz->ssampsread - dz->buflen;
			if(seekback < 0) {
				sprintf(errstr,"BAD SEEK\n");
				return(PROGRAM_ERROR);
			}
			if((sndseekEx(dz->ifd[0],seekback,0)< 0)){
                sprintf(errstr,"sndseek() 5 failed\n");
                return SYSTEM_ERROR;
            }
			dz->total_samps_read = seekback;
			if((exit_status = read_samps(ibuf,dz))<0)
				return(exit_status);
			ibufstart_in_file -= dz->buflen;
		}
		while(startupsplice > dz->total_samps_read) {
			ibufstart_in_file += dz->ssampsread;
			if((exit_status = read_samps(ibuf,dz))<0)
				return(exit_status);
			if(dz->ssampsread == 0) {
				break;
			}
		}
		ibufpos = startupsplice - ibufstart_in_file;
		splice_steps = dz->iparam[PKCH_SPLICE]/chans;
		splice_incr = this_norm/(double)splice_steps;
		spliceval = 0.0;
		for(k = 0; k < splice_steps; k++) {
			for(j=0;j<chans; j++) {
				obuf1[obufpos] = (float)((obuf1[obufpos] + (ibuf[ibufpos] * spliceval)) * dz->param[PKCH_GAIN]);
				obufpos++;
				ibufpos++;
			}
			spliceval += splice_incr;
			if(ibufpos >= dz->ssampsread) {
				if(dz->ssampsread < dz->buflen) {
					finished = 1;
					break;
				}
				ibufstart_in_file += dz->buflen;
				ibufpos -= dz->buflen;
				if((exit_status = read_samps(ibuf,dz))<0)
					return(exit_status);
				if(dz->ssampsread == 0) {
					finished = 1;
					break;
				}
			}
			if(obufpos >= dz->buflen * 2) {
				sprintf(errstr,"AAA Peak-event length too large for buffer dblbuflen = %d eventlen = %d\n",dz->buflen * 2,(dz->iparam[PKCH_SPLICE] * 2) + dz->iparam[PKCH_WIDTH]);
				return(DATA_ERROR);
			}
		}
		if(finished)
			break;
		if(readend_of_file)
			readlen = readend_of_file;
		else
			readlen = dz->iparam[PKCH_WIDTH];
		for(k = 0; k < readlen; k++) {
			obuf1[obufpos] = (float)((obuf1[obufpos] + (this_norm * ibuf[ibufpos])) * dz->param[PKCH_GAIN]);
			ibufpos++;
			obufpos++;
			if(ibufpos >= dz->ssampsread) {
				if(dz->ssampsread < dz->buflen) {
					finished = 1;
					break;
				}
				ibufstart_in_file += dz->buflen;
				ibufpos -= dz->buflen;
				if((exit_status = read_samps(ibuf,dz))<0)
					return(exit_status);
				if(dz->ssampsread == 0) {
					finished = 1;
					break;
				}
			}
			if(obufpos >= dz->buflen * 2) {
				sprintf(errstr,"BBB Peak-event length too large for buffer dblbuflen = %d eventlen = %d\n",dz->buflen * 2,(dz->iparam[PKCH_SPLICE] * 2) + dz->iparam[PKCH_WIDTH]);
				return(DATA_ERROR);
			}
		}
		if(finished || readend_of_file)
			break;

		spliceval = this_norm;
		for(k = 0; k < splice_steps; k++) {
			for(j=0;j<chans; j++) {
				obuf1[obufpos] = (float)((obuf1[obufpos] + (ibuf[ibufpos] * spliceval)) * dz->param[PKCH_GAIN]);
				obufpos++;
				ibufpos++;
			}
			spliceval -= splice_incr;
			if(ibufpos >= dz->ssampsread) {
				if(dz->ssampsread < dz->buflen) {
					finished = 1;
					break;
				}
				ibufstart_in_file += dz->buflen;
				ibufpos -= dz->buflen;
				if((exit_status = read_samps(ibuf,dz))<0)
					return(exit_status);
				if(dz->ssampsread == 0) {
					finished = 1;
					break;
				}
			}
			if(obufpos >= dz->buflen * 2) {
				sprintf(errstr,"CCC Peak-event length too large for buffer dblbuflen = %d eventlen = %d\n",dz->buflen * 2,(dz->iparam[PKCH_SPLICE] * 2) + dz->iparam[PKCH_WIDTH]);
				return(DATA_ERROR);
			}
		}
		if(finished)
			break;
		if(repeater < dz->iparam[PKCH_REPET]) {		// rep;eater checked against current val of PKCH_REPET param, which amy change through time
			m -= mstep;								//	if not done enough repetitions, prevent advance along input peaks table
			repeater++;
		} else										// once PKCH_REPET repetitions done, allows advance along peak table
			repeater = 0;
	}
	if(obufpos > 0) {
		for(kk=0;kk<obufpos;kk++)
			outmax = max(outmax,fabs((double)obuf1[kk]));
		if((exit_status = write_samps(obuf1,obufpos,dz))<0)
			return(exit_status);
	}
	if(outmax >= 1.0) {
		fprintf(stdout,"ERROR: clipping! reduce level\n");
		fflush(stdout);
	}
	return FINISHED;
}

/**************************** GENERATE_ENVELOPE_AS_OUTPUT *****************************/

int generate_envelope_as_output(int envcnt,dataptr dz)
{
	int exit_status;
	float *ibuf = dz->sampbuf[0];
	double lasttime = 0.0, thistime;
	int m, chwidth, halfchwidth, centring, peakadj = 0;
	int startupsplice, endupsplice, startdownsplice, enddownsplice = 0, envlen;
	int ibufstart_in_file, insamptime;
	double srate = (double)dz->infile->srate, invsrate = 1.0/srate;
	int chans = dz->infile->channels;

	fflush(stdout);
	endupsplice = -1;
	if((exit_status = read_samps(ibuf,dz))<0)
		return(exit_status);
	ibufstart_in_file = 0;
	envlen = envcnt * 2;
	if(dz->brksize[PKCH_WIDTH] == 0) {
		chwidth = dz->iparam[PKCH_WIDTH]/chans;
		halfchwidth = chwidth/2;
		centring = (int)round(dz->param[PKCH_SKEW] * chwidth);
		peakadj = (centring - halfchwidth) * chans;
	}
	for(m = 0;m<envlen;m+= 2) {
		if(dz->brksize[PKCH_WIDTH] > 0) {
			read_value_from_brktable(dz->env[m],PKCH_WIDTH,dz);
			dz->iparam[PKCH_WIDTH]  = (int)round(dz->param[PKCH_WIDTH] * MS_TO_SECS * srate) * chans;
			chwidth = dz->iparam[PKCH_WIDTH]/chans;
			halfchwidth = chwidth/2;
			centring = (int)round(dz->param[PKCH_SKEW] * chwidth);
			peakadj = (centring - halfchwidth) * chans;
		}
		if(dz->brksize[PKCH_SPLICE] > 0) {
			read_value_from_brktable(dz->env[m],PKCH_SPLICE,dz);
			dz->iparam[PKCH_SPLICE] = (int)round(dz->param[PKCH_SPLICE] * MS_TO_SECS * srate) * chans;
		}
		if((insamptime = (int)round(dz->env[m] * srate) * chans) >= dz->insams[0])
			break;
		insamptime += peakadj;
		startupsplice = max(insamptime - dz->iparam[PKCH_SPLICE],0);
		if(startupsplice < endupsplice) {
			sprintf(errstr,"Peak envelopes overlap at time %lf\n",dz->env[m]);
			return(DATA_ERROR);
		}
		endupsplice	    = insamptime;
		startdownsplice	= endupsplice + dz->iparam[PKCH_WIDTH];
		enddownsplice   = startdownsplice +  dz->iparam[PKCH_SPLICE];
		if((thistime = (startupsplice/chans) * invsrate) < lasttime) {
			sprintf(errstr,"New peakwidth too wide for the input data.\n");
			return(DATA_ERROR);
		}
		lasttime = thistime;
		if((thistime = (endupsplice/chans) * invsrate) < lasttime) {
			sprintf(errstr,"New peakwidth too wide for the input data.\n");
			return(DATA_ERROR);
		}
		lasttime = thistime;
		if((thistime = (startdownsplice/chans) * invsrate) < lasttime) {
			sprintf(errstr,"New peakwidth too wide for the input data.\n");
			return(DATA_ERROR);
		}
		lasttime = thistime;
		if((thistime = (enddownsplice/chans) * invsrate) < lasttime || thistime <= 0.0) {
			sprintf(errstr,"New peakwidth too wide for the input data.\n");
			return(DATA_ERROR);
		}
		lasttime = thistime;
	}
	fprintf(stdout,"INFO: Generating output envelope.\n");
	for(m = 0;m<envlen;m+= 2) {
		if(dz->brksize[PKCH_WIDTH] > 0) {
			read_value_from_brktable(dz->env[m],PKCH_WIDTH,dz);
			dz->iparam[PKCH_WIDTH]  = (int)round(dz->param[PKCH_WIDTH] * MS_TO_SECS * srate) * chans;
			chwidth = dz->iparam[PKCH_WIDTH]/chans;
			halfchwidth = chwidth/2;
			centring = (int)round(dz->param[PKCH_SKEW] * chwidth);
			peakadj = (centring - halfchwidth) * chans;
		}
		if(dz->brksize[PKCH_SPLICE] > 0) {
			read_value_from_brktable(dz->env[m],PKCH_SPLICE,dz);
			dz->iparam[PKCH_SPLICE] = (int)round(dz->param[PKCH_SPLICE] * MS_TO_SECS * srate) * chans;
		}
		if((insamptime = (int)round(dz->env[m] * srate) * chans) >= dz->insams[0])
			break;
		insamptime += peakadj;
		startupsplice = max(insamptime - dz->iparam[PKCH_SPLICE],0);	//	e.g. At start of file, if peak is at zero, startupsplice -> 0
		endupsplice	    = insamptime;
		startdownsplice	= endupsplice + dz->iparam[PKCH_WIDTH];
		enddownsplice   = startdownsplice +  dz->iparam[PKCH_SPLICE];
		if(m == 0 && startupsplice != 0)
			fprintf(dz->fp,"%lf\t%lf\n",0.0,0.0);
																//	Splices are always > 0.0, except where peak is at start of file
																//	When startsplice can be 0.0. In this case endupsplice = startupsplice
		if(endupsplice > startupsplice)							//	So we must avoid time-duplication in the env file
			fprintf(dz->fp,"%lf\t%lf\n",(startupsplice/chans) * invsrate,0.0);
		fprintf(dz->fp,"%lf\t%lf\n",(endupsplice/chans) * invsrate,1.0);
		if(dz->iparam[PKCH_WIDTH] > 0)							//	Avoid duplication of time-point for zero-width peaks
			fprintf(dz->fp,"%lf\t%lf\n",(startdownsplice/chans) * invsrate,1.0);
		fprintf(dz->fp,"%lf\t%lf\n",(enddownsplice/chans) * invsrate,0.0);
	}
	if(enddownsplice < dz->insams[0])
		fprintf(dz->fp,"%lf\t%lf\n",(dz->insams[0]) * invsrate,0.0);
	return FINISHED;
}

/*************************** GETMAXSAMP *******************************/

double getmaxsamp(int startsamp, int sampcnt,float *buffer)
{
	int  i, endsamp = startsamp + sampcnt;
	double thisval, thismaxsamp = 0.0;
	for(i = startsamp; i<endsamp; i++) {
		if((thisval =  fabs(buffer[i]))>thismaxsamp)		   
			thismaxsamp = thisval;
	}
	return thismaxsamp;
}

/*************************** CREATE_PEAKCHOP_SNDBUFS **************************
 *
 * Buffer size is a power of two, and more than big enough to take max number of repeats at minimum tempo
 */

int create_peakchop_sndbufs(dataptr dz)
{
	int exit_status, n;
	int bigbufsize, twopow;
	double rpet, tempo, mindur, pkwidth, splice;
	int rpetmax;
	if(dz->sbufptr == 0 || dz->sampbuf==0) {
		sprintf(errstr,"buffer pointers not allocated: create_sndbufs()\n");
		return(PROGRAM_ERROR);
	}
	if(dz->brksize[PKCH_REPET] > 0) {
		if((exit_status = get_maxvalue_in_brktable(&rpet,PKCH_REPET,dz)) < 0)
			return(exit_status);
		rpetmax = (int)ceil(rpet);
		rpetmax++;
	} else
		rpetmax = dz->iparam[PKCH_REPET] + 1;
	if(dz->brksize[PKCH_TEMPO] > 0) {
		if((exit_status = get_minvalue_in_brktable(&tempo,PKCH_TEMPO,dz)) < 0)
			return(exit_status);
		tempo = 60.0 / tempo;
	} else
		tempo = dz->param[PKCH_TEMPO];
	mindur = tempo * rpetmax * 4.0;
	if(dz->brksize[PKCH_WIDTH] > 0) {
		if((exit_status = get_maxvalue_in_brktable(&pkwidth,PKCH_WIDTH,dz)) < 0)
			return(exit_status);
		pkwidth *= MS_TO_SECS;
	} else
		pkwidth = dz->param[PKCH_WIDTH] * MS_TO_SECS;
	if(dz->brksize[PKCH_SPLICE] > 0) {
		if((exit_status = get_maxvalue_in_brktable(&splice,PKCH_SPLICE,dz)) < 0)
			return(exit_status);
		splice *= MS_TO_SECS;
	} else
		splice = dz->param[PKCH_SPLICE] * MS_TO_SECS;
	pkwidth += (2 * splice);
	mindur = max(mindur,pkwidth * 4);
	bigbufsize = (int)ceil(mindur * (double)dz->infile->srate) * dz->infile->channels;
	twopow = 2;
	while(twopow < bigbufsize)
		twopow *= 2;
	bigbufsize = twopow;
	dz->buflen = bigbufsize;	
	if((dz->bigbuf = (float *)calloc(dz->buflen * dz->bufcnt,sizeof(float))) == NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
		return(PROGRAM_ERROR);
	}
	for(n=0;n<dz->bufcnt;n++)
		dz->sbufptr[n] = dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
	dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
	return(FINISHED);
}

/********************* BEYOND_ENDTIME *********************/

int beyond_endtime(char **cmdline,int cmdlinecnt,dataptr dz)
{
	float endtime;
	if(cmdlinecnt < 2)
		return 0;
	if(!this_value_is_numeric(cmdline[1]))
		return 0;
	if(sscanf(cmdline[1],"%f",&endtime)<1)
		return 0;
	if(endtime > dz->duration)
		return 1;
	return 0;
}

/********************* THIS_VALUE_IS_NUMERIC *********************/

int this_value_is_numeric(char *str)
{   char *p, *q, *end;
	int	point, valid;
	for(;;) {
		point = 0;
		p = str;
		while(isspace(*p))
			p++;
		q = p;	
		if(!isdigit(*p) && *p != '.' && *p!='-')
			return(0);
		if(*p == '.'|| *p == '-') {
			if(*p == '-') {
				p++;
			} else {
				point++;
				p++;
			}
		}
		for(;;) {
			if(*p == '.') {
				if(point)
					return(0);
				else {
					point++;
					p++;
					continue;
				}
			}
			if(isdigit(*p)) {
				p++;
				continue;
			} else {
				if(!isspace(*p) && *p!=ENDOFSTR)
					return(0);
				else {
					end = p;
				    p = q;
					valid = 0;
		    		while(p!=end) {
					   	if(isdigit(*p))
					  		valid++;
						p++;
					}
					if(valid)
						return(1);
					return(0);
				}
			}
 		}
	}
	return(0);				/* NOTREACHED */
}
