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



// TODO

// so far I've got FE_WINDOWS (mode 4) outputting time frq amp TRIPLES .. converted to samps frq color
// AND these ARE SNACK displayed in Features window.
//	calling SNACK with evv(SN_FEATURES_PEAKS)
// and displaying the SPECTRUM and the peak data on TOP OF THE SPECTRAL DISPLAY, to see if it is credible !!!!
//
// cmdline::												DEFAULT
//						     lof hif mindist-btwwn peaks  scan-wsize  no-of-pks	   no-of-windows	at-time	 sort-anal-files-to-frq-order
// features get 4 infile.ana 350 8000		43				500			4			14				.5			-s
//
// NOTICEABLE THAT ISOLATED POINTS COULD BE DELETED
// isolated means no point within next 4(3?) windows is within 500/2 = 250 Hz of the point
//
//

// Create Sloom display for modes 6,5,4
// Tesat on vocal data!!
// If outputs seem reasonable, do code for modes 1,2,3

/*
 * mode 1: EDIT TIMES OUTPUT: times to edit out longest OR loudest example of each feature.
 * mode 2: EDIT TIMES OUTPUT: times to edit out every example of J marks
 * mode 3: ENVEL OUTPUT: times to envelope everything except the marked items.
 * mode 4: TEST MODE: creates data display of N windows at time K
 * mode 5: TEST OUTPUT: outputs peakfrqs and spectral amplitudes to put on display and compare.
 * mode 6: TEST MODE: creates data display of average peaks over spectrum from time A to time B
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
#include <speccon.h>
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

#define EQUALFRQ 1

char errstr[2400];

int anal_infiles = 1;
int	sloom = 0;
int sloombatch = 0;

const char* cdp_version = "7.1.0";

#define MIDIMINFRQ (8.175799)
#define MIDIMAXFRQ (12543.853951)
#define MIDDLE_C_FRQ	(261.625565)
#define	MIN_FE_DUR		(10)	/* minimum feature length (mS) */
#define DFLT_MAX_FE_LEN	(600)	/* max length of a feature (mS) */
#define FE_MAX_WIN		(14)	/* resolution on a 900 pixel wide display with 60 semitones */
#define DFLT_PK_ERROR	(2.0)	/* semitones */
#define DFLT_PK_STEP	(1.0)	/* min step between peaks (semitones) */
#define MAJOR_3RD		(4.0)	/* semitones */
#define FE_MAX_PKCNT	(6)
#define DFLT_PEAKS_CNT	(3)		/* no of formant peaks to search for */
#define FE_CNT_MAX		(1000)	/* max no of different features to search for */
#define FE_CNT_TYPICAL	(40)	/* typical no of different features to search for */
#define	FE_TAILMAX		(1000)
#define	FE_TAILDUR		MIN_FE_DUR
#define FE_SPLICEMAX	(50)
#define FE_SPLICELEN	(5)

#define FE_MINWINDOW (5)

#define DFLT_MAX_AVWIN_SIZE (1000.0)
#define FE_WINSIZ_DFLT		(500.0)

#define FE_LOFRQ	(0)
#define FE_HIFRQ	(1)
#define FE_STEP		(2)
#define FE_WINSIZE	(3)
#define FE_PKCNT	(4)

#define FE_WINCNT	(5)
#define FE_WINTIME	(6)

#define FE_START	(5)
#define FE_END		(6)

#define FE_MIN		(5)
#define FE_MAX		(6)
#define FE_ERROR	(7)
#define FE_FECNT	(8)
#define FE_TAIL		(9)
#define FE_SPLICE	(10)

/* modes */

#define FE_BEST		(0)
#define FE_EVERY	(1)
#define FE_ENVEL	(2)
#define FE_WINDOWS	(3)
#define FE_AVERAGE	(4)
#define FE_CHECK	(5)

#define FE_SORTBUF 0
#define FE_LONGEST 0

#define MIN_DB	-96.0

/* frequencies should lie in specific channels in the PVOC data */
/* in case this is not true, look in channels a little above and a little below the channels in which frq expected to be found */
/* The 'little' is defined in semitones ..... or windows if EQUALFRQ defined */
#define FRQ_SRCH_ERRORBND	(4)


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
static int  setup_the_application(dataptr dz);
static int  setup_the_param_ranges_and_defaults(dataptr dz);
static int	check_the_param_validity_and_consistency(dataptr dz);
static int  get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz);
static int get_the_mode_no(char *str, dataptr dz);
static int  setup_and_init_input_brktable_constants(dataptr dz,int brkcnt);

/* BYPASS LIBRARY GLOBAL FUNCTION TO GO DIRECTLY TO SPECIFIC APPLIC FUNCTIONS */

static int features_scan(dataptr dz);
static double semit_to_ratio(double val);
static int peaks_in_window(double botfrq,double intv_to_bot_of_srch,double intv_to_top_of_srch,double intv_to_centre_of_window,double intv_to_top_of_window,
				double intv_to_next_window,int *lookformax,double *lastmaxamp,int *lastmaxampchan,double *lastmaxampchanfrq,
				double *lastminamp,int *lastminampchan,double *lastminampchanfrq,int *win_peakchan,float *buf,dataptr dz);
static int locate_pitch_centre_of_each_peak(int peakandtrofcnt,int *win_peakchan,float *win_peakamp,float *win_peakfrq,double footfrq,
				double intv_errorbnd,double intv_to_top_of_window,double intv_to_centre_of_window,double intv_to_next_window,
				float *buf,dataptr dz);
static void frqsort_buf(float *buf,dataptr dz);
//static float amp_to_db(double amp, double silence);
//static void tellme(char *str);

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
		if((get_the_process_no(argv[0],dz))<0)
			return(FAILED);
		cmdline++;
		cmdlinecnt--;
		dz->maxmode = 6;
		if(cmdlinecnt <= 0) {
			sprintf(errstr,"Too few commandline parameters.\n");
			return(FAILED);
		}
		if((get_the_mode_no(cmdline[0],dz))<0)
			return(FAILED);
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
		switch(dz->mode) {
		case(FE_CHECK):
			if(cmdlinecnt < 7) {
				fprintf(stderr,"Too few commandline parameters.\n");
				return(FAILED);
			} else if(cmdlinecnt > 8) {
				fprintf(stderr,"Too many commandline parameters.\n");
				return(FAILED);
			}
			break;
		case(FE_WINDOWS):
		case(FE_AVERAGE):
			if(cmdlinecnt < 8) {
				fprintf(stderr,"Too few commandline parameters.\n");
				return(FAILED);
			} else if(cmdlinecnt > 9) {
				fprintf(stderr,"Too many commandline parameters.\n");
				return(FAILED);
			}
			break;
		case(FE_BEST):
			if(cmdlinecnt < 13) {
				fprintf(stderr,"Too few commandline parameters.\n");
				return(FAILED);
			} else if(cmdlinecnt > 15) {
				fprintf(stderr,"Too many commandline parameters.\n");
				return(FAILED);
			}
			break;
		case(FE_EVERY):
		case(FE_ENVEL):
			if(cmdlinecnt < 13) {
				fprintf(stderr,"Too few commandline parameters.\n");
				return(FAILED);
			} else if(cmdlinecnt > 14) {
				fprintf(stderr,"Too many commandline parameters.\n");
				return(FAILED);
			}
			break;
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
	if(dz->mode < 3) {	/* modes 4 & 5 stream output to stdout */
		if((exit_status = handle_the_outfile(&cmdlinecnt,&cmdline,is_launched,dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
	}

//	handle_formants()			redundant
//	handle_formant_quiksearch()	redundant
//	handle_special_data()		redundant
 
	if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {		// CDP LIB
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	//check_param_validity_and_consistency .....
	if((exit_status = check_the_param_validity_and_consistency(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	is_launched = TRUE;

	//allocate_large_buffers() ... replaced by	CDP LIB
	dz->extra_bufcnt = 0;
	dz->bptrcnt		 = 1;
	if((exit_status = establish_spec_bufptrs_and_extra_buffers(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = allocate_single_buffer(dz)) < 0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	//param_preprocess()						redundant
	//spec_process_file =
	if((exit_status = features_scan(dz)) < 0) {
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
	dz->infilecnt = ONE_NONSND_FILE;
	//establish_bufptrs_and_extra_buffers():
	dz->array_cnt=2;
	if((dz->parray  = (double **)malloc(dz->array_cnt * sizeof(double *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for internal double arrays.\n");
		return(MEMORY_ERROR);
	}
	dz->parray[0] = NULL;
	dz->parray[1] = NULL;
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
	int exit_status, hasext = 0;
	char *filename = NULL;
	char ext[64], *p;
	strcpy(ext,".txt");			 /* default extension */
	filename = (*cmdline)[0];
	strcpy(dz->outfilename,filename);	   
	p = dz->outfilename;
	while(*p != ENDOFSTR) {
		if(*p == '.') {			 /* overwrite dflt extension with user's extension, if it exists */
			hasext = 1;
			break;
		}
		p++;
	}
	switch(dz->mode) {
	case(FE_EVERY):
	case(FE_ENVEL):
		if(!sloom) {
			if(hasext) {
				p = dz->outfilename;
				while(*p != ENDOFSTR) {
					if(*p == '.') {			 /* overwrite dflt extension with user's extension, if it exists */
						strcpy(ext,p);
						*p = ENDOFSTR;		 /* remove any extension from outfilename */
						break;
					}
					p++;
				}
			}
			strcat(dz->outfilename,"0"); /* add numbering to 1st outfilename */
			strcat(dz->outfilename,ext); /* replace extension on outfilename */
			hasext = 1;
		}
		/* fall thro */
	case(FE_BEST):
		if(!sloom && !hasext)
			strcat(dz->outfilename,ext);
		if((exit_status = create_sized_outfile(dz->outfilename,dz))<0)
			return(exit_status);
		break;
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

/************************* SETUP_THE_APPLICATION *******************/

int setup_the_application(dataptr dz)
{
	int exit_status;
	aplptr ap;
	if((exit_status = establish_application(dz))<0)		// GLOBAL
		return(FAILED);
	ap = dz->application;
	dz->input_data_type = ANALFILE_ONLY;
	// SEE parstruct FOR EXPLANATION of next 2 functions
	switch(dz->mode) {
	case(FE_WINDOWS):
		/* mode 1: TEST MODE: creates data display of N windows at time K */
		if((exit_status = set_param_data(ap,0   ,11,7,"ddddiid0000"))<0)	/* , lofrq, hifrq limits, N windows, time K*/
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","s",1,0,"0"))<0)
			return(FAILED);
		dz->process_type	= OTHER_PROCESS;	
		dz->outfiletype  	= NO_OUTPUTFILE;
		break;
	case(FE_AVERAGE):
		/* mode 2: TEST MODE: creates data display of average peaks over spectrum from time A to time B */
		if((exit_status = set_param_data(ap,0   ,11,7,"ddddidd0000"))<0)	/* lofrq, hifrq limits, time A, time B */
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","s",1,0,"0"))<0)
			return(FAILED);
		dz->process_type	= OTHER_PROCESS;	
		dz->outfiletype  	= NO_OUTPUTFILE;
		break;
	case(FE_CHECK):
		/* mode 2: TEST MODE: display analysis window against peak data */
		if((exit_status = set_param_data(ap,0   ,11,6,"ddddid00000"))<0)	/* lofrq, hifrq limits, time A, time B */
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","s",1,0,"0"))<0)
			return(FAILED);
		dz->process_type	= OTHER_PROCESS;	
		dz->outfiletype  	= NO_OUTPUTFILE;
		break;
	case(FE_BEST):
		/* mode 3: EDIT TIMES OUTPUT: times to edit out loudest (or longest) example of each feature. */
		if((exit_status = set_param_data(ap,0   ,11,11,"ddddidddidd"))<0)	/* lofrq, hifrq limits , 9 params */
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","sd",2,0,"00"))<0)	/* longest, rather than loudest, feature */
			return(FAILED);
		dz->process_type	= TO_TEXTFILE;	
		dz->outfiletype  	= TEXTFILE_OUT;
		break;
	case(FE_EVERY):
		/* mode 4: EDIT TIMES OUTPUT: times to edit out every example of J marks */
		if((exit_status = set_param_data(ap,0   ,11,11,"ddddidddidd"))<0)	/* lofrq, hifrq limits , 9 params + no of marks to display */
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","s",1,0,"0"))<0)
			return(FAILED);
		dz->process_type	= TO_TEXTFILE;	
		dz->outfiletype  	= TEXTFILE_OUT;
		break;
	case(FE_ENVEL):
		/* mode 5: ENVEL OUTPUT: times to envelope away everything except the marked items. */
		if((exit_status = set_param_data(ap,0   ,11,11,"ddddidddidd"))<0)	/* lofrq, hifrq limits , 9 params */
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","s",1,0,"0"))<0)
			return(FAILED);
		dz->process_type	= TO_TEXTFILE;	
		dz->outfiletype  	= TEXTFILE_OUT;
		break;
	default:
		sprintf(errstr,"Unknown mode\n");
		return(DATA_ERROR);
	}
	dz->has_otherfile = FALSE;
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
	dz->chwidth 	= dz->nyquist/(double)dz->clength;
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
	ap->lo[FE_LOFRQ]			= MIDIMINFRQ;	/* lofrq limit of peak search */
	ap->hi[FE_LOFRQ]			= MIDIMAXFRQ;
	ap->default_val[FE_LOFRQ]	= MIDDLE_C_FRQ;
	ap->lo[FE_HIFRQ]			= MIDIMINFRQ;	/* hifrq limit of peak search */
	ap->hi[FE_HIFRQ]			= MIDIMAXFRQ;
	ap->default_val[FE_HIFRQ]	= MIDIMAXFRQ;
#ifdef EQUALFRQ
	ap->lo[FE_STEP]				= dz->chwidth;			/* stepsize for peak search (hz) */
	ap->hi[FE_STEP]				= dz->nyquist / 2.0;
	ap->default_val[FE_STEP]	= dz->chwidth;
	ap->lo[FE_WINSIZE]			= dz->chwidth;			/* sizeof window over which amps averaged to find peak location (hz) */
	ap->hi[FE_WINSIZE]			= DFLT_MAX_AVWIN_SIZE;
	ap->default_val[FE_WINSIZE]	= FE_WINSIZ_DFLT;
#else
	ap->lo[FE_STEP]				= 0.1;			/* stepsize for peak search (semitones) */
	ap->hi[FE_STEP]				= SEMITONES_PER_OCTAVE;
	ap->default_val[FE_STEP]	= DFLT_PK_STEP;
	ap->lo[FE_WINSIZE]			= 0.1;			/* sizeof window over which amps averaged to find peak location (semitones) */
	ap->hi[FE_WINSIZE]			= SEMITONES_PER_OCTAVE * 2.0;
	ap->default_val[FE_WINSIZE]	= MAJOR_3RD;
#endif
	ap->lo[FE_PKCNT]			= 1;			/* no of peaks to search for */
	ap->hi[FE_PKCNT]			= FE_MAX_PKCNT;
	ap->default_val[FE_PKCNT]	= DFLT_PEAKS_CNT;
	switch(dz->mode) {
		case(FE_WINDOWS):
		/* mode 1: TEST MODE: creates data display of N windows at time K */
		ap->lo[FE_WINCNT]			= 1;			/* no of windows to display */
		ap->hi[FE_WINCNT]			= dz->wlength;
		ap->default_val[FE_WINCNT]	= 1;
		ap->lo[FE_WINTIME]			= 0.0;			/* time of first window to display */
		ap->hi[FE_WINTIME]			= dz->duration;
		ap->default_val[FE_WINTIME]	= dz->duration/2.0;
		break;
	case(FE_AVERAGE):
		/* mode 2: TEST MODE: creates data display of average peaks over spectrum from time A to time B */
		ap->lo[FE_START]			= 0.0;			/* start time in sound */
		ap->hi[FE_START]			= dz->duration;
		ap->default_val[FE_START]	= 0.0;
		ap->lo[FE_END]				= 0.0;			/* end time in sound */
		ap->hi[FE_END]				= dz->duration;
		ap->default_val[FE_END]		= dz->duration;
		break;
	case(FE_CHECK):
		/* mode 2: TEST MODE: creates data display of average peaks over spectrum from time A to time B */
		ap->lo[FE_START]			= 0.0;			/* start time in sound */
		ap->hi[FE_START]			= dz->duration;
		ap->default_val[FE_START]	= dz->duration/2.0;
		break;
	case(FE_BEST):
	case(FE_EVERY):
	case(FE_ENVEL):
		ap->lo[FE_MIN]				= dz->frametime * SECS_TO_MS;	/* min dur of any feature to take note of (ms) */
		ap->hi[FE_MIN]				= dz->duration  * SECS_TO_MS;
		ap->default_val[FE_MIN]		= (int)round((MIN_FE_DUR * MS_TO_SECS)/dz->frametime) * dz->frametime * SECS_TO_MS;
		ap->lo[FE_MAX]				= dz->frametime * SECS_TO_MS;	/* max dur of any feature to take note of (ms) */
		ap->hi[FE_MAX]				= dz->duration  * SECS_TO_MS;
		ap->default_val[FE_MAX]		= DFLT_MAX_FE_LEN;
		ap->lo[FE_ERROR]			= 0.0;							/* error in peak equivalence (semitones) */
		ap->hi[FE_ERROR]			= SEMITONES_PER_OCTAVE;
		ap->default_val[FE_ERROR]	= DFLT_PK_ERROR;
		ap->lo[FE_FECNT]			= 1;							/* no of features to rank (in order of number of occurences) */
		ap->hi[FE_FECNT]			= FE_CNT_MAX;
		ap->default_val[FE_FECNT]	= FE_CNT_TYPICAL;
		ap->lo[FE_TAIL]				= 0.0;							/* ms of sound before and after feature to retain when it is edited out */
		ap->hi[FE_TAIL]				= FE_TAILMAX;
		ap->default_val[FE_TAIL]	= FE_TAILDUR;
		ap->lo[FE_SPLICE]			= 1.0;							/* splicelen (mS) */
		ap->hi[FE_SPLICE]			= FE_SPLICEMAX;
		ap->default_val[FE_SPLICE]	= FE_SPLICELEN;
		break;
	}
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

/*********************** CHECK_THE_PARAM_VALIDITY_AND_CONSISTENCY *********************/

int check_the_param_validity_and_consistency(dataptr dz)
{
	double diff;
	if(dz->param[FE_HIFRQ] - dz->param[FE_LOFRQ] < dz->chwidth * 2.0) {
		sprintf(errstr,"Low frequency and high frequency limits of search are too close.");
		return(USER_ERROR);
	}
#ifdef EQUALFRQ
	if(dz->param[FE_HIFRQ] - dz->param[FE_LOFRQ] < dz->param[FE_STEP]) {
		sprintf(errstr,"Low frequency and high frequency limits of search are too close for the stepsize specified.");
		return(USER_ERROR);
	}
#else
	if(dz->param[FE_HIFRQ]/dz->param[FE_LOFRQ] < pow(2.0,(dz->param[FE_STEP] * 2.0)/SEMITONES_PER_OCTAVE)) {
		sprintf(errstr,"Low frequency and high frequency limits of search are too close for the stepsize specified.");
		return(USER_ERROR);
	}
#endif
	switch(dz->mode) {
	case(FE_WINDOWS):
		if(dz->duration - (dz->iparam[FE_WINCNT] * dz->frametime) <= dz->param[FE_WINTIME]) {
			sprintf(errstr,"Time of window to display is too close to end, for the windowcnt specified.");
			return(USER_ERROR);
		}
		dz->iparam[FE_WINTIME] = (int)round(dz->param[FE_WINTIME]/dz->frametime);
		break;
	case(FE_CHECK):
		dz->iparam[FE_START] = (int)round(dz->param[FE_START]/dz->frametime);
		dz->iparam[FE_START] = min(dz->wlength - 1,dz->iparam[FE_START]);
		break;
	case(FE_AVERAGE):
		if((diff = dz->param[FE_END] - dz->param[FE_START]) < 0.0) {
			sprintf(errstr,"Start and End times are incompatible.");
			return(USER_ERROR);
		}
		if((dz->iparam[FE_START] = (int)round(dz->param[FE_START]/dz->frametime)) >= dz->wlength)
			dz->iparam[FE_START] = dz->wlength - 1;
		if((dz->iparam[FE_END]	 = (int)round(dz->param[FE_END]/dz->frametime)) >= dz->wlength)
			dz->iparam[FE_END] = dz->wlength;
		if(dz->iparam[FE_START] == dz->iparam[FE_END])
			dz->iparam[FE_END]++;
		break;
	case(FE_BEST):
	case(FE_EVERY):
	case(FE_ENVEL):
		if(dz->param[FE_MAX] - dz->param[FE_MIN] < 0.0) {
			sprintf(errstr,"Maximum and minimum feature length are incompatible.");
			return(USER_ERROR);
		}
		if(dz->param[FE_STEP] >= dz->param[FE_WINSIZE]) {
			sprintf(errstr,"Step in searching for peaks must be less than peak-window-size.");
			return(USER_ERROR);
		}
		break;
	}
	return FINISHED;
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

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	if (!strcmp(prog_identifier_from_cmdline,"get"))		dz->process = FEATURES;
	else {
		sprintf(errstr,"Unknown program identification string '%s'\n",prog_identifier_from_cmdline);
		return(USAGE_ONLY);
	}
	return(FINISHED);
}

/******************************** USAGE1 ********************************/

int usage1(void)
{
	fprintf(stderr,
	"Use an anlysis file to find the MOST PROMINENT FEATURES in a sound source.\n"
	"\n"
	"Mode 1: Create data to cut best example of each prominent feature from sound source.\n"
	"\n"
	"Mode 2: Create data to cut out all examples of 1 (or more) prominent feature(s).\n"
	"        Creates 1 outfile per output feature.\n"
	"\n"
	"Mode 3: Create data-files to envelope the source so only 1 feature remains.\n"
	"        Creates 1 envelope file per extracted feature.\n"
	"\n"
	"Mode 4: Output peak data for 1 or more windows at a specified time in file.\n"
	"\n"
	"Mode 5: Output statistics showing most prominent peak frqs in time-slot selected.\n"
	"\n"
	"Mode 6: Output display spectrum + peaks, to check if peaks credible.\n"
	"\n"
	"Type 'features get' for more information.\n");
	return(USAGE_ONLY);
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
	if(!strcmp(str,"get")) {
		fprintf(stderr,
	    "USAGE: features get 1-3 inanalfile  outname  lof hif step winsiz pkcnt [PARAMS]\n"
	    "OR:    features get 4-6 inanalfile           lof hif step winsiz pkcnt [PARAMS]\n"
		"\n"
		"To find what each of the 5 modes does, type 'features'.\n"
		"\n"
		"PARAMS are ...\n"
		"(mode 1)   min  max  error  featurecnt  tail  splice [-s] [-d]\n"
		"(mode 2-3) min  max  error  featurecnt  tail  splice [-s] \n"
		"(mode 4)   wincnt  time [-s]\n"
		"(mode 5)   starttime  endtime [-s]\n"
		"(mode 6)   time [-s]\n"
		"\n"
		"LOF        lowest frq to search for spectral peaks (%.0lf - %.0lf: dflt %.0lf)\n"
		"HIF        highest frq to search for spectral peaks (%.0lf - %.0lf: dflt %.0lf)\n"
#ifdef EQUALFRQ
		"STEP       minimum distance between peaks (hz) (chanwidth - nyqist/2: dflt chwdith)\n"
		"WINSIZ     scan-windowsize for peak search (hz) (chanwidth - %.0lf: dflt %.0lf)\n"
#else
		"STEP       minimum distance between peaks (semitones) (%.1lf - %.0lf: dflt %.0lf)\n"
		"WINSIZ     scan-windowsize for peak search (semitones) (%.1lf - %.0lf: dflt %.0lf)\n"
#endif
		"PKCNT      number of peaks to locate (1 - %d: dflt %d)\n"
		"MIN        minimum size of features to find (mS) (%d - filelen: dflt %d)\n"
		"MAX        maximum size of features to find (mS) (%d - filelen: dflt %d)\n"
		"ERROR      acceptable error in equating peaks (semitones) (0 - %.0lf: dflt %.0lf)\n"
		"FEATURECNT max number of features to find (1 - %d: dflt %d)\n"
		"TAIL       length of snd to keep before and after feature (mS) (0 - %d: dflt %d))\n"
		"SPLICE     splicelength for feature extraction (mS) (1 - %d: dflt %d))\n"
		"-s         Sort the analysis data into frq order, before searching for peaks.\n"
		"-d         extract longest examples (default: extract loudest examples)\n"
		"\n"
		"WINCNT    number of windows to output (1 - %d)\n"
		"TIME      time of (first) window to output (0 - filelen).\n"
		"\n"
		"STARTTIME start time in sound (0 - filelen)\n"
		"ENDTIME   end time in sound (0 - filelen)\n"
		"\n",ceil(MIDIMINFRQ),floor(MIDIMAXFRQ),MIDDLE_C_FRQ,ceil(MIDIMINFRQ),floor(MIDIMAXFRQ),floor(MIDIMAXFRQ),
#ifdef EQUALFRQ
		DFLT_MAX_AVWIN_SIZE,FE_WINSIZ_DFLT,
#else
		0.1,SEMITONES_PER_OCTAVE,DFLT_PK_STEP,
		0.1,SEMITONES_PER_OCTAVE * 2.0,MAJOR_3RD,
#endif
		FE_MAX_PKCNT,DFLT_PEAKS_CNT,
		MIN_FE_DUR,MIN_FE_DUR,
		MIN_FE_DUR,DFLT_MAX_FE_LEN,
		SEMITONES_PER_OCTAVE,DFLT_PK_ERROR,
		FE_CNT_MAX,FE_CNT_TYPICAL,
		FE_TAILMAX,FE_TAILDUR,
		FE_SPLICEMAX,FE_SPLICELEN,
		FE_MAX_WIN);
	} else
		fprintf(stdout,"Unknown option '%s'\n",str);
	return(USAGE_ONLY);
}

int usage3(char *str1,char *str2)
{
	fprintf(stderr,"Insufficient parameters on command line.\n");
	return(USAGE_ONLY);
}

/*********************** GET_THE_MODE_NO ***************************/

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

/*********************** FEATURES_SCAN ***************************
 *
 * Examimes each analysis-window, using a peak(searching)-window to find peaks in the analysis-window.
 */

#define SAFETY_MARGIN (8)

int features_scan(dataptr dz)
{

	int wc, cc, vc, n, lastminampchan, lastmaxampchan, lookformax, peakcnt, minpkpos=0, wins_in_buf, OK;
	double lastminamp, lastmaxamp, botfrq, interval;
	double intv_errorbnd, intv_to_next_window, intv_to_centre_of_window, intv_to_top_of_window, intv_to_top_of_srch, intv_to_bot_of_srch;
	double lastmaxampchanfrq, lastminampchanfrq, maxamp, thisval, ampsum, silence;
#ifndef EQUALFRQ
	double minmidi;
#endif
	int *win_peakchan;
	int k, j, startwin, endwin, *peakstats=NULL;
	float **peakamp=NULL, **peakfrq=NULL, *win_peakfrq;
	float *win_peakamp, *totamp=NULL;

	/* ARRAYS FOR FEATURE ANALYSIS */

	if(dz->mode != FE_CHECK) {
		if((peakamp = (float **)malloc(dz->wlength * sizeof(float *)))==NULL) {	/* Array to store all peaks amplitude data */
			sprintf(errstr,"INSUFFICIENT MEMORY to store amplitudes pointers for all possible peaks.\n");
			return(MEMORY_ERROR);
		}
		if((peakfrq = (float **)malloc(dz->wlength * sizeof(float *)))==NULL) {	/* Array to store all peaks frq data */
			sprintf(errstr,"INSUFFICIENT MEMORY to store frequency pointers for all possible peaks.\n");
			return(MEMORY_ERROR);
		}
		for(k = 0;k < dz->wlength; k++) {
			if((peakamp[k] = (float *)malloc(dz->iparam[FE_PKCNT] * sizeof(float)))==NULL) {	/* Arrays to store all peaks amplitude data */
				sprintf(errstr,"INSUFFICIENT MEMORY to store amplitudes for all possible peaks.\n");
				return(MEMORY_ERROR);
			}
			for(j = 0;j < dz->param[FE_PKCNT];j++)
				peakamp[k][j] = (float)0.0;
			if((peakfrq[k] = (float *)malloc(dz->iparam[FE_PKCNT] * sizeof(float)))==NULL) {	/* Arrays to store all peaks frq data */
				sprintf(errstr,"INSUFFICIENT MEMORY to store frequencies for all possible peaks.\n");
				return(MEMORY_ERROR);
			}
			for(j = 0;j < dz->param[FE_PKCNT];j++)
				peakfrq[k][j] = (float)0.0;
		}
	}
	if(dz->mode == FE_WINDOWS) {
		if((totamp = (float *)malloc(dz->wlength * sizeof(float *)))==NULL) {	/* Array to store all WINDOW amplitude data */
			sprintf(errstr,"INSUFFICIENT MEMORY to store window amplitudes for all windows.\n");
			return(MEMORY_ERROR);
		}
		silence = pow(10.0,(MIN_DB/20.0));
	}
	/* ARRAYS AND COINSTANTS FOR PEAK SEARCHING IN ANALYSIS WINDOW */

#ifdef EQUALFRQ
	interval = dz->param[FE_HIFRQ] - dz->param[FE_LOFRQ];
#else
	interval = unchecked_hztomidi(dz->param[FE_HIFRQ]) - unchecked_hztomidi(dz->param[FE_LOFRQ]);
#endif
	peakcnt  = (int)ceil(interval/dz->param[FE_STEP]) + SAFETY_MARGIN;
	if((win_peakchan = (int *)malloc(peakcnt * sizeof(int)))==NULL) {	/* Array to store peaks found in an analysis-window */
		sprintf(errstr,"INSUFFICIENT MEMORY to store window peak channel data.\n");
		return(MEMORY_ERROR);
	}
	if((win_peakamp = (float *)malloc(peakcnt * sizeof(float)))==NULL) {	/* Array to store peaks found in an analysis-window */
		sprintf(errstr,"INSUFFICIENT MEMORY to store window peak amplitude data.\n");
		return(MEMORY_ERROR);
	}
	if((win_peakfrq = (float *)malloc(peakcnt * sizeof(float)))==NULL) {	/* Array to store peaks found in an analysis-window */
		sprintf(errstr,"INSUFFICIENT MEMORY to store window peak frequency data.\n");
		return(MEMORY_ERROR);
	}
	if(dz->mode == FE_AVERAGE) {
		if((peakstats = (int *)malloc(peakcnt * sizeof(int)))==NULL) {	/* Array to store peaks found in an analysis-window */
			sprintf(errstr,"INSUFFICIENT MEMORY to store window peak statistics data.\n");
			return(MEMORY_ERROR);
		}
		memset((char *)peakstats,0,peakcnt * sizeof(int));
	}
#ifdef EQUALFRQ
	intv_to_next_window      = dz->param[FE_STEP];
	intv_to_centre_of_window = dz->param[FE_WINSIZE] / 2.0;
	intv_to_top_of_window    = dz->param[FE_WINSIZE];
	/* we search for possible data over a group of channels that is slightly larger than the peak-window = search-window */
	/* just in case any frequency data is not in anticipated channel : to find the channel limits of srch, generate frqs that (theoretically) */
	/* correspond to those channels ..... */
	intv_to_top_of_srch = dz->param[FE_WINSIZE] + (FRQ_SRCH_ERRORBND * dz->chwidth);
	intv_to_bot_of_srch = -(FRQ_SRCH_ERRORBND * dz->chwidth);
#else
	intv_to_next_window      = semit_to_ratio(dz->param[FE_STEP]);
	intv_to_centre_of_window = semit_to_ratio(dz->param[FE_WINSIZE] / 2.0);
	intv_to_top_of_window    = semit_to_ratio(dz->param[FE_WINSIZE]);
	/* we search for possible data over a group of channels that is slightly larger than the peak-window = search-window */
	/* just in case any frequency data is not in anticipated channel : to find the channel limits of srch, generate frqs that (theoretically) */
	/* correspond to those channels ..... */
	intv_to_top_of_srch = semit_to_ratio(dz->param[FE_WINSIZE] + FRQ_SRCH_ERRORBND);
	intv_to_bot_of_srch = semit_to_ratio(-FRQ_SRCH_ERRORBND);
#endif
	intv_errorbnd = semit_to_ratio(FRQ_SRCH_ERRORBND);

	/* EXTRACT PEAK INFO FROM ANALYSIS WINDOWS */
	if(dz->mode == FE_CHECK) {
		if((sndseekEx(dz->ifd[0],dz->iparam[FE_START] * dz->wanted,0)<0)){
            sprintf(errstr,"sndseek() failed\n");
            return SYSTEM_ERROR;
        }
        
		if((dz->ssampsread = fgetfbufEx(dz->bigfbuf,dz->wanted,dz->ifd[0],0)) < 0) {
			sprintf(errstr,"Can't read samples from input soundfile\n");
			return(SYSTEM_ERROR);
		}
		lastminamp = HUGE;					/* forces 1st minamp value to be less than previous minamp position */
		lastminampchan = 0;
		lastmaxampchanfrq = -(dz->nyquist);	/* forces first minampchanfrq to be greater than lastmaxampchanfrq */
		lastmaxamp = -HUGE;					/* forces 1st maxamp value to be greater than previous maxamp position */
		lastmaxampchan = -1;
		lastminampchanfrq = -(dz->nyquist);	/* forces first maxampchanfrq to be greater than lastminampchanfrq */
		lookformax = 1;						/* always begin by searching for a maximum (lastmaxamp set to -HUGE) */
		peakcnt = 0;						/* initialise peak counter */
		botfrq = dz->param[FE_LOFRQ] / intv_to_centre_of_window;	/* initialise bottom frq of 1st peak-window to search */

		/* FIND ALL THE PEAKS IN THE WINDOW --> array of peak trof [peak trof ......] in 'win_peakchan' */

/* NEW */
		if(dz->vflag[FE_SORTBUF])
			frqsort_buf(dz->bigfbuf,dz);		
/* NEW */

		peakcnt = peaks_in_window(botfrq,intv_to_bot_of_srch,intv_to_top_of_srch,intv_to_centre_of_window,intv_to_top_of_window,intv_to_next_window,&lookformax,
			  &lastmaxamp,&lastmaxampchan,&lastmaxampchanfrq,&lastminamp,&lastminampchan,&lastminampchanfrq,win_peakchan,dz->bigfbuf,dz);
		if(peakcnt > 0) {
			if(ODD(peakcnt))	/* final trough not recorded in peak data... i.e. window was still falling in amp at top of data */
				win_peakchan[peakcnt++] = dz->clength;	/* put final trough in final channel */ 

			/* LOCATE CENTRE FRQ AND (CENTRAL-WINDOW) AMPLITUDE OF EACH PEAK --> arrays 'win_peakamp' and 'win_peakfrq' */
			/* peakcnt gets count of actual peaks, rather than count of peaks+troughs as before */

			peakcnt = locate_pitch_centre_of_each_peak(peakcnt,win_peakchan,win_peakamp,win_peakfrq,botfrq,
						intv_errorbnd,intv_to_top_of_window,intv_to_centre_of_window,intv_to_next_window,dz->bigfbuf,dz);

			/* KEEP ONLY THE LOUDEST PEAKS */

			while(peakcnt > dz->iparam[FE_PKCNT]) {	/* If too many peaks */
				minpkpos = 0;
				for(k = 1;k<peakcnt;k++) {			/* find position of quietest */
					if(win_peakamp[k] < win_peakamp[minpkpos])
						minpkpos = k;
				}
				peakcnt--;						/* eliminate either by dropping off end of list */
				if(minpkpos < peakcnt) {		/* or by shuffling later values down over it */
					for(k = minpkpos;k < peakcnt;k++) {
						win_peakamp[k] = win_peakamp[k+1];
						win_peakfrq[k] = win_peakfrq[k+1];
					}
				}
			}
		}
		/* Output number of peaks, peakfrqs; then entire window data */
		fprintf(stdout,"%d\n",peakcnt);
		for(wc = 0; wc < peakcnt;wc++)
			fprintf(stdout,"%lf\n",win_peakfrq[wc]);
		maxamp = 0.0;
		for(cc = 0, vc= 0; cc < dz->clength;cc++,vc+=2)
			maxamp = max(maxamp,dz->bigfbuf[AMPP]);		/* find max amp value, for normalisation */
		if(maxamp <= 0.0) {
			for(cc = 0, vc= 0; cc < dz->clength;cc++,vc+=2)
				fprintf(stdout,"%lf\n",0.0);
		} else {
			for(cc = 0, vc= 0; cc < dz->clength;cc++,vc+=2)
				fprintf(stdout,"%lf\n",dz->bigfbuf[AMPP]/maxamp);
		}
		fflush(stdout);
		return(FINISHED);
	}
	wc = 0;
	while(dz->samps_left > 0) {
		if((dz->ssampsread = fgetfbufEx(dz->bigfbuf,dz->buflen,dz->ifd[0],0)) < 0) {
			sprintf(errstr,"Can't read samples from input soundfile\n");
			return(SYSTEM_ERROR);
		}
		dz->samps_left -= dz->ssampsread;	
		dz->total_samps_read += dz->ssampsread;
		wins_in_buf = dz->ssampsread / dz->wanted; 	
		dz->flbufptr[0] = dz->bigfbuf;
		for(n=0;n<wins_in_buf;n++) {
			lastminamp = HUGE;					/* forces 1st minamp value to be less than previous minamp position */
			lastminampchan = 0;
			lastmaxampchanfrq = -(dz->nyquist);	/* forces first minampchanfrq to be greater than lastmaxampchanfrq */
			lastmaxamp = -HUGE;					/* forces 1st maxamp value to be greater than previous maxamp position */
			lastmaxampchan = -1;
			lastminampchanfrq = -(dz->nyquist);	/* forces first maxampchanfrq to be greater than lastminampchanfrq */
			lookformax = 1;						/* always begin by searching for a maximum (lastmaxamp set to -HUGE) */
			peakcnt = 0;						/* initialise peak counter */
#ifdef EQUALFRQ
			botfrq = dz->param[FE_LOFRQ] - intv_to_centre_of_window;	/* initialise bottom frq of 1st peak-window to search */
#else
			botfrq = dz->param[FE_LOFRQ] / intv_to_centre_of_window;	/* initialise bottom frq of 1st peak-window to search */
#endif
			/* FIND ALL THE PEAKS IN THE WINDOW --> array of peak trof [peak trof ......] in 'win_peakchan' */
			if(dz->vflag[FE_SORTBUF])
				frqsort_buf(dz->flbufptr[0],dz);		
			peakcnt = peaks_in_window(botfrq,intv_to_bot_of_srch,intv_to_top_of_srch,intv_to_centre_of_window,intv_to_top_of_window,intv_to_next_window,&lookformax,
				  &lastmaxamp,&lastmaxampchan,&lastmaxampchanfrq,&lastminamp,&lastminampchan,&lastminampchanfrq,win_peakchan,dz->flbufptr[0],dz);
			if(peakcnt == 0) {
				dz->flbufptr[0] += dz->wanted;
				if(dz->mode == FE_WINDOWS) {
					ampsum = 0.0;
					for(cc = 0, vc = 0; cc < dz->clength; cc++, vc+=2) 
						ampsum += dz->flbufptr[0][AMPP];
					totamp[wc] = (float)ampsum;
				}
				wc++;
				continue;	/* peakamp[wc]/peakfrq[wc] retain initial (0,0) vals = a no-peaks window */
			}
			if(ODD(peakcnt))	/* final trough not recorded in peak data... i.e. window was still falling in amp at top of data */
				win_peakchan[peakcnt++] = dz->clength;	/* put final trough in final channel */ 


			/* LOCATE CENTRE FRQ AND (CENTRAL-WINDOW) AMPLITUDE OF EACH PEAK --> arrays 'win_peakamp' and 'win_peakfrq' */
			/* peakcnt gets count of actual peaks, rather than count of peaks+troughs as before */

			peakcnt = locate_pitch_centre_of_each_peak(peakcnt,win_peakchan,win_peakamp,win_peakfrq,botfrq,
						intv_errorbnd,intv_to_top_of_window,intv_to_centre_of_window,intv_to_next_window,dz->flbufptr[0],dz);

			/* ELIMINATE PEAKS AT LOWER SEARCH BOUNDARY */

			for(k = 0;k<peakcnt;k++) {
				if(win_peakfrq[k] < dz->param[FE_LOFRQ] + FLTERR)
					minpkpos = k;
				peakcnt--;						/* eliminate either by dropping off end of list */
				if(minpkpos < peakcnt) {		/* or by shuffling later values down over it */
					for(k = minpkpos;k < peakcnt;k++) {
						win_peakamp[k] = win_peakamp[k+1];
						win_peakfrq[k] = win_peakfrq[k+1];
					}
				}
			}

			/* KEEP ONLY THE LOUDEST PEAKS */

			while(peakcnt > dz->iparam[FE_PKCNT]) {	/* If too many peaks */
				minpkpos = 0;
				for(k = 1;k<peakcnt;k++) {			/* find position of quietest */
					if(win_peakamp[k] < win_peakamp[minpkpos])
						minpkpos = k;
				}
				peakcnt--;						/* eliminate either by dropping off end of list */
				if(minpkpos < peakcnt) {		/* or by shuffling later values down over it */
					for(k = minpkpos;k < peakcnt;k++) {
						win_peakamp[k] = win_peakamp[k+1];
						win_peakfrq[k] = win_peakfrq[k+1];
					}
				}
			}
			/* STORE PEAK DATA IN FEATURES ARRAY -- if peakcnt < dz->iparam[FE_PKCNT0, features array retains dummy vals 0.0 */
			for(k = 0;k < peakcnt; k++) {
				peakamp[wc][k] = win_peakamp[k];
				peakfrq[wc][k] = win_peakfrq[k];
			}
			if(dz->mode == FE_WINDOWS) {
				ampsum = 0.0;
				for(cc = 0, vc = 0; cc < dz->clength; cc++, vc+=2) 
					ampsum += dz->flbufptr[0][AMPP];
				totamp[wc] = (float)ampsum;
			}
			dz->flbufptr[0] += dz->wanted;
			wc++;
		}
	}
	switch(dz->mode) {
	case(FE_BEST):
	case(FE_EVERY):
	case(FE_ENVEL):
		/* NOT WRITTEN YET */
		break;
	case(FE_WINDOWS):
		/* Output number of peaks, number of windows, minfrq, maxfrq; then time, (normalised) amp, frq data in triples */
		startwin = dz->iparam[FE_WINTIME];
		endwin = min(startwin + dz->iparam[FE_WINCNT],dz->wlength);
		fprintf(stdout,"%d %d\n",dz->iparam[FE_PKCNT],dz->iparam[FE_WINCNT]);
		fprintf(stdout,"%lf %lf\n",dz->param[FE_LOFRQ],dz->param[FE_HIFRQ]);
		fprintf(stdout,"%lf %lf\n",startwin * dz->frametime,endwin * dz->frametime);
		OK = 0;
		for(wc = startwin; wc < endwin; wc++) {
			if(totamp[wc] > MIN_DB) {
				OK = 1;
				break;
			}
		}
		if (!OK) {
			sprintf(errstr,"NO SIGNIFICANT SIGNAL-LEVEL FOUND");
			return(DATA_ERROR);
		}
		for(wc = startwin; wc < endwin; wc++) 
			fprintf(stdout,"%f\n",totamp[wc]);
		maxamp = 0.0;
		for(wc = startwin; wc < endwin;wc++) {
			for(k=0;k<dz->iparam[FE_PKCNT];k++)
				maxamp = max(maxamp,peakamp[wc][k]);		/* find max amp value, for normalisation */
		}
		if(maxamp <= 0.0) {
			for(wc = startwin; wc < endwin;wc++)  {
				for(k=0;k<dz->iparam[FE_PKCNT];k++) {
					fprintf(stdout,"%lf %lf %lf\n",wc * dz->frametime,peakfrq[wc][k],0.0);
				}
			}
		} else {
			for(wc = startwin; wc < endwin;wc++)  {
				for(k=0;k<dz->iparam[FE_PKCNT];k++) {
					fprintf(stdout,"%lf %lf %lf\n",wc * dz->frametime,peakfrq[wc][k],peakamp[wc][k]/maxamp);
				}
			}
		}
		fflush(stdout);
		break;
	case(FE_AVERAGE):
#ifndef EQUALFRQ
		minmidi  = unchecked_hztomidi(dz->param[FE_LOFRQ]);
#endif
		startwin = dz->iparam[FE_START];
		endwin   = dz->iparam[FE_END];
		peakcnt  = (int)ceil(interval/dz->param[FE_STEP]);	/* no of possible peak values */
		for(wc = startwin; wc < endwin;wc++) {
			for(k = 0;k < dz->iparam[FE_PKCNT]; k++) {
				if(peakfrq[wc][k] > 0.0) {
#ifdef EQUALFRQ
					j = (int)floor((peakfrq[wc][k] - dz->param[FE_LOFRQ])/dz->param[FE_STEP]);
#else
					j = (int)floor((unchecked_hztomidi(peakfrq[wc][k]) - minmidi)/dz->param[FE_STEP]);
#endif
					peakstats[j]++;
				}
			}
		}
		maxamp = 0.0;
		for(k = 0;k < peakcnt; k++) {
			maxamp = max(maxamp,(double)peakstats[k]);		/* find max stats value, for normalisation */
		}
		/* Output minfrq, maxfrq; then "amp", frq data in pairs */
		fprintf(stdout,"%lf %lf\n",dz->param[FE_LOFRQ],dz->param[FE_HIFRQ]);
#ifdef EQUALFRQ
		thisval = dz->param[FE_LOFRQ];
#else
		thisval = minmidi;
#endif
		if(maxamp <= 0.0) {
			for(k = 0;k < peakcnt; k++)  {
#ifdef EQUALFRQ
				fprintf(stdout,"%lf %lf\n",thisval,(double)0.0);
#else
				fprintf(stdout,"%lf %lf\n",miditohz(thisval),(double)0.0);
#endif
				thisval += dz->param[FE_STEP];
			}
		} else {
			for(k = 0;k < peakcnt; k++)  {
#ifdef EQUALFRQ
				fprintf(stdout,"%lf %lf\n",thisval,(double)peakstats[k]/maxamp);
#else
				fprintf(stdout,"%lf %lf\n",miditohz(thisval),(double)peakstats[k]/maxamp);
#endif
				thisval += dz->param[FE_STEP];
			}
		}
		fflush(stdout);
		break;
	}
	return(FINISHED);
}


/********************************************** PEAKS_IN_WINDOW **************************************************
 *
 * Scan an analysis window to find alternate peaks and troughs.
  */

int peaks_in_window(double botfrq,double intv_to_bot_of_srch,double intv_to_top_of_srch,double intv_to_centre_of_window,double intv_to_top_of_window,
			  double intv_to_next_window,int *lookformax,double *lastmaxamp,int *lastmaxampchan,double *lastmaxampchanfrq,
			  double *lastminamp,int *lastminampchan,double *lastminampchanfrq,int *win_peakchan,float *buf,dataptr dz)
{
/* TEST */
static int gocnt = 0;
//int gocnt2 = 0;
/* TEST */
	double srchtopfrq, srchbotfrq, centrefrq, topfrq, minamp, maxamp, maxampchanfrq=0.0, minampchanfrq=0.0;
	int srchbotchan, srchtopchan, founddata, cc, vc, maxampchan=0, minampchan=0, peakcnt;

	peakcnt = 0;
				/* SET UP SEARCH WINDOWS TO LOOK FOR AMPLITUDE PEAKS AND TROUGHS */	
#ifdef EQUALFRQ
	while((srchtopfrq = botfrq + intv_to_top_of_srch) < dz->nyquist) {		/* set frq corresponding to top channel of search-window */
		centrefrq = botfrq + intv_to_centre_of_window;						/* set centrefrq of actual-window */
		topfrq    = botfrq + intv_to_top_of_window;							/* set topfrq of actual-window of search */
		if((srchbotfrq = botfrq + intv_to_bot_of_srch) <= dz->halfchwidth)	/* set frq corresponding to bottom channel of search-window */
#else
	while((srchtopfrq = botfrq * intv_to_top_of_srch) < dz->nyquist) {		/* set frq corresponding to top channel of search-window */
		centrefrq = botfrq * intv_to_centre_of_window;						/* set centrefrq of actual-window */
		topfrq    = botfrq * intv_to_top_of_window;							/* set topfrq of actual-window of search */
		if((srchbotfrq = botfrq * intv_to_bot_of_srch) <= dz->halfchwidth)	/* set frq corresponding to bottom channel of search-window */
#endif
			srchbotfrq = dz->halfchwidth + FLTERR;
		if(srchtopfrq > dz->param[FE_HIFRQ])
			break;
		srchbotchan = (int)round(srchbotfrq/dz->chwidth);					/* set bot and top chans of search-window */
		srchtopchan = (int)round(srchtopfrq/dz->chwidth);

		minamp = HUGE;														/* initialise vals of max and min amp in search-window */
		maxamp = -HUGE;
		founddata = 0;
				/* SEARCH WINDOW FOR AMPLITUDE PEAKS AND TROUGHS */	
		for(cc = srchbotchan, vc = cc*2; cc < srchtopchan; cc++,vc+=2) {	/* 	Find max and min amplitudes in search-window */
			if(buf[FREQ] >= botfrq && buf[FREQ] <= topfrq) {	/* considering only channel data whose frq lies within actual-window limits */
				founddata = 1;
				if(buf[AMPP] > maxamp) {
					maxamp = buf[AMPP];
					maxampchan = cc;
					maxampchanfrq = buf[FREQ];
				}
				if(buf[AMPP] < minamp) {
					minamp = buf[AMPP];
					minampchan = cc;
					minampchanfrq = buf[FREQ];
				}
			}
		}
		if(!founddata) {
#ifdef EQUALFRQ
			botfrq += intv_to_next_window;
#else
			botfrq *= intv_to_next_window;
#endif
			continue;
		}									
				/* COMPARE SUCCESIVE WINDOWS FOR TRUE PEAKS AND TROUGHS */	
		if(*lookformax) {												/* If searching for a maximum */
			if(maxamp < *lastmaxamp) {									/* If we've passed the local maximum */
				*lookformax = 0;										/* switch to looking for a trough */
				win_peakchan[(peakcnt)++] = *lastmaxampchan;				/* store the location of the peak */
				
				if(minampchanfrq > *lastmaxampchanfrq) {				/* if we already have a minamp AFTER the maxamp chan */
					*lastminamp = minamp;								/* initialise search for trough at trough so far */
					*lastminampchan = minampchan;
					*lastminampchanfrq = minampchanfrq;
				} else {												/* else */
					*lastminamp = HUGE;									/* initialise search for trough: forces a lastminamp to be set at BETA */
				}
				*lastmaxamp = -HUGE;									/* reinitialise the local maximum: */
																		/* forces a lastmaxamp to be set at ALPHA next time lookformax = 1 */

			} else {													/* if we've not passed the local maximum */		/* ALPHA */
				*lastmaxamp = maxamp;									/* remember the current maximum */
				*lastmaxampchan = maxampchan;
				*lastmaxampchanfrq = maxampchanfrq;
			}
		} else {														
			if(minamp > *lastminamp) {									/* If we've passed the local trough */
				*lookformax = 1;										/* switch to looking for a peak */
				win_peakchan[(peakcnt)++] = *lastminampchan;				/* store the location of the trough */
				
				if(maxampchanfrq > *lastminampchanfrq) {				/* if we already have a maxamp AFTER the minamp chan */
					*lastmaxamp = maxamp;								/* initialise search for trough at trough so far */
					*lastmaxampchan = maxampchan;
					*lastmaxampchanfrq = maxampchanfrq;
				} else {												/* else */
					*lastmaxamp = -HUGE;									/* initialise search for peak: forces a lastmaxamp to be set at ALPHA */
				}
				*lastminamp = HUGE;										/* reinitialise the local minimum */
																		/* forces a lastminamp to be set at BETA next time lookformax = 0 */

			} else {													/* if we've not passed the local maximum */		/* BETA */
				*lastminamp = minamp;									/* remember the current maximum */
				*lastminampchan = minampchan;
				*lastminampchanfrq = minampchanfrq;
			}
		}
#ifdef EQUALFRQ
		botfrq += intv_to_next_window;
#else
		botfrq *= intv_to_next_window;
#endif
	}
/* TEST */
gocnt++;
/* TEST */
	return peakcnt;
}

int locate_pitch_centre_of_each_peak(int peakandtrofcnt,int *win_peakchan,float *win_peakamp,float *win_peakfrq,double footfrq,
				  double intv_errorbnd,double intv_to_top_of_window,double intv_to_centre_of_window,double intv_to_next_window,
				  float *buf,dataptr dz)
{
	int k, cc, vc, actual_peakcnt, gotdata;				
	double pktopfrq, pkbotfrq;			/* frqs at top and bottom of peak */
	double botfrq, centrefrq, topfrq;	/* frqs at top and bottom of scanning-window in peak */
	double srchbotfrq, srchtopfrq;		/* frqs corresponding to channels at outer search-edges of scanning-window in peak */
	double peakampsum, ampsum;
	int srchbotchan, srchtopchan;
	k = 1; /* set k to location of 1st trough in data */
	actual_peakcnt = 0;
	pkbotfrq = footfrq;
	while(k < peakandtrofcnt) {
		/* SET UP PEAK-PARAMETERS */
		pktopfrq  = win_peakchan[k] * dz->chwidth;	/* trough at top edge of peak */
#ifdef EQUALFRQ
		centrefrq = (pktopfrq + pkbotfrq)/2.0;
#else
		centrefrq = (unchecked_hztomidi(pktopfrq) + unchecked_hztomidi(pkbotfrq))/2.0;
		centrefrq = miditohz(centrefrq);			/* default the peak centrefrq to pitch-midway between troughs */
#endif
		/* SET UP PEAK-SCANNING WINDOW */
		botfrq = pkbotfrq;
#ifdef EQUALFRQ
		topfrq = botfrq + intv_to_top_of_window;
#else
		topfrq = botfrq * intv_to_top_of_window;
#endif
		peakampsum = -HUGE;
		gotdata = 0;
		/* MOVE PEAK-SCANNING WINDOW ACROSS PEAK TO FIND PITCH-POSITION OF MAXIMUM LEVEL */
		while(topfrq <= pktopfrq) {
			srchbotfrq  = max((botfrq / intv_errorbnd),dz->halfchwidth + FLTERR);
			srchtopfrq  = min((topfrq * intv_errorbnd),dz->nyquist - FLTERR);
			srchbotchan = (int)round(srchbotfrq/dz->chwidth);		/* set bot and top chans of search-window */
			srchtopchan = (int)round(srchtopfrq/dz->chwidth);		/* set bot and top chans of search-window */
			ampsum = 0.0;
			for(cc=srchbotchan,vc = cc*2;cc<= srchtopchan;cc++,vc+=2) {
				if(buf[FREQ] >= botfrq && buf[FREQ] <= topfrq) {	/* sum the amplitude over the scanning-window */
					gotdata = 1;
					ampsum += buf[AMPP];
				}
			}
			if(ampsum > peakampsum) {								/* find the location of loudest scanning-window */
				peakampsum = ampsum;
#ifdef EQUALFRQ
				centrefrq = botfrq + intv_to_centre_of_window;
#else
				centrefrq = botfrq * intv_to_centre_of_window;
#endif
			}
#ifdef EQUALFRQ
			botfrq += intv_to_next_window;
			topfrq = botfrq + intv_to_top_of_window;
#else
			botfrq *= intv_to_next_window;
			topfrq = botfrq * intv_to_top_of_window;
#endif
		}
		if(!gotdata) {
			peakampsum = 0.0;
		}
		win_peakamp[actual_peakcnt] = (float)peakampsum;	/* store loudest scanning-window-amplitude and centre-frq of peak */
		win_peakfrq[actual_peakcnt] = (float)centrefrq;
		actual_peakcnt++;									/* count peaks (only) */
		/* MOVE TO NEXT PEAK */
		pkbotfrq = pktopfrq;
		k += 2;
	}
	return actual_peakcnt;
}

double semit_to_ratio(double val)
{
	val = val/SEMITONES_PER_OCTAVE;
	val = pow(2.0,val);
	return val;
}

void frqsort_buf(float *buf,dataptr dz)
{
	int ccone, ampone, frqone, cctwo, amptwo, frqtwo;
	int lowtop = dz->clength - 1;
	float temp;

	for(ccone = 0,ampone = 0; ccone < lowtop; ccone++,ampone+=2) {
		frqone = ampone + 1;
		for(cctwo = ccone+1,amptwo=cctwo*2; cctwo < dz->clength; cctwo++,amptwo+=2) {
			frqtwo = amptwo+1;
			if(buf[frqtwo] < buf[frqone]) {
				temp = buf[frqone];
				buf[frqone] = buf[frqtwo];
				buf[frqtwo] = temp;
				temp = buf[ampone];
				buf[ampone] = buf[amptwo];
				buf[amptwo] = temp;
			}
		}
	}
}
#if 0
void tellme(char *str) {
	fprintf(stdout,"INFO: %s\n",str);
	fflush(stdout);
}

float amp_to_db(double amp, double silence)
{
	if(amp <= silence)
		return (float)MIN_DB;
	amp = log10(amp) * 20.0;
	return (float)amp;
}
#endif
