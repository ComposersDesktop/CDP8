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
#include <pnames.h>
#include <extdcon.h>


#if defined unix || defined __GNUC__
#define round(x) lround((x))
#endif


#ifndef HUGE
#define HUGE 3.40282347e+38F
#endif

char errstr[2400];

#define infilespace  rampbrksize

int anal_infiles = 1;
int	sloom = 0;
int sloombatch = 0;

const char* cdp_version = "7.1.0";

//CDP LIB REPLACEMENTS
static int setup_iterline_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_iterline_param_ranges_and_defaults(dataptr dz);
static int handle_the_outfile(int *cmdlinecnt,char ***cmdline,dataptr dz);
static int handle_the_special_data(char *str,dataptr dz);
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
static int iterline(dataptr dz);
static int create_the_iterbufs(double maxpscat,dataptr dz);
static int do_iterate_preprocess(dataptr dz);
static int read_the_input_snds(double *maxinsamp,int passno,dataptr dz);
static int iter_shift_interp_stereo(int cnt,int passno, double *gain,double *pshift,int local_write_start,
		double *maxoutsamp,int pstep,int iterating,double thistrans,dataptr dz);
static int fixa_iter_shift_interp_stereo(int cnt,int passno,double *gain,double *pshift,int local_write_start,
		double *maxoutsamp,int pstep,int iterating,double thistrans,dataptr dz);
static int read_interpd_transposition_value(double thistime,double *thistrans,dataptr dz);
static int read_stepd_transposition_value(double thistime,double *thistrans,dataptr dz);
static int get_next_writestart(int write_start,dataptr dz);
static int iterate(int cnt,int pass,double *gain,double *pshift,int write_end,int local_write_start,
			double *maxoutsamp,int pstep,int iterating,double thistrans,dataptr dz);
static double get_gain(dataptr dz);
static double get_pshift(dataptr dz);
static void choose_the_transposed_snd(double *thistrans,int *filindx);



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
		if((exit_status = setup_iterline_application(dz))<0) {
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
	if((exit_status = setup_iterline_param_ranges_and_defaults(dz))<0) {
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
	if((exit_status = handle_extra_infiles(&cmdline,&cmdlinecnt,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);		
		return(FAILED);
	}
	if(dz->infilecnt != 25) {
		sprintf(errstr,"Process requires 25 input files.\n");
		return(USER_ERROR);
	}
	// handle_outfile() = 
	if((exit_status = handle_the_outfile(&cmdlinecnt,&cmdline,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	dz->array_cnt = 1;
	if((dz->parray = (double **)malloc(dz->array_cnt * sizeof(double *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create segment lengths arrays.\n");
		return(MEMORY_ERROR);
	}
//	handle_formants()			redundant
//	handle_formant_quiksearch()	redundant
	if((exit_status = handle_the_special_data(cmdline[0],dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	cmdlinecnt--;
	cmdline++;
	if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {		// CDP LIB
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
//	check_param_validity_and_consistency() redundant
	is_launched = TRUE;
	dz->bufcnt = 28;
	if((dz->sampbuf = (float **)malloc(sizeof(float *) * (dz->bufcnt+1)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffers.\n");
		return(MEMORY_ERROR);
	}
	if((dz->sbufptr = (float **)malloc(sizeof(float *) * dz->bufcnt+1))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffer pointers.\n");
		return(MEMORY_ERROR);
	}
	for(n = 0;n <dz->bufcnt; n++)
		dz->sampbuf[n] = dz->sbufptr[n] = (float *)0;
	dz->sampbuf[n] = (float *)0;

	if((exit_status = do_iterate_preprocess(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	//spec_process_file =
	if((exit_status = iterline(dz))<0) {
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

/************************* SETUP_ITERLINE_APPLICATION *******************/

int setup_iterline_application(dataptr dz)
{
	int exit_status;
	aplptr ap;
	if((exit_status = establish_application(dz))<0)		// GLOBAL
		return(FAILED);
	ap = dz->application;
	if((exit_status = set_param_data(ap,ITERTRANS,8,7,"dDDDD0di"))<0)
		return(FAILED);
	if((exit_status = set_vflgs(ap,"",0,"","n",1,0,"0"))<0)
		return(FAILED);
	// set_legal_infile_structure -->
	dz->has_otherfile = FALSE;
	// assign_process_logic -->
	dz->input_data_type = MANY_SNDFILES;
	dz->process_type	= UNEQUAL_SNDFILE;	
	dz->outfiletype  	= SNDFILE_OUT;
	dz->maxmode = 2;
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

/************************* SETUP_ITERLINE_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_iterline_param_ranges_and_defaults(dataptr dz)
{
	int exit_status;
	aplptr ap = dz->application;
	// set_param_ranges()
	ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
	// NB total_input_param_cnt is > 0 !!!
	if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
		return(FAILED);
	// get_param_ranges()
	ap->lo[ITER_DUR] 	= dz->duration;
	ap->hi[ITER_DUR] 	= BIG_TIME;
	ap->default_val[ITER_DUR]		= dz->duration * 2.0; 	
	ap->lo[ITER_DELAY]	= FLTERR;
	ap->hi[ITER_DELAY]	= ITER_MAX_DELAY;
	ap->default_val[ITER_DELAY]  	= dz->duration;
	ap->lo[ITER_RANDOM]	= 0.0;
	ap->hi[ITER_RANDOM]	= 1.0;
	ap->default_val[ITER_RANDOM] 	= 0.0;
	ap->lo[ITER_PSCAT]	= 0.0;
	ap->hi[ITER_PSCAT]	= ITER_MAXPSHIFT;	
	ap->default_val[ITER_PSCAT]  	= 0.0;
	ap->lo[ITER_ASCAT]	= 0.0;
	ap->hi[ITER_ASCAT]	= 1.0;
	ap->default_val[ITER_ASCAT]  	= 0.0;
	ap->lo[ITER_GAIN]	= 0.0;
	ap->hi[ITER_GAIN]	= 1.0;
	ap->default_val[ITER_GAIN]   	= DEFAULT_ITER_GAIN; /* 0.0 */ 
	ap->lo[ITER_RSEED]	= 0.0;
	ap->hi[ITER_RSEED]	= MAXSHORT;
	ap->default_val[ITER_RSEED]  	= 0.0;
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
			if((exit_status = setup_iterline_application(dz))<0)
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
	int exit_status;
	exit_status = set_internalparam_data("diiii", ap);
	return(exit_status);
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
	usage2("iterlinef");
	return(USAGE_ONLY);
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	if(!strcmp(prog_identifier_from_cmdline,"iterlinef"))				dz->process = ITERLINEF;
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
	if(!strcmp(str,"iterlinef")) {
		fprintf(stdout,
	    "USAGE:\n"
	    "iterlinef iterlinef mode infile outfile tdata outduration\n"
		"     [-ddelay] [-rrand] [-ppshift] [-aampcut] [-ggain] [-sseed] [-n]\n"
		"\n"
		"ITERATE AN INPUT SOUND SET, FOLLOWING A TRANSPOSITION LINE.\n"
		"The input set must consist of 25 transpositions of a source\n"
		"at intervals of one semitone, in ascending order.\n"
		"Input sounds must be of approx equal duration.\n"
		"The central sound (file 13) is the reference sound for transpositions (see below)\n"
		"\n"
		"MODE 1: interpolate between transpositions.\n"
		"MODE 2: step between transpositions.\n"
		"\n"
		"TDATA   File of time-transposition pairs, transpositions in semitones.\n"
		"        Transpositions are relative to the central sound (file 13) of the set.\n"
		"DELAY   (average) delay between iterations.\n"
		"RAND    delaytime-randomisation: Range 0 - 1: Default 0\n"
		"PSHIFT  max of random pitchshift of each iter: Range 0 - %.0lf semitones\n"
		"        e.g.  2.5 =  2.5 semitones up or down.\n"
		"AMPCUT  max of random amp-reduction on each iter: Range 0-1: default 0\n"
		"GAIN    Overall Gain: Range 0 - 1:\n"
		"        Special val 0, produces maximum acceptable level.\n"
		"        (this is overridden by the \"-n\" normalisation flag - see below).\n"
		"SEED	 the same seed-number will produce identical output on rerun,\n"
		"        (Default: (0) random sequence is different every time).\n"
		"-n      Normalise output (max out level = max in level)> only with NON-zero seed.\n",ITER_MAXPSHIFT);
	} else
		fprintf(stdout,"Unknown option '%s'\n",str);
	return(USAGE_ONLY);
}

int usage3(char *str1,char *str2)
{
	fprintf(stderr,"Insufficient parameters on command line.\n");
	return(USAGE_ONLY);
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

/**************************** HANDLE_THE_SPECIAL_DATA ****************************/

int handle_the_special_data(char *str,dataptr dz)
{
	FILE *fp;
	double dummy, lasttime = 0.0;
	char temp[200], *p;
	int istime = 1;
	int cnt = 0;

	if((fp = fopen(str,"r"))==NULL) {
		sprintf(errstr,"Cannot open file %s to read transposition data.\n",str);
		return(DATA_ERROR);
	}
	while(fgets(temp,200,fp)!=NULL) {
		p = temp;
		while(isspace(*p))
			p++;
		if(*p == ';' || *p == ENDOFSTR)	//	Allow comments in file
			continue;
		while(get_float_from_within_string(&p,&dummy)) {
			if(istime) {
				if(cnt == 0) {
					if(dummy != 0.0) {
						sprintf(errstr,"Initial time in data in file %s must be zero.\n",str);
						return(DATA_ERROR);
					}
				} else {
					if(dummy <= lasttime) {
						sprintf(errstr,"Times do not advance between %lf and %lf in file %s\n",lasttime,dummy,str);
						return(DATA_ERROR);
					}
				}
				lasttime = dummy;
			} else if(dummy > 12 || dummy < -12) {
				sprintf(errstr,"Found transposition value (%lf) out of range (-12 to +12) in file %s\n",dummy,str);
				return(DATA_ERROR);
			}
			istime = !istime;
			cnt++;
		}
	}
	if(cnt == 0) {
		sprintf(errstr,"No data found in file %s\n",str);
		return(DATA_ERROR);
	}
	if(!EVEN(cnt)) {
		sprintf(errstr,"Data not paired correctly in file %s\n",str);
		return(DATA_ERROR);
	}
	if((dz->parray[0] = (double *)malloc(cnt * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store transposition data.\n");
		return(MEMORY_ERROR);
	}
	cnt = 0;
	fseek(fp,0,0);
	while(fgets(temp,200,fp)!=NULL) {
		p = temp;
		while(isspace(*p))
			p++;
		if(*p == ';' || *p == ENDOFSTR)	//	Allow comments in file
			continue;
		while(get_float_from_within_string(&p,&dummy)) {
			dz->parray[0][cnt] = dummy;
			cnt++;
		}
	}
	fclose(fp);
	dz->itemcnt = cnt;
	return FINISHED;
}

/**************************** DO_ITERATE_PREPROCESS ******************************/

int do_iterate_preprocess(dataptr dz)
{
	int exit_status, m;
	double maxpscat, mindelay;
	int is_unity_gain = FALSE;
	int mindelay_samps, inmsampsize, maxoverlay_cnt, maxlen;
	if(dz->iparam[ITER_RSEED] > 0)
		srand((int)dz->iparam[ITER_RSEED]);
	else {
		if(dz->vflag[0]) {
			sprintf(errstr,"NORMALISATION CANNOT BE USED IF SEED VALUE IS ZERO\n");
			return USER_ERROR;
		}
		initrand48();
	}
	if(dz->brksize[ITER_PSCAT]) {
		if((exit_status = get_maxvalue_in_brktable(&maxpscat,ITER_PSCAT,dz))<0)
			return(exit_status);
	} else
		maxpscat = dz->param[ITER_PSCAT];
	if(dz->vflag[0] == 0 && (dz->param[ITER_GAIN]==DEFAULT_ITER_GAIN)) {
		if(dz->brksize[ITER_DELAY]) {
			if((exit_status = get_minvalue_in_brktable(&mindelay,ITER_DELAY,dz))<0)
				return(exit_status);
		} else
			mindelay = dz->param[ITER_DELAY];
		mindelay_samps = round(mindelay * (double)dz->infile->srate);
		maxlen = dz->insams[0];
		for(m=1;m<dz->infilecnt;m++)
			maxlen = max(maxlen,dz->insams[m]);
		inmsampsize = maxlen/dz->infile->channels;
		maxoverlay_cnt = round(((double)inmsampsize/(double)mindelay_samps)+1.0);
		maxoverlay_cnt++;
		dz->param[ITER_GAIN]   = 1.0/(double)maxoverlay_cnt;
	} else
		dz->param[ITER_GAIN] = 1.0;
	
	if(!dz->brksize[ITER_DELAY])
		dz->iparam[ITER_MSAMPDEL] = round(dz->param[ITER_DELAY] * (double)dz->infile->srate);
	dz->param[ITER_STEP] = pow(2.0,dz->parray[0][1] * OCTAVES_PER_SEMITONE);; /* 1st sound is transposed by first transposition value */
	if(flteq(dz->param[ITER_GAIN],1.0))
		is_unity_gain = TRUE;
	dz->iparam[ITER_DO_SCALE] = TRUE;
	dz->iparam[ITER_PROCESS] = ST_INTP_SHIFT;
	if(dz->param[ITER_ASCAT] == 0.0) {
		if(is_unity_gain)
			dz->iparam[ITER_DO_SCALE] = FALSE;
		dz->iparam[ITER_PROCESS] = FIXA_ST_INTP_SHIFT;
	}
	return create_the_iterbufs(maxpscat,dz);
}

/*************************** CREATE_THE_ITERBUFS **************************
 *
 * (1)	Create extra spaces for interpolation guard points at end of infile.
 *
 * (2) 	Allow for any cumulative addition errors in interpolation.
 *
 * (3)	Output buffer must be at least as big as the overflow buffer.
 *	Output buffer must be big enough for the whole of any possible
 *	data overflow (in overflow_size buff) to be copied back into it.
 *	This is because the overflow buffer is ZEROED after such a copy
 *	and if a 2nd copy of the overflow back into the main buffer
 *	were necessary , we would be copying zeroes rather than true data.
 *
 *
 *		true buffer	    			overflow
 *  |-----------------------------|------------------------------------------|
 *			    worst			  ^					    					 ^
 *		    possible case		  ^					    					 ^
 *		                 	      ^----->-delayed by maxdelay_size to ->-----^
 *			         			  ^<-restored by -buffer_size into truebuf-<-^
 *  |<-------- BUFFER_SIZE------->
 *
 */

int create_the_iterbufs(double maxpscat,dataptr dz)
{
	int seccnt;
	double k;
	int extra_space, infile_space, max_infile_space, big_buffer_size;
	int overflowsize, maxinsize;

	int framesize = F_SECSIZE * sizeof(float) * dz->infile->channels;
	int chans = dz->infile->channels, m;
	unsigned int min_bufsize;
	maxinsize = dz->insams[0];
	for(m=1;m < dz->infilecnt;m++)
		maxinsize = max(maxinsize,dz->insams[m]);
	infile_space = maxinsize;
	if(dz->param[ITER_PSCAT] > 0.0) {
		infile_space += dz->infile->channels;			/* 1 */
		k = pow(2.0,maxpscat * OCTAVES_PER_SEMITONE);
		overflowsize = (round((double)(maxinsize/chans) * k) * chans) + 1;
		overflowsize += ITER_SAFETY; 						/* 2 */	
	} else
		overflowsize = maxinsize;
	max_infile_space = 0;
	for(m=0;m < dz->infilecnt;m++) {
		infile_space = dz->insams[m];
		if((seccnt = dz->insams[m]/F_SECSIZE) * F_SECSIZE < infile_space)
			seccnt++;
		infile_space = F_SECSIZE * seccnt;
		max_infile_space = max(infile_space,max_infile_space);
	}
	infile_space = max_infile_space * dz->infilecnt;
	extra_space = infile_space + overflowsize;
	min_bufsize = (extra_space * sizeof(float)) + framesize;
	dz->buflen     = infile_space;
	big_buffer_size = dz->buflen + extra_space;
	if((dz->bigbuf = (float *)Malloc(big_buffer_size * sizeof(float)))==NULL) {
		sprintf(errstr, "INSUFFICIENT MEMORY to create sound buffers.\n");
		return(MEMORY_ERROR);
	}
	dz->sbufptr[0] = dz->sampbuf[0] = dz->bigbuf;
	for(m=0;m<dz->infilecnt;m++)
		dz->sbufptr[m+1] = dz->sampbuf[m+1] = dz->sampbuf[m] + max_infile_space;
	dz->sbufptr[26] = dz->sampbuf[26] = dz->sampbuf[25] + dz->buflen;
	dz->sbufptr[27] = dz->sampbuf[27] = dz->sampbuf[26] + overflowsize;
	for(m=0;m<dz->infilecnt;m++)
		memset((char *)dz->sampbuf[m],0,(size_t)(max_infile_space * sizeof(float)));
	memset((char *)dz->sampbuf[25],0,(size_t)(dz->buflen * sizeof(float)));
	memset((char *)dz->sampbuf[26],0,(size_t)(overflowsize * sizeof(float)));
	dz->infilespace = infile_space; 
	return(FINISHED);
}

/****************************** DO_ITERATION **************************/

#define ACCEPTABLE_LEVEL 0.75

int iterline(dataptr dz)
{
	int    exit_status, iterating;
	int   write_end = 0, tail, cnt, arraysize = BIGARRAY, n;
	float *tailend;
	int    bufs_written, finished;
	double thistime, thistrans;
	int   out_sampdur = 0;
	int   write_start, local_write_start;
	double one_over_sr = 1.0/(double)(dz->infile->srate * dz->infile->channels), maxoutsamp = 0.0, maxinsamp = 0.0;
	int    passno, is_penult = 0, pstep, m;
	int   k;
	double *gain, *pshift, gaingain = -1.0;
	int   *wstart;
	double *trans = dz->parray[0];

	pstep = ITER_STEP;
	iterating = 1;
	if ((gain = (double *)malloc(arraysize * sizeof(double)))==NULL) {
		sprintf(errstr,"Insufficient memory to store gain values\n");
		return(MEMORY_ERROR);
	}
	if ((pshift = (double *)malloc(arraysize * sizeof(double)))==NULL) {
		sprintf(errstr,"Insufficient memory to store pitch shift values\n");
		return(MEMORY_ERROR);
	}
	if ((wstart = (int *)malloc(arraysize * sizeof(int)))==NULL) {
		sprintf(errstr,"Insufficient memory to store pitch shift values\n");
		return(MEMORY_ERROR);
	}
	out_sampdur  = round(dz->param[ITER_DUR] * (double)dz->infile->srate) * dz->infile->channels;
	if(sloom)
		dz->tempsize = out_sampdur;		
	thistrans = trans[1];
	for(passno=0;passno<2;passno++) {
		is_penult = 0;
		cnt = 0;
		bufs_written = 0;
		write_start = 0;
		maxoutsamp = 0.0;
		memset((char *)dz->sampbuf[25],0,dz->buflen * sizeof(float));
		for(m=0;m<dz->infilecnt;m++)
			sndseekEx(dz->ifd[m],0L,0);
		display_virtual_time(0L,dz);
		fflush(stdout);
		if(passno > 0) {
			print_outmessage_flush("Second pass, for greater level\n");
			dz->tempsize = dz->total_samps_written;
			dz->total_samps_written = 0;
			memset((char *)dz->sampbuf[0],0,(dz->sampbuf[27] - dz->sampbuf[0]) * sizeof(float));
		}
		if((exit_status = read_the_input_snds(&maxinsamp,passno,dz))<0)
			return(exit_status);
		if(maxinsamp <= 0.0) {
			sprintf(errstr,"NO SIGNIFICANT LEVEL IN INPUT SOUND\n");
			return DATA_ERROR;
		}
		if(dz->iparam[ITER_DO_SCALE]) {
			for(m=0;m<dz->infilecnt;m++) {
				for(n=0; n < dz->insams[m]; n++)
					dz->sampbuf[m][n] = (float)(dz->sampbuf[m][n] * dz->param[ITER_GAIN]);
			}
		}
		/* 1 */
		local_write_start = 0;
		switch(dz->iparam[ITER_PROCESS]) {
		case(ST_INTP_SHIFT):      	
			write_end = iter_shift_interp_stereo(0,passno,gain,pshift,local_write_start,&maxoutsamp,pstep,iterating,thistrans,dz);		
			break;
		case(FIXA_ST_INTP_SHIFT): 	
			write_end = fixa_iter_shift_interp_stereo(0,passno,gain,pshift,local_write_start,&maxoutsamp,pstep,iterating,thistrans,dz); 
			break;
		}
		thistime = 0.0;
		if((exit_status = read_values_from_all_existing_brktables(thistime,dz))<0)
			return(exit_status);
		if(dz->mode == 0) {		//	interp transposition
			if((exit_status = read_interpd_transposition_value(thistime,&thistrans,dz)) < 0)
				return exit_status;
		} else {
			if((exit_status = read_stepd_transposition_value(thistime,&thistrans,dz)) < 0)
				return exit_status;
		}
		if(dz->brksize[ITER_DELAY])
			dz->iparam[ITER_MSAMPDEL] = round(dz->param[ITER_DELAY] * (double)dz->infile->srate);
		if(passno==0)
			wstart[cnt] = get_next_writestart(write_start,dz);
		write_start = wstart[cnt];
		local_write_start = write_start;
		finished = FALSE;
		for(;;) {
			if(write_start >= out_sampdur)
				finished = TRUE;
			if(finished)
				break;
			while(local_write_start >= dz->buflen) {
				if(passno > 0) {
					if((exit_status = write_samps(dz->sampbuf[25],dz->buflen,dz))<0)
						return(exit_status);
				}
				bufs_written++;
				tail = write_end - dz->buflen;
				memset((char *)dz->sampbuf[25],0,dz->buflen * sizeof(float));
				if(tail > 0) {
					memmove((char *)dz->sampbuf[25],(char *)dz->sampbuf[26],tail * sizeof(float));
					tailend = dz->sampbuf[25] + tail;
				} else
					tailend = dz->sampbuf[26];
				memset((char *)tailend,0,(dz->sampbuf[27] - tailend) * sizeof(float));
				local_write_start -= dz->buflen;
				write_end         -= dz->buflen;
			}
			cnt++;
			if((passno == 0) && (cnt >= arraysize)) {
				arraysize += BIGARRAY;
				if ((gain = (double *)realloc((char *)gain,arraysize * sizeof(double)))==NULL) {
					sprintf(errstr,"Insufficient memory to store gain values (2)\n");
					return(MEMORY_ERROR);
				}
				if ((pshift = (double *)realloc((char *)pshift,arraysize * sizeof(double)))==NULL) {
					sprintf(errstr,"Insufficient memory to store gain values (2)\n");
					return(MEMORY_ERROR);
				}
				if ((wstart = (int *)realloc((char *)wstart,arraysize * sizeof(int)))==NULL) {
					sprintf(errstr,"Insufficient memory to store gain values (2)\n");
					return(MEMORY_ERROR);
				}
			}
			thistime = ((dz->buflen * bufs_written) + local_write_start) * one_over_sr;
			
			if((exit_status = read_values_from_all_existing_brktables(thistime,dz))<0)
				return(exit_status);
			if(dz->mode == 0) {		//	interp transposition
				if((exit_status = read_interpd_transposition_value(thistime,&thistrans,dz)) < 0)
					return exit_status;
			} else {
				if((exit_status = read_stepd_transposition_value(thistime,&thistrans,dz)) < 0)
					return exit_status;
			}
			if(is_penult) {
				dz->param[ITER_PSCAT] = 0.0;
				dz->param[ITER_ASCAT] = 0.0;
			}
			if(dz->brksize[ITER_DELAY])
				dz->iparam[ITER_MSAMPDEL] = round(dz->param[ITER_DELAY] * (double)dz->infile->srate);
			write_end = iterate(cnt,passno,gain,pshift,write_end,local_write_start,&maxoutsamp,pstep,iterating,thistrans,dz);
			if(passno==0)
				wstart[cnt] = get_next_writestart(write_start,dz);
			write_start = wstart[cnt];
			local_write_start = write_start - (bufs_written * dz->buflen);
		}
		if(passno > 0) {
			if(write_end > 0) {
				if((exit_status = write_samps(dz->sampbuf[25],write_end,dz))<0)
					return(exit_status);
			}
		} else {
			if(maxoutsamp <= 0.0) {
				sprintf(errstr,"No significant signal level found");
				return(DATA_ERROR);
			}
			if(dz->vflag[0])
				gaingain = maxinsamp/maxoutsamp;
			else if(maxoutsamp < ACCEPTABLE_LEVEL || maxoutsamp > 0.99)
				gaingain = ACCEPTABLE_LEVEL/maxoutsamp;
			else
				gaingain = 1.0;
			switch(dz->iparam[ITER_PROCESS]) {
			case(ST_INTP_SHIFT):      	
				for(k=0;k<=cnt;k++)
					gain[k] *= gaingain;
				break;
			case(FIXA_ST_INTP_SHIFT): 	
				for(k=0;k<=cnt;k++)
					gain[k] = gaingain;
				break;
			}
		}
	}
	return FINISHED;
}

/*********************** ITER_SHIFT_INTERP_STEREO *************************/

//TW COMPLETELY UPDATED FUNCTION : (flt-converted)
int iter_shift_interp_stereo(int cnt,int passno, double *gain,double *pshift,int local_write_start,
		double *maxoutsamp,int pstep,int iterating,double thistrans,dataptr dz)
{
	register int i = 0, j = local_write_start, k, inmsampsize;
	int n, m;
	double d = 0.0, part = 0.0;
	float val, nextval, diff;
	float *outbuf = dz->sampbuf[25], *inbuf;
	double z;
	double thisgain;
	choose_the_transposed_snd(&thistrans,&m);	//	Selects the appropriate input sound, and resets the transpos value
	thistrans = pow(2.0,thistrans * OCTAVES_PER_SEMITONE);
	inbuf  = dz->sampbuf[m];
	inmsampsize = (dz->insams[m]/dz->infile->channels) + 1;
	dz->param[pstep] *= thistrans;
	if(passno == 0) {
		gain[cnt] = get_gain(dz);
		thisgain = gain[cnt];

		while(i < inmsampsize) {
			k = i*dz->infile->channels;
			for(n=0;n<dz->infile->channels;n++) {
				val     = inbuf[k];
				nextval = inbuf[k+dz->infile->channels];
				diff    = nextval - val;
				z = val + ((double)diff * part);
				if(iterating)
					z = (z * thisgain);
				z += outbuf[j];
				*maxoutsamp = max(*maxoutsamp,fabs(z));
				outbuf[j++] = (float)z;
				k++;
			}
			d   += dz->param[pstep];
			i    = (int)d; 			/* TRUNCATE */
			part = d - (double)i; 
		}
		pshift[cnt] = get_pshift(dz);
		dz->param[pstep] = pshift[cnt];
	} else {
		thisgain = gain[cnt];

		while(i < inmsampsize) {
			k = i*dz->infile->channels;
			for(n=0;n<dz->infile->channels;n++) {
				val     = inbuf[k];
				nextval = inbuf[k+dz->infile->channels];
				diff    = nextval - val;
				z = val + ((double)diff * part);
				if(iterating)
					z = (z * thisgain);
				z += outbuf[j];
				outbuf[j++] = (float)z;
				k++;
			}
			d   += dz->param[pstep];
			i    = (int)d; 			/* TRUNCATE */
			part = d - (double)i; 
		}
		dz->param[pstep] = pshift[cnt];
	}
	return(j);
}

/*********************** FIXA_ITER_SHIFT_INTERP_STEREO *************************/

//TW COMPLETELY UPDATED FUNCTION : (flt-converted)
int fixa_iter_shift_interp_stereo(int cnt,int passno,double *gain,double *pshift,int local_write_start,
		double *maxoutsamp,int pstep,int iterating,double thistrans,dataptr dz)
{
	register int i = 0, j = local_write_start, k, inmsampsize;
	int n, m;
	double d = 0.0, part = 0.0;
	float val, nextval, diff;
	float *outbuf = dz->sampbuf[25], *inbuf;
	double z;
	choose_the_transposed_snd(&thistrans,&m);	//	Selects the appropriate input sound, and resets the transpos value
	thistrans = pow(2.0,thistrans * OCTAVES_PER_SEMITONE);
	inbuf  = dz->sampbuf[m];
	inmsampsize = (dz->insams[m]/dz->infile->channels) + 1;
	dz->param[pstep] *= thistrans;
	if(passno == 0) {
		while(i < inmsampsize) {
			k = i*dz->infile->channels;
			for(n=0;n<dz->infile->channels;n++) {
				val     = inbuf[k];
				nextval = inbuf[k+dz->infile->channels];
				diff    = nextval - val;
				z = val + ((double)diff * part);
				z += outbuf[j];
				*maxoutsamp = max(*maxoutsamp,fabs(z));
				outbuf[j++] = (float)z;
				k++;
			}
			d   += dz->param[pstep];
			i    = (int)d; 			/* TRUNCATE */
			part = d - (double)i; 
		}
		pshift[cnt] = get_pshift(dz);
	} else {
		while(i < inmsampsize) {
			k = i*dz->infile->channels;
			for(n=0;n<dz->infile->channels;n++) {
				val     = inbuf[k];
				nextval = inbuf[k+dz->infile->channels];
				diff    = nextval - val;
				z = val + ((double)diff * part);
				if(iterating)
					z = (z * gain[cnt]);
				z += outbuf[j];
				outbuf[j++] = (float)z;
				k++;
			}
			d   += dz->param[pstep];
			i    = (int)d; 			/* TRUNCATE */
			part = d - (double)i; 
		}
	}
	dz->param[pstep] = pshift[cnt];
	return(j);
}

/**************************** READ_INTERPD_TRANSPOSITION_VALUE *****************************/

int read_interpd_transposition_value(double thistime,double *thistrans,dataptr dz)
{
    double *p, val, *trans = dz->parray[0], *transend = dz->parray[0] + dz->itemcnt;
    double hival, loval, hitim, lotim;
	p = trans;
	while(thistime >= *p) {
		p += 2;
		if(p >= transend) {
			*thistrans = trans[dz->itemcnt - 1];
			return FINISHED;
		}
	}
	hival  = *(p+1);
	hitim  = *p;
	loval  = *(p-1);
	lotim  = *(p-2);
	val    = (thistime - lotim)/(hitim - lotim);
	val   *= (hival - loval);
	val   += loval;
	*thistrans = val;
    return(FINISHED);
}

/**************************** READ_STEPD_TRANSPOSITION_VALUE *****************************/

int read_stepd_transposition_value(double thistime,double *thistrans,dataptr dz)
{
    double *p, *trans = dz->parray[0], *transend = dz->parray[0] + dz->itemcnt;
    double hival, loval, hitim, lotim, histep, lostep;
	p = trans;
	while(thistime >= *p) {
		p += 2;
		if(p >= transend) {
			*thistrans = trans[dz->itemcnt - 1];
			return FINISHED;
		}
	}
	hival  = *(p+1);
	hitim  = *p;
	loval  = *(p-1);
	lotim  = *(p-2);
	histep = hitim - thistime;
	lostep = thistime - lotim;
	if(histep < lostep)
		*thistrans = hival;
	else
		*thistrans = loval;
    return(FINISHED);
}

/*************************** READ_THE_INPUT_SNDS **************************/

int read_the_input_snds(double *maxinsamp,int passno,dataptr dz)
{
	int samps, k, samps_read;
	double thismaxinsamp;
	int n,m;
	if(passno == 0)
		*maxinsamp = HUGE;
	for(m=0;m<dz->infilecnt;m++) {
		if((samps_read = fgetfbufEx(dz->sampbuf[m], dz->insams[m]/* + SECSIZE*/,dz->ifd[m],0)) <= 0) {
			sprintf(errstr,"Can't read bytes from input soundfile\n");
			if(samps_read<0)
				return(SYSTEM_ERROR);
			return(DATA_ERROR);
		}
		if(samps_read!=dz->insams[m]) {
			sprintf(errstr, "Failed to read all of source file. read_the_input_snds()\n");
			return(PROGRAM_ERROR);
		}
		samps = samps_read / dz->infile->channels;

		if(dz->param[ITER_PSCAT] > 0.0) {
			k = samps * dz->infile->channels;
			for(n=0;n<dz->infile->channels;n++) {
				dz->sampbuf[m][k] = (float)0;
				k++;		/* GUARD POINTS FOR INTERPOLATION */
			}
		}
		if(passno == 0) {
			thismaxinsamp = 0.0;
			for(k=0;k < dz->insams[m];k++) {
				if(fabs(dz->sampbuf[m][k]) > thismaxinsamp)
					thismaxinsamp = fabs(dz->sampbuf[m][k]);
			}
			if(thismaxinsamp < *maxinsamp)
				*maxinsamp = thismaxinsamp;		//	Find MINIMUM maximum
		}
	}
	return(FINISHED);
}

/*************************** GET_NEXT_WRITESTART ****************************/

int get_next_writestart(int write_start,dataptr dz)
{
	int this_step;
	double d;  
	int mwrite_start = write_start/dz->infile->channels;
	if(dz->param[ITER_RANDOM] > 0.0) {
		d = ((drand48() * 2.0) - 1.0) * dz->param[ITER_RANDOM];
		d += 1.0;
		this_step = (int)round((double)dz->iparam[ITER_MSAMPDEL] * d);
		mwrite_start += this_step;
	} else
		mwrite_start += dz->iparam[ITER_MSAMPDEL];
	write_start = mwrite_start * dz->infile->channels;
	return(write_start);
}    

/******************************* ITERATE *****************************/

//TW COMPLETELY UPDATED FUNCTION : (flt-converted)
int iterate(int cnt,int pass,double *gain,double *pshift,int write_end,int local_write_start,
 double *maxoutsamp,int pstep,int iterating,double thistrans,dataptr dz)
{
	int wr_end = 0;
	switch(dz->iparam[ITER_PROCESS]) {
	case(ST_INTP_SHIFT):      	
		wr_end = iter_shift_interp_stereo(cnt,pass,gain,pshift,local_write_start,maxoutsamp,pstep,iterating,thistrans,dz);		
		break;
	case(FIXA_ST_INTP_SHIFT): 	
		wr_end = fixa_iter_shift_interp_stereo(cnt,pass,gain,pshift,local_write_start,maxoutsamp,pstep,iterating,thistrans,dz); 
		break;
	}
	return max(wr_end,write_end);
}

/******************************** GET_GAIN *****************************/

double get_gain(dataptr dz)
{
	double scatter;
	double newlgain = 0.0;
	newlgain = dz->param[ITER_GAIN];
	if(dz->param[ITER_ASCAT] > 0.0) {
		scatter  = drand48() * dz->param[ITER_ASCAT];
		scatter  = 1.0 - scatter;
		newlgain = scatter * (double)dz->param[ITER_GAIN];
	}
	return(newlgain);
}

/******************************** GET_PSHIFT *****************************/

double get_pshift(dataptr dz)
{
	double scatter;
	scatter = (drand48() * 2.0) - 1.0;
	scatter *= dz->param[ITER_PSCAT];
	return(pow(2.0,scatter * OCTAVES_PER_SEMITONE));
}

/*************************** CHOOSE_THE_TRANSPOSED_SND **************************/

void choose_the_transposed_snd(double *thistrans,int *filindx)
{
	int tone_transpose;

// TRANSPOSITION IS STILL IN SEMITONES : CONVERT TO NEAREST
	if(*thistrans < 0.0)
		tone_transpose = -(int)round(-(*thistrans));
	else
		tone_transpose = (int)round((*thistrans));
	*filindx = tone_transpose + 12;
	*thistrans -= (tone_transpose);		//	Transposition of actual sound used
}
