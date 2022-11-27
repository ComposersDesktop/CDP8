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
NB Parameter 1 should be the pitch-brkpnt data !!
USAGE:
oneform get informfile out1ffile time
oneform put 1-2 infile1 1ffile outfile
oneform combine pitchfile 1ffile outfile
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
#include <speccon.h>

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

#define WINDIV (6.0)
#define BUMZEROCNT (20)		/* No of zeros that can occur WITHIN a genuine signal */
#define SMOOTHWIN (40)		/* Max no samples to smooth if glitch at end */
#define PITCHERROR (1.5)
/* windows are three times smaller than the pitch-cycle */

//CDP LIB REPLACEMENTS
static int setup_oneform_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_oneform_param_ranges_and_defaults(dataptr dz);
static int handle_the_outfile(int *cmdlinecnt,char ***cmdline,dataptr dz);
static int setup_oneform_application(dataptr dz);
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

static int	handle_the_extra_infiles(char ***cmdline,int *cmdlinecnt,dataptr dz);
static int	create_oneform_bufs(dataptr dz);
static int	allocate_two_formant_buffers(dataptr dz);
static int	setup_the_formant_params(int fd,dataptr dz);
static int	apply_formant(int windows_in_buf,dataptr dz);

static int	oneform_get(dataptr dz);
static int	oneform_put(dataptr dz);
static int	oneform_combine(dataptr dz);
static int	setup_channel_midfrqs(dataptr dz);
static int	check_formant_buffer(dataptr dz);
static int	preset_channels(dataptr dz);
static int	write_pitched_window(double fundamental,double *amptotal,dataptr dz);
static int	write_silent_window(dataptr dz);
static int	normalise_in_file(double normaliser,dataptr dz);
static int	write_unpitched_window(double *amptotal,dataptr dz);
static int	do_specform(dataptr dz);
static int	form_filter(dataptr dz);
static int	form_gain(dataptr dz);
static char *get_the_other_filename_xx(char *filename,char c);
static void get_the_other_filename(char *filename,char c);
static int	readformhead(char *str,int ifd,dataptr dz);
static int get_the_mode_from_cmdline(char *str,dataptr dz);

extern int	getspecenvamp(double *thisamp,double thisfrq,int storeno,dataptr dz);
extern int extract_specenv(int bufptr_no,int storeno,dataptr dz);
extern int get_channel_corresponding_to_frq(int *chan,double thisfrq,dataptr dz);

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
		if(dz->process == ONEFORM_PUT) {
			dz->maxmode = 2;
			if((get_the_mode_from_cmdline(cmdline[0],dz))<0) {
				print_messages_and_close_sndfiles(exit_status,is_launched,dz);
				return(FAILED);
			}
			cmdline++;
			cmdlinecnt--;
		}
		// setup_particular_application =
		if((exit_status = setup_oneform_application(dz))<0) {
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
	if((exit_status = setup_oneform_param_ranges_and_defaults(dz))<0) {
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

//	handle_extra_infiles() ......
	if((exit_status = handle_the_extra_infiles(&cmdline,&cmdlinecnt,dz))<0) {	
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);	
		return(FAILED);
	}
	// handle_outfile() = 
	if((exit_status = handle_the_outfile(&cmdlinecnt,&cmdline,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
//	handle_formants()	redundant
	if(dz->process == ONEFORM_PUT) {
		if((exit_status = handle_formant_quiksearch(&cmdlinecnt,&cmdline,dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
	}
	dz->infile->specenvcnt = dz->specenvcnt;
	if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {		// CDP LIB
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
//	check_param_validity_and_consistency()		redundant
	is_launched = TRUE;
	switch(dz->process) {
	case(ONEFORM_GET):		dz->bptrcnt = 2; break;
	case(ONEFORM_PUT):		dz->bptrcnt = 4; break;
	case(ONEFORM_COMBINE):	dz->bptrcnt = 4; break;
	}
	if((dz->flbufptr = (float **)malloc(sizeof(float *) * dz->bptrcnt))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for float sample buffers.\n");
		return(MEMORY_ERROR);
	}
	for(n = 0;n <dz->bptrcnt; n++)
		dz->flbufptr[n] = NULL;
	if((exit_status = create_oneform_bufs(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	//param_preprocess()						redundant
	//spec_process_file =
	switch(dz->process) {
	case(ONEFORM_GET):		exit_status = oneform_get(dz);		break;
	case(ONEFORM_PUT):		exit_status = oneform_put(dz);		break;
	case(ONEFORM_COMBINE):	exit_status = oneform_combine(dz);	break;
	}
	if(exit_status<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = complete_output(dz))<0) {
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
	if(!sloom) {
		if(file_has_invalid_startchar(filename) || value_is_numeric(filename)) {
			sprintf(errstr,"Outfile name %s has invalid start character(s) or looks too much like a number.\n",filename);
			return(DATA_ERROR);
		}
	}
	if(dz->process == ONEFORM_COMBINE) {
		dz->all_words = 0;									
		if((exit_status = store_filename(filename,dz))<0)
			return(exit_status);
		if(!sloom) {
			if((filename = get_the_other_filename_xx(filename,'1')) == NULL) {
				sprintf(errstr,"Insufficient memory to store modified outfilename.\n");
				return MEMORY_ERROR;
			}
		} else {
			get_the_other_filename(filename,'1');
		}
	}
	dz->needpeaks = 0;
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

/************************* SETUP_ONEFORM_APPLICATION *******************/

int setup_oneform_application(dataptr dz)
{
	int exit_status;
	aplptr ap;
	if((exit_status = establish_application(dz))<0)		// GLOBAL
		return(FAILED);
	ap = dz->application;
	// SEE parstruct FOR EXPLANATION of next 2 functions
	switch(dz->process) {
	case(ONEFORM_GET):
		if((exit_status = set_param_data(ap,0   ,1,1,"d"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","",0,0,""))<0)
			return(FAILED);
		dz->input_data_type = FORMANTFILE_ONLY;
		dz->process_type	= EQUAL_FORMANTS;		/* Not 'equal' but this assignation worksd! */	
		dz->outfiletype  	= FORMANTS_OUT;
		break;
	case(ONEFORM_PUT):
		if((exit_status = set_param_data(ap,0   ,0,0,""))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"lhg",3,"ddd","",0,0,""))<0)
			return(FAILED);
		dz->input_data_type = ANAL_AND_FORMANTS;
		dz->process_type	= BIG_ANALFILE;
		dz->outfiletype  	= ANALFILE_OUT;
		break;
	case(ONEFORM_COMBINE):
		if((exit_status = set_param_data(ap,0   ,0,0,""))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","",0,0,""))<0)
			return(FAILED);
		dz->input_data_type = PITCH_AND_FORMANTS;
		dz->process_type	= PITCH_TO_ANAL;
		dz->outfiletype  	= ANALFILE_OUT;
		break;
	}
	dz->has_otherfile = FALSE;
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
		}
		switch(dz->process) {
		case(ONEFORM_GET):
			if(infile_info->filetype != FORMANTFILE)  {
				sprintf(errstr,"File %s is not of correct type\n",cmdline[0]);
				return(DATA_ERROR);
			}
			break;
		case(ONEFORM_PUT):
			if(infile_info->filetype != ANALFILE)  {
				sprintf(errstr,"File %s is not of correct type\n",cmdline[0]);
				return(DATA_ERROR);
			}
			break;
		case(ONEFORM_COMBINE):
			if(infile_info->filetype != PITCHFILE)  {
				sprintf(errstr,"File %s is not of correct type\n",cmdline[0]);
				return(DATA_ERROR);
			}
			break;
		}
		if((exit_status = copy_parse_info_to_main_structure(infile_info,dz))<0) {
			sprintf(errstr,"Failed to copy file parsing information\n");
			return(PROGRAM_ERROR);
		}
		free(infile_info);
	}
	return(FINISHED);
}

/************************* SETUP_ONEFORM_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_oneform_param_ranges_and_defaults(dataptr dz)
{
	int exit_status;
	aplptr ap = dz->application;
	// set_param_ranges()
	ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
	// NB total_input_param_cnt is > 0 !!!s
	if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
		return(FAILED);
	// get_param_ranges()
	switch(dz->process) {
	case(ONEFORM_GET):
		ap->lo[0]			= 0.0;
		ap->hi[0]			= dz->duration;
		ap->default_val[0]	= dz->duration/2.0;
		dz->maxmode = 0;
		break;
	case(ONEFORM_PUT):
		ap->formant_qksrch = TRUE;
		ap->lo[FORM_FTOP]			= PITCHZERO;
		ap->hi[FORM_FTOP]			= dz->nyquist;
		ap->default_val[FORM_FTOP]	= dz->nyquist;
		ap->lo[FORM_FBOT]			= PITCHZERO;
		ap->hi[FORM_FBOT]			= dz->nyquist;
		ap->default_val[FORM_FBOT]	= PITCHZERO;
		ap->lo[FORM_GAIN]			= FLTERR;
		ap->hi[FORM_GAIN]			= FORM_MAX_GAIN;
		ap->default_val[FORM_GAIN]	= 1.0;
		dz->maxmode = 2;
		break;
	case(ONEFORM_COMBINE):
		dz->maxmode = 0;
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
			if((exit_status = setup_oneform_application(dz))<0)
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


/******************************** USAGE ********************************/

int usage1()
{
	fprintf(stderr,
	"\nOPERATIONS WITH SINGLE FORMANTS\n\n"
    "USAGE: oneform name (mode) infile(s) outfile params\n"
	"\n"
	"where NAME can be any one of\n"
	"\n"
	"get   put   combine\n\n"
	"Type 'oneform get' for more info on 'oneform get'..ETC.\n");
	return(USAGE_ONLY);
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
	if(!strcmp(str,"get")) {
		fprintf(stderr,
	    "USAGE:\n"
	    "oneform get informantfile 1f-outfile time\n"
 		"\n"
		"Extract formants at a specific time in a formant file.\n"
		"\n"
		"INFORMANTFILE  A formant data file.\n"
		"1F-OUTFILE     A single-moment-formants datafile.\n"
		"TIME           Time in inputfile where single-moment-formants are to be extracted.\n"
		"\n");
	} else if(!strcmp(str,"put")) {
		fprintf(stdout,
	    "USAGE:\n"
	    "oneform put 1-2 inanalfile 1f-infile outanalfile [-llolim] [-hhighlim] [-ggain]\n"
		"\n"
		"Impose formant in a single-moment-formants datafile on an analysis file.\n"
		"\n"
		"In mode 1: the single-moment-formant is imposed on the analysis file.\n"
		"In mode 2: attempts to replace analysis file formants with single-moment-formant.\n"
		"\n"
		"INANALFILE   An analysis file.\n"
		"1F-INFILE    A single-moment-formant datafile.\n"
		"OUTANALFILE  The new analysis file generated.\n"
		"LOLIM        Low frequency limit, set spectrum to zero below this limit.\n"
		"HILIM        High frequency limit, set spectrum to zero above this limit.\n"
		"GAIN         Overall gain on output.\n"
		"\n");
	} else if(!strcmp(str,"combine")) {
		fprintf(stdout,
	    "USAGE:\n"
	    "oneform combine pitchfile 1f-infile outanalfile\n"
		"\n"
		"Generate new sound from pitch data and single-moment-formants data.\n"
		"\n"
		"PITCHFILE    A binary pitchfile.\n"
		"1F-INFILE    A single-moment-formant datafile.\n"
		"OUTANALFILE  The new analysis file generated.\n"
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

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	if      (!strcmp(prog_identifier_from_cmdline,"get"))				dz->process = ONEFORM_GET;
	else if (!strcmp(prog_identifier_from_cmdline,"put"))				dz->process = ONEFORM_PUT;
	else if (!strcmp(prog_identifier_from_cmdline,"combine"))			dz->process = ONEFORM_COMBINE;
	else {
		sprintf(errstr,"Unknown program identification string '%s'\n",prog_identifier_from_cmdline);
		return(USAGE_ONLY);
	}
	return(FINISHED);
}

/*************************** CREATE_ONEFORM_BUFS **************************/

int create_oneform_bufs(dataptr dz)
{
	if(dz->process == ONEFORM_GET)
		return allocate_two_formant_buffers(dz);
	if(dz->process == ONEFORM_COMBINE)
		dz->wanted = dz->infile->origchans;
	else
		dz->wanted = dz->infile->channels;
	return allocate_analdata_plus_formantdata_buffer(dz);
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

/************************ HANDLE_THE_EXTRA_INFILES *********************/

int handle_the_extra_infiles(char ***cmdline,int *cmdlinecnt,dataptr dz)
{
					/* OPEN ANY FURTHER INFILES, CHECK COMPATIBILITY, STORE DATA AND INFO */
	int  n, exit_status;
	char *filename;
	int wlengthf, wanted = 0, clength;
	int arraycnt;

	if(dz->infilecnt > 1) {
		for(n=1;n<dz->infilecnt;n++) {
			filename = (*cmdline)[0];
			if((dz->ifd[n] = sndopenEx(filename,0,CDP_OPEN_RDONLY)) < 0) {
				sprintf(errstr,"cannot open input file %s to read data.\n",filename);
				return(DATA_ERROR);
			}
			if((exit_status = readformhead(*(cmdline)[0],dz->ifd[n],dz))<0)
				return(exit_status);
			if((dz->insams[n] = sndsizeEx(dz->ifd[n])) <= (int)(DESCRIPTOR_DATA_BLOKS * dz->specenvcnt)) {
				sprintf(errstr, "File %s contains no formant data.\n",filename);
				return(DATA_ERROR);
			}
			if((wlengthf = (dz->insams[n]/dz->specenvcnt) - DESCRIPTOR_DATA_BLOKS)<=0) {
				sprintf(errstr,"No data in formant file %s\n",filename);
				return(DATA_ERROR);
			}
			if((wlengthf = (dz->insams[n]/dz->specenvcnt) - DESCRIPTOR_DATA_BLOKS)<=0) {
				sprintf(errstr,"No data in formant file %s\n",filename);
				return(DATA_ERROR);
			}
			if((wlengthf = (dz->insams[n]/dz->specenvcnt) - DESCRIPTOR_DATA_BLOKS)!=1) {
				sprintf(errstr,"file %s is not data for a SINGLE-formant\n",filename);
				return(DATA_ERROR);
			}
			dz->descriptor_samps = dz->specenvcnt * DESCRIPTOR_DATA_BLOKS;
			switch(dz->infile->filetype) {
			case(ANALFILE):	
				wanted 	= dz->infile->channels;	
				break;
			case(PITCHFILE):
				wanted 	= dz->infile->origchans;
				break;
			}
			clength = wanted/2;
			arraycnt   = clength + 1;
			if((dz->specenvfrq = (float *)malloc((arraycnt) * sizeof(float)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY for formant frq array.\n");
				return(MEMORY_ERROR);
			}
			if((dz->specenvpch = (float *)malloc((arraycnt) * sizeof(float)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY for formant pitch array.\n");
				return(MEMORY_ERROR);
			}
			/*RWD  zero the data */
			memset(dz->specenvpch,0,arraycnt * sizeof(float));

			if((dz->specenvamp = (float *)malloc((arraycnt) * sizeof(float)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY for formant aplitude array.\n");
				return(MEMORY_ERROR);
			}
			if((dz->specenvtop = (float *)malloc((arraycnt) * sizeof(float)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY for formant frq limit array.\n");
				return(MEMORY_ERROR);
			}
			(*cmdline)++;
			(*cmdlinecnt)--;
		}
 	}
	return(FINISHED);
}


int oneform_get(dataptr dz)
{
	int exit_status;
	int   windows_in_buf, wc, descripblok;
	int outcnt = 0;
	float *outbuf = dz->flbufptr[1];
	if(sloom)
		dz->total_samps_read = 0L;
	if((exit_status = read_samps(dz->bigfbuf,dz)) < 0)
		return(exit_status);

	// READ SEPCENV HEAD DATA
	if(dz->ssampsread < dz->descriptor_samps) {
		sprintf(errstr,"No significant data in formantfile.\n");
		return(DATA_ERROR);
	}
	dz->flbufptr[0] = dz->bigfbuf;
	descripblok = 2 * dz->infile->specenvcnt;
	memcpy((char *)dz->flbufptr[1],(char *)dz->flbufptr[0],(descripblok * sizeof(float))); 
	dz->flbufptr[0] += descripblok;
	dz->flbufptr[1] += descripblok;
	outcnt = descripblok;
	windows_in_buf = (dz->ssampsread - descripblok)/dz->infile->specenvcnt;    
	dz->time = 0.0f;
	do {
		wc = 0;
		while(wc < windows_in_buf) {
			if(dz->time >= dz->param[0]) {
				memcpy((char *)dz->flbufptr[1],(char *)dz->flbufptr[0],dz->infile->specenvcnt * sizeof(float));
				outcnt += dz->infile->specenvcnt;
				if((exit_status = write_samps(outbuf,outcnt,dz))<0)
					return(exit_status);
				return(FINISHED);
			}
			dz->flbufptr[0] += dz->infile->specenvcnt;		/* move along bigfbuf */
			dz->time = (float)(dz->time + dz->frametime);
			wc++;
		}
		if((exit_status = read_samps(dz->bigfbuf,dz)) < 0) {
			sprintf(errstr,"Sound read error.\n");
			return(SYSTEM_ERROR);
		}
		dz->flbufptr[0] = dz->bigfbuf;
		windows_in_buf = dz->ssampsread/dz->infile->specenvcnt;
	} while(windows_in_buf > 0);
	sprintf(errstr,"FAILED TO FIND THE SPECIFIED FORMANT.\n");
	return(GOAL_FAILED);
}

int oneform_put(dataptr dz)
{
	int exit_status, current_status;
	int windows_in_buf;
 
	if(sloom)
		dz->total_samps_read = 0L;

	dz->time = 0.0f;
	if((exit_status = setup_the_formant_params(dz->ifd[1],dz))<0)
		return(exit_status);
	dz->specenvamp2 = dz->flbufptr[1];
	dz->clength		= dz->wanted / 2;
	dz->chwidth 	= dz->nyquist/(double)(dz->clength-1);
	dz->halfchwidth = dz->chwidth/2.0;
	if(sloom)
		dz->total_samps_read = 0L;
	if((exit_status = read_samps(dz->bigfbuf,dz)) < 0)
		return(exit_status);
	while(dz->ssampsread > 0) {
    	dz->flbufptr[0] = dz->bigfbuf;
    	windows_in_buf = dz->ssampsread/dz->wanted;
	   	if((exit_status = apply_formant(windows_in_buf,dz))<0) {
			dz->specenvamp2 = (float *)0;
			return(exit_status);
		}
		current_status = exit_status;
		if(dz->flbufptr[0] - dz->bigfbuf > 0) {
			if((exit_status = write_samps(dz->bigfbuf,(dz->flbufptr[0] - dz->bigfbuf),dz))<0) {
				dz->specenvamp2 = NULL;
				return(exit_status);
			}
		}
		if((exit_status = read_samps(dz->bigfbuf,dz)) < 0)
			return(exit_status);
	}
 	dz->specenvamp2 = NULL;
	return(FINISHED);
}


int oneform_combine(dataptr dz)
{
#define MAXANALAMP	(1.0)

	int exit_status;
	double normaliser, amptotal, thisfrq, maxamptotal = 0.0;

	dz->wanted      = dz->infile->origchans;	/* to reconstruct an analfile !! */
    dz->clength		= dz->wanted / 2;
	if((dz->freq = (float *)malloc(dz->clength * sizeof(float)))==NULL) {
		sprintf(errstr,"NO MEMORY FOR INTERMEDIATE FRQ BUFFERS.\n");
		return(MEMORY_ERROR);
	}
	dz->nyquist		= (double)dz->infile->origrate/2.0;
	dz->chwidth 	= dz->nyquist/(double)(dz->clength-1);
	dz->halfchwidth = dz->chwidth/2.0;
//	read_samps() ALREADY DONE IN setup_the_formant_params()
	if((exit_status = setup_the_formant_params(dz->ifd[1],dz))<0)
		return(exit_status);
	dz->flbufptr[1] = dz->flbufptr[2] + dz->descriptor_samps;
	if((exit_status = setup_channel_midfrqs(dz))<0)
		return(exit_status);
	if((exit_status = check_formant_buffer(dz))<0)
		return(exit_status);
	dz->flbufptr[0] = dz->bigfbuf;
 	while(dz->total_windows < dz->wlength){
		amptotal = 0.0;
		if((exit_status = preset_channels(dz))<0)
			return(exit_status);
		if((thisfrq = dz->pitches[dz->total_windows])>0.0) {
			if((exit_status = write_pitched_window(thisfrq,&amptotal,dz))<0)
				return(exit_status);
		} else if(flteq(thisfrq,NOT_SOUND)) {
			amptotal = 0.0;
			if((exit_status = write_silent_window(dz))<0)
				return(exit_status);
		} else {
			if((exit_status = write_unpitched_window(&amptotal,dz))<0)
				return(exit_status);
		}

		if(amptotal > maxamptotal)
			maxamptotal = amptotal;
		if((dz->flbufptr[0] += dz->wanted) >= dz->flbufptr[2]) {
			if((exit_status = write_exact_samps(dz->bigfbuf,dz->buflen,dz))<0)
				return(exit_status);
			dz->flbufptr[0] = dz->bigfbuf;
		}
		dz->total_windows++;
	}
	if(dz->flbufptr[0] != dz->bigfbuf) {
		if((exit_status = write_samps(dz->bigfbuf,(dz->flbufptr[0] - dz->bigfbuf),dz))<0)
			return(exit_status);
	}
	if(maxamptotal > MAXANALAMP)
		normaliser = MAXANALAMP/maxamptotal;
	else
		normaliser = 1.0;
	if((exit_status = normalise_in_file(normaliser,dz))<0)
		return(exit_status);
	dz->specenvamp = NULL;	/* prevent SUPERFREE from attempting to free this pointer */
	return(FINISHED);
}



/**************************** ALLOCATE_TWO_FORMANT_BUFFERS ******************************/

int allocate_two_formant_buffers(dataptr dz)
{
	unsigned int buffersize;
	if(dz->bptrcnt <= 0) {
		sprintf(errstr,"bufptr not established in allocate_two_formant_buffers()\n");
		return(PROGRAM_ERROR);
	}
	buffersize = dz->specenvcnt * BUF_MULTIPLIER;
	dz->buflen = buffersize;
	buffersize += 1;
	if((dz->bigfbuf	= (float*) malloc(buffersize * 2 * sizeof(float)))==NULL) {  
		sprintf(errstr,"INSUFFICIENT MEMORY for sound buffers.\n");
		return(MEMORY_ERROR);
	}
	dz->big_fsize = dz->buflen;
	dz->flbufptr[0] = dz->bigfbuf;
	dz->flbufptr[0][dz->big_fsize] = 0.0f;	/* safety value */
	dz->flbufptr[1]  = dz->flbufptr[0] + dz->buflen + 1;
	dz->flbufptr[1][dz->big_fsize] = 0.0f;	/* safety value */
	return(FINISHED);
}

/******************** SETUP_THE_FORMANT_PARAMS **********************/

int setup_the_formant_params(int fd,dataptr dz)
{
	int samps_read;
	int fsamps_to_read = dz->descriptor_samps/DESCRIPTOR_DATA_BLOKS;
	
	if(dz->buflen2 <= 0) {
		sprintf(errstr,"buflen2 not established: setup_the_formant_params()\n");
		return(PROGRAM_ERROR);
	}
	if(dz->buflen2 < dz->descriptor_samps) {
		sprintf(errstr,"buflen2 smaller than descriptor_samps: setup_the_formant_params()\n");
		return(PROGRAM_ERROR);
	}
	if((samps_read = fgetfbufEx(dz->flbufptr[2], dz->buflen2,fd,0)) < dz->descriptor_samps) {
		if(samps_read<0) {
			sprintf(errstr,"Sound read error.\n");
			return(SYSTEM_ERROR);
		}
		sprintf(errstr,"No data in formantfile.\n");
		return(DATA_ERROR);
	}
	dz->ssampsread = samps_read;
	dz->flbufptr[1] = dz->flbufptr[2];
	memmove((char *)dz->specenvpch,(char *)dz->flbufptr[1],(size_t)(fsamps_to_read * sizeof(float)));
	dz->flbufptr[1] += dz->specenvcnt;
	memmove((char *)dz->specenvtop,(char *)dz->flbufptr[1],(size_t)(fsamps_to_read * sizeof(float)));
	dz->flbufptr[1] += dz->specenvcnt;
	return(FINISHED);
}

/****************************** APPLY_FORMANT ***************************/

int apply_formant(int windows_in_buf,dataptr dz)
{
	int exit_status;
	int wc;
	double pre_amptotal, post_amptotal;
	for(wc=0; wc<windows_in_buf; wc++) {
		if(dz->total_windows==0) {
			dz->flbufptr[0] += dz->wanted;
			dz->total_windows++;
			dz->time = (float)(dz->time + dz->frametime);
			continue;			/* SKIP FIRST (ZERO-AMP) WINDOW */
		}
		if(dz->mode==FORM_REPLACE) {
			rectify_window(dz->flbufptr[0],dz);
			if((exit_status = extract_specenv(0,0,dz))<0)
				return(exit_status);
		}
		dz->specenvamp2  = dz->flbufptr[1];
		pre_amptotal = post_amptotal = 0.0;
		if((exit_status = get_totalamp(&pre_amptotal,dz->flbufptr[0],dz->wanted))<0)
			return(exit_status);
		if((exit_status = do_specform(dz))<0)
			return(exit_status);
		if((exit_status = form_filter(dz))<0)
				return(exit_status);
		if((exit_status = get_totalamp(&post_amptotal,dz->flbufptr[0],dz->wanted))<0)
			return(exit_status);
		if((exit_status = normalise(pre_amptotal,post_amptotal,dz))<0)
			return(exit_status);
		if(!flteq(dz->param[FORM_GAIN],1.0)) {
			if((exit_status = form_gain(dz))<0)
				return(exit_status);
  				/* NB This check must be BEFORE move_along_formant_buffer() */
		}
		dz->flbufptr[0] += dz->wanted;
  		if(++dz->total_windows >= dz->wlength)
			return(FINISHED);
		dz->time = (float)(dz->time + dz->frametime);
	}
	return(CONTINUE);
}

/**************************** DO_SPECFORM ***************************
 *
 * Impose formants onto an analfile.
 */

int do_specform(dataptr dz)
{
	int exit_status;
	int vc;
	double thisspecamp0, thisspecamp1, thisvalue;
	rectify_window(dz->flbufptr[0],dz);
	switch(dz->mode) {
	case(FORM_REPLACE):
		for(vc = 0; vc < dz->wanted ; vc += 2) {
			if((exit_status = getspecenvamp(&thisspecamp0,(double)dz->flbufptr[0][FREQ],0,dz))<0)
				return(exit_status);
			if((exit_status = getspecenvamp(&thisspecamp1,(double)dz->flbufptr[0][FREQ],1,dz))<0)
				return(exit_status);
			if(thisspecamp0 < VERY_TINY_VAL)
				dz->flbufptr[0][AMPP] = 0.0f;
			else {
				if((thisvalue = dz->flbufptr[0][AMPP] * thisspecamp1/thisspecamp0)< VERY_TINY_VAL)
					dz->flbufptr[0][AMPP] = 0.0f;
				else
					dz->flbufptr[0][AMPP] = (float)thisvalue;
			}
		}
		break;
	case(FORM_IMPOSE):
		for(vc = 0; vc < dz->wanted ; vc += 2) {
			if((exit_status = getspecenvamp(&thisspecamp1,(double)dz->flbufptr[0][FREQ],1,dz))<0)
				return(exit_status);
			if((thisvalue = dz->flbufptr[0][AMPP] * thisspecamp1)< VERY_TINY_VAL)
				dz->flbufptr[0][AMPP] = 0.0f;
			else
				dz->flbufptr[0][AMPP] = (float)thisvalue;
		}
		break;
	default:
		sprintf(errstr,"Unknown case in do_specform()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/****************************** FORM_FILTER ***************************/

int form_filter(dataptr dz)
{
	int vc;
	for(vc = 0; vc < dz->wanted ; vc += 2) {
		if(dz->flbufptr[0][FREQ] < dz->param[FORM_FBOT]
			|| dz->flbufptr[0][FREQ] > dz->param[FORM_FTOP])
			dz->flbufptr[0][AMPP] = 0.0f;
	}
	return(FINISHED);
}

/****************************** FORM_GAIN ***************************/

int form_gain(dataptr dz)
{
	int vc;
	for(vc=0;vc<dz->wanted;vc+=2)
		dz->flbufptr[0][AMPP] = (float)(dz->flbufptr[0][AMPP] * dz->param[FORM_GAIN]);
	return(FINISHED);
}

/*************************** SETUP_CHANNEL_MIFDRQS ********************/

int setup_channel_midfrqs(dataptr dz)
{
	int cc;
	double thisfrq = 0.0;
	for(cc=0;cc<dz->clength;cc++) {
		dz->freq[cc] = (float)thisfrq;	/* Set up midfrqs of PVOC channels */
		thisfrq += dz->chwidth;
	}
	return(FINISHED);
}

/*************************** CHECK_FORMANT_BUFFER ********************/

int check_formant_buffer(dataptr dz)
{
	if(dz->flbufptr[1] >= dz->flbufptr[3]) {
		if(fgetfbufEx(dz->flbufptr[2],dz->buflen2,dz->ifd[1],0)<0) {
			sprintf(errstr,"data miscount in check_formant_buffer()\n");
			return(SYSTEM_ERROR);
		}
		dz->flbufptr[1] = dz->flbufptr[2];
	}
	return(FINISHED);
}

/*************************** PRESET_CHANNELS ********************/

int preset_channels(dataptr dz)
{
	int vc, cc;
	for(cc = 0, vc = 0; cc < dz->clength; cc++, vc+=2) {
		dz->flbufptr[0][AMPP] = 0.0f;
		dz->flbufptr[0][FREQ] = dz->freq[cc];
	}
	return(FINISHED);
}

/*************************** WRITE_PITCHED_WINDOW ********************/

int write_pitched_window(double fundamental,double *amptotal,dataptr dz)
{
	int exit_status;
	double thisamp, thisfrq = fundamental;
	int n = 1, vc, cc;
	dz->specenvamp = dz->flbufptr[1];
	while(thisfrq < dz->nyquist) {
		if((exit_status = get_channel_corresponding_to_frq(&cc,thisfrq,dz))<0)
			return(exit_status);
		vc = cc * 2;
		if((exit_status = getspecenvamp(&thisamp,thisfrq,0,dz))<0)
			return(exit_status);
		if(thisamp  > dz->flbufptr[0][vc]) {
			*amptotal -= dz->flbufptr[0][vc];
			dz->flbufptr[0][AMPP] = (float)thisamp;
			dz->flbufptr[0][FREQ] = (float)thisfrq;
			*amptotal += thisamp;
		}
		n++;
		thisfrq = fundamental * (double)n;
	}
	return(FINISHED);
}

/*************************** WRITE_SILENT_WINDOW ********************/

int write_silent_window(dataptr dz)
{
	int vc, cc;
	double thisfrq = 0.0;
	for(cc=0, vc =0; cc < dz->clength; cc++, vc += 2) {
		dz->flbufptr[0][AMPP] = (float)0.0;
		dz->flbufptr[0][FREQ] = (float)thisfrq;
		thisfrq += dz->chwidth;
	}
	return(FINISHED);
}

/********************************** NORMALISE_IN_FILE ***********************************/

int normalise_in_file(double normaliser,dataptr dz)
{
	int exit_status;
	int vc;
	int bufcnt = 0;
	int total_samps_to_write = dz->total_samps_written;
	int samps_to_write;
	float *bufend;
//TW NEW MECHANISM writes renormalised vals from original file created (tempfile) into true outfile
// true outfile gets the name the user originally input
//seek eliminated
	if(sndcloseEx(dz->ifd[0]) < 0) {
		sprintf(errstr, "WARNING: Can't close input soundfile\n");
		return(SYSTEM_ERROR);
	}
	dz->ifd[0] = -1;
	if(sndcloseEx(dz->ofd) < 0) {
		sprintf(errstr, "WARNING: Can't close output soundfile\n");
		return(SYSTEM_ERROR);
	}
	dz->ofd = -1;
	if((dz->ifd[0] = sndopenEx(dz->outfilename,0,CDP_OPEN_RDWR)) < 0) {	   /*RWD Nov 2003, was RDONLY */
		sprintf(errstr,"Failure to reopen file %s for renormalisation.\n",dz->outfilename);
		return(SYSTEM_ERROR);
	}
	sndseekEx(dz->ifd[0],0,0);
	if((exit_status = create_sized_outfile(dz->wordstor[0],dz))<0) {
		sprintf(errstr,"Failure to create file %s for renormalisation.\n",dz->wordstor[0]);
		return(exit_status);
	}
	if((exit_status = reset_peak_finder(dz)) < 0)
		return exit_status;
	dz->total_samps_written = 0;
	fprintf(stdout,"INFO: Normalising.\n");
	fflush(stdout);
	while(total_samps_to_write>0) {
		if(fgetfbufEx(dz->bigfbuf,dz->buflen,dz->ifd[0],0)<0) {
			sprintf(errstr,"Sound read error.\n");
			close_and_delete_tempfile(dz->outfilename,dz);
			return(SYSTEM_ERROR);
		}
		dz->flbufptr[0]  = dz->bigfbuf;
		samps_to_write  = min(dz->buflen,total_samps_to_write);
		bufend = dz->flbufptr[0] + samps_to_write;
		while(dz->flbufptr[0] < bufend) {
			for(vc = 0;vc < dz->wanted; vc+=2)
				dz->flbufptr[0][AMPP] = (float)(dz->flbufptr[0][AMPP] * normaliser);
			dz->flbufptr[0] += dz->wanted;
		}
		if(samps_to_write>=dz->buflen) {
			if((exit_status = write_exact_samps(dz->bigfbuf,samps_to_write,dz))<0) {
				close_and_delete_tempfile(dz->outfilename,dz);
				return(exit_status);
			}
		} else if(samps_to_write > 0) {
			if((exit_status = write_samps(dz->bigfbuf,samps_to_write,dz))<0) {
				close_and_delete_tempfile(dz->outfilename,dz);
				return(exit_status);
			}
		}
		total_samps_to_write -= samps_to_write;
		bufcnt++;
	}	
	close_and_delete_tempfile(dz->outfilename,dz);
	return(FINISHED);
}

/*************************** WRITE_UNPITCHED_WINDOW ********************/

#define NOISEBASEX	(0.5)

int write_unpitched_window(double *amptotal,dataptr dz)
{
	int exit_status;
	double thisamp, basefrq = 0.0, thisfrq;
	double rand_offset;
	double noisrange = 1.0 - NOISEBASEX;
	int n = 1, vc, cc;
	int ccnt =  dz->wanted/2;
	n = 1;
	while(n <= ccnt) {
		rand_offset = (drand48() - (.5)) * dz->chwidth;
		thisfrq = basefrq + rand_offset;
		if(thisfrq < 0.0 || thisfrq > dz->nyquist)	
			continue;
		if((exit_status = get_channel_corresponding_to_frq(&cc,thisfrq,dz))<0)
			return(exit_status);
		vc = cc * 2;
		if((exit_status = getspecenvamp(&thisamp,thisfrq,0,dz))<0)
			return(exit_status);
		if(thisamp  > dz->flbufptr[0][vc]) {
			*amptotal -= dz->flbufptr[0][vc];
			dz->flbufptr[0][AMPP] = (float)(thisamp * (drand48() * noisrange));
			dz->flbufptr[0][FREQ] = (float)thisfrq;
			*amptotal += thisamp;
		}
		basefrq += dz->chwidth;
		n++;
	}
	return(FINISHED);
}

/*************************** GET_THE_OTHER_FILENAME_XX ********************/

char *get_the_other_filename_xx(char *filename,char c)
{
	char *p, *nufilename;
	int len = strlen(filename);
	char temp[] = "_cdptemp";
	len += 9;
	if((nufilename = (char *)malloc(len)) == NULL)
		return NULL;
	strcpy(nufilename,filename);
	strcat(nufilename,temp);
	p = nufilename + len;
	*p = ENDOFSTR;
	p--;
	*p = c;
	return nufilename;
}

/*************************** GET_THE_OTHER_FILENAME ********************/

void get_the_other_filename(char *filename,char c)
{
	char *p, *end;
	p = filename + strlen(filename);
	end = p;
	while(p > filename) {
		p--;
		if(*p == '.')
			break;				/* return start of final name extension */
		else if(*p == '/' || *p == '\\') {
			p =  end;			/* directory path component found before finding filename extension */
			break;				/* go to end of name */
		}
	}
	if(p == filename)
		p = end;				/* no finalname extension found, go to end of name */
	p--;						/* insert '1' at name end */
	*p = c;
	return;
}

/***************************** READFORMHEAD ********************************/

int readformhead(char *str,int ifd,dataptr dz)
{
	int exit_status;
	SFPROPS props = {0};
	int isenv = 0;

	infileptr inputfile;
	if((inputfile = (infileptr)malloc(sizeof(struct filedata)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for infile structure to test file data.");
		return(MEMORY_ERROR);
	} else if((exit_status = cdparse(str,inputfile))<0) {
		sprintf(errstr,"Failed to parse input file %s\n",str);
		return(PROGRAM_ERROR);
	}
	if(!snd_headread(ifd,&props)) {
		fprintf(stdout,"INFO: Failure to read properties, in %s\n",str);
		fflush(stdout);
		return(DATA_ERROR);
	}
	inputfile->srate = props.srate;
	inputfile->channels = props.chans;

	switch(props.samptype) {
	case(SHORT16):	inputfile->stype = SAMP_SHORT;	break;
	case(FLOAT32):	inputfile->stype = SAMP_FLOAT;	break;
	case(SHORT8):	inputfile->stype = SAMP_BYTE;	break;
	default:	
		/* remaining symbols have same int value */	
		inputfile->stype = (int)props.samptype;	/* RWD April 2005 */
		break;
	}
//TW TEMPORARY SUBSTITUTION
	if(sndgetprop(ifd,"is a formant file",(char *) &isenv,sizeof(int)) >= 0)
		inputfile->filetype = FORMANTFILE;
	else {
		sprintf(errstr,"Second fvile is not a formant file\n");
		return(DATA_ERROR);
	}
	dz->specenvcnt = props.specenvcnt;
	/* fall through */
	if(inputfile->channels != 1) {
		sprintf(errstr,"Channel count not equal to 1 in 2nd file.\n");
		return(DATA_ERROR);
	}
	if(sndgetprop(ifd,"orig channels",(char *)&(inputfile->origchans),sizeof(int)) < 0) {
		sprintf(errstr,"Cannot read original channels count of 2nd file\n");
		return(DATA_ERROR);
	}
	if(dz->process == ONEFORM_PUT) {
		if(inputfile->origchans != dz->infile->channels) {
			sprintf(errstr,"incompatible original channels count in the 2 files.\n");
			return(DATA_ERROR);
		}
	} else {
		if(inputfile->origchans != dz->infile->origchans) {
			sprintf(errstr,"incompatible original channels count in the 2 files.\n");
			return(DATA_ERROR);
		}
	}
	if(sndgetprop(ifd,"original sampsize",(char *)&(inputfile->origstype),sizeof(int)) < 0) {
		sprintf(errstr,"Cannot read original sample type in 2nd file.\n");
		return(DATA_ERROR);
	}
	if(inputfile->origstype != dz->infile->origstype) {
		sprintf(errstr,"incompatible original types in the 2 files.\n");
		return(DATA_ERROR);
	}
	if(sndgetprop(ifd,"original sample rate",(char *)&(inputfile->origrate),sizeof(int)) < 0) {
		sprintf(errstr,"Cannot read original sample rate in 2nd file.\n");
		return(DATA_ERROR);
	}
	if(inputfile->origrate != dz->infile->origrate) {
		sprintf(errstr,"incompatible original sample rates in the 2 files.\n");
		return(DATA_ERROR);
	}
	if(sndgetprop(ifd,"arate",(char *)&(inputfile->arate),sizeof(float)) < 0) {
		sprintf(errstr,"Cannot read analysis rate in 2nd file\n");
		return(DATA_ERROR);
	}
	if(inputfile->arate != dz->infile->arate) {
		sprintf(errstr,"incompatible analysis rates in the 2 files.\n");
		return(DATA_ERROR);
	}
	if(sndgetprop(ifd,"analwinlen",(char *)&(inputfile->Mlen),sizeof(int)) < 0) {
		sprintf(errstr,"Cannot read analysis window length in 2nd file\n");
		return(DATA_ERROR);
	}
	if(inputfile->Mlen != dz->infile->Mlen) {
		sprintf(errstr,"incompatible analysis window lengths in the 2 files.\n");
		return(DATA_ERROR);
	}
	if(sndgetprop(ifd,"decfactor",(char *)&(inputfile->Dfac),sizeof(int)) < 0) {
		sprintf(errstr,"Cannot read original decimation factor in 2nd file.\n");
		return(DATA_ERROR);
	}
	if(inputfile->Dfac != dz->infile->Dfac) {
		sprintf(errstr,"incompatible decimation factor in the 2 files.\n");
		return(DATA_ERROR);
	}
	return(FINISHED);
}

/****************************** GET_THE_MODE_FROM_CMDLINE *********************************/

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

