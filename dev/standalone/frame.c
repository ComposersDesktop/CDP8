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
static int setup_frame_arrays(dataptr dz);
static int setup_frame_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_frame_param_ranges_and_defaults(dataptr dz);
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
static int check_the_param_validity_and_consistency(dataptr dz);
static int rotate(dataptr dz);
static int read_the_special_data(char *filename,dataptr dz);
static int snake_test(int n,double lasttime,int linecnt,char *filename,dataptr dz);
static void get_next_snake(dataptr dz);
static int get_first_rotation_orientation(int n,dataptr dz);
static void set_stereo_levels(double pos, double *llevel, double *rlevel);
static void do_smear(int loutchan,int routchan,double lsig,double rsig,int bufpos,int chans,float *obuf,double smear);
static int reorient_test(dataptr dz);
static int editchans_test(dataptr dz) ;
static int reorient(dataptr dz);
static int mirror(dataptr dz);
static int bilateral(dataptr dz);
static int beast_bilateral(dataptr dz);
static int swapchans(dataptr dz);
static int envchans(dataptr dz);

#define FRAME_OCHAN_LEFT	0	/* the output left channel for output of moving sig, for each input channel, assuming sound is rotating round ring */
#define FRAME_OCHAN_RIGHT	1	/* the output right channel for output of moving sig, for each input channel, assuming sound is rotating round ring */
#define FRAME_OL			2	/* the output left channel for output of moving sig, for each input channel, once snaking has been factored in */
#define FRAME_OR			3	/* the output right channel for output of moving sig, for each input channel, once snaking has been factored in */
#define FRAME_SNAKE			4	/* store of ALL snaking values, over all times */
#define FRAME_SNAKEPERM		5	/* current snake values */
#define FRAME_ORIENT		6	/* orientation of first non-zero rotation : with 2 rotations, can be two values */

#define FRAME_POS			0	/* inter-lspkr position at current time */
#define FRAME_STEP			1	/* movement step between loudspeaker pair, same for every input channel : with 2 rotations, can be 2 step values */
#define FRAME_LLEVEL		2	/* Relative level on left lspkr of a pair */
#define FRAME_RLEVEL		3	/* Relative level on rigth lspkr of a pair */

#define MAX_ROT				500	/* Max rate of frame-rotation: slow enough to ensure that spatial step between samples is NOT >= 1 */
								/* as alogirithm depends on intrer-speaker position (range 0-1) being reset within the 0-1 range simply by adding or subtracting 1 */
								/* whenever it oversteps those bounds, so can't step from any position WITHIN range, by >= 1 */
#define ROTATION0	0
#define ROTATION1	1

#define EVEN(x)	(!ODD(x))
#define ROOT2	(1.4142136)

#define next_snake_loc	ringsize
#define next_snake_time	total_windows

int SMEAR;

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
	dz->maxmode = 8;
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
		if((exit_status = setup_frame_application(dz))<0) {
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
	switch(dz->mode) {
	case(0):	SMEAR = 1;	break;
	case(1):	SMEAR = 2;	break;
	}
	// parse_infile_and_hone_type() = 
	if((exit_status = parse_infile_and_check_type(cmdline,dz))<0) {
		exit_status = print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	// setup_param_ranges_and_defaults() =
	if((exit_status = setup_frame_param_ranges_and_defaults(dz))<0) {
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
	if((dz->mode < 3) || (dz->mode == 6)) {
		if((exit_status = read_the_special_data(cmdline[0],dz))<0) {
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
	if(dz->mode != 6) {
		if((exit_status = setup_frame_arrays(dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
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
	//param_preprocess()						redundant
	//spec_process_file =
	switch(dz->mode) {
	case(2):
	case(3):
	case(4):
	case(7):
		if((exit_status = reorient(dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		break;
	case(5):
		if((exit_status = swapchans(dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		break;
	case(6):
		if((exit_status = envchans(dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		break;
	default:
		if((exit_status = rotate(dz))<0) {
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

/************************* SETUP_FRAME_APPLICATION *******************/

int setup_frame_application(dataptr dz)
{
	int exit_status;
	aplptr ap;
	if((exit_status = establish_application(dz))<0)		// GLOBAL
		return(FAILED);
	ap = dz->application;
	// SEE parstruct FOR EXPLANATION of next 2 functions
	switch(dz->mode) {
	case(0):
		if((exit_status = set_param_data(ap,FRAMEDATA   ,1,1,"D"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"s",1,"d","",0,0,""))<0)
			return(FAILED);
		break;
	case(1):
		if((exit_status = set_param_data(ap,FRAMEDATA   ,2,2,"DD"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"s",1,"d","",0,0,""))<0)
			return(FAILED);
		break;
	case(2):
		if((exit_status = set_param_data(ap,FRAMEDATA   ,0,0,""))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","",0,0,""))<0)
			return(FAILED);
		break;
	case(3):
		if((exit_status = set_param_data(ap,0,1,1,"d"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","",0,0,""))<0)
			return(FAILED);
		break;
	case(4):
		if((exit_status = set_param_data(ap,0,0,0,""))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","b",1,0,"0"))<0)
			return(FAILED);
		break;
	case(5):
		if((exit_status = set_param_data(ap,0,2,2,"ii"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","",0,0,""))<0)
			return(FAILED);
		break;
	case(6):
		if((exit_status = set_param_data(ap,FRAMEDATA,1,1,"D"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","",0,0,"0"))<0)
			return(FAILED);
		break;
	case(7):
		if((exit_status = set_param_data(ap,0,0,0,""))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","b",1,0,"0"))<0)
			return(FAILED);
		break;
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
		} else if(infile_info->channels < 2)  {
			sprintf(errstr,"File %s is not of correct type (must be multichannel)\n",cmdline[0]);
			return(DATA_ERROR);
		}
		if((dz->mode == 1) && ODD(infile_info->channels)) {
			sprintf(errstr,"File %s is not of correct type (must have even number of channels)\n",cmdline[0]);
			return(DATA_ERROR);
		}
		if((exit_status = copy_parse_info_to_main_structure(infile_info,dz))<0) {
			sprintf(errstr,"Failed to copy file parsing information\n");
			return(PROGRAM_ERROR);
		}
		free(infile_info);
	}
	return(FINISHED);
}

/************************* SETUP_FRAME_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_frame_param_ranges_and_defaults(dataptr dz)
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
	case(1):
		ap->lo[1]	= -500.0;
		ap->hi[1]	= 500.0;
		ap->default_val[1]	= -1.0;
		/* fall thro */
	case(0):
		ap->lo[0]	= -MAX_ROT;
		ap->hi[0]	= MAX_ROT;
		ap->default_val[0]	= 1.0;
		ap->lo[SMEAR]	= 0.0;
		ap->hi[SMEAR]	= 0.5;
		ap->default_val[SMEAR]	= 0.0;
		break;
	case(2):
	case(4):
		break;
	case(3):
		ap->lo[0]	= 0.0;
		ap->hi[0]	= 16.5;
		ap->default_val[0]	= 1.0;
		break;
	case(5):
		ap->lo[0]	= 1.0;
		ap->hi[0]	= 16.0;
		ap->default_val[0]	= 1.0;
		ap->lo[1]	= 1.0;
		ap->hi[1]	= 16.0;
		ap->default_val[1]	= 2.0;
		break;
	case(6):
		ap->lo[0]	= 0.0;
		ap->hi[0]	= 1.0;
		ap->default_val[0]	= 0.0;
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
			if((exit_status = setup_frame_application(dz))<0)
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
	dz->larray_cnt = 7; 
	dz->array_cnt = 4; 
	if((dz->parray  = (double **)malloc(dz->array_cnt * sizeof(double *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for internal double array.\n");
		return(MEMORY_ERROR);
	}
	if((dz->lparray  = (int **)malloc(dz->larray_cnt * sizeof(int *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for internal long arrays.\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<dz->larray_cnt;n++)
		dz->lparray[n] = NULL;
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


/******************************** USAGE1 ********************************/

int usage1(void)
{
	usage2("shift");
	return(USAGE_ONLY);
}

/**************************** SETUP_FRAME_ARRAYS *****************************/

int setup_frame_arrays(dataptr dz)
{
	int chans = dz->infile->channels;

	if((dz->lparray[FRAME_OCHAN_LEFT]	= malloc(chans * sizeof(int)))==NULL) {
		sprintf(errstr,"Insufficient memory to store left-rotation data.\n");
		return(MEMORY_ERROR);
	}
	if((dz->lparray[FRAME_OCHAN_RIGHT]	= malloc(chans * sizeof(int)))==NULL) {
		sprintf(errstr,"Insufficient memory to store right-rotation data.\n");
		return(MEMORY_ERROR);
	}
	if((dz->lparray[FRAME_OL]			= malloc(chans * sizeof(int)))==NULL) {
		sprintf(errstr,"Insufficient memory to store snaked-left-rotation data.\n");
		return(MEMORY_ERROR);
	}
	if((dz->lparray[FRAME_OR]			= malloc(chans * sizeof(int)))==NULL) {
		sprintf(errstr,"Insufficient memory to store snaked-right-rotation data.\n");
		return(MEMORY_ERROR);
	}
	if((dz->lparray[FRAME_SNAKEPERM]	= malloc(chans * sizeof(int)))==NULL) {
		sprintf(errstr,"Insufficient memory to store current snake permutation data.\n");
		return(MEMORY_ERROR);
	}
	if((dz->lparray[FRAME_ORIENT]		= malloc(2 * sizeof(int)))==NULL) {
		sprintf(errstr,"Insufficient memory to store first_orientation vals.\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[FRAME_POS]			= malloc(2 * sizeof(double)))==NULL) {
		sprintf(errstr,"Insufficient memory to store rotation data.\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[FRAME_STEP]			= malloc(2 * sizeof(double)))==NULL) {
		sprintf(errstr,"Insufficient memory to store spatial step data.\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[FRAME_LLEVEL]		= malloc(2 * sizeof(double)))==NULL) {
		sprintf(errstr,"Insufficient memory to store left level vals.\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[FRAME_RLEVEL]		= malloc(2 * sizeof(double)))==NULL) {
		sprintf(errstr,"Insufficient memory to store right level vals.\n");
		return(MEMORY_ERROR);
	}
	return FINISHED;
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	if(!strcmp(prog_identifier_from_cmdline,"shift"))				dz->process = FRAME;
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
	if(!strcmp(str,"shift")) {
		fprintf(stderr,
	    "USAGE:\n"
	    "frame shift 1 infile outfile snake rotation [-ssmear]\n"
	    "frame shift 2 infile outfile snake rotation1 rotation2 [-ssmear]\n"
	    "frame shift 3 infile outfile reorient\n"
	    "frame shift 4 infile outfile mirrorplane\n"
	    "frame shift 5 infile outfile [-b]\n"
	    "frame shift 6 infile outfile swapA swapB\n"
	    "frame shift 7 infile outfile chaninfo gain\n"
	    "frame shift 8 infile outfile [-b]\n"
		"\n"
		"Modes 1 & 2 Rotate the entire frame of a multichannel file.\n"
		"Mode 3 Changes the channel assignment of a multichannel file.\n"
		"Mode 4 Mirrors the channel output around specified mirrorplane.\n"
		"Mode 5 Converts between ring-numbered & bilaterally numbered outchans.\n"
		"Mode 6 Swaps any pair of channels (swapA and swapB).\n"
		"Mode 7 Allows any channel, or set of channels, to be enveloped,\n"
		"       independently of the other channels.\n"
		"Mode 8 Converts between ring-numbered & BEAST bilateral numbering.\n"
		"\n"
		"ROTATION    rotation-rate in cycles (complete frame-rotations) per sec.\n"
		"            Mode 2 (only with files with even number of input channels)\n"
		"            rotates odd & even chans independently; has 2 rotation vals.\n"
		"            Negative values produce anticlockwise rotation.\n"
		"            Rotation rate can vary through time. Range -500 to +500.\n"
		"SMEAR       Extent to which channel-signals bleed onto adjacent chans.\n"
		"            (range 0 to 0.5 : default 0)\n"
		"SNAKE       In clockwise rotation,in (e.g.) 8 chans, input chan-1 follows\n"
		"            the route 1->2->3->4->5->6->7->8->1 etc. around lspkrs.\n"
		"            Other input chans form a \"snake\" following chan-1 round.\n"
		"            To produce rotation (default) set '\"snake\" to ZERO.\n"
		"            HOWEVER, channels might snake along a different route,\n"
		"            & route (which could vary over time) can be specified\n"
		"            in a data file of \"time : snake-list\" values\n"
		"            e.g. might specify route at time zero ,for 8-chan file\n"
		"            \"0.0 8 6 5 2 7 3 4 1\" & change route at a later time\n"
		"            e.g. \"1.378245 4 1 5 2 6 7 8 3\"\n"
		"            1st time-val in data must be zero. Times must increase.\n"
		"            IN MODE 2, odd & even chans rotate independently.\n"
		"            1->3->5->7->1 etc and 2->4->6->8->2 etc\n"
		"            Snake data directs motion round a different route.\n"
		"            e.g.  the snake \"8 6 5 2 7 3 4 1\" above\n"
		"            produces one snake around the odd entries\n"
		"            i.e. 8 -> 5 -> 7 -> 4 -> 8 etc\n"
		"            and a 2nd snake around the even entries\n"
		"            i.e. 6 -> 2 -> 3 -> 1 -> 6 etc\n"
		"REORIENT    List of ALL input chans, in new positions they will have.\n"
		"            e.g. for 4-chans: Data 4 1 2 3\n"
		"            sends Ch1 to 4, Ch2 -> 1, Ch3 -> 2 and Ch4 -> 3\n"
		"MIRRORPLANE Line around which frame is (symmetrically) mirrored.\n"
		"            Values can be any (integer) outchannel number OR\n"
		"            any half-way position between outchans e.g. 1.5, 2.5\n"
		"            With N chans, 'N.5' lies between Nth & 1st chan.\n"
		"-b (Mode 5) Convert bilateral to ring (Default, ring to bilateral).\n"
		"            Numbering of outchans can be ring, or bilateral.\n"
		"            e.g. for 7 outchans ...\n"
		"                 RING                   BILATERAL\n"
		"                   1                        1\n"
		"                7     2                  2     3\n"
		"               6       3                4       5\n"
		"                5     4                  6     7\n"
		"            All multichan pan programs assume RING numbering.\n"
		"            Use this mode to convert out of and into bilateral format.\n"
		"-b (Mode 8) Convert BEAST bilateral to ring (Default, ring to BEAST).\n"
		"            BEAST bilateral numbering for 8 outchans ...\n"
		"                 RING                     BEAST\n"
		"                   1                        7\n"
		"                8     2                  1     2\n"
		"               7       3                3       4\n"
		"                6     4                  5     6\n"
		"                   5                        8\n"
		"SWAPA,SWAPB The 2 channels that are to be swapped.\n"
		"CHANINFO    A single channel number, or a list of channels in a file.\n"
		"GAIN        Gain to apply to enveloped channels (can vary over time).\n");
	} else
		fprintf(stdout,"Unknown option '%s'\n",str);
	return(USAGE_ONLY);
}

int usage3(char *str1,char *str2)
{
	fprintf(stderr,"Insufficient parameters on command line.\n");
	return(USAGE_ONLY);
}

/******************************** FRAME ********************************/

int rotate(dataptr dz)
{
	int exit_status, n;
	int chans = dz->infile->channels, out_lspkr_step;		/* out_lspkr_step: For single rotation, step between adjacent lspkrs, out_lspkr_step is 1 */
	double time, srate = (double)dz->infile->srate;			/* For double rotation, step between even lspkrs (or between odd), out_lspkr_step is 2 */
	int nextclick;
	int click = (int)round(srate/(double)MAX_ROT) * chans; /* Timestep between each reading of rotation-speed brktables */
	double *inter_lspkr_position = dz->parray[FRAME_POS];	/* relative position (0-1) between lpskr pair (currently) associated with input chan */
															/* NB same VALUE for ALL input chans, but output-lspkr-pair different for each input chan */
	double *step	  = dz->parray[FRAME_STEP];				/* change in relative-position between lspkr-pair */
	double *llevel	  = dz->parray[FRAME_LLEVEL];			/* level on 'left' lpskr of pair, to produce apparent motion */
	double *rlevel	  = dz->parray[FRAME_RLEVEL];			/* level on 'right' lpskr of pair, to produce apparent motion */
	int *orient      = dz->lparray[FRAME_ORIENT];			/* first non-zero motion; either clock or anticlock */
	int *ochan_left  = dz->lparray[FRAME_OCHAN_LEFT];		/* 'left' lspkr of output-lspkr pair currently associated with specific input-channel */
	int *ochan_right = dz->lparray[FRAME_OCHAN_RIGHT];		/* 'right' lspkr of output-lspkr pair currently associated with specific input-channel */
	int *ol          = dz->lparray[FRAME_OL];				/* 'left' lspkr of output-lspkr pair once 'snaking' is factored in */
	int *or		  = dz->lparray[FRAME_OR];				/* 'right' lspkr of output-lspkr pair once 'snaking' is factored in */
	int *snakeperm   = dz->lparray[FRAME_SNAKEPERM];		/* Current snaking-path */
	int insampcnt, bufpos, inhere, outhere;
	float *ibuf = dz->sampbuf[0], *obuf = dz->sampbuf[1];
	double lsig, rsig;				/* input signal sent to 'left' and to 'right' lspkrs at current out-location. */
	double smearcompensate = 1.0;	/* Reduction in principle dignal to allow for sigs from adjacent chans been smeared into it. */

	if(dz->param[SMEAR] > 0.0)
		smearcompensate = 1 - (2.0 * dz->param[SMEAR]);

	time = 0.0;
	click = (int)round(srate/(double)MAX_ROT) * chans; /* Timestep between each reading of rotation-speed brktables */

	/* ESTABLISH VALUE OF FIRST SPATIAL STEP, AND ORIENTATION OF FIRST MOTION (clock or anticlock) */

	if((exit_status = read_values_from_all_existing_brktables(0.0,dz))<0)
		return(exit_status);
	step[0] = (dz->param[ROTATION0]/srate) * chans;
	orient[0] = get_first_rotation_orientation(0,dz);
	if(dz->mode == 1) {
		step[1] = (dz->param[ROTATION1]/srate) * chans;
		orient[1] = get_first_rotation_orientation(1,dz);
		if((orient[0] == 0) && (orient[1] == 0)) {
			sprintf(errstr,"No rotations specified (rotation speeds always zero)\n");
			return(DATA_ERROR);
		}
		out_lspkr_step = 2;	/* motions are only from an even lspkr to another even lpskr, or from an odd lspkr to another odd lspkr */
	} else {
		out_lspkr_step = 1;	/* motion moves from one lspkr to any other */
	}
	/*  IF THERE IS SNAKING, SETUP FIRST SNAKING-ROUTE */

	if(dz->itemcnt)	{
		dz->next_snake_loc = 0;
		get_next_snake(dz);
	}

	/* ESTABLISH CHANNEL-PAIRS BETWEEN WHICH INPUT-CHANNELS MOVE, INITIALLY, AND ESTABLISH INITIAL INTER-SPEAKER POSITION */

	if(dz->mode == 0) {
		if(orient[0] >= 0) {					/* clockwise */
			for(n=0;n<chans;n++) {
				ochan_left[n] = n;				/* output channel starts at location of input chan */
				ol[n] = ochan_left[n];
				ochan_right[n] = (n+out_lspkr_step) % chans;	/*input chans will move rightwards to adjacent output channel */
				or[n] = ochan_right[n];
				if(dz->itemcnt)					/* If there is snaking, 'adjacency' is redefined by the snaking-path */
					or[n] = (int)snakeperm[or[n]];
			}
			inter_lspkr_position[0] = -step[0];	/* Offset to left at start, so when algo starts to run by stepping to right, it's set back to initial position at 0 */
		} else {							/* anticlockwise */
			for(n=0;n<chans;n++) {
				ochan_right[n] = n;				/* output channel starts at location of input chan */
				or[n] = ochan_right[n];
				ochan_left[n] = n - out_lspkr_step;	/*input chans will move leftwards to adjacent output channel */
				if(ochan_left[n] < 0)
					ochan_left[n] += chans;
				ol[n] = ochan_left[n];
				if(dz->itemcnt)					/* If there is snaking, 'adjacency' is redefined by the snaking-path */
					ol[n] = (int)snakeperm[ol[n]];
			}
			inter_lspkr_position[0] = step[0];	/* Offset to right at start, so when algo starts to run by stepping to left, it's set back to initial position at 0 */
		}
	} else {
		if(orient[0] > 0) {
			for(n=0;n<chans;n+=2) {		/* EVEN lspkrs only */
				ochan_left[n] = n;
				ol[n] = ochan_left[n];
				ochan_right[n] = (n+out_lspkr_step) % chans;
				or[n] = ochan_right[n];
				if(dz->itemcnt)
					or[n] = (int)snakeperm[or[n]];
			}
			inter_lspkr_position[0] = -step[0];
		} else {
			for(n=0;n<chans;n+=2) {
				ochan_right[n] = n;
				or[n] = ochan_right[n];
				ochan_left[n] = n - out_lspkr_step;
				if(ochan_left[n] < 0)
					ochan_left[n] += chans;
				ol[n] = ochan_left[n];
				if(dz->itemcnt)
					ol[n] = (int)snakeperm[ol[n]];
			}
			inter_lspkr_position[0] = step[0];
		}
		if(orient[1] > 0) {
			for(n=1;n<chans;n+=2) {		/* ODD lspkrs only */
				ochan_left[n] = n;
				ol[n] = ochan_left[n];
				ochan_right[n] = (n+out_lspkr_step) % chans;
				or[n] = ochan_right[n];
				if(dz->itemcnt)
					or[n] = (int)snakeperm[or[n]];
			}
			inter_lspkr_position[1] = -step[1];
		} else {
			for(n=1;n<chans;n+=2) {
				ochan_right[n] = n;
				or[n] = ochan_right[n];
				ochan_left[n] =  n - out_lspkr_step;
				if(ochan_left[n] < 0)
					ochan_left[n] += chans;
				ol[n] = ochan_left[n];
				if(dz->itemcnt)
					ol[n] = (int)snakeperm[ol[n]];
			}
			inter_lspkr_position[1] = step[1];
		}
	}	
	nextclick = click;	/* Establish next time at which to read any rotation brkpnt data */
	insampcnt = 0;
	bufpos = 0;
	if((exit_status = read_samps(ibuf,dz))<0)			/* buflen is automatically a multipler of input channel count. */
		return(exit_status);
	memset((char *)obuf,0,dz->buflen * sizeof(float));	/* set obuf to zero, as values are ADDED into it */

	/* OUTER LOOP - PROCESS ENTIRE INPUT FILE TO OUTPUT */
	while(insampcnt < dz->insams[0]) {
		/* INNER LOOP - PROCESS FILE UP TO NEXT POTENTIAL ROTATION-BREAKPOINT READ */
		while(insampcnt < nextclick) {
			if(insampcnt >= dz->insams[0])
				break;
			inter_lspkr_position[0] += step[0];	/* Find next spatial location */

			/* IF inter-speaker position goes outside current lspkr pair, change to next loudpseaker pair, and readjust position to 0-1 range */

			if(dz->mode == 0) {
				if(inter_lspkr_position[0] < 0.0 || inter_lspkr_position[0] > 1.0) {
					if(inter_lspkr_position[0] > 1.0) {
						for(n=0;n<chans;n++) {
							ochan_left[n] = ochan_right[n];
							ol[n] = ochan_left[n];
							ochan_right[n] = (ochan_right[n] + out_lspkr_step) % chans;
							or[n] = ochan_right[n];
						}
						inter_lspkr_position[0] -= 1.0;
					} else {
						for(n=0;n<chans;n++) {
							ochan_right[n] = ochan_left[n];
							or[n] = ochan_right[n];
							ochan_left[n] = ochan_left[n] - out_lspkr_step;
							if(ochan_left[n] < 0)
								ochan_left[n] += chans;
							ol[n] = ochan_left[n];
						}
						inter_lspkr_position[0] += 1.0;
					}
					if(dz->itemcnt) { /* if snaking, redefine 'adjaceny' */
						for(n=0;n<chans;n++) {
							or[n] = (int)snakeperm[or[n]];
							ol[n] = (int)snakeperm[ol[n]];
						}
					}
				}
			} else {
				inter_lspkr_position[1] += step[1];	/* Find next spatial location in 2nd rotation, too*/

				if(inter_lspkr_position[0] < 0.0 || inter_lspkr_position[0] > 1.0) {
					if(inter_lspkr_position[0] > 1.0) {
						for(n=0;n<chans;n+=2) {		/* EVEN lpskrs only, NB */
							ochan_left[n] = ochan_right[n];
							ol[n] = ochan_left[n];
							ochan_right[n] = (ochan_right[n] + out_lspkr_step) % chans;
							or[n] = ochan_right[n];
						}
						inter_lspkr_position[0] -= 1.0;
					} else {
						for(n=0;n<chans;n+=2) {
							ochan_right[n] = ochan_left[n];
							or[n] = ochan_right[n];
							ochan_left[n] = ochan_left[n] - out_lspkr_step;
							if(ochan_left[n] < 0)
								ochan_left[n] += chans;
							ol[n] = ochan_left[n];
						}
						inter_lspkr_position[0] += 1.0;
					}
					if(dz->itemcnt) { /* if snaking, redefine 'adjaceny' */
						for(n=0;n<chans;n+=2) {
							or[n] = (int)snakeperm[or[n]];
							ol[n] = (int)snakeperm[ol[n]];
						}
					}
				}
				if(inter_lspkr_position[1] < 0.0 || inter_lspkr_position[1] > 1.0) {
					if(inter_lspkr_position[1] > 1.0) {
						for(n=1;n<chans;n+=2) {		/* ODD lpskrs only, NB */
							ochan_left[n] = ochan_right[n];
							ol[n] = ochan_left[n];
							ochan_right[n] = (ochan_right[n] + out_lspkr_step) % chans;
							or[n] = ochan_right[n];
						}
						inter_lspkr_position[1] -= 1.0;
					} else {
						for(n=1;n<chans;n+=2) {
							ochan_right[n] = ochan_left[n];
							or[n] = ochan_right[n];
							ochan_left[n] = ochan_left[n] - out_lspkr_step;
							if(ochan_left[n] < 0)
								ochan_left[n] += chans;
							ol[n] = ochan_left[n];
						}
						inter_lspkr_position[1] += 1.0;
					}
					if(dz->itemcnt) { /* if snaking, redefine 'adjaceny' */
						for(n=1;n<chans;n+=2) {
							or[n] = (int)snakeperm[or[n]];
							ol[n] = (int)snakeperm[ol[n]];
						}
					}
				}
			}
			/* SET RELATIVE LEVELS ON (every) LOUDSPEAKER PAIR, FROM POSITION INFO */

			set_stereo_levels(inter_lspkr_position[0],&(llevel[0]),&(rlevel[0]));
			if(dz->mode == 1)
				set_stereo_levels(inter_lspkr_position[1],&(llevel[1]),&(rlevel[1]));

			/* CALCULATE THE OUTPUT SUMMED FROM EVERY INPUT CHAN */

			if(dz->mode == 0) {
				for(n=0;n<chans;n++) {
					inhere = bufpos + n;		/* location of input sample is offset by 'n' to find the correct channel */
					outhere = bufpos + ol[n];	/* location of output to related 'left' lspkr is offset similarly to correct channel */
					lsig = llevel[0] * ibuf[inhere] * smearcompensate;
					obuf[outhere] = (float)(obuf[outhere] + lsig);
					outhere = bufpos + or[n];	/* location of output to related 'right' lspkr is offset similarly to correct channel */
					rsig = rlevel[0] * ibuf[inhere] * smearcompensate;
					obuf[outhere] = (float)(obuf[outhere] + rsig);
					if(dz->param[SMEAR] > 0.0)
						do_smear(ol[n],or[n],lsig,rsig,bufpos,chans,obuf,dz->param[SMEAR]);
				}
			} else {
				for(n=0;n<chans;n+=2) {		/* EVEN chans only */
					inhere = bufpos + n;
					outhere = bufpos + ol[n];
					lsig = llevel[0] * ibuf[inhere] * smearcompensate;
					obuf[outhere] = (float)(obuf[outhere] + lsig);
					outhere = bufpos + or[n];
					rsig = rlevel[0] * ibuf[inhere] * smearcompensate;
					obuf[outhere] = (float)(obuf[outhere] + rsig);
					if(dz->param[SMEAR] > 0.0)
						do_smear(ol[n],or[n],lsig,rsig,bufpos,chans,obuf,dz->param[SMEAR]);
				}
				for(n=1;n<chans;n+=2) {		/* ODD chans only */
					inhere = bufpos + n;
					outhere = bufpos + ol[n];
					lsig = llevel[1] * ibuf[inhere] * smearcompensate;
					obuf[outhere] = (float)(obuf[outhere] + lsig);
					outhere = bufpos + or[n];
					rsig = rlevel[1] * ibuf[inhere] * smearcompensate;
					obuf[outhere] = (float)(obuf[outhere] + rsig);
					if(dz->param[SMEAR] > 0.0)
						do_smear(ol[n],or[n],lsig,rsig,bufpos,chans,obuf,dz->param[SMEAR]);
				}
			}
			insampcnt += chans;				/* step to next group-sample of input */
			bufpos += chans;
											/* once whole buffer is processed, write output, and read new input */
			if(bufpos >= dz->ssampsread) {
				if((exit_status = write_samps(obuf,dz->ssampsread,dz))<0)
					return(exit_status);
				memset((char *)obuf,0,dz->buflen * sizeof(float));
				if((exit_status = read_samps(ibuf,dz))<0)
					return(exit_status);
				if(dz->ssampsread == 0)
					break;
				bufpos = 0;
			}
	
			/* IF MORE SNAKE DATA, IF IT'S TIME TO READ IT, READ IT */

			if(dz->next_snake_time && (insampcnt >= dz->next_snake_time))
				get_next_snake(dz);
		}

		/* ON REACHING NEXT POTENTIAL ROTATION-BRKPOINT, READ ROTATION-SPEED VALUE */

		time = (double)(insampcnt/chans)/srate;
		if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
			return(exit_status);
		step[0] = (dz->param[ROTATION0]/srate) * chans;
		if(dz->mode == 1)
			step[1] = (dz->param[ROTATION1]/srate) * chans;
		
		nextclick += click;		/* Set next (sample)time at which to read rotation-speed */
	}
	if(bufpos > 0) {
		if((exit_status = write_samps(obuf,bufpos,dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}

/************************************************** GET_NEXT_SNAKE **************************************************/

void get_next_snake(dataptr dz)
{
	int entrylen = dz->infile->channels + 1;
	int k = dz->next_snake_loc + 1, n;
	for(n=0;n < dz->infile->channels;n++) {
		dz->lparray[FRAME_SNAKEPERM][n] = dz->lparray[FRAME_SNAKE][k];
		k++;
	}
	dz->next_snake_loc += entrylen;
	if(dz->next_snake_loc < dz->itemcnt)
		dz->next_snake_time = dz->lparray[FRAME_SNAKE][dz->next_snake_loc];	/* time(in samples) of start of next snake data */
	else
		dz->next_snake_time = 0;	/* flags, no more snakes */
}

/************************** GET_FIRST_ROTATION_ORIENTATION ********************************/

int get_first_rotation_orientation(int n,dataptr dz)
{
	int j;
	if(dz->brksize[n] == 0) {
		if(dz->param[n] == 0.0)	/* no rotation */
			return 0;
		else if(dz->param[n] > 0.0)	/* clockwise rotation */
			return 1;
		return -1;				/* anticlockwise rotation */
	}
	for(j=1;j<dz->brksize[n];j+=2) {
		if(dz->brk[n][j] > 0.0)		/* first rotation is clockwise */
			return 1;
		if(dz->brk[n][j] < 0.0)		/* first rotation is anticlockwise */
			return -1;
	}
	return 0;					/* no (non-zero) rotation found */
}

/************************** READ_THE_SPECIAL_DATA ********************************/

int read_the_special_data(char *filename,dataptr dz)
{
	int exit_status, entrylen=0, linecnt, ival;
	int n = 0, k, chans = dz->infile->channels;
	double time, lasttime, val;
	char temp[200], *p, *thisword, temp2[200];

	thisword = temp2;
	p = filename;
	if((sloom && *p == '@') || (!sloom && isdigit(*p))) {
		switch(dz->mode) {
		case(2):
			sprintf(errstr,"Data must be in a file\n");
			return(DATA_ERROR);
		case(6):
			if(sloom)
				p++;
			if((sscanf(p,"%d",&n)) < 1) {
				sprintf(errstr,"No such channel (%s) in the input file.\n",p);
				return(DATA_ERROR);
			}
			if((n < 1) || (n > dz->infile->channels)) {
				sprintf(errstr,"No such channel (%d) in the input file.\n",n);
				return(DATA_ERROR);
			}
			if((dz->lparray[0] = malloc(sizeof(int)))==NULL) {
				sprintf(errstr,"Insufficient memory to store chans-to-process info.\n");
				return(MEMORY_ERROR);
			}
			dz->lparray[0][0] = n - 1;		//	Convert to 0 to N-1 frame
			dz->itemcnt = 1;
			return FINISHED;
		default:
			dz->itemcnt = 0;			/* flags 'no snake data' */
			dz->next_snake_time = 0;	/* flags 'no more snake data' */
			return FINISHED;
		}
	}
	if((dz->fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,"Cannot open datafile %s\n",filename);
		return(DATA_ERROR);
	}
	while(fgets(temp,200,dz->fp)!=NULL) {
		p = temp;
		if(is_an_empty_line_or_a_comment(p))
			continue;
		while(get_word_from_string(&p,&thisword))
			n++;
	}
	if(n==0) {
		sprintf(errstr,"No data in file %s\n",filename);
		return(DATA_ERROR);
	}
	dz->itemcnt = n;
	switch(dz->mode) {
	case(2):
		entrylen = chans;		// reorientation
		if(n != entrylen) {
			sprintf(errstr,"Reorient data in file %s is in incorrect format.\n",filename);
			return(DATA_ERROR);
		}
		if((dz->lparray[FRAME_SNAKE] = malloc(dz->itemcnt * sizeof(int)))==NULL) {
			sprintf(errstr,"Insufficient memory to store reorient data from file %s.\n",filename);
			return(MEMORY_ERROR);
		}
		break;
	case(0):
	case(1):
		entrylen = chans + 1;	// time-variable rotation-snake
		k = n % entrylen;
		if(k != 0) {
			sprintf(errstr,"Snake data in file %s is in incorrect format.\n",filename);
			return(DATA_ERROR);
		}
		if((dz->lparray[FRAME_SNAKE] = malloc(dz->itemcnt * sizeof(int)))==NULL) {
			sprintf(errstr,"Insufficient memory to store snaking data from file %s.\n",filename);
			return(MEMORY_ERROR);
		}
		break;
	case(6):
		if((dz->lparray[0] = malloc((dz->itemcnt + 4) * sizeof(int)))==NULL) {
			sprintf(errstr,"Insufficient memory to store edit-chans data in file %s.\n",filename);
			return(MEMORY_ERROR);
		}
		break;
	}

	if(fseek(dz->fp,0,0)<0) {
		sprintf(errstr,"fseek() failed in read_the_special_data()\n");
		return(SYSTEM_ERROR);
	}
	n = 0;
	linecnt = 1;
	lasttime = 0.0;
	while(fgets(temp,200,dz->fp)!=NULL) {
		p = temp;
		if(is_an_empty_line_or_a_comment(temp))
			continue;
		while(get_word_from_string(&p,&thisword)) {
			switch(dz->mode) {
			case(0):
			case(1):
				if(n % entrylen == 0) {
					if(sscanf(thisword,"%lf",&time)!=1) {
						sprintf(errstr,"Problem reading Time: line %d: file %s\n",linecnt,filename);
						return(DATA_ERROR);
					}
					if(n == 0) {
						if(time != 0.0) {
							sprintf(errstr,"First time is not zero in file %s\n",filename);
							return(DATA_ERROR);
						}
					} else {
						if(time <= lasttime) {
							sprintf(errstr,"Time does not advance at line %d in file %s\n",linecnt,filename);
							return(DATA_ERROR);
						}
							/* TEST SNAKE VALIDITY in last set of 'chans' values */
						if((exit_status = snake_test(n,lasttime,linecnt-1,filename,dz)) < 0)
							return(exit_status);
						lasttime = time;
					}
					dz->lparray[FRAME_SNAKE][n] = (int)round(time * dz->infile->srate) * chans;
				} else {
					if(sscanf(thisword,"%lf",&val)!=1) {
						sprintf(errstr,"Problem reading Value: line %d: file %s\n",linecnt,filename);
						return(DATA_ERROR);
					}
					ival = (int)round(val);
					if(ival < 1 || ival > chans) {
						sprintf(errstr,"Invalid channel number (%d) in file %s : line %d\n",ival,filename,linecnt);
						return(DATA_ERROR);
					}
					ival--;
					dz->lparray[FRAME_SNAKE][n] = ival;
				}
				break;
			case(2):
				if(sscanf(thisword,"%lf",&val)!=1) {
					sprintf(errstr,"Problem reading Value: file %s\n",filename);
					return(DATA_ERROR);
				}
				ival = (int)round(val);
				if(ival < 1 || ival > chans) {
					sprintf(errstr,"Invalid channel number (%d) in file %s\n",ival,filename);
					return(DATA_ERROR);
				}
				ival--;
				dz->lparray[FRAME_SNAKE][n] = ival;
				break;
			case(6):
				if(sscanf(thisword,"%lf",&val)!=1) {
					sprintf(errstr,"Problem reading Value: file %s\n",filename);
					return(DATA_ERROR);
				}
				ival = (int)round(val);
				if(ival < 1 || ival > chans) {
					sprintf(errstr,"Invalid channel number (%d) in file %s\n",ival,filename);
					return(DATA_ERROR);
				}
				ival--;
				dz->lparray[0][n] = ival;
				break;
			}
			n++;
		}
		linecnt++;
	}
	if(fclose(dz->fp)<0) {
		fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
		fflush(stdout);
	}
	switch(dz->mode) {
	case(2):
		if((exit_status = reorient_test(dz)) < 0)	/* Test reorientation entries */
			return(exit_status);
		break;
	case(6):
		if((exit_status = editchans_test(dz)) < 0)	/* Test edit-chans entries */
			return(exit_status);
		break;
	default:
		if((exit_status = snake_test(n,lasttime,linecnt-1,filename,dz)) < 0)	/* Test final set of snake entries */
			return(exit_status);
		break;
	}
	return(FINISHED);
}

/************************************************ SNAKE_TEST *****************************************/

int snake_test(int n,double lasttime,int linecnt,char *filename,dataptr dz)
{
	int m, j, k;
	int chans = dz->infile->channels;
	m = n - chans;
	for(k = m; k < n-1; k++) {
		for(j = k + 1; j < n; j++) {
			if(dz->lparray[FRAME_SNAKE][k] == dz->lparray[FRAME_SNAKE][j]) {
				sprintf(errstr,"Invalid snake sequence: file %s time %lf line %d: (channel %d repeated)\n",filename,lasttime,linecnt,dz->lparray[FRAME_SNAKE][k]);
				return(DATA_ERROR);
			}
		}
	}
	return(FINISHED);
}

/************************************************ SET_STEREO_LEVELS *****************************************/

void set_stereo_levels(double pos, double *llevel, double *rlevel)
{
	double relpos, temp, holecompensate;
	double zerocentredposition = (pos * 2.0) - 1.0;	/* range  -1 to 1 */
	if(zerocentredposition < 0) 
		relpos = -zerocentredposition;				/* range   0 to 1 : position relative to centre of stereo */
	else 
		relpos = zerocentredposition;
	temp = 1.0 + (relpos * relpos);					/* calculate hole in middle compensation */
	holecompensate = ROOT2 / sqrt(temp);
	*rlevel = pos * holecompensate;
	*llevel = (1.0 - pos) * holecompensate;
}

/************************************************ DO_SMEAR *****************************************/

void do_smear(int loutchan,int routchan,double lsig,double rsig,int bufpos,int chans,float *obuf,double smear)
{
	int smearleft,  smearright;
	// smear left output chan to its own left and right
	if((smearleft = loutchan - 1) < 0)
		smearleft += chans;
	if((smearright = loutchan + 1) >= chans)
		smearright -= chans;
	smearleft  += bufpos;
	smearright += bufpos;
	obuf[smearleft]  = (float)(obuf[smearleft]  + (lsig * smear));
	obuf[smearright] = (float)(obuf[smearright] + (lsig * smear));

	// smear right output chan to its own left and right
	if((smearleft = routchan - 1) < 0)
		smearleft += chans;
	smearleft += bufpos;
	if((smearright = routchan + 1) >= chans)
		smearright -= chans;
	smearright += bufpos;
	obuf[smearleft]  = (float)(obuf[smearleft]  + (rsig * smear));
	obuf[smearright] = (float)(obuf[smearright] + (rsig * smear));
}

/************************************************ REORIENT *****************************************/

int reorient(dataptr dz)
{
	int exit_status, chans = dz->infile->channels;
	float *ibuf = dz->sampbuf[0];
	float *obuf = dz->sampbuf[1];
	int *ochan = dz->lparray[FRAME_SNAKE];
	int n, m;
	while(dz->samps_left) {
		if((exit_status = read_samps(ibuf,dz))<0)
			return(exit_status);
		for(n=0;n < dz->ssampsread;n+=chans) {
			for(m=0;m <chans;m++)
				obuf[ochan[m]+n] = ibuf[m+n];
		}
		if((exit_status = write_samps(obuf,dz->ssampsread,dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}

/************************************************ REORIENT_TEST *****************************************/

int reorient_test(dataptr dz) 
{
	int chans = dz->infile->channels;
	int *ochan = dz->lparray[FRAME_SNAKE];
	int n, m;
	for(n=0;n<chans-1;n++) {
		if(ochan[n] >= chans) {
			sprintf(errstr,"Invalid Channel (%d) in reorientation data\n",ochan[n]+1);
			return(DATA_ERROR);
		}
		for(m=n+1;m<chans;m++) {
			if(ochan[n] == ochan[m]) {
				sprintf(errstr,"Duplicated Channel (%d) in reorientation data\n",ochan[n]+1);
				return(DATA_ERROR);
			}
		}
	}
	if(ochan[n] >= chans) {
		sprintf(errstr,"Invalid Channel (%d) in reorientation data\n",ochan[n]+1);
		return(DATA_ERROR);
	}
	return FINISHED;
}

/************************************************ EDITCHANS_TEST *****************************************/

int editchans_test(dataptr dz) 
{
	int *echan = dz->lparray[0];
	int n, m;
	for(n=0;n<dz->itemcnt-1;n++) {
		for(m=n+1;m<dz->itemcnt;m++) {
			if(echan[n] == echan[m]) {
				sprintf(errstr,"repeated channel (%d) in channel edit data\n",echan[n]+1);
				return(DATA_ERROR);
			}
		}
	}
	return FINISHED;
}

/************************************ CHECK_THE_PARAM_VALIDITY_AND_CONSISTENCY ****************************/

int check_the_param_validity_and_consistency(dataptr dz)
{
	int exit_status, k;
	double frac;
	switch(dz->mode) {
	case(3):
		k = (int)floor(dz->param[0]);
		if(k > dz->infile->channels) {
			sprintf(errstr,"Mirrorplane (%.1lf) not compatible with channel count (%d)\n",dz->param[0],dz->infile->channels);
			return(DATA_ERROR);
		}
		if(k != dz->param[0]) {
			frac = dz->param[0] - (double)k;
			if(!flteq(frac,0.5)) {
				sprintf(errstr,"Mirrorplane value must an be integers or a half-integer (e.g. 1.5, 3.5)\n");
				return(DATA_ERROR);
			}
		}
		if((exit_status = mirror(dz))<0)
			return(exit_status);
		break;
	case(4):
		if((exit_status = bilateral(dz))<0)
			return(exit_status);
		break;
	case(7):
		if((exit_status = beast_bilateral(dz))<0)
			return(exit_status);
		break;
	case(5):
		if(dz->iparam[0] > dz->infile->channels) {
			sprintf(errstr,"Channel %d does not exist in input file.\n",dz->iparam[0]);
			return(DATA_ERROR);
		}
		if(dz->iparam[1] > dz->infile->channels) {
			sprintf(errstr,"Channel %d does not exist in input file.\n",dz->iparam[1]);
			return(DATA_ERROR);
		}
		if(dz->iparam[0] == dz->iparam[1]) {
			sprintf(errstr,"Can't swap channel %d with itself\n",dz->iparam[0]);
			return(DATA_ERROR);
		}
		dz->iparam[0]--;		// Change to 0 to N-1 frame 
		dz->iparam[1]--;
		break;
	}
	return FINISHED;
}

/************************************ MIRROR ****************************/

int mirror(dataptr dz)
{
	int mirrorplane, outchans = dz->infile->channels, ichan, ochan, lastchan, n;
	int *mirrormap;
	if((dz->lparray[FRAME_SNAKE] = malloc(outchans * sizeof(int)))==NULL) {
		sprintf(errstr,"Insufficient memory to store mirroring data.\n");
		return(MEMORY_ERROR);
	}
	mirrormap = dz->lparray[FRAME_SNAKE];
	mirrorplane = (int)round(dz->param[0] * 2.0);	//	2.0  maps to 4	2.5 maps to 5
	if(EVEN(mirrorplane))
		mirrorplane = (int)round(dz->param[0]);		// 2.0 maps to 2
	else
		mirrorplane = -(mirrorplane/2);				//	5 (2.5) maps to -2 : 7 (3.5) to -3 etc.
	if(mirrorplane > 0) {			//	mirror centred in lspkr
		mirrorplane--;				//	lspkrs are numbered 0 to (N-1)
		for(n=0; n < outchans; n++) {			// if mirrorplane is at zero
			ichan = mirrorplane + n;			//	 0	 1	 2	 3	 4	 5	 6	 7		OR		0	1	 2	 3	 4	 5	 6
			while(ichan >= outchans)
				ichan -= outchans;				//	(even chancnt) GOES TO						(odd chancnt) GOES TO
			ochan = mirrorplane - n;
			while(ochan < 0)					//	 0  -1	-2	-3	-4	-5	-6	-7				0  -1   -2	-3	-4	-5	-6
				ochan += outchans;				// = 0	 7	 6	 5	 4	 3	 2	 1			  = 0	6	 5	 4	 3	 2	 1
			mirrormap[ichan] = ochan;
		}
	} else {						//	mirror centred between lspkrs
		mirrorplane = -mirrorplane;				//	-2 to 2 etc. mirror is between 2 & 3 of internal numbering (0 to (N-1))
		mirrorplane--;				//	lspkrs are numbered 0 to (N-1)
												// if mirrorplane is at 0.5, (mirrorplane value here 0)
		if(EVEN(outchans)) {					// originally 1.5: ABOVE: doubled = 3 : to -(N/2)= -1: HERE to -N = 1: to decr = 0)
			for(n=1; n <= outchans; n++) {		//	 1	 2	 3	 4	 5	 6	 7	 8
				ichan = mirrorplane + n;		// = 1	 2	 3	 4	 5	 6	 7	 0
				while(ichan >= outchans)
					ichan -= outchans;			//	(even chancnt) GOES TO
				ochan = mirrorplane - n + 1;
				while(ochan < 0)				//	 0  -1	-2	-3	-4	-5	-6	-7
					ochan += outchans;			// = 0	 7	 6	 5	 4	 3	 2	 1
				mirrormap[ichan] = ochan;
			}
		} else {
			lastchan = outchans/2;				//	for 7, lastchan = 3
			lastchan++;							//	for 7, lastchan = 4
			for(n=1; n <= lastchan; n++) {		// if mirrorplane is at 0.5
				ichan = mirrorplane + n;		//   1	 2	 3	 4
				while(ichan >= outchans)
					ichan -= outchans;			//	(odd chancnt) GOES TO
				ochan = mirrorplane - n + 1;
				while(ochan < 0)				//   0  -1  -2  -3	
					ochan += outchans;			//  =0	 6	 5	 4
				mirrormap[ichan] = ochan;
				mirrormap[ochan] = ichan;		//	and vice versa
			}
		}
	}
	return(FINISHED);
}

/************************************ BILATERAL ************************/

int bilateral(dataptr dz)
{
	int outchans = dz->infile->channels, ichan, ochan, split;
	int *bi;
	int toring = dz->vflag[0];
	if((dz->lparray[FRAME_SNAKE] = malloc(outchans * sizeof(int)))==NULL) {
		sprintf(errstr,"Insufficient memory to store bilateralisation data.\n");
		return(MEMORY_ERROR);
	}
	bi = dz->lparray[FRAME_SNAKE];
	split = (int)ceil((double)outchans/2.0);	//	8 --> 4		7 --> 4
	ichan = outchans - 1;
	ochan = 1;
	while(ichan >= split) {						//	with 8 (even no) chans			with 7 (odd no) chans
		if(toring)								//	7	6	5	4			OR		6	5	4
			bi[ochan] = ichan;					//	GOES TO							GOES TO
		else									//	1	3	5	7					1	3	5
			bi[ichan] = ochan;
		ichan--;
		ochan += 2;
	}
	while (ichan >= 0) {						//	with 8 (even no) chans			with 7 (odd no) chans
		if(toring)								//	3	2	1	0			OR		3	2	1	0
			bi[ichan * 2] = ichan;				//	GOES TO							GOES TO
		else									//	6	4	2	0					6	4	2	0
			bi[ichan] = ichan * 2;				
		ichan--;
	}
	return(FINISHED);							//	   0			  0				   0			  0
}												//	 7	 1			1	2			 6	 1			1	2
												//	6	  2	  TO   3	 4			5	  2	  TO   3	 4
												//	 5	 3			5	6			 4	 3			5	6
												//	   4			  7

/************************************ BEAST_BILATERAL ************************/

int beast_bilateral(dataptr dz)
{
	int outchans = dz->infile->channels, ichan, ochan;
	int *bi;
	int toring = dz->vflag[0], even, lastevenchanno, lastoddchanno = 0, limit;
	if((dz->lparray[FRAME_SNAKE] = malloc(outchans * sizeof(int)))==NULL) {
		sprintf(errstr,"Insufficient memory to store bilateralisation data.\n");
		return(MEMORY_ERROR);
	}
	bi = dz->lparray[FRAME_SNAKE];
	even = 1;
	if((outchans/2) * 2 != outchans) {
		even = 0;
		lastevenchanno = outchans - 1;			// 6 for 7channel file
	} else {
		lastevenchanno = outchans - 2;			// 6 for 8channel file
		lastoddchanno  = outchans - 1;			// 7 for 8channel file
	}
	if(toring)
		bi[lastevenchanno] = 0;
	else
		bi[0] = lastevenchanno;						//	0(1) -->6(7)
	if(even) {
		if(toring)
			bi[lastoddchanno] = outchans/2;
		else
			bi[outchans/2] = lastoddchanno;			//	4(5) -->7(8)	if even number of chans
	}
	limit = outchans/2;						// for 8 chans, up to 4
	if(!even)
		limit++;							// for 7 chans, up to 4
	ichan = 1;
	while(ichan < limit) {
		if(toring)
			bi[(ichan * 2) - 1] = ichan;
		else
			bi[ichan] = (ichan * 2) - 1;	//	1->1 2->3 3->5 (= 2->2 3->4 4->6)
		ichan++;
	}
	ochan = 0;
	ichan = outchans - 1;					//	for 8chans 7, for 7chans 6
	limit = outchans/2;						// for 8 chans, down to >4, for 7 chans, down to >3
	while(ichan > limit) {
		if(toring)
			bi[ochan] = ichan;
		else
			bi[ichan] = ochan;				//	7->0 6->2 5->4 (= 8->1 7->3 6->5)
		ichan--;
		ochan += 2;
	}
	return(FINISHED);							//	   0			  6				   0			  6
}												//	 7	 1			0	1			 6	 1			0	1
												//	6	  2	  TO   2	 3			5	  2	  TO   2	 3
												//	 5	 3			4	5			 4	 3			4	5
												//	   4			  7

/************************************ SWAPCHAN ************************/

int swapchans(dataptr dz)
{
	int exit_status;
	float *buf = dz->sampbuf[0], temp;
	int n;
	int ochans = dz->infile->channels;
	int thischan = dz->iparam[0], thatchan = dz->iparam[1];

	if((exit_status = read_samps(buf,dz))<0)
		return(exit_status);
	if(dz->ssampsread == 0) {
		sprintf(errstr,"No data in input soundfile\n");
		return(DATA_ERROR);
	}
	while(dz->ssampsread > 0) {
		for(n=0;n<dz->ssampsread;n+=ochans) {
			temp = buf[n + thischan];
			buf[n + thischan] = buf[n + thatchan];
			buf[n + thatchan] = temp;
		}			
		if((exit_status = write_samps(buf,dz->ssampsread,dz))<0)
			return(exit_status);
		if((exit_status = read_samps(buf,dz))<0)
			return(exit_status);
	}
	return FINISHED;
}

/************************************ ENVCHANS ************************/

int envchans(dataptr dz)
{
	int exit_status;
	float *buf = dz->sampbuf[0];
	int n;
	int ochans = dz->infile->channels, k, thisochan;
	int *chaninfo = dz->lparray[0], chunksamptime;
	double thisgain, nextgain=0, gainstep, timestep, gainincr;
	int thistime, nexttime;
	int thisbrk = 0, nextbrk = 2;
	double *gainvals = NULL;

	if(dz->brksize[0]) {
		gainvals = dz->brk[0];
		thistime = (int)round(gainvals[thisbrk] * (double)dz->infile->srate);
		if(thistime > 0.0) {
			nexttime = (int)round(gainvals[nextbrk] * (double)dz->infile->srate);
			thisgain = gainvals[thisbrk+1];
			nextgain = thisgain;		//	If before start of brktable, set gain to first val in table
			gainincr = 0.0;				//	and gain-increment to 0
		} else {
			nexttime = (int)round(gainvals[nextbrk] * (double)dz->infile->srate);
			thisgain = gainvals[thisbrk+1];
			nextgain = gainvals[nextbrk+1];
			gainstep = nextgain - thisgain;
			timestep = nexttime - thistime;
			gainincr = gainstep/(double)timestep;
			thisbrk += 2;
			nextbrk += 2;
		}
	} else {
		thisgain = dz->param[0];
		gainincr = 0.0;
		nexttime = dz->insams[0] + 2;		//	i.e. larger than chunksamptime counter can reach
	}
	if((exit_status = read_samps(buf,dz))<0)
		return(exit_status);
	if(dz->ssampsread == 0) {
		sprintf(errstr,"No data in input soundfile\n");
		return(DATA_ERROR);
	}
	chunksamptime = 0;
	while(dz->ssampsread > 0) {
		for(n=0;n<dz->ssampsread;n+=ochans,chunksamptime++) {
			if(chunksamptime >= nexttime) {
				if(nextbrk < dz->brksize[0] * 2) {
					thistime = nexttime;
					thisgain = nextgain;
					nexttime = (int)round(gainvals[nextbrk] * (double)dz->infile->srate);
					nextgain = gainvals[nextbrk+1];
					gainstep = nextgain - thisgain;
					timestep = nexttime - thistime;
					gainincr = gainstep/(double)timestep;
					thisbrk += 2;
					nextbrk += 2;
				} else
					gainincr = 0.0;		//	If run off end of brktable, keep  gain steady
			}
			for(k = 0; k < dz->itemcnt;k++) {
				thisochan = chaninfo[k];
				buf[n + thisochan] = (float)(buf[n + thisochan] * thisgain);
			}
			thisgain += gainincr;
			if(gainincr > 0.0)				//	Avoid rounding errors
				thisgain = min(thisgain,nextgain);
			else if(gainincr < 0.0)
				thisgain = max(thisgain,nextgain);
		}
		if((exit_status = write_samps(buf,dz->ssampsread,dz))<0)
			return(exit_status);
		if((exit_status = read_samps(buf,dz))<0)
			return(exit_status);
	}
	return FINISHED;
}
