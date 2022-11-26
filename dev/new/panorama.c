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

#define	SIGNAL_TO_LEFT	(0)
#define SIGNAL_TO_RIGHT (1)
#define ROOT2			(1.4142136)

#ifdef unix
#define round(x) lround((x))
#endif

char errstr[2400];

int anal_infiles = 1;
int	sloom = 0;
int sloombatch = 0;

const char* cdp_version = "7.1.0";

//CDP LIB REPLACEMENTS
static int check_panorama_param_validity_and_consistency(dataptr dz);
static int setup_panorama_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_panorama_param_ranges_and_defaults(dataptr dz);
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
static int handle_the_special_data(char *str,dataptr dz);
static int panorama(dataptr dz);
static void pancalc(double position,double *leftgain,double *rightgain);
static int panorama_param_preprocess(dataptr dz);

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
		dz->maxmode = 2;
		if((get_the_mode_from_cmdline(cmdline[0],dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		cmdline++;
		cmdlinecnt--;
		// setup_particular_application =
		if((exit_status = setup_panorama_application(dz))<0) {
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
	if(dz->infilecnt < 2) {
		sprintf(errstr,"Process  only works with more than one input file\n");
		exit_status = print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(exit_status);		 
	}
/* TEST */
fprintf(stderr,"555 dz->infilecnt = %d\n",dz->infilecnt);
/* TEST */
	// parse_infile_and_hone_type() = 
	dz->process = MIXDUMMY;
	if((exit_status = parse_infile_and_check_type(cmdline,dz))<0) {
		exit_status = print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	dz->process = PANORAMA;
	// setup_param_ranges_and_defaults() =
	if((exit_status = setup_panorama_param_ranges_and_defaults(dz))<0) {
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
	dz->process = MIXDUMMY;
	if((exit_status = handle_extra_infiles(&cmdline,&cmdlinecnt,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);		
		return(FAILED);
	}
	dz->process = PANORAMA;
/* TEST */
fprintf(stderr,"999 cmdline[0] = %s\n",cmdline[0]);
/* TEST */
	// handle_outfile() = 
	if((exit_status = handle_the_outfile(&cmdlinecnt,&cmdline,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
/* TEST */
fprintf(stderr,"AAA cmdline[0] = %s\n",cmdline[0]);
/* TEST */

//	handle_formants()			redundant
//	handle_formant_quiksearch()	redundant
	if(dz->mode == 1) {
		if((exit_status = handle_the_special_data(cmdline[0],dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		cmdlinecnt--;
		cmdline++;
	}
/* TEST */
fprintf(stderr,"BBB\n");
/* TEST */
	if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {		// CDP LIB
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
/* TEST */
fprintf(stderr,"CCC\n");
/* TEST */
//	check_param_validity_and_consistency....
	if((exit_status = check_panorama_param_validity_and_consistency(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	is_launched = TRUE;
	if((exit_status = panorama_param_preprocess(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	//process_file =
	if((exit_status = panorama(dz))<0) {
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
	p = filename;
	while(*p != ENDOFSTR) {
		if(*p == '.') {
			*p = ENDOFSTR;
			break;
		}
		p++;
	}
	strcpy(dz->outfilename,filename);	   
	if(!sloom)
		strcat(dz->outfilename,".mmx");
	if((exit_status = create_sized_outfile(dz->outfilename,dz))<0)
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

/************************* SETUP_PANORAMA_APPLICATION *******************/

int setup_panorama_application(dataptr dz)
{
	int exit_status;
	aplptr ap;
	if((exit_status = establish_application(dz))<0)		// GLOBAL
		return(FAILED);
	ap = dz->application;
	// SEE parstruct FOR EXPLANATION of next 2 functions
	if(dz->mode == 1) {
		if((exit_status = set_param_data(ap,PANOLSPKRS,5,3,"00ddi"))<0)
			return(FAILED);
	} else {
		if((exit_status = set_param_data(ap,0,5,5,"idddi"))<0)
			return(FAILED);
	}
	if((exit_status = set_vflgs(ap,"r",1,"d","pq",2,0,"00"))<0)
		return(FAILED);
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
		} else if(infile_info->channels != 1)  {
			sprintf(errstr,"File %s is not of correct type (must be mono)\n",cmdline[0]);
			return(DATA_ERROR);
		} else if((exit_status = copy_parse_info_to_main_structure(infile_info,dz))<0) {
			sprintf(errstr,"Failed to copy file parsing information\n");
			return(PROGRAM_ERROR);
		}
		free(infile_info);
	}
	dz->all_words = 0;
	if((exit_status = store_filename(cmdline[0],dz))<0)
		return(exit_status);
	return(FINISHED);
}

/************************* SETUP_PANORAMA_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_panorama_param_ranges_and_defaults(dataptr dz)
{
	int exit_status;
	aplptr ap = dz->application;
	// set_param_ranges()
	ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
	// NB total_input_param_cnt is > 0 !!!
	if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
		return(FAILED);
	// get_param_ranges()
	if(dz->mode == 0) {
		ap->lo[PANO_LCNT]	= 3.0;
		ap->hi[PANO_LCNT]	= 16.0;
		ap->default_val[PANO_LCNT]	= 8;
		ap->lo[PANO_LWID]	= 190.0;
		ap->hi[PANO_LWID]	= 360.0;
		ap->default_val[PANO_LWID]	= 360;
	}
	ap->lo[PANO_SPRD]	= 0.0;
	ap->hi[PANO_SPRD]	= 360.0;
	ap->default_val[PANO_SPRD]	= 360;
	ap->lo[PANO_OFST]	= -180;
	ap->hi[PANO_OFST]	= 180;
	ap->default_val[PANO_OFST] = 0;
	ap->lo[PANO_CNFG]	= 1;
	ap->hi[PANO_CNFG]	= 128;
	ap->default_val[PANO_CNFG] = 1;
	ap->lo[PANO_RAND]	= 0;
	ap->hi[PANO_RAND]	= 1;
	ap->default_val[PANO_RAND] = 0;
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
			if((exit_status = setup_panorama_application(dz))<0)
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
	usage2("panorama");
	return(USAGE_ONLY);
}

/**************************** CHECK_PANORAMA_PARAM_VALIDITY_AND_CONSISTENCY *****************************/

int check_panorama_param_validity_and_consistency(dataptr dz)
{
	double *lspkrpos, *pos, rightmost, leftmost, anglestep;
	int n;
	if(dz->mode == 0) {							//	Set lspkr positions for mode 0
		if((dz->parray = (double **)malloc(2 * sizeof(double *)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for position storage arrays.\n");
			return(MEMORY_ERROR);
		}
		dz->itemcnt = dz->iparam[PANO_LCNT];	//	Array for loudspeaker positions
		if((dz->parray[0] = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to store loudspeaker positions.\n");
			return(MEMORY_ERROR);												
		}										//	Array for sound positions
		if((dz->parray[1] = (double *)malloc(dz->infilecnt * sizeof(double)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to store sound positions.\n");
			return(MEMORY_ERROR);
		}										//	Array for sorting
		if((dz->parray[2] = (double *)malloc(max(dz->infilecnt,dz->itemcnt) * sizeof(double)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to store sound positions.\n");
			return(MEMORY_ERROR);
		}									
		lspkrpos = dz->parray[0];
		pos = dz->parray[1];
		if(dz->param[PANO_LWID] < 360) {		//	distribute loudspeaker positions across angular width
			rightmost = dz->param[PANO_LWID]/2.0;
			leftmost  = 360.0 - dz->param[PANO_LWID]/2.0;
			anglestep = dz->param[PANO_LWID]/(double)(dz->itemcnt-1);
			for(n=dz->itemcnt-1;n>0;n--) {
				lspkrpos[n] = rightmost;
				if(flteq(lspkrpos[n],0.0))
					lspkrpos[n] = 0.0;
				else if(lspkrpos[n] < 0.0)
					lspkrpos[n] += 360.0;
				rightmost -= anglestep;
			}
			lspkrpos[0] = leftmost;
		} else {								//	distribute loudspeaker positions around circle
			anglestep = 360.00/dz->itemcnt;
			if(dz->vflag[0])					//	Front pair
				rightmost = -(anglestep/2.0);
			else								//	Front single
				rightmost = 0.0;
			for(n = 0;n < dz->itemcnt;n++) {
				lspkrpos[n] = rightmost;
				if(lspkrpos[n] < 0.0)
					lspkrpos[n] += 360.0;
				rightmost += anglestep;
			}
		}
	} 
	n = dz->infilecnt/dz->iparam[PANO_CNFG];	//	Check configuration parameter
	if(n * dz->iparam[PANO_CNFG] != dz->infilecnt) {
		sprintf(errstr,"Sound configuration value is not a divisor of number of input files.\n");
		return(DATA_ERROR);
	}											//	Check randomisation param
	if((dz->infilecnt == 2) && (dz->iparam[PANO_SPRD] < 360.0)) {
		if(dz->param[PANO_RAND] > 0.0) {
			sprintf(errstr,"No position randomisation possible with 2 inputs, and sound-spread < 360.\n");
			return(DATA_ERROR);
		}
	}
	return FINISHED;
}

/**************************** PANORAMA_PARAM_PREPROCESS *****************************/

int panorama_param_preprocess(dataptr dz)
{
	double *lspkrpos, *pos, *sort, halfdiffdn, halfdiffup, diffdn, diffup, randoff, temp;
	double sndleftmost, sndrightmost, sndangstep;
	int n, m, k, up, dn, stepcnt, thisstep;

	lspkrpos = dz->parray[0];
	pos      = dz->parray[1];
	sort     = dz->parray[2];
	for(n = 0;n <dz->itemcnt-1;n++) {					//	sort loudspeaker positions into angular ascending order from 0
		for(m=n;m<dz->itemcnt;m++) {
			if(lspkrpos[n] > lspkrpos[m]) {
				temp = lspkrpos[n];
				lspkrpos[n]  = lspkrpos[m];
				lspkrpos[m] = temp; 
			}
		}
	}													//	For no spaces in the configuration
	if(dz->iparam[PANO_CNFG] == 1) {					//	Position sounds AS IF THEY ARE zero-centred.
		sndleftmost  = 360 - dz->param[PANO_SPRD]/2.0;	//	startingSat the rightmost sound, divide snd-spread angle equally
		sndrightmost = dz->param[PANO_SPRD]/2.0;		//	For 360, snds (X) and steps (--) are equal in number
		stepcnt = dz->infilecnt;						//	X--X--X--X--X--X--
		if(dz->param[PANO_SPRD] < 360.0)				//	For < 360, there is 1 less step
			stepcnt--;									//	X--X--X--X--X--X
		sndangstep = dz->param[PANO_SPRD]/(double)stepcnt;
		if(dz->param[PANO_SPRD] >= 360.0) {				//	For 360, the last position, oocurs BEFORE the end of the angle-spread
			sndleftmost += sndangstep; 					//	or else 1st and last snds are superimposed:
			if((ODD(dz->infilecnt) && !dz->vflag[1])	//	If a sound at 0 degrees required, offset all vals half-a-turn
			|| (EVEN(dz->infilecnt) && dz->vflag[1])) {	//	Starting at 180, with an even number of files, 0 degrees will be used
				sndrightmost -= sndangstep/2.0;			//	so only offset if pairing-flag is set
				if(flteq(sndrightmost,0.0))				//	Starting at 180, with an odd number of files, 0 degrees will not be used
					sndrightmost = 0.0;					//	so offset if pairing-flag is not used
				else if(sndrightmost < 0)
					sndrightmost += 360.0;
				sndleftmost  -= sndangstep/2.0;
				if(flteq(sndleftmost,0.0))
					sndleftmost = 0.0;
				else if(sndleftmost < 0)
					sndleftmost += 360.0;
			}
		}
		for(n=dz->infilecnt - 1;n>0;n--) {
			pos[n] = sndrightmost;
			sndrightmost -= sndangstep;
			if(flteq(sndrightmost,0.0))
				sndrightmost = 0.0;
			else if(sndrightmost < 0)
				sndrightmost += 360.0;
		}
		pos[0] = sndleftmost;
	} else {											//	Where there are space in the configuration (config > 1)
		sndleftmost = 360 - dz->param[PANO_SPRD]/2.0;	//	introducing appropriate gaps into sound layout.
		sndrightmost = dz->param[PANO_SPRD]/2.0;
		thisstep = 0;
		if(dz->param[PANO_SPRD] < 360.0) {										//	6 positions with config 2, produces pairs with spaces 
			stepcnt = dz->infilecnt + (dz->infilecnt/dz->iparam[PANO_CNFG]) - 2; //	X--X--o--X--X--o--X--X 
			sndangstep = dz->param[PANO_SPRD]/(double)stepcnt;					//	So there is 1 space for each pair	6/2 = 3	
			for(n=dz->infilecnt - 1;n>0;n--) {									//	Except there is no space associated with last pair = (6/2) - 1
				pos[n] = sndrightmost;											//	Number of items (poss + spaces) = 6 + (6/2) - 1
				sndrightmost -= sndangstep;										//	Number of steps between items is 1 less, 6 + (6/2) - 2 = 8
				if(++thisstep == dz->iparam[PANO_CNFG]) {						//	6 positions with config 3, produces triplets with spaces
					sndrightmost -= sndangstep;									//	X--X--X--o--X--X--X 
					thisstep = 0;												//	So there is a space for each triplet	6/3 = 2	
				}																//	Except there is no space associated with last triplet = (6/3) - 1
				if(sndrightmost < 0)											//	Number of items (poss + spaces) = 6 + (6/3) - 1
					sndrightmost += 360.0;										//	Number of steps between items is 1 less, 6 + (6/3) - 1 = 7 
			}
			pos[0] = sndleftmost;
		} else {																//	Sound surround situation differs
			stepcnt = dz->infilecnt + (dz->infilecnt/dz->iparam[PANO_CNFG]);	//	(1) There is a space after EVERY position-set
			sndangstep = dz->param[PANO_SPRD]/(double)stepcnt;					//	(2) There is a step between EVERY position-or-space
			sndrightmost = 180;													//	Start at rear, ending on left of space
			sndleftmost = 180 + sndangstep;										//	So first and last don't overlap, move leftmost by a turn.
			if(EVEN(stepcnt)) {													//	In order to centre the central-group at 0 degrees
				thisstep = dz->iparam[PANO_CNFG]/2;								//	If an even number of steps,
				if(EVEN(dz->iparam[PANO_CNFG]))									//	Start counting spaces ALREADY half-way through a set
					sndrightmost -= (sndangstep * 0.5);							//	And, if an even set, need to offset the group start by half a turn
			} else {															//	If an odd number of steps, force a gap at rear, by
				sndrightmost -= sndangstep;										//	offsetting rightmost by 1 turn
				thisstep = 0;
			}
			for(n=dz->infilecnt - 1;n>=0;n--) {
				pos[n] = sndrightmost;
				sndrightmost -= sndangstep;
				if(++thisstep == dz->iparam[PANO_CNFG]) {
					sndrightmost -= sndangstep;
					thisstep = 0;
				}
				if(sndrightmost < 0)
					sndrightmost += 360.0;
				else if(sndrightmost >= 360.0)
					sndrightmost -= 360.0;
			}
		}
	}
	for(n=0;n<dz->infilecnt;n++) {						//	Offset the sound-positions
		pos[n] += dz->param[PANO_OFST];
		if(pos[n] > 360.0)
			pos[n] -= 360.0;
		else if(pos[n] < 0)
			pos[n] += 360.0;
	}
	for(n = 0;n <dz->infilecnt-1;n++) {					//	Sort sound positions into angular ascending order from 0
		for(m=n;m<dz->infilecnt;m++) {
			if(pos[n] > pos[m]) {
				temp = pos[n];
				pos[n]  = pos[m];
				pos[m] = temp; 
			}
		}
	}
	if(dz->param[PANO_RAND] > 0.0) {					//	Randomise the sound-positions
		for(n = 0;n <dz->infilecnt;n++) {				
			if(pos[n] > 180) {							//	sort into leftmost to rightmost order (for randomising positions)
				m = n;
				for(k = 0;m < dz->infilecnt;m++,k++)
					sort[k] = pos[m]; 
				for(m = 0;k < dz->infilecnt;m++,k++)
					sort[k] = pos[m]; 
				for(m=0;m<dz->infilecnt;m++)
					pos[m] = sort[m];
				break;
			}
		}
		for(n = 0;n < dz->infilecnt;n++)				//	Store original positions
			sort[n] = pos[n];
		dn = 0;
		up = 2;
		for(n = 1;n < dz->infilecnt-1;n++) {			//	Randomise all inner positions
			diffdn = (sort[n]  - sort[dn]);				//	X--r--r--r--r--r--X for < 360
			diffup = (sort[up] - sort[n]);				//
			if(diffdn < 0)
				diffdn += 360;
			if(diffup < 0)
				diffup += 360;
			halfdiffup = diffup/2.0;
			halfdiffdn = diffdn/2.0;
			randoff = (drand48() * 2.0) - 1.0;			//	X--r--r--r--r--r--X-- for 360
			randoff *= dz->param[PANO_RAND];			//	NB no effect if only 2 infiles
			if(randoff > 0.0) {
				pos[n] = sort[n] + (halfdiffup * randoff);
				if(pos[n] >= 360.0)
					pos[n] -= 360.0;
			} else {
				pos[n] = sort[n] - (halfdiffdn * randoff);
				if(pos[n] < 0.0)
					pos[n] += 360.0;
			}
			up++;
			dn++;
		}												//	For 360 degree layout
		if(dz->param[PANO_SPRD] >= 360) {				//	Randomise last outer positions
			if(dz->infilecnt >= 3) {					//	X--r--r--r--r--r--R--
				n = dz->infilecnt-1; 
				dn = dz->infilecnt-2;
				up = 0;	
				diffdn = sort[n]  - sort[dn];
				diffup = sort[up] - sort[n];
				if(diffdn < 0)
					diffdn += 360;
				if(diffup < 0)
					diffup += 360;
				halfdiffup = diffup/2.0;
				halfdiffdn = diffdn/2.0;
				randoff = (drand48() * 2.0) - 1.0;
				randoff *= dz->param[PANO_RAND];
				if(randoff > 0.0) {
					pos[n] = sort[n] + (halfdiffup * randoff);
					if(pos[n] >= 360.0)
						pos[n] -= 360.0;
				} else {
					pos[n] = sort[n] - (halfdiffdn * randoff);
					if(pos[n] < 0.0)
						pos[n] += 360.0;
				}
				n  = 0;									//	Randomise first outer position
				up = 1;									//	R--r--r--r--r--r--r--
				dn = dz->infilecnt-1;
				diffdn = sort[n] - sort[dn];
				diffup = sort[up] - sort[n];
				if(diffdn < 0)
					diffdn += 360;
				if(diffup < 0)
					diffup += 360;
				halfdiffup = diffup/2.0;
				halfdiffdn = diffdn/2.0;
				randoff = (drand48() * 2.0) - 1.0;
				randoff *= dz->param[PANO_RAND];
				if(randoff > 0.0) {
					pos[n] = sort[n] + (halfdiffup * randoff);
					if(pos[n] >= 360.0)
						pos[n] -= 360.0;
				} else {
					pos[n] = sort[n] - (halfdiffdn * randoff);
					if(pos[n] < 0.0)
						pos[n] += 360.0;
				}
			} else {									//	If only 2 infiles, nothing has changed, so
				n  = 0;									//	Randomise first outer position
				up = 1;									//	R--X
				dn = dz->infilecnt - 1;
				diffup = sort[up] - sort[n];
				if(diffup < 0)
					diffup += 360;
				diffdn = 360.0 - diffup;
				halfdiffup = diffup/2.0;
				halfdiffdn = diffdn/2.0;
				randoff = (drand48() * 2.0) - 1.0;
				randoff *= dz->param[PANO_RAND];
				if(randoff > 0.0) {
					pos[n] = sort[n] + (halfdiffup * randoff);
					if(pos[n] >= 360.0)
						pos[n] -= 360.0;
				} else {
					pos[n] = sort[n] - (halfdiffdn * randoff);
					if(pos[n] < 0.0)
						pos[n] += 360.0;
				}										//	Randomise last outer position
				n = 1;									//	r--R
				temp = halfdiffup;
				halfdiffup = halfdiffdn;
				halfdiffdn = temp;
				randoff = (drand48() * 2.0) - 1.0;
				randoff *= dz->param[PANO_RAND];
				if(randoff > 0.0) {
					pos[n] = sort[n] + (halfdiffup * randoff);
					if(pos[n] >= 360.0)
						pos[n] -= 360.0;
				} else {
					pos[n] = sort[n] - (halfdiffdn * randoff);
					if(pos[n] < 0.0)
						pos[n] += 360.0;
				}
			}
		}
		for(n = 0;n <dz->infilecnt-1;n++) {	//	Resort sound positions into angular ascending order from 0
			for(m=n;m<dz->infilecnt;m++) {
				if(pos[n] > pos[m]) {
					temp = pos[n];
					pos[n]  = pos[m];
					pos[m] = temp; 
				}
			}
		}
	}
	for(n = 0;n <dz->infilecnt;n++) {	//	sort into leftmost to rightmost order (for assigning infiles)
		if(pos[n] > 180) {
			m = n;
			for(k = 0;m < dz->infilecnt;m++,k++)
				sort[k] = pos[m]; 
			for(m = 0;k < dz->infilecnt;m++,k++)
				sort[k] = pos[m]; 
			for(m=0;m<dz->infilecnt;m++)
				pos[m] = sort[m];
			break;
		}
	}
	return FINISHED;
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	if(!strcmp(prog_identifier_from_cmdline,"panorama"))	dz->process = PANORAMA;
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
	if(!strcmp(str,"panorama")) {
		fprintf(stdout,
	    "USAGE: panorama panorama 1 infile infile2 [infile3.....] outmixfile\n"
		"           lspk_cnt lspk_aw sounds_aw sounds_ao config [-rrand] [-p] [-q]\n"
	    "OR:    panorama panorama 2 infile infile2 [infile3.....] outmixfile\n"
		"           lspk_positions   sounds_aw sounds_ao config [-rrand] [-p] [-q]\n"
		"\n"
		"Distribute (>1) mono files in spatial panorama, in multichannel mixfile.\n"
		"Lspkrs assumed to surround listening area from front outwards.\n"
		"Input sounds distributed in order from leftmost to rightmost position.\n"
		"\n"
		"LSPK_CNT  Number of lspkrs (mode 1 equally spaced: not ness so in mode 2).\n"
		"LSPK_AW   Angular width of loudspeaker array (190 - 360), front centre at 0.\n"
		"          Loudspeaker array assumed symmetrical around a centre-line\n"
		"          running through front-centre of lspkr array and centre of auditorium.\n"
		"          Thus, if front centre is at 0 then \n" 
		"          190 spread is from -95 to (+)95; 360 spread is from -180 to (+)180.\n"
		"\n"
		"SOUNDS_AW angular width of output sounds (equal to or less than LSPK_AW).\n"
		"\n"
		"SOUNDS_AO angular offset of output snds. (Only possible if SOUNDS_AW < LSPK_AW)\n"
		"          (angle between centre-line of snds and centre-line of lspkrs)\n"
		"\n"
		"CONFIG   Distribution of output sounds within output angle\n"
		"         config = 1: sounds equally spaced\n"
		"         config = 2: 2 sounds equally spaced, followed by gap, etc.\n"
		"         config = 3: 3 sounds equally spaced, followed by gap\n"
		"         and so on. CONFIG must be a divisor of the number of input sounds.\n"
		"\n"
		"RAND     Randomisation of sound positions (0-1).\n"
		"\n"
		"LSPK_POSITIONS (Mode 2) Textfile list of angular positions of (3-16) lspkrs.\n"
		"          Positions specified by +ve values from 0 (front centre) clockwise, so\n"
		"          vals to right of centre lie between 0 (front)and 180 (rear)\n"
		"          vals to left of centre lie between >180 (rear) and 360(=0) (front)\n"
		"\n"
		"-p       If angular width of lspkrs (LSPK_AW) < 360 :\n"
		"         Odd number of lspkrs gives 1 lspkr at centre front.\n"
		"         Even number of lspkrs gives pair of lspkrs centred on front.\n"
		"         If LSP_AW = 360, speaker orientation is ambiguous.\n"
		"         Default is 1 lspkr at centre front.\n"
		"         \"-p\" flag give PAIR of lspkrs at front (IGNORED if LSP_AW < 360 )\n"
		"-q       Same logic for SOUND positions ( ignored if SOUNDS_AW < 360 )\n"
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

/******************************** PANORAMA ********************************/

int panorama(dataptr dz)
{
	double *lspkrpos = dz->parray[0], *pos = dz->parray[1];
	int chans = dz->infile->channels, gotext = 0;
	double thispos, loangle, hiangle = 0.0, lspkr_angle, left_lspkr_offset, stereopos, leftgain, rightgain, lspkrposlo, lspkrposhi;
	int n, lo, hi;
	char temp[4000], temp2[200], *p;

	sprintf(temp,"%i\n",dz->itemcnt);
	if(fputs(temp,dz->fp) < 0) {
		sprintf(errstr,"Error writing to output data file\n");
		return(PROGRAM_ERROR);
	}
	for(n=0; n< dz->infilecnt; n++) {
		loangle = -1;
		thispos = pos[n];
		lo = 0;
		hi = 1;
		while(lo < dz->itemcnt) {
			lspkrposlo = lspkrpos[lo];
			lspkrposhi = lspkrpos[hi];
			if(lspkrposhi < lspkrposlo) {
				if((thispos >= lspkrposlo && thispos <= 360.0)
				|| (thispos >= 0.0 && thispos < lspkrposhi)) {
					loangle = lspkrpos[lo];
					hiangle = lspkrpos[hi] + 360;
					break;
				}
			} else if (thispos >= lspkrposlo && thispos < lspkrposhi) {
				loangle = lspkrpos[lo];
				hiangle = lspkrpos[hi];
				break;
			}
			if(++hi >= dz->itemcnt)
				hi = 0;
			lo++;
		}
		if(loangle < 0.0) {
			sprintf(errstr,"Error locating lspkr pair\n");
			return(PROGRAM_ERROR);
		}
		lspkr_angle = hiangle - loangle;
		if(lspkrpos[lo] < thispos)
			left_lspkr_offset =  thispos - lspkrpos[lo];
		else
			left_lspkr_offset =  thispos - (lspkrpos[lo] - 360.0);
		if(flteq(left_lspkr_offset,360.0))
			left_lspkr_offset = 0.0;
		stereopos = left_lspkr_offset/lspkr_angle;		//	Range 0 - 1
		stereopos  = (stereopos * 2.0) - 1.0;			//	Range -1 to +1

		pancalc(stereopos,&leftgain,&rightgain);
		lo++;		//	Change from data-numbering (0 to N-1) to channel-routing numbering (1 to N)
		hi++;
		sprintf(temp2,"%s",dz->wordstor[n]);
		p = temp2;
		gotext = 0;
		while(*p != ENDOFSTR) {
			if(*p == '.') {
				gotext = 1;
				break;
			}
			p++;
		}
		if(!gotext)
			strcat(temp2,".wav");
		strcpy(temp,temp2);
		strcat(temp," 0.0 ");
		sprintf(temp2,"%i",chans);
		strcat(temp,temp2);
		if(!flteq(leftgain,0.0)) {
			sprintf(temp2," 1:%i",lo);
			strcat(temp,temp2);
			strcat(temp," ");
			sprintf(temp2,"%lf",leftgain);
			strcat(temp,temp2);
		}
		if(!flteq(rightgain,0.0)) {
			sprintf(temp2," 1:%i",hi);
			strcat(temp,temp2);
			strcat(temp," ");
			sprintf(temp2,"%lf",rightgain);
			strcat(temp,temp2);
		}
		strcat(temp,"\n");
		if(fputs(temp,dz->fp) < 0) {
			sprintf(errstr,"Error writing to output data file\n");
			return(PROGRAM_ERROR);
		}
	}
	return FINISHED;
}

/**************************** HANDLE_THE_SPECIAL_DATA ****************************/

int handle_the_special_data(char *str,dataptr dz)
{		
	int n, m, k;
	double *lspkrpos, *sort;
	double dummy = 0.0, leftangle, rightangle, thispos, tempval;
	FILE *fp;
	int cnt;
	char temp[200], *p;
	if((dz->parray = (double **)malloc(3 * sizeof(double *)))==NULL) {			//	Arrays fpr lspkr positions, sound positions and sorting
		sprintf(errstr,"INSUFFICIENT MEMORY for position storage arrays.\n");
		return(MEMORY_ERROR);
	}
	if((fp = fopen(str,"r"))==NULL) {
		sprintf(errstr,"Cannot open file %s to read loudspeaker locations.\n",str);
		return(DATA_ERROR);
	}
	cnt = 0;
	while(fgets(temp,200,fp)!=NULL) {
		p = temp;
		if(*p == ';')	//	Allow comments in file
			continue;

		while(get_float_from_within_string(&p,&dummy)) {
			if(dummy < 0.0 || dummy > 360) {
				sprintf(errstr,"Angular position (%lf) of loudspeaker %d out of range (0-360).\n",dummy,cnt+1);
				return(DATA_ERROR);
			}
			if(flteq(dummy,360.0))
				dummy = 0.0;
			cnt++;
		}
	}
	if(cnt > 16 || cnt < 3) {
		sprintf(errstr,"Process only works with 3 or more (and no more than 16) loudspeakers. You have entered %d\n",cnt);
		return(DATA_ERROR);
	}
	dz->itemcnt = cnt;
	if((dz->parray[0] = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL) {	//	Array for loudspeaker positions
		sprintf(errstr,"INSUFFICIENT MEMORY to store loudspeaker positions.\n");
		return(MEMORY_ERROR);												
	}																				//	Array for sound positions
	if((dz->parray[1] = (double *)malloc(dz->infilecnt * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store sound positions.\n");
		return(MEMORY_ERROR);
	}																				//	Array for sorting lspkr AND snd positions
	if((dz->parray[2] = (double *)malloc(max(dz->infilecnt,dz->itemcnt) * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store loudspeaker positions sort.\n");
		return(MEMORY_ERROR);
	}
	lspkrpos  = dz->parray[0];
	sort = dz->parray[2];
	fseek(fp,0,0);
	cnt = 0;
	while(fgets(temp,200,fp)!=NULL) {
		p = temp;
		if(*p == ';')	//	Allow comments in file
			continue;

		while(get_float_from_within_string(&p,&dummy)) {
			if(cnt > 0) {
				for(n = 0; n < cnt; n++) {
					if(fabs(lspkrpos[n] - dummy) < 1.0) {
						sprintf(errstr,"Loudspeaker positions at %f and %f are unrealisatically close\n",lspkrpos[n],dummy);
						return(DATA_ERROR);
					}
				}
			}
			lspkrpos[cnt] = dummy;
			cnt++;
		}
	}
	if(fclose(fp)<0) {
		fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",str);
		fflush(stdout);
	}
	leftangle  = 0.0;
	rightangle = 0.0;
	for(n = 0;n <cnt;n++) {				//	sort into angular ascending order
		if(lspkrpos[n] > 180) {
			thispos = 360 - lspkrpos[n];
			if(thispos > leftangle)
				leftangle = thispos;
		} else if(lspkrpos[n] > rightangle)
			rightangle = lspkrpos[n];
	}
	if(leftangle + rightangle <= 180) {
		sprintf(errstr,"Loudspeakers do not encircle the listening area.\n");
		return(DATA_ERROR);
	}
	for(n = 0;n <cnt-1;n++) {			//	sort into angular ascending order
		for(m=n;m<cnt;m++) {
			if(lspkrpos[n] > lspkrpos[m]) {
				tempval = lspkrpos[n];
				lspkrpos[n]  = lspkrpos[m];
				lspkrpos[m] = tempval; 
			}
		}
	}
	sort = dz->parray[2];
	for(n = 0;n <cnt;n++) {			//	sort into leftmost to rightmost order
		if(lspkrpos[n] > 180) {
			m = n;
			for(k = 0;m < cnt;m++,k++)
				sort[k] = lspkrpos[m]; 
			for(m = 0;k < cnt;m++,k++)
				sort[k] = lspkrpos[m]; 
			for(m=0;m<cnt;m++)
				lspkrpos[m] = sort[m];
			break;
		}
	}
	return(FINISHED);
}

/****************************** GET_THE_MODE_FROM_CMDLINE *********************************/

int get_the_mode_from_cmdline(char *str, dataptr dz)
{
	if(sscanf(str,"%d",&dz->mode)!=1) {
		sprintf(errstr,"Cannot read mode of program.\n");
		fprintf(stdout,"INFO: %s\n",errstr);
		fflush(stdout);
		return(USAGE_ONLY);
	}
	if(dz->mode <= 0 || dz->mode > dz->maxmode) {
		sprintf(errstr,"Program mode value [%d] is out of range [1 - %d].\n",dz->mode,dz->maxmode);
		fprintf(stdout,"INFO: %s\n",errstr);
		fflush(stdout);
		return(USAGE_ONLY);
	}
	dz->mode--;		/* CHANGE TO INTERNAL REPRESENTATION OF MODE NO */
	return(FINISHED);
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
