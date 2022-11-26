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

#define CHAN_SRCHRANGE_F		(4)
#define DEFAULT_FORMANT_BANDS	(4)
#define MIN_TOOBIG_RATIO		(1000.0)

#define INANAL0		0
#define INANAL1		1
#define OUTANAL		2
#define SPECENVTOP	3
#define SPECENVFRQ	4
#define SPECENVPCH	5
#define SPECENVAMP	6
#define SPECENVAMP2	7
#define ACCUOCTS	8

#define TOP_OF_LOW_OCTAVE_BANDS 4
#define CHAN_ABOVE_LOW_OCTAVES  8

#ifdef unix
#define round lround
#endif

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
static int	handle_the_extra_infile(char ***cmdline,int *cmdlinecnt,dataptr dz);
static int  open_the_outfile(dataptr dz);
static int  setup_the_application(dataptr dz);
static int  setup_the_param_ranges_and_defaults(dataptr dz);
static int  get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz);
static int  get_the_mode_no(char *str, dataptr dz);
static int  setup_and_init_input_brktable_constants(dataptr dz,int brkcnt);

/* BYPASS LIBRARY GLOBAL FUNCTION TO GO DIRECTLY TO SPECIFIC APPLIC FUNCTIONS */

static int spectwin(dataptr dz);
static int allocate_spectwin_buffers(dataptr dz);
static int extract_specenv(int bufptr_no,int storeno,dataptr dz);
static int set_specenv_frqs(int arraycnt,dataptr dz);
static int setup_octaveband_steps(double **interval,dataptr dz);
static int setup_low_octave_bands(int arraycnt,dataptr dz);
static int getspecenvamp(double *thisamp,float thisfrq,int storeno,dataptr dz);
static int check_spectwin_param_validity_and_consistency(dataptr dz);

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
		if(argc < 5) {
			sprintf(errstr,"Too few commandline parameters.\n");
			return(FAILED);
		}
		cmdline    = argv;
		cmdlinecnt = argc;
		if((get_the_process_no(argv[0],dz))<0)
			return(FAILED);
		cmdline++;
		cmdlinecnt--;
		dz->maxmode = 4;
		if((get_the_mode_no(cmdline[0],dz))<0) {
			if(!sloom)
				fprintf(stderr,"%s",errstr);
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

//	handle_extra_infiles() =
	if((exit_status = handle_the_extra_infile(&cmdline,&cmdlinecnt,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	// handle_outfile() = 
	if((exit_status = handle_the_outfile(&cmdlinecnt,&cmdline,is_launched,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}

//	handle_formants()			redundant
//	handle_formant_quiksearch()	redundant
//	handle_special_data()		redundant except
	if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {		// CDP LIB
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = check_spectwin_param_validity_and_consistency(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	dz->extra_bufcnt = 0;
	dz->bptrcnt = 9;
	if((exit_status = establish_spec_bufptrs_and_extra_buffers(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = allocate_spectwin_buffers(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = open_the_outfile(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	is_launched = TRUE;
	//param_preprocess()						redundant
	if((exit_status = spectwin(dz))<0) {
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
	char *filename = (*cmdline)[0], *p;
	strcpy(dz->outfilename,filename);
	p = dz->outfilename + strlen(dz->outfilename);
	p--;
	while(p > dz->outfilename) {
		if(*p == '.') {
			*p = ENDOFSTR;
			break;
		}
		p--;
	}
	strcat(dz->outfilename,".ana");
	(*cmdline)++;
	(*cmdlinecnt)--;
	return(FINISHED);
}

/***************************** OPEN_THE_OUTFILE **************************/

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

/************************* SETUP_THE_APPLICATION *******************/

int setup_the_application(dataptr dz)
{
	int exit_status;
	aplptr ap;
	if((exit_status = establish_application(dz))<0)		// GLOBAL
		return(FAILED);
	ap = dz->application;
	// SEE parstruct FOR EXPLANATION of next 2 functions
	if((exit_status = set_param_data(ap,0,0,0,""))<0)
		return(FAILED);
	if((exit_status = set_vflgs(ap,"efdsr",5,"DDidd","",0,0,""))<0)
		return(FAILED);
		// set_legal_infile_structure -->
	dz->has_otherfile = FALSE;
	// assign_process_logic -->
	dz->input_data_type = TWO_ANALFILES;
	dz->process_type	= MIN_ANALFILE;	
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
	dz->nyquist		= dz->infile->origrate/2.0;
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
	ap->lo[0]			= 0;
	ap->hi[0]			= 1;
	ap->default_val[0]	= 1;
	ap->lo[1]			= 0;
	ap->hi[1]			= 1;
	ap->default_val[1]	= 1;
	ap->lo[2]			= 0;
	ap->hi[2]			= 8;
	ap->default_val[2]	= 0;
	ap->lo[3]			= 0;
	ap->hi[3]			= 48;
	ap->default_val[3]	= 0;
	ap->lo[4]			= 0;
	ap->hi[4]			= 1;
	ap->default_val[4]	= 1;
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

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	if      (!strcmp(prog_identifier_from_cmdline,"spectwin"))		dz->process = SPECTWIN;
	else {
		sprintf(errstr,"Unknown program identification string '%s'\n",prog_identifier_from_cmdline);
		return(USAGE_ONLY);
	}
	return(FINISHED);
}

/******************************** USAGE1 ********************************/

int usage1(void)
{
	return(usage2("spectwin"));
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
	if(!strcmp(str,"spectwin")) {
		fprintf(stdout,
	    "USAGE: spectwin spectwin\n"
		"       1-4 inanal1 inanal2 outanal [-ffrqint] [-eenvint] [-ddupl -sstep -rdec]\n"
		"\n"
		"FRQINT   Dominance of spectral freqs of infile2 (range  0-1: dflt 1.0).\n"
		"ENVINT   Dominance of spectral envelope of infile2 (range  0-1: dflt 1.0).\n"
		"DUPL     Duplicate original sound-1 \"dupl\" times, at higher pitches.\n"
		"STEP     pitchstep (semitones) at each duplication.\n"
		"DEC      Level multiplier from one transposition to next, in duplications.\n"
		"\n"
		"MODE 1  Formant envelope of file 1 v Formant envelope of file 2.\n"
		"MODE 2  Formant envelope of file 1 v Total envelope of file 2.\n"
		"MODE 3  Total envelope of file 1 v Formant envelope of file 2.\n"
		"MODE 4  Total envelope of file 1 v total envelope of file 2.\n"
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

/************************ SPECTWIN *********************/

int spectwin(dataptr dz)
{
	int exit_status, cc, vc, n, chan, newcc;
	double fi = dz->param[TWIN_FRQINTP], ei = dz->param[TWIN_ENVINTP];
	float *ibuf0 = dz->flbufptr[INANAL0], *ibuf1 = dz->flbufptr[INANAL1], *obuf = dz->flbufptr[OUTANAL];
	float *accu = dz->flbufptr[ACCUOCTS];
	double thisamp0, thisamp1, rolloff, frqmult;
	float amp, frq;
	int sampsread0, sampsread1;
	int OK  = 1;
	dz->total_windows = 0;
	if(sloom)
		dz->total_samps_read = 0L;
	dz->time = 0.0f;
	while(OK) {
		if((sampsread0 = fgetfbufEx(ibuf0, dz->buflen,dz->ifd[0],0)) < 0) {
			sprintf(errstr,"Can't read samples from input soundfile 1.\n");
			return(SYSTEM_ERROR);
		}
		rectify_window(ibuf0,dz);
		if(dz->iparam[2] > 0) {
			memcpy((char *)accu,(char *)ibuf0,dz->buflen * sizeof(float));
			for(cc=0,vc=0;cc <dz->clength;cc++,vc+=2) {
				amp = ibuf0[AMPP];
				rolloff = dz->param[4];
				for(n = 1;n <= dz->iparam[2];n++) {
					frqmult = pow(2.0,((dz->param[3] * n)/SEMITONES_PER_OCTAVE));
					frq = (float)(ibuf0[FREQ] * frqmult);
					if(frq >= dz->nyquist)
						break;
					chan = (int)floor(frq/dz->chwidth);
					newcc = chan * 2;
					if(amp * rolloff > accu[newcc]) {
						accu[newcc]   = amp;
						accu[newcc+1] = frq;
					}
					rolloff *= dz->param[4];
				}
			}
			memcpy((char *)ibuf0,(char *)accu,dz->buflen * sizeof(float));
		}
		if((sampsread1 = fgetfbufEx(ibuf1, dz->buflen,dz->ifd[1],0)) < 0) {
			sprintf(errstr,"Can't read samples from input soundfile 2.\n");
			return(SYSTEM_ERROR);
		}
		dz->ssampsread = min(sampsread0,sampsread1);
		dz->samps_left -= dz->ssampsread;	
		dz->total_samps_read += dz->ssampsread;
		if(dz->ssampsread <= 0)
			break;
		rectify_window(ibuf1,dz);
		if(dz->total_windows==0)
			memset((char *)obuf,0,dz->buflen * sizeof(float));
		else {
			if((exit_status = read_values_from_all_existing_brktables(dz->time,dz))<0)
				return(exit_status);
			switch(dz->mode) {
			case(0):
				if((exit_status = extract_specenv(0,0,dz))<0)
					return exit_status;
				if((exit_status = extract_specenv(1,1,dz))<0)
					return exit_status;
				for(cc=0,vc=0;cc <dz->clength;cc++,vc+=2) {
					if((exit_status= getspecenvamp(&thisamp0,ibuf0[FREQ],0,dz)) < 0)
						return exit_status;
					if((exit_status= getspecenvamp(&thisamp1,ibuf1[FREQ],1,dz)) < 0)
						return exit_status;
					obuf[AMPP] = (float)((thisamp0 * (1-ei)) + (thisamp1 * ei));
					obuf[FREQ] = (float)((ibuf0[FREQ] * (1-fi)) + (ibuf1[FREQ] * fi));
				}
				break;
			case(1):
				if((exit_status = extract_specenv(0,0,dz))<0)
					return exit_status;
				for(cc=0,vc=0;cc <dz->clength;cc++,vc+=2) {
					if((exit_status= getspecenvamp(&thisamp0,ibuf0[FREQ],0,dz)) < 0)
						return exit_status;
					obuf[AMPP] = (float)((thisamp0 * (1-ei)) + (ibuf1[AMPP] * ei));
					obuf[FREQ] = (float)((ibuf0[FREQ] * (1-fi)) + (ibuf1[FREQ] * fi));
				}
				break;
			case(2):
				if((exit_status = extract_specenv(1,1,dz))<0)
					return exit_status;
				for(cc=0,vc=0;cc <dz->clength;cc++,vc+=2) {
					if((exit_status= getspecenvamp(&thisamp1,ibuf1[FREQ],1,dz)) < 0)
						return exit_status;
				obuf[AMPP] = (float)((ibuf0[AMPP] * (1-ei)) + (thisamp1 * ei));
					obuf[FREQ] = (float)((ibuf0[FREQ] * (1-fi)) + (ibuf1[FREQ] * fi));
				}
				break;
			case(3):
				for(cc=0,vc=0;cc <dz->clength;cc++,vc+=2) {
					obuf[AMPP] = (float)((ibuf0[AMPP] * (1-ei)) + (ibuf1[AMPP] * ei));
					obuf[FREQ] = (float)((ibuf0[FREQ] * (1-fi)) + (ibuf1[FREQ] * fi));
				}
				break;
			}
		}
		if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
			return(exit_status);
		dz->total_windows++;
		dz->time = (float)(dz->time + dz->frametime);
	}
	return(FINISHED);
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

/********************** EXTRACT_SPECENV *******************/

int extract_specenv(int bufptr_no,int storeno,dataptr dz)	 /* bufptr_no = flbufptr number (usually 0) */
{
	int    n, cc, vc, specenvcnt_less_one;
	int    botchan, topchan;
	double botfreq, topfreq;
	double bwidth_in_chans;
	float *ampstore;
	switch(storeno) {
	case(0):	ampstore = dz->specenvamp;	break;
	case(1):	ampstore = dz->specenvamp2;	break;
	default:
		sprintf(errstr,"Unknown storenumber in extract_specenv()\n");
		return(PROGRAM_ERROR);
	}
	specenvcnt_less_one = dz->infile->specenvcnt - 1;
	vc = 0;
	for(n=0;n<dz->buflen;n++)
		ampstore[n] = (float)0.0;
	topfreq = 0.0f;
	n = 0;
	while(n < specenvcnt_less_one) {
		botfreq  = topfreq;
		botchan  = (int)((botfreq + dz->halfchwidth)/dz->chwidth); /* TRUNCATE */
		botchan -= CHAN_SRCHRANGE_F;
		botchan  =  max(botchan,0);
		topfreq  = dz->specenvtop[n];
		topchan  = (int)((topfreq + dz->halfchwidth)/dz->chwidth); /* TRUNCATE */
		topchan += CHAN_SRCHRANGE_F;
		topchan  =  min(topchan,dz->clength);
		for(cc = botchan,vc = botchan * 2; cc < topchan; cc++,vc += 2) {
			if(dz->flbufptr[bufptr_no][FREQ] >= botfreq && dz->flbufptr[bufptr_no][FREQ] < topfreq)
				ampstore[n] = (float)(ampstore[n] + dz->flbufptr[bufptr_no][AMPP]);
		}
		bwidth_in_chans   = (double)(topfreq - botfreq)/dz->chwidth;
		ampstore[n] = (float)(ampstore[n]/bwidth_in_chans);
		n++;
	}
	return(FINISHED);
}

/*************************** ALLOCATE_ANALDATA_PLUS_FORMANTDATA_BUFFER ****************************/

int allocate_spectwin_buffers(dataptr dz)
{
	int exit_status, n;
	/*int cnt = 0;*/
	if(dz->bptrcnt < 9) {
		sprintf(errstr,"Insufficient bufptrs established in allocate_analdata_plus_formantdata_buffer()\n");
		return(PROGRAM_ERROR);
	}
	dz->buflen = dz->wanted;
	if((dz->bigfbuf = (float*)malloc(dz->buflen * dz->bptrcnt * sizeof(float)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for sound buffers.\n");
		return(MEMORY_ERROR);
	}
	dz->big_fsize  = dz->buflen;
	dz->flbufptr[0] = dz->bigfbuf;
	for(n = 1;n < 9; n++)
		dz->flbufptr[n] = dz->flbufptr[n-1] + dz->buflen;
	dz->specenvtop  = dz->flbufptr[SPECENVTOP];
	dz->specenvfrq  = dz->flbufptr[SPECENVFRQ];
	dz->specenvpch  = dz->flbufptr[SPECENVPCH];
	dz->specenvamp  = dz->flbufptr[SPECENVAMP];
	dz->specenvamp2 = dz->flbufptr[SPECENVAMP2];
	memset((char *)dz->specenvpch, 0,dz->buflen * sizeof(float));
	memset((char *)dz->specenvfrq, 0,dz->buflen * sizeof(float));
	memset((char *)dz->specenvtop, 0,dz->buflen * sizeof(float));
	memset((char *)dz->specenvamp, 0,dz->buflen * sizeof(float));
	memset((char *)dz->specenvamp2,0,dz->buflen * sizeof(float));
	dz->clength = dz->infile->channels/2;
	if((exit_status = set_specenv_frqs(dz->clength + 1,dz))<0)
		return(exit_status);
	return(FINISHED);
}

/************************ SET_SPECENV_FRQS ************************
 *
 * FREQWISE BANDS = number of channels for each specenv point
 * PICHWISE BANDS  = number of points per octave
 */

int set_specenv_frqs(int arraycnt,dataptr dz)
{
	int exit_status;
	double bandbot, *interval;
	int m, n, k = 0, cc;
	dz->formant_bands = DEFAULT_FORMANT_BANDS;
	if((exit_status = setup_octaveband_steps(&interval,dz))<0)
		return(exit_status);
	if((exit_status = setup_low_octave_bands(arraycnt,dz))<0)
		return(exit_status);
	k  = TOP_OF_LOW_OCTAVE_BANDS;
	cc = CHAN_ABOVE_LOW_OCTAVES;
	while(cc <= dz->clength) {
		m = 0;
		if((bandbot = dz->chwidth * (double)cc) >= dz->nyquist)
			break;
		for(n=0;n<dz->formant_bands;n++) {
			if(k >= arraycnt) {
				sprintf(errstr,"Formant array too small: set_specenv_frqs()\n");
				return(PROGRAM_ERROR);
			}
			dz->specenvfrq[k]   = (float)(bandbot * interval[m++]);
			dz->specenvpch[k]   = (float)log10(dz->specenvfrq[k]);
			dz->specenvtop[k++] = (float)(bandbot * interval[m++]);
		}	
		cc *= 2; 		/* 8-16: 16-32: 32-64 etc as 8vas, then split into bands */
	}
	dz->specenvfrq[k]  = (float)dz->nyquist;
	dz->specenvpch[k]  = (float)log10(dz->nyquist);
	dz->specenvtop[k]  = (float)dz->nyquist;
	k++;
	dz->infile->specenvcnt = k;
	return(FINISHED);
}

/************************* SETUP_OCTAVEBAND_STEPS ************************/

int setup_octaveband_steps(double **interval,dataptr dz)
{
	double octave_step;
	int n = 1, m = 0, halfbands_per_octave = dz->formant_bands * 2;
	if((*interval = (double *)malloc(halfbands_per_octave * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY establishing interval array for formants.\n");
		return(MEMORY_ERROR);
	}
	while(n < halfbands_per_octave) {
		octave_step   = (double)n++/(double)halfbands_per_octave;
		(*interval)[m++] = pow(2.0,octave_step);
	}
	(*interval)[m] = 2.0;
	return(FINISHED);
}

/************************ SETUP_LOW_OCTAVE_BANDS ***********************
 *
 * Lowest PVOC channels span larger freq steps and therefore we must
 * group them in octave bands, rather than in anything smaller.
 */

int setup_low_octave_bands(int arraycnt,dataptr dz)
{
	if(arraycnt < LOW_OCTAVE_BANDS) {
		sprintf(errstr,"Insufficient array space for low_octave_bands\n");
		return(PROGRAM_ERROR);
	}
	dz->specenvfrq[0] = (float)1.0;					/* frq whose log is 0 */
	dz->specenvpch[0] = (float)0.0;
	dz->specenvtop[0] = (float)dz->chwidth;			/* 8VA wide bands */
	dz->specenvfrq[1] = (float)(dz->chwidth * 1.5); /* Centre Chs 1-2 */	
	dz->specenvpch[1] = (float)log10(dz->specenvfrq[1]);
	dz->specenvtop[1] = (float)(dz->chwidth * 2.0);
	dz->specenvfrq[2] = (float)(dz->chwidth * 3.0); /* Centre Chs 2-4 */
	dz->specenvpch[2] = (float)log10(dz->specenvfrq[2]);
	dz->specenvtop[2] = (float)(dz->chwidth * 4.0);
	dz->specenvfrq[3] = (float)(dz->chwidth * 6.0); /* Centre Chs 4-8 */	
	dz->specenvpch[3] = (float)log10(dz->specenvfrq[3]);
	dz->specenvtop[3] = (float)(dz->chwidth * 8.0);
	return(FINISHED);
}

/**************************** GETSPECENVAMP *************************/

int getspecenvamp(double *thisamp,float thisfrq,int storeno,dataptr dz)
{
	double pp, ratio, ampdiff;
	float *ampstore;
	int z = 1;
	if(thisfrq<0.0)	{ 	/* NOT SURE THIS IS CORRECT */
		*thisamp = 0.0;	/* SHOULD WE PHASE INVERT & RETURN A -ve AMP ?? */
		return(FINISHED);	
	}
	if(thisfrq<=1.0)
		pp = 0.0;
	else
		pp = log10(thisfrq); 
	while( dz->specenvpch[z] < pp){
		z++;
		/*RWD may need to trap on size of array? */
		if(z == dz->infile->specenvcnt -1)
			break;
	}
	switch(storeno) {
	case(0):	
		ampstore = dz->specenvamp;	break;
	case(1):	
		ampstore = dz->specenvamp2;	break;
	default:
		sprintf(errstr,"Unknown storenumber in getspecenvamp()\n");
		return(PROGRAM_ERROR);
	}
	ratio    = (pp - dz->specenvpch[z-1])/(dz->specenvpch[z] - dz->specenvpch[z-1]);
	ampdiff  = ampstore[z] - ampstore[z-1];
	*thisamp = ampstore[z-1] + (ampdiff * ratio);
	*thisamp = max(0.0,*thisamp);
	return(FINISHED);
}

int check_spectwin_param_validity_and_consistency(dataptr dz)
{
	int n;
	double frqmult;
	for(n = 1;n <= dz->iparam[2];n++) {
		frqmult = pow(2.0,((dz->param[3] * n)/SEMITONES_PER_OCTAVE));
		if(frqmult > 65536.0) {
			sprintf(errstr,"Transposition of %d * %lf semitones ( = %lf octaves) too great.\n",n,dz->param[3],(dz->param[3] * n)/SEMITONES_PER_OCTAVE);
			return(DATA_ERROR);
		}
	}
	return FINISHED;
}