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

#define PAK_SCALE (6.0)		//	Scaling of centring 0-1 range, to make it more linear in effect
#define PAK_MINSEG (32)		//	minimum length of halfwave to level-adjust with -f
#define PAK_NORM  (0.95)	//	Level of normalised packet

#define BUFEXPAND	(3)

#ifdef unix
#define round(x) lround((x))
#endif

char errstr[2400];

int anal_infiles = 1;
int	sloom = 0;
int sloombatch = 0;

const char* cdp_version = "7.1.0";

//CDP LIB REPLACEMENTS
static int setup_packet_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_packet_param_ranges_and_defaults(dataptr dz);
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
static int packet(dataptr dz);

static int find_all_positive_peaks(int startsearch,int endsearch,int *peakcnt,dataptr dz);
static int find_all_local_minima(int peakcnt,int *local_minima_cnt,dataptr dz);
static int locate_zero_crossings(int local_minima_cnt,dataptr dz);
static int create_packet_sndbufs(dataptr dz);
static int packet_param_preprocess(dataptr dz);
static void shave(float *obuf,int *outcnt);

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
		if((exit_status = setup_packet_application(dz))<0) {
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
	if((exit_status = setup_packet_param_ranges_and_defaults(dz))<0) {
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
	dz->larray_cnt = 2;
	if((dz->lparray = (int **)malloc(dz->larray_cnt * sizeof(int *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for internal long arrays.\n");
		return(MEMORY_ERROR);
	}
	dz->array_cnt = 3;
	if((dz->parray = (double **)malloc(dz->array_cnt * sizeof(double *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for internal integer arrays.\n");
		return(MEMORY_ERROR);
	}
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

	if((exit_status = create_packet_sndbufs(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = packet_param_preprocess(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	//spec_process_file =
	if((exit_status = packet(dz))<0) {
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
	int n;
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
	if(sloom) {						//	IF sloom, drop trailing zero
		n = strlen(dz->outfilename);
		n--;
		dz->outfilename[n] = ENDOFSTR;
	}
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

/************************* SETUP_PACKET_APPLICATION *******************/

int setup_packet_application(dataptr dz)
{
	int exit_status;
	aplptr ap;
	if((exit_status = establish_application(dz))<0)		// GLOBAL
		return(FAILED);
	ap = dz->application;
	// SEE parstruct FOR EXPLANATION of next 2 functions
	if((exit_status = set_param_data(ap,PAK_TIMES,3,3,"ddd"))<0)
		return(FAILED);
	if((exit_status = set_vflgs(ap,"",0,"","nfs",3,0,"000"))<0)
		return(FAILED);
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

/************************* SETUP_PACKET_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_packet_param_ranges_and_defaults(dataptr dz)
{
	int exit_status;
	aplptr ap = dz->application;
	// set_param_ranges()
	ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
	// NB total_input_param_cnt is > 0 !!!
	if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
		return(FAILED);
	// get_param_ranges()
	ap->lo[PAK_DUR]	= 2.0;
	ap->hi[PAK_DUR]	= (dz->duration/2.0) * SECS_TO_MS;
	ap->default_val[PAK_DUR] = 50.0;
	ap->lo[PAK_SQZ]	= 0.0;
	ap->hi[PAK_SQZ]	= 1000.0;
	ap->default_val[PAK_SQZ]	= 1.0;
	ap->lo[PAK_CTR]	= -1.0;
	ap->hi[PAK_CTR]	= 1.0;
	ap->default_val[PAK_CTR]	= 0.0;
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
			if((exit_status = setup_packet_application(dz))<0)
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
	usage2("packet");
	return(USAGE_ONLY);
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	if(!strcmp(prog_identifier_from_cmdline,"packet"))				dz->process = PACKET;
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
	if(!strcmp(str,"packet")) {
		fprintf(stdout,
	    "USAGE:\n"
	    "packet packet 1 infile outfile times mindur narrowing centring [-n|-f] [-s]\n"
	    "packet packet 2 infile outfile times dur    narrowing centring [-n|-f] [-s]\n"
		"\n"
		"Isolate or generate  a sound packet.\n"
		"\n"
		"Mode 1:    Found packet:  Looks for signal minima as edges of wave-packet.\n"
		"Mode 2:   Forced packet: Creates packet at specified time.\n"
		"\n"
		"TIMES     Time (or textfile of times) at which packet is extracted/created.\n"
		"\n"
		"MINDUR    Min duration of (found) packet (in mS) (less than half src duration).\n"
		"DUR       Duration of (forced) packet (in mS)  (less than half src duration).\n"
		"\n"
		"NARROWING Narrowing of packet envelope (0 - 1000).\n"
		"          Values below 1.0 broaden the packet.\n"
		"          Values very close to zero may produce clicks (square-wave envelope).\n"
		"          Very high vals with very short packets may produce\n"
		"          click-impulses or silence.\n"
		"\n"
		"CENTRING  Centring of peak of packet envelope.\n"
		"          0  peak at centre: -1 peak at start: 1 peak at end.\n"
		"          If packet content has varying level, true peak position may not\n"
		"          correspond to envelope peak position, unless \"-f\" flag used.\n"
		"\n"
		"-n        Normalise packet level.\n"
		"\n"
		"-f        Packet wave maxima and minima forced up(down)to packet contour.\n"
		"          (default, packet envelope simply imposed on existing signal).\n"
		"          (normalisatiojn flag ignored if \"-f\" flag used\n"
		"\n"
		"-s        Shave off leading or trailing silence.\n"
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

/**************************** PACKET_PARAM_PREPROCESS *************************/

int packet_param_preprocess (dataptr dz)
{
	int n, halftabsize = TREMOLO_TABSIZE/2;
	int isneg = 0, tablopos, tabhipos;
	double *costab, *temptab, diff, tabrem, tabincr, lotabincr, hitabincr, readpos, frac;

	if((dz->parray[0] = (double *)malloc((TREMOLO_TABSIZE + 1) * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for sine table.\n");
		return(MEMORY_ERROR);
	}
	costab = dz->parray[0];
	if((dz->parray[2] = (double *)malloc((TREMOLO_TABSIZE + 1) * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for sine table.\n");
		return(MEMORY_ERROR);
	}
	temptab = dz->parray[2];
	for(n=0;n<TREMOLO_TABSIZE;n++) {
		costab[n] = cos(PI * 2.0 * ((double)n/(double)TREMOLO_TABSIZE));
		costab[n] += 1.0;
		costab[n] /= 2.0;
		costab[n] = 1.0 - costab[n];
	}
	costab[n] = 0.0; /* wrap around point */
	if(!flteq(dz->param[PAK_SQZ],1.0)) {
		for(n=0;n<TREMOLO_TABSIZE;n++)
			costab[n] = pow(costab[n],dz->param[PAK_SQZ]);
	}
	if(!flteq(dz->param[PAK_CTR],1.0)) {
		if(dz->param[PAK_CTR] < 0.0) {
			frac = 1.0 + dz->param[PAK_CTR];
			isneg = 1;
		} else 
			frac = 1.0 - dz->param[PAK_CTR];
		if(isneg) {
			lotabincr = 1.0/frac;
			hitabincr = 1.0/(2.0 - frac);
		} else {
			lotabincr = 1.0/(2.0 - frac);
			hitabincr = 1.0/frac;
		}
		readpos = 0;
		tabincr = lotabincr;
		for(n=0;n<TREMOLO_TABSIZE;n++) {
			if(readpos >= halftabsize)
				tabincr = hitabincr;
			tablopos = (int)floor(readpos);
			tabhipos = min(tablopos + 1,TREMOLO_TABSIZE);
			tabrem     = readpos - (double)tablopos;
			diff       = costab[tabhipos] - costab[tablopos];
			temptab[n] = costab[tablopos] + (diff * tabrem);
			readpos += tabincr;
		}
		for(n=0;n<TREMOLO_TABSIZE;n++)
			costab[n] = temptab[n];
	}
	return(FINISHED);
}

/******************************** PACKET ********************************/

int packet(dataptr dz)
{
	int exit_status, chans = dz->infile->channels, fileno, j, OK, gotit, negwritten, poswritten = 0, ispos;
	float *ibuf = dz->sampbuf[0], *obuf = dz->sampbuf[1];
	double *costab = dz->parray[0];
	int n, m, k, nn = -1, srate = dz->infile->srate, *packet_at = dz->lparray[0], *pos, peakcnt = 0, local_minima_cnt = 0;
	int halfspan = 0, span, pakat, startspan, endspan, pack_centre, pack_start = 0, pack_end = 0;
	int true_startspan = 0, true_endspan = 0, truehalfspan = 0, truespan = 0;
	int pack_gpstart, outcnt, outgpcnt, tabsize = TREMOLO_TABSIZE, tablopos, tabhipos;
	int  upstart = -1, maxat = -1, dnstart = -1, minat = -1;
	char filename[200],  thisnum[8];
	double tabpos, tabincr, tabrem, diff, env, maxval = -1.0, minval = 1.0, adjust, normaliser, maxsamp;

	dz->iparam[PAK_DUR] = (int)round(dz->param[PAK_DUR] * MS_TO_SECS * (double)srate) * chans;
	if((dz->lparray[1] = (int *)malloc(dz->iparam[PAK_DUR] * BUFEXPAND * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store peak locations.\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[1] = (double *)malloc(dz->iparam[PAK_DUR] * BUFEXPAND * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store peaks.\n");
		return(MEMORY_ERROR);
	}
	pos = dz->lparray[1];
	if(dz->mode == 0) {
		truehalfspan = (int)ceil((dz->param[PAK_DUR] * MS_TO_SECS/2.0) * (double)srate) * chans;
		truespan = truehalfspan * 2;
		halfspan = truehalfspan * 3;
	}
	for(fileno = 0;fileno < dz->itemcnt; fileno++) {
		if(dz->mode == 0) {
			pakat  = packet_at[fileno];
			startspan = pakat - halfspan;
			if(startspan < 0)
				startspan = 0;
			endspan = pakat + halfspan;
			if(endspan > dz->insams[0])
				endspan = dz->insams[0];
			span = endspan - startspan;
			if(span < dz->iparam[PAK_DUR]) {
				fprintf(stdout,"WARNING: Packet %d too close to start or end of file: add silence to (start or end) of source.\n",fileno+1);
				fflush(stdout);
				continue;
			}
			true_startspan = pakat - truehalfspan;
			true_endspan   = pakat + truehalfspan;
			if(startspan > 0) {
				true_startspan -= startspan;
				true_endspan   -= startspan;
				endspan        -= startspan; 
				sndseekEx(dz->ifd[0],startspan,0);
				startspan = 0;
			}
			if((exit_status = read_samps(ibuf,dz))<0)
				return(exit_status);
			peakcnt = 0;
			if((exit_status = find_all_positive_peaks(startspan,endspan,&peakcnt,dz)) < 0)
				return exit_status;
			local_minima_cnt = 0;
			if((exit_status = find_all_local_minima(peakcnt,&local_minima_cnt,dz)) < 0)
				return(exit_status);
			if((exit_status = locate_zero_crossings(local_minima_cnt,dz)) < 0)
				return (exit_status);
			OK = 0;
			gotit = 0;
			for(n = 0;n < local_minima_cnt; n++) {
				if(pos[n] > true_startspan) {			//  Once we reach the start of true-span
					if(n > 0) {							//	Find first minimum before true-span
						k = n-1;
						pack_start = (pos[k]/chans) * chans;
					} else {
						k = 0;
						pack_start = 0;
					}
					endspan = max(pack_start + truespan,true_endspan);
					if(endspan > dz->buflen) {
						fprintf(stdout,"WARNING: Failed to find long-enough segment for time %d\n",fileno + 1);
						fflush(stdout);
						OK = 0;
						break;
					}
					m = k;								//	Fine first minimum after true-span
					while(m < local_minima_cnt) {
						if(pos[m] >= endspan) {
							pack_end = (pos[m]/chans) * chans;
							OK = 1;
							gotit = 1;
							break;
						}
						m++;
					} 
					if(!gotit) {
						fprintf(stdout,"WARNING: Failed to find segment for time %d\n",fileno + 1);
						fflush(stdout);
						OK = 0;
					}
					break;								//	Break, whether or not we found a valid cut
				}
			}
			if(!OK)										//	If no valid cut here, move to next designated time
				continue;
		} else {
			pack_centre  = packet_at[fileno];
			span     = (int)ceil((dz->param[PAK_DUR] * MS_TO_SECS) * (double)srate) * chans;
			halfspan = (int)ceil((dz->param[PAK_DUR] * MS_TO_SECS/2.0) * (double)srate) * chans;
			pack_start = pack_centre - halfspan;
			pack_end   = pack_centre + halfspan;
			if(pack_start <= 0) {
				pack_start = 0;
				pack_end = pack_start + span;
				if(pack_end > dz->buflen) {
					sprintf(errstr,"Error in buffer size estimation\n");
					return(PROGRAM_ERROR);
				}
			} else {
				sndseekEx(dz->ifd[0],pack_start,0);
				pack_end -= pack_start; 
				pack_start = 0;
			}
			if((exit_status = read_samps(ibuf,dz))<0)
				return(exit_status);
		}
		if(fileno > 0) {
			if(sndcloseEx(dz->ofd) < 0) {
				sprintf(errstr,"Failed to close output file %d\n",fileno+1);
				return(SYSTEM_ERROR);
			}
			dz->ofd = -1;
		}
		strcpy(filename,dz->outfilename);
		sprintf(thisnum,"%d",fileno);
		strcat(filename,thisnum);
		strcat(filename,".wav");
		if((exit_status = create_sized_outfile(filename,dz))<0)
			return(exit_status);
		sndseekEx(dz->ifd[0],0,0);
		reset_filedata_counters(dz);
		pack_gpstart = pack_start/chans;
		for(n = pack_start, outcnt = 0;n < pack_end; n++, outcnt++)
			obuf[outcnt] = ibuf[n];				//	Copy basic packet to outbuf
		outgpcnt = outcnt/chans;
		tabincr = (double)tabsize/(double)(outgpcnt - 1);	//	Force last read to be at end of envelope table
		tabpos = 0;
		for(n = 0; n < outcnt; n+=chans) {		//	Impose envelope on packet
			tablopos = (int)floor(tabpos);
			tabhipos = min(tablopos + 1,TREMOLO_TABSIZE);
			tabrem   = tabpos - (double)tablopos;
			diff = costab[tabhipos] - costab[tablopos];
			env  = costab[tablopos] + (diff * tabrem);
			for(j=0;j<chans;j++)
				obuf[n+j] = (float)(obuf[n+j] * env);
			tabpos += tabincr;
		}
		if(dz->vflag[1]) {						//	if "forced"
			negwritten = 0;						//	Force levels to follow packet envelope countour
			poswritten = 0;
			ispos = 0;
			for(j = 0; j < chans; j++) {		//	for each channel in turn.
				for(n = 0; n < outgpcnt; n++) {	//	Search the packet
					nn = (n * chans) + j;		//	Start in specific channel
					switch(ispos) {
					case(0):					//	Does it go +ve or -ve initially?
						if(obuf[nn] > 0) {
							upstart = nn;
							ispos = 1;
							maxval = obuf[nn];
							maxat = n;
						} else if(obuf[nn] < 0) {
							dnstart = nn;
							ispos = -1;
							minval = obuf[nn];
							minat = n;			//	NB index to groupcnt, not sample-cnt
						}
						break;
					case(1):					//	+ve vals
						if(obuf[nn] > 0) {								//	While > 0, Keep looking for maximum
							maxval = max(maxval,obuf[nn]);
							maxat = n;
						} else {										//	Once goes -ve
							if(nn - upstart >= PAK_MINSEG * chans) {
								tabpos = maxat * tabincr;				//	Find corresponding (group-)position in envelope
								tablopos = (int)floor(tabpos);
								tabhipos = min(tablopos + 1,TREMOLO_TABSIZE);
								tabrem   = tabpos - (double)tablopos;
								diff = costab[tabhipos] - costab[tablopos];
								env = costab[tablopos] + (diff * tabrem);
								adjust = env/maxval;					//	Find ratio of envelope to max sample pf +ve excursion
													
								for(m = upstart; m < nn; m+= chans)		//	Adjust the +ve excursion to level of envelope
									obuf[m] = (float)(obuf[m] * adjust);
							}
							poswritten = 1;								//	Note that adjustment made (or ignored)
							negwritten = 0;
							ispos = -1;									//	Mark start of -ve excursion
							minval = obuf[nn];
							minat = n;
							dnstart = nn;
						}
						break;
					case(-1):					//	-ve vals
						if(obuf[nn] <= 0) {
							minval = min(minval,obuf[nn]);
							minat = nn;
						} else {
							if(nn - upstart >= PAK_MINSEG * chans) {
								tabpos = minat * tabincr;
								tablopos = (int)floor(tabpos);
								tabhipos = min(tablopos + 1,TREMOLO_TABSIZE);
								tabrem   = tabpos - (double)tablopos;
								diff = costab[tabhipos] - costab[tablopos];
								env = costab[tablopos] + (diff * tabrem);
								adjust = fabs(env/minval);				//	 Find ratio of envelope to fabs(maxsample) of -ve excursion
								for(m = dnstart; m < nn; m+= chans)
									obuf[m] = (float)(obuf[m] * adjust);
							}
							negwritten = 1;
							poswritten = 0;
							ispos = 1;
							maxval = obuf[nn];
							maxat = n;
							upstart = nn;
						}
						break;
					}
				}
				if(ispos && !poswritten) {								//	Finish off final excursion
					if(nn < 0 || upstart < 0 || maxval < 0.0 || maxat < 0) {
						fprintf(stderr,"Error in accounting 1\n");
						return(PROGRAM_ERROR);
					}
					tabpos = maxat * tabincr;
					tablopos = (int)floor(tabpos);
					tabhipos = min(tablopos + 1,TREMOLO_TABSIZE);
					tabrem   = tabpos - (double)tablopos;
					diff = costab[tabhipos] - costab[tablopos];
					env = costab[tablopos] + (diff * tabrem);
					adjust = env/maxval;
					for(m = upstart; m < nn; m+= chans)
						obuf[m] = (float)(obuf[m] * adjust);
				} else if(!ispos && !negwritten) {
					if(nn < 0 || dnstart < 0 || minval > 0.0 || minat < 0) {
						fprintf(stderr,"Error in accounting 2\n");
						return(PROGRAM_ERROR);
					}
					tabpos = minat * tabincr;
					tablopos = (int)floor(tabpos);
					tabhipos = min(tablopos + 1,TREMOLO_TABSIZE);
					tabrem   = tabpos - (double)tablopos;
					diff = costab[tabhipos] - costab[tablopos];
					env = costab[tablopos] + (diff * tabrem);
					adjust = fabs(env/maxval);
					for(m = dnstart; m < nn; m+= chans)
						obuf[m] = (float)(obuf[m] * adjust);
				}
			}
			for(n = 0; n < outcnt; n++)
				obuf[n] = (float)(obuf[n] * PAK_NORM);
		} else if(dz->vflag[0]) {
			maxsamp = 0.0;
			for(n = 0; n < outcnt; n++)
				maxsamp = max(fabs(obuf[n]),maxsamp);
			if(maxsamp < PAK_NORM) {
				normaliser = PAK_NORM/maxsamp;
				for(n = 0; n < outcnt; n++)
					obuf[n] = (float)(obuf[n] * normaliser);
			}
		}
		if(dz->vflag[2])
			shave(obuf,&outcnt);
		if((exit_status = write_samps(obuf,outcnt,dz))<0)
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

/**************************** HANDLE_THE_SPECIAL_DATA ****************************/

int handle_the_special_data(char *str,dataptr dz)
{		
	double dummy = 0.0;
	int *q;
	FILE *fp;
	int cnt, isfile = 0;
	char temp[200], *p;
	if(!sloom) {
		if(!value_is_numeric(str) && file_has_invalid_startchar(str)) {
    		sprintf(errstr,"Cannot read time value [%s]\n",str);
			return(USER_ERROR);
		}
		if(value_is_numeric(str)) {
			if(sscanf(str,"%lf",&dummy)<1) {
		   		sprintf(errstr,"Cannot read times parameters '%s'\n",str);
				return(USER_ERROR);
			}	
		} else {
			isfile = 1;
		}
	} else {					/* TK convention, all numeric values are preceded by NUMERICVAL_MARKER */
		if(str[0]==NUMERICVAL_MARKER) {		 
			str++;				
			if(strlen(str)<=0 || sscanf(str,"%lf",&dummy)!=1) {
				sprintf(errstr,"Invalid time value (%s) encountered.\n",str);
				return(USER_ERROR);
			}
		} else {
			isfile = 1;
		}
	}
	if(isfile) {
		if((fp = fopen(str,"r"))==NULL) {
			sprintf(errstr,"Cannot open file %s to read times.\n",str);
			return(DATA_ERROR);
		}
		cnt = 0;
		while(fgets(temp,200,fp)!=NULL) {
			p = temp;
			if(*p == ';')	//	Allow comments in file
				continue;
			while(get_float_from_within_string(&p,&dummy)) {
				if(dummy < 0.0) {
					sprintf(errstr,"Time (%lf) less than zero in cuts file.\n",dummy);
					return(DATA_ERROR);
				} else if(dummy > dz->duration) {
					sprintf(errstr,"Time (%lf) beyond end of file (%lf).\n",dummy,dz->duration);
						return(DATA_ERROR);
				}
				cnt++;
			}
		}
		if(cnt == 0) {
			sprintf(errstr,"No valid times in file.\n");
			return(DATA_ERROR);
		}
		dz->itemcnt = cnt;
		dz->larray_cnt = 1;
		if((dz->lparray[0] = (int *)malloc(dz->itemcnt * sizeof(int)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to store packet times.\n");
			return(MEMORY_ERROR);
		}
		fseek(fp,0,0);
		cnt = 0;
		q = dz->lparray[0];
		while(fgets(temp,200,fp)!=NULL) {
			p = temp;
			if(*p == ';')	//	Allow comments in file
				continue;
			while(get_float_from_within_string(&p,&dummy)) {
				*q = (int)round(dummy * (double)(dz->infile->srate)) * dz->infile->channels;
				q++;
			}
		}
		if(fclose(fp)<0) {
			fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",str);
			fflush(stdout);
		}
	} else {
		if(dummy < 0.0) {
			sprintf(errstr,"Time (%lf) less than zero.\n",dummy);
			return(DATA_ERROR);
		} else if(dummy > dz->duration) {
			sprintf(errstr,"Time (%lf) beyond end of file (%lf).\n",dummy,dz->duration);
				return(DATA_ERROR);
		}
		if((dz->lparray[0] = (int *)malloc(sizeof(int)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to store packet time.\n");
			return(MEMORY_ERROR);
		}
		dz->lparray[0][0] = (int)round(dummy * (double)(dz->infile->srate)) * dz->infile->channels;
		dz->itemcnt = 1;
	}
	return(FINISHED);
}

/****************************** FIND_ALL_POSITIVE_PEAKS ***********************************/

int find_all_positive_peaks(int startsearch,int endsearch,int *peakcnt,dataptr dz)
{
	double *peak = dz->parray[1], thispeak;
	int *pos  = dz->lparray[1];
	float *ibuf = dz->sampbuf[0];
	int thissamp = startsearch, thispos;
	while(ibuf[thissamp] <= 0) {		/* skip values below zero */
		if(++thissamp >= endsearch)
			break;
	}
	if(thissamp >= endsearch) {
		sprintf(errstr,"Cannot locate any peaks in the signal. Are the search times incorrect??\n");
		return(DATA_ERROR);
	}
	thispeak = ibuf[thissamp];
	thispos = thissamp;
	thissamp++;
	while(thissamp < endsearch) {
		if(ibuf[thissamp] >= 0.0) {
			if(ibuf[thissamp] > thispeak) {	/* search for (positive) peak val */
				thispeak = ibuf[thissamp];	
				thispos  = thissamp;
			}
		} else {
			peak[*peakcnt] = thispeak;		/* once signal becomes -ve3, store last found peak */
			pos[*peakcnt] = thispos;
			(*peakcnt)++;
			while(ibuf[thissamp] < 0) {		/* then skip over -ve part of signal */
				if(++thissamp >= endsearch)
					break;
			}
			thispeak = ibuf[thissamp];		/* once dignal is +ve again, set up an initial value for peak */
			thispos  = thissamp;
		}
		thissamp++;
	}
	if(*peakcnt > 0) {						/* check for peak found near end, before signal goes -ve once more */
		if((thispos != pos[(*peakcnt)-1]) && (thispeak > 0.0)) {
			peak[*peakcnt] = thispeak;
			pos[*peakcnt] = thispos;
			(*peakcnt)++;
		}
	}
	if(*peakcnt < 3) {
		sprintf(errstr,"Insufficient signal peaks found (need 3).??\n");
		return(DATA_ERROR);
	}
	return(FINISHED);
}

/****************************** FIND_ALL_LOCAL_MINIMA ***********************************/

int find_all_local_minima(int peakcnt,int *local_minima_cnt,dataptr dz)
{
	int thispeak;
	double *peak = dz->parray[1];
	int *pos = dz->lparray[1];
/*	double peakmin = peak[0];*/
	int finished = 0;
	*local_minima_cnt = 0;
	thispeak = 1;
	while(thispeak < peakcnt) {
		while(peak[thispeak] <= peak[thispeak-1]) {	/* while peaks are falling, look for local peak minimum */
			if(++thispeak >= peakcnt) {
				finished = 1;
				break;
			}
		}
		if(finished)
			break;
		peak[*local_minima_cnt] = peak[thispeak-1];	/* store value and position of local mimimum */
		pos[*local_minima_cnt]  = pos[thispeak-1];
		(*local_minima_cnt)++;
		while(peak[thispeak] >= peak[thispeak-1]) {	/* skip over rising sequence of peaks */
			if(++thispeak >= peakcnt) {
				break;
			}
		}
	}
	if(*local_minima_cnt < 2) {
		sprintf(errstr,"Insufficient local minima found in inital search.\n");
		return(DATA_ERROR);
	}
	return(FINISHED);
}

/****************************** LOCATE_ZERO_CROSSINGS ***********************************/

int locate_zero_crossings(int local_minima_cnt,dataptr dz)
{
	int finished = 0;
	int n;
	float  *ibuf = dz->sampbuf[0];
	double *peak = dz->parray[1];
	int   *pos  = dz->lparray[1];
	for(n=0;n<local_minima_cnt;n++) {
		while (peak[n] >= 0.0) {				/* advance position from minimum +ve peak until value crosses zero */
			if(++pos[n] >= dz->buflen) {
				finished = 1;
				break;
			}
			peak[n] = ibuf[pos[n]];
		}
		if(finished)
			break;
	}
	return(FINISHED);
}

/*************************** CREATE_PACKET_SNDBUFS **************************/

int create_packet_sndbufs(dataptr dz)
{
	int n;
	int bigbufsize;
	int framesize = F_SECSIZE * dz->infile->channels;
	if(dz->sbufptr == 0 || dz->sampbuf==0) {
		sprintf(errstr,"buffer pointers not allocated: create_packet_sndbufs()\n");
		return(PROGRAM_ERROR);
	}
	dz->buflen = (int)ceil(dz->param[PAK_DUR] * (double)dz->infile->srate) * dz->infile->channels;
	dz->buflen *= BUFEXPAND;
	dz->buflen = (dz->buflen / framesize)  * framesize;
	dz->buflen += framesize;
	bigbufsize = dz->buflen * sizeof(float);
	if((dz->bigbuf = (float *)malloc(bigbufsize  * dz->bufcnt)) == NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
		return(PROGRAM_ERROR);
	}
	for(n=0;n<dz->bufcnt;n++)
		dz->sbufptr[n] = dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
	dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
	return(FINISHED);
}

/*************************** SHAVE **************************/

void shave(float *obuf,int *outcnt)
{
	int n = 0, m, k = *outcnt;
	while(obuf[n] == 0.0) {
		n++;
		if(n >= *outcnt)
			return;
	}
	if(n > 0) {
		for(k = 0,m = n;m < *outcnt; m++,k++)
			obuf[k] = obuf[m];
		*outcnt = k;
	}
	k--;
	while(obuf[k] == 0.0)
		k--;
	*outcnt = k + 1;
}
