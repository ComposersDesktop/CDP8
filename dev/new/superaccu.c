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
#include <cdpmain.h>
#include <tkglobals.h>
#include <pnames.h>
#include <focus.h>
#include <processno.h>
#include <modeno.h>
#include <globcon.h>
#include <logic.h>
#include <filetype.h>
#include <mixxcon.h>
#include <flags.h>
#include <speccon.h>
#include <arrays.h>
#include <special.h>
#include <focus.h>
#include <sfsys.h>
#include <osbind.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <srates.h>
#include <float.h>
#include <standalone.h>

#ifdef unix
#define round lround
#endif

#define SUPACDECAY (0)	//  Rate of decay of sustained partials
#define SUPACGLIS  (1)	//	Rate of glis of sustained partials
#define SUPACREASS (0)	//	Reassign glissed partials to appropriate channels

#define TUNING_CENT_UP (1.00057779)
#define TUNING_CENT_DOWN (0.999422543649)

char errstr[2400];

int anal_infiles = 1;
int	sloom = 0;
int sloombatch = 0;

const char* cdp_version = "7.1.0";

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

/* CDP LIB FUNCTION MODIFIED TO AVOID CALLING setup_particular_application() */

static int  parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);

/* SIMPLIFICATION OF LIB FUNC TO APPLY TO JUST THIS FUNCTION */

static int  parse_infile_and_check_type(char **cmdline,dataptr dz);
static int  handle_the_outfile(int *cmdlinecnt,char ***cmdline,int is_launched,dataptr dz);
static int  handle_the_special_data(int *cmdlinecnt,char ***cmdline,dataptr dz);
static int  setup_the_application(dataptr dz);
static int  setup_the_param_ranges_and_defaults(dataptr dz);
static int  setup_and_init_input_brktable_constants(dataptr dz,int brkcnt);
static int  get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz);
static int  get_the_mode_no(char *str, dataptr dz);
static int  this_outer_loop(dataptr dz);
static int  this_inner_loop(int windows_in_buf,dataptr dz);

/* BYPASS LIBRARY GLOBAL FUNCTION TO GO DIRECTLY TO SPECIFIC APPLIC FUNCTIONS */

static int   superaccu(int checkstop, dataptr dz);
static int   rwd_accumulate(int index,float *flbufptr, float *windowbuf,dataptr dz);
static float tune(float hz,dataptr dz);

static int  setup_tempered_data(dataptr dz);
static int  read_tuning_data(char *filename,dataptr dz);

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
		if((exit_status = make_initial_cmdline_check(&argc,&argv))<0) {		// CDP LIB
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		cmdline    = argv;
		cmdlinecnt = argc;
		if((get_the_process_no(argv[0],dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		cmdline++;
		cmdlinecnt--;
		dz->maxmode = 4;
		if((get_the_mode_no(cmdline[0],dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		cmdline++;
		cmdlinecnt--;
		if((exit_status = setup_the_application(dz))<0) {
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
	if((exit_status = setup_the_param_ranges_and_defaults(dz))<0) {
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

	// handle_outfile() = 
	if((exit_status = handle_the_outfile(&cmdlinecnt,&cmdline,is_launched,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if(dz->mode > 0) {
		if ((dz->parray = (double **)malloc(sizeof(double *)))==NULL) {
			sprintf(errstr,"Insufficient memory to store tuning data pointer\n");
			return(MEMORY_ERROR);
		}
	}

//	handle_formants()			redundant
//	handle_formant_quiksearch()	redundant
//	handle_special_data()		redundant except
	if(dz->mode > 1) {
		if((exit_status = handle_the_special_data(&cmdlinecnt,&cmdline,dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
	} else if(dz->mode == 1) {
		if((exit_status = setup_tempered_data(dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
	} else 
		dz->itemcnt = 0;
	if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {		// CDP LIB
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	//check_param_validity_and_consistency .....
	is_launched = TRUE;

	//allocate_large_buffers() ... replaced by	CDP LIB
	dz->extra_bufcnt =  1;
	dz->bptrcnt = 4;
	if((exit_status = establish_spec_bufptrs_and_extra_buffers(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = allocate_double_buffer(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((dz->windowbuf[0] = (float *)malloc(sizeof(float) * dz->buflen))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for extra float sample buffer.\n");
		return(MEMORY_ERROR);
	}
		
	//param_preprocess()						redundant
	if((exit_status = param_preprocess(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	//spec_process_file =
	dz->total_windows = 0;
	if((exit_status = this_outer_loop(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if(exit_status < 0) {
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

/************************ HANDLE_THE_EXTRA_INFILE *********************/

int handle_the_extra_infile(char ***cmdline,int *cmdlinecnt,dataptr dz)
{
					/* OPEN ONE EXTRA ANALFILE, CHECK COMPATIBILITY */
	int  exit_status;
	char *filename;
	fileptr fp2;
	int fileno = 1;
	double maxamp, maxloc;
	int maxrep;
	int getmax = 0, getmaxinfo = 0;
	infileptr ifp;
	fileptr fp1 = dz->infile; 
	filename = (*cmdline)[0];
	if((dz->ifd[fileno] = sndopenEx(filename,0,CDP_OPEN_RDONLY)) < 0) {
		sprintf(errstr,"cannot open input file %s to read data.\n",filename);
		return(DATA_ERROR);
	}	
	if((ifp = (infileptr)malloc(sizeof(struct filedata)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store data on later infile. (1)\n");
		return(MEMORY_ERROR);
	}
	if((fp2 = (fileptr)malloc(sizeof(struct fileprops)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store data on later infile. (2)\n");
		return(MEMORY_ERROR);
	}
	if((exit_status = readhead(ifp,dz->ifd[1],filename,&maxamp,&maxloc,&maxrep,getmax,getmaxinfo))<0)
		return(exit_status);
	copy_to_fileptr(ifp,fp2);
	if(fp2->filetype != ANALFILE) {
		sprintf(errstr,"%s is not an analysis file.\n",filename);
		return(DATA_ERROR);
	}
	if(fp2->origstype != fp1->origstype) {
		sprintf(errstr,"Incompatible original-sample-type in input file %s.\n",filename);
		return(DATA_ERROR);
	}
	if(fp2->origrate != fp1->origrate) {
		sprintf(errstr,"Incompatible original-sample-rate in input file %s.\n",filename);
		return(DATA_ERROR);
	}
	if(fp2->arate != fp1->arate) {
		sprintf(errstr,"Incompatible analysis-sample-rate in input file %s.\n",filename);
		return(DATA_ERROR);
	}
	if(fp2->Mlen != fp1->Mlen) {
		sprintf(errstr,"Incompatible analysis-window-length in input file %s.\n",filename);
		return(DATA_ERROR);
	}
	if(fp2->Dfac != fp1->Dfac) {
		sprintf(errstr,"Incompatible decimation factor in input file %s.\n",filename);
		return(DATA_ERROR);
	}
	if(fp2->channels != fp1->channels) {
		sprintf(errstr,"Incompatible channel-count in input file %s.\n",filename);
		return(DATA_ERROR);
	}
	if((dz->insams[fileno] = sndsizeEx(dz->ifd[fileno]))<0) {	    /* FIND SIZE OF FILE */
		sprintf(errstr, "Can't read size of input file %s.\n"
		"open_checktype_getsize_and_compareheader()\n",filename);
		return(PROGRAM_ERROR);
	}
	if(dz->insams[fileno]==0) {
		sprintf(errstr, "File %s contains no data.\n",filename);
		return(DATA_ERROR);
	}
	(*cmdline)++;
	(*cmdlinecnt)--;
	return(FINISHED);
}

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
	dz->infilecnt = ONE_NONSND_FILE;
	//establish_bufptrs_and_extra_buffers():
	dz->bufcnt  = 0;
	dz->extra_bufcnt =  1; 
	dz->bptrcnt = 1;
	if((exit_status = establish_spec_bufptrs_and_extra_buffers(dz))<0)	  
			return(exit_status);
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
	int exit_status;
	char *filename = NULL;
	filename = (*cmdline)[0];
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

/************************* SETUP_THE_APPLICATION *******************/

int setup_the_application(dataptr dz)
{
	int exit_status;
	aplptr ap;
	if((exit_status = establish_application(dz))<0)		// GLOBAL
		return(FAILED);
	ap = dz->application;
	// SEE parstruct FOR EXPLANATION of next 2 functions
	if(dz->mode > 1) {
		if((exit_status = set_param_data(ap,TUNING,0,0,""))<0)
			return(FAILED);
	} else {
		if((exit_status = set_param_data(ap,0   ,0,0,""))<0)
			return(FAILED);
	}
	if((exit_status = set_vflgs(ap,"dg",2,"DD","r",1,0,"0"))<0)
		return(FAILED);
	// set_legal_infile_structure -->
	dz->has_otherfile = FALSE;
	// assign_process_logic -->
	dz->input_data_type = ANALFILE_ONLY;
	dz->process_type	= BIG_ANALFILE;	
	dz->outfiletype  	= ANALFILE_OUT;
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
		} else if(infile_info->filetype != ANALFILE)  {
			sprintf(errstr,"File %s is not of correct type\n",cmdline[0]);
			return(DATA_ERROR);
		} else if((exit_status = copy_parse_info_to_main_structure(infile_info,dz))<0) {
			sprintf(errstr,"Failed to copy file parsing information\n");
			return(PROGRAM_ERROR);
		}
		free(infile_info);
	}
	dz->clength		= dz->wanted / 2;
	dz->chwidth 	= dz->nyquist/(double)(dz->clength-1);
	dz->halfchwidth = dz->chwidth/2.0;
	return(FINISHED);
}

/************************* SETUP_THE_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_the_param_ranges_and_defaults(dataptr dz)
{
	int exit_status;
	aplptr ap = dz->application;
	// set_param_ranges()
	ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
	// NB total_input_param_cnt is > 0 !!!s
	if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
		return(FAILED);
	// get_param_ranges()
	ap->lo[SUPACDECAY]			= 0.00001;
	ap->hi[SUPACDECAY]			= 0.9;
	ap->default_val[SUPACDECAY]	= 0.01;
	ap->lo[SUPACGLIS]			= -MAXGLISRATE/dz->frametime;
	ap->hi[SUPACGLIS]			= MAXGLISRATE/dz->frametime;
	ap->default_val[SUPACGLIS]	= 0;
	dz->maxmode = 0;
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
			if((exit_status = setup_the_application(dz))<0)
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

/*********************** PARAM_PREPROCESS *********************/

int param_preprocess(dataptr dz)
{
	int n;
	double *p;
		   		/* decayratio per sec -> decayratio per window */
	if(dz->brksize[SUPACDECAY]) {
		p = dz->brk[SUPACDECAY] + 1;
		for(n=0;n<dz->brksize[SUPACDECAY];n++) {
			*p = exp(log(*p) * dz->frametime);		
			 p += 2;
		}
	} else
		dz->param[SUPACDECAY] = exp(log(dz->param[SUPACDECAY]) * dz->frametime);

			  	/* glis in 8va per sec -> frqratio per window */
	if(dz->brksize[SUPACGLIS]) {
		p = dz->brk[SUPACGLIS] + 1;
		for(n=0;n<dz->brksize[SUPACGLIS];n++) {
			*p = pow(2.0,*p * dz->frametime);
			 p += 2;
		}
	} else
		dz->param[SUPACGLIS] = pow(2.0,dz->param[SUPACGLIS] * dz->frametime);
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

int inner_loop(int *peakscore,int *descnt,int *in_start_portion,int *least,int *pitchcnt,int windows_in_buf,dataptr dz)
{
	return FINISHED;
}


/******************************** USAGE1 ********************************/

int usage1(void)
{
	return usage2("superaccu");
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
	if(!strcmp(str,"superaccu")) {
		fprintf(stdout,
		"superaccu superaccu 1 inanalfile outanalfile [-ddecay] [-gglis] [-r]\n"
		"superaccu superaccu 2 inanalfile outanalfile [-ddecay] [-gglis] [-r]\n"
		"superaccu superaccu 3 inanalfile outanalfile tuning [-ddecay] [-gglis] [-r]\n"
		"superaccu superaccu 4 inanalfile outanalfile tuning [-ddecay] [-gglis] [-r]\n"
		"\n"
		"SUSTAIN EACH SPECTRAL BAND, UNTIL LOUDER DATA APPEARS IN THAT BAND\n"
		"\n"
		"MODE 2 forces (start of) resonances to tempered scale.\n"
		"\n"
		"For modes 3 and 4 (start of ) resonances are forced to ...\n"
		"\n"
		"MODE 3 frqs specified in \"tuning\" (harmonic set).\n"
		"MODE 4 frqs and their 8vas, specified in \"tuning\" (harmonic field).\n"
		"\n"
		"\n"
		"TUNING Set of frequencies for tuning the resonance (starts)\n"
		"-DECAY sutained channel data attenuates by factor DECAY per sec.\n"
		"      (Possible Range : >0.0 to 0.9 : Default 1.0 (no attenuation))\n"
		"-GLIS  sutained channel data glisses at GLIS 8vas per sec.\n"
		"      (Approx Range : -11.7 to 11.7 : Default 0)\n"
		"-r     reassign the glissing pitches to appropriate channels.\n");
	} else
		fprintf(stdout,"Unknown option '%s'\n",str);
	return(USAGE_ONLY);
}


int usage3(char *str1,char *str2)
{
	fprintf(stderr,"Insufficient parameters on command line.\n");
	return(USAGE_ONLY);
}

/**************************** THIS_OUTER_LOOP ****************************/

int this_outer_loop(dataptr dz)
{
	int exit_status, continuing = 0, cc, vc;
	int samps_read, got, windows_in_buf, cnt;
	dz->time = 0.0f;
	if(dz->bptrcnt <= 0) {
		sprintf(errstr,"flbufptr[0] not established by this_outer_loop()\n");
		return(PROGRAM_ERROR);
	}
    while((samps_read = fgetfbufEx(dz->bigfbuf, dz->buflen,dz->ifd[0],0)) > 0) {
    	got = samps_read;
    	dz->flbufptr[0] = dz->bigfbuf;
    	windows_in_buf = got/dz->wanted;
   		if((exit_status = this_inner_loop(windows_in_buf,dz))<0)
			return(exit_status);
		continuing = exit_status;
		if((exit_status = write_exact_samps(dz->bigfbuf,samps_read,dz))<0)
			return(exit_status);
	}
	if(samps_read < 0) {
		sprintf(errstr,"Sound read error.\n");
		return(SYSTEM_ERROR);
	}
   	dz->flbufptr[0] = dz->bigfbuf;
	if(continuing) {
		fprintf(stdout,"INFO: creating resonant tail\n");
		fflush(stdout);
	}
	cnt = 0;
	while(continuing) {
		for(cc = 0, vc = 0; cc < dz->clength; cc++,  vc += 2)
			dz->flbufptr[0][AMPP] = 0.0f;
		if((exit_status = read_values_from_all_existing_brktables((double)dz->time,dz))<0)
			return(exit_status);
		if((exit_status = superaccu(1,dz))<0)
			return(exit_status);
		continuing = exit_status;
		if((exit_status = write_exact_samps(dz->bigfbuf,dz->wanted,dz))<0)
			return(exit_status);
		dz->time = (float)(dz->time + dz->frametime);
	}
	return(FINISHED);
}

/**************************** THIS_INNER_LOOP ****************************/

int this_inner_loop(int windows_in_buf,dataptr dz)
{
	int exit_status = FINISHED;
	int wc;

   	for(wc=0; wc<windows_in_buf; wc++) {
		if(dz->total_windows==0) {
			if((exit_status = skip_or_special_operation_on_window_zero(dz))<0)
				return(exit_status);
			if(exit_status==TRUE) {
				dz->flbufptr[0] += dz->wanted;
				dz->total_windows++;
				dz->time = (float)(dz->time + dz->frametime);
				continue;
			}
		}
		if((exit_status = read_values_from_all_existing_brktables((double)dz->time,dz))<0)
			return(exit_status);

		if((exit_status = superaccu(0,dz))<0)
			return(exit_status);
		dz->flbufptr[0] += dz->wanted;
		dz->total_windows++;
		dz->time = (float)(dz->time + dz->frametime);
	}
	return(exit_status);
}

/***************** SKIP_OR_SPECIAL_OPERATION_ON_WINDOW_ZERO ************/

int skip_or_special_operation_on_window_zero(dataptr dz)
{
	int vc;
	for(vc = 0; vc < dz->wanted; vc += 2) {
		dz->windowbuf[0][AMPP] = dz->flbufptr[0][AMPP];

	}
	return(TRUE);
}

/********************************** SUPERACCU **********************************/

int superaccu(int checkstop,dataptr dz)
{
	int exit_status;
	int vc, cc, truevc,reassigned = 0, lastampi, nextampi, lastfrqi, nextfrqi;
	float amp, frq, trueamp, lolim, hilim;
	float *storebuf = dz->flbufptr[2];
	double sum;
	if(dz->vflag[SUPACREASS])	/* Retain initially-read values of source */
		memcpy((char *)storebuf,(char *)dz->flbufptr[0],dz->buflen * sizeof(float));
	if(dz->param[SUPACGLIS] > 0.0) {
		if(dz->param[SUPACDECAY] < 1.0) {
			for(cc = 0, vc = 0; cc < dz->clength; cc++,  vc += 2) {
				dz->windowbuf[0][AMPP] = (float)(dz->windowbuf[0][AMPP] * dz->param[SUPACDECAY]);
				dz->windowbuf[0][FREQ] = (float)(dz->windowbuf[0][FREQ] * dz->param[SUPACGLIS]);
				if((exit_status = rwd_accumulate(vc,dz->flbufptr[0],dz->windowbuf[0],dz))<0)
					return(exit_status);
			}
		} else {
			for(cc = 0, vc = 0; cc < dz->clength; cc++,  vc += 2) {
				dz->windowbuf[0][FREQ] = (float)(dz->windowbuf[0][FREQ] * dz->param[SUPACGLIS]);
				if((exit_status = rwd_accumulate(vc,dz->flbufptr[0],dz->windowbuf[0],dz))<0)
					return(exit_status);
			}
		}
	} else if(dz->param[SUPACDECAY] < 1.0) {
		for(cc = 0, vc = 0; cc < dz->clength; cc++,  vc += 2) {
			dz->windowbuf[0][AMPP] = (float)(dz->windowbuf[0][AMPP] * dz->param[SUPACDECAY]);
			if((exit_status = rwd_accumulate(vc,dz->flbufptr[0],dz->windowbuf[0],dz))<0)
				return(exit_status);
		}
	} else {	 
		for(cc = 0, vc = 0; cc < dz->clength; cc++,  vc += 2) {
			if((exit_status = rwd_accumulate(vc,dz->flbufptr[0],dz->windowbuf[0],dz))<0)
				return(exit_status);
		}
	}
	if(dz->vflag[SUPACREASS] && dz->vflag[SUPACGLIS]) {	/* Reassign to better chans, only if glissing */
		lolim = (float)-dz->chwidth;
		reassigned = 0;
		for(cc = 0, vc = 0; cc < dz->clength; cc++,  vc += 2) {
			hilim = (float)(lolim + dz->chwidth);
			amp = dz->flbufptr[0][AMPP];
			frq = dz->flbufptr[0][FREQ];
			if(frq < lolim || frq > hilim) {							/* if frq lies outside channel span */
				truevc = (int)round(frq/dz->chwidth) * 2;
				if(vc != truevc) {
					trueamp = dz->flbufptr[0][truevc];
					if(trueamp < amp) {
						dz->flbufptr[0][truevc]   = amp;				/* Move glissing pitch to other channel */
						dz->flbufptr[0][truevc+1] = frq;
						dz->flbufptr[0][AMPP] = storebuf[AMPP];	/* Restore input value of abandoned channel */
						dz->flbufptr[0][FREQ] = storebuf[FREQ];
						reassigned = 1;
					}
				}
			}
			lolim = (float)(lolim + dz->chwidth);
		}
		if(reassigned) {
			for(cc = 1, vc = 2; cc < dz->clength-1; cc++,  vc += 2) {
				lastampi = vc - 2;
				nextampi = vc + 2;			/* If this channel is a local peak, force sidechans to same frq */
				lastfrqi = vc - 1;
				nextfrqi = vc + 3;			/* If this channel is a local peak, force sidechans to same frq */
				if((dz->flbufptr[0][AMPP] > (3 * dz->flbufptr[0][lastampi])) && (dz->flbufptr[0][AMPP] > (3 * dz->flbufptr[0][nextampi]))) {
					dz->flbufptr[0][lastfrqi] = dz->flbufptr[0][FREQ];
					dz->flbufptr[0][nextfrqi] = dz->flbufptr[0][FREQ];
				}
			}
		}
	}
	if(checkstop) {
		sum = 0.0;
		for(cc = 0, vc = 0; cc < dz->clength; cc++,  vc += 2)
			sum += dz->flbufptr[0][AMPP];
		if(flteq(sum,0.0))
			return(FINISHED);
	}
	return(CONTINUE);
}

/********************************** RWD_ACCUMULATE **********************************/

int rwd_accumulate(int index,float *flbufptr, float *windowbuf,dataptr dz)
{
	int frq = index+1;
	if(flbufptr[index] > windowbuf[index])  {	 /* if current amp > amp in accumulator */
		windowbuf[index] = flbufptr[index]; 
		windowbuf[frq] = flbufptr[frq];	 /* replace amp in accumulator with current amp */
		if(dz->itemcnt)
			windowbuf[frq] = tune(windowbuf[frq],dz);
	} else {
		flbufptr[index] = windowbuf[index]; 	 /* else replace current amp with amp in accumulator */
		flbufptr[frq] = windowbuf[frq];								
	}
	return(FINISHED);
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	if      (!strcmp(prog_identifier_from_cmdline,"superaccu"))		dz->process = SUPERACCU;
	else {
		sprintf(errstr,"Unknown program identification string '%s'\n",prog_identifier_from_cmdline);
		return(USAGE_ONLY);
	}
	return(FINISHED);
}

/************************** TUNE *****************************/

float tune(float hz, dataptr dz)
{
	int n;
	double mindiff, thisdiff, outval;
	float val;
	mindiff = fabs(dz->parray[0][0] - hz);
	outval = dz->parray[0][0];
	for(n=1;n<dz->itemcnt;n++) {
		thisdiff = fabs(dz->parray[0][n] - hz);
		if(thisdiff < mindiff) {
			mindiff = thisdiff;
			outval = dz->parray[0][n];
		}
	}
	val = (float)outval;
	return(val);
}

/************************ GET_THE_MODE_NO *********************/

int get_the_mode_no(char *str, dataptr dz)
{
	if(sscanf(str,"%d",&dz->mode)!=1) {
		sprintf(errstr,"Cannot read mode of program.\n");
		return(USAGE_ONLY);
	}
	if(dz->mode <= 0 || dz->mode > dz->maxmode) {
		sprintf(errstr,"Program mode value [%d] is out of range [1 - %d].\n",dz->mode,dz->maxmode);
		return(USAGE_ONLY);
	}
	dz->mode--;		/* CHANGE TO INTERNAL REPRESENTATION OF MODE NO */
	return(FINISHED);
}

/************************ READ_TUNING_DATA *********************/

int read_tuning_data(char *filename,dataptr dz)
{
	aplptr ap = dz->application;
	int n, m, maxcnt, endcnt, got;
	char temp[200], *q;
	double *p, dummy, frq, origfrq, intune;
	ap->data_in_file_only 	= TRUE;
	ap->special_range 		= TRUE;
	ap->min_special 		= SPEC_MINFRQ;
	ap->max_special 		= (dz->nyquist * 2.0)/3.0;
	if((dz->fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,"Cannot open datafile %s\n",filename);
		return(DATA_ERROR);
	}
	n = 0;
	while(fgets(temp,200,dz->fp)!=NULL) {
		q = temp;
		if(is_an_empty_line_or_a_comment(q))
			continue;
		while(get_float_from_within_string(&q,&dummy))
			n++;
	}
	if(n==0) {
		sprintf(errstr,"No data in file %s\n",filename);
		return(DATA_ERROR);
	}
	dz->itemcnt = n;
	maxcnt = dz->itemcnt * 16;
	if(dz->mode == 3) {
		if ((dz->parray[0] = (double *)malloc(maxcnt * sizeof(double)))==NULL) {
			sprintf(errstr,"Insufficient memory to store tuning data\n");
			return(MEMORY_ERROR);
		}
	} else {
		if ((dz->parray[0] = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL) {
			sprintf(errstr,"Insufficient memory to store tuning data\n");
			return(MEMORY_ERROR);
		}
	}
	p = dz->parray[0];
	if(fseek(dz->fp,0,0)<0) {
		sprintf(errstr,"fseek() failed in read_tuning_data()\n");
		return(SYSTEM_ERROR);
	}
	n = 0;
	while(fgets(temp,200,dz->fp)!=NULL) {
		q = temp;
		if(is_an_empty_line_or_a_comment(temp))
			continue;
		if(n >= dz->itemcnt) {
			sprintf(errstr,"Accounting problem read_tuning_data\n");
			return(PROGRAM_ERROR);
		}
		while(get_float_from_within_string(&q,&dummy)) {
			dummy = miditohz(dummy);
			if(dummy <= ap->min_special || dummy >= ap->max_special) {
				sprintf(errstr,"Pitch out of range (%lf - %lf) : line %d: file %s\n",ap->min_special,ap->max_special,n+1,filename);
				return(DATA_ERROR);
			}
			*p = dummy;
			p++;
		}
		n++;
	}
	dz->itemcnt = n;
	if(fclose(dz->fp)<0) {
		fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
		fflush(stdout);
	}
	if(dz->mode == 3) {		//	Expand harmonic set over whole harmonic fielsd
		endcnt = dz->itemcnt;
		for(n=0;n<dz->itemcnt;n++) {
			origfrq = dz->parray[0][n];
			frq = origfrq/2.0;
			while(frq > SPEC_MINFRQ) {
				got = 0;
				for(m = 0; m < endcnt;m++) {
					intune = frq/dz->parray[0][m];
					if(intune > TUNING_CENT_DOWN  && intune  < TUNING_CENT_UP) {
						got = 1;
						break;
					}
				}
				if(!got) {
					dz->parray[0][endcnt] = frq; 
					if(++endcnt >= maxcnt) {
						sprintf(errstr,"Insufficient storage space allotted for harmonic field frequencies\n");
						return(PROGRAM_ERROR);
					}
				}
				frq /= 2.0;
			}
			frq = origfrq * 2.0;
			while(frq < dz->nyquist) {
				got = 0;
				for(m = 0; m < endcnt;m++) {
					intune = frq/dz->parray[0][m];
					if(intune > TUNING_CENT_DOWN  && intune  < TUNING_CENT_UP) {
						got = 1;
						break;
					}
				}
				if(!got) {
					dz->parray[0][endcnt] = frq; 
					if(++endcnt >= maxcnt) {
						sprintf(errstr,"Insufficient storage space allotted for harmonic field frequencies\n");
						return(PROGRAM_ERROR);
					}
				}
				frq *= 2.0;
			}
		}
		dz->itemcnt = endcnt;
	}
	return(FINISHED);
}

/************************ SETUP_TEMPERED_DATA *********************/

int setup_tempered_data(dataptr dz)
{
	int n;
	dz->itemcnt = 128;
	n = 0;
	if ((dz->parray[0] = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL) {
		sprintf(errstr,"Insufficient memory to store tuning data\n");
		return(MEMORY_ERROR);
	}
	while(n < 128)  {
		dz->parray[0][n] = miditohz(n);
		n++;
	}
	return(FINISHED);
}

/************************ HANDLE_THE_SPECIAL_DATA *********************/

int handle_the_special_data(int *cmdlinecnt,char ***cmdline,dataptr dz)
{
	int exit_status;
	if(!sloom) {
		if(*cmdlinecnt <= 0) {
			sprintf(errstr,"Insufficient parameters on command line.\n");
			return(USAGE_ONLY);
		}
	}
	if((exit_status = read_tuning_data((*cmdline)[0],dz))<0)
		return(exit_status);
	(*cmdline)++;		
	(*cmdlinecnt)--;
	return(FINISHED);
}

