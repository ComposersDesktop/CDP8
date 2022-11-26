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
#include <pnames.h>
#include <speccon.h>
#include <standalone.h>
#include <tkglobals.h>
#include <filetype.h>
#include <modeno.h>
#include <cdpmain.h>
#include <logic.h>
#include <globcon.h>
#include <cdpmain.h>
#include <sfsys.h>
#include <osbind.h>
#include <ctype.h>
#include <string.h>

#ifdef unix
#define round(x) lround((x))
#endif

#define EIGHT_OVER_SEVEN (1.142857143)
#define LOCSPREAD (5)

static int specsphinx(dataptr dz);
static int specvolve(dataptr dz);
static int spec_sphinx(float* inbuf1,float* inbuf2,float* ampval,int* amploc,float* frqval,int* frqloc,dataptr dz);

char errstr[2400];
const char* cdp_version = "7.1.0";

/* extern */ int sloom = 0;
/* extern */ int	sloombatch = 0;
/* extern */ int anal_infiles = 1;
/* extern */ int is_converted_to_stereo = -1;

/* CDP LIBRARY FUNCTIONS TRANSFERRED HERE */

static int 	set_param_data(aplptr ap, int special_data,int maxparamcnt,int paramcnt,char *paramlist);
static int  set_vflgs(aplptr ap,char *optflags,int optcnt,char *optlist,
				char *varflags,int vflagcnt, int vparamcnt,char *varlist);
static int 	setup_parameter_storage_and_constants(int storage_cnt,dataptr dz);
static int	initialise_is_int_and_no_brk_constants(int storage_cnt,dataptr dz);
static int	mark_parameter_types(dataptr dz,aplptr ap);
static int  establish_application(dataptr dz);
static int  application_init(dataptr dz);
static int  initialise_vflags(dataptr dz);
static int  setup_input_param_defaultval_stores(int tipc,aplptr ap);
static int  setup_and_init_input_param_activity(dataptr dz,int tipc);
static int  get_tk_cmdline_word(int *cmdlinecnt,char ***cmdline,char *q);
static int  assign_file_data_storage(int infilecnt,dataptr dz);
//static int  store_wordlist(char *filename,dataptr dz);
static int setup_and_init_input_brktable_constants(dataptr dz,int brkcnt);

/* CDP LIB FUNCTION MODIFIED TO AVOID CALLING setup_particular_application() */

static int  parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);

/* SIMPLIFICATION OF LIB FUNC TO APPLY TO JUST THIS FUNCTION */

static int  parse_infile_and_check_type(char **cmdline,dataptr dz);
static int  handle_the_outfile(int *cmdlinecnt,char ***cmdline,int is_launched,dataptr dz);
static int  setup_specsphinx_application(dataptr dz);
static int  setup_specsphinx_param_ranges_and_defaults(dataptr dz);
static int  open_the_first_infile(char *filename,dataptr dz);
static int  handle_the_extra_infiles(char ***cmdline,int *cmdlinecnt,dataptr dz);

static int get_the_mode_from_cmdline(char *str,dataptr dz);

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
	int exit_status;
/*	FILE *fp   = NULL; */
	dataptr dz = NULL;
//	char *special_data_string = NULL;
	char **cmdline;
	int  cmdlinecnt;
	aplptr ap;
	int is_launched = FALSE;

						/* CHECK FOR SOUNDLOOM */
//TW UPDATE
	if(argc==2 && (strcmp(argv[1],"--version") == 0)) {
		fprintf(stdout,"%s\n",cdp_version);
		fflush(stdout);
		return 0;
	}
	if((sloom = sound_loom_in_use(&argc,&argv)) > 1) {
		sloom = 0;
		sloombatch = 1;
	}
	if(sflinit("cdp")){
		sfperror("cdp: initialisation\n");
		return(FAILED);
	}

						  /* SET UP THE PRINCIPLE DATASTRUCTURE */
	if((exit_status = establish_datastructure(&dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}

	if(!sloom) {
						  /* INITIAL CHECK OF CMDLINE DATA */
		if((exit_status = make_initial_cmdline_check(&argc,&argv))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		cmdline    = argv;	/* GET PRE_DATA, ALLOCATE THE APPLICATION, CHECK FOR EXTRA INFILES */
		cmdlinecnt = argc;
// get_process_and_mode_from_cmdline -->
		if (!strcmp(argv[0],"specsphinx")) {		
			dz->process = SPECSPHINX;
		} else
			usage1();
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
		if((exit_status = setup_specsphinx_application(dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		if((exit_status = count_and_allocate_for_infiles(cmdlinecnt,cmdline,dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
	} else {
		//parse_TK_data() =
		if((exit_status = parse_sloom_data(argc,argv,&cmdline,&cmdlinecnt,dz))<0) {  	/* includes setup_particular_application()      */
			exit_status = print_messages_and_close_sndfiles(exit_status,is_launched,dz);/* and cmdlinelength check = sees extra-infiles */
			return(exit_status);		 
		}
	}
	ap = dz->application;

	// parse_infile_and_hone_type() = 
	if((exit_status = parse_infile_and_check_type(cmdline,dz))<0) {
		exit_status = print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	// setup_param_ranges_and_defaults = MOVED IN THIS CASE ONLY TO LATER

					/* OPEN FIRST INFILE AND STORE DATA, AND INFORMATION, APPROPRIATELY */

	if((exit_status = open_the_first_infile(cmdline[0],dz))<0) {	
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);	
		return(FAILED);
	}
	cmdlinecnt--;
	cmdline++;
	
	if((exit_status = handle_the_extra_infiles(&cmdline,&cmdlinecnt,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);		
		return(FAILED);
	}
	if((exit_status = setup_specsphinx_param_ranges_and_defaults(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	// FOR display_virtual_time 
	dz->tempsize = min(dz->insams[0],dz->insams[1]);

	// handle_outfile
	if((exit_status = handle_the_outfile(&cmdlinecnt,&cmdline,is_launched,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}

	// handle_formants
	// handle_formant_quiksearch
	// handle_special_data
 	if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {		// CDP LIB
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	// check_param_validity_and_consistency = 
 	is_launched = TRUE;

	if((exit_status = allocate_double_buffer(dz))<0){
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	// param_preprocess = 
	// spec_process_file
	if(dz->mode == 0) {
		if((exit_status = specsphinx(dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
	} else {
		if((exit_status = specvolve(dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
	}
	if((exit_status = complete_output(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	exit_status = print_messages_and_close_sndfiles(FINISHED,is_launched,dz);
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
	int storage_cnt, n;
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
		dz->infilecnt = -2;	/* flags 2 or more */
	// establish_bufptrs_and_extra_buffers
	dz->extra_bufcnt =  0; 
	dz->bptrcnt = 5; 	
	// setup_internal_arrays_and_array_pointers()
	dz->fptr_cnt = 2;
	dz->array_cnt = 5;
	if((dz->fptr = (float  **)malloc(dz->fptr_cnt * sizeof(float *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for internal float-pointer arrays.\n");
		return(MEMORY_ERROR);
	}
	if(dz->array_cnt > 0) {  
		if((dz->parray  = (double **)malloc(dz->array_cnt * sizeof(double *)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for internal double arrays.\n");
			return(MEMORY_ERROR);
		}
		for(n=0;n<dz->array_cnt;n++)
			dz->parray[n] = NULL;
	}
	for(n=0;n<dz->fptr_cnt;n++)
		dz->fptr[n] = NULL;
	return establish_spec_bufptrs_and_extra_buffers(dz);
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

/************************* SETUP_SPECSPHINX_APPLICATION *******************/

int setup_specsphinx_application(dataptr dz)
{
	int exit_status;
	aplptr ap;
	if((exit_status = establish_application(dz))<0)		// GLOBAL
		return(FAILED);
	ap = dz->application;
	// SEE parstruct FOR EXPLANATION of next 2 functions
	if((exit_status = set_param_data(ap,0   ,0,0,""      ))<0)
		return(FAILED);
	if(dz->mode == 0) {
		if((exit_status = set_vflgs(ap,  "af",2,"DD"  ,""   ,0,0,""   ))<0)
			return(FAILED);
	} else {
		if((exit_status = set_vflgs(ap,  "bg",2,"DD"  ,""   ,0,0,""   ))<0)
			return(FAILED);
	}
	// THERE IS NO NEED TO set_formant_flags in this case....
	// Following only needed if internal params are linked to dz structure
	// set_legal_infile_structure -->
	dz->has_otherfile = FALSE;
	// assign_process_logic -->
	dz->input_data_type = TWO_ANALFILES;
	dz->process_type	= BIG_ANALFILE;	
	dz->outfiletype  	= ANALFILE_OUT;
	return application_init(dz);	//GLOBAL
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
			sprintf(errstr,"Failed tp parse input file %s\n",cmdline[0]);
			return(PROGRAM_ERROR);
		} else if(infile_info->filetype != ANALFILE)  {
			sprintf(errstr,"File %s is not of correct type\n",cmdline[0]);
			return(DATA_ERROR);
		} else if((exit_status = copy_parse_info_to_main_structure(infile_info,dz))<0) {
			sprintf(errstr,"Failed to copy file parsing information\n");
			return(PROGRAM_ERROR);
		}
		free(infile_info);
	}
	if((exit_status = set_chunklens_and_establish_windowbufs(dz))<0)
		return(exit_status);
	return(FINISHED);
}

/************************* SETUP_SPECSPHINX_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_specsphinx_param_ranges_and_defaults(dataptr dz)
{
	int exit_status;
	aplptr ap = dz->application;
	// set_param_ranges()
	ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
	// setup_input_param_range_stores()
	if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
		return(FAILED);
	// get_param_ranges()
	if(dz->mode == 0) {
		ap->lo[0]   		= 0.0;
		ap->hi[0]   		= 1.0;
		ap->default_val[0]	= 0.0;
		ap->lo[1]   		= 0.0;
		ap->hi[1]   		= 1.0;
		ap->default_val[1]	= 0.0;
	} else {
		ap->lo[0]   		= -1.0;
		ap->hi[0]   		= 1.0;
		ap->default_val[0]	= 0.0;
		ap->lo[1]   		= 0.01;
		ap->hi[1]   		= 100.0;
		ap->default_val[1]	= 1.0;
	}
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
			if((exit_status = setup_specsphinx_application(dz))<0)
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

/************************** OPEN_THE_FIRST_INFILE *****************************/

int open_the_first_infile(char *filename,dataptr dz)
{
	if((dz->ifd[0] = sndopenEx(filename,0,CDP_OPEN_RDONLY)) < 0) {
		sprintf(errstr,"Failure to open file %s for input.\n",filename);
		return(SYSTEM_ERROR);
	}
	if(dz->infilecnt<=0 || dz->infile->filetype!=ANALFILE) {
		sprintf(errstr,"%s is wrong type of file for this process.\n",filename);
		return(DATA_ERROR);
	}
	dz->samps_left = dz->insams[0];
	return(FINISHED);
}

/************************ HANDLE_THE_EXTRA_INFILES *********************/

int handle_the_extra_infiles(char ***cmdline,int *cmdlinecnt,dataptr dz)
{
					/* OPEN ANY FURTHER INFILES, CHECK COMPATIBILITY, STORE DATA AND INFO */
	int  exit_status, n;
	char *filename;

	if(dz->infilecnt > 1) {
		for(n=1;n<dz->infilecnt;n++) {
			filename = (*cmdline)[0];
			if((exit_status = handle_other_infile(n,filename,dz))<0)
				return(exit_status);
			(*cmdline)++;
			(*cmdlinecnt)--;
		}
	} else {
		sprintf(errstr,"Insufficient input files for this process\n");
		return(DATA_ERROR);
	}
	return(FINISHED);
}

/************************ HANDLE_THE_OUTFILE *********************/

int handle_the_outfile(int *cmdlinecnt,char ***cmdline,int is_launched,dataptr dz)
{
	char *filename = NULL;
	int n;
	int stype = SAMP_FLOAT;

	if(sloom) {
		filename = (*cmdline)[0];
	} else {
		if(*cmdlinecnt<=0) {
			sprintf(errstr,"Insufficient cmdline parameters.\n");
			return(USAGE_ONLY);
		}
		filename = (*cmdline)[0];
		if(filename[0]=='-' && filename[1]=='f') {
			sprintf(errstr,"-f flag used incorrectly on command line (output is not a sound file).\n");
			return(USAGE_ONLY);
		}
		if(file_has_invalid_startchar(filename) || value_is_numeric(filename)) {
			sprintf(errstr,"Outfile name %s has invalid start character(s) or looks too much like a number.\n",filename);
			return(DATA_ERROR);
		}
	}
	dz->true_outfile_stype = stype;
	dz->outfilesize = min(dz->insams[1],dz->insams[0]);	
	if((dz->ofd = sndcreat_formatted(filename,dz->outfilesize,stype,
			dz->infile->channels,dz->infile->srate,CDP_CREATE_NORMAL)) < 0) {
		sprintf(errstr,"Cannot open output file %s\n", filename);
		return(DATA_ERROR);
	}
	dz->outchans = dz->infile->channels;
	dz->needpeaks = 1;
	dz->outpeaks = (CHPEAK *) malloc(sizeof(CHPEAK) * dz->outchans);
	if(dz->outpeaks==NULL)
		return MEMORY_ERROR;
	dz->outpeakpos = (unsigned int *) malloc(sizeof(unsigned int) * dz->outchans);
	if(dz->outpeakpos==NULL)
		return MEMORY_ERROR;
	for(n=0;n < dz->outchans;n++){
		dz->outpeaks[n].value = 0.0f;
		dz->outpeaks[n].position = 0;
		dz->outpeakpos[n] = 0;
	}
	strcpy(dz->outfilename,filename);	   
	(*cmdline)++;
	(*cmdlinecnt)--;
	return(FINISHED);
}

/******************************** USAGE ********************************/

int usage1(void)
{
	return usage2("specsphinx");
}

int usage2(char *str)
{
	if(!strcmp(str,"specsphinx")) {
		fprintf(stdout,
		"USAGE: specsphinx specsphinx 1 analfile1 analfile2 outanalfile\n"
	    "       [-aampbalance] [-ffrqbalance]\n"
		"\n"
		"IMPOSE CHANNEL AMPLITUDES OF FILE2 ON CHANNEL FRQS OF FILE 1\n"
		"\n"
		"AMPBALANCE  Proportion of File1 amps retained (Default 0)\n"
		"FRQBALANCE  Proportion of File2 frqs injected into output spectrum\n"
		"(Default 0)\n"
		"\n"
		"USAGE: specsphinx specsphinx 2 analfile1 analfile2 outanalfile\n"
	    "       [-bbias] [-ggain]\n"
		"\n"
		"MULTIPLY THE SPECTRA\n"
		"\n"
		"BIAS    When non-zero, adds proportion of original files to output.\n"
		"\n      < 1 : add analfile1 to outanalfile in proportion (-bias)/(1+bias)"
		"\n      > 1 : add analfile2 to outanalfile in proportion (bias/(1-bias)"
		"\n"
		"GAIN   Overall gain on output.\n"
		"\n");
	} else
		fprintf(stdout,"Unknown option '%s'\n",str);
	return(USAGE_ONLY);
}

int usage3(char *str1,char *str2)
{
	sprintf(errstr,"Insufficient parameters on command line.\n");
	return(USAGE_ONLY);
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

int get_process_no(char *prog_identifier_from_cmdline,dataptr dz)
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


/***************************** SPECSPHINX *****************************/

int specsphinx(dataptr dz)
{
	int exit_status;
	int ssamps2read;
	double time = 0.0;
	float *inbuf1 = dz->bigfbuf;
	float *inbuf2 = dz->flbufptr[2];
	float *ampval = NULL, *frqval = NULL;
	int	  *amploc = NULL, *frqloc = NULL;

	if((sndseekEx(dz->ifd[1],0,0)<0)){        
        sprintf(errstr,"sndseek() failed in 2nd file\n");
        return SYSTEM_ERROR;
    }
	if((sndseekEx(dz->ifd[0],0,0)<0)){        
        sprintf(errstr,"sndseek() failed in 1st file\n");
        return SYSTEM_ERROR;
    }
	dz->total_samps_read = 0;
	dz->total_windows = 0;
	dz->samps_left = min(dz->insams[0],dz->insams[1]);
	dz->flbufptr[0] = dz->bigfbuf;

	if((dz->brksize[1] > 0.0) || (dz->param[1] > 0)) {
		if((ampval = (float *)malloc(dz->clength * sizeof(float)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for ampvals array.\n");
			return(MEMORY_ERROR);
		}
		if((amploc = (int *)malloc(dz->clength * sizeof(int)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for ampvals array.\n");
			return(MEMORY_ERROR);
		}
		if((frqval = (float *)malloc(dz->clength * sizeof(float)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for ampvals array.\n");
			return(MEMORY_ERROR);
		}
		if((frqloc = (int *)malloc(dz->clength * LOCSPREAD * sizeof(int)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for ampvals array.\n");
			return(MEMORY_ERROR);
		}
	}

	while(dz->samps_left > 0) {
		if((dz->ssampsread = fgetfbufEx(inbuf1, dz->wanted,dz->ifd[0],0)) < 0) {
			sprintf(errstr,"Can't reread samples to do spectral interpolation.\n");
			return(SYSTEM_ERROR);
		}

		if((ssamps2read = fgetfbufEx(inbuf2, dz->wanted,dz->ifd[1],0)) < 0) {
			sprintf(errstr,"Can't reread samples to do spectral interpolation.\n");
			return(SYSTEM_ERROR);
		}
		dz->ssampsread = min(dz->ssampsread,ssamps2read);
		dz->samps_left -= dz->ssampsread;	
		dz->total_samps_read += dz->ssampsread;
		if(dz->brksize[0] || dz->brksize[1]) {
			if(dz->brksize[0]) {
				if((exit_status = read_value_from_brktable(time,0,dz))<0)
					return(exit_status);
			}
			if(dz->brksize[1]) {
				if((exit_status = read_value_from_brktable(time,1,dz))<0)
					return(exit_status);
			}
			time += dz->frametime;
		}
		if(dz->total_windows > 0) {	/* Skip Window 0, no significant data */
			spec_sphinx(inbuf1,inbuf2,ampval,amploc,frqval,frqloc,dz);
		}
		if((exit_status = write_samps(dz->flbufptr[0],dz->wanted,dz))<0)
			return(exit_status);
		dz->total_windows++;
	}
	return(FINISHED);
}

/***************************** SPECVOLVE *****************************/

int specvolve(dataptr dz)
{
	int exit_status, cc, vc;
	int ssamps2read;
	double time = 0.0, val;
	float *inbuf1 = dz->bigfbuf;
	float *inbuf2 = dz->flbufptr[2];

	if((sndseekEx(dz->ifd[1],0,0)<0)){        
        sprintf(errstr,"sndseek() failed in 2nd file\n");
        return SYSTEM_ERROR;
    }
	if((sndseekEx(dz->ifd[0],0,0)<0)){        
        sprintf(errstr,"sndseek() failed in 1st file\n");
        return SYSTEM_ERROR;
    }
	dz->flbufptr[0] = dz->bigfbuf;
	dz->total_samps_read = 0;
	dz->total_windows = 0;
	dz->samps_left = min(dz->insams[0],dz->insams[1]);
	while(dz->samps_left > 0) {
		if((dz->ssampsread = fgetfbufEx(inbuf1, dz->wanted,dz->ifd[0],0)) < 0) {
			sprintf(errstr,"Can't reread samples to do spectral interpolation.\n");
			return(SYSTEM_ERROR);
		}

		if((ssamps2read = fgetfbufEx(inbuf2, dz->wanted,dz->ifd[1],0)) < 0) {
			sprintf(errstr,"Can't reread samples to do spectral interpolation.\n");
			return(SYSTEM_ERROR);
		}
		dz->ssampsread = min(dz->ssampsread,ssamps2read);
		dz->samps_left -= dz->ssampsread;	
		dz->total_samps_read += dz->ssampsread;
		if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
			return(exit_status);
		// bias = dz->param[0]
		// gain = dz->param[1]
		if(dz->param[0] > 0.0) {
			for(cc = 0, vc = 0; cc < dz->clength; cc++, vc += 2) {
				val = inbuf1[AMPP] * inbuf2[AMPP] * dz->param[1];
				val *= 1.0 - dz->param[0];
				val += inbuf2[AMPP] * dz->param[0];
				inbuf1[AMPP] = (float)val;
			}
		} else if(dz->param[0] < 0.0) {
			for(cc = 0, vc = 0; cc < dz->clength; cc++, vc += 2) {
				val = inbuf1[AMPP] * inbuf2[AMPP] * dz->param[1];
				val *= 1.0 + dz->param[0];
				val += inbuf1[AMPP] * (-dz->param[0]);
				inbuf1[AMPP] = (float)val;
			}
		} else {
			for(cc = 0, vc = 0; cc < dz->clength; cc++, vc += 2)
				inbuf1[AMPP] = (float)(inbuf1[AMPP] * inbuf2[AMPP] * dz->param[1]);
		}
		if((exit_status = write_samps(dz->flbufptr[0],dz->wanted,dz))<0)
			return(exit_status);
		time += dz->frametime;
		dz->total_windows++;
	}
	return(FINISHED);
}

/***************************** SPEC_SPHINX *****************************/

int spec_sphinx(float* inbuf1,float* inbuf2,float* ampval,int* amploc,float* frqval,int* frqloc,dataptr dz)
{
	int cc, vc, kk, ii, jj, frqstorecnt, initvc, OK, srchcnt, frqsetcnt = 0;
	float amp, frq, frqhere;
	double frqratio;

	if(dz->param[1] > 0) {
		for(cc = 0, vc = 0; cc < dz->clength; cc++, vc += 2) {
			ampval[cc] = inbuf2[AMPP];					//	Store  input buffer 2's data and location
			frqval[cc] = inbuf2[FREQ];
			amploc[cc] = AMPP;
		}
		memset((char *)ampval,0,dz->clength * sizeof(float));
		memset((char *)frqval,0,dz->clength * sizeof(float));
		memset((char *)amploc,0,dz->clength * sizeof(int));
		memset((char *)frqloc,0,dz->clength * sizeof(int));
		for(cc = 0; cc < dz->clength-1; cc++) {
			for(ii = cc; ii < dz->clength; ii++) {
				if(ampval[ii] > ampval[cc]) {			//	Sort data into decreasing amplitude order
					amp = ampval[cc];
					ampval[cc] = ampval[ii];
					ampval[ii]  = amp;
					frq = frqval[cc];
					frqval[cc] = frqval[ii];
					frqval[ii]  = frq;
					kk = amploc[cc];
					amploc[cc] = amploc[ii];
					amploc[ii]  = kk;
					kk = amploc[cc];
				}
			}
		}
		frqstorecnt = 0;
		for(kk = 0; kk < dz->clength; kk++) {			//	In descending loudness order ...
			frqhere = frqval[kk];						//	Frq at the (next) loudest channel
			initvc = amploc[kk] + 1;					//	Location of that frq in input buffer
			OK = 1;
			for(jj = 0; jj < frqstorecnt;jj++) {		
				if(initvc == frqloc[jj]) {				//	If we've already stored this frq-location, skip and go to next
					OK = 0;
					break;
				}
			}
			if(!OK)
				continue;
			frqstorecnt = kk * LOCSPREAD;				//  Start position in frq-locations store
			frqloc[frqstorecnt++] = initvc;				//	Store this frq in the loudest-frq-locations store
			srchcnt = 1;								//	We now have 1 value for the location of this frq in the input buffer					
			vc = initvc - 2;							//	Location of frq below it in input buffer
			while(vc > 0) {								//  Don't search beyond bottom of buffer
				if(inbuf2[vc] > frqhere)				//	Compare previous inbuf frq with initial inbuf frq
					frqratio = inbuf2[vc]/frqhere;
				else
					frqratio = frqhere/inbuf2[vc];
				if(frqratio < EIGHT_OVER_SEVEN) {		// if "same" frq
					frqloc[frqstorecnt++] = vc;			//	Store this is an equivalent location of start frq
					srchcnt++;							//	increment count of stored locations
				} else									//	But if NOT same frq, assume ensuing frqs 
					break;								//	will be even further out of tune, and break.
				if(srchcnt >= LOCSPREAD)				//	Once we've filled all 4 other slots in the frq-locations store,
					break;								//	quit search	
				vc -= 2;								//	descend to next frq in input buffer
			}
			if(srchcnt < LOCSPREAD) {					//	If not already filled all storage frq-locations array spaces...
				vc = initvc + 2;						//	Location of frq above initial frq in input buffer
				while(vc < dz->wanted) {			//  Don't search beyond end of buffer
					if(inbuf2[vc] > frqhere)			//	Compare next inbuf frq with initial inbuf frq
						frqratio = inbuf2[vc]/frqhere;
					else
						frqratio = frqhere/inbuf2[vc];
					if(frqratio < EIGHT_OVER_SEVEN) {	//  if "same" frq
						frqloc[frqstorecnt++] = vc;		//	Store this as an equivalent location of start frq
						srchcnt++;
					} else								//	if it's not "same" frq, assume we're out of similar frqs
						break;							//	quit search
					if(srchcnt >= LOCSPREAD)			//	Once we've filled all 4 similar-frq locations, quit search	
						break;
					vc += 2;							//	ascend to next frq in input buffer
				}
				while(srchcnt < LOCSPREAD) {			//	If still not filled all available array spaces.. 
					frqloc[frqstorecnt++] = -1;			//	fill rest of similar-frq locations with -1 flags
					srchcnt++;
				}
			}				
		}
		frqsetcnt = frqstorecnt/LOCSPREAD;				//	No of sets of prominent frq-locations
	}
	for(cc = 0, vc = 0; cc < dz->clength; cc++, vc += 2) {
		frq = inbuf1[FREQ];
		amp = (float)(((1.0 - dz->param[0]) * inbuf2[AMPP]) + (dz->param[0] * inbuf1[AMPP]));
		inbuf1[AMPP] = amp;
		inbuf1[FREQ] = frq;
	}
	if(dz->param[1] > 0) {										//	Proportion of 2nd spectrum's most prominent frqs to impose on output
		dz->iparam[1] = (int)round(dz->param[1] * frqsetcnt);	//	Number of 2nd spectrum's most prominent frqs to impose on output
		for(kk = 0; kk < dz->iparam[1]; kk++) {					//	For each 2ndspectrum's most prominent frq
			frqstorecnt = kk * LOCSPREAD;
			for(jj = 0; jj < LOCSPREAD; jj++) {					//	Find their locations in the 2nd spectrums input data
				vc = frqloc[frqstorecnt++];		
				if(vc > 0)										// (and if data location is not flagged as empty)
					inbuf1[vc] = inbuf2[vc];					//	Impose that 2nd-sectra frq on to output spectrum
				else
					break;
			}	
		}
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

