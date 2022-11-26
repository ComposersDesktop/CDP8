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



/*
 * Need to separate lparrays into INTEGER and UNSIGNED LONG
 * THEN try compiling
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

#define SIGNAL_TO_LEFT  (0)
#define SIGNAL_TO_RIGHT (1)
#define ROOT2		(1.4142136)
#define dupl descriptor_samps

#ifdef unix
#define round(x) lround((x))
#endif

char errstr[2400];

int anal_infiles = 1;
int	sloom = 0;
int sloombatch = 0;

const char* cdp_version = "7.1.0";

//CDP LIB REPLACEMENTS
static int check_madrid_param_validity_and_consistency(dataptr dz);
static int setup_madrid_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int handle_the_special_data(char *str,dataptr dz);
static int setup_madrid_param_ranges_and_defaults(dataptr dz);
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
static int madrid(dataptr dz);
static int madrid_param_preprocess(dataptr dz);
static int create_madrid_sndbufs(dataptr dz);
static void pancalc(double position,double *leftgain,double *rightgain);
static void rndintperm(int *perm,int cnt);

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
	int exit_status;
	dataptr dz = NULL;
	char **cmdline, sfnam[400];
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
		if((exit_status = setup_madrid_application(dz))<0) {
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
	if((exit_status = setup_madrid_param_ranges_and_defaults(dz))<0) {
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
	if(dz->infilecnt > 1) {
		if((exit_status = handle_extra_infiles(&cmdline,&cmdlinecnt,dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);		
			return(FAILED);
		}
	}
	// handle_outfile() = 
	if((exit_status = handle_the_outfile(&cmdlinecnt,&cmdline,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}

//	handle_formants()			redundant
//	handle_formant_quiksearch()	redundant
 	if(dz->mode == 1) {
		strcpy(sfnam,cmdline[0]);
		cmdlinecnt--;
		cmdline++;
	}
	if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {		// CDP LIB
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = open_the_outfile(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = check_madrid_param_validity_and_consistency(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	is_launched = TRUE;
	if((exit_status = create_madrid_sndbufs(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = madrid_param_preprocess(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if(dz->mode == 1) {
		if((exit_status = handle_the_special_data(sfnam,dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
	}
	//spec_process_file =
	if((exit_status = madrid(dz))<0) {
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
	p = filename;					//	Drop file extension
	while(*p != ENDOFSTR) {
		if(*p == '.') {
			*p = ENDOFSTR;
			break;
		}
		p++;
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
	dz->infile->channels = dz->iparam[MAD_CHANS];
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

/************************* SETUP_MADRID_APPLICATION *******************/

int setup_madrid_application(dataptr dz)
{
	int exit_status;
	aplptr ap;
	if((exit_status = establish_application(dz))<0)		// GLOBAL
		return(FAILED);
	ap = dz->application;
	// SEE parstruct FOR EXPLANATION of next 2 functions
	if(dz->mode == 0) {
		if((exit_status = set_param_data(ap,0   ,6,6,"diiDDD"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"s",1,"i","elrR",4,0,"0000"))<0)
			return(FAILED);
	} else {
		if((exit_status = set_param_data(ap,MAD_SEQUENCE,6,6,"diiDDD"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"s",1,"i","el",2,0,"00"))<0)
			return(FAILED);
	}
	// set_legal_infile_structure -->
	dz->has_otherfile = FALSE;
	// assign_process_logic -->
	if(dz->mode == 0)
		dz->input_data_type = ONE_OR_MANY_SNDFILES;
	else
		dz->input_data_type = MANY_SNDFILES;
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

/************************* SETUP_MADRID_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_madrid_param_ranges_and_defaults(dataptr dz)
{
	int exit_status;
	aplptr ap = dz->application;
	// set_param_ranges()
	ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
	// NB total_input_param_cnt is > 0 !!!
	if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
		return(FAILED);
	// get_param_ranges()
	ap->lo[MAD_DUR]	= dz->duration;
	ap->hi[MAD_DUR]	= 32767;
	ap->default_val[MAD_DUR]	= 20;
	ap->lo[MAD_CHANS]	= 2;
	ap->hi[MAD_CHANS]	= 16;
	ap->default_val[MAD_CHANS] = 8;
	ap->lo[MAD_STRMS]	= 2;
	ap->hi[MAD_STRMS]	= 64;
	ap->default_val[MAD_STRMS] = 3;
	ap->lo[MAD_DELF]	= 0;
	ap->hi[MAD_DELF]	= 1000;
	ap->default_val[MAD_DELF] = 0.5;
	ap->lo[MAD_STEP]	= 0;
	ap->hi[MAD_STEP]	= 60;
	ap->default_val[MAD_STEP] = .2;
	ap->lo[MAD_RAND]	= 0;
	ap->hi[MAD_RAND]	= 1;
	ap->default_val[MAD_RAND] = 0;
	ap->lo[MAD_SEED]	= 0;
	ap->hi[MAD_SEED]	= 256;
	ap->default_val[MAD_SEED] = 256;
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
			if((exit_status = setup_madrid_application(dz))<0)
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
	usage2("madrid");
	return(USAGE_ONLY);
}

/**************************** MADRID_PARAM_PREPROCESS ****************************/

int madrid_param_preprocess(dataptr dz)
{

	int exit_status, chans = dz->iparam[MAD_CHANS];
	double srate = (double)dz->infile->srate, maxrand, minstep, maxsteps, leftgain, rightgain;
	int imaxsteps, iminstep, maxoverlay, maxinsize, n, m;
	int *lmost, *rmost;
	double *pos, *llev, *rlev;

	dz->iparam[MAD_DUR] = (int)round(dz->param[MAD_DUR] * srate);

	//	Calculate minimum possible step between events
	
	if(dz->brksize[MAD_STEP]) {
		if((exit_status = get_minvalue_in_brktable(&minstep,MAD_STEP,dz))<0)
			return PROGRAM_ERROR;
	} else
		minstep = dz->param[MAD_STEP];
	if(dz->brksize[MAD_RAND]) {
		if((exit_status = get_maxvalue_in_brktable(&maxrand,MAD_RAND,dz))<0)
			return PROGRAM_ERROR;
	} else
		maxrand = dz->param[MAD_RAND];
	minstep = minstep - (minstep/2.0 * maxrand);
	iminstep = (int)floor(dz->param[MAD_STEP] * srate);
	if(iminstep > 4)
		iminstep -= 4;	//	SAFETY

	//	Calculate maximum possible overlay of streams
	
	maxinsize = dz->insams[0];
	for(n=1;n < dz->infilecnt;n++) 
		maxinsize = max(maxinsize,dz->insams[n]);
	maxoverlay = maxinsize/iminstep;
	if(maxoverlay * iminstep != maxinsize)
		maxoverlay++;
	dz->dupl = maxoverlay;

	//	Calculate maximum possible timesteps
	
	maxsteps  = dz->param[MAD_DUR]/minstep;
	imaxsteps = (int)ceil(maxsteps * srate);
	imaxsteps += 4;	// SAFETY
	dz->itemcnt = imaxsteps;

/******************************************************************************
 * INTEGER ARRAYS								  LONG ARRAYS
 *	  points to									 	  points to
 *			 lmost | | | |				MODE1		 			 ptrfile
 *			 | rmost | | |				sequence 			 | itimes
 *			 | | ptrfile |				| |		 	   array | | |
 *			 | | | strperm				| |		 		no:	 0 1 |
 *			 | | | | fileperm			| |		 		  	 | | |
 *			 | | | | | fileon			| |		     size   strmcnt*dupl
 *	   array | | | | |   onoff-flags..	| |		 		 	 | maxsteps
 *		no:	 0 1 2 3 4 5 6  1-for-each	6+strmcnt 			 | | |
 *			 | | | | | | |	stream		| |
 *		  _	 | | | | | | |--------------| |
 *		 |   strmcnt | | |				| |
 *		 |	 | strmcnt | |  maxsteps	maxsteps
 *	   size	 | | strmcnt*dupl			| |
 *		 |	 | | | strmcnt 				| |
 *		 |   | | | | infilecnt			| |
 *		 |_  | | | | | maxsteps			| |
 *
 *	dz->iparray[0]  = leftmost channel for output for each stream
 *	dz->iparray[1]  = rightmost channel for output for each stream
 *	dz->iparray[2]  = insound associated with ptr
 *	dz->iparray[3]  = permutations of streams, to select those to be on/off.
 *	dz->iparray[4]  = permutations of infile order - if ness.
 *	dz->iparray[5]  = infile in use at each step.
 *	dz->iparray + 6 points to MAD_STRMS arrays of onoff flags
 *
 *	dz->lparray[0]  = pointers associated with streams (if repeats overlap one another, need duplicate pointers)
 *	dz->lparray[1]  = times of events, in samples.
 */	
	if(dz->mode == 1) {
		if((dz->iparray = (int **)malloc((7 + dz->iparam[MAD_STRMS]) * sizeof(int *)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to create integer arrays. Reduce stream cnt or duration, or increase step.\n");
			return(MEMORY_ERROR);
		} 
	} else {
		if((dz->iparray = (int **)malloc((6 + dz->iparam[MAD_STRMS]) * sizeof(int *)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to create integer arrays. Reduce stream cnt or duration, or increase step.\n");
			return(MEMORY_ERROR);
		} 
	}
	for(n = 0; n < 2;n++) {
		if((dz->iparray[n] = (int *)malloc(dz->iparam[MAD_STRMS] * sizeof(int)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to create integer array %d. Reduce stream cnt or duration, or increase step.\n",n+1);
			return(MEMORY_ERROR);
		}
	}
	if((dz->iparray[2] = (int *)malloc((dz->dupl * dz->iparam[MAD_STRMS]) * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create integer array 3. Reduce stream cnt or duration, or increase step.\n");
		return(MEMORY_ERROR);
	}
	if((dz->iparray[3] = (int *)malloc((dz->iparam[MAD_STRMS] + 1) * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create integer array 4. Reduce stream cnt or duration, or increase step.\n");
		return(MEMORY_ERROR);
	}
	if((dz->iparray[4] = (int *)malloc((dz->infilecnt + 1) * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create integer array 5. Reduce stream cnt or duration, or increase step.\n");
		return(MEMORY_ERROR);
	}
	if((dz->iparray[5] = (int *)malloc(imaxsteps * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create integer array 6. Reduce stream cnt or duration, or increase step.\n");
		return(MEMORY_ERROR);
	}
	for(n = 0, m = 6; n < dz->iparam[MAD_STRMS];n++,m++) {
		if((dz->iparray[m] = (int *)malloc(imaxsteps * sizeof(int)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to create integer array %d.\nReduce stream count to <= %d\n",m+1,m-7);
			return(MEMORY_ERROR);
		}
	}
	if(dz->mode == 1)  {
		if((dz->iparray[m] = (int *)malloc(imaxsteps * sizeof(int)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to create integer array %d. Reduce stream cnt or duration, or increase step.\n",m+1);
			return(MEMORY_ERROR);
		}
	}
	if((dz->lparray = (int **)malloc(2 * sizeof(int *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create longs arrays. Reduce stream cnt or duration, or increase step.\n");
		return(MEMORY_ERROR);
	}
	if((dz->lparray[0] = (int *)malloc((dz->iparam[MAD_STRMS] * dz->dupl) * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create longs array 1. Reduce stream cnt or duration, or increase step.\n");
		return(MEMORY_ERROR);
	}
	if((dz->lparray[1] = (int *)malloc(imaxsteps * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create longs array 2. Reduce stream cnt or duration, or increase step.\n");
		return(MEMORY_ERROR);
	}

	lmost = dz->iparray[0];
	rmost = dz->iparray[1];

/*********************************************
 *
 *	dz->parray[0] = level of leftmost channel for output of each stream
 *	dz->parray[1] = level of rightmost channel for output of each stream
 */
		
	if((dz->parray = (double **)malloc(2 * sizeof(double *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create doubles arrays.\n");
		return(MEMORY_ERROR);
	}
	for(n = 0; n < 2;n++) {
		if((dz->parray[n] = (double *)malloc(dz->iparam[MAD_STRMS] * sizeof(double)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to create doubles array %d.\n",n+1);
			return(MEMORY_ERROR);
		}
	}
	pos  = dz->parray[0];	//	Used initially for position, then overwritten by left level value
	llev = dz->parray[0];
	rlev = dz->parray[1];

	//	Calculate spatial positions of streams, and associated LR-levels

	if(dz->iparam[MAD_STRMS] == chans) {
		for(n = 0;n<chans;n++) {
			lmost[n] = n;
			rmost[n] = (lmost[n] + 1) % chans;
			llev[n] = 1.0;
			rlev[n] = 0.0;
		}
	} else {
		if (chans == 1) {
			for(n = 0;n<dz->iparam[MAD_STRMS];n++) {
				lmost[n] = 0;
				rmost[n] = 0;
				llev[n] = 1.0;		//	Daft, but keeps same algo for all cases
				rlev[n] = 0.0;
			}
		} else {
			if(dz->vflag[MAD_LINEAR] || chans == 2) {
				for(n = 0;n<dz->iparam[MAD_STRMS];n++) {
					pos[n] = ((chans - 1) * n)/(double)(dz->iparam[MAD_STRMS] - 1);
					lmost[n] = (int)floor(pos[n]);
					pos[n]  -= lmost[n]; 
				}
			} else {
				for(n = 0;n<dz->iparam[MAD_STRMS];n++) {
					pos[n] = (chans * n)/(double)dz->iparam[MAD_STRMS];
					lmost[n] = (int)floor(pos[n]);
					pos[n]  -= lmost[n]; 
				}
			}
		}
		for(n = 0;n<dz->iparam[MAD_STRMS];n++) {
			rmost[n] = (lmost[n] + 1) % chans;
			if(flteq(pos[n],0.0)) {
				rlev[n] = 0.0;
				llev[n] = 1.0;	//	pos values overwritten by associated level values (ETC below)
			} else if(flteq(pos[n],1.0)) {
				rlev[n] = 1.0;
				llev[n] = 0.0;
			} else {
				pos[n] *= 2.0;
				pos[n] -= 1.0;	//	Change position to -1 to +1 range
				pancalc(pos[n],&leftgain,&rightgain);
				rlev[n] = rightgain;
				llev[n] = leftgain;
			}
		}
	}
	return FINISHED;
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	if(!strcmp(prog_identifier_from_cmdline,"madrid"))				dz->process = MADRID;
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
	if(!strcmp(str,"madrid")) {
		fprintf(stdout,
	    "USAGE:\n"
	    "madrid madrid 1 inf [inf2 ...] outf dur ochans strmcnt delfact step rand\n"
		"              [-sseed] [-l] [-e] [-r|-R]\n"
	    "madrid madrid 2 inf inf2 [inf3...] seqfile outf dur ochans strmcnt delfact step rand\n"
		"              [-sseed] [-l] [-e]\n"
		"\n"
		"Spatially syncopate repetitions of src(s) \n"
		"by randomly deleting items from the spatially-separated repetition streams.\n"
		"\n"
		"For each output event, source played at 1 or more spatial locations, SYNCHRONOUSLY.\n"
		"Depending on which/how many locations are used for an output event,\n"
		"the output appears differently located in space, and differently accented.\n"
		"\"DELFACT\" and \"-e\" are hence the crucial rhythmicising parameter here.\n"
		"\n"
		"SEQFILE   textfile containing list of numbers in range 1 to count-of-infiles.\n"
		"          Determines the order in which infiles are used in the output.\n"
		"\n"
		"DUR       Duration of output sound.\n"
		"OCHANS    Number of channels in output sound.\n"
		"STRMCNT   Number of spatially distinct streams.\n"
		"DELFACT   Proportion of items to (randomly) delete.\n"
		"          Vals from 0 to 1 delete that proportion of events in the various streams.\n"
		"          For vals > 1, proportion of events at single locations increases,\n"
		"          and, if the \"-e\" flag is set, the proportion of empty events increases.\n"		
		"STEP      Time between event repetitions.\n"
		"RAND      Randomisation of step size (0-1).\n"
		"SEED      Value to initialise the randomisation OF DELETIONS.\n"
		"          With a non-zero value, re-running the process with the SAME parameters\n"
		"          will produce the same output. Otherwise deletions are always different.\n"
		"-e        allow Empty events i.e. sound absent at some of the repeat-steps.\n"
		"-l        For OCHANS > 2, lspkr array assumed to be circular.\n"
		"          -l flag forces array to be linear (with defined left and right ends).\n"
		"-r        Randomly permute the order of input sounds used in the output.\n"
		"          (ALL input sounds used ONCE before next order-permutation is generated).\n"
		"-R        Randomly select next input sound (selection unrelated to last selection).\n"
		"\n"
		"-r and -R cannot be used in combination.\n"
		"When only 1 input sound is used, neither flag has any effect.\n"
		"\n"
		"DELFACT, STEP and RAND can vary over time.\n"
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

/******************************** MADRID ********************************
 *
 * INTEGER ARRAYS								  LONG ARRAYS
 *	  points to									 	  points to
 *			 lmost | | | |				|		 			 ptrfile
 *			 | rmost | | |				|		 			 | itimes
 *			 | | ptrfile |				|		 	   array | | |
 *			 | | | strperm				|		 		no:	 0 1 |
 *			 | | | | fileperm			|		 		  	 | | |
 *			 | | | | | fileon			|		     size   strmcnt*dupl
 *	   array | | | | |   onoff-flags..	|		 		 	 | maxsteps
 *		no:	 0 1 2 3 4 5 6  1-for-each	|		 			 | | |
 *			 | | | | | | |	stream		|
 *		  _	 | | | | | | |--------------|
 *		 |   strmcnt | | |				|
 *		 |	 | strmcnt | |  maxsteps	|
 *	   size	 | | strmcnt*dupl			|
 *		 |	 | | | strmcnt 				|
 *		 |   | | | | infilecnt			|
 *		 |_  | | | | | maxsteps			|
 *
 *	dz->iparray[0]  = leftmost channel for output for each stream
 *	dz->iparray[1]  = rightmost channel for output for each stream
 *	dz->iparray[2]  = insound associated with ptr
 *	dz->iparray[3]  = permutations of streams, to select those to be on/off.
 *	dz->iparray[4]  = permutations of infile order - if ness.
 *	dz->iparray[5]  = infile in use.
 *	dz->iparray + 6 points to MAD_STRMS arrays of onoff flags
 *
 *	dz->lparray[0]  = pointers associated with streams
 *	dz->lparray[1]  = times of events, in samples.
 */	

#define ibuf dz->sampbuf
	
int madrid(dataptr dz)
{
	int exit_status, finished;
	double *llev = dz->parray[0], *rlev = dz->parray[1]; 
	int *lmost = dz->iparray[0], *rmost = dz->iparray[1], *inf = dz->iparray[2], *seq = NULL;
	int *strmperm = dz->iparray[3], *srcsperm = dz->iparray[4], *fileon = dz->iparray[5];
	int *ptr = dz->lparray[0], *itime = dz->lparray[1];
	int **onoff = dz->iparray + 6;
	float *obuf = dz->sampbuf[dz->infilecnt];
	double srate = (double)dz->infile->srate;
	int insnd = 0, ptrcnt, srcspermno, streams_off, lastinlastperm, xs, logxs = 0, chans = dz->iparam[MAD_CHANS];
	int n, nn, m, samps_read, totaloutsamps, outcnt, inexttime, stepcnt, opos, eventcnt = 0;
	double maxsamp = 0.0, time = 0.0, nexttime, endtime, normaliser;

	if(dz->iparam[MAD_SEED] > 0)
		srand((int)dz->iparam[MAD_SEED]);
	else
		initrand48();

	if(dz->mode == 1)
		seq = dz->iparray[6 + dz->iparam[MAD_STRMS]];

	ptrcnt = dz->dupl * dz->iparam[MAD_STRMS];
	for(n=0;n<ptrcnt;n++) {
		ptr[n] = -1;		//	Mark all buffer pointers as available
		inf[n] = 0;			//	SAFETY: Connect all ptrs with infile 0
	}
							//	Read all infiles, filling buffers
	for(n=0;n<dz->infilecnt;n++) {
		if((samps_read  = fgetfbufEx(dz->sampbuf[n], dz->insams[n],dz->ifd[n],0))<0) {
			sprintf(errstr,"Sound read error with input soundfile %d: %s\n",n+1,sferrstr());
			return(SYSTEM_ERROR);
		}
		srcsperm[n] = n;	//	Initialise permutation-of-insounds
	}
	totaloutsamps = dz->iparam[MAD_DUR];
	nexttime = 0.0;
	inexttime = 0;
	stepcnt = -1;
	outcnt = 0;
	opos = 0;
	srcspermno = 0;
	eventcnt = 0;
	fprintf(stdout,"INFO: First pass: assessing level.\n");
	fflush(stdout);
	memset((char *)obuf,0,dz->buflen * sizeof(float));
	while(outcnt < dz->iparam[MAD_DUR]) {
		if(outcnt >= inexttime) {
			itime[stepcnt] = inexttime;
			stepcnt++;
			eventcnt++;
			time = nexttime;
			if(time >= dz->param[MAD_DUR])	//	AAA: see note at foot of loop
				break;
			itime[stepcnt] = inexttime;
			if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
				return exit_status;

			//	GET SOUND FOR THIS EVENT : If only 1 infile, insnd defaults to 0

			if(dz->infilecnt > 1) {
				if(dz->mode == 1)
					insnd = seq[eventcnt];
				else {
					srcspermno %= dz->infilecnt;
					if(dz->vflag[MAD_INPERM]) {
						if(srcspermno == 0) {
							lastinlastperm = srcsperm[dz->infilecnt - 1];
							rndintperm(srcsperm,dz->infilecnt);	
							if(lastinlastperm == srcsperm[0]) {
								if(dz->infilecnt > 2) {			//	If no of srcs > 2
									nn = srcsperm[0];			//	Avoid first component of this perm
									srcsperm[0] = srcsperm[1];	//	being same as last component of previous perm
									srcsperm[1] = nn;			//	(with 2 files, this restraints would force a 01010101010 alternating pattern)
								}
							}
						}
						insnd = srcsperm[srcspermno];
					} else if(dz->vflag[MAD_INRAND])
						insnd = (int)floor(drand48() * dz->infilecnt);
					 else
						insnd = srcsperm[srcspermno];
					 srcspermno++;
				}
			}
			endtime = time + (double)(dz->insams[insnd])/srate;	//	Check this event will not overrun duration
			if(endtime >= dz->param[MAD_DUR])					//	and if it does, quit
				break;
			fileon[stepcnt] = insnd;
			
			//	SELECT STREAMS THAT ARE ON FOR THIS EVENT
			
			if(dz->param[MAD_DELF] <= 0) {					//	If no deletions
				streams_off = 0;
				for(n=0;n<dz->iparam[MAD_STRMS];n++)		//	Turn on all streams
					onoff[n][stepcnt] = 1;
			} else {										//	Select number of streams on, at random				
				rndintperm(strmperm,dz->iparam[MAD_STRMS]);	//	Randomly permute order of streams
															//	If the delfactor is > 1, bias the random selection
				xs = max(0,((int)round(dz->param[MAD_DELF]) - 1));
				if(xs)
					logxs = (int)(round(LOG2(xs)));
				if(dz->vflag[MAD_GAPS]) {
					streams_off = (int)floor(drand48() * (dz->iparam[MAD_STRMS]+1+(xs*2)));	//	Range 0 to streamscnt + xs
					if((xs = streams_off - dz->iparam[MAD_STRMS]) > 0) {
						if(xs > logxs)
							streams_off = dz->iparam[MAD_STRMS];		//	For delfactor > 1, add bias to NO streams on
						else
							streams_off = dz->iparam[MAD_STRMS] - 1;	//	and add (less) bias to only 1 stream on
					}
				} else {
					streams_off = (int)floor(drand48() * dz->iparam[MAD_STRMS]+xs);		//	Range 0 to streamscnt-1 + xs
					if(streams_off >= dz->iparam[MAD_STRMS])
						streams_off = dz->iparam[MAD_STRMS] - 1;						//	For delfactor > 1, add bias to only 1 stream on
				}
				for(n = 0;n < streams_off;n++)				//	Mark first "streams_off" streams in strmperm as off
					onoff[strmperm[n]][stepcnt] = 0;
				for(;n < dz->iparam[MAD_STRMS];n++)			//	Mark remainder as on
					onoff[strmperm[n]][stepcnt] = 1;
			}
			if(streams_off == dz->iparam[MAD_STRMS]) {		//	If all streams are off, do not move to next insound yet
				srcspermno--;
				eventcnt--;
			}
			//	FIND AN INACTIVE POINTER ASSOCIATED WITH EACH ON-STREAM, AND SET IT TO START COUNTING IN THE SPECIFIED insnd BUFFER

			for(n = 0;n<dz->iparam[MAD_STRMS];n++) {
				if(onoff[n][stepcnt]) {						//	For every stream that is on
					for(m=n; m < ptrcnt; m+= dz->iparam[MAD_STRMS]) {	//	Find an available buffer-pointer assocd with that stream
						if(ptr[m] < 0 || ptr[m] >= dz->insams[inf[m]]) {
							ptr[m] = 0;						//	and set it to start counting in an infile
							inf[m] = insnd;					//	indicating which infile it is pointing to.
							break;							//	(as events may overlap within a stream, a ptr may still be playing tail of a DIFFERENT sound
						}									//	 when new event starts, so we need to remember which insound a pointer WAS reading from)
					}
				}
			}
		}

		//	WRITE TO NEXT SAMPLE IN OUTBUFFER FROM ALL ACTIVE POINTERS

		for(n = 0;n<ptrcnt;n++) {							//	For every pointer...
			if(ptr[n] >= 0 && ptr[n] < dz->insams[inf[n]]) {//	If it is active
				nn = n % dz->iparam[MAD_STRMS];				//	Find the associated stream
															//	Add contribution to outbuffer, in correct spatial position
				obuf[opos + lmost[nn]] = (float)(obuf[opos + lmost[nn]] + (ibuf[inf[n]][ptr[n]] * llev[nn]));
				obuf[opos + rmost[nn]] = (float)(obuf[opos + rmost[nn]] + (ibuf[inf[n]][ptr[n]] * rlev[nn]));
					//		position-nn										pointer[n]			levels[nn]
					//	assocd with output stream				   	  is associated with	are associated with
					//	and corresponds to chans					     sound inf[n]			spatial pos
					//	   in output buffer							   which it is reading	 of output stream
			}
			if(ptr[n] >= 0)									//	Advance all switched-on ptrs, whether are not they are currently reading a file.
				ptr[n]++;									//	Once they are beyond file end, they become available to select for next read.
		}

		//	ADVANCE IN OUTBUFFER, WRITING OUTPUT IF BUFFER IS FULL
		
		opos += chans;
		if(opos >= dz->buflen) {
			fprintf(stdout,"INFO: Level Check at %lf secs\n",(double)outcnt/srate);
			fflush(stdout);
			for(n = 0;n < dz->buflen;n++)
				maxsamp = max(maxsamp,fabs(obuf[n]));
			memset((char *)obuf,0,dz->buflen * sizeof(float));
			opos = 0;
		}
		
		//	IF WE'VE JUST STARTED A NEW EVENT, DETERMINE TIME OF NEXT EVENT

		if(outcnt >= inexttime) {
			if(dz->param[MAD_RAND] > 0) {
				nexttime = ((drand48() * 2.0) - 1.0)/2.0;	//	Range +- 1/2
				nexttime *= dz->param[MAD_RAND];			//	Max Range +- 1/2
				nexttime *= dz->param[MAD_STEP];			//	Max Range +- 1/2 STEP
				nexttime += dz->param[MAD_STEP];			//	Max Range 1/2STEP to 3/2STEP
				nexttime += time;
			} else 
				nexttime = time + dz->param[MAD_STEP];
			inexttime = (int)round(nexttime * srate);
		}

		//	ADVANCE THE OUTPUT GROUP-SAMPLE COUNTER

		outcnt++;
	}
	//	COMPLETE OUTPUT FROM ANY ADVANCING POINTERS

	finished = 0;
	while(!finished) {
		finished = 1;
		for(n = 0;n<ptrcnt;n++) {							//	For every pointer...
			if(ptr[n] > 0 && ptr[n] < dz->insams[inf[n]]) {	//	If it is active
				finished = 0;								//	Flag not yet finished
				break;
			}
		}
		if(finished)
			break;
		for(n = 0;n<ptrcnt;n++) {
			if(ptr[n] > 0 && ptr[n] < dz->insams[inf[n]]) {
				nn = n % dz->iparam[MAD_STRMS];
				obuf[opos + lmost[nn]] = (float)(obuf[opos + lmost[nn]] + (ibuf[inf[n]][ptr[n]] * llev[nn]));
				obuf[opos + rmost[nn]] = (float)(obuf[opos + rmost[nn]] + (ibuf[inf[n]][ptr[n]] * rlev[nn]));
			}
			if(ptr[n] > 0)
				ptr[n]++;
		}
		opos += chans;
		if(opos >= dz->buflen) {
			fprintf(stdout,"INFO: Level Check at %lf secs\n",(double)outcnt/srate);
			fflush(stdout);
			for(n = 0;n < dz->buflen;n++)
				maxsamp = max(maxsamp,fabs(obuf[n]));
			memset((char *)obuf,0,dz->buflen * sizeof(float));
			opos = 0;
		}
		outcnt++;
	}
	if(opos) {
		fprintf(stdout,"INFO: Level Check at %lf secs\n",(double)outcnt/srate);
		fflush(stdout);
		for(n = 0;n < opos;n++)
			maxsamp = max(maxsamp,fabs(obuf[n]));
	}
	dz->tempsize = outcnt * chans;	/* For sloom progress bar */
	normaliser = 1.0;
	if(maxsamp > 0.95) {
		normaliser = 0.95/maxsamp;
		fprintf(stdout,"INFO: Normalising by %lf secs\n",(double)normaliser);
		fflush(stdout);
	}	
	for(n=0;n<ptrcnt;n++) {
		ptr[n] = -1;
		inf[n] = 0;
	}
	nexttime = 0.0;
	inexttime = 0;
	stepcnt = -1;
	opos = 0;
	outcnt = 0;
	fprintf(stdout,"INFO: Second pass: generating output.\n");
	fflush(stdout);
	memset((char *)obuf,0,dz->buflen * sizeof(float));
	while(outcnt < dz->iparam[MAD_DUR]) {
		if(outcnt >= inexttime) {

			//	GO TO NEXT STEP VALUES, CHECKING WE'RE NOT AT OUTPUT END			

			stepcnt++;
			time = (double)inexttime/srate;
			if(time >= dz->param[MAD_DUR])
				break;
			insnd = fileon[stepcnt];
			endtime = time + (double)(dz->insams[insnd])/srate;
			if(endtime >= dz->param[MAD_DUR])
				break;

			//	FIND AN INACTIVE POINTER ASSOCIATED WITH EACH ON-STREAM, AND SET IT TO START COUNTING IN THE SPECIFIED insnd BUFFER

			for(n = 0;n<dz->iparam[MAD_STRMS];n++) {
				if(onoff[n][stepcnt]) {						//	For every stream that is on
					for(m=n; m < ptrcnt; m+= dz->iparam[MAD_STRMS]) {	//	Find an available buffer-pointer assocd with that stream
						if(ptr[m] < 0 || ptr[m] >= dz->insams[inf[m]]) {
							ptr[m] = 0;					//	and set it to start counting in an infile
							inf[m] = insnd;				//	indicating which infile it is pointing to.
							break;							//	(as events may overlap within a stream, a ptr may still be playing tail of a DIFFERENT sound
						}									//	 when new event starts, so we need to remember which insound a pointer WAS reading from)
					}
				}
			}
		}

		//	WRITE TO NEXT SAMPLE IN OUTBUFFER FROM ALL ACTIVE POINTERS

		for(n = 0;n<ptrcnt;n++) {							//	For every pointer...
			if(ptr[n] >= 0 && ptr[n] < dz->insams[inf[n]]) {	//	If it is active
				nn = n % dz->iparam[MAD_STRMS];				//	Find the associated stream
															//	Add contribution to outbuffer, in correct spatial position
				obuf[opos + lmost[nn]] = (float)(obuf[opos + lmost[nn]] + (ibuf[inf[n]][ptr[n]] * llev[nn]));
				obuf[opos + rmost[nn]] = (float)(obuf[opos + rmost[nn]] + (ibuf[inf[n]][ptr[n]] * rlev[nn]));
					//		position-nn										pointer[n]			levels[nn]
					//	assocd with output stream				   	  is associated with	are associated with
					//	and corresponds to chans					     sound inf[n]			spatial pos
					//	   in output buffer							   which it is reading	 of output stream
			}
			if(ptr[n] >= 0)									//	Advance all switched-on ptrs, whether are not they are currently reading a file.
				ptr[n]++;									//	Once they are beyond file end, they become available to select for next read.
		}

		//	ADVANCE IN OUTBUFFER, WRITING OUTPUT IF BUFFER IS FULL
		
		opos += chans;
		if(opos >= dz->buflen) {
			if(normaliser < 1.0) {
				for(n = 0;n<dz->buflen;n++)
					obuf[n] = (float)(obuf[n] * normaliser);
			}
			if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
				return(exit_status);
			memset((char *)obuf,0,dz->buflen * sizeof(float));
			opos = 0;
		}
		
		//	IF WE'VE JUST STARTED A NEW EVENT, DETERMINE TIME OF NEXT EVENT

		if(outcnt >= inexttime)
			inexttime = itime[stepcnt];

		//	ADVANCE THE OUTPUT GROUP-SAMPLE COUNTER

		outcnt++;
	}
	finished = 0;
	while(!finished) {
		finished = 1;
		for(n = 0;n<ptrcnt;n++) {							//	For every pointer...
			if(ptr[n] > 0 && ptr[n] < dz->insams[inf[n]]) {	//	If it is active
				finished = 0;								//	Flag not yet finished
				break;
			}
		}
		if(finished)
			break;
		for(n = 0;n<ptrcnt;n++) {
			if(ptr[n] > 0 && ptr[n] < dz->insams[inf[n]]) {
				nn = n % dz->iparam[MAD_STRMS];
				obuf[opos + lmost[nn]] = (float)(obuf[opos + lmost[nn]] + (ibuf[inf[n]][ptr[n]] * llev[nn]));
				obuf[opos + rmost[nn]] = (float)(obuf[opos + rmost[nn]] + (ibuf[inf[n]][ptr[n]] * rlev[nn]));
			}
			if(ptr[n] > 0)
				ptr[n]++;
		}
		opos += chans;
		if(opos >= dz->buflen) {
			if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
				return(exit_status);
			memset((char *)obuf,0,dz->buflen * sizeof(float));
			opos = 0;
		}
		outcnt++;
	}
	if(opos) {
		if(normaliser < 1.0) {
			for(n = 0;n<dz->buflen;n++)
				obuf[n] = (float)(obuf[n] * normaliser);
		}
		if((exit_status = write_samps(obuf,opos,dz))<0)		
			return(exit_status);
	}
	return FINISHED;
}

/**************************** CREATE_MADRID_SNDBUFS ****************************/

int create_madrid_sndbufs(dataptr dz)
{
	int n, safety = 4;
	unsigned int lastbigbufsize, bigbufsize = 0;
	float *bottom;
	dz->bufcnt = dz->infilecnt+1;
	if((dz->sampbuf = (float **)malloc(sizeof(float *) * (dz->bufcnt+1)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffers.\n");
		return(MEMORY_ERROR);
	}
	if((dz->sbufptr = (float **)malloc(sizeof(float *) * dz->bufcnt))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffer pointers.\n");
		return(MEMORY_ERROR);
	}
	lastbigbufsize = 0;
	bigbufsize = 0;
	for(n=0;n<dz->infilecnt;n++) {
		bigbufsize += (dz->insams[n] + safety) * sizeof(float);
		if(bigbufsize < lastbigbufsize) {
			sprintf(errstr,"Insufficient memory to store the input soundfiles in buffers.\n");
			return(MEMORY_ERROR);
		}
		lastbigbufsize = bigbufsize;
	}
	dz->buflen = NTEX_OBUFSIZE * dz->iparam[MAD_CHANS];	
	bigbufsize += (dz->buflen + (safety * dz->iparam[MAD_CHANS])) * sizeof(float);
	if(bigbufsize < lastbigbufsize) {
		sprintf(errstr,"Insufficient memory to store the input soundfiles in buffers.\n");
		return(MEMORY_ERROR);
	}
	if((dz->bigbuf = (float *)malloc(bigbufsize)) == NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
		return(PROGRAM_ERROR);
	}
	bottom = dz->bigbuf;
	for(n = 0;n<dz->infilecnt;n++) {
		dz->sbufptr[n] = dz->sampbuf[n] = bottom;
		bottom += dz->insams[n] + safety;
	}
	dz->sbufptr[n] = dz->sampbuf[n] = bottom;
	return(FINISHED);
}

/*********************** RNDINTPERM ************************/

void rndintperm(int *perm,int cnt)
{
	int n,t,k;
	memset((char *)perm,0,cnt * sizeof(int));
	for(n=0;n<cnt+1;n++) {
		t = (int)(drand48() * (double)(n+1)); /* TRUNCATE */
		if(t==n) {
			for(k=n;k>0;k--)
				perm[k] = perm[k-1];
			perm[0] = n;
		} else {
			for(k=n;k>t;k--)
				perm[k] = perm[k-1];
			perm[t] = n;
		}
	}
	for(n=0;n<cnt;n++)
		perm[n]--;
}

/************************************ PANCALC *******************************/

void pancalc(double position,double *leftgain,double *rightgain)
{
	int dirflag;
	double temp;
	double relpos;
	double reldist, invsquare;

	if(position < 0.0)
		dirflag = SIGNAL_TO_LEFT;		/* signal on left */
	else
		dirflag = SIGNAL_TO_RIGHT;

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
		if(dirflag == SIGNAL_TO_LEFT){
			*leftgain = invsquare;
			*rightgain = 0.0;
		} else {   /* SIGNAL_TO_RIGHT */
			*rightgain = invsquare;
			*leftgain = 0;
		}
	}
}

/**************************** CHECK_MADRID_PARAM_VALIDITY_AND_CONSISTENCY *****************************/

int check_madrid_param_validity_and_consistency(dataptr dz)
{
	if(dz->mode == 0) {
		if(dz->vflag[MAD_INPERM] && dz->vflag[MAD_INRAND]) {
			sprintf(errstr,"You cannot set both the random permutation (-r), and the random selection (-R) flag.\n");
			return(DATA_ERROR);
		}
	}
	return FINISHED;
}

/****************************** GET_THE_MODE_FROM_CMDLINE *********************************/

int get_the_mode_from_cmdline(char *str,dataptr dz)
{
	char temp[200], *p;
	if(sscanf(str,"%s",temp)!=1) {
		fprintf(stderr,"Cannot read mode of program.\n");
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

/**************************** HANDLE_THE_SPECIAL_DATA ****************************/

int handle_the_special_data(char *str,dataptr dz)
{
	int *evseq = dz->iparray[6 + dz->iparam[MAD_STRMS]], warned = 0, totalindatacnt, eventcnt, OK;
	FILE *fp;
	double dummy;
	int seqcnt, idummy;
	char temp[200], *p;

	if((fp = fopen(str,"r"))==NULL) {
		sprintf(errstr,"Cannot open file %s to read sequence data.\n",str);
		return(DATA_ERROR);
	}
	totalindatacnt = 0;
	while(fgets(temp,200,fp)!=NULL) {
		p = temp;
		while(isspace(*p))
			p++;
		if(*p == ';' || *p == ENDOFSTR)	//	Allow comments in file
			continue;
		while(get_float_from_within_string(&p,&dummy)) {
			idummy = (int)round(dummy);
			if(idummy < 1) {
				sprintf(errstr,"Found sequence value %d in file %s: Sequence values must be >= 1\n",idummy,str);
				return(DATA_ERROR);
			} else if(idummy > dz->infilecnt) {
				if(!warned) {
					fprintf(stdout,"WARNING: Found sequence value(s) %d outside the range 1 to %d: These will be wrapped into range.\n",idummy,dz->infilecnt);
					fflush(stdout);
					warned = 1;
				}
			}
			totalindatacnt++;
		}
	}
	fseek(fp,0,0);
	seqcnt = 0;
	eventcnt = 0;
	OK = 1;
	while(fgets(temp,200,fp)!=NULL) {
		p = temp;
		while(isspace(*p))
			p++;
		if(*p == ';' || *p == ENDOFSTR)	//	Allow comments in file
			continue;
		while(get_float_from_within_string(&p,&dummy)) {
			idummy = (int)round(dummy);
			idummy--;
			idummy %= dz->infilecnt;
			seqcnt++;
			evseq[eventcnt++] = idummy;
			if(eventcnt >= dz->itemcnt) {	//	If more data than spaces for events
				OK = 0;						//	Quit
				break;
			}
		}
		if(!OK)
			break;
	}
	fclose(fp);
	if(totalindatacnt > dz->itemcnt) {
		fprintf(stdout,"INFO: Not all of sequence in file %s will be used.\n",str);
		fflush(stdout);
	}
	while(eventcnt < dz->itemcnt) {					//	If length of sequence indata is less than number of events
		evseq[eventcnt] = evseq[eventcnt % seqcnt];	//	Duplicate sequence through all possible events
		eventcnt++;
	}
	return FINISHED;
}