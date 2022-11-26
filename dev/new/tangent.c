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
 *	Calculations for position as created as explained in text abutting the code (see param_preprocess).
 *	either using angle-subtended to the listener, or position in apparent width of lspks at given distrance.
 *
 *	However, panning was more effective when extended to an extra loudspeaker,
 *  So what really happens is that the original panning procedures are followed, 
 *  but, with approaching motion,  to the lspkr BEFORE the focus (goal) of the movement
 *  (or, with receding motion,  from the lspkr AFTER the focus (start) of the movement)
 *	And then extended to the goal lspkr by steps equal to the largest step used in the calculated motion.
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

#define	RECEDE 0
#define	LEFT   1
#define	EXTEND 2

#define EXTENSION ringsize

#ifdef unix
#define round(x) lround((x))
#endif

char errstr[2400];

int anal_infiles = 1;
int	sloom = 0;
int sloombatch = 0;

const char* cdp_version = "7.1.0";

//CDP LIB REPLACEMENTS
static int setup_tangent_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_tangent_param_ranges_and_defaults(dataptr dz);
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

static int tangent(dataptr dz);
static void pancalc(double position,double *leftgain,double *rightgain);
static int tangent_param_preprocess(dataptr dz);
static int store_the_filename(char *filename,dataptr dz);
static int store_the_further_filename(int n,char *filename,dataptr dz);
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
		dz->maxmode = 8;
		if((exit_status = get_the_mode_from_cmdline(cmdline[0],dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(exit_status);
		}
		cmdline++;
		cmdlinecnt--;
		// setup_particular_application =
		if((exit_status = setup_tangent_application(dz))<0) {
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
	if((exit_status = setup_tangent_param_ranges_and_defaults(dz))<0) {
		exit_status = print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	dz->all_words = 0;
	dz->ifd[0] = -1;
	switch(dz->process) {
	case(TAN_ONE) :
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
	case(TAN_TWO) :
	case(TAN_SEQ) :
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
				sprintf(errstr,"Possibly too many parameters on commandline.\n");
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
	case(TAN_LIST) :
		if((exit_status = read_input_sndfiles_list(cmdline[0],dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);	
			return(FAILED);
		}
		cmdlinecnt--;
		cmdline++;
		break;
	}
	if(dz->ifd[0] >= 0) {
		sndcloseEx(dz->ifd[0]);
		dz->ifd[0] = -1;
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
	is_launched = TRUE;
//	create_sndbufs())		redundant
	if((exit_status = tangent_param_preprocess(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = open_the_outfile(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	//spec_process_file =
	if((exit_status = tangent(dz))<0) {
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

/************************* SETUP_TANGENT_APPLICATION *******************/

int setup_tangent_application(dataptr dz)
{
	int exit_status;
	aplptr ap;
	if((exit_status = establish_application(dz))<0)		// GLOBAL
		return(FAILED);
	ap = dz->application;
	// SEE parstruct FOR EXPLANATION of next 2 functions
	switch(dz->process) {
	case(TAN_ONE):
		exit_status = set_param_data(ap,0   ,5,4,"didd0");
		break;
	case(TAN_TWO):
		exit_status = set_param_data(ap,0   ,5,5,"diddd");
		break;
	case(TAN_SEQ):
	case(TAN_LIST):
		exit_status = set_param_data(ap,0   ,5,3,"d0dd0");
		break;
	}
	if(exit_status < 0)
		return(exit_status);
	switch(dz->mode) {
	case(0):
		exit_status = set_vflgs(ap,"fjs",3,"iDd","rl",2,0,"00");
		break;
	case(1):
		exit_status = set_vflgs(ap,"fj",2,"iD","rl",2,0,"00");
		break;
	}
	if(exit_status < 0)
		return(exit_status);
	// set_legal_infile_structure -->
	dz->has_otherfile = FALSE;
	// assign_process_logic -->
	switch(dz->process) {
	case(TAN_ONE):
		dz->input_data_type = SNDFILES_ONLY;
		break;
	case(TAN_TWO):
		dz->input_data_type = TWO_SNDFILES;
		break;
	case(TAN_SEQ):
		dz->input_data_type = MANY_SNDFILES;
		break;
	case(TAN_LIST):
		dz->input_data_type = SNDLIST_ONLY;
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
		if(dz->process == TAN_LIST) {
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

/************************* SETUP_TANGENT_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_tangent_param_ranges_and_defaults(dataptr dz)
{
	int exit_status;
	aplptr ap = dz->application;
	// set_param_ranges()
	ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
	// NB total_input_param_cnt is > 0 !!!
	if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
		return(FAILED);
	// get_param_ranges()
	ap->lo[TAN_DUR]	= dz->duration * 2.0;
	ap->hi[TAN_DUR]	= 32767;
	ap->default_val[TAN_DUR]	= 8;
	if(dz->process == TAN_ONE || dz->process == TAN_TWO) {
		ap->lo[TAN_STEPS]	= 2;
		ap->hi[TAN_STEPS]	= 32767;
		ap->default_val[TAN_STEPS] = 24;
	}
	if(dz->mode == 0) {
		ap->lo[TAN_MANG]	= 90;
		ap->hi[TAN_MANG]	= 135;
		ap->default_val[TAN_MANG] = 130;
	} else {
		ap->lo[TAN_SKEW]	= 0;
		ap->hi[TAN_SKEW]	= 1;
		ap->default_val[TAN_SKEW] = .75;
	}
	ap->lo[TAN_DEC]	= 0;
	ap->hi[TAN_DEC]	= 1;
	ap->default_val[TAN_DEC] = .9;
	if(dz->process == TAN_TWO) {
		ap->lo[TAN_FBAL]	= 0;
		ap->hi[TAN_FBAL]	= 1;
		ap->default_val[TAN_FBAL] = .9;
	}
	ap->lo[TAN_FOCUS]	= 1;
	ap->hi[TAN_FOCUS]	= 8;
	ap->default_val[TAN_FOCUS] = 1;
	ap->lo[TAN_JITTER]	= 0;
	ap->hi[TAN_JITTER]	= 1;
	ap->default_val[TAN_JITTER] = 0;
	if(dz->mode == 0) {
		ap->lo[TAN_SLOW]	= 0;
		ap->hi[TAN_SLOW]	= 1;
		ap->default_val[TAN_SLOW] = 0.5;
	}
	dz->maxmode = 8;
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
			if((exit_status = setup_tangent_application(dz))<0)
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
	fprintf(stdout,
	"\nCREATE TANGENT MOTION TO OR FROM 8-CHANNEL RING\n\n"
	"USAGE: tangent NAME mode infile(s) outfile parameters: \n"
	"\n"
	"where NAME can be any one of\n"
	"\n"
	"onefile    twofiles    sequence    list\n\n"
	"Type 'tangent onefile' for more info on tangent onefile..ETC.\n");
	return(USAGE_ONLY);
}

/************************************* TANGENT_PARAM_PREPROCESS *********************************/

int tangent_param_preprocess(dataptr dz)
{
	int stepcnt;
	double *lens, *angles;
	double rightangle = PI/2.0, maxangle = 0.0, tangentlen, linear_steplen, sum_of_angles, angle, newtotlen;
	int steps_betwixt_12, n, m;
	double direct_steplen, subtended_steplen;
	if(dz->mode==0) {
		dz->param[TAN_MANG] -= 45.0;		//	If motion starts at centre (lspkr 1)	
		if(dz->param[TAN_MANG] < 46.0)		//	max angle value lies between lspkrs 3 & 4 (90 and 135)
			dz->param[TAN_MANG] = 46.0;		//  We do the calulation AS IF we're starting at lspkr 2
		if(dz->param[TAN_MANG] > 89.0)		//	So the maxangle is between 45 and 90
			dz->param[TAN_MANG] = 89.0;		//	Force end of motion to be between lspkrs 3 & 4
	}
	if(dz->process == TAN_SEQ || dz->process == TAN_LIST)						
		stepcnt = dz->infilecnt;
	else
		stepcnt = dz->iparam[TAN_STEPS];
	dz->array_cnt = 1;
	if((dz->parray = (double **)malloc(dz->array_cnt * sizeof(double *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create segment lengths arrays.\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[0] = (double *)malloc((stepcnt + 1) * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store segment lengths.\n");
		return(MEMORY_ERROR);
	}
	lens = dz->parray[0];
	angles = lens;		//	Same object used: naming for code clarity.

	if(dz->mode == 0) {

		maxangle = dz->param[TAN_MANG] * PI/180.0;
		//	assuming circle radius = 1			//	  __________tangentlen______________
		tangentlen = tan(maxangle);				//	 |								__/
		linear_steplen = tangentlen/stepcnt;	//	 |\				      _________/
		sum_of_angles = 0.0;					//	1|maxangle   _________/
												//	 | __\_____/
												//	 |/
		for(n=1;n<=stepcnt;n++) {
			angle = rightangle - sum_of_angles - atan(1/(linear_steplen * n));
			angles[n-1] = angle;
			sum_of_angles += angle;
		}

		if(!flteq(dz->param[TAN_SLOW],1.0)) {
			sum_of_angles = 0.0;
			for(n=0;n<stepcnt;n++) {
				angles[n] = pow(angles[n],dz->param[TAN_SLOW]);
				sum_of_angles += angles[n];
			}
		}										//	   x		     x		x
												//	 ------		   ------|------		
												//  | 	  .		  |		.	  .
		// steps are now made equivalent to		//  |    .		  |    .	.
		// subtended angles						//  | a . (s=0)	  |	s . a .	
		newtotlen = 0;							// 1|  .		 1|  .	.	
		for(n=0;n<stepcnt;n++) {				//  | .			  | . .			
			lens[n] = angles[n]/sum_of_angles;	//  |. b		  |..   b		
			newtotlen += lens[n];				//  |______		  |_____________		
		}										//
		// Find new lens as proportion of total	//  Angle a is left over after 
		for(n=0;n<stepcnt;n++)					//  subtracting b, and sum of previous angles (s)
			lens[n] /= newtotlen;				//	from rightangle
												//	b = arctan(1/Nx)
		// In plane of lspkrs,					
		// total distance between lspkes 1 and 3 = 2
		for(n=0;n<stepcnt;n++)					  //	Angular distance to lspkr 3 = rightangle
			lens[n] *= (maxangle/rightangle) * 2; //	Distance to maxangle is = (maxangle/rightangle) * length 2
	} else {

		steps_betwixt_12 = (int)floor(stepcnt * (1.0 - dz->param[TAN_SKEW]));
		direct_steplen = 1.0/(double)steps_betwixt_12;					//	\___x_\__x__\/		 /
																		//	 \	   \	/		/ At every step (y),
																		//	  \		\  /	   /  Width subtended by lspkrs
		for(n=0;n<steps_betwixt_12;n++) // Assume equal steps			//	   \__x__\/		  /	  increases by x.
			lens[n] = direct_steplen;	//  btwn lspkrs 1+2				//		\	 /		 /	  If inter-lskpr dist (z) = 1	
																		//		 \	/y		/	steps as proportion of interlspkr dist
																		//		  \0______0/	are x(1+x), x(1+2x) etc.	  	
		newtotlen = 0;													//	0 =    \ z=1  /	
		subtended_steplen = (2 * direct_steplen) * sin(PI/8.0);			// lspkrs at\	 /	L = listener
		for(m = 1; n < stepcnt; n++,m++) {		// For remaining steps	// 45degrees \  /
			lens[n] = subtended_steplen;		// In diagram, x		//			  \L
			lens[n] /= (1.0 + (m * lens[n]));	// In diagram, x/(1+mx)	//	y = direct_steplen. Angle at lspkr (0) = 45 = PI/4
			newtotlen += lens[n];										//  Bisecting angle, sin(PI/8) = (x/2)/y = x/2y
		} 																//  so x = 2ysin(PI/8)
		
		// Finally scale to lie vetween lsprs							
		for(n = steps_betwixt_12; n < stepcnt; n++)
			lens[n] /= newtotlen;
	}	

													//  Now extend the motion to the next lspkr	
	dz->EXTENSION = max(1,(int)floor(1.0/lens[0]));	//	Makes steps in extension >= to maximum previous step
	if(dz->vflag[LEFT]) {
		dz->iparam[TAN_FOCUS]--;
		if(dz->iparam[TAN_FOCUS] < 1)				//	Move the focus of the movement calculated above to the previous lspkr
			dz->iparam[TAN_FOCUS] = 8;				//	Then add the extension,
	} else {										//	taking the complete path to the
		dz->iparam[TAN_FOCUS]++;					//	original FOCUS loudspeaker
		if(dz->iparam[TAN_FOCUS] > 8)
			dz->iparam[TAN_FOCUS] = 1;
	}
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
	if(!strcmp(prog_identifier_from_cmdline,"onefile"))			dz->process = TAN_ONE;
	else if(!strcmp(prog_identifier_from_cmdline,"twofiles"))	dz->process = TAN_TWO;
	else if(!strcmp(prog_identifier_from_cmdline,"sequence"))	dz->process = TAN_SEQ;
	else if(!strcmp(prog_identifier_from_cmdline,"list"))		dz->process = TAN_LIST;
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
	if(!strcmp(str,"onefile")) {
		fprintf(stdout,
	    "USAGE: tangent onefile\n"
	    " 1 infile    outf dur steps maxangle dec     [-ffoc] [-jjitter] [-sslow] [-r] [-l]\n"
	    " 2 infile    outf dur steps skew     dec     [-ffoc] [-jjitter]          [-r] [-l]\n"
		"\n"
		"Play repets of mono sound on tangent path to (or from) 8-channel, front-centred\n"
		"loudspeaker array. Lspkrs numbered clockwise, starting with \"1\" at front-centre.\n"
		"\n"
		"MODE 1  For focus at 1, tangent starts along line at right-angles to lspkr 2.\n"
		"MODE 2  For focus at 1 tangent starts along line formed by lspkrs 2 and 3.\n"
		"\n"
		"DUR        Duration of output file.\n"
		"STEPS      Count of events in tangent stream (Modes 1-4 only).\n"
		"           Note that in ALL modes, in the part of the stream closest to the focus\n"
		"           SAME initial (receding) or final (approaching) event is repeated,\n"
		"           and these repeated events are ADDITIONAL to this step-count.\n"
		"MAXANGLE  (Mode 1) Motion between 3 lspkr pairs (e.g. 1-2, 2-3, 3-4)\n"
		"           Max angle of rotation of motion lies between 3rd pair (  90 to 135 ).\n"
		"          (lsprks 3-4 in the example)\n"
		"SKEW      (Mode 2) Motion between 3 lspkr pairs (e.g. 1-2, 2-3, 3-4)\n"
		"           Skew is ratio (0-1) of time spent between last pair & penultimate pair\n"
		"          (lspkr pairs 3-4 and 2-3, in the example)\n"
		"DEC        Loudness decimation on passing from one event to the next.\n"
		"FOC        Focal lpskr to or from which motion takes place (default, front centre).\n"
		"JITTER     Randomisation of event timings (0-1, default 0) Can vary over time.\n"
		"SLOW       Slow (or speed up) the pan-motion acceleration (default 0.5).\n"
		"           Smaller values make stream arrive more slowly at goal.\n"
		"           For approaching motion, 0.5 makes arrival not too fast.\n"
		"           For receding motion, 1.0, makes initial motion rapid.\n"
		"-r         Sounds RECEDE (default, sound approaches)\n"
		"-l         Motion to or from LEFT of focal lspkr (default: to or from right).\n");
	} else if(!strcmp(str,"twofiles")) {
		fprintf(stdout,
	    "USAGE: tangent twofiles\n"
	    " 1 inf1 inf2 outf dur steps maxangle dec bal [-ffoc] [-jjitter] [-sslow] [-r] [-l]\n"
	    " 2 inf1 inf2 outf dur steps skew     dec bal [-ffoc] [-jjitter]          [-r] [-l]\n"
		"\n"
		"Play two syncd mono sounds on tangent path to (or from) 8-channel, front-centred\n"
		"loudspeaker array. Lspkrs numbered clockwise, starting with \"1\" at front-centre.\n"
		"Output begins with 1st input sound, then gradually mixes in second input sound.\n"
		"If one sound is a filtered version of the other, suggesting greater distance,\n"
		"For an approaching sequence, put filtered sound as sound 1\n"
		"but for a receding sequence, put filtered sound as sound 2.\n"
		"\n"
		"MODE 1  For focus at 1, tangent starts along line at right-angles to lspkr 2.\n"
		"MODE 2  For focus at 1 tangent starts along line formed by lspkrs 2 and 3.\n"
		"\n"
		"DUR        Duration of output file.\n"
		"STEPS      Count of events in tangent stream (Modes 1-4 only).\n"
		"           Note that in ALL modes, in the part of the stream closest to the focus\n"
		"           SAME initial (receding) or final (approaching) event is repeated,\n"
		"           and these repeated events are ADDITIONAL to this step-count.\n"
		"MAXANGLE  (Mode 1) Motion between 3 lspkr pairs (e.g. 1-2, 2-3, 3-4)\n"
		"           Max angle of rotation of motion lies between 3rd pair (  90 to 135 ).\n"
		"          (lsprks 3-4 in the example)\n"
		"SKEW      (Mode 2) Motion between 3 lspkr pairs (e.g. 1-2, 2-3, 3-4)\n"
		"           Skew is ratio (0-1) of time spent between last pair & penultimate pair\n"
		"          (lspkr pairs 3-4 and 2-3, in the example)\n"
		"DEC        Loudness decimation on passing from one event to the next.\n"
		"BAL        Progressive accumulation of 2nd sound in mix.\n"
		"FOC        Focal lpskr to or from which motion takes place (default, front centre).\n"
		"JITTER     Randomisation of event timings (0-1, default 0) Can vary over time.\n"
		"SLOW       Slow (or speed up) the pan-motion acceleration (default 0.5).\n"
		"           Smaller values make stream arrive more slowly at goal.\n"
		"           For approaching motion, 0.5 makes arrival not too fast.\n"
		"           For receding motion, 1.0, makes initial motion rapid.\n"
		"-r         Sounds RECEDE (default, sound approaches)\n"
		"-l         Motion to or from LEFT of focal lspkr (default: to or from right).\n");
	} else if(!strcmp(str,"sequence")) {
		fprintf(stdout,
	    "USAGE: tangent sequence\n"
	    " 1 file1 file2 [file3..] outf dur maxangle dec [-ffoc] [-jjitter] [-sslow] [-r] [-l]\n"
	    " 2 file1 file2 [file3..] outf dur skew     dec [-ffoc] [-jjitter]          [-r] [-l]\n"
		"\n"
		"Play sequence of mono sounds on tangent path to (or from) 8-channel, front-centred\n"
		"loudspeaker array. Lspkrs numbered clockwise, starting with \"1\" at front-centre.\n"
		"\n"
		"MODE 1  For focus at 1, tangent starts along line at right-angles to lspkr 2.\n"
		"MODE 2  For focus at 1 tangent starts along line formed by lspkrs 2 and 3.\n"
		"\n"
		"DUR        Duration of output file.\n"
		"MAXANGLE  (Mode 1) Motion between 3 lspkr pairs (e.g. 1-2, 2-3, 3-4)\n"
		"           Max angle of rotation of motion lies between 3rd pair (  90 to 135 ).\n"
		"          (lsprks 3-4 in the example)\n"
		"SKEW      (Mode 2) Motion between 3 lspkr pairs (e.g. 1-2, 2-3, 3-4)\n"
		"           Skew is ratio (0-1) of time spent between last pair & penultimate pair\n"
		"          (lspkr pairs 3-4 and 2-3, in the example)\n"
		"DEC        Loudness decimation on passing from one event to the next.\n"
		"FOC        Focal lpskr to or from which motion takes place (default, front centre).\n"
		"JITTER     Randomisation of event timings (0-1, default 0) Can vary over time.\n"
		"SLOW       Slow (or speed up) the pan-motion acceleration (default 0.5).\n"
		"           Smaller values make stream arrive more slowly at goal.\n"
		"           For approaching motion, 0.5 makes arrival not too fast.\n"
		"           For receding motion, 1.0, makes initial motion rapid.\n"
		"-r         Sounds RECEDE (default, sound approaches)\n"
		"-l         Motion to or from LEFT of focal lspkr (default: to or from right).\n");
	} else if(!strcmp(str,"list")) {
		fprintf(stdout,
	    "USAGE: tangent list\n"
	    " 1 textfile  outf dur maxangle dec [-ffoc] [-jjitter] [-sslow] [-r] [-l]\n"
	    " 2 textfile  outf dur skew     dec [-ffoc] [-jjitter]          [-r] [-l]\n"
		"\n"
		"Play sequence of mono sounds (listed in the textfile) on tangent path to (or from)\n"
		"8-channel front-centred lspkr array, numbered clockwise, from \"1\" at front-centre.\n"
		"\n"
		"MODE 1  For focus at 1, tangent starts along line at right-angles to lspkr 2.\n"
		"MODE 2  For focus at 1 tangent starts along line formed by lspkrs 2 and 3.\n"
		"\n"
		"DUR        Duration of output file.\n"
		"MAXANGLE  (Mode 1) Motion between 3 lspkr pairs (e.g. 1-2, 2-3, 3-4)\n"
		"           Max angle of rotation of motion lies between 3rd pair (  90 to 135 ).\n"
		"          (lsprks 3-4 in the example)\n"
		"SKEW      (Mode 2) Motion between 3 lspkr pairs (e.g. 1-2, 2-3, 3-4)\n"
		"           Skew is ratio (0-1) of time spent between last pair & penultimate pair\n"
		"          (lspkr pairs 3-4 and 2-3, in the example)\n"
		"DEC        Loudness decimation on passing from one event to the next.\n"
		"FOC        Focal lpskr to or from which motion takes place (default, front centre).\n"
		"JITTER     Randomisation of event timings (0-1, default 0) Can vary over time.\n"
		"SLOW       Slow (or speed up) the pan-motion acceleration (default 0.5).\n"
		"           Smaller values make stream arrive more slowly at goal.\n"
		"           For approaching motion, 0.5 makes arrival not too fast.\n"
		"           For receding motion, 1.0, makes initial motion rapid.\n"
		"-r         Sounds RECEDE (default, sound approaches)\n"
		"-l         Motion to or from LEFT of focal lspkr (default: to or from right).\n");
	} else
		fprintf(stdout,"Unknown option '%s'\n",str);
	return(USAGE_ONLY);
}

int usage3(char *str1,char *str2)
{
	fprintf(stderr,"Insufficient parameters on command line.\n");
	return(USAGE_ONLY);
}

/******************************** TANGENT ********************************/

int tangent(dataptr dz) 
{
	int exit_status;
	char temp[400], temp2[400], temp3[400], temp4[400];
	int stepcnt;
	int lspkr = 1, rspkr = 2, n, m, swapped = 0;
	double dur = dz->param[TAN_DUR], thistime, thispos = 0.0, pos, leftgain = 0.0, rightgain = 0.0;
	double timestep, extstep, maxangle = 0.0, gain = 1.0, fbal = 1.0, thisgain, rightangle = PI/2.0, time, jitter;
	double *lens = dz->parray[0];
	
	thistime = 0.0;
	if(dz->process == TAN_SEQ || dz->process == TAN_LIST)
		stepcnt = dz->infilecnt;
	else
		stepcnt = dz->iparam[TAN_STEPS];
	timestep = dur/(double)(stepcnt + dz->EXTENSION);
	sprintf(temp3,"%s",dz->wordstor[0]);
	sprintf(temp,"8\n");
	if(fputs(temp,dz->fp) < 0) {
		sprintf(errstr,"Error writing to output data file\n");
		return(PROGRAM_ERROR);
	}
	if(dz->process == TAN_TWO)
		sprintf(temp4,"%s",dz->wordstor[1]);

	if(dz->mode==0)
		maxangle = dz->param[TAN_MANG] * PI/180.0;
	if(dz->vflag[RECEDE]) {		//	If receeding
		thispos = 0.0;								//	Start at closest approach

		//	Create PRE-extension to next lspkr at full level and equal steps

		if(dz->vflag[LEFT]) {
			lspkr = dz->iparam[TAN_FOCUS] + 1;
			if(lspkr > 8)
				lspkr = 1;
			rspkr = lspkr - 1;
			if(rspkr < 1)
				rspkr = 8;
		} else {
			lspkr = dz->iparam[TAN_FOCUS] - 1;
			if(lspkr < 1)
				lspkr = 8;
			rspkr = lspkr + 1;
			if(rspkr > 8)
				rspkr = 1;
		}
		extstep = 1.0/dz->EXTENSION;
		for(n = 0;n < dz->EXTENSION;n++) {
			time = thistime;
			if(time > 0.0) {
				if(dz->brksize[TAN_JITTER]) {
					if((exit_status = read_value_from_brktable(thistime,TAN_JITTER,dz))<0)
						return(exit_status);
				}
				if(dz->param[TAN_JITTER] > 0.0) {
					jitter = (drand48() * 2.0) - 1.0;
					jitter *= dz->param[TAN_JITTER];
					jitter *= timestep/2.0;
					time += jitter;
				}
			}
			pos = (thispos * 2.0) - 1.0;		//	Convert to -1 to 1 format	
			pancalc(pos,&leftgain,&rightgain);
			strcpy(temp,temp3);					//	NB Modes 4 & 5 retain initial sound event during extension
			strcat(temp," ");
			sprintf(temp2,"%lf",time);
			strcat(temp,temp2);
			strcat(temp," 1 1:");
			sprintf(temp2,"%d",lspkr);
			strcat(temp,temp2);
			strcat(temp," ");
			sprintf(temp2,"%lf",leftgain);
			strcat(temp,temp2);
			strcat(temp," 1:");
			sprintf(temp2,"%d",rspkr);
			strcat(temp,temp2);
			strcat(temp," ");
			sprintf(temp2,"%lf",rightgain);
			strcat(temp,temp2);
			strcat(temp,"\n");
			if(fputs(temp,dz->fp) < 0) {
				sprintf(errstr,"Error writing to output data file\n");
				return(PROGRAM_ERROR);
			}
			thistime += timestep;
			thispos += extstep;
		}

		//	Now do the tangent motion

		fbal = 1.0;
		gain = 1.0;
		thispos = 0.0;								//	Start at closest approach
		lspkr = dz->iparam[TAN_FOCUS];				//	on closest lspkrs					
		if(dz->vflag[LEFT]) {
			rspkr = lspkr - 1;
			if(rspkr < 1)
				rspkr = 8;
		} else {
			rspkr = lspkr + 1;
			if(rspkr > 8)
				rspkr = 1;
		}
		for(n=0;n<stepcnt;n++) {
			thispos += lens[n];						//	Position of next segment
			if(thispos > 1) {
				if(!swapped) {			//	If JUST Reached lspkrs 1
					lspkr = rspkr;						//	Reposition between lskprs 1 & 2
					if(dz->vflag[LEFT]) {
						rspkr = lspkr - 1;
						if(rspkr < 1)
							rspkr = 8;
					} else {
						rspkr = lspkr + 1;
						if(rspkr > 8)
							rspkr = 1;
					}
					swapped = 1;
				}
				pos = ((thispos - 1) * 2.0) - 1.0;	//	Convert to -1 to 1 format	
			} else
				pos = (thispos * 2.0) - 1.0;		//	Convert to -1 to 1 format	
			time = thistime;
			if(time > 0.0) {
				if(dz->brksize[TAN_JITTER]) {
					if((exit_status = read_value_from_brktable(thistime,TAN_JITTER,dz))<0)
						return(exit_status);
				}
				if(dz->param[TAN_JITTER] > 0.0) {
					jitter = (drand48() * 2.0) - 1.0;
					jitter *= dz->param[TAN_JITTER];
					jitter *= timestep/2.0;
					time += jitter;
				}
			}
			thisgain = gain;
			if(dz->process == TAN_TWO)
				thisgain *= fbal;
			pancalc(pos,&leftgain,&rightgain);
			if(dz->process == TAN_SEQ || dz->process == TAN_LIST)	//	For multifile processes, change filename for each event
				sprintf(temp3,"%s",dz->wordstor[n]);
			strcpy(temp,temp3);
			strcat(temp," ");
			sprintf(temp2,"%lf",time);
			strcat(temp,temp2);
			strcat(temp," 1 1:");
			sprintf(temp2,"%d",lspkr);
			strcat(temp,temp2);
			strcat(temp," ");
			sprintf(temp2,"%lf",leftgain * thisgain);
			strcat(temp,temp2);
			strcat(temp," 1:");
			sprintf(temp2,"%d",rspkr);
			strcat(temp,temp2);
			strcat(temp," ");
			sprintf(temp2,"%lf",rightgain * thisgain);
			strcat(temp,temp2);
			strcat(temp,"\n");
			if(fputs(temp,dz->fp) < 0) {
				sprintf(errstr,"Error writing to output data file\n");
				return(PROGRAM_ERROR);
			}
			if(dz->process == TAN_TWO) {
				thisgain = gain;
				thisgain *= max(0.0,1.0 - fbal);
				strcpy(temp,temp4);					//	Mix in filtered version of file
				strcat(temp," ");
				sprintf(temp2,"%lf",time);
				strcat(temp,temp2);
				strcat(temp," 1 1:");
				sprintf(temp2,"%d",lspkr);
				strcat(temp,temp2);
				strcat(temp," ");
				sprintf(temp2,"%lf",leftgain * thisgain);
				strcat(temp,temp2);
				strcat(temp," 1:");
				sprintf(temp2,"%d",rspkr);
				strcat(temp,temp2);
				strcat(temp," ");
				sprintf(temp2,"%lf",rightgain * thisgain);
				strcat(temp,temp2);
				strcat(temp,"\n");
				if(fputs(temp,dz->fp) < 0) {
					sprintf(errstr,"Error writing to output data file\n");
					return(PROGRAM_ERROR);
				}
			}
			thistime += timestep;				//	Time of next segment
			gain *= dz->param[TAN_DEC];
			if(dz->process == TAN_TWO)
				fbal *= dz->param[TAN_FBAL];
		}
	} else {				//	If approaching
		if(dz->mode==1)
			thispos = 2.0;
		else										//	Start at max distance
			thispos = (maxangle/rightangle) * 2;
		if(dz->vflag[LEFT]) {
			lspkr = dz->iparam[TAN_FOCUS] - 1;		//	on furthest lspkrs					
			if(lspkr < 1)
				lspkr = 8;
			rspkr = lspkr - 1;
			if(rspkr < 1)
				rspkr = 8;
		} else {
			lspkr = dz->iparam[TAN_FOCUS] + 1;		//	on furthest lspkrs					
			if(lspkr > 8)
				lspkr = 1;
			rspkr = lspkr + 1;
			if(rspkr > 8)
				rspkr = 1;
		}
		gain = pow(dz->param[TAN_DEC],(stepcnt - 1));
		dz->param[TAN_DEC] = 1.0/dz->param[TAN_DEC];
		if(dz->process == TAN_TWO) {
			fbal = pow(dz->param[TAN_FBAL],(stepcnt - 1)); 
			dz->param[TAN_FBAL] = 1.0/dz->param[TAN_FBAL];
		}
		for(n=0,m = stepcnt - 1;n<stepcnt;n++,m--) {
			if(thispos < 1) {
				if(!swapped) {						//	If JUST Reach lspkrs 1
					if(dz->vflag[LEFT]) {
						lspkr++;					//	Reposition between lskprs 1 & 2
						if(lspkr > 8)
							lspkr = 1;
						rspkr++;
						if(rspkr > 8)
							rspkr = 1;
					} else {
						lspkr--;					//	Reposition between lskprs 1 & 2
						if(lspkr < 1)
							lspkr = 8;
						rspkr--;
						if(rspkr < 1)
							rspkr = 8;
					}
					swapped = 1;
				}
				pos = (thispos * 2.0) - 1.0;		//	Convert to -1 to 1 format	
			} else
				pos = ((thispos - 1) * 2.0) - 1.0;	//	Convert to -1 to 1 format	
			time = thistime;
			if(time > 0.0) {
				if(dz->brksize[TAN_JITTER]) {
					if((exit_status = read_value_from_brktable(thistime,TAN_JITTER,dz))<0)
						return(exit_status);
				}
				if(dz->param[TAN_JITTER] > 0.0) {
					jitter = (drand48() * 2.0) - 1.0;
					jitter *= dz->param[TAN_JITTER];
					jitter *= timestep/2.0;
					time += jitter;
				}
			}
			thisgain = gain;
			if(dz->process == TAN_TWO)
				thisgain *= fbal;
			pancalc(pos,&leftgain,&rightgain);
			if(dz->process == TAN_SEQ || dz->process == TAN_LIST)		//	For multifile processes, change filename for each event
				sprintf(temp3,"%s",dz->wordstor[n]);
			strcpy(temp,temp3);
			strcat(temp," ");
			sprintf(temp2,"%lf",time);
			strcat(temp,temp2);
			strcat(temp," 1 1:");
			sprintf(temp2,"%d",lspkr);
			strcat(temp,temp2);
			strcat(temp," ");
			sprintf(temp2,"%lf",leftgain * gain);
			strcat(temp,temp2);
			strcat(temp," 1:");
			sprintf(temp2,"%d",rspkr);
			strcat(temp,temp2);
			strcat(temp," ");
			sprintf(temp2,"%lf",rightgain * gain);
			strcat(temp,temp2);
			strcat(temp,"\n");
			if(fputs(temp,dz->fp) < 0) {
				sprintf(errstr,"Error writing to output data file\n");
				return(PROGRAM_ERROR);
			}
			if(dz->process == TAN_TWO) {
				thisgain = gain;
				thisgain *= max(1.0 - fbal,0.0);
				strcpy(temp,temp4);					//	Mix in filtered version of file
				strcat(temp," ");
				sprintf(temp2,"%lf",time);
				strcat(temp,temp2);
				strcat(temp," 1 1:");
				sprintf(temp2,"%d",lspkr);
				strcat(temp,temp2);
				strcat(temp," ");
				sprintf(temp2,"%lf",leftgain * thisgain);
				strcat(temp,temp2);
				strcat(temp," 1:");
				sprintf(temp2,"%d",rspkr);
				strcat(temp,temp2);
				strcat(temp," ");
				sprintf(temp2,"%lf",rightgain * thisgain);
				strcat(temp,temp2);
				strcat(temp,"\n");
				if(fputs(temp,dz->fp) < 0) {
					sprintf(errstr,"Error writing to output data file\n");
					return(PROGRAM_ERROR);
				}
			}
			thispos -= lens[m];					//	Position of next segment
			thistime += timestep;				//	Time of next segment
			gain *= dz->param[TAN_DEC];
			if(dz->process == TAN_TWO)
				fbal *= dz->param[TAN_FBAL];
		}

		//	Create POST-extension to next lspkr at full level and equal steps

		if(dz->EXTENSION) {
			if(dz->vflag[LEFT]) {
				lspkr = dz->iparam[TAN_FOCUS];
				rspkr = lspkr + 1;
				if(rspkr > 8)
					rspkr = 1;
			} else {
				lspkr = dz->iparam[TAN_FOCUS];
				rspkr = lspkr - 1;
				if(rspkr < 1)
					rspkr = 8;
			}
			extstep = 1.0/dz->EXTENSION;
			thispos = extstep;
			for(n = 0;n < dz->EXTENSION;n++) {
				time = thistime;
				if(time > 0.0) {
					if(dz->brksize[TAN_JITTER]) {
						if((exit_status = read_value_from_brktable(thistime,TAN_JITTER,dz))<0)
							return(exit_status);
					}
					if(dz->param[TAN_JITTER] > 0.0) {
						jitter = (drand48() * 2.0) - 1.0;
						jitter *= dz->param[TAN_JITTER];
						jitter *= timestep/2.0;
						time += jitter;
					}
				}
				pos = (thispos * 2.0) - 1.0;		//	Convert to -1 to 1 format	
				pancalc(pos,&leftgain,&rightgain);
				strcpy(temp,temp3);					//	NB Modes 4 & 5 retain final sound event during extension
				strcat(temp," ");
				sprintf(temp2,"%lf",time);
				strcat(temp,temp2);
				strcat(temp," 1 1:");
				sprintf(temp2,"%d",lspkr);
				strcat(temp,temp2);
				strcat(temp," ");
				sprintf(temp2,"%lf",leftgain);
				strcat(temp,temp2);
				strcat(temp," 1:");
				sprintf(temp2,"%d",rspkr);
				strcat(temp,temp2);
				strcat(temp," ");
				sprintf(temp2,"%lf",rightgain);
				strcat(temp,temp2);
				strcat(temp,"\n");
				if(fputs(temp,dz->fp) < 0) {
					sprintf(errstr,"Error writing to output data file\n");
					return(PROGRAM_ERROR);
				}
				thistime += timestep;
				thispos = min(thispos + extstep, 1.0);
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
		sprintf(errstr,"Cannot open file %s to read sonudfile names.\n",filename);
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

