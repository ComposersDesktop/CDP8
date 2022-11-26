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


#ifdef unix
#define round(x) lround((x))
#endif

char errstr[2400];

int anal_infiles = 1;
int	sloom = 0;
int sloombatch = 0;

const char* cdp_version ="7.1.0";

#define	dig		 ringsize
#define	digsteps itemcnt
#define arrraysize rampbrksize
#define CACOSTABSIZ	16384

//CDP LIB REPLACEMENTS
static int check_cantor_param_validity_and_consistency(dataptr dz);
static int setup_cantor_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_cantor_param_ranges_and_defaults(dataptr dz);
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
//static double dbtolevel(double val);
static int cantor(dataptr dz);
static int create_cantor_sndbufs(dataptr dz);
static int cut_hole(int startseg,int endseg,double holsttfrac,double holendfrac,double splincr,float *ibuf,float *obuf,int holeno,dataptr dz);
static int woblwobl(float *ibuf,float *obuf,int *woblcnt,int stepcnt,double maxatten,dataptr dz);

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
		if((get_the_process_no(argv[0],dz))<0)
			return(FAILED);
		cmdline++;
		cmdlinecnt--;
		dz->maxmode = 3;
		if((exit_status = get_the_mode_from_cmdline(cmdline[0],dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(exit_status);
		}
		cmdline++;
		cmdlinecnt--;
		// setup_particular_application =
		if((exit_status = setup_cantor_application(dz))<0) {
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
	if((exit_status = setup_cantor_param_ranges_and_defaults(dz))<0) {
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
	if((exit_status = check_cantor_param_validity_and_consistency(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	is_launched = TRUE;
	if((exit_status = create_cantor_sndbufs(dz))<0) {								// CDP LIB
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = open_the_outfile(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	//param_preprocess()						redundant
	//spec_process_file =
	if((exit_status = cantor(dz))<0) {
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
	char *p, *filename = (*cmdline)[0];
	int n;
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
	if(sloom) {						//	IF sloom, drop trailing zero
		n = strlen(dz->outfilename);
		n--;
		dz->outfilename[n] = ENDOFSTR;
	}
//	strcpy(dz->outfilename,filename);	   
	(*cmdline)++;
	(*cmdlinecnt)--;
	return(FINISHED);
}

/************************ OPEN_THE_OUTFILE *********************/

int open_the_outfile(dataptr dz)
{
	int exit_status;
	char filename[400];
	strcpy(filename,dz->outfilename);
	strcat(filename,"0");
	if((exit_status = create_sized_outfile(filename,dz))<0)
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

/************************* SETUP_CANTOR_APPLICATION *******************/

int setup_cantor_application(dataptr dz)
{
	int exit_status;
	aplptr ap;
	if((exit_status = establish_application(dz))<0)		// GLOBAL
		return(FAILED);
	ap = dz->application;
	// SEE parstruct FOR EXPLANATION of next 2 functions
	if(dz->mode == 2) {
		if((exit_status = set_param_data(ap,0   ,5,5,"diidd"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","",0,0,""))<0)
			return(FAILED);
	} else {
		if((exit_status = set_param_data(ap,0   ,5,5,"ddddd"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","e",1,0,"0"))<0)
			return(FAILED);
	}
	// set_legal_infile_structure -->
	dz->has_otherfile = FALSE;
	// assign_process_logic -->
	dz->input_data_type = SNDFILES_ONLY;
	dz->process_type	= EQUAL_SNDFILE;	
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

/************************* SETUP_CANTOR_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_cantor_param_ranges_and_defaults(dataptr dz)
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
	case(0):
		ap->lo[CA_HOLEN]	= 0.0;
		ap->hi[CA_HOLEN]	= 99.0;
		ap->default_val[CA_HOLEN]	= 0.333;
		ap->lo[CA_HOLEDIG]	= 0.001;
		ap->hi[CA_HOLEDIG]	= 1.0;
		ap->default_val[CA_HOLEDIG] = 0.1;
		ap->lo[CA_TRIGLEV]	= 0.001;
		ap->hi[CA_TRIGLEV]	= 1.0;
		ap->default_val[CA_TRIGLEV]	= 0.5;
		ap->lo[CA_SPLEN]	= 3;
		ap->hi[CA_SPLEN]	= 50;
		ap->default_val[CA_SPLEN] = 5;
		break;
	case(1):
		ap->lo[CA_HOLEN]	= 0.0;
		ap->hi[CA_HOLEN]	= dz->duration/3.0;
		ap->default_val[CA_HOLEN] = dz->duration/3.0;
		ap->lo[CA_HOLEDIG]	= 0.001;
		ap->hi[CA_HOLEDIG]	= 1.0;
		ap->default_val[CA_HOLEDIG] = 0.1;
		ap->lo[CA_TRIGLEV]	= 0.001;
		ap->hi[CA_TRIGLEV]	= 1.0;
		ap->default_val[CA_TRIGLEV]	= 0.5;
		ap->lo[CA_SPLEN]	= 3;
		ap->hi[CA_SPLEN]	= 50;
		ap->default_val[CA_SPLEN] = 5;
		break;
	case(2):
		ap->lo[CA_HOLEN]	= 0.0;
		ap->hi[CA_HOLEN]	= 0.99;
		ap->default_val[CA_HOLEN] = 0.0;
		ap->lo[CA_HOLEDIG]	= 2;
		ap->hi[CA_HOLEDIG]	= 256;
		ap->default_val[CA_HOLEDIG] = 8;
		ap->lo[CA_WOBDEC]	= 0.01;
		ap->hi[CA_WOBDEC]	= 1;
		ap->default_val[CA_WOBDEC]	= 0.5;
		ap->lo[CA_WOBBLES]	= 1;
		ap->hi[CA_WOBBLES]	= 100;
		ap->default_val[CA_WOBBLES]	= 4;
		break;
	}
	ap->lo[CA_MAXDUR]	= dz->duration * 2;
	ap->hi[CA_MAXDUR]	= 32767;
	ap->default_val[CA_MAXDUR]	= 60.0;
	dz->maxmode = 0;
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
			if((exit_status = setup_cantor_application(dz))<0)
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
	usage2("set");
	return(USAGE_ONLY);
}

/**************************** CHECK_CANTOR_PARAM_VALIDITY_AND_CONSISTENCY *****************************/

int check_cantor_param_validity_and_consistency(dataptr dz)
{
	int cnt, totalcnt, firstholen, n, m, k;
	int chans = dz->infile->channels;
	dz->iparam[CA_SPLEN] = (int)round(dz->param[CA_SPLEN] * MS_TO_SECS * (double)dz->infile->srate);
	dz->digsteps = (int)ceil(1.0/dz->param[CA_HOLEDIG]);
	dz->dig = dz->iparam[CA_SPLEN]/dz->digsteps;		//	chunk of splicelen carved at each dig
	if(dz->dig * dz->digsteps != dz->iparam[CA_SPLEN]) {
		dz->dig++;
		dz->iparam[CA_SPLEN] = dz->dig * dz->digsteps;	//	splicelen is a multiple of level decrements
	}
	switch(dz->mode) {
	case(0):
		dz->param[CA_HOLEN] /= 100.0; 	
		firstholen = (int)round((dz->insams[0]/chans) * dz->param[CA_HOLEN]);
		if(!dz->vflag[0] && (firstholen <= dz->iparam[CA_SPLEN] * 2)) {
			sprintf(errstr,"Hole too short for splice length specified.\n");
			return(DATA_ERROR);
		}
		break;
	case(1):
		dz->iparam[CA_HOLEN] = (int)round(dz->param[CA_HOLEN] * (double)dz->infile->srate) * chans;  	
		if(dz->iparam[CA_SPLEN] * chans * 2 >= dz->iparam[CA_HOLEN]) {
			sprintf(errstr,"Holes too small for splice length given.\n");
			return(DATA_ERROR);
		}
		if(dz->iparam[CA_SPLEN] < chans) {
			sprintf(errstr,"Holes too small.\n");
			return(DATA_ERROR);
		}
		break;
	}
	dz->iparam[CA_MAXDUR] = (int)round(dz->param[CA_MAXDUR] * (double)dz->infile->srate) * chans; 
	
	if(dz->mode == 2) {
		if((dz->parray = (double **)malloc(dz->iparam[CA_WOBBLES] * sizeof(double *)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to create data double arrays.\n");
			return(MEMORY_ERROR);
		}
		for(n=0;n < dz->iparam[CA_WOBBLES];n++) {
			if((dz->parray[n] = (double *)malloc((CACOSTABSIZ + 1) * sizeof(double)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY to create cosin data table.\n");
				return(MEMORY_ERROR);
			}
		}
		for(n=0;n < dz->iparam[CA_WOBBLES];n++) {
			for(m=0;m < CACOSTABSIZ;m++) {
				dz->parray[n][m] = cos(((double)m/(double)CACOSTABSIZ) * 2.0 * PI);
				dz->parray[n][m] += 1.0;
				dz->parray[n][m] /= 2.0;
				dz->parray[n][m] = 1.0 - dz->parray[n][m];
			}
			dz->parray[n][m] = dz->parray[n][m-1];
		}
		for(n=0,m = dz->iparam[CA_WOBBLES];n < dz->iparam[CA_WOBBLES]-1;n++,m--) {
			for(k=0;k < CACOSTABSIZ;k++) 
				dz->parray[n][k] = pow(dz->parray[n][k],m);
			dz->parray[n][k] = dz->parray[n][k-1];
		}
	} else {
		if((dz->iparray = (int **)malloc(3 * sizeof(int *)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to create data integer arrays.\n");
			return(MEMORY_ERROR);
		}
		dz->arrraysize = 0;
		totalcnt = dz->insams[0]/chans;
		while(totalcnt >= 1) {
			dz->arrraysize++;
			if(totalcnt == 1)
				break;
			cnt = totalcnt/2;
			if(cnt * 2 != totalcnt)
				cnt++;
			totalcnt = cnt;
		}
		dz->arrraysize += 4;// SAFETY
		if((dz->iparray[0] = (int *)malloc(dz->arrraysize * sizeof(int)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to create hole count stores.\n");
			return(MEMORY_ERROR);
		}
		for(cnt=0;cnt<dz->arrraysize;cnt++)
			dz->iparray[0][cnt] = 1;

		if((dz->iparray[1] = (int *)malloc(dz->arrraysize * sizeof(int)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to create hole count stores.\n");
			return(MEMORY_ERROR);
		}
		for(cnt=0;cnt<dz->arrraysize;cnt++)
			dz->iparray[1][cnt] = CONTINUE;
		if((dz->iparray[2] = (int *)malloc(dz->arrraysize * sizeof(int)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to create hole count stores.\n");
			return(MEMORY_ERROR);
		}
		for(cnt=0;cnt<dz->arrraysize;cnt++)
			dz->iparray[2][cnt] = 0;
	}
	return FINISHED;
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	if(!strcmp(prog_identifier_from_cmdline,"set"))				dz->process = CANTOR;
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
	if(!strcmp(str,"set")) {
		fprintf(stdout,
	    "USAGE:\n"
	    "cantor set infile outfile 1-2 holesize holedig depth-trig splicelen maxdur [-e]\n"
	    "cantor set infile outfile 3   holelev  holedig layercnt layerdec maxdur\n"
		"\n"
		"Gradually cut hole in central 3rd of input sound.\n"
		"Cut holes in central 3rd of the remaining segments, and so on.\n"
		"Output is a sequence of sounds with more and more holes cut.\n"
		"Mode 3 uses superimposed vibrati envelopes\n"
		"\n"
		"MODES 1 and 2\n"
		"\n"
		"HOLESIZE   MODE 1: Percentage of current segment-time taken up by hole.\n"
		"           Size of hole depends on size of segment being cut.\n"
		"           MODE 2: (Fixed) Duration of holes.\n"
		"HOLEDIG    Depth of each cut as hole is gradually created (>0-1).\n"
		"DEPTH-TRIG Level-Depth of hole triggering next hole-cutting.\n"
		"SPLICELEN  Splicelength in mS.\n"
		"MAXDUR     Maximum total duration of all the output sound.\n"
		"-e         Extend sound beyond splicelen limits.\n"
		"\n"
		"MODES 3\n"
		"\n"
		"HOLELEV    Level of signal at base of holes.\n"
		"HOLEDIG    How many repets before full-depth reached.\n"
		"LAYERCNT   Number of vibrato layers used.\n"
		"LAYERDEC   Depth of next vibrato in relation to previous\n"
		"MAXDUR     Maximum total duration of all the output sound.\n"
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

/******************************** CANTOR ********************************/

int cantor(dataptr dz)
{
	int exit_status, status;
	char thisnum[200], filename[400];
	double holsttfrac = 1.0, holendfrac = 1.0;
	float *ibuf = dz->sampbuf[0], *obuf = dz->sampbuf[1];
	int startseg = 0, sum_total_samps = 0, fileno = 0, woblcnt = 0;
	int endseg   = dz->insams[0];
	double splincr = 1.0/dz->iparam[CA_SPLEN];
	int stepcnt = 0;
	double maxatten = 0.0;
	if(dz->mode == 2) {
		stepcnt  = dz->iparam[CA_HOLEDIG];
		maxatten = 1.0 - dz->param[CA_HOLEN];
	}
	if((exit_status = read_samps(ibuf,dz))<0)
		return(exit_status);
	if(dz->mode == 0) {
		holsttfrac = (1.0 - dz->param[CA_HOLEN])/2.0;
		holendfrac = 1.0 - holsttfrac;
	}
	for(;;) {
		if(fileno > 0) {
			if(sndcloseEx(dz->ofd) < 0) {
				sprintf(errstr,"Failed to close output file %d\n",fileno+1);
				return(SYSTEM_ERROR);
			}
			dz->ofd = -1;
			strcpy(filename,dz->outfilename);
			sprintf(thisnum,"%d",fileno);
			strcat(filename,thisnum);
			strcat(filename,".wav");
			if((exit_status = create_sized_outfile(filename,dz))<0)
				return(exit_status);
			sndseekEx(dz->ifd[0],0,0);
			reset_filedata_counters(dz);
		}
		memcpy((char *)obuf,(char *)ibuf,dz->insams[0] * sizeof(float));
		if(dz->mode == 2)
			exit_status = woblwobl(ibuf,obuf,&woblcnt,stepcnt,maxatten,dz);
		else
			exit_status = cut_hole(startseg,endseg,holsttfrac,holendfrac,splincr,ibuf,obuf,0,dz);
		if(exit_status<0)
			return GOAL_FAILED;
		status = exit_status;
		if((exit_status = write_samps(obuf,dz->insams[0],dz))<0)
			return(exit_status);
		if(status != CONTINUE)
			break;
		memset((char *)obuf,0,dz->insams[0] * sizeof(float));
		sum_total_samps += dz->total_samps_written;
		if(sum_total_samps >= dz->iparam[CA_MAXDUR])
			return FINISHED;
		fileno++;
	}
	return exit_status;
}

/******************************** CUT_HOLE ********************************/

int cut_hole(int startseg,int endseg,double holsttfrac,double holendfrac,double splincr,float *ibuf,float *obuf,int holeno,dataptr dz)
{
	int chans = dz->infile->channels, k, j, m;
	int seglen, holstt, holend, hollen, digdepth, digcnt, holendsttsplice, splen, digmax;
	int gphollen, halfgphollen, holgpcentre, over;
	double spliceval, centrelevel;
	int *exitval = dz->iparray[1];
	int passlimit = (int)round(pow(2,holeno)); 
	int passno = dz->iparray[2][holeno];
	seglen = endseg - startseg;
	splen = dz->iparam[CA_SPLEN];						//	and its length
	digdepth = dz->dig;									//	digdepth is how far we dig hole (furTher) on each pass, till will reach full cut
	if(dz->mode == 0) {
		if(seglen <= 0) {
			exitval[holeno] = FINISHED;
			return FINISHED;
		}
		holstt = (int)round((seglen/chans) * holsttfrac) + startseg;
		holend = (int)round((seglen/chans) * holendfrac) + startseg;
		hollen = holend - holstt;							//	Find start and end of hole, and its length
		if(dz->vflag[0]) {									//	If extending to smallest possible segment
			if(hollen <= 0)	{								//	If no hole to dig, quit	
				exitval[holeno] = FINISHED;
				return FINISHED;
			}
			else if(hollen <= 2 * splen) {					//	Or if hole is smaller than 2-splices, reduce splicelens
				splen = hollen/2;
				digdepth = (int)round((double)splen/(double)dz->digsteps);
				if(digdepth < 1)
					return FINISHED;
				splincr  = 1.0/(double)splen;
			}
		} else {											//	Once holes smaller than 2 splices, quit
			if(hollen <= 2 * splen) {
				exitval[holeno] = FINISHED;
				return FINISHED;
			}
		}
	} else {
		if(seglen <= dz->iparam[CA_HOLEN]) {
			exitval[holeno] = FINISHED;
			return FINISHED;
		}
		gphollen     = dz->iparam[CA_HOLEN]/chans;
		halfgphollen = gphollen/2; 
		holgpcentre  = (seglen/chans)/2;
		holstt = holgpcentre - halfgphollen;
		holend = holstt + gphollen;
		holstt *= chans;
		holend *= chans;
		holstt += startseg;
		holend += startseg;
		if((over = startseg - holstt) > 0) {
			holstt += over;
			holend += over;
			if(holend >= endseg) {
				return FINISHED;
			}
		} else if((over = holend - endseg) > 0) {
			holend -= over;
			holstt -= over;
			if(holstt < startseg) {
			}
			return FINISHED;
		}
		splincr  = 1.0/(double)splen;
	}
	holendsttsplice = holend - splen;
	digcnt = dz->iparray[0][holeno];
	holstt *= chans;
	holend *= chans;
	holendsttsplice *= chans;
	spliceval = 1.0 - splincr;
	digmax = min(splen, digcnt * digdepth);				//	Maximum depth (therefore sample count) to which we dig splice on this pass
	holendsttsplice = holend - digmax;				    //	Find where upsplice at end of hole begins
	for(k = 0, j = holstt; k < digmax;k++,j+=chans) {
		for(m=0;m<chans;m++)							//	Dig downsplice as far as ness on this pass
			obuf[j+m] = (float)(ibuf[j+m] * spliceval);
		spliceval = max(0.0,spliceval - splincr);
	}
	centrelevel = spliceval;
	while(j < holendsttsplice) {						//	Central part is reduced in level (until it gets to zero after successive passes)
		obuf[j] = (float)(ibuf[j] * spliceval);
		j++;
	}
	spliceval += splincr;
	for(k = 0; k < digmax;k++,j+=chans) {				//	Do upsplice
		for(m=0;m<chans;m++)
			obuf[j+m] = (float)(ibuf[j+m] * spliceval);
		spliceval = min(1.0,spliceval + splincr);
	}
	passno++;
	dz->iparray[2][holeno] = passno;				//	Count number of passes, so we know when we're at last one
	if(passno >= passlimit)
		(dz->iparray[0][holeno])++;					//	Count no of digs, so we know how deep to dig in next sound

	if(centrelevel < dz->param[CA_TRIGLEV]) {			//	If centre of hole reaches triggering level
														//	Dig holes in the two sidearms of this segment
		if((exitval[holeno+1] = cut_hole(startseg,holstt,holsttfrac,holendfrac,splincr,ibuf,obuf,holeno+1,dz))<0)
			return(exitval[holeno+1]);
		if(exitval[holeno+1] == FINISHED && (passno >= passlimit))
			exitval[holeno] = FINISHED;				
		if((exitval[holeno+1] = cut_hole(holend,endseg,holsttfrac,holendfrac,splincr,ibuf,obuf,holeno+1,dz))<0)
			return(exitval[holeno+1]);
		if(exitval[holeno+1] == FINISHED && (passno >= passlimit))
			exitval[holeno] = FINISHED;				
	}
	if(exitval[holeno] == FINISHED && (passno >= passlimit)) {
		return FINISHED;
	}
	exitval[holeno] = CONTINUE;				
	return CONTINUE;
}

/**************************** CREATE_CANTOR_SNDBUFS ****************************/

int create_cantor_sndbufs(dataptr dz)
{
	int n;
	int bigbufsize;
	int framesize;
	framesize = F_SECSIZE * dz->infile->channels;
	dz->bufcnt = 2;
	if((dz->sampbuf = (float **)malloc(sizeof(float *) * (dz->bufcnt+1)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffers.\n");
		return(MEMORY_ERROR);
	}
	if((dz->sbufptr = (float **)malloc(sizeof(float *) * dz->bufcnt))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffer pointers.\n");
		return(MEMORY_ERROR);
	}
	dz->buflen = dz->insams[0];
	dz->buflen = (dz->buflen / framesize) * framesize;
	dz->buflen += framesize;
	bigbufsize = dz->buflen * sizeof(float);
	if(bigbufsize <=0 || bigbufsize * 2 <=0) {
		sprintf(errstr,"Input sound too large for this process\n");
		return(DATA_ERROR);
	}
	if((dz->bigbuf = (float *)malloc(bigbufsize  * dz->bufcnt)) == NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
		return(PROGRAM_ERROR);
	}
	for(n=0;n<dz->bufcnt;n++)
		dz->sbufptr[n] = dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
	dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
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

/****************************** WOBLWOBL *********************************/

int woblwobl(float *ibuf,float *obuf,int *woblcnt,int stepcnt,double maxatten,dataptr dz)
{
	double factor, interval, tabpos, tablen = (double)CACOSTABSIZ, frac, diff, val;
	double *tab;
	int t, n, ipos;
	int  fac;
	factor = 1.0;								//	How deep is the vibrato layer
	for(t=0;t < dz->iparam[CA_WOBBLES];t++) {	//	Apply each vibrato layer in turn
		tab = dz->parray[t];
		fac = (t*2) + 1;						//	Read table faster for later (faster) vibrato layers	
		interval = pow(2.0,((double)(*woblcnt)/(double)stepcnt));	//	Range 1 - 2 oct
		interval -= 1.0;												//	Range 0 - 1 oct
		interval *= factor;												//	Reduce interval for later wobbles
		interval = min(interval,1.0);									//	Block at 8va max
		interval *= maxatten;					//	Confine loudness variation to range specified		
		tabpos = 0.0;
		if(interval > 0.0) {
			for(n=0;n<dz->insams[0];n++) {
				tabpos = ((double)n/(double)dz->insams[0]) * fac * tablen;
				while(tabpos >= CACOSTABSIZ)
					tabpos -= CACOSTABSIZ;
				ipos = (int)floor(tabpos);
				frac = tabpos - (double)ipos;
				diff = tab[ipos+1] - tab[ipos];
				frac *= diff;
				val  = tab[ipos] + frac;						//	Val from inverted cos table range 0 up to 1
				val *= interval;								//	range 0 up to maxrange
				val = 1.0 - val;								//	Val from 1 down to (1.0-maxatten)
				obuf[n] = (float)(obuf[n] * val);
			}
		}
		factor *= dz->param[CA_WOBDEC];			//	Reduce depth of next vibrato layer
	}
	(*woblcnt)++;
	return(CONTINUE);
}
