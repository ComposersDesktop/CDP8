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
#include <science.h>
#include <ctype.h>
#include <sfsys.h>
#include <string.h>
#include <srates.h>
#include <standalone.h>


#ifdef unix
#define round(x) lround((x))
#endif

char errstr[2400];

int anal_infiles = 0;
int	sloom = 0;
int sloombatch = 0;

const char* cdp_version = "7.1.0";

static int check_filtrage_param_validity_and_consistency(dataptr dz);
static int setup_filtrage_application(dataptr dz);
static int setup_filtrage_param_ranges_and_defaults(dataptr dz);

static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
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
static int open_the_outfile(dataptr dz);
static void rndintperm(int *perm,int cnt);
static int get_the_mode_from_cmdline(char *str,dataptr dz);
static int filtrage(dataptr dz);

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
		if((exit_status = setup_filtrage_application(dz))<0) {
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

	// setup_param_ranges_and_defaults() =
	if((exit_status = setup_filtrage_param_ranges_and_defaults(dz))<0) {
		exit_status = print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	// handle_outfile() = 
	if((exit_status = handle_the_outfile(&cmdlinecnt,&cmdline,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {		// CDP LIB
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
//	check_param_validity_and_consistency....
	if((exit_status = check_filtrage_param_validity_and_consistency(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = open_the_outfile(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((dz->parray  = (double **)malloc(3 * sizeof(double *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for filter values arrays.\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[0] = (double *)malloc(dz->iparam[FILTR_CNT] * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for filter frqs array.\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[1] = (double *)malloc(dz->iparam[FILTR_CNT] * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for filter amplitudes array.\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[2] = (double *)malloc(dz->iparam[FILTR_CNT] * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for filter amplitude permutation array.\n");
		return(MEMORY_ERROR);
	}
	if((exit_status = filtrage(dz))<0) {
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

	if((storage_cnt = tipc + ap->internal_param_cnt)>0) {		  
		if((exit_status = setup_parameter_storage_and_constants(storage_cnt,dz))<0)	  
			return(exit_status);
		if((exit_status = initialise_is_int_and_no_brk_constants(storage_cnt,dz))<0)	  
			return(exit_status);
	}													   
 	if((exit_status = mark_parameter_types(dz,ap))<0)	  
		return(exit_status);
	dz->infilecnt = 0;
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
	if(!sloom) {
		if(file_has_invalid_startchar(filename) || value_is_numeric(filename)) {
			sprintf(errstr,"Outfile name %s has invalid start character(s) or looks too much like a number.\n",filename);
			return(DATA_ERROR);
		}
	}
	p = filename + strlen(filename);
	p--;
	while(*p != '.') {
		p--;
		if(p <= filename) {
			break;
		}
		if(*p == '.') {
			*p = ENDOFSTR;
			break;
		}
	}
	strcpy(dz->outfilename,filename);	   
	strcat(dz->outfilename,".txt");
	(*cmdline)++;
	(*cmdlinecnt)--;
	return(FINISHED);
}

/************************ OPEN_THE_OUTFILE *********************/

int open_the_outfile(dataptr dz)
{
	int exit_status;
	if((exit_status = create_sized_outfile(dz->outfilename,dz))<0)
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

/************************* SETUP_SPECTRUM_APPLICATION *******************/

int setup_filtrage_application(dataptr dz)
{
	int exit_status;
	aplptr ap;
	if((exit_status = establish_application(dz))<0)		// GLOBAL
		return(FAILED);
	ap = dz->application;
	// SEE parstruct FOR EXPLANATION of next 2 functions
	if(dz->mode == 0) {
		if((exit_status = set_param_data(ap,0   ,11,9,"diddddddd00"))<0)
			return(FAILED);
	} else {
		if((exit_status = set_param_data(ap,0   ,11,11,"diddddddddd"))<0)
			return(FAILED);
	}
	if((exit_status = set_vflgs(ap,"s",1,"i","",0,0,""))<0)
		return(FAILED);
	// assign_process_logic -->
	dz->input_data_type = NO_FILE_AT_ALL;
	dz->process_type	= TO_TEXTFILE;	
	dz->outfiletype  	= TEXTFILE_OUT;
	// set_legal_infile_structure -->
	dz->has_otherfile = FALSE;
	dz->maxmode = 2;
	return application_init(dz);	//GLOBAL
}

/************************* SETUP_FILTRAGE_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_filtrage_param_ranges_and_defaults(dataptr dz)
{
	int exit_status;
	aplptr ap = dz->application;
	// set_param_ranges()
	ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
	// NB total_input_param_cnt is > 0 !!!
	if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
		return(FAILED);
	// get_param_ranges()
	ap->lo[FILTR_DUR]	= 0;
	ap->hi[FILTR_DUR]	= 32767;	// duration
	ap->default_val[FILTR_DUR] = 10;
	ap->lo[FILTR_CNT]	= 2;
	ap->hi[FILTR_CNT]	= 400;		// number of filters
	ap->default_val[FILTR_CNT] = 64;
	ap->lo[FILTR_MMIN]	= 0;
	ap->hi[FILTR_MMIN]	= 127;		// min MIDI value
	ap->default_val[FILTR_MMIN] = 0;
	ap->lo[FILTR_MMAX]	= 0;
	ap->hi[FILTR_MMAX]	= 127;		// max MIDI value
	ap->default_val[FILTR_MMAX] = 0;
	ap->lo[FILTR_DIS]	= 0.01;
	ap->hi[FILTR_DIS]	= 100;		// filter distribution over range
	ap->default_val[FILTR_DIS] = 1;
	ap->lo[FILTR_RND]	= 0;
	ap->hi[FILTR_RND]	= 1;		// filter randomisation
	ap->default_val[FILTR_RND] = 0;
	ap->lo[FILTR_AMIN]	= 0;
	ap->hi[FILTR_AMIN]	= 1;		// minumum filter amplitude
	ap->default_val[FILTR_AMIN] = .1;
	ap->lo[FILTR_ARND]	= 0;
	ap->hi[FILTR_ARND]	= 1;		// filter amplitude randomisation
	ap->default_val[FILTR_ARND] = 0;
	ap->lo[FILTR_ADIS]	= -1;
	ap->hi[FILTR_ADIS]	= 1;		// filter amp distrib (ascending,descending,random)
	ap->default_val[FILTR_ADIS] = 0;
	if(dz->mode == 1) {
		ap->lo[FILTR_STEP]	= .01;
		ap->hi[FILTR_STEP]	= 3600;		// time step between defined filter states
		ap->default_val[FILTR_STEP] = 1;
		ap->lo[FILTR_SRND]	= 0;
		ap->hi[FILTR_SRND]	= 1;		// randomisation of timestep
		ap->default_val[FILTR_SRND] = 0;
	}
	ap->lo[FILTR_SEED]	= 0;
	ap->hi[FILTR_SEED]	= 512;		// randomn seed value
	ap->default_val[FILTR_SEED] = 0;
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
			if((exit_status = setup_filtrage_application(dz))<0)
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
					sprintf(errstr,"TK sent brktablesize > 0 for input_data_type [%d] not using brktables.\n",dz->input_data_type);
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
//		if(!sloom)
//			fprintf(stderr,errstr);
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
	return(usage2("filtrage"));
}

/**************************** CHECK_FILTRAGE_PARAM_VALIDITY_AND_CONSISTENCY *****************************/

int check_filtrage_param_validity_and_consistency(dataptr dz)
{
	double distrib, factor, randstep;
	if(dz->param[FILTR_MMIN] >= dz->param[FILTR_MMAX]) {
		sprintf(errstr,"INCOMPATIBLE MAXIMUM AND MINIMUM MIDI VALUES.\n");
		return(DATA_ERROR);
	}
	distrib = fabs(dz->param[FILTR_ADIS]);
	if(distrib < 1.0) {
		factor = 1.0 - distrib;					//	each factor of the total cnt of filters, is independintly randpermd in amplitude
		randstep = dz->iparam[FILTR_CNT] * factor;
		if(randstep < 2) {
			if(dz->param[FILTR_ADIS] < 0.0)
				sprintf(errstr,"FILTER DISTRIBUTION TOO CLOSE TO -1\n");
			else 
				sprintf(errstr,"FILTER DISTRIBUTION TOO CLOSE TO 1\n");
			return DATA_ERROR;
		}
	}
	if(dz->iparam[FILTR_SEED] > 0)
		srand((int)dz->iparam[FILTR_SEED]);
	return FINISHED;
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	if(!strcmp(prog_identifier_from_cmdline,"filtrage"))					dz->process = FILTRAGE;
	else {
		sprintf(errstr,"Unknown program identification string '%s'\n",prog_identifier_from_cmdline);
		return(USAGE_ONLY);
	}
	return(FINISHED);
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
	if(!strcmp(str,"filtrage")) {
		fprintf(stdout,
		"USAGE:\n"
		"filtrage filtrage 1 outfiltdata dur cnt min max distrib rand ampmin amprand\n"
		"           ampdistrib [-sseed]\n"
		"OR\n"
		"filtrage filtrage 2 outfiltdata dur cnt min max distrib rand ampmin amprand\n"
		"           ampdistrib timestep timerand [-sseed]\n"
		"\n"
		"Generate randomised varibank filter files.\n"
		"Mode 1 generates a fixed filter: Mode 2 generates a time-varying filter.\n"
		"\n"
		"DUR        Duration spanned by output filter file.\n"
		"CNT        Number of parallel filters.\n"
		"MIN        Minimum MIDI value for filters.\n"
		"MAX        Maximum MIDI value for filters.\n"
		"DISTRIB    Distribution of pitch values. 1 = pitch-linear.\n"
		"           > 1 squeezed towards low pitches : < 1, squeezed towards high pitches.\n"
		"RAND       Randomisation of filter pitches.\n"
		"AMPMIN     Minimum filter amplitude (max = 1.0)\n"
		"AMPRAND    Randomisation of filter amplitudes.\n"
		"AMPDISTRIB Distribution of filter amplitudes.\n"
		"           1  Increasing with pitch :  -1  Decreasing with pitch :  0  Random.\n"
		"           Intermediate values give decreasing degrees of randomisation.\n"
		"TIMESTEP   Timestep between each specified set of filter-pitches.\n"
		"TIMERAND   Randomisation of timestep.\n"
		"SEED       Non-zero values fix the randomisation so that, on repeating the process,\n"
		"           identical random values are reproduced.\n"
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

/************************** FILTRAGE **********************************/

int filtrage(dataptr dz) 
{
	double *fltrmidi = dz->parray[0];
	double *fltramp = dz->parray[1];
	double *fltrdis = dz->parray[2];
	double midirange = dz->param[FILTR_MMAX] - dz->param[FILTR_MMIN];
	double amprange = 1.0 - dz->param[FILTR_AMIN];
	double time = 0.0, steptime = 0.0, offset, frac, hidiff, lodiff, randval, factor, randstep, maxfilter, tempval, distrib;
	int filtcnt = 0, maxfilt, minfilt, n, m, gpcnt;
	char *temp, temp2[128];
	int stringsize = 24;
	int *perm;

	if((temp = (char *)malloc(dz->iparam[FILTR_CNT] * 2 * stringsize * sizeof(char)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for storing filter output line.\n");
		return(MEMORY_ERROR);
	}
	if((perm = (int *)malloc(dz->iparam[FILTR_CNT] * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for permutation array.\n");
		return(MEMORY_ERROR);
	}
	while(steptime <= dz->param[FILTR_DUR] || filtcnt < 2) {
		for(n=0;n < dz->iparam[FILTR_CNT]; n++) {
			frac = (double)n/(double)dz->iparam[FILTR_CNT];
			if(dz->param[FILTR_DIS] != 1.0)
				frac = pow(frac,dz->param[FILTR_DIS]);
			fltrmidi[n] = frac;
		}
		for(n=0;n < dz->iparam[FILTR_CNT]; n++) {
			fltrmidi[n] *= midirange;
			fltrmidi[n] += dz->param[FILTR_MMIN];
		}
		if(dz->param[FILTR_RND] != 0.0) {
			for(n=1;n < dz->iparam[FILTR_CNT]-1; n++) {
				lodiff = fltrmidi[n] - fltrmidi[n-1];
				hidiff = fltrmidi[n+1] - fltrmidi[n];
				randval = ((drand48() * 2.0) - 1.0)/2.0;		//	+- 1/2
				if(randval >= 0.0)
					fltrdis[n] = fltrmidi[n] + (hidiff * randval * dz->param[FILTR_RND]);
				else
					fltrdis[n] = fltrmidi[n] - (lodiff * randval * dz->param[FILTR_RND]);
			}
			hidiff = fltrmidi[1] - fltrmidi[0];
			randval = drand48()/2.0;		//	0 to 1/2
			fltrdis[0] = fltrmidi[0] + (hidiff * randval * dz->param[FILTR_RND]);
			lodiff = fltrmidi[dz->iparam[FILTR_CNT] - 1] - fltrmidi[dz->iparam[FILTR_CNT] - 2];
			randval = drand48()/2.0;		//	0 to 1/2
			fltrdis[dz->iparam[FILTR_CNT] - 1] = fltrmidi[dz->iparam[FILTR_CNT] - 1] - (lodiff * randval * dz->param[FILTR_RND]);
			for(n=0;n < dz->iparam[FILTR_CNT]; n++)
				fltrmidi[n] = fltrdis[n];
		}
		for(n=0;n < dz->iparam[FILTR_CNT]; n++) {					//	Linear or warped distrib, increasing
			frac = (double)n/(double)(dz->iparam[FILTR_CNT] - 1);
			fltramp[n] = frac * amprange;
			fltramp[n] += dz->param[FILTR_AMIN];
		}	
		if(dz->param[FILTR_ADIS] < 0) {							//	Sort to decreasing order
			for(n=0,m=dz->iparam[FILTR_CNT]-1;n < dz->iparam[FILTR_CNT]/2; n++,m--) {
				tempval = fltramp[n];
				fltramp[n] = fltramp[m];
				fltramp[m] = tempval;
			}
		}
		if(dz->param[FILTR_ARND] != 0.0) {							//	Randomise, still in increasing-or-decresing-order
			for(n=1;n < dz->iparam[FILTR_CNT]-1; n++) {
				lodiff = fltramp[n] - fltramp[n-1];
				hidiff = fltramp[n+1] - fltramp[n];
				randval = ((drand48() * 2.0) - 1.0)/2.0;		//	+- 1/2
				if(randval >= 0.0)
					fltrdis[n] = fltramp[n] + (hidiff * randval * dz->param[FILTR_ARND]);
				else
					fltrdis[n] = fltramp[n] - (lodiff * randval * dz->param[FILTR_ARND]);
			}
			hidiff = fltramp[1] - fltramp[0];
			randval = drand48()/2.0;		//	0 to 1/2
			fltrdis[0] = fltramp[0] + (hidiff * randval * dz->param[FILTR_ARND]);
			lodiff = fltramp[dz->iparam[FILTR_CNT] - 1] - fltramp[dz->iparam[FILTR_CNT] - 2];
			randval = drand48()/2.0;		//	0 to 1/2
			fltrdis[dz->iparam[FILTR_CNT] - 1] = fltramp[dz->iparam[FILTR_CNT] - 1] - (lodiff * randval * dz->param[FILTR_ARND]);
			for(n=0;n < dz->iparam[FILTR_CNT]; n++)
				fltramp[n] = fltrdis[n];
		}
		distrib = fabs(dz->param[FILTR_ADIS]);
		if(distrib < 1.0) {							//	Randomly redistribute amplitudes
			factor = 1.0 - distrib;					//	each factor of the total cnt of filters, is independintly randpermd in amplitude
			randstep = dz->iparam[FILTR_CNT] * factor;			//	This counts blocks of filters to be randpermd
			maxfilter = 0.0;
			maxfilt = 0;
			do {
				minfilt = maxfilt;
				maxfilter += randstep;
				maxfilt = round(maxfilter);
				maxfilt = min(maxfilt,dz->iparam[FILTR_CNT]);
				gpcnt = maxfilt - minfilt;
				if(gpcnt > 1) {
					rndintperm(perm,gpcnt);
					for(n=0;n<gpcnt;n++)				//	Randperm the amplitudes
						fltrdis[minfilt+n]  = fltramp[minfilt + perm[n]];
					for(n=0;n<gpcnt;n++)
						fltramp[minfilt+n]  = fltrdis[minfilt+n];
				}
			} while(maxfilter < dz->iparam[FILTR_CNT]);
		}
		sprintf(temp,"%lf",time);
		for(n=0;n<dz->iparam[FILTR_CNT];n++) {
			sprintf(temp2,"  %lf  %lf",fltrmidi[n],fltramp[n]);
			strcat(temp,temp2);
		}
		strcat(temp,"\n");
		if(fputs(temp,dz->fp)<0) {
			sprintf(errstr,"FAILED TO WRITE FILTER DATA TO FILE\n");
			return SYSTEM_ERROR;
		}
		if(dz->mode == 0) {
			time = dz->param[FILTR_DUR];
			sprintf(temp,"%lf",time);
			for(n=0;n<dz->iparam[FILTR_CNT];n++) {
				sprintf(temp2,"  %lf  %lf",fltrmidi[n],fltramp[n]);
				strcat(temp,temp2);
			}
			strcat(temp,"\n");
			if(fputs(temp,dz->fp)<0) {
				sprintf(errstr,"FAILED TO WRITE FILTER DATA TO FILE\n");
				return SYSTEM_ERROR;
			}
			break;	
		}
		steptime += dz->param[FILTR_STEP];
		if(dz->param[FILTR_SRND] > 0.0) {
			randval = ((drand48() * 2.0) - 1.0)/2.0;		//	+- 1/2
			offset = dz->param[FILTR_STEP] * randval * dz->param[FILTR_SRND];
			time = steptime + offset;
		} else
			time = steptime;
		filtcnt++;
	}
	return FINISHED;
}

/*********************** RNDINTPERM ************************/

void rndintperm(int *perm,int cnt)
{
	int n,t,k;
	memset((char *)perm,0,cnt * sizeof(int));
	for(n=0;n<cnt;n++) {
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

