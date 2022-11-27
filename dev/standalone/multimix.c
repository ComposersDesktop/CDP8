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

static double time6;

//CDP LIB REPLACEMENTS
static int setup_multimix_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_multimix_param_ranges_and_defaults(dataptr dz);
static int handle_other_infiles(char ***cmdline,int *cmdlinecnt,dataptr dz);
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
//static double dbtolevel(double val);
static int multimix(dataptr dz);
static int open_file_and_retrieve_props(int filecnt,char *filename,int *srate,int *maxchans,dataptr dz);
static int create_and_output_mixfile_line(int infilno,int max_namelen,double *running_total,dataptr dz);
static int filename_extension_check(char *filename);
static int check_the_param_validity_and_consistency(dataptr dz);

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
	int exit_status;
	dataptr dz = NULL;
	char **cmdline;
	int  cmdlinecnt;
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
		if((exit_status = get_the_process_no(argv[0],dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(exit_status);
		}
		dz->maxmode = 8;
		cmdline++;
		cmdlinecnt--;
		if((exit_status = get_the_mode_from_cmdline(cmdline[0],dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(exit_status);
		}
		cmdline++;
		cmdlinecnt--;
		// setup_particular_application =
		if((exit_status = setup_multimix_application(dz))<0) {
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
	if((exit_status = setup_multimix_param_ranges_and_defaults(dz))<0) {
		exit_status = print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	// open_first_infile		CDP LIB
	if((exit_status = open_first_infile(cmdline[0],dz))<0) {	
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);	
		return(FAILED);
	}
	if((exit_status = filename_extension_check(cmdline[0]))<0) {	
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);	
		return(FAILED);
	}
	if((exit_status = store_filename(cmdline[0],dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);		
		return(FAILED);
	}
	cmdlinecnt--;
	cmdline++;

	// handle_extra_infiles() =
	if((exit_status = handle_other_infiles(&cmdline,&cmdlinecnt,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);		
		return(FAILED);
	}
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

	if((exit_status = check_the_param_validity_and_consistency(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	is_launched = TRUE;
	//param_preprocess()						redundant
	//spec_process_file =
	if((exit_status = multimix(dz))<0) {
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

/************************* SETUP_MULTIMIX_APPLICATION *******************/

int setup_multimix_application(dataptr dz)
{
	int exit_status;
	aplptr ap;
	if((exit_status = establish_application(dz))<0)		// GLOBAL
		return(FAILED);
	ap = dz->application;
	// SEE parstruct FOR EXPLANATION of next 2 functions
	switch(dz->mode) {
	case(2):
	case(3):
		if((exit_status = set_param_data(ap,0   ,1,1,"d"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","",0,0,""))<0)
			return(FAILED);
		break;
	case(4):
		if((exit_status = set_param_data(ap,0   ,4,4,"dddd"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","",0,0,""))<0)
			return(FAILED);
		break;
	case(6):
		if((exit_status = set_param_data(ap,0   ,2,2,"ii"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"st",2,"id","",0,0,""))<0)
			return(FAILED);
		break;
	case(7):
		if((exit_status = set_param_data(ap,0   ,1,1,"i"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","",0,0,""))<0)
			return(FAILED);
		break;
	default:
		if((exit_status = set_param_data(ap,0   ,0,0,""))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","",0,0,""))<0)
			return(FAILED);
		break;
	}
	// set_legal_infile_structure -->
	dz->has_otherfile = FALSE;
	// assign_process_logic -->
	dz->input_data_type = MANY_SNDFILES;
	dz->process_type	= TO_TEXTFILE;	
	dz->outfiletype  	= TEXTFILE_OUT;
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
		} else if((dz->mode == 3 || dz->mode == 4) && infile_info->channels > 2)  {
			sprintf(errstr,"File %s has too many channels for this mode\n",cmdline[0]);
			return(DATA_ERROR);
		} else if((dz->mode == 5 || dz->mode == 6) && infile_info->channels != 1)  {
			sprintf(errstr,"File %s has too many channels for this mode\n",cmdline[0]);
			return(DATA_ERROR);
		} else if((exit_status = copy_parse_info_to_main_structure(infile_info,dz))<0) {
			sprintf(errstr,"Failed to copy file parsing information\n");
			return(PROGRAM_ERROR);
		}
		free(infile_info);
	}
	return(FINISHED);
}

/************************* SETUP_MULTIMIX_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_multimix_param_ranges_and_defaults(dataptr dz)
{
	int exit_status;
	aplptr ap = dz->application;
	ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
	// NB total_input_param_cnt is > 0 !!!
	if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
		return(FAILED);
	// get_param_ranges()
	switch(dz->mode) {
	case(2):
		ap->lo[0]	= 0.0;
		ap->hi[0]	= 10000.0;
		ap->default_val[0]	= 0.0;
		if(!sloom)
			put_default_vals_in_all_params(dz);
		break;
	case(3):
		ap->lo[0]	= 0.0;
		ap->hi[0]	= 256.0;
		ap->default_val[0]	= 1.0;
		if(!sloom)
			put_default_vals_in_all_params(dz);
		break;
	case(4):
		ap->lo[0]	= 0.0;
		ap->hi[0]	= 1.0;
		ap->default_val[0]	= 1.0;
		ap->lo[1]	= 0.0;
		ap->hi[1]	= 1.0;
		ap->default_val[1]	= 1.0;
		ap->lo[2]	= 0.0;
		ap->hi[2]	= 1.0;
		ap->default_val[1]	= 1.0;
		ap->lo[3]	= 0.0;
		ap->hi[3]	= 1.0;
		ap->default_val[3]	= 1.0;
		if(!sloom)
			put_default_vals_in_all_params(dz);
		break;
	case(6):
		ap->lo[0]	= 2;
		ap->hi[0]	= 16.0;
		ap->default_val[0]	= 8;
		ap->lo[1]	= 1;
		ap->hi[1]	= 16.0;
		ap->default_val[1]	= 1;
		ap->lo[2]	= -16;
		ap->hi[2]	= 16;
		ap->default_val[2]	= 1;
		ap->lo[3]	= 0.0;
		ap->hi[3]	= 3600.0;
		ap->default_val[3]	= 0.0;
		if(!sloom)
			put_default_vals_in_all_params(dz);
		break;
	case(7):
		ap->lo[0]	= 2;
		ap->hi[0]	= 16.0;
		ap->default_val[0]	= 8;
		if(!sloom)
			put_default_vals_in_all_params(dz);
		break;
	}
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
			if((exit_status = setup_multimix_application(dz))<0)
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
	usage2("create");
	return(USAGE_ONLY);
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	if(!strcmp(prog_identifier_from_cmdline,"create"))				dz->process = MULTIMIX;
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
	if(!strcmp(str,"create")) {
		fprintf(stderr,
	    "CONVERT LIST OF SNDFILES TO A MULTICHANNEL MIXFILE\n\n"
	    "USAGE:\n"
	    "multimix create 1-2 infile1 infile2 [infile3..] mixfile\n"
	    "multimix create 3 infile1 infile2 [infile3..] mixfile timestep\n"
	    "multimix create 4 infile1 infile2 [infile3..] mixfile balance\n"
	    "multimix create 5 infile1 infile2 [infile3..] mixfile stage wide rearwide rear\n"
	    "multimix create 6 infile1 infile2 [infile3..] mixfile\n"
	    "multimix create 7 infile1 infile2 [infile3..] mixfile ochans startch [-sskip -ttimestep]\n"
	    "multimix create 8 infile1 infile2 [infile3..] mixfile ochans\n"
		"\n"
		"ANY SOUNDFILES\n"
		"\n"
		"mode 1 - all files start at time zero.\n"
		"mode 2 - each file starts where previous file ends.\n"
		"mode 3 - each file starts 'timestep' after previous file-start.\n"
		"mode 8 - each file starts at zero, leftmost chan to outchan 1: specify outchan count.\n"
		"\n"
		"TIMESTEP    time in seconds between entry of each file in mix.\n"
		"\n"
		"MONO OR STEREO FILES ONLY\n"
		"\n"
		"mode 4 - Distributes stereo over narrow and wide stereo pairs.\n"
		"mode 5 - Distributes stereo over 8 loudspeakers (pair-at-front format).\n"
		"\n"
		"Mono is treated as two identical channels of stereo\n"
		"\n"
		"BALANCE     proportion of stereo signal assigned to wider stereo pair.\n"
		"\n"
		"STAGE       level of signal assigned to front loudspeaker pair.\n"
		"WIDE        level of signal assigned to front-wide loudspeaker pair.\n"
		"REARWIDE    level of signal assigned to rear-wide loudspeaker pair.\n"
		"REAR        level of signal assigned to rear loudspeaker pair.\n"
		"\n"
		"MONO FILES ONLY\n"
		"\n"
		"mode 6 - Distributes N mono files, in order, to N successive output channels.\n"
		"mode 7 - Distributes mono files, in order, to successive chans of outfile.\n"
		"         If more files than ochans, restart again where it began.\n"
		"\n"
		"OCHANS     Number of channels in output file.\n"
		"STARTCH    Output channel to which first input channel is assigned.\n"
		"SKIP       Files can be distributed to each channel in turn (skip 1).\n"
		"           to alternate channels (skip 2), etc\n"
		"           in reverse chan order (skip -1), etc : Defaults to 1.\n"
		"TIMESTEP   Timestep between successive entries (default 0).\n");
	} else
		fprintf(stdout,"Unknown option '%s'\n",str);
	return(USAGE_ONLY);
}

int usage3(char *str1,char *str2)
{
	fprintf(stderr,"Insufficient parameters on command line.\n");
	return(USAGE_ONLY);
}

/******************************** MULTIMIX ********************************/

int multimix(dataptr dz)
{
	int    exit_status;
	int   srate = 0;
	int	   this_namelen, max_namelen = 0, outchans = 8, orig_infilecnt = dz->infilecnt;
	int   n;
	double running_total = 0.0;
	int maxchans = dz->infile->channels;
	sndcloseEx(dz->ifd[0]);
	dz->ifd[0] = -1;
	if(dz->all_words==0) {
		sprintf(errstr,"No words in source file.\n");
		return(PROGRAM_ERROR);
	}
	dz->infilecnt = 1;		/* program now utilises only 1 file at a time : needs data storage for only 1 file */
	if((exit_status = establish_file_data_storage_for_mix((int)1,dz))<0)
		return(exit_status);
	for(n=0;n<dz->all_words;n++) {						/* for each sndfile name */
		if((this_namelen = strlen(dz->wordstor[n]))<=0) {
			sprintf(errstr,"filename error: line %d\n",dz->linecnt+1);
			return(PROGRAM_ERROR);
		}
		max_namelen = max(max_namelen,this_namelen);
	}
	dz->itemcnt = dz->all_words;
	if(dz->mode == 6)
		running_total = (double)(dz->iparam[1] - 1);	/* startchan in 0 to N-1 form */
	for(n=0;n<dz->itemcnt;n++) {						/* for each sndfile name */
		if((exit_status = open_file_and_retrieve_props(n,dz->wordstor[n],&srate,&maxchans,dz))<0)
			return(exit_status);
		if((exit_status = create_and_output_mixfile_line(n,max_namelen,&running_total,dz))<0)
			return(exit_status);
	}
	switch(dz->mode) {
	case(0):
	case(1):
	case(2):
		outchans = maxchans;
		break;
	case(5):
		outchans = orig_infilecnt;
		break;
	case(6):
		time6 = 0.0;
		/* fall thro */
	case(7):
		outchans = dz->iparam[0];
		break;
	}
	fprintf(dz->fp,"%d\n",outchans);			// Store ouput channel countr
	for(n=dz->itemcnt;n<dz->all_words;n++)		//	Get and output the stored output lines
		fprintf(dz->fp,"%s\n",dz->wordstor[n]);
	return(FINISHED);
}

/************************ HANDLE_OTHER_INFILES *********************/

int handle_other_infiles(char ***cmdline,int *cmdlinecnt,dataptr dz)
{
					/* OPEN ANY FURTHER INFILES, CHECK COMPATIBILITY, STORE DATA AND INFO */
	int  exit_status;
	int  n;
	char *filename;
	if(dz->infilecnt > 1) {
		for(n=1;n<dz->infilecnt;n++) {
			filename = (*cmdline)[0];
			if((exit_status = filename_extension_check(filename)) < 0)
				return(exit_status);
			if((exit_status = store_further_filename(n,filename,dz))<0)
				return(exit_status);
			(*cmdline)++;
			(*cmdlinecnt)--;
		}
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

/**************************** ESTABLISH_FILE_DATA_STORAGE_FOR_MIX ********************************/

int establish_file_data_storage_for_mix(int filecnt,dataptr dz)
{
	int n;
	if(dz->insams!=NULL)		
		free(dz->insams);	 	/* in TK insams[0] also used in parse accounting */
	if(dz->ifd!=NULL)
		free(dz->ifd);
	if((dz->insams = (int *)malloc(filecnt * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to allocate infile samplesize array.\n");
		return(MEMORY_ERROR);
	}
	if((dz->ifd = (int  *)malloc(filecnt * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to allocate infile pointers array.\n");		   	
		return(MEMORY_ERROR);
	}
	for(n=0;n<filecnt;n++)
		dz->ifd[n] = -1;
	return(FINISHED);
}

/************************* OPEN_FILE_AND_RETRIEVE_PROPS *******************************/

int open_file_and_retrieve_props(int filecnt,char *filename,int *srate,int *maxchans,dataptr dz)
{
	int exit_status;
	double maxamp, maxloc;
	int maxrep;
	int getmax = 0, getmaxinfo = 0;
	infileptr ifp;
	if((ifp = (infileptr)malloc(sizeof(struct filedata)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store data on file %s\n",filename);
		return(MEMORY_ERROR);
	}
	/* OK to use sndopenEx? */
	if((dz->ifd[0] = sndopenEx(filename,0,CDP_OPEN_RDONLY)) < 0) {
		sprintf(errstr,"Failed to open sndfile %s\n",filename);
		return(SYSTEM_ERROR);
	}
	/* this use revised version that recognizes floatsam sndfile */
	if((exit_status = readhead(ifp,dz->ifd[0],filename,&maxamp,&maxloc,&maxrep,getmax,getmaxinfo))<0)
		return(exit_status);
	copy_to_fileptr(ifp,dz->infile);
	if(dz->infile->filetype!=SNDFILE) {
		sprintf(errstr,"%s is not a soundfile\n",filename);
		return(PROGRAM_ERROR);
	}
	if(filecnt==0)
		*srate = dz->infile->srate;
	else if(dz->infile->srate != *srate) {
		sprintf(errstr,"incompatible srate: file %s\n",filename);
		return(DATA_ERROR);
	}
	if((dz->mode == 3 || dz->mode == 4) && dz->infile->channels > 2) {
		sprintf(errstr,"Too many channels in file %s\n",filename);
		return(DATA_ERROR);
	}
	if((dz->mode == 5 || dz->mode == 6) && dz->infile->channels != 1) {
		sprintf(errstr,"Too many channels in file %s\n",filename);
		return(DATA_ERROR);
	}
	*maxchans = max(*maxchans,dz->infile->channels);
	if((dz->insams[0] = sndsizeEx(dz->ifd[0]))<0) {	    			
		sprintf(errstr, "Can't read size of input file %s\n",filename);
		return(PROGRAM_ERROR);
	}
	if(dz->insams[0] <=0) {
		sprintf(errstr, "Zero size for input file %s\n",filename);
		return(DATA_ERROR);
	}			
	if(sndcloseEx(dz->ifd[0])<0) {
		sprintf(errstr, "Failed to close input file %s\n",filename);
		return(SYSTEM_ERROR);
	}
	dz->ifd[0] = -1;
	return(FINISHED);
}

/************************* CREATE_AND_OUTPUT_MIXFILE_LINE *******************************/

int create_and_output_mixfile_line(int infilno,int max_namelen,double *running_total,dataptr dz)
{
	int exit_status,space_count, n, thischan;
	double filetime = 0.0;
	double outerlevel = 0.0, innerlevel = 0.0, maxlevel, adjust;
	char line[4000], linemore[4000];
	if(dz->mode == 3) {
		outerlevel = dz->param[0];
		innerlevel	= 1.0;
		maxlevel = max(innerlevel,outerlevel);
		adjust = 1.0/maxlevel;
		outerlevel = min(1.0,outerlevel * adjust);
		innerlevel = min(1.0,innerlevel * adjust);
	}
	if((space_count = max_namelen - strlen(dz->wordstor[infilno]))<0) {
		sprintf(errstr,"Anomaly in space counting: create_and_output_mixfile_line()\n");
		return(PROGRAM_ERROR);
	}
	space_count++;
	sprintf(line,"%s",dz->wordstor[infilno]);
	for(n=0;n<space_count;n++)
		strcat(line," ");
	switch(dz->mode) {
		case(1):
			filetime = *running_total;
			*running_total += (double)(dz->insams[0]/dz->infile->channels)/(double)dz->infile->srate;
			break;
		case(2):		
			filetime = *running_total;
			*running_total += dz->param[0];
			break;
		case(6):		
			filetime = time6;
			time6 += dz->param[3];
			break;
		default:
			break;	// filetime = 0.0
	}
	sprintf(linemore,"%.4lf  %d",filetime,dz->infile->channels);
	strcat(line,linemore);
	switch(dz->mode) {
	case(3):
		switch(dz->infile->channels) {
		case(MONO):
			sprintf(linemore,"  1:1  %.4f",innerlevel);
			strcat(line,linemore);
			sprintf(linemore,"  1:8  %.4f",outerlevel);
			strcat(line,linemore);
			sprintf(linemore,"  1:2  %.4f",innerlevel);
			strcat(line,linemore);
			sprintf(linemore,"  1:3  %.4f",outerlevel);
			strcat(line,linemore);
			break;
		case(STEREO):
			sprintf(linemore,"  1:1  %.4f",innerlevel);
			strcat(line,linemore);
			sprintf(linemore,"  1:8  %.4f",outerlevel);
			strcat(line,linemore);
			sprintf(linemore,"  2:2  %.4f",innerlevel);
			strcat(line,linemore);
			sprintf(linemore,"  2:3  %.4f",outerlevel);
			strcat(line,linemore);
			break;
		}
		break;
	case(4):
		switch(dz->infile->channels) {
		case(MONO):
			sprintf(linemore,"  1:1  %.4f",dz->param[0]);
			strcat(line,linemore);
			sprintf(linemore,"  1:8  %.4f",dz->param[1]);
			strcat(line,linemore);
			sprintf(linemore,"  1:7  %.4f",dz->param[2]);
			strcat(line,linemore);
			sprintf(linemore,"  1:6  %.4f",dz->param[3]);
			strcat(line,linemore);
			sprintf(linemore,"  1:2  %.4f",dz->param[0]);
			strcat(line,linemore);
			sprintf(linemore,"  1:3  %.4f",dz->param[1]);
			strcat(line,linemore);
			sprintf(linemore,"  1:4  %.4f",dz->param[2]);
			strcat(line,linemore);
			sprintf(linemore,"  1:5  %.4f",dz->param[3]);
			strcat(line,linemore);
			break;
		case(STEREO):
			sprintf(linemore,"  1:1  %.4f",dz->param[0]);
			strcat(line,linemore);
			sprintf(linemore,"  1:8  %.4f",dz->param[1]);
			strcat(line,linemore);
			sprintf(linemore,"  1:7  %.4f",dz->param[2]);
			strcat(line,linemore);
			sprintf(linemore,"  1:6  %.4f",dz->param[3]);
			strcat(line,linemore);
			sprintf(linemore,"  2:2  %.4f",dz->param[0]);
			strcat(line,linemore);
			sprintf(linemore,"  2:3  %.4f",dz->param[1]);
			strcat(line,linemore);
			sprintf(linemore,"  2:4  %.4f",dz->param[2]);
			strcat(line,linemore);
			sprintf(linemore,"  2:5  %.4f",dz->param[3]);
			strcat(line,linemore);
			break;
		}
		break;
	case(5):
		sprintf(linemore,"  1:%d  1.0",infilno+1);					//	Send input file to next output chan
		strcat(line,linemore);
		break;
	case(6):
		n = (int)round(*running_total);
		sprintf(linemore,"  1:%d  1.0",n+1);	//	Send input file to next current chan
		strcat(line,linemore);				
		n += dz->iparam[2];						//	Advance output chan by 'skip'
		while(n < 0)
			n += dz->iparam[0];					//	Wrap round total output chans
		while(n >= dz->iparam[0])
			n -= dz->iparam[0];					//	Wrap round total output chans
		*running_total = (double)n;
		break;
	case(7):
		if(dz->infile->channels > dz->iparam[0]) {
			sprintf(errstr,"Input file with %d channels exceeds output channel cnt (%d)\n",dz->infile->channels,dz->iparam[0]);
			return(DATA_ERROR);
		}
		/* fall thro */
	default:
		for(thischan = 1;thischan <= dz->infile->channels; thischan++) {
			sprintf(linemore,"  %d:%d  1.0",thischan,thischan);					//	Send each input chan to SAME output chan
			strcat(line,linemore);
		}
		break;
	}
	//	Used end-of filename-storage to store completed lines
	if((exit_status = store_further_filename(infilno+dz->itemcnt,line,dz))<0)
		return(exit_status);
	return(FINISHED);
}

/************************* FILENAME_EXTENSION_CHECK *******************************/

int filename_extension_check(char *filename) 
{
	char *p;
	p = filename;
	while(*p != ENDOFSTR) {
		if(*p == '.') {
			return(FINISHED);
		}
		p++;
	}
	sprintf(errstr,"Full filename, with extension, required.\n");
	return(DATA_ERROR);
}

/************************* CHECK_THE_PARAM_VALIDITY_AND_CONSISTENCY *******************************/

int check_the_param_validity_and_consistency(dataptr dz)
{
	if(dz->mode == 6) {
		if(dz->iparam[1] > dz->iparam[0]) {
			sprintf(errstr,"Start Channel is greater than number of Output Channels specified.\n");
			return(DATA_ERROR);
		}
	}
	return FINISHED;
}
