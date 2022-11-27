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



/*********************************************
 *
 * THIS IS A MODEL for a program that USES THE CDP LIBRARIES
 *	but where those LIBS DO NOT NEED TO BE ALTERED.
 *
 *	CMDLINE must have STANDARD CDP FORM
 *
 *	ALL FUNCTIONS that would have needed CDP libraries to be updated
 *	are replaced by local functions in the code, to avoid recompiling CDP libs.
 *
 *	All lib functions that need to be satisfied, but are not actively used, are here as DUMMIES
 *
 *	cdparams_other() AND tkusage_other() NEED TO BE UPDATED
 *	standalone.h NEEDS TO BE UPDATED
 *	gobo AND gobosee NEED TO BE UPDATED
 *
 *********************************************
 *
 *	THIS PARTICULAR FUNCTION USES A NEW FILETYPE (multichannel mixfile)
 *	AND HENCE REQUIRED MODS TO cdparse() (IMPLEMENTED)
 *
 *********************************************
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

int anal_infiles = 0;
int	sloom = 0;
int sloombatch = 0;

const char* cdp_version = "7.1.0";

/* STRUCTURES SPECIFIC TO THIS APPLICATION */

struct actvalm {
int  ifd;				/* file from which to read samples */
int  bufno;				/* these must be assigned before we start using this struct */
int samplen;			/* number of samples produced from whole file */
int	inchans;			/* channels in infile */
int route[513];			/* This is the routing pairs from MAX_OUTCHANS inchans to MAX_OUTCHANS outchans */
double level[256];		/* this is weighting of each possible routing */
} ;					

typedef struct actvalm  *actvalmptr;

struct actionm {
int   	  position;  	/* time of action, in samples  				  */
actvalmptr val;	  		/* all associated values, stored in an actval */
int       role;	  		/* ON , REFILL, or OFF 						  */
} ;

typedef struct actionm  *actmptr;

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
static int  store_wordlist(char *filename,dataptr dz);

/* CDP LIB FUNCTION MODIFIED TO AVOID CALLING setup_particular_application() */

static int  parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);

/* SIMPLIFICATION OF LIB FUNC TO APPLY TO JUST THIS FUNCTION */

static int  parse_infile_and_check_type(char **cmdline,dataptr dz);
static int  handle_the_outfile(int *cmdlinecnt,char ***cmdline,int is_launched,dataptr dz);
static int  setup_newmix_application(dataptr dz);
static int  setup_newmix_param_ranges_and_defaults(dataptr dz);

/* BYPASS LIBRARY GLOBAL FUNCTION TO GO DIRECTLY TO SPECIFIC APPLIC FUNCTIONS */

static int	create_mix_buffers(dataptr dz);
static int	new_mix_preprocess(actmptr **act,dataptr dz);
static int	new_mmix(actmptr *act,dataptr dz);
static int	set_up_newmix(actmptr **act,dataptr dz);

/* FUNCTIONS SPECIFIC TO THIS APLIC */

static int  do_refill_acts(actmptr **act,dataptr dz);
static void sort_actions(actmptr **act,dataptr dz);
static int  init_inbufs(dataptr dz);

static int  establish_action_value_storage_for_mix(dataptr dz);
static int  open_file_and_get_props(int filecnt,char *filename,int *srate,int chans,dataptr dz);
static int  allocate_actions(actmptr **act,int filecnt,dataptr dz);
static int  unmark_freeable_bufs(actmptr **f,int **bufsflag,int *bufsflagcnt,int startpos,int this,int longbitsize,dataptr dz);
static int  get_free_buf(int **bufsflag,int *bufsflagcnt,int longbitsize,int *thisbuf,dataptr dz);
static int  unmark_buf(int **bufsflag,int *bufsflagcnt,int thisbuf,int longbitsize);
static int  mark_buf(int **bufsflag,int *bufsflagcnt,int thisbuf,int longbitsize,dataptr dz);
static int  reallocate_bufsflag(int z,int **bufsflag,int *bufsflagcnt);
static int  adjust_check_mixdata_in_line(int chans,double *level,int *route,int outchans,int entrycnt,int lineno,dataptr dz);
static int  get_newmixdata_in_line(int total_words,double *time,int *chans,
				double *level,int *route,int filecnt,int *entrycnt,dataptr dz);
static int  check_level(char *str,double *level,int lineno,int entrycnt);

static int  adjust_buffer_status(actmptr *act,int n,int *thispos,int *active_bufcnt,dataptr dz);
static int  read_samps_to_an_inbuf(actmptr *act,int n,dataptr dz);
static int  adjust_activebufs_list(int active_bufcnt,dataptr dz);
static int  do_mix(int samps_to_mix,int *position_in_outbuf,int active_bufcnt,int *outbuf_space,dataptr dz);
static int  do_silence(int samps_to_mix,int *position_in_outbuf,int *outbuf_space,dataptr dz);
static int  mix_read_samps(actmptr *act,float *inbuf,int samps_to_read,int n,dataptr dz);

static int IsNumeric(char *str);

static actvalmptr 	*valstor;
//static actmptr 		*act;

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
	int exit_status;
	dataptr dz = NULL;
	char **cmdline;
	int  cmdlinecnt;
	aplptr ap;
	int is_launched = FALSE;
	actmptr *act;
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

		if((exit_status = make_initial_cmdline_check(&argc,&argv))<0) {		// CDP LIB
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		cmdline    = argv;
		cmdlinecnt = argc;
		// THERE IS NO MODE WITH THIS PROGRAM
		if (!strcmp(argv[0],"multichan")) {		
			dz->process = MIXMULTI;
			dz->mode = 0;
		} else
			usage1();
		cmdline++;
		cmdlinecnt--;
		// setup_particular_application =
		if((exit_status = setup_newmix_application(dz))<0) {
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
	dz->itemcnt = dz->out_chans;
	// setup_param_ranges_and_defaults() =
	if((exit_status = setup_newmix_param_ranges_and_defaults(dz))<0) {
		exit_status = print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	//open_first_infile =
	if((exit_status = store_wordlist(cmdline[0],dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);	
		return(FAILED);
	}
	cmdlinecnt--;
	cmdline++;

//	handle_extra_infiles() : redundant
	// handle_outfile() = 
	if((exit_status = handle_the_outfile(&cmdlinecnt,&cmdline,is_launched,dz))<0) {
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
	//check_param_validity_and_consistency() = 
	if((exit_status = set_up_newmix(&act,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}

 	is_launched = TRUE;

	//allocate_large_buffers() =
	if((exit_status = create_mix_buffers(dz))<0){
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	//param_preprocess() =
	if((exit_status = new_mix_preprocess(&act,dz))<0) {	
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	//groucho_process_file =
	if((exit_status = new_mmix(act,dz))<0) {
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
	int tipc;
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
	//THERE ARE NO brktables USED IN THIS PROCESS
	if((storage_cnt = tipc + ap->internal_param_cnt)>0) {		  
		if((exit_status = setup_parameter_storage_and_constants(storage_cnt,dz))<0)	  
			return(exit_status);
		if((exit_status = initialise_is_int_and_no_brk_constants(storage_cnt,dz))<0)	  
			return(exit_status);
	}													   
 	if((exit_status = mark_parameter_types(dz,ap))<0)	  
			return(exit_status);
	
	// establish_infile_constants() replaced by
	dz->infilecnt = ONE_NONSND_FILE;
	//establish_bufptrs_and_extra_buffers(): not required, handled by set_up_newmix()
	// setup_internal_arrays_and_array_pointers(): not required
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

/***************************** HANDLE_THE_OUTFILE **************************/

int handle_the_outfile(int *cmdlinecnt,char ***cmdline,int is_launched,dataptr dz)
{
	char *filename = NULL;

	if(!sloom && (*cmdlinecnt<=0)) {
		sprintf(errstr,"Insufficient cmdline parameters.\n");
		return(USAGE_ONLY);
	}
	filename = (*cmdline)[0];
	if(filename[0]=='-' && filename[1]=='f') {
		dz->floatsam_output = 1;
		dz->true_outfile_stype = SAMP_FLOAT;
		filename+= 2;
	}
	strcpy(dz->outfilename,filename);	   
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

/************************* SETUP_NEWMIX_APPLICATION *******************/

int setup_newmix_application(dataptr dz)
{
	int exit_status;
	aplptr ap;
	if((exit_status = establish_application(dz))<0)		// GLOBAL
		return(FAILED);
	ap = dz->application;
	// SEE parstruct FOR EXPLANATION of next 2 functions
	if((exit_status = set_param_data(ap,0   ,0,0,""      ))<0)
		return(FAILED);
	if((exit_status = set_vflgs(ap,  "seg",3,"ddd"  ,""     ,0,0,""     ))<0)
		return(FAILED);
	// THERE IS NO NEED TO set_formant_flags in this case....
	// Following only needed if internal params are linked to dz structure
	if((exit_status = set_internalparam_data("iii",ap))<0)
		return(FAILED);
	// set_legal_infile_structure -->
	dz->has_otherfile = FALSE;
	// assign_process_logic -->
	dz->input_data_type = MIXFILES_ONLY;
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
			sprintf(errstr,"Failed tp parse input file %s\n",cmdline[0]);
			return(PROGRAM_ERROR);
		} else if(infile_info->filetype != MIX_MULTI)  {
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

/************************* SETUP_NEWMIX_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_newmix_param_ranges_and_defaults(dataptr dz)
{
	int exit_status;
	aplptr ap = dz->application;
	// set_param_ranges()
	ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
	// NB total_input_param_cnt is > 0 !!!s
	if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
		return(FAILED);
	// get_param_ranges()
	ap->lo[MIX_START]			= 0.0;
	ap->hi[MIX_START]			= dz->duration;
	ap->lo[MIX_END]				= 0.0;
	ap->hi[MIX_END]				= dz->duration;
	ap->lo[MIX_ATTEN]			= 0.0;
	ap->hi[MIX_ATTEN]			= 1.0;
	// initialise_param_values()
	ap->default_val[MIX_START]	= 0.0;
	ap->default_val[MIX_END]	= dz->duration;
	ap->default_val[MIX_ATTEN]	= 1.0;
	if(!sloom)
		put_default_vals_in_all_params(dz);
	return(FINISHED);
}

/************************* STORE_WORDLIST *****************************/

int store_wordlist(char *filename,dataptr dz)
{
	char temp[1000],*p,*q;
	int n;
	int total_wordcnt = 0;
	int wordcnt_in_line;
	int line_cnt = 0;
	FILE *fp;

	if((dz->wordstor = (char **)malloc(dz->all_words * sizeof(char *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for wordstores.\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<dz->all_words;n++)		/* initialise, for testing and safe freeing */
		dz->wordstor[n] = NULL;

	if((dz->wordcnt  = (int *)malloc(dz->linecnt * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for line wordcounts.\n");
		return(MEMORY_ERROR);
	}
	if((fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,"Failed to open file %s for input.\n",filename);
		return(DATA_ERROR);
	}
	while(fgets(temp,1000,fp)!=NULL) {
		p = temp;
		if(is_an_empty_line_or_a_comment(p))
			continue;
		wordcnt_in_line = 0;
		while(get_word_from_string(&p,&q)) {
			if((dz->wordstor[total_wordcnt] = (char *)malloc((strlen(q) + 1) * sizeof(char)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY for wordstor %d\n",total_wordcnt+1);
				return(MEMORY_ERROR);
			}
			strcpy(dz->wordstor[total_wordcnt],q);
			total_wordcnt++;
			wordcnt_in_line++;
		}
		dz->wordcnt[line_cnt] = wordcnt_in_line;
		line_cnt++;
	}
	if(dz->infile->filetype==SNDLIST) {		/* reorganise snds in sndlist onto separate lines */
		if(dz->linecnt != dz->all_words) {
		 	if((dz->wordcnt  = (int *)realloc(dz->wordcnt,dz->all_words * sizeof(int)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY to reallocate word counts.\n");
				return(MEMORY_ERROR);
			}
			dz->linecnt = dz->all_words;
			for(n=0;n<dz->linecnt;n++)
				dz->wordcnt[n] = 1;
		}
	}
	fclose(fp);
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
			if((exit_status = setup_newmix_application(dz))<0)
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

int get_process_no(char *prog_identifier_from_cmdline,dataptr dz)
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

/*************************** CREATE_MIX_BUFFERS **************************
 *
 *	Input files are read into a special buffer, then converted to the format
 *	of the outfile. This may involve an increase or decrease in the number of
 *	channels.
 *	Basic buffersize therefore needs an extra factor relating to the max number of channels
 *	found in both input files and output files.
 *	dz->itemcnt stores this value.
 *	There are .... 
 *	1)	One buffer to store any infile data before converting it to the outfile format.
 *	2)	a buffer for each infile AFTER conversion to outfile format...
 *		During the course of the mix, buffers no longer in use by any current infile, may be reused ....
 *		(the max number of buffers needed is counted before we get to this function,
 *		and the count stored in dz->bufcnt: (buffer management is described in act[]).
 *	3)	an output buffer,
 */

int create_mix_buffers(dataptr dz)
{
	int exit_status, ratio;
	size_t bigbufsize;
	size_t bloksize = SECSIZE * dz->out_chans;  		/* 1 */

	/* buffers for infiles have been counted while reading mixfile */
	dz->bufcnt++;						/* output buffer */
										/* if any infile has MORE chans than outfile, allow for this */
	ratio = (int)ceil((double)dz->itemcnt/(double)dz->out_chans);
	dz->bufcnt += ratio;				/* This defines size for a SINGLE (input-conversion) buffer */
										/* used for converting infile format to outfile format */
	bloksize = dz->bufcnt * bloksize;
	bigbufsize = (size_t)Malloc(-1);
	if((bigbufsize = (bigbufsize/bloksize) * bloksize) <= 0)
		bigbufsize = bloksize;
	if((dz->bigbuf = (float *)Malloc(bigbufsize)) == NULL) {
		sprintf(errstr, "INSUFFICIENT MEMORY to create sound buffers.\n");
		return(MEMORY_ERROR);
	}

	dz->buflen = (int)(bigbufsize/sizeof(float)/dz->bufcnt);	/* length of floats buffers */
	if((exit_status = establish_groucho_bufptrs_and_extra_buffers(dz))<0)
		return(exit_status);
	dz->sampbuf[OBUFMIX]   = dz->bigbuf;
	dz->sampbuf[STEREOBUF] = dz->bigbuf + dz->buflen;
								/* this defines size of input-conversion buffer */	
	dz->sampbuf[IBUFMIX]   = dz->sampbuf[STEREOBUF] + (dz->buflen * ratio);
								/* post-conversion input buffers start at IBUFMAX */
	dz->bufcnt -= (1 + ratio);	/* bufcnt becomes a count of post-conversion input-bufs only */
	return(FINISHED);
}

/******************************** USAGE1 ********************************/

int usage1(void)
{
	sprintf(errstr,
    "USAGE:	newmix multichan mixfile outsndfile [-sSTART] [-eEND] [-gATTENUATION]\n\n"
    "START       gives starttime START (to start mixing later than zero).\n"
    "END         gives output endtime END (to stop mix before its true end).\n"
	"Note that the START and END params are intended for mix TESTING purposes only.\n"
	"If you want to keep output from such a testmix, you should TOPNTAIL it.\n"
	"\n"
	"Mixfile for multichannel work is same as standard CDP mixfiles BUT...\n"
	"1) There is an extra initial line that states no. of output chans.\n"
	"2) On ensuing lines, input and output channels are numbered 1,2,3 etc\n"
	"3) Routing of input to output is indicated by inchan, colon, outchan\n"
	"   e.g. 1:4 sends input channel 1 to ouptut channel 4\n"
	"5) The levels on these channels are in the range -1 to 1\n"
	"6) An input channel must be routed to an output channel, with a level\n"
	"     e.g. 1:1 1.0 2:4 .5 (input 1 to output 1, input 2 to output 4)\n"
	"7) You can route an input to many outs e,g  1:1 .5 1:2 .3 1:4 .7 etc.\n"
	"8) You must take care with levels, where more than 1 input goes to same output.\n");
	return(USAGE_ONLY);
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
	return usage1();
}


/******************************** USAGE3 ********************************/

int usage3(char *str1,char *str2)
{	
	sprintf(errstr,"Insufficient parameters on command line.\n");
	return(USAGE_ONLY);
}

/************************** MIX_PREPROCESS ******************/

int new_mix_preprocess(actmptr **act,dataptr dz)
{
	int exit_status;
	fprintf(stdout,"INFO: Establishing mixing structures: This may take a few moments!!\n");
	fflush(stdout);
	if((exit_status = do_refill_acts(act,dz))<0)						
		return(exit_status);
	sort_actions(act,dz);			
	if(dz->iparam[MIX_STRTPOS] > 0)	   		/* Allow for mix not starting at zero time */
		dz->iparam[MIX_STRTPOS_IN_ACTION] = (int)max(dz->iparam[MIX_STRTPOS] - (*act)[0]->position,0L);  
	dz->tempsize = ((*act)[dz->iparam[MIX_TOTAL_ACTCNT]-1]->position - dz->iparam[MIX_STRTPOS_IN_ACTION]);
	return init_inbufs(dz);		
}

/*************************** DO_REFILL_ACTS **************************/

int do_refill_acts(actmptr **act,dataptr dz)
{
	int n;
	int samps_used, actcnt = dz->iparam[MIX_TOTAL_ACTCNT];
	int arraysize = actcnt + BIGARRAY;							/* Generate more space for further actions. */
	if((*act=(actmptr *)realloc(*act,arraysize * sizeof(actmptr)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to construct buffer-fill action pointers.\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<dz->iparam[MIX_TOTAL_ACTCNT];n+=2) {				/* actions paired as ON/OFF, so we look only at ON bufs */
		if((*act)[n]->val->samplen > dz->buflen) {				/* If more data in (action's) file than fits in 1 buffer */
			samps_used = 0;		
			while((samps_used += dz->buflen) < (*act)[n]->val->samplen) {
				if(((*act)[actcnt] = (actmptr)malloc(sizeof(struct actionm)))==NULL) {
					sprintf(errstr,"INSUFFICIENT MEMORY to construct buffer-fill actions.\n");
					return(MEMORY_ERROR);
				}
				(*act)[actcnt]->val      = (*act)[n]->val;	/* Create a new action, using same vals as the original */
				(*act)[actcnt]->position = (*act)[n]->position + samps_used;/* Positioned 1 further buflen later, and */
				(*act)[actcnt]->role     = MIX_ACTION_REFILL;	/* with role REFILL */
				if(++actcnt >= arraysize) {
					arraysize += BIGARRAY;
					if((*act=(actmptr *)realloc(*act,arraysize * sizeof(actmptr)))==NULL) {
						sprintf(errstr,"INSUFFICIENT MEMORY to reallocate buffer-fill action pointers.\n");
						return(MEMORY_ERROR);
					}
				}
			}
		}
	}
	if((*act=(actmptr *)realloc(*act,actcnt * sizeof(actmptr)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to reallocate buffer-fill actions.\n");
		return(MEMORY_ERROR);
	}
	dz->iparam[MIX_TOTAL_ACTCNT] = (int)actcnt;
	return(FINISHED);
}

/************************* SORT_ACTIONS ****************************/

void sort_actions(actmptr **act,dataptr dz)
{
	actmptr temp;   
	int n, m;
	for(n=0;n<dz->iparam[MIX_TOTAL_ACTCNT]-1;n++) {
		for(m=n+1;m<dz->iparam[MIX_TOTAL_ACTCNT];m++) {
			if((*act)[m]->position < (*act)[n]->position) {
				temp   = (*act)[n];
				(*act)[n] = (*act)[m];	
				(*act)[m] = temp;
			}
		}
	}
}

/****************************** INIT_INBUFS *********************************
 *
 * (1)	Create the space for buffers within each bufitem. NB enough space
 * 		for ALL these buffers must be allocated in the initial creation
 * 		of 'inbuf' in create_buffers.
 * (2)	Create the pointers to point to start and current_position in ACTIVE bufs.
 */

int init_inbufs(dataptr dz)
{
	int n;
	float *thisbuf = dz->sampbuf[IBUFMIX];
	dz->buflist = (mixbufptr *)malloc(dz->bufcnt * sizeof(mixbufptr));
	for(n=0;n<dz->bufcnt;n++) {
		dz->buflist[n] = NULL;
		if((dz->buflist[n] = (mixbufptr)malloc(sizeof(struct bufitem)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for buffer list.\n");
			return(MEMORY_ERROR);
		}
		dz->buflist[n]->status = MIX_ACTION_OFF;
		dz->buflist[n]->buf    = thisbuf;
		dz->buflist[n]->here   = dz->buflist[n]->buf;
		thisbuf += dz->buflen;
	}
	if((dz->activebuf = (int *)malloc(dz->bufcnt * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for active buffer list.\n");
		return(MEMORY_ERROR);
	}
	if((dz->activebuf_ptr = (float **)malloc(dz->bufcnt * sizeof(float *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for active buffer pointer list.\n");
		return(MEMORY_ERROR);
	}
	return(FINISHED);
}

/********************** SET_UP_MIX ****************************/
 
int set_up_newmix(actmptr **act,dataptr dz)
{
	int exit_status;
	double time, level[256];
	int    chans;
	int route[513], k;
	int   startpos, endpos, lineno, j;
	int *bufsflag = NULL;
	int bufsflagcnt = 0;
	double filetime, eventend;
	int longbitsize = sizeof(int) * CHARBITSIZE;
	int srate=0, end_of_mix = -1;
	int mix_end_set   = FALSE;
	int mix_end_specified = FALSE;
	int mix_start_set = FALSE;
	char *filename;
	int filecnt = 0, entrycnt = 0;												
	int total_words = 0;
	int outchans = dz->out_chans;

	if(dz->param[MIX_START] >= dz->param[MIX_END]) {
		sprintf(errstr,"Mix starts after it ends.\n");
		return(USER_ERROR);
	}
	dz->iparam[MIX_STRTPOS] 		  = 0;
	dz->iparam[MIX_STRTPOS_IN_ACTION] = 0;
	dz->iparam[MIX_TOTAL_ACTCNT]      = 0;
	dz->bufcnt = 0;		/* buffers are allocated in "mark_buf" */
	if((exit_status = establish_file_data_storage_for_mix(dz->linecnt,dz))<0)
		return(exit_status);
	if((exit_status = establish_action_value_storage_for_mix(dz))<0)
		return(exit_status);

	total_words++;										/* 1st word = first line = outchan count */
	for(lineno=1;lineno<dz->linecnt;lineno++) {			/* for each mixfile line */
		memset((char *)route,0,513 * sizeof(int));
		filename = dz->wordstor[total_words];			/* get the filename */
		if(strlen(filename)<=0) {
			sprintf(errstr,"filename error: line %d: set_up_newmix()\n",lineno+1);
			return(PROGRAM_ERROR);
		}
		if((exit_status = get_newmixdata_in_line(total_words,&time,&chans,&(level[0]),&(route[0]),lineno,&entrycnt,dz))<0)
			return(exit_status);						/* read the mixfile line data */			
		if(chans > dz->itemcnt)
			dz->itemcnt = chans;
		total_words += dz->wordcnt[lineno];				/* tally of all words in file, acts as base to measure start of next line */
		if(time >= dz->param[MIX_END])					/* If line starts after specified mix end, ignore */
			continue;
		if((exit_status = 
		adjust_check_mixdata_in_line(chans,&(level[0]),&(route[0]),dz->out_chans,entrycnt,lineno,dz))<0)
			return(exit_status);						/* do any attenuation required by input para, */
														/* & check routing compatible with infile and outfile channel counts */
		if((exit_status = open_file_and_get_props(filecnt,filename,&srate,chans,dz))<0)
			return(exit_status);						/* check infile specified exists, is a sndfile, contains data, */
														/* has channel count specified in mix line */
														/* and has srate same as all other soundfiles in mix */

		filetime = (double)(dz->insams[filecnt]/chans)/(double)srate;
		if((eventend = time + filetime) < dz->param[MIX_START])	/* If line ends before specified mix start, ignore */
			continue;
		if(!mix_start_set) {							/* if mix start param not set yet, set it */
			dz->iparam[MIX_STRTPOS]           = (int)round(dz->param[MIX_START] * (double)(srate * dz->out_chans));
			dz->iparam[MIX_STRTPOS_IN_ACTION] = dz->iparam[MIX_STRTPOS];
			mix_start_set = TRUE;
		}
		if(!mix_end_set) {								/* if mix end param not set yet, set it */
			if(!flteq(dz->param[MIX_END],dz->application->default_val[MIX_END])) {	 /* if mixend param given by user */
				end_of_mix = round(dz->param[MIX_END] * (double)srate) * dz->out_chans;
				if(end_of_mix < 0) {
					sprintf(errstr,"Error in end_of_mix logic: set_up_newmix()\n");
					return(PROGRAM_ERROR);
				}
				mix_end_specified = TRUE;
			}
			mix_end_set = TRUE;
		}
		if((exit_status = allocate_actions(act,filecnt,dz))<0)	/* assign values to mix ON and OFF actions */
			return(exit_status);

		startpos = round(time * (double)srate);				/* startposition (in grouped samples), of this file */
		startpos             *= outchans;					/* startposition, in output samples, of this file */
		dz->insams[filecnt]  /= chans;						/* length of input file, in grouped-samples */
		dz->insams[filecnt]  *= outchans;					/* EFFECTIVE sample-dur of file in output */

		if((exit_status = unmark_freeable_bufs				/* finds buffers which are not (any longer) in use */
		(act,&bufsflag,&bufsflagcnt,startpos,dz->iparam[MIX_TOTAL_ACTCNT],longbitsize,dz))<0)
			return(exit_status);
		if((exit_status = get_free_buf(&bufsflag,&bufsflagcnt,longbitsize,&(valstor[filecnt]->bufno),dz))<0) 	
			return(exit_status);	  						/* Allocate a buffer (for htis file) which is not currently in use */	

		endpos =  startpos + dz->insams[filecnt];			/* Find end-of-current-file in output stream */
		if(mix_end_specified && (end_of_mix < endpos)) {	/* If file ends aftert mix ends */
			endpos  = end_of_mix;							/* curtail (effective) length */
			dz->insams[filecnt] = end_of_mix - startpos;
		}
		valstor[filecnt]->samplen = dz->insams[filecnt]; 	/* store (effective) length */
		valstor[filecnt]->inchans = chans;				 	/* store infile channel-cnt */
		for(j=0;j<entrycnt;j++)	{
			k = j * 2;
			(valstor[filecnt])->route[k] = route[k];		/* store the in->out channel routing for this file */
			k++;
			(valstor[filecnt])->route[k] = route[k];		/* store the in->out channel routing for this file */
			(valstor[filecnt]->level)[j] = level[j];		/* store the levels for each channel routing, for this file */
		}
		valstor[filecnt]->ifd  	= dz->ifd[filecnt];			/* store file-pointer to input file */

		(*act)[dz->iparam[MIX_TOTAL_ACTCNT]++]->position = startpos;	/* store outputstream-position of mix start */
		(*act)[dz->iparam[MIX_TOTAL_ACTCNT]++]->position = endpos;		/* store outputstream-position of mix end   */

		filecnt++;											/* count the ACTUALLY USED lines, for which files have been opened */
	}
	if(!mix_end_set || filecnt==0) {
		sprintf(errstr,"No mixfile line is active within the time limits specified.\n");
		return(DATA_ERROR);
	}
	if(bufsflagcnt)
		free(bufsflag);
    dz->bufcnt++;		/* bufcnt is number assigned to highest assigned buf, COUNTING FROM ZERO */
						/* Hence, actual bufcnt is 1 more than this */

	dz->infile->channels = dz->out_chans;	/* output channels(evenutally derived from dz->infile->channels) */
//TW SET UP AFTER OUTCHANS KNOWN
	if((exit_status = create_sized_outfile(dz->outfilename,dz))<0)
		return(exit_status);
	return(FINISHED);
}

/**************************** ESTABLISH_FILE_DATA_STORAGE_FOR_MIX ********************************/

int establish_file_data_storage_for_mix(int filecnt,dataptr dz)
{
	int n;
	if(dz->insams!=NULL)		
		free(dz->insams);	 	/* in TK insams[0] also used in parse accounting */
	if(dz->ifd!=NULL)
		free(dz->ifd);
	if((dz->insams = (int *)malloc(filecnt * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to allocate infile samplesize array.\n");
		return(MEMORY_ERROR);
	}
	if((dz->ifd = (int  *)malloc(filecnt * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to allocate infile pointers array.\n");		   	
		return(MEMORY_ERROR);
	}
	for(n=0;n<filecnt;n++)
		dz->ifd[n] = -1;
	return(FINISHED);
}

/**************************** ESTABLISH_ACTION_VALUE_STORAGE_FOR_MIX ********************************/

int establish_action_value_storage_for_mix(dataptr dz)
{
	int n;
// Comment: valstor is declarted as an "actvalptr", but this is just an address
// so we're using it to point to an "actvalmptr"
	if((valstor  = (actvalmptr *)malloc(dz->linecnt * sizeof(actvalmptr)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for mix action values store.\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<dz->linecnt;n++) {
		valstor[n] = NULL;	
        /* RWD 4 Dec 2010 the usual uninitialised data again.... */
		if((valstor[n]=(actvalmptr) calloc(1,sizeof(struct actvalm)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for mix action value store %d\n",n+1);				
			return(MEMORY_ERROR);
		}
	}
	return(FINISHED);
}

/**************************** GET_MIXDATA_IN_LINE ********************************/

int get_newmixdata_in_line(int total_words,double *time,int *chans,
double *level,int *route,int lineno,int *entrycnt,dataptr dz)
{
	int exit_status, here, is_level, n, test;
	int wordcnt = dz->wordcnt[lineno], got_colon, in_chan, out_chan;
	char temp[2000];
	char *p;
	char *q, *z;
	double thislevel;
	z = 0;			// non-valid inital value: z is address of 1st char of out_chan in an in-chan_out-chan pair e.g. address of '6' in '3:6'
	in_chan = 0;	// non-valid inital value: in_chan is the input channel in an in-chan_out-chan pair e.g. 3 in '3:6'
	test = wordcnt - 3;
	if((test <= 0) || ODD(test)) {
		here = total_words;
		strcpy(temp,dz->wordstor[here]);
		sprintf(errstr,"Wrong number of entries on line %d : %s\n",lineno+1,temp);
		return(PROGRAM_ERROR);
	}
	if(sscanf(dz->wordstor[total_words+1],"%lf",time)!=1
	|| sscanf(dz->wordstor[total_words+2],"%d",chans)!=1) {
		sprintf(errstr,"Error scanning data: line %d %s %s\n",lineno+1,dz->wordstor[total_words+1],dz->wordstor[total_words+2]);
		return(PROGRAM_ERROR);
	}
	here = total_words+3;
	is_level = 0;
	*entrycnt = 0;
	while(here < total_words + wordcnt) {
		if(*entrycnt >= 256) {
			sprintf(errstr,"Too many channel routing columns in line %d\n",lineno+1);
			return(DATA_ERROR);
		}
		if(is_level) {
			if((exit_status = check_level(dz->wordstor[here],&thislevel,lineno,*entrycnt))<0)
				return(exit_status);
			*(level + *entrycnt) = thislevel;
		} else {
			p = dz->wordstor[here];
			q = p + strlen(dz->wordstor[here]);
			got_colon = 0;
			while(p < q) {
				if(!isdigit(*p)) {
					if(p == dz->wordstor[here])
						return(DATA_ERROR);
					else if(got_colon)
						return(DATA_ERROR);
					else if(*p != ':')
						return(DATA_ERROR);
					else {
						*p = ENDOFSTR;
						in_chan = atoi(dz->wordstor[here]);
						*p = ':';
						z = p+1;
						got_colon = 1;
					}
				}
				p++;
			}
			if(!got_colon || z == p)
				return(DATA_ERROR);
			out_chan = atoi(z);
			if(in_chan > *chans || in_chan < 1)
				return(DATA_ERROR);
			if(out_chan > dz->out_chans)
				return(DATA_ERROR);
			n = (*entrycnt) * 2;
			route[n++] = in_chan;
			route[n]   = out_chan;
		}
		if(is_level)
			(*entrycnt)++;
		is_level = !is_level;
		here++;
	}
	return(FINISHED);
}

/**************************** ADJUST_CHECK_MIXDATA_IN_LINE ********************************/

int  adjust_check_mixdata_in_line(int chans,double *level,int *route,int outchans,int entrycnt,int lineno,dataptr dz)
{
	int len = entrycnt * 2, n, inchanroute, outchanroute;
	if(chans > MAX_OUTCHAN) {
		sprintf(errstr,"channel count %d greater than maximum (%d): line %d\n",chans,MAX_OUTCHAN,lineno+1);
		return(DATA_ERROR);
	}
	if(!(flteq(dz->param[MIX_ATTEN],1.0))) {		/* do any pre-attenuation of the entire mix, specified by atten parameter */
		for(n=0;n<entrycnt;n++)
			level[n] *= dz->param[MIX_ATTEN];
	}
	for(n = 0;n < len; n+=2) {						/* check compatibility routing mnemonics with channel cnt of infile & outfile */
		inchanroute = route[n];
		outchanroute = route[n+1];
		if(inchanroute > chans) {
			sprintf(errstr,
			"channel routing %d:%d uses input channel %d: there are only %d input channels: line %d\n",
			inchanroute,outchanroute,inchanroute,chans,lineno+1);
			return(DATA_ERROR);
		}
		if(outchanroute > outchans) {
			sprintf(errstr,
			"output channel count %d (implied by route %d:%d) greater than actual output channels (%d): line %d\n",
			outchanroute,inchanroute,outchanroute,dz->out_chans,lineno+1);
			return(DATA_ERROR);
		}
	}
	return(FINISHED);
}

/************************* OPEN_FILE_AND_GET_PROPS *******************************/

int open_file_and_get_props(int filecnt,char *filename,int *srate,int chans,dataptr dz)
{
	int exit_status;
	double maxamp, maxloc;
	int maxrep;
	int getmax = 0, getmaxinfo = 0;
	infileptr ifp;
	if((ifp = (infileptr)malloc(sizeof(struct filedata)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store data on files.\n");
		return(MEMORY_ERROR);
	}
	if((dz->ifd[filecnt] = sndopenEx(filename,0,CDP_OPEN_RDONLY)) < 0) {
		sprintf(errstr,"Failed to open sndfile %s\n",filename);
		return(SYSTEM_ERROR);
	}
	if((exit_status = readhead(ifp,dz->ifd[filecnt],filename,&maxamp,&maxloc,&maxrep,getmax,getmaxinfo))<0)
		return(exit_status);
	copy_to_fileptr(ifp,dz->infile);
	if(dz->infile->filetype!=SNDFILE) {
		sprintf(errstr,"Non soundfile encountered [%s]: open_file_and_get_props()\n",filename);
		return(PROGRAM_ERROR);
	}
	if(dz->infile->channels!=chans) {
		sprintf(errstr,"Incorrect channels found [%s]: open_file_and_get_props()\n",filename);
		return(PROGRAM_ERROR);
	}
	if(filecnt==0)
		*srate = dz->infile->srate;
	else if(dz->infile->srate != *srate) {
		sprintf(errstr,"incompatible srates: [file %s] open_file_and_get_props()\n",filename);
		return(PROGRAM_ERROR);
	}
	if((dz->insams[filecnt] = sndsizeEx(dz->ifd[filecnt]))<0) {	    			
		sprintf(errstr, "Can't read size of input file %s: open_file_and_get_props()\n",filename);
		return(PROGRAM_ERROR);
	}
	if(dz->insams[filecnt] <=0) {
		sprintf(errstr, "Zero size for input file %s: open_file_and_get_props()\n",filename);
		return(PROGRAM_ERROR);
	}			
	return(FINISHED);
}

/*********************** ALLOCATE_ACTIONS *************************/

int allocate_actions(actmptr **act,int filecnt,dataptr dz)
{
	int new_total = dz->iparam[MIX_TOTAL_ACTCNT] + 2;
	if(dz->iparam[MIX_TOTAL_ACTCNT]==0) {
		if((*act = (actmptr *)malloc(new_total * sizeof(actmptr)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for mix actions store.\n");
			return(MEMORY_ERROR);
		}
	} else {
		if((*act=(actmptr *)realloc(*act,new_total * sizeof(actmptr)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY ro reallocate mix actions store.\n");
			return(MEMORY_ERROR);
		}
	}
	if(((*act)[dz->iparam[MIX_TOTAL_ACTCNT]] = (actmptr)malloc(sizeof(struct actionm)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for final mix action store.\n");
		return(MEMORY_ERROR);
	}
	(*act)[dz->iparam[MIX_TOTAL_ACTCNT]]->val    = valstor[filecnt];
	(*act)[dz->iparam[MIX_TOTAL_ACTCNT]]->role   = MIX_ACTION_ON;
	if(((*act)[dz->iparam[MIX_TOTAL_ACTCNT]+1] = (actmptr)malloc(sizeof(struct actionm)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for further mix action store.\n");
		return(MEMORY_ERROR);
	}
	(*act)[dz->iparam[MIX_TOTAL_ACTCNT]+1]->val  = valstor[filecnt];
	(*act)[dz->iparam[MIX_TOTAL_ACTCNT]+1]->role = MIX_ACTION_OFF;
	return(FINISHED);
}

/************************* UNMARK_FREEABLE_BUFS ***********************
 *
 * (1)	If a buffer has been switched off BEFORE now, then it is
 *		available for use!! unmark it!!
 * (2)	If a buffer is subsequently turned back on, this catches it!!
 *		A buffer can ONLY be turned back on LATER (!), and is hence
 *		LATER in this list EVEN though it is not yet fully time-sorted!!  (1998 ???)
 */

int unmark_freeable_bufs(actmptr **act,int **bufsflag,int *bufsflagcnt,int startpos,int this,int longbitsize,dataptr dz)
{
	int exit_status;
	int n;
	for(n=0;n<this;n++) {
		switch((*act)[n]->role) {
		case(MIX_ACTION_ON):				/* 2 */
			if((exit_status = mark_buf(bufsflag,bufsflagcnt,(*act)[n]->val->bufno,longbitsize,dz))<0)
				return(exit_status);
			break;
		case(MIX_ACTION_OFF):				/* 1 */
			if((*act)[n]->position < startpos)	{
				if((exit_status = unmark_buf(bufsflag,bufsflagcnt,(*act)[n]->val->bufno,longbitsize))<0)
					return(exit_status);
			}
			break;
		}
	}
	return(FINISHED);
}

/************************** GET_FREE_BUF ***************************
 *
 * Get the FIRST available free buffer.
 * (1)	Going through each long.
 * (2)	Set the mask to start of long.
 * (3)	For each position in the long..
 * (4)	If that byte is NOT set, break, thisbuf counts which byte
 *	and therefore which buffer it is.
 * (5)	Advance the mask.
 * (6)	Advance the buffer counter.
 * (7)	Set the appropriate bit for this buffer, return buffer no.
 */

int get_free_buf(int **bufsflag,int *bufsflagcnt,int longbitsize,int *thisbuf,dataptr dz)
{
	int exit_status;
	int y, z, got_it = 0;
	int mask;
	*thisbuf = 0;
	for(z=0;z<*bufsflagcnt;z++) {			/* 1 */
		mask = 1;	    					/* 2 */
		for(y=0;y<longbitsize;y++) {		/* 3 */
			if(!(mask & (*bufsflag)[z])) {	/* 4 */
				got_it = 1;
				break;
			}
			mask <<= 1;						/* 5 */
			(*thisbuf)++;					/* 6 */
		}
		if(got_it)
			break;
	}
	if((exit_status = mark_buf(bufsflag,bufsflagcnt,*thisbuf,longbitsize,dz))<0)	/* 7 */
		return(exit_status);
	return(FINISHED);
}

/************************** CHECK_LEVEL ************************/

int check_level(char *str,double *level,int lineno,int entrycnt)
{
	if(is_dB(str)) {
		if(!get_leveldb(str,level)) {
			sprintf(errstr,"Failed to find (dB) level %d: line %d\n",entrycnt+1,lineno+1);
			return(PROGRAM_ERROR);
		}		
	} else if(!(IsNumeric(str))) {
		sprintf(errstr,"Failed to find level %d: line %d\n",entrycnt+1,lineno+1);
		return(DATA_ERROR);
	} else {
		if(sscanf(str,"%lf",level)!=1) {
			sprintf(errstr,"Failed to find level %d: line %d\n",entrycnt+1,lineno+1);
			return(PROGRAM_ERROR);
		}
	}
// FEB 2007
//	if(*level < 0.0) {
//		sprintf(errstr,"Level %d less than 0.0: line %d\n",entrycnt+1,lineno+1);
//		return(PROGRAM_ERROR);
//	}
	return(FINISHED);
}

/************************** UNMARK_BUF ***************************/

int unmark_buf(int **bufsflag,int *bufsflagcnt,int thisbuf,int longbitsize)
{
	int exit_status;
	int mask = 1;	    
	int z = thisbuf/longbitsize;
/* 1998 --> */
	if(z >= *bufsflagcnt) {
		if((exit_status = reallocate_bufsflag(z,bufsflag,bufsflagcnt))<0)
			return(exit_status);
	}
/* <-- 1998 */
	thisbuf -= (z * longbitsize);
	mask <<= thisbuf;
	mask = ~mask;
	(*bufsflag)[z] &= mask;
	return(FINISHED);
}

/************************** MARK_BUF ***************************
 *
 * (1)	Which int is the appropriate byte in.
 * (2)	What is it's index into this int.
 * (3)	Move mask aint the int.
 * (4)	Set bit.
 * (5)	Keep note of max number of bufs in use.
 */

int mark_buf(int **bufsflag,int *bufsflagcnt,int thisbuf,int longbitsize,dataptr dz)
{
	int exit_status;
	int mask = 1;    
	int z = thisbuf/longbitsize;	/* 1 */
/* 1998 --> */
	if(z >= *bufsflagcnt) {
		if((exit_status = reallocate_bufsflag(z,bufsflag,bufsflagcnt))<0)
			return(exit_status);
	}
/* <-- 1998 */
	if(thisbuf > dz->bufcnt)
		dz->bufcnt = thisbuf;
	thisbuf -= (z * longbitsize);	/* 2 */
	mask <<= thisbuf;				/* 3 */
	(*bufsflag)[z] |= mask;			/* 4 */
	return(FINISHED);
}

/**************************** REALLOCATE_BUFSFLAG ********************************/

int reallocate_bufsflag(int z,int **bufsflag,int *bufsflagcnt)
{
	int n;
	if(*bufsflagcnt==0) 
		*bufsflag = (int *)malloc((z+1) * sizeof(int));
	else if((*bufsflag = (int *)realloc(*bufsflag,(z+1) * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for buffer flags store.\n");
		return(MEMORY_ERROR);
	}
	for(n=*bufsflagcnt;n<z;n++)
		*bufsflag[n] = 0;		
	*bufsflagcnt = z+1;
	return(FINISHED);
}

/*************************** NEW_MMIX *******************************/

int new_mmix(actmptr *act,dataptr dz)
{
	int exit_status;
	int n;
	int thispos, nextpos, samps_to_mix;
	int position_in_outbuf = 0;
	int active_bufcnt = 0;
	int outbuf_space = dz->buflen; /* position in the mix is measured in output samples. */
	fprintf(stdout,"INFO: Mixing the sounds.\n");
	fflush(stdout);
	display_virtual_time(0L,dz);
	if(sloom) {
		if(dz->iparam[MIX_STRTPOS_IN_ACTION] > 0) {
			fprintf(stdout,"INFO: Skipping initial part of mix.\n");
			fflush(stdout);
		}
	}
	for(n=0;n<dz->iparam[MIX_TOTAL_ACTCNT]-1;n++) {	/* Switch bufs ON or OFF as indicated in act[], & get position */
		if((exit_status = adjust_buffer_status(act,n,&thispos,&active_bufcnt,dz))<0)
			return(exit_status);					/* and while doing so, count active buffers. */

		nextpos = (act[n+1])->position;				/* Get next position, (from which to calc dur of this mix-chunk) */
		if((exit_status = adjust_activebufs_list(active_bufcnt,dz))<0)
			return(exit_status);					/* Ensure only pointers in ACTIVE bufs are in buf-pointer list */
		if(dz->iparam[MIX_STRTPOS] > 0) {			/* If mix does not start at zero */ 
			if(nextpos <= dz->iparam[MIX_STRTPOS]){	/* update MIX_STRTPOS_IN_ACTION */
				dz->iparam[MIX_STRTPOS_IN_ACTION] = (int)(dz->iparam[MIX_STRTPOS_IN_ACTION] - (nextpos - thispos));	
				continue;							/* and skip action until we reach a valid mix-action */
			}
		}											/* If we're in a valid mix action */
 													/* i.e. there is >zero time between this action and next */
													/* AND time is later than time where mix-action starts */
		if((samps_to_mix = nextpos - thispos - dz->iparam[MIX_STRTPOS_IN_ACTION])>0) {
			if(active_bufcnt==0) {					/* If no buffers are active, fill time with silence. */
				if((exit_status = do_silence(samps_to_mix,&position_in_outbuf,&outbuf_space,dz))<0)
					return(exit_status);
			} else {		   						/* Else, do mix */
				if((exit_status = do_mix(samps_to_mix,&position_in_outbuf,active_bufcnt,&outbuf_space,dz))<0)
					return(exit_status);
													/* Having got to start of actual mixing, */
			}										/* MIX_STRTPOS_IN_ACTION is set to zero  */
 													/* and NO LONGER  affects calculations.  */
			dz->iparam[MIX_STRTPOS_IN_ACTION] = dz->iparam[MIX_STRTPOS]  = 0;
		}
	}												/* Write any data remaining in output buffer. */
	if(position_in_outbuf > 0) 
		return write_samps(dz->sampbuf[OBUFMIX],position_in_outbuf,dz);	
	return FINISHED;
}

/************************** ADJUST_BUFFER_STATUS **************************
 *
 * 	And keep track of number of active buffers (active_bufcnt).
 */

int adjust_buffer_status(actmptr *act,int n,int *thispos,int *active_bufcnt,dataptr dz)
{
	int exit_status;
	switch(act[n]->role) {
	case(MIX_ACTION_ON):									/* buffer is being used for 1st time in this mix-action */
		(*active_bufcnt)++;									/* Increment the count of ACTIVE buffers */
		dz->buflist[act[n]->val->bufno]->status = MIX_ACTION_ON;	/* Change buffer status to ON */
		/* fall thro */
	case(MIX_ACTION_REFILL):								/* buffer is being reused OR used for 1st time */
		if((exit_status = read_samps_to_an_inbuf(act,n,dz))<0)	/* Read_samples into buffer */
			return(exit_status);
		dz->buflist[act[n]->val->bufno]->here = dz->buflist[act[n]->val->bufno]->buf;
		break;												/* Reset location_in_buffer_pointer (here) to buffer start */

	case(MIX_ACTION_OFF):									/* buffer is finished with */
		dz->buflist[act[n]->val->bufno]->status = MIX_ACTION_OFF;	/* Change buffer status to OFF */
		(*active_bufcnt)--;									/* Decrement the number of ACTIVE buffers */
		break;
	default:
		sprintf(errstr,"Unknown case in adjust_buffer_status()\n");
//TEST ONLY
// RWD for 64bit arch, need to use size_t to convert pointer!
		sprintf(errstr,"adjust_buffer_status()\nact[%d] = %d act = %d\n",n,(int)(size_t)act[n],(int)(size_t)act);
		return(PROGRAM_ERROR);
	}
	*thispos = act[n]->position; 							/* Return position (starttime-in-samps) of the action */
	return(FINISHED);
}

/************************* READ_SAMPS_TO_AN_INBUF ***************************/

int read_samps_to_an_inbuf(actmptr *act,int n,dataptr dz)
{
	int exit_status;
	int j, m, inpos, outpos, inchan, outchan;
	double level;
	float *inbuf = dz->buflist[act[n]->val->bufno]->buf;
	int *p;
	int  samps_to_read = (dz->buflen/dz->out_chans) * act[n]->val->inchans;
		/* dz->buflen = size of output buffer */
		/* STEREOBUF size is calculated to allow for max channel-cnt of infiles, */
		/* even if some input files have MORE chans than output. */
		/* If the input file has LESS chans than output, */
		/* less than 'buflen' gets read, and 'STEREOBUF' gets expanded into inbuf */
		/* if infile has MORE chans than output, */
		/* more than 'buflen' gets read, and 'STEREOBUF' gets contracted into inbuf */

	if((exit_status = mix_read_samps(act,dz->sampbuf[STEREOBUF],samps_to_read,n,dz))<0)
		return(exit_status);
	memset((char *)inbuf,0,dz->buflen * sizeof(float));
	for(j = 0; j < 256; j++) {
		m = j * 2;
		p = act[n]->val->route + m; 
		if(*p == 0)
			break;
		inchan = (*p - 1);
		p++;
		outchan = (*p - 1);
		level = *(act[n]->val->level + j); 
		/* Unconverted infile is stored in STEREOBUF : we convert it to outfile format .....*/
		/* Starting at infile-channel 'inpos' step through infile by infile-chancnt 'act[n]->val->inchans' */
		/* scaling the value by 'level', and adding it into the outbuf-channel-format 'inbuf' */
		/* Write to outbuf at outfile-channel 'outchan' and stepping by outfile chancnt 'dz->out_chans' */
		for(inpos=inchan,outpos=outchan;inpos<dz->ssampsread;inpos+=act[n]->val->inchans,outpos+=dz->out_chans)
			inbuf[outpos] = (float)(inbuf[outpos] + (dz->sampbuf[STEREOBUF][inpos] * level));
	}
	return(FINISHED);
}

/*********************** ADJUST_ACTIVEBUFS_LIST **************************
 *
 * Set the activebuf pointers to point only to the active buffers!!
 * NB active_bufcnt has been reset by adjust_buffer_status()
 */

int adjust_activebufs_list(int active_bufcnt,dataptr dz)
{   
	int n, k = 0;
	for(n=0;n<dz->bufcnt;n++) {
		if(dz->buflist[n]->status == MIX_ACTION_ON) {
			dz->activebuf[k] = n;
			dz->activebuf_ptr[k++] = dz->buflist[n]->here;
		}
	}
	if(k>active_bufcnt) {
		sprintf(errstr,"Accounting error: adjust_activebufs_list()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/************************* DO_MIX **********************************
 *
 * This algorithm puts all the samples from buffer 1 into output, then
 * sums in all the samples form buffer 2, and so on.
 */

int do_mix(int samps_to_mix,int *position_in_outbuf,int active_bufcnt,int *outbuf_space,dataptr dz)
{
	int exit_status;
	int  opos = *position_in_outbuf;
	int   m, start = opos;
	int  n, overflow;
	float *obuf = dz->sampbuf[OBUFMIX];
	float *outptr = obuf+opos;

	if(dz->iparam[MIX_STRTPOS] > 0) {			/* If mix doesn't start at mixlist start, */						
		for(n=0;n<active_bufcnt;n++)			/* and hence most likely not at buffer start, */
			dz->activebuf_ptr[n] += dz->iparam[MIX_STRTPOS_IN_ACTION];
	} 											/* increment pointers in all active_bufs to actual mix start */
	if((overflow = samps_to_mix - (*outbuf_space)) <= 0) {	/* If samps_to_write DOESN'T overflow output buffer */
	
		memmove((char *)outptr,(char *)(dz->activebuf_ptr[0]),samps_to_mix  * sizeof(float));
		dz->activebuf_ptr[0] += samps_to_mix;				/* Increment pointer in this active buf */
		dz->buflist[dz->activebuf[0]]->here += samps_to_mix;/* Increment current_position_in_buffer_pointer also */
		opos += samps_to_mix;								/* Increment position_in_outbuf */
								 							
		for(m=1;m<active_bufcnt;m++) {						/* For each of remaining ACTIVE buffers */
			opos = start;									/* Reset output-buf pointer to start-of-write point */
			for(n=0;n<samps_to_mix;n++) 					/* Add in samples from the other buffers */
				obuf[opos++] += *(dz->activebuf_ptr[m]++);
			dz->buflist[dz->activebuf[m]]->here += samps_to_mix;		
															/* And update current_position_in_buffer_pointers */
		}
		if((*outbuf_space = dz->buflen - opos)<=0) {		/* if output buffer is full */
			if((exit_status = write_samps(obuf,dz->buflen ,dz))<0)
				return(exit_status);						/* write a full buffer */
			*outbuf_space = dz->buflen;						/* and reset available space and buffer position */
			opos = 0;
		}
		*position_in_outbuf = opos;
		return(FINISHED);
															/* IF samps_to_write DOES overflow output buffer */
	} else {												/* which can only happen ONCE, */
															/* as in- & out- bufs are same size */
		if(*outbuf_space>0) {
			memmove((char *)(outptr),(char *)(dz->activebuf_ptr[0]),(*outbuf_space) * sizeof(float));
			dz->activebuf_ptr[0] += *outbuf_space;
			dz->buflist[dz->activebuf[0]]->here += *outbuf_space;	
			opos += *outbuf_space;
			for(m=1;m<active_bufcnt;m++) {
				opos = start;
				for(n=0;n<*outbuf_space;n++)
					obuf[opos++] += *(dz->activebuf_ptr[m]++);
				dz->buflist[dz->activebuf[m]]->here += *outbuf_space;
			}
		}
		if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
			return(exit_status);
		if(overflow) {
			start = opos = 0;
			memmove((char *)obuf,(char *)(dz->activebuf_ptr[0]),overflow*sizeof(float));
			dz->activebuf_ptr[0] += overflow;
			dz->buflist[dz->activebuf[0]]->here += overflow;
			opos = overflow;
			for(m=1;m<active_bufcnt;m++) {
				opos = start;
				for(n=0;n<overflow;n++)
					obuf[opos++] += *(dz->activebuf_ptr[m]++);
				dz->buflist[dz->activebuf[m]]->here += overflow;
			}
		}
	}
	*outbuf_space = dz->buflen - opos;						/* Reset the space-left-in-outbuf */
	*position_in_outbuf = opos;
	return(FINISHED);
}

/************************* DO_SILENCE ***********************************/

int do_silence(int samps_to_mix,int *position_in_outbuf,int *outbuf_space,dataptr dz)
{
	int exit_status;
	int opos = *position_in_outbuf;
	int overflow;
	float *obuf = dz->sampbuf[OBUFMIX];
	while((overflow = samps_to_mix - *outbuf_space) > 0) {
		memset((char *)(obuf+opos),0,(*outbuf_space) * sizeof(float));
		if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
			return(exit_status);
		opos = 0;
		*outbuf_space = dz->buflen;
		samps_to_mix = overflow;
	}
	if(samps_to_mix) {
		memset((char *)(obuf+opos),0,samps_to_mix*sizeof(float));
		opos += samps_to_mix;
		*outbuf_space = dz->buflen - opos;
	}
	*position_in_outbuf = opos;
	return(FINISHED);
}

/*************************** MIX_READ_SAMPS **************************/

int mix_read_samps(actmptr *act,float *inbuf,int samps_to_read,int n,dataptr dz)
{
	int thisfile = act[n]->val->ifd;
	if((dz->ssampsread = fgetfbufEx(inbuf, samps_to_read,thisfile,0)) < 0) {
		sprintf(errstr, "Can't read samps from input soundfile %d\n",n+1);
		return(SYSTEM_ERROR);
	}
	return(FINISHED);
}

/*************************** ISNUMERIC **************************/

int IsNumeric(char *str)
{
	char *p;
	int got_point = 0;
	p = str;
	while(*p != ENDOFSTR) {
		if(isdigit(*p))
			p++;
		else if(*p == '.') {
			if(got_point)
				return 0;
			got_point = 1;
			p++;
		} else 
			return 0;
	}
	if(p == str)
		return 0;
	return 1;
}
