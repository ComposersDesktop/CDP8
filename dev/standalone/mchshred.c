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



/* MCHSHRED -- 
 *
 * Do a shred, scattering the fragments at random over multichan space
 *
 * (mode 1) mono --> outchans
 * (mode 2) inchans to equal-outchans
 * in mode 2 MSHR_OUTCHANS is internal, and equals dz->infile->channels

		MODE 1
		if((exit_status = set_param_data(ap,0   ,4,4,"iidd"))<0)
			return(FAILED);
		MODE 2
		if((exit_status = set_param_data(ap,0   ,4,3,"0idd"))<0)
			return(FAILED);

		if((exit_status = set_vflgs(ap,"",0,"","",0,0,""))<0)
			return(FAILED);

		MODE 1
		exit_status = set_internalparam_data("iiiiiiiiiiiii",ap);	break;
		MODE 2
		exit_status = set_internalparam_data("iiiiiiiiiiiiii",ap);	break;

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
//#include <memory.h>
//#include <string.h>

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

static int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz);
static int get_the_mode_from_cmdline(char *str,dataptr dz);
static int setup_mchshred_application(dataptr dz);
static int setup_mchshred_param_ranges_and_defaults(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
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
static int setup_and_init_input_brktable_constants(dataptr dz,int brkcnt);

static void do_endsplice(float *i,int chans,dataptr dz);
static void do_bufend_splices(int current_buf,dataptr dz);
static void ptr_sort(int end,dataptr dz);
static void do_startsplice(float *i,int chans,dataptr dz);
static void permute_chunks(dataptr dz);
static void insert(int n,int t,dataptr dz);
static void prefix(int n,dataptr dz);
static void shuflup(int k,dataptr dz);
static void heavy_scat(dataptr dz);
static void normal_scat(dataptr dz);
static int get_basis_lengths(int *work_len,dataptr dz);
static int shred(int shredno,int current_buf,int work_len,dataptr dz);
static int mchshred_process(dataptr dz);
static int create_mchshred_buffers(dataptr dz);
static int mchshred_pconsistency(dataptr dz);
static void permute_chans(int outchans,dataptr dz);
static void insertch(int n,int t,int outchans,dataptr dz);
static void prefixch(int n,int outchans,dataptr dz);
static void shuflupch(int k,int outchans,dataptr dz);
static void rotate(float *splicbuf,int len,int *permch,float *rot,int outchans);

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
		if((exit_status = setup_mchshred_application(dz))<0) {
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
	if((exit_status = setup_mchshred_param_ranges_and_defaults(dz))<0) {
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
	if((exit_status = mchshred_pconsistency(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
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
//	create_sndbufs()....
	if((exit_status = create_mchshred_buffers(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = open_the_outfile(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	//param_preprocess()						redundant
	//spec_process_file =
	if((exit_status = mchshred_process(dz))<0) {
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
	if(dz->mode == 0) {
		dz->infile->channels = dz->iparam[MSHR_OUTCHANS];
		dz->outfile->channels = dz->iparam[MSHR_OUTCHANS];
	}
	if((exit_status = create_sized_outfile(dz->outfilename,dz))<0)
		return(exit_status);
	if(dz->mode == 0)
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

/************************* SETUP_MCHSHRED_APPLICATION *******************/

int setup_mchshred_application(dataptr dz)
{
	int exit_status;
	aplptr ap;
	if((exit_status = establish_application(dz))<0)		// GLOBAL
		return(FAILED);
	ap = dz->application;
	// SEE parstruct FOR EXPLANATION of next 2 functions
	switch(dz->mode) {
	case(0):
		exit_status = set_param_data(ap,0   ,4,4,"iddi");
		break;
	case(1):
		exit_status = set_param_data(ap,0   ,4,3,"idd0");
		break;
	}
	if(exit_status <0)
		return(FAILED);
	if((exit_status = set_vflgs(ap,"",0,"","",0,0,""))<0)
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
		} else if((infile_info->channels == 1 && dz->mode != 0) || (infile_info->channels != 1 && dz->mode != 1))  {
			sprintf(errstr,"File %s does not have correct number of channels for this mode\n",cmdline[0]);
			return(DATA_ERROR);
		} else if((exit_status = copy_parse_info_to_main_structure(infile_info,dz))<0) {
			sprintf(errstr,"Failed to copy file parsing information\n");
			return(PROGRAM_ERROR);
		}
		free(infile_info);
	}
	return(FINISHED);
}

/************************* SETUP_MCHSHRED_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_mchshred_param_ranges_and_defaults(dataptr dz)
{
	int exit_status;
	aplptr ap = dz->application;
	// set_param_ranges()
	double duration = (double)(dz->insams[0]/dz->infile->channels)/(double)(dz->infile->srate);
	
	ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
	// NB total_input_param_cnt is > 0 !!!
	if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
		return(FAILED);
	// get_param_ranges()
	ap->lo[0] = 1.0;
	ap->hi[0] = (double)MSHR_MAX;
	ap->default_val[0]	= 1;
	ap->lo[1]  	= (double)((MSHR_SPLICELEN * 3)/(double)dz->infile->srate);
	ap->hi[1]  	= (duration/2.0)-FLTERR;
	ap->default_val[1]	= max((duration/8.0),((double)(MSHR_SPLICELEN * 3)/(double)dz->infile->srate));
	ap->lo[2]	= 0.0;
	ap->hi[2]	= (double)MSHR_MAX_SCATTER;
	ap->default_val[2] = 1.0;
	ap->lo[3]	= 2;
	ap->hi[3]	= 16;
	ap->default_val[3]	= 8;
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
			if((exit_status = setup_mchshred_application(dz))<0)
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
	usage2("shred");
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
	if(!strcmp(prog_identifier_from_cmdline,"shred"))				dz->process = MCHSHRED;
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

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
	if(!strcmp(str,"shred")) {
		fprintf(stderr,
	    "USAGE:\n"
	    "mchshred shred 1 infile outfile repeats chunklen scatter outchans \n"
	    "mchshred shred 2 infile outfile repeats chunklen scatter\n"
		"\n"
		"Sound is cut into random segments, which are then reassembled\n"
		"in random order, within the original duration.\n"
		"Further shreds, shred the previously shredded output.\n"
		"\n"
		"The shredding process distributes the output segments over\n"
		" a randopm permutation of the output channels.\n"
		"\n"
  		"REPEATS   no. of repeats of shredding process.\n"
  		"CHUNKLEN  average length of chunks to cut & permute.\n"
  		"SCATTER   randomisation of cuts (0 to K): default 1.\n"
  		"          where K = total no. of chunks (snd-duration/chunklen).\n"
		"          If scatter = 0, reorders without shredding.\n"
  		"OUTCHAN   Mode 1 shreds a mono input to a multichannel output.\n"
  		"          with 'outchan' channels.\n"
  		"          Mode 2 shreds an already multichannel file.\n"
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

/***************************** MCHSHRED_PCONSISTENCY *************************/

int mchshred_pconsistency(dataptr dz)
{
	int chans = dz->infile->channels;
	double duration = (double)(dz->insams[0]/chans)/(double)dz->infile->srate;
    initrand48();
	dz->iparam[MSHR_CHCNT] = round(duration/dz->param[MSHR_CHLEN]);
	if(dz->param[MSHR_SCAT] > (double)dz->iparam[MSHR_CHCNT]) {
		sprintf(errstr,"Scatter value cannot be greater than infileduration/chunklength.\n");
		return(DATA_ERROR);
	}    
	if(dz->param[MSHR_SCAT] > 1.0)
		dz->iparam[MSHR_SCAT] = round(dz->param[MSHR_SCAT]);
	else
		dz->iparam[MSHR_SCAT] = 0;		
	/* setup splice params */
	if(dz->mode == 0)
		chans = dz->iparam[MSHR_OUTCHANS];
	else
		chans = dz->infile->channels;
	dz->iparam[MSHR_SPLEN]     = MSHR_SPLICELEN  * chans;
    if((dz->lparray[MSHR_CHUNKPTR] = (int *)malloc(dz->iparam[MSHR_CHCNT] * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for chunk cuts array.\n");
		return(MEMORY_ERROR);
	}
    if((dz->lparray[MSHR_CHUNKLEN] = (int *)malloc(dz->iparam[MSHR_CHCNT] * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for chunk lens array.\n");
		return(MEMORY_ERROR);
	}
    if((dz->iparray[MSHR_PERM]     = (int  *)malloc(dz->iparam[MSHR_CHCNT] * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for chunk permutation array.\n");
		return(MEMORY_ERROR);
	}
    if((dz->iparray[MSHR_PERMCH]   = (int  *)malloc(chans * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for channel permutation array.\n");
		return(MEMORY_ERROR);
	}
	 if((dz->fptr[MSHR_ROTATE]  = (float  *)malloc(chans * sizeof(float)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for channel permutation buffer.\n");
		return(MEMORY_ERROR);
	}
    dz->lparray[MSHR_CHUNKPTR][0] = 0;	/* first chunk is at start of buffer */
	return(FINISHED);
}

/*************************** CREATE_MCHSHRED_BUFFERS **************************/

int create_mchshred_buffers(dataptr dz)
{
	int bigfilesize;
	if(dz->mode == 0)
		bigfilesize = dz->insams[0] * dz->iparam[MSHR_OUTCHANS];
	else
		bigfilesize = dz->insams[0];
	dz->bufcnt = 3;
	if((dz->bigbuf = (float *)malloc((bigfilesize * dz->bufcnt) * sizeof(float)))==NULL) {
		sprintf(errstr,"File too big for this process\n");
		return(DATA_ERROR);
	}	
	dz->buflen = bigfilesize;
	dz->sampbuf[0] = dz->bigbuf;
	dz->sampbuf[1] = dz->sampbuf[0] + dz->buflen;
	dz->sampbuf[2] = dz->sampbuf[1] + dz->buflen;
	dz->iparam[MSHR_LAST_BUFLEN] = bigfilesize;	/* i.e. buflen = true outfile filelen */
	dz->iparam[MSHR_LAST_CHCNT]  = dz->iparam[MSHR_CHCNT];
	dz->iparam[MSHR_LAST_SCAT]   = dz->iparam[MSHR_SCAT];
	if(dz->iparam[MSHR_LAST_CHCNT] < 2) {
		fprintf(stdout, "WARNING: FINAL BUFFER WON'T BE SHREDDED (Too short for chunklen set).\n");
		fprintf(stdout, "WARNING: It will shred if you\n");
		fprintf(stdout, "WARNING: a) shorten infile by (>) chunklen, OR\n");
		fprintf(stdout, "WARNING: b) alter chunklen until last buffer has >1 chunk in it.\n");
		fflush(stdout);
	}
	memset((char *) (dz->sampbuf[0]),0,(dz->buflen * dz->bufcnt) * sizeof(float));
	return(FINISHED);
}

/************************* MCHSHRED_PROCESS ***************************/

int mchshred_process(dataptr dz)
{
	int exit_status;
	int outchans;
	int n, cnt = 0, checker = 0, work_len, lfactor;
	double factor = (double)dz->insams[0]/(double)dz->iparam[MSHR_CNT];
	int current_buf;
	if(dz->mode == 0)
		outchans = dz->iparam[MSHR_OUTCHANS];
	else
		outchans = dz->infile->channels;
	memset((char*)dz->sampbuf[2],0,dz->buflen * sizeof(float));
	if(sloom && dz->iparam[MSHR_SCAT]) {
		fprintf(stdout,"WARNING: There is a finite possibility program will not terminate.\n");
		fprintf(stdout,"WARNING: If in doubt, press STOP\n");
		fflush(stdout);
	}
	do {
		if(cnt == 0 && dz->infile->channels == 1) {
			dz->buflen /= outchans;
			if((exit_status = read_samps(dz->sampbuf[0],dz))<0)
				return(exit_status);
			dz->buflen *= outchans;
		} else {
			if((exit_status = read_samps(dz->sampbuf[0],dz))<0)
				return(exit_status);
		}
		current_buf = 0;
		if(dz->samps_left <= 0 ) {
			dz->buflen = dz->iparam[MSHR_LAST_BUFLEN];
			dz->iparam[MSHR_SCAT] = dz->iparam[MSHR_LAST_SCAT];
			dz->iparam[MSHR_CHCNT]  = dz->iparam[MSHR_LAST_CHCNT];
		}
		if((dz->iparam[MSHR_CHCNT_LESS_ONE] = dz->iparam[MSHR_CHCNT] - 1)<=0) {
			if(cnt==1) {
				sprintf(errstr,"SOUNDFILE TOO SMALL to shred.\n");
				return(DATA_ERROR);
			} else {
				n = dz->buflen;
				dz->buflen = dz->ssampsread;
				do_bufend_splices(current_buf,dz);
				if((exit_status = write_exact_samps(dz->sampbuf[current_buf],dz->buflen,dz))<0)
					return(exit_status);
				dz->buflen = n;
			}
		} else {
			if((exit_status = get_basis_lengths(&work_len,dz))<0)
				return(exit_status);
			if(!sloom && !sloombatch)
				checker = 0;
			else
				display_virtual_time(0L,dz);
			for(n=0;n<dz->iparam[MSHR_CNT];n++) {
				if((exit_status = shred(n,current_buf,work_len,dz))<0)
					return(exit_status);
				current_buf = !current_buf;	

				if(!sloom && !sloombatch) {
					fprintf(stdout,"INFO: %d of %d\n",n+1,dz->iparam[MSHR_CNT]);
					fflush(stdout);
				} else {
					lfactor = (int)round((double)n * factor);
					display_virtual_time(lfactor,dz);
				}
			}
			do_bufend_splices(current_buf,dz);
			if((exit_status = write_exact_samps(dz->sampbuf[current_buf],dz->buflen,dz))<0)
				return(exit_status);
		}
	} while(dz->samps_left > 0);
	return(FINISHED);
}

/******************************* SHRED **************************
 *
 *	AUGUST 2010
 *
 * The logic below is false. The 1st item of the shred DOES
 * need a startsplice because it is taken from some arbitrary place
 * in the source, so will start abruptly. Similarly, the last segment
 * DOES need an endsplice, for the samew reason. So THis has been
 * corrected here .... however the original CDP shred program
 * is INCORRECT ... but I have retained it as that is what
 * one expects to hear!!
 *
 * (1)	If the random-scattering of chunk boundaries is <= 1.0, scatter
 *	each chunk boundary separately (normal_scat).
 *	Else, scatter chunk boundaries over groups of chunklens (heavy_scat).
 * (2)	Calculate the lengths of the chunks, from difference of their positions.
 * (2A)	NB the length of the is the difference between its start and
 *	THE END OF THE work_len (NOT  the ned of the buffer). Chunklens are
 *	measured from start of startsplice to START of endsplice (see
 *	diagram).
 * (3)	Generate a permutation (permm[]) of the chunk numbers.
 *
 * (4)	The chunk that is to be FIRST in the permuted set does not need
 *	to be spliced at its beginning (as there's nothing prior to it,
 *	to splice into) but only at its end.
 * (a)	The address from which we copy is position in current buffer
 *	where chunk associated with 0 (chunk[permm[0]) is.
 * (b)	The address to copy TO is start of the OTHER  buffer (buf[!this]).
 * (c) 	The unit_len of the chunk we're copying is chunklen[permm[0]].
 * (d)	Copy all of this.
 * (f)	Copy SPLICELEN extra (full_len is SPLICELEN bigger that unit_len..
 *	.. see diagram), making an endsplice on it as we do.
 *
 * (5)	For the rest of the chunks APART FROM THE LAST....
 * (a)	as 4(a): n associated with perm[n]..
 * (b)  advance in new buffer by chunklen of PREVIOUSLY written chunk..
 *	This 'length' is length to START of its endsplice, which is where
 *	new chunk is spliced in.
 * (c)  as 4(c)
 * (d)  Do a startsplice as we copy from old_address to new_address.
 * (e)	Copy from END of this startsplice, a segment of length chnklen
 *	(length of the total copied chunk) MINUS length of that startsplice
 *	(SPLICELEN).
 * (f)	as 4(f).
 *
 * (6)	For the last chunk, we don't need an endsplice (nothing to splice to)
 * (a-d)as 5(a-d).
 * (e)	Copy from end of STARTSPLICE to end of ENDSPLICE ... i.e.
 *	a whole chunklen, because full_len - SPLICELEN = chunklen.
 *
 *         ___full_len___
 *        |              |
 *         _chunklen__   |
 *        | S         |
 *        | T         |E
 *        | R ________|N
 *        | T/|       \D
 *        | / |        \
 *	  |/  |         \|
 *            |          |
 *            |_chunklen_|
 *        |
 *        |___full_len___|
 *
 * (7)	Set memory in old buffer to 0 (because we ADD into it, with splices).
 */

int shred(int shredno,int current_buf,int work_len,dataptr dz)
{   
	int n;
	int outchans, outchansratio, permno;							//	2 buffers are used
	float *old_addr, *new_addr;										//	current_buf  (0 or 1) is read from	
	float *splicbuf = dz->sampbuf[2];								//	!current_buf (1 or 0) is written to
	double val;
	int chnk_len, totlen, k;
	char *destination;
	int *chunkptr = dz->lparray[MSHR_CHUNKPTR];
	int *chunklen = dz->lparray[MSHR_CHUNKLEN];
	int  *permm    = dz->iparray[MSHR_PERM];
	int  *permch   = dz->iparray[MSHR_PERMCH];
	float *rot     = dz->fptr[MSHR_ROTATE];
	if(dz->mode == 0)												//	No of chans in output
		outchans = dz->iparam[MSHR_OUTCHANS];						//	depends on whether input is mono (mode 0)
	else															//	or multichan (mode 1)
		outchans = dz->infile->channels;
	outchansratio = (int)(outchans/dz->infile->channels);			//	Ratio chans in out to chans in input
	memset((char*)splicbuf,0,dz->buflen * sizeof(float));			//	Preset splicebuffer to zero
	if(!dz->iparam[MSHR_SCAT])	
		normal_scat(dz);											//	Generate the cut points
	else	
		heavy_scat(dz);

	for(n=0;n<dz->iparam[MSHR_CHCNT_LESS_ONE];n++)					//	Deduce the cunkjlens from the cut points
		chunklen[n] = chunkptr[n+1] - chunkptr[n];
	chunklen[n] = work_len - chunkptr[n];							//	Get length of last chunk
	permute_chunks(dz);												//	Permute chunk order
	permute_chans(outchans,dz);										//	Generate a rand permutation of output chans
	permno = 0;														//	Set output chan counter to zer0
	if(dz->infile->channels == 1 && shredno == 0) {					//	First shred of mono file scatters src to N outchans
		old_addr = dz->sampbuf[current_buf] + chunkptr[permm[0]];	//	Go to get first (permd) chunlk of mono source
		new_addr = dz->sampbuf[!current_buf];						//	Set goal place as start of output buffer
		chnk_len = chunklen[permm[0]];								//	Get length of chunk to cut
		totlen = (chnk_len + dz->iparam[MSHR_SPLEN]) * outchans;	//	Add length of final downsplice, and get actula length in OUTbuffer
		destination = (char *)new_addr;
	
		for(k = permch[permno]; k < totlen; k+=outchans)			//	For mono src, all first chunk goes to some rand chan
			splicbuf[k] = *old_addr++;								//	while all other chans remain (preset) at zero
		permno++;													//	Proceed to next permd channel, rady for next chunk

		// NEW AUGUST 2010
		do_startsplice(splicbuf,outchans,dz);
		
		
		do_endsplice(splicbuf+totlen,outchans,dz);					//	splice end of segment in splicebuf
		memcpy(destination,(char *)splicbuf,totlen*sizeof(float));
		memset((char*)splicbuf,0,dz->buflen * sizeof(float));		//	copy splicebuf to output, and zero splicebuf
		/* MIDDLE-SEGMENTS IN BUFFER */								
		for(n=1;n<dz->iparam[MSHR_CHCNT_LESS_ONE];n++) {			//	For later segments, do the same, except
			old_addr  = dz->sampbuf[current_buf]  + chunkptr[permm[n]];
			new_addr += chnk_len * outchansratio;					//	Advance in outbuf by len of inchunk * no of outchans
			chnk_len  = chunklen[permm[n]];							
			totlen = (chnk_len + dz->iparam[MSHR_SPLEN]) * outchans;//	chunk gets written to next random-permd channel
			for(k = permch[permno]; k < totlen; k+=outchans) 
				splicbuf[k] = *old_addr++;
			if(++permno >= outchans) {								//	Proceed to next channel permd channel
				permute_chans(outchans,dz);							//	If all of perm used up, do a new perm
				permno = 0;											//	and start at start of new perm
			}
			do_startsplice(splicbuf,outchans,dz);					//	This time, splice start of chunk as well as
			do_endsplice(splicbuf+totlen,outchans,dz);				//	end of chunk
			for(k = 0;k < totlen;k++) {								//	Must now ADD the chunk into the output
				val = *(new_addr+k);								//	so that splices overlap
				val += splicbuf[k];
				*(new_addr+k) = (float)val;
			}
			memset((char*)splicbuf,0,dz->buflen * sizeof(float));
		}							/* f */
			/* NEW END-SEGMENT IN BUFFER */							//	For the final chunk, proceed as before
		old_addr  = dz->sampbuf[current_buf] + chunkptr[permm[n]];	
		new_addr += chnk_len;
		chnk_len  = chunklen[permm[n]];
		totlen = (chnk_len + dz->iparam[MSHR_SPLEN]) * outchans; 
		for(k = permch[permno]; k < totlen; k+=outchans) 
			splicbuf[k] = *old_addr++;
		do_startsplice(splicbuf,outchans,dz);						//	but this time, no end splice is needed

		// NEW AUGUST 2010
		do_endsplice(splicbuf+totlen,outchans,dz);				//	end of chunk

		for(k = 0;k < totlen;k++) {
			val = *(new_addr+k);
			val += splicbuf[k];
			*(new_addr+k) = (float)val;
		}
		memset((char*)splicbuf,0,dz->buflen * sizeof(float));
	} else {														//	For mono input AFTER first pass, chunks are now in multichan buf.
			/* NEW FIRST-SEGMENT IN BUFFER */						//	So chunk Location and Len are multiplied by outchanratio (outch/inchan)
																	//	For multichan input, outchanratio = 1, chunks are just standard size
		old_addr = dz->sampbuf[current_buf] + (chunkptr[permm[0]] * outchansratio);	// (Location)
		new_addr = dz->sampbuf[!current_buf];						//	For 1st chunk written, proceed as before, but ...
		chnk_len = chunklen[permm[0]];		/* c */
		totlen = (chnk_len + dz->iparam[MSHR_SPLEN]) * outchansratio; // (Len)
		destination = (char *)new_addr;
		memcpy((char *)splicbuf,(char *)old_addr,totlen*sizeof(float));	//	Simply copy the required chunk into the splicebuffer

		// NEW AUGUST 2010
		do_startsplice(splicbuf,outchans,dz);

		do_endsplice(splicbuf+totlen,outchans,dz);					//	and do end splice
		rotate(splicbuf,totlen,permch,rot,outchans);				//	Permute order of channels in entire chunk
		permute_chans(outchans,dz);									//	Get next channel-perm, ready for next chunk
		memcpy(destination,(char *)splicbuf,totlen*sizeof(float));
		memset((char*)splicbuf,0,dz->buflen * sizeof(float));		
			/* MIDDLE-SEGMENTS IN BUFFER */							//	subsequent chunks treated similarly
		for(n=1;n<dz->iparam[MSHR_CHCNT_LESS_ONE];n++) {
			old_addr  = dz->sampbuf[current_buf]  + (chunkptr[permm[n]]*outchansratio);
			new_addr += chnk_len * outchansratio;
			chnk_len = chunklen[permm[n]];
			totlen  = (chnk_len + dz->iparam[MSHR_SPLEN]) * outchansratio;
			memcpy((char *)splicbuf,(char *)old_addr,totlen*sizeof(float));
			do_startsplice(splicbuf,outchans,dz);					//	except now we also need a startsplice
			do_endsplice(splicbuf+totlen,outchans,dz);
			rotate(splicbuf,totlen,permch,rot,outchans);
			permute_chans(outchans,dz);
			for(k = 0;k < totlen;k++) {								//	and output must be ADDED into outbuf, so splices overlap
				val = *(new_addr+k);
				val += splicbuf[k];
				*(new_addr+k) = (float)val;
			}
			memset((char*)splicbuf,0,dz->buflen * sizeof(float));
		}							/* f */
			/* NEW END-SEGMENT IN BUFFER */							//	and final chunk treated siilarly
		old_addr  = dz->sampbuf[current_buf] + (chunkptr[permm[n]] * outchansratio);
		new_addr += chnk_len * outchansratio;
		chnk_len = chunklen[permm[n]];
		totlen  = (chnk_len + dz->iparam[MSHR_SPLEN]) * outchansratio;
		memcpy((char *)splicbuf,(char *)old_addr,totlen*sizeof(float));
		do_startsplice(splicbuf,outchans,dz);						//	except NO endsplice required

		// NEW AUGUST 2010
		do_endsplice(splicbuf+totlen,outchans,dz);
		
		rotate(splicbuf,totlen,permch,rot,outchans);				//	Permute order of channels in entire chunk
		for(k = 0;k < totlen;k++) {
			val = *(new_addr+k);
			val += splicbuf[k];
			*(new_addr+k) = (float)val;
		}
		memset((char*)splicbuf,0,dz->buflen * sizeof(float));
	}
				 /* RESET BUFFERS */								//	Set to zero (what will become) output buffer, for next pass
	memset((char *)dz->sampbuf[current_buf],0,dz->buflen * sizeof(float)); 
	return(FINISHED);
}

/*********************** GET_BASIS_LENGTHS **********************
 *
 *
 *    ______________buflen_______________
 *   |............worklen.............   |  buflen - SPLICELEN  = worklen.
 *   | unitlen unitlen		      	  |  |  unit_len * dz->iparam[MSHR_CHCNT] = worklen.
 *   |	      |	      | 	          |  |  
 *   |_full_len_      |       _full_len__|  full_len = worklen + SPLICELEN.
 *   |	      |	|     |      |	      |  |
 *   |   _____| |_____|  ____|   _____|  |
 *   |  /|    \ /     \ /     \ /     \  |
 *   | / |    |X|      X       X       \ |
 *   |/  |    / \     / \     / \       \|
 *   |	 |    | 
 *   |	 |    | 
 *       rawlen
 *
 *
 */

int get_basis_lengths(int *work_len,dataptr dz)
{   
	int inbuflen;
	int excess, full_len, endunit_len, endfull_len, chans = dz->infile->channels;
	if(dz->mode == 0)
		inbuflen = dz->buflen/dz->iparam[MSHR_OUTCHANS];
	else
		inbuflen = dz->buflen;
	*work_len = inbuflen - (dz->iparam[MSHR_SPLEN] * chans);
	dz->iparam[MSHR_UNITLEN] = (int)round((*work_len)/dz->iparam[MSHR_CHCNT]);
	excess = dz->iparam[MSHR_UNITLEN] % chans;
	dz->iparam[MSHR_UNITLEN] -= excess;
	excess   = (*work_len) - (dz->iparam[MSHR_UNITLEN] * dz->iparam[MSHR_CHCNT]);
	if(excess % chans) {
		sprintf(errstr,"Problem in buffer accounting.\n");
		return(PROGRAM_ERROR);
	}
	dz->iparam[MSHR_RAWLEN]  = dz->iparam[MSHR_UNITLEN] - dz->iparam[MSHR_SPLEN];
	full_len = dz->iparam[MSHR_UNITLEN] + dz->iparam[MSHR_SPLEN];
	endunit_len = dz->iparam[MSHR_UNITLEN] + excess;
	endfull_len = full_len + excess;
	dz->iparam[MSHR_ENDRAWLEN]  = dz->iparam[MSHR_RAWLEN]  + excess;
	if(full_len < (dz->iparam[MSHR_SPLEN] /*  * 2 */) || endfull_len < (dz->iparam[MSHR_SPLEN]/* * 2 */)) {
		sprintf(errstr,"Chunksizes %d and %d too small for splices. mshr_splen = %d\n",full_len,endfull_len,dz->iparam[MSHR_SPLEN]);
		return(DATA_ERROR);
	}
	if(dz->iparam[MSHR_SCAT]) {
		dz->iparam[MSHR_SCATGRPCNT] = (int)(dz->iparam[MSHR_CHCNT]/dz->iparam[MSHR_SCAT]);
		dz->iparam[MSHR_ENDSCAT]    = (int)(dz->iparam[MSHR_CHCNT] - (dz->iparam[MSHR_SCATGRPCNT] * dz->iparam[MSHR_SCAT]));
		dz->iparam[MSHR_RANGE]      = dz->iparam[MSHR_UNITLEN] * dz->iparam[MSHR_SCAT];
		dz->iparam[MSHR_ENDRANGE]   = ((dz->iparam[MSHR_ENDSCAT]-1) * dz->iparam[MSHR_UNITLEN]) + endunit_len;
	}
	return(FINISHED);
}

/************************** NORMAL_SCAT ******************************
 *
 * (1)	TOTLA_LEN generates the unscattered positions of the chunks.
 *	Each is UNIT_LEN long, so they are equally spaced at UNIT_LEN
 *	intervals.
	We can't scatter the FIRST chunk as it MUST start at ZERO!!
 * (2)	For all chunks except the first and last...
 * (3)	    Scatter position of chunk over +- 1/2 of value of scatter,
 *	    times the RAW-distance (not including splices) between chunks.
 * (4)      Add (could be negative) this scattering to orig position.
 * (5)	For the last chunk, do the same, scattering over RAW-len of previous
 *	chunk, if scatter -ve, and over endraw_len of final chunk, if +ve.
 */

void normal_scat(dataptr dz)
{   
	double this_scatter;
	int n, k;
	int chunkscat, total_len = dz->iparam[MSHR_UNITLEN];	/* 1 */
	int *chunkptr = dz->lparray[MSHR_CHUNKPTR];
	for(n=1;n<dz->iparam[MSHR_CHCNT_LESS_ONE];n++) {			/* 2 */
		this_scatter  = (drand48() - 0.5) * dz->param[MSHR_SCAT];
		chunkscat = (int)(this_scatter * (double)dz->iparam[MSHR_RAWLEN]);
		k = chunkscat % dz->infile->channels;
		chunkscat -= k;
		chunkptr[n] = total_len + chunkscat;
		total_len  += dz->iparam[MSHR_UNITLEN];				/* 4 */
	}
	this_scatter  = (drand48() - 0.5) * dz->param[MSHR_SCAT];
	if(this_scatter<0.0) {					/* 5 */
		chunkscat   = (int)(this_scatter * (double)dz->iparam[MSHR_RAWLEN]);
		k = chunkscat % dz->infile->channels;
		chunkscat -= k;
		chunkptr[n] = total_len - chunkscat;
	} else {
		chunkscat   = (int)(this_scatter * (double)dz->iparam[MSHR_ENDRAWLEN]);
		k = chunkscat % dz->infile->channels;
		chunkscat -= k;
		chunkptr[n] = total_len + chunkscat;
	}
}

/*********************** HEAVY_SCAT ***************************
 *
 * (1)	Start at the chunk (this=1) AFTER the first (which can't be moved).
 * (2) 	STARTPTR marks the start of the chunk GROUP (and will be advanced
 *	by RANGE, which is length of chunk-group).
 * (3)	The loop will generate a set of positions for the chunks in
 *	a chunk-group. In the first chunkgroup the position of the
 *	first chunk (start of file) can't be moved, so loop starts at
 *	(first=) 1. Subsequemt loop passes start at 0.
 * (4)	For eveery chunk-group.
 * (5)	  Set the index of the first chunk in this group (start) to the
 *	  current index (this).
 * (6)	  For every member of this chunk-group.
 * (7)	    Generate a random-position within the chunk-grp's range
 *	    and check it is not too close ( < SPLICELEN) to the others.
 *	      Set a checking flag (OK).
 * (8)	      Generate a position within the range, and after the startptr.
 * (9)		Compare it with all previously generated positions in this
 *		chunk-grp AND with last position of previous chunk-group!!
 * 		If it's closer than SPLICELEN, set OK = 0, drop out of
		checking loop and generate another position instead.
 * (10)	      If the position is OK, drop out of position generating loop..
 * (11)	    Advance to next chunk in this group.
 * (12)	  Once all this group is done, advance the group startpoint by RANGE.
 * (13)	  After FIRST grp, all positions can by varied, so set the initial
 *	  loop counter to (first=)0.
 * (14)	If there are chunks left over (endscat!=0)..
 *	  Follow the same procedure for chunks in end group, using the
 *	  alternative variables, endscat and endrange.
 */

void heavy_scat(dataptr dz)
{   
	int	thiss = 1, that, start, n, m, OK;						/* 1 */
	int k;
	int startptr = 0;											/* 2 */
	int endptr = 0;
	int first = 1;												/* 3 */
	int *chunkptr = dz->lparray[MSHR_CHUNKPTR];
	for(n=0;n<dz->iparam[MSHR_SCATGRPCNT];n++) {					/* 4 */
		start = thiss;											/* 5 */
		endptr += dz->iparam[MSHR_RANGE];
		for(m=first;m<dz->iparam[MSHR_SCAT];m++) {				/* 6 */
			do {												/* 7 */
				OK = 1;					
				chunkptr[thiss] = (int)(drand48()*dz->iparam[MSHR_RANGE]); 	/* TRUNCATE (?)*/
				chunkptr[thiss] += startptr; 					/* 8 */
				k = chunkptr[thiss] % dz->infile->channels;
				chunkptr[thiss] -= k;
				for(that=start-1; that<thiss; that++) {
					if(abs(chunkptr[thiss] - chunkptr[that])<dz->iparam[MSHR_SPLEN]) {
						OK = 0;									/* 9 */
						break;
					}
					if(abs(endptr - chunkptr[thiss])<dz->iparam[MSHR_SPLEN]) {
						OK = 0;
						break;
					}
				}
			} while(!OK);										/* 10 */
			thiss++;											/* 11 */
		}
		startptr += dz->iparam[MSHR_RANGE];						/* 12 */
		first = 0;												/* 13 */
	}

	endptr += dz->iparam[MSHR_ENDRANGE];

	if(dz->iparam[MSHR_ENDSCAT]) {								/* 14 */
		start = thiss;
		for(m=0;m<dz->iparam[MSHR_ENDSCAT];m++) {
			do {
				OK = 1;
				chunkptr[thiss] = (int)(drand48() * dz->iparam[MSHR_ENDRANGE]);	  /* TRUNCATE (?) */
				chunkptr[thiss] += startptr;
				k = chunkptr[thiss] % dz->infile->channels;
				chunkptr[thiss] -= k;
				for(that=start-1; that<thiss; that++) {
					if(abs(chunkptr[thiss] - chunkptr[that])<dz->iparam[MSHR_SPLEN]) {
						OK = 0;
						break;
					}
					if(abs(endptr - chunkptr[thiss])<dz->iparam[MSHR_SPLEN]) {
						OK = 0;
						break;
					}
				}
			} while(!OK);
			thiss++;
		}
	}
	ptr_sort(thiss,dz);
}

/*************************** PERMUTE_CHUNKS ***************************/

void permute_chunks(dataptr dz)
{   
	int n, t;
	for(n=0;n<dz->iparam[MSHR_CHCNT];n++) {
		t = (int)(drand48() * (double)(n+1));	 /* TRUNCATE */
		if(t==n)
			prefix(n,dz);
		else
			insert(n,t,dz);

	}
}

/****************************** INSERT ****************************/

void insert(int n,int t,dataptr dz)
{   
	shuflup(t+1,dz);
	dz->iparray[MSHR_PERM][t+1] = n;
}

/****************************** PREFIX ****************************/

void prefix(int n,dataptr dz)
{   
	shuflup(0,dz);
	dz->iparray[MSHR_PERM][0] = n;
}

/****************************** SHUFLUP ****************************/

void shuflup(int k,dataptr dz)
{   
	int n;
	for(n = dz->iparam[MSHR_CHCNT_LESS_ONE]; n > k; n--)
		dz->iparray[MSHR_PERM][n] = dz->iparray[MSHR_PERM][n-1];
}

/************************* DO_STARTSPLICE *************************
 *
 * (0)	For each position in splice.
 * (1)	Get the value from the source file & multiply it by position in splice.
 *	We 'should' multiply it by position/SPLICELEN,
 *	so new value is SPLICELEN too large.
 * (2)	Add a rounding factor (if value were correct, this would be 1/2)
 *	But everything is SPLICELEN times too big, so add SHRED_HSPLICELEN.
 * (3)	Divide by SPLICELEN.
 * (4)	ADD this value to existing value (this is a splice TO another chunk!!).
 */

void do_startsplice(float *i,int chans,dataptr dz)
{   
	double z;
	int n, k;
	for(n = 0; n <dz->iparam[MSHR_SPLEN]; n ++) {			/* 0 */
		for(k = 0;k < chans; k++) {
			z  = (*i) * (double)n/(double)dz->iparam[MSHR_SPLEN];	/* 1 */
			*i = (float)z;									/* 4 */
			i++;
		}
	}
}

/************************* DO_ENDSPLICE *************************/

void do_endsplice(float *i,int chans,dataptr dz)
{   
	double z;
	int n, k;
	i--;
	for(n = 0; n < dz->iparam[MSHR_SPLEN];n++) {
		for(k = 0; k < chans; k++) {
			z  = *i * (double)n/(double)dz->iparam[MSHR_SPLEN];
			*i = (float)z;
			i--;
		}
	}
}

/********************* DO_BUFEND_SPLICES *************************/

void do_bufend_splices(int current_buf,dataptr dz)
{
	double z;
	int  n, k, outchans;
	float *b = dz->sampbuf[current_buf];
	if(dz->mode == 0)
		outchans = dz->iparam[MSHR_OUTCHANS];
	else
		outchans = dz->infile->channels;
	for(n = 0; n <dz->iparam[MSHR_SPLEN]; n ++) {
		for(k = 0;k<outchans;k++) {
			z     = (*b) * (double)n/(double)dz->iparam[MSHR_SPLEN];
			*b++  = (float)z;
		}
	}
	b = dz->sampbuf[current_buf] + dz->buflen;
	b--;
	for(n = 0; n <dz->iparam[MSHR_SPLEN]; n ++) {
		for(k = 0;k<outchans;k++) {
			z     = (*b) * (double)n/(double)dz->iparam[MSHR_SPLEN];
			*b--  = (float)z;
		}
	}
}

/************************** PTR_SORT ***************************/

void ptr_sort(int end,dataptr dz)
{   
	int i,j;
	int a;
	int *chunkptr = dz->lparray[MSHR_CHUNKPTR];
	for(j=1;j<end;j++) {
		a = chunkptr[j];
		i = j-1;
		while(i >= 0 && chunkptr[i] > a) {
			chunkptr[i+1]=chunkptr[i];
			i--;
		}
		chunkptr[i+1] = a;
	}
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
	dz->iparray[MSHR_PERMCH][t+1] = n;
}

/****************************** PREFIX ****************************/

void prefixch(int n,int outchans,dataptr dz)
{   
	shuflupch(0,outchans,dz);
	dz->iparray[MSHR_PERMCH][0] = n;
}

/****************************** SHUFLUPCH ****************************/

void shuflupch(int k,int outchans,dataptr dz)
{   
	int n;
	for(n = outchans - 1; n > k; n--)
		dz->iparray[MSHR_PERMCH][n] = dz->iparray[MSHR_PERMCH][n-1];
}

/****************************** ROTATE ****************************
 *
 * Assign each N-chan chunk to a permuted set of channels.
 */

void rotate(float *splicbuf,int len,int *permch,float *rot,int outchans)
{	
	int k, n, blokcnt = len/outchans;
	for(k=0;k<blokcnt;k++) {
		for(n = 0;n<outchans;n++)
			rot[n] = splicbuf[permch[n]];
		for(n = 0;n<outchans;n++)
			splicbuf[n] = rot[n];
		splicbuf += outchans;
	}
}

/***************************** SETUP_INTERNAL_ARRAYS_AND_ARRAY_POINTERS **************************/

int setup_internal_arrays_and_array_pointers(dataptr dz)
{
	int n;		 
	dz->fptr_cnt   = 1;
	dz->iarray_cnt = 2;
	dz->larray_cnt = 2;
	if((dz->iparray = (int     **)malloc(dz->iarray_cnt * sizeof(int *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for internal int arrays.\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<dz->iarray_cnt;n++)
		dz->iparray[n] = NULL;

	if((dz->lparray = (int    **)malloc(dz->larray_cnt * sizeof(int *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for internal long arrays.\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<dz->larray_cnt;n++)
		dz->lparray[n] = NULL;

	if((dz->fptr = (float  **)malloc(dz->fptr_cnt * sizeof(float *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for internal float-pointer arrays.\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<dz->fptr_cnt;n++)
		dz->fptr[n] = NULL;

	return(FINISHED);
}

/***************************** SET_LEGAL_INTERNALPARAM_STRUCTURE **************************/

int set_legal_internalparam_structure(int process,int mode,aplptr ap)
{
	int exit_status;
	exit_status = set_internalparam_data("iiiiiiiiiiiii",ap);
	return(exit_status);		
}
