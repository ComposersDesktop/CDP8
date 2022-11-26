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



//
//	BUG dz->brksize[ITF_DEL] IS 14 .... should be much bigger ... this is 



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
#include <science.h>
#include <ctype.h>
#include <sfsys.h>
#include <string.h>
#include <srates.h>
#include <pnames.h>
#include <extdcon.h>
#include <limits.h>
#ifdef unix
#include <aaio.h>
#endif


#ifdef unix
#define round(x) lround((x))
#endif

char errstr[2400];

#define infilespace  rampbrksize
#define ampvaried    is_rectified
#define overflowsize temp_sampsize

int anal_infiles = 1;
int	sloom = 0;
int sloombatch = 0;

#define MINFADE	2		// minimum fade of element in MS

const char* cdp_version = "7.1.0";

//CDP LIB REPLACEMENTS
static int setup_iterfof_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_iterfof_param_ranges_and_defaults(dataptr dz);
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
static int get_the_mode_from_cmdline(char *str,dataptr dz);
static int setup_and_init_input_brktable_constants(dataptr dz,int brkcnt);
static int iterfof(dataptr dz);
static int create_the_iterbufs(dataptr dz);
static int do_iterfof_preprocess(dataptr dz);
static int read_stepd_delay_value(double thistime,double *notestarttime,double *noteendtime,double *portint,dataptr dz);
static int transpos_read_element(int *transposcnt,double incr,dataptr dz);
static int dovibrato(int *delays,dataptr dz);
static int setup_lineportion_params(double *linegains,double *linefades, double *upfades, double *linegaps,dataptr dz);
static int read_lineportion_gain(double thistime,double *lasttime,double *linegain,double *linegains,dataptr dz);
static int read_lineportion_envelope(double thistime,double *envlasttime,int *linedur,int *linefade,double *linefadeincr,int *upfade,double *upfadeincr, double *lineenv,
							double *linefades,double *upfades,double *linegaps, int minfade, double *pshift, dataptr dz);
static double portion_env(int *linedur,int *linefade,double *lineenv,double linefadeincr,int *upfade,double upfadeincr);

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
		dz->maxmode = 4;
		if((exit_status = get_the_mode_from_cmdline(cmdline[0],dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(exit_status);
		}
		cmdline++;
		cmdlinecnt--;
		// setup_particular_application =
		if((exit_status = setup_iterfof_application(dz))<0) {
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
	if((exit_status = setup_iterfof_param_ranges_and_defaults(dz))<0) {
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
//	handle_formant_quiksearch()	redundant
//	handle_the_special_data()	redundant
	if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {		// CDP LIB
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
//	check_param_validity_and_consistency() redundant
	is_launched = TRUE;
	dz->bufcnt = 4;
	if((dz->sampbuf = (float **)malloc(sizeof(float *) * (dz->bufcnt+1)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffers.\n");
		return(MEMORY_ERROR);
	}
	if((dz->sbufptr = (float **)malloc(sizeof(float *) * dz->bufcnt+1))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffer pointers.\n");
		return(MEMORY_ERROR);
	}
	for(n = 0;n <dz->bufcnt; n++)
		dz->sampbuf[n] = dz->sbufptr[n] = (float *)0;
	dz->sampbuf[n] = (float *)0;

	if((exit_status = do_iterfof_preprocess(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	//spec_process_file =
	if((exit_status = iterfof(dz))<0) {
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

/************************* SETUP_ITERLINE_APPLICATION *******************/

int setup_iterfof_application(dataptr dz)
{
	int exit_status;
	aplptr ap;
	if((exit_status = establish_application(dz))<0)		// GLOBAL
		return(FAILED);
	ap = dz->application;
	if((exit_status = set_param_data(ap,0,2,2,"Dd"))<0)
		return(FAILED);
	if(EVEN(dz->mode))
		exit_status = set_vflgs(ap,"patTErvVdD",10,"DDDDdDDDDD","s",1,1,"i");
	else
		exit_status = set_vflgs(ap,"patTErvVdDgGFfSPi",17,"DDDDdDDDDDDDDDDiD","s",1,1,"i");
	if(exit_status < 0)
		return FAILED;
	// set_legal_infile_structure -->
	dz->has_otherfile = FALSE;
	// assign_process_logic -->
	dz->input_data_type = SNDFILES_ONLY;
	dz->process_type	= UNEQUAL_SNDFILE;	
	dz->outfiletype  	= SNDFILE_OUT;
	dz->maxmode = 4;
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
		} else if(infile_info->channels != MONO)  {
			sprintf(errstr,"File %s is not of correct type: must be MONO\n",cmdline[0]);
			return(DATA_ERROR);
		} else if((exit_status = copy_parse_info_to_main_structure(infile_info,dz))<0) {
			sprintf(errstr,"Failed to copy file parsing information\n");
			return(PROGRAM_ERROR);
		}
		free(infile_info);
	}
	return(FINISHED);
}

/************************* SETUP_ITERLINE_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_iterfof_param_ranges_and_defaults(dataptr dz)
{
	int exit_status;
	aplptr ap = dz->application;
	// set_param_ranges()
	ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
	// NB total_input_param_cnt is > 0 !!!
	if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
		return(FAILED);
	// get_param_ranges()
	if(dz->mode <2) {
		ap->lo[ITF_DEL] 	= -24;
		ap->hi[ITF_DEL] 	= 12;
		ap->default_val[ITF_DEL] = 0; 	
	} else {
		ap->lo[ITF_DEL] 	= 24;
		ap->hi[ITF_DEL] 	= 96;
		ap->default_val[ITF_DEL] = 60; 	
	}
	ap->lo[ITF_DUR] 	= dz->duration;
	ap->hi[ITF_DUR] 	= BIG_TIME;
	ap->default_val[ITF_DUR] = 20; 	
	ap->lo[ITF_RAND]	= 0.0;
	ap->hi[ITF_RAND]	= 1.0;
	ap->default_val[ITF_RAND] = 0.0;
	ap->lo[ITF_VMIN]	= 0.0;
	ap->hi[ITF_VMIN]	= 20.0;
	ap->default_val[ITF_VMIN] = 0.0;
	ap->lo[ITF_VMAX]	= 0.0;
	ap->hi[ITF_VMAX]	= 20.0;
	ap->default_val[ITF_VMAX] = 0.0;
	ap->lo[ITF_DMIN]	= 0.0;
	ap->hi[ITF_DMIN]	= 2.0;
	ap->default_val[ITF_DMIN] = 0.0;
	ap->lo[ITF_DMAX]	= 0.0;
	ap->hi[ITF_DMAX]	= 2.0;
	ap->default_val[ITF_DMAX] = 0.0;
	ap->lo[ITF_PRND]	= 0.0;
	ap->hi[ITF_PRND]	= 2.0;
	ap->default_val[ITF_PRND] = 0.0;
	ap->lo[ITF_AMPC]	= 0.0;
	ap->hi[ITF_AMPC]	= 1.0;
	ap->default_val[ITF_AMPC] = 0.0;
	ap->lo[ITF_TRIM]	= 0.0;
	ap->hi[ITF_TRIM]	= dz->duration;
	ap->default_val[ITF_TRIM] = 0.0;
	ap->lo[ITF_TRBY]	= 0.0;
	ap->hi[ITF_TRBY]	= dz->duration;
	ap->default_val[ITF_TRBY] = 0.0;
	ap->lo[ITF_SLOP]	= 1.0;
	ap->hi[ITF_SLOP]	= 4.0;
	ap->default_val[ITF_SLOP] = 1.0;
	if(EVEN(dz->mode)) {
		ap->lo[ITF_SEED1]	= 0.0;
		ap->hi[ITF_SEED1]	= MAXSHORT;
		ap->default_val[ITF_SEED1] = 0.0;
	} else {
		ap->lo[ITF_GMIN]	= 0.0;
		ap->hi[ITF_GMIN]	= 1.0;
		ap->default_val[ITF_GMIN] = 1.0;
		ap->lo[ITF_GMAX]	= 0.0;
		ap->hi[ITF_GMAX]	= 1.0;
		ap->default_val[ITF_GMAX] = 1.0;
		ap->lo[ITF_UFAD]	= 0.0;
		ap->hi[ITF_UFAD]	= 10;
		ap->default_val[ITF_UFAD] = 0.0;
		ap->lo[ITF_FADE]	= 0.0;
		ap->hi[ITF_FADE]	= 10;
		ap->default_val[ITF_FADE] = 0.0;
		ap->lo[ITF_GAPP]	= 0.0;
		ap->hi[ITF_GAPP]	= 1.0;
		ap->default_val[ITF_GAPP] = 0.0;
		ap->lo[ITF_PORT]	= -1;
		ap->hi[ITF_PORT]	= 2.0;
		ap->default_val[ITF_PORT] = 0.0;
		ap->lo[ITF_PINT]	= 0.0;
		ap->hi[ITF_PINT]	= 2.0;
		ap->default_val[ITF_PINT] = 0.0;
		ap->lo[ITF_SEED2]	= 0.0;
		ap->hi[ITF_SEED2]	= MAXSHORT;
		ap->default_val[ITF_SEED2] = 0.0;
	}
	dz->maxmode = 4;
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
			if((exit_status = setup_iterfof_application(dz))<0)
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
	int exit_status;
	exit_status = set_internalparam_data("diiii", ap);
	return(exit_status);
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
	usage2("iterfof");
	return(USAGE_ONLY);
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	if(!strcmp(prog_identifier_from_cmdline,"iterfof"))				dz->process = ITERFOF;
	else {
		fprintf(stderr,"Unknown program identification string '%s'\n",prog_identifier_from_cmdline);
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
	if(!strcmp(str,"iterfof")) {
		fprintf(stderr,
	    "USAGE:\n"
	    "iterfof iterfof 1-4 infile outfile linedata outduration\n"
		"     [-pprand] [-aampcut] [-ttrimto] [-Ttrimby] [-Etrimslope]\n"
		"     [-rrand] [-vvibmin] [-Vvibmax] [-depmin] [-Ddepmax]\n"
		"     [-ggainmin] [-Ggainmax] [-Fupfade] [-ffade] [-Sseparation]\n"
		"     [-Pportamento] [-iinterval]\n"
		"     [-sseed]\n"
		"\n"
		"ITERATE AN INPUT FOF OR SOUND-PACKET, FOLLOWING A DEFINED PITCHLINE.\n"
		"\n"
		"MODES 1 & 2: linedata is transposition in semitones.\n"
		"(Input sound assumed to represent 1 wavelength, at zero transposition.)\n"
		"MODES 3 & 4: linedata is MIDI pitch.\n"
		"(Input sound can be anything.)\n"
		"\n"
		"LINEDATA   Modes 1 & 2: semitone-transposition (can vary through time).\n"
		"           (varies delay between segments in output)\n"
		"           Modes 3 & 4: MIDI-pitch (can vary through time).\n"
		"IF \"linedata\" is a breakpoint file\n"
		"MODES 1 & 3: interpolate between timed values in the file.\n"
		"MODES 2 & 4: step between timed values in the file.\n"
		"\n"
		"SEED       the same seed-number will produce similar output on rerun.\n"
		"\n"
		"(FOR MORE INFORMATION ----- hit any key)\n"
		"\n");
	}
	while(!kbhit())
		;
	if(kbhit()) {
		fprintf(stderr,
		"Parameters of the line-elements\n"
		"\n"
		"PRAND      randomises pitch of segments (semitones in range +- 2)\n"
		"AMPCUT     max of random amp-reduction on each iter: Range 0-1:\n"
		"           default 0 (no amp-reduction)\n"
		"TRIMTO     Shorten elements to specified duration\n"
		"TRIMBY     Fade elements over specified duration\n"
		"TRIMSLOPE  Slope of any fades\n"
		"\n"
		"Parameters of the line\n"
		"\n"
		"RAND       transposition/pitch randomisation: Range 0 - 1: Default 0\n"
		"           (randomises delay between segments in output)\n"
		"VIBMIN/MAX Min & max vib frequency (frq set rand between these).\n"
		"DEPMIN/MAX Min & max vib depth (semitones) (depth set rand between these).\n"
		"(Modes 2 and 4 only)\n"
		"GAINMIN/MAX min and max level of stable-pitched portions of line.\n"
		"UPFADE     duration of fade into stable-pitched portions.\n"
		"FADE       duration of fade out of stable-pitched portions.\n"
		"SEPARATION end-fraction of any stable-pitched portion which is silent.\n"
		"PORTAMENTO type of line-portamento (if any)\n"
		"           0 = none : 1 = up : -1 = down : 2 = randomly up/down\n"
		"INTERVAL   Portamento interval (semitones : range 0 - 2)\n"
		"           Interval attained only by start of next note.\n"
		"           (If note fades before next note enters, interval not reached).\n");
	} else
		fprintf(stderr,"Unknown option '%s'\n",str);
	return(USAGE_ONLY);
}

int usage3(char *str1,char *str2)
{
	fprintf(stderr,"Insufficient parameters on command line.\n");
	return(USAGE_ONLY);
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
			fprintf(stderr,"Invalid mode of program entered.\n");
			return(USAGE_ONLY);
		}
		p--;
	}
	if(sscanf(str,"%d",&dz->mode)!=1) {
		fprintf(stderr,"Cannot read mode of program.\n");
		return(USAGE_ONLY);
	}
	if(dz->mode <= 0 || dz->mode > dz->maxmode) {
		fprintf(stderr,"Program mode value [%d] is out of range [1 - %d].\n",dz->mode,dz->maxmode);
		return(USAGE_ONLY);
	}
	dz->mode--;		/* CHANGE TO INTERNAL REPRESENTATION OF MODE NO */
	return(FINISHED);
}

/**************************** DO_ITERATE_PREPROCESS ******************************/

int do_iterfof_preprocess(dataptr dz)
{
	int n, m;
	double wavelen, val, srate;
	if(dz->brksize[ITF_DEL] == 1) {
		dz->param[ITF_DEL] = dz->brk[ITF_DEL][1];
		dz->brksize[ITF_DEL] = 0;					//	Shrink 1-line brkfile to single value parameter
	}
	if(dz->mode < 2) {	//	Data is transpositions
		wavelen = (double)dz->insams[0];
		if(dz->brksize[ITF_DEL]) {
			for(n=0,m=1;n<dz->brksize[ITF_DEL];n++,m+=2) {
				val = pow(2.0,dz->brk[ITF_DEL][m]/SEMITONES_PER_OCTAVE); //	Semitones to frq ratio
				dz->brk[ITF_DEL][m] = round(wavelen/val);				//	Delay = wavelen/frqratio
			}
		} else {
			val = pow(2.0,dz->param[ITF_DEL]/SEMITONES_PER_OCTAVE);		//	Semitones to frq ratio
			dz->param[ITF_DEL] = round(wavelen/val);					//	Delay = wavelen/frqratio
		}
	} else {
		srate = (double)dz->infile->srate;
		if(dz->brksize[ITF_DEL]) {
			for(n=0,m=1;n<dz->brksize[ITF_DEL];n++,m+=2) {
				val = miditohz(dz->brk[ITF_DEL][m]);					//	MIDI to frq
				dz->brk[ITF_DEL][m] = round(srate/val);					//	Delay = srate/frq
			}
		} else {
			val = miditohz(dz->param[ITF_DEL]);							//	MIDI to frq
			dz->param[ITF_DEL] = round(srate/val);						//	Delay = srate/frq
		}
	}
	if(dz->brksize[ITF_AMPC] == 0 && dz->param[ITF_AMPC] == 0.0)
		dz->ampvaried = 0;
	else
		dz->ampvaried = 1;
	if(ODD(dz->mode)) {
		if(dz->brksize[ITF_DEL] == 0) {		//	If only a single transpos/pitch specified
			if(dz->brksize[ITF_GMIN] || dz->brksize[ITF_GMAX] || dz->brksize[ITF_FADE] || dz->brksize[ITF_GAPP]) {
				sprintf(errstr,"Line portion parameters set as time-varying, but only one note is being generated.\n");
				return DATA_ERROR;
			}
		}
	}
	return create_the_iterbufs(dz);
}

/*************************** CREATE_THE_ITERBUFS **************************
 *
 * (1)	Create extra spaces for interpolation guard points at end of infile.
 *
 * (2) 	Allow for transposition of source
 *
 * (3)	Output buffer must be at least as big as the overflow buffer.
 *	Output buffer must be big enough for the whole of any possible
 *	data overflow (in overflow_size buff) to be copied back into it.
 *	This is because the overflow buffer is ZEROED after such a copy
 *	and if a 2nd copy of the overflow back into the main buffer
 *	were necessary , we would be copying zeroes rather than true data.
 *
 *
 *		true buffer (insams[0])    			(insams[0]) + SAFETY
 *  |-----------------------------|------------------------------------------|
 *								  ^					worst					 ^
 *								  ^				possible case		  		 ^
 *  |<-------- BUFFER_SIZE------->
 *
 */

int create_the_iterbufs(dataptr dz)
{
	int seccnt;
	int extra_space, infile_space, big_buffer_size;
	int overflow_size;

	infile_space = dz->insams[0] + 1;					/* Allows for wraparound point for tansposition interps */
	overflow_size = dz->insams[0];
	overflow_size *= 2; 									/* Allows for transposition of source up to 1 octave lower */	
	if((seccnt = overflow_size/F_SECSIZE) * F_SECSIZE < overflow_size)
		seccnt++;
	overflow_size = F_SECSIZE * overflow_size;
	if((seccnt = infile_space/F_SECSIZE) * F_SECSIZE < infile_space)
		seccnt++;
	infile_space = F_SECSIZE * seccnt;

	extra_space = infile_space + (overflow_size * 2);	/* overflow_size for overflow, and also for transposition buffer */
	dz->buflen     = infile_space;
	big_buffer_size = dz->buflen + extra_space;
	if((dz->bigbuf = (float *)Malloc(big_buffer_size * sizeof(float)))==NULL) {
		sprintf(errstr, "INSUFFICIENT MEMORY to create sound buffers.\n");
		return(MEMORY_ERROR);
	}
	dz->sbufptr[0] = dz->sampbuf[0] = dz->bigbuf;						//  INPUT BUFFER
	dz->sbufptr[1] = dz->sampbuf[1] = dz->sampbuf[0] + infile_space;	//	OUTPUT BUFFER
	dz->sbufptr[2] = dz->sampbuf[2] = dz->sampbuf[1] + dz->buflen;		//	OUTPUT BUFFER OVERFLOW
	dz->sbufptr[3] = dz->sampbuf[3] = dz->sampbuf[2] + overflow_size;	//	TRANSPOSITION BUFFER
	dz->sbufptr[4] = dz->sampbuf[4] = dz->sampbuf[3] + overflow_size;
	memset((char *)dz->sampbuf[0],0,(size_t)(infile_space * sizeof(float)));
	memset((char *)dz->sampbuf[1],0,(size_t)(dz->buflen * sizeof(float)));
	memset((char *)dz->sampbuf[2],0,(size_t)(overflow_size * sizeof(float)));
	dz->infilespace = infile_space; 
	dz->overflowsize = overflow_size;
	return(FINISHED);
}

/****************************** ITERFOF *************************/

#define ACCEPTABLE_MAXLEVEL 0.9

int iterfof(dataptr dz)
{
	int    exit_status, dovib;
	int   n, m, k, obufpos, absobufpos, *delays, ldelay, transposcnt, last_write = 0, this_last_write, local_last_write, minfade;
	int    bufs_written, bufs_before;
	double time = 0.0, normaliser = 1.0, gain, delay, srate = (double)dz->infile->srate, *gains, *pshifts;
	double *linegains = NULL, *linefades = NULL, *upfades = NULL, *linegaps = NULL;
	double maxoutsamp = 0.0, pshift;
	int    passno;
	float  *ibuf = dz->sampbuf[0], *obuf = dz->sampbuf[1], *ovflw = dz->sampbuf[2], *tbuf = dz->sampbuf[3], *getbuf;
	int   dozeros, trimdur, trimfade, fade_to, write_to;
	double spliceval, spliceincr, splicer, lasttime, envlasttime, lineoutgain, outtime;
	double linegain, linefadeincr = 0.0, lineenv = 1.0, portint = 0.0;
	int   linedur = 0, linefade = 0, upfade = 0;
	double upfadeincr = 0.0, notestarttime = 0.0, noteendtime = 0.0;
	double thisdelay, time_in_note, note_dur, timefrac, current_port_int, current_port_trans;
	int   itfseed;
	if(EVEN(dz->mode))
		itfseed = dz->iparam[ITF_SEED1];
	else
		itfseed = dz->iparam[ITF_SEED2];
	if(sloom)
		dz->tempsize = (int)round(dz->param[ITF_DUR] * srate) * dz->infile->channels;
	srand((int)itfseed);
	initrand48();

	minfade = (int)round(MINFADE * MS_TO_SECS * srate);

	print_outmessage_flush("Initial pass, checking segment count.\n");
	k = 0;
	notestarttime = -1.0;
	noteendtime = 0.0;
	while(time < dz->param[ITF_DUR]) {
		if(dz->brksize[ITF_DEL]) {
			if(ODD(dz->mode)) {
				if((exit_status = read_stepd_delay_value(time,&notestarttime,&noteendtime,&portint,dz))<0)
					return exit_status;
				thisdelay = dz->param[ITF_DEL];
				if(!flteq(portint,0.0)) {							//	If portamento interval > 0
					time_in_note = time - notestarttime;			//	Find fraction of time into current note
					note_dur = noteendtime - notestarttime;
					timefrac = time_in_note/note_dur;
					current_port_int = portint * timefrac;
					current_port_trans = pow(2.0,(current_port_int/SEMITONES_PER_OCTAVE));
					thisdelay /= current_port_trans;
				}
			} else {
				if((exit_status = read_value_from_brktable(time,ITF_DEL,dz))<0)
					return exit_status;
				thisdelay = dz->param[ITF_DEL];
			}
		} else
			thisdelay = dz->param[ITF_DEL];

		if(dz->brksize[ITF_RAND]) {
			if((exit_status = read_value_from_brktable(time,ITF_RAND,dz))<0)
				return exit_status;
		}
		if(dz->param[ITF_RAND] > 0.0) {
			delay = (drand48() * 2.0) - 1.0;		//	Range -1 to +1
			delay /= 2.0;							//	Range -1/2 to +1/2
			delay *= dz->param[ITF_RAND];			//	Range scaled to +- rand max * 1/2
			delay += 1.0;							//	Range scaled to 1 +- rand max * 1/2
			delay *= thisdelay;
			ldelay = (int)round(delay);
		} else
			ldelay = (int)round(thisdelay);
		time += (double)ldelay/srate;				//	update time for next table-read
		k++;
	}
	dz->itemcnt = k;
	if((delays = (int *)malloc(sizeof(int) * (k + 20)))==NULL) {		//	20 is safety margin
		sprintf(errstr,"INSUFFICIENT MEMORY establishing delays store.\n");
		return(MEMORY_ERROR);
	}
	if((gains = (double *)malloc(sizeof(double) * (k + 20)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY establishing gains store.\n");
		return(MEMORY_ERROR);
	}
	if((pshifts = (double *)malloc(sizeof(double) * (k + 20)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY establishing gains store.\n");
		return(MEMORY_ERROR);
	}
	if(ODD(dz->mode)) {
		if((linegains = (double *)malloc(sizeof(double) * (dz->brksize[ITF_DEL] + 1)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY establishing linegains store.\n");
			return(MEMORY_ERROR);
		}
		if((linefades = (double *)malloc(sizeof(double) * (dz->brksize[ITF_DEL] + 1)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY establishing linefades store.\n");
			return(MEMORY_ERROR);
		}
		if((upfades = (double *)malloc(sizeof(double) * (dz->brksize[ITF_DEL] + 1)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY establishing linefades store.\n");
			return(MEMORY_ERROR);
		}
		if((linegaps = (double *)malloc(sizeof(double) * (dz->brksize[ITF_DEL] + 1)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY establishing linegaps store.\n");
			return(MEMORY_ERROR);
		}
		if((exit_status = setup_lineportion_params(linegains,linefades,upfades,linegaps,dz))<0)
			return exit_status;
	}


	srand((int)itfseed);				//	Restart SAME random sequence
	initrand48();
	notestarttime = -1.0;				//	Initialise reading of note starttimes
	noteendtime = 0.0;
	portint = 0.0;

	sndseekEx(dz->ifd[0],0L,0);
	memset((char *)dz->sampbuf[1],0,(dz->sampbuf[3] - dz->sampbuf[1]) * sizeof(float));
	time = 0.0;
	k = 0;
	while(time < dz->param[ITF_DUR]) {
		if(dz->brksize[ITF_DEL]) {
			if(ODD(dz->mode)) {
				if((exit_status = read_stepd_delay_value(time,&notestarttime,&noteendtime,&portint,dz))<0)
					return exit_status;
				thisdelay = dz->param[ITF_DEL];
				if(!flteq(portint,0.0)) {						//	If portamento interval > 0
					time_in_note = time - notestarttime;		//	Find fraction of time into current note
					note_dur = noteendtime - notestarttime;
					timefrac = time_in_note/note_dur;
					current_port_int = portint * timefrac;
					current_port_trans = pow(2.0,(current_port_int/SEMITONES_PER_OCTAVE));
					thisdelay /= current_port_trans;			//	Adjust current delay for any portamento
				}
			} else {
				if((exit_status = read_value_from_brktable(time,ITF_DEL,dz))<0)
					return exit_status;
				thisdelay = dz->param[ITF_DEL];
			}
		} else
			thisdelay = dz->param[ITF_DEL];

		if(dz->brksize[ITF_RAND]) {
			if((exit_status = read_value_from_brktable(time,ITF_RAND,dz))<0)
				return exit_status;
		}
		if(dz->param[ITF_RAND] > 0.0) {
			delay = (drand48() * 2.0) - 1.0;		//	Range -1 to +1
			delay /= 2.0;							//	Range -1/2 to +1/2
			delay *= dz->param[ITF_RAND];			//	Range scaled to +- rand max * 1/2
			delay += 1.0;							//	Range scaled to 1 +- rand max * 1/2
			delay *= thisdelay;
			ldelay = (int)round(delay);
		} else
			ldelay = (int)round(thisdelay);
		delays[k] = ldelay;
		time += (double)ldelay/srate;				//	update time for next table-read
		k++;
	}

	dovib = 1;
	if(dz->brksize[ITF_VMIN]==0 && flteq(dz->param[ITF_VMIN],0.0)
	&& dz->brksize[ITF_VMAX]==0 && flteq(dz->param[ITF_VMAX],0.0)
	&& dz->brksize[ITF_DMIN]==0 && flteq(dz->param[ITF_DMIN],0.0)
	&& dz->brksize[ITF_DMAX]==0 && flteq(dz->param[ITF_DMAX],0.0))
		dovib = 0;
	if(dovib) {
		if((exit_status = dovibrato(delays,dz))<0)
			return exit_status;
	}
	memset((char *)dz->sampbuf[0],0,(dz->sampbuf[3] - dz->sampbuf[0]) * sizeof(float));

	if((exit_status = read_samps(ibuf,dz))<0)
		return(exit_status);

	for(passno=2;passno<4;passno++) {
		sndseekEx(dz->ifd[0],0L,0);
		memset((char *)dz->sampbuf[1],0,(dz->sampbuf[3] - dz->sampbuf[1]) * sizeof(float));
		time = 0.0;
		lasttime = -1.0;
		envlasttime = -1.0;
		obufpos = 0;
		absobufpos = 0;
		bufs_written = 0;
		k = 0;
		if(passno == 2)
			print_outmessage_flush("Second pass, assessing level.\n");
		else {
			print_outmessage_flush("Third pass, writing sound.\n");
			dz->tempsize = dz->total_samps_written;
			dz->total_samps_written = 0;
		}
		display_virtual_time(0L,dz);
		fflush(stdout);
		for(k = 0;k < dz->itemcnt;k++) {
			if(passno == 2) {
				if(dz->brksize[ITF_PRND]) {
					if((exit_status = read_value_from_brktable(time,ITF_PRND,dz))<0)
						return exit_status;
				}
				if(flteq(dz->param[ITF_PRND],0.0))
					pshifts[k] = 1.0;
				else {
					pshift = (drand48() * 2.0) - 1.0;
					pshift *= dz->param[ITF_PRND];
					pshift = pow(2.0,pshift/SEMITONES_PER_OCTAVE);	//	semitones to frq ratio
					pshifts[k] = 1.0/pshift;						//	frq ratio to read-increment
				}
				if(dz->ampvaried) {
					gain = drand48() * dz->param[ITF_AMPC];	//	Random amp reduction up to max at ITF_MAXC
					gain = 1.0 - gain;						//	Actual gain
				} else
					gain = 1.0;
				if(ODD(dz->mode)) {
					if((exit_status = read_lineportion_gain(time,&lasttime,&linegain,linegains,dz))<0)
						return exit_status;
					gain *= linegain;
				}
				gains[k] = gain;							//	On pass 2, store gain
			}
			ldelay = delays[k];								//	Get stored delay
			
			gain = gains[k] * normaliser;					//	On pass 2, normaliser = 1: On pass 3, gain gets adjusted by normaliser (if ness)
			
			if(dz->brksize[ITF_TRIM]) {
				if((exit_status = read_value_from_brktable(time,ITF_TRIM,dz))<0)
					return exit_status;
			}											
			trimdur = 0;								//	Trim ZERO indicates no trim
			if(dz->param[ITF_TRIM] > 0.0) 
				trimdur = (int)round(dz->param[ITF_TRIM] * srate);

			if(dz->brksize[ITF_TRBY]) {
				if((exit_status = read_value_from_brktable(time,ITF_TRBY,dz))<0)
					return exit_status;
			}											
			trimfade = 0;								//	Trimfade ZERO indicates no fade
			if(dz->param[ITF_TRBY] > 0.0)
				trimfade = (int)round(dz->param[ITF_TRBY] * srate);
			
			if(!flteq(pshifts[k],1.0)) {
				if((exit_status = transpos_read_element(&transposcnt,pshifts[k],dz)) < 0)
					return exit_status;						//	Transpos source into intermediate tbuf
				getbuf = tbuf;								//	Set to read from tbuf
			} else {
				transposcnt = dz->insams[0];				//	Simply a 1-1 read from input buffer
				getbuf = ibuf;								//	Set to read from input buffer
			}
			dozeros = 0;			
			if(trimdur > 0) {								//	If trim is to be done
				dozeros = max(transposcnt - trimdur,0);		//	Can the trim be done on the length of sound available (some smaples will be zeroed) ??
				if(dozeros  > 0) {							//	If trim can be done
					if(trimfade == 0)						//	If no fade is specified, use minimum fade (if poss)
						trimfade = min(transposcnt - dozeros,minfade);
					else									//	Else use specified fade (if poss)
						trimfade = min(transposcnt - dozeros,trimfade);
				} else if(trimfade > 0)						//	If trim can't be done, but fade is specified
						trimfade = min(transposcnt,trimfade); //	Set up fade

			} else if(trimfade > 0)							//	If trim is not specified, but fade is set
				trimfade = min(transposcnt,trimfade);		//	Check fade length

			fade_to  = transposcnt - dozeros;				//	Number of non-zero samples to write
			write_to = fade_to - trimfade;					//	Number of original level samples to write
			for(n = 0; n < write_to;n++) {					//	Write all samples before any element_fade
				obuf[obufpos] = (float)(obuf[obufpos] + (getbuf[n] * gain));
				obufpos++;
			}
			if(trimfade) {									//	Where necessary, do fade of element
				spliceval = 1.0;
				spliceincr = 1.0/(double)trimfade;
				for(; n < fade_to;n++) {
					spliceval = max(0.0,spliceval - spliceincr);
					splicer = pow(spliceval,dz->param[ITF_SLOP]); 
					obuf[obufpos] = (float)(obuf[obufpos] + (getbuf[n] * gain * splicer));
					obufpos++;
				}
			}
			this_last_write = (bufs_written * dz->buflen) + obufpos;
			if(this_last_write > last_write)
				last_write = this_last_write;

			absobufpos += delays[k];						//	Absolute position of next write in output
			bufs_before = absobufpos/dz->buflen;			//	Number of full buffers preceding this
			while(bufs_written < bufs_before) {				//	If not yet written this many bufs
				if(passno == 2) {							//	output the missing bufs
					if(ODD(dz->mode)) {						//	Where line is in pitched portions, adjust for envelope of these pitched-portions
						for(m = 0; m < dz->buflen;m++) {
							outtime = (double)((bufs_written * dz->buflen) + m)/srate;
							if((exit_status = read_lineportion_envelope(outtime,&envlasttime,&linedur,&linefade,&linefadeincr,&upfade,&upfadeincr,
								&lineenv,linefades,upfades,linegaps,minfade,pshifts,dz))<0)
								return exit_status;
							lineoutgain = portion_env(&linedur,&linefade,&lineenv,linefadeincr,&upfade,upfadeincr);
							maxoutsamp = max(maxoutsamp,fabs(obuf[m] * lineoutgain));
						}

					} else {
						for(m = 0; m < dz->buflen;m++)
							maxoutsamp = max(maxoutsamp,fabs(obuf[m]));
					}
				} else {
					if(ODD(dz->mode)) {						//	Where line is in pitched portions, adjust for envelope of these pitched-portions
						for(m = 0; m < dz->buflen;m++) {
							outtime = (double)((bufs_written * dz->buflen) + m)/srate;
							if((exit_status = read_lineportion_envelope(outtime,&envlasttime,&linedur,&linefade,&linefadeincr,&upfade,&upfadeincr,
								&lineenv,linefades,upfades,linegaps,minfade,pshifts,dz))<0)
								return exit_status;
							lineoutgain = portion_env(&linedur,&linefade,&lineenv,linefadeincr,&upfade,upfadeincr);
							obuf[m] = (float)(obuf[m] * lineoutgain);
						}
					}
					if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
						return(exit_status);
				}
				memcpy((char *)obuf,(char *)ovflw,dz->buflen * sizeof(float));
				memset((char *)ovflw,0,dz->buflen * sizeof(float));
				bufs_written++;
			}
			obufpos = absobufpos % dz->buflen;				//	Find position in output buffer
			time += (double)delays[k]/srate;				//	update time for next table-read
		}
		local_last_write = last_write - (bufs_written * dz->buflen);
		if(local_last_write > 0) {							//	Flush output
			if(passno == 2) {
				if(ODD(dz->mode)) {					//	Where line is in pitched portions, adjust for envelope of these pitched-portions
					for(m = 0; m < local_last_write;m++) {
						outtime = (double)((bufs_written * dz->buflen) + m)/srate;
						if((exit_status = read_lineportion_envelope(outtime,&envlasttime,&linedur,&linefade,&linefadeincr,&upfade,&upfadeincr,
							&lineenv,linefades,upfades,linegaps,minfade,pshifts,dz))<0)
							return exit_status;
						lineoutgain = portion_env(&linedur,&linefade,&lineenv,linefadeincr,&upfade,upfadeincr);
						maxoutsamp = max(maxoutsamp,fabs(obuf[m] * lineoutgain));
					}
				} else {
					for(m = 0; m < local_last_write;m++)
						maxoutsamp = max(maxoutsamp,fabs(obuf[m]));
				}
			} else {
				if(ODD(dz->mode)) {						//	Where line is in pitched portions, adjust for envelope of these pitched-portions
					for(m = 0; m < local_last_write;m++) {
						outtime = (double)((bufs_written * dz->buflen) + m)/srate;
						if((exit_status = read_lineportion_envelope(outtime,&envlasttime,&linedur,&linefade,&linefadeincr,&upfade,&upfadeincr,
							&lineenv,linefades,upfades,linegaps,minfade,pshifts,dz))<0)
							return exit_status;
						lineoutgain = portion_env(&linedur,&linefade,&lineenv,linefadeincr,&upfade,upfadeincr);
						obuf[m] = (float)(obuf[m] * lineoutgain);
					}
				}
				if((exit_status = write_samps(obuf,local_last_write,dz))<0)
					return(exit_status);
			}
		}
		if(passno == 2) {									//	If overload, set normaliser < 1.0
			if(maxoutsamp > ACCEPTABLE_MAXLEVEL)
				normaliser = ACCEPTABLE_MAXLEVEL/maxoutsamp;
		}
	}
	return FINISHED;
}

/**************************** READ_STEPD_DELAY_VALUE *****************************/

int read_stepd_delay_value(double thistime,double *notestarttime,double *noteendtime,double *portint,dataptr dz)
{
	int exit_status;
    double *p, *del = dz->brk[ITF_DEL], *delend = dz->brk[ITF_DEL] + (dz->brksize[ITF_DEL] * 2);
	double currentnotestart = 0.0, val, thisport;
	int lastval = (dz->brksize[ITF_DEL] * 2) - 1;
	p = del;
	while(thistime >= *p) {
		p += 2;
		if(p >= delend) {
			dz->param[ITF_DEL] = del[lastval];
			currentnotestart = *(delend-2);
			*noteendtime = dz->param[ITF_DUR];
			break;
		}
	}
	if(p < delend) {
		dz->param[ITF_DEL] = *(p-1);
		currentnotestart = *(p-2); 
		*noteendtime = *p;
	}
	if(currentnotestart > *notestarttime) { //	IF in new note
		if(dz->iparam[ITF_PORT] == 0)										
			*portint = 0.0;
		else {								//	IF portamento is set
			if(dz->brksize[ITF_PINT]) {		//	Get portamento interval
				if((exit_status = read_value_from_brktable(currentnotestart,ITF_PINT,dz))<0)
					return exit_status;
			}
			thisport = dz->param[ITF_PINT];
			switch(dz->iparam[ITF_PORT]) {	//	Portamento up, down or randomly up/dn
			case(1): 	
				break;
			case(-1):
				thisport = -thisport;
				break;
			default:
				val = drand48();
				if(val < 0.5)
					thisport = -thisport;
				break;
			}
			*portint = thisport;			//	Set (end of) portamento interval
		}
	}
	*notestarttime = currentnotestart;		//	(Re)set start of current note
	return FINISHED;
}

/********************** TRANSPOS_READ_ELEMENT ********************************/

int transpos_read_element(int *transposcnt,double incr,dataptr dz)
{
	float *ibuf = dz->sampbuf[0], *tbuf = dz->sampbuf[3];
	int k, lo, hi;
	double dpos, loval, hival, diff, frac;
	dpos = 0.0;
	k = 0;
	while(dpos < (double)dz->insams[0]) {
		lo = (int)floor(dpos);
		hi = (int)ceil(dpos);
		frac = dpos - (double)lo;
		loval = ibuf[lo];
		hival = ibuf[hi];
		diff = hival - loval;
		tbuf[k++] = (float)(loval + (diff * frac));
		if(k > dz->overflowsize) {
			sprintf(errstr,"TRANSPOSITION BUFFER OVERFLOW\n");
			return PROGRAM_ERROR;
		}
		dpos += incr;
	}
	*transposcnt = k;
	return FINISHED;
}

/********************** DOVIBRATO ********************************/

int dovibrato(int *delays,dataptr dz)
{
	int cyclestart, n, exit_status;
	int *vdelays;
	double *sintab;
	double vfrq, vdep, time, sintabincr, sintabpos, loval, hival, frac, diff, vval, srate = (double)dz->infile->srate;
	double time_advance;
	int k, samptime, lo, hi;

	if((vdelays = (int *)malloc(sizeof(int) * dz->itemcnt))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY establishing vibrato delays store.\n");
		return(MEMORY_ERROR);
	}
	if((sintab = (double *)malloc(sizeof(double) * (TS_SINTABSIZE + 1)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY establishing sintable array for vibrato.\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n < TS_SINTABSIZE;n++)									//	Vibrato sintable
		sintab[n] = sin((double)n/(double)TS_SINTABSIZE) * TWOPI;
	sintab[n] = 0.0;												//	wraparound point

	if(dz->brksize[ITF_VMIN]) {										//	Initialise vibrato params
		if((exit_status = read_value_from_brktable(0,ITF_VMIN,dz))<0)
			return exit_status;
	}
	if(dz->brksize[ITF_VMAX]) {
		if((exit_status = read_value_from_brktable(0,ITF_VMAX,dz))<0)
			return exit_status;
	}
	vfrq = drand48() * (dz->param[ITF_VMAX] - dz->param[ITF_VMIN]);
	vfrq += dz->param[ITF_VMIN];
	time_advance = (double)delays[0]/srate;						//	Set increment in sintable
	sintabincr = time_advance * TS_SINTABSIZE * vfrq;
	if(dz->brksize[ITF_DMIN]) {
		if((exit_status = read_value_from_brktable(0,ITF_DMIN,dz))<0)
			return exit_status;
	}
	if(dz->brksize[ITF_DMAX]) {
		if((exit_status = read_value_from_brktable(0,ITF_DMAX,dz))<0)
			return exit_status;
	}
	vdep = drand48() * (dz->param[ITF_DMAX] - dz->param[ITF_DMIN]);
	vdep += dz->param[ITF_DMIN];
	sintabpos = 0.0;
	samptime = 0;
	time = 0.0;
	cyclestart = 0;
	for(k=0;k < dz->itemcnt;k++) {
		if(cyclestart) {					//	At the start of a vibrato sine-cycle
			time = (double)samptime/srate;	//	Reset all vibrato values
			if(dz->brksize[ITF_VMIN]) {		
				if((exit_status = read_value_from_brktable(time,ITF_VMIN,dz))<0)
					return exit_status;
			}
			if(dz->brksize[ITF_VMAX]) {
				if((exit_status = read_value_from_brktable(time,ITF_VMAX,dz))<0)
					return exit_status;
			}
			vfrq = drand48() * (dz->param[ITF_VMAX] - dz->param[ITF_VMIN]);
			vfrq += dz->param[ITF_VMIN];
			time_advance = (double)delays[k]/srate;						//	Set increment in sintable
			sintabincr = time_advance * TS_SINTABSIZE * vfrq;
			if(dz->brksize[ITF_DMIN]) {
				if((exit_status = read_value_from_brktable(time,ITF_DMIN,dz))<0)
					return exit_status;
			}
			if(dz->brksize[ITF_DMAX]) {
				if((exit_status = read_value_from_brktable(time,ITF_DMAX,dz))<0)
					return exit_status;
			}
			vdep = drand48() * (dz->param[ITF_DMAX] - dz->param[ITF_DMIN]);
			vdep += dz->param[ITF_DMIN];
			cyclestart = 0;
		}
		lo = (int)floor(sintabpos);	//	Read sin table, interpolating
		hi = (int)ceil(sintabpos);
		frac = sintabpos - (double)lo;
		loval = sintab[lo];
		hival = sintab[hi];
		diff = hival - loval;
		vval = loval + (diff * frac);
		vval *= vdep;								//	Multiply by semitone depth
		vval = pow(2.0,vval/SEMITONES_PER_OCTAVE);	//	Convert to frq ratio
		vval = 1.0/vval;							//	Convert to change in wavelen (> frq -> < wavelen)
		vdelays[k] = (int)round((double)delays[k] * vval);	//	Vibrato modifies wavelen
	
		sintabpos += sintabincr;					//	Advance in sintable
		if(sintabpos >= TS_SINTABSIZE) {			//	Once sine-cycle completed, set flag to reset vibrato params (above)
			cyclestart = 1;
			sintabpos -= (double)TS_SINTABSIZE;
		}
		samptime += delays[k];				//	Advance time
	}
	for(k=0;k < dz->itemcnt;k++)			//	Convert orig delays to vibratoed delays
		delays[k] = vdelays[k];
	free(vdelays);
	return FINISHED;
}

/**************************** SETUP_LINEPORTION_PARAMS *****************************/

int setup_lineportion_params(double *linegains,double *linefades, double *upfades, double *linegaps,dataptr dz)
{
	int exit_status;
    double *p, *del = dz->brk[ITF_DEL], *delend = dz->brk[ITF_DEL] + (dz->brksize[ITF_DEL] * 2);
	int line_portion_cnt = 0, line_portion_end = dz->brksize[ITF_DEL] + 1;
	double time, linegain, diff;
	p = del;
	while(p < delend) {
		time = *p;
		if(line_portion_cnt >= line_portion_end) {
			sprintf(errstr,"Array overrun setting line-portion parameter arrays.\n");
			return PROGRAM_ERROR;
		}
		if(dz->brksize[ITF_GMIN]) {
			if((exit_status = read_value_from_brktable(time,ITF_GMIN,dz))<0)
				return exit_status;
		}
		if(dz->brksize[ITF_GMAX]) {
			if((exit_status = read_value_from_brktable(time,ITF_GMAX,dz))<0)
				return exit_status;
		}
		if(dz->brksize[ITF_FADE]) {
			if((exit_status = read_value_from_brktable(time,ITF_FADE,dz))<0)
				return exit_status;
		}
		if(dz->brksize[ITF_UFAD]) {
			if((exit_status = read_value_from_brktable(time,ITF_UFAD,dz))<0)
				return exit_status;
		}
		if(dz->brksize[ITF_GAPP]) {
			if((exit_status = read_value_from_brktable(time,ITF_GAPP,dz))<0)
				return exit_status;
		}
		if(!flteq(dz->param[ITF_GMIN],dz->param[ITF_GMAX])) {
			diff = dz->param[ITF_GMAX] - dz->param[ITF_GMIN];
			linegain = (drand48() * diff) + dz->param[ITF_GMIN];
		} else
			linegain = dz->param[ITF_GMAX];
		linegains[line_portion_cnt] = linegain;
		linefades[line_portion_cnt] = dz->param[ITF_FADE];
		upfades[line_portion_cnt]   = dz->param[ITF_UFAD];
		linegaps[line_portion_cnt]  = dz->param[ITF_GAPP];
		line_portion_cnt++;
		p +=2;
	}
    return(FINISHED);
}

/**************************** READ_LINEPORTION_GAIN *****************************/

int read_lineportion_gain(double thistime,double *lasttime,double *linegain,double *linegains,dataptr dz)
{
    double *p, *del, *delend;
	double portion_time;
	int k = 0;
	if(dz->brksize[ITF_DEL]) {
		del = dz->brk[ITF_DEL];
		delend = dz->brk[ITF_DEL] + (dz->brksize[ITF_DEL] * 2);
		p = del;
		while(thistime >= *p) {				//	Step forward in portion-pitches brkfile until time in brkfile exceeds NOW (thistime)
			k++;
			p += 2;
			if(p >= delend)					//	If step off end of breakfile, quit
				break;
		}
		k--;
		p -= 2;								//	We must be in previous brkfile portion, so step back to starttime of previous portion
		portion_time = *p;	
		if(portion_time > *lasttime) {		//	If starttime of current portion is NOT same as starttime of previous portion
			*linegain = linegains[k];		//	Reset the portion params
			*lasttime = portion_time;		//	And set new starttime for current portion
		}
	} else
		*linegain = 1.0;
	return(FINISHED);
}

/**************************** READ_LINEPORTION_ENVELOPE *****************************/

int read_lineportion_envelope(double thistime,double *envlasttime,int *linedur,int *linefade,double *linefadeincr,int *upfade,double *upfadeincr, double *lineenv,
							double *linefades,double *upfades,double *linegaps, int minfade, double *pshifts,dataptr dz)
{
    double *p, *del, *delend;
	double portion_time, ratio, lineend, penultdur, nulineend, srate = (double)dz->infile->srate;
	int dur, fade, gap, up_fade, slopes;
	int k = 0;
	if(dz->brksize[ITF_DEL]) {
		del = dz->brk[ITF_DEL]; 
		delend = dz->brk[ITF_DEL] + (dz->brksize[ITF_DEL] * 2);
		p = del;
		while(thistime >= *p) {				//	Step forward in portion-pitches brkfile until time in brkfile exceeds NOW (thistime)
			k++;
			p += 2;
			if(p >= delend)					//	If step off end of breakfile, quit
				break;
		}
		k--;
		p -= 2;								//	We must be in previous brkfile portion, so step back to starttime of previous portion
		portion_time = *p;	
		if(portion_time > *envlasttime) {	//	If starttime of current portion is NOT same as starttime of previous portion
			p += 2;							//	Find duration of new line-portion
			if(p >= delend) {
				lineend = dz->param[ITF_DUR] + (dz->duration/pshifts[dz->itemcnt-1]);
				if(p-4 >= dz->brk[ITF_DEL]) {
					penultdur = *(p-2) - *(p-4);	//	Curtail last pitch to duration of prevoius line-pitch
					nulineend = *(p-2) + penultdur;
					lineend = min(lineend,nulineend);
				}
			}
			else
				lineend = *p;
			dur = (int)((double)(lineend - portion_time) * srate);
			gap = (int)((double)dur * linegaps[k]);	//	Silence between portions is proportion of duration
			dur -= gap;						//	Duration (at full level) reduced by length of gap
			up_fade = (int)(upfades[k] * srate);
			if(up_fade < minfade)			//	Upfade cannot be less than a (no-clicks) minimum
				up_fade = minfade;
			fade = (int)(linefades[k] * srate);		
			if(fade < minfade)				//	Fade cannot be less than a (no-clicks) minimum
				fade = minfade;
			if((slopes = up_fade + fade) >= dur) {
				ratio = (double)slopes/(double)dur;
				up_fade = (int)round(up_fade * ratio);
				fade = (int)round(fade * ratio);
				slopes = up_fade + fade;
				while(slopes >= dur) {
					if(up_fade >= fade)
						up_fade--;
					else
						fade--;
					slopes--;
				}
			}
			dur -= slopes;					//	Duration (at full level) reduced by length of fades
			*upfade = up_fade;
			*upfadeincr = 1.0/(double)up_fade;
			*linefadeincr = 1.0/fade;	
			*linedur = dur;					//	Set up line-counters
			*linefade = fade;
			*lineenv  = 1.0;				//	initialise line envelope to 1.0
			*envlasttime = portion_time;	//	And set new starttime for current portion
		}
	} else {
		*linedur = INT_MAX;
		*linefade = 0;
		*lineenv = 1.0;
		*upfade = 0;
	}
	return(FINISHED);
}

/**************************** PORTION_ENV *****************************/

double portion_env(int *linedur, int *linefade, double *lineenv, double linefadeincr,int *upfade,double upfadeincr)
{
	double gain;
	if(*upfade > 0) {
		gain = max(0.0,1.0 - (*upfade * upfadeincr));
		(*upfade)--;
	} else if(*linedur > 0) {	//	Still in non-fade, non-silence part of portion
		gain = 1.0;
		(*linedur)--;			//	No change to level
	} else {
		if(*linefade > 0) {		//	If in fade
			*lineenv = max(*lineenv - linefadeincr,0.0);	//	Adjust lineenv
			gain = *lineenv;	//	Multiply element gain by lineenv
			(*linefade)--;
		} else {
			gain = 0.0;			//	If fade has reached 0 , gain is zero
		}
	}
	return gain;
}
