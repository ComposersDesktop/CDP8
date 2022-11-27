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
#include <limits.h>

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

char infilename[1000];

#define BIGGERSIL	(8)		/* keep only silences which are more than 1/8 length of max silence */
#define RETIME_ENVSIZE (1000)
#define RETIME_WINSIZE (20)

//CDP LIB REPLACEMENTS
static int check_retime_param_validity_and_consistency(dataptr dz);
static int setup_retime_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_retime_param_ranges_and_defaults(dataptr dz);
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
static int handle_the_special_data(int *cmdlinecnt,char ***cmdline,dataptr dz);
static int create_retime_sndbufs(dataptr dz);
static int retime(dataptr dz);
static int setup_the_special_data_ranges(int mode,int srate,double duration,double nyquist,int wlength,int channels,aplptr ap);
static int advance_obuf(int *obufpos,dataptr dz);
static int advance_ibuf(int *abs_read_start,int *read_end_in_buf,int *last_time_in_inbuf,int *next_time_in_inbuf,
				int *min_energy_point,int silcnt,int *silat,dataptr dz);
static int find_min_energy_point(int *min_energy_point,int last_time_in_inbuf,int next_time_in_inbuf,int winsize,float *ibuf,float *env,int *pos,dataptr dz);

static int read_ideal_data(char *filename,dataptr dz);
static int read_retime_data(char *filename,dataptr dz);
static int read_retempo_data(char *filename,dataptr dz);
static int read_retime_mask(char *filename,dataptr dz);
static int ideal_retime(dataptr dz);
static int shave(dataptr dz);
static int retime_found_peaks(dataptr dz);
static int locate_peaks(dataptr dz);
static int establish_cuts_array(dataptr dz);
static int eliminate_lost_ideal_and_real_events(int *lost,int lostcnt,dataptr dz);
static int reposition_peak(dataptr dz);
static int repeat_beats(dataptr dz);
static int get_initial_silence(int *initial_silence,dataptr dz);
static int find_sound_start(dataptr dz);
static int read_retime_fnam(char *filename,dataptr dz);
static int find_shortest_event(dataptr dz);
static int count_events(int paramno,int arrayno,int *eventsegscnt, dataptr dz);
static int equalise_event_levels(dataptr dz);
static int retempo_events(dataptr dz);
static int mask_events(dataptr dz);
static int usage22(char *str,char *str2);

#define MMin		scalefact		/* Use 'dz->scalefact' param to store MM */
#define	offset		chwidth			/* use 'dz->chwidth' param to store offset */

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
		} else if(argc == 3) {
			usage22(argv[1],argv[2]);	
			return(FAILED);
		}
	}
	dz->maxmode = 14;
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
		if((exit_status = get_the_mode_from_cmdline(cmdline[0],dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(exit_status);
		}
		cmdline++;
		cmdlinecnt--;
		// setup_particular_application =
		if((exit_status = setup_retime_application(dz))<0) {
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
	if((exit_status = setup_retime_param_ranges_and_defaults(dz))<0) {
		exit_status = print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	// open_first_infile		CDP LIB
	strcpy(infilename,cmdline[0]);
	if((exit_status = open_first_infile(cmdline[0],dz))<0) {	
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);	
		return(FAILED);
	}
	cmdlinecnt--;
	cmdline++;

//	handle_extra_infiles() : redundant
	// handle_outfile() = 
	if(dz->mode != 11) {
		if(dz->mode != 10) {
			if((exit_status = handle_the_outfile(&cmdlinecnt,&cmdline,dz))<0) {
				print_messages_and_close_sndfiles(exit_status,is_launched,dz);
				return(FAILED);
			}
		} else if(sloom) {			//	mode 10, no outfile
			cmdlinecnt--;			//	in sloom, always an outfile name in cmdline
			cmdline++;
		}
	}

//	handle_formants()			redundant
//	handle_formant_quiksearch()	redundant
	if((exit_status = handle_the_special_data(&cmdlinecnt,&cmdline,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {		// CDP LIB
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
//	check_param_validity_and_consistency....
	if((exit_status = check_retime_param_validity_and_consistency(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	is_launched = TRUE;
	switch(dz->mode) {
	case(0):
		dz->bufcnt = 5;
		break;
	case(1):
		dz->bufcnt = 4;
		break;
	case(2):
	case(8):
	case(10):
	case(11):
		dz->bufcnt = 1;
		break;
	case(3):
	case(4):
	case(5):
	case(6):
		dz->bufcnt = 3;
		break;
	case(7):
		dz->bufcnt = 4;
		break;
	case(9):
		dz->bufcnt = 1;
		break;
	case(12):
	case(13):
		dz->bufcnt = 2;
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
	if((exit_status = create_retime_sndbufs(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	//param_preprocess()						redundant
	//spec_process_file =
	switch(dz->mode) {
	case(0):
		if((exit_status = retime(dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		break;
	case(1):
		if((exit_status = ideal_retime(dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		break;
	case(2):
		if((exit_status = shave(dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		break;
	case(3):
	case(4):
		if((exit_status = retime_found_peaks(dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		break;
	case(5):
	case(6):
		if((exit_status = retempo_events(dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		break;
	case(7):
		if((exit_status = repeat_beats(dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		break;
	case(8):
		if((exit_status = mask_events(dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		break;
	case(9):
		if((exit_status = equalise_event_levels(dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		break;
	case(10):
		if((exit_status = find_shortest_event(dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		break;
	case(11):
		if((exit_status = find_sound_start(dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		break;
	case(12):
	case(13):
		if((exit_status = reposition_peak(dz))<0) {
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

/************************* SETUP_RETIME_APPLICATION *******************/

int setup_retime_application(dataptr dz)
{
	int exit_status;
	aplptr ap;
	if((exit_status = establish_application(dz))<0)		// GLOBAL
		return(FAILED);
	ap = dz->application;
	// SEE parstruct FOR EXPLANATION of next 2 functions
	switch(dz->mode) {
	case(0):
		if((exit_status = set_param_data(ap,RETIME_DATA   ,1,1,"d"))<0)
			return(FAILED);
		break;
	case(1):
		if((exit_status = set_param_data(ap,IDEAL_DATA   ,3,3,"dDd"))<0)
			return(FAILED);
		break;
	case(2):
		if((exit_status = set_param_data(ap,0   ,4,4,"dddd"))<0)
			return(FAILED);
		break;	
	case(3):
		if((exit_status = set_param_data(ap,0,3,3,"ddd"))<0)
			return(FAILED);
		break;
	case(4):
		if((exit_status = set_param_data(ap,0,2,2,"Dd"))<0)
			return(FAILED);
		break;
	case(5):
		if((exit_status = set_param_data(ap,RETEMPO_DATA,4,4,"dddd"))<0)
			return(FAILED);
		break;
	case(6):
		if((exit_status = set_param_data(ap,RETEMPO_DATA,3,3,"ddd"))<0)
			return(FAILED);
		break;
	case(7):
		if((exit_status = set_param_data(ap,0,5,5,"ddiid"))<0)
			return(FAILED);
		break;
	case(8):
		if((exit_status = set_param_data(ap,RETIME_MASK,1,1,"d"))<0)
			return(FAILED);
		break;
	case(9):
		if((exit_status = set_param_data(ap,0,2,2,"dd"))<0)
			return(FAILED);
		break;
	case(10):
		if((exit_status = set_param_data(ap,0,1,1,"d"))<0)
			return(FAILED);
		break;
	case(11):
		if((exit_status = set_param_data(ap,RETIME_FNAM,0,0,""))<0)
			return(FAILED);
		break;
	case(12):
		if((exit_status = set_param_data(ap,0,1,1,"d"))<0)
			return(FAILED);
		break;
	case(13):
		if((exit_status = set_param_data(ap,0,2,2,"dd"))<0)
			return(FAILED);
		break;
	}
	switch(dz->mode) {
	case(4):
		if((exit_status = set_vflgs(ap,"sea",3,"ddd","",0,0,""))<0)
			return(FAILED);
		break;
	case(9):
		if((exit_status = set_vflgs(ap,"mp",2,"id","",0,0,""))<0)
			return(FAILED);
		break;
	default:
		if((exit_status = set_vflgs(ap,"",0,"","",0,0,""))<0)
			return(FAILED);
		break;
	}
	// set_legal_infile_structure -->
	dz->has_otherfile = FALSE;
	// assign_process_logic -->
	switch(dz->mode) {
	case(8):
	case(9):
		dz->input_data_type = SNDFILES_ONLY;
		dz->process_type	= EQUAL_SNDFILE;	
		dz->outfiletype  	= SNDFILE_OUT;
		break;
	case(10):
	case(11):
		dz->input_data_type = SNDFILES_ONLY;
		dz->process_type	= OTHER_PROCESS;	
		dz->outfiletype  	= NO_OUTPUTFILE;
		break;
	default:
		dz->input_data_type = SNDFILES_ONLY;
		dz->process_type	= UNEQUAL_SNDFILE;	
		dz->outfiletype  	= SNDFILE_OUT;
		break;
	}
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

/************************* SETUP_RETIME_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_retime_param_ranges_and_defaults(dataptr dz)
{
	int exit_status;
	double srate = (double)dz->infile->srate;
	int chans = dz->infile->channels;
	aplptr ap = dz->application;
	// set_param_ranges()
	ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
	// NB total_input_param_cnt is > 0 !!!
	if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
		return(FAILED);
	// get_param_ranges()
	switch(dz->mode) {
	case(0):
		ap->lo[0]	= 0.0;
		ap->hi[0]	= 400.0;
		ap->default_val[0]	= 60;
		break;
	case(1):
		ap->lo[0]	= 0.0;
		ap->hi[0]	= 400.0;
		ap->default_val[0]	= 60;
		ap->lo[1]	= 1.0;
		ap->hi[1]	= 1000.0;
		ap->default_val[1]	= 20;
		ap->lo[2]	= 1.0;
		ap->hi[2]	= 1000.0;
		ap->default_val[2]	= 10;
		break;
	case(2):
		ap->lo[0]	 = (2.0/srate) * SECS_TO_MS;
		ap->hi[0]	 = 10000.0;
		ap->default_val[0] = (2.0/srate) * SECS_TO_MS;
		ap->lo[1]	= 1.0;
		ap->hi[1]	= 1000.0;
		ap->default_val[1]	= 40;
		ap->lo[2]	= 1.0;
		ap->hi[2]	= 1000.0;
		ap->default_val[2]	= 10;
		ap->lo[3]	= 1.0;
		ap->hi[3]	= 1000.0;
		ap->default_val[3]	= 20;
		break;
	case(3):
		ap->lo[0]	= 0.0;
		ap->hi[0]	= 6000.0;
		ap->default_val[0]	= 60;
		ap->lo[1]	= (2.0/srate) * SECS_TO_MS;
		ap->hi[1]	= 10000.0;
		ap->default_val[1]	= 10;
		ap->lo[2]	= 0.0;
		ap->hi[2]	= 1.0;
		ap->default_val[2]	= 1;
		break;
	case(4):
		ap->lo[0]	= 0.01;
		ap->hi[0]	= 100.0;
		ap->default_val[0]	= 1.0;
		ap->lo[1]	= (2.0/srate) * SECS_TO_MS;
		ap->hi[1]	= 10000.0;
		ap->default_val[1]	= 10;
		ap->lo[2]	= 0.0;
		ap->hi[2]	= (double)(dz->insams[0]/chans)/srate;
		ap->default_val[2]	= 0.0;
		ap->lo[3]	= 0.0;
		ap->hi[3]	= (double)(dz->insams[0]/chans)/srate;
		ap->default_val[3]	= (double)(dz->insams[0]/chans)/srate;
		ap->lo[4]	= 0.0;
		ap->hi[4]	= (double)(dz->insams[0]/chans)/srate;
		ap->default_val[4]	= 0.0;
		break;
	case(5):
		ap->lo[0]	= 0.01;			// MM
		ap->hi[0]	= 1000.0;
		ap->default_val[0]	= 60.0;
		ap->lo[1]	= 0.00;			// Offset
		ap->hi[1]	= 1000.0;
		ap->default_val[1]	= 0.0;
		ap->lo[2]	 = (2.0/srate) * SECS_TO_MS;
		ap->hi[2]	 = 10000.0;		// Minsil
		ap->default_val[2] = (2.0/srate) * SECS_TO_MS;
		ap->lo[3]	 = 0.0;
		ap->hi[3]	 = 1.0;			// Pregain
		ap->default_val[3] = 1.0;
		break;
	case(6):
		ap->lo[0]	= 0.00;			// Offset
		ap->hi[0]	= 1000.0;
		ap->default_val[0]	= 0.0;
		ap->lo[1]	 = (2.0/srate) * SECS_TO_MS;
		ap->hi[1]	 = 10000.0;		// Minsil
		ap->default_val[1] = (2.0/srate) * SECS_TO_MS;
		ap->lo[2]	 = 0.0;
		ap->hi[2]	 = 1.0;			// Pregain
		ap->default_val[2] = 1.0;
		break;
	case(7):
		ap->lo[MM]	= 0.01;
		ap->hi[MM]	= 1000.0;
		ap->default_val[MM]	= 60.0;
		ap->lo[BEAT_AT]	= 0.0;
		ap->hi[BEAT_AT]	= (double)(dz->insams[0]/chans)/srate;
		ap->default_val[BEAT_AT] = 0.0;
		ap->lo[BEAT_CNT] = 1.0;
		ap->hi[BEAT_CNT] = 24.0;
		ap->default_val[BEAT_CNT] = 1.0;
		ap->lo[BEAT_REPEATS] = 1.0;
		ap->hi[BEAT_REPEATS] = 1000.0;
		ap->default_val[BEAT_REPEATS] = 1.0;
		ap->lo[BEAT_SILMIN]	 = (2.0/srate) * SECS_TO_MS;
		ap->hi[BEAT_SILMIN]	 = 10000.0;
		ap->default_val[BEAT_SILMIN] = (2.0/srate) * SECS_TO_MS;
		break;
	case(8):
		ap->lo[0]	 = (2.0/srate) * SECS_TO_MS;
		ap->hi[0]	 = 10000.0;		// Minsil
		ap->default_val[0] = (2.0/srate) * SECS_TO_MS;
		break;
	case(9):
		ap->lo[0]	 = (2.0/srate) * SECS_TO_MS;
		ap->hi[0]	 = 10000.0;
		ap->default_val[0] = (2.0/srate) * SECS_TO_MS;
		ap->lo[1]	 = 0.0;
		ap->hi[1]	 = 1.0;
		ap->default_val[1] = 0.5;
		ap->lo[2]	 = 0.0;
		ap->hi[2]	 = 32767.0;
		ap->default_val[2] = 0.0;
		ap->lo[3]	 = 0.0;
		ap->hi[3]	 = 1.0;
		ap->default_val[3] = 1.0;
		break;
	case(10):
		ap->lo[0]	 = (2.0/srate) * SECS_TO_MS;
		ap->hi[0]	 = 10000.0;
		ap->default_val[0] = (2.0/srate) * SECS_TO_MS;
		break;
	case(11):
		break;
	case(12):
		ap->lo[0]	= 0.0;
		ap->hi[0]	= 3600.0;
		ap->default_val[0]	= 0.0;
		break;
	case(13):
		ap->lo[0]	= 0.0;
		ap->hi[0]	= 3600.0;
		ap->default_val[0]	= 0.0;
		ap->lo[1]	= 0.0;
		ap->hi[1]	= 3600.0;
		ap->default_val[1]	= 0.0;
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
			if((exit_status = setup_retime_application(dz))<0)
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

int establish_bufptrs_and_extra_buffers(dataptr dz)
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
	usage2("retime");
	return(USAGE_ONLY);
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	if(!strcmp(prog_identifier_from_cmdline,"retime"))				dz->process = RETIME;
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
	if(!strcmp(str,"retime")) {
		fprintf(stderr,
	    "USAGE:\n"
	    "retime retime modenumber parameters.....\n"
		"\n"
		"MODE 1  Output user-specified peaks at regular pulse at given tempo.\n"
		"MODE 3  Shorten existing (silence-separated) events.\n"
		"MODE 4  Find existing (silence-separated) events and output at regular MM.\n"
		"MODE 5  Find existing (silence-separated) events and change speed by factor.\n"
		"MODE 6  Find existing (silence-separated) events: position at specified beats.\n"
		"MODE 7  Find existing (silence-separated) events: position at specified times.\n"
		"MODE 8  Mark (silence-separated) event within sound & repeat at specified tempo.\n"
		"MODE 9  Replace by silence (silence-separated) events in specified pattern.\n"
		"MODE 10 Adjusts levels (silence-separated) events, so more equal, or accented.\n"
		"MODE 11 Finds durations of shortest and longest (silence-separated) event.\n"
		"MODE 12 Find start of sound in file (1st non-zero sample).\n"
		"MODE 13 Find peak and move all data so peak goes to specified time.\n"
		"MODE 14 Mark peak, then move all data so peak goes to specified time.\n"
		"\n"
		"Retime works by editing existing files (removing sound or adding silence)\n"
		"and is best suited to making small changes to rhythmic placement.\n"
		"\n"
		"Type \"retime retime modenumber\" for more details\n");
	} else
		fprintf(stdout,"Unknown option '%s'\n",str);
	return(USAGE_ONLY);
}

int usage3(char *str1,char *str2)
{
	fprintf(stderr,"Insufficient parameters on command line.\n");
	return(USAGE_ONLY);
}

/******************************** USAGE22 ********************************/

int usage22(char *str,char *str2)
{
	if(strcmp(str,"retime")) {
		fprintf(stdout,"Unknown option '%s'\n",str);
	} else if(!strcmp(str2,"1")) {
		fprintf(stderr,
		"USAGE:\n"
		"retime retime 1 infile outfile refpoints tempo\n"
		"\n"
		"Output user-specified peaks at regular pulse at given tempo.\n"
		"\n"
		"Best applied to files with clear peaks.\n"
		"\n"
		"REFPOINTS    times of peaks in the infile which will become on-the-beat events\n"
		"             in the outfile.\n"
		"TEMPO        Tempo of OUTFILE, as a MM (vals > 20 to 400)\n"
		"             OR (for vals below 1.0) a beat duration in secs.\n");
	} else if(!strcmp(str2,"2")) {
		fprintf(stderr,"Mode only accessible via Sound Loom Properties Files\n");
	} else if(!strcmp(str2,"3")) {
		fprintf(stderr,
		"USAGE:\n"
		"retime retime 3 infile outfile minsil inevwidth outevwidth splicelen\n"
		"\n"
		"Shorten existing (silence-separated) events.\n"
		"\n"
		"MINSIL       Dur (mS) of min silence between events (Range 2000/srate to 10000).\n"
		"INEVWIDTH    (Min) width (in mS) of events in infile (Range 1 -1000)\n"
		"OUTEVWIDTH   Width (in mS) of events in outfile (Range 1- 1000)\n"
		"SPLICELEN    Dur of splices (mS) cutting events in outfile (Range 1- 1000).\n");
	} else if(!strcmp(str2,"4")) {
		fprintf(stderr,
		"USAGE:\n"
		"retime retime 4 infile outfile tempo minsil pregain\n"
		"\n"
		"Find existing (silence-separated) events and output at regular MM.\n"
		"\n"
		"TEMPO        Tempo of OUTFILE, as a MM (vals > 20 to 6000)\n"
		"             OR (for vals below 1.0) a beat duration in secs.\n"
		"MINSIL       Dur (mS) of min silence between events (Range 2000/srate to 10000).\n"
		"PREGAIN      Gain of input signal (Range >0 to 1).\n");
	} else if(!strcmp(str2,"5")) {
		fprintf(stderr,
		"USAGE:\n"
		"retime retime 5 infile outfile factor minsil [-sstart -eend -async]\n"
		"\n"
		"Find existing (silence-separated) events and change speed by factor.\n"
		"\n"
		"MINSIL       Dur (mS) of min silence between events (Range 2000/srate to 10000).\n"
		"FACTOR       Speed-change factor (can vary over time) (Range 0.01 - 100).\n"
		"START        Time at which speed changing begins.\n"
		"END          Time at which speed changing ends.\n"
		"SYNC         Approx time of infile-event which syncs with its copy in output.\n"
		"             (Time should be WITHIN the event. Zero implies sync at 1st event).\n");
	} else if(!strcmp(str2,"6")) {
		fprintf(stderr,
		"USAGE:\n"
		"retime retime 6 infile outfile retempodata tempo offset minsil pregain\n"
		"\n"
		"Find existing (silence-separated) events: position at specified beats.\n"
		"\n"
		"RETEMPODATA  File contains position of events in outfile, in BEATS,\n"
		"             where events assumed to start at beat zero.\n"
		"TEMPO        Tempo of OUTFILE, as a MM (vals > 20 to 1000)\n"
		"             OR (for vals below 1.0) a beat duration in secs.\n"
		"OFFSET       Time of first sounding event in output file (Range 0 -1000).\n"
		"MINSIL       Dur (mS) of min silence between events (Range 2000/srate to 10000).\n"
		"PREGAIN      Gain of input signal (Range >0 to 1).\n");
	} else if(!strcmp(str2,"7")) {
		fprintf(stderr,
		"USAGE:\n"
		"retime retime 7 infile outfile retempodata offset minsil pregain\n"
		"\n"
		"Find existing (silence-separated) events: position at specified times.\n"
		"\n"
		"RETEMPODATA  File contains position of events in outfile, in SECONDS,\n"
		"             where events assumed to start at time zero.\n"
		"MINSIL       Dur (mS) of min silence between events (Range 2000/srate to 10000).\n"
		"PREGAIN      Gain of input signal (Range >0 to 1).\n"
		"OFFSET       Time of first sounding event in output file (range 0 - 1000).\n");
	} else if(!strcmp(str2,"8")) {
		fprintf(stderr,
		"USAGE:\n"
		"retime retime 8 infile outfile tempo eventtime cnt repeats minsil\n"
		"\n"
		"Mark one (silence-separated) event within a soundfile,\n"
		"OR the 1st event of \"cnt\" events (see below),\n"
		"and repeat it (or them) at a specified tempo WITHIN the original sound.\n"
		"\n"
		"TEMPO        Tempo of OUTFILE, as a MM (vals > 20 to 1000)\n"
		"             OR (for vals below 1.0) a beat duration in secs.\n"
		"EVENTTIME    Time (roughly) of (1st) event to repeat (time must be INSIDE event).\n"
		"CNT          Number of (silence-separated) events to capture (Range 1 to 24).\n"
		"REPEATS      No. of times to repeat event(s): (NB: 2 repeats of A produces AAA).\n"
		"             (Range 1 to 1000).\n"
		"MINSIL       Dur (mS) of min silence between events (Range 2000/srate to 10000).\n");
	} else if(!strcmp(str2,"9")) {
		fprintf(stderr,
		"USAGE:\n"
		"retime retime 9 infile outfile maskdata minsil\n"
		"\n"
		"Replace by silence (silence-separated) events in specified pattern.\n"
		"\n"
		"MASKDATA     File with sequence of 0s and 1s: 0 masks event: 1 reveals event.\n"
		"             0s and 1s must be separated by space, or on different lines.\n"
		"             Pattern of masking is repeated once its end is reached.\n"
		"MINSIL       Dur (mS) of min silence between events (Range 2000/srate to 10000).\n");
	} else if(!strcmp(str2,"10")) {
		fprintf(stderr,
		"USAGE:\n"
		"retime retime 10 infile outfile minsil evening [-mmeter] [-mpregain]\n"
		"\n"
		"Adjust levels (silence-separated) events, so more equal, or accented.\n"
		"\n"
		"MINSIL   Dur (mS) of min silence between events (Range 2000/srate to 10000).\n"
		"PREGAIN  Gain of input signal (Range >0 to 1).\n"
		"METER    Pattern of accented beats.\n"
		"         0 : no events emphasized.\n"
		"         3 : every 3rd event accented.\n"
		"EVENING  Range 0 - 1.\n"
		"    With METER 0: the events are evened-out in level by \"EVENING\".\n"
		"    0 has no effect, while 1 tries to make event-levels equal.\n"    
		"    Intermediate values produce more or less evening-out of levels.\n"    
		"    With nonzero METER e.g. 3,\n"
		"    unaccented beats take level relative specified in \"EVENING\".\n"
		"    e.g. Evening = 0.3 sets unaccented beats to 1/3 level of accented.\n");
	} else if(!strcmp(str2,"11")) {
		fprintf(stderr,
		"USAGE:\n"
		"retime retime 11 infile minsil\n"
		"\n"
		"MODE 11 Finds durations of shortest and longest (silence-separated) event.\n"
		"\n"
		"MINSIL   Dur (mS) of min silence between events (Range 2000/srate to 10000).\n");
	} else if(!strcmp(str2,"12")) {
		fprintf(stderr,
		"USAGE:\n"
		"retime retime 12 infile outfile.txt\n"
		"\n"
		"Find start of sound in file (1st non-zero sample).\n"
		"\n"
		"Outfile must have '.txt' extension.\n"
		"If file already exists, output is APPENDED to that file.\n");
	} else if(!strcmp(str2,"13")) {
		fprintf(stderr,
		"USAGE:\n"
		"retime retime 13 infile outfile goalpeaktime\n"
		"\n"
		"Find sound peak and move all data so peak goes to specified time.\n"
		"\n"
		"GOALPEAKTIME Time to which file peak is moved (Range 0 - 3600)\n"
		"(must be greater than the original peaktime).\n");
	} else if(!strcmp(str2,"14")) {
		fprintf(stderr,
		"USAGE:\n"
		"retime retime 14 infile outfile goalpeaktime peaktime\n"
		"\n"
		"Specify peak position, then move data so peak goes to specified time.\n"
		"\n"
		"GOALPEAKTIME Time to which file peak is moved (Range 0 - 3600).\n"
		"             (must be greater than the original peaktime).\n"
		"PEAKTIME     Time of existing peak in file.\n");
	} else {
		fprintf(stdout,"Unknown mode '%s'\n",str);
	}
	return(USAGE_ONLY);
}

/******************************** CHECK_RETIME_PARAM_VALIDITY_AND_CONSISTENCY **********************************/

int check_retime_param_validity_and_consistency(dataptr dz)
{
	int exit_status;
	double srate = (double)dz->infile->srate;
	int chans = dz->infile->channels;

	switch(dz->mode) {
	case(3):
		if(dz->param[2] == 0.0) {
			sprintf(errstr,"Pregain parameter of zero, will produce silent output.\n");
			return(DATA_ERROR);
		}
		/* fall thro */
	case(0):
		if(dz->param[MM] >= 20)
			dz->param[MM] = 60.0 / dz->param[MM];
		else if(dz->param[MM] > 1.0 || dz->param[MM] < FLTERR) {
			sprintf(errstr,"Output tempo vals must be > 0 AND less than 1 (beat duration) OR >= 20 (MM)\n");
			return(DATA_ERROR);
		}
		dz->iparam[MM] = (int)round(dz->param[MM] * srate) * chans;
		break;
	case(1):
		if(dz->param[MM] >= 20)
			dz->param[MM] = 60.0 / dz->param[MM];
		else if(dz->param[MM] > 1.0 || dz->param[MM] < FLTERR) {
			sprintf(errstr,"Output tempo vals must be > 0 AND less than 1 (beat duration) OR >= 20 (MM)\n");
			return(DATA_ERROR);
		}
		dz->iparam[MM] = (int)round(dz->param[MM] * srate) * chans;
		dz->iparam[RETIME_SPLICE] = (int)round(dz->param[RETIME_SPLICE] * MS_TO_SECS * srate) * chans;
		if(dz->brksize[RETIME_WIDTH] == 0)
			dz->iparam[RETIME_WIDTH] = (int)round(dz->param[RETIME_WIDTH] * MS_TO_SECS * srate) * chans; 
		if((exit_status = establish_cuts_array(dz)) < 0)
			return(exit_status);
		break;
	case(2):
		dz->iparam[0] = (int)round(dz->param[0] * MS_TO_SECS * srate) * chans;
		dz->iparam[1] = (int)round(dz->param[1] * MS_TO_SECS * srate) * chans;
		dz->iparam[2] = (int)round(dz->param[2] * MS_TO_SECS * srate) * chans;
//JUNE 2010
		dz->iparam[3] = (int)round(dz->param[3] * MS_TO_SECS * srate) * chans;
		break;
	case(13):
		dz->iparam[1] = (int)round(dz->param[1] * srate) * chans;
		/* fall thro */
	case(12):
		dz->iparam[0] = (int)round(dz->param[0] * srate) * chans;
		break;
	}
	switch(dz->mode) {
	case(4):
		if((!dz->brksize[0]) && (dz->param[0] == 1)) {
			sprintf(errstr,"No change in tempo.\n");
			return(DATA_ERROR);
		}
		dz->iparam[2] = (int)round(dz->param[2] * srate) * chans;
		dz->iparam[3] = (int)round(dz->param[3] * srate) * chans;
		dz->iparam[4] = (int)round(dz->param[4] * srate) * chans;
		/* fall thro */
	case(3):
		dz->iparam[1] = (int)round(dz->param[1] * MS_TO_SECS * srate) * chans;
		if((exit_status = locate_peaks(dz)) < 0)
			return(exit_status);
		break;
	case(5):
		dz->iparam[0] = (int)round((60.0/dz->param[0]) * srate);
		dz->iparam[1] = (int)round(dz->param[1] * srate) * chans;
		dz->iparam[2] = (int)round(dz->param[2] * MS_TO_SECS * srate) * chans;
		if(flteq(dz->param[3],0.0)) {
			sprintf(errstr,"Pregain is (effectively) zero: output will be silence.\n");
			return(DATA_ERROR);
		}
		break;
	case(6):
		dz->iparam[0] = (int)round(dz->param[0] * srate) * chans;
		dz->iparam[1] = (int)round(dz->param[1] * MS_TO_SECS * srate) * chans;
		if(flteq(dz->param[2],0.0)) {
			sprintf(errstr,"Pregain is (effectively) zero: output will be silence.\n");
			return(DATA_ERROR);
		}
		break;
	case(7):
		if(dz->param[MM] >= 20)
			dz->param[MM] = 60.0 / dz->param[MM];
		else if(dz->param[MM] > 1.0 || dz->param[MM] < FLTERR) {
			sprintf(errstr,"Output tempo vals must be > 0 AND less than 1 (beat duration) OR >= 20 (MM)\n");
			return(DATA_ERROR);
		}
		dz->iparam[MM] = (int)round(dz->param[MM] * srate) * chans;
		dz->iparam[BEAT_AT] = (int)round(dz->param[BEAT_AT] * srate) * chans;
		dz->iparam[BEAT_SILMIN] = (int)round(dz->param[BEAT_SILMIN] * MS_TO_SECS * srate) * chans;
		break;
	case(8):
		dz->iparam[0] = (int)round(dz->param[0] * MS_TO_SECS * srate) * chans;
		break;
	case(9):
		dz->iparam[0] = (int)round(dz->param[0] * MS_TO_SECS * srate) * chans;
		break;
	case(10):
		dz->iparam[0] = (int)round(dz->param[0] * MS_TO_SECS * srate) * chans;
		break;
	}
	return FINISHED;
}

/*************************************** READ_RETIME_DATA *********************************/

int read_retime_data(char *filename,dataptr dz)
{
	double d, lasttime;
	int *p, arraysize = 0;
	char temp[200], *q;
	FILE *fp;
	if((fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,"Failed to open file %s for input.\n",filename);
		return(DATA_ERROR);
	}
	while(fgets(temp,200,fp)!=NULL) {
		q = temp;
		while(get_float_from_within_string(&q,&d)) {
			arraysize++;
		}
	}	    
	dz->itemcnt = arraysize;	
	if((dz->lparray[0] = (int *) malloc(arraysize * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store retiming data.\n");
		return(MEMORY_ERROR);
	}
	p = dz->lparray[0];
	fseek(fp,0,0);
	lasttime = -1.0;
	while(fgets(temp,200,fp)!=NULL) {
		q = temp;
		while(get_float_from_within_string(&q,&d)) {
			if(d < 0.0 || d < lasttime) {
				sprintf(errstr,"TIME (%lf) OUT OF SEQUENCE OR INVALID, IN FILE %s.\n",d,filename);
				return(DATA_ERROR);
			}
			lasttime = d;
			*p = (int)(round(d * (double)dz->infile->srate) * dz->infile->channels); 
			p++;
		}
	}	    
	return(FINISHED);
}

/*************************************** READ_RETEMPO_DATA *********************************/

int read_retempo_data(char *filename,dataptr dz)
{
	double d, lastbeat, *p;
	int arraysize = 0, beatcnt;
	char temp[200], *q;
	FILE *fp;
	if((fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,"Failed to open file %s for input.\n",filename);
		return(DATA_ERROR);
	}
	while(fgets(temp,200,fp)!=NULL) {
		q = temp;
		while(get_float_from_within_string(&q,&d)) {
			arraysize++;
		}
	}	    
	dz->itemcnt = arraysize;	
	if((dz->parray[0] = (double *) malloc(arraysize * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store retiming data.\n");
		return(MEMORY_ERROR);
	}
	p = dz->parray[0];
	fseek(fp,0,0);
	lastbeat = -1.0;
	beatcnt = 1;
	while(fgets(temp,200,fp)!=NULL) {
		q = temp;
		while(get_float_from_within_string(&q,&d)) {
			if(d < dz->application->min_special || d > dz->application->max_special) {
				sprintf(errstr,"BEAT (%lf) OUT OF RANGE (%lf to %lf), IN FILE %s.\n",d,dz->application->min_special,dz->application->max_special,filename);
				return(DATA_ERROR);
			}
			if(d <= lastbeat) {
				sprintf(errstr,"BEAT %d (= %lf) OUT OF SEQUENCE IN FILE %s.\n",beatcnt,d,filename);
				return(DATA_ERROR);
			}
			lastbeat = d;
			*p = d; 
			p++;
			beatcnt++;
		}
	}	    
	return(FINISHED);
}

/*************************************** READ_RETIME_MASK *********************************/

int read_retime_mask(char *filename,dataptr dz)
{
	double d;
	int arraysize = 0, *p;
	char temp[200], *q;
	FILE *fp;
	if((fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,"Failed to open file %s for input.\n",filename);
		return(DATA_ERROR);
	}
	while(fgets(temp,200,fp)!=NULL) {
		q = temp;
		while(get_float_from_within_string(&q,&d)) {
			arraysize++;
		}
	}	    
	dz->itemcnt = arraysize;	
	if((dz->lparray[0] = (int *) malloc(arraysize * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store retiming data.\n");
		return(MEMORY_ERROR);
	}
	p = dz->lparray[0];
	fseek(fp,0,0);
	while(fgets(temp,200,fp)!=NULL) {
		q = temp;
		while(get_float_from_within_string(&q,&d)) {
			if(d < 0.0 || d > 1.0 || (d > 0.0 && d < 1.0)) {
				sprintf(errstr,"MASK VALUE (%lf) INVALID IN FILE %s.\n",d,filename);
				return(DATA_ERROR);
			}
			*p = (int)round(d); 
			p++;
		}
	}	    
	return(FINISHED);
}

/*************************************** READ_IDEAL_DATA *********************************/

int read_ideal_data(char *filename,dataptr dz)
{
	double d, lasttime0, lasttime1, *idealtimes, *realtimes;
	int finished;
	int *realtimesamps, arraysize = 0, cnt;
	char temp[200], *q;
	FILE *fp;
	if((fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,"Failed to open file %s for input.\n",filename);
		return(DATA_ERROR);
	}
	while(fgets(temp,200,fp)!=NULL) {
		q = temp;
		while(get_float_from_within_string(&q,&d)) {
			arraysize++;
		}
	}
	if(ODD(arraysize)) {
		sprintf(errstr,"Bad count of data in file %s.\n",filename);
		return(DATA_ERROR);
	}
	arraysize -= 2;
	if(arraysize <= 0) {
		sprintf(errstr,"Insufficient data in file %s.\n",filename);
		return(DATA_ERROR);
	}
	arraysize /= 2;
	dz->itemcnt = arraysize;	
	if((dz->parray[0] = (double *) malloc(arraysize * sizeof(double)))==NULL) {		// realtimes
		sprintf(errstr,"INSUFFICIENT MEMORY to store event data(1).\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[1] = (double *) malloc(arraysize * sizeof(double)))==NULL) {		// idealtimes
		sprintf(errstr,"INSUFFICIENT MEMORY to store event data(2).\n");
		return(MEMORY_ERROR);
	}
	if((dz->lparray[0] = (int *) malloc(arraysize * sizeof(int)))==NULL) {		// realtimes as samps
		sprintf(errstr,"INSUFFICIENT MEMORY to store event data(3).\n");
		return(MEMORY_ERROR);
	}
	if((dz->lparray[1] = (int *) malloc(arraysize * sizeof(int)))==NULL) {		// idealtimes as samps
		sprintf(errstr,"INSUFFICIENT MEMORY to store event data(4).\n");
		return(MEMORY_ERROR);
	}
	if((dz->lparray[2] = (int *) malloc(arraysize * 4 * sizeof(int)))==NULL) {	// realtime start & end of both splices, in samples
		sprintf(errstr,"INSUFFICIENT MEMORY to store event data(5).\n");
		return(MEMORY_ERROR);
	}
	finished = 0;
	realtimesamps  = dz->lparray[0];
	realtimes      = dz->parray[0];
	idealtimes     = dz->parray[1];
	fseek(fp,0,0);
	lasttime0 = -1.0;
	lasttime1 = -1.0;
	cnt = 0;
	while(fgets(temp,200,fp)!=NULL) {
		q = temp;
		while(get_float_from_within_string(&q,&d)) {
			switch(cnt) {
			case(0):
				if(d <= 10 || d >= 600) {
					sprintf(errstr,"MM (%lf) INVALID, IN FILE %s.\n",d,filename);
					return(DATA_ERROR);
				}
				dz->MMin = d;
				break;
			case(1):
				if(d < 0) {
					sprintf(errstr,"OFFSET (%lf) INVALID, IN FILE %s.\n",d,filename);
					return(DATA_ERROR);
				}
				dz->offset = d;
				break;
			default:
				if(EVEN(cnt)) {
					if(d < 0.0 || d < lasttime0) {
						sprintf(errstr,"LHS TIME (%lf) OUT OF SEQUENCE OR INVALID, IN FILE %s.\n",d,filename);
						return(DATA_ERROR);
					}
					*realtimes = d;
					realtimes++;
					*realtimesamps = (int)(round(d * (double)dz->infile->srate) * dz->infile->channels); 
					if(*realtimesamps >= dz->insams[0]) {
						dz->itemcnt = realtimesamps - dz->lparray[0];
						finished = 1;
					}
					realtimesamps++;
					lasttime0 = d;
				} else {
					if(d < 0.0 || d < lasttime1) {
						sprintf(errstr,"RHS TIME (%lf) OUT OF SEQUENCE OR INVALID, IN FILE %s.\n",d,filename);
						return(DATA_ERROR);
					}
					*idealtimes = d;
					idealtimes++;
					if(finished)
						break;
					lasttime1 = d;
				}
				break;
			}
			cnt++;
		}
	}
	if(dz->itemcnt <= 0) {
		sprintf(errstr,"LHS TIMES IN FILE %s ARE ALL BEYOND END OF SOUNDFILE.\n",filename);
		return(DATA_ERROR);
	}
	return(FINISHED);
}

/************************ HANDLE_THE_SPECIAL_DATA *********************/

int handle_the_special_data(int *cmdlinecnt,char ***cmdline,dataptr dz)
{
	int exit_status;
	aplptr ap = dz->application;

	if(ap->special_data) {
		if(!sloom) {
			if(*cmdlinecnt <= 0) {
				sprintf(errstr,"Insufficient parameters on command line.\n");
				return(USAGE_ONLY);
			}
		}
		if((exit_status = setup_the_special_data_ranges
		(dz->mode,dz->infile->srate,dz->duration,dz->nyquist,dz->wlength,dz->infile->channels,ap))<0)
			return(exit_status);
		if((exit_status = read_special_data((*cmdline)[0],dz))<0)
			return(exit_status);
		(*cmdline)++;		
		(*cmdlinecnt)--;
	}
	return(FINISHED);
}

/************************ SETUP_SPECIAL_DATA_RANGES *********************/

int setup_the_special_data_ranges(int mode,int srate,double duration,double nyquist,int wlength,int channels,aplptr ap)
{
	switch(ap->special_data) {
	case(RETIME_DATA):
	case(IDEAL_DATA):
	case(RETEMPO_DATA):
		ap->min_special 	= 0;
		ap->max_special 	= 3600;		// guestimate of highest poss user silence or max possible beat-cnt
		break;
	case(RETIME_FNAM):
		ap->special_range 	  = FALSE;
		break;
	case(RETIME_MASK):
		ap->min_special 	= 0;
		ap->max_special 	= 1;
		break;
	default:
		sprintf(errstr,"Unknown special_data type: setup_the_special_data_ranges()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/************************* READ_SPECIAL_DATA *******************/

int read_special_data(char *str,dataptr dz)	
{
	aplptr ap = dz->application;
	switch(ap->special_data) {
	case(RETIME_DATA): return read_retime_data(str,dz);
	case(IDEAL_DATA): return read_ideal_data(str,dz);
	case(RETEMPO_DATA): return read_retempo_data(str,dz);
	case(RETIME_FNAM): return read_retime_fnam(str,dz);
	case(RETIME_MASK): return read_retime_mask(str,dz);
	}
	return(FINISHED);
}

/*************************** CREATE_RETIME_SNDBUFS **************************/

int create_retime_sndbufs(dataptr dz)
{
	size_t bigbufsize;
	int n, step, in, out, minsize = 0;
	size_t framesize = dz->infile->channels * F_SECSIZE;
	int *realcuts, *chunklen;
	switch(dz->mode) {
	case(0):
		for(n=2;n < dz->itemcnt;n++) {
			step = dz->lparray[0][n] - dz->lparray[0][n-1];
			if(step > minsize)
				minsize = step * 4;
		}
		break;
	case(1):
		realcuts = dz->lparray[2];
		minsize = 0;
		for(n=0,in=0,out=3;n < dz->itemcnt;n++,in+=4,out+=4)
			minsize = max(minsize,realcuts[out] - realcuts[in]);
		break;
	case(2):
	case(5):
	case(6):
	case(8):
	case(9):
	case(10):
	case(11):
	case(12):
	case(13):
		minsize = framesize;
		break;
	case(3):
	case(4):
		chunklen = dz->lparray[1];
		minsize = 0;
		for(n=0;n < dz->itemcnt;n++)
			minsize = max(minsize,chunklen[n]);
		break;
	case(7):
		minsize = dz->iparam[MM] * (dz->iparam[BEAT_CNT] + 24);	/* 24 is a safety margin, in case grabbable beat is longer than defined */
		break;
	}
	if(dz->sbufptr == 0 || dz->sampbuf==0) {
		sprintf(errstr,"buffer pointers not allocated: create_retime_sndbufs()\n");
		return(PROGRAM_ERROR);
	}
	bigbufsize = (size_t)Malloc(-1);
	dz->buflen = (int)(bigbufsize / sizeof(float));	
	dz->buflen = (dz->buflen / (framesize * dz->bufcnt)) * (framesize * dz->bufcnt);
	dz->buflen /= dz->bufcnt;
	while(dz->buflen < minsize)
		dz->buflen += framesize;
	if(dz->buflen <= 0) {
		sprintf(errstr,"BUFFERS TOO LARGE.\n");
		return(PROGRAM_ERROR);
	}
	bigbufsize = dz->buflen * sizeof(float);
	if(bigbufsize * dz->bufcnt <= 0) {
		sprintf(errstr,"BUFFERS TOO LARGE.\n");
		return(PROGRAM_ERROR);
	}
	if((dz->bigbuf = (float *)malloc(bigbufsize * dz->bufcnt)) == NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
		return(PROGRAM_ERROR);
	}
	for(n=0;n<dz->bufcnt;n++)
		dz->sbufptr[n] = dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
	dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
	memset((char *)dz->sampbuf[0],0,dz->buflen * dz->bufcnt * sizeof(float));
	return(FINISHED);
}

/***************************** SETUP_INTERNAL_ARRAYS_AND_ARRAY_POINTERS **************************/

int setup_internal_arrays_and_array_pointers(dataptr dz)
{
	int n;		 
	dz->ptr_cnt    = -1;		/* base constructor...process */
	dz->array_cnt  = -1;
	dz->iarray_cnt = -1;
	dz->larray_cnt = -1;

	switch(dz->mode) {
	case(0):
		dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 1; dz->ptr_cnt = 0;	dz->fptr_cnt = 0;
		break;
	case(1):
		dz->array_cnt = 2; dz->iarray_cnt = 0; dz->larray_cnt = 3; dz->ptr_cnt = 0;	dz->fptr_cnt = 0;
		break;
	case(2):
		dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 1; dz->ptr_cnt = 0;	dz->fptr_cnt = 0;
		break;
	case(3):
	case(4):
	case(8):
		dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 2; dz->ptr_cnt = 0;	dz->fptr_cnt = 0;
		break;
	case(5):
	case(6):
		dz->array_cnt = 1; dz->iarray_cnt = 0; dz->larray_cnt = 1; dz->ptr_cnt = 0;	dz->fptr_cnt = 0;
		break;
	case(7):
		dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 1; dz->ptr_cnt = 0;	dz->fptr_cnt = 0;
		break;
	case(9):
		dz->array_cnt = 1; dz->iarray_cnt = 0; dz->larray_cnt = 2; dz->ptr_cnt = 0;	dz->fptr_cnt = 0;
		break;
	case(10):
		dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 1; dz->ptr_cnt = 0;	dz->fptr_cnt = 0;
		break;
	case(11):
	case(12):
	case(13):
		dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0;	dz->fptr_cnt = 0;
		break;
	}
	if(dz->array_cnt < 0 || dz->iarray_cnt < 0 || dz->larray_cnt < 0 || dz->ptr_cnt < 0 || dz->fptr_cnt < 0) {
		sprintf(errstr,"array_cnt not set in setup_internal_arrays_and_array_pointers()\n");	   
		return(PROGRAM_ERROR);
	}
	if(dz->array_cnt) {
		if((dz->parray  = (double **)malloc(dz->array_cnt * sizeof(double *)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for internal double arrays.\n");
			return(MEMORY_ERROR);
		}
		for(n=0;n<dz->array_cnt;n++)
			dz->parray[n] = NULL;
	}
	if(dz->larray_cnt) {
		if((dz->lparray  = (int **)malloc(dz->larray_cnt * sizeof(int *)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for internal long arrays.\n");
			return(MEMORY_ERROR);
		}
		for(n=0;n<dz->larray_cnt;n++)
			dz->lparray[n] = NULL;
	}
	return(FINISHED);
}

/************************* RETIME **********************************/

int retime(dataptr dz)
{
	int exit_status, insig;
	int chans = dz->infile->channels;
	double srate = dz->infile->srate;
	int *intimes =  dz->lparray[0];
	int outstep = dz->iparam[MM];
	float *ibuf = dz->sampbuf[0], *obuf = dz->sampbuf[2];
	int obufpos = 0;
	int maxsil = 4;
	int splicelen = (int)round(15.0 * MS_TO_SECS * srate) * chans;
	int winsize = (int)round(RETIME_WINSIZE * MS_TO_SECS * srate) * chans;
	int *silat, *sillen, *pos;
	float *env;
	int abs_read_start, read_end_in_buf, last_time_in_inbuf, next_time_in_inbuf, samps_to_copy, timescnt, lasttime, timestep, timechange;
	int n, m, j, k, silcnt = 0, small_loc, large_loc, this_splicelen, splicestart, splicend, min_energy_point = 0;
	int smallest, largest;
	double spliceincr, spliceval;
	int silence_to_set, this_timechange;
	if ((silat = (int *)malloc((maxsil+1) * sizeof(int)))==NULL) {
		sprintf(errstr,"Insufficient memory to store silence locations between beats\n");
		return(MEMORY_ERROR);
	}
	if ((sillen = (int *)malloc((maxsil+1) * sizeof(int)))==NULL) {
		sprintf(errstr,"Insufficient memory to store lengths of silences between beats\n");
		return(MEMORY_ERROR);
	}
	if ((env = (float *)malloc(RETIME_ENVSIZE * sizeof(float)))==NULL) {
		sprintf(errstr,"Insufficient memory to store envelope data\n");
		return(MEMORY_ERROR);
	}
	if ((pos = (int *)malloc(RETIME_ENVSIZE * sizeof(int)))==NULL) {
		sprintf(errstr,"Insufficient memory to store envelope positions\n");
		return(MEMORY_ERROR);
	}
	memset((char *)dz->bigbuf,0,5 * dz->buflen * sizeof(float));
													/* read sound into (double) inbuf */
	if((exit_status = read_samps(dz->sampbuf[0],dz)) < 0)
		return(exit_status);
	abs_read_start = 0;
	read_end_in_buf = dz->ssampsread;
	if(dz->ssampsread == dz->buflen) {
		if((exit_status = read_samps(dz->sampbuf[1],dz)) < 0) {
			return(exit_status);
		}
		read_end_in_buf += dz->ssampsread;
	}
	last_time_in_inbuf = 0;

	for(n=0;n<dz->itemcnt;n++) {
		if(intimes[n] >= dz->insams[0]) {
			dz->itemcnt = n;
			break;
		}
	}
	if(dz->itemcnt == 0) {
		sprintf(errstr,"ALL ENTERED TIMES ARE TOO LATE FOR THIS SOUND.\n");
		return(DATA_ERROR);
	}
	if(dz->itemcnt < 2) {
		sprintf(errstr,"TOO FEW ENTERED TIMES (MUST BE AT LEAST 2).\n");
		return(DATA_ERROR);
	}
	if(intimes[0] > 0) {							/* Deal with first time (copy initial block of sound) */
		samps_to_copy = intimes[0];
		while(samps_to_copy >= dz->buflen) {
			memcpy((char *)obuf,(char *)ibuf,dz->buflen * sizeof(float));
			obufpos = dz->buflen;
			if((exit_status = advance_obuf(&obufpos,dz)) < 0)
				return(exit_status);
			if((exit_status = advance_ibuf(&abs_read_start,&read_end_in_buf,&last_time_in_inbuf,&next_time_in_inbuf,&min_energy_point,silcnt,silat,dz)) < 0)
				return(exit_status);
			samps_to_copy -= dz->buflen;
		}
		if(samps_to_copy) {
			memcpy((char *)obuf,(char *)ibuf,samps_to_copy * sizeof(float));
			obufpos += samps_to_copy;
			if(obufpos >= dz->buflen * 2) {
				if((exit_status = advance_obuf(&obufpos,dz)) < 0)
					return(exit_status);
			}
		}
		last_time_in_inbuf = samps_to_copy;
	}
	timescnt = 1;
	lasttime = intimes[0];
	next_time_in_inbuf = lasttime - abs_read_start;
	for(timescnt = 1;timescnt < dz->itemcnt;timescnt++) {
		last_time_in_inbuf = next_time_in_inbuf;

		while(last_time_in_inbuf >= dz->buflen) {		/* advance in inbuf if ness */
			if((exit_status = advance_ibuf(&abs_read_start,&read_end_in_buf,&last_time_in_inbuf,&next_time_in_inbuf,&min_energy_point,silcnt,silat,dz)) < 0)
				return(exit_status);
		}
		next_time_in_inbuf = intimes[timescnt] - abs_read_start;
		timestep = next_time_in_inbuf - last_time_in_inbuf;
													/* Find step between marked times */
		timechange = outstep - timestep;			/* Find difference bedtween marked times and required times */


					/* IF SILENCE IS TO BE INSERTED */
		
		if(timechange > chans) {

					/* LOOK FOR EXISTING SILENCES */
		
			for(n=0;n<maxsil+1;n++)
				sillen[n] = 0;
			silcnt = 0;
			k = last_time_in_inbuf;
			insig = 1;
			while(k < next_time_in_inbuf) {
				if(ibuf[k] == 0.0) {
					insig = 0;
					if(sillen[silcnt] == 0)			/* If not started counting zero in this silence */
						silat[silcnt] = k;			/* mark where 1st zero is */
					sillen[silcnt]++;				/* count zeros */

				} else {
					if(!insig) {
						if(sillen[silcnt] >= chans * 2) {	/* if we've counted more than 'chan' zeros' in a previous silence */


							silcnt++;					/* this is a valid silence, keep it, and go to next */
							if(silcnt > maxsil) {		/* if we've used up the silence array */
								smallest = (int)floor(MAXINT);
								small_loc = 0;
								for(n=0;n<maxsil+1;n++) {	/* find shortest silence */
									if(sillen[n] <= smallest) {
										smallest = sillen[n];
										small_loc = n;
									}
								}						/* overwrite smallest silence, by shuffling vals back */
								for(n = small_loc+1;n<maxsil+1;n++) {
									sillen[n-1] = sillen[n];
									silat[n-1]  = silat[n];
								}						/* NB, if smallest is last, this will be overwritten by next silence, if any */
								silcnt--;
							}
						} else {
							sillen[silcnt] = 0;			/* else, invalid silence, reset silence-zeroconuter to zero */
						}
						insig = 1;
					}
				}
				k++;
			}

				/* IF SILENCES HAVE BEEN FOUND, INSERT MORE SILENCE WITHIN THE LARGEST OF THESE */
			if(silcnt > 0) {
				if(silcnt > 1) {
					largest = 0;
					large_loc = 0;
					for(n=0;n<silcnt;n++) {			/* find largest silence */
						if(sillen[n] > largest) {
							largest = sillen[n];
							large_loc = n;
						}
					}
					for(m=0;m<silcnt;m++) {			/* remove too small silences from silences list */
						if(m != large_loc) {
							if((double)largest/(double)sillen[m] > BIGGERSIL) {
								for(n = m+1;n<silcnt;n++) {
									sillen[n-1] = sillen[n];
									silat[n-1]  = silat[n];
								}					/* NB, if smallest is last, this will be overwritten by next silence, if any */
								if(large_loc > m)
									large_loc--;
								silcnt--;
								m--;
							}
						}
					}
				}
				while(timechange/silcnt < chans)  {		/* if the individual silence-inserts are too small, ignore some silences */
					smallest = (int)floor(MAXINT);
					small_loc = 0;
					for(n=0;n<silcnt;n++) {	/* find shortest silence */
						if(sillen[n] <= smallest) {
							smallest = sillen[n];
							small_loc = n;
						}
					}								/* overwrite smallest silence, by shuffling vals back */
					for(n = small_loc+1;n<silcnt;n++) {
						sillen[n-1] = sillen[n];
						silat[n-1]  = silat[n];
					}								/* NB, if smallest is last, this will be overwritten by next silence, if any */
					silcnt--;
				}
				this_timechange = timechange/silcnt;	/* individual inserted silences are 1-over-silcnt * actual timechange */
				this_timechange = (this_timechange/chans) * chans;
				if(this_timechange < chans)
					this_timechange = chans;
				for(n=0;n<silcnt;n++) {				/* insert the silences */
					samps_to_copy = silat[n] - last_time_in_inbuf;
					while(samps_to_copy >= dz->buflen) {
						memcpy((char *)(obuf+obufpos),(char *)(ibuf+last_time_in_inbuf),dz->buflen * sizeof(float));
						obufpos += dz->buflen;
						if(obufpos >= dz->buflen * 2) {
							if((exit_status = advance_obuf(&obufpos,dz)) < 0)
								return(exit_status);
						}
						last_time_in_inbuf += dz->buflen;
						if(last_time_in_inbuf >= dz->buflen) {
							if((exit_status = advance_ibuf(&abs_read_start,&read_end_in_buf,&last_time_in_inbuf,&next_time_in_inbuf,&min_energy_point,silcnt,silat,dz)) < 0)
								return(exit_status);
						}
						samps_to_copy -= dz->buflen;
					}
					if(samps_to_copy) {
						memcpy((char *)(obuf+obufpos),(char *)(ibuf+last_time_in_inbuf),samps_to_copy * sizeof(float));
						obufpos += samps_to_copy;
						if(obufpos >= dz->buflen * 2) {
							if((exit_status = advance_obuf(&obufpos,dz)) < 0)
								return(exit_status);
						}
						last_time_in_inbuf += samps_to_copy;
						if(last_time_in_inbuf >= dz->buflen) {
							if((exit_status = advance_ibuf(&abs_read_start,&read_end_in_buf,&last_time_in_inbuf,&next_time_in_inbuf,&min_energy_point,silcnt,silat,dz)) < 0)
								return(exit_status);
						}
					}
					silence_to_set = this_timechange;
					while(silence_to_set >= dz->buflen) {
						memset((char *)(obuf+obufpos),0,dz->buflen * sizeof(float));
						obufpos += dz->buflen;
						if(obufpos >= dz->buflen * 2) {
							if((exit_status = advance_obuf(&obufpos,dz)) < 0)
								return(exit_status);
						}
						silence_to_set -= dz->buflen;
					}
					if(silence_to_set) {
						memset((char *)(obuf+obufpos),0,silence_to_set * sizeof(float));
						obufpos += silence_to_set;
						if(obufpos >= dz->buflen * 2) {
							if((exit_status = advance_obuf(&obufpos,dz)) < 0)
								return(exit_status);
						}
					}					
					last_time_in_inbuf = silat[n];

				}
												/* copy any remaining insound before next time-marker */
				samps_to_copy = next_time_in_inbuf - last_time_in_inbuf;

				while(samps_to_copy >= dz->buflen) {
					memcpy((char *)(obuf+obufpos),(char *)(ibuf+last_time_in_inbuf),dz->buflen * sizeof(float));
					obufpos += dz->buflen;
					if(obufpos >= dz->buflen * 2) {
						if((exit_status = advance_obuf(&obufpos,dz)) < 0)
							return(exit_status);
					}
					last_time_in_inbuf += dz->buflen;
					if(last_time_in_inbuf >= dz->buflen) {
						if((exit_status = advance_ibuf(&abs_read_start,&read_end_in_buf,&last_time_in_inbuf,&next_time_in_inbuf,&min_energy_point,silcnt,silat,dz)) < 0)
								return(exit_status);
					}
					samps_to_copy -= dz->buflen;
				}
				if(samps_to_copy) {
					memcpy((char *)(obuf+obufpos),(char *)(ibuf+last_time_in_inbuf),samps_to_copy * sizeof(float));
					obufpos += samps_to_copy;
					if(obufpos >= dz->buflen * 2) {
						if((exit_status = advance_obuf(&obufpos,dz)) < 0)
							return(exit_status);
					}
					last_time_in_inbuf += samps_to_copy;
					if(last_time_in_inbuf >= dz->buflen) {
						if((exit_status = advance_ibuf(&abs_read_start,&read_end_in_buf,&last_time_in_inbuf,&next_time_in_inbuf,&min_energy_point,silcnt,silat,dz)) < 0)
								return(exit_status);
					}
				}

			} else {

				/* ELSE, NO EXISTING SILENCES ... INSERTING SILENCE AT MIN ENERGY POINT */
		
				if((exit_status = find_min_energy_point(&min_energy_point,last_time_in_inbuf,next_time_in_inbuf,winsize,ibuf,env,pos,dz)) < 0)
					return(exit_status);
				samps_to_copy = min_energy_point - last_time_in_inbuf;
				while(samps_to_copy >= dz->buflen) {
					memcpy((char *)(obuf+obufpos),(char *)(ibuf+last_time_in_inbuf),dz->buflen * sizeof(float));
					obufpos += dz->buflen;
					if(obufpos >= dz->buflen * 2) {
						if((exit_status = advance_obuf(&obufpos,dz)) < 0)
							return(exit_status);
					}
					last_time_in_inbuf += dz->buflen;
					if(last_time_in_inbuf >= dz->buflen) {		/* advance in inbuf if ness */
						if((exit_status = advance_ibuf(&abs_read_start,&read_end_in_buf,&last_time_in_inbuf,&next_time_in_inbuf,&min_energy_point,silcnt,silat,dz)) < 0)
							return(exit_status);
					}
					samps_to_copy -= dz->buflen;
				}
				if(samps_to_copy) {
					memcpy((char *)(obuf+obufpos),(char *)(ibuf+last_time_in_inbuf),samps_to_copy * sizeof(float));
					obufpos += samps_to_copy;		/* copy to min energy pont */
					if(obufpos >= dz->buflen * 2) {
						if((exit_status = advance_obuf(&obufpos,dz)) < 0)
							return(exit_status);
					}								/* insert silence */
					last_time_in_inbuf += samps_to_copy;
					if(last_time_in_inbuf >= dz->buflen) {		/* advance in inbuf if ness */
						if((exit_status = advance_ibuf(&abs_read_start,&read_end_in_buf,&last_time_in_inbuf,&next_time_in_inbuf,&min_energy_point,silcnt,silat,dz)) < 0)
							return(exit_status);
					}
				}				
				
				while(timechange >= dz->buflen) {
					memset((char *)(obuf+obufpos),0,dz->buflen * sizeof(float));
					obufpos += dz->buflen;
					if(obufpos >= dz->buflen * 2) {
						if((exit_status = advance_obuf(&obufpos,dz)) < 0)
							return(exit_status);
					}
					timechange -= dz->buflen;
				}
				if(timechange) {
					memset((char *)(obuf+obufpos),0,timechange * sizeof(float));
					obufpos += timechange;
					if(obufpos >= dz->buflen * 2) {
						if((exit_status = advance_obuf(&obufpos,dz)) < 0)
							return(exit_status);
					}								/* copy from min energy point to next-time */
				}
				last_time_in_inbuf = min_energy_point;
				samps_to_copy = next_time_in_inbuf - min_energy_point;

				while(samps_to_copy >= dz->buflen) {
					memcpy((char *)(obuf+obufpos),(char *)(ibuf+last_time_in_inbuf),dz->buflen * sizeof(float));
					obufpos += dz->buflen;
					if(obufpos >= dz->buflen * 2) {
						if((exit_status = advance_obuf(&obufpos,dz)) < 0)
							return(exit_status);
					}
					last_time_in_inbuf += dz->buflen;
					if(last_time_in_inbuf >= dz->buflen) {		/* advance in inbuf if ness */
						if((exit_status = advance_ibuf(&abs_read_start,&read_end_in_buf,&last_time_in_inbuf,&next_time_in_inbuf,&min_energy_point,silcnt,silat,dz)) < 0)
							return(exit_status);
					}
					samps_to_copy -= dz->buflen;
				}
				if(samps_to_copy) {
					memcpy((char *)(obuf+obufpos),(char *)(ibuf+last_time_in_inbuf),samps_to_copy * sizeof(float));
					obufpos += samps_to_copy;
					if(obufpos >= dz->buflen * 2) {
						if((exit_status = advance_obuf(&obufpos,dz)) < 0)
							return(exit_status);
					}
					last_time_in_inbuf += samps_to_copy;
					if(last_time_in_inbuf >= dz->buflen) {		/* advance in inbuf if ness */
						if((exit_status = advance_ibuf(&abs_read_start,&read_end_in_buf,&last_time_in_inbuf,&next_time_in_inbuf,&min_energy_point,silcnt,silat,dz)) < 0)
							return(exit_status);
					}
				}
			}
		} else if(timechange < -chans) {		/* Time must be removed, do overlap at min-level point */
			if((exit_status = find_min_energy_point(&min_energy_point,last_time_in_inbuf,next_time_in_inbuf,winsize,ibuf,env,pos,dz)) < 0)
				return(exit_status);
												/* Copy samples up to min energy point */
			samps_to_copy = min_energy_point - last_time_in_inbuf;
			this_splicelen = min(samps_to_copy,splicelen);
			splicestart = samps_to_copy - this_splicelen;
			spliceincr = 1.0 / this_splicelen;
			spliceval = 1.0;
			k = 0;
			while(samps_to_copy >= dz->buflen) {			
				for(j=0;j < dz->buflen;j++) {
					if(k >= splicestart)
						spliceval -= spliceincr;
					obuf[obufpos] = (float) (obuf[obufpos] + (ibuf[last_time_in_inbuf] * spliceval));
					last_time_in_inbuf++;
					if(last_time_in_inbuf >= dz->buflen) {		/* advance in inbuf if ness */
						if((exit_status = advance_ibuf(&abs_read_start,&read_end_in_buf,&last_time_in_inbuf,&next_time_in_inbuf,&min_energy_point,silcnt,silat,dz)) < 0)
							return(exit_status);
					}
					obufpos++;
					k++;
				}
				if(obufpos >= dz->buflen * 2) {
					if((exit_status = advance_obuf(&obufpos,dz)) < 0)
						return(exit_status);
				}
				samps_to_copy -= dz->buflen;
			}			
			while(samps_to_copy > 0) {
				if(k >= splicestart)
					spliceval -= spliceincr;
				obuf[obufpos] = (float) (obuf[obufpos] + (ibuf[last_time_in_inbuf] * spliceval));
				last_time_in_inbuf++;
				obufpos++;
				samps_to_copy--;
				k++;
			}
			obufpos += timechange;					/* step back in obuf, (timechange is -ve) so samples overlap, shortening obuf file here */
			if(obufpos < 0) {
				fprintf(stdout,"WARNING: Time modfification too extreme for buffers: Quitting before end of input sound.\n");
				fflush(stdout);
				obufpos -= timechange;
				break;
			}	
			if(obufpos >= dz->buflen * 2) {
				if((exit_status = advance_obuf(&obufpos,dz)) < 0)
					return(exit_status);	
			}									/* Copy samples from min energy point to next specified time */
			samps_to_copy = next_time_in_inbuf - min_energy_point;
			this_splicelen = min(samps_to_copy,splicelen);
			splicend = this_splicelen;
			spliceincr = 1.0 / this_splicelen;
			spliceval = 0.0;
			k = 0;
			while(samps_to_copy >= dz->buflen) {
				for(j=0;j < dz->buflen;j++) {
					if(k < splicend)
						spliceval += spliceincr;
					obuf[obufpos] = (float) (obuf[obufpos] + (ibuf[last_time_in_inbuf] * spliceval));
					last_time_in_inbuf++;
					if(last_time_in_inbuf >= dz->buflen) {		/* advance in inbuf if ness */
						if((exit_status = advance_ibuf(&abs_read_start,&read_end_in_buf,&last_time_in_inbuf,&next_time_in_inbuf,&min_energy_point,silcnt,silat,dz)) < 0)
							return(exit_status);
					}
					obufpos++;
					samps_to_copy--;
					k++;
				}
				if(obufpos >= dz->buflen * 2) {
					if((exit_status = advance_obuf(&obufpos,dz)) < 0)
						return(exit_status);
				}
				samps_to_copy -= dz->buflen;
			}			
			while(samps_to_copy > 0) {
				if(k < splicend)
					spliceval += spliceincr;
				obuf[obufpos] = (float) (obuf[obufpos] + (ibuf[last_time_in_inbuf] * spliceval));
				last_time_in_inbuf++;
				if(last_time_in_inbuf >= dz->buflen) {		/* advance in inbuf if ness */
					if((exit_status = advance_ibuf(&abs_read_start,&read_end_in_buf,&last_time_in_inbuf,&next_time_in_inbuf,&min_energy_point,silcnt,silat,dz)) < 0)
						return(exit_status);
				}
				obufpos++;
				samps_to_copy--;
				k++;
			}
			if(obufpos >= dz->buflen * 2) {
				if((exit_status = advance_obuf(&obufpos,dz)) < 0)
					return(exit_status);
			}
		}
	}
	if(obufpos > 0) {
		if((exit_status = write_samps(dz->sampbuf[2],obufpos,dz))<0)
			return(exit_status);
	}
	if(read_end_in_buf > last_time_in_inbuf) {
		if((exit_status = write_samps(dz->sampbuf[0] + last_time_in_inbuf,read_end_in_buf - last_time_in_inbuf,dz))<0)	/* Write ALL samps remaining in inbuf */
			return(exit_status);
	}	
	while(dz->total_samps_read < dz->insams[0]) {						/* Read and write any remaining samples in infile */
		if((exit_status = read_samps(dz->sampbuf[0],dz)) < 0)
			return(exit_status);
		if((exit_status = write_samps(dz->sampbuf[0],dz->ssampsread,dz))<0)
			return(exit_status);
	}
	return FINISHED;
}

/******************************************* ADVANCE_OBUF *******************************************/

int advance_obuf(int *obufpos,dataptr dz) 
{
	int exit_status;
	if((exit_status = write_samps(dz->sampbuf[2],dz->buflen,dz))<0)
		return(exit_status);
	memcpy((char *)dz->sampbuf[2],(char *)dz->sampbuf[3],dz->buflen * sizeof(float));
	memcpy((char *)dz->sampbuf[3],(char *)dz->sampbuf[4],dz->buflen * sizeof(float));
	memset((char *)dz->sampbuf[4],0,dz->buflen * sizeof(float));
	*obufpos -= dz->buflen;
	return FINISHED;
}

/******************************************* ADVANCE_IBUF *******************************************/

int advance_ibuf(int *abs_read_start,int *read_end_in_buf,int *last_time_in_inbuf,int *next_time_in_inbuf,int *min_energy_point,int silcnt,int *silat,dataptr dz) 
{
	int exit_status, n;
	memcpy((char *)dz->sampbuf[0],(char *)dz->sampbuf[1],dz->buflen * sizeof(float));
	memset((char *)dz->sampbuf[1],0,dz->buflen * sizeof(float));
	*read_end_in_buf -= dz->buflen;
	*last_time_in_inbuf -= dz->buflen;
	*next_time_in_inbuf -= dz->buflen;
	*min_energy_point   -= dz->buflen;
	if(silcnt > 0) {
		for(n= 0;n < silcnt;n++)
			silat[n] -= dz->buflen;
	}
	if((exit_status = read_samps(dz->sampbuf[1],dz)) < 0)
		return(exit_status);
	if(dz->ssampsread > 0) {
		*abs_read_start  += dz->buflen;
		*read_end_in_buf += dz->ssampsread;
	}
	return(FINISHED);
}

/******************************************* FIND_MIN_ENERGY_POINT *******************************************/

int find_min_energy_point(int *min_energy_point,int last_time_in_inbuf,int next_time_in_inbuf,int winsize,float *ibuf,float *env,int *pos,dataptr dz)
{
	int envcnt = 0;
	int envarraycnt = 0;
	double maxsamp = 0.0, minsamp, max_local;
	int dist, k, minloc, startt, endd;
	int chans = dz->infile->channels, phase;
	float min_energy;
	int minpos;
	if((dist = next_time_in_inbuf - last_time_in_inbuf) >= winsize * 2) {

			/* FIND GROSS ENVELOPE USING 20mS WINDOWS */

		for(k = last_time_in_inbuf;k < next_time_in_inbuf; k++,dist--) {
			if(envarraycnt >= RETIME_ENVSIZE) {
				sprintf(errstr,"Envelope overflow at gross envelope stage\n");
				return(MEMORY_ERROR);
			}
			maxsamp = max(fabs(ibuf[k]),maxsamp);
			envcnt++;
			if((envcnt >= winsize) && (dist > winsize)) {
				env[envarraycnt++] = (float)maxsamp;
				envcnt = 0;
			}							/* incorporate any final window < wsize in length, into final window */
		}
		if(envcnt > 0)
			envarraycnt++;

			/* FIND MINIMUM IN GROSS ENVELOPE */
				
		minsamp = HUGE;
		minloc = 0;
		for(k=0;k < envarraycnt;k++) {	/* find minimum in envelope */
			if(env[k] < minsamp) {
				minsamp = env[k];
				minloc = k;
			}
		}								
		
		/* FIND START AND END OF MINIMUM ENERGY WINDOW */

		startt = last_time_in_inbuf + (minloc * winsize);
		if(k == envcnt - 1)
			endd = next_time_in_inbuf;
		else
			endd = last_time_in_inbuf + ((minloc+1) * winsize);
	} else {

		/* IF TIMESTEP TOO SHORT FOR 20mS WINDOWING, USE WHOLE LENGTH OF TIMESTEP INSTEAD */
				
		startt = last_time_in_inbuf;
		endd   = next_time_in_inbuf;
	}
		/*		FIND MINIMUM ENERGY POINT */

	k = startt;
	while(ibuf[k] == 0.0) {
		k++;
		if(k == endd) {					/* if entire window is zero, put min-energy point in middle of it */
			*min_energy_point = (((startt + endd)/2)/chans) * chans;
			return(FINISHED);
		}
	}
	max_local = 0.0;
	envcnt = 0;
	if(ibuf[k] > 0.0)
		phase = 1;
	else
		phase = -1;
	while(k < endd) {
		if(envcnt >= RETIME_ENVSIZE) {
			sprintf(errstr,"Envelope overflow at min-energy stage\n");
			return(MEMORY_ERROR);
		}
		switch(phase) {
		case(1):
			if(ibuf[k] < 0.0) {				/* If phase has changed */
				env[envcnt++] = (float)max_local;	/* store local energy peak, and go to next envelope-store location */
				max_local = 0.0;
				phase = -1;
			} else {
				if(ibuf[k] > max_local) {	/* get local energy peak, and its location */
					pos[envcnt] = k;
					max_local = ibuf[k];
				}
				k++;
			}
			break;
		case(-1):
			if(ibuf[k] > 0.0) {
				env[envcnt] = (float)max_local;
				pos[envcnt++] = k;
				max_local = 0.0;
				phase = 1;
			} else {
				if(fabs(ibuf[k]) > max_local) {
					pos[envcnt] = k;
					max_local = fabs(ibuf[k]);
				}
				k++;
			}
			break;
		}
	}
	env[envcnt++] = (float)max_local;

		/* FIND MINIMUM LOCAL ENERGY-PEAK */	
		
	min_energy = env[0];
	minpos = pos[0];
	for(k = 1;k<envcnt;k++) {
		if(env[k] < min_energy) {
			min_energy = env[k];
			minpos = pos[k];
		}
	}

	/* FIND FIRST ZERO-CROSSING AFTER (OR BEFORE) MIN LOCAL ENERGY-PEAK */
	
	k = minpos;								
	if(ibuf[k] > 0.0) {
		while(ibuf[k] > 0.0) {
			k++;
			if(k == endd) {
				k = minpos;
				while(ibuf[k] > 0.0) {
					k--;
					if(k == startt) {
						*min_energy_point = k;
						fprintf(stdout,"WARNING: Failed to find zero-crossing: try filtering out lowest frqs\n");
						fflush(stdout);
						return(FINISHED);
					}
				}
			}
		}
	} else if(ibuf[k] < 0.0) {
		while(ibuf[k] < 0.0) {
			k++;
			if(k == endd) {
				k = minpos;
				while(ibuf[k] < 0.0) {
					k--;
					if(k == startt) {
						*min_energy_point = k;
						fprintf(stdout,"WARNING: Failed to find zero-crossing: try filtering out lowest frqs\n");
						fflush(stdout);
						return(FINISHED);
					}
				}
			}
		}
	} 
	*min_energy_point = k;
	return(FINISHED);
}

/******************************************* IDEAL_RETIME *******************************************/

int ideal_retime(dataptr dz)
{
	int exit_status;
	int chans = dz->infile->channels, k, thisevent;
	int *cuts = dz->lparray[2], in, on, off, out;	// in = start of upsplice, on = start of full-level chunk, off = end of full-level chunk, out = end of downsplice
	int *idealsamps  = dz->lparray[1];
	float *ibuf = dz->sampbuf[0], *obuf = dz->sampbuf[1], *ovflowbuf = dz->sampbuf[2], *splbuf = dz->sampbuf[3];
	int obufpos, lastwrite, ibufstart, obufstart, overflow, chunklen, upsplen, dnsplen, upsplicecnt, dnsplicecnt, n, m;
	double upincr, dnincr, splval;

	memset((char *)obuf,0,2 * dz->buflen * sizeof(float));
	memset((char *)splbuf,0,dz->buflen * sizeof(float));
	ibufstart = 0;
	obufstart = 0;
	lastwrite = 0;

	/* FOR EVERY OUTCHUNK and INCHUNK */

	for(thisevent=0,in=0,on=1,off=2,out=3;thisevent<dz->itemcnt;thisevent++,in+=4,on+=4,off+=4,out+=4) {

		obufpos = idealsamps[thisevent] - obufstart;
		while(obufpos >= dz->buflen) {						// WHERE NESS, WRITE COMPLETED BUFFERS OR SILENT BUFS (e.g. at start) 
			if((exit_status = write_samps(obuf,dz->buflen,dz)) < 0)
				return exit_status;							
			memset((char *)obuf,0,dz->buflen * sizeof(float));
			if((overflow = lastwrite - dz->buflen) > 0) {	// IF LAST WRITE SPILLED OVER INTO OVERFLOW BUFFER, COPY OVERFLOW INTO MAIN OBUF
				memcpy((char *)obuf,(char*)ovflowbuf,overflow * sizeof(float));
				memset((char *)ovflowbuf,0,dz->buflen * sizeof(float));
			}
			lastwrite = 0;
			obufpos -= dz->buflen;
			obufstart += dz->buflen;						// INCREMENTING THE SAMPLCNT OF obufstart
		}
		chunklen = cuts[out] - cuts[in];					// GET DATA FOR THIS CHUNK
		upsplen  = cuts[on]  - cuts[in];
		dnsplen  = cuts[out] - cuts[off];

		m = cuts[in] - ibufstart;

		// COPY CHUNK FROM INBUF TO SPLICE BUF

		for(n = 0;n < chunklen;n++) {
			while(m >= dz->ssampsread) {				// WHERE NECESSARY, SKIP TO NEXT INBUF
				m -= dz->ssampsread;
				ibufstart += dz->ssampsread;			// INCREMENTING THE SAMPLCNT OF ibufstart
				if((exit_status = read_samps(ibuf,dz)) < 0)
					return exit_status;
				if(dz->ssampsread == 0) {				//	IF END OF FILE REACHED BEFORE END OF CHUNK
					chunklen = n;						//	SHORTEN CHUNK
					dnsplen  = 0;						//	REMOVE DOWNSPLICE AT END
					dz->itemcnt = thisevent;			//	FORCE INNER LOOP EXIT
					break;
				}
			}
			if(dz->ssampsread == 0)						//	AND FORCE OUTER LOOP EXIT
				break;
			splbuf[n] = ibuf[m];
			m++;
		}

		// DO SPLICES IN SPLICE BUF
		
		upsplicecnt = upsplen/chans;
		upincr = 1.0 /(double)upsplicecnt;
		splval = 0.0;
		m = 0;
		for(n=0;n<upsplicecnt;n++) {
			for(k=0;k<chans;k++)
				splbuf[m+k] = (float)(splbuf[m+k] * splval);
			m += chans;
			splval += upincr;
		}
		m = chunklen - dnsplen;
		dnsplicecnt = dnsplen/chans;
		dnincr = 1.0 /(double)dnsplicecnt;
		splval = 1.0 - dnincr;
		for(n=0;n<dnsplicecnt;n++) {
			for(k=0;k<chans;k++)
				splbuf[m+k] = (float)(splbuf[m+k] * splval);
			m += chans;
			splval -= dnincr;
		}

		// ADD CHUNK FROM SPLICE BUF INTO OUTBUF

		for(n = 0;n < chunklen;n++) {
			obuf[obufpos] = (float)(obuf[obufpos] + splbuf[n]);
			obufpos++;		/* NB overflow allowed here */
		}

		// NOTE WHERE FARTHEST WRITE WAS MADE

		lastwrite = max(lastwrite,obufpos);
	}
	if(lastwrite > 0) {
		if((exit_status = write_samps(obuf,lastwrite,dz)) < 0)
			return exit_status;
	}
	return FINISHED;
}

/*************************** SHAVE ****************************/

int shave(dataptr dz)
{
	int exit_status, chans = dz->infile->channels, warned;
	float *ibuf = dz->sampbuf[0];
	int *events;
	int ibufpos = 0, buf_start, n, m, j, total_shrunk_dur, evlen, data_end;
	int eventsegscnt, orig_eventsegscnt, cnt;
	double splic, splicincr;
	int event_start, orig_event_end, new_event_end, new_event_splice_end;

	splicincr = 1.0/(double)(dz->iparam[2]/chans);

	if((exit_status = count_events(0,0,&eventsegscnt,dz))<0) {
		return(exit_status);
	}
	events = dz->lparray[0];

	/* ELIMININATE ALL EVENTS WHICH CANNOT BE SHORTENED
	 *							   new
	 *	retained				   end
	 *  _________________________splice
	 * |						 \	   : old
	 * |	orig event			  \	   : end
	 * |    - - - - - - - - - - - -\- -: splice
	 * |  /							\  :\
	 * | /							 \ :  \
	 * |/  --- found dur of old event-\-----\
	 *								   : 	
	 *								 Limit
	 *								   :
	 */
	warned = 0;
	orig_eventsegscnt = eventsegscnt;
// JUNE 2010
	total_shrunk_dur = dz->iparam[1] + dz->iparam[3];
	for(n=0;n<eventsegscnt;n+=2) {
		evlen = events[n+1] - events[n];
		evlen -= dz->iparam[3];
		if(evlen  <= 0) {
			if(!warned) {
				fprintf(stdout,"WARNING: Some events are shorter than the 'splice length' specified.\n");
				fflush(stdout);
				warned = 1;
			}
		}
		if(evlen <= total_shrunk_dur) {
			for(m = n+2; m < eventsegscnt; m++)
				events[m-2] = events[m];
			eventsegscnt -= 2;
			n -= 2;
		}
	}
	if(eventsegscnt <= 0) {
		sprintf(errstr,"No Events Will Be Shortened, With These Parameter Settings.\n");
		return(DATA_ERROR);
	} else {
		fprintf(stdout,"INFO: %d of %d events will be shortened\n",eventsegscnt/2,orig_eventsegscnt/2);
		fflush(stdout);
	}
    if((sndseekEx(dz->ifd[0],0,0)<0)){        
        sprintf(errstr,"sndseek() failed\n");
        return SYSTEM_ERROR;
    }
	reset_filedata_counters(dz);
	buf_start = 0;
	dz->ssampsread = 0;
	cnt = 0;
	event_start	   = events[cnt++];
	orig_event_end = events[cnt++];
//JUNE 2010
	new_event_end  = event_start + dz->iparam[2];
//JUNE 2010
	new_event_splice_end  = new_event_end + dz->iparam[3];
	if((exit_status = read_samps(ibuf,dz))<0) {
		sprintf(errstr,"Failed to read data from sndfile.\n");
		return(DATA_ERROR);
	}
	while(dz->ssampsread) {
		while(event_start >= dz->buflen) {
			if((exit_status = write_samps(ibuf,dz->buflen,dz))<0) {
				sprintf(errstr,"Failed to write data to sndfile.\n");
				return(DATA_ERROR);
			}
			buf_start += dz->buflen;
			event_start    -= dz->buflen;
			new_event_end  -= dz->buflen;
			new_event_splice_end -= dz->buflen;
			orig_event_end -= dz->buflen;
			if((exit_status = read_samps(ibuf,dz))<0) {
				sprintf(errstr,"Failed to read data from sndfile.\n");
				return(DATA_ERROR);
			}
		}
		ibufpos = event_start;
		while(ibufpos < new_event_end) {
			if(ibufpos >= dz->buflen) {
				if((exit_status = write_samps(ibuf,dz->buflen,dz))<0) {
					sprintf(errstr,"Failed to write data to sndfile.\n");
					return(DATA_ERROR);
				}
				buf_start += dz->buflen;
				new_event_end  -= dz->buflen;
				new_event_splice_end -= dz->buflen;
				orig_event_end -= dz->buflen;
				if((exit_status = read_samps(ibuf,dz))<0) {
					sprintf(errstr,"Failed to read data from sndfile.\n");
					return(DATA_ERROR);
				}
				ibufpos = 0;
			}
			ibufpos++;
		}
		splic = 1.0 - splicincr;
		while(ibufpos < new_event_splice_end) {
			if(ibufpos >= dz->buflen) {
				if((exit_status = write_samps(ibuf,dz->buflen,dz))<0) {
					sprintf(errstr,"Failed to write data to sndfile.\n");
					return(DATA_ERROR);
				}
				buf_start += dz->buflen;
				new_event_splice_end -= dz->buflen;
				orig_event_end -= dz->buflen;
				if((exit_status = read_samps(ibuf,dz))<0) {
					sprintf(errstr,"Failed to read data from sndfile.\n");
					return(DATA_ERROR);
				}
				ibufpos = 0;
			}
			for(j=0;j<chans;j++)
				ibuf[ibufpos+j] = (float)(ibuf[ibufpos+j] * splic);
			splic -= splicincr;
			ibufpos += chans;
		}	
		if(cnt >= eventsegscnt)
			break;
		while(ibufpos < orig_event_end) {
			if(ibufpos >= dz->buflen) {
				if((exit_status = write_samps(ibuf,dz->buflen,dz))<0) {
					sprintf(errstr,"Failed to write data to sndfile.\n");
					return(DATA_ERROR);
				}
				buf_start += dz->buflen;
				orig_event_end -= dz->buflen;
				if((exit_status = read_samps(ibuf,dz))<0) {
					sprintf(errstr,"Failed to read data from sndfile.\n");
					return(DATA_ERROR);
				}
				ibufpos = 0;
			}
			ibuf[ibufpos++] = 0.0f;
		}
		if(cnt >= eventsegscnt)		//	SAFETY, in case of accounting error
			break;
		event_start    = events[cnt++] - buf_start;
		orig_event_end = events[cnt++] - buf_start;
//JUNE 2010
		new_event_end  = event_start + dz->iparam[2];
//JUNE 2010
		new_event_splice_end = new_event_end + dz->iparam[3];
		data_end = dz->insams[0] - buf_start;
		if(new_event_splice_end >= data_end) {
			new_event_end = data_end;
			new_event_splice_end  = data_end;
		}
	}
	if(ibufpos > 0) {
		if((exit_status = write_samps(ibuf,ibufpos,dz))<0) {
			sprintf(errstr,"Failed to write data to sndfile.\n");
			return(DATA_ERROR);
		}
	}
	return FINISHED;
}

/******************************************* LOCATE-PEAKS *******************************************/

#define MAX_CHUNKS	(4000)

int locate_peaks(dataptr dz)
{
	int exit_status;
	double srate = (double)dz->infile->srate;
	int chans = dz->infile->channels;
	float *tempibuf;
	int inchunk = 0;
	int silcnt = 0;
	int thischunklen = 0, thischunkcounter = 0, chunkcnt = 0, n, bufstart;
	int *chunkat, *chunklen;
	int minsil = (int)round((dz->param[1] * MS_TO_SECS) * srate) * chans;

	dz->buflen = F_SECSIZE * chans;
	if((tempibuf = (float *)malloc(dz->buflen * sizeof(float)))==NULL) {
		sprintf(errstr,"Insufficient memory create temp inbuf to read chunklens.\n");
		return(MEMORY_ERROR);
	}
	if((dz->lparray[0] = (int *)malloc(MAX_CHUNKS * sizeof(int)))==NULL) {
		sprintf(errstr,"Insufficient memory to store peak positions\n");
		return(MEMORY_ERROR);
	}
	if((dz->lparray[1] = (int *)malloc(MAX_CHUNKS * sizeof(int)))==NULL) {
		sprintf(errstr,"Insufficient memory to store peak widths\n");
		return(MEMORY_ERROR);
	}
	chunkat  = dz->lparray[0];
	chunklen = dz->lparray[1];

	if((exit_status = read_samps(tempibuf,dz)) < 0)
		return exit_status;
	bufstart = 0;
	while (dz->ssampsread > 0) {
		for(n=0;n<dz->ssampsread;n++) {
			if(tempibuf[n] > 0 || tempibuf[n] < 0) {
				if(silcnt && (thischunkcounter > 0))	//	If we've been in silence, and still counting chunk
					thischunklen = thischunkcounter;	//	Reset chunklen to chunkcounter
				silcnt = 0;
				if(inchunk)	{							//	If in a chunk, keep counting until encountering silence
					thischunklen++;
					thischunkcounter++;
				} else {
					chunkat[chunkcnt] = bufstart + n;	//	If just found chunk start, set up a new chunk and start counting it
					inchunk = 1;
					thischunklen = 1;
					thischunkcounter = 1;
				}
			} else {
				if(inchunk) {								//	If (was) in a chunk, but come into silence
					thischunkcounter++;						//	chunklen stops counting (assuming chunk has ended) BUT
															//	thischunkcounter continues to count in case this is a silence WITHIN an event
					if(silcnt >= minsil) {					//	Once we're sure we have minimum silence gap
						chunklen[chunkcnt] = thischunklen;	//	Then save the chunk
						inchunk = 0;
						thischunklen = 0;
						thischunkcounter = 0;				//	Set chunkcounter back to zero
						chunkcnt++;
						if(chunkcnt >= MAX_CHUNKS) {
							sprintf(errstr,"More than %d chunks : Cannot proceed\n",MAX_CHUNKS);
							return(DATA_ERROR);
						}
					}
				}
				silcnt++;									//	Count silence
			}
		}
		bufstart += dz->ssampsread;
		if((exit_status = read_samps(tempibuf,dz)) < 0)
			return exit_status;
	}
	if(inchunk) {
		chunklen[chunkcnt] = thischunklen;
		chunkcnt++;
	}
	dz->itemcnt = chunkcnt;
	switch(dz->itemcnt) {
	case(0):
		sprintf(errstr,"No peaks found in soundfile.\n");
		return(MEMORY_ERROR);
	case(1):
		fprintf(stdout,"WARNING: WARNING: only 1 event found: Change inter-event silence ??\n");
		fflush(stdout);
		break;
	default:
		fprintf(stdout,"INFO: %d events found\n",dz->itemcnt);
		fflush(stdout);
		break;
	}
	free(tempibuf);
	if((sndseekEx(dz->ifd[0],0,0)<0)){        
        sprintf(errstr,"sndseek() failed\n");
        return SYSTEM_ERROR;
    }
	reset_filedata_counters(dz);
	return FINISHED;
}

/******************************************* RETIME_FOUND_PEAKS *******************************************/

int retime_found_peaks(dataptr dz)
{
	int exit_status, done = 0, chans = dz->infile->channels;
	double srate = (double)dz->infile->srate, time, timefact = 1.0;
	float *ibuf = dz->sampbuf[0], *obuf = dz->sampbuf[1], *ovflwbuf = dz->sampbuf[2];
	int *chunkat = dz->lparray[0], *chunklen = dz->lparray[1];
	int ibufstart = 0;		//	position in infile of start of current input buffer
	int obufstart = 0;		//	position in outfile of start of current output buffer
	int osamppos = 0;		//	position in outfile
	int obufpos = 0;		//	position within current buffer
	int lastwrite = 0;		//	position in current outbuf (or its overflow) of the furthest write
	int n, m, k, chunkstart, chunkend, step, overflow;
	int chanoffset;		//	in multichan case, ensures output chunk positioned at same channel as input chunk
	int initial_silence = 0;//	length of leading silence before 1st event in output
	int pregain = 0;
	if((dz->mode == 3) && dz->param[2] < 1.0)
		pregain = 1;
	memset((char *)obuf,0,dz->buflen * sizeof(float));
	memset((char *)ovflwbuf,0,dz->buflen * sizeof(float));

	if(dz->brksize[0] == 0)
		timefact = 1.0/dz->param[0];
	
	if((exit_status = get_initial_silence(&initial_silence,dz)) < 0)
		return(exit_status);
	osamppos += initial_silence;

	for(n=0;n<dz->itemcnt;n++) {
		chanoffset = (chunkat[n]/chans) * chans;			//	Find any channel chanoffset of the in-chunk */
		chanoffset = chunkat[n] - chanoffset;
		while(dz->total_samps_read < chunkat[n]) {			//	If next chunk to read is beyond current input buffer
			ibufstart += dz->ssampsread;					//	Get another buffer of data
			if((exit_status = read_samps(ibuf,dz)) < 0)
				return exit_status;
			if(pregain) {
				for(m=0;m<dz->ssampsread;m++)
					ibuf[m] = (float)(ibuf[m] * dz->param[2]);
			}
			if(dz->ssampsread == 0) {
				fprintf(stdout,"WARNING: Ran out of samples in input (1)\n");
				fflush(stdout);
				done = 1;
				break;
			}
		}
		if(done)
			break;
		chunkstart = chunkat[n] - ibufstart;				//	get address of chunkstart within this buffer
		chunkend   = chunkstart + chunklen[n];				//	and relative address of chunk end

		if(n > 0) {											//	After first inchunk, calculate position of next outchunk
			switch(dz->mode) {
			case(3):
				osamppos += dz->iparam[MM];					//	Advance by beat-len step
				break;
			case(4):
				if(chunkat[n-1] < dz->iparam[2] || chunkat[n-1] >= dz->iparam[3])
					timefact = 1.0;							//	If previous chunk lies outside limits, don't time-shrink
				else {
					if(dz->brksize[0]) {					//	Advance by distance between beats * speed-change factor
						time = (chunkat[n]/chans)/srate;
						read_value_from_brktable(time,0,dz);
					}
					timefact = 1.0/dz->param[0];
				}
				step = (int)round((double)(chunkat[n] - chunkat[n-1]) * timefact);
				step = (step/chans) * chans;
				osamppos += step;
				break;
			}
		}
		osamppos += chanoffset;
		while((obufpos = osamppos - obufstart) >= dz->buflen) {					//	If next write is beyond buffer end
			if((exit_status = write_samps(obuf,dz->buflen,dz))<0)				//	Write buffer(s)
				return(exit_status);	
			memset((char *)obuf,0,dz->buflen * sizeof(float));					//	And set to zero (as buffer is ADDED into)
			if((overflow = lastwrite - dz->buflen) > 0) {						//	If there wasc anhy buffer overflow
				memcpy((char *)obuf,(char *)ovflwbuf,overflow * sizeof(float));	//	Bakcopy it into buffer
				memset((char *)ovflwbuf,0,dz->buflen * sizeof(float));			//	and zero overflow buffer
			}
			lastwrite -= dz->buflen;
			obufstart += dz->buflen;											//	Advance buffer-start count in output samples
		}
		k = chunkstart;
		while(k < chunkend) {
			obuf[obufpos] = (float)(obuf[obufpos] + ibuf[k]);						//	Add chunk into outbuf
			obufpos++;
			if(++k >= dz->ssampsread) {												//	If at input buffer end
				ibufstart += dz->ssampsread;										//	Update address of buffer start
				k -= dz->ssampsread;												//	decrease both counter, and counter-end marker, by length of last buf
				chunkend -= dz->ssampsread;
				if((exit_status = read_samps(ibuf,dz)) < 0)							//	Read more insamps
					return exit_status;
				if(dz->ssampsread == 0) {
					if(k != chunkend) {
						fprintf(stdout,"WARNING: Ran out of samples in input (2)\n");
						fflush(stdout);
					}
					done = 1;
					break;
				}
				if(pregain) {
					for(m=0;m<dz->ssampsread;m++)
						ibuf[m] = (float)(ibuf[m] * dz->param[2]);
				}
			}
			lastwrite = max(lastwrite,obufpos);										//	Note address of MAXIMUM write, withib obuf (or its overflow)
		}
		if(done)
			break;
	}
	if(lastwrite > 0) {
		if((exit_status = write_samps(obuf,lastwrite,dz))<0)						//	Write last buffer
			return(exit_status);	
	}	
	if(dz->total_samps_written <= 0) {
		sprintf(errstr,"No output samples written\n");
		return(PROGRAM_ERROR);
	}
	return FINISHED;
}

/******************************************* ESTABLISH_CUTS_ARRAY *******************************************/

int establish_cuts_array(dataptr dz)
{
	int chans = dz->infile->channels;
	double srate = (double)dz->infile->srate;
	int twosplices, n, in, on, off, out, lastin = 0, laston = 0, lastoff = 0,  lastout = 0, gap;
	int eliminated = 0;
	double *realtimes	= dz->parray[0];
	double *idealtimes	= dz->parray[1];
	int *realsamps		= dz->lparray[0];
	int *idealsamps	= dz->lparray[1];
	int *realcuts		= dz->lparray[2];
	double mm_in, timescaling; 
	int *lost,lostcnt;
	if((lost = (int *)malloc(dz->itemcnt * sizeof(int)))==NULL) {
		sprintf(errstr,"Insufficient memory to lost array\n");
		return(MEMORY_ERROR);
	}
	twosplices = dz->iparam[RETIME_SPLICE] * 2;

	mm_in = 60.0 / dz->MMin;
	if (!flteq(mm_in,dz->param[MM])) {				// if output tempo != input tempo
		timescaling = dz->param[MM]/mm_in;			// Convert to output tempo
		for(n=0;n<dz->itemcnt;n++) {				// Retaining 1st accent at same time
			idealtimes[n] -= dz->offset;
			idealtimes[n] *= timescaling;
			idealtimes[n] += dz->offset;
		}
	}
	for(n=0;n<dz->itemcnt;n++)						// Convert to samples
		idealsamps[n] = (int)(round(idealtimes[n] * srate) * chans);  

	/*	FROM THE EVENT TIMING INFO, GENERATE splicestart (in) peakstart (on) peakend (off) and spliceend (out) info */
	/*	adjusting splice lengths or event lengths (or eliminating events) where events are TOO close. */
	/*	Idea is to avoid output event overlaps which would create echos in the output */
	lostcnt = 0;
	for(n=0,in=0,on=1,off=2,out=3;n<dz->itemcnt;n++,in+=4,on+=4,off+=4,out+=4) {	// Get positions of starts and ends of insplice and outsplice for each event	
		realcuts[in] = realsamps[n] - dz->iparam[RETIME_SPLICE];					//	in,on,off,out in that order...
		if(realcuts[in] < 0)
			realcuts[in] = 0;						// First splicestart may be too near file start
		realcuts[on] = realsamps[n];
		if(n >= 1) {
			gap = realcuts[on] - realcuts[lastoff];	// Distance between start of event-proper and last event-proper (ignoring splices)
			if(gap < twosplices) {					// If 2 events are too close (less than 2 splices)
				gap = realcuts[on] - (realcuts[laston] + twosplices);	// gap between start of previous event + 2 splices (dn-up and current event)
				if(gap >= 0) {
					realcuts[lastout] = realcuts[in];
					realcuts[lastoff] = realcuts[lastout] - dz->iparam[RETIME_SPLICE];
				} else {							//Eliminate last event
					realcuts[lastin] = realcuts[in];
					realcuts[laston] = realcuts[on];
					eliminated++;
					in  -= 4;
					on  -= 4;
					off -= 4;
					out -= 4;
					lost[lostcnt++] = n;
				}
			}
		}
		if(dz->brksize[RETIME_WIDTH]) {				// Read brkvals relative to time in INFILE
			read_value_from_brktable(realtimes[n],RETIME_WIDTH,dz);
			dz->iparam[RETIME_WIDTH] = (int)round(dz->param[RETIME_WIDTH] * MS_TO_SECS * srate) * chans; 
		}
		realcuts[off] = realsamps[n] + dz->iparam[RETIME_WIDTH];
		realcuts[out] = realcuts[off] + dz->iparam[RETIME_SPLICE];
		lastin  = in;
		laston  = on;
		lastoff = off;
		lastout = out;
	}
	if(eliminated) {
		fprintf(stdout,"INFO: With this splicelength, %d events have been eliminated\n", eliminated);
		fflush(stdout);
		dz->itemcnt = eliminate_lost_ideal_and_real_events(lost,lostcnt,dz);
	}
	return FINISHED;
}

/*************************** ELIMINATE_LOST_IDEAL_AND_REAL_EVENTS ****************************/

int eliminate_lost_ideal_and_real_events(int *lost,int lostcnt,dataptr dz) 
{
	double *realtimes	= dz->parray[0];
	double *idealtimes	= dz->parray[1];
	int *realsamps		= dz->lparray[0];
	int *idealsamps	= dz->lparray[1];
	int outcnt = dz->itemcnt, n, k, j;
	for(n = 0;n < lostcnt;n++) {
		k = lost[n];
		if(k+1 < outcnt) {
			for(j = k+1; j < outcnt; j++) {
				realtimes[j-1]  = realtimes[j];
				idealtimes[j-1] = idealtimes[j];
				realsamps[j-1]  = realsamps[j];
				idealsamps[j-1] = idealsamps[j];
			}
		}
		outcnt--;	
	}
	return outcnt;
}

/*************************** REPOSITION_PEAK ****************************/

int reposition_peak(dataptr dz)
{
	int exit_status;
	float *ibuf = dz->sampbuf[0], *obuf = dz->sampbuf[1];
	double srate =(double)dz->infile->srate;
	int chans = dz->infile->channels, gotstartsig = 0;
	double maxsamp = 0.0, thissamp;
	int startsig = 0, newstartsig;
	int maxpos = 0, n, obufpos, bufstart = 0, shift = 0;
	switch(dz->mode) {
	case(12):
		while(dz->samps_left > 0) {
			if((exit_status = read_samps(ibuf,dz))<0)
				return(exit_status);
			for(n=0;n<dz->ssampsread;n++) {
				if(!gotstartsig) {
					if(ibuf[n] != 0.0) {
						startsig = n + bufstart;
						gotstartsig = 1;
					}
				}
				thissamp = fabs(ibuf[n]);
				if(thissamp > maxsamp) {
					maxsamp = thissamp;
					maxpos = bufstart + n;
				}
			}
			bufstart += dz->ssampsread;
		}
		if((maxpos = (maxpos/chans) * chans) == 0) {
			sprintf(errstr,"No Peak found in file.\n");
			return(DATA_ERROR);
		}
		shift = dz->iparam[0] - maxpos;
		break;
	case(13):
		while(dz->samps_left > 0) {
			if((exit_status = read_samps(ibuf,dz))<0)
				return(exit_status);
			for(n=0;n<dz->ssampsread;n++) {
				if(!gotstartsig) {
					if(ibuf[n] != 0.0) {
						startsig = n + bufstart;
						gotstartsig = 1;
						break;
					}
				}
			}
			if(gotstartsig)
				break;
			bufstart += dz->ssampsread;
		}
		maxpos = (dz->iparam[1]/chans) * chans;
		shift = dz->iparam[0] - maxpos;
		break;
	}
	if(!gotstartsig) {
		sprintf(errstr,"No signal found in soundfile.\n");
		return(DATA_ERROR);
	}
	if((newstartsig = startsig + shift) < 0) {
		sprintf(errstr,"Peak at %lf secs: snd starts at %lf: Cannot move peak to %lf secs\n",
			(double)(maxpos/chans)/srate,(double)(startsig/chans)/srate,dz->param[0]);
		return(DATA_ERROR);
	} else if(shift == 0) {
		sprintf(errstr,"Peak is already at %lf\n",dz->param[0]);
		return(DATA_ERROR);
	}
	if((sndseekEx(dz->ifd[0],0,0)<0)){        
        sprintf(errstr,"sndseek() failed\n");
        return SYSTEM_ERROR;
    }
	reset_filedata_counters(dz);
	memset((char *)obuf,0,dz->buflen * sizeof(float));
	memset((char *)ibuf,0,dz->buflen * sizeof(float));
	if(shift > 0) {
		while(shift > dz->buflen) {
			if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
				return(exit_status);
			shift -= dz->buflen;
		}
		obufpos = shift;
	} else {
		while(newstartsig >= dz->buflen) {
			if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
				return(exit_status);
			newstartsig -= dz->buflen;
		}
		obufpos = newstartsig;
        if((sndseekEx(dz->ifd[0],startsig,0)<0)){        
            sprintf(errstr,"sndseek() failed\n");
            return SYSTEM_ERROR;
        }
		dz->samps_left = dz->insams[0] - startsig;
	}
	while(dz->samps_left > 0) {
		if((exit_status = read_samps(ibuf,dz))<0)
			return(exit_status);
		for(n=0;n<dz->ssampsread;n++) {
			obuf[obufpos++] = ibuf[n];
			if(obufpos >= dz->buflen) {
				if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
					return(exit_status);
				obufpos = 0;
			}
		}
	}
	if(obufpos > 0) {
		if((exit_status = write_samps(obuf,obufpos,dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}

/*************************** REPEAT_BEATS ****************************/

int repeat_beats(dataptr dz)
{
	int exit_status;
	float *ibuf = dz->sampbuf[0], *obuf = dz->sampbuf[1], *overflw = dz->sampbuf[2], *beatsbuf = dz->sampbuf[3];
	int *events;
	int ibufpos = 0, obufpos, bbufpos, buf_start, k, j;
	int eventsegscnt;
	int beats_start, beats_end, beats_len, beats_step, skip, beatsloc, input_end;

	memset((char *)obuf,0,dz->buflen * 2 * sizeof(float));

	/* COUNT (VALID-LENGTH) SILENCES IN DATA */
	
	if((exit_status = count_events(BEAT_SILMIN,0,&eventsegscnt,dz))<0) {
		return(exit_status);
	}
	events = dz->lparray[0];

	fprintf(stdout,"INFO: Found %d events: Locating and cutting event to repeat.\n",eventsegscnt/2);
	fflush(stdout);

	/*	LOCATE AND MEASURE THE EVENTS TO BE REPEATED */

	beats_start = -1;
	for(k = 0;k < eventsegscnt; k += 2) {
/* TEST */
fprintf(stderr,"dz->iparam[BEAT_AT] = %d events[k] = %d events[k+1] = %d\n",dz->iparam[BEAT_AT],events[k],events[k+1]);
/* TEST */
		if(dz->iparam[BEAT_AT] >= events[k] && dz->iparam[BEAT_AT] <= events[k+1]) {
			beats_start = events[k];
			break;
		}
	}
	if(beats_start < 0) {
		sprintf(errstr,"Time indicating event position does not lie within a sounding event.\n");
		return(DATA_ERROR);
	}
	beats_step = dz->iparam[MM] * dz->iparam[BEAT_CNT];
	beats_end = beats_start + beats_step;
	while(k < eventsegscnt) {					/* beats_end must be after beats_start so we can resume search at k */
		if(beats_end < events[k])
			break;
		k++;
	}
	if(ODD(k)) {						/* beatend lies within a new event */
		j = k - 2;
		if((j < 0) || (events[j] <= beats_start))	/* if cannot splice at end of previous event - splice at end of this event, making event longer  */
			beats_end = events[k];
		else							/* else splice at end of previous event, making event shorter */	
			beats_end = events[j];

	}				/* ELSE beatend lies within silence, before a new event, OK to splice there */
	if((beats_len = beats_end - beats_start) > dz->buflen) {
		sprintf(errstr,"Buffers too small to store beats-to-copy. Probably too many events not separated by silence.\n");		/* Should be impossible !! */
		return(DATA_ERROR);
	}

	if((skip = beats_step - beats_len) < 0) {	/* if events have longer duration than step between them, must step back in obuf when writing output */
		fprintf(stdout,"WARNING: Repeated events will OVERLAP each other.\n");
		fflush(stdout);
	}

		/*	COPY EVENTS TO REPEAT, TO A BUFFER */
	
	beatsloc = (beats_start/dz->buflen) * dz->buflen;
    if((sndseekEx(dz->ifd[0],beatsloc,0)<0)){        
        sprintf(errstr,"sndseek() failed\n");
        return SYSTEM_ERROR;
    }
	ibufpos = beats_start - beatsloc;

	if((exit_status = read_samps(ibuf,dz))<0) {
		sprintf(errstr,"Failed to read data from sndfile.\n");
		return(DATA_ERROR);
	}
	bbufpos = 0;
	while(bbufpos < beats_len) {
		beatsbuf[bbufpos] = ibuf[ibufpos++];
		if(ibufpos >= dz->buflen) {
			if((exit_status = read_samps(ibuf,dz))<0) {
				sprintf(errstr,"Failed to read data from sndfile.\n");
				return(DATA_ERROR);
			}
			if(dz->ssampsread == 0) {
				sprintf(errstr,"Ran off end of file before beats could be cut.\n");
				return(DATA_ERROR);
			}
			ibufpos = 0;
		}
		bbufpos++;
	}

	fprintf(stdout,"INFO: Repeating Events.\n");
	fflush(stdout);

		/*	COPY START OF ORIG EVENT, BEFORE REPETS START */

	if((sndseekEx(dz->ifd[0],0,0)<0)){        
        sprintf(errstr,"sndseek() failed\n");
        return SYSTEM_ERROR;
    }
	reset_filedata_counters(dz);
	dz->ssampsread = 0;
	buf_start = 0;
	input_end = dz->insams[0];
	if((exit_status = read_samps(ibuf,dz))<0) {
		sprintf(errstr,"Failed to read data from sndfile.\n");
		return(DATA_ERROR);
	}
	while(beats_start >= dz->ssampsread) {
		if((exit_status = write_samps(ibuf,dz->ssampsread,dz))<0)
			return(exit_status);
		beats_start -= dz->ssampsread;
		beats_end   -= dz->ssampsread;
		input_end	-= dz->ssampsread;
		if((exit_status = read_samps(ibuf,dz))<0) {
			sprintf(errstr,"Failed to read data from sndfile.\n");
			return(DATA_ERROR);
		}
	}
	if(beats_start > 0)
		memcpy((char *)obuf,(char *)ibuf,beats_start * sizeof(float));
	obufpos = beats_start;

		/*	COPY THE REPEATED EVENTS */

	for(k = 0;k < dz->iparam[BEAT_REPEATS]; k++) {
		for(bbufpos = 0;bbufpos < beats_len; bbufpos++) {
			obuf[obufpos] = (float)(obuf[obufpos] + beatsbuf[bbufpos]);
			if(++obufpos >= dz->buflen * 2) {
				if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
					return(exit_status);
				memcpy((char *)obuf,(char *)overflw,dz->buflen * sizeof(float));
				memset((char *)overflw,0,dz->buflen * sizeof(float));
				obufpos -= dz->buflen;
			}
		}
		obufpos += skip;
		if(obufpos  < 0) {
			sprintf(errstr,"Overlap of events failed in output buffers.\n");
			return(DATA_ERROR);
		} 
		else if(obufpos >= dz->buflen * 2) {
			while (obufpos >= dz->buflen * 2) {
				if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
					return(exit_status);
				memcpy((char *)obuf,(char *)overflw,dz->buflen * sizeof(float));
				memset((char *)overflw,0,dz->buflen * sizeof(float));
				obufpos -= dz->buflen;
			}
		}
	}

		/*	COPY REMAINDER OF ORIGINAL EVENT */

	ibufpos = beats_start;
	k = ibufpos;
	while(k < input_end) {
		if(ibufpos >= dz->ssampsread) {
			if((exit_status = read_samps(ibuf,dz))<0) {
				sprintf(errstr,"Failed to read data from sndfile.\n");
				return(DATA_ERROR);
			}
			if(dz->ssampsread == 0) {
				fprintf(stdout,"WARNING: Reached end of infile too soon.\n");
				fflush(stdout);
				break;
			}
			ibufpos = 0;
		}
		obuf[obufpos] = (float)(obuf[obufpos] + ibuf[ibufpos]);
		if(++obufpos >= dz->buflen * 2) {
			if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
				return(exit_status);
			memcpy((char *)obuf,(char *)overflw,dz->buflen * sizeof(float));
			memset((char *)overflw,0,dz->buflen * sizeof(float));
			obufpos -= dz->buflen;
		}
		ibufpos++;
		k++;
	}
	if(obufpos > 0) {
		if((exit_status = write_samps(obuf,obufpos,dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}

/*************************** GET_INITIAL_SILENCE ****************************/

int get_initial_silence(int *initial_silence,dataptr dz)
{
	int *chunkat = dz->lparray[0], *chunklen = dz->lparray[1];
	int step, bakstep = INT_MAX, n, syncchunk, synctime, new_synctime, outlen = 0, silence_change;
	int chans = dz->infile->channels;
	double timefact = 1.0, time, srate = (double)dz->infile->srate;

	if(dz->iparam[RETIME_SYNCAT] == 0) {				/* If syncpoint for file is specified as start of file, return */
		*initial_silence = chunkat[0];
		return (FINISHED);
	}

	/* FIND WHICH CHUNK IS NEAREST TO SPECIFIED SYNCTIME */

	for(n=0;n<dz->itemcnt;n++) {							
		if(chunkat[n] > dz->iparam[RETIME_SYNCAT]) {			/* If we're at chunk after synctime */
			step  = chunkat[n] - dz->iparam[RETIME_SYNCAT];		/* which chunk is nearer to synctime? this chunkstart */
			if(n > 0)											/* OR (the end of) the previous chunk, if there is one */	
				bakstep = dz->iparam[RETIME_SYNCAT] - (chunkat[n-1] + chunklen[n-1]);
			if(bakstep < step)
				n--;
			break;
		}
	}													/* if no chuck has been found AFTER synctime, n gets to dz->itemcnt  */										
	if(n == dz->itemcnt)								/* But in this case, last chunk is sync-chunk */
		n--;											/* so mark last chunk as sync-chunk */
	syncchunk = n;
	if(syncchunk == 0) {								/* If sync is at first event ... */
		*initial_silence = chunkat[0];					/* initial silence-offset of events does not change */
		return(FINISHED);
	}
	synctime = chunkat[syncchunk];						/* the original synctime is start of event closest to synctime */
	new_synctime = synctime;							/* Prevents error-warnings : new_synctime is always set below */

	for(n=0;n<dz->itemcnt;n++) {
		if(n==0)
			outlen = (chunkat[0]/chans) * chans;
		else {
			if(chunkat[n-1] < dz->iparam[2] || chunkat[n-1] >= dz->iparam[3])
				timefact = 1.0;							//	If previous chunk lies outside limits, don't time-shrink
			else {
				if(dz->brksize[0]) {					//	Advance by distance between beats * speed-change factor
					time = (chunkat[n]/chans)/srate;
					read_value_from_brktable(time,0,dz);
				}
				timefact = 1.0/dz->param[0];
			}
			step = (int)round((double)(chunkat[n] - chunkat[n-1]) * timefact);
			step = (step/chans) * chans;
			outlen += step;								//	Position in output advances by the timechanged step
			if(n == syncchunk) {						//	If it's the syncchunk that we've just repositioned 
				new_synctime = outlen;					//	It's new start-time is the new syncing time
				break;
			}
		}
	}
	silence_change = synctime - new_synctime;
	*initial_silence = chunkat[0] + silence_change;
	if(*initial_silence < 0) {
		fprintf(stdout,"WARNING: Cannot synchronise accent in output with that in input: syncing at event start.\n");
		fflush(stdout);
		*initial_silence = chunkat[0];
	}
	return(FINISHED);
}

/*************************** FIND_SOUND_START ****************************/

int find_sound_start(dataptr dz)
{
	int exit_status, gotit = 0;
	float *ibuf = dz->sampbuf[0];
	int n = 0, start_buf = 0;
	double srate = (double)dz->infile->srate, time;
	dz->ssampsread = 0;
	while(dz->samps_left > 0) {
		start_buf += dz->ssampsread;
		if((exit_status = read_samps(ibuf,dz)) < 0)
			return(exit_status);
		for(n=0;n<dz->ssampsread;n++) {
			if(ibuf[n] != 0.0) {
				gotit = 1;
				break;
			}
		}
		if(gotit)
			break;
	}
	time = (double)(n + start_buf)/srate;
	fprintf(dz->fp,"%lf\t%s\n",time,infilename);
	return FINISHED;
}

/*************************** READ_RETIME_FNAM ****************************/

int read_retime_fnam(char *filename,dataptr dz)
{
	char nufilename[1000];
	char *p = strrchr(filename, '.');
	strcpy(nufilename,filename);
	if(p == NULL)
		strcat(nufilename,".txt");
	else {
		p++;
		if(_stricmp(p,"txt")) {
			sprintf(errstr,"Output textfile (%s) must have a '.txt' extension, or none.\n",filename);
			return(DATA_ERROR);
		}
	}
	if((dz->fp = fopen(nufilename,"a"))==NULL) {		/* permits bulk-processing of files to same outfile */
		sprintf(errstr,"Cannot open output file %s\n",nufilename);
		return(USER_ERROR);
	}
	dz->process_type	= TO_TEXTFILE;	
	dz->outfiletype  	= TEXTFILE_OUT;
	return(FINISHED);
}
	
/*************************** FIND_SHORTEST_EVENT ****************************/

int find_shortest_event(dataptr dz)
{
	int exit_status, inchans = dz->infile->channels;
	int n = 0, evlen, evlenshortest, evlenlongest, eventsegscnt, *events;
	double srate = (double)dz->infile->srate, longest, shortest;

	if((exit_status = count_events(0,0,&eventsegscnt,dz))<0) {
		return(exit_status);
	}
	evlenshortest = dz->insams[0] + 1;
	evlenlongest  = -1;
	events = dz->lparray[0];
	for(n=0;n<eventsegscnt;n+=2) {
		evlen = events[n+1] - events[n];
		evlenshortest = min(evlen,evlenshortest);
		evlenlongest  = max(evlen,evlenlongest);
	}
	shortest = (double)(evlenshortest/inchans)/srate;
	longest  = (double)(evlenlongest/inchans)/srate;
	fprintf(stdout,"INFO: \n");
	fprintf(stdout,"INFO: Shortest event = %lf secs ::  = %lf mS \n",shortest,shortest * SECS_TO_MS);
	fprintf(stdout,"INFO: Longest event = %lf secs\n",longest);
	return FINISHED;
}

/*************************** COUNT_EVENTS ****************************/

int count_events(int paramno,int arrayno,int *eventsegscnt, dataptr dz)
{
	int exit_status;
	float *ibuf = dz->sampbuf[0];
	int *silences, *events;
	int	ibufpos = 0, silstart = 0, silend, buf_start, k;
	int insil = 0, event_at_end = 1;
	int silsegscnt = 0;
	fprintf(stdout,"INFO: Counting silences between events.\n");
	fflush(stdout);
	while(dz->samps_left > 0) {
		if((exit_status = read_samps(ibuf,dz))<0) {
			sprintf(errstr,"Failed to read data from sndfile.\n");
			return(DATA_ERROR);
		}
		for(ibufpos=0;ibufpos<dz->ssampsread;ibufpos++) {
			if(ibuf[ibufpos] == 0.0)
				insil++;
			else {
				if(insil) {
					if(insil >= dz->iparam[paramno]) {
						silsegscnt++;
					}
				}
				insil = 0;
			}
		}
	}
	if(insil) 
		silsegscnt++;
	if(silsegscnt == 0) {
		sprintf(errstr,"NO SILENCE-GAPS FOUND IN FILE.\n");
		return(DATA_ERROR);
	}
	silsegscnt++;			/* in case less silences than sounds */
	if((sndseekEx(dz->ifd[0],0,0)<0)){        
        sprintf(errstr,"sndseek() failed\n");
        return SYSTEM_ERROR;
    }
	reset_filedata_counters(dz);
	buf_start = 0;
	/* STORE LOCATION OF STARTS AND ENDS OF SILENCE BLOCKS */

	if((dz->lparray[arrayno] = (int *)malloc((silsegscnt * 2) * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY TO STORE SILENCE LOCATIONS.\n");
		return(MEMORY_ERROR);
	}
	silsegscnt = 0;
	insil = 0;
	silences = dz->lparray[arrayno];
	fprintf(stdout,"INFO: Marking silence-separated Events.\n");
	fflush(stdout);
	while(dz->samps_left > 0) {
		if((exit_status = read_samps(ibuf,dz))<0) {
			sprintf(errstr,"Failed to read data from sndfile.\n");
			return(DATA_ERROR);
		}
		for(ibufpos=0;ibufpos<dz->ssampsread;ibufpos++) {
			if(ibuf[ibufpos] == 0.0) {
				if(insil == 0)
					silstart = buf_start + ibufpos;
				insil++;
			} else {
				if(insil) {
					if(insil >= dz->iparam[paramno]) {
						silend = buf_start + ibufpos;
						silences[silsegscnt++] = silstart;
						silences[silsegscnt++] = silend;
					}
				}
				insil = 0;
			}
		}
		buf_start += dz->ssampsread;
	}
	if(insil) {		/* if silence at end */
		silend = dz->insams[0];
		silences[silsegscnt++] = silstart;
		silences[silsegscnt++] = silend;
		event_at_end = 0;
	}
	events = dz->lparray[arrayno];	/* we write over the original data store */
	if(silences[0] == 0) {					/* If first silence is at time zero */
		for(k=1;k<silsegscnt;k++)			/* map gaps between silences backwards into (=) events ( losing first silence in process) */
			events[k-1] = silences[k];		/* --X--X-- to XX-XX- */
		if(event_at_end)					/* if last silence does not end at file end, it starts a final event */
			events[silsegscnt-1] = dz->insams[0];	/* so add the end of file as end of final event, and silsegcnt stays same. --X--X-- thro XX-XX- to  XX-XX-XX  */
		else								/* otherwise final silent end, does NOT start a new event, so delete last entry */			
			silsegscnt -= 2;				/* --X--X-- thro XX-XX- to XX-XX: subtract a start-end pair from count */ 

	} else {								/* first event at time zero */	
		for(k=silsegscnt;k>0;k--)			/* map gaps between silences forwards into (=) events (eventually using extra address space assigned at end) */
			events[k] = silences[k-1];		
		events[0] = 0;						/* Add start of file as start of firsst event. --X--X-- to XX-XX-XX- */
													
		if(event_at_end) {					/* if last silence does not end at file end, it starts a final event */
			events[silsegscnt+1] = dz->insams[0];	/* so add the end of file as end of final event */
			silsegscnt += 2;				/* --X--X-- thro XX-XX-XX- to  XX-XX-XX-XX add a start-end pair to count */
		} else {
			/* last silence is at end of file, and does not start an event .... --X--X-- thro XX-XX-XX- to XX-XX-XX  silsegscnt stays as is, last entry forgotten */	
		}
	}
	*eventsegscnt = silsegscnt;
	return FINISHED;
}

/*************************** EQUALISE_EVENT_LEVELS ****************************/

int equalise_event_levels(dataptr dz)
{
	int exit_status, warned = 0, done = 0, accents = dz->iparam[2];
	float *ibuf = dz->sampbuf[0];
	int *events;
	int n;
	int eventsegscnt, abs_samp_cnt, cnt, envcnt, actual_envcnt;
	double *env, maxval, boostrange, maxboost, boost, equaliser = dz->param[1], pregain = dz->param[3];
	int event_start, event_end, bufpos;

	if((exit_status = count_events(0,0,&eventsegscnt,dz))<0) {
		return(exit_status);
	}
	if((envcnt = eventsegscnt/2) * 2 != eventsegscnt)
		envcnt++;
	events = dz->lparray[0];
	if((dz->parray[0] = (double *)malloc(envcnt * sizeof(double))) == NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY establishing envelope storage.\n");
		return(MEMORY_ERROR);
	}
	env = dz->parray[0];
	if((sndseekEx(dz->ifd[0],0,0)<0)){        
        sprintf(errstr,"sndseek() failed\n");
        return SYSTEM_ERROR;
    }
	reset_filedata_counters(dz);
	dz->ssampsread = 0;
	cnt = 0;
	actual_envcnt = 0;
	abs_samp_cnt = 0;
	bufpos = 0;

	event_start	= events[cnt++];
	event_end   = events[cnt++];
	if((exit_status = read_samps(ibuf,dz))<0) {
		sprintf(errstr,"Failed to read data from sndfile.\n");
		return(DATA_ERROR);
	}
	fprintf(stdout,"INFO: Extracting events envelope\n");
	fflush(stdout);
	while(abs_samp_cnt < dz->insams[0]) {
		while(abs_samp_cnt < event_start) {
			if(bufpos >= dz->ssampsread) {
				if((exit_status = read_samps(ibuf,dz))<0) {
					sprintf(errstr,"Failed to read data from sndfile.\n");
					return(DATA_ERROR);
				}
				bufpos = 0;
				if(dz->ssampsread == 0) {
					done = 1;
					break;
				}
			}
			abs_samp_cnt++;
			bufpos++;
		}
		if(done)
			break;
		maxval = 0.0;
		while(abs_samp_cnt < event_end) {
			if(bufpos >= dz->ssampsread) {
				if((exit_status = read_samps(ibuf,dz))<0) {
					sprintf(errstr,"Failed to read data from sndfile.\n");
					return(DATA_ERROR);
				}
				bufpos = 0;
				if(dz->ssampsread == 0) {
					done = 1;
					break;
				}
			}
			if(fabs(ibuf[bufpos]) > maxval)
				maxval = fabs(ibuf[bufpos]);
			abs_samp_cnt++;
			bufpos++;
		}
		if(actual_envcnt >= envcnt) {
			sprintf(errstr,"Bug : Error in envelope accounting (1)\n");
			return(PROGRAM_ERROR);
		}
		env[actual_envcnt++] = maxval;
		if(done || (cnt >= eventsegscnt))
			break;
		event_start	= events[cnt++];
		event_end   = events[cnt++];
	}
	if(envcnt != actual_envcnt) {
		fprintf(stdout,"WARNING: Error in envelope accounting (2)\n");
		fflush(stdout);
	}
	maxval = 0;
	for(n=0;n<actual_envcnt;n++)
		maxval = max(maxval,env[n]);
	for(n=0;n<actual_envcnt;n++) {
		maxboost = min(50.0,maxval/env[n]);
		switch(accents) {
		case(0):								//	No accents
			boostrange = maxboost - env[n];		//	Events less than max
			boost = boostrange * equaliser;		//	Boosted by amount dependent on equaliser
			env[n] = (env[n] + boost)/env[n];	
			break;
		case(1):								//	All events equally accented
			env[n] = maxboost * pregain;		//	All events get level of max event
			break;
		default:
			if(n % accents == 0)				//	Accented beats get maxlevel
				env[n] = maxboost * pregain;
			else								//	Unaccented beats set to equaliser * maxlevel
				env[n] = maxboost * pregain * equaliser;	
			break;
		}
	}
	for(n=actual_envcnt;n<envcnt;n++)
		env[n] = 1.0;		//	Deal with potential accounting problems with envelope
	if((sndseekEx(dz->ifd[0],0,0)<0)){        
        sprintf(errstr,"sndseek() failed\n");
        return SYSTEM_ERROR;
    }
	reset_filedata_counters(dz);
	dz->ssampsread = 0;
	if((exit_status = read_samps(ibuf,dz))<0) {
		sprintf(errstr,"Failed to read data from sndfile.\n");
		return(DATA_ERROR);
	}
	cnt = 0;
	n = 0;
	abs_samp_cnt = 0;
	bufpos = 0;
	event_start	= events[cnt++];
	event_end   = events[cnt++];

	fprintf(stdout,"INFO: Adjusting events loudness\n");
	fflush(stdout);
	while(abs_samp_cnt < dz->insams[0]) {
		while(abs_samp_cnt < event_start) {
			if(bufpos >= dz->ssampsread) {
				if((exit_status = write_samps(ibuf,dz->ssampsread,dz))<0) {
					sprintf(errstr,"Failed to write data to sndfile.\n");
					return(DATA_ERROR);
				}
				if((exit_status = read_samps(ibuf,dz))<0) {
					sprintf(errstr,"Failed to read data from sndfile.\n");
					return(DATA_ERROR);
				}
				bufpos = 0;
				if(dz->ssampsread == 0) {
					done = 1;
					break;
				}
			}
			abs_samp_cnt++;
			bufpos++;
		}
		if(done)
			break;
		while(abs_samp_cnt < event_end) {
			if(n > envcnt) {
				n--;
				if(!warned) {
					fprintf(stdout,"WARNING: Error in envelope accounting (3)\n");
					fflush(stdout);
					warned = 1;
				}
			}
			if(bufpos >= dz->ssampsread) {
				if((exit_status = write_samps(ibuf,dz->ssampsread,dz))<0) {
					sprintf(errstr,"Failed to write data to sndfile.\n");
					return(DATA_ERROR);
				}
				if((exit_status = read_samps(ibuf,dz))<0) {
					sprintf(errstr,"Failed to read data from sndfile.\n");
					return(DATA_ERROR);
				}
				bufpos = 0;
				if(dz->ssampsread == 0) {
					done = 1;
					break;
				}
			}
			ibuf[bufpos] = (float)(ibuf[bufpos] * env[n]);
			abs_samp_cnt++;
			bufpos++;
		}
		if(done)
			break;
		n++;
		if(cnt >= eventsegscnt) {
			event_start = abs_samp_cnt;
			event_end = dz->insams[0];
		} else {
			event_start	= events[cnt++];
			event_end   = events[cnt++];
		}
	}
	if(bufpos > 0) {
		if((exit_status = write_samps(ibuf,bufpos,dz))<0) {
			sprintf(errstr,"Failed to write data to sndfile.\n");
			return(DATA_ERROR);
		}
	}
	return FINISHED;
}

/******************************************* RETEMPO_EVENTS *******************************************/

int retempo_events(dataptr dz)
{
	int exit_status, done = 0, chans = dz->infile->channels;
	float *ibuf = dz->sampbuf[0], *obuf = dz->sampbuf[1], *ovflwbuf = dz->sampbuf[2];
	int *chunkat;
	double srate = (double)dz->infile->srate;
	double *beat = dz->parray[0];
	int ibufstart = 0;		//	position in infile of start of current input buffer
	int obufstart = 0;		//	position in outfile of start of current output buffer
	int osamppos  = 0;		//	position in outfile
	int obufpos   = 0;		//	position within current output buffer
	int lastwrite = 0;		//	position in current outbuf (or its overflow) of the furthest write
	int n, m, k, b, chunkstart, chunkend, overflow;
	int chanoffset;		//	in multichan case, ensures output chunk positioned at same channel as input chunk
	int thisbeat = 0;			//	Current beat position in outfile, in samples
	int tempo=0, startoffset = 0;
	double pregain = 0.0;
	int eventsegscnt = 0;

	switch(dz->mode) {
	case(5):
		tempo		= dz->iparam[0];
		startoffset = dz->iparam[1];
		pregain	    = dz->param[3];
		if((exit_status = count_events(2,0,&eventsegscnt,dz))<0)
			return(exit_status);
		break;
	case(6):
		startoffset = dz->iparam[0];
		pregain	    = dz->param[2];
		if((exit_status = count_events(1,0,&eventsegscnt,dz))<0)
			return(exit_status);
		break;
	}
	chunkat = dz->lparray[0];
	if((sndseekEx(dz->ifd[0],0,0)<0)){        
        sprintf(errstr,"sndseek() failed\n");
        return SYSTEM_ERROR;
    }
	reset_filedata_counters(dz);
	dz->ssampsread = 0;
	if(dz->itemcnt * 2 < eventsegscnt) {
		sprintf(errstr,"Found %d events, but only %d out-event-beat-placements specified.\n",eventsegscnt/2,dz->itemcnt);
		return(DATA_ERROR);
	} else if(dz->itemcnt * 2 > eventsegscnt) {
		fprintf(stdout,"WARNING: Found %d events : %d out-event-beat-placements specified.\n",eventsegscnt/2,dz->itemcnt);
		fflush(stdout);
	} 
	memset((char *)obuf,0,dz->buflen * sizeof(float));
	memset((char *)ovflwbuf,0,dz->buflen * sizeof(float));

	for(n=0, b= 0;n<eventsegscnt;n+=2,b++) {
		chanoffset = (chunkat[n]/chans) * chans;			//	Find any channel chanoffset; of the in-chunk */
		chanoffset = chunkat[n] - chanoffset;
		while(dz->total_samps_read < chunkat[n]) {			//	If next chunk to read is beyond current input buffer
			ibufstart += dz->ssampsread;					//	Get another buffer of data
			if((exit_status = read_samps(ibuf,dz)) < 0)
				return exit_status;
			for(m=0;m<dz->ssampsread;m++)
				ibuf[m] = (float)(ibuf[m] * pregain);
			if(dz->ssampsread == 0) {
				fprintf(stdout,"WARNING: Ran out of samples in input (1)\n");
				fflush(stdout);
				done = 1;
				break;
			}
		}
		if(done)
			break;
		chunkstart = chunkat[n] - ibufstart;				//	get address of chunkstart within this buffer
		chunkend   = chunkat[n+1] - ibufstart;				//	and relative address of chunk end

		switch(dz->mode) {
		case(5):
			thisbeat = (int)round(beat[b] * (double)tempo) * chans;
			break;
		case(6):
			thisbeat = (int)round(beat[b] * srate) * chans;
			break;
		}
		osamppos = startoffset + thisbeat;
		osamppos += chanoffset;
		obufpos = osamppos - obufstart;
		if(obufpos < 0) {
			sprintf(errstr,"Events too long (relative to silence) to achieve this tempo change.\n");
			return(DATA_ERROR);
		}
		while(obufpos >= dz->buflen) {					//	If next write is beyond buffer end
			if((exit_status = write_samps(obuf,dz->buflen,dz))<0)				//	Write buffer(s)
				return(exit_status);	
			memset((char *)obuf,0,dz->buflen * sizeof(float));					//	And set to zero (as buffer is ADDED into)
			if((overflow = lastwrite - dz->buflen) > 0) {						//	If there was any buffer overflow
				memcpy((char *)obuf,(char *)ovflwbuf,overflow * sizeof(float));	//	Bakcopy it into buffer
				memset((char *)ovflwbuf,0,dz->buflen * sizeof(float));			//	and zero overflow buffer
			}
			lastwrite -= dz->buflen;
			obufstart += dz->buflen;											//	Advance buffer-start count in output samples
			obufpos -= dz->buflen;
		}
		k = chunkstart; 
		while(k < chunkend) {
			if(k >= dz->ssampsread) {												//	If at input buffer end
				ibufstart += dz->ssampsread;										//	Update address of buffer start
				k -= dz->ssampsread;												//	decrease both counter, and counter-end marker, by length of last buf
				chunkend -= dz->ssampsread;
				if((exit_status = read_samps(ibuf,dz)) < 0)							//	Read more insamps
					return exit_status;
				if(dz->ssampsread == 0) {
					if(k != chunkend) {
						fprintf(stdout,"WARNING: Ran out of samples in input (2)\n");
						fflush(stdout);
					}
					done = 1;
					break;
				}
				for(m=0;m<dz->ssampsread;m++)
					ibuf[m] = (float)(ibuf[m] * pregain);
			}
			if(obufpos >= dz->buflen) {											//	If next write is beyond output buffer end
				if((exit_status = write_samps(obuf,dz->buflen,dz))<0)				//	Write buffer(s)
					return(exit_status);	
				memset((char *)obuf,0,dz->buflen * sizeof(float));					//	And set to zero (as buffer is ADDED into)
				if((overflow = lastwrite - dz->buflen) > 0) {						//	If there was any buffer overflow
					memcpy((char *)obuf,(char *)ovflwbuf,overflow * sizeof(float));	//	Bakcopy it into buffer
					memset((char *)ovflwbuf,0,dz->buflen * sizeof(float));			//	and zero overflow buffer
				}
				lastwrite -= dz->buflen;
				obufstart += dz->buflen;											//	Advance buffer-start count in output samples
				obufpos = 0;			
			}
			obuf[obufpos] = (float)(obuf[obufpos] + ibuf[k]);						//	Add chunk into outbuf
			obufpos++;
			k++;
			lastwrite = max(lastwrite,obufpos);										//	Note address of MAXIMUM write, withib obuf (or its overflow)
		}
		if(done)
			break;
	}
	if(lastwrite > 0) {
		if((exit_status = write_samps(obuf,lastwrite,dz))<0)						//	Write last buffer
			return(exit_status);	
	}	
	if(dz->total_samps_written <= 0) {
		sprintf(errstr,"No output samples written\n");
		return(PROGRAM_ERROR);
	}
	return FINISHED;
}

/******************************************* MASK_EVENTS *******************************************/

int mask_events(dataptr dz)
{
	int exit_status, done = 0, chans = dz->infile->channels;
	float *buf = dz->sampbuf[0];
	int *chunkat, *mask = dz->lparray[0];
	int ibufstart = 0;		//	position in infile of start of current input buffer
	int n, m, k, chunkstart, chunkend;
	int chanoffset;		//	in multichan case, ensures output chunk positioned at same channel as input chunk
	int eventsegscnt = 0;

	if((exit_status = count_events(0,1,&eventsegscnt,dz))<0)
		return(exit_status);
	chunkat = dz->lparray[1];
	if((sndseekEx(dz->ifd[0],0,0)<0)){        
        sprintf(errstr,"sndseek() failed\n");
        return SYSTEM_ERROR;
    }
	reset_filedata_counters(dz);
	if((exit_status = read_samps(buf,dz)) < 0)
		return exit_status;
	m = 0;
	for(n=0;n<eventsegscnt;n+=2) {
		chanoffset = (chunkat[n]/chans) * chans;			//	Find any channel chanoffset; of the in-chunk */
		chanoffset = chunkat[n] - chanoffset;
		while(dz->total_samps_read < chunkat[n]) {			//	If next chunk to read is beyond current input buffer
			ibufstart += dz->ssampsread;					//	Write cuurent buffer
			if((exit_status = write_samps(buf,dz->ssampsread,dz)) < 0)
				return exit_status;
			if((exit_status = read_samps(buf,dz)) < 0)		//	Get another buffer of data
				return exit_status;
			if(dz->ssampsread == 0) {
				fprintf(stdout,"WARNING: Ran out of samples in input (1)\n");
				fflush(stdout);
				done = 1;
				break;
			}
		}
		if(done)
			break;
		chunkstart = chunkat[n] - ibufstart;				//	get address of chunkstart within this buffer
		chunkend   = chunkat[n+1] - ibufstart;				//	and relative address of chunk end
		k = chunkstart;
		while(k < chunkend) {
			if(k >= dz->ssampsread) {						//	If at input buffer end
				ibufstart += dz->ssampsread;				//	Update address of buffer start
				k -= dz->ssampsread;						//	decrease both counter, and counter-end marker, by length of last buf
				chunkend -= dz->ssampsread;					//	Write data
				if((exit_status = write_samps(buf,dz->ssampsread,dz)) < 0)
					return exit_status;
				if((exit_status = read_samps(buf,dz)) < 0)	//	Read more insamps
					return exit_status;
				if(dz->ssampsread == 0) {
					if(k != chunkend) {
						fprintf(stdout,"WARNING: Ran out of samples in input (2)\n");
						fflush(stdout);
					}
					done = 1;
					break;
				}
			}
			if(mask[m] == 0)		//	Zero event, where masked
				buf[k] = 0.0f;
			k++;
		}
		if(done)
			break;
		if(++m >= dz->itemcnt)		//	Loop around mask
			m = 0;
	}
	while(dz->ssampsread > 0) {							//	Copy end of file
		if((exit_status = write_samps(buf,dz->ssampsread,dz)) < 0)
			return exit_status;
		if((exit_status = read_samps(buf,dz)) < 0)
			return exit_status;
	}
	return FINISHED;
}
