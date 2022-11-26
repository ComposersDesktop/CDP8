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

#define SHRLEVLIM (0.95)
#define MINUS60DB (0.001)

#ifdef unix
#define round(x) lround((x))
#endif

#define	maxival scalefact

char errstr[2400];

int anal_infiles = 1;
int	sloom = 0;
int sloombatch = 0;

const char* cdp_version = "7.1.0";

//CDP LIB REPLACEMENTS
static int check_shrink_validity_and_consistency(dataptr dz);
static int setup_shrink_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_shrink_param_ranges_and_defaults(dataptr dz);
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

static int shrink(dataptr dz);
static int shrink2(dataptr dz);

static int create_shrink_sndbufs(dataptr dz);
static void shrink_normalise(float *buf,int len,double gain,int maxat);
static double getmaxval(float *buf,int len,int *maxat);
static void reverse_data(float *obuf,int seglen,dataptr dz);
static int shrink_param_preprocess(dataptr dz);

static int extract_env_from_sndfile(int *envcnt,dataptr dz);
static void getenv_of_buffer(int samps_to_process,int envwindow_sampsize,double convertor,double time_convertor,float **envptr,float *buffer,dataptr dz);
static double getmaxsamp(int startsamp, int sampcnt,float *buffer);
static int eliminate_too_low_peaks(int *envcnt,int *trofcnt,dataptr dz);
static int eliminate_redundant_trofs(int *envcnt,int *trofcnt,int orig_env_size,dataptr dz);
static int shuffle_trofs(int troflocalcnt,int *trofcnt,int start_of_trofs,int *trofno,float peaktime,float lastpeaktime,int *lastrofstored,dataptr dz);
static int find_exact_peaktimes(int envcnt,int *peaks,dataptr dz);
static int copy_peaktimes_to_env_array(dataptr dz);
static int find_exact_troftimes(int trofcnt,int *trofs,dataptr dz);
static int test_buffers(int envcnt,int trofcnt,dataptr dz);
static void extract_peaks_from_envelope(int *envcnt,dataptr dz);
static void extract_trofs_from_envelope(int *trofcnt,int orig_env_size,dataptr dz);
static int buffers_in_sndfile(int buffer_size,dataptr dz);
static int windows_in_sndfile(dataptr dz);
static int find_outsamps(dataptr dz);
static int handle_the_special_data(char *str,dataptr dz);
static int insert_trof(int *trofcnt,int trofno,float *troftime,float peaktime,float lastpeaktime,int orig_env_size,dataptr dz);
static int copy_peaktimes(dataptr dz);
static int find_local_minpeak(int *minpeakat,float *minpeak,int searchstt,int searchend,int searchlen, double lastpeaktime, double peaktime, dataptr dz);

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
		dz->maxmode = 6;
		if((exit_status = get_the_mode_from_cmdline(cmdline[0],dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(exit_status);
		}
		cmdline++;
		cmdlinecnt--;
		// setup_particular_application =
		if((exit_status = setup_shrink_application(dz))<0) {
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
	if((exit_status = setup_shrink_param_ranges_and_defaults(dz))<0) {
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
	dz->array_cnt	= 3;
	if((dz->parray  = (double **)malloc(dz->array_cnt * sizeof(double *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for internal double arrays.\n");
		return(MEMORY_ERROR);
	}
	dz->larray_cnt	= 3;
	if((dz->lparray  = (int **)malloc(dz->larray_cnt * sizeof(int *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for internal double arrays.\n");
		return(MEMORY_ERROR);
	}
	if(dz->mode == SHRM_LISTMX) {
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
	if((exit_status = check_shrink_validity_and_consistency(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	is_launched = TRUE;
	if((exit_status = create_shrink_sndbufs(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = shrink_param_preprocess(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	//spec_process_file =
	if((exit_status = open_the_outfile(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	switch(dz->mode) {
	case(SHRM_FINDMX):
	case(SHRM_LISTMX):
		exit_status = shrink2(dz);
		break;
	default:
		exit_status = shrink(dz);
		break;
	}
	if(exit_status <0) {
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
	if(dz->mode == SHRM_FINDMX || dz->mode == SHRM_LISTMX) {
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
	} else
		strcpy(dz->outfilename,filename);	   
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
	if(dz->mode == SHRM_FINDMX || dz->mode == SHRM_LISTMX)
		strcat(filename,"0.wav");
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

/************************* SETUP_SHRINK_APPLICATION *******************/

int setup_shrink_application(dataptr dz)
{
	int exit_status;
	aplptr ap;
	if((exit_status = establish_application(dz))<0)		// GLOBAL
		return(FAILED);
	ap = dz->application;
	// SEE parstruct FOR EXPLANATION of next 2 functions
	switch(dz->mode) {
	case(SHRM_TIMED):
		if((exit_status = set_param_data(ap,0   ,6,6,"dddddd"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"smr",3,"ddd","ni",2,0,"00"))<0)
			return(FAILED);
		break;
	case(SHRM_FINDMX):
		if((exit_status = set_param_data(ap,0   ,6,5,"0ddddd"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"smrglq",6,"dddddd","nieo",4,0,"0000"))<0)
			return(FAILED);
		break;
	case(SHRM_LISTMX):
		if((exit_status = set_param_data(ap,SHRFOC,6,5,"0ddddd"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"smrgl",5,"ddddd","nieo",4,0,"0000"))<0)
			return(FAILED);
		break;
	default:
		if((exit_status = set_param_data(ap,0   ,6,5,"0ddddd"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"smr",3,"ddd","ni",2,0,"00"))<0)
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

/************************* SETUP_SHRINK_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_shrink_param_ranges_and_defaults(dataptr dz)
{
	int exit_status;
	aplptr ap = dz->application;
	// set_param_ranges()
	ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
	// NB total_input_param_cnt is > 0 !!!
	if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
		return(FAILED);
	// get_param_ranges()
	if(dz->mode == SHRM_TIMED) {
		ap->lo[SHR_TIME] = 0.0;
		ap->hi[SHR_TIME] = dz->duration;
		ap->default_val[SHR_TIME] = dz->duration/2.0;
	}
	ap->lo[SHR_INK]	= 0;
	ap->hi[SHR_INK]	= 1;
	ap->default_val[SHR_INK] = 0.9;
	if(dz->mode == SHRM_FINDMX || dz->mode == SHRM_LISTMX) {
		ap->lo[SHR_WSIZE] = 1;
		ap->hi[SHR_WSIZE] = 128;
		ap->default_val[SHR_WSIZE] = 50;
		ap->lo[SHR_AFTER]	= 0.0;
		ap->hi[SHR_AFTER]	= dz->duration;
		ap->default_val[SHR_AFTER] = 0.0;
	} else {
		ap->lo[SHR_GAP]	= dz->duration;
		ap->hi[SHR_GAP]	= 60;
		ap->default_val[SHR_GAP] = dz->duration * 2.0;
		ap->lo[SHR_DUR]	= dz->duration * 2.0;
		ap->hi[SHR_DUR]	= 32767;
		ap->default_val[SHR_DUR] = dz->duration * 8;
	}
	ap->lo[SHR_CNTRCT]	= 0;
	ap->hi[SHR_CNTRCT]	= 1;
	ap->default_val[SHR_CNTRCT]	= 1;
	ap->lo[SHR_SPLEN]	= 2;
	ap->hi[SHR_SPLEN]	= 50.0;
	ap->default_val[SHR_SPLEN] = 5;
	ap->lo[SHR_SMALL]	= 0;
	ap->hi[SHR_SMALL]	= dz->insams[0];
	ap->default_val[SHR_SMALL] = 0;
	ap->lo[SHR_MIN]	    = 0;
	ap->hi[SHR_MIN]	    = 32767;
	ap->default_val[SHR_MIN] = 0;
	ap->lo[SHR_RAND]	    = 0;
	ap->hi[SHR_RAND]	    = 1;
	ap->default_val[SHR_RAND] = 0;
	if(dz->mode == SHRM_FINDMX || dz->mode == SHRM_LISTMX) {
		ap->lo[SHR_GATE] = 0;
		ap->hi[SHR_GATE] = 1;
		ap->default_val[SHR_GATE] = 0;
		ap->lo[SHR_LEN] = 0;
		ap->hi[SHR_LEN] = dz->duration;
		ap->default_val[SHR_LEN] = 0;
	}
	if(dz->mode == SHRM_FINDMX) {
		ap->lo[SHR_SKEW] = 0;
		ap->hi[SHR_SKEW] = 1;
		ap->default_val[SHR_SKEW] = 0.25;
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
			if((exit_status = setup_shrink_application(dz))<0)
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
	usage2("shrink");
	return(USAGE_ONLY);
}

/**************************** CHECK_SHRINK_VALIDITY_AND_CONSISTENCY *****************************/

int check_shrink_validity_and_consistency(dataptr dz)
{
	int chans = dz->infile->channels;
	double srate = (double)dz->infile->srate;
	dz->iparam[SHR_SPLEN] = (int)round(dz->param[SHR_SPLEN] * srate * MS_TO_SECS);
	dz->iparam[SHR_MIN]   = (int)round(dz->param[SHR_MIN]   * srate) * chans;
	dz->iparam[SHR_SMALL] = (int)round(dz->param[SHR_SMALL] * srate) * chans;
	if(dz->mode == SHRM_FINDMX || dz->mode == SHRM_LISTMX) {
		if(dz->param[SHR_AFTER] >= dz->duration) {
			sprintf(errstr,"Contraction start point is too late.\n");
			return(DATA_ERROR);
		} else if(dz->param[SHR_AFTER] >= (3.0 * dz->duration)/4.0) {
			fprintf(stdout,"WARNING: Contraction start point is late in source.\n");
			fflush(stdout);
		}
		dz->iparam[SHR_LEN] = (int)round(dz->param[SHR_LEN] * srate) * chans;
	}
	else {
		dz->iparam[SHR_GAP]   = (int)round(dz->param[SHR_GAP]   * srate) * chans;
		if(dz->param[SHR_GAP] <= dz->param[SHR_MIN]) {
			sprintf(errstr,"Minimum event separation must be less than initial event separation.\n");
			return(DATA_ERROR);
		}
	}
	if(dz->param[SHR_CNTRCT] < dz->param[SHR_INK]) {
		sprintf(errstr,"Contraction of inter-events distance can't be less than shrinkage of sounds.\n");
		return(DATA_ERROR);
	}
	if(((dz->iparam[SHR_SPLEN] * 2) + 1) * chans > dz->iparam[SHR_SMALL]) {
		if(dz->iparam[SHR_SMALL] > 0) {
			sprintf(errstr,"Minimum sound length must be greater than 2 splice lengths.\n");
			return(DATA_ERROR);
		} else {
			dz->iparam[SHR_SMALL] = ((dz->iparam[SHR_SPLEN] * 2) + 1) * chans;
			if(dz->iparam[SHR_MIN] == 0)
				dz->iparam[SHR_MIN] = ((dz->iparam[SHR_SPLEN] * 2) + 1) * chans;
		}
	}
	if(dz->iparam[SHR_SMALL] > dz->iparam[SHR_MIN]) {
		if(dz->iparam[SHR_MIN] == 0)
			dz->iparam[SHR_MIN] = dz->iparam[SHR_SMALL];
		else {
			sprintf(errstr,"Minimum sound length must be less than minimum event separation.\n");
			return(DATA_ERROR);
		}
	}
	if(dz->insams[0] <= dz->param[SHR_SMALL]) {
		sprintf(errstr,"Minimum sound length must be less than input sound length.\n");
		return(DATA_ERROR);
	}
	if(((dz->iparam[SHR_SPLEN] * 2) + 1) * chans > dz->iparam[SHR_SMALL]) {
		sprintf(errstr,"Minimum sound length must be greater than length of two splices.\n");
		return(DATA_ERROR);
	}
	if(dz->mode == SHRM_TIMED) {
		dz->iparam[SHR_TIME] = (int)round(dz->param[SHR_TIME] * srate) * chans;
		if(dz->iparam[SHR_TIME] <= dz->iparam[SHR_SPLEN] * chans) 
			dz->mode = SHRM_START;
		else if(dz->iparam[SHR_TIME] >= dz->insams[0] - (dz->iparam[SHR_SPLEN] * chans)) 
			dz->mode = SHRM_CENTRE;
	}
	return FINISHED;
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	if(!strcmp(prog_identifier_from_cmdline,"shrink"))				dz->process = SHRINK;
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
	if(!strcmp(str,"shrink")) {
		fprintf(stdout,
	    "USAGE: shrink shrink 1-3 infile outfile shrinkage\n"
	    "gap contract dur spl [-ssmall] [-mmin] [-rrnd] [-n] [-i]\n"
	    "USAGE: shrink shrink 4   infile outfile time shrinkage\n"
	    "gap contract dur spl [-ssmall] [-mmin] [-rrnd] [-n] [-i]\n"
		"USAGE: shrink shrink 5 infil generic-outfile-name shrinkage\n"
		"wsiz contract aft spl [-ssmall] [-mmin] [-rrnd] [-llen] [-ggate] [-qskew]\n"
		"                                               [-n] [-i] [-e] [-o]\n"
		"USAGE: shrink shrink 6 infil generic-outfile-name peaktimes shrinkage\n"
		"wsiz contract aft spl [-ssmall] [-mmin] [-rrnd] [-llen] [-ggate]\n"
		"                                               [-n] [-i] [-e] [-o]\n"
		"REPEAT SOUND, shortening it on each repetition.\n"
		"Mode 1: Shrink from end.            Mode 2: Shrink around midpoint.\n"
		"Mode 3: Shrink from start.          Mode 4: Shrink around specified time.\n"
		"OR, ISOLATE EVENTS IN SOUND, and shorten as we proceed from one to next.\n"
		"Mode 5: Shrink around found peaks and output each segment + mixfile.\n"
		"Mode 6: Shrink around specified peaks and output each segment + mixfile.\n"
		"TIME      Time around which shrinkage takes place.\n"
		"SHRINKAGE Shortening factor of sound from one repeat to next.\n"
		"          Shrinkage stops once events too short for splices.\n"
		"GAP       Initial timestep between output events (>= sound duration).\n"
		"CONTRACT  Shortening of gaps between output events.\n"
		"          1.0 = events equally spaced, < 1.0 events become closer\n"
		"          Events can't overlap, so minimum \"contraction\" = \"shrinkage\".\n"
		"DUR       (Minimum) duration of output.\n"
		"AFT       Time after which shrinkage begins.\n"
		"SPL       Splice length in mS.\n"
		"SMALL     Minimum sound length (after which sounds are of equal length).\n"
		"MIN       Minimum event separation (after which events are regular in time).\n"
		"RND       Randomisation of timings AFTER min event separation reached.\n"
		"PEAKTIMES Textfile list of times of peaks in sound.\n"
		"WSIZ      Windowsize (in mS) for extracting envelope (1-100 dflt 100)\n"
		"LEN       Minimum segment length before sound squeezing can begin (for -e flag)\n"
		"GATE      Level (relative to max) below which found peaks ignored (0-1 dflt 0)\n"
		"SKEW      How envelope centred on segment (0 to 1: dflt 0.25). 0.5 = central.\n"
		"          Zero value switches the flag off.\n"
		"-n        Equalise maximum level of output events (if possible).\n"
		"-i        Inverse: Reverse each segment in output.\n"
		"          Reversing outfile creates stream of unreversed segments\n"
		"          where segments expand/accelerate (rather than shrink/contract).\n"
		"-e        Even Squeeze: Sounds shorten in regular manner, from 1st squeezed segment.\n"
		"          (squeezed snd lengths not dependent on length of input segments).\n"
		"-o        Omit any too-quiet events once a fixed end tempo has been reached.\n"
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

/******************************** SHRINK ********************************/

int shrink(dataptr dz)
{
	int exit_status, chans = dz->infile->channels, sndchanged, continuation;
	double spliceval = 0.0, splincr = 1.0/(double)dz->iparam[SHR_SPLEN], srate = (double)dz->infile->srate;
	int sndlen, lastsndlen, seglen, lastseglen = 0, halflen, centre, startseg, endseg, remnant, maxat = 0, n = 0, m, k;
	int splen = dz->iparam[SHR_SPLEN], splenlen, dblsplenlen, startsplice, gpinsams, gpinsegm, randrange = 0, offset;
	double outdur, thismaxval, gain = 1.0, contraction, shrinkage, val;
	float *ibuf = dz->sampbuf[0], *obuf = dz->sampbuf[1];
	splenlen = splen * chans;
	dblsplenlen = ((splen * 2) + 1) * chans;
	gpinsegm = dz->iparam[SHR_GAP]/chans;			//	Initial length of segment, in sample-groups
	gpinsams = dz->insams[0]/chans;					//	Initial length of sound, in sample-groups
	if((exit_status = read_samps(ibuf,dz))<0)
		return(exit_status);
	if(dz->maxival > SHRLEVLIM) {
		fprintf(stdout,"INFO: Input sound very high level: reducing slightly.\n");
		fflush(stdout);
		gain = SHRLEVLIM/dz->maxival;
		shrink_normalise(ibuf,dz->insams[0],gain,maxat);
	}
	if(dz->param[SHR_RAND] > 0.0) {					//	Range for random-seg-dur variation
		seglen = dz->iparam[SHR_GAP];				//	Calculated fron difference of seglen and sndlen 
		sndlen = dz->insams[0];						//	When they reach their minima
		if(dz->param[SHR_CNTRCT] < 1.0) {
			shrinkage   = 1.0;
			contraction = 1.0;
			while(seglen > dz->iparam[SHR_MIN]) { 
				sndlen = (int)round((double)gpinsams * shrinkage) * chans;
				sndlen = max(sndlen,dblsplenlen);	//	Seg can't be shorter than 2 splices
				if(sndlen < dz->iparam[SHR_SMALL])
					sndlen = dz->iparam[SHR_SMALL];
				seglen = (int)round((double)gpinsegm * contraction) * chans;
				seglen = max(seglen,sndlen);		//	Can't overlap output segs
				contraction *= dz->param[SHR_CNTRCT];
				shrinkage   *= dz->param[SHR_INK];
			}
		}
		randrange = seglen - sndlen;
		if(randrange <= chans)
			dz->param[SHR_RAND] = 0.0;
	}
	memset((char *)obuf,0,dz->buflen * sizeof(float));
	memcpy((char *)obuf,(char *)ibuf,dz->insams[0] * sizeof(float));
	seglen = dz->iparam[SHR_GAP];
	sndlen = dz->insams[0];
	lastsndlen = sndlen;
	if(dz->mode == SHRM_TIMED)
		centre = dz->iparam[SHR_TIME];
	else
		centre = (gpinsams/2) * chans;
	if(dz->vflag[SHRNK_INVERT])	
		sndchanged = 1;		// Forces initial sound to be reversed
	else
		sndchanged = 0;
	outdur = 0.0;
	continuation = 0;
	contraction = dz->param[SHR_CNTRCT];					//	Initial values of contraction and shrinkage
	shrinkage   = dz->param[SHR_INK];
	for(;;) {
		if(sndchanged && dz->vflag[SHRNK_NORM]) {			//	Only do normalise, if seg is different from last
			thismaxval = getmaxval(obuf,seglen,&maxat);
			if(thismaxval > FLTERR) {
				if (thismaxval < dz->maxival) {
					gain = dz->maxival/thismaxval;
					shrink_normalise(obuf,seglen,gain,maxat);

				}
			} else {
				fprintf(stdout,"WARNING: Remaining sound too quiet to be normalised: Exiting early.\n");
				fflush(stdout);
				return FINISHED;
			}
		}
		if(dz->vflag[SHRNK_INVERT] && sndchanged)
			reverse_data(obuf,seglen,dz);
		if((exit_status = write_samps(obuf,seglen,dz))<0)	//	Write a segment
			return(exit_status);
		outdur += (double)(seglen/chans)/srate;				//	Assess total output length, and exit if limit exceeded
		if(outdur >= dz->param[SHR_DUR])
			break;
		sndchanged = 0;										//	Shrink sound and contract segment
		sndlen = (int)round((double)gpinsams * shrinkage) * chans;
		sndlen = max(sndlen,dblsplenlen);					//	Sound can't be shorter than 2 splices
		if(sndlen < dz->iparam[SHR_SMALL])					
			sndlen = dz->iparam[SHR_SMALL];					//	If there's a limiting sound size, shrink no further
															//	contract segment
		seglen = (int)round((double)gpinsegm * contraction) * chans;
		seglen = max(seglen,sndlen);						//	Can't overlap output segs
		if(seglen < dz->iparam[SHR_MIN]) {					//	If contracting, & segs reach a min size, stop contraction
			seglen = dz->iparam[SHR_MIN];					//	and set "continuation" flag.	
			continuation = 1;
		}													//	If no continuation set,segs reach min size when snap to snd minsize
		if(seglen == lastseglen) {							//	and can contract no further, so are same seglen.
			if(dz->param[SHR_CNTRCT] < 1.0 && !continuation) //  In this case,  Halt process.
				return FINISHED;							//	Segs can be same len if NOT contracting (SHR_CNTRCT = 1.0)		
		}
		if(seglen != sndlen && dz->param[SHR_RAND] > 0.0) {	//	If space between snds, and randomisation set
			val = (drand48() * 2.0) - 1.0;					//	Randomise output segment position
			val *= dz->param[SHR_RAND];
			offset = (int)round((randrange/chans) * val) * chans;
			seglen += offset;
		}
		if(sndlen != lastsndlen) {							//	Only recalculate sound output, if soundlength has changed
			sndchanged = 1;										
			memset((char *)obuf,0,dz->buflen * sizeof(float));
			memcpy((char *)obuf,(char *)ibuf,dz->insams[0] * sizeof(float));
			switch(dz->mode) {
			case(SHRM_START):
				spliceval = 1.0;
				startsplice = sndlen - splenlen;
				for(n=startsplice;n < sndlen;n+=chans) {		//	DNSPLICE
					spliceval = max(0.0,spliceval - splincr);
					for(m=0;m<chans;m++) 
						obuf[n+m] = (float)(obuf[n+m] * spliceval);
				}
				break;
			case(SHRM_END):
				spliceval = 0.0;
				halflen = ((sndlen/chans)/2) * chans;
				startseg = max(0,centre - halflen);
				endseg   = min(dz->insams[0],centre + halflen);
				for(k=0,n = startseg;k < splen;k++,n+=chans) {	//	UPSPLICE
					spliceval = min(1.0,spliceval + splincr);
					for(m=0;m<chans;m++) 
						obuf[n+m] = (float)(obuf[n+m] * spliceval);
				}
				spliceval = 0.0;
				for(k=0,n = endseg-chans;k < splen;k++,n-=chans) {	
					spliceval = min(1.0,spliceval + splincr);	//	DOWNSPLICE (bkwds)
					for(m=0;m<chans;m++) 
						obuf[n+m] = (float)(obuf[n+m] * spliceval);
				}
				for(m=startseg,n=0;m < endseg;m++,n++)			//	COPY SEG TO BUF START
					obuf[n] = obuf[m];
				break;
			case(SHRM_CENTRE):
				spliceval = 0.0;
				startseg = dz->insams[0] - sndlen;
				endseg   = dz->insams[0];
				spliceval = 0.0;
				for(k=0,n = startseg;k < splen;k++,n+=chans) {	//	UPSPLICE
					spliceval = min(1.0,spliceval + splincr);
					for(m=0;m<chans;m++) 
						obuf[n+m] = (float)(obuf[n+m] * spliceval);
				}
				for(m=startseg,n=0;m < endseg;m++,n++)			//	COPY SEG TO BUF START
					obuf[n] = obuf[m];
				break;
			case(SHRM_TIMED):
				spliceval = 0.0;
				halflen = ((sndlen/chans)/2) * chans;
				startseg = max(0,centre - halflen);
				endseg   = min(dz->insams[0],startseg + sndlen);
				for(k=0,n = startseg;k < splen;k++,n+=chans) {	//	UPSPLICE
					spliceval = min(1.0,spliceval + splincr);
					for(m=0;m<chans;m++) 
						obuf[n+m] = (float)(obuf[n+m] * spliceval);
				}
				spliceval = 0.0;
				for(k=0,n = endseg-chans;k < splen;k++,n-=chans) {
					spliceval = min(1.0,spliceval + splincr);	//	DOWNSPLICE (bkwds)
					for(m=0;m<chans;m++) 
						obuf[n+m] = (float)(obuf[n+m] * spliceval);
				}
				for(m=startseg,n=0;m < endseg;m++,n++)			//	COPY SEG TO BUF START
					obuf[n] = obuf[m];
				break;
			}
			remnant = dz->buflen - n;							//	Zero remainder of buffer
			memset((char *)(obuf + n),0,remnant * sizeof(float));
			lastsndlen = sndlen;								//	Remember last sound length
		}
		lastseglen = seglen;									//	Remember last sound-separation
		contraction *= dz->param[SHR_CNTRCT];
		shrinkage   *= dz->param[SHR_INK];
	}
	return FINISHED;
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

/****************************** CREATE_SHRINK_SNDBUFS *********************************/

int create_shrink_sndbufs(dataptr dz)
{
	int n;
	int bigbufsize, twopow, framesize;
	if(dz->mode == SHRM_FINDMX || dz->mode == SHRM_LISTMX) {	//	Create 4 sec buffer (larger than any syllable which is a power of 2 in size)
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

		bigbufsize = (int)ceil(4 * dz->infile->srate) * dz->infile->channels;
		twopow = 2;
		while(twopow < bigbufsize)
			twopow *= 2;
		bigbufsize = twopow;
		dz->buflen = bigbufsize;	
		if((dz->bigbuf = (float *)calloc(dz->buflen * dz->bufcnt,sizeof(float))) == NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
			return(PROGRAM_ERROR);
		}
		for(n=0;n<dz->bufcnt;n++)
			dz->sbufptr[n] = dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
		dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
	} else {
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
		dz->buflen = (int)round(dz->param[SHR_GAP] * dz->infile->srate) * dz->infile->channels;
		dz->buflen = (dz->buflen / framesize) * framesize;
		dz->buflen += framesize;
		bigbufsize = dz->buflen * sizeof(float);
		if(bigbufsize <=0 || bigbufsize * 2 <=0) {
			sprintf(errstr,"Input sound too large for this process\n");
			return(DATA_ERROR);
		}
		if((dz->bigbuf = (float *)malloc(bigbufsize * dz->bufcnt)) == NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
			return(PROGRAM_ERROR);
		}
		for(n=0;n<dz->bufcnt;n++)
			dz->sbufptr[n] = dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
		dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
	}
	return(FINISHED);
}

/****************************** SHRINK_NORMALISE *********************************/

void shrink_normalise(float *buf,int len,double gain,int maxat)
{
	int n = 0;
	double val = fabs(buf[maxat]) * gain;
	if(val > SHRLEVLIM)
		gain = SHRLEVLIM/fabs(buf[maxat]);
	while( n < len) {
		buf[n] = (float)(buf[n] * gain);
		n++;
	}
}

/****************************** REVERSE_DATA *********************************/

void reverse_data(float *obuf,int seglen,dataptr dz)
{
	int chans = dz->infile->channels;
	float temp;
	int gplen  = seglen/chans, n, m, j;
	int centre = (gplen/2) * chans;
	for(n=0,m = seglen - chans; n<centre;n+=chans,m-=chans) {
		for(j=0;j<chans;j++) {
			temp      = obuf[n+j];
			obuf[n+j] = obuf[m+j];
			obuf[m+j] = temp;
		}
	}
}

/*********************** SHRINK_PARAM_PREPROCESS *********************/

int shrink_param_preprocess(dataptr dz)
{
	int exit_status, chans = dz->infile->channels;
	int maxlen, cnt, len;
	int envcnt, trofcnt, orig_env_size, n, j, k, unadjusted_envwindow_sampsize;
	int *peaks, *trofs;
	float *buf = dz->sampbuf[0];
	double maxsamp, thisval, srate = (double)dz->infile->srate;
			// Trim envelopesize to fit exactly into buffers (which are a pow of two in length)
	if(dz->mode == SHRM_FINDMX || dz->mode == SHRM_LISTMX) {
		unadjusted_envwindow_sampsize = round(dz->param[SHR_WSIZE] * MS_TO_SECS * srate) * chans;
		if(unadjusted_envwindow_sampsize < dz->buflen) {
			k = dz->buflen;
			while(unadjusted_envwindow_sampsize<k)
				k /= 2;
			j = k * 2;
			if(j - unadjusted_envwindow_sampsize > unadjusted_envwindow_sampsize - k)
				dz->iparam[SHR_WSIZE] = (int)k;
			else
				dz->iparam[SHR_WSIZE] = (int)j;
		} else {
			k = round((double)unadjusted_envwindow_sampsize/(double)dz->buflen);
			dz->iparam[SHR_WSIZE] = (int)(dz->buflen * k);
		}
	}				// Get and store maximum source sample, if required
	fprintf(stdout,"INFO: Finding maximum sample.\n");
	fflush(stdout);
	buf = dz->sampbuf[0];
	if((exit_status = read_samps(buf,dz))<0)
		return(exit_status);
	maxsamp = 0.0;
	while(dz->ssampsread > 0) {
		n = 0;
		while(n < dz->ssampsread) {
			thisval = fabs(buf[n]);
			if(thisval > maxsamp)
				maxsamp = thisval;
			n++;
		}
		if((exit_status = read_samps(buf,dz))<0)
			return(exit_status);
	}
	if(maxsamp <= FLTERR) {
		sprintf(errstr,"NO SIGNIFICANT SIGNAL FOUND IN SOURCE\n");
		return(DATA_ERROR);
	}
	if(dz->mode == SHRM_FINDMX || dz->mode == SHRM_LISTMX) {
		if(dz->param[SHR_GATE] > 0.0)
			dz->param[SHR_GATE] *= maxsamp;
	}
	dz->maxival = maxsamp;
	if((sndseekEx(dz->ifd[0],0,0)) < 0) {
		sprintf(errstr,"sndseek() 1 failed.\n");
		return(SYSTEM_ERROR);
	}
	reset_filedata_counters(dz);
	if(sloom)
		display_virtual_time(dz->total_samps_read,dz);		

	if(dz->mode < SHRM_FINDMX)
		return FINISHED;

	for(n=0;n<dz->larray_cnt;n++)
		dz->lparray[n] = NULL;
	if((exit_status = extract_env_from_sndfile(&envcnt,dz))<0)	//	Data to env, also Copies data to origenv
		return(exit_status);
	orig_env_size = envcnt;
	if(dz->mode == SHRM_FINDMX) {
		extract_peaks_from_envelope(&envcnt,dz);				// env reduced to PEAKS as time-val pairs
		if(envcnt == 0) {
			sprintf(errstr,"No peaks found. Silent file.\n");
			return(DATA_ERROR);
		}
	} else {													//	Inpput peaktimes copied to env, with dummy vals 0.0 (no longer needed)
		if(dz->itemcnt > orig_env_size) {
			sprintf(errstr,"No of peaks specified exceeds number of peaks found in file: reduce window-size or no of peak.\n");
			return DATA_ERROR;
		}
		if((exit_status = copy_peaktimes_to_env_array(dz))<0)
			return(exit_status);
		envcnt = dz->itemcnt;
	}
	extract_trofs_from_envelope(&trofcnt,orig_env_size,dz);	// TROFS to 'origenv' as time-val pairs
	if(dz->mode == SHRM_LISTMX) {
		if(trofcnt < dz->itemcnt) {
			sprintf(errstr,"Found too few troughs (%d) for no of peaks specified (%d): decrease windowsize.\n",trofcnt,dz->itemcnt);
			return(DATA_ERROR);
		}
	}	
	if(trofcnt == 0) {
		sprintf(errstr,"No troughs found.\n");
		return(PROGRAM_ERROR);
	}
	if(dz->mode == SHRM_FINDMX && dz->param[SHR_GATE] > 0.0) {
		if((exit_status = eliminate_too_low_peaks(&envcnt,&trofcnt,dz))<0)
			return(exit_status);
	}
	if((dz->lparray[LOCAL_PK_AT] = (int *)malloc(dz->buflen * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for internal long arrays.\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[LOCAL_PK_VAL] = (double *)malloc(dz->buflen * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for internal double arrays.\n");
		return(MEMORY_ERROR);
	}
	if((exit_status = eliminate_redundant_trofs(&envcnt,&trofcnt,orig_env_size,dz))<0)
		return(exit_status);
	if((sndseekEx(dz->ifd[0],0,0)) < 0) {
		sprintf(errstr,"sndseek() 2 failed\n");
		return(SYSTEM_ERROR);
	}
	if((dz->lparray[SHR_PEAKS]  = (int *)malloc(envcnt * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for array to store peaktimes.\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[SHR_INVTGAPS]  = (double *)malloc(envcnt * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for array to store inverse-time info.\n");
		return(MEMORY_ERROR);
	}
	if((dz->lparray[SHR_TROFS]  = (int *)malloc(trofcnt * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for array to store troughtimes.\n");
		return(MEMORY_ERROR);
	}
	peaks = dz->lparray[SHR_PEAKS];
	trofs = dz->lparray[SHR_TROFS];
	dz->itemcnt  = envcnt;
	dz->ringsize = trofcnt;

	if(dz->mode == SHRM_FINDMX) {
		if((sndseekEx(dz->ifd[0],0,0)) < 0) {
			sprintf(errstr,"sndseek() 3 failed.\n");
			return(SYSTEM_ERROR);
		}
		reset_filedata_counters(dz);
		if(sloom)
			display_virtual_time(dz->total_samps_read,dz);		
		if((exit_status = find_exact_peaktimes(envcnt,peaks,dz))<0)
			return(exit_status);
	} else {
		if((exit_status = copy_peaktimes(dz))<0)
			return(exit_status);
	}
	if((sndseekEx(dz->ifd[0],0,0)) < 0) {
		sprintf(errstr,"sndseek() 4 failed.\n");
		return(SYSTEM_ERROR);
	}
	reset_filedata_counters(dz);
	if(sloom)
		display_virtual_time(dz->total_samps_read,dz);		

	if((exit_status = find_exact_troftimes(trofcnt,trofs,dz))<0)
		return(exit_status);
	if((exit_status = test_buffers(envcnt,trofcnt,dz))<0)
		return(exit_status);
	if((sndseekEx(dz->ifd[0],0,0)) < 0) {
		sprintf(errstr,"sndseek() 5 failed.\n");
		return(SYSTEM_ERROR);
	}
	if(dz->iparam[SHR_LEN] > 0) {
		maxlen = 0;
		cnt = 0;
		for(n=1;n<envcnt;n++) {
			len = peaks[n] - peaks[n-1];
			maxlen = max(len,maxlen);
			if(len < dz->iparam[SHR_LEN])
				cnt++;
		}
		if(maxlen < dz->iparam[SHR_LEN]) {
			sprintf(errstr,"Minimum length setting is longer than all segments in file.\n");
			return(DATA_ERROR);
		}
		if(cnt) {
			fprintf(stdout,"INFO: Min length setting is longer than %d out of %d segments in file.\n",cnt,envcnt);
			fflush(stdout);
		}
	}
	reset_filedata_counters(dz);
	if(sloom)
		display_virtual_time(dz->total_samps_read,dz);		
	return FINISHED;
}

/************************* EXTRACT_PEAKS_FROM_ENVELOPE *******************************/

void extract_peaks_from_envelope(int *envcnt,dataptr dz)
{
	int orig_env_size = *envcnt, n, t=0, v=0;
	int islocalmax = 0;
	float lastenv = 0.0;
	int envloc = 0;
	float *env = dz->env;
	fprintf(stdout,"INFO: Finding peaks.\n");
	fflush(stdout);
	for(n=1;n < orig_env_size; n++) {
		t = n * 2;	// indexes time
		v = t + 1;	// indexes value
		if(env[v] <= lastenv) {
			if(islocalmax) {
				env[envloc] = env[t-2];		//	overwrites original envelope, with env peaks
				envloc++;
				env[envloc] = env[v-2];		//	overwrites original envelope, with env peaks
				envloc++;
			}
			islocalmax = 0;
		} else
			islocalmax = 1;
		lastenv = env[v];
	}
	if(islocalmax) {						//	Capture last peak, if a max
		env[envloc] = env[t-2];
		envloc++;
		env[envloc] = env[v-2];
		envloc++;
	}
	*envcnt = envloc/2;
}

/************************* EXTRACT_TROFS_FROM_ENVELOPE *******************************/

void extract_trofs_from_envelope(int *trofcnt,int orig_env_size,dataptr dz)
{
	int n, t=0, v=0;
	int islocalmin = 0;
	float lastenv = 0.0;
	int envloc = 0;
	float *env = dz->origenv;
	fprintf(stdout,"INFO: Finding troughs.\n");
	fflush(stdout);
	for(n=1;n < orig_env_size; n++) {
		t = n * 2;	// indexes time
		v = t + 1;	// indexes value
		if(env[v] > lastenv) {
			if(islocalmin) {
				env[envloc] = env[t-2];		//	overwrites original envelope, with env trofs
				envloc++;
				env[envloc] = env[v-2];		//	overwrites original envelope, with env trofs
				envloc++;
			}
			islocalmin = 0;
		} else
			islocalmin = 1;
		lastenv = env[v];
	}
	if(islocalmin) {						//	Capture last trof, if a min
		env[envloc] = env[t-2];
		envloc++;
		env[envloc] = env[v-2];
		envloc++;
	}
	*trofcnt = envloc/2;
}

/****************************** EXTRACT_ENV_FROM_SNDFILE ******************************/

int extract_env_from_sndfile(int *envcnt,dataptr dz)
{
	int exit_status, safety = 100;
	int n, bufcnt, totmem;
	double convertor = 1.0/F_ABSMAXSAMP;
	double time_convertor = 1.0/(dz->infile->channels * dz->infile->srate);
	float *envptr;
	float *buffer = dz->sampbuf[0];
	fprintf(stdout,"INFO: Finding envelope of source.\n");
	fflush(stdout);
	bufcnt = buffers_in_sndfile(dz->buflen,dz);
	*envcnt = windows_in_sndfile(dz);
	totmem = (*envcnt * 2) + safety;
	if((dz->env=(float *)calloc(totmem,sizeof(float)))==NULL) { // *2 -> accomodates time,val PAIRS
		sprintf(errstr,"INSUFFICIENT MEMORY for envelope array.\n");
		return(MEMORY_ERROR);
	}
	if((dz->origenv =(float *)calloc(totmem,sizeof(float)))==NULL) { // *2 -> accomodates time,val PAIRS
		sprintf(errstr,"INSUFFICIENT MEMORY for envelope array copy.\n");
		return(MEMORY_ERROR);
	}
	envptr = dz->env;
	for(n = 0; n < bufcnt; n++)	{
		if((exit_status = read_samps(dz->sampbuf[0],dz))<0)
			return(exit_status);
		if(sloom)
			display_virtual_time(dz->total_samps_read,dz);		
		getenv_of_buffer(dz->ssampsread,dz->iparam[SHR_WSIZE],convertor,time_convertor,&envptr,buffer,dz);
	}
	for(n=0;n<totmem;n++)
		dz->origenv[n] = dz->env[n];
	return(FINISHED);
}

/************************* GETENV_OF_BUFFER *******************************/

void getenv_of_buffer(int samps_to_process,int envwindow_sampsize,double convertor,double time_convertor,float **envptr,float *buffer,dataptr dz)
{
	int  start_samp = 0;
	int bufstart = dz->total_samps_read - dz->ssampsread;
	float *env = *envptr;
	while(samps_to_process >= envwindow_sampsize) {
		*env++ = (float)((start_samp + bufstart) * time_convertor);
		*env++ = (float)(getmaxsamp(start_samp,envwindow_sampsize,buffer) * convertor);
		start_samp  += envwindow_sampsize;
		samps_to_process -= envwindow_sampsize;
	}
	if(samps_to_process) {	/* Handle any final short buffer */
		*env++ = (float)((start_samp + bufstart) * time_convertor);
		*env++ = (float)(getmaxsamp(start_samp,samps_to_process,buffer) * convertor);
	}
	*envptr = env;
}

/*************************** GETMAXSAMP *******************************/

double getmaxsamp(int startsamp, int sampcnt,float *buffer)
{
	int  i, endsamp = startsamp + sampcnt;
	double thisval, thismaxsamp = 0.0;
	for(i = startsamp; i<endsamp; i++) {
		if((thisval =  fabs(buffer[i]))>thismaxsamp)		   
			thismaxsamp = thisval;
	}
	return thismaxsamp;
}

/************************* ELIMINATE_TOO_LOW_PEAKS *******************************
 *
 * This also eliminates any trofs between the previous peak and the eliminated peak
 */

int eliminate_too_low_peaks(int *envcnt,int *trofcnt,dataptr dz)
{
	int new_envcnt = *envcnt;
	int tabend = new_envcnt * 2;
	int envpos = 0, trofpos = 0, trofstart = 0, j, k, jj, gated, t, v, tt, vv, t1, v1, t2, v2, eliminate;
	double troftime = -1.0;
	float peaktime, mintroft, mintrofv;
	fprintf(stdout,"INFO: Gating low-level peaks.\n");
	fflush(stdout);
	while(envpos < new_envcnt) {
		t = envpos * 2;
		v = t + 1;
		peaktime = dz->env[v];
		trofstart = trofpos;
		tt = trofpos * 2;
		troftime = dz->origenv[tt];
		if(dz->env[v] < dz->param[SHR_GATE]) {
			while(troftime < peaktime) {
				trofpos++;
				tt = trofpos * 2;
				troftime = dz->origenv[tt];
			}											//	PURE SAFETY: SHOULD NOT BE MORE THAN 1 TROF BETWEEN 2 PEAKS!!!
						
			eliminate = (trofpos - trofstart) - 1;		//	Is there more than 1 trof between peaks,
			if(eliminate > 0) {							//	If so
				tt = trofstart * 2;
				vv = tt + 1;
				mintroft = dz->origenv[tt];				//	Find and store the minimum trof value
				mintrofv = dz->origenv[vv];
				for(k = trofstart+1; k < trofpos;k++) {
					tt = k * 2;
					vv = tt + 1;
					if(dz->origenv[vv] < mintrofv) {
						mintroft = dz->origenv[tt];
						mintrofv = dz->origenv[vv];
					}
				}										//	Eliminate all intervening trofs except 1
				for(k = trofstart+1, j = trofpos; j < *trofcnt;k++,j++) {
					t1 = k * 2;
					v1 = t1 + 1;
					t2 = j * 2;
					v2 = t2 + 1;
					dz->origenv[t1] = dz->origenv[t2];
					dz->origenv[v1] = dz->origenv[v2];
				}										//	And overwrite the other one with the mintrof data
				k = trofstart;
				t1 = k * 2;
				v1 = t1 + 1;
				dz->origenv[t1] = mintroft;
				dz->origenv[v1] = mintrofv;

				*trofcnt -= eliminate;					//	Reduce troftable size
			}
			new_envcnt--;
			tabend = new_envcnt * 2;					//	Eliminate peak
			jj = t;
			while(jj < tabend) {						//	Reduce peaktable size
				dz->env[jj] = dz->env[jj + 2];
				jj++;
			}
		} else {
			while(troftime < peaktime) {				//	If peak is OK, skip over trofs till we're beyond peak
				trofpos++;
				tt = trofpos * 2;
				troftime = dz->origenv[tt];
			}
			envpos++;
		}
	}
	if(new_envcnt == 0) {
		sprintf(errstr,"No peaks retained at this gate level.\n");
		return(DATA_ERROR);
	}
	if((gated = *envcnt - new_envcnt) > 0) {
		fprintf(stdout,"INFO: %d peaks removed by gate: %d peaks remain\n",gated,new_envcnt);
		fflush(stdout);
		*envcnt = new_envcnt;
	}
	return FINISHED;
}

/************************* ELIMINATE_REDUNDANT_TROFS *******************************/

int eliminate_redundant_trofs(int *envcnt,int *trofcnt,int orig_env_size,dataptr dz)
{
	int exit_status, troflocalcnt, start;
	int trofno, peakno, start_of_trofs, lastrofstored = 0, total_trofs_stored;
	int n, t, v, cnt, k, j, t1, t2, v1, v2;
	float peaktime, lastpeaktime, troftime;
	fprintf(stdout,"INFO: Rationalising troughs.\n");
	fflush(stdout);
	if((dz->env[0] < dz->origenv[0]) && !flteq(dz->env[0],0.0)) {	//	If 1st peak before 1st trof but NOT at zero
		for(n = *trofcnt; n > 0;n--)	{							//	Insert trof at zero
			t = n * 2;
			v  = t + 1;
			dz->origenv[t] = dz->origenv[t-2];
			dz->origenv[v]  = dz->origenv[v-2];
		}
		dz->origenv[0] = 0.0f;
		dz->origenv[1] = 0.0f;
		(*trofcnt)++;
	}
	peaktime = 0.0;
	if(flteq(dz->env[0],0.0))
		start = 1;
	else
		start = 0;
	trofno = 0;
	for(peakno = start; peakno < *envcnt;peakno++) {
		lastpeaktime  = peaktime;
		t = peakno * 2;
		peaktime = dz->env[t];
		start_of_trofs = trofno;
		t = trofno * 2;
		troftime = dz->origenv[t];
		troflocalcnt = 0;
		if(troftime >= peaktime) {
			if((exit_status =insert_trof(trofcnt,trofno,&troftime,peaktime,lastpeaktime,orig_env_size,dz))<0)
				return exit_status;
		}
		while(troftime < peaktime) {
			troflocalcnt++;
			if(++trofno >= *trofcnt)
				break;
			t = trofno * 2;
			troftime = dz->origenv[t];
		}
		if((exit_status =shuffle_trofs(troflocalcnt,trofcnt,start_of_trofs,&trofno,peaktime,lastpeaktime,&lastrofstored,dz))<0)
			return exit_status;
	}
	if(*trofcnt == 0) {
		sprintf(errstr,"No troughs retained : programming problem.\n");
		return(PROGRAM_ERROR);
	}
	total_trofs_stored = lastrofstored + 1;
	if(total_trofs_stored < *trofcnt) {					//	Count any trofs remaining from orig-array with times before last peak
		n = total_trofs_stored;
		cnt = 0;
		while(dz->origenv[n*2] < peaktime) {
			cnt++;
			if(++n >= *trofcnt)
				break;
		}
		for(k = n,j = total_trofs_stored; k < *trofcnt;k++,j++) {
			t1 = j * 2;									//	Eliminate these extra trofs still in array before last peak
			v1 = t1 + 1;
			t2 = k * 2;
			v2 = t2 + 1;
			dz->origenv[t1] = dz->origenv[t2];
			dz->origenv[v1] = dz->origenv[v2];
		}
		*trofcnt -= cnt;
	}
	if(*trofcnt > total_trofs_stored)					//	If there are trofs AFTER the peak
		*trofcnt = total_trofs_stored + 1;				//	Keep first (only) (SAFETY, should be only 1)
	else if(!flteq(peaktime,dz->duration)) {			//	Else, if last peak is not at end of file
		t = (*trofcnt) * 2;								//	Insert a trof at end of file
		v = t + 1;
		dz->origenv[t] = (float)dz->duration;
		dz->origenv[v] = 0.0f;
		(*trofcnt)++;
	}
	return FINISHED;
}

/************************* SHUFFLE_TROFS *******************************/

int shuffle_trofs(int troflocalcnt,int *trofcnt,int start_of_trofs,int *trofno,float peaktime,float lastpeaktime,int *lastrofstored,dataptr dz)
{
	int k, j, t, v, t1, v1, t2, v2, maxtrofat;
	int allsame, eliminate;
	float maxtrof;
	switch(troflocalcnt) {
	case(0):													//	Should only happen if first peak at zero.
		if(!flteq(peaktime,0.0)) {								//	But if it happens elsewhere insert a trof if it does
			for(k = *trofcnt; k > *trofno;k--)	{				//	insert trof-place at current location.
				t = k * 2;
				v = t + 1;
				dz->origenv[t] = dz->origenv[t-2];
				dz->origenv[v] = dz->origenv[v-2];
			}
			t = k * 2;
			v = t + 1;
			dz->origenv[t] = (float)((peaktime - lastpeaktime)/2.0); //	Time halfweay between peaks on either side
			dz->origenv[v] = 0.0f;									 //	Trof value 0.0
			(*trofcnt)++;
		}
		*lastrofstored = start_of_trofs;
		break;
	case(1):		//	If trof falls between 2 peaks, that's OK
		*lastrofstored = start_of_trofs;
		break;
	case(2):		//	If 2 trofs falls between 2 peaks, 1 associated with left, other with right,peak
		if(*trofno > 0) {								
			t = (*trofno) * 2;
			v = t + 1;
			if(flteq(dz->origenv[t],dz->origenv[t-2])) {			//	BUT if trof same as last, eliminate
				for(k = *trofno; k < *trofcnt;k++)	{				
					t = k * 2;
					v = t + 1;
					dz->origenv[t-2] = dz->origenv[t];
					dz->origenv[v-2] = dz->origenv[v];
				}
				(*trofcnt)--;
			}
			*lastrofstored = start_of_trofs;
		} else
			*lastrofstored = start_of_trofs + 1;

		break;
	default:		//	If > 2 trofs eliminate higher trofs
		maxtrof = -1.0f;
		maxtrofat = -1;
		while(troflocalcnt > 2) {
			for(k = start_of_trofs; k < *trofno; k++) {	// Find maximum trof
				t = k * 2;
				v = t + 1;
				if(dz->origenv[v] > maxtrof)
					maxtrof = dz->origenv[v];
					maxtrofat = k;
			}
			if(maxtrofat < 0) {
				sprintf(errstr,"Programming Error in search in multitrof\n");
				return(PROGRAM_ERROR);
			}
			allsame = 1;							//	Are all trofs same ??
			for(k = start_of_trofs; k < *trofno; k++) {
				t = k * 2;
				v = t + 1;
				if(!flteq(dz->origenv[v],maxtrof)) {
					allsame = 0;
					break;
				}
			}
			if(allsame) {							//	If all same, eliminate all but 2 edge ones
				for(k = start_of_trofs+1,j = *trofno-1;j<*trofcnt; k++,j++) {
					t1 = k * 2;
					v1 = t1 + 1;
					t2 = j * 2;
					v2 = t2 + 1;
					dz->origenv[t1] = dz->origenv[t2];
					dz->origenv[v1] = dz->origenv[v2];
				}
				eliminate = troflocalcnt - 2;
				*trofcnt -= eliminate;
				*trofno  -= eliminate;
				troflocalcnt = 2;
			} else {							//	Else, eliminate loudest trof, and continue round loop
				for(k = maxtrofat+1;k < *trofcnt; k++)	{
					t = k * 2;
					v = t + 1;
					dz->origenv[t-2] = dz->origenv[t];
					dz->origenv[v-2] = dz->origenv[v];
				}
				(*trofcnt)--;
				(*trofno)--;
				troflocalcnt--;
			}
		}
		*lastrofstored = start_of_trofs + 1;
		break;
	}
	return FINISHED;
}

/************************* FIND_EXACT_PEAKTIMES *******************************/

int find_exact_peaktimes(int envcnt,int *peaks,dataptr dz)
{
	int exit_status;
	double maxsamp, val;
	int n, t, v, maxtime, samptime, searchend, ibufstart_in_file, start, end;
	float *env = dz->env;
	float *buf = dz->sampbuf[0];
	if((exit_status = read_samps(buf,dz))<0)
		return(exit_status);
	ibufstart_in_file = 0;	
	if(env[0] == 0.0) {
		peaks[0] = 0;
		start = 1;
	} else
		start = 0;
	if(flteq(env[envcnt - 1],dz->duration)) {
		peaks[envcnt - 1] = dz->insams[0];
		end = envcnt - 1;
	} else
		end = envcnt;
	for(n=start;n < end; n++) {
		t = n * 2;	// indexes time
		v = t + 1;	// indexes value
		samptime = (int)round(env[t] * (double)dz->infile->srate) * dz->infile->channels;
		if((searchend = samptime + dz->iparam[SHR_WSIZE]) >= dz->total_samps_read) {
			if(dz->ssampsread < dz->buflen) {
				samptime -= ibufstart_in_file;
				searchend = dz->ssampsread;
			} else {
				ibufstart_in_file = (samptime/F_SECSIZE) * F_SECSIZE;
				if((sndseekEx(dz->ifd[0],ibufstart_in_file,0)) < 0) {
					sprintf(errstr,"sndseek() 6 failed\n");
					return(SYSTEM_ERROR);
				}
				dz->total_samps_read = ibufstart_in_file;
				if((exit_status = read_samps(buf,dz))<0)
					return(exit_status);
				if(dz->ssampsread == 0)
					break;
				if(sloom)
					display_virtual_time(dz->total_samps_read,dz);		
				samptime -= ibufstart_in_file;
				searchend = min(samptime + dz->iparam[SHR_WSIZE],dz->ssampsread);
			}
		} else {
			samptime  -= ibufstart_in_file;
			searchend -= ibufstart_in_file;
		}
		maxtime = 0;
		maxsamp = fabs(buf[samptime++]);
		while(samptime < searchend) {
			val = fabs(buf[samptime]);
			if(val > maxsamp) {
				maxtime = samptime;
				maxsamp = val;
			}
			samptime++;
		}
		maxtime += ibufstart_in_file;
		maxtime /= dz->infile->channels;	//	peaks must be at channel-group boundaries
		maxtime *= dz->infile->channels;
		peaks[n] = maxtime;
	}
	return FINISHED;
}

/************************* FIND_EXACT_TROFTIMES ********************************/

int find_exact_troftimes(int trofcnt,int *trofs,dataptr dz)
{
	int exit_status, chans = dz->infile->channels;
	double peaktime, lastpeaktime, srate = (double)dz->infile->srate;
	int n, t, v, p, searchstt, searchend, ibufstart_in_file, start, end, minpeakat, troftime;
	int searchlen;
	float *env = dz->origenv, minpeak;
	int *peaks =  dz->lparray[SHR_PEAKS];
	float *buf = dz->sampbuf[0];
	if((exit_status = read_samps(buf,dz))<0)
		return(exit_status);
	ibufstart_in_file = 0;	
	if(env[0] == 0.0) {
		trofs[0] = 0;
		start = 1;
	} else
		start = 0;
	if(flteq(env[trofcnt - 1],dz->duration)) {
		trofs[trofcnt - 1] = dz->insams[0];
		end = trofcnt - 1;
	} else
		end = trofcnt;
	for(n=start;n < end; n++) {
		t = n * 2;	// indexes time
		v = t + 1;	// indexes value
		troftime = (int)round(env[t] * (double)dz->infile->srate) * dz->infile->channels;
		searchstt = troftime/dz->iparam[SHR_WSIZE];			//	Locate window in which trough occurred
		searchstt *= dz->iparam[SHR_WSIZE];					//	Set search start to start of window
		searchend = min(dz->insams[0],searchstt + dz->iparam[SHR_WSIZE]);	//	and end to window end
		for(p = 0;p < dz->itemcnt;p++) {
			if(peaks[p] > troftime)							//	Find times of enclosing peaks
				break;
		}
		if(p <dz->itemcnt)								
			peaktime = (peaks[p]/chans)/srate;
		else
			peaktime = dz->duration;
		if(p > 0)
			lastpeaktime = (peaks[p-1]/chans)/srate;
		else
			lastpeaktime = 0.0;

		if(dz->mode == SHRM_LISTMX) {						// If these are specified (not found) peaks
			if(searchend > peaks[p])						//	and searchend is beyond next peak, 
				searchend = peaks[p];						//	move search end to peak-position
			if(p > 0) {										//	Check if peak which precedes trough is after search start
				if(peaks[p-1] > searchstt)					//	and if so, move search start to peak-position
					searchstt = peaks[p-1];
			}
		}
		searchlen = searchend - searchstt;
		if((exit_status = find_local_minpeak(&minpeakat,&minpeak,searchstt,searchend,searchlen,lastpeaktime,peaktime,dz))< 0)
			return exit_status;
		trofs[n] = minpeakat;
	}
	return FINISHED;
}

/******************************** SHRINK2 ********************************/

int shrink2(dataptr dz)
{
	int exit_status, chans = dz->infile->channels, tooquiet = 0, at_contraction_end = 0, quiet, suppressed = 0;
	int shrink_started = 0;
	double spliceval = 0.0, splincr = 1.0/(double)dz->iparam[SHR_SPLEN], srate = (double)dz->infile->srate;
	int sndlen, lastsndlen = 0, evenly_sndlen = 0, inseglen, outseglen, centre, maxat = 0, n = 0, m, k, halflen;
	int splen = dz->iparam[SHR_SPLEN], splenlen, dblsplenlen, randrange = 0, offset;
	int trofno, peakno, extraspace, start = 0, end = 0, nextrof, nextpeak, sndwidth, halfsndwidth, centring, peakadj;
	int startsnd = 0, endsnd = 0, lastoutseglen = 0;
	double outdur, lastoutdur = 0.0, thismaxval, gain = 1.0, contraction, shrinkage, val;
	float *ibuf = dz->sampbuf[0], *obuf = dz->sampbuf[1];
	int *peaks = dz->lparray[SHR_PEAKS], *trofs = dz->lparray[SHR_TROFS], trofcnt = dz->ringsize, peakcnt = dz->itemcnt;
	double *invtgaps = dz->parray[SHR_INVTGAPS];
	char num[200], filename[400], temp[400], temp2[64];
	splenlen = splen * chans;
	dblsplenlen = ((splen * 2) + 1) * chans;
	outdur = 0.0;
	contraction = 1;								//	Initial values of contraction and shrinkage
	shrinkage   = 1;
	outdur = 0.0;
	trofno = 0;
	dz->insams[0] = find_outsamps(dz);	//	This sets up correct printout of progress-bar

	strcpy(filename,dz->outfilename);	//	 Open the mixfile (sndfile already open)
	sprintf(temp,"%d",peakcnt);
	strcat(filename,temp);
	if((dz->fp = fopen(filename,"w"))==NULL) {
		sprintf(errstr,"Cannot open output mixfile %s\n",filename);
		return(USER_ERROR);
	}
	for (peakno = 0; peakno < peakcnt;peakno++) {
		strcpy(filename,dz->outfilename);
		sprintf(num,"%d",peakno);
		strcat(filename,num);
		strcat(filename,".wav");
		extraspace = 0;
		if(peakno == 0 && peaks[0] == 0) {
			start = 0;
			end   = trofs[0];
		} else {
			while(trofs[trofno] < peaks[peakno]) {
				if(++trofno >= trofcnt) {		//	If reach end without finding trof AFTER peak
					start = trofs[trofno-1];	//	peak must be at end
					end   = peaks[peakno];		//	so peak is end of segment
					break;
				}
			}
			if(trofno < trofcnt) {				//	Otherwise use trofs on either side of peak
				end   = trofs[trofno];			//	To define INPUT segment size from which to cut sound
				start = trofs[trofno-1];
			}
		}
		if(trofno < trofcnt-1 && peakno < peakcnt - 1) {
			nextrof  = trofs[trofno + 1];		//	Where there are 2 trofs between peaks
			nextpeak = peaks[peakno + 1];		//	Note the gap between the trofs
			if(nextrof < nextpeak)				//	To add to the OUTPUT segment length	
				extraspace = trofs[trofno + 1] - trofs[trofno];
		}
		inseglen = end - start;
		memset((char *)obuf,0,dz->buflen * sizeof(float));

		if(inseglen < dblsplenlen) {								//	If input segment too short
			fprintf(stdout,"WARNING: Input found-segment %d too short\n",peakno+1);
			fflush(stdout);
			continue;												// skip to next segment
		}
		sndseekEx(dz->ifd[0],start,0);
		if((exit_status = read_samps(ibuf,dz))<0)
			return(exit_status);
		memcpy((char *)obuf,(char *)ibuf,dz->ssampsread * sizeof(float));

		if(outdur > dz->param[SHR_AFTER]) {							//	If shrinkage is about to start or has started
			if(dz->vflag[SHRNK_EVENLY]) {							//	If shrinking evenly
				if(!shrink_started) {								//	If even-shrinking about to start
					evenly_sndlen = (int)round((double)inseglen/chans * shrinkage) * chans;	//	Get first shrunksnd len from seglen
					if(dz->iparam[SHR_LEN] > 0) {					//	If there's a min length for first shrunk
						if(dz->iparam[SHR_LEN] > evenly_sndlen)		//	and this is greater than  calcd length,
							evenly_sndlen = dz->iparam[SHR_LEN];	//	increase sndlen to min length
					}
					if(evenly_sndlen > inseglen) {					//	IF this sndlength is TOO BIG for the segment
						shrink_started = 0;
						sndlen = inseglen;
					} else {										//	Else set sndlen to newly calcd length
						shrink_started = 1;
						sndlen = evenly_sndlen;
					}
				} else {											//	IF even-shrinkage already started
					evenly_sndlen = (int)round((double)evenly_sndlen/chans * shrinkage) * chans;
					sndlen = evenly_sndlen;							//	calc shrink from previous (theoretical) sndlen
					if(sndlen > inseglen)							//	But it can't be greater than inseglen
						sndlen = inseglen;
				}													//	If not shrinking evenly but shrinkage has started
			} else													//	Calculate sndlen from seglen
				sndlen = (int)round((double)inseglen/chans * shrinkage) * chans;
			if(sndlen < dz->iparam[SHR_SMALL]) {
				if(dz->iparam[SHR_SMALL] <= inseglen)
					sndlen = dz->iparam[SHR_SMALL];					//	If there's a limiting sound size, stick at limiting size
				else
					sndlen = inseglen;								//	unless that's not possible (inseg too small)
			}
		} else														//	If shrinkage not started, sndlen = inseglen
			sndlen = inseglen;
		sndlen = max(sndlen,dblsplenlen);							//	Sound can't be shorter than 2 splices
		switch(dz->mode) {
		case(SHRM_FINDMX):
			centre = ((inseglen/chans)/2)*chans;					//	Actual centre of input data
			sndwidth = sndlen/chans;								//	Width of snd to be cut
			halfsndwidth = sndwidth/2;
			centring = (int)round(dz->param[SHR_SKEW] * sndwidth);
			peakadj = (centring - halfsndwidth) * chans;
			if(peakadj < 0) {										//	Snd is cut with centre at start, middle or end (etc) of src
				startsnd = max(0,centre + peakadj);					//	IF there is space ....
				endsnd   = startsnd + sndlen;
				if(endsnd >= inseglen) {
					endsnd = inseglen;
					startsnd = endsnd - sndlen;
				}
			} else if(peakadj > 0) {								//	IF not, adjust appropriately
				endsnd   = min(inseglen,centre + peakadj);
				startsnd = endsnd - sndlen;
				if(startsnd < 0) {
					startsnd = 0;
					endsnd = startsnd + sndlen;
				}
			}
			if(endsnd - startsnd < dblsplenlen) {
				sprintf(errstr,"Error in sound length calculation.\n");
				return(PROGRAM_ERROR);
			}
			break;
		case(SHRM_LISTMX):
			centre = peaks[peakno] - start;
			halflen = ((sndlen/chans)/2) * chans;
			startsnd = centre - halflen;
			if(startsnd < 0)
				startsnd = 0;
			endsnd = startsnd + sndlen;
			endsnd = min(endsnd,inseglen);
			break;
		}														//	If shrinking evenly
		if(dz->vflag[SHRNK_EVENLY] && outdur > dz->param[SHR_AFTER])	//	Calc outseglen from previous
			outseglen = (int)round((double)(lastoutseglen/chans) * shrinkage) * chans;
		else {															//	Else, 
			outseglen = inseglen + extraspace;							//	outseglen includes any extra space between snds
			if(outdur > dz->param[SHR_AFTER])							//	Contract all this
				outseglen = ((int)round((double)(outseglen/chans) * contraction) * chans);
		}
		outseglen = max(outseglen,sndlen);				 //	Can't overlap output segs
		if(outseglen <= dz->iparam[SHR_MIN])	{		 //	If contracting, & segs reach a min size, stop contraction
			outseglen = dz->iparam[SHR_MIN];
			at_contraction_end = 1;
		}
		if(dz->param[SHR_RAND] > 0.0) {					 //	If randomisation set, randomise output segment position
			randrange  = ((outseglen - sndlen)/2)/chans; //	If there is any space
			if(randrange > 1) {
				val = (drand48() * 2.0) - 1.0;
				val *= dz->param[SHR_RAND];
				offset = (int)round(randrange * val) * chans;
				outseglen += offset;
			}
		}
		spliceval = 0.0;
		for(k=0,n = startsnd;k < splen;k++,n+=chans) {	//	Upsplice
			spliceval = min(1.0,spliceval + splincr);
			for(m=0;m<chans;m++) 
				obuf[n+m] = (float)(obuf[n+m] * spliceval);
		}
		spliceval = 0.0;
		for(k=0,n = endsnd-chans;k < splen;k++,n-=chans) {	
			spliceval = min(1.0,spliceval + splincr);	//	Downsplice (bkwds)
			for(m=0;m<chans;m++) 
				obuf[n+m] = (float)(obuf[n+m] * spliceval);
		}
		if(startsnd > 0) {
			for(m=startsnd,n=0;m < endsnd;m++,n++)		//	Copy seg to buf start
				obuf[n] = obuf[m];
			endsnd = n;
		}
		for(n = endsnd;n < outseglen;n++)				//	Zero remainder of output
			obuf[n] = 0.0f;

		quiet = 0;
		if(dz->vflag[SHRNK_NORM]) {
			thismaxval = getmaxval(obuf,sndlen,&maxat);
			if(thismaxval > MINUS60DB) {
				if (thismaxval < dz->maxival) {
					gain = dz->maxival/thismaxval;
					shrink_normalise(obuf,sndlen,gain,maxat);
				}
			} else {
				tooquiet++;
				quiet = 1;
			}
		}
		if(dz->vflag[SHR_SUPPRESS] && at_contraction_end && quiet) {
			tooquiet--;
			suppressed++;
			continue;
		}
		if(dz->vflag[SHRNK_INVERT])
			reverse_data(obuf,outseglen,dz);
		if(peakno > 0) {
			if(sndcloseEx(dz->ofd) < 0) {
				sprintf(errstr,"Failed to close output file %d\n",peakno+1);
				return(SYSTEM_ERROR);
			}
			dz->ofd = -1;
			if((exit_status = create_sized_outfile(filename,dz))<0)
				return(exit_status);
			sndseekEx(dz->ifd[0],0,0);
			reset_filedata_counters(dz);
		}
		if((exit_status = write_samps(obuf,outseglen,dz))<0)	//	Write a segment
			return(exit_status);
		lastoutseglen = outseglen;
		strcpy(temp,filename);			//	filename
		sprintf(temp2," %lf",outdur);	//	time
		strcat(temp,temp2);		
		sprintf(temp2," %d",chans);		//	channel count
		strcat(temp,temp2);		
		strcat(temp," 1.0\n");			//	gain 1.0
		if(fputs(temp,dz->fp) < 0) {
			sprintf(errstr,"Error writing to output mixfile\n");
			return(PROGRAM_ERROR);
		}
		if(dz->vflag[SHRNK_INVERT])
			lastoutdur = outdur;
		outdur += (double)(outseglen/chans)/srate;				//	Get output length
		if(dz->vflag[SHRNK_INVERT] && peakno > 0)
			invtgaps[peakno-1] = (((double)(sndlen - lastsndlen)/chans)/srate) + lastoutdur;
		if(outdur > dz->param[SHR_AFTER]) {
			contraction *= dz->param[SHR_CNTRCT];
			shrinkage   *= dz->param[SHR_INK];
		}
		if(dz->vflag[SHRNK_INVERT])
			lastsndlen = sndlen;
	}
	fclose(dz->fp);
	if(dz->vflag[SHRNK_INVERT]) {
		strcpy(filename,dz->outfilename);	//	 Open the inverse time-gaps listing file
		sprintf(temp,"%d",peakcnt+1);
		strcat(filename,temp);
		if((dz->fp = fopen(filename,"w"))==NULL) {
			sprintf(errstr,"Cannot open output file %s\n",filename);
			return(USER_ERROR);
		}
		for (peakno = 0; peakno < peakcnt-1;peakno++) {
			sprintf(temp,"%lf\n",invtgaps[peakno]);
			if(fputs(temp,dz->fp) < 0) {
				sprintf(errstr,"Error writing to output mixfile\n");
				return(PROGRAM_ERROR);
			}
		}
		fclose(dz->fp);
	}
	if(tooquiet) {
		fprintf(stdout,"INFO: %d segments too quiet to be normalised.\n",tooquiet);
		fflush(stdout);
	}
	if(suppressed) {
		fprintf(stdout,"INFO: %d segments suppressed\n",suppressed);
		fflush(stdout);
	}
	return FINISHED;
}

/****************************** GETMAXVAL *********************************/

double getmaxval(float *buf,int len,int *maxat)
{
	int n = 0;
	double maxval = 0.0;
	*maxat = 0;
	while( n < len) {
		if(fabs(buf[n]) > maxval) {
			maxval = fabs(buf[n]);
			*maxat = n;
		}
		n++;
	}
	return maxval;
}

/****************************** TEST_BUFFERS *********************************
 *
 * Test length of found segments against assigned buffersize
 */

int test_buffers(int envcnt,int trofcnt,dataptr dz)
{
	int *peaks = dz->lparray[SHR_PEAKS], *trofs = dz->lparray[SHR_TROFS], n, m, segstart, segend, peakpos, seglen;
	int maxseglen = dz->buflen, twopow, bigbufsize, bigat = 0, totalcnt;
	double srate = (double)dz->infile->srate;
	int chans = dz->infile->channels;
	m = 0;
	if (peaks[0] < trofs[0])
		segstart = peaks[0];	//	If data starts with peak, this is start of first segment
	else						//	otherwise
		segstart = trofs[0];	//	Data starts at first trof
	totalcnt = segstart;
	for(n = 1; n < envcnt; n++) {
		peakpos = peaks[n];
		while(trofs[m] < peakpos) {	//	Move through trofs until cross peak
			if(++m >= trofcnt)
				break;
		}
		segend = trofs[m-1];		//	Segend is trof before peak
		seglen = segend - segstart;
		if(seglen > maxseglen) {
			maxseglen = seglen;
			bigat = totalcnt;
		}
		totalcnt += seglen;
		segstart = segend;
	}
	if(maxseglen > dz->buflen) {
		free(dz->bigbuf);
		twopow = 2;
		while(twopow < maxseglen)
			twopow *= 2;
		bigbufsize = twopow;
		dz->buflen = bigbufsize;	
		if((dz->bigbuf = (float *)calloc(dz->buflen * dz->bufcnt,sizeof(float))) == NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to store largest segment (at %lf).\n",(double)(bigat/chans)/srate);
			return(PROGRAM_ERROR);
		}
		for(n=0;n<dz->bufcnt;n++)
			dz->sbufptr[n] = dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
		dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
	}
	return FINISHED;
}

/****************************** BUFFERS_IN_SNDFILE ******************************/

int buffers_in_sndfile(int buffer_size,dataptr dz)
{
	int bufcnt;
	if(((bufcnt = dz->insams[0]/buffer_size)*buffer_size)!=dz->insams[0])
		bufcnt++;
	return(bufcnt);
}
/****************************** WINDOWS_IN_SNDFILE [GET_ENVSIZE] ******************************/

int windows_in_sndfile(dataptr dz)
{
	int envsize, winsize = dz->iparam[SHR_WSIZE];
	if(((envsize = dz->insams[0]/winsize) * winsize)!=dz->insams[0])
		envsize++;
	return(envsize);
}

/**************************************** FIND_OUTDUR *****************************************
 *
 *	Calcualte expected output duration for use in calculating proprtional vals to send 
 *	to sloom progress-bar.
 *
 *	This calculation ignores suppressed (too quiet) segments in output
 *	so generates a length val >= (rather than =) to the actual output duration-in-samples.
 *	This is OK for progress-bar display.
 */

int find_outsamps(dataptr dz)
{
	int chans = dz->infile->channels;
	double srate = (double)dz->infile->srate;
	int inseglen, outseglen, lastoutseglen = 0, sndlen, evenly_sndlen = 0;
	int splen = dz->iparam[SHR_SPLEN], splenlen, dblsplenlen, gpinsegm;
	int trofno, peakno, extraspace, start = 0, end = 0, nextrof, nextpeak;
	double outdur, contraction, shrinkage;
	int *peaks = dz->lparray[SHR_PEAKS], *trofs = dz->lparray[SHR_TROFS], trofcnt = dz->ringsize, peakcnt = dz->itemcnt;
	splenlen = splen * chans;
	dblsplenlen = ((splen * 2) + 1) * chans;
	gpinsegm = dz->iparam[SHR_GAP]/chans;			//	Initial length of segment, in sample-groups
	outdur = 0.0;
	contraction = 1;								//	Initial values of contraction and shrinkage
	shrinkage   = 1;
	outdur = 0.0;
	trofno = 0;
	for (peakno = 0; peakno < peakcnt;peakno++) {
		extraspace = 0;
		if(peakno == 0 && peaks[0] == 0) {
			start = 0;
			end   = trofs[0];
		} else {
			while(trofs[trofno] < peaks[peakno]) {
				if(++trofno >= trofcnt) {		//	If reach end without finding trof AFTER peak
					start = trofs[trofno-1];	//	peak must be at end
					end   = peaks[peakno];		//	so peak is end of segment
					break;
				}
			}
			if(trofno < trofcnt) {				//	Otherwise use trofs on either side of peak
				end   = trofs[trofno];			//	To define INPUT segment size from which to cut sound
				start = trofs[trofno-1];
			}
		}
		if(trofno < trofcnt-1 && peakno < peakcnt - 1) {
			nextrof  = trofs[trofno + 1];		//	Where there are 2 trofs between peaks
			nextpeak = peaks[peakno + 1];		//	Note the gap between the trofs
			if(nextrof < nextpeak)				//	To add to the OUTPUT segment length	
				extraspace = trofs[trofno + 1] - trofs[trofno];
		}
		inseglen = end - start;
		if(inseglen < dblsplenlen) {								//	If input segment too short
			outseglen = inseglen + extraspace;
			outdur += (double)(outseglen /chans)/srate;				//	Get output length
			if(outdur > dz->param[SHR_AFTER]) {
				contraction *= dz->param[SHR_CNTRCT];
				shrinkage   *= dz->param[SHR_INK];
			}
			continue;
		}

		if(outdur > dz->param[SHR_AFTER]) {							//	If shrinkage is about to start or has started
			if(dz->vflag[SHRNK_EVENLY]) {							//	If shrinking evenly
				if(evenly_sndlen == 0) {							//	If even-shrinking about to start
					evenly_sndlen = (int)round((double)inseglen/chans * shrinkage) * chans;	//	Get first shrunksnd len from seglen
					if(dz->iparam[SHR_LEN] > 0) {					//	If there's a min length for first shrunk
						if(dz->iparam[SHR_LEN] > evenly_sndlen)		//	and this is greater than  calcd length,
							evenly_sndlen = dz->iparam[SHR_LEN];	//	increase sndlen to min length
					}
					if(evenly_sndlen > inseglen) {					//	IF this sndlength is TOO BIG for the segment
						evenly_sndlen = 0;							//	Don't do shrinking yet
						sndlen = inseglen;
					} else											//	Else set sndlen to newly calcd length
						sndlen = evenly_sndlen;
				} else {											//	IF even-shrinkage already started
					evenly_sndlen = (int)round((double)evenly_sndlen/chans * shrinkage) * chans;
					sndlen = evenly_sndlen;							//	calc shrink from previous (theoretical) sndlen
					if(sndlen > inseglen) {							//	But it can't be greater than inseglen
						sndlen = inseglen;
					}
				}													//	If not shrinking evenly but shrinkage has started
			} else													//	Calculate sndlen from seglen
				sndlen = (int)round((double)inseglen/chans * shrinkage) * chans;
			if(sndlen < dz->iparam[SHR_SMALL] && dz->iparam[SHR_SMALL] >= inseglen)
				sndlen = dz->iparam[SHR_SMALL];						//	If there's a limiting sound size, shrink no further

		} else														//	If shrinkage not started, sndlen = inseglen
			sndlen = inseglen;

		sndlen = max(sndlen,dblsplenlen);						//	Sound can't be shorter than 2 splices
		if(outdur > dz->param[SHR_AFTER] && dz->vflag[SHRNK_EVENLY])	//	Calc outseglen from previous
			outseglen = (int)round((double)(lastoutseglen/chans) * shrinkage) * chans;
		else {															//	Else, outseglen includes any extra space between snds
			outseglen = inseglen + extraspace;							//	Contract all this
			outseglen = ((int)round((double)(outseglen/chans) * contraction) * chans);
		}
		if(outseglen < dz->iparam[SHR_MIN])
			outseglen = dz->iparam[SHR_MIN];
		outseglen = max(outseglen,sndlen);						//	Can't overlap output segs
		
		outdur += (double)(outseglen/chans)/srate;				//	Get output length
		if(outdur > dz->param[SHR_AFTER]) {
			contraction *= dz->param[SHR_CNTRCT];
			shrinkage   *= dz->param[SHR_INK];
		}
		lastoutseglen = outseglen;
	}
	return (int)ceil(outdur * srate) * chans;
}

/**************************** HANDLE_THE_SPECIAL_DATA ****************************/

int handle_the_special_data(char *str,dataptr dz)
{		
	double dummy = 0.0, lasttime = -1.0;
	double *q;
	FILE *fp;
	int cnt;
	char temp[200], *p;
	if((fp = fopen(str,"r"))==NULL) {
		sprintf(errstr,"Cannot open file %s to read peak times.\n",str);
		return(DATA_ERROR);
	}
	cnt = 0;
	while(fgets(temp,200,fp)!=NULL) {
		p = temp;
		if(*p == ';')	//	Allow comments in file
			continue;
		while(get_float_from_within_string(&p,&dummy)) {
			if(dummy < 0.0) {
				sprintf(errstr,"Time (%lf) less than zero in peak times file %s.\n",dummy,str);
				return(DATA_ERROR);
			} else if(dummy > dz->duration) {
				sprintf(errstr,"Time (%lf) beyond end of sound (%lf) in peak times file %s.\n",dummy,dz->duration,str);
				return(DATA_ERROR);
			} else if(dummy <= lasttime) {
				sprintf(errstr,"Times (%lf then %lf) do not increase in peak times file %s.\n",dummy,lasttime,str);
				return(DATA_ERROR);
			}
			lasttime = dummy;
			cnt++;
		}
	}
	if(cnt == 0) {
		sprintf(errstr,"No valid times in peak-times file %s.\n",str);
		return(DATA_ERROR);
	}
	dz->itemcnt = cnt;
	dz->larray_cnt = 1;
	if((dz->parray[SHR_INPUT_PKS] = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store packet times.\n");
		return(MEMORY_ERROR);
	}
	fseek(fp,0,0);
	q = dz->parray[SHR_INPUT_PKS];
	while(fgets(temp,200,fp)!=NULL) {
		p = temp;
		if(*p == ';')	//	Allow comments in file
			continue;
		while(get_float_from_within_string(&p,&dummy)) {
			*q = dummy;
			q++;
		}
	}
	if(fclose(fp)<0) {
		fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",str);
		fflush(stdout);
	}
	return(FINISHED);
}

/************************* COPY_PEAKTIMES_TO_ENV_ARRAY *******************************/

int copy_peaktimes_to_env_array(dataptr dz)
{
	int n, m;
	float  *env = dz->env;
	double *pks = dz->parray[SHR_INPUT_PKS];
	for(n=0,m=0;n < dz->itemcnt; n++,m+=2) {
		env[m] = (float)pks[n];
		env[m+1] = 0.0f;		//	Dummy zero vals for peaks: vals not needed at this stage
	}
	return FINISHED;
}

/************************* COPY_PEAKTIMES *******************************/

int copy_peaktimes(dataptr dz)
{
	int n;
	double *pks = dz->parray[SHR_INPUT_PKS];
	int *peaks = dz->lparray[SHR_PEAKS];
	for(n=0;n < dz->itemcnt; n++)
		peaks[n] = (int)round(pks[n] * (double)dz->infile->srate) * dz->infile->channels;
	return FINISHED;
}

/************************* INSERT_TROF *******************************/

int insert_trof(int *trofcnt,int trofno,float *troftime,float peaktime,float lastpeaktime,int orig_env_size,dataptr dz)
{
	int exit_status, chans = dz->infile->channels;
	double srate = (double)dz->infile->srate;
	int searchstt, searchend, searchlen, n, t, v;
	int minpeakat;
	float minpeak;

	(*trofcnt)++;
	if((*trofcnt) * 2 > orig_env_size) {
		sprintf(errstr,"Too few troughs found in sound: use smaller envelope.\n");
		return(DATA_ERROR);
	}
	for(n = *trofcnt; n>trofno;n--) {		//	Make space
		t = n * 2;
		v = t + 1;
		dz->origenv[t] = dz->origenv[t-2];
		dz->origenv[v] = dz->origenv[v-2];
	}
	t = trofno * 2;							//	point to space
	v = t + 1;

	searchstt = (int)round(lastpeaktime * srate) * chans;
	searchend   = (int)round(peaktime * srate) * chans;
	searchlen    = searchend - searchstt;

	if((exit_status = find_local_minpeak(&minpeakat,&minpeak,searchstt,searchend,searchlen,lastpeaktime,peaktime,dz))< 0)
		return exit_status;
	dz->origenv[t] = (float)((minpeakat/chans)/srate);
	dz->origenv[v] = minpeak;
	*troftime = dz->origenv[t];
	return FINISHED;
}

/************************* FIND_LOCAL_MINPEAK *******************************/

int find_local_minpeak(int *minpeakat,float *minpeak,int searchstt,int searchend,int searchlen, double lastpeaktime, double peaktime, dataptr dz)
{
	int exit_status, mergelast, chans = dz->infile->channels;
	double *local_peak = dz->parray[LOCAL_PK_VAL];
	double srate = dz->infile->srate, timegap, midtime;
	int *local_peak_at = dz->lparray[LOCAL_PK_AT];
	int wincnt, local_peak_cnt, maxat, n, bufsampcnt, sampcnt, covered, remnant, wsiz, midsamp;
	float maxsamp, *ibuf = dz->sampbuf[0];
	wsiz = TINY_WSIZE * chans;
	if(searchlen/wsiz < 3) {				//	If can't find a small enough window to search for min-peak
		timegap = peaktime - lastpeaktime;	//	Insert trof at midtime between peaks
		midtime = peaktime + timegap/2.0;
		midsamp = (int)round(midtime * srate) * chans;
		if(midsamp == searchstt || midsamp == searchend) {
			sprintf(errstr,"No space to insert trough between peaks at %lf and %lf.\n",lastpeaktime,peaktime);
			return(DATA_ERROR);
		}
		*minpeakat = midsamp;
		*minpeak   = 0.0f;
		return FINISHED;
	}
	mergelast = 0;
	wincnt = searchlen/wsiz;						//	Find number of small windows
	covered = wincnt * wsiz;
	remnant = searchlen - covered;					//	Find any short-window (remnant) at end
	if(remnant < wsiz/2)							//	If remnant is less than half-windowsize
		mergelast = 1;								//	treat it as part of previous window
	if((sndseekEx(dz->ifd[0],searchstt,0)) < 0) {
		sprintf(errstr,"sndseek() 8 failed.\n");
		return(SYSTEM_ERROR);
	}
	sampcnt = 0;
	local_peak_cnt = 0;
	wincnt  = 0;
	maxsamp = 0.0f;
	maxat   = searchstt;
	dz->total_samps_read = 0;
	dz->ssampsread = 0;
	while(dz->total_samps_read < searchlen) {			//	Search for local peaks
		if((exit_status = read_samps(ibuf,dz))<0)
			return(exit_status);
		if(dz->ssampsread == 0)
			break;
		bufsampcnt = 0;
		while(sampcnt < searchlen) {
			if(fabs(ibuf[bufsampcnt]) > maxsamp) {
				maxsamp = (float)fabs(ibuf[bufsampcnt]);
				maxat   = sampcnt + searchstt;
			}
			if(++wincnt >= wsiz) {
				local_peak[local_peak_cnt]    = maxsamp;
				local_peak_at[local_peak_cnt] = maxat;
				local_peak_cnt++;
				wincnt = 0;
				maxsamp = (float)fabs(ibuf[bufsampcnt]);
			}
			sampcnt++;
			if(++bufsampcnt >= dz->ssampsread)
				break;
		}
	}
	if(local_peak_cnt == 0) {
		sprintf(errstr,"Failed to find trof between peaks at %lf and %lf\n",lastpeaktime,peaktime);
		return PROGRAM_ERROR;
	}
	if(mergelast && local_peak_cnt >= 2) {			// If short last window, merge it with previous window
		if(local_peak[local_peak_cnt-1] > local_peak[local_peak_cnt-2]) {
			local_peak[local_peak_cnt-2]    = local_peak[local_peak_cnt-1];
			local_peak_at[local_peak_cnt-2] = local_peak_at[local_peak_cnt-1];
		}
		local_peak_cnt--;
	}
	*minpeak   = (float)local_peak[0];					//	Find minimum local peak (= trof)
	*minpeakat = local_peak_at[0];
	for(n=1;n < local_peak_cnt; n++) {
		if(local_peak[n] < *minpeak) {
			*minpeak   = (float)local_peak[n];
			*minpeakat = local_peak_at[n];
		}
	}
	return FINISHED;
}
