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

#define	ISO_WINLEN 10 //	envelope search-window 10 mS

#ifdef unix
#define round(x) lround((x))
#endif

char errstr[2400];

int anal_infiles = 1;
int	sloom = 0;
int sloombatch = 0;

const char* cdp_version = "7.1.0";

//CDP LIB REPLACEMENTS
static int setup_isolate_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_isolate_param_ranges_and_defaults(dataptr dz);
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
static int get_the_mode_no(char *str,dataptr dz);
static int setup_and_init_input_brktable_constants(dataptr dz,int brkcnt);
static int handle_the_special_data(char *str,dataptr dz);
static int check_the_param_validity_and_consistency(dataptr dz);

static int isolate(dataptr dz);
static int do_a_cut(int *cuts,int n,double spliceincr,dataptr dz);
static int eliminate_abutted_cuts(int *cuts,dataptr dz);
static int do_remnant_cut(int fileno,int *cuts,double spliceincr,dataptr dz);
static int do_a_multicut(int n,double spliceincr,dataptr dz);
static int convert_brkvals_db_to_gain(int paramno,dataptr dz);
static int isolate_param_preprocess(dataptr dz);
static double convert_gain_to_dB_from_minus96(double val);
static int reverse_cuts(int *array,int itemcnt,dataptr dz);
static int reverse_sndread(float *buf,int n, dataptr dz);

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
		if((get_the_process_no(argv[0],dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		cmdline++;
		cmdlinecnt--;
		dz->maxmode = 5;
		if((get_the_mode_no(cmdline[0],dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		cmdline++;
		cmdlinecnt--;
		// setup_particular_application =
		if((exit_status = setup_isolate_application(dz))<0) {
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
	if((exit_status = setup_isolate_param_ranges_and_defaults(dz))<0) {
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
//	handle_special_data()		
	if(dz->mode != ISO_THRESH) {
		if((exit_status = handle_the_special_data(cmdline[0],dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		cmdlinecnt--;
		cmdline++;
	}
	if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {		// CDP LIB
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
//	check_param_validity_and_consistency....
	if((exit_status = check_the_param_validity_and_consistency(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
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

	if((exit_status = create_sndbufs(dz))<0) {							// CDP LIB
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	//param_preprocess()					
	if(dz->mode == ISO_THRESH) {
		if((exit_status = isolate_param_preprocess(dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
	}
	//spec_process_file =
	if((exit_status = open_the_outfile(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = isolate(dz))<0) {
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
	int n;
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
	if(sloom) {						//	IF sloom, drop trailing zero
		n = strlen(dz->outfilename);
		n--;
		dz->outfilename[n] = ENDOFSTR;
	}
	(*cmdline)++;
	(*cmdlinecnt)--;
	return(FINISHED);
}

/************************ OPEN_THE_OUTFILE *********************/

int open_the_outfile(dataptr dz)
{
	int exit_status;
	char filename[200], temp[8];
	strcpy(filename,dz->outfilename);
	switch(dz->mode) {
	case(ISO_SEGMNT):
	case(ISO_SLICED):
	case(ISO_OVRLAP):
		if(dz->vflag[1]) {
			sprintf(temp,"%d",(dz->itemcnt/2) - 1);
			strcat(filename,temp);
		} else
			strcat(filename,"0");
		break;
	default:
		strcat(filename,"0");
		break;
	}
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

/************************* SETUP_ISOLATE_APPLICATION *******************/

int setup_isolate_application(dataptr dz)
{
	int exit_status;
	aplptr ap;
	if((exit_status = establish_application(dz))<0)		// GLOBAL
		return(FAILED);
	ap = dz->application;
	// SEE parstruct FOR EXPLANATION of next 2 functions
	switch(dz->mode) {
	case(ISO_SEGMNT):
		if((exit_status = set_param_data(ap,ISOLATES,2,0,"00"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"s",1,"d","xr",2,0,"00"))<0)
			return(FAILED);
		break;
	case(ISO_GROUPS):
		if((exit_status = set_param_data(ap,ISOGROUPS,2,0,"00"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"s",1,"d","xr",2,0,"00"))<0)
			return(FAILED);
		break;
	case(ISO_SLICED):
		if((exit_status = set_param_data(ap,ISOSLICES,2,0,"00"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"s",1,"d","xr",2,0,"00"))<0)
			return(FAILED);
		break;
	case(ISO_OVRLAP):
		if((exit_status = set_param_data(ap,ISOSYLLS,2,0,"00"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"sd",2,"dd","xr",2,0,"00"))<0)
			return(FAILED);
		break;
	case(ISO_THRESH):
		if((exit_status = set_param_data(ap,0      ,2,2,"DD"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"sml",3,"ddd","xr",2,0,"00"))<0)
			return(FAILED);
		break;
	}
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

/************************* SETUP_ISOLATE_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_isolate_param_ranges_and_defaults(dataptr dz)
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
	case(ISO_OVRLAP):
		ap->lo[ISO_SPL]	= 0;
		ap->hi[ISO_SPL]	= 500;
		ap->default_val[ISO_SPL] = 15;
		ap->lo[ISO_DOV]	= 0;
		ap->hi[ISO_DOV]	= 20;
		ap->default_val[ISO_DOV] = 5;
		break;
	case(ISO_THRESH):
		ap->lo[ISO_THRON]	= -60;
		ap->hi[ISO_THRON]	= 0;
		ap->default_val[ISO_THRON] = 0;
		ap->lo[ISO_THROFF]	= -96;
		ap->hi[ISO_THROFF]	= 0;
		ap->default_val[ISO_THROFF] = -96;
		ap->lo[ISO_SPL]	= 0;
		ap->hi[ISO_SPL]	= 500;
		ap->default_val[ISO_SPL] = 15;
		ap->lo[ISO_MIN]	= 20;
		ap->hi[ISO_MIN]	= 500;
		ap->default_val[ISO_MIN] = 50;
		ap->lo[ISO_LEN]	= 0;
		ap->hi[ISO_LEN]	= 500;
		ap->default_val[ISO_LEN] = 0;
		break;
	default:
		ap->lo[ISO_SPL]	= 0;
		ap->hi[ISO_SPL]	= 500;
		ap->default_val[ISO_SPL] = 15;
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
			if((exit_status = setup_isolate_application(dz))<0)
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
	usage2("isolate");
	return(USAGE_ONLY);
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	if(!strcmp(prog_identifier_from_cmdline,"isolate"))				dz->process = ISOLATE;
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
	if(!strcmp(str,"isolate")) {
		fprintf(stdout,
	    "USAGE:\n"
		"isolate isolate 1-2 inf outnam cutsfile [-ssplice] [-x] [-r]\n"
		"isolate isolate 3   inf outnam dBon dBoff [-ssplice] [-mmin] [-llen] [-x] [-r]\n"
		"isolate isolate 4   inf outnam slicefile [-ssplice] [-x] [-r]\n"
	    "isolate isolate 5   inf outnam slicefile [-ssplice] [-ddovetail] [-x] [-r]\n"
		"\n"
		"Cut specified segments from input file, but retain silent surrounds\n"
		"so chunks occur at same time in the outfiles, as in the input file.\n"
	    "Modes 1, 2 & 3 also generate file of all material left over after the cutting.\n"
	    "Original file reconstructible by mixing all these components.\n"
		"INF       Input soundfile\n"
		"OUTNAM    Generic outfile name. Outfiles numbered from 0\n"
		"CUTSFILE/SLICEFILE  \n"
		"Mode 1    Creates SEVERAL output files each containing ONE segment of src. \n"
		"          Cutsfile is list of time-pairs,(start and end of each segment)\n"
		"          in time order, and not time-overlapped.\n"
		"Mode 2    Can create output files each containing SEVERAL segment of src.\n"
		"          If a cutsfile line lists several time-pairs (start+end of segs),\n"
		"          ALL segments on that line will be output in a single file.\n"
		"          No time-pairs must overlap. Time-pairs on a line must be in time order.\n"
		"Mode 3    Creates ONE output file consisting of SEVERAL disjunct segments.\n"
		"          Segment starts and ends located using threshold-on and -off parameters.\n"
		"          if \"len\" set, only start portion of segment (length \"len\") is kept.\n"
		"\n"
		"          In modes 1-3, an EXTRA FILE OF REMNANTS (if any) is created.\n"
		"          Also, if cuts abutt or are so close that end+start splices overlap,\n"
		"          end of first cut moved back, and start of 2nd moved forward\n"
		"          so that they overlap by a single splicelength.\n"
		"\n"
		"Mode 4    Cuts the ENTIRE file into disjunct segments\n"
		"          Creating SEVERAL output files each containing ONE segment of src.\n"
		"          Slicefile is list of (increasing) times where sound is to be cut.\n"
		"Mode 5    As mode 4: segments overlapped slightly (separates speech syllables).\n"
		"\n"
		"SPLICE    Length of splice in mS (range 0 - 500 default 15)\n"
		"DBON      (Mode 3) dB level at which a segment is recognised to begin.\n"
		"DBOFF     (Mode 3) dB level where recognised seg triggered to end (less than DBON).\n"
		"MIN       (Mode 3) min duration in mS of segs to accept (> 2 * splicelen)\n"
		"LEN       (Mode 3) duration in mS of (part-)segment to actually keep (> 0)\n"
		"          If \"len\" not set, or set to zero, entire original segments are kept.\n"
		"DOVETAIL  (Mode 5) overlap of cut segments in mS (range 0 -20 default 5)\n"
		"-x        Add silence to ends of segments files, so they are same length as src\n"
		"-r        Reverse all cut-segment files (but not the remnant file)\n"
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

/******************************** ISOLATE *********************************/
 
int isolate(dataptr dz)
{
	int exit_status = CONTINUE, itemincr;
	int *cuts = dz->lparray[0];
	int n = 0, k, fileno, filefirst;
	double spliceincr = 1.0/(double)dz->iparam[ISO_SPL];
	char filename[200], thisnum[8];
	switch(dz->mode) {
	case(ISO_SEGMNT):
	case(ISO_SLICED):
	case(ISO_OVRLAP):
		if(dz->vflag[1])
			filefirst = (dz->itemcnt/2) - 1;
		else
			filefirst = 0;
		break;
	default:
		filefirst = 0;
		break;
	}
	fileno = filefirst;
	fprintf(stdout,"INFO: Cutting segments\n");
	fflush(stdout);
	while(n < dz->itemcnt) {
		if(fileno != filefirst) {
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
		} else if(dz->vflag[1]) {	//	If output cutfiles are to be time-reversed, reverse the cut-arrays
			switch(dz->mode) {
			case(ISO_GROUPS):
			case(ISO_THRESH):	
				for(k=0;k<dz->itemcnt;k++) {
					if((exit_status = reverse_cuts(dz->lparray[k],dz->iparray[0][k],dz))<0)
						return(exit_status);
				}
				break;
			default:
				if((exit_status = reverse_cuts(dz->lparray[0],dz->itemcnt,dz))<0)
					return(exit_status);
				break;
			}
		}
		switch(dz->mode) {
		case(ISO_GROUPS):
		case(ISO_THRESH):
			if((exit_status = do_a_multicut(n,spliceincr,dz))< 0)
				return exit_status;
			itemincr = 1;		//	Moves line by line up cut values
			break;
		default:
			if((exit_status = do_a_cut(cuts,n,spliceincr,dz))< 0)
				return exit_status;
			itemincr = 2;		//	Moves pair by pair up cut values
			break;
		}
		sndseekEx(dz->ifd[0],0,0);
		reset_filedata_counters(dz);
		switch(dz->mode) {
		case(ISO_SEGMNT):
			if(dz->vflag[1]) {
				if(fileno > 0)
					fileno--;
				else
					fileno = dz->itemcnt/2;
			} else
				fileno++;
			break;
		case(ISO_SLICED):
		case(ISO_OVRLAP):
			if(dz->vflag[1])
				fileno--;
			else
				fileno++;
			break;
		default:
			fileno++;
			break;
		}
		n += itemincr;
		if(exit_status == FINISHED)
			break;
	}
	if(sndcloseEx(dz->ofd) < 0) {
		sprintf(errstr,"Failed to close output file %d\n",fileno+1);
		return(SYSTEM_ERROR);
	}
	dz->ofd = -1;
	switch(dz->mode ) {
	case(ISO_GROUPS):						//	As the array of ALL the cuts is never reversed, no need to re-reverse here.
		cuts = dz->lparray[dz->itemcnt];	//	This is the array with all the cuts sorted into order
		dz->itemcnt = dz->specenvcnt;		//	itemcnt was the the cnt of arrays: replace it with the total count of cuts
		return do_remnant_cut(fileno,cuts,spliceincr,dz);
		break;
	case(ISO_THRESH):	
		if(dz->vflag[1]) {					//	As array of cuts was reversed, need to re-reverse here
			if((exit_status = reverse_cuts(dz->lparray[0],dz->specenvcnt,dz))<0)
				return(exit_status);
		}
		cuts = dz->lparray[0];				//	Do remnant cut on the initial (only) array
		dz->itemcnt = dz->specenvcnt;		//	itemcnt was the the cnt of arrays (1): replace it with the count of cuts
		return do_remnant_cut(fileno,cuts,spliceincr,dz);
		break;
	case(ISO_SEGMNT):	
		if(dz->vflag[1]) {					//	As array of cuts was reversed, need to re-reverse here
			if((exit_status = reverse_cuts(dz->lparray[0],dz->itemcnt,dz))<0)
				return(exit_status);
		}
		return do_remnant_cut(fileno,cuts,spliceincr,dz);
		break;
	}
	return FINISHED;
}

/******************************** CHECK_THE_PARAM_VALIDITY_AND_CONSISTENCY *********************************
 *
 *	If 2 cuts abutt, or their end-splice and start-splice overlap
 *	the cut times are moved to be a while splicelen apart.  
 */

int check_the_param_validity_and_consistency(dataptr dz)
{
	int exit_status;
	int *cuts, *allcuts, gap, halfgap, temp1, temp2, origcutstt, origcutend;
	int *linecnts, gotit = 0;
	int chans = dz->infile->channels, halfsplen, dblsplen, dblsplcnt, n, m, j, cnt, totalcnt;
	double srate = (double)dz->infile->srate;
	
	dz->param[ISO_SPL] *= MS_TO_SECS;
	dz->iparam[ISO_SPL] = (int)round(dz->param[ISO_SPL] * srate);
	dz->iparam[ISO_SPL] = (dz->iparam[ISO_SPL] / 2) * 2;	//	splicelen must be divisible by 2, for seg separation algo to work
	if(dz->mode == ISO_THRESH) {
		dz->param[ISO_LEN] *= MS_TO_SECS;
		dz->iparam[ISO_LEN] = (int)round(dz->param[ISO_LEN] * srate) * chans;
		dz->param[ISO_MIN] *= MS_TO_SECS;
		dz->iparam[ISO_MIN] = (int)round(dz->param[ISO_MIN] * srate) * chans;
		if(dz->iparam[ISO_MIN] <= dz->iparam[ISO_SPL] * chans * 2) {
			sprintf(errstr,"Minimum seglen (%lf) must be > 2 * splicelen (%lf)\n",dz->param[ISO_MIN],2 * dz->param[ISO_SPL]);
			return(DATA_ERROR);
		}
		if(!(dz->brksize[ISO_THRON] || dz->brksize[ISO_THROFF])) {
			if(dz->param[ISO_THRON] <= dz->param[ISO_THROFF]) {
				sprintf(errstr,"On (%lf) and Off (%lf) thresholds are incompatible\n",dz->param[ISO_THRON],dz->param[ISO_THROFF]);
				return(DATA_ERROR);
			}
		}
		if(dz->brksize[ISO_THRON])
			exit_status = convert_brkvals_db_to_gain(ISO_THRON,dz);
		else
			exit_status = convert_dB_at_or_below_zero_to_gain(&(dz->param[ISO_THRON]));
		if(exit_status < 0)
			return(exit_status);

		if(dz->brksize[ISO_THROFF])
			exit_status = convert_brkvals_db_to_gain(ISO_THROFF,dz);
		else
			exit_status = convert_dB_at_or_below_zero_to_gain(&(dz->param[ISO_THROFF]));
		return exit_status;
	}
	cuts = dz->lparray[0];
	halfsplen = dz->iparam[ISO_SPL]/2;						//	half number of sample-groups in splice
	dblsplen  = dz->iparam[ISO_SPL] * 2;					//	length of two splices in sample-groups
	dblsplcnt = dblsplen * chans;							//	length of two splices in samples
	dz->iparam[ISO_SPL] = dz->iparam[ISO_SPL];
	switch(dz->mode) {
	case(ISO_SEGMNT):
		for(n=2;n < dz->itemcnt;n+=2) {
			gap = (cuts[n] - cuts[n-1])/chans;
			if(gap < dblsplen) {								//	If cuts abutt, or their splices overlap
				halfgap = gap/2;								//	Move them to the same position
				cuts[n-1] += halfgap * chans;					//	midway between end of 1st and start of 2nd
				cuts[n-1] -= halfsplen * chans;					//	Then move them apart, the 1st halfsplcelen back
				cuts[n]   -= halfgap * chans;					//	The 2nd halfsplicelen forward
				cuts[n]   += halfsplen * chans;					//	If the gap was an odd number of samples, adjust 2nd by 1 sample-group
				if(ODD(gap))
					cuts[n] -= chans;
				if(cuts[n-1] - cuts[n-2] < 0) {					//	and check if either of shortened cuts is now reduced to <= zero
					sprintf(errstr,"Adjusting position of abutted cuts %d and %d makes 1st cut disappear\n",(n/2 - 1),n/2);
					return(DATA_ERROR);
				}
				if(cuts[n+1] - cuts[n] < 0) {
					sprintf(errstr,"Adjusting position of abutted cuts %d and %d makes 2nd cut disappear\n",(n/2 - 1),n/2);
					return(DATA_ERROR);
				}
			}
		}
		if(cuts[dz->itemcnt-1] > dz->insams[0])					//	Ensure last cut is before end or at end
			cuts[dz->itemcnt-1] = dz->insams[0];
		break;
	case(ISO_GROUPS):
		linecnts = dz->iparray[0];
		for(m=0;m < dz->itemcnt;m++) {
			cuts = dz->lparray[m];
			for(n=2;n < linecnts[m];n+=2) {
				gap = (cuts[n] - cuts[n-1])/chans;
				if(gap < dblsplen) {				//	If cuts abutt, or their splices overlap, error
					sprintf(errstr,"Segments %d and %d on line %d overlap\n",(n/2),(n/2)+1,m+1);
					return DATA_ERROR;
				}
			}
			if(cuts[linecnts[m]-1] > dz->insams[0])	//	Ensure last cut is before end or at end
				cuts[linecnts[m]-1] = dz->insams[0];
		}
		allcuts = dz->lparray[dz->itemcnt];			//	Copy all cuts to same array
		totalcnt = 0;
		for(m=0;m < dz->itemcnt;m++) {
			cuts = dz->lparray[m];
			cnt = linecnts[m];
			memcpy((char *)(allcuts + totalcnt),(char *)cuts,cnt * sizeof(int));
			totalcnt += cnt;
		}
		dz->specenvcnt = totalcnt;
		for(n=0;n < totalcnt-2;n+=2) {				//	Sort into ascending order
			for(m=n+2;m<totalcnt;m+=2) {
				if(allcuts[m] < allcuts[n]) {
					temp1 = allcuts[n];
					temp2 = allcuts[n+1];
					allcuts[n]   = allcuts[m];
					allcuts[n+1] = allcuts[m+1];
					allcuts[m]   = temp1;
					allcuts[m+1] = temp2;
				}
			}
		}
		for(n=2;n < totalcnt;n+=2) {				//	Ensure cuts in different files do NOT overlap
			gap = (allcuts[n] - allcuts[n-1])/chans;
			if(gap < 0) {
				sprintf(errstr,"Two of segments in different outfiles overlap one another.\n");
				return(DATA_ERROR);
			}
			if(gap < dblsplen) {						
				origcutstt = allcuts[n];
				origcutend = allcuts[n-1];
				halfgap = gap/2;								//	Move them to the same position
				allcuts[n-1] += halfgap * chans;				//	midway between end of 1st and start of 2nd
				allcuts[n-1] -= halfsplen * chans;				//	Then move them apart, the 1st halfsplcelen back
				allcuts[n]   -= halfgap * chans;				//	The 2nd halfsplicelen forward
				allcuts[n]   += halfsplen * chans;				//	If the gap was an odd number of samples, adjust 2nd by 1 sample-group
				if(ODD(gap))
					allcuts[n] -= chans;
				if(allcuts[n-1] - allcuts[n-2] < 0) {			//	and check if either of shortened cuts is shortened to < zero
					sprintf(errstr,"Adjusting position of abutted cuts makes 1st cut disappear\n");
					return(DATA_ERROR);
				}
				if(allcuts[n+1] - allcuts[n] < 0) {
					sprintf(errstr,"Adjusting position of abutted makes 2nd cut disappear\n");
					return(DATA_ERROR);
				}
				gotit = 0;
				for(m = 0;m < dz->itemcnt;m++) {				//	Find cuts in original arrays and move them!!
					cuts = dz->lparray[m];
					for(j=0;j < linecnts[m];j+=2) {
						if(cuts[j] == origcutstt) {
							cuts[j] = allcuts[n];
							gotit = 1;
							break;
						}
					}
					if(gotit)
						break;
				}
				gotit = 0;
				for(m = 0;m < dz->itemcnt;m++) {
					cuts = dz->lparray[m];
					for(j=1;j < linecnts[m];j+=2) {
						if(cuts[j] == origcutend) {
							cuts[j] = allcuts[n-1];
							gotit = 1;
							break;
						}
					}
					if(gotit)
						break;
				}
			}
		}
		break;
	case(ISO_SLICED):
		if(cuts[0] != 0) {							//	If no sliuce at zero time
			for(n=dz->itemcnt;n > 0;n--)			//	Force a slice
				cuts[n] = cuts[n-1];				//	By shuffling values up	
			cuts[0] = 0;							//	and inserting 0 value at start
			dz->itemcnt++;
		}
		if(cuts[dz->itemcnt-1] >= dz->insams[0])	//	Ensure last cut is before end or at end
			dz->itemcnt--;
		m = (dz->itemcnt * 2) - 1;
		cuts[m--] = dz->insams[0];					//	Put end of last seg at end of file
		for(n=dz->itemcnt-1;n>0;n--) {				
			cuts[m--] = cuts[n];					//	Copy previous slice time to seg-start
			cuts[m--] = cuts[n];					//	Copy same slice time as previous segend
		}											//	cuts[0] = 0, stays where it is	
		dz->itemcnt *= 2;
		for(n=2;n < dz->itemcnt;n+=2) {				//	Dis-abutt, as above
			if(dz->iparam[ISO_SPL] == 0) {
				if(cuts[n-1] - cuts[n-2] == 0) {
					sprintf(errstr,"With zero splice, abutted cuts %d and %d produce a zero length segment\n",(n/2)-1,n/2);
					return(DATA_ERROR);
				}
				if(cuts[n+1] - cuts[n] == 0) {
					sprintf(errstr,"With zero splice, abutted cuts %d and %d produce a zero length segment\n",(n/2),n/2+1);
					return(DATA_ERROR);
				}
			} else {
				cuts[n-1] -= halfsplen * chans;
				cuts[n]   += halfsplen * chans;
				if(cuts[n-1] - cuts[n-2] < 0) {
					sprintf(errstr,"Adjusting position of abutted cuts %d and %d makes 1st shrink to zero\n",(n/2)-1,n/2);
					return(DATA_ERROR);
				}
				if(cuts[n+1] - cuts[n] < 0) {
					sprintf(errstr,"Adjusting position of abutted cuts %d and %d makes 2nd shrink to zero\n",n/2,(n/2)+1);
					return(DATA_ERROR);
				}
			}
		}
		break;
	case(ISO_OVRLAP):
		if(cuts[0] != 0) {							//	If no sliuce at zero time
			for(n=dz->itemcnt;n > 0;n--)			//	Force a slice
				cuts[n] = cuts[n-1];				//	By shuffling values up	
			cuts[0] = 0;							//	and inserting 0 value at start
			dz->itemcnt++;
		}
		if(cuts[dz->itemcnt-1] >= dz->insams[0])	//	Remove endcut if its at or before soundend
			dz->itemcnt--;
		m = (dz->itemcnt * 2) - 1;
		cuts[m--] = dz->insams[0];					//	Put end of last seg at end of file
		for(n=dz->itemcnt-1;n>0;n--) {				
			cuts[m--] = cuts[n];					//	Copy previous slice time to seg-start
			cuts[m--] = cuts[n];					//	Copy same slice time as previous segend
		}											//	cuts[0] = 0, stays where it is	
		dz->itemcnt *= 2;							//	For syllable separation, add dovetails
		dz->param[ISO_DOV] *= MS_TO_SECS;
		dz->iparam[ISO_DOV] = (int)round(dz->param[ISO_DOV] * srate * chans);
		for(n=0;n < dz->itemcnt;n+=2) {
			cuts[n]   = max(0,cuts[n] - dz->iparam[ISO_DOV]);
			cuts[n+1] = min(dz->insams[0],cuts[n+1] + dz->iparam[ISO_DOV]);
		}
		break;
	}
	return FINISHED;
}

/**************************** HANDLE_THE_SPECIAL_DATA ****************************/

int handle_the_special_data(char *str,dataptr dz)
{		
	int done, iseven;
	double dummy = 0.0, lasttime = 0.0;
	int *q;
	FILE *fp;
	int cnt, origcnt, totalcnt, linecnt;
	char temp[200], *p;
	if((fp = fopen(str,"r"))==NULL) {
		sprintf(errstr,"Cannot open file %s to read template.\n",str);
		return(DATA_ERROR);
	}
	switch(dz->mode) {
	case(ISO_SEGMNT):
	case(ISO_SLICED):
	case(ISO_OVRLAP):
		cnt = 0;
		done = 0;
		lasttime = -1;
		iseven = 1;
		while(fgets(temp,200,fp)!=NULL) {
			p = temp;
			if(*p == ';')	//	Allow comments in file
				continue;
			while(get_float_from_within_string(&p,&dummy)) {
				if(dummy < 0.0) {
					sprintf(errstr,"Time (%lf)less than zero in cuts file.\n",dummy);
					return(DATA_ERROR);
				} else if(dummy >= dz->duration) {
					if(cnt == 0) {
						sprintf(errstr,"First cut is beyond end of file\n");
						return(DATA_ERROR);
					} else if(iseven) {
						fprintf(stdout,"WARNING: some of cut times are beyond end of input file: ignoring these\n");
						fflush(stdout);
						done = 1;
						break;
					} else {		// Keep any cut extending beyond file end, and break from loop
						iseven = !iseven;
						cnt++;
						done = 1;
						break;
					}
				}
				if(dz->mode == ISO_SEGMNT) {
					if(iseven) {
						if(dummy < lasttime) {
							sprintf(errstr,"End of cut %d (%lf) and start of cut %d (%lf) overlap.\n",cnt/2,lasttime,(cnt/2 + 1),dummy);
							return(DATA_ERROR);
						}
					} else {
						if(dummy <= lasttime) {
							sprintf(errstr,"Cut portion %d (%lf %lf) length less than or equal to zero.\n",(cnt/2 + 1),lasttime,dummy);
							return(DATA_ERROR);
						}
					}
				} else {
					if(dummy <= lasttime) {
						sprintf(errstr,"Times (%lf & %lf) not in ascending order.\n",lasttime,dummy);
						return(DATA_ERROR);
					}
				}
				lasttime = dummy;
				iseven = !iseven;
				cnt++;
			}
			if(done)
				break;
		}
		if(cnt == 0) {
			sprintf(errstr,"No valid times in file.\n");
			return(DATA_ERROR);
		}
		dz->itemcnt = cnt;
		origcnt = cnt;
		if(dz->mode == ISO_SEGMNT) {			//	Cut times must be paired
			if(!iseven) {
				sprintf(errstr,"Invalid time pairing (odd number of times) in file.\n");
				return(DATA_ERROR);
			}
		} else {
			cnt = (cnt + 1) * 2;	//	Each slice time will generate a pair of values, with an extra pair at end
		}
		dz->larray_cnt = 1;
		if((dz->lparray = (int **)malloc(dz->larray_cnt * sizeof(int *)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for internal integer arrays.\n");
			return(MEMORY_ERROR);
		}
		if((dz->lparray[0] = (int *)malloc(cnt * sizeof(int)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to store segment cuts.\n");
			return(MEMORY_ERROR);
		}
		fseek(fp,0,0);
		done = 0;
		cnt = 0;
		q = dz->lparray[0];
		while(fgets(temp,200,fp)!=NULL) {
			p = temp;
			if(*p == ';')	//	Allow comments in file
				continue;
			while(get_float_from_within_string(&p,&dummy)) {
				*q = (int)round(dummy * (double)(dz->infile->srate)) * dz->infile->channels;
				q++;
				if(++cnt >= origcnt) {
					done = 1;
					break;
				}
			}
			if(done)
				break;
		}
		break;
	case(ISO_GROUPS):
		totalcnt = 0;
		linecnt = 0;
		while(fgets(temp,200,fp)!=NULL) {
			p = temp;
			if(*p == ';')	//	Allow comments in file
				continue;
			iseven = 1;
			done = 0;
			cnt = 0;
			lasttime = -1;
			while(get_float_from_within_string(&p,&dummy)) {
				if(dummy < 0.0) {
					sprintf(errstr,"Time (%lf)less than zero in cuts file.\n",dummy);
					return(DATA_ERROR);
				} else if(dummy >= dz->duration) {
					if(cnt == 0) {
						sprintf(errstr,"First cut is beyond end of file\n");
						return(DATA_ERROR);
					} else if(iseven) {
						fprintf(stdout,"WARNING: some of cut times are beyond end of input file: ignoring these\n");
						fflush(stdout);
						done = 1;
						break;
					} else {		// Keep any cut extending beyond file end, and break from loop
						cnt++;
						iseven = !iseven;
						done = 1;
						break;
					}
				}
				if(iseven) {
					if(dummy < lasttime) {
						sprintf(errstr,"End of cut %d (%lf) and start of cut %d (%lf) overlap.\n",cnt/2,lasttime,(cnt/2 + 1),dummy);
						return(DATA_ERROR);
					}
				} else {
					if(dummy <= lasttime) {
						sprintf(errstr,"Cut portion %d (%lf %lf) length less than or equal to zero.\n",(cnt/2 + 1),lasttime,dummy);
						return(DATA_ERROR);
					}
				}
				lasttime = dummy;
				iseven = !iseven;
				cnt++;
			}
			if(cnt > 0) {
				if(!iseven) {
					sprintf(errstr,"Invalid time pairing (odd number of times) on line %d\n",linecnt);
					return(DATA_ERROR);
				}
				totalcnt += cnt;
				linecnt++;
			}
		}
		if(linecnt == 0) {
			sprintf(errstr,"No valid lines found in file\n");
			return(DATA_ERROR);
		}
		dz->itemcnt = linecnt;
		dz->iarray_cnt = 1;
		if((dz->iparray = (int **)malloc(dz->iarray_cnt * sizeof(int *)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for internal integer arrays.\n");
			return(MEMORY_ERROR);
		}
		if((dz->iparray[0] = (int *)malloc(linecnt * sizeof(int)))==NULL) {				//	Store length of each cuts store 
			sprintf(errstr,"INSUFFICIENT MEMORY to store lengths of cut-groups.\n");
			return(MEMORY_ERROR);
		}
		dz->larray_cnt = linecnt+1;
		if((dz->lparray = (int **)malloc(dz->larray_cnt * sizeof(int *)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for internal long arrays.\n");
			return(MEMORY_ERROR);
		}
		if((dz->lparray[linecnt] = (int *)malloc(totalcnt * sizeof(int)))==NULL) {	// This array used later to compare ALL cuts
			sprintf(errstr,"INSUFFICIENT MEMORY so store cut-groups.\n");
			return(MEMORY_ERROR);
		}
		linecnt = 0;
		fseek(fp,0,0);
		while(fgets(temp,200,fp)!=NULL) {
			p = temp;
			if(*p == ';')	//	Allow comments in file
				continue;
			iseven = 1;
			done = 0;
			cnt = 0;
			lasttime = -1;
			while(get_float_from_within_string(&p,&dummy)) {
				if(dummy >= dz->duration) {
					if(iseven) {
						done = 1;
						break;
					} else {		// Keep any cut extending beyond file end, and break from loop
						cnt++;
						iseven = !iseven;
						done = 1;
						break;
					}
				}
				lasttime = dummy;
				iseven = !iseven;
				cnt++;
			}
			if(cnt > 0) {
				dz->iparray[0][linecnt] = cnt;
				if((dz->lparray[linecnt] = (int *)malloc(cnt * sizeof(int)))==NULL) {
					sprintf(errstr,"INSUFFICIENT MEMORY count of line %d.\n",linecnt+1);
					return(MEMORY_ERROR);
				}
				linecnt++;
			}
		}
		fseek(fp,0,0);
		linecnt = 0;
		while(fgets(temp,200,fp)!=NULL) {
			p = temp;
			if(*p == ';')	//	Allow comments in file
				continue;
			iseven = 1;
			done = 0;
			cnt = 0;
			lasttime = -1;
			q = dz->lparray[linecnt];
			while(get_float_from_within_string(&p,&dummy)) {
				if(dummy >= dz->duration) {
					if(iseven) {
						done = 1;
						break;
					} else {		// Keep any cut extending beyond file end, and break from loop
						*q = (int)round(dummy * (double)(dz->infile->srate)) * dz->infile->channels;
						cnt++;
						iseven = !iseven;
						done = 1;
						break;
					}
				}
				*q++ = (int)round(dummy * (double)(dz->infile->srate)) * dz->infile->channels;
				lasttime = dummy;
				iseven = !iseven;
				cnt++;
			}
			if(cnt > 0)
				linecnt++;
		}
		break;
	}
	if(fclose(fp)<0) {
		fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",str);
		fflush(stdout);
	}
	return(FINISHED);
}

/******************************** DO_A_CUT *********************************/
 
int do_a_cut(int *cuts,int n,double spliceincr,dataptr dz) 
{
	int exit_status, do_continue = 1, bufcnt = 0;
	int chans = dz->infile->channels, splicelen = dz->iparam[ISO_SPL], dostartsplice = 0, doendsplice = 0, k, j;
	float *ibuf = dz->sampbuf[0], *obuf = dz->sampbuf[1];
	int thisstart, thisend, tocopy, bufremain, last_total_samps_read, hereinbuf;
	double splicer;

	if(dz->vflag[1]) {
		bufcnt = dz->insams[0]/dz->buflen;
		if(bufcnt * dz->buflen == dz->insams[0])
			bufcnt--;
		if((exit_status = reverse_sndread(ibuf,bufcnt,dz)) < 0) 
			return(exit_status);
			bufcnt--;
	} else if((exit_status = read_samps(ibuf,dz)) < 0) 
		return(exit_status);
	memset((char *)obuf,0,dz->buflen * sizeof(float));
	thisstart   = cuts[n]; 
	if(cuts[n] >= splicelen * chans) {	//	If room to do splice, move to splice start, and flag startsplicing
		thisstart -= (splicelen * chans);
		dostartsplice = 1;
	} else {							//	If no room to do splice
		thisstart = 0;					//	extend cut-portion back to start of file
		dostartsplice = 0;				//	and flag NO startsplice
	}

	thisend = cuts[n+1];				//	If room to do splice, move to splice start, and flag endsplicing
	if(dz->insams[0] - cuts[n+1] >= splicelen * chans)
		doendsplice = 1;
	else {								//	If no room to do splice
		thisend = dz->insams[0];		//	extend (or curtial) end of cut-portion to end of file
		doendsplice = 0;				//	and flag NO endsplice
		do_continue = 0;				//	and flag quit cuttings
	}
	while(thisstart >= dz->total_samps_read) {
		if((exit_status = write_samps(obuf,dz->ssampsread,dz))<0)
			return(exit_status);						//	Write anything that's left in buffer & then
		if(dz->vflag[1]) {								//  zeros in any space before start of cut	
			if((exit_status = reverse_sndread(ibuf,bufcnt,dz)) < 0) 
				return(exit_status);
			bufcnt--;
		} else if((exit_status = read_samps(ibuf,dz)) < 0) 
			return(exit_status);
		if(dz->ssampsread == 0) {
			sprintf(errstr,"Error 1 in sample read accounting.\n");
			return(PROGRAM_ERROR);
		}
	}
	last_total_samps_read = dz->total_samps_read - dz->ssampsread;
	hereinbuf = thisstart - last_total_samps_read;
	if(dostartsplice) {
		if(dz->vflag[1])
			splicer = spliceincr;
		else
			splicer = 0.0;
		for(k = 0;k < splicelen; k++) {
			for(j = 0; j < chans; j++) {
				obuf[hereinbuf] = (float)(ibuf[hereinbuf] * splicer);
				hereinbuf++;
			}
			splicer += spliceincr;
			splicer = min(splicer,1.0);
			if(hereinbuf >= dz->buflen) {
				if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
					return(exit_status);
				memset((char *)obuf,0,dz->buflen * sizeof(float));
				if(dz->vflag[1]) {
					if((exit_status = reverse_sndread(ibuf,bufcnt,dz)) < 0) 
						return(exit_status);
					bufcnt--;
				} else if((exit_status = read_samps(ibuf,dz)) < 0) 
					return(exit_status);
				if(dz->ssampsread == 0) {
					sprintf(errstr,"Error 2 in sample read accounting.\n");
					return(PROGRAM_ERROR);
				}
				hereinbuf = 0;
			}
		}
	}
	tocopy = thisend - cuts[n];	//	Copy the chunk to output
	bufremain = dz->ssampsread - hereinbuf;
	while(bufremain <= tocopy) {
		memcpy((char *)(obuf + hereinbuf),(char *)(ibuf + hereinbuf),bufremain * sizeof(float));
		if((exit_status = write_samps(obuf,dz->ssampsread,dz))<0)
			return(exit_status);
		memset((char *)obuf,0,dz->buflen * sizeof(float));
		tocopy -= bufremain;
		if(dz->vflag[1]) {
			if(bufcnt < 0) {
				if(tocopy == 0 && !doendsplice) {
					if(dz->total_samps_written == dz->insams[0])
						break;
				}
			}
			if((exit_status = reverse_sndread(ibuf,bufcnt,dz)) < 0) 
				return(exit_status);
			bufcnt--;
		} else if((exit_status = read_samps(ibuf,dz)) < 0) 
			return(exit_status);
		if((dz->ssampsread == 0) && doendsplice && dz->iparam[ISO_SPL] > 0) {
			sprintf(errstr,"Error 3 in sample read accounting.\n");
			return(PROGRAM_ERROR);
		}
		bufremain = dz->ssampsread;
		hereinbuf = 0;
		if(tocopy <= 0) {
			if(doendsplice && dz->iparam[ISO_SPL] > 0) {
				sprintf(errstr,"Error in read-write accounting 1\n");
				return(PROGRAM_ERROR);
			} else if(tocopy < 0) {
				sprintf(errstr,"Error in read-write accounting 2\n");
				return(PROGRAM_ERROR);
			} else
				break;
		}
	}
	if(dz->total_samps_written == dz->insams[0]) {
		if(tocopy || doendsplice) {
			sprintf(errstr,"Error 3A in sample read accounting.\n");
			return(PROGRAM_ERROR);
		}
		return(CONTINUE);
	}
	if(tocopy) {
		if(!doendsplice && dz->iparam[ISO_SPL] != 0) {
			sprintf(errstr,"Error in read-write accounting 3\n");
			return(PROGRAM_ERROR);
		}
		memcpy((char *)(obuf + hereinbuf),(char *)(ibuf + hereinbuf),tocopy * sizeof(float));
		hereinbuf += tocopy;
	}
	if(doendsplice && dz->iparam[ISO_SPL] != 0) {
		if(dz->vflag[1])
			splicer = 1.0 - spliceincr;
		else
			splicer = 1.0;
		last_total_samps_read = dz->total_samps_read - dz->ssampsread;
		for(k = 0;k < splicelen; k++) {
			for(j = 0; j < chans; j++) {
				obuf[hereinbuf] = (float)(ibuf[hereinbuf] * splicer);
				hereinbuf++;
			}
			splicer -= spliceincr;
			splicer = max(splicer,0.0);
			if(hereinbuf >= dz->buflen) {
				if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
					return(exit_status);
				memset((char *)obuf,0,dz->buflen * sizeof(float));
				if(dz->vflag[1]) {
					if((exit_status = reverse_sndread(ibuf,bufcnt,dz)) < 0) 
						return(exit_status);
					bufcnt--;
				} else if((exit_status = read_samps(ibuf,dz)) < 0) 
					return(exit_status);
				if(dz->ssampsread == 0) {
					sprintf(errstr,"Error 4 in sample read accounting.\n");
					return(PROGRAM_ERROR);
				}
				hereinbuf = 0;
			}
		}
	}

	if(dz->vflag[0]) {
		while(dz->ssampsread > 0) {
			if((exit_status = write_samps(obuf,dz->ssampsread,dz))<0)
				return(exit_status);
			memset((char *)obuf,0,dz->buflen * sizeof(float));
			if(dz->vflag[1]) {
				if(bufcnt < 0) {
					if(dz->total_samps_written == dz->insams[0])
						break;
				}
				if((exit_status = reverse_sndread(ibuf,bufcnt,dz)) < 0) 
					return(exit_status);
				bufcnt--;
			} else if((exit_status = read_samps(ibuf,dz)) < 0) 
				return(exit_status);
		}
	} else if(hereinbuf) {
		if((exit_status = write_samps(obuf,hereinbuf,dz))<0)
			return(exit_status);
	}
	if(do_continue)
		exit_status = CONTINUE;
	else
		exit_status = FINISHED;
	return exit_status;
}

/******************************** DO_REMNANT_CUT *********************************/
 
int do_remnant_cut(int fileno,int *cuts,double spliceincr,dataptr dz) 
{
	int exit_status, docontinue = 1;
	int chans = dz->infile->channels, splicelen = dz->iparam[ISO_SPL], dodownsplice = 0, doupsplice = 0, n, k, j;
	float *ibuf = dz->sampbuf[0], *obuf = dz->sampbuf[1];
	int tocopy, bufremain, hereinbuf, lastpassend, copyend, zeroend, tozero;
	double splicer;
	char filename[200], thisnum[8];
	fprintf(stdout,"INFO: Cutting remnant\n");
	fflush(stdout);
	if((exit_status = read_samps(ibuf,dz)) < 0) 
		return(exit_status);
	if((exit_status = eliminate_abutted_cuts(cuts,dz)) == FINISHED)
		return exit_status;
	strcpy(filename,dz->outfilename);
	sprintf(thisnum,"%d",fileno);
	strcat(filename,thisnum);
	strcat(filename,".wav");
	if((exit_status = create_sized_outfile(filename,dz))<0)
		return(exit_status);
	reset_filedata_counters(dz);
	memset((char *)obuf,0,dz->buflen * sizeof(float));
	hereinbuf = 0;
	lastpassend = 0;						//	Start copying samples from where last upsplice ended (or start of file)
	for(n=0; n< dz->itemcnt;n += 2) {
		copyend = cuts[n]; 
		if(copyend < splicelen * chans)	{	//	If there was no room to do splice on start of 1st segment originally cut
			copyend = 0;					//	Nothing to copy at start of file
			dodownsplice = 0;				
		} else {							//	Else copy only up to start of downsplice
			copyend = cuts[n] - (splicelen * chans);
			dodownsplice = 1;				//	and flag to then do downsplice				
		}	
		zeroend = cuts[n+1];				//	If there was no room to do splice on end of (last) segment originally cut
		if((dz->insams[0] - cuts[n+1] < splicelen * chans) \
		|| (dz->insams[0] - cuts[n+1] == 0)) {	 // or if splicelen is 0 and we're at end of file
			zeroend = dz->insams[0];		//	extend zeroed portion to end of file
			doupsplice = 0;					//	and flag NO endsplice,
			docontinue = 0;					//	PLUS flag quit cuttings
		} else								//	Else, flag upsplice needed
			doupsplice = 1;
		tocopy = copyend - lastpassend;		//	Copy the chunk to output (if nothing to copy, this is 0 - 0)

		bufremain = dz->ssampsread - hereinbuf;
		while(tocopy >= bufremain) {
			memcpy((char *)(obuf + hereinbuf),(char *)(ibuf + hereinbuf),bufremain * sizeof(float));
			if((exit_status = write_samps(obuf,dz->ssampsread,dz))<0)
				return(exit_status);		//	copy chunk to output before start of cut
			memset((char *)obuf,0,dz->buflen * sizeof(float));
			if((exit_status = read_samps(ibuf,dz)) < 0) 
				return(exit_status);
			if(dz->ssampsread == 0) {
				sprintf(errstr,"Error 5 in sample read accounting.\n");
				return(PROGRAM_ERROR);
			}
			tocopy -= bufremain;
			bufremain = dz->ssampsread;
			hereinbuf = 0;
		}
		if(tocopy > 0) {
			memcpy((char *)(obuf + hereinbuf),(char *)(ibuf + hereinbuf),tocopy * sizeof(float));
			hereinbuf += tocopy;
		}
		if(dodownsplice && dz->iparam[ISO_SPL] != 0) {
			splicer = 1.0;
			for(k = 0;k < splicelen; k++) {
				for(j = 0; j < chans; j++) {
					obuf[hereinbuf] = (float)(ibuf[hereinbuf] * splicer);
					hereinbuf++;
				}
				splicer -= spliceincr;
				if(hereinbuf >= dz->buflen) {
					if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
						return(exit_status);
					memset((char *)obuf,0,dz->buflen * sizeof(float));
					if((exit_status = read_samps(ibuf,dz)) < 0) 
						return(exit_status);
					if(dz->ssampsread == 0) {
						sprintf(errstr,"Error 6 in sample read accounting.\n");
						return(PROGRAM_ERROR);
					}
					hereinbuf = 0;
				}
			}
		}
		tozero = zeroend - cuts[n];	//	Zero output where cut chunk has been taken
		bufremain = dz->ssampsread - hereinbuf;
		while(bufremain <= tozero) {
			if((exit_status = write_samps(obuf,dz->ssampsread,dz))<0)
				return(exit_status);							//	Don't write anything extra into obuf
			memset((char *)obuf,0,dz->buflen * sizeof(float));	//	And refill new outbufs with zeros
			if((exit_status = read_samps(ibuf,dz)) < 0) 
				return(exit_status);
			if(dz->ssampsread == 0) {
				if(doupsplice && dz->iparam[ISO_SPL] != 0) {
					sprintf(errstr,"Error 7 in sample read accounting.\n");
					return(PROGRAM_ERROR);
				}
			}
			tozero -= bufremain;
			bufremain = dz->ssampsread;
			hereinbuf = 0;
			if(bufremain == 0) {
				if(tozero > 0) {
					sprintf(errstr,"Error in read-write accounting 4\n");
					return(PROGRAM_ERROR);
				}
			}
			if(tozero == 0) {
				break;
			}
		}
		if(tozero > 0)
			hereinbuf += tozero;

		if(doupsplice && dz->iparam[ISO_SPL] != 0) {
			splicer = 0;
			for(k = 0;k < splicelen; k++) {
				for(j = 0; j < chans; j++) {
					obuf[hereinbuf] = (float)(ibuf[hereinbuf] * splicer);
					hereinbuf++;
				}
				splicer += spliceincr;
				if(hereinbuf >= dz->buflen) {
					if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
						return(exit_status);
					memset((char *)obuf,0,dz->buflen * sizeof(float));
					if((exit_status = read_samps(ibuf,dz)) < 0) 
						return(exit_status);
					if(dz->ssampsread == 0) {
						sprintf(errstr,"Error 8 in sample read accounting.\n");
						return(PROGRAM_ERROR);
					}
					hereinbuf = 0;
				}
			}
		}
		if(!docontinue)
			break;
		lastpassend = cuts[n+1] + (splicelen * chans); 
	}
	if(docontinue && dz->ssampsread != 0) {		//	There was a splice up at the end of last cut (so not at end of file)
												//	But if it was a zero splice, could be at end of file
		tocopy = dz->insams[0] - (cuts[dz->itemcnt - 1] + (splicelen * chans));
		bufremain = dz->ssampsread - hereinbuf;
		while(bufremain <= tocopy) {
			memcpy((char *)(obuf + hereinbuf),(char *)(ibuf + hereinbuf),bufremain * sizeof(float));
			if((exit_status = write_samps(obuf,dz->ssampsread,dz))<0)
				return(exit_status);
			memset((char *)obuf,0,dz->buflen * sizeof(float));
			if((exit_status = read_samps(ibuf,dz)) < 0) 
				return(exit_status);
			tocopy -= bufremain;
			hereinbuf = 0;
			bufremain = dz->ssampsread;
			if(tocopy <= 0) {
				if(tocopy < 0) {
					sprintf(errstr,"Error in read-write accounting 6\n");
					return(PROGRAM_ERROR);
				}
				break;
			}
		}
		if(tocopy) {
			sprintf(errstr,"Error in read-write accounting 7\n");
			return(PROGRAM_ERROR);
		}
	}
	return exit_status;
}

/********************************** ELIMINATE_ABUTTED_CUTS ******************************
 *
 *	Where cut chunks overlap each other, there is no remnant to save between them
 *	so the cutpoints at this point are eliminated before remnant is created.
 */

int eliminate_abutted_cuts(int *cuts,dataptr dz)
{
	int n, m, dlbspcnt  = dz->iparam[ISO_SPL] * 2 * dz->infile->channels; //	length of two splices in samples
	for(n=2;n < dz->itemcnt;n+=2) {
		if(cuts[n] - cuts[n-1] <= dlbspcnt) {
			for(m = n+1;m < dz->itemcnt;m++)
				cuts[m-2] = cuts[m];
			dz->itemcnt -= 2;
			n -= 2;
		}
	}
	if(dz->itemcnt == 2 && cuts[0] == 0 && cuts[1] == dz->insams[0]) {
		fprintf(stdout,"INFO: No remnant file\n");
		fflush(stdout);
		return FINISHED;
	}
	return CONTINUE;
}

/****************************** GET_MODE *********************************/

int get_the_mode_no(char *str, dataptr dz)
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

/******************************** DO_A_MULTICUT *********************************/
 
int do_a_multicut(int jj,double spliceincr,dataptr dz) 
{
	int exit_status, bufcnt = 0;
	int chans = dz->infile->channels, splicelen = dz->iparam[ISO_SPL], dodownsplice = 0, doupsplice = 0, n, k, j;
	float *ibuf = dz->sampbuf[0], *obuf = dz->sampbuf[1];
	int tocopy, bufremain, hereinbuf, lastpassend, copyend, zeroend, tozero;
	double splicer;
	int itemcnt = dz->iparray[0][jj];
	int  *cuts = dz->lparray[jj];
	if(dz->vflag[1]) {
		bufcnt = dz->insams[0]/dz->buflen;
		if(bufcnt * dz->buflen == dz->insams[0])
			bufcnt--;
		if((exit_status = reverse_sndread(ibuf,bufcnt,dz)) < 0) 
			return(exit_status);
		bufcnt--;
	} else if((exit_status = read_samps(ibuf,dz)) < 0) 
		return(exit_status);
	memset((char *)obuf,0,dz->buflen * sizeof(float));
	hereinbuf = 0;
	lastpassend = 0;						//	Start copying samples from where last upsplice ended (or start of file)
	for(n=0; n< itemcnt;n += 2) {
		zeroend = cuts[n]; 
		if(zeroend < splicelen * chans)	{	//	If there was no room to do splice on start of 1st segment originally cut
			zeroend = 0;					//	Nothing to zero at start of file
			doupsplice = 0;				
		} else {							//	Else zero up to start of upsplice
			zeroend = cuts[n] - (splicelen * chans);
			doupsplice = 1;					//	and flag to then do upsplice				
		}	
		copyend = cuts[n+1];				//	If there is no room to do splice on end of (last) segment originally cut
		if(dz->insams[0] - copyend < splicelen * chans) {
			copyend = dz->insams[0];		//	extend copied portion to end of file
			dodownsplice = 0;				//	and flag NO endsplice,
		} else								//	Else, flag downsplice needed
			dodownsplice = 1;
		tozero = zeroend - lastpassend;		//	Zero the chunk to output (if nothing to zero, this is 0 - 0)
		bufremain = dz->ssampsread - hereinbuf;
		while(tozero >= bufremain) {
			if((exit_status = write_samps(obuf,dz->ssampsread,dz))<0)
				return(exit_status);		//	copy chunk to output before start of cut
			memset((char *)obuf,0,dz->buflen * sizeof(float));
			if(dz->vflag[1]) {
				if((exit_status = reverse_sndread(ibuf,bufcnt,dz)) < 0) 
					return(exit_status);
				bufcnt--;
			} else if((exit_status = read_samps(ibuf,dz)) < 0) 
				return(exit_status);
			if(dz->ssampsread == 0) {
				sprintf(errstr,"Error 9 in sample read accounting at segment %d.\n",(n/2)+1);
				return(PROGRAM_ERROR);
			}
			tozero -= bufremain;
			bufremain = dz->ssampsread;
			hereinbuf = 0;
		}
		if(tozero > 0)
			hereinbuf += tozero;
		if(doupsplice && dz->iparam[ISO_SPL] != 0) {
			if(dz->vflag[1])
				splicer = spliceincr;
			else
				splicer = 0.0;
			for(k = 0;k < splicelen; k++) {
				for(j = 0; j < chans; j++) {
					obuf[hereinbuf] = (float)(ibuf[hereinbuf] * splicer);
					hereinbuf++;
				}
				splicer += spliceincr;
				splicer  = min(splicer,1.0);
				if(hereinbuf >= dz->buflen) {
					if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
						return(exit_status);
					memset((char *)obuf,0,dz->buflen * sizeof(float));
					if(dz->vflag[1]) {
						if((exit_status = reverse_sndread(ibuf,bufcnt,dz)) < 0) 
							return(exit_status);
						bufcnt--;
					} else if((exit_status = read_samps(ibuf,dz)) < 0) 
						return(exit_status);
					if(dz->ssampsread == 0) {
						sprintf(errstr,"Error 10 in sample read accounting.\n");
						return(PROGRAM_ERROR);
					}
					hereinbuf = 0;
				}
			}
		}
		tocopy = copyend - cuts[n];	//	Copy cut chunk to output
		bufremain = dz->ssampsread - hereinbuf;
		while(bufremain <= tocopy) {
			memcpy((char *)(obuf + hereinbuf),(char *)(ibuf + hereinbuf),bufremain * sizeof(float));
			if((exit_status = write_samps(obuf,dz->ssampsread,dz))<0)
				return(exit_status);							//	Don't write anything extra into obuf
			memset((char *)obuf,0,dz->buflen * sizeof(float));	//	And refill new outbufs with zeros
			tocopy -= bufremain;
			if(dz->vflag[1]) {
				if(dz->total_samps_written == dz->insams[0]) {
					if(tocopy == 0 && !dodownsplice)
						break;
				}
				if((exit_status = reverse_sndread(ibuf,bufcnt,dz)) < 0) 
					return(exit_status);
				bufcnt--;
			} else if((exit_status = read_samps(ibuf,dz)) < 0) 
				return(exit_status);
			if(dz->ssampsread == 0) {
				if(dodownsplice && dz->iparam[ISO_SPL] != 0) {
					sprintf(errstr,"Error 11 in sample read accounting.\n");
					return(PROGRAM_ERROR);
				}
			}
			bufremain = dz->ssampsread;
			hereinbuf = 0;
			if(bufremain == 0) {
				if(tocopy > 0) {
					sprintf(errstr,"Error 12 in read-write accounting  at segment %d.\n",(n/2)+1);
					return(PROGRAM_ERROR);
				}
			}
			if(tocopy == 0) {
				break;
			}
		}
		if(dz->total_samps_written == dz->insams[0]) {
			if(tocopy || dodownsplice) {
				sprintf(errstr,"Error 13A in sample read accounting.\n");
				return(PROGRAM_ERROR);
			}
			return(CONTINUE);
		}
		if(tocopy > 0) {
			memcpy((char *)(obuf + hereinbuf),(char *)(ibuf + hereinbuf),tocopy * sizeof(float));
			hereinbuf += tocopy;
		}

		if(dodownsplice && dz->iparam[ISO_SPL] != 0) {
			if(dz->vflag[1])
				splicer = 1.0 - spliceincr;
			else
				splicer = 1.0;
			for(k = 0;k < splicelen; k++) {
				for(j = 0; j < chans; j++) {
					obuf[hereinbuf] = (float)(ibuf[hereinbuf] * splicer);
					hereinbuf++;
				}
				splicer -= spliceincr;
				splicer = max(splicer,0.0);
				if(hereinbuf >= dz->buflen) {
					if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
						return(exit_status);
					memset((char *)obuf,0,dz->buflen * sizeof(float));
					if(dz->vflag[1]) {
						if((exit_status = reverse_sndread(ibuf,bufcnt,dz)) < 0) 
							return(exit_status);
						bufcnt--;
					} else if((exit_status = read_samps(ibuf,dz)) < 0) 
						return(exit_status);
					if(dz->ssampsread == 0) {
						sprintf(errstr,"Error 14 in sample read accounting.\n");
						return(PROGRAM_ERROR);
					}
					hereinbuf = 0;
				}
			}
		}
		lastpassend = cuts[n+1] + (splicelen * chans); 
	}
	if(dz->vflag[0]) {
		while(dz->ssampsread > 0) {
			if((exit_status = write_samps(obuf,dz->ssampsread,dz))<0)
				return(exit_status);
			memset((char *)obuf,0,dz->buflen * sizeof(float));
			if(dz->vflag[1]) {
				if(bufcnt < 0) {
					if(dz->total_samps_written == dz->insams[0])
						break;
				}
				if((exit_status = reverse_sndread(ibuf,bufcnt,dz)) < 0) 
					return(exit_status);
				bufcnt--;
			} else if((exit_status = read_samps(ibuf,dz)) < 0) 
				return(exit_status);
		}
	} else if(hereinbuf > 0) {
		if((exit_status = write_samps(obuf,hereinbuf,dz))<0)
			return(exit_status);
	}
	return CONTINUE;
}

/******************************** CONVERT_BRKVALS_DB_TO_GAIN *************************/

int convert_brkvals_db_to_gain(int paramno,dataptr dz)
{
	int exit_status;
	double *p = dz->brk[paramno];
	double *pend = p + (dz->brksize[paramno] * 2);
	p++;
	while(p < pend) {
		if((exit_status = convert_dB_at_or_below_zero_to_gain(p))<0)
			return(exit_status);
		p += 2;
	}
	return(FINISHED);
}

/******************************** ISOLATE_PARAM_PREPROCESS *************************/

int isolate_param_preprocess(dataptr dz)
{
	int exit_status, chans = dz->infile->channels, n;
	int lastsampsread, winlen, winend, segstart = 0, maxloc, wincnt;
	int *cuts;
	float *ibuf = dz->sampbuf[0];
	double time, srate = (double)dz->infile->srate, winmax, maxenvsamp = 0.0, minenvsamp = 0.0;
	int gotmax, cutscnt, segcnt;
	
	fprintf(stdout,"INFO: Finding and Counting segments\n");
	fflush(stdout);
	if((exit_status = read_samps(ibuf,dz)) < 0) 
		return(exit_status);
	winlen = (int)round(ISO_WINLEN * MS_TO_SECS * srate) * chans;
	segcnt = 0;
	gotmax = 0;
	maxloc = 0;
	lastsampsread = 0;
	wincnt = 0;
	winend = winlen;
	while(dz->ssampsread > 0) {
		winmax = -1;
		if(dz->brksize[ISO_THRON] || dz->brksize[ISO_THROFF]) {
			time = (double)(lastsampsread + wincnt) * srate;
			if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
				return(exit_status);
			if(dz->param[ISO_THRON] >= dz->param[ISO_THROFF]) {
				sprintf(errstr,"On (%lf) and Off (%lf) thresholds are incompatible at time %lf secs\n",
				convert_gain_to_dB_from_minus96(dz->param[ISO_THRON]),convert_gain_to_dB_from_minus96(dz->param[ISO_THROFF]),time);
				return(DATA_ERROR);
			}
		}
		while(wincnt<winend) {
			if(wincnt >= dz->ssampsread) {
				wincnt -= dz->ssampsread;
				winend -= dz->ssampsread;
				lastsampsread = dz->total_samps_read;
				if((exit_status = read_samps(ibuf,dz)) < 0) 
					return(exit_status);
				if(dz->ssampsread == 0) 
					break;
			}
			if(fabs(ibuf[wincnt]) > winmax) {
				winmax = fabs(ibuf[wincnt]);
				maxloc = ((lastsampsread + wincnt)/chans) * chans;	//	Round location to a chan-group boundary
			}
			wincnt++;
		}
		if(gotmax) {
			if(winmax <= dz->param[ISO_THROFF]) {
				if(maxloc - segstart >= dz->iparam[ISO_MIN]) {
					segcnt++;
					gotmax = 0;
				}
			}
		} else {
			if(winmax >= dz->param[ISO_THRON]) {
				gotmax = 1;
				segstart = maxloc;
			}
		}
		maxenvsamp = max(winmax,maxenvsamp);
		minenvsamp = min(winmax,minenvsamp);
		wincnt = winend;
		winend += winlen;
	}
	
	if(gotmax) {
		if(dz->insams[0] - segstart >= dz->iparam[ISO_MIN])
			segcnt++;
	}
	if(segcnt == 0) {
		sprintf(errstr,"No segments found above threshold and of sufficient length\n(maxlevel %.2lf dB, minlevel %.2lf dB)\n",
		convert_gain_to_dB_from_minus96(maxenvsamp),convert_gain_to_dB_from_minus96(minenvsamp));
		return(DATA_ERROR);
	}
	segcnt *= 2;
	dz->specenvcnt = segcnt;	//	specevcnt used to  store total number of cuts, for remnant-pass
	dz->itemcnt = 1;
	dz->iarray_cnt = 1;
	dz->larray_cnt = 1;
	if((dz->iparray = (int **)malloc(dz->iarray_cnt * sizeof(int *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for internal integer arrays.\n");
		return(MEMORY_ERROR);
	}
	if((dz->iparray[0] = (int *)malloc(dz->itemcnt * sizeof(int)))==NULL) {		//	Store length of cut store 
		sprintf(errstr,"INSUFFICIENT MEMORY to store lengths of cut-groups.\n");
		return(MEMORY_ERROR);
	}
	dz->iparray[0][0] = segcnt;
	if((dz->lparray = (int **)malloc(dz->larray_cnt * sizeof(int *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for internal integer arrays.\n");
		return(MEMORY_ERROR);
	}
	if((dz->lparray[0] = (int *)malloc(segcnt * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store segment cuts.\n");
		return(MEMORY_ERROR);
	}
	sndseekEx(dz->ifd[0],0,0);
	reset_filedata_counters(dz);
	if((exit_status = read_samps(ibuf,dz)) < 0) 
		return(exit_status);
	cutscnt = 0;
	cuts = dz->lparray[0];
	gotmax = 0;
	maxloc = 0;
	lastsampsread = 0;
	wincnt = 0;
	winend = winlen;
	fprintf(stdout,"INFO: Storing segment positions.\n");
	fflush(stdout);
	while(dz->ssampsread > 0) {
		winmax = -1;
		while(wincnt<winend) {
			if(wincnt >= dz->ssampsread) {
				wincnt -= dz->ssampsread;
				winend -= dz->ssampsread;
				lastsampsread = dz->total_samps_read;
				if((exit_status = read_samps(ibuf,dz)) < 0) 
					return(exit_status);
				if(dz->ssampsread == 0) 
					break;
			}
			if(fabs(ibuf[wincnt]) > winmax) {
				winmax = fabs(ibuf[wincnt]);
				maxloc = ((lastsampsread + wincnt)/chans) * chans;	//	Round location to a chan-group boundary
			}
			wincnt++;
		}
		if(gotmax) {
			if(winmax <= dz->param[ISO_THROFF]) {
				if(maxloc - segstart >= dz->iparam[ISO_MIN]) {
					cuts[cutscnt++] = segstart;			
					cuts[cutscnt++] = maxloc;			
					gotmax = 0;
				}
			}
		} else {
			if(winmax >= dz->param[ISO_THRON]) {
				gotmax = 1;
				segstart = maxloc;
			}
		}
		wincnt = winend;
		winend += winlen;
	}
	if(gotmax) {
		if(dz->insams[0] - segstart >= dz->iparam[ISO_MIN]) {
			cuts[cutscnt++] = segstart;			
			cuts[cutscnt++] = dz->insams[0];			
		}
	}
	if(dz->iparam[ISO_LEN] > 0) {
		for(n=0;n < cutscnt;n+=2)
			cuts[n+1] = cuts[n] + dz->iparam[ISO_LEN];
	}
	sndseekEx(dz->ifd[0],0,0);
	reset_filedata_counters(dz);
	return FINISHED;
}

/******************************** CONVERT_GAIN_TO__FROM_MINUS96 *************************/

double convert_gain_to_dB_from_minus96(double val)
{
	double mindb = 1.0/pow(10,(96.0/20.0));
	if(val <= mindb)
		val = -96.0;
	else if(flteq(val,1.0))
		val = 0.0;
	else {
		val = 1.0/val;
		val = log10(val);
		val *= 20.0;
		val = -val;
		if(val < -96.0)
			val = -96.0;
	}
	return val;
}

/******************************** REVERSE_SNDREAD *************************
 *
 * Read buffer and reverse it
 */

int reverse_sndread(float *ibuf,int bufcnt, dataptr dz)
{
	int  exit_status;
	int n, m, j, up, dn;
	float temp;
	int total_samps_read = dz->total_samps_read;
	if(bufcnt < 0) {
		sprintf(errstr,"Error in reverse sample read accounting\n");
		return(PROGRAM_ERROR);
	}
	sndseekEx(dz->ifd[0],bufcnt * dz->buflen,0);
	if((exit_status = read_samps(ibuf,dz)) < 0) 
		return(exit_status);
	total_samps_read += dz->ssampsread;
	dz->total_samps_read = total_samps_read;
	n = dz->ssampsread/2;
	if(n * 2 != dz->ssampsread)	{ //	ODD count of samples
		up = n+1;
		dn = n-1;
	} else {
		up = n;
		dn = n-1;
	}
	for(m=up,j = dn;j >= 0;m++,j--) {
		temp    = ibuf[j];
		ibuf[j] = ibuf[m];
		ibuf[m] = temp;
	}
	return FINISHED;
}

/******************************** REVERSE_CUTS *************************
 *
 * reverse long-array of paired values (start+end of cuts)
 */

int reverse_cuts(int *array,int itemcnt,dataptr dz)
{
	int m, j, up, dn;
	int temp;
	int n = itemcnt/2;
	if(n * 2 != itemcnt)	{ //	ODD count in cuts array
		sprintf(errstr,"Array to reverse is not correctly paired\n");
		return(DATA_ERROR);
	}
	up = n;
	dn = n-1;
	for(m=up,j = dn;j >= 0;m++,j--) {
		temp = array[j];
		array[j] = dz->insams[0] - array[m];
		array[m] = dz->insams[0] - temp;
	}
	return FINISHED;
}
