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

#ifdef unix
#include <aaio.h>
#endif

#if defined unix || defined __GNUC__
#define round(x) lround((x))
#endif
#ifndef HUGE
#define HUGE 3.40282347e+38F
#endif

//	Use existing "long" items in struct "dz" to remember various process constants

#define envcount itemcnt			//	Number of timed-envelope entries
#define envsrcs  ringsize			//	number of the array at which envelope data begins
#define envbuf   rampbrksize		//	number of the array where interpolated envelope is constructed
#define outlevs  temp_sampsize		//	number of the array where current levels on each multichan out-channel are stored
#define	fltcnt	 is_transpos		//	Constant related to the order of the filter
#define	fltmul	 is_flat			//	Filter multiplier
#define	fltractv could_be_pitch		//	Flags whether filtering will be required
#define fulldur  is_sharp			//	Duration of event

#define SIGNAL_TO_LEFT  (0)			//	Parameters for stereo-hole-in-middle-compensation
#define SIGNAL_TO_RIGHT (1)
#define ROOT2		(1.4142136)

#define	FILTER_GAIN	(-96.0)			//	Filter gain for lo-pass filter

#define FLT_LPHP_ARRAYS_PER_FILTER (4)
#define FLT_DEN1			(0)
#define FLT_DEN2			(1)
#define FLT_CN				(2)

#define FLT_S1_BASE			(3)
#define FLT_S2_BASE			(4)
#define FLT_E1_BASE			(5)
#define FLT_E2_BASE			(6)

char errstr[2400];

int anal_infiles = 1;
int	sloom = 0;
int sloombatch = 0;

const char* cdp_version = "7.1.0";

//CDP LIB REPLACEMENTS
static int check_fracture_param_validity_and_consistency(dataptr dz);
static int setup_fracture_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_fracture_param_ranges_and_defaults(dataptr dz);
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
static int fracture(int ibuflen,int envbuflen,int overflow,dataptr dz);

static int handle_the_special_data(char *,dataptr dz);
static int read_the_special_data(dataptr dz);
static int fragment_param_preprocess (int *maxpulse,double *maxtrans,int *minfragmax,dataptr dz);
static int create_fracture_sndbufs(int maxpulse,double maxtrans,int minfragmax,int *ibuflen,int *envbuflen,int *overflow,int *mbuflen,dataptr dz);
static int read_the_envelope(double now, dataptr dz);
static int getstakwritestt(double stakcntr,int ldur,double incr);
static double getstakcentre(double *tempenv,dataptr dz);
static void pancalc(double position,double *leftgain,double *rightgain);

// mode > 0
static int panspread(int *sectcnt,double *centre,double *spread,int *cntrswitch,int *outside,double *fmix,int samps_to_process,dataptr dz);
static int spread_pan(double *thiscentre,int *cntrswitch,int *outside,double *spread,double *fmix,dataptr dz);

static int  filter_process(int *samps_to_process,int *endzeros,dataptr dz);
static int  do_lphp_filter_stereo(dataptr dz);
static int  lphp_filt_stereo(double *e1,double *e2,double *s1,double *s2,double *den1,double *den2,double *cn,dataptr dz,int chan);
static int  setup_lphp_filter(dataptr dz);
static int  establish_order_of_filter(double passfrq,double stopfrq,dataptr dz);
static int  allocate_internal_params_lphp(dataptr dz);
static void calculate_filter_poles_lphp(double  signd,int filter_order,double passfrq,dataptr dz);
static void initialise_filter_coeffs_lphp(dataptr dz);

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
	int exit_status;
	dataptr dz = NULL;
	char **cmdline;
	int  cmdlinecnt;
	int maxpulse = 0, ibuflen = 0, envbuflen = 0, minfragmax = 0, overflow = 0, mbuflen = 0;
	double maxtrans = 1.0;
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
		if((exit_status = setup_fracture_application(dz))<0) {
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
	if((exit_status = setup_fracture_param_ranges_and_defaults(dz))<0) {
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
//	check_param_validity_and_consistency....
	if((exit_status = check_fracture_param_validity_and_consistency(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = fragment_param_preprocess(&maxpulse,&maxtrans,&minfragmax,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	is_launched = TRUE;
	if((exit_status = create_fracture_sndbufs(maxpulse,maxtrans,minfragmax,&ibuflen,&envbuflen,&overflow,&mbuflen,dz))<0) {							
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = open_the_outfile(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = fracture(ibuflen,envbuflen,overflow,dz))<0) {
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
/* RWD malloc changed to calloc; helps debug version run as release! */

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
	(*cmdline)++;
	(*cmdlinecnt)--;
	return(FINISHED);
}

/************************ OPEN_THE_OUTFILE *********************/

int open_the_outfile(dataptr dz)
{
	int exit_status;
	dz->infile->channels = dz->iparam[FRAC_CHANS];
	if((exit_status = create_sized_outfile(dz->outfilename,dz))<0)
		return(exit_status);
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

/************************* SETUP_FRACTURE_APPLICATION *******************/

int setup_fracture_application(dataptr dz)
{
	int exit_status;
	aplptr ap;
	if((exit_status = establish_application(dz))<0)		// GLOBAL
		return(FAILED);
	ap = dz->application;
	// SEE parstruct FOR EXPLANATION of next 2 functions
	if(dz->mode == 0) {
		if((exit_status = set_param_data(ap,ENVSERIES,9,5,"iiDDD0000"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"rpdvestSmM",10,"DDDDDDDiDD","yl",2,0,"00"))<0)
			return(FAILED);
	} else {
		if((exit_status = set_param_data(ap,ENVSERIES,9,9,"iiDDDiDdd"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"rpdvesthmiazclfjkwg",19,"DDDDDDDiDDddddddddd","y",1,0,"0"))<0)
			return(FAILED);
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
		} else if(infile_info->channels != 1)  {
			sprintf(errstr,"File %s is not of correct type (must be mono)\n",cmdline[0]);
			return(DATA_ERROR);
		} else if((exit_status = copy_parse_info_to_main_structure(infile_info,dz))<0) {
			sprintf(errstr,"Failed to copy file parsing information\n");
			return(PROGRAM_ERROR);
		}
		free(infile_info);
	}
	return(FINISHED);
}

/************************* SETUP_FRACTURE_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_fracture_param_ranges_and_defaults(dataptr dz)
{
	int exit_status;
	aplptr ap = dz->application;
	// set_param_ranges()
	ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
	// NB total_input_param_cnt is > 0 !!!
	if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
		return(FAILED);
	// get_param_ranges()
	ap->hi[FRAC_CHANS] = 16;
	if(dz->mode == 0) {
		ap->lo[FRAC_CHANS] = 2;
		ap->default_val[FRAC_CHANS] = 2;
	} else {
		ap->lo[FRAC_CHANS] = 4;
		ap->default_val[FRAC_CHANS] = 8;
	}
	ap->lo[FRAC_STRMS] = 4;
	ap->hi[FRAC_STRMS] = 512;
	ap->default_val[FRAC_STRMS] = 16;
	ap->lo[FRAC_PULSE] = .05;
	ap->hi[FRAC_PULSE] = 8;
	ap->default_val[FRAC_PULSE] = 1;
	ap->lo[FRAC_DEPTH] = 0;
	ap->hi[FRAC_DEPTH] = 8;
	ap->default_val[FRAC_DEPTH] = 1;
	ap->lo[FRAC_STACK] = 0;
	ap->hi[FRAC_STACK] = 12;
	ap->default_val[FRAC_STACK] = 0;
	ap->lo[FRAC_INRND] = 0;
	ap->hi[FRAC_INRND] = 1;
	ap->default_val[FRAC_INRND] = 0;
	ap->lo[FRAC_OUTRND] = 0;
	ap->hi[FRAC_OUTRND] = 1;
	ap->default_val[FRAC_OUTRND] = 0;
	ap->lo[FRAC_SCAT] = 0;
	ap->hi[FRAC_SCAT] = 1;
	ap->default_val[FRAC_SCAT] = 0;
	ap->lo[FRAC_LEVRND] = 0;
	ap->hi[FRAC_LEVRND] = 1;
	ap->default_val[FRAC_LEVRND] = 0;
	ap->lo[FRAC_ENVRND] = 0;
	ap->hi[FRAC_ENVRND] = 32767;
	ap->default_val[FRAC_ENVRND] = 0;
	ap->lo[FRAC_STKRND] = 0;
	ap->hi[FRAC_STKRND] = 1;
	ap->default_val[FRAC_STKRND] = 0;
	ap->lo[FRAC_PCHRND] = 0;
	ap->hi[FRAC_PCHRND] = 1200;
	ap->default_val[FRAC_PCHRND] = 0;
	ap->lo[FRAC_SEED] = 0;
	ap->hi[FRAC_SEED] = 32767;
	ap->default_val[FRAC_SEED] = 0;
	ap->lo[FRAC_MIN] = 0;
	ap->hi[FRAC_MIN] = 1;
	ap->default_val[FRAC_MIN] = 0;
	ap->lo[FRAC_MAX] = 0;
	ap->hi[FRAC_MAX] = 16;
	ap->default_val[FRAC_MAX] = 0;
	if(dz->mode > 0) {
		ap->lo[FRAC_CENTRE] = 0;
		ap->hi[FRAC_CENTRE] = 16;
		ap->default_val[FRAC_CENTRE] = 1;
		ap->lo[FRAC_FRONT] = -4;	// -2= -infinity : further -2 allows for max-mdepth (* 2) as rear of moving-front dragged behind front
		ap->hi[FRAC_FRONT] = 2;		//	2 = +infinity
		ap->default_val[FRAC_FRONT] = 0;
		ap->lo[FRAC_MDEPTH] = 0;
		ap->hi[FRAC_MDEPTH] = 1;
		ap->default_val[FRAC_MDEPTH] = 0.5;
		ap->lo[FRAC_ROLLOFF] = 0.0;
		ap->hi[FRAC_ROLLOFF] = 1.0;
		ap->default_val[FRAC_ROLLOFF] = 0.0;
		ap->lo[FRAC_ATTEN] = 1.0;
		ap->hi[FRAC_ATTEN] = 10.0;
		ap->default_val[FRAC_ATTEN] = 3.0;
		ap->lo[FRAC_ZPOINT] = 0.0;
		ap->hi[FRAC_ZPOINT] = 1.0;
		ap->default_val[FRAC_ZPOINT] = 0.66;
		ap->lo[FRAC_CONTRACT] = 0.0;
		ap->hi[FRAC_CONTRACT] = 1.0;
		ap->default_val[FRAC_CONTRACT] = 0.66;
		ap->lo[FRAC_FPOINT] = 0.0;
		ap->hi[FRAC_FPOINT] = 1.0;
		ap->default_val[FRAC_FPOINT] = 0.66;
		ap->lo[FRAC_FFACTOR] = 0.0;
		ap->hi[FRAC_FFACTOR] = 1.0;
		ap->default_val[FRAC_FFACTOR] = 0.66;
		ap->lo[FRAC_FFREQ] = 10.0;
		ap->hi[FRAC_FFREQ] = (dz->nyquist/2.0) * 0.8;
		ap->default_val[FRAC_FFREQ] = 500.0;
		ap->lo[FRAC_UP] = 0.0;
		ap->hi[FRAC_UP] = 1.0;
		ap->default_val[FRAC_UP] = 0.0;
		ap->lo[FRAC_DN] = 0.0;
		ap->hi[FRAC_DN] = 1.0;
		ap->default_val[FRAC_DN] = 0.0;
		ap->lo[FRAC_GAIN] = 0.0;
		ap->hi[FRAC_GAIN] = 1.0;
		ap->default_val[FRAC_GAIN] = 1.0;
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
			if((exit_status = setup_fracture_application(dz))<0)
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
	usage2("fracture");
	return(USAGE_ONLY);
}

/**************************** CHECK_FRACTURE_PARAM_VALIDITY_AND_CONSISTENCY *****************************/

int check_fracture_param_validity_and_consistency(dataptr dz)
{
	int exit_status;
	double maxval, minval, xs;
	int n,m;
	if(dz->brksize[FRAC_STACK]) {
		for(n=0,m=1;n < dz->brksize[FRAC_STACK];n++,m+=2) {
			if(dz->brk[FRAC_STACK][m] <= 0.0) {
				sprintf(errstr,"Stacking value cannot be zero inside a breakpoint file,\n");
				return(DATA_ERROR);
			}
		}
	}
	if(dz->iparam[FRAC_SEED] > 0)
		srand((int)dz->iparam[FRAC_SEED]);
	if(dz->brksize[FRAC_DEPTH]) {
		if((exit_status = get_maxvalue_in_brktable(&maxval,FRAC_DEPTH,dz))<0)
			return PROGRAM_ERROR;
	} else
		maxval = dz->param[FRAC_DEPTH];
	if(maxval <= 1.0) {
		if(dz->brksize[FRAC_STACK] || (dz->param[FRAC_STACK] != 0)) {
			fprintf(stdout,"WARNING: Stack parameter set, but Depth never exceeds 1, so no stacking.\n");
			fflush(stdout);
			dz->brksize[FRAC_STACK] = 0;
			dz->param[FRAC_STACK] = 0.0;
		}
		if(dz->brksize[FRAC_STKRND] || (dz->param[FRAC_STKRND] != 0)) {
			fprintf(stdout,"WARNING: Stack randomisation parameter set, but Depth never exceeds 1, so no stacking.\n");
			fflush(stdout);
			dz->brksize[FRAC_STKRND] = 0;
			dz->param[FRAC_STKRND] = 0.0;
		}
	} else {		//	If stack value will be operational, if set to zero, reset NOW to 12
		if(!dz->brksize[FRAC_STACK] && dz->param[FRAC_STACK] == 0.0)
			dz->param[FRAC_STACK] = 12.0;
	}
	if(dz->mode > 0) {
		if(dz->iparam[FRAC_CHANS] % 4 != 0) {
			sprintf(errstr,"Number of output channels must be a multiple of 4.\n");
			return(DATA_ERROR);
		}
		xs = dz->param[FRAC_MDEPTH] * 2;
		if(dz->brksize[FRAC_FRONT]) {
			maxval = dz->brk[FRAC_FRONT][1];
			minval = maxval;
			for(n=1,m=3;n<dz->brksize[FRAC_FRONT];n++,m+=2) {	
				if(dz->brk[FRAC_FRONT][m] > minval) {
					sprintf(errstr,"Front value must move from high values to low values,\n");
					return(DATA_ERROR);
				}
				minval = dz->brk[FRAC_FRONT][m];
			}
			dz->fulldur = dz->brk[FRAC_FRONT][(dz->brksize[FRAC_FRONT] -1) * 2];
		} else {
			dz->fulldur = dz->duration;
			maxval = dz->param[FRAC_FRONT];
			minval = dz->param[FRAC_FRONT];
		}
		dz->fltractv = 0;
		if(maxval > 1.0 || minval < -(1 + xs))
			dz->fltractv = 1;

		if(dz->param[FRAC_UP] + dz->param[FRAC_DN] >= 1.0) {
			sprintf(errstr,"Fade up and Fade down times must add up to LESS THEN 1.\n");
			return(DATA_ERROR);
		}
		dz->param[FRAC_UP] *= dz->fulldur;
		dz->param[FRAC_DN] *= dz->fulldur;
	}
	return FINISHED;
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	if(!strcmp(prog_identifier_from_cmdline,"fracture"))			dz->process = FRACTURE;
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
	if(!strcmp(str,"fracture")) {
		fprintf(stderr,
	    "USAGE:fracture fracture 1\n"
	    "infil outfil etab chns strms pulse edpth stkint [-hseed] [-mmin] [-imax] [-rrrnd]\n"
		"[-pprnd] [-ddisp] [-vlrnd] [-eernd] [-srnd] [-ttrnd] [-y] [-l]\n"
		"\n"
		"OR\n"
		"\n"
	    "USAGE:fracture fracture 2\n"
	    "infil outfil etab chns strms pulse edpth stkint cntre frnt depth rolloff [-hseed]\n"
		"[-mmin] [-imax] [-rrrnd] [-pprnd] [-ddisp] [-vlrnd] [-eernd] [-srnd] [-ttrnd]\n"
		"[-aatten] [-zzpoint] [-ccontract] [] [-llopnt] [-ffmix] [-jffrq] [-kup] [-wdn] [-y]\n"
		"\n"
		"DISPERSE MONO SIGNAL INTO FRAGMENTS SPREAD OVER N-CHANNEL SPACE.\n"
		"\n"
		"Mode 1: Output is N-channel dispersal in N-channel space.\n"
		"Mode 2: Output is stereo dispersal (posibly moving) in surround space.\n"
		"\n"
		"PRESS ANY KEY TO CONTINUE\n");
		while(!kbhit())
			;
		if(kbhit()) {
			fprintf(stderr,
			"ETAB     Textfile: Each line is a TIME followed by 7 PAIRS of envelope-data.\n"
			"         envelope-data represented in form \"etime lev\"\n"
			"         where \"etime\" is relative time WITHIN envelope (values between 0 & 1)\n"
			"         and \"lev\" is level at etime (values 0-1). In each envelope-data set\n"
			"         \"lev\" must start and end at zero and rise to a max value of 1.0\n"
			"CHNS     Number of channels in output file (2-16). (Mode 2, multiples of 4 only).\n"
			"STRMS    No. of spatial positions (streams) for resulting fragments ( >=4 ).\n"
			"PULSE    Average gap btwn 1 set-of-frags (each in different strm) & next set.\n"
			"         Disp = 0: all frags in sync at each (possibly randomised) pulse.\n"
			"         Disp > 0: frags time-scattered around pulse centre. (see below).\n"
			"EDPTH    Envelope depth. 1: env cuts down to zero, 0.75: cuts 3/4 way to zero,\n"
			"(+STACK) 0.1: cuts only 1/10 way to zero,   0: env has no effect on source.\n"
			"         Once depth exceeds 1, fragments begin to stack (but NOT before)\n"
			"         (i.e. transposed copies added to fragment, synced at envelope peak).\n"
			"         Depth 2 -> Stack 1: 1st transposed element added at full level.\n"
			"         Depth 1.5 -> Stack 06.5: 1st transpd element added at 1/2 (0.5) level.\n"
			"         Depth 2.5 -> Stack 1.5: 1st added at full level, 2nd at 1/2 level etc\n"
			"STKINT   Interval of (upward) transposition in stack, in semitones (0-12).\n"
			"         Default(0) is read as octave(12). NO ZEROS in stack brkpoint files.\n"
			"SEED     If NOT zero, repeating process with same seed gives identical output.\n"
			"MIN/MAX  Minimum/maximum duration of fragments. If zero, no minimum/maximum.\n"
			"RRND     Randomisation of read-time in src. Range 0-1.\n"
			"PRND     Randomisation of pulse-time in output. Range 0-1.\n"
			"         In both cases, Max(1) scatters in range +- half-duration of pulse.\n"
			"DISP     Dispersal (scatter) of output timings between different streams.\n"
			"         If pulse(+prnd) gives time \"P\", then for disp 0: all frags are at \"P\"\n"
			"         for disp 1: frags scattered within max range (+- half-dur of pulse).\n"
			"LRND     Randomisation of levels (volume) of fragments. Range  0 - 1.\n"
			"         0: All frags full level.  1: Frags at random levels between 0 & full.\n"
			"ERND     Randomisation of envelope used. A time range (trange).\n"
			"         Event at \"now\" reads etable at randtime btwn \"now\" & now-minus-trange.\n"
			"SRND     Randomisation of stack. Range 0 - 1 (NB stack value (S) = DEPTH-1)\n"
			"         0: Stack value is S.   1: Stack val selected at random between 0 & S.\n"
			"TRND     Random tranpose of fragments. Range 0-1200 cents (1 octave upwards).\n"
			"         Event pitch randomised between pitch+trnd and pitch-trnd.\n"
			"-z       Permit stacking of very short events. Default = forbid, prevent clipping.\n"
			"-l       For more than 2 output chans, lspkrs assumed to encircle listeners,\n"
			"         with a single lspkr at centre front.\n"
			"         Setting -l flag assumes linear array, with leftmost+rightmost lspkrs.\n"
			"\n"
			"NB: STKINT, MAX, RRND, LRND, SRND and TRND are INACTIVE if depth < 1.\n"
			"All these params can timevary EXCEPT \"chns\", \"strms\" & \"seed\".\n"
			"\n"
			"PRESS KEY \"2\" TO SEE EXTRA OPTIONS AVAILABLE IN MODE 2\n"
			"OR ANY OTHER KEY TO FINISH.\n");
		}
		if(getch()=='2') {	
            fprintf(stderr,
            "MODE 2 ONLY: Additional parameters.\n"
            "\n"
            "CNTRE    Channel from which stereo-image spreads out.\n"
            "FRNT     Output leading edge: 1 = in cntr lspkr: -1= in lspkr opposite cntr.\n"
            "         0 = on bisector of entire sound-surround space.\n"
            "         2 = infinitely far away in direction of centre,\n"
            "         -(2+(depth*2)) = infinitely far away in direction opposite to centre.\n"
            "         If front moves, forward movement only (no reversals of direction)\n"
            "         and if doesn't go to infinity, event itself fades to 0 from midtime.\n"
            "DEPTH    maximum fraction of all output chans turned on, behind front.\n"
            "ROLLOFF  level fall as signal is spread over several chans. Range 0-1.\n"
            "         0 = no fall in level, 1 = level divided by number of chans in use.\n"
            "ATTEN    Level Attenuation factor for distance, or sound fade-out.\n"
            "         (Range >= 1 : 1 = linear).\n"
            "ZPOINT   Point where image subtends zero angle(mono): 0 circle edge, 1 infinity\n"
            "CONTRACT Contraction factor narrowing distant image width (>=1 : 1= linear).\n"
            "LOPNT    Distance where lopas-filtering is total ( 0 circle edge, 1 infinity.\n"
            "FMIX     Factor for mixing lopas-filtrd signal into orig, with distance (>=1).\n"
            "FFREQ    Lo-pass filter cut-off frequency.\n"
            "UP       If NOT zero, proportion of overall dur over which event fades from 0.\n"
            "         This is independent of any fadeup-with-approach-to-circle.\n"
            "DN       If NOT zero, proportion of overall dur over which event fades to 0.\n"
            "         This is independent of any fadeout-with-distance-from-circle.\n"
            "GAIN     Overall gain (0-1). If \"contract\" is very much faster than \"atten\",\n"
            "         rare possibility of overload, as signal is forced to mono.\n"
            "-z       Permit stacking of very short events. Default = forbid, prevent clipping.\n"
            "\n"
            "in MODE 2, of the additional parameters, only \"frnt\" can vary in time.\n");
		}
	} else
		fprintf(stdout,"Unknown option '%s'\n",str);
	return(USAGE_ONLY);
}

int usage3(char *str1,char *str2)
{
	fprintf(stderr,"Insufficient parameters on command line.\n");
	return(USAGE_ONLY);
}

/******************************** FRACTURE *******************************/

int fracture(int ibuflen,int envbuflen,int overflow,dataptr dz)
{
	int exit_status, chans, tim, lev, stim, etim, toget, thistack, filter_state = 0, dostack;
	double srate = (double)dz->infile->srate, maxsamp = 0.0, normaliser;
	int n, m, total_samps_written = 0, end_read_buf_at = -1, start_read_buf_at = 0;
	int strmcnt = dz->iparam[FRAC_STRMS];
/*
 *	LONG ARRAYS: S = number of streams      iptr (start of read WITHIN current input buffer, for each stream)
 *											| temp storage of event starttimes
 *			|---------------|---------------| | |
 *			|  segendsamp	|	inreadsamp	| |	|	NB segendsamp is in MONO samps
 *	address	0				S				2S|	|	need to change to (segendsamp * chans)
 *			|				|				| 2S+1	+ lmost(rmost) % dz->buflen
 *	 array	|	maxevents	|	maxevents	| |	|	for write!!
 *	length	|				|				strmcnt
 *			|				|				| strmcnt
 */

	int **evend = dz->lparray, **iread = dz->lparray + strmcnt;	//	Event endtime & input-read position, for each \fragment in each stream in each event-group
	int *iptr  = dz->lparray[2*strmcnt];			//	Pointers into input buffer, for each stream in current event-group
	int *evstt = dz->lparray[(2*strmcnt)+1];		//	Envelope starttime in each stream in current event-group
/*
 *	DOUBLE ARRAYS: S = number of streams									|				llev
 *							|												|				| rlev
 *			|---------------|---------------|---------------|---------------|---------------| | pos
 *			|	depth		|	level		|	transpos	|	envreadtime	|	stacking	| | | tempenv  
 *			|				|				|				|				|				5S| | | envdata
 *			|				|				|				|				|				| 5S+1| | stakcentre
 *	address	0				S				2S				3S				4S				| | 5S+2| | grptimes
 *			|				|				|				|				|				| | | 5S+3| | |
 *			|				|				|				|				|				| | | | 5S+4| |
 *	 array	|	maxevents	|	maxevents	|	maxevents	|	maxevents	|	maxevents	| | | | | 5S+5|
 *	length	|				|				|				|				|				S S S | | | 5S+6
 *			|				|				|				|				|				| | | 14| | | |
 *			|				|				|				|				|				| | | | dz->envcount * 15
 *			|				|				|				|				|				| | | | | S	| |
 *			|				|				|				|				|				| | | | | | maxevents
 */
	double **depth = dz->parray;					//	Envelope (end) depth for each event in each stream
	double **evlev = dz->parray + strmcnt;			//	Level for each event in each stream
	double **evpch = dz->parray + (2 * strmcnt);	//	Transposition for each event in each stream
	double **eread = dz->parray + (3 * strmcnt);	//	Position to read envelope table for each event in each stream
	double **stakk = dz->parray + (4 * strmcnt);	//	Stacking value for each event in each stream
	double *llev   = dz->parray[5 * strmcnt], *rlev = dz->parray[(5 * strmcnt)+1];	//	Left and Right level for each stream, to create correct positioning
	double *tempenv = dz->parray[dz->envbuf];		//	Temporary envelope store
	double *stkcntr = dz->parray[(5 * strmcnt)+5];	//	Central peak of current envelope for each stream
	double *gptime = dz->parray[(5 * strmcnt)+6];	//	group-Timing of events (for reading brktables)
/*
 *	INTEGER ARRAYS: S = number of streams
 *
 *			lmost
 *			| rmost
 *			| | |
 *	address	0 1 |
 *			| | |
 *	 array	S S |
 *   length	| | |
 */
	int *lmost = dz->iparray[0], *rmost = dz->iparray[1];	//	Left & right lspkr for each stream
/*
 *	SOUND BUFFERS
 *	|	input	| enveloping|  stacking	|  output	| overflow	|
 *
 */
	float *ibuf  = dz->sampbuf[0];
	float *ebuf  = dz->sampbuf[1];
	float *stkbuf= dz->sampbuf[2];
	float *obuf  = dz->sampbuf[3];
	float *ovflw = dz->sampbuf[4];
	int write_end = 0;
	double gp_stependtime = 0.0, last_gp_stependtime = 0.0, time, endtime, readtime;
	double init_depth, startdepth, enddepth, depthchange, thisdepth;
	double gp_stepdur, last_gp_stepdur = 0.0, randscat, maxeoffset, stack, this_stack, level, lbas, tdiff, ldiff, eval, frac;
	double transpos, incr, dsampcnt, loval, hival, diff, val, levrand, outincr, dipos, centre = 0.0;
	int losamp, hisamp, sectcnt = 0, cntrswitch = 0, outside = 0;
	int evcnt = 0, total_evcnt, minevstt, minread = 0, maxread = 0, ldur, sampcnt, stakwrite_at, opos, lpos, rpos, samps_to_write, iposlo, iposhi;
	int samps_to_read, samps_read, this_write_end, evlen, endevent, endeventend, fracmax = 0, endzeros = 0, samps_to_process;
	int display_limit = 2 * dz->infile->srate, this_display_limit = display_limit;
	double maxscatrange, lastev, offset, scatter_ambit_stt, scatter_ambit_end, available_dur, spread = 0.0, fmix = 0.0, prand;
	if(dz->mode == 0)
		chans = dz->iparam[FRAC_CHANS];
	else
		chans = STEREO;	//	Process initially creates stereo image which is then dispersed over multichannel space
	fprintf(stdout,"INFO: Level Check at %lf secs\n",(double)(total_samps_written/chans)/srate);
	fflush(stdout);
	while(gp_stependtime < dz->duration) {
		time = gp_stependtime;					//	Set start-time to end of last GROUP step
		gptime[evcnt] = time;
		if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
			return exit_status;
		if(dz->param[FRAC_MAX] > 0.0 && dz->param[FRAC_MIN] >= dz->param[FRAC_MAX]) {
			sprintf(errstr,"Max (%lf) and min (%lf) fragment-lengths incompatible at %lf secs\n",
			dz->param[FRAC_MIN],dz->param[FRAC_MAX],time);
			return DATA_ERROR;
		}
		dz->iparam[FRAC_MAX] = (int)ceil(dz->param[FRAC_MAX] * srate);
		init_depth = dz->param[FRAC_DEPTH];		//	Initial depth retained here for first event
		
		//	SET THE STREAMS-GROUP END-POSITION, POSSIBLY RANDOMISED
		
		gp_stepdur = dz->param[FRAC_PULSE];		//	Get duration of this step
		if(dz->param[FRAC_OUTRND] > 0.0) {		//	If ness, random scatter this
			randscat = (drand48() * 2.0) - 1.0;	//	Range -1 to +1
			randscat *= gp_stepdur/2.0;			//	Range (1/2*stepdur) to (3/2*stepdur)
			gp_stepdur += randscat;				//	Modify grp-stepdur
		}
		gp_stependtime = time + gp_stepdur;		//	This will be start of next event

		//	GET THE EVENT START AND END WRITE POSITIONS FOR EACH STREAM (POSSIBLT SCATTERED)

		for(n=0;n<strmcnt;n++) {				//	Note the start sample of events in each stream
			if(evcnt == 0)
				evstt[n] = 0;					//	First events start at file start
			else								//	Later events start where previous stream-event ended
				evstt[n] = evend[n][evcnt-1];
		}
		maxscatrange = gp_stepdur/2.0;			//	Max range (+ve or -ve) over which events can be scattered
		for(n=0;n<strmcnt;n++) {				//	Get the output end-timings of the streams
			endtime = gp_stependtime;			//	Do scattering, if required
			if(dz->param[FRAC_SCAT] > 0.0) {	//	Around the (already rand-offset) end-timing of the group	
				if(dz->param[FRAC_MIN] > 0.0) {
					scatter_ambit_stt = gp_stependtime - maxscatrange;
					scatter_ambit_end = gp_stependtime + maxscatrange;
					if(evcnt == 0)
						lastev = 0.0;
					else
						lastev = (evend[n][evcnt-1])/srate;			//	End time of previous event
					available_dur = scatter_ambit_end - lastev;
					if(available_dur <= dz->param[FRAC_MIN]) {		//	If insufficient space for min-dur fragment
						if(evcnt == 0)
							evend[n][evcnt] = 0;
						else
							evend[n][evcnt] = evend[n][evcnt-1];	//	Reduce event length to zero i.e. eliminate the event
						continue;
					} else {
						endtime = lastev + dz->param[FRAC_MIN];		//	Move base of rand-vary to min-duration point
						if(endtime >= scatter_ambit_stt) {			//	If the min length falls inside ambit of current gp-position
							available_dur -= dz->param[FRAC_MIN];	//	Length to scatter-amongst is what remains
						} else {									//	If NOT
							endtime = scatter_ambit_stt;			//	force base into ambit of current gp-position
							available_dur = maxscatrange * 2.0;		//	Length to scatter-amongst is whole ambit around gp-position
						}	
							//	Do randomisation within the remaining range
						randscat = drand48();						//	Range 0 to 1
						randscat *= available_dur * dz->param[FRAC_SCAT];
					}
				} else {
					randscat = (drand48() * 2) - 1.0;		//	Range -1 to 1
					if(evcnt > 0) {
						lastev = (evend[n][evcnt-1])/srate;	//	End time of previous event
						offset = gp_stependtime - lastev;	//	Time between end of last event and current event_group centre
						if(offset <= 0) {					//	Previous event end is beyond current gpevent-end
							evend[n][evcnt] = evend[n][evcnt-1];	//	Reduce event length to zero i.e. eliminate the event
							continue;
						} else if(offset < maxscatrange && randscat < 0.0)	//	If scattering DOWN, and offset smaller than maxrange
							randscat *= offset * dz->param[FRAC_SCAT];		//	Scatter within offset
						else											//	In all other cases, scatter within true maxrange
							randscat *= maxscatrange * dz->param[FRAC_SCAT];
					} else
						randscat *= maxscatrange * dz->param[FRAC_SCAT];
					
				}
				endtime  += randscat;			//	Use the actual group step-position as basis to calc rand-offsets
			}
			evend[n][evcnt] = (int)round(endtime * srate);
		}

		//	DECIDE IF A WRITE-OUTBUF IS REQUIRED, AND IF SO, DO IT

		minevstt = evstt[0];					//	Find minimum event start
		for(n=1;n<strmcnt;n++)
			minevstt = min(minevstt,evstt[n]);
		minevstt *= chans;						//	Convert to frame of output (has more channels)
		minevstt -= total_samps_written;		//	Convert to frame of output-buffer
		if(minevstt >= dz->buflen) {			//	If next write will start at or after buffer end
			if(total_samps_written/chans > this_display_limit) {
				fprintf(stdout,"INFO: Level Check at %lf secs\n",(double)(total_samps_written/chans)/srate);
				fflush(stdout);
				this_display_limit += display_limit;
			}
			for(n=0;n < dz->buflen;n++)			//	"Write", copy overflow back, zero overflow
				maxsamp = max(maxsamp,fabs(obuf[n]));
			total_samps_written += dz->buflen;
			write_end -= dz->buflen;
			for(n = 0;n < write_end;n++)		//	Copy back overflow into obuf (overflow may be longer than obuf)
				obuf[n] = ovflw[n];				//	and zero remainder of overflow
			memset((char *)(obuf+write_end),0,(dz->buflen + overflow - write_end) * sizeof(float));
		}

		//	FIND END DEPTH OF EACH STREAM
		
		for(n=0;n<strmcnt;n++) {
			endtime = (double)(evend[n][evcnt])/srate; //	Get true endtime of stream-event
			if(dz->brksize[FRAC_DEPTH]) {
				if((exit_status = read_value_from_brktable(endtime,FRAC_DEPTH,dz))<0)
					return(exit_status);					//	And read depth at end of event
			}
			depth[n][evcnt] = dz->param[FRAC_DEPTH];
		}

		//	FIND READ SAMPLE OF EACH STREAM, (RANDOMISED IF REQUIRED) AND MAX AND MINIMUM READ

		if(evcnt == 0) {		//	At very start, all segments are read at 0
			for(n=0;n<strmcnt;n++)		
				iread[n][evcnt] = 0;
			minread = 0;
			maxread = 0;
			for(n=0;n<strmcnt;n++)
				maxread = max(maxread,evend[n][evcnt]);

		
			startdepth = init_depth;			//  Depth already read at start of loop
			enddepth = depth[n][evcnt];
			if(startdepth < 1 || enddepth < 1)	//	If envelopes don't cut down to zero at both ends
				fracmax = 0;
			else
				fracmax = dz->iparam[FRAC_MAX];
		} else {
			for(n=0;n<strmcnt;n++) {				//	Scatter the output read-timings for the streams (if ness) 
				startdepth = depth[n][evcnt - 1];	//	Startdepth = previous end-depth
				enddepth = depth[n][evcnt];
				if(startdepth < 1 || enddepth < 1) {		//	If envelopes don't cut down to zero at both ends
					iread[n][evcnt] = evend[n][evcnt - 1];	//	reads & writes in same place, avoid discontinuity in output sig
					fracmax = 0;
				} else {	
					readtime = last_gp_stependtime;
					if(dz->param[FRAC_INRND] > 0) {
						randscat = (drand48() * 2) - 1.0;	//	Around the timing START of the group	
						randscat *= (last_gp_stepdur/2.0) * dz->param[FRAC_INRND];
						readtime += randscat;//	Using the actual group step-duration as basis to calc rand offsets
					}
					iread[n][evcnt] = (int)round(readtime * srate);
					fracmax = dz->iparam[FRAC_MAX];
				}
				evlen = evend[n][evcnt] - evend[n][evcnt-1];
				if(fracmax > 0)
					evlen = min(evlen,fracmax);
				if(n==0) {
					minread = iread[n][evcnt];
					maxread = iread[n][evcnt] + evlen;
				} else {
					minread = min(iread[n][evcnt],minread);
					maxread = max(iread[n][evcnt] + evlen,maxread);
				}
			}
		}
		
		//	SEARCH TO APPROPRIATE PLACE IN INPUT, AND SET IPTRS
		
		if(maxread - minread > ibuflen) {
			sprintf(errstr,"INPUT BUFFERSIZE MISCALCULATION\n");
			return(PROGRAM_ERROR);
		}
		if(maxread > end_read_buf_at) {
			sndseekEx(dz->ifd[0],minread,0);
			start_read_buf_at = minread;
			samps_to_read = maxread - minread;
			memset((char *)ibuf,0,ibuflen * sizeof(float));
			if((samps_read  = fgetfbufEx(ibuf,ibuflen,dz->ifd[0],0))<0) {
				sprintf(errstr,"Sound read error: %s\n",sferrstr());
				return(SYSTEM_ERROR);
			}
			if(samps_read < samps_to_read) {
				fprintf(stdout,"INFO: Ran out of input samples: Terminating.\n");
				fflush(stdout);
				break;
			}
			end_read_buf_at = start_read_buf_at + samps_read;
		}
		for(n=0;n<strmcnt;n++)
			iptr[n] = iread[n][evcnt] - start_read_buf_at;
			
		//	FIND THE TIMES AT WHICH TO READ ENVELOPE-TEMPLATE, AND THE DEPTH
		
		if(evcnt == 0) {								//	At very start, all envelope are read at 0
			for(n=0;n<strmcnt;n++)		
				eread[n][evcnt] = 0.0;
		} else {										//  Otherwise,
			for(n=0;n<strmcnt;n++) {					//	initially set envelope read position to start of segment, for each stream		
				eread[n][evcnt] = (double)(evend[n][evcnt - 1])/srate;
				if(dz->param[FRAC_ENVRND] > 0.0) {		//	If the envelope reading is scattered over a time-range
					maxeoffset = min(dz->param[FRAC_ENVRND],eread[n][evcnt]);
					randscat = drand48() * maxeoffset;	// Check timerange does not exceed "now"
					eread[n][evcnt] -= randscat;		//	Then generate randomisation of read position
				}
			}
		}
		for(n=0;n<strmcnt;n++) {						//	Find depth at end of each stream
			endtime = (double)(evend[n][evcnt])/srate; //	Get true endtime of stream-event
			if(dz->brksize[FRAC_DEPTH]) {
				if((exit_status = read_value_from_brktable(endtime,FRAC_DEPTH,dz))<0)
					return(exit_status);					//	And read depth at end of event
			}
			depth[n][evcnt] = dz->param[FRAC_DEPTH];
		}
		//	READ ENVELOPE-TEMPLATE, MODIFY DEPTHS AND TIMINGS (AND CHECK FOR STACKING)

		for(n=0;n<strmcnt;n++) {						//	Reads an envelope template at specified time, and modify
			read_the_envelope(eread[n][evcnt],dz);		//	Read from input data-envelopes to temp envelope-store "tempenv"
			stkcntr[n] = getstakcentre(tempenv,dz);		//	Note where the peak is, for any stacking

			//	Get envelope depth+stacking value
			
			if(evcnt == 0)								//	At very start
				startdepth = init_depth;				//  Depth already read at start of loop
			else										//	Else
				startdepth = depth[n][evcnt - 1];		//	Startdepth = previous end-depth
			enddepth = depth[n][evcnt];
			stack = 0;
			if(startdepth >= 1.0 && enddepth >= 1.0) {	
				
				//	If depth values beyond 1, this represents stacking.

				stack = startdepth - 1.0;				//	Derive stacking value from depth at start of event.
				startdepth = 1.0;						//	and reset depth values to depth-maxima (1.0)
				enddepth = 1.0;							//	NB Both depths must be 1, so envelope edges tied to zero, so stack causes no glitches

			} else {

				//	If depths is not everywhere at max, interp depth data onto env

				depthchange = enddepth - startdepth;
				for(tim=0,lev=1;tim<14;tim+=2,lev+=2) {
					thisdepth = depthchange * tempenv[tim];	//	Times are between 0 and 1, so time also represents FRACTION of time
					thisdepth += startdepth;
					if(thisdepth > 1.0)
						thisdepth = 1.0;
					level = 1.0 - thisdepth;			//	Max Depth = min env level  depth 1 -> 0 level| depth 1/3 -> level2/3| depth 0 -> level 1
					level += (tempenv[lev] * thisdepth);//	Env fills remaining space	i.e  whole space | upper 1/3 of space:	| None of space, env has no effect
					tempenv[lev] = level;
				}
			}

			//	COPY APPROPRIATE INPUT SAMPLES TO ENVELOPE BUFFER, AND DO ENVELOPING
			
			if(evcnt == 0) 
				ldur = evend[n][0];
			else
				ldur = evend[n][evcnt] - evend[n][evcnt-1];
			if(fracmax > 0)
				ldur = min(ldur,fracmax);
			if(ldur > envbuflen) {
				sprintf(errstr,"ENVELOPE BUFFERSIZE MISCALCULATION: required sample duration = %d available envelope buffer length = %d\n",ldur,envbuflen);
				return(PROGRAM_ERROR);
			}
			memset((char *)ebuf,0,envbuflen * sizeof(float));
			memcpy((char *)ebuf,(char *)(ibuf + iptr[n]),ldur * sizeof(float));
			stim = 0;
			etim = 2;
			lbas  = tempenv[stim+1];					//	Start of envelope segment	
			tdiff = tempenv[etim] - tempenv[stim];		//	Timestep in envelope segment
			ldiff = tempenv[etim+1] - tempenv[stim+1];	//	Level step in envelope segment
			for(sampcnt=0;sampcnt<ldur;sampcnt++) {
				eval = tempenv[13];						//	Default to end value, in case we run off end of table
				if(etim < 14) {
					toget = 1;
					frac = (double)sampcnt/(double)ldur; //	Find time-fraction
					while(frac > tempenv[etim]) {		//	Locate which envelope seg we're in
						stim = etim;					//	advancing to next where ness
						etim += 2;
						if(etim >= 14) {				//	Gets default value at table end
							toget = 0;
							break;
						} else {						
							lbas  = tempenv[stim+1];	//	and resetting segment constants
							tdiff = tempenv[etim] - tempenv[stim];
							ldiff = tempenv[etim+1] - tempenv[stim+1];
						}
					}
					if(toget) {							//	Interp in envelope table								
						eval = (frac - tempenv[stim])/tdiff;
						eval *= ldiff;
						eval += lbas;
					}
				}										//	Use envelope val to scale input
				ebuf[sampcnt] = (float)(ebuf[sampcnt] * eval);
			}

			//	IF STACKING - IF STACKING RANDOMISED SET A STACK VALUE - STORE STACKING VALUE

			if(stack > 0) {
				dostack = 1;
				if(ldur < 8192) {				//	Prevent stacking of very short events	
					if(!dz->vflag[0]) {			//	Unless allowed
						dostack = 0;
						stack = 0;
					}
				}
				if (dostack) {
					if(dz->param[FRAC_STKRND] > 0) {
						this_stack = stack;
						stack *= drand48() * dz->param[FRAC_STKRND];
						stack = this_stack - stack;
					}
				}
			}
			stakk[n][evcnt] = stack;					//	Store stack values for next pass
			if(stack > 0) {
				memset((char *)stkbuf,0,envbuflen * sizeof(float));
				thistack = (int)floor(stack);								// e.g. 2.0		e.g. 2.3
				while(thistack > 0) {										//	--> 2		--> 2
					if(flteq((double)thistack,stack))
						level = 1.0;										//	level 1	
																			//	trans remains 2
					else {
						level = stack - (double)thistack;					//	level 0.3
						thistack++;											//	trans 3
					}
					transpos = dz->param[FRAC_STACK] * thistack;			//	Derives stacking interval from multiple of stack parameter
					incr = pow(2.0,(transpos/SEMITONES_PER_OCTAVE));		//	semitones to frq ratio
					stakwrite_at = getstakwritestt(stkcntr[n],ldur,incr);	//	Find where to start writing this particular transposition into stakbuf
					dsampcnt = 0.0;
					while(dsampcnt < ldur) {							//	Read the enveloped orig, with an incr that transposes it
						losamp = (int)floor(dsampcnt);					//	Interpolating as ness
						hisamp = losamp+1;
						frac  = dsampcnt - (double)losamp;
						loval = ebuf[losamp];
						hival = ebuf[hisamp];
						diff  = hival - loval;
						diff *= frac;
						val = loval + diff;								//	Add the interpd val into stakbuffer
						stkbuf[stakwrite_at] = (float)(stkbuf[stakwrite_at] + val);	//	(There may be more than 1 stack component to add)	
						stakwrite_at++;									//	Incr normally in stacking buffer
						dsampcnt += incr;								//	Incr transpositionwise in read-from buff
					}
					thistack--;												//	-->1	--> 1
					stack = (double)thistack;								//	Will now generate full level for lower stack components
				}
				
				// Once all staks generated, add stacks to original envelope buffer

				for(m=0;m<ldur;m++)
					ebuf[m] = (float)(ebuf[m] + stkbuf[m]);
			}
				//	IF LEVEL IS RANDOMISED, AND EVENTS ARE FULLY SEPARATED (Depth 1 at both ends) GET A RANDOM LEVEL AND APPLY IT
			
			if(startdepth >= 1.0 && enddepth >= 1.0) {						//	Level or pitch-shifting can only happem
				if(dz->param[FRAC_LEVRND] > 0) {							//	if edges of event are tied to zero-level
					levrand = drand48() * dz->param[FRAC_LEVRND];
					levrand = 1.0 - levrand;
					evlev[n][evcnt] = levrand;
					for(m=0;m<ldur;m++)
						ebuf[m] = (float)(ebuf[m] * evlev[n][evcnt]);
				} else 
					evlev[n][evcnt] = 1.0;

				//	IF PITCH IS RANDOMISED, GET A RANDOM PITCH-INCR READY TO READ ENVELOPED DATA TO OUTPUT BUFFER
				//	IF NOT, THIS GETS VALUE 1 (No pitch change)

				if(dz->param[FRAC_PCHRND] > 0) {
					prand = drand48() * dz->param[FRAC_PCHRND];
					outincr = pow(2.0,(prand/SEMITONES_PER_OCTAVE));	//	Convert from semitones to frq-ratio
				} else
					outincr = 1.0;
				evpch[n][evcnt] = outincr;			//	Store
			} else {
				evlev[n][evcnt] = 1.0;
				evpch[n][evcnt] = 1.0;
				outincr = 1.0;
			}
				//	SAFETY CHECK FOR OUTPUT BUFFER OVERFLOW

			opos = evstt[n] * chans;			//	Change write position to frame of output
			opos -= total_samps_written; 		//	Change write position to frame of output-buffer

			samps_to_write = (int)ceil((double)ldur/outincr) * chans;
			if(samps_to_write + opos >= dz->buflen + overflow) {
				sprintf(errstr,"OVERFLOW BUFFER ITSELF OVERFLOWED WHEN PITCH INCR WAS %lf.\n",outincr);
				return(PROGRAM_ERROR);
			}

				//	WRITE OUTPUT

			lpos = opos + lmost[n];				//	Change write position to correct left and right channels
			rpos = opos + rmost[n];
			dipos = 0.0;
			while(dipos < ldur) {				//	This should wraparound OK, as buffer is preset with zeros
				iposlo = (int)floor(dipos);	//	So wraparound sample will be zero
				iposhi = iposlo+1;
				frac = dipos - (double)iposlo;
				loval = ebuf[iposlo];
				hival = ebuf[iposhi];
				diff  = hival - loval;
				diff *= frac;
				val = loval + diff;				//	Add the interpd enveloped-val into output buffer
				obuf[lpos] = (float)(obuf[lpos] + (val * llev[n]));
				obuf[rpos] = (float)(obuf[rpos] + (val * rlev[n]));
				dipos += outincr;				//	Interpolate in input (enveloped) sound
				lpos += chans;					//	Jump by a whole channel-group in output buffer
				rpos += chans;
			}
			this_write_end = (lpos/chans) * chans;		//	Round up to end of last-group written, to find end of write in buffer
			write_end = max(this_write_end,write_end);	//	Find write_end from all stream writes (and previous writes)
		}
		
		//	ONCE ALL STREAMS ADDED TO OUTPUT

		evcnt++;									//	Count event
		last_gp_stependtime = gp_stependtime;		//	Remember startsamp and sampduration of previous group-segment
		last_gp_stepdur = gp_stepdur;				//	Need for generating input-read time
	}
	total_evcnt = evcnt;
	if(write_end > 0) {
		fprintf(stdout,"INFO: Level Check at %lf secs\n",(double)(total_samps_written/chans)/srate);
		fflush(stdout);
		for(n=0;n < write_end;n++)			//	"Write", copy overflow back, zero overflow
			maxsamp = max(maxsamp,fabs(obuf[n]));
		total_samps_written += write_end;
	}
	normaliser = 1.0;
	if(maxsamp > 0.95) {
		normaliser = 0.95/maxsamp;
		fprintf(stdout,"INFO: Normalising by %lf secs\n",(double)normaliser);
		fflush(stdout);
	}	
	for(n=0;n<strmcnt;n++) {						//	In each stream
		endevent = evcnt-1;							//	Find the last active event
		endeventend = evend[n][endevent];			//	i.e. last event NOT of zero length
		m = endevent - 1;
		while(m >= 0) {
			if(evend[n][m] < endeventend)			//	If previous event begins earlier, we've found end event
				break;
			else {									//	Else end of two events coincides, thus zero-length event.
				endevent = m;						//	Skip back over it,to find real last-active-event
				m--;
			}
		}
		depth[n][endevent] = 1;						//	Force this last event to fade to zero at stream's end
	}
	fprintf(stdout,"INFO: Second pass: generating output.\n");
	fflush(stdout);

	if(sloom) {
		if(dz->mode == 0)
			dz->insams[0] = total_samps_written;	//	This forces sloom progress bar to proceed correctly, without mod to libraries
		else
			dz->insams[0] = (total_samps_written/STEREO) * dz->iparam[FRAC_CHANS];
	}											
	sndseekEx(dz->ifd[0],0,0);
	reset_filedata_counters(dz);
	memset((char *)obuf,0,(dz->buflen + overflow) * sizeof(float));	//	reset output and overflow buffers
	memset((char *)ibuf,0,ibuflen * sizeof(float));			//	reset input buffer
	total_samps_written = 0;
	end_read_buf_at = -1;
	start_read_buf_at = 0;
	write_end = 0;
	evcnt = 0;
	while(evcnt < total_evcnt) {
		time = gptime[evcnt];					//	Set start-time to end of last GROUP step
		if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
			return exit_status;
		dz->iparam[FRAC_MAX] = (int)ceil(dz->param[FRAC_MAX] * srate);
		init_depth = dz->param[FRAC_DEPTH];
		
		//	GET THE EVENT START POSITIONS FOR EACH STREAM

		for(n=0;n<strmcnt;n++) {				//	Note the start sample of events in each stream
			if(evcnt == 0)
				evstt[n] = 0;
			else
				evstt[n] = evend[n][evcnt-1];
		}

		//	DECIDE IF A WRITE-OUTBUF IS REQUIRED, AND IF SO, DO IT

		minevstt = evstt[0];					//	Find minimum event start
		for(n=1;n<strmcnt;n++)
			minevstt = min(minevstt,evstt[n]);
		minevstt *= chans;						//	Convert to frame of output (has more channels)
		minevstt -= total_samps_written;		//	Convert to frame of output-buffer
		if(minevstt >= dz->buflen) {			//	If next write will start at or after buffer end
			for(n=0;n < dz->buflen;n++)			//	Normalise, write output, copy overflow back, zero overflow
				obuf[n] = (float)(obuf[n] * normaliser);
			if(dz->mode == 0) {
				if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
					return(exit_status);
				total_samps_written += dz->buflen;
				write_end -= dz->buflen;
				for(n = 0;n < write_end;n++)		//	Copy back overflow into obuf (overflow may be longer than obuf)
					obuf[n] = ovflw[n];				//	and zero remainder of overflow
				memset((char *)(obuf+write_end),0,(dz->buflen + overflow - write_end) * sizeof(float));
			} else {
				samps_to_process = dz->buflen;
				if(dz->fltractv) {
					if((exit_status = filter_process(&samps_to_process,&endzeros,dz))<0)
						return(exit_status);
				}
				if((exit_status = panspread(&sectcnt,&centre,&spread,&cntrswitch,&outside,&fmix,samps_to_process,dz))<0)
					return(exit_status);
				if(exit_status == CONTINUE) {
					total_samps_written += dz->buflen;
					write_end -= dz->buflen;
					for(n = 0;n < write_end;n++)		//	Copy back overflow into obuf (overflow may be longer than obuf)
						obuf[n] = ovflw[n];				//	and zero remainder of overflow
					memset((char *)(obuf+write_end),0,(dz->buflen + overflow - write_end) * sizeof(float));
				} else {
					write_end = 0;
					break;
				}
			}
		}

		//	FIND READ SAMPLE OF EACH STREAM, AND MAX AND MINIMUM READ

		if(evcnt == 0) {		//	At very start, all segments are read at 0
			minread = 0;
			maxread = 0;
			for(n=0;n<strmcnt;n++)
				maxread = max(maxread,evend[n][evcnt]);
			startdepth = init_depth;			//	Startdepth = previous end-depth
			enddepth = depth[n][evcnt];
			if(startdepth < 1 || enddepth < 1)	//	If envelopes don't cut down to zero at both ends
				fracmax = 0;					//	Max fragment length not used
			else
				fracmax = dz->iparam[FRAC_MAX];
		} else {
			for(n=0;n<strmcnt;n++) {				//	Scatter the output read-timings for the streams (if ness) 
				startdepth = depth[n][evcnt - 1];	//	Startdepth = previous end-depth
				enddepth = depth[n][evcnt];
				if(startdepth < 1 || enddepth < 1)	//	If envelopes don't cut down to zero at both ends
					fracmax = 0;					//	Max fragment length not used
				else
					fracmax = dz->iparam[FRAC_MAX];
				evlen = evend[n][evcnt] - evend[n][evcnt-1];
				if(fracmax > 0)
					evlen = min(evlen,fracmax);
				if(n==0) {
					minread = iread[n][evcnt];
					maxread = iread[n][evcnt] + evlen;
				} else {
					minread = min(iread[n][evcnt],minread);
					maxread = max(iread[n][evcnt] + evlen,maxread);
				}
			}
		}

		//	SEARCH TO APPROPRIATE PLACE IN INPUT (IF NECESSARY), AND SET IPTRS
		
		if(maxread > end_read_buf_at) {
			sndseekEx(dz->ifd[0],minread,0);
			start_read_buf_at = minread;
			samps_to_read = maxread - minread;
			if((samps_read  = fgetfbufEx(ibuf,ibuflen,dz->ifd[0],0))<0) {
				sprintf(errstr,"Sound read error: %s\n",sferrstr());
				return(SYSTEM_ERROR);
			}
			if(samps_read < samps_to_read) {
				fprintf(stdout,"WARNING: Sound Read anomaly - short buffer read, probably at end of file: Terminating.\n");
				fflush(stdout);		//	This shouled not happen as it's trapped on the first pass
				break;
			}
			end_read_buf_at = start_read_buf_at + samps_read;
		}
		for(n=0;n<strmcnt;n++)
			iptr[n] = iread[n][evcnt] - start_read_buf_at;
		
		//	READ ENVELOPE-TEMPLATE, MODIFY DEPTHS AND TIMINGS (AND CHECK FOR STACKING)

		for(n=0;n<strmcnt;n++) {						//	Reads and envelope template at specified time, and modify
			read_the_envelope(eread[n][evcnt],dz);		//	Read from input data-envelopes to temp envelope-store "tempenv"
			stkcntr[n] = getstakcentre(tempenv,dz);		//	Note where the peak is, for any stacking

			//	Get envelope depth+stacking value
			
			if(evcnt == 0)								//	At very start
				startdepth = init_depth;				//  Depth already read at start of loop
			else										//	Else
				startdepth = depth[n][evcnt - 1];		//	Startdepth = previous end-depth
			enddepth = depth[n][evcnt];
			if(startdepth >= 1.0 && enddepth >= 1.0) {	
				startdepth = 1.0;
				enddepth = 1.0;
			} else {
				depthchange = enddepth - startdepth;
				for(tim=0,lev=1;tim<14;tim+=2,lev+=2) {
					thisdepth = depthchange * tempenv[tim];	//	Times are between 0 and 1, so time also represents FRACTION of time
					thisdepth += startdepth;
					if(thisdepth > 1.0)
						thisdepth = 1.0;
					level = 1.0 - thisdepth;			//	Max Depth = min env level  depth 1 -> 0 level| depth 1/3 -> level2/3| depth 0 -> level 1
					level += (tempenv[lev] * thisdepth);//	Env fills remaining space	i.e  whole space | upper 1/3 of space:	| None of space, env has no effect
					tempenv[lev] = level;
				}
			}

			//	COPY APPROPRIATE INPUT SAMPLES TO ENVELOPE BUFFER, AND DO ENVELOPING
			
			if(evcnt == 0)
				ldur = evend[n][0];
			else
				ldur = evend[n][evcnt] - evend[n][evcnt-1];
			if(fracmax > 0)
				ldur = min(ldur,fracmax);

			memset((char *)ebuf,0,envbuflen * sizeof(float));
			memcpy((char *)ebuf,(char *)(ibuf + iptr[n]),ldur * sizeof(float));
			stim = 0;
			etim = 2;
			lbas  = tempenv[stim+1];					//	Start of envelope segment	
			tdiff = tempenv[etim] - tempenv[stim];		//	Timestep in envelope segment
			ldiff = tempenv[etim+1] - tempenv[stim+1];	//	Level step in envelope segment
			for(sampcnt=0;sampcnt<ldur;sampcnt++) {
				eval = tempenv[13];						//	Default to end value, in case we run off end of table
				if(etim < 14) {
					toget = 1;
					frac = (double)sampcnt/(double)ldur; //	Find time-fraction
					while(frac > tempenv[etim]) {		 //	Locate which envelope seg we're in
						stim = etim;					//	advancing to next where ness
						etim += 2;
						if(etim >= 14) {
							toget = 0;
							break;
						} else {
							lbas  = tempenv[stim+1];	//	and resetting segment constants
							tdiff = tempenv[etim] - tempenv[stim];
							ldiff = tempenv[etim+1] - tempenv[stim+1];
						}
					}
					if(toget) {							//	Interp in envelope table								
						eval = (frac - tempenv[stim])/tdiff;
						eval *= ldiff;
						eval += lbas;
					}
				}										//	Use envelope val to scale input
				ebuf[sampcnt] = (float)(ebuf[sampcnt] * eval);
			}

			//	IF STACKING - DO STACKING

			stack = stakk[n][evcnt];
			if(stack > 0) {
				memset((char *)stkbuf,0,envbuflen * sizeof(float));
				thistack = (int)floor(stack);								// e.g. 2.0		e.g. 2.3
				while(thistack > 0) {										//	--> 2		--> 2
					if(flteq((double)thistack,stack))
						level = 1.0;										//	level 1	
																			//	trans remains 2
					else {
						level = stack - (double)thistack;					//	level 0.3
						transpos = dz->param[FRAC_STACK] * (1 + thistack);	//	trans 3
					}
					transpos = dz->param[FRAC_STACK] * thistack;			//	Derives stacking interval from multiple of stack parameter
					incr = pow(2.0,(transpos/SEMITONES_PER_OCTAVE));		//	semitones to frq ratio
					stakwrite_at = getstakwritestt(stkcntr[n],ldur,incr);	//	Find where to start writing this particular transposition into stakbuf
					dsampcnt = 0.0;
					while(dsampcnt < ldur) {							//	Read the enveloped orig, with an incr that transposes it
						losamp = (int)floor(dsampcnt);					//	Interpolating as ness
						hisamp = losamp+1;
						frac  = dsampcnt - (double)losamp;
						loval = ebuf[losamp];
						hival = ebuf[hisamp];
						diff  = hival - loval;
						diff *= frac;
						val = loval + diff;								//	Add the interpd val into stakbuffer
						stkbuf[stakwrite_at] = (float)(stkbuf[stakwrite_at] + val);	//	(There may be more than 1 stack component to add)	
						stakwrite_at++;									//	Incr normally in stacking buffer
						dsampcnt += incr;								//	Incr transpositionwise in read-from buff
					}
					thistack--;												//	-->1	--> 1
					stack = (double)thistack;								//	Will now generate full level for lower stack components
				}
				
				// Once all staks generated, add stacks to original envelope buffer

				for(m=0;m<ldur;m++)
					ebuf[m] = (float)(ebuf[m] + stkbuf[m]);
			}
				//	IF LEVEL IS NOT 1.0, APPLY IT

			if(!flteq(evlev[n][evcnt],1.0)) {
				for(m=0;m<ldur;m++)
					ebuf[m] = (float)(ebuf[m] * evlev[n][evcnt]);
			}
			
				//	GET PITCH-INCR 

			outincr = evpch[n][evcnt];

				//	WRITE TO OUTPUT BUF WITH APPROPRIATE PITCHSHIFT, AND SPATIALISATION

			opos = evstt[n] * chans;			//	Change write position to frame of output
			opos -= total_samps_written; 		//	Change write position to frame of output-buffer
			lpos = opos + lmost[n];				//	Change write position to correct left and right channels
			rpos = opos + rmost[n];
			dipos = 0.0;
			while(dipos < ldur) {				//	This should wraparound OK, as buffer is preset with zeros
				iposlo = (int)floor(dipos);	//	So wraparound sample will be zero
				iposhi = iposlo+1;
				frac = dipos - (double)iposlo;
				loval = ebuf[iposlo];
				hival = ebuf[iposhi];
				diff  = hival - loval;
				diff *= frac;
				val = loval + diff;				//	Add the interpd enveloped-val into output buffer
				obuf[lpos] = (float)(obuf[lpos] + (val * llev[n]));
				obuf[rpos] = (float)(obuf[rpos] + (val * rlev[n]));
				dipos += outincr;				//	Interpolate in input (enveloped) sound
				lpos += chans;					//	Jump by a whole channel-group in output buffer
				rpos += chans;
			}
			this_write_end = (lpos/chans) * chans;		//	Round up to end of last-group written, to find end of write in buffer
			write_end = max(this_write_end,write_end);	//	Find write_end from all stream writes (and previous writes)
		}
		evcnt++;									//	Count event
	}
	if(write_end > 0) {
		for(n=0;n < write_end;n++)					//	Normalise remainder of output (ALL OF IT)
			obuf[n] = (float)(obuf[n] * normaliser);
		if(dz->mode == 0) {							//	In mode 0, write all of remaining output
			if((exit_status = write_samps(obuf,write_end,dz))<0)
				return(exit_status);
		} else {									//	In mode > 0, process output in chunks of buflen samps (or less)

			while(write_end > dz->buflen) {
				samps_to_process = dz->buflen;
				if(dz->fltractv) {
					if((exit_status = filter_process(&samps_to_process,&endzeros,dz))<0)
						return(exit_status);
				}
				if((exit_status = panspread(&sectcnt,&centre,&spread,&cntrswitch,&outside,&fmix,dz->buflen,dz))<0)
					return(exit_status);
				if(exit_status == CONTINUE) {
					write_end -= dz->buflen;
					for(n = 0;n < write_end;n++)		//	Copy back overflow into obuf (overflow may be longer than obuf)
						obuf[n] = ovflw[n];				//	and zero remainder of overflow
					memset((char *)(obuf+write_end),0,(dz->buflen + overflow - write_end) * sizeof(float));
				} else {
					write_end = 0;
					break;
				}
			}
			if(dz->fltractv) {						//	In the fitered case, filter will continue to output, even if no input
				samps_to_process = write_end;
				do {
					if((exit_status = filter_process(&samps_to_process,&endzeros,dz))<0)
						return(exit_status);		//	Filter returns number of samples that are not (trailing) zeros
					filter_state = exit_status;		//	& filter_state = CONTINUE if not at end of its data, or FINISHED if reached end
					if(samps_to_process) {			//	Only spatialise data if filter outputs some data ...
						if((exit_status = panspread(&sectcnt,&centre,&spread,&cntrswitch,&outside,&fmix,samps_to_process,dz))<0)
							return(exit_status);
					}								//	Quit this loop once filter runs to stream of zeros (filter_state = FINISHED)
					samps_to_process = 0;			//	After first buffer, no samples are being sent to filter
				} while(filter_state == CONTINUE);
			} else if(write_end > 0) {
				if((exit_status = panspread(&sectcnt,&centre,&spread,&cntrswitch,&outside,&fmix,write_end,dz))<0)
					return(exit_status);
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

/**************************** HANDLE_THE_SPECIAL_DATA ****************************/

int handle_the_special_data(char *str,dataptr dz)
{
	int exit_status;
	double dummy = 0.0, lasttime = 0.0, lastetime = 0.0, maxlevel = 0.0;
	FILE *fp;
	int cnt, linecnt;
	char temp[800], *p;

	if((fp = fopen(str,"r"))==NULL) {
		sprintf(errstr,"Cannot open file %s to read times.\n",str);
		return(DATA_ERROR);
	}
	linecnt = 0;
	while(fgets(temp,200,fp)!=NULL) {
		p = temp;
		while(isspace(*p))
			p++;
		if(*p == ';' || *p == ENDOFSTR)	//	Allow comments in file
			continue;
		cnt = 0;
		while(get_float_from_within_string(&p,&dummy)) {
			switch(cnt) {
			case(0):
				if(linecnt == 0) {
					if(dummy != 0) {
						sprintf(errstr,"First time (first entry in first line) in envelopes data (%lf) must be zero.\n",dummy);
						return(DATA_ERROR);
					} else
						lasttime = dummy;
				} else {
					if(dummy <= lasttime) {
						sprintf(errstr,"Times (first entry) do not advance at line %d in transpositions data.\n",linecnt+1);
						return(DATA_ERROR);
					}
				}
				break;
			case(1):
				if(dummy != 0) {
					sprintf(errstr,"First envelope time (2nd entry) (%lf) in envelope data line (%d) must be zero.\n",dummy,linecnt+1);
					return(DATA_ERROR);
				}
				lastetime = dummy;
				break;
			case(2):
				if(dummy != 0) {
					sprintf(errstr,"First level (3rd entry) (%lf) in envelope data line (%d) must be zero.\n",dummy,linecnt+1);
					return(DATA_ERROR);
				}
				maxlevel = dummy;
				break;
			case(13):
				if(!flteq(dummy,1.0)) {
					sprintf(errstr,"Last envelope time (14th entry) (%lf) in envelope data line (%d) must be ONE.\n",dummy,linecnt+1);
					return(DATA_ERROR);
				}
				break;
			case(14):
				if(dummy != 0) {
					sprintf(errstr,"Last level (15th entry) (%lf) in envelope data line (%d) must be zero.\n",dummy,linecnt+1);
					return(DATA_ERROR);
				}
				break;
			default:
				if(dummy < 0.0 || dummy > 1.0) {
					sprintf(errstr,"Values and envelope-times in envelope data must lie between 0 and 1 (see line %d).\n",linecnt+1);
					return(DATA_ERROR);
				}
				if(ODD(cnt)) {	// ODD entries, envelope-time
					if(dummy <= lastetime) {
						sprintf(errstr,"Envelope times do not increase at entry %d in line %d.\n",cnt+1,linecnt+1);
						return(DATA_ERROR);
					}
					lastetime = dummy;
				} else
					maxlevel = max(dummy,maxlevel);
				break;
			}
			cnt++;
		}
		if(cnt != 15) {
			sprintf(errstr,"Invalid number of entries (%d) on line %d: should be 15 (1 time plus 7-etime-level pairs\n",cnt,linecnt+1);
			return(DATA_ERROR);
		}
		if(!flteq(maxlevel,1.0)) {
			sprintf(errstr,"Envelope data in line %d does not rise to maximum of 1.0.\n",linecnt+1);
			return(DATA_ERROR);
		}
		linecnt++;
	}
	if(linecnt == 0) {
		sprintf(errstr,"No data found in envelope data file.\n");
		return(DATA_ERROR);
	}
	dz->envcount = linecnt;		//	Number of different envelopes ....
	dz->all_words = 0;
	if((exit_status = store_filename(str,dz))<0)
		return(exit_status);
	fclose(fp);
	return FINISHED;
}

/**************************** READ_THE_SPECIAL_DATA ****************************/

int read_the_special_data(dataptr dz)
{
	double *envdata, dummy;
	FILE *fp;
	int cnt;
	char temp[800], *p;

	envdata = dz->parray[dz->envsrcs];
	if((fp = fopen(dz->wordstor[0],"r"))==NULL) {
		sprintf(errstr,"Cannot open file %s to envelope data times.\n",dz->wordstor[0]);
		return(DATA_ERROR);
	}
	cnt = 0;
	while(fgets(temp,200,fp)!=NULL) {
		p = temp;
		while(isspace(*p))
			p++;
		if(*p == ';' || *p == ENDOFSTR)	//	Allow comments in file
			continue;
		while(get_float_from_within_string(&p,&dummy)) {
			envdata[cnt] = dummy;
			cnt++;
		}
	}
	return FINISHED;
}

/**************************** FRAGMENT_PARAM_PREPROCESS ****************************/

int fragment_param_preprocess (int *maxpulse,double *maxtrans,int *minfragmax,dataptr dz)
{
	int exit_status, chans, strmcnt;
	int n, m, eventcnt;
	double d_minpulse, d_maxpulse, d_minfragmax, srate, leftgain, rightgain, val;
	int *lmost, *rmost;
	double *llev, *rlev, *pos;

	if(dz->mode == 0)
		chans = dz->iparam[FRAC_CHANS];
	else
		chans = STEREO;
	strmcnt = dz->iparam[FRAC_STRMS];
	srate = (double)dz->infile->srate;

	if(dz->brksize[FRAC_PCHRND]) {
		for(n=0,m=1;n < dz->brksize[FRAC_PCHRND];n++,m+=2) {
			val = dz->brk[FRAC_PCHRND][m];
			val /= 100.0;									//	Convert from cents to semitones
			dz->brk[FRAC_PCHRND][m] = val;
		}
		if((exit_status = get_maxvalue_in_brktable(&val,FRAC_PCHRND,dz))<0)
			return PROGRAM_ERROR;
		*maxtrans = pow(2.0,(val/SEMITONES_PER_OCTAVE));
	} else {
		dz->param[FRAC_PCHRND] /= 100.0;
		val = dz->param[FRAC_PCHRND];
		*maxtrans = pow(2.0,(val/SEMITONES_PER_OCTAVE));	//	Retain max transposition for calculation of buffer sizes
	}
	if(dz->brksize[FRAC_MIN]) {
		if((exit_status = get_maxvalue_in_brktable(&d_minfragmax,FRAC_MIN,dz))<0)
			return PROGRAM_ERROR;
	} else
		d_minfragmax = dz->param[FRAC_MIN];
	*minfragmax = (int)ceil(d_minfragmax * srate);
	if(dz->brksize[FRAC_PULSE]) {
		if((exit_status = get_minvalue_in_brktable(&d_minpulse,FRAC_PULSE,dz))<0)
			return PROGRAM_ERROR;
		if((exit_status = get_maxvalue_in_brktable(&d_maxpulse,FRAC_PULSE,dz))<0)
			return PROGRAM_ERROR;
	} else {
		d_minpulse = dz->param[FRAC_PULSE];
		d_maxpulse = dz->param[FRAC_PULSE];
	}
	d_maxpulse *= 2.0;	//	Allow for maximum randomisation extension of pulse-duration = 1/2 pulse dur, but at BOTH ends
	*maxpulse = (int)ceil(d_maxpulse * srate);
	d_minpulse /= 2.0;	//	Allow for maximum randomisation reduction of pulse-duration
	eventcnt = (int)ceil(dz->duration/d_minpulse) + 16;	// SAFETY

/*
 *	DOUBLE ARRAYS: S = number of streams									|				llev
 *							|												|				| rlev
 *			|---------------|---------------|---------------|---------------|---------------| | pos
 *			|	depth		|	level		|	transpos	|	envreadtime	|	stacking	| | | tempenv  
 *			|				|				|				|				|				5S| | | envdata
 *			|				|				|				|				|				| 5S+1| | stakcentre
 *	address	0				S				2S				3S				4S				| | 5S+2| | grptimes
 *			|				|				|				|				|				| | | 5S+3| | chan-levels(mode>0 only)
 *			|				|				|				|				|				| | | | 5S+4| | |
 *	 array	|	maxevents	|	maxevents	|	maxevents	|	maxevents	|	maxevents	| | | | | 5S+5| |
 *	length	|				|				|				|				|				S S S | | | 5S+6| then 11 filter arrays
 *			|				|				|				|				|				| | | 14| | | | |
 *			|				|				|				|				|				| | | | dz->envcount * 15
 *			|				|				|				|				|				| | | | | S	| | |
 *			|				|				|				|				|				| | | | | | maxevents
 *																							| | | |	| | | outchans
 */
	if((dz->parray = (double **)malloc(((strmcnt * 5) + 19) * sizeof(double *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for double arrays.\n");
		return(MEMORY_ERROR);
	}
	for(n=0,m=1;n<strmcnt;n++,m++) {											//	Envelope depth/stack of each-event-in-each-stream 
		if((dz->parray[n] = (double *)malloc(eventcnt * sizeof(double)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for stream %d depth array.\n",m);
			return(MEMORY_ERROR);
		}
	}
	for(m=1;n<strmcnt*2;n++,m++) {												//	Level of each-event-in-each-stream
		if((dz->parray[n] = (double *)malloc(eventcnt * sizeof(double)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for stream %d level array.\n",m);
			return(MEMORY_ERROR);
		}
	}
	for(m=1;n<strmcnt*3;n++,m++) {												//	Transpos for each-event-in-each-stream
		if((dz->parray[n] = (double *)malloc(eventcnt * sizeof(double)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for stream %d transposition array.\n",m);
			return(MEMORY_ERROR);
		}
	}
	for(m=1;n<strmcnt*4;n++,m++) {												//	Times in envtable where envdata read, for each-event-in-each-stream
		if((dz->parray[n] = (double *)malloc(eventcnt * sizeof(double)))==NULL) {	//	NB these could have been randomised
			sprintf(errstr,"INSUFFICIENT MEMORY for stream %d envelope readtime array.\n",m);
			return(MEMORY_ERROR);
		}
	}
	for(m=1;n<strmcnt*5;n++,m++) {												//	Stacking values for each-event-in-each-stream
		if((dz->parray[n] = (double *)malloc(eventcnt * sizeof(double)))==NULL) {	//	NB these could have been randomised
			sprintf(errstr,"INSUFFICIENT MEMORY for stream %d envelope readtime array.\n",m);
			return(MEMORY_ERROR);
		}
	}
	if((dz->parray[n++] = (double *)malloc(strmcnt * sizeof(double)))==NULL) {	//	Stream level at leftmost channel
		sprintf(errstr,"INSUFFICIENT MEMORY for left-levels array.\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[n++] = (double *)malloc(strmcnt * sizeof(double)))==NULL) {	//	Stream level at rightmost channel
		sprintf(errstr,"INSUFFICIENT MEMORY for right-levels array.\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[n++] = (double *)malloc(strmcnt * sizeof(double)))==NULL) {	//	Positioning of output streams
		sprintf(errstr,"INSUFFICIENT MEMORY for storing input envelope data.\n");
		return(MEMORY_ERROR);
	}
	dz->envbuf = n;		//	Store array number of temporary envelope
	if((dz->parray[n++] = (double *)malloc(14 * sizeof(double)))==NULL) {		//	Stores temporary envelope calculated for specific event
		sprintf(errstr,"INSUFFICIENT MEMORY for current envelope array.\n");
		return(MEMORY_ERROR);
	}
	dz->envsrcs = n;		//	Store array number of input envelope data
	if((dz->parray[n++] = (double *)malloc((dz->envcount * 15) * sizeof(double)))==NULL) {	//	Original envelope data in blocks of time+14 = 15
		sprintf(errstr,"INSUFFICIENT MEMORY for storing input envelope data.\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[n++] = (double *)malloc(strmcnt * sizeof(double)))==NULL) {	//	Centre of envelope (for stacking)
		sprintf(errstr,"INSUFFICIENT MEMORY for storing input envelope data.\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[n++] = (double *)malloc(eventcnt * sizeof(double)))==NULL) {		//	Group-time of events
		sprintf(errstr,"INSUFFICIENT MEMORY for storing input envelope data.\n");
		return(MEMORY_ERROR);
	}
	if(dz->mode > 0) {																//	Channel levels in calculation of multichannel output 
		dz->outlevs = n;
		if((dz->parray[n] = (double *)malloc(dz->iparam[FRAC_CHANS] * sizeof(double)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to store out-levels for multichannel output.\n");
			return(MEMORY_ERROR);
		}
	}
/*
 *	INTEGER ARRAYS: S = number of streams
 *
 *			lmost
 *			| rmost
 *			| | |
 *	address	0 1 |
 *			| | |
 *	 array	S S |
 *   length	| | |
 */
	if((dz->iparray = (int **)malloc(2 * sizeof(int *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for integer arrays.\n");
		return(MEMORY_ERROR);
	}
	if((dz->iparray[0] = (int *)malloc(strmcnt * sizeof(int)))==NULL) {	//	leftmost channel for each stream
		sprintf(errstr,"INSUFFICIENT MEMORY for stream leftmost channels.\n");
		return(MEMORY_ERROR);
	}
	if((dz->iparray[1] = (int *)malloc(strmcnt * sizeof(int)))==NULL) {	//	rightmost channel for each stream
		sprintf(errstr,"INSUFFICIENT MEMORY for stream rightmost channels.\n");
		return(MEMORY_ERROR);
	}

/*
 *	LONG ARRAYS: S = number of streams      iptr (start of read WITHIN current input buffer, for each stream)
 *											| temp storage of event starttimes
 *			|---------------|---------------| | |
 *			|  segendsamp	|	inreadsamp	| |	|	NB segendsamp is in MONO samps
 *	address	0				S				2S|	|	need to change to (segendsamp * chans)
 *			|				|				| 2S+1	+ lmost(rmost) % dz->buflen
 *	 array	|	maxevents	|	maxevents	| |	|	for write!!
 *	length	|				|				strmcnt
 *			|				|				| strmcnt
 */

	if((dz->lparray = (int **)malloc(((strmcnt*2)+2) * sizeof(int *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for ints arrays.\n");
		return(MEMORY_ERROR);
	}
	for(n=0,m=1;n<strmcnt;n++,m++) {							//	End sample for each-event-in-each-stream, counted in MONO 
		if((dz->lparray[n] = (int *)malloc(eventcnt * sizeof(int)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for stream %d eventend sampletimes.\n",m);
			return(MEMORY_ERROR);
		}
	}
	for(m=1;n<strmcnt*2;n++,m++) {								//	Startsample in source for each-event-in-each-stream
		if((dz->lparray[n] = (int *)malloc(eventcnt * sizeof(int)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for stream %d eventread sampletimes.\n",m);
			return(MEMORY_ERROR);
		}
	}															//	Pointers within CURRENT buffer, to start of segments to read, for each stream	
	if((dz->lparray[n++] = (int *)malloc(strmcnt * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for stream src Pointers in input buffers.\n");
		return(MEMORY_ERROR);
	}															//	Temporary storage of event start-times for each stream
	if((dz->lparray[n] = (int *)malloc(strmcnt * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for stream src Pointers in input buffers.\n");
		return(MEMORY_ERROR);
	}

	if((exit_status = read_the_special_data(dz))<0) //	Read the envelope data input
		return exit_status;

	//	Assign output streams to correct positions among output channels
	
	lmost = dz->iparray[0];
	rmost = dz->iparray[1];
	llev =  dz->parray[strmcnt * 5];
	rlev =  dz->parray[(strmcnt * 5) + 1];
	pos  =  dz->parray[(strmcnt * 5) + 2];

	if(strmcnt == chans) {
		for(n = 0;n<chans;n++) {
			lmost[n] = n;
			rmost[n] = (lmost[n] + 1) % chans;
			llev[n] = 1.0;
			rlev[n] = 0.0;
		}
	} else {
		if((dz->mode == 0 && dz->vflag[1]) || chans == 2) {
			for(n = 0;n<strmcnt;n++) {
				pos[n] = ((chans - 1) * n)/(double)(strmcnt - 1);
				lmost[n] = (int)floor(pos[n]);
				pos[n]  -= lmost[n]; 
			}
		} else {
			for(n = 0;n<strmcnt;n++) {
				pos[n] = (chans * n)/(double)strmcnt;
				lmost[n] = (int)floor(pos[n]);
				pos[n]  -= lmost[n]; 
			}
		}
		for(n = 0;n<strmcnt;n++) {
			rmost[n] = (lmost[n] + 1) % chans;
			if(flteq(pos[n],0.0)) {
				rlev[n] = 0.0;
				llev[n] = 1.0;	//	pos values overwritten by associated level values (ETC below)
			} else if(flteq(pos[n],1.0)) {
				rlev[n] = 1.0;
				llev[n] = 0.0;
			} else {
				pos[n] *= 2.0;
				pos[n] -= 1.0;	//	Change position to -1 to +1 range
				pancalc(pos[n],&leftgain,&rightgain);
				rlev[n] = rightgain;
				llev[n] = leftgain;
			}
		}
	}
	if(dz->mode > 0) {

		//	SET UP FILTER

		if((exit_status = setup_lphp_filter(dz))<0) //	Set up the distance filter
			return exit_status;
	}
	return FINISHED;
}

/**************************** CREATE_FRACTURE_SNDBUFS ****************************/

int create_fracture_sndbufs(int maxpulse,double maxtrans,int minfragmax,int *ibuflen,int *envbuflen,int *overflow, int *mbuflen, dataptr dz)
{
	int framesize = dz->infile->channels * F_SECSIZE;
	int chans;
	unsigned int bigbufsize;
	int seccnt, obuflen, fbuflen = 0;
	double d_obuflen;
	if(dz->mode == 0)
		chans = dz->iparam[FRAC_CHANS];
	else
		chans = STEREO;
	if((dz->sampbuf = (float **)malloc(sizeof(float *) * 8))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffers.\n");
		return(MEMORY_ERROR);
	}
	if((dz->sbufptr = (float **)malloc(sizeof(float *) * 8))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffer pointers.\n");
		return(MEMORY_ERROR);
	}
	*ibuflen = maxpulse;				//	Already allows for maximum random expansion of maximum read-segment length
	*ibuflen *= 3;						//	Allow for maximum offset of reads in different streams
	*ibuflen += minfragmax;				//	Allows for minimum fragment-size being larger than maxpulse
	*ibuflen += 64;						//	SAFETY
	seccnt = *ibuflen/framesize;
	if(seccnt * framesize != *ibuflen) {
		seccnt++;
		*ibuflen = seccnt * framesize;
	}
	*envbuflen = maxpulse * 2;			//	Frag start and ends can be randomly displaced away from one another ... allow a maximum safety margin!!
	*envbuflen += minfragmax;			//	If a minimum fragment-size specified, allows for length of minsize + any rand-extension 
	*envbuflen += 16;	//	SAFETY
	seccnt = (*envbuflen)/framesize;
	if(seccnt * framesize != *envbuflen) {
		seccnt++;
		*envbuflen = seccnt * framesize;
	}
	d_obuflen = (double)maxpulse;
	if(maxtrans > 1.0)					//	Allow for maximum transposition (downwards) of data (redundant: transpos always upwards)
		d_obuflen *= maxtrans;
	obuflen = (int)ceil(d_obuflen) + 16;	//	SAFETY
	obuflen *= chans;
	seccnt = obuflen/framesize;
	if(seccnt * framesize != obuflen) {
		seccnt++;
		obuflen = seccnt * framesize;
	}
	if(dz->mode > 0) {
		*mbuflen = (obuflen/STEREO) * dz->iparam[FRAC_CHANS];
		fbuflen = obuflen;
	}
	*overflow = obuflen * 2;				//	Allow for 2 whole maximal segments overwrite of end of output buffer
	*overflow += minfragmax;				//	Allow for min-length segments which are bigger than maxpulse

	dz->buflen = obuflen;
	bigbufsize = *ibuflen + (*envbuflen * 2) + obuflen + *overflow + fbuflen + *mbuflen;
	if((dz->bigbuf = (float *)malloc(bigbufsize * sizeof(float))) == NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
		return(MEMORY_ERROR);
	}
/*
 *	SOUND BUFFERS
 *
 *	MODE 1 |	input	| enveloping|  stacking	|  output	| overflow	|
 *
 *	MODE 2 |	input	| enveloping|  stacking	|   stereo	|	stereo	|  filter	| multichan |
 *													output	   overflow		buffer		output
 */
	dz->sampbuf[0] = dz->bigbuf;
	dz->sbufptr[0] = dz->sampbuf[0];
	dz->sampbuf[1] = dz->sampbuf[0] + *ibuflen;
	dz->sbufptr[1] = dz->sampbuf[1];
	dz->sampbuf[2] = dz->sampbuf[1] + *envbuflen;
	dz->sbufptr[2] = dz->sampbuf[2];
	dz->sampbuf[3] = dz->sampbuf[2] + *envbuflen;
	dz->sbufptr[3] = dz->sampbuf[3];
	dz->sampbuf[4] = dz->sampbuf[3] + obuflen;
	dz->sbufptr[4] = dz->sampbuf[4];
	dz->sampbuf[5] = dz->sampbuf[4] + *overflow;
	dz->sbufptr[5] = dz->sampbuf[5];
	dz->sampbuf[6] = dz->sampbuf[5] + fbuflen;
	dz->sbufptr[6] = dz->sampbuf[6];
	dz->sampbuf[7] = dz->sampbuf[6] + *mbuflen;
	dz->sbufptr[7] = dz->sampbuf[7];
	return FINISHED;
}

/**************************** READ_THE_ENVELOPE ****************************/

int read_the_envelope(double now, dataptr dz)
{
	double *envarray = dz->parray[dz->envsrcs], *tempenv = dz->parray[dz->envbuf];
	int n, m, k, thisenv, lastenv, lastj, thisj, endj;
	double time_of_envelope, lasttime, thistime, timediff, timefrac, val;

	for(n = 0,m= 0;m < dz->envcount; n+=15,m++) {	//	Step through the time values, at every 15th entry
		time_of_envelope = envarray[n];	
		if(time_of_envelope >= now) {			//	Once time-of-envelope is beyond "now" time
			thisenv = m;						//	Mark the two envelopes bracketing "now"
			lastenv = m-1;
			if(lastenv < 0) {					//	If there is no previous envelope, we must be at zero time 
				for(k=0;k<14;k++)				//	so copy envelope at zero time to output (temporary envelope)
					tempenv[k] = envarray[k+1];
				return FINISHED;
			} else {							//	Otherwise, get the array-indexes of the times of the bracketing envelopes
				lastj = lastenv * 15;
				thisj = thisenv * 15;
				endj = thisj;					//	And remember the index of the END of the last-envelope data
				lasttime = envarray[lastj];		//	get those bracketing times
				thistime = envarray[thisj];		//	and calculate the fraction of time we are at, between the bracketing times
				timediff = thistime - lasttime;
				timefrac = (now - lasttime)/timediff;
				lastj++;						// Step from time-of-envelope to envelope data itself
				thisj++;
				k = 0;							//	Initialise counter for output temporary-envelope
				while(lastj < endj) {			//	For every (time and level) entry in the bracketing envelopes
					val = envarray[thisj] - envarray[lastj];
					val *= timefrac;			//	Interpolate using timefrac
					val += envarray[lastj];
					tempenv[k++] = val;
					lastj++;
					thisj++;
				}
				return FINISHED;
			}
		}
	}									//	If we've gone through all data without reaching input "time", use final envelope
	lastenv = m-1;
	lastj = lastenv * 15;
	lastj++;
	for(k=0;k<14;k++)
		tempenv[k] = envarray[lastj++];
	return FINISHED;
}

/**************************** GETSTAKCENTRE ****************************
 * 
 * Find time of (first) peak in envelope
 */

double getstakcentre(double *tempenv,dataptr dz)
{
	int tim, lev;
	double maxlev = -1.0;
	double maxtim = 0.0;
	for(tim=0,lev=1;tim < 14;tim+=2,lev+=2) {
		if(tempenv[lev] > maxlev) {
			maxlev = tempenv[lev];
			maxtim = tempenv[tim];
		}
	}
	return maxtim;
}

/**************************** GETSTAKWRITESTT ****************************
 *
 *	Find sample in stak-buffer where transposed-data, centred on peak, should start to be written
 */

int getstakwritestt(double stakcntr,int ldur,double incr)
{
	double stakcentre = (double)ldur * stakcntr;
	double shrinkto = stakcentre/incr;
	return (int)round(stakcentre - shrinkto);
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

/*************************** PANSPREAD ****************************
 *
 *	This function takes the output of mode 0 (but forced to stereo)
 *	and uses it as the stereo input to the multichannel spatialiseation process (of "mchanpan" - MODE 3- stereo input branch)
 *
 *	Data is fed in in chunks of dz->buflen (or possibly less for the last chunk)
 *	and written to multichannel output from here.
 *
 *	There is no reason to run this part of the function during the level-testing pass
 *	as, once the stereo level has been normalised, there is no (per channel) level increase at this stage.
 *
 *	Signal gain has several componenets
 *	Stereo positioning gain (relative level on Left/Right loudspeakers) is already included in the input stereo signal
 *	Position gain perpendicular to stereo-plain OR attenuation with distance beyond the circle
 *	Natural attenuation of the image "atten" where it fades before reaching infinity.
 *	Externally imposed gain-reduction "gain" to avoid an overload.
 */

int panspread(int *sectcnt,double *centre,double *spread,int *cntrswitch,int *outside,double *fmix,int samps_to_process,dataptr dz)
{
	int exit_status, pan_exit_status = CONTINUE, outchans = dz->iparam[FRAC_CHANS], m, k;
	int ibufpos, obufpos = 0;
	double *levels;
	int inchans = STEREO;
	float *ibuf = dz->sampbuf[3];	//	Input buffer is the former "obuf", now used as intermediate stereo-storage
	float *fbuf = dz->sampbuf[5];	//	buffer for filtering
	float *obuf = dz->sampbuf[6];	//	and the output buffer is the multichan-outbuf "mbuf"
	
	double time = 0.0, srate = (double)dz->infile->srate, gain = dz->param[FRAC_GAIN];
	int obuflen = (dz->buflen/STEREO) * outchans;
	double stereo_pos, atten, timeratio;
	double relpos, temp, holecompensate;
	double left_leftchan_level, left_ritechan_level;
	double  valcentre, valoffcen;
	double fadeoutat = dz->fulldur - dz->param[FRAC_DN];
	int halfstage = outchans/2;
	int c_left, c_rite, cen_tr;
	double left_contrib_to_c_left, rite_contrib_to_c_left, left_contrib_to_c_rite, rite_contrib_to_c_rite, val;

	levels = dz->parray[dz->outlevs];	//	Array created earlier.

	memset((char *)obuf,0,obuflen * sizeof(float));

	if(dz->param[FRAC_DN] <= 0.0)
		fadeoutat = dz->fulldur * 200.0;	//	i.e. an unreachable time

	for(ibufpos = 0, obufpos = 0;ibufpos < samps_to_process;ibufpos+=inchans,obufpos += outchans) {
		if((*sectcnt) % 256 == 0) {
			time = (double)(*sectcnt)/srate;
			if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
				return(exit_status);
			if(spread_pan(centre,cntrswitch,outside,spread,fmix,dz) == FINISHED) {
				pan_exit_status = FINISHED;
				break;
			}
		}
		if(time < dz->param[FRAC_UP]) {					//	Factor in any natural decay of event (i.e NOT distance realted)
			timeratio = time/dz->param[FRAC_UP];
			atten = pow(timeratio,dz->param[FRAC_ATTEN]);
		} else if(time > fadeoutat) {
			timeratio = (time - fadeoutat)/dz->param[FRAC_DN];
			timeratio = 1.0 - timeratio;
			atten = pow(timeratio,dz->param[FRAC_ATTEN]);
		} else			
			atten = 1.0;
		atten *= gain;									//	Factor in any externally imposed gain-limiting.
		if(*outside) {
			stereo_pos = 1.0 - *spread;					//	Spread is to left of centre, incresing leftward, but stereo-position measured rightward
			relpos = fabs(0.5 - stereo_pos) * 2.0;		// position relative_to_stereo_centre : Range 0-1 goes to 1-0-1
			temp = 1.0 + (relpos * relpos);				// calculate stereo-hole-in-middle compensation 
			holecompensate = ROOT2 / sqrt(temp);
			left_leftchan_level = *spread       * levels[0] * holecompensate;	// contrib of left chan to left edge of shrunk image			
			left_ritechan_level = (1-(*spread)) * levels[0] * holecompensate;	// contrib of right chan to left edge of shrunk image

			//	ADD IN ANY FILTERED SOUND, TO STEREO IMAGE

			if(*fmix > 0) {
				ibuf[ibufpos]   = (float)(((1-(*fmix)) * ibuf[ibufpos])   + (*fmix * fbuf[ibufpos]));
				ibuf[ibufpos+1] = (float)(((1-(*fmix)) * ibuf[ibufpos+1]) + (*fmix * fbuf[ibufpos+1]));
			}
			if(*cntrswitch == 0) {
				valcentre = ibuf[ibufpos+1];	//	The RHS of stereo will remain where it is
				valoffcen = ibuf[ibufpos];		//	The LHS will drift towards the right, till they become mono, at right (=final image centre)
			} else {
				valcentre = ibuf[ibufpos];		//	LHS & RHS of original stereo signal switched, because centre moved to opposite pole: see elsewhere
				valoffcen = ibuf[ibufpos+1];
			}
			valcentre = valcentre + (left_ritechan_level * valoffcen);	//	Stereo-right + an increasing portion of stereo-left
			valoffcen = left_leftchan_level * valoffcen;				//	a decreasing portion of stereo left
			cen_tr = (int)floor(*centre);
			c_left = cen_tr - 1;
			if(c_left < 0)
				c_left += outchans;
			obuf[obufpos + cen_tr] = (float)(valcentre * atten * levels[0]);	//	Attenuation is any dying away of signal proper (nothing to do with distance)
			obuf[obufpos + c_left] = (float)(valoffcen * atten * levels[0]);	//	levels[0] is the level associated with distance from circle
		} else {
			c_left = (int)floor(*centre);
			c_rite = c_left + 1;
			if(c_rite >= outchans)
				c_rite -= outchans;
			for(k=1;k<halfstage;k++) {			//	All loudpseakers to left of central pair get left signal (unless reading from opp. centre : cntrswitch = 1)
				m = c_left - k;					//	Values in "levels" decide how much of the signal is played
				if(m < 0)
					m += outchans;
				if(*cntrswitch)
					obuf[obufpos + m] = (float)(ibuf[ibufpos+1] * levels[m] * atten);
				else
					obuf[obufpos + m] = (float)(ibuf[ibufpos] * levels[m] * atten);
			}
			for(k=1;k<halfstage;k++) {			//	All loudpseakers to right of central pair get right signal (unless etc.)
				m = c_rite + k;
				if(m >= outchans)
					m -= outchans;
				if(*cntrswitch)
					obuf[obufpos + m] = (float)(ibuf[ibufpos] * levels[m] * atten);
				else
					obuf[obufpos + m] = (float)(ibuf[ibufpos+1] * levels[m] * atten);
			}									//	Central pair get a mix of left and right signals
			stereo_pos = *centre - (double)c_left;
			left_contrib_to_c_left = (1.0 + (2.0 * stereo_pos))/2.0;
			left_contrib_to_c_left = min(1.0,left_contrib_to_c_left);
			rite_contrib_to_c_left = (1.0 - (2.0 * stereo_pos))/2.0;
			rite_contrib_to_c_left = max(0.0,rite_contrib_to_c_left);
			if(*cntrswitch)
				val = (ibuf[ibufpos+1] * left_contrib_to_c_left) + (ibuf[ibufpos] * rite_contrib_to_c_left);
			else
				val = (ibuf[ibufpos] * left_contrib_to_c_left) + (ibuf[ibufpos+1] * rite_contrib_to_c_left);
			val *= levels[c_left];
			obuf[obufpos + c_left] = (float)(val * atten);

			left_contrib_to_c_rite = ((2.0 * stereo_pos) - 1.0)/2.0;
			left_contrib_to_c_rite = max(0.0,left_contrib_to_c_rite);
			rite_contrib_to_c_rite = (3.0 - (2.0 * stereo_pos))/2.0;
			rite_contrib_to_c_rite = min(1.0,rite_contrib_to_c_rite);
			if(*cntrswitch)
				val = (ibuf[ibufpos+1] * left_contrib_to_c_rite) + (ibuf[ibufpos] * rite_contrib_to_c_rite);
			else
				val = (ibuf[ibufpos] * left_contrib_to_c_rite) + (ibuf[ibufpos+1] * rite_contrib_to_c_rite);
			val *= levels[c_rite];
			obuf[obufpos + c_rite] = (float)(val * atten);
			left_contrib_to_c_rite = ((2.0 * stereo_pos) - 1.0)/2.0;
		}
		(*sectcnt)++;
	}
	if(obufpos > 0) {
		if((exit_status = write_samps(obuf,obufpos,dz))<0)
			return(exit_status);
	}
	return pan_exit_status;
}

/* WITH CENTRE BETWEEN CHANNELS ...
 *		left_contrib_to_c_left =		*		rite_contrib_to_c_left =		*		left_contrib_to_c_rite =		*		rite_contrib_to_c_rite =		
 *			MAX of 1.0 AND				*			MIN of 0.0 AND				*			MAX of 0.0 AND				*			MAX of 1.0 AND				
 *	  (1.0 + (2.0 * stereo_pos)/2.0;	*	  (1.0 - (2.0 * stereo_pos)/2.0;	*	  ((2.0 * stereo_pos) - 1.0)/2.0;	*	  (3.0 - (2.0 * stereo_pos)/2.0;		
 *										*										*										*										
 *			  POSITION OF CENTRE		*			  POSITION OF CENTRE		*			  POSITION OF CENTRE		*			  POSITION OF CENTRE		
 *		  c-left			  c_rite	*		  c-left			  c_rite	*		  c-left			  c_rite	*		  c-left			  c_rite	
 *		1.0	| . . . .________|			*		1.0	|. . . . . . . . |			*		1.0	| . . . . . . . .|			*		1.0	|________  . . . |			
 *			|	   /		 |			*			|				 |			*			|				 |			*			|		  \		 |			
 *			|    /			 |			*			|    			 |			*			|    			 |			*			|    		\	 |			
 *			|  /			 |			*			|				 |			*			|				 |			*			|			  \	 |			
 *		0.5	|/ . . . . . . . |			*		0.5	|. . . . . . . . |			*		0.5	|. . . . . . . . |			*		0.5	|. . . . . . . .\|			
 *			|				 |			*			|\				 |			*			|				/|			*			|				 |			
 *			|				 |			*			|  \			 |			*			|			  /	 |			*			|				 |			
 *			|				 |			*			|	 \			 |			*			|			/	 |			*			|	 			 |			
 *		0.0	| . . . . . .  . |			*		0.0	| . .  \_________|			*		0.0	|_________/ .  . |			*		0.0	| . . . . . . . .|			
 *
 */

/*************************** SPREAD_PAN ****************************/

int spread_pan(double *thiscentre,int *cntrswitch,int *outside,double *spread,double *fmix,dataptr dz)
{
	int outchans   = dz->iparam[FRAC_CHANS];
	double centre  = (double)dz->iparam[FRAC_CENTRE];
	double halfspread = 0.0, front = dz->param[FRAC_FRONT], ffront;
	double depth;
	double rolloff = dz->param[FRAC_ROLLOFF];
	double *levels = dz->parray[dz->outlevs];
	double spredej_left, spredej_right;
	int spredej_left_leftchan, spredej_left_rightchan, spredej_right_leftchan, spredej_right_rightchan, k, j, ochan;
	double range, maxlevel, hole, stereopos_left, stereopos_right, relpos, temp, holecompensate;
	double holej_right, holej_left, zleft, zright, floor_zleft, floor_zright;
	double left_leftchan_level, right_rightchan_level;
	double kk, jj, holing, mingap;
	double contraction_distance, filter_distance;

	*cntrswitch = 0;
	*outside = 0;

	depth = (dz->param[FRAC_MDEPTH] * outchans)/2.0;	//	Find (max) no of outchans to turn on
	depth = min(depth,(double)outchans/2.0);			//	depth (no of ON lspks) to each side of centre is 1/2 of this

	if((centre = centre - 1.0) < 0.0)					//	Change from 1-to-N to 0-to-(N-1) frame, for calculations
		centre += (double)outchans;

	if(fabs(front) <= 1.0) {							//	If WITHIN the circle of lspkrs
		centre -= 0.5;									//	force the active centre to be BETWEEN 2 channels
		if(centre < 0.0)
			centre += outchans;
		if(front >= 0)									//	Front moves from a spread of 1 lspkr-width (at 1)
			*spread = (1-front) * (outchans/2 - 1) + 1;	//	to a spread of 1/2-of-lspkrs (outchans/2) at midline
		else {												
			front = -front;								//	After midline, hole in lspkrs has same relation to (abs value of) front.
			*spread = (1-front) * (outchans/2 - 1) + 1;  //	so calculate symmetrically
			*spread = outchans - *spread;				//	then subtract hole from total lspkrs
		}
	} else {											//	Leading-edge of front is outside circle of lspkrs
		*outside = 1;
		if(front < -1) {								//	If passed OUT of ring, place front at its TRAILING edge, for calculations
			front += (dz->param[FRAC_MDEPTH] * 2);
			if(front <= -2)								//	Once trailing edge of data reaches infinity, curtail output 
				return FINISHED;
			else if(front >= -1) {						//	If trailing edge of front has (unlike true front) NOT left circle of lspkrs
				centre -= 0.5;							//	force the active centre to be BETWEEN 2 channels
				if(centre < 0.0)
					centre += outchans;
				*outside = 0;							//	Do normal inside-circle calculations BUT
				centre -= (double)(outchans/2);			//	Invert frame of reference
				if(centre < 0.0)						//	 centre -> leaving-centre opposite original centre
					centre += (double)outchans;
				front  = -front;						//	Front inverted symmetrically about centre-line (0)
				*cntrswitch = 1;						//	and left and right inverted in calling loop
				if(front >= 0)							//	Then calculate spread in normal way, but from this leaving-centre
					*spread = (1-front) * (outchans/2 - 1) + 1;
				else {
					front = -front;
					*spread = (1-front) * (outchans/2 - 1) + 1;
					*spread = outchans - *spread;
				}										//	As depth MUST now be covering all chans from trailing edge (new "front") to leaving-centre
				depth = (*spread)/2.0;					//	set depth to real halfspread,
			}
		}
		if(*outside) {									//	Once front OUTSIDE circle, shrink to MONO into the original-specified central channel
			ffront = fabs(front);						//	Calculations for outside ring are symmetrical	(Range 1 to 2)
			ffront -= 1.0;								//	Change range to 0-1 : ringedge-to-infinity		(Range 0 to 1)

			maxlevel = pow((1.0 - ffront),dz->param[FRAC_ATTEN]);				//	As front increases, loudness decreases 
																				//	(and only 1channel active, so no scaling required for no. of chans				
			contraction_distance = min(1.0,ffront/dz->param[FRAC_ZPOINT]);		//	Fraction of distance towards zero-angle point, max 1.0
			*spread = pow((1.0 - contraction_distance),dz->param[FRAC_CONTRACT]);//	As contraction_distance increases, width decreases
			halfspread = (*spread)/2.0;

			filter_distance = min(1.0,ffront/dz->param[FRAC_FPOINT]);			//	Fraction of distance towards total-filering point, max 1.0
			*fmix = pow(filter_distance,dz->param[FRAC_FFACTOR]);				//	As filter_distance increases, proportion filtered sig in mix increases

			centre  = (double)dz->iparam[FRAC_CENTRE];	//	Revert to original channel-centred centre
			if((centre = centre - 1.0) < 0.0)			//	Change from 1-to-N to 0-to-(N-1) frame, for calculations
				centre += outchans;
			if(front < 0.0) {							//	If LEAVING circle,
				*cntrswitch = 1;						//	Invert left and right in calling loop
				centre += outchans/2;					//	Invert centre, to opposite-to-centre channel
				if(centre > outchans)
					centre -= outchans;
			}
			for(ochan = 0;ochan < outchans;ochan++)
				levels[ochan] = 0.0;
			levels[0] = maxlevel;	//	Central channel to which we'll shrink gets current maxlevel, but we canstore it in levels[0]
			*thiscentre = centre;
			return(CONTINUE);		//	we're returning an "outside" flag, a moved centre, a shrunk "halfspread" and a filtering-mix value
		}
	}
	halfspread = (*spread)/2.0;

	if(depth <= 0.0)
		depth = FLTERR;
	
	if((halfspread = (*spread)/2.0) == 0.0)
		halfspread = FLTERR;

	for(ochan = 0;ochan < outchans;ochan++)
		levels[ochan] = 0.0;

	/* Establish maxlevel (determined by rolloff) */

	if(*spread < 1.0)
		range = 0.0;
	else
		range = 1.0 - (1.0/(*spread));
	maxlevel = 1.0 - (rolloff * range);

	// Set all channels fully within the spread to maxlevel
																
	spredej_left = centre - halfspread;							//	spredej_left and right could be determined 
	while(spredej_left  < 0)									//	by a "position-of-front" parameter
		spredej_left += (double)outchans;						//	working backwards towards centre, using halfspread
	spredej_right = centre + halfspread;						
	while(spredej_right >= outchans)							//	Once this is done .. easier to deal with 
		spredej_right -= (double)outchans;						//	distant (filtered) signalwith no (or limited) width
	spredej_left_leftchan  = (int)floor(spredej_left);			//	at trailing, or leading centre
	if((spredej_left_rightchan  = spredej_left_leftchan + 1) >= outchans)
		spredej_left_rightchan = 0;								//	Centre is behind front if front moving away from entre
	spredej_right_leftchan = (int)floor(spredej_right);			//	and <= hlafway to f/b midoint.
																//	Having crossed that, centre moves to OPPOSITE position.
	hole = *spread - (depth * 2.0);								//	Once available lspk-spread is < specified spread,
																//	there.s no hole in the middle.
	if(*spread >= outchans) {									//	Once available loudspeaker spread < 2
		for(k = 0;k<outchans; k++)								//	spread determined articially by how we deal with 
			levels[k] = maxlevel;								//	distance (beyond octagon) effect, and how we filter
		if(hole <= 0.0)											//	as we shrink the stereo width.
			return CONTINUE;
	} else if(*spread > 1.0) {

		k = spredej_left_leftchan;
		if(spredej_left_leftchan == spredej_left)
			levels[k] = maxlevel;
		if(++k >= outchans)
			k -= outchans;

		while(k != spredej_right_leftchan) {
			levels[k] = maxlevel;
			if(++k >= outchans)
				k -= outchans;
		}
		levels[spredej_right_leftchan] = maxlevel;
	} else {													//	In fact, this is what is happening here !!!!

		/* Deal with case where spread extends to both sides of a channel, even if less than 1.0 */
		
		zleft  = centre - halfspread;
		zright = centre + halfspread;
		if(zleft < 0.0) {		
			zleft  += outchans;
			zright += outchans;
		}
		if(zleft >= 0.0)
			floor_zleft = floor(zleft);
		else
			floor_zleft = -ceil(fabs(zleft));
		if(zright >= 0.0)
			floor_zright = floor(zright);
		else
			floor_zright = -ceil(fabs(zright));

		if(floor_zleft < floor_zright)
			levels[spredej_right_leftchan] = maxlevel;
	}
	/* Do fractional channels on leading edges of spread */

	stereopos_left  = spredej_left - (double)spredej_left_leftchan;

	relpos = fabs(0.5 - stereopos_left) * 2.0;		// position relative_to_stereo_centre : Range 0 - 1
	temp = 1.0 + (relpos * relpos);					// calculate stereo-hole-in-middle compensation 
	holecompensate = ROOT2 / sqrt(temp);

	left_leftchan_level = (1 - stereopos_left) * maxlevel * holecompensate;
	if(spredej_left_leftchan != spredej_right_leftchan)
		levels[spredej_left_leftchan] = left_leftchan_level;

	stereopos_right = spredej_right - (double)spredej_right_leftchan;

	relpos = fabs(0.5 - stereopos_right) * 2.0;		// position relative_to_stereo_centre : Range 0 - 1
	temp = 1.0 + (relpos * relpos);					// calculate stereo-hole-in-middle compensation 
	holecompensate = ROOT2 / sqrt(temp);

	right_rightchan_level = stereopos_right * maxlevel * holecompensate;
	if(spredej_right > spredej_right_leftchan) {
		if((spredej_right_rightchan = spredej_right_leftchan + 1) >= outchans)
			spredej_right_rightchan -= outchans;


		if(spredej_right_rightchan != spredej_left_rightchan) {
			if(spredej_right_rightchan == spredej_left_leftchan) {
				levels[spredej_right_rightchan] += right_rightchan_level;
			} else {
				levels[spredej_right_rightchan] = right_rightchan_level;
			}
		}
	}

	/* IF hole in the middle of the spread, because depth too small, find edges of hole */

	halfspread = (*spread)/2.0;

	if(hole > 0.0) {
		holej_right = centre + halfspread - depth;
		holej_left = centre - halfspread + depth;
		k = (int)ceil(holej_left);
		j = (int)floor(holej_right);		// Look at all chans in hole
		while(k <= j) {
			kk = k - holej_left;
			jj = holej_right - k;
			mingap = min(jj,kk);
			holing = min(mingap,1.0);
			holing = 1.0 -  holing;
			ochan = k;
			while(ochan < 0)
				ochan += outchans;
			while(ochan >= outchans)
				ochan -= outchans;
			levels[ochan] *= holing;
			k++;
		}
	}
	*thiscentre = centre;
	return CONTINUE;
}

/*
 *		INNER HOLE
 *		range = Starts at 1.0 , where no hole. Ends at appropriate stereo-position-level (this_level), once hole extends as far as an existing chan;
 *		mingap = minimum distance from holeedge to nearest channel
 *		mingap += hole;	we then add this to size of hole, to see size of area including hole and distance to nearest chan. Always <= 1.0;
 *		holeratio = hole/mingap; with a tiny hole, this is always c. zero, as hole enlarges, gets bigger, till, when hole touches a channel, it gets to be 1.0
 *		this_level = 1.0 - (range * holeratio);   When holeratio = 1, level becomes 1.0 - range = this_level i.e. expected stereo level
 *		= 1.0 - (1.0 - this_level) = this_level i.e. expected stereo level
 */

/*
 *	POSITIONS OF MOVING FRONT
 *	2 = "+infinitely" distant: 1 = in plane of front-centre lspkr : 0 = in plane of midline:	
 *  -1 = in plane of read lspkr:   -2 = "-infinitely" distant
 *
 *	Depth-param now expressed as fraction of lspkr-space. 1 = chans, 1/2 = chans/2 etc ..... (so original spread param = depth *chans)
 *
 *					2			Let "spread" at 1 be 1/chans (i.e. 1/2 lspkr width on either side of central lspkr)
 *								"spread" at centre (front = 0) is always ochans/2
 *					1			For "front" in range 1 - 0 therefore.
 *				x		x			spread = ((1 - front) * (chans/2 - 1)) + 1 and calculations can proceed as normal
 *								For front in range 0 to -1,by symmetry
 *			   x	0	 x			spread = chans - (((1 + front) * (chans/2 - 1)) + 1)
 *								
 *				x		x		As the front has a depth, and therefore a trailing edge which does not reach -1 at same time as true front,
 *					-1			calculations relating to fade-out and filtering between -1 and -2, have to be based on trailing edge vals.
 *								So trailing edge only reaches -2 when front is at -2+depth.
 *								User needs to build this in to the "fonr" breakpoint file data.
 *					-2			
 *								With the new value of depth, trailing edge is at front + (depth * 2)
 *								This is "+" as front always move +ve to -ve
 *								and uses factor "2" because, when depth 1 = all lspkrs in use, but distance front to back (1 to -1) = 2.
 *								So if 1/2 loudpseakers are activated, trailing edge is half-way back round space and: 
 *								and "HALFway" for front means distance of "1" (Thus depth[1/2] * 2 = 1) 
 */


//FILTER
// 	set stopfrq [expr $sd(filt) + (double($sd(filt))/4.0)]
//	set cmd [file join $evv(CDPROGRAM_DIR) filter]
//	set cmd [list $cmd lohi 1 $sd_infnam cdptest0$evv(SNDFILE_EXT) -96 $sd(filt) $stopfrq]

/****************************** FILTER_PROCESS *************************/

int filter_process(int *samps_to_process,int *endzeros,dataptr dz)
{
	int exit_status;
	float *fbuf = dz->sampbuf[5], *obuf = dz->sampbuf[3];
	int n;
	memset((char *)fbuf,0,dz->buflen * sizeof(float));
	if(*samps_to_process)
		memcpy((char *)fbuf,(char *)obuf,*samps_to_process * sizeof(float));
	dz->ssampsread = dz->buflen;	//	To force filter to continue to operate, when input exhausted
	if((exit_status  = do_lphp_filter_stereo(dz)) <0)
		return(exit_status);
	if( *samps_to_process == 0) {
		n = dz->buflen - 1;
		while(n >= 0) {					//	Force exit when 256 consecutive grp-samples are zero
			if(fbuf[n] == 0.0)
				(*endzeros)++;
			else
				break;
			n--;
		}
		n++;
		if(*endzeros >= 256 * STEREO) {
			*samps_to_process = n;
			return FINISHED;
		}
	}
	return(CONTINUE);
}

/***************************** FILTER *************************************/

int do_lphp_filter_stereo(dataptr dz)
{
	int exit_status, i;
	int index, fbase = dz->outlevs + 1;
	
	double *e1,*e2,*s1,*s2;
	
	double *den1 = dz->parray[fbase + FLT_DEN1];
	double *den2 = dz->parray[fbase + FLT_DEN2];
	double *cn	 = dz->parray[fbase + FLT_CN];	

	for(i=0; i < STEREO; i++) {
		index = i * FLT_LPHP_ARRAYS_PER_FILTER;
	
		e1	 = dz->parray[fbase + FLT_E1_BASE + index];
		e2	 = dz->parray[fbase + FLT_E2_BASE + index];
		s1	 = dz->parray[fbase + FLT_S1_BASE + index];
		s2	 = dz->parray[fbase + FLT_S2_BASE + index];
		if((exit_status = lphp_filt_stereo(e1,e2,s1,s2,den1,den2,cn,dz,i))<0)
			return exit_status;
	}
	return(FINISHED);
}

/***************************** LPHP_FILT_CHAN *************************************/

int lphp_filt_stereo(double *e1,double *e2,double *s1,double *s2,
					double *den1,double *den2,double *cn,dataptr dz,int chan)
{
	int i;
	int  k;
	float *buf = dz->sampbuf[5];
	double ip, op = 0.0, b1;
 	for (i = chan ; i < dz->ssampsread; i+= STEREO) {
		ip = (double) buf[i];
		for (k = 0 ; k < dz->fltcnt; k++) {
			b1    = dz->fltmul * cn[k];
			op    = (cn[k] * ip) + (den1[k] * s1[k]) + (den2[k] * s2[k]) + (b1 * e1[k]) + (cn[k] * e2[k]);
			s2[k] = s1[k];
			s1[k] = op;
			e2[k] = e1[k];
			e1[k] = ip;
		}
		if(op >= HUGE || op <= -HUGE) {
			sprintf(errstr,"Filter numeric overflow at %lf secs: try a different (higher) cutoff frequency.\n",
			(double)((dz->total_samps_written/dz->infile->channels) + (i/STEREO))/(double)dz->infile->srate);
			return GOAL_FAILED;
		}
		op *= dz->scalefact;
		if (fabs(op) > 1.0) {
			dz->scalefact *= .9999;
			if (op  > 0.0)
				op = 1.0;
			else 
				op = -1.0;
		}
		buf[i] = (float)op;
	}
	return FINISHED;
}

/********************************* SETUP_LPHP_FILTER *****************************/

int setup_lphp_filter(dataptr dz)
{
	int exit_status;
	int filter_order;
	double signd = 1.0, passfrq, stopfrq;
	dz->scalefact = 1.0;	//	Start with no prescaling: turn down if filter blows up
	passfrq = dz->param[FRAC_FFREQ];
	stopfrq = dz->param[FRAC_FFREQ] * 1.25;
	signd = -1.0;
	dz->fltmul = 2.0;
	filter_order = establish_order_of_filter(passfrq,stopfrq,dz);

	if((exit_status = allocate_internal_params_lphp(dz))<0)
		return(exit_status);
	calculate_filter_poles_lphp(signd,filter_order,passfrq,dz);
	initialise_filter_coeffs_lphp(dz);
	return(FINISHED);
}

/********************************* ESTABLISH_ORDER_OF_FILTER *****************************/

int establish_order_of_filter(double passfrq,double stopfrq,dataptr dz)
{
	int filter_order;
	double tc, tp, tt, pii, xx, yy, fltgain = FILTER_GAIN;
	double sr = (double)dz->infile->srate;
	pii = 4.0 * atan(1.0);
	passfrq = pii * passfrq/sr;
	tp = tan(passfrq);
	stopfrq = pii * stopfrq/sr;
	tc = tan(stopfrq);
	tt = tc / tp ;
	tt = (tt * tt);
	fltgain = fabs(fltgain);
	fltgain = fltgain * log(10.0)/10.0 ;
	fltgain = exp(fltgain) - 1.0 ;
	xx = log(fltgain)/log(tt) ;
	yy = floor(xx);
	if ((xx - yy) == 0.0 )
		yy = yy - 1.0 ;
	filter_order = ((int)yy) + 1;
	if (filter_order <= 1) 
		filter_order = 2;
	dz->fltcnt = filter_order/2 ;
	filter_order = 2 * dz->fltcnt;
	dz->fltcnt = min(dz->fltcnt,200);
	filter_order = 2 * dz->fltcnt;
	return(filter_order);
}

/********************************* ALLOCATE_INTERNAL_PARAMS_LPHP *****************************/

int allocate_internal_params_lphp(dataptr dz)
{
	int fbase = dz->outlevs + 1;
	int i;
	if((dz->parray[fbase + FLT_DEN1]	  = (double *)calloc(dz->fltcnt * sizeof(double),sizeof(char)))==NULL
	|| (dz->parray[fbase + FLT_DEN2]	  = (double *)calloc(dz->fltcnt * sizeof(double),sizeof(char)))==NULL
	|| (dz->parray[fbase + FLT_CN]		  = (double *)calloc(dz->fltcnt * sizeof(double),sizeof(char)))==NULL
	|| (dz->parray[fbase + FLT_S1_BASE]   = (double *)calloc(dz->fltcnt * sizeof(double),sizeof(char)))==NULL
	|| (dz->parray[fbase + FLT_E1_BASE]   = (double *)calloc(dz->fltcnt * sizeof(double),sizeof(char)))==NULL
	|| (dz->parray[fbase + FLT_S2_BASE]   = (double *)calloc(dz->fltcnt * sizeof(double),sizeof(char)))==NULL
	|| (dz->parray[fbase + FLT_E2_BASE]   = (double *)calloc(dz->fltcnt * sizeof(double),sizeof(char)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for arrays of filter parameters.\n");
		return(MEMORY_ERROR);
	}
	for(i = 1; i < STEREO;i++) {
		int index = i*FLT_LPHP_ARRAYS_PER_FILTER;
		if((dz->parray[fbase + FLT_S1_BASE + index]   = (double *)calloc(dz->fltcnt * sizeof(double),sizeof(char)))==NULL
		|| (dz->parray[fbase + FLT_E1_BASE + index]   = (double *)calloc(dz->fltcnt * sizeof(double),sizeof(char)))==NULL
		|| (dz->parray[fbase + FLT_S2_BASE + index]   = (double *)calloc(dz->fltcnt * sizeof(double),sizeof(char)))==NULL
		|| (dz->parray[fbase + FLT_E2_BASE + index]   = (double *)calloc(dz->fltcnt * sizeof(double),sizeof(char)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for arrays of filter parameters.\n");
			return(MEMORY_ERROR);
		}
	}
	return(FINISHED);
}

/********************************* CALCULATE_FILTER_POLES_LPHP *****************************/

void calculate_filter_poles_lphp(double  signd,int filter_order,double passfrq,dataptr dz)
{
	double ss, xx, aa, tppwr, x1, x2, cc;
	double pii = 4.0 * atan(1.0);
	double tp = tan(passfrq);
	int    k;
	int fbase = dz->outlevs + 1;
	ss = pii / (double)(2 * filter_order);
	for (k = 0; k < dz->fltcnt; k++ ) {
		xx = (double) ((2.0 * (k+1)) - 1.0);
		aa = -sin(xx * ss);
		tppwr = pow(tp,2.0);
		cc = 1.0 - (2.0 * aa * tp) + tppwr;
		x1 = 2.0 * (tppwr - 1.0)/cc ;
		x2 = (1.0 + (2.0 * aa * tp) + tppwr)/cc ;
		dz->parray[fbase + FLT_DEN1][k] = signd * x1;
		dz->parray[fbase + FLT_DEN2][k] = -x2 ;
		dz->parray[fbase + FLT_CN][k]   = pow(tp,2.0)/cc ;
	}
}

/********************************* INITIALISE_FILTER_COEFFS_LPHP *****************************/

void initialise_filter_coeffs_lphp(dataptr dz)
{
	int k;
	int i,index;
	int fbase = dz->outlevs + 1;
	for (k = 0 ; k < dz->iparam[FLT_CNT]; k++) {
		dz->parray[fbase + FLT_S1_BASE][k] = 0.0;
		dz->parray[fbase + FLT_S2_BASE][k] = 0.0;
		dz->parray[fbase + FLT_E1_BASE][k] = 0.0;
		dz->parray[fbase + FLT_E2_BASE][k] = 0.0;
	}
	for(i=1;i < STEREO; i++){
		index = i *	FLT_LPHP_ARRAYS_PER_FILTER;
		for (k = 0 ; k < dz->iparam[FLT_CNT]; k++) {
			dz->parray[fbase + FLT_S1_BASE + i][k] = 0.0;
			dz->parray[fbase + FLT_S2_BASE + i][k] = 0.0;
			dz->parray[fbase + FLT_E1_BASE + i][k] = 0.0;
			dz->parray[fbase + FLT_E2_BASE + i][k] = 0.0;
		}
	}
}

////
//	HEREH When do we start the filtering ??
//	How do we use the finishing strategy for the filter tail ??
//	How do we test whether filter works (output filter !!!!)

/**************************** ESTABLISH_ECHOS ***************************

void establish_echos(int *maxdelay,dataptr dz)
{
	int n;
	double *delay = dz->parray[dz->echobas];		//	size 20
	double *gainl = dz->parray[dz->echobas + 1];	//	size 20
	double *gainr = dz->parray[dz->echobas + 2];	//	size 20

	double tdelay[] = {0.131832, 0.215038, 0.322274, 0.414122, 0.504488, 0.637713, 0.730468, 0.808751, 0.910460, 1.027041, 1.132028, 1.244272, 1.336923, 1.427700, 1.528503, 1.618661, 1.715413, 1.814730, 1.914843, 2.000258};
	double gain[]	= {0.500000,0.354813,0.354813,0.251189,0.125893,0.125893,0.063096,0.063096,0.031623,0.031623,0.012589,0.012589,0.005012,0.005012,0.002512,0.002512,0.002512,0.002512,0.002512,0.002512};
	double pan[]    = {.9,.5,-.5,0.1,.7,-.7,.3,-.3,.15,-.15,.85,-.85,.4,-.4,.6,-.6,.225,-.225,.775,-.775};

	for(n=0;n < dz->iparam[FRAC_ECHOCNT]; n++) 
		delay[n] = (int)round(tdelay[n] * dz->infile->srate * dz->param[STAD_SIZE]) * STEREO;
	*maxdelay = delay[dz->iparam[FRAC_ECHOCNT - 1]];
	if(dz->param[FRAC_EROLL]<1.0) {
		for(n=0;n<dz->iparam[FRAC_ECHOCNT];n++)
			gain[n] = pow(gain[n],dz->param[FRAC_EROLL]);
	}
	for(n=0;n<dz->iparam[FRAC_ECHOCNT];n++) {
		if(pan[n] < 0.0) {	// LEFTWARDS 
		    gainl[n] = gain[n] * dz->param[FRAC_EPREGN];
		    gainr[n] = gain[n] * dz->param[FRAC_EPREGN] * (1.0+pan[n]);
		} else {		   // RIGHTWARDS
		    gainl[n] = gain[n] * dz->param[FRAC_EPREGN] * (1.0-pan[n]);
		    gainr[n] = gain[n] * dz->param[FRAC_EPREGN];
		}
	}
}
*/
