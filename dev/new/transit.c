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

#define SIGNAL_TO_LEFT  (0)
#define SIGNAL_TO_RIGHT (1)
#define ROOT2		(1.4142136)

#define DEG90	(PI/2.0)
#define DEG45	(PI/4.0)
#define DEG22	(PI/8.0)			//	Actually 22.5 degrees
#define DEG67	((3.0 * PI)/8.0)	//	Actually 67.5 degrees
#define DEG135	((3.0 * PI)/4.0)

#define TR_MIN_GAIN (0.000016)

#ifdef unix
#define round(x) lround((x))
#endif

char errstr[2400];

int anal_infiles = 1;
int	sloom = 0;
int sloombatch = 0;

const char* cdp_version = "7.1.0";

//CDP LIB REPLACEMENTS
static int setup_transit_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_transit_param_ranges_and_defaults(dataptr dz);
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

static int transit(dataptr dz);
static void pancalc(double position,double *leftgain,double *rightgain);
static int transit_param_preprocess(dataptr dz);
static int store_the_filename(char *filename,dataptr dz);
static int store_the_further_filename(int n,char *filename,dataptr dz);
static int get_position(double *thispos,int *spkr1,int *spkr2,double linear_steplen,int n,double *distance_to_centre,
			double first_quadrant,double second_quadrant,int *on_2nd_pair,int *on_3rd_pair,dataptr dz);
static int insert_sounds(int *snd,dataptr dz);
static int calc_filters(double *balance,int *spkr,dataptr dz);
static void leftwards(double *pos, double *time, double *level, int *spkr, dataptr dz);
static int ReorientData(double *pos, double *time, double *level, int *spkr, dataptr dz);
static int check_transit_param_validity_and_consistency(dataptr dz);
static int read_input_sndfiles_list(char *filename,dataptr dz);
static int allocate_the_filespace(dataptr dz);

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
	int exit_status;
	dataptr dz = NULL;
	char **cmdline;
	int  cmdlinecnt, n;
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
		dz->maxmode = 5;
		if((exit_status = get_the_mode_from_cmdline(cmdline[0],dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(exit_status);
		}
		cmdline++;
		cmdlinecnt--;
		// setup_particular_application =
		if((exit_status = setup_transit_application(dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		if((exit_status = count_and_allocate_for_infiles(cmdlinecnt,cmdline,dz))<0) {		// CDP LIB
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		if(dz->process == TRANSITF) {
			if(cmdlinecnt < 9) {
				fprintf(stderr,"Insufficient parameters for this process.\n");
				return(FAILED);
			}
		} else if(dz->process == TRANSITFD) {
			if(cmdlinecnt < 11) {
				fprintf(stderr,"Insufficient parameters for this process.\n");
				return(FAILED);
			}
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
	if((exit_status = setup_transit_param_ranges_and_defaults(dz))<0) {
		exit_status = print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	dz->all_words = 0;
	dz->ifd[0] = -1;
	switch(dz->process) {
	case(TRANSITL):
		if((exit_status = read_input_sndfiles_list(cmdline[0],dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);	
			return(FAILED);
		}
		cmdlinecnt--;
		cmdline++;
		break;
	case(TRANSIT):
		// open_first_infile		CDP LIB
		if((exit_status = open_first_infile(cmdline[0],dz))<0) {	
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);	
			return(FAILED);
		}
		if((exit_status = store_the_filename(cmdline[0],dz))<0)
			return(exit_status);
		cmdlinecnt--;
		cmdline++;
		break;
	default:
		if((exit_status = open_first_infile(cmdline[0],dz))<0) {	
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);	
			return(FAILED);
		}
		if((exit_status = store_the_filename(cmdline[0],dz))<0)
			return(exit_status);
		cmdlinecnt--;
		cmdline++;
		for(n=1;n<dz->infilecnt;n++) {
			if((exit_status = handle_other_infile(n,cmdline[0],dz))<0) {
				sprintf(errstr,"Possibly too many parameters on commandline, or bad flag.\n");
				print_messages_and_close_sndfiles(exit_status,is_launched,dz);	
				return(FAILED);
			}
			sndcloseEx(dz->ifd[n]);
			dz->ifd[n] = -1;
			if((exit_status = store_the_further_filename(dz->all_words,cmdline[0],dz))<0)
				return(exit_status);
			cmdlinecnt--;
			cmdline++;
		}
		break;
	}
	if(dz->ifd[0] >= 0) {
		sndcloseEx(dz->ifd[0]);
		dz->ifd[0] = -1;
	}
	exit_status = FINISHED;
	switch(dz->process) {
	case(TRANSITF):	 if(dz->infilecnt != 2)	exit_status = FAILED;	break;
	case(TRANSITD):	 if(dz->infilecnt < 2)	exit_status = FAILED;	break;
	case(TRANSITFD): if(dz->infilecnt < 4)	exit_status = FAILED;	break;
	case(TRANSITS):	 
	case(TRANSITL):	 
		if(dz->infilecnt < 3 || EVEN(dz->infilecnt)) {
			fprintf(stdout,"ERROR: On odd number ( >=3 ) of soundfiles required for this process.\n");
			fflush(stdout);
			return(FAILED);
		}
	}
	if(exit_status == FAILED) {
		fprintf(stdout,"ERROR: Wrong number of input soundfiles for this process.\n");
		fflush(stdout);
		return(FAILED);
	}	
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
//	check_param_validity_and_consistency()		redundant
	if((exit_status = check_transit_param_validity_and_consistency(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	is_launched = TRUE;
//	create_sndbufs())		redundant
	if((exit_status = transit_param_preprocess(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = open_the_outfile(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	//spec_process_file =
	if((exit_status = transit(dz))<0) {
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
	strcpy(dz->outfilename,filename);
	p = dz->outfilename;
	while(*p != ENDOFSTR) {
		if(*p == '.') {
			*p = ENDOFSTR;
			break;
		}
		p++;
	}
	strcat(dz->outfilename,".mmx");
	(*cmdline)++;
	(*cmdlinecnt)--;
	return(FINISHED);
}

/************************ OPEN_THE_OUTFILE *********************/

int open_the_outfile(dataptr dz)
{
	int exit_status;
	dz->infile->channels = 8;
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

/************************* SETUP_TRANSIT_APPLICATION *******************/

int setup_transit_application(dataptr dz)
{
	int exit_status;
	aplptr ap;
	if((exit_status = establish_application(dz))<0)		// GLOBAL
		return(FAILED);
	ap = dz->application;
	// SEE parstruct FOR EXPLANATION of next 2 functions
	switch(dz->process) {
	case(TRANSIT):
	case(TRANSITD):
		if((exit_status = set_param_data(ap,0   ,6,5,"ddidd0"))<0)
			return(exit_status);
		if((exit_status = set_vflgs(ap,"tdem",4,"dddd","l",1,0,"0"))<0)
			return(exit_status);
		break;
	case(TRANSITF):
	case(TRANSITFD):
		if((exit_status = set_param_data(ap,0   ,6,6,"ddiddd"))<0)
			return(exit_status);
		if((exit_status = set_vflgs(ap,"tdem",4,"dddd","l",1,0,"0"))<0)
			return(exit_status);
		break;
	case(TRANSITS):
	case(TRANSITL):
		if((exit_status = set_param_data(ap,0   ,6,4,"dd0dd0"))<0)
			return(exit_status);
		if((exit_status = set_vflgs(ap,"",0,"","l",1,0,"0"))<0)
			return(exit_status);
		break;
	}
	// set_legal_infile_structure -->
	dz->has_otherfile = FALSE;
	// assign_process_logic -->
	switch(dz->process) {
	case(TRANSITL):
		dz->input_data_type = SNDLIST_ONLY;
		break;
	case(TRANSIT):
		dz->input_data_type = SNDFILES_ONLY;
		break;
	case(TRANSITF):
		dz->input_data_type = TWO_SNDFILES;
		break;
	default:
		dz->input_data_type = MANY_SNDFILES;
		break;
	}
	dz->process_type	= TO_TEXTFILE;	
	dz->outfiletype  	= TEXTFILE_OUT;
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
		if(dz->process == TRANSITL) {
			if(!(infile_info->filetype & SNDLIST))  {
				sprintf(errstr,"File %s is not of correct type\n",cmdline[0]);
				return(DATA_ERROR);
			}
		} else {
			if(infile_info->filetype != SNDFILE)  {
				sprintf(errstr,"File %s is not of correct type\n",cmdline[0]);
				return(DATA_ERROR);
			} else if(infile_info->channels != 1)  {
				sprintf(errstr,"File %s is not of correct type (must be mono)\n",cmdline[0]);
				return(DATA_ERROR);
			} else if((exit_status = copy_parse_info_to_main_structure(infile_info,dz))<0) {
				sprintf(errstr,"Failed to copy file parsing information\n");
				return(PROGRAM_ERROR);
			}
		}
		free(infile_info);
	}
	return(FINISHED);
}

/************************* SETUP_TRANSIT_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_transit_param_ranges_and_defaults(dataptr dz)
{
	int exit_status;
	aplptr ap = dz->application;
	// set_param_ranges()
	ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
	// NB total_input_param_cnt is > 0 !!!
	if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
		return(FAILED);
	// get_param_ranges()
	if(EVEN(dz->mode)) {
		ap->lo[TRAN_FOCUS]	= 1;
		ap->hi[TRAN_FOCUS]	= 8;
		ap->default_val[TRAN_FOCUS] = 1;
	} else {
		ap->lo[TRAN_FOCUS]	= 1.5;
		ap->hi[TRAN_FOCUS]	= 8.5;
		ap->default_val[TRAN_FOCUS] = 1.5;
	}
	ap->lo[TRAN_DUR]	= dz->duration * 2.0;
	ap->hi[TRAN_DUR]	= 32767;
	ap->default_val[TRAN_DUR]	= 8;
	if(dz->process < TRANSITS) {
		ap->lo[TRAN_STEPS]	= 2;
		ap->hi[TRAN_STEPS]	= 32767;
		ap->default_val[TRAN_STEPS] = 24;
	}
	switch(dz->mode) {
	case(GLANCING):
		ap->lo[TRAN_MAXA]	= 22.5;
		ap->hi[TRAN_MAXA]	= 90;
		ap->default_val[TRAN_MAXA] = 85;
		break;
	case(EDGEWISE):
		ap->lo[TRAN_MAXA]	= 22.5;
		ap->hi[TRAN_MAXA]	= 90;
		ap->default_val[TRAN_MAXA] = 85;
		break;
	case(CROSSING):
		ap->lo[TRAN_MAXA]	= 45;
		ap->hi[TRAN_MAXA]	= 90;
		ap->default_val[TRAN_MAXA] = 85;
		break;
	case(CLOSE):
		ap->lo[TRAN_MAXA]	= 67.5;
		ap->hi[TRAN_MAXA]	= 90;
		ap->default_val[TRAN_MAXA] = 85;
		break;
	case(CENTRAL):
		ap->lo[TRAN_MAXA]	= 1;
		ap->hi[TRAN_MAXA]	= 1000;
		ap->default_val[TRAN_MAXA] = 10;
		break;
	}
	ap->lo[TRAN_DEC]	= 0;
	ap->hi[TRAN_DEC]	= 1;
	ap->default_val[TRAN_DEC] = .9;
	if(dz->process == TRANSITF || dz->process == TRANSITFD) {
		ap->lo[TRAN_FBAL]	= 0;
		ap->hi[TRAN_FBAL]	= 1;
		ap->default_val[TRAN_FBAL] = .9;
	}
	if(dz->process < TRANSITS) {
		ap->lo[TRAN_THRESH]	= 0;
		ap->hi[TRAN_THRESH]	= 1;
		ap->default_val[TRAN_THRESH] = 0.0;
		ap->lo[TRAN_DECLIM]	= 0;
		ap->hi[TRAN_DECLIM]	= 1;
		ap->default_val[TRAN_DECLIM] = 0.0;
		ap->lo[TRAN_MINLEV]	= 0;
		ap->hi[TRAN_MINLEV]	= 1;
		ap->default_val[TRAN_MINLEV] = 0.0;
		ap->lo[TRAN_MAXDUR]	= dz->duration * 2.0;
		ap->hi[TRAN_MAXDUR]	= 32767;
		ap->default_val[TRAN_MAXDUR] = 0.0;
	}
	dz->maxmode = 5;
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
			if((exit_status = setup_transit_application(dz))<0)
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
	fprintf(stderr,
	"\nCREATE MOTION CROSSING AN 8-CHANNEL RING\n\n"
	"USAGE: transit NAME mode infile(s) outfile parameters: \n"
	"\n"
	"where NAME can be any one of\n"
	"\n"
	"simple    filtered    doppler    doplfilt    sequence    list\n\n"
	"Type 'transit simple' for more info on transit simple..ETC.\n");
	return(USAGE_ONLY);
}


/************************************* CHECK_TRANSIT_PARAM_VALIDITY_AND_CONSISTENCY *********************************/

int check_transit_param_validity_and_consistency(dataptr dz)
{
	char temp[200], *p;
	int got0 = 0, got9 = 0, got5 = 0, OK = 0;
	if(EVEN(dz->mode)) {
		sprintf(temp,"%lf",dz->param[TRAN_FOCUS]);
		p = temp + strlen(temp);
		p--;
		got5 = 0;
		while(p != temp) {
			if(*p == '0') {
				if(got9)
					break;
				got0 = 1;
				p--;
			} else if(*p == '9') {
				if(got0)
					break;
				got9 = 1;
				p--;
			} else if(*p == '.') {
				if(!(got0 || got9))
					break;
				OK = 1;
				break;
			} else
				break;
		}
		if(!OK) {
			sprintf(errstr,"FOCUS must have integer values in this mode.\n");
			return(DATA_ERROR);
		}
		dz->iparam[TRAN_FOCUS] = (int)round(dz->param[TRAN_FOCUS]);
		if(dz->iparam[TRAN_FOCUS] < 1 || dz->iparam[TRAN_FOCUS] > 8) {
			sprintf(errstr,"Invalid Motion Centre.\n");
			return(DATA_ERROR);
		}
		dz->param[TRAN_FOCUS] = (double)dz->iparam[TRAN_FOCUS];
	} else {
		sprintf(temp,"%lf",dz->param[TRAN_FOCUS]);
		p = temp + strlen(temp);
		p--;
		got5 = 0;
		while(p != temp) {
			if(*p == '0') {
				if(got5 || got9)
					break;
				got0 = 1;
				p--;
			} else if(*p == '9') {
				if(got5 || got0)
					break;
				got9 = 1;
				p--;
			} else if(*p == '5') {
				if(got9)
					break;
				got5 = 1;
				p--;
			} else if(*p == '4') {
				if(got0)
					break;
				got5 = 1;
				p--;
			} else if(*p == '.') {
				if(!got5)
					break;
				OK = 1;
				break;
			} else
				break;
		}
		if(!OK) {
			sprintf(errstr,"FOCUS must have half integer values (1.5 : 4.5 : etc) in this mode.\n");
			return(DATA_ERROR);
		}
		dz->iparam[TRAN_FOCUS] = (int)round(dz->param[TRAN_FOCUS] * 2);
		dz->param[TRAN_FOCUS]  = (double)dz->iparam[TRAN_FOCUS]/2.0;
		if(dz->param[TRAN_FOCUS] <= 0.0)
			dz->param[TRAN_FOCUS] += 8.0;
		else if(dz->param[TRAN_FOCUS] > 8.0)
			dz->param[TRAN_FOCUS] -= 8.0;
	}
	if(dz->process < TRANSITS) {
		if(dz->param[TRAN_THRESH] > 0.0) {
			if(dz->param[TRAN_DECLIM] <= 0.0 || dz->param[TRAN_MINLEV] <= 0.0 || dz->param[TRAN_MAXDUR] <= 0.0) {
				sprintf(errstr,"Threshold set: decimation-limit,minlevel & maxdur must have non-zero vals.\n");
				return(DATA_ERROR);
			}
			if(dz->param[TRAN_DECLIM] <= dz->param[TRAN_DEC]) {
				sprintf(errstr,"Decimation Maximum must be greater than Decimation.\n");
				return(DATA_ERROR);
			}
			if(dz->param[TRAN_MINLEV] >= dz->param[TRAN_THRESH]) {
				sprintf(errstr,"Minimum Extension Gain must be less than Extension Threshold.\n");
				return(DATA_ERROR);
			}
			if(dz->param[TRAN_MAXDUR] <= dz->param[TRAN_DUR]) {
				sprintf(errstr,"Maximum Duration either not set, or less than Duration (%lf).\n",dz->param[TRAN_DUR]);
				return(DATA_ERROR);
			}
		} else if(dz->param[TRAN_DECLIM] > 0.0 || dz->param[TRAN_MINLEV] > 0.0 || dz->param[TRAN_MAXDUR] > 0.0) {
			fprintf(stdout,"WARNING: Threshold NOT set: decimation-limit, minlevel and maxdur values ignored.\n");
			fflush(stdout);
		}
	}
	if(flteq(dz->param[TRAN_DEC],0.0)) {
		fprintf(stdout,"WARNING: Decimation zero will delte all events except the central event(s).\n");
		fflush(stdout);
	}
	return FINISHED;
}

/************************************* TRANSIT_PARAM_PREPROCESS *********************************/

int transit_param_preprocess(dataptr dz)
{
	int exit_status;
	int arraysize, n, m, stepcnt;	
	double linear_steplen = 0, timestep, dur, dec, lev, newdec, now, distance_to_centre, first_quadrant = 0, second_quadrant = 0, thispos;
	double *level, *pos, *time, *balance;
	int *spkr, *snd;
	int in_extension, at_decr_incr_end, cnt, on_2nd_pair, on_3rd_pair, spkr1 = 0, spkr2 = 0;
	if(dz->mode != CENTRAL)
		dz->param[TRAN_MAXA] *= PI/180.0;
	if(dz->process >= TRANSITS)
		stepcnt = (dz->infilecnt/2) + 1;
	else
		stepcnt = dz->iparam[TRAN_STEPS];
	switch(dz->mode) {
	case(GLANCING):													//	Assuming halfradius (distance lspkr to centre) = 1
		linear_steplen = tan(dz->param[TRAN_MAXA]);					//  tan of maxangle = Total-pathlength/halfradius
		linear_steplen /= (double)stepcnt;							//	Divide total len by number of steps to get steplen
		break;														//	(as fraction of halfradius = 1).
	case(EDGEWISE):													//	If distance from path to centre is less than half-radius
		linear_steplen = tan(dz->param[TRAN_MAXA]) * cos(DEG22);	//	tan of maxangle = Total-pathlength/distance_to_centre
		linear_steplen /= (double)stepcnt;							//	(here distance_to_centre = cos22)
		break;														//	Divide total len by number of steps to get real len
	case(CROSSING):													//	ETC
		linear_steplen = tan(dz->param[TRAN_MAXA]) * cos(DEG45);
		linear_steplen /= (double)stepcnt;
		break;
	case(CLOSE):
		linear_steplen = tan(dz->param[TRAN_MAXA]) * cos(DEG67);
		linear_steplen /= (double)stepcnt;
		break;
	case(CENTRAL):													//	Here steplen is simply total-length divided by event-count
		linear_steplen = dz->param[TRAN_MAXA]/(double)stepcnt;
		break;
	}

	timestep = dz->param[TRAN_DUR]/(double)stepcnt;
	if((dz->process < TRANSITS) && dz->param[TRAN_THRESH] > 0.0) {	//	If path extension being used
		in_extension = 0;											//	Check how many more steps this involves
		at_decr_incr_end = 0;										//	Before mallocing storage arrays
		dur = timestep;
		cnt = 1;
		lev = 1.0;
		dec = dz->param[TRAN_DEC];
		while(dur <dz->param[TRAN_MAXDUR]) {						//	Quit if maximum duration reached
			lev = lev * dec;
			if(in_extension && (lev < dz->param[TRAN_MINLEV]))		//	Quit if minimum level reached
				break;
			if(in_extension || (lev < dz->param[TRAN_THRESH])) {	//	Once level falls below threshold
				if(!at_decr_incr_end) {								//	Start incrementing decrement
					newdec = dec + (1.0 - dec)/2.0;					
					if(newdec >= dz->param[TRAN_DECLIM])			//	If decrement reaches the prescribed maximum
						at_decr_incr_end = 1;						//	Stop incrementing decrement
					else
						dec = newdec;
				}
				in_extension = 1;
			}
			dur += timestep;
			cnt++;
		}
	} else
		cnt = stepcnt;
	arraysize = cnt * 2;											//	Events approach then recede, doubling number of events
	arraysize += 4;	//	SAFETY
	dz->ringsize = arraysize;
	dz->array_cnt = 4;
	if((dz->parray = (double **)malloc(dz->array_cnt * sizeof(double *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create data float arrays.\n");
		return(MEMORY_ERROR);
	}
	dz->iarray_cnt = 2;
	if((dz->iparray = (int **)malloc(dz->iarray_cnt * sizeof(int *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create data integer arrays.\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[TR_LEVEL] = (double *)malloc(arraysize * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store segment levels.\n");
		return(MEMORY_ERROR);
	}
	level = dz->parray[TR_LEVEL];
	if((dz->parray[TR_POSITION] = (double *)malloc(arraysize * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store segment positions.\n");
		return(MEMORY_ERROR);
	}
	pos = dz->parray[TR_POSITION];
	if((dz->parray[TR_TIME] = (double *)malloc(arraysize * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store segment positions.\n");
		return(MEMORY_ERROR);
	}
	time = dz->parray[TR_TIME];
	if((dz->parray[TR_FLTMIX] = (double *)malloc(arraysize * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store segment balance.\n");
		return(MEMORY_ERROR);
	}
	balance = dz->parray[TR_FLTMIX];
	if((dz->iparray[TR_SPKRPAIR] = (int *)malloc(arraysize * 2 * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store lspkr pairs.\n");
		return(MEMORY_ERROR);
	}
	spkr = dz->iparray[TR_SPKRPAIR];
	if((dz->iparray[TR_SNDFILE] = (int *)malloc(arraysize * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store soundfiles used.\n");
		return(MEMORY_ERROR);
	}
	snd = dz->iparray[TR_SNDFILE];
	dec = dz->param[TRAN_DEC];
	in_extension = 0;
	at_decr_incr_end = 0;
	now = 0.0;								//	d = linear_steplen.
	lev = 1.0;								//	Distance to centre = x
	on_2nd_pair = 0;						//  tan of angle at centre = (d * n)/x
	on_3rd_pair = 0;						//
	time[0] = 0.0;							//		O-d-|-d-|-d-	    -d-|-d-|-d-
	level[0] = 1.0;							//   O	|  O			 O | O
	switch(dz->mode) {						//      |x				   |x
	case(GLANCING):							//  O	c	O	~OR~   O   |   O	~OR~ ETC   
		distance_to_centre = 1.0;			//						   c
		first_quadrant  = DEG45;			//	 O	   O		   O	   O
		second_quadrant = DEG45;			//		O
		spkr1 = dz->iparam[TRAN_FOCUS];		//						 O   O	
		spkr2 = spkr1 + 1;
		if(spkr2 > 8)
			spkr2 -= 8;
		pos[0] = 0.0;
		break;
	case(EDGEWISE):
		distance_to_centre = cos(DEG22);
		first_quadrant  = DEG22;
		second_quadrant = DEG67;
		spkr1 = (int)floor(dz->param[TRAN_FOCUS]);
		spkr2 = spkr1 + 1;
		if(spkr2 > 8)
			spkr2 -= 8;
		pos[0] = 0.5;
		break;
	case(CROSSING):
		distance_to_centre = cos(DEG45);
		first_quadrant  = DEG45;
		second_quadrant = DEG45;	//	REDUNDANT
		spkr1 = dz->iparam[TRAN_FOCUS] - 1;
		if(spkr1 <= 0)
			spkr1 += 8;
		spkr2 = spkr1 + 2;
		if(spkr2 > 8)
			spkr2 -= 8;
		pos[0] = 0.5;
		break;
	case(CLOSE):
		distance_to_centre = cos(DEG67);
		first_quadrant  = DEG67;
		second_quadrant = DEG45;	//	REDUNDANT
		spkr1 = (int)floor(dz->param[TRAN_FOCUS]) - 1;
		if(spkr1 <= 0)
			spkr1 += 8;
		spkr2 = spkr1 + 3;
		if(spkr2 > 8)
			spkr2 -= 8;
		pos[0] = 0.5;
		break;
	case(CENTRAL):
		distance_to_centre = 0;
		first_quadrant     = 0;		//	REDUNDANT
		second_quadrant    = 0;		//	REDUNDANT
		spkr1 = dz->iparam[TRAN_FOCUS] - 2;
		if(spkr1 <= 0)
			spkr1 += 8;
		spkr2 = spkr1 + 4;
		if(spkr2 > 8)
			spkr2 -= 8;
		pos[0] = 0.5;
		break;
	}
	spkr[0] = spkr1;
	spkr[1] = spkr2;
	
	for(n=1,m=2;n<stepcnt;n++,m+=2) {
		if((exit_status = get_position(&thispos,&spkr1,&spkr2,linear_steplen,n,&distance_to_centre,first_quadrant,second_quadrant,&on_2nd_pair,&on_3rd_pair,dz))<0)
			return(exit_status);
		pos[n] = thispos;
		spkr[m]   = spkr1;
		spkr[m+1] = spkr2;
		now += timestep;
		lev *= dec;
		level[n] = lev;
		time[n] = now;
		if((dz->process < TRANSITS) && dz->param[TRAN_THRESH] > 0.0) {	//	Data extended
			if(in_extension || (lev < dz->param[TRAN_THRESH])) {
				if(!at_decr_incr_end) {
					newdec = dec + (1.0 - dec)/2.0;
					if(newdec >= dz->param[TRAN_DECLIM])
						at_decr_incr_end = 1;
					else
						dec = newdec;
				}
				in_extension = 1;
			}
			if(in_extension && (lev < dz->param[TRAN_MINLEV]))
				break;
		}
	}
	if((dz->process < TRANSITS) && dz->param[TRAN_THRESH] > 0.0) {	//	Data extended
		while(now <dz->param[TRAN_MAXDUR]) {
			lev *= dec;
			if(in_extension && (lev < dz->param[TRAN_MINLEV]))
				break;
			if((exit_status = get_position(&thispos,&spkr1,&spkr2,linear_steplen,n,&distance_to_centre,first_quadrant,second_quadrant,&on_2nd_pair,&on_3rd_pair,dz))<0)
				return(exit_status);
			pos[n] = thispos;
			spkr[m]   = spkr1;
			spkr[m+1] = spkr2;
			now += timestep;
			level[n] = lev;
			time[n] = now;
			n++;
			m += 2;
		}
	}
	dz->itemcnt = n;
	if((exit_status = ReorientData(pos,time,level,spkr,dz)) < 0)	//	Mirror receding motion into approaching motion
		return exit_status;
	if(dz->process == TRANSITF || dz->process == TRANSITFD) {		//	Insert filter balance data
		if((exit_status = calc_filters(balance,spkr,dz)) < 0)
			return exit_status;
	}
	if((exit_status = insert_sounds(snd,dz)) < 0)					//	Assign files for doppler-shift
		return exit_status;
	if(dz->vflag[0])												//	 Move towards left, if flagged
		leftwards(pos,time,level,spkr,dz);
	return (FINISHED);
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

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	if(!strcmp(prog_identifier_from_cmdline,"simple"))			dz->process = TRANSIT;
	else if(!strcmp(prog_identifier_from_cmdline,"filtered"))	dz->process = TRANSITF;
	else if(!strcmp(prog_identifier_from_cmdline,"doppler"))	dz->process = TRANSITD;
	else if(!strcmp(prog_identifier_from_cmdline,"doplfilt"))	dz->process = TRANSITFD;
	else if(!strcmp(prog_identifier_from_cmdline,"sequence"))	dz->process = TRANSITS;
	else if(!strcmp(prog_identifier_from_cmdline,"list"))		dz->process = TRANSITL;
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
	if(!strcmp(str,"simple")) {
		fprintf(stdout,
	    "USAGE: transit simple 1-5 infile outfile\n"
	    "       focus dur steps max dec [-tthres -dlim -etlim -mmaxdur] [-l]\n"
		"\n"
		"Place repetitions of a mono sound on tangent path to & from 8-channel array.\n"
		"Lspkrs equidistant in a ring, numbered clockwise, Output is a mixfile.\n"
		"\n"
		"MODES:\n"
		"      1                2                3               4               5\n"
		"   (glancing)      (edgewise)      (crossing)         (close)        (central)\n"
		"->>---O->>      ->>--O---O->>           O             O   O             O\n"
		"  O       O                    ->>--O-------O->>                    O       O\n"
		"                  O         O                  ->>-O---------O->>\n"
		" O         O                       O         O                 ->>-O---------O->>\n"
		"                  O         O                      O         O\n"
		"  O       O                         O       O                       O       O\n"
		"      O              O   O              O             O   O             O\n"
		"\n"
		"FOCUS  Centre of motion. Integer for odd modes, 1.5 ; 2.5 etc for even modes.\n"
		"DUR    Duration of motion from edge to centre (ONLY).\n"
		"STEPS  Count of events from edge to centre (ONLY).\n"
		"MAXA   (Modes 1-4) Maxangle from centreline reached( < 90).\n"
		"MAXD   (Mode 5) Max distance from centre reached, where halfradius = 1.\n"
		"DEC    Gain decrement (>0 <1) on passing from one event to the next.\n"
		"\n"
		"To extend motion further (increasing duration and total number of events)....\n"
		"gain decrement can be modofied with these 4 parameters ... \n"
		"\n"
		"THRESH Threshold (overall) gain at which gain decrement starts to increase.\n"
		"LIM    Maximum level of gain decrement after this point (> DEC).\n"
		"TLIM   Minimum (overall) gain at which event ends ( < THRESH).\n"
		"MAXDUR Max duration motion edge-to-centre (in case TLIM never reached) ( >= DUR).\n"
		"\n"
		"-l     Motion towards left of focal position (Default: motion towards right).\n");
	} else if(!strcmp(str,"filtered")) {
		fprintf(stdout,
	    "USAGE: transit filtered 1-5 infile1 infile2 outfile\n"
	    "       focus dur steps max dec fdec [-tthres -dlim -etlim -mmaxdur] [-l]\n"
		"\n"
		"Place repetitions of a mono sound on tangent path to & from 8-channel array.\n"
		"Lspkrs equidistant in a ring, numbered clockwise, Output is a mixfile.\n"
		"\n"
		"Second sound is filtered version of first, suggesting greater distance.\n"
		"Gradually mix in more of 2nd input sound, with greater distance from centre.\n"
		"\n"
		"MODES:\n"
		"      1                2                3               4               5\n"
		"   (glancing)      (edgewise)      (crossing)         (close)        (central)\n"
		"->>---O->>      ->>--O---O->>           O             O   O             O\n"
		"  O       O                    ->>--O-------O->>                    O       O\n"
		"                  O         O                  ->>-O---------O->>\n"
		" O         O                       O         O                 ->>-O---------O->>\n"
		"                  O         O                      O         O\n"
		"  O       O                         O       O                       O       O\n"
		"      O              O   O              O             O   O             O\n"
		"\n"
		"FOCUS  Centre of motion. Integer for odd modes, 1.5 ; 2.5 etc for even modes.\n"
		"DUR    Duration of motion from edge to centre (ONLY).\n"
		"STEPS  Count of events from edge to centre (ONLY).\n"
		"MAXA   (Modes 1-4) Maxangle from centreline reached( < 90).\n"
		"MAXD   (Mode 5) Max distance from centre reached, where halfradius = 1.\n"
		"DEC    Gain decrement (>0 <1) on passing from one event to the next.\n"
		"FDEC   Progressive mix-in of infile2 (range 0-1).\n"
		"\n"
		"To extend motion further (increasing duration and total number of events)....\n"
		"gain decrement can be modofied with these 4 parameters ... \n"
		"\n"
		"THRESH Threshold (overall) gain at which gain decrement starts to increase.\n"
		"LIM    Maximum level of gain decrement after this point (> DEC).\n"
		"TLIM   Minimum (overall) gain at which event ends ( < THRESH).\n"
		"MAXDUR Max duration motion edge-to-centre (in case TLIM never reached) ( >= DUR).\n"
		"\n"
		"-l     Motion towards left of focal position (Default: motion towards right).\n");
	} else if(!strcmp(str,"doppler")) {
		fprintf(stdout,
	    "USAGE: transit doppler 1-5 infile1 infile2 [infile3 ....] outfile\n"
		"       focus dur steps max dec [-tthres -dlim -etlim -mmaxdur] [-l]\n"
		"\n"
		"Place repetitions of a mono sound on tangent path to & from 8-channel array.\n"
		"Lspkrs equidistant in a ring, numbered clockwise, Output is a mixfile.\n"
		"\n"
		"Second sound is pitch-shifted version of first, suggesting doppler shift.\n"
		"Change to sound 2 after centre is passed.\n"
		"(Possibly insert more sounds around motioncentre: suggest gradual doppler change\n"
		" NB order of sounds is aprroaching-sound, final-sound, intermediate-sounds.)\n"
		"\n"
		"MODES:\n"
		"      1                2                3               4               5\n"
		"   (glancing)      (edgewise)      (crossing)         (close)        (central)\n"
		"->>---O->>      ->>--O---O->>           O             O   O             O\n"
		"  O       O                    ->>--O-------O->>                    O       O\n"
		"                  O         O                  ->>-O---------O->>\n"
		" O         O                       O         O                 ->>-O---------O->>\n"
		"                  O         O                      O         O\n"
		"  O       O                         O       O                       O       O\n"
		"      O              O   O              O             O   O             O\n"
		"\n"
		"FOCUS  Centre of motion. Integer for odd modes, 1.5 ; 2.5 etc for even modes.\n"
		"DUR    Duration of motion from edge to centre (ONLY).\n"
		"STEPS  Count of events from edge to centre (ONLY).\n"
		"MAXA   (Modes 1-4) Maxangle from centreline reached( < 90).\n"
		"MAXD   (Mode 5) Max distance from centre reached, where halfradius = 1.\n"
		"DEC    Gain decrement (>0 <1) on passing from one event to the next.\n"
		"\n"
		"To extend motion further (increasing duration and total number of events)....\n"
		"gain decrement can be modofied with these 4 parameters ... \n"
		"\n"
		"THRESH Threshold (overall) gain at which gain decrement starts to increase.\n"
		"LIM    Maximum level of gain decrement after this point (> DEC).\n"
		"TLIM   Minimum (overall) gain at which event ends ( < THRESH).\n"
		"MAXDUR Max duration motion edge-to-centre (in case TLIM never reached) ( >= DUR).\n"
		"\n"
		"-l     Motion towards left of focal position (Default: motion towards right).\n");
	} else if(!strcmp(str,"doplfilt")) {
		fprintf(stdout,
	    "USAGE: transit doplfilt 1-5 infil1 infil2 infil3 infil4 [infil5...] outfil\n"
		"       focus dur steps max dec fdec [-tthres -dlim -etlim -mmaxdur] [-l]\n"
		"\n"
		"Place repetitions of a mono sound on tangent path to & from 8-channel array.\n"
		"Lspkrs equidistant in a ring, numbered clockwise, Output is a mixfile.\n"
		"\n"
		"Second sound is filtered version of infile1, suggesting greater distance.\n"
		"Gradually mix in more of 2nd input sound, with greater distance from centre.\n"
		"Third sound is pitch-shifted version of first, suggesting doppler shift.\n"
		"Switch to sound3 after centre is passed.\n"
		"Fourth sound is filtered version of third, suggesting greater distance.\n"
		"(Possibly insert more sounds around motioncentre: suggest gradual doppler change\n"
		" NB order of sounds is\n"
		" aprroaching-snd, same_snd filtered, final-sound, final-sound filtered,\n"
		" followed by any intermediate sounds at the doppler-shift centre.)\n"
		"\n"
		"MODES:1                2                3               4               5\n"
		"   (glancing)      (edgewise)      (crossing)         (close)        (central)\n"
		"->>---O->>      ->>--O---O->>           O             O   O             O\n"
		"  O       O                    ->>--O-------O->>                    O       O\n"
		"                  O         O                  ->>-O---------O->>\n"
		" O         O                       O         O                 ->>-O---------O->>\n"
		"                  O         O                      O         O\n"
		"  O       O                         O       O                       O       O\n"
		"      O              O   O              O             O   O             O\n"
		"\n"
		"FOCUS  Centre of motion. Integer for odd modes, 1.5 ; 2.5 etc for even modes.\n"
		"DUR    Duration of motion from edge to centre (ONLY).\n"
		"STEPS  Count of events from edge to centre (ONLY).\n"
		"MAXA   (Modes 1-4) Maxangle from centreline reached( < 90).\n"
		"MAXD   (Mode 5) Max distance from centre reached, where halfradius = 1.\n"
		"DEC    Gain decrement (>0 <1) on passing from one event to the next.\n"
		"\n"
		"To extend motion further (increasing duration and total number of events)....\n"
		"gain decrement can be modofied with these 4 parameters ... \n"
		"\n"
		"THRESH Threshold (overall) gain at which gain decrement starts to increase.\n"
		"LIM    Maximum level of gain decrement after this point (> DEC).\n"
		"TLIM   Minimum (overall) gain at which event ends ( < THRESH).\n"
		"MAXDUR Max duration motion edge-to-centre (in case TLIM never reached) ( >= DUR).\n"
		"\n"
		"-l     Motion towards left of focal position (Default: motion towards right).\n");
	} else if(!strcmp(str,"sequence")) {
		fprintf(stdout,
	    "USAGE: transit sequence 1-5 infile infile2 infile3 [infile4 ....] outfile focus dur max dec [-l]\n"
		"\n"
		"Position a sequence of (at least 3) mono sounds (an odd number of sounds)\n"
		"on tangent path to & from 8-channel array.\n"
		"Lspkrs equidistant in a ring, numbered clockwise, Output is a mixfile.\n"
		"\n"
		"MODES:\n"
		"      1                2                3               4               5\n"
		"   (glancing)      (edgewise)      (crossing)         (close)        (central)\n"
		"->>---O->>      ->>--O---O->>           O             O   O             O\n"
		"  O       O                    ->>--O-------O->>                    O       O\n"
		"                  O         O                  ->>-O---------O->>\n"
		" O         O                       O         O                 ->>-O---------O->>\n"
		"                  O         O                      O         O\n"
		"  O       O                         O       O                       O       O\n"
		"      O              O   O              O             O   O             O\n"
		"\n"
		"FOCUS  Centre of motion. Integer for odd modes, 1.5 ; 2.5 etc for even modes.\n"
		"DUR    Duration of motion from edge to centre (ONLY).\n"
		"MAXA   (Modes 1-4) Maxangle from centreline reached( < 90).\n"
		"MAXD   (Mode 5) Max distance from centre reached, where halfradius = 1.\n"
		"DEC    Gain decrement (>0 <1) on passing from one event to the next.\n"
		"\n"
		"-l     Motion towards left of focal position (Default: motion towards right).\n");
	} else if(!strcmp(str,"list")) {
		fprintf(stdout,
	    "USAGE: transit list 1-5 intextfile outfile focus dur max dec [-l]\n"
		"\n"
		"Position a sequence of (at least 3) mono sounds, listed in a textfile,\n"
		"(there must be an odd number of sounds)\n"
		"on tangent path to & from 8-channel array.\n"
		"Lspkrs equidistant in a ring, numbered clockwise, Output is a mixfile.\n"
		"\n"
		"MODES:\n"
		"      1                2                3               4               5\n"
		"   (glancing)      (edgewise)      (crossing)         (close)        (central)\n"
		"->>---O->>      ->>--O---O->>           O             O   O             O\n"
		"  O       O                    ->>--O-------O->>                    O       O\n"
		"                  O         O                  ->>-O---------O->>\n"
		" O         O                       O         O                 ->>-O---------O->>\n"
		"                  O         O                      O         O\n"
		"  O       O                         O       O                       O       O\n"
		"      O              O   O              O             O   O             O\n"
		"\n"
		"FOCUS  Centre of motion. Integer for odd modes, 1.5 ; 2.5 etc for even modes.\n"
		"DUR    Duration of motion from edge to centre (ONLY).\n"
		"MAXA   (Modes 1-4) Maxangle from centreline reached( < 90).\n"
		"MAXD   (Mode 5) Max distance from centre reached, where halfradius = 1.\n"
		"DEC    Gain decrement (>0 <1) on passing from one event to the next.\n"
		"\n"
		"-l     Motion towards left of focal position (Default: motion towards right).\n");
	} else
		fprintf(stdout,"Unknown option '%s'\n",str);
	return(USAGE_ONLY);
}

int usage3(char *str1,char *str2)
{
	fprintf(stderr,"Insufficient parameters on command line.\n");
	return(USAGE_ONLY);
}

/******************************** TRANSIT ********************************/

int transit(dataptr dz) 
{
	int doplbas = 0, lspkr, rspkr, sound, n, m;
	char temp[400], temp2[400];
	double thispos, thistime, lev, leftgain, rightgain, bal, lgain, rgain;
	double *level = dz->parray[TR_LEVEL], *pos = dz->parray[TR_POSITION], *time = dz->parray[TR_TIME], *balance = dz->parray[TR_FLTMIX];
	int *spkr = dz->iparray[TR_SPKRPAIR], *snd = dz->iparray[TR_SNDFILE];

	if(dz->process == TRANSITFD)		//	Note where doplshifting (extra) sounds begin in input files
		doplbas = 4;
	else if(dz->process == TRANSITF)
		doplbas = 2;
	sprintf(temp,"%d\n",8);
	if(fputs(temp,dz->fp) < 0) {
		sprintf(errstr,"Error writing to output data file\n");
		return(PROGRAM_ERROR);
	}
	for(n=0,m=0;n<dz->itemcnt;n++,m+=2) {
		sound   = snd[n];
		thistime= time[n];
		lev		= level[n];
		thispos = pos[n];
		lspkr   = spkr[m];
		rspkr   = spkr[m+1];
		if(dz->mode == CENTRAL && (fabs(thispos) >= 1)) {
			if(thispos >= 1.0) {	//	end of motion
				if(dz->vflag[0]) {	//	leftwards
					rightgain = 0.0;
					leftgain  = 1.0;
				} else {
					leftgain  = 0.0;
					rightgain = 1.0;
				}
			} else {				//	start of motion
				if(dz->vflag[0]) {	//	leftwards
					leftgain  = 0.0;
					rightgain = 1.0;
				} else {
					rightgain = 0.0;
					leftgain  = 1.0;
				}
			}
		} else {
			thispos = (thispos * 2.0) - 1.0;		//	Convert to -1 to 1 format	
			pancalc(thispos,&leftgain,&rightgain);	//	Lspkr levels corresponding to interlspkr distance
		}
		if(dz->process == TRANSITF || ((dz->process == TRANSITFD) && (sound < doplbas))) {
			bal = balance[n];
			lgain = leftgain  * bal;			//	IF filtered version, times relative level of unfiltered sound
			rgain = rightgain * bal;
		} else {
			lgain = leftgain;
			rgain = rightgain;
		}
		lgain *= lev;								//	times level-factor for distance
		rgain *= lev;
		if(lgain > TR_MIN_GAIN || rgain > TR_MIN_GAIN) {
			sprintf(temp,"%s",dz->wordstor[sound]);	//	Only if we have a valid output
			strcat(temp," ");						//	Output mix line corresponding to event
			sprintf(temp2,"%lf",thistime);
			strcat(temp,temp2);
			if(lgain > TR_MIN_GAIN) {
				strcat(temp," 1 1:");
				sprintf(temp2,"%d",lspkr);
				strcat(temp,temp2);
				strcat(temp," ");
				sprintf(temp2,"%lf",lgain);
				strcat(temp,temp2);
			}
			if(rgain > TR_MIN_GAIN ) {
				strcat(temp," 1:");
				sprintf(temp2,"%d",rspkr);
				strcat(temp,temp2);
				strcat(temp," ");
				sprintf(temp2,"%lf",rgain);
				strcat(temp,temp2);
			}
			strcat(temp,"\n");
			if(fputs(temp,dz->fp) < 0) {
				sprintf(errstr,"Error writing to output data file\n");
				return(PROGRAM_ERROR);
			}
		}										//	If there's a filtered version of the sound
		if(dz->process == TRANSITF || ((dz->process == TRANSITFD) && (sound < doplbas))) {
			bal = balance[n];
			lgain = leftgain  * (1.0 - bal);
			rgain = rightgain * (1.0 - bal);	//	Get level of filtered version of sound
			lgain *= lev;						//	times level-factor for distance
			rgain *= lev;
			if(lgain > TR_MIN_GAIN || rgain > TR_MIN_GAIN) {
				sound++;						//	Get filtered sound
				sprintf(temp,"%s",dz->wordstor[sound]);
				strcat(temp," ");
				sprintf(temp2,"%lf",thistime);
				strcat(temp,temp2);
				if(lgain > TR_MIN_GAIN) {
					strcat(temp," 1 1:");
					sprintf(temp2,"%d",lspkr);
					strcat(temp,temp2);
					strcat(temp," ");
					sprintf(temp2,"%lf",lgain);
					strcat(temp,temp2);
				}
				if(rgain > TR_MIN_GAIN ) {
					strcat(temp," 1:");
					sprintf(temp2,"%d",rspkr);
					strcat(temp,temp2);
					strcat(temp," ");
					sprintf(temp2,"%lf",rgain);
					strcat(temp,temp2);
				}
				strcat(temp,"\n");
				if(fputs(temp,dz->fp) < 0) {
					sprintf(errstr,"Error writing to output data file\n");
					return(PROGRAM_ERROR);
				}
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

/****************************** STORE_THE_FILENAME *********************************/

int store_the_filename(char *filename,dataptr dz)
{
	char *p;
	int k;
	if(dz->wordstor != NULL) {
		sprintf(errstr,"Cannot store filename: wordstor already allocated.\n");
		return(PROGRAM_ERROR);
	}
	if((dz->wordstor = (char **)malloc(dz->infilecnt * sizeof(char *)))==NULL) {
		sprintf(errstr,"Cannot store filename.\n");
		return(MEMORY_ERROR);
	}
	p = filename + strlen(filename);
	p--;
	while(p != filename) {
		if(*p == '.') {
			if(strcmp(p,".wav")) {
				sprintf(errstr,"Input file %s is not a soundfile.\n",filename);
				return(DATA_ERROR);
			}
			break;
		}
		p--;
	}
	if(p == filename)
		k = 5;
	else
		k = 1;
	if((dz->wordstor[0] = (char *)malloc((strlen(filename)+1) * sizeof(char)))==NULL) {
		sprintf(errstr,"Cannot store filename.\n");
		return(MEMORY_ERROR);
	}
	if((dz->wordstor[0] = (char *)malloc((strlen(filename)+k) * sizeof(char)))==NULL) {
		sprintf(errstr,"Cannot store further filename.\n");
		return(MEMORY_ERROR);
	}
	strcpy(dz->wordstor[0],filename);
	if(k==5)
		strcat(dz->wordstor[0],".wav");
	dz->all_words++;
	return(FINISHED);
}

/****************************** STORE_THE_FURTHER_FILENAME *********************************/

int store_the_further_filename(int n,char *filename,dataptr dz)
{
	char *p;
	int k;
	p = filename + strlen(filename);
	p--;
	while(p != filename) {
		if(*p == '.') {
			if(strcmp(p,".wav")) {
				sprintf(errstr,"Input file %s is not a soundfile.\n",filename);
				return(DATA_ERROR);
			}
			break;
		}
		p--;
	}
	if(p == filename)
		k = 5;
	else
		k = 1;

	if((dz->wordstor[n] = (char *)malloc((strlen(filename)+k) * sizeof(char)))==NULL) {
		sprintf(errstr,"Cannot store further filename.\n");
		return(MEMORY_ERROR);
	}
	strcpy(dz->wordstor[n],filename);
	if(k==5)
		strcat(dz->wordstor[n],".wav");
	dz->all_words++;
	return(FINISHED);
}

/**************************************** REORIENTDATA *********************************************/

int ReorientData(double *pos, double *time, double *level, int *spkr, dataptr dz)
{
	double  timeoffset, central_pos = 1.5, ddiff, thispos;
	int central_spkr = 1, centre_at_lspkr, diff;
	int totalcnt, n, k, kk, nn;
	if(EVEN(dz->mode))
		centre_at_lspkr = 1;
	else
		centre_at_lspkr = 0;
	totalcnt = (dz->itemcnt * 2) - 1;
	if(totalcnt > dz->ringsize) {
		sprintf(errstr,"Error in calculating array sizes.\n");
		return(PROGRAM_ERROR);
	}
	for(k = totalcnt-1,n = dz->itemcnt-1;n>=0;n--,k--) {	//	Move data to array top
		pos[k]   = pos[n];									//	XXXXooo to
		level[k] = level[n];								//	oooXXXX
		time[k]  = time[n];
	}
	for(k = dz->itemcnt-2,n= dz->itemcnt;k>=0;k--,n++) {	//	Copy in inverse order, round centre item
		pos[k]   = pos[n];	//	But lspkr pairs inverted	//  oooXabc to
		level[k] = level[n];								//	cbaXabc
		time[k]  = -time[n];								//	Inverting the times
	}
	timeoffset = -time[0];									//	Reset times to start at 0.0
	for(n=0;n < totalcnt;n++)
		time[n] = max(0.0,time[n] + timeoffset);
	for(k = totalcnt-1,n = dz->itemcnt-1;n>=0;n--,k--) {	//	Move lspkr data to array top
		kk = k * 2;
		nn = n * 2;
		spkr[kk++] = spkr[nn++];
		spkr[kk]   = spkr[nn];
	}														//	Now invert lspkrs in approach part of array
	if(dz->mode == CENTRAL) {							
		for(k = dz->itemcnt-2,n= dz->itemcnt;k>=0;k--,n++) {	//	IF crossing centre ...	
			kk = k * 2;
			nn = n * 2;
			if(spkr[nn] != spkr[nn+1]) {						//	If on a spkr-pair (i.e. between lspkrs)
				spkr[kk]   = spkr[nn+1];						//	Invert the pair
				spkr[kk+1] = spkr[nn];
			} else {											//	Else (on a single lspkr - i.e. exited lspkr-ring)
				spkr[kk] = spkr[nn] - 4;						//	Put both signals on the opposite lspkr
				if(spkr[kk] <= 0)								
					spkr[kk] += 8;
				spkr[kk+1] = spkr[kk];
			}
		}
	} else {												//	Otherwise
		if(centre_at_lspkr)
			central_spkr = dz->iparam[TRAN_FOCUS];				//	Either Centred at lspkr
		else													//	or
			central_pos = dz->param[TRAN_FOCUS];				//	Centred between left and right lspkr
		for(k = dz->itemcnt-2,n= dz->itemcnt;k>=0;k--,n++) {	
			kk = k * 2;											//	Copy in inverse order, round centre item
			nn = n * 2;
			if(centre_at_lspkr) {								//	If centred on a loudspeaker	
				diff = spkr[nn] - central_spkr;					//	Find distance from centre (allowing for wraparound at lspkr 8)
				if(diff > 4)									//	e.g. centre at 7, next spkr at 1
					diff -= 8;									//	diff 1-7 = -6 equivalent to step of 2 lspkrs
				spkr[kk] = central_spkr - diff;					//	Orient lspkrs in opposite sense from centre

			} else {											//	If not centred AT a lspkr (but in-between 2)
				ddiff = (double)spkr[nn] - central_pos;			//	Measure distance from centre position
				if(ddiff > 4)
					ddiff -=  8.0;
				thispos   = central_pos - ddiff;				//	Invert it and
				if(thispos > 0.0)								//	Get corresponding lspkr
					spkr[kk] = (int)round(thispos);
				else
					spkr[kk] = -(int)round(-thispos);
			}
			if(spkr[kk] > 8)									//	Adjust fro wrap-around lspkr 8
				spkr[kk] -= 8;
			else if(spkr[kk] < 1)
				spkr[kk] += 8;
			kk++;
			nn++;
			if(centre_at_lspkr) {								//	Do same for other loudspeaker of pair
				diff = spkr[nn] - central_spkr;
				if(diff > 0)
					diff -= 8;
				spkr[kk] = central_spkr - diff;
			} else {
				ddiff = (double)spkr[nn] - central_pos;
				if(ddiff > 4)
					ddiff -=  8.0;
				thispos   = central_pos - ddiff;
				if(thispos > 0.0)
					spkr[kk] = (int)round(thispos);
				else
					spkr[kk] = -(int)round(-thispos);
			}
			if(spkr[kk] > 8)
				spkr[kk] -= 8;
			else if(spkr[kk] < 1)
				spkr[kk] += 8;
		}
	}
	dz->itemcnt = totalcnt;										//	Remember (new) number of events
	return FINISHED;
}

/**************************************** LEFTWARDS *********************************************/

void leftwards(double *pos, double *time, double *level, int *spkr, dataptr dz)
{
	int  central_pos, temp1, temp2;
	int n, m, nn, mm;
	central_pos = dz->itemcnt/2;
	for(n = central_pos -1,m= central_pos +1;n >= 0;n--,m++) {
		nn = n * 2;
		mm = m * 2;
		temp1 = spkr[nn];
		temp2 = spkr[nn+1];
		spkr[nn]   = spkr[mm];
		spkr[nn+1] = spkr[mm+1];
		spkr[mm]   = temp1;
		spkr[mm+1] = temp2;
	}
	return;
}

/**************************************** GET_POSITION *********************************************/

int get_position(double *thispos,int *spkr1,int *spkr2,double linear_steplen,int n,
 double *distance_to_centre,double first_quadrant,double second_quadrant,int *on_2nd_pair,int *on_3rd_pair,dataptr dz)
{
	double angle;
	switch(dz->mode) {
	case(GLANCING):
		angle = atan((linear_steplen * (double)n)/(*distance_to_centre));
		if(angle <= first_quadrant)		//	first_quadrant = 45	
			*thispos = angle/DEG45;		//
		else {							//		O-d-|-d-|-d-
			if(!(*on_2nd_pair)) {		//   O	|  O
				*spkr1 = *spkr2;		//      |x							
				if(*spkr1 > 8)			//  O	c	O.......
					(*spkr1) -= 8;		//
				if(++(*spkr2) > 8)		//	 O	   O
					(*spkr2) -= 8;		//		O
				*on_2nd_pair = 1;
			}
			*thispos = (angle - first_quadrant)/DEG45;
		}
		break;
	case(EDGEWISE):
		angle = atan((linear_steplen * (double)n)/(*distance_to_centre));
		if(angle <= first_quadrant)							//	first_quadrant = 22.5	
			*thispos = (angle + first_quadrant)/DEG45;		//
		else if(angle <= second_quadrant) {					//	  O---O-------
			if(!(*on_2nd_pair)) {							//   	|
				*spkr1 = *spkr2;							//  O   |x  O
				if(*spkr1 > 8)								//  	c............
					(*spkr1) -= 8;							//	O       O	
				if(++(*spkr2) > 8)							//
					(*spkr2) -= 8;							//	  O   O
				*on_2nd_pair = 1;
			}
			*thispos = (angle - first_quadrant)/DEG45;
		} else {
			if(!(*on_3rd_pair)) {							//	second_quadrant = 67.5
				if(++(*spkr1)> 8)
					(*spkr1) -= 8;
				if(++(*spkr2) > 8)
					(*spkr2) -= 8;
				*on_3rd_pair = 1;
			}
			*thispos = (angle - second_quadrant)/DEG45;
		}
		break;
	case(CROSSING):
		angle = atan((linear_steplen * (double)n)/(*distance_to_centre));
		if(angle <= first_quadrant)							//	first_quadrant = 45	
			*thispos = (angle + first_quadrant)/DEG90;		//
		else {												//		O
			if(!(*on_2nd_pair)) {							//   O	 --O--------
				*spkr1 = *spkr2;							//      |x							
				if(*spkr1 > 8)								//  O	c	O.......
					(*spkr1) -= 8;							//
				if(++(*spkr2) > 8)							//	 O	   O
					(*spkr2) -= 8;							//		O
				*on_2nd_pair = 1;
			}
			*thispos = (angle - first_quadrant)/DEG45;
		}
		break;
	case(CLOSE):
		angle = atan((linear_steplen * (double)n)/(*distance_to_centre));
		if(angle <= first_quadrant)							//	first_quadrant = 67.5	
			*thispos = (angle + first_quadrant)/DEG135;		//
		else {												//	  O   O
			if(!(*on_2nd_pair)) {							//   	
				*spkr1 = *spkr2;							//  O   |x--O---------
				if(*spkr1 > 8)								//  	c............
					(*spkr1) -= 8;							//	O       O	
				if(++(*spkr2)> 8)							//
					(*spkr2) -= 8;							//	  O   O
				*on_2nd_pair = 1;							
			}
			*thispos = (angle - first_quadrant)/DEG45;
		}
		break;
	case(CENTRAL):
		(*distance_to_centre) += linear_steplen;
		if((*distance_to_centre) > 1.0)	{					//	i.e. outside lspkr ring
			*spkr1 = *spkr2;								//	all level is on single lspkr
			*thispos = 1.0;
		} else
			*thispos = *distance_to_centre;
		break;
	}
	if(*thispos > 1.0) {
		sprintf(errstr,"Position calculation error.\n");
		return(PROGRAM_ERROR);
	}
	return FINISHED;
}

/**************************************** CALC_FILTERS *********************************************/

int calc_filters(double *balance,int *spkr,dataptr dz)
{
	int exclude = 0, centre_event = dz->itemcnt/2, central_spkr, no_of_unfiltered, xs, unfiltered_cnt;
	int left_centre, right_centre, centreleft, n, m, k;
	double bal;
	if(dz->process == TRANSITFD) {
		exclude = dz->infilecnt - 4; //	Don't filter the extra pitch-shifting files
		if(exclude > 0)
			exclude += 2;			//	and don't filter the events on either side of this
	}
	switch(dz->mode) {
	case(GLANCING):
		if(exclude > 0) {
			if(ODD(exclude)) {
				left_centre  = centre_event - (exclude/2);
				right_centre = centre_event + (exclude/2);
			} else {
				left_centre  = centre_event - (exclude/2);
				right_centre = centre_event + (exclude/2);
			}
		} else {
			left_centre  = centre_event;
			right_centre = centre_event;
		}
		if(left_centre < 0)
			left_centre = 0;
		if(right_centre >= dz->itemcnt)
			right_centre = dz->itemcnt - 1;
		for(n = left_centre; n <= right_centre; n++)
			balance[n] = 1.0;
		n = right_centre + 1;
		bal = 1.0;
		while(n < dz->itemcnt) {
			bal *= dz->param[TRAN_FBAL];
			balance[n] = bal;
			n++;
		}
		n = left_centre - 1;
		bal = 1.0;
		while(n >= 0) {
			bal *= dz->param[TRAN_FBAL];
			balance[n] = bal;
			n--;
		}
		break;
	case(EDGEWISE):
	case(CROSSING):
	case(CLOSE):
	case(CENTRAL):
		centreleft = centre_event * 2;		//	array-index of left speaker of central event
		m = centreleft;
		central_spkr = spkr[m];				//	Actual left speaker of central event
		while(spkr[m] == central_spkr) {
			m += 2;
			if(m >= dz->itemcnt * 2)
				break;
		}
		m /= 2;								//	Array-index of balance-array where sound leaves lspkr-ring
		if(exclude > 0) {
			no_of_unfiltered = (m - centre_event) * 2;
			no_of_unfiltered--;				//	Check if room for doppler-shifting sounds
			xs = exclude - no_of_unfiltered;
			if(xs > 0)						//	and if not, increase span of unfiltered events, if possible
				m = min(dz->itemcnt,m+xs);
		}
		unfiltered_cnt = 0;
		for(n = centre_event;n <= m;n++) {	//	While sound is crossing lspkrs (& not pitch-changing), don't filter
			balance[n] = 1.0;
			unfiltered_cnt++;
		}
		bal = 1.0;
		while(n < dz->itemcnt) {
			bal *= dz->param[TRAN_FBAL];
			balance[n] = bal;
			n++;
		}
		for(k = 1, n = centre_event-1;k < unfiltered_cnt;k++,n--)
			balance[n] = 1.0;
		bal = 1.0;
		while(n >= 0) {
			bal *= dz->param[TRAN_FBAL];
			balance[n] = bal;
			n--;
		}
		break;
	}
	return FINISHED;
}

/**************************************** INSERT_SOUNDS *********************************************/

int insert_sounds(int *snd,dataptr dz)
{
	int doplevents = 0, centre_event = dz->itemcnt/2, sndbas = 0;
	int left_centre, right_centre, n, k, snd1 = 0, snd2 = 0;
	switch(dz->process) {
	case(TRANSITD):
	case(TRANSITFD):
		if(dz->process == TRANSITFD) {
			sndbas = 4;
			snd1 = 0;
			snd2 = 2;
		} else if(dz->process == TRANSITD) {
			sndbas = 2;
			snd1 = 0;
			snd2 = 1;
		}
		doplevents = dz->infilecnt - sndbas; // Number of sounds which gradually-pshift to doplshifted end-sound
		if(doplevents > 0) {
			if(ODD(doplevents)) {
				left_centre  = centre_event - (doplevents/2);
				right_centre = centre_event + (doplevents/2);
			} else {
				left_centre  = centre_event - (doplevents/2);
				right_centre = centre_event + (doplevents/2) - 1;
			}
		} else {
			left_centre  = centre_event;
			right_centre = centre_event - 1;
		}									//	Put any gradual dopl-shift srcs in middle porition of motion
		for(n = left_centre,k=0; n <= right_centre; n++,k++) {
			if(n >= 0 && n < dz->itemcnt)
				snd[n] = sndbas + k;
		}									//	Assign first (or first of filter-pair) of sounds to approaching path
		n = left_centre - 1;
		while(n >= 0)
			snd[n--] = snd1;
		n = right_centre + 1;				//	Assign second (or 2nd of filter-pair) of sounds to receding path
		while(n < dz->itemcnt)
			snd[n++] = snd2;
		break;
	case(TRANSITS):
	case(TRANSITL):
		for(n = 0;n < dz->itemcnt;n++)
			snd[n] = n;
		break;
	default:
		for(n = 0;n < dz->itemcnt;n++)
			snd[n] = 0;
		break;
	}
	return FINISHED;
}

/****************************** READ_INPUT_SNDFILES_LIST *********************************
 *
 *	Ensure Files are mono
 *  Compare properties (srate and channel-count)
 *	Store channel-cnt and the duration.
 *
 */

int read_input_sndfiles_list(char *filename,dataptr dz)
{
	int exit_status, n;
	char *p, fnam[400];
	infileptr infile_info;
	FILE *fp;
	if((fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,"Cannot open file %s to read souNdfile names.\n",filename);
		return(DATA_ERROR);
	}
	dz->infilecnt = 0;
	while(fgets(fnam,200,fp)!=NULL) {
		p = fnam;
		while(isspace(*p))
			p++;
		if(*p == ';')				//	Allow comments in file
			continue;
		dz->infilecnt++;
	}
	if(dz->infilecnt <= 0) {
		sprintf(errstr,"No data found in file %s\n",filename);
		return(DATA_ERROR);
	}
	dz->infile->filetype = SNDFILE;
	dz->input_data_type  = MANY_SNDFILES;
	if((exit_status = allocate_the_filespace(dz))<0)
		return(exit_status);
	fseek(fp,0,0);
	n = 0;
	while(fgets(fnam,200,fp)!=NULL) {
		p = fnam;
		while(isspace(*p))
			p++;
		if(*p == ';')				//	Allow comments in file
			continue;
		p = fnam + strlen(fnam);	//	Remove newline character
		p--;
		while(p != fnam) {
			if(*p == '\n') {
				*p = ENDOFSTR;
				break;
			}
			p--;
		}
		if(n == 0) {
			if((exit_status = open_first_infile(fnam,dz))<0)
				return exit_status;
			if((infile_info = (infileptr)malloc(sizeof(struct filedata)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY for infile structure to test TK data.");
				return(MEMORY_ERROR);
			}
			if((exit_status = cdparse(fnam,infile_info))<0)
				return(exit_status);
			if(infile_info->channels != MONO) {
				sprintf(errstr,"Process only works with mono input files.");
				return(DATA_ERROR);
			}
			dz->infile->srate    = infile_info->srate;
			dz->infile->channels = infile_info->channels;
			dz->duration         = infile_info->duration;
			if((exit_status = store_the_filename(fnam,dz))<0)
				return(exit_status);
		} else {
			if((exit_status = handle_other_infile(n,fnam,dz))<0)
				return exit_status;
			sndcloseEx(dz->ifd[n]);
			dz->ifd[n] = -1;
			if((exit_status = store_the_further_filename(dz->all_words,fnam,dz))<0)
				return(exit_status);
		}
		n++;
	}
	fclose(fp);
	if(dz->ifd[0] >= 0) {
		sndcloseEx(dz->ifd[0]);
		dz->ifd[0] = -1;
	}
	return FINISHED;
}

/************************ ALLOCATE_THE_FILESPACE *********************/

int allocate_the_filespace(dataptr dz)
{
	int n;
	if((dz->ifd = (int *)malloc(dz->infilecnt * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for infile poniters array.\n");
		return(MEMORY_ERROR);
	}					
	if((dz->insams = (int *)malloc(dz->infilecnt * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for infile-sampsize array.\n");
		return(MEMORY_ERROR);
	}					
	for(n=0;n<dz->infilecnt;n++) {
		dz->ifd[n]        = -1;
		dz->insams[n]     = 0L;
	}
	return(FINISHED);
}

