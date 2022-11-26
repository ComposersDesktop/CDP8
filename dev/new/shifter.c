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
 *	2 MODES :	mode 0: 1 infile
 *				mode 1: 1 infile per cycle	
 *				Program sorts cycles into ascending order, and assumes cycles (say 11,12,13) assocd with infiles 0,1,2
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
#define dupl descriptor_samps

#ifdef unix
#define round(x) lround((x))
#endif

char errstr[2400];

int anal_infiles = 1;
int	sloom = 0;
int sloombatch = 0;

const char* cdp_version = "7.1.0";

//CDP LIB REPLACEMENTS
static int check_shifter_param_validity_and_consistency(dataptr dz);
static int setup_shifter_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_shifter_param_ranges_and_defaults(dataptr dz);
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
static int shifter(dataptr dz);
static int create_shifter_sndbufs(dataptr dz);
static int handle_the_special_data(char *str,dataptr dz);
static void gen_times_array(int a, int b, int *itimes, int cycledur, dataptr dz);
static void gen_regular_times_array(int a, int *itimes, double cycledur,dataptr dz);
static int get_max_overlap(int maxcycle,int maxinfiledur,int * cycle,dataptr dz);
static void pancalc(double position,double *leftgain,double *rightgain);
static void rndintperm(int *perm,int cnt);
static int shifter_param_preprocess(dataptr dz);

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
		dz->maxmode = 2;
		if((exit_status = get_the_mode_from_cmdline(cmdline[0],dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(exit_status);
		}
		cmdline++;
		cmdlinecnt--;
		// setup_particular_application =
		if((exit_status = setup_shifter_application(dz))<0) {
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
	if((exit_status = setup_shifter_param_ranges_and_defaults(dz))<0) {
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

	if(dz->infilecnt > 1) {
		if((exit_status = handle_extra_infiles(&cmdline,&cmdlinecnt,dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);		
			return(FAILED);
		}
	}
	// handle_outfile() = 
	if((exit_status = handle_the_outfile(&cmdlinecnt,&cmdline,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = handle_the_special_data(cmdline[0],dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	cmdlinecnt--;
	cmdline++;

//	handle_formants()			redundant
//	handle_formant_quiksearch()	redundant
//	handle_special_data()		redundant
 
	if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {		// CDP LIB
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
//	check_param_validity_and_consistency....
	if((exit_status = check_shifter_param_validity_and_consistency(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = open_the_outfile(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	is_launched = TRUE;
	if((exit_status = create_shifter_sndbufs(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = shifter_param_preprocess(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	//spec_process_file =
	if((exit_status = shifter(dz))<0) {
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

/************************* SETUP_SHIFTER_APPLICATION *******************/

int setup_shifter_application(dataptr dz)
{
	int exit_status;
	aplptr ap;
	if((exit_status = establish_application(dz))<0)		// GLOBAL
		return(FAILED);
	ap = dz->application;
	// SEE parstruct FOR EXPLANATION of next 2 functions
	if((exit_status = set_param_data(ap,SHFCYCLES,7,7,"ddiiiid"))<0)
		return(FAILED);
	if((exit_status = set_vflgs(ap,"",0,"","zrl",3,0,"000"))<0)
		return(FAILED);
	// set_legal_infile_structure -->
	dz->has_otherfile = FALSE;
	// assign_process_logic -->
	if(dz->mode == 0)
		dz->input_data_type = SNDFILES_ONLY;
	else
		dz->input_data_type = MANY_SNDFILES;
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

/************************* SETUP_SHIFTER_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_shifter_param_ranges_and_defaults(dataptr dz)
{
	int exit_status;
	aplptr ap = dz->application;
	// set_param_ranges()
	ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
	// NB total_input_param_cnt is > 0 !!!
	if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
		return(FAILED);
	// get_param_ranges()

	ap->lo[SHF_CYCDUR]	= 0.01;
	ap->hi[SHF_CYCDUR]	= 3600.0;
	ap->default_val[SHF_CYCDUR]	= 2;
	ap->lo[SHF_OUTDUR]	= 0.0;
	ap->hi[SHF_OUTDUR]	= 32767.0;
	ap->default_val[SHF_OUTDUR]	= 8;
	ap->lo[SHF_OCHANS]	= 1;
	ap->hi[SHF_OCHANS]	= 16;
	ap->default_val[SHF_OCHANS]	= 2;
	ap->lo[SHF_SUBDIV]	= 3;
	ap->hi[SHF_SUBDIV]	= 64;
	ap->default_val[SHF_SUBDIV]	= 16;
	ap->lo[SHF_LINGER]	= 0.0;
	ap->hi[SHF_LINGER]	= 32767;
	ap->default_val[SHF_LINGER] = 2;
	ap->lo[SHF_TRNSIT]	= 0.0;
	ap->hi[SHF_TRNSIT]	= 32767;
	ap->default_val[SHF_TRNSIT] = 2;
	ap->lo[SHF_LBOOST]	= 0.0;
	ap->hi[SHF_LBOOST]	= 10.0;
	ap->default_val[SHF_LBOOST] = 1.0;
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
			if((exit_status = setup_shifter_application(dz))<0)
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
	usage2("shifter");
	return(USAGE_ONLY);
}

/**************************** CHECK_SHIFTER_PARAM_VALIDITY_AND_CONSISTENCY *****************************/

int check_shifter_param_validity_and_consistency(dataptr dz)
{
	double srate = (double)dz->infile->srate;
	int k, j;
	
	if(dz->param[SHF_CYCDUR] > dz->param[SHF_OUTDUR]) {
		sprintf(errstr,"Cycle duration is less than output duration.\n");
		return(DATA_ERROR);
	}
	if(dz->iparam[SHF_LINGER] + dz->iparam[SHF_TRNSIT] < 1.0) {
		sprintf(errstr,"Linger and Transit parameters must add to >= 1.\n");
		return(DATA_ERROR);
	}
	dz->iparam[SHF_OUTDUR] = (int)round(dz->param[SHF_OUTDUR] * srate);
	dz->iparam[SHF_CYCDUR] = (int)round(dz->param[SHF_CYCDUR] * srate);

	if(dz->vflag[SHF_ZIG] && dz->vflag[SHF_RND]) {
		sprintf(errstr,"Zigzag and Random flags cannot be used in combination.\n");
		return(DATA_ERROR);
	}
	k = dz->iparam[SHF_SUBDIV]/2;
	j = dz->iparam[SHF_SUBDIV]/3;
	if((2 * k != dz->iparam[SHF_SUBDIV]) && (3 * j != dz->iparam[SHF_SUBDIV])) {
		sprintf(errstr,"Neat subdivision is not a multiple of 2 or 3.\n");
		return(DATA_ERROR);
	}
	dz->param[SHF_SUBDIV] = (double)dz->iparam[SHF_SUBDIV];

	return FINISHED;
}

/******************************** DBTOLEVEL ***********************/

double dbtolevel(double val)
{
	int isneg = 0;
	if(flteq(val,0.0))
		return(1.0);
	if(val < 0.0) {
		val = -val;
		isneg = 1;
	}
	val /= 20.0;
	val = pow(10.0,val);
	if(isneg)
		val = 1.0/val;
	return(val);
}	

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	if(!strcmp(prog_identifier_from_cmdline,"shifter"))				dz->process = SHIFTER;
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
	if(!strcmp(str,"shifter")) {
		fprintf(stdout,
	    "USAGE: shifter shifter \n"
	    "1 inf out cycles cycdur dur ochans subdiv linger transit boost [-z|-r] [-l]\n"
	    "2 inf1 inf2 [inf3 ....] cycles cycdur dur ochans subdiv linger transit boost\n"
		"          [-z|-r] [-l]\n"
		"\n"
		"Generate simultaneous repetition cycles, shifting focus from one to another.\n"
		"\n"
		"Mode 1: Uses the same input sound for all cycles.\n"
		"Mode 2: In this mode the number of input files must equal the number of cycles.\n"
		"        Assigns the input files, in order, to the cycles, in order.\n"
		"CYCLES  Textfile listing the number of beats in each cycle.\n"
		"CYCDUR  Duration of 1 complete cycle.\n"
		"DUR     Required duration of the output sound.\n"
		"OCHANS  Number of channels in output.\n"
		"SUBDIV  Minimum division of the beat: > 4 and a multiple of 2 and or 3\n"
		"LINGER  Number of cycles to remain in a fixed focus.\n"
		"TRANSIT Number of cycles to make transition to next focus.\n"
		"        The sum of LINGER  & TRANSIT must be >= 1.\n"
		"BOOST   With standard stream level \"L\", add \"boost * L\" to focus stream level.\n"
		"\n"
		"        Normal procedure is to move focus through the list of cycles.\n"
		"        e.g. with cycles 11,12,13 focus moves 11,12,13,11,12,13,etc\n"
		"\n"
		"-z      Flag causes focus to ZIGZAG through the cycles.\n"
		"        e.g. with cycles 11,12,13 focus moves 11,12,13,12,11,12,13,12,etc\n"
		"\n"
		"-r      Flag causes focus to select a RANDOM order of the cycles\n"
		"        e.g. 12,11,13, move through those, then select another random order etc.\n"
		"\n"
		"        If number of output channels > 2, lspkr array assumed sound-surround.\n"
		"-l      Flag changes to a linear array, with a leftmost and rightmost lspkr.\n"
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

/******************************** SHIFTER ********************************
 *
 * INTEGER ARRAYS					lmost			  LONG ARRAYS							DOUBLE ARRAYS
 *		  cycles				    | rmost				| thisreltime
 *		  |	time-arrays				| | perm			sndptrs		nextreltime				llev
 *		  |	|						| |	| | foc			| |      	|		  abstime		| rlev
 *		  |	|-----------------------| | | |	| 			| |---------|---------|---------|	| | |
 *		  |	|						| | | |	|			| |	cycleno	| cycleno | cycleno |	| | |
 *	array |	|						| | | |	|			| |	 arrays	|  arrays |  arrays |	| | |
 *	  no  0	1 to ringsize			ringsize+1			0 1			|		  |			|	0 1 |
 *		  |	|						| ringsize+2		| |			|		  |			|	| | |
 *		  | |						| | ringsize+3		| |			|		  |			|	| | |
 *		  |	|						| | | ringsize+4	| |			|		  |			|	| | |
 *		  |	|						| | | |	|			| |			|		  |			|	| | |
 *		  |	|						cycleno	|			| |			|		  |			|	| | |
 *		  cycleno					| cycleno			cycleno*dupl|		  |			|	cycleno
 *	size  |	maxcycle				| | cycleno			| maxcycle	maxcycle  maxcycle	|	| cycleno
 *		  |	|						| | | maxcycle		| |			|		  |			|	| | |
 *		  |	|						| | | | gpeventcnt	| |			|		  |			|	| | |
 *
 *	A CYCLE is a compete set of repetitions.
 *	AN EVENT is (the start of) a single src-reading
 */

#define ibuf dz->sampbuf

int shifter(dataptr dz)
{
	int exit_status, zig, permcnt, focus = 0, next_focus, transcnt = 0, ptrcnt, finished, transbase, chans = dz->iparam[SHF_OCHANS];
	int *cycle = dz->iparray[0], **reltime = dz->iparray+1, *perm = dz->iparray[dz->ringsize+3], *foc = dz->iparray[dz->ringsize+4];
	int *lmost = dz->iparray[dz->ringsize+1], *rmost= dz->iparray[dz->ringsize+2], *fflag = dz->iparray[dz->ringsize+5];
	int **thisreltime = dz->lparray + 1, **nextreltime = dz->lparray + 1 + dz->itemcnt, **abstime = dz->lparray + 1 + (dz->itemcnt*2);
	int *ptr = dz->lparray[0];
	int outcnt, opos, cycgrpcntr, cyccntr, maxcycle, cycgrpsize, n, m, k, mm, nn, gpcnt, timediff, samps_read, transbas = 0, transdur;
	float *obuf = dz->sampbuf[dz->infilecnt];
	double *llev = dz->parray[0], *rlev = dz->parray[1];
	double normaliser, maxsamp = 0.0, trans_position, transfrac, boost = 1.0, srate = (double)dz->infile->srate;
	
	for(n=0;n<dz->infilecnt;n++) {
		if((samps_read  = fgetfbufEx(dz->sampbuf[n], dz->insams[n],dz->ifd[n],0))<0) {
			sprintf(errstr,"Sound read error with input soundfile %d: %s\n",n+1,sferrstr());
			return(SYSTEM_ERROR);
		}
	}	
	zig = 1;							//	zigzag starts in zig direction
	permcnt = 0;						//	counter for permutations of cycle focus.
	ptrcnt  = dz->itemcnt * dz->dupl;	//	All pointers available to read infile(s)
	for(n = 0; n < ptrcnt; n++)
		ptr[n] = -1;	//	Mark all sound pointers as off

	maxcycle = 0;
	for(n = 0;n < dz->itemcnt;n++)
		maxcycle = max(maxcycle,cycle[n]);
	for(n = 0;n < dz->infilecnt;n++)
		ibuf[n] = dz->sampbuf[n];

	// Cycle-group is a whole set of LINGER+TRANSIT cycles

	cycgrpsize = (dz->iparam[SHF_LINGER] + dz->iparam[SHF_TRNSIT]) * dz->iparam[SHF_CYCDUR];

	opos = 0;
	cycgrpcntr = 0;					//	This counts samples through a cycle-group
	gpcnt = 0;						//	This counts the cycle-groups
	cyccntr = 0;					//	This counts samples through a cycle
	outcnt = 0;						//	This counts the output group-samples
	transdur = dz->iparam[SHF_TRNSIT] * dz->iparam[SHF_CYCDUR];
	if(dz->vflag[SHF_RND]) {
		rndintperm(perm,dz->itemcnt);
		next_focus = perm[0];
	} else
		next_focus  = 0;

	fprintf(stdout,"INFO: First pass: assessing level.\n");
	fflush(stdout);
	memset((char *)obuf,0,dz->buflen * sizeof(float));

	while(outcnt < dz->iparam[SHF_OUTDUR]) {

		//	IF AT START OF NEW CYCLE-GROUP

		if(cycgrpcntr % cycgrpsize == 0) {
			cycgrpcntr = 0;

			//	SET UP NEW LINGER-TRANSIT COUNTER
			
			transcnt = -dz->iparam[SHF_LINGER];
			transbas = outcnt + (dz->iparam[SHF_LINGER] * dz->iparam[SHF_CYCDUR]);

			//	DECIDE WHICH CYCLE IS THE FOCUS (AND WHICH THE NEXT)

			if(dz->vflag[SHF_ZIG]) {
				focus = next_focus;
				if(zig) {
					next_focus = focus + 1;
					if(next_focus >= dz->itemcnt) {
						zig = !zig;
						next_focus -= 2;
					}
				} else {
					next_focus = focus - 1;
					if(next_focus < 0) {
						zig = !zig;
						next_focus = 1;
					}
				}
			} else if(dz->vflag[SHF_RND]) {
				if(permcnt == dz->itemcnt-1) {
					focus = perm[permcnt-1];
					do {
						rndintperm(perm,dz->itemcnt);
					} while(perm[0] != focus);
					next_focus = perm[0];
				} else {
					if(permcnt == dz->itemcnt)
						permcnt = 0;
					focus = perm[permcnt];
					next_focus = perm[permcnt+1];
				}
				permcnt++;
			} else {
				focus = next_focus;
				next_focus = (focus+1) % dz->itemcnt;
			}
			foc[gpcnt] = focus;
			foc[gpcnt+1] = next_focus;
			
			//	GET THE ABSOLUTE TIMINGS OF THE EVENTS IN EACH CYCLE
																		//	With 3 cycles	00, 01, 02, 10, 11, 12, 20, 21, 22, AND if the focus is the 2nd cycle (1)
			for(n = 0,m = focus; n < dz->itemcnt;n++,m+=dz->itemcnt) {	//	we want             x            x           x		(as focus is 2nd of each pair)
				for(k = 0; k < maxcycle;k++)							//	which is           focus       focus		focus
					thisreltime[n][k] = reltime[m][k];					//						(m)		  +itemcnt	  +itemcnt*2
			}
			
			//	IF THERE'S ANY TRANSIT
			
			//	GET WHAT ABSOLUTE TIMINGS WOULD BE IF NEXT EVENT WERE THE FOCUS

			if(dz->iparam[SHF_TRNSIT] > 0) {							//	With 3 cycles	00, 01, 02, 10, 11, 12, 20, 21, 22, if next focus is 3rd cycle (2)
				for(n = 0,m = next_focus; n < dz->itemcnt;n++,m+=dz->itemcnt) {//	we want			x            x           x		(as focus is 2nd of each pair)	
					for(k = 0; k < maxcycle;k++)						//	which is			   focus       focus	    focus
						nextreltime[n][k] = reltime[m][k];				//						    (m)	      +itemcnt	  +itemcnt*2
				}
			}

			//	INITIALLY, USE THE focus EVENT TIMINGS AS THE TRUE TIMINGS (MAY BE MODIFIED LATER)
			
			for(n = 0;n < dz->itemcnt; n++) {							//	THIS IS REDUNDANT , as done at cycle start
				for(k = 0; k < maxcycle;k++)
					abstime[n][k] = thisreltime[n][k] + outcnt;
			}
			cyccntr = 0;
			gpcnt++;
		}
		
		//	IF WE'RE STARTING A NEW CYCLE

		if(cyccntr % dz->iparam[SHF_CYCDUR] == 0) {

			if(dz->iparam[SHF_TRNSIT] > 0) {

				//	IF WE'RE IN A TRANSITION (rather than being in a linger, where transcnt IS -ve), GET TRUE TIMINGS WITHIN CYCLE

				if(transcnt >= 0) {
					transbase = transcnt;							//	Which cycle are we in, within the transition
					for(n = 0;n < dz->itemcnt; n++) {
						for(k = 0; k < cycle[n];k++) {				//	For each event within the cycle									
							trans_position = (double)k/(double)cycle[n];										//	How far into the cycle are we
							transfrac = (double)(trans_position + transbase)/(double)dz->iparam[SHF_TRNSIT];	//  What fraction of this transition is this event at
							timediff  = nextreltime[n][k] - thisreltime[n][k];									//	What is timediff between positions at start and end of transition
							timediff  = (int)round((double)timediff * transfrac);								//	Relative adjustment at this point in transition
							abstime[n][k] = thisreltime[n][k] + timediff + outcnt;								//	True time value at this point in transition
						}
					}
				} else {
					for(n = 0; n < dz->itemcnt;n++) {
						for(k = 0; k < maxcycle;k++)
							abstime[n][k] = thisreltime[n][k] + outcnt;
					}
				}
				transcnt++;
			} else {
				for(n = 0; n < dz->itemcnt;n++) {
					for(k = 0; k < maxcycle;k++)
						abstime[n][k] = thisreltime[n][k] + outcnt;
				}
			}
		}
		
		//	CHECK EACH CYCLE TO SEE IF WE'RE AT TIME TO INITIATE NEW EVENT

		for(n = 0;n<dz->itemcnt;n++) {
			for(k=0;k < cycle[n];k++) {
				if(abstime[n][k] == outcnt)	{
				//	AND IF SO, GET APPROPRIATE POINTERS TO INPUT FILE(S) FOR THIS CYCLE
					for(m=n; m < ptrcnt; m+= dz->itemcnt) {	//	Find an available buffer-pointer assocd with that cycle
						if(dz->mode == 1)
							mm = n;							//	Associate the ptr with the correct file
						else
							mm = 0;
						if(ptr[m] < 0 || ptr[m] >= dz->insams[mm]) {	//	If pointer is available
							ptr[m] = 0;									//	set it to start counting in an infile
							if(n == focus)
								fflag[m] = 1;
							else if(n == next_focus)
								fflag[m] = 2;
							else
								fflag[m] = 0;
							break;
						}
					}
				} else if(abstime[n][k] > outcnt)			//	Don't search beyond "now"
					break;
			}
		}

		//	FINALLY, WRITE TO NEXT SAMPLE IN OUTBUFFER FROM ALL ACTIVE POINTERS

		trans_position = outcnt - transbas;
		transfrac = (double)trans_position/(double)transdur;//  What fraction of this transition is this event at (only valud once we're IN the transition

		for(n = 0;n<ptrcnt;n++) {							//	For every pointer...
			nn = n % dz->itemcnt;							//	Associate the ptr with the correct cycle+spatial-pos
			if(dz->mode == 1)
				mm = nn;									//	Associate the ptr with the correct file
			else
				mm = 0;
			if(ptr[n] >= 0 && ptr[n] < dz->insams[mm]) {	//	If it is active
				boost = 1.0;
				if(dz->param[SHF_LBOOST] > 0) {				//	If there's a level boost to the focus stream
					if(trans_position < 0) {				//	Before transit starts, boost level of focus stream pointers (only)
							if(fflag[n] == 1) 
								boost = 1.0 + dz->param[SHF_LBOOST];
					} else {								//	Once we're in a transition
						if(fflag[n] == 1)					//	If this is a focus stream, decrement level
							boost = 1.0 + ((1.0 - transfrac) * dz->param[SHF_LBOOST]);
						else if(fflag[n] == 2)				//	If this is a next-focus stream, increment level
							boost = 1.0 + (transfrac * dz->param[SHF_LBOOST]);
					}
				}
				obuf[opos + lmost[nn]] = (float)(obuf[opos + lmost[nn]] + (ibuf[mm][ptr[n]] * llev[nn] * boost));
				obuf[opos + rmost[nn]] = (float)(obuf[opos + rmost[nn]] + (ibuf[mm][ptr[n]] * rlev[nn] * boost));
					//		position-nn										pointer[n]			levels[nn]
					//	assocd with output cycle				   	  is associated with	are associated with
					//	and corresponds to chans					       sound mm			  spatial pos
					//	   in output buffer							   which it is reading	 of output cycle
			}
			if(ptr[n] >= 0)									//	Advance all switched-on ptrs, whether are not they are currently reading a file.
				ptr[n]++;									//	Once they are beyond file end, they become available to select for next read.
		}

		//	ADVANCE IN OUTBUF, AND TEST LEVEL OUTPUT IF BUFFER FULL
		
		opos += chans;
		if(opos >= dz->buflen) {
			fprintf(stdout,"INFO: Level Check at %lf secs\n",(double)outcnt/srate);
			fflush(stdout);
			for(n = 0;n < dz->buflen;n++)
				maxsamp = max(maxsamp,fabs(obuf[n]));
			memset((char *)obuf,0,dz->buflen * sizeof(float));
			opos = 0;
		}
		cycgrpcntr++;		//	Counter notes where we are in a cycle-group
		cyccntr++;			//	Counter notes where we are in a cycle
		outcnt++;			//	Counter notes where we are in the output
	}

	//	COMPLETE ALL ACTIVE WRITES

	finished = 0;
	while(!finished) {
		finished = 1;
		for(n = 0;n<ptrcnt;n++) {							//	For every pointer...
			if(dz->mode == 1)
				mm = n % dz->infilecnt;
			else
				mm = 0;
			if(ptr[n] > 0 && ptr[n] < dz->insams[mm]) {		//	If it is active
				finished = 0;								//	Flag not yet finished
				break;
			}
		}
		if(finished)
			break;

		trans_position = outcnt - transbas;
		transfrac = (double)trans_position/(double)transdur;	//  What fraction of this transition is this event at

		for(n = 0;n<ptrcnt;n++) {
			nn = n % dz->itemcnt;
			if(dz->mode == 1)
				mm = nn;
			else
				mm = 0;
			if(ptr[n] > 0 && ptr[n] < dz->insams[mm]) {
				boost = 1.0;
				if(dz->param[SHF_LBOOST] > 0) {				//	If there's a level boost to the focus stream
					if(trans_position < 0) {				//	Before transit starts, boost level of focus stream pointers (only)
							if(fflag[n] == 1) 
								boost = 1.0 + dz->param[SHF_LBOOST];
					} else {
						if(fflag[n] == 1) 
							boost = 1.0 + ((1.0 - transfrac) * dz->param[SHF_LBOOST]);
						else if(fflag[n] == 2) 
							boost = 1.0 + (transfrac * dz->param[SHF_LBOOST]);
					}
				}
				obuf[opos + lmost[nn]] = (float)(obuf[opos + lmost[nn]] + (ibuf[mm][ptr[n]] * llev[nn] * boost));
				obuf[opos + rmost[nn]] = (float)(obuf[opos + rmost[nn]] + (ibuf[mm][ptr[n]] * rlev[nn] * boost));
			}
			if(ptr[n] > 0)
				ptr[n]++;
		}
		opos += chans;
		if(opos >= dz->buflen) {
			for(n = 0;n < dz->buflen;n++)
				maxsamp = max(maxsamp,fabs(obuf[n]));
			memset((char *)obuf,0,dz->buflen * sizeof(float));
			opos = 0;
		}
		outcnt++;
	}
	if(opos) {
		for(n = 0;n < opos;n++)
			maxsamp = max(maxsamp,fabs(obuf[n]));
	}

	dz->tempsize = outcnt * chans;	/* For sloom progress bar */
	normaliser = 1.0;
	if(maxsamp > 0.95) {
		normaliser = 0.95/maxsamp;
		fprintf(stdout,"INFO: Normalising by %lf secs\n",(double)normaliser);
		fflush(stdout);
	}	
	opos = 0;
	cycgrpcntr = 0;
	cyccntr = 0;
	gpcnt = 0;
	outcnt = 0;
	fprintf(stdout,"INFO: Second pass: creating output.\n");
	fflush(stdout);
	memset((char *)obuf,0,dz->buflen * sizeof(float));

	while(outcnt < dz->iparam[SHF_OUTDUR]) {

		//	IF AT START OF NEW CYCLE-GROUP

		if(cycgrpcntr % cycgrpsize == 0) {
			cycgrpcntr = 0;

			//	SET UP NEW LINGER-TRANSIT COUNTER
			
			transcnt = -dz->iparam[SHF_LINGER];
			transbas = outcnt + (dz->iparam[SHF_LINGER] * dz->iparam[SHF_CYCDUR]);

			//	DECIDE WHICH CYCLE IS THE FOCUS (AND WHICH THE NEXT)

			focus = foc[gpcnt];
			next_focus = foc[gpcnt+1];
			
			//	GET THE ABSOLUTE TIMINGS OF THE EVENTS IN EACH CYCLE
																		//	With 3 cycles	00, 01, 02, 10, 11, 12, 20, 21, 22, AND if the focus is the 2nd cycle (1)
			for(n = 0,m = focus; n < dz->itemcnt;n++,m+=dz->itemcnt) {	//	we want             x            x           x		(as focus is 2nd of each pair)
				for(k = 0; k < maxcycle;k++)							//	which is           focus       focus		focus
					thisreltime[n][k] = reltime[m][k];					//						(m)		  +itemcnt	  +itemcnt*2
			}
			
			//	IF THERE'S ANY TRANSIT
			
			//	GET WHAT ABSOLUTE TIMINGS WOULD BE IF NEXT EVENT WERE THE FOCUS

			if(dz->iparam[SHF_TRNSIT] > 0) {							//	With 3 cycles	00, 01, 02, 10, 11, 12, 20, 21, 22, if next focus is 3rd cycle (2)
				for(n = 0,m = next_focus; n < dz->itemcnt;n++,m+=dz->itemcnt) {//	we want			x            x           x		(as focus is 2nd of each pair)	
					for(k = 0; k < maxcycle;k++)						//	which is			   focus       focus	    focus
						nextreltime[n][k] = reltime[m][k];				//						    (m)	      +itemcnt	  +itemcnt*2
				}
			}

			//	INITIALLY, USE THE focus EVENT TIMINGS AS THE TRUE TIMINGS (MAY BE MODIFIED LATER)
			
			for(n = 0;n < dz->itemcnt; n++) {
				for(k = 0; k < maxcycle;k++)
					abstime[n][k] = thisreltime[n][k] + outcnt;
			}
			cyccntr = 0;
			gpcnt++;
		}
		
		//	IF WE'RE STARTING A NEW CYCLE

		if(cyccntr % dz->iparam[SHF_CYCDUR] == 0) {

			if(dz->iparam[SHF_TRNSIT] > 0) {

				//	IF WE'RE IN A TRANSITION (rather than being in a linger, where transcnt IS -ve), ADJUST TIMINGS WITHIN FIRST CYCLE

				if(transcnt >= 0) {
					transbase = transcnt;							//	Which cycle are we in, within the transition
					for(n = 0;n < dz->itemcnt; n++) {
						for(k = 0; k < cycle[n];k++) {				//	For each event within the cycle									
							trans_position = ((double)k/(double)cycle[n]);										//	How far into the cycle are we
							transfrac = (double)(trans_position + transbase)/(double)dz->iparam[SHF_TRNSIT];	//  What fraction of this transition is this event at
							timediff  = nextreltime[n][k] - thisreltime[n][k];									//	What is timediff between positions at start and end of transition
							timediff  = (int)round((double)timediff * transfrac);								//	Relative adjustment at this point in transition
							abstime[n][k] = thisreltime[n][k] + timediff + outcnt;								//	True time value at this point in transition
						}
					}
				} else {
					for(n = 0; n < dz->itemcnt;n++) {
						for(k = 0; k < maxcycle;k++)
							abstime[n][k] = thisreltime[n][k] + outcnt;
					}
				}
				transcnt++;
			} else {
				for(n = 0; n < dz->itemcnt;n++) {
					for(k = 0; k < maxcycle;k++)
						abstime[n][k] = thisreltime[n][k] + outcnt;
				}
			}
		}
		
		//	CHECK EACH CYCLE TO SEE IF WE'RE AT TIME TO INITIATE NEW EVENT

		for(n = 0;n<dz->itemcnt;n++) {
			for(k=0;k < cycle[n];k++) {
				if(abstime[n][k] == outcnt)	{
				//	AND IF SO, GET APPROPRIATE POINTERS TO INPUT FILE(S) FOR THIS CYCLE
					for(m=n; m < ptrcnt; m+= dz->itemcnt) {	//	Find an available buffer-pointer assocd with that cycle
						if(dz->mode == 1)
							mm = n;							//	Associate the ptr with the correct file
						else
							mm = 0;
						if(ptr[m] < 0 || ptr[m] >= dz->insams[mm]) {	//	If pointer is available
							ptr[m] = 0;									//	set it to start counting in an infile
							if(n==focus)
								fflag[m] = 1;
							else if(n==next_focus)
								fflag[m] = 2;
							else
								fflag[m] = 0;
							break;
						}
					}
				} else if(abstime[n][k] > outcnt)
					break;
			}
		}

		//	FINALLY, WRITE TO NEXT SAMPLE IN OUTBUFFER FROM ALL ACTIVE POINTERS

		trans_position = outcnt - transbas;
		transfrac = (double)trans_position/(double)transdur;	//  What fraction of this transition is this event at

		for(n = 0;n<ptrcnt;n++) {							//	For every pointer...
			nn = n % dz->itemcnt;							//	Associate the ptr with the correct cycle/pos
			if(dz->mode == 1)
				mm = n % dz->itemcnt;						//	Associate the ptr with the correct file
			else
				mm = 0;
			if(ptr[n] >= 0 && ptr[n] < dz->insams[mm]) {	//	If it is active
				boost = 1.0;
				if(dz->param[SHF_LBOOST] > 0) {				//	If there's a level boost to the focus stream
					if(trans_position < 0) {				//	Before transit starts, boost level of focus stream pointers (only)
							if(fflag[n] == 1) 
								boost = 1.0 + dz->param[SHF_LBOOST];
					} else {
						if(fflag[n] == 1) 
							boost = 1.0 + ((1.0 - transfrac) * dz->param[SHF_LBOOST]);
						else if(fflag[n] == 2) 
							boost = 1.0 + (transfrac * dz->param[SHF_LBOOST]);
					}
				}
				obuf[opos + lmost[nn]] = (float)(obuf[opos + lmost[nn]] + (ibuf[mm][ptr[n]] * llev[nn] * boost));
				obuf[opos + rmost[nn]] = (float)(obuf[opos + rmost[nn]] + (ibuf[mm][ptr[n]] * rlev[nn] * boost));
					//		position-nn										pointer[n]			levels[nn]
					//	assocd with output cycle				   	  is associated with	are associated with
					//	and corresponds to chans					       sound mm			  spatial pos
					//	   in output buffer							   which it is reading	 of output cycle
			}
			if(ptr[n] >= 0)									//	Advance all switched-on ptrs, whether are not they are currently reading a file.
				(ptr[n])++;									//	Once they are beyond file end, they become available to select for next read.
		}
		opos += chans;
		if(opos >= dz->buflen) {
			if(normaliser < 1.0) {
				for(n = 0;n < dz->buflen;n++)
					obuf[n] = (float)(obuf[n] * normaliser);
			}
			if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
				return(exit_status);
			memset((char *)obuf,0,dz->buflen * sizeof(float));
			opos = 0;
		}
		cycgrpcntr++;		//	Counter notes where we are in a cycle-group
		cyccntr++;			//	Counter notes where we are in a cycle
		outcnt++;			//	Counter notes where we are in the output
	}

	//	COMPLETE ALL ACTIVE WRITES

	finished = 0;
	while(!finished) {
		finished = 1;
		for(n = 0;n<ptrcnt;n++) {							//	For every pointer...
			if(dz->mode == 1)
				mm = n % dz->infilecnt;
			else
				mm = 0;
			if(ptr[n] > 0 && ptr[n] < dz->insams[mm]) {		//	If it is active
				finished = 0;								//	Flag not yet finished
				break;
			}
		}
		if(finished)
			break;

		trans_position = outcnt - transbas;
		transfrac = (double)trans_position/(double)transdur;	//  What fraction of this transition is this event at

		for(n = 0;n<ptrcnt;n++) {
			nn = n % dz->itemcnt;
			if(dz->mode == 1)
				mm = nn;
			else
				mm = 0;
			if(ptr[n] > 0 && ptr[n] < dz->insams[mm]) {
				boost = 1.0;
				if(dz->param[SHF_LBOOST] > 0) {				//	If there's a level boost to the focus stream
					if(trans_position < 0) {				//	Before transit starts, boost level of focus stream pointers (only)
							if(fflag[n] == 1) 
								boost = 1.0 + dz->param[SHF_LBOOST];
					} else {
						if(fflag[n] == 1) 
							boost = 1.0 + ((1.0 - transfrac) * dz->param[SHF_LBOOST]);
						else if(fflag[n] == 2) 
							boost = 1.0 + (transfrac * dz->param[SHF_LBOOST]);
					}
				}
				obuf[opos + lmost[nn]] = (float)(obuf[opos + lmost[nn]] + (ibuf[mm][ptr[n]] * llev[nn] * boost));
				obuf[opos + rmost[nn]] = (float)(obuf[opos + rmost[nn]] + (ibuf[mm][ptr[n]] * rlev[nn] * boost));
			}
			if(ptr[n] > 0)
				ptr[n]++;
		}
		opos += chans;
		if(opos >= dz->buflen) {
			if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
				return(exit_status);
			memset((char *)obuf,0,dz->buflen * sizeof(float));
			opos = 0;
		}
		outcnt++;
	}
	if(opos) {
		if(normaliser < 1.0) {
			for(n = 0;n < opos;n++)
				obuf[n] = (float)(obuf[n] * normaliser);
		}
		if((exit_status = write_samps(obuf,opos,dz))<0)
			return(exit_status);
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

/******************************** GEN_TIMES_ARRAY **************************
 *
 *	Time placement of a beat cycle in b beat cycle.
 *	e.g. a = 11 : b = 13   a beats are placed at
 *
 *	13/11 = 1+2/11   26/11 = 2+4/11    39/11 = 3+6/11 etc
 *
 *	"cycledur" is the duration of the entire 13-beat (and 11-beat) cycle
 *
 *  itimes stores the samplecnt-times of the 11-beats-in-13, approxed to nearest 1/12 beat
 */


void gen_times_array(int a, int b, int *itimes, int cycledur, dataptr dz)
{
	int n = 0, m;
	double real_timestep = (double)a/(double)b, fraction, twelfths;
	while(n < (double)a/2.0) {
		 fraction = (real_timestep * n)/(double)a;	//	fraction of cycle
		 twelfths = (int)round(fraction * dz->param[SHF_SUBDIV]);	//	fraction to nearest 1/SUBDIV beats in 1/SUBDIV-beats
		 fraction = twelfths/dz->param[SHF_SUBDIV];					//	fraction to nearest 1/SUBDIV beats
		 itimes[n]= (int)round(cycledur * fraction);//	time within cycle, to nearest 1/12 beat
		 n++;
	}												//	|  |  |  |  |	a4 (EVEN), array has value at centre
	m = n-1;										//	|   |   |   |
	if(EVEN(a))	{									//	
		itimes[n++] = cycledur/2;					//	|   |   |   |	a3 (ODD), array has NO value at centre
	}
	while(n < a) {									//	|  |  |  |  |
		itimes[n] = cycledur - itimes[m];	
		n++;										//	Make times are symmetrical around centre-position
		m--;
	}
}

void gen_regular_times_array(int a, int *itimes, double cycledur, dataptr dz)
{
	int n = 0;
	double fraction, real_timestep = cycledur/(double)a, srate = (double)dz->infile->srate;
	while(n < a) {
		 fraction = real_timestep * n;				//	fraction of cycle
		 itimes[n]= (int)round(fraction * srate);
		 n++;
	}
}

/********************************** GET_MAX_OVERLAP *********************************************
 *
 *	Find maximum self-overlay of input sounds, so we can have duplicate pointers.
 */

int get_max_overlap(int maxcycle,int maxinfiledur,int *cycle,dataptr dz)
{
	double srate = (double)dz->infile->srate;
	double min_cycunit_dur;
	int overlay, maxoverlay, n;
	if(dz->mode == 0) {
		min_cycunit_dur = ((double)dz->param[SHF_CYCDUR]/(double)maxcycle);		//	Division of cycleduration by largest cycle count gives fastest repeat time
		overlay = (int)ceil(dz->duration/min_cycunit_dur);						//	Division of largest infile by this gives maxoverlay
	} else {
		maxoverlay = 0;
		for(n=0;n<dz->infilecnt;n++) {
			min_cycunit_dur = ((double)dz->param[SHF_CYCDUR]/(double)cycle[n]);	//	Division of cycleduration by largest cycle count gives fastest repeat time
			overlay = (int)ceil(((double)dz->insams[n]/srate)/min_cycunit_dur);		//	Division of largest infile by this gives maxoverlay
			maxoverlay = max(maxoverlay,overlay);
		}
		overlay = maxoverlay;
	}
	return overlay;
}

/**************************** SHIFT_PARAM_PREPROCESS ****************************
 *
 *	cycleno = number of cycles
 *	ringsize = cycleno * : array for every pair of cycles
 *	These are to be arranged in ascending order, so for 11,12,13 ..
 *	11/12,11/13,11/13, 12/11,12/12,12/13,13/11,13/12,13/13
 *	For 11,12,13 largest array has 13 points, so make all arrays this big
 *
 * INTEGER ARRAYS					lmost			  LONG ARRAYS							DOUBLE ARRAYS
 *		  cycles				    | rmost				| thisreltime
 *		  |	time-arrays				| | perm			sndptrs		nextreltime				llev
 *		  |	|						| |	| foc			| |					  abstime		| rlev
 *		  |	|-----------------------| | | |	fflag			| |---------|---------|---------|	| | |
 *		  |	|						| | | |	| |			| |	cycleno	| cycleno | cycleno |	| | |
 *	array |	|						| | | |	| |			| |	 arrays	|  arrays |  arrays |	| | |
 *	  no  0	1 to ringsize			ringsize+1|			0 1			|		  |			|	0 1 |
 *		  |	|						| ringsize+2		| |			|		  |			|	| | |
 *		  | |						| | ringsize+3		| |			|		  |			|	| | |
 *		  |	|						| | | ringsize+4	| |			|		  |			|	| | |
 *		  |	|						| | | |	ringsize+5	| |			|		  |			|	| | |
 *		  |	|						cycleno	| |			| |			|		  |			|	| | |
 *		  cycleno					| cycleno |			cycleno*dupl|		  |			|	cycleno
 *	size  |	maxcycle				| | cycleno			| maxcycle	maxcycle  |			|	| cycleno
 *		  |	|						| | | maxcycle		| |			|		  |			|	| | |
 *		  |	|						| | | gpeventcnt	  
 *		  |	|						| | | | cycleno*dupl 
 *
 *	time-arrays store timings of 11 beats in 13, etc for all pairs of cycles
 *	lmost, and rmost are leftmost,rightmost lspkrs associated with stream position in output
 *	perm allows cycle focus order to be permuted (-r flag)
 *	foc remembers the order of focusings for the 2nd pass
 *	fflag flags whether any pointer is in focus(1), in next-focus(2) or neither(0)
 *
 *	sndptrs are the pointers into the source associated with each cycle
 *	thisreltime are the relative times of the current-event cycle with a specified focus
 *	nextreltime are the absolute times if the next-event cycle with a different focus
 *	abstime are the absolute times (possibly) adjusted for any transition between one focus and another
 *
 *	llev and rlev are the levels on relevant left and right speakers for each cycle
 *	perm allows order of cycle-focus in output to be permuted
 */

int shifter_param_preprocess(dataptr dz)
{
	int *cycle = dz->iparray[0], *lmost = NULL, *rmost = NULL;
	int n, m, k, chans = dz->iparam[SHF_OCHANS];
	int *ptr = NULL;
	int maxinfiledur, gpeventcnt;
	double *pos = NULL, *llev = NULL, *rlev = NULL;
	double leftgain, rightgain, srate = (double)dz->infile->srate;
	int maxcycle = 0;

	for(n = 0;n < dz->itemcnt;n++)
		maxcycle = max(maxcycle,cycle[n]);

	dz->ringsize = dz->itemcnt * dz->itemcnt;	//	Number of cycle-pairs

	//	integer arrays to store relative timings of events in all pairings of cycles

	for(n = 1; n <= dz->ringsize;n++) {
		if((dz->iparray[n] = (int *)malloc(maxcycle * sizeof(int)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to create integer array %d.\n",n+1);
			return(MEMORY_ERROR);
		}
	}

	//	integer arrays for leftmost and rightmost chan for pos of each cycle-output, and array for permuting infile order

	for(n = 1; n <= 3;n++) {
		if((dz->iparray[dz->ringsize+n] = (int *)malloc(dz->itemcnt * sizeof(int)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to create integer array %d.\n",dz->ringsize+n+1);
			return(MEMORY_ERROR);
		}
	}

	//	Array to remember which cycle is in focus, if randperm is used

	gpeventcnt = dz->iparam[SHF_CYCDUR] * (dz->iparam[SHF_LINGER] + dz->iparam[SHF_TRNSIT]);	//	Size of group-cycle
	gpeventcnt = (int)ceil(dz->iparam[SHF_OUTDUR]/gpeventcnt) + 4;  /* SAFETY */

	if((dz->iparray[dz->ringsize+4] = (int *)malloc(gpeventcnt * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create integer array %d.\n",dz->ringsize+5);
		return(MEMORY_ERROR);
	}

	if((dz->lparray = (int **)malloc(((dz->itemcnt * 3)+1) * sizeof(int *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create longs arrays.\n");
		return(MEMORY_ERROR);
	}

	//	Find maximum amount of self-overlap of input sounds, so we can have duplicate pointers into src-sound(s)	

	maxinfiledur = dz->insams[0];
	for(n=1;n<dz->infilecnt;n++ )
		maxinfiledur = max(maxinfiledur,dz->insams[n]);
	dz->dupl = get_max_overlap(maxcycle,maxinfiledur,cycle,dz);
	dz->dupl++;	//	SAFETY, as events are shifted slightly by the process, so could generate a further overlap

	//	long arrays for pointers to src(s) for each cycle + duplications where there could be source self-overlap

	if((dz->lparray[0] = (int *)malloc((dz->itemcnt * dz->dupl) * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create longs arrays for sound pointers.\n");
		return(MEMORY_ERROR);
	}

	//	Array to remember whether pointer is in focus, next-focus, or neither

	if((dz->iparray[dz->ringsize+5] = (int *)malloc((dz->itemcnt*dz->dupl) * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create integer array %d.\n",dz->ringsize+6);
		return(MEMORY_ERROR);
	}

	//	Integer arrays to store the absolute (param) timings of a particular focused cycle

	for(n = 1; n <= dz->itemcnt * 3;n++) {
		if((dz->lparray[n] = (int *)malloc(maxcycle * sizeof(int)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to create long arrays for absolute sampletime of cycles.\n");
			return(MEMORY_ERROR);
		}
	}
	//	2 double array for llev, rlev

	if((dz->parray = (double **)malloc(sizeof(double *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create doubles arrays.\n");
		return(MEMORY_ERROR);
	}

	for(n=0;n<2;n++) {
		if((dz->parray[n] = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to create doubles array %d.\n",n+1);
			return(MEMORY_ERROR);
		}
	}

	//	Calculate the relative-timings of events (in samples) in cycle-combos, and store	

	dz->iparam[SHF_CYCDUR] = (int)round(dz->param[SHF_CYCDUR] * srate);
	k = 1;
	for(n = 0;n < dz->itemcnt;n++) {
		for(m = 0;m < dz->itemcnt;m++) {
			if(m == n)
				gen_regular_times_array(cycle[n],dz->iparray[k],dz->param[SHF_CYCDUR],dz);
			else
				gen_times_array(cycle[n],cycle[m],dz->iparray[k],dz->iparam[SHF_CYCDUR],dz);
			k++;
		}
	}

	//	Calculate spatial positions of streams, and associated LR-levels, and store

	lmost = dz->iparray[dz->ringsize+1];
	rmost = dz->iparray[dz->ringsize+2];
	pos  = dz->parray[0];	//	Used initially for position, then overwritten by left level value
	llev = dz->parray[0];
	rlev = dz->parray[1];

	if(dz->itemcnt == chans) {
		for(n = 0;n<chans;n++) {
			lmost[n] = n;
			rmost[n] = (lmost[n] + 1) % chans;
			llev[n] = 1.0;
			rlev[n] = 0.0;
		}
	} else {
		if (chans == 1) {
			for(n = 0;n<dz->itemcnt;n++) {
				lmost[n] = 0;
				rmost[n] = 0;
				llev[n] = 1.0;		//	Daft, but keeps same algo for all cases
				rlev[n] = 0.0;
			}
		} else {
			if(dz->vflag[SHF_LIN] || chans == 2) {
				for(n = 0;n<dz->itemcnt;n++) {
					pos[n] = ((chans - 1) * n)/(double)(dz->itemcnt - 1);
					lmost[n] = (int)floor(pos[n]);
					pos[n]  -= lmost[n]; 
				}
			} else {
				for(n = 0;n<dz->itemcnt;n++) {
					pos[n] = (chans * n)/(double)dz->itemcnt;
					lmost[n] = (int)floor(pos[n]);
					pos[n]  -= lmost[n]; 
				}
			}
		}
		for(n = 0;n<dz->itemcnt;n++) {
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
	ptr = dz->lparray[0];
	for(n = 0;n<dz->itemcnt * dz->dupl;n++)
		ptr[n] = 0;
	return FINISHED;
}

/**************************** HANDLE_THE_SPECIAL_DATA ****************************/

int handle_the_special_data(char *str,dataptr dz)
{
	int cnt;
	int *cycle;
	FILE *fp;
	double dummy;
	int idummy, n, m, k;
	char temp[200], *p;
	if((fp = fopen(str,"r"))==NULL) {
		sprintf(errstr,"Cannot open file %s to read cyclelength data.\n",str);
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
			idummy = (int)round(dummy);
			if(idummy < 2) {
				sprintf(errstr,"Found cyclelength value %d in file %s: Cyclelength values must be >= 2\n",idummy,str);
				return(DATA_ERROR);
			}
			if(idummy > 32767) {
				sprintf(errstr,"Found cyclelength value %d in file %s: Too large\n",idummy,str);
				return(DATA_ERROR);
			}
			cnt++;
		}
	}
	if(dz->infilecnt > 1 && cnt != dz->infilecnt) {
		sprintf(errstr,"Number of cyclelengths (%d) found in file %s does not tally with number of src sounds (%d)\n",cnt,str,dz->infilecnt);
		return(DATA_ERROR);
	}
	dz->itemcnt = cnt;								//	Number of cycles
	dz->ringsize = dz->itemcnt *  dz->itemcnt;		//	Number of cycle pairs

/******************************************************************************
 *
 *	cycleno = number of cycles
 *	ringsize = cycleno * (cycleno-1) : array for every pair of cycles
 *	These are to be arranged in ascending order, so for 11,12,13 ..
 *	11/12,11/13,12/11,12/13,13/11,13/12
 *	Array of 13 points in time of 12 needs 13 entries ..
 *	For 11,12,13 largest array has 13 points, so make all arrays this big
 *
 * INTEGER ARRAYS					lmost
 *									| rmost
 *		  cycles				    | | perm 
 *		  |	time-arrays				| | | foc 
 *		  |	|						| | | | fflag
 *		  |	|-----------------------| | | | | |
 *	array |	|						ringsize+1|
 *	  no  0	1 to ringsize			| ringsize+2
 *		  |	|						| | ringsize+3
 *		  | |						| | | ringsize+4
 *		  cycleno					cycleno ringsize+5
 *	size  |	maxcycle				| cycleno |
 *		  |	|						| | cycleno
 *		  |	|						| | | gpeventcnt
 *		  |	|						| | | | cycleno*dupl
 *		  |	|						| | | | | |
 *
 *	time-arrays store timings of 11 beats in 13, etc for all pairs of cycles
 *	lmost, and rmost are leftmost,rightmost lspkrs associated with stream position in output
 *	perm allows permutation of focus order, for random-perm-focus option
 *	foc remembers which stream is focus for each group-event
 *  tflag marks whether pointer has focus(1) or next-focus(2) or neither(0)
 */

	if((dz->iparray = (int **)malloc((dz->ringsize + 6) * sizeof(int *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create integer arrays.\n");
		return(MEMORY_ERROR);
	}
	if((dz->iparray[0] = (int *)malloc(dz->itemcnt * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create cyclecnt storage array.\n");
		return(MEMORY_ERROR);
	}
	cycle = dz->iparray[0];	
	cnt = 0;
	fseek(fp,0,0);
	while(fgets(temp,200,fp)!=NULL) {
		p = temp;
		while(isspace(*p))
			p++;
		if(*p == ';' || *p == ENDOFSTR)	//	Allow comments in file
			continue;
		while(get_float_from_within_string(&p,&dummy))
			cycle[cnt++] = (int)round(dummy);
	}
	fclose(fp);
	for(n=0;n < cnt-1;n++) {		//	Sort into ascending order
		for(m = n+1;m<cnt;m++)  {
			if(cycle[m] == cycle[n]) {
				sprintf(errstr,"Two identical cycless of length %d found.\n",cycle[n]);
				return(DATA_ERROR);
			} else if (cycle[m] < cycle[n]) {
				k = cycle[n];
				cycle[n] = cycle[m];
				cycle[m] = k;
			}
		}
	}
	return FINISHED;
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

/**************************** CREATE_SHIFTER_SNDBUFS ****************************/

int create_shifter_sndbufs(dataptr dz)
{
	int n, safety = 4;
	unsigned int lastbigbufsize, bigbufsize = 0;
	float *bottom;
	dz->bufcnt = dz->infilecnt+1;
	if((dz->sampbuf = (float **)malloc(sizeof(float *) * (dz->bufcnt+1)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffers.\n");
		return(MEMORY_ERROR);
	}
	if((dz->sbufptr = (float **)malloc(sizeof(float *) * dz->bufcnt))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffer pointers.\n");
		return(MEMORY_ERROR);
	}
	lastbigbufsize = 0;
	bigbufsize = 0;
	for(n=0;n<dz->infilecnt;n++) {
		bigbufsize += (dz->insams[n] + safety) * sizeof(float);
		if(bigbufsize < lastbigbufsize) {
			sprintf(errstr,"Insufficient memory to store the input soundfiles in buffers.\n");
			return(MEMORY_ERROR);
		}
		lastbigbufsize = bigbufsize;
	}
	dz->buflen = NTEX_OBUFSIZE * dz->iparam[SHF_OCHANS];	
	bigbufsize += (dz->buflen + (safety * dz->iparam[SHF_OCHANS])) * sizeof(float);
	if(bigbufsize < lastbigbufsize) {
		sprintf(errstr,"Insufficient memory to store the input soundfiles in buffers.\n");
		return(MEMORY_ERROR);
	}
	if((dz->bigbuf = (float *)malloc(bigbufsize)) == NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
		return(PROGRAM_ERROR);
	}
	bottom = dz->bigbuf;
	for(n = 0;n<dz->infilecnt;n++) {
		dz->sbufptr[n] = dz->sampbuf[n] = bottom;
		bottom += dz->insams[n] + safety;
	}
	dz->sbufptr[n] = dz->sampbuf[n] = bottom;
	return(FINISHED);
}

/*********************** RNDINTPERM ************************/

void rndintperm(int *perm,int cnt)
{
	int n,t,k;
	memset((char *)perm,0,cnt * sizeof(int));
	for(n=0;n<cnt+1;n++) {
		t = (int)(drand48() * (double)(n+1)); /* TRUNCATE */
		if(t==n) {
			for(k=n;k>0;k--)
				perm[k] = perm[k-1];
			perm[0] = n;
		} else {
			for(k=n;k>t;k--)
				perm[k] = perm[k-1];
			perm[t] = n;
		}
	}
	for(n=0;n<cnt;n++)
		perm[n]--;
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
	dz->infile->channels = dz->iparam[SHF_OCHANS];
	if((exit_status = create_sized_outfile(dz->outfilename,dz))<0)
		return(exit_status);
	dz->infile->channels = 1;
	return(FINISHED);
}

