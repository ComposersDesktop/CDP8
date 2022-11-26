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



//In  the middle of converting this to more general power.
//Wee the new usage2 .....




// SPEKPOINTS = param0 = ANALPOINTS in Loom interface
// SPEKDUR = param1

// definbed in science.h
// #define SPEKSR		44100
// #define SPEKPOINTS	0
// #define SPEKSRATE	1
// #define SPEKDUR		2

// CONSIDER the data for SPECVARY having extra entries numbering the days(times) on which spectra measured??
// so that the size of textdata files do not become huge (if data is interpd as text, will be much bigger!!)
//

// TO AVOID ARTEFACTS OF THE Channel-centre frqs
// We need to generate frqs for the spectra that reflect something in the source (don't just use centre frq!!)

/******************************** PROBLEM *******************
//
 // !!!!!!!!!!!!!  PROBLEM !!!!!!!!!!!!!!
//
// NEED A WAY TO DEAL WITH OVERLOAD - i.e. REDUCE SPECTRAL LEVEL !!!!!!
*/

/*
 * NEW DEVELOPMENTS
 *
 * Data files have definite frq only for peaks and troughs ... other frqs 0.0
 * Other channels have -ve amps for troughs and +ve amps for peaks AND all others.
 *
 * Before C prog. Take the original data, and use a cubic-spline approach to find
 * true local maxima andminima with a resolution of 1/64th tone.
 *
 * Command line must now be
 *
 * spectrum fixed/variable outfile datafile chans srate dur [-hH] [-fF] [-rR] [-iI] [-zZ] [-gG] [-v]
 *
 * -h H = number of harmonics of peaks to have definite frqs
 * -f F = fraction of channels to be randomly varied (lower) than the amplitude curve value
 * -r R = max possible variation of selected chans (in F) from the amplitude curve value
 * -i I = proprtion of the bounding-curve to add to the inverted spectrum
 *
 *	nb nb nb WHAT TO DO IF SPECTRUM IS NOT INVERTED .... whatval to give param, and how to give it!!!
 *
 *
 *
 * -z Z = rate at which frq is focused (zoomed) on apeak value, when spectrum changes from
 *		  one (pre-timeinterp) window to the next.
 * -g G = overallgain on output
 * -v Generate a text output suitable to view on screen.
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



//CDP LIB REPLACEMENTS
static int check_spectrum_param_validity_and_consistency(int **perm,dataptr dz);
static int setup_spectrum_application(dataptr dz);
static int setup_spectrum_param_ranges_and_defaults(dataptr dz);
static void init_specsynth(dataptr dz);
static int allocate_spectrum_buffer(dataptr dz);
static int specformat(dataptr dz);
static int spectrum(int *perm,dataptr dz);
static int specvary(dataptr dz);
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
static int handle_the_special_data(int *cmdlinecnt,char ***cmdline,dataptr dz);
static int test_the_special_data(dataptr dz);
static int open_the_outfile(dataptr dz);
static int setup_and_init_input_brktable_constants(dataptr dz,int brkcnt);
static void insert_harmonics(int **peaked,int *peakcnt,double harmamp,float fundamental,double pkwidth,dataptr dz);
static void randomiseamps(int *perm,int *peaked,dataptr dz);
static void rndintperm(int *perm,int cnt);
static void spread_peaks(int *peaked,double spreaddnratio,double spreadupratio,dataptr dz);
static int  get_float_with_e_from_within_string(char **str,double *val);
static int read_nearest_value_from_brktable(double thistime,int paramno,dataptr dz);
static int check_spekline_param_validity_and_consistency(dataptr dz);
static double dbtolevel(double val);
static int speclines(dataptr dz);
static int get_the_mode_from_cmdline(char *str,dataptr dz);
static int speclinesfilt(dataptr dz);

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
	int exit_status, *perm;
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
		if(dz->process == SPEKLINE) {
			dz->maxmode = 2;
			if((exit_status = get_the_mode_from_cmdline(cmdline[0],dz))<0) {
				print_messages_and_close_sndfiles(exit_status,is_launched,dz);
				return(exit_status);
			}
			cmdline++;
			cmdlinecnt--;
		} else
			dz->maxmode = 0;
		// setup_particular_application =
		if((exit_status = setup_spectrum_application(dz))<0) {
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
	if((exit_status = setup_spectrum_param_ranges_and_defaults(dz))<0) {
		exit_status = print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	// handle_outfile() = 
	if((exit_status = handle_the_outfile(&cmdlinecnt,&cmdline,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = handle_the_special_data(&cmdlinecnt,&cmdline,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {		// CDP LIB
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
//	check_param_validity_and_consistency....
	if((exit_status = check_spectrum_param_validity_and_consistency(&perm,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if(dz->process == SPEKLINE) {
		if(dz->mode == 0)
			init_specsynth(dz);
		else
			dz->nyquist = dz->iparam[SPEKSRATE]/2.0;
	} else {
		if(dz->process != SPEKFRMT)
	//	set up all spectrum params
			init_specsynth(dz);
		else
			dz->nyquist = dz->iparam[SPEKSRATE]/2.0;
	}
	if((exit_status = test_the_special_data(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	is_launched = TRUE;
	if((exit_status = open_the_outfile(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if(dz->process != SPEKFRMT) {
		if((exit_status = 	allocate_spectrum_buffer(dz))<0) {							// CDP LIB
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
	}
	if(dz->process == SPEKLINE) {
		if((exit_status = check_spekline_param_validity_and_consistency(dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
	} else
		srand(1);
	switch(dz->process) {
	case(SPEKFRMT):
		if((exit_status = specformat(dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		break;
	case(SPEKTRUM):
		if((exit_status = spectrum(perm,dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		break;
	case(SPEKVARY):
		if((exit_status = specvary(dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		break;
	case(SPEKLINE):
		if(dz->mode == 0) {
			if((exit_status = speclines(dz))<0) {
				print_messages_and_close_sndfiles(exit_status,is_launched,dz);
				return(FAILED);
			}
		} else {
			if((exit_status = speclinesfilt(dz))<0) {
				print_messages_and_close_sndfiles(exit_status,is_launched,dz);
				return(FAILED);
			}
		}
		break;
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
//			if(!sloom)
//				fprintf(stderr,errstr);
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
//			if(!sloom)
//				fprintf(stderr,errstr);
			return(MEMORY_ERROR);
		}
		strcpy(ap->option_list,optlist);
		if((ap->option_flags = (char *)malloc((size_t)(optcnt+1)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY: for option_flags\n");
//			if(!sloom)
//				fprintf(stderr,errstr);
			return(MEMORY_ERROR);
		}
		strcpy(ap->option_flags,optflags); 
	}
	ap->vflag_cnt = (char) vflagcnt;		   
	ap->variant_param_cnt = (char) vparamcnt;
	if(vflagcnt) {
		if((ap->variant_list  = (char *)malloc((size_t)(vflagcnt+1)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY: for variant_list\n");
//			if(!sloom)
//				fprintf(stderr,errstr);
			return(MEMORY_ERROR);
		}
		strcpy(ap->variant_list,varlist);		
		if((ap->variant_flags = (char *)malloc((size_t)(vflagcnt+1)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY: for variant_flags\n");
//			if(!sloom)
//				fprintf(stderr,errstr);
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

	if(dz->input_data_type==UNRANGED_BRKFILE_ONLY) {
		dz->extrabrkno = brkcnt;			  
		brkcnt++;		/* create brktable poniter for param0, and use point to and read (parray) input data during process */
	} else
		brkcnt++;
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
	dz->infilecnt = 0;
	return(FINISHED);
}

/******************************** SETUP_AND_INIT_INPUT_BRKTABLE_CONSTANTS ********************************/

int setup_and_init_input_brktable_constants(dataptr dz,int brkcnt)
{	
	int n;
	if((dz->brk      = (double **)malloc(brkcnt * sizeof(double *)))==NULL) {
		sprintf(errstr,"setup_and_init_input_brktable_constants(): 1\n");
//		if(!sloom)
//			fprintf(stderr,errstr);
		return(MEMORY_ERROR);
	}
	if((dz->brkptr   = (double **)malloc(brkcnt * sizeof(double *)))==NULL) {
		sprintf(errstr,"setup_and_init_input_brktable_constants(): 6\n");
//		if(!sloom)
//			fprintf(stderr,errstr);
		return(MEMORY_ERROR);
	}
	if((dz->brksize  = (int    *)malloc(brkcnt * sizeof(int)))==NULL) {
		sprintf(errstr,"setup_and_init_input_brktable_constants(): 2\n");
//		if(!sloom)
//			fprintf(stderr,errstr);
		return(MEMORY_ERROR);
	}
	if((dz->firstval = (double  *)malloc(brkcnt * sizeof(double)))==NULL) {
		sprintf(errstr,"setup_and_init_input_brktable_constants(): 3\n");
//		if(!sloom)
//			fprintf(stderr,errstr);
		return(MEMORY_ERROR);												  
	}
	if((dz->lastind  = (double  *)malloc(brkcnt * sizeof(double)))==NULL) {
		sprintf(errstr,"setup_and_init_input_brktable_constants(): 4\n");
//		if(!sloom)
//			fprintf(stderr,errstr);
		return(MEMORY_ERROR);
	}
	if((dz->lastval  = (double  *)malloc(brkcnt * sizeof(double)))==NULL) {
		sprintf(errstr,"setup_and_init_input_brktable_constants(): 5\n");
//		if(!sloom)
//			fprintf(stderr,errstr);
		return(MEMORY_ERROR);
	}
	if((dz->brkinit  = (int     *)malloc(brkcnt * sizeof(int)))==NULL) {
		sprintf(errstr,"setup_and_init_input_brktable_constants(): 7\n");
//		if(!sloom)
//			fprintf(stderr,errstr);
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
//		if(!sloom)
//			fprintf(stderr,errstr);
		return(MEMORY_ERROR);
	}
	if((dz->iparam      = (int    *)calloc(storage_cnt, sizeof(int)   ))==NULL) {
		sprintf(errstr,"setup_parameter_storage_and_constants(): 2\n");
//		if(!sloom)
//			fprintf(stderr,errstr);
		return(MEMORY_ERROR);
	}
	if((dz->is_int      = (char   *)calloc(storage_cnt, sizeof(char)))==NULL) {
		sprintf(errstr,"setup_parameter_storage_and_constants(): 3\n");
//		if(!sloom)
//			fprintf(stderr,errstr);
		return(MEMORY_ERROR);
	}
	if((dz->no_brk      = (char   *)calloc(storage_cnt, sizeof(char)))==NULL) {
		sprintf(errstr,"setup_parameter_storage_and_constants(): 5\n");
//		if(!sloom)
//			fprintf(stderr,errstr);
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
//			if(!sloom)
//				fprintf(stderr,errstr);
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
//			if(!sloom)
//				fprintf(stderr,errstr);
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
//			if(!sloom)
//				fprintf(stderr,errstr);
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
//			if(!sloom)
//				fprintf(stderr,errstr);
			return(PROGRAM_ERROR);
		}
	}
	return(FINISHED);
}

/************************ HANDLE_THE_OUTFILE *********************/

int handle_the_outfile(int *cmdlinecnt,char ***cmdline,dataptr dz)
{
	char *filename = (*cmdline)[0];
	if(filename[0]=='-' && filename[1]=='f') {
		dz->floatsam_output = 1;
		dz->true_outfile_stype = SAMP_FLOAT;
		filename+= 2;
	}
	if(!sloom) {
		if(file_has_invalid_startchar(filename) || value_is_numeric(filename)) {
			sprintf(errstr,"Outfile name %s has invalid start character(s) or looks too much like a number.\n",filename);
//			if(!sloom)
//				fprintf(stderr,errstr);
			return(DATA_ERROR);
		}
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
	if((exit_status = create_sized_outfile(dz->outfilename,dz))<0)
		return(exit_status);
	if(dz->process != SPEKFRMT && dz->process != SPEKLINE) {
		if(dz->vflag[0])
			dz->is_rectified = 1;
		else
			dz->is_rectified = 0;
	}		
	return(FINISHED);
}

/***************************** ESTABLISH_APPLICATION **************************/

int establish_application(dataptr dz)
{
	aplptr ap;
	if((dz->application = (aplptr)malloc(sizeof (struct applic)))==NULL) {
		sprintf(errstr,"establish_application()\n");
//		if(!sloom)
//			fprintf(stderr,errstr);
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
//		if(!sloom)
//			fprintf(stderr,errstr);
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
//		if(!sloom)
//			fprintf(stderr,errstr);
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
//		if(!sloom)
//			fprintf(stderr,errstr);
		return(MEMORY_ERROR);
	}
	for(n=0;n<tipc;n++)
		dz->is_active[n] = (char)0;
	return(FINISHED);
}

/************************* SETUP_SPECTRUM_APPLICATION *******************/

int setup_spectrum_application(dataptr dz)
{
	int exit_status;
	aplptr ap;
	if((exit_status = establish_application(dz))<0)		// GLOBAL
		return(FAILED);
	ap = dz->application;
	// SEE parstruct FOR EXPLANATION of next 2 functions
	switch(dz->process) {
	case(SPEKTRUM):
		if((exit_status = set_param_data(ap,0   ,3,3,"iid"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"hbfrsatwm",9,"idddddiDd","d",1,0,"0"))<0)
			return(FAILED);
		dz->input_data_type = NO_FILE_AT_ALL;
		dz->process_type	= BIG_ANALFILE;	
		dz->outfiletype  	= ANALFILE_OUT;
		break;
	case(SPEKVARY):
		if((exit_status = set_param_data(ap,0   ,3,3,"iid"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"hbfrsatwmz",10,"idddddiDdd","d",1,0,"0"))<0)
			return(FAILED);
		dz->input_data_type = NO_FILE_AT_ALL;
		dz->process_type	= BIG_ANALFILE;	
		dz->outfiletype  	= ANALFILE_OUT;
		break;
	case(SPEKFRMT):
		if((exit_status = set_param_data(ap,0   ,2,2,"ii"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","a",1,0,"0"))<0)
			return(FAILED);
		dz->input_data_type = BRKFILES_ONLY;
		dz->process_type	= TO_TEXTFILE;	
		dz->outfiletype  	= TEXTFILE_OUT;
		break;
	case(SPEKLINE):
		if(dz->mode == 0) {
			if((exit_status = set_param_data(ap,SPEKLDATA,12,12,"iididddddddd"))<0)
				return(FAILED);
		} else {
			if((exit_status = set_param_data(ap,SPEKLDATA,12,8,"0id00dddd0dd"))<0)
				return(FAILED);
		}
		if((exit_status = set_vflgs(ap,"",0,"","",0,0,""))<0)
			return(FAILED);
		dz->input_data_type = NO_FILE_AT_ALL;
		if(dz->mode == 0) {
			dz->process_type	= BIG_ANALFILE;	
			dz->outfiletype  	= ANALFILE_OUT;
		} else {
			dz->process_type	= TO_TEXTFILE;	
			dz->outfiletype  	= TEXTFILE_OUT;
		}
		break;
	}
	// set_legal_infile_structure -->
	dz->has_otherfile = FALSE;
	// assign_process_logic -->
	return application_init(dz);	//GLOBAL
}

/************************* SETUP_SPECTRUM_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_spectrum_param_ranges_and_defaults(dataptr dz)
{
	int exit_status;
	aplptr ap = dz->application;
	// set_param_ranges()
	ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
	// NB total_input_param_cnt is > 0 !!!
	if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
		return(FAILED);
	// get_param_ranges()
	if(dz->process != SPEKLINE || dz->mode != 0) {
		ap->lo[SPEKPOINTS]	= 2;
		ap->hi[SPEKPOINTS]	= 16380;		// CHANNEL CNT
		ap->default_val[SPEKPOINTS]	= 2048;
	}
	if(dz->process == SPEKLINE && dz->mode == 0) {
		ap->lo[SPEKPOINTS]	= 2;
		ap->hi[SPEKPOINTS]	= 16380;		// CHANNEL CNT
		ap->default_val[SPEKPOINTS]	= 2048;
	}
	ap->lo[SPEKSRATE]	= 44100;
	ap->hi[SPEKSRATE]	= 96000;		// SRATE
	ap->default_val[SPEKSRATE] = 44100;
	ap->lo[SPEKDUR]	= 0;
	ap->hi[SPEKDUR]	= 32767;			// DURATION
	ap->default_val[SPEKDUR] = 10;
	if(dz->process == SPEKLINE) {
		if(dz->mode == 0) {
			ap->lo[SPEKHARMS]	= 0;
			ap->hi[SPEKHARMS]	= 64;			// (MAX) NO OF HARMONICS TO EMPHASIZE
			ap->default_val[SPEKHARMS] = 0;
			ap->lo[SPEKBRITE]	= -96;
			ap->hi[SPEKBRITE]	= 0;			// BRIGHTNESS (RELATIVE LEVEL OF SUCCESSIVE HARMONICS)
			ap->default_val[SPEKBRITE] = 0;
		}
		ap->lo[SPEKDATLO]	= 0;			//	Bottom of input data range
		ap->hi[SPEKDATLO]	= 48000;
		ap->default_val[SPEKDATLO] = 0;
		ap->lo[SPEKDATHI]	= 0;			//	Top of input data range
		ap->hi[SPEKDATHI]	= 48000;
		ap->default_val[SPEKDATHI] = 22050;
		ap->lo[SPEKSPKLO]	= 0;			//	Bottom of spectral data range
		ap->hi[SPEKSPKLO]	= 48000;
		ap->default_val[SPEKSPKLO] = 0;
		ap->lo[SPEKSPKHI]	= 0;			//	Top of spectral data range
		ap->hi[SPEKSPKHI]	= 48000;
		ap->default_val[SPEKSPKHI] = 22050;
		if(dz->mode == 0) {
			ap->lo[SPEKMAX]	= 0.001;			//	Maximum channel gain in output spectrum
			ap->hi[SPEKMAX]	= 1;
			ap->default_val[SPEKMAX] = 1;
		}
		ap->lo[SPEKWARP]	= 0.1;			//	Warping of input spectrum
		ap->hi[SPEKWARP]	= 10;
		ap->default_val[SPEKWARP] = 1;
		ap->lo[SPEKAWARP]	= 1;			//	Warping of input amplitudes
		ap->hi[SPEKAWARP]	= 100;
		ap->default_val[SPEKAWARP] = 1;
	} else {
		if(dz->process != SPEKFRMT) {
			ap->lo[2]	= 0.1;
			ap->hi[2]	= 3600;			// OUTPUT DURATION
			ap->default_val[2] = 10;
			ap->lo[SPEKHARMS]	= 0;
			ap->hi[SPEKHARMS]	= 64;			// (MAX) NO OF HARMONICS TO EMPHASIZE
			ap->default_val[SPEKHARMS] = 0;
			ap->lo[SPEKBRITE]	= 0;
			ap->hi[SPEKBRITE]	= 1;			// BRIGHTNESS (RELATIVE LEVEL OF SUCCESSIVE HARMONICS)
			ap->default_val[SPEKBRITE] = 0;
			ap->lo[SPEKRANDF]	= 0.0;
			ap->hi[SPEKRANDF]	= 1.0;			// MAX PROPORTION OF NON-PEAK CHANS TO VARY AWAY FROM AMPLITUDE-CURVE
			ap->default_val[SPEKRANDF] = 0;
			ap->lo[SPEKRANDA]	= 0.1;
			ap->hi[SPEKRANDA]	= 1.0;			// MAX RAND EXCURSION OF AMPLITUDE FROM AMPLITUDE CURVE (FOR SPECIFIED CHANS)
			ap->default_val[SPEKRANDA] = 0;
			ap->lo[SPEKSPRED]	= 0;
			ap->hi[SPEKSPRED]	= 0.1;
			ap->default_val[SPEKSPRED] = 0;		//	SPREAD OF SPECTRAL PEAKS
			ap->lo[SPEKGAIN]	= 0.001;
			ap->hi[SPEKGAIN]	= 1.0;
			ap->default_val[SPEKGAIN] = 1;		//	OVERALL GAIN APPLIED TO OUTPUT
			ap->lo[SPEKTYPE]	= 0;
			ap->hi[SPEKTYPE]	= 5;
			ap->default_val[SPEKTYPE] = 0;		//	TYPE OF DATA INTERPRETATION
			ap->lo[SPEKWIDTH]	= 0.0;
			ap->hi[SPEKWIDTH]	= 96000;		//	RESET LATER
			ap->default_val[SPEKWIDTH] = 0;		//	WIDTH (ASPECT RATIO) OF SPECTRAL PEAKS
			ap->lo[SPEKMXASP]	= 0.0;
			ap->hi[SPEKMXASP]	= 96000;		//	RESET LATER
			ap->default_val[SPEKMXASP] = 0;		//	MAXIMUM ASPECT RATIO
			if(dz->process == SPEKVARY) {
				ap->lo[SPEKZOOM]	= 0.1;
				ap->hi[SPEKZOOM]	= 10.0;
				ap->default_val[SPEKZOOM] = 1;		//	RATE OF ZOOMING IN ON NEW PEAKDATA
			}
		}
	}
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
//			if(!sloom)
//				fprintf(stderr,errstr);
			return(DATA_ERROR);
		}
		switch(cnt) {
		case(1):	
			if(sscanf(argv[cnt],"%d",&dz->process)!=1) {
				sprintf(errstr,"Cannot read process no. sent from TK\n");
//				if(!sloom)
//					fprintf(stderr,errstr);
				return(DATA_ERROR);
			}
			break;

		case(2):	
			if(sscanf(argv[cnt],"%d",&dz->mode)!=1) {
				sprintf(errstr,"Cannot read mode no. sent from TK\n");
//				if(!sloom)
//					fprintf(stderr,errstr);
				return(DATA_ERROR);
			}
			if(dz->mode > 0)
				dz->mode--;
			//setup_particular_application() =
			if((exit_status = setup_spectrum_application(dz))<0)
				return(exit_status);
			ap = dz->application;
			break;

		case(3):	
			if(sscanf(argv[cnt],"%d",&infilecnt)!=1) {
				sprintf(errstr,"Cannot read infilecnt sent from TK\n");
//				if(!sloom)
//					fprintf(stderr,errstr);
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
//				if(!sloom)
//					fprintf(stderr,errstr);
				return(DATA_ERROR);
			}
			break;
		case(INPUT_FILESIZE+4):	
			if(sscanf(argv[cnt],"%d",&filesize)!=1) {
				sprintf(errstr,"Cannot read infilesize sent from TK\n");
//				if(!sloom)
//					fprintf(stderr,errstr);
				return(DATA_ERROR);
			}
			dz->insams[0] = filesize;	
			break;
		case(INPUT_INSAMS+4):	
			if(sscanf(argv[cnt],"%d",&insams)!=1) {
				sprintf(errstr,"Cannot read insams sent from TK\n");
//				if(!sloom)
//					fprintf(stderr,errstr);
				return(DATA_ERROR);
			}
			dz->insams[0] = insams;	
			break;
		case(INPUT_SRATE+4):	
			if(sscanf(argv[cnt],"%d",&dz->infile->srate)!=1) {
				sprintf(errstr,"Cannot read srate sent from TK\n");
//				if(!sloom)
//					fprintf(stderr,errstr);
				return(DATA_ERROR);
			}
			break;
		case(INPUT_CHANNELS+4):	
			if(sscanf(argv[cnt],"%d",&dz->infile->channels)!=1) {
				sprintf(errstr,"Cannot read channels sent from TK\n");
//				if(!sloom)
//					fprintf(stderr,errstr);
				return(DATA_ERROR);
			}
			break;
		case(INPUT_STYPE+4):	
			if(sscanf(argv[cnt],"%d",&dz->infile->stype)!=1) {
				sprintf(errstr,"Cannot read stype sent from TK\n");
//				if(!sloom)
//					fprintf(stderr,errstr);
				return(DATA_ERROR);
			}
			break;
		case(INPUT_ORIGSTYPE+4):	
			if(sscanf(argv[cnt],"%d",&dz->infile->origstype)!=1) {
				sprintf(errstr,"Cannot read origstype sent from TK\n");
//				if(!sloom)
//					fprintf(stderr,errstr);
				return(DATA_ERROR);
			}
			break;
		case(INPUT_ORIGRATE+4):	
			if(sscanf(argv[cnt],"%d",&dz->infile->origrate)!=1) {
				sprintf(errstr,"Cannot read origrate sent from TK\n");
//				if(!sloom)
//					fprintf(stderr,errstr);
				return(DATA_ERROR);
			}
			break;
		case(INPUT_MLEN+4):	
			if(sscanf(argv[cnt],"%d",&dz->infile->Mlen)!=1) {
				sprintf(errstr,"Cannot read Mlen sent from TK\n");
//				if(!sloom)
//					fprintf(stderr,errstr);
				return(DATA_ERROR);
			}
			break;
		case(INPUT_DFAC+4):	
			if(sscanf(argv[cnt],"%d",&dz->infile->Dfac)!=1) {
				sprintf(errstr,"Cannot read Dfac sent from TK\n");
//				if(!sloom)
//					fprintf(stderr,errstr);
				return(DATA_ERROR);
			}
			break;
		case(INPUT_ORIGCHANS+4):	
			if(sscanf(argv[cnt],"%d",&dz->infile->origchans)!=1) {
				sprintf(errstr,"Cannot read origchans sent from TK\n");
//				if(!sloom)
//					fprintf(stderr,errstr);
				return(DATA_ERROR);
			}
			break;
		case(INPUT_SPECENVCNT+4):	
			if(sscanf(argv[cnt],"%d",&dz->infile->specenvcnt)!=1) {
				sprintf(errstr,"Cannot read specenvcnt sent from TK\n");
//				if(!sloom)
//					fprintf(stderr,errstr);
				return(DATA_ERROR);
			}
			dz->specenvcnt = dz->infile->specenvcnt;
			break;
		case(INPUT_WANTED+4):	
			if(sscanf(argv[cnt],"%d",&dz->wanted)!=1) {
				sprintf(errstr,"Cannot read wanted sent from TK\n");
//				if(!sloom)
//					fprintf(stderr,errstr);
				return(DATA_ERROR);
			}
			break;
		case(INPUT_WLENGTH+4):	
			if(sscanf(argv[cnt],"%d",&dz->wlength)!=1) {
				sprintf(errstr,"Cannot read wlength sent from TK\n");
//				if(!sloom)
//					fprintf(stderr,errstr);
				return(DATA_ERROR);
			}
			break;
		case(INPUT_OUT_CHANS+4):	
			if(sscanf(argv[cnt],"%d",&dz->out_chans)!=1) {
				sprintf(errstr,"Cannot read out_chans sent from TK\n");
//				if(!sloom)
//					fprintf(stderr,errstr);
				return(DATA_ERROR);
			}
			break;
			/* RWD these chanegs to samps - tk will have to deal with that! */
		case(INPUT_DESCRIPTOR_BYTES+4):	
			if(sscanf(argv[cnt],"%d",&dz->descriptor_samps)!=1) {
				sprintf(errstr,"Cannot read descriptor_samps sent from TK\n");
//				if(!sloom)
//					fprintf(stderr,errstr);
				return(DATA_ERROR);
			}
			break;
		case(INPUT_IS_TRANSPOS+4):	
			if(sscanf(argv[cnt],"%d",&dz->is_transpos)!=1) {
				sprintf(errstr,"Cannot read is_transpos sent from TK\n");
//				if(!sloom)
//					fprintf(stderr,errstr);
				return(DATA_ERROR);
			}
			break;
		case(INPUT_COULD_BE_TRANSPOS+4):	
			if(sscanf(argv[cnt],"%d",&dz->could_be_transpos)!=1) {
				sprintf(errstr,"Cannot read could_be_transpos sent from TK\n");
//				if(!sloom)
//					fprintf(stderr,errstr);
				return(DATA_ERROR);
			}
			break;
		case(INPUT_COULD_BE_PITCH+4):	
			if(sscanf(argv[cnt],"%d",&dz->could_be_pitch)!=1) {
				sprintf(errstr,"Cannot read could_be_pitch sent from TK\n");
//				if(!sloom)
//					fprintf(stderr,errstr);
				return(DATA_ERROR);
			}
			break;
		case(INPUT_DIFFERENT_SRATES+4):	
			if(sscanf(argv[cnt],"%d",&dz->different_srates)!=1) {
				sprintf(errstr,"Cannot read different_srates sent from TK\n");
//				if(!sloom)
//					fprintf(stderr,errstr);
				return(DATA_ERROR);
			}
			break;
		case(INPUT_DUPLICATE_SNDS+4):	
			if(sscanf(argv[cnt],"%d",&dz->duplicate_snds)!=1) {
				sprintf(errstr,"Cannot read duplicate_snds sent from TK\n");
//				if(!sloom)
//					fprintf(stderr,errstr);
				return(DATA_ERROR);
			}
			break;
		case(INPUT_BRKSIZE+4):	
			if(sscanf(argv[cnt],"%d",&inbrksize)!=1) {
				sprintf(errstr,"Cannot read brksize sent from TK\n");
//				if(!sloom)
//					fprintf(stderr,errstr);
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
//						if(!sloom)
//							fprintf(stderr,errstr);
						return(DATA_ERROR);
					}
					if(dz->brksize == NULL) {
						sprintf(errstr,"CDP has not established storage space for input brktable.\n");
//						if(!sloom)
//							fprintf(stderr,errstr);
						return(PROGRAM_ERROR);
					}
					dz->brksize[dz->extrabrkno]	= inbrksize;
					break;
				default:
					sprintf(errstr,"TK sent brktablesize > 0 for input_data_type [%d] not using brktables.\n",dz->input_data_type);
//					if(!sloom)
//						fprintf(stderr,errstr);
					return(PROGRAM_ERROR);
				}
				break;
			}
			break;
		case(INPUT_NUMSIZE+4):	
			if(sscanf(argv[cnt],"%d",&dz->numsize)!=1) {
				sprintf(errstr,"Cannot read numsize sent from TK\n");
//				if(!sloom)
//					fprintf(stderr,errstr);
				return(DATA_ERROR);
			}
			break;
		case(INPUT_LINECNT+4):	
			if(sscanf(argv[cnt],"%d",&dz->linecnt)!=1) {
				sprintf(errstr,"Cannot read linecnt sent from TK\n");
//				if(!sloom)
//					fprintf(stderr,errstr);
				return(DATA_ERROR);
			}
			break;
		case(INPUT_ALL_WORDS+4):	
			if(sscanf(argv[cnt],"%d",&dz->all_words)!=1) {
				sprintf(errstr,"Cannot read all_words sent from TK\n");
//				if(!sloom)
//					fprintf(stderr,errstr);
				return(DATA_ERROR);
			}
			break;
		case(INPUT_ARATE+4):	
			if(sscanf(argv[cnt],"%f",&dz->infile->arate)!=1) {
				sprintf(errstr,"Cannot read arate sent from TK\n");
//				if(!sloom)
//					fprintf(stderr,errstr);
				return(DATA_ERROR);
			}
			break;
		case(INPUT_FRAMETIME+4):	
			if(sscanf(argv[cnt],"%lf",&dummy)!=1) {
				sprintf(errstr,"Cannot read frametime sent from TK\n");
//				if(!sloom)
//					fprintf(stderr,errstr);
				return(DATA_ERROR);
			}
			dz->frametime = (float)dummy;
			break;
		case(INPUT_WINDOW_SIZE+4):	
			if(sscanf(argv[cnt],"%f",&dz->infile->window_size)!=1) {
				sprintf(errstr,"Cannot read window_size sent from TK\n");
//				if(!sloom)
//					fprintf(stderr,errstr);
					return(DATA_ERROR);
			}
			break;
		case(INPUT_NYQUIST+4):	
			if(sscanf(argv[cnt],"%lf",&dz->nyquist)!=1) {
				sprintf(errstr,"Cannot read nyquist sent from TK\n");
//				if(!sloom)
//					fprintf(stderr,errstr);
				return(DATA_ERROR);
			}
			break;
		case(INPUT_DURATION+4):	
			if(sscanf(argv[cnt],"%lf",&dz->duration)!=1) {
				sprintf(errstr,"Cannot read duration sent from TK\n");
//				if(!sloom)
//					fprintf(stderr,errstr);
				return(DATA_ERROR);
			}
			break;
		case(INPUT_MINBRK+4):	
			if(sscanf(argv[cnt],"%lf",&dz->minbrk)!=1) {
				sprintf(errstr,"Cannot read minbrk sent from TK\n");
//				if(!sloom)
//					fprintf(stderr,errstr);
				return(DATA_ERROR);
			}
			break;
		case(INPUT_MAXBRK+4):	
			if(sscanf(argv[cnt],"%lf",&dz->maxbrk)!=1) {
				sprintf(errstr,"Cannot read maxbrk sent from TK\n");
//				if(!sloom)
//					fprintf(stderr,errstr);
				return(DATA_ERROR);
			}
			break;
		case(INPUT_MINNUM+4):	
			if(sscanf(argv[cnt],"%lf",&dz->minnum)!=1) {
				sprintf(errstr,"Cannot read minnum sent from TK\n");
//				if(!sloom)
//					fprintf(stderr,errstr);
				return(DATA_ERROR);
			}
			break;
		case(INPUT_MAXNUM+4):	
			if(sscanf(argv[cnt],"%lf",&dz->maxnum)!=1) {
				sprintf(errstr,"Cannot read maxnum sent from TK\n");
//				if(!sloom)
//					fprintf(stderr,errstr);
				return(DATA_ERROR);
			}
			break;
		default:
			sprintf(errstr,"case switch item missing: parse_sloom_data()\n");
//			if(!sloom)
//				fprintf(stderr,errstr);
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
//			if(!sloom)
//				fprintf(stderr,errstr);
			return(MEMORY_ERROR);
		}
	} else {
		if((*cmdline = (char **)realloc(*cmdline,((*cmdlinecnt)+1) * sizeof(char *)))==NULL)	{
			sprintf(errstr,"INSUFFICIENT MEMORY for TK cmdline array.\n");
//			if(!sloom)
//				fprintf(stderr,errstr);
			return(MEMORY_ERROR);
		}
	}
	if(((*cmdline)[*cmdlinecnt] = (char *)malloc((strlen(q) + 1) * sizeof(char)))==NULL)	{
		sprintf(errstr,"INSUFFICIENT MEMORY for TK cmdline item %d.\n",(*cmdlinecnt)+1);
//		if(!sloom)
//			fprintf(stderr,errstr);
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
	"\nCREATE SPECTRA FROM TEXT DATA\n\n"
	"USAGE: spectrum NAME outanalfile datafile pointcnt srate dur parameters\n"
	"\n"
	"where NAME can be any one of\n"
	"\n"
	"format  fixed   varying  lines\n\n"
	"Type 'spectrum format' for more info on spectrum format..ETC.\n");
	return(USAGE_ONLY);
}

/**************************** CHECK_SPECTRUM_PARAM_VALIDITY_AND_CONSISTENCY *****************************/

int check_spectrum_param_validity_and_consistency(int **perm,dataptr dz)
{
	int k, j, i, n, OK = 0;
	double lastpkfrq;
	if(dz->process == SPEKLINE && dz->mode > 0)
		return FINISHED;
	k = 1;
	while(k < dz->iparam[SPEKPOINTS]) {
		k *= 2;
		if(k == dz->iparam[SPEKPOINTS]){
			OK= 1;
			break;
		}
	}
	if(!OK) {
		sprintf(errstr,"ANALYSIS POINTS PARAMETER MUST BE A POWER OF TWO.\n");
		return(DATA_ERROR);
	}
 	if(dz->iparam[1] < 44100 || BAD_SR(dz->iparam[1])) {
		sprintf(errstr,"INVALID SAMPLE RATE ENTERED (44100,48000,88200,96000 only).\n");
		return(DATA_ERROR);
	}
	if(dz->process == SPEKLINE) {
		dz->iparam[SPEKHARMS]++;
		return FINISHED;
	}
	if(dz->process != SPEKFRMT)	{ 
		dz->iparam[SPEKHARMS]++;	// Change count of harmonics to multiplier of max harmonic
		if(dz->param[SPEKRANDF] > 1.0 && flteq(dz->param[SPEKRANDA],0.0)) {
			sprintf(errstr,"NO MAX ATTENUATION EXCURSION SET FOR CHANNELS THAT VARY FROM AMPLITUDE ENVELOPE\n");
			return(DATA_ERROR);
		}
		if(dz->param[SPEKRANDA] > 1.0 && flteq(dz->param[SPEKRANDF],0.0)) {
			sprintf(errstr,"MAX AMPLITUDE EXCURSION SET, BUT PROPORTION OF CHANNELS TO FALL BELOW ENVELOPE, NOT SET.\n");
			return(DATA_ERROR);
		}
		if((*perm = (int *)malloc(dz->iparam[SPEKPOINTS]*sizeof(int)))==NULL) {
			sprintf(errstr,"NO MEMORY FOR CHANNEL PERMUTATIONS\n");
			return(DATA_ERROR);
		}
		if(dz->brksize[SPEKWIDTH] > 0) {
			lastpkfrq = 0.0;
			for(j=0,k=1,i=1;j<dz->brksize[SPEKWIDTH] * 2; j+=2, k+=2, i++) {
				n= (int)round(dz->brk[SPEKWIDTH][j]); 
				if(dz->brk[SPEKWIDTH][j] <= lastpkfrq) {
					sprintf(errstr,"SPECTRUM PEAK FRQ %d (%lf) IN WIDTH-ASPECT FILE IS INVALID OR NOT INCREASING\n",i,dz->brk[SPEKWIDTH][j]);
					return(DATA_ERROR);
				}
				lastpkfrq = dz->brk[SPEKWIDTH][j];
				if(dz->brk[SPEKWIDTH][k] > dz->param[SPEKMXASP]) {
					sprintf(errstr,"SPECTRUM PEAK %d WIDTH-ASPECT (%lf) EXCEEDS TOP OF RANGE (%lf)\n",i,dz->brk[SPEKWIDTH][k],dz->param[SPEKMXASP]);
					return(DATA_ERROR);
				}
			}
		}
		k = dz->vflag[1] + dz->vflag[2] + dz->vflag[3]; 
		if(k > 1) {
			sprintf(errstr,"w,s AND v FLAGS CANNOT BE USED IN COMBINATION WITH ONE ANOTHER.\n");
			return(DATA_ERROR);
		} else if(k > 0) {
			if(!dz->brksize[SPEKWIDTH]) {
				sprintf(errstr,"w,s AND v FLAGS CANNOT BE USED WITHOUT PEAK WIDTH DATA.\n");
				return(DATA_ERROR);
			}
		}
	}
	return(FINISHED);
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	if(!strcmp(prog_identifier_from_cmdline,"fixed"))					dz->process = SPEKTRUM;
	else if(!strcmp(prog_identifier_from_cmdline,"varying"))			dz->process = SPEKVARY;
	else if(!strcmp(prog_identifier_from_cmdline,"format"))				dz->process = SPEKFRMT;
	else if(!strcmp(prog_identifier_from_cmdline,"lines"))				dz->process = SPEKLINE;
	else {
		sprintf(errstr,"Unknown program identification string '%s'\n",prog_identifier_from_cmdline);
//		if(!sloom)
//			fprintf(stderr,errstr);
		return(USAGE_ONLY);
	}
	return(FINISHED);
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
	if(!strcmp(str,"format")) {
		fprintf(stdout,
		"USAGE: spectrum format outdatafile indatafile pointcnt srate\n"
		"\n"
		"Convert data in \"frq amp\" format to special format for sound conversion.\n"
		"Assume data represents a graph of frq against amplitude.\n"
		"SEE ALSO \"spectrum lines\"\n"
		"\n"
		"INDATAFILE   text data as \"frq amp\" pairs.\n"
		"             frq range 0 - nyquist, with frequencies always increasing.\n"
		"             amp range, >= 0 .\n"
		"OUTDATAFILE  textfile as \"frq amp\" pairs\n"
		"             showing peaks, troughs and amplitude envelope.\n"
		"             Local peaks indicated by peak frq and amplitude.\n"
		"             Local troughs indicated by -(frq) and amplitude.\n"
		"             Elsewhere frq = zero (i.e. unspecific frq within channel)\n"
		"                       amp = amplitude at channel centre frq.\n"
		"POINTCNT     Number of analysis points per window.\n"
		"SRATE        Samplerate of sound which will eventually be generated.\n"
		"\n"
		"Ideally, input should have many more frq points than pointcnt in output\n"
		"if peaks and troughs of spectrum are to be accurately located.\n"
		"\n");
	} else if(!strcmp(str,"fixed")) {
		fprintf(stdout,
		"USAGE: spectrum fixed outanalfile datafile pointcnt srate dur\n"
		"                [-hH] [-bB] [-fF] [-rR] [-sS] [-aA] [-tT] [-wW] [-mM] [-d]\n"
		"Generate fixed spectrum, from input data.\n"
		"\n"
		"DATAFILE  textfile as \"frq amp\" pairs\n"
		"          showing peaks, troughs and amplitude envelope.\n"
		"          Local peaks indicated by peak frq and amplitude.\n"
		"          Local troughs indicated by -(frq) and amplitude.\n"
		"          Elsewhere frq = zero (i.e. unspecific frq within channel)\n"
		"                    amp = amplitude at channel centre frq.\n"
		"POINTCNT  Number of analysis points per window.\n"
		"SRATE     Samplerate of sound which would be generated from anal output data.\n"
		"DURATION  Duration of sound output.\n"
		"H    Number of HARMONICS of peaks to be emphasized.\n"
		"B    BRIGHTNESS: relative loudness of successive harmonics.\n"
		"     (For TYPES 1,4 & 5 - see below - this is max brightness applied).\n"
		"F    FRACTION of analchans whose level falls below amp contour of spectrum.\n"
		"R    Max RANDOM attenuation excursion of amplitude in those channels.\n"
		"S    Freq SPREAD of peaks (0.0001 to 0.1)\n"
		"A    ATTENUATION applied to the spectrum.\n"
		"T    TYPE of spectrum:\n"
		"   0 Brightness as specified.\n"
		"   1 Brightness depend on peakwidth.\n"
		"   2 Peak freq randonly varies (with time) across peak-width.\n"
		"   3 Ditto, plus harmonics randvary independently of peak frq.\n"
		"   4 Case 1 + Case 2\n"
		"   5 Case 1 + Case 3\n"
		"Following data only useful if t flag > 0\n"
		"W    WIDTH of peaks (as aspect ratio): zero means no peak info \n"
		"     otherwise expects a brkpoint file of pairs of\n"
		"     peak-frq and peak-aspect (pkheight/pkwidth/nyquist)\n"
		"M    MAXIMUM peakwidth aspect (corresponding to max brightness)\n"
		"\n"
		"-d   Minimal dovetailing of output sound.\n"
		"\n");
	} else if(!strcmp(str,"varying")) {
		fprintf(stdout,
	    "USAGE: spectrum variable outanalfile datafile pointcnt srate dur\n"
		"                [-hH] [-bB] [-fF] [-rR] [-aA] [-zZ] [-v]\n"
		"Generate time-varying spectrum, from input data.\n"
		"\n"
		"DATAFILE  Is a textfile with a series of spectra as text data\n"
		"          listed as \"frq amp\" pairs, showing peaks, troughs\n"
		"          and amplitude envelope.\n"
		"          Local peaks indicated by peak frq and amplitude.\n"
		"          Local troughs indicated by -(frq) and amplitude.\n"
		"          Elsewhere frq = zero (i.e. unspecific frq within channel)\n"
		"                    amp = amplitude at channel centre frq.\n"
		"POINTCNT  Number of analysis points per window.\n"
		"SRATE     Samplerate of sound which would be generated from anal output data.\n"
		"DURATION  Duration of sound output.\n"
		"H    Number of HARMONICS of peaks to be emphasized.\n"
		"B    BRIGHTNESS: relative loudness of successive harmonics.\n"
		"F    FRACTION of analchans whose level falls below amp contour of spectrum.\n"
		"R    Max RANDOM attenuation excursion of amplitude in those channels..\n"
		"A    ATTENUATION applied to the spectrum.\n"
		"Z    Rate of ZOOM to new peak, if peak-chan changes.\n"
		"     1 = linear: more than 1 zooms most rapidly near to peakwindow time:\n"
		"     less than 1 zooms least rapidly near to peakwindow time:\n"
		"-v   Minimal dovetailing of output sound.\n"
		"\n");
	} else if(!strcmp(str,"lines")) {
		fprintf(stdout,
		"USAGE: spectrum lines 1 outanalfile indatafile pointcnt srate duration \n"
		"            harmonics rolloff datafoot datatop specfoot spectop max datawarp ampwarp\n"
		"USAGE: spectrum lines 2 outfiltfile indatafile duration \n"
		"            datafoot datatop specfoot spectop datawarp ampwarp\n"
		"\n"
		"Convert data in \"frq amp\" format representing spectral lines, to sound spectrum.\n"
		"(Mode 2 produces a varifilt filter-file \"equivalent\" to the spectrum).\n"
		"To convert a \"frq amp\" GRAPH to a spectrum ,see \"spectrum format\".\n"
		"\n"
		"INDATAFILE   text data as \"frq amp\" pairs.\n"
		"             frequencies always increasing.\n"
		"             amp range, >= 0 .\n"
		"OUTDATAFILE  textfile as \"frq amp\" pairs\n"
		"             showing peaks, troughs and amplitude envelope.\n"
		"             Local peaks indicated by peak frq and amplitude.\n"
		"             Local troughs indicated by -(frq) and amplitude.\n"
		"             Elsewhere frq = zero (i.e. unspecific frq within channel)\n"
		"                       amp = amplitude at channel centre frq.\n"
		"POINTCNT     Number of analysis points per window.\n"
		"SRATE        Samplerate of sound which will eventually be generated.\n"
		"DURATION     Duration of output spectrum.\n"
		"HARMONICS    Number of harmonics to add to original spectral components.\n"
		"ROLLOFF	  Amplitude loss from one harmonic to next (dB).\n"
		"DATAFOOT     Lowest frq in INPUT to be represented in output.\n"
		"DATATOP      Highest frq in INPUT to be represented in output.\n"
		"SPECFOOT     Lowest frq in OUTPUT representation of spectrum.\n"
		"SPECTOP      Highest frq in OUTPUT representation of spectrum.\n"
		"MAX          Amplitude of maximum spectral channel.\n"
		"             (input data amplitudes  first normalised to range 0 to 1.\n"
		"             these amplitudes are then scaled by \"max\"\n"
		"DATAWARP     If not equal to 1, input spectrum is mapped non-linearly...\n"
		"             if \"warp\" > 1, lower frqs are squeezed downwards.\n"
		"             if \"warp\" < 1, upper frqs are squeezed upwards.\n"
		"AMPWARP      If > 1, low input spectrum amplitudes are adjusted upwards.\n"
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

/******************************** INIT_SPECSYNTH (redundant)  ********************************/

void init_specsynth(dataptr dz)
{
	dz->outfile->origrate	= dz->infile->origrate  = (int)dz->iparam[SPEKSRATE];
	dz->outfile->stype		= dz->infile->stype		= SAMP_FLOAT;
	dz->outfile->origstype	= dz->infile->origstype = SAMP_SHORT;

	dz->outfile->Mlen		= dz->infile->Mlen		= dz->iparam[SPEKPOINTS];	// 1024
	dz->outfile->channels	= dz->infile->channels	= dz->outfile->Mlen + 2;	// 1026
	dz->wanted				= dz->infile->channels;
	dz->clength = dz->wanted/2;
	dz->nyquist = (double)(dz->outfile->origrate / 2);		
	dz->outfile->Dfac  = dz->infile->Dfac  = dz->infile->Mlen / 8;
	dz->outfile->arate = dz->infile->arate = (float) dz->outfile->origrate / (float)dz->outfile->Dfac;
	dz->outfile->srate = dz->infile->srate = (int) dz->outfile->arate;
	dz->frametime	= (float)(1.0/dz->outfile->arate);
	dz->wlength		= (int)round(dz->param[SPEKDUR] * dz->outfile->arate);
	dz->chwidth		= dz->nyquist/(double)(dz->clength- 1);
	dz->halfchwidth	= dz->chwidth / 2;
	dz->needpeaks = 0;
}

/**************************** ALLOCATE_SPECTRUM_BUFFER ******************************/

int allocate_spectrum_buffer(dataptr dz)
{
	dz->buflen = dz->wanted;
	if((dz->bigfbuf	= (float*) malloc(dz->buflen * sizeof(float)))==NULL) {  
		sprintf(errstr,"INSUFFICIENT MEMORY for sound buffers.\n");
//		if(!sloom)
//			fprintf(stderr,errstr);
		return(MEMORY_ERROR);
	}
	return(FINISHED);
}

/************************** HANDLE_THE_SPECIAL_DATA **********************************/

int handle_the_special_data(int *cmdlinecnt,char ***cmdline,dataptr dz)
{
	int cnt, linecnt;
	char *filename = (*cmdline)[0];
	FILE *fp;
	double *p, dummy;
	char temp[200], *q;
	if(filename[0]=='-' && filename[1]=='f') {
		dz->floatsam_output = 1;
		dz->true_outfile_stype = SAMP_FLOAT;
		filename+= 2;
	}
	if((fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,	"Can't open data file %s to read data.\n",filename);
//		if(!sloom)
//			fprintf(stderr,errstr);
		return(DATA_ERROR);
	}
	cnt = 0;
	p = &dummy;
	while(fgets(temp,200,fp)==temp) {
		q = temp;
		if(*q == ';')	//	Allow comments in file
			continue;
		while(get_float_with_e_from_within_string(&q,p))
			cnt++;
	}
	if(cnt == 0) {
		sprintf(errstr,"No data in data file %s\n",filename);
//		if(!sloom)
//			fprintf(stderr,errstr);
		return(DATA_ERROR);
	}
	if((dz->parray  = (double **)malloc(sizeof(double *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for input data in file %s.\n",filename);
//		if(!sloom)
//			fprintf(stderr,errstr);
		return(MEMORY_ERROR);
	}
	cnt += 4;		// Allow space for zero and nyquist values, if need to be added later
	if((dz->parray[0] = (double *)malloc(cnt * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for input data in file %s.\n",filename);
//		if(!sloom)
//			fprintf(stderr,errstr);
		return(MEMORY_ERROR);
	}
	cnt -= 4;
	fseek(fp,0,0);
	linecnt = 1;
	p = dz->parray[0];
	while(fgets(temp,200,fp)==temp) {
		q = temp;
		if(*q == ';')	//	Allow comments in file
			continue;
		while(get_float_with_e_from_within_string(&q,p)) {
			p++;
		}
		linecnt++;
	}
	if(fclose(fp)<0) {
		fprintf(stdout,"WARNING: Failed to close file %s.\n",filename);
		fflush(stdout);
	}
	dz->itemcnt = cnt;
	(*cmdline)++;
	(*cmdlinecnt)--;
	return(FINISHED);
}

/************************** TEST_THE_SPECIAL_DATA **********************************/

int test_the_special_data(dataptr dz)
{
	double *p, maxamp = 0.0, normaliser = 1.0,  lastfrq = -1.0;
	int isamp = 0, linecnt = 1, lastfrqindx = dz->itemcnt - 2;
	int n,m;
	p = dz->parray[0];
	while(linecnt <= dz->itemcnt/2) {
		if(isamp) {
			if(*p < 0.0) {
				sprintf(errstr,"Amp (%lf) out of range ( < 0 ) at line %d in datafile.\n",*p,linecnt);
				return(DATA_ERROR);
			}
			maxamp = max(*p,maxamp);
		} else {
			if(dz->process == SPEKLINE) {	/* TW: Can accept frq vals above output-nyquist */
				if(*p < 0.0) {
					sprintf(errstr,"Frq (%lf) out of range (< 0 ) at line %d in datafile.\n",*p,linecnt);
					return(DATA_ERROR);
				}
				if(*p < lastfrq) {
					sprintf(errstr,"Frqs (%lf) do not increase at line %d in datafile.\n",*p,linecnt);
					return(DATA_ERROR);
				}
				lastfrq = *p;
			} else if(dz->process == SPEKFRMT) {
				if(*p < 0.0 || *p > dz->nyquist) {
					sprintf(errstr,"Frq (%lf) out of range (0 - %lf) at line %d in datafile.\n",*p,dz->nyquist,linecnt);
					return(DATA_ERROR);
				}
				if(*p < lastfrq) {
					sprintf(errstr,"Frqs (%lf) do not increase at line %d in datafile.\n",*p,linecnt);
					return(DATA_ERROR);
				}
				lastfrq = *p;
			} else if(fabs(*p) > dz->nyquist) {
				sprintf(errstr,"Frq (%lf) out of range (0 - %lf) at line %d in datafile.\n",fabs(*p),dz->nyquist,linecnt);
				return(DATA_ERROR);
			} 
		}
		if(isamp)
			linecnt++;
		isamp = !isamp;
		p++;
	}
	if(maxamp > 1.0) {
		normaliser = 1.0/maxamp;
		p = dz->parray[0];
		p++;
		linecnt = 1;
		while(linecnt <= dz->itemcnt/2) {
			*p *= normaliser;
			linecnt++;
			p += 2;
		}
	}
	switch(dz->process) {
	case(SPEKFRMT):
		if(!flteq(dz->parray[0][lastfrqindx],dz->nyquist)) {		//	Space has been allocated for these extra points
			dz->parray[0][dz->itemcnt] = dz->nyquist;
			dz->itemcnt++;
			dz->parray[0][dz->itemcnt] = 0.0;
			dz->itemcnt++;
		}
		if(dz->parray[0][0] != 0.0) {
			dz->itemcnt += 2;
			n = dz->itemcnt - 1;
			m = n - 2;
			while(m >= 0) {
				dz->parray[0][n] = dz->parray[0][m];
				m--;
				n--;
			}
			dz->parray[0][0] = 0.0;
			dz->parray[0][1] = 0.0;
		}
		break;
	case(SPEKTRUM):
		if(dz->itemcnt != dz->wanted) {
			sprintf(errstr,"Data count (%d) should be just 2 more than analysis-points parameter (%d)\n",dz->itemcnt,dz->wanted - 2);
			return(DATA_ERROR);
		}
		break;
	case(SPEKVARY):
		if((dz->itemcnt / dz->wanted) * dz->wanted != dz->itemcnt) {
			sprintf(errstr,"Data count (%d) in file must be a multiple of (analysis_points_parameter + 2) (%d)\n",dz->itemcnt,dz->wanted);
			return(DATA_ERROR);
		}
		dz->itemcnt /= dz->wanted;
		break;
	}
	return(FINISHED);
}

/************************** SPECTRUM **********************************/

int spectrum(int *perm,dataptr dz)
{
	int vc, cc, aspindx, exit_status;
	double thistime, dfade, frq, chbot, harmamp, spreaddnratio = 0.0, spreadupratio = 0.0, fader;
	double orig_brightness, pkaspect, pkwidth, pkfrq, pkoffset;
	float orig_frequency;
	int *peaked, peakcnt, orig_peakcnt;

	/* ESTABLISH BRKPOINT TABLE OF TRUE AMPLITUDE ENVELOPE OF INPUT DATA */

	if((dz->brk[0] = (double *)malloc(dz->clength * 2 * sizeof(double)))==NULL) {
		sprintf(errstr,"Establishing brktable for amp envelope failed.\n");
//		if(!sloom)
//			fprintf(stderr,errstr);
		return(MEMORY_ERROR);
	}

	if((peaked = (int *)malloc(dz->clength * sizeof(int)))==NULL) {
		sprintf(errstr,"Establishing brktable for peak markers.\n");
//		if(!sloom)
//			fprintf(stderr,errstr);
		return(MEMORY_ERROR);
	}

	if(dz->param[SPEKSPRED] > 0) {
		spreaddnratio = 1 - dz->param[SPEKSPRED];
		spreadupratio = 1 + dz->param[SPEKSPRED];
	}
		
	dz->brksize[0] = dz->clength;
	dz->brkinit[0] = 0;
	frq = 0.0;
	for(cc = 0, vc = 0; cc < dz->clength; cc++, vc +=2) {
		dz->brk[0][vc+1] = dz->parray[0][vc+1];
		if(flteq(dz->parray[0][vc],0.0))					//	If input chan frq is unassigned (0.0) set frq to midfrq of chan
			dz->brk[0][vc] = frq;
		else												//	Else, set frq to absolute val of input frq (maxima are +frq, minima are -frq)
			dz->brk[0][vc] = fabs(dz->parray[0][vc]);
		frq += dz->chwidth;
	}

	/* ZERO START WINDOW */
	thistime = 0.0;
	frq = 0;
	for(cc = 0, vc = 0; cc < dz->clength; cc++, vc +=2) {
		dz->bigfbuf[AMPP] = 0.0f;
		dz->bigfbuf[FREQ] = (float)frq;
		frq += dz->chwidth;
	}
	if((exit_status = write_samps(dz->bigfbuf,dz->wanted,dz))<0)
		return(exit_status);

	thistime += dz->frametime;
	
	/* 88 WINDOW FADE-IN */

	if(!dz->is_rectified) {
		dfade = 4096;
		while(dfade > 1) {
			chbot = -dz->chwidth;
			for(cc = 0; cc < dz->clength; cc++)
				peaked[cc] = 0;
			peakcnt = 0;
			orig_peakcnt = 0;
			for(cc = 0, vc = 0; cc < dz->clength; cc++, vc +=2) {
				if(peaked[cc]) {
					if(cc > 0) {											//  IF already a marked peak (i.e. a harmonic of a previous peak)
						if(dz->parray[0][vc] > 0.0) {						//	BUT	this channel is a peak in orig data ... overwrite harmonic with new peak info
							dz->bigfbuf[AMPP] = (float)(dz->param[SPEKGAIN] * dz->parray[0][vc+1]/(double)dfade);
							dz->bigfbuf[FREQ] = (float)dz->parray[0][vc];
							pkwidth = 0.0;
							switch(dz->iparam[SPEKTYPE]) {
							case(0):	//	Brightness defined by brightness-rolloff param
								if(dz->iparam[SPEKHARMS] > 1 && dz->param[SPEKBRITE] > 0.0) {
									harmamp = dz->bigfbuf[AMPP] * dz->param[SPEKBRITE];
									insert_harmonics(&peaked,&peakcnt,harmamp,dz->bigfbuf[FREQ],pkwidth,dz);
								}
								break;
							case(1):	//	Brightness (also) determined by peak-width-aspect
								orig_brightness = dz->param[SPEKBRITE];
								read_nearest_value_from_brktable(dz->bigfbuf[FREQ],SPEKWIDTH,dz);
								pkaspect = dz->param[SPEKWIDTH];
								dz->param[SPEKBRITE] = (pkaspect/dz->param[SPEKMXASP]) * orig_brightness;
								if(dz->iparam[SPEKHARMS] > 1 && dz->param[SPEKBRITE] > 0.0) {
									harmamp = dz->bigfbuf[AMPP] * dz->param[SPEKBRITE];
									insert_harmonics(&peaked,&peakcnt,harmamp,dz->bigfbuf[FREQ],pkwidth,dz);
								}
								dz->param[SPEKBRITE] = orig_brightness;
								break;
							case(2):	//	Brightness as case 0 : frq of peaks random spread across peak-width
								read_nearest_value_from_brktable(dz->bigfbuf[FREQ],SPEKWIDTH,dz);
								pkaspect = dz->param[SPEKWIDTH];
								pkwidth = (dz->parray[0][vc+1]/pkaspect) * dz->nyquist;
								pkwidth = min(pkwidth,2 * dz->bigfbuf[FREQ]);	// Avoid -ve frqs
								pkoffset = drand48() * pkwidth;
								pkfrq  = dz->bigfbuf[FREQ] - (pkwidth/2.0);
								pkfrq += pkoffset;
								dz->bigfbuf[FREQ] = (float)pkfrq;
								if(dz->iparam[SPEKHARMS] > 1 && dz->param[SPEKBRITE] > 0.0) {
									harmamp = dz->bigfbuf[AMPP] * dz->param[SPEKBRITE];
									insert_harmonics(&peaked,&peakcnt,harmamp,dz->bigfbuf[FREQ],pkwidth,dz);
								}
								break;
							case(3):	//	As case 2 : frq of harmonics also random spread across peak-width
								orig_frequency  = dz->bigfbuf[FREQ];
								read_nearest_value_from_brktable(dz->bigfbuf[FREQ],SPEKWIDTH,dz);
								pkaspect = dz->param[SPEKWIDTH];
								pkwidth = (dz->parray[0][vc+1]/pkaspect) * dz->nyquist;
								pkwidth = min(pkwidth,2 * dz->bigfbuf[FREQ]);	// Avoid -ve frqs
								pkoffset = drand48() * pkwidth;
								pkfrq  = dz->bigfbuf[FREQ] - (pkwidth/2.0);
								pkfrq += pkoffset;
								dz->bigfbuf[FREQ] = (float)pkfrq;
								if(dz->iparam[SPEKHARMS] > 1 && dz->param[SPEKBRITE] > 0.0) {
									harmamp = dz->bigfbuf[AMPP] * dz->param[SPEKBRITE];
									insert_harmonics(&peaked,&peakcnt,harmamp,orig_frequency,pkwidth,dz);
								}
								break;
							case(4):	//	case 1 + case 2
								orig_brightness = dz->param[SPEKBRITE];
								read_nearest_value_from_brktable(dz->bigfbuf[FREQ],SPEKWIDTH,dz);
								pkaspect = dz->param[SPEKWIDTH];
								dz->param[SPEKBRITE] = (pkaspect/dz->param[SPEKMXASP]) * orig_brightness;
								pkwidth = (dz->parray[0][vc+1]/pkaspect) * dz->nyquist;
								pkwidth = min(pkwidth,2 * dz->bigfbuf[FREQ]);	// Avoid -ve frqs
								pkoffset = drand48() * pkwidth;
								pkfrq  = dz->bigfbuf[FREQ] - (pkwidth/2.0);
								pkfrq += pkoffset;
								dz->bigfbuf[FREQ] = (float)pkfrq;
								if(dz->iparam[SPEKHARMS] > 1 && dz->param[SPEKBRITE] > 0.0) {
									harmamp = dz->bigfbuf[AMPP] * dz->param[SPEKBRITE];
									insert_harmonics(&peaked,&peakcnt,harmamp,dz->bigfbuf[FREQ],pkwidth,dz);
								}
								dz->param[SPEKBRITE] = orig_brightness;
								break;
							case(5):	//	case 1 + case 3
								orig_brightness = dz->param[SPEKBRITE];
								orig_frequency  = dz->bigfbuf[FREQ];
								read_nearest_value_from_brktable(dz->bigfbuf[FREQ],SPEKWIDTH,dz);
								pkaspect = dz->param[SPEKWIDTH];
								dz->param[SPEKBRITE] = (pkaspect/dz->param[SPEKMXASP]) * orig_brightness;
								pkwidth = (dz->parray[0][vc+1]/pkaspect) * dz->nyquist;
								pkwidth = min(pkwidth,2 * dz->bigfbuf[FREQ]);	// Avoid -ve frqs
								pkoffset = drand48() * pkwidth;
								pkfrq  = dz->bigfbuf[FREQ] - (pkwidth/2.0);
								pkfrq += pkoffset;
								dz->bigfbuf[FREQ] = (float)pkfrq;
								if(dz->iparam[SPEKHARMS] > 1 && dz->param[SPEKBRITE] > 0.0) {
									harmamp = dz->bigfbuf[AMPP] * dz->param[SPEKBRITE];
									insert_harmonics(&peaked,&peakcnt,harmamp,orig_frequency,pkwidth,dz);
								}
								dz->param[SPEKBRITE] = orig_brightness;
								break;
							}
						}
					}
				} else if(dz->parray[0][vc] <= 0.0) {					//	Frq unspecific (marked as 0.0), assign rand frq within chan
					frq = drand48() * dz->chwidth;
					frq += chbot;
					frq = max(frq,0.0);
					frq = min(frq,dz->nyquist);
					dz->bigfbuf[FREQ] = (float)frq;
					if((exit_status = read_value_from_brktable(frq,0,dz))<0)	//	Read amplitude from frq/amp brktable
						return(exit_status);
					dz->bigfbuf[AMPP] = (float)(dz->param[SPEKGAIN] * dz->param[0]/(double)dfade);	// NB dz->param[0] being used to store brkpnt read val
				} else {														//	Frq is a peak, retain it and its amplitude	
					dz->bigfbuf[AMPP] = (float)(dz->param[SPEKGAIN] * dz->parray[0][vc+1]/(double)dfade);
					dz->bigfbuf[FREQ] = (float)dz->parray[0][vc];
					peaked[cc] = 1;
					pkwidth = 0.0;
					orig_peakcnt++;
					peakcnt++;
					switch(dz->iparam[SPEKTYPE]) {
					case(0):	//	Brightness defined by brightness-rolloff param
						if(dz->iparam[SPEKHARMS] > 1 && dz->param[SPEKBRITE] > 0.0) {
							harmamp = dz->bigfbuf[AMPP] * dz->param[SPEKBRITE];
							insert_harmonics(&peaked,&peakcnt,harmamp,dz->bigfbuf[FREQ],pkwidth,dz);
						}
						break;
					case(1):	//	Brightness (also) determined by peak-width-aspect
						orig_brightness = dz->param[SPEKBRITE];
						read_nearest_value_from_brktable(dz->bigfbuf[FREQ],SPEKWIDTH,dz);
						pkaspect = dz->param[SPEKWIDTH];
						dz->param[SPEKBRITE] = (pkaspect/dz->param[SPEKMXASP]) * orig_brightness;
						if(dz->iparam[SPEKHARMS] > 1 && dz->param[SPEKBRITE] > 0.0) {
							harmamp = dz->bigfbuf[AMPP] * dz->param[SPEKBRITE];
							insert_harmonics(&peaked,&peakcnt,harmamp,dz->bigfbuf[FREQ],pkwidth,dz);
						}
						dz->param[SPEKBRITE] = orig_brightness;
						break;
					case(2):	//	Brightness as case 0 : frq of peaks random spread across peak-width
						read_nearest_value_from_brktable(dz->bigfbuf[FREQ],SPEKWIDTH,dz);
						pkaspect = dz->param[SPEKWIDTH];
						pkwidth = (dz->parray[0][vc+1]/pkaspect) * dz->nyquist;
						pkwidth = min(pkwidth,2 * dz->bigfbuf[FREQ]);	// Avoid -ve frqs
						pkoffset = drand48() * pkwidth;
						pkfrq  = dz->bigfbuf[FREQ] - (pkwidth/2.0);
						pkfrq += pkoffset;
						dz->bigfbuf[FREQ] = (float)pkfrq;
						if(dz->iparam[SPEKHARMS] > 1 && dz->param[SPEKBRITE] > 0.0) {
							harmamp = dz->bigfbuf[AMPP] * dz->param[SPEKBRITE];
							insert_harmonics(&peaked,&peakcnt,harmamp,dz->bigfbuf[FREQ],pkwidth,dz);
						}
						break;
					case(3):	//	As case 2 : frq of harmonics also random spread across peak-width
						orig_frequency  = dz->bigfbuf[FREQ];
						read_nearest_value_from_brktable(dz->bigfbuf[FREQ],SPEKWIDTH,dz);
						pkaspect = dz->param[SPEKWIDTH];
						pkwidth = (dz->parray[0][vc+1]/pkaspect) * dz->nyquist;
						pkwidth = min(pkwidth,2 * dz->bigfbuf[FREQ]);	// Avoid -ve frqs
						pkoffset = drand48() * pkwidth;
						pkfrq  = dz->bigfbuf[FREQ] - (pkwidth/2.0);
						pkfrq += pkoffset;
						dz->bigfbuf[FREQ] = (float)pkfrq;
						if(dz->iparam[SPEKHARMS] > 1 && dz->param[SPEKBRITE] > 0.0) {
							harmamp = dz->bigfbuf[AMPP] * dz->param[SPEKBRITE];
							insert_harmonics(&peaked,&peakcnt,harmamp,orig_frequency,pkwidth,dz);
						}
						break;
					case(4):	//	case 1 + case 2
						orig_brightness = dz->param[SPEKBRITE];
						read_nearest_value_from_brktable(dz->bigfbuf[FREQ],SPEKWIDTH,dz);
						pkaspect = dz->param[SPEKWIDTH];
						dz->param[SPEKBRITE] = (pkaspect/dz->param[SPEKMXASP]) * orig_brightness;
						pkwidth = (dz->parray[0][vc+1]/pkaspect) * dz->nyquist;
						pkwidth = min(pkwidth,2 * dz->bigfbuf[FREQ]);	// Avoid -ve frqs
						pkoffset = drand48() * pkwidth;
						pkfrq  = dz->bigfbuf[FREQ] - (pkwidth/2.0);
						pkfrq += pkoffset;
						dz->bigfbuf[FREQ] = (float)pkfrq;
						if(dz->iparam[SPEKHARMS] > 1 && dz->param[SPEKBRITE] > 0.0) {
							harmamp = dz->bigfbuf[AMPP] * dz->param[SPEKBRITE];
							insert_harmonics(&peaked,&peakcnt,harmamp,dz->bigfbuf[FREQ],pkwidth,dz);
						}
						dz->param[SPEKBRITE] = orig_brightness;
						break;
					case(5):	//	case 1 + case 3
						orig_brightness = dz->param[SPEKBRITE];
						orig_frequency  = dz->bigfbuf[FREQ];
						read_nearest_value_from_brktable(dz->bigfbuf[FREQ],SPEKWIDTH,dz);
						pkaspect = dz->param[SPEKWIDTH];
						dz->param[SPEKBRITE] = (pkaspect/dz->param[SPEKMXASP]) * orig_brightness;
						pkwidth = (dz->parray[0][vc+1]/pkaspect) * dz->nyquist;
						pkwidth = min(pkwidth,2 * dz->bigfbuf[FREQ]);	// Avoid -ve frqs
						pkoffset = drand48() * pkwidth;
						pkfrq  = dz->bigfbuf[FREQ] - (pkwidth/2.0);
						pkfrq += pkoffset;
						dz->bigfbuf[FREQ] = (float)pkfrq;
						if(dz->iparam[SPEKHARMS] > 1 && dz->param[SPEKBRITE] > 0.0) {
							harmamp = dz->bigfbuf[AMPP] * dz->param[SPEKBRITE];
							insert_harmonics(&peaked,&peakcnt,harmamp,orig_frequency,pkwidth,dz);
						}
						dz->param[SPEKBRITE] = orig_brightness;
						break;
					}
				}
				chbot += dz->chwidth;
			}
			if(dz->param[SPEKRANDF] > 0.0)
				randomiseamps(perm,peaked,dz);

			if(dz->param[SPEKSPRED] > 0)
				spread_peaks(peaked,spreaddnratio,spreadupratio,dz);

			if((exit_status = write_samps(dz->bigfbuf,dz->wanted,dz))<0)
				return(exit_status);
			dfade /= 1.1;
			thistime += dz->frametime;
		}
	}
	while(thistime < dz->param[SPEKDUR]) {
		chbot = -dz->chwidth;
		for(cc = 0; cc < dz->clength; cc++)
			peaked[cc] = 0;
		peakcnt = 0;
		orig_peakcnt = 0;
		for(cc = 0, vc = 0; cc < dz->clength; cc++, vc +=2) {
			if(peaked[cc]) {
				if(cc > 0) {											//  IF already a marked peak (i.e. a harmonic of a previous peak)
					if(dz->parray[0][vc] > 0.0) {						//	BUT	this channel is a peak in orig data ... overwrite harmonic with new peak info
						dz->bigfbuf[AMPP] = (float)(dz->param[SPEKGAIN] * dz->parray[0][vc+1]);
						dz->bigfbuf[FREQ] = (float)dz->parray[0][vc];
						pkwidth = 0.0;
						aspindx = (orig_peakcnt * 2) + 1;
						switch(dz->iparam[SPEKTYPE]) {
						case(0):	//	Brightness defined by brightness-rolloff param
							if(dz->iparam[SPEKHARMS] > 1 && dz->param[SPEKBRITE] > 0.0) {
								harmamp = dz->bigfbuf[AMPP] * dz->param[SPEKBRITE];
								insert_harmonics(&peaked,&peakcnt,harmamp,dz->bigfbuf[FREQ],pkwidth,dz);
							}
							break;
						case(1):	//	Brightness (also) determined by peak-width-aspect
							orig_brightness = dz->param[SPEKBRITE];
							read_nearest_value_from_brktable(dz->bigfbuf[FREQ],SPEKWIDTH,dz);
							pkaspect = dz->param[SPEKWIDTH];
							dz->param[SPEKBRITE] = (pkaspect/dz->param[SPEKMXASP]) * orig_brightness;
							if(dz->iparam[SPEKHARMS] > 1 && dz->param[SPEKBRITE] > 0.0) {
								harmamp = dz->bigfbuf[AMPP] * dz->param[SPEKBRITE];
								insert_harmonics(&peaked,&peakcnt,harmamp,dz->bigfbuf[FREQ],pkwidth,dz);
							}
							dz->param[SPEKBRITE] = orig_brightness;
							break;
						case(2):	//	Brightness as case 0 : frq of peaks random spread across peak-width
							read_nearest_value_from_brktable(dz->bigfbuf[FREQ],SPEKWIDTH,dz);
							pkaspect = dz->param[SPEKWIDTH];
							pkwidth = ((dz->parray[0][vc+1])/pkaspect) * dz->nyquist;
							pkwidth = min(pkwidth,2 * dz->bigfbuf[FREQ]);	// Avoid -ve frqs
							pkoffset = drand48() * pkwidth;
							pkfrq  = dz->bigfbuf[FREQ] - (pkwidth/2.0);
							pkfrq += pkoffset;
							dz->bigfbuf[FREQ] = (float)pkfrq;
							if(dz->iparam[SPEKHARMS] > 1 && dz->param[SPEKBRITE] > 0.0) {
								harmamp = dz->bigfbuf[AMPP] * dz->param[SPEKBRITE];
								insert_harmonics(&peaked,&peakcnt,harmamp,dz->bigfbuf[FREQ],pkwidth,dz);
							}
							break;
						case(3):	//	As case 2 : frq of harmonics also random spread across peak-width
							orig_frequency  = dz->bigfbuf[FREQ];
							read_nearest_value_from_brktable(dz->bigfbuf[FREQ],SPEKWIDTH,dz);
							pkaspect = dz->param[SPEKWIDTH];
							pkwidth = (dz->parray[0][vc+1]/pkaspect) * dz->nyquist;
							pkwidth = min(pkwidth,2 * dz->bigfbuf[FREQ]);	// Avoid -ve frqs
							pkoffset = drand48() * pkwidth;
							pkfrq  = dz->bigfbuf[FREQ] - (pkwidth/2.0);
							pkfrq += pkoffset;
							dz->bigfbuf[FREQ] = (float)pkfrq;
							if(dz->iparam[SPEKHARMS] > 1 && dz->param[SPEKBRITE] > 0.0) {
								harmamp = dz->bigfbuf[AMPP] * dz->param[SPEKBRITE];
								insert_harmonics(&peaked,&peakcnt,harmamp,orig_frequency,pkwidth,dz);
							}
							break;
						case(4):	//	case 1 + case 2
							orig_brightness = dz->param[SPEKBRITE];
							read_nearest_value_from_brktable(dz->bigfbuf[FREQ],SPEKWIDTH,dz);
							pkaspect = dz->param[SPEKWIDTH];
							dz->param[SPEKBRITE] = (pkaspect/dz->param[SPEKMXASP]) * orig_brightness;
							pkwidth = ((dz->parray[0][vc+1])/pkaspect) * dz->nyquist;
							pkwidth = min(pkwidth,2 * dz->bigfbuf[FREQ]);	// Avoid -ve frqs
							pkoffset = drand48() * pkwidth;
							pkfrq  = dz->bigfbuf[FREQ] - (pkwidth/2.0);
							pkfrq += pkoffset;
							dz->bigfbuf[FREQ] = (float)pkfrq;
							if(dz->iparam[SPEKHARMS] > 1 && dz->param[SPEKBRITE] > 0.0) {
								harmamp = dz->bigfbuf[AMPP] * dz->param[SPEKBRITE];
								insert_harmonics(&peaked,&peakcnt,harmamp,dz->bigfbuf[FREQ],pkwidth,dz);
							}
							dz->param[SPEKBRITE] = orig_brightness;
							break;
						case(5):	//	case 1 + case 3
							orig_brightness = dz->param[SPEKBRITE];
							orig_frequency  = dz->bigfbuf[FREQ];
							read_nearest_value_from_brktable(dz->bigfbuf[FREQ],SPEKWIDTH,dz);
							pkaspect = dz->param[SPEKWIDTH];
							dz->param[SPEKBRITE] = (pkaspect/dz->param[SPEKMXASP]) * orig_brightness;
							pkwidth = (dz->parray[0][vc+1]/pkaspect) * dz->nyquist;
							pkwidth = min(pkwidth,2 * dz->bigfbuf[FREQ]);	// Avoid -ve frqs
							pkoffset = drand48() * pkwidth;
							pkfrq  = dz->bigfbuf[FREQ] - (pkwidth/2.0);
							pkfrq += pkoffset;
							dz->bigfbuf[FREQ] = (float)pkfrq;
							if(dz->iparam[SPEKHARMS] > 1 && dz->param[SPEKBRITE] > 0.0) {
								harmamp = dz->bigfbuf[AMPP] * dz->param[SPEKBRITE];
								insert_harmonics(&peaked,&peakcnt,harmamp,orig_frequency,pkwidth,dz);
							}
							dz->param[SPEKBRITE] = orig_brightness;
							break;
						}
					}
				}
			} else if(dz->parray[0][vc] <= 0.0) {
				frq = drand48() * dz->chwidth;						//	Frq unspecific, assign rand frq within chan
				frq += chbot;
				frq = max(frq,0.0);
				frq = min(frq,dz->nyquist);
				dz->bigfbuf[FREQ] = (float)frq;
				if((exit_status = read_value_from_brktable(frq,0,dz))<0)	//	Read amplitude from frq/amp brktable
					return(exit_status);
				dz->bigfbuf[AMPP] = (float)(dz->param[SPEKGAIN] * dz->param[0]);
			} else {
				dz->bigfbuf[AMPP] = (float)(dz->param[SPEKGAIN] * dz->parray[0][vc+1]);	//	Peaks take on their true values
				dz->bigfbuf[FREQ] = (float)fabs(dz->parray[0][vc]);					//	BUT NB input data is frq/amp ... output data is amp/frq
				peaked[cc] = 1;
				pkwidth = 0.0;
				aspindx = (orig_peakcnt * 2) + 1;
				orig_peakcnt++;
				peakcnt++;
				switch(dz->iparam[SPEKTYPE]) {
				case(0):	//	Brightness defined by brightness-rolloff param
					if(dz->iparam[SPEKHARMS] > 1 && dz->param[SPEKBRITE] > 0.0) {
						harmamp = dz->bigfbuf[AMPP] * dz->param[SPEKBRITE];
						insert_harmonics(&peaked,&peakcnt,harmamp,dz->bigfbuf[FREQ],pkwidth,dz);
					}
					break;
				case(1):	//	Brightness (also) determined by peak-width-aspect
					orig_brightness = dz->param[SPEKBRITE];
					read_nearest_value_from_brktable(dz->bigfbuf[FREQ],SPEKWIDTH,dz);
					pkaspect = dz->param[SPEKWIDTH];
					dz->param[SPEKBRITE] = (pkaspect/dz->param[SPEKMXASP]) * orig_brightness;
					if(dz->iparam[SPEKHARMS] > 1 && dz->param[SPEKBRITE] > 0.0) {
						harmamp = dz->bigfbuf[AMPP] * dz->param[SPEKBRITE];
						insert_harmonics(&peaked,&peakcnt,harmamp,dz->bigfbuf[FREQ],pkwidth,dz);
					}
					dz->param[SPEKBRITE] = orig_brightness;
					break;
				case(2):	//	Brightness as case 0 : frq of peaks random spread across peak-width
					read_nearest_value_from_brktable(dz->bigfbuf[FREQ],SPEKWIDTH,dz);
					pkaspect = dz->param[SPEKWIDTH];
					pkwidth = (dz->parray[0][vc+1]/pkaspect) * dz->nyquist;
					pkwidth = min(pkwidth,2 * dz->bigfbuf[FREQ]);	// Avoid -ve frqs
					pkoffset = drand48() * pkwidth;
					pkfrq  = dz->bigfbuf[FREQ] - (pkwidth/2.0);
					pkfrq += pkoffset;
					dz->bigfbuf[FREQ] = (float)pkfrq;
					if(dz->iparam[SPEKHARMS] > 1 && dz->param[SPEKBRITE] > 0.0) {
						harmamp = dz->bigfbuf[AMPP] * dz->param[SPEKBRITE];
						insert_harmonics(&peaked,&peakcnt,harmamp,dz->bigfbuf[FREQ],pkwidth,dz);
					}
					break;
				case(3):	//	As case 2 : frq of harmonics also random spread across peak-width
					orig_frequency  = dz->bigfbuf[FREQ];
					read_nearest_value_from_brktable(dz->bigfbuf[FREQ],SPEKWIDTH,dz);
					pkaspect = dz->param[SPEKWIDTH];
					pkwidth = (dz->parray[0][vc+1]/pkaspect) * dz->nyquist;
					pkwidth = min(pkwidth,2 * dz->bigfbuf[FREQ]);	// Avoid -ve frqs
					pkoffset = drand48() * pkwidth;
					pkfrq  = dz->bigfbuf[FREQ] - (pkwidth/2.0);
					pkfrq += pkoffset;
					dz->bigfbuf[FREQ] = (float)pkfrq;
					if(dz->iparam[SPEKHARMS] > 1 && dz->param[SPEKBRITE] > 0.0) {
						harmamp = dz->bigfbuf[AMPP] * dz->param[SPEKBRITE];
						insert_harmonics(&peaked,&peakcnt,harmamp,orig_frequency,pkwidth,dz);
					}
					break;
				case(4):	//	case 1 + case 2
					orig_brightness = dz->param[SPEKBRITE];
					read_nearest_value_from_brktable(dz->bigfbuf[FREQ],SPEKWIDTH,dz);
					pkaspect = dz->param[SPEKWIDTH];
					dz->param[SPEKBRITE] = (pkaspect/dz->param[SPEKMXASP]) * orig_brightness;
					pkwidth = (dz->parray[0][vc+1]/pkaspect) * dz->nyquist;
					pkwidth = min(pkwidth,2 * dz->bigfbuf[FREQ]);	// Avoid -ve frqs
					pkoffset = drand48() * pkwidth;
					pkfrq  = dz->bigfbuf[FREQ] - (pkwidth/2.0);
					pkfrq += pkoffset;
					dz->bigfbuf[FREQ] = (float)pkfrq;
					if(dz->iparam[SPEKHARMS] > 1 && dz->param[SPEKBRITE] > 0.0) {
						harmamp = dz->bigfbuf[AMPP] * dz->param[SPEKBRITE];
						insert_harmonics(&peaked,&peakcnt,harmamp,dz->bigfbuf[FREQ],pkwidth,dz);
					}
					dz->param[SPEKBRITE] = orig_brightness;
					break;
				case(5):	//	case 1 + case 3
					orig_brightness = dz->param[SPEKBRITE];
					orig_frequency  = dz->bigfbuf[FREQ];
					read_nearest_value_from_brktable(dz->bigfbuf[FREQ],SPEKWIDTH,dz);
					pkaspect = dz->param[SPEKWIDTH];
					dz->param[SPEKBRITE] = (pkaspect/dz->param[SPEKMXASP]) * orig_brightness;
					pkwidth = (dz->parray[0][vc+1]/pkaspect) * dz->nyquist;
					pkwidth = min(pkwidth,2 * dz->bigfbuf[FREQ]);	// Avoid -ve frqs
					pkoffset = drand48() * pkwidth;
					pkfrq  = dz->bigfbuf[FREQ] - (pkwidth/2.0);
					pkfrq += pkoffset;
					dz->bigfbuf[FREQ] = (float)pkfrq;
					if(dz->iparam[SPEKHARMS] > 1 && dz->param[SPEKBRITE] > 0.0) {
						harmamp = dz->bigfbuf[AMPP] * dz->param[SPEKBRITE];
						insert_harmonics(&peaked,&peakcnt,harmamp,orig_frequency,pkwidth,dz);
					}
					dz->param[SPEKBRITE] = orig_brightness;
					break;
				}
			}
			chbot += dz->chwidth;
		}

		if(dz->param[SPEKRANDF] > 0.0)
			randomiseamps(perm,peaked,dz);

		if(dz->param[SPEKSPRED] > 0)
			spread_peaks(peaked,spreaddnratio,spreadupratio,dz);

		if((exit_status = write_samps(dz->bigfbuf,dz->wanted,dz))<0) {
			return(exit_status);
		}
		thistime += dz->frametime;
	}
	dfade = 1.1;

	if(dz->is_rectified) {
		dfade = 1.0;
		fader = 4.0;
	} else { 
	/* 87 WINDOW FADE-OUT */
		dfade = 1.1;
		fader = 1.1;
	}
	while(dfade <= 4096) {
		chbot = -dz->chwidth;
		for(cc = 0; cc < dz->clength; cc++)
			peaked[cc] = 0;
		peakcnt = 0;
		orig_peakcnt = 0;
		for(cc = 0, vc = 0; cc < dz->clength; cc++, vc +=2) {
			if(peaked[cc]) {
				if(cc > 0) {											//  IF already a marked peak (i.e. a harmonic of a previous peak)
					if(dz->parray[0][vc] > 0.0) {						//	BUT	this channel is a peak in orig data ... overwrite harmonic with new peak info
						dz->bigfbuf[AMPP] = (float)(dz->param[SPEKGAIN] * dz->parray[0][vc+1]/dfade);
						dz->bigfbuf[FREQ] = (float)dz->parray[0][vc];
						pkwidth = 0.0;
						aspindx = (orig_peakcnt * 2) + 1;
						switch(dz->iparam[SPEKTYPE]) {
						case(0):	//	Brightness defined by brightness-rolloff param
							if(dz->iparam[SPEKHARMS] > 1 && dz->param[SPEKBRITE] > 0.0) {
								harmamp = dz->bigfbuf[AMPP] * dz->param[SPEKBRITE];
								insert_harmonics(&peaked,&peakcnt,harmamp,dz->bigfbuf[FREQ],pkwidth,dz);
							}
							break;
						case(1):	//	Brightness (also) determined by peak-width-aspect
							orig_brightness = dz->param[SPEKBRITE];
							read_nearest_value_from_brktable(dz->bigfbuf[FREQ],SPEKWIDTH,dz);
							pkaspect = dz->param[SPEKWIDTH];
							dz->param[SPEKBRITE] = (pkaspect/dz->param[SPEKMXASP]) * orig_brightness;
							if(dz->iparam[SPEKHARMS] > 1 && dz->param[SPEKBRITE] > 0.0) {
								harmamp = dz->bigfbuf[AMPP] * dz->param[SPEKBRITE];
								insert_harmonics(&peaked,&peakcnt,harmamp,dz->bigfbuf[FREQ],pkwidth,dz);
							}
							dz->param[SPEKBRITE] = orig_brightness;
							break;
						case(2):	//	Brightness as case 0 : frq of peaks random spread across peak-width
							read_nearest_value_from_brktable(dz->bigfbuf[FREQ],SPEKWIDTH,dz);
							pkaspect = dz->param[SPEKWIDTH];
							pkwidth = (dz->parray[0][vc+1]/pkaspect) * dz->nyquist;
							pkwidth = min(pkwidth,2 * dz->bigfbuf[FREQ]);	// Avoid -ve frqs
							pkoffset = drand48() * pkwidth;
							pkfrq  = dz->bigfbuf[FREQ] - (pkwidth/2.0);
							pkfrq += pkoffset;
							dz->bigfbuf[FREQ] = (float)pkfrq;
							if(dz->iparam[SPEKHARMS] > 1 && dz->param[SPEKBRITE] > 0.0) {
								harmamp = dz->bigfbuf[AMPP] * dz->param[SPEKBRITE];
								insert_harmonics(&peaked,&peakcnt,harmamp,dz->bigfbuf[FREQ],pkwidth,dz);
							}
							break;
						case(3):	//	As case 2 : frq of harmonics also random spread across peak-width
							orig_frequency  = dz->bigfbuf[FREQ];
							read_nearest_value_from_brktable(dz->bigfbuf[FREQ],SPEKWIDTH,dz);
							pkaspect = dz->param[SPEKWIDTH];
							pkwidth = (dz->parray[0][vc+1]/pkaspect) * dz->nyquist;
							pkwidth = min(pkwidth,2 * dz->bigfbuf[FREQ]);	// Avoid -ve frqs
							pkoffset = drand48() * pkwidth;
							pkfrq  = dz->bigfbuf[FREQ] - (pkwidth/2.0);
							pkfrq += pkoffset;
							dz->bigfbuf[FREQ] = (float)pkfrq;
							if(dz->iparam[SPEKHARMS] > 1 && dz->param[SPEKBRITE] > 0.0) {
								harmamp = dz->bigfbuf[AMPP] * dz->param[SPEKBRITE];
								insert_harmonics(&peaked,&peakcnt,harmamp,orig_frequency,pkwidth,dz);
							}
							break;
						case(4):	//	case 1 + case 2
							orig_brightness = dz->param[SPEKBRITE];
							read_nearest_value_from_brktable(dz->bigfbuf[FREQ],SPEKWIDTH,dz);
							pkaspect = dz->param[SPEKWIDTH];
							dz->param[SPEKBRITE] = (pkaspect/dz->param[SPEKMXASP]) * orig_brightness;
							pkwidth = (dz->parray[0][vc+1]/pkaspect) * dz->nyquist;
							pkwidth = min(pkwidth,2 * dz->bigfbuf[FREQ]);	// Avoid -ve frqs
							pkoffset = drand48() * pkwidth;
							pkfrq  = dz->bigfbuf[FREQ] - (pkwidth/2.0);
							pkfrq += pkoffset;
							dz->bigfbuf[FREQ] = (float)pkfrq;
							if(dz->iparam[SPEKHARMS] > 1 && dz->param[SPEKBRITE] > 0.0) {
								harmamp = dz->bigfbuf[AMPP] * dz->param[SPEKBRITE];
								insert_harmonics(&peaked,&peakcnt,harmamp,dz->bigfbuf[FREQ],pkwidth,dz);
							}
							dz->param[SPEKBRITE] = orig_brightness;
							break;
						case(5):	//	case 1 + case 3
							orig_brightness = dz->param[SPEKBRITE];
							orig_frequency  = dz->bigfbuf[FREQ];
							read_nearest_value_from_brktable(dz->bigfbuf[FREQ],SPEKWIDTH,dz);
							pkaspect = dz->param[SPEKWIDTH];
							dz->param[SPEKBRITE] = (pkaspect/dz->param[SPEKMXASP]) * orig_brightness;
							pkwidth = (dz->parray[0][vc+1]/pkaspect) * dz->nyquist;
							pkwidth = min(pkwidth,2 * dz->bigfbuf[FREQ]);	// Avoid -ve frqs
							pkoffset = drand48() * pkwidth;
							pkfrq  = dz->bigfbuf[FREQ] - (pkwidth/2.0);
							pkfrq += pkoffset;
							dz->bigfbuf[FREQ] = (float)pkfrq;
							if(dz->iparam[SPEKHARMS] > 1 && dz->param[SPEKBRITE] > 0.0) {
								harmamp = dz->bigfbuf[AMPP] * dz->param[SPEKBRITE];
								insert_harmonics(&peaked,&peakcnt,harmamp,orig_frequency,pkwidth,dz);
							}
							dz->param[SPEKBRITE] = orig_brightness;
							break;
						}
					}
				}
			} else if(dz->parray[0][vc] <= 0.0) {
				frq = drand48() * dz->chwidth;						//	Frq unspecific, assign rand frq within chan
				frq += chbot;
				frq = max(frq,0.0);
				frq = min(frq,dz->nyquist);
				dz->bigfbuf[FREQ] = (float)frq;
				if((exit_status = read_value_from_brktable(frq,0,dz))<0)	//	Read amplitude from frq/amp brktable
					return(exit_status);
				dz->bigfbuf[AMPP] = (float)(dz->param[SPEKGAIN] * dz->param[0]/(double)dfade);
			} else {
				dz->bigfbuf[AMPP] = (float)(dz->param[SPEKGAIN] * dz->parray[0][vc+1]/(double)dfade);
				dz->bigfbuf[FREQ] = (float)dz->parray[0][vc];
				peaked[cc] = 1;
				pkwidth = 0.0;
				aspindx = (orig_peakcnt * 2) + 1;
				orig_peakcnt++;
				peakcnt++;
				switch(dz->iparam[SPEKTYPE]) {
				case(0):	//	Brightness defined by brightness-rolloff param
					if(dz->iparam[SPEKHARMS] > 1 && dz->param[SPEKBRITE] > 0.0) {
						harmamp = dz->bigfbuf[AMPP] * dz->param[SPEKBRITE];
						insert_harmonics(&peaked,&peakcnt,harmamp,dz->bigfbuf[FREQ],pkwidth,dz);
					}
					break;
				case(1):	//	Brightness (also) determined by peak-width-aspect
					orig_brightness = dz->param[SPEKBRITE];
					read_nearest_value_from_brktable(dz->bigfbuf[FREQ],SPEKWIDTH,dz);
					pkaspect = dz->param[SPEKWIDTH];
					dz->param[SPEKBRITE] = (pkaspect/dz->param[SPEKMXASP]) * orig_brightness;
					if(dz->iparam[SPEKHARMS] > 1 && dz->param[SPEKBRITE] > 0.0) {
						harmamp = dz->bigfbuf[AMPP] * dz->param[SPEKBRITE];
						insert_harmonics(&peaked,&peakcnt,harmamp,dz->bigfbuf[FREQ],pkwidth,dz);
					}
					dz->param[SPEKBRITE] = orig_brightness;
					break;
				case(2):	//	Brightness as case 0 : frq of peaks random spread across peak-width
					read_nearest_value_from_brktable(dz->bigfbuf[FREQ],SPEKWIDTH,dz);
					pkaspect = dz->param[SPEKWIDTH];
					pkwidth = (dz->parray[0][vc+1]/pkaspect) * dz->nyquist;
					pkwidth = min(pkwidth,2 * dz->bigfbuf[FREQ]);	// Avoid -ve frqs
					pkoffset = drand48() * pkwidth;
					pkfrq  = dz->bigfbuf[FREQ] - (pkwidth/2.0);
					pkfrq += pkoffset;
					dz->bigfbuf[FREQ] = (float)pkfrq;
					if(dz->iparam[SPEKHARMS] > 1 && dz->param[SPEKBRITE] > 0.0) {
						harmamp = dz->bigfbuf[AMPP] * dz->param[SPEKBRITE];
						insert_harmonics(&peaked,&peakcnt,harmamp,dz->bigfbuf[FREQ],pkwidth,dz);
					}
					break;
				case(3):	//	As case 2 : frq of harmonics also random spread across peak-width
					orig_frequency  = dz->bigfbuf[FREQ];
					read_nearest_value_from_brktable(dz->bigfbuf[FREQ],SPEKWIDTH,dz);
					pkaspect = dz->param[SPEKWIDTH];
					pkwidth = (dz->parray[0][vc+1]/pkaspect) * dz->nyquist;
					pkwidth = min(pkwidth,2 * dz->bigfbuf[FREQ]);	// Avoid -ve frqs
					pkoffset = drand48() * pkwidth;
					pkfrq  = dz->bigfbuf[FREQ] - (pkwidth/2.0);
					pkfrq += pkoffset;
					dz->bigfbuf[FREQ] = (float)pkfrq;
					if(dz->iparam[SPEKHARMS] > 1 && dz->param[SPEKBRITE] > 0.0) {
						harmamp = dz->bigfbuf[AMPP] * dz->param[SPEKBRITE];
						insert_harmonics(&peaked,&peakcnt,harmamp,orig_frequency,pkwidth,dz);
					}
					break;
				case(4):	//	case 1 + case 2
					orig_brightness = dz->param[SPEKBRITE];
					read_nearest_value_from_brktable(dz->bigfbuf[FREQ],SPEKWIDTH,dz);
					pkaspect = dz->param[SPEKWIDTH];
					dz->param[SPEKBRITE] = (pkaspect/dz->param[SPEKMXASP]) * orig_brightness;
					pkwidth = (dz->parray[0][vc+1]/pkaspect) * dz->nyquist;
					pkwidth = min(pkwidth,2 * dz->bigfbuf[FREQ]);	// Avoid -ve frqs
					pkoffset = drand48() * pkwidth;
					pkfrq  = dz->bigfbuf[FREQ] - (pkwidth/2.0);
					pkfrq += pkoffset;
					dz->bigfbuf[FREQ] = (float)pkfrq;
					if(dz->iparam[SPEKHARMS] > 1 && dz->param[SPEKBRITE] > 0.0) {
						harmamp = dz->bigfbuf[AMPP] * dz->param[SPEKBRITE];
						insert_harmonics(&peaked,&peakcnt,harmamp,dz->bigfbuf[FREQ],pkwidth,dz);
					}
					dz->param[SPEKBRITE] = orig_brightness;
					break;
				case(5):	//	case 1 + case 3
					orig_brightness = dz->param[SPEKBRITE];
					orig_frequency  = dz->bigfbuf[FREQ];
					read_nearest_value_from_brktable(dz->bigfbuf[FREQ],SPEKWIDTH,dz);
					pkaspect = dz->param[SPEKWIDTH];
					dz->param[SPEKBRITE] = (pkaspect/dz->param[SPEKMXASP]) * orig_brightness;
					pkwidth = (dz->parray[0][vc+1]/pkaspect) * dz->nyquist;
					pkwidth = min(pkwidth,2 * dz->bigfbuf[FREQ]);	// Avoid -ve frqs
					pkoffset = drand48() * pkwidth;
					pkfrq  = dz->bigfbuf[FREQ] - (pkwidth/2.0);
					pkfrq += pkoffset;
					dz->bigfbuf[FREQ] = (float)pkfrq;
					if(dz->iparam[SPEKHARMS] > 1 && dz->param[SPEKBRITE] > 0.0) {
						harmamp = dz->bigfbuf[AMPP] * dz->param[SPEKBRITE];
						insert_harmonics(&peaked,&peakcnt,harmamp,orig_frequency,pkwidth,dz);
					}
					dz->param[SPEKBRITE] = orig_brightness;
					break;
				}
			}
			chbot += dz->chwidth;
		}
		if(dz->param[SPEKRANDF] > 0.0)
			randomiseamps(perm,peaked,dz);

		if(dz->param[SPEKSPRED] > 0)
			spread_peaks(peaked,spreaddnratio,spreadupratio,dz);
		
		if((exit_status = write_samps(dz->bigfbuf,dz->wanted,dz))<0)
			return(exit_status);
		dfade *= fader;
	}
	thistime = 0.0;

	/* PAD END WITH ZEROS */
	frq = 0.0;
	while(thistime < 0.01) {
		for(cc = 0, vc = 0; cc < dz->clength; cc++, vc +=2) {
			dz->bigfbuf[AMPP] = 0.0f;
			dz->bigfbuf[FREQ] = (float)frq;
			frq += dz->chwidth;
		}
		if((exit_status = write_samps(dz->bigfbuf,dz->wanted,dz))<0)
			return(exit_status);
		thistime += dz->frametime;
	}
	return(FINISHED);
}

/************************************** INSERT_HARMONICS ****************************************/

void insert_harmonics(int **peaked,int *peakcnt,double harmamp,float fundamental,double pkwidth,dataptr dz)
{
	int n, harmchan;
	double harmfrq, pkoffset;
	for(n = 2;n <= dz->iparam[SPEKHARMS];n++) {
		harmfrq = fundamental * (double)n;
		if(dz->iparam[SPEKTYPE] == 3 || dz->iparam[SPEKTYPE] == 5) {
			pkoffset = drand48() * pkwidth;
			harmfrq -= (pkwidth/2.0);
			harmfrq += pkoffset;
		}
		if(harmfrq <= fundamental)
			 continue;
		harmchan = (int)round(harmfrq/dz->chwidth);
		if(harmchan >= dz->clength || harmfrq >= dz->nyquist)
			return;
		(*peaked)[harmchan] = 1;

		(*peakcnt)++;
		dz->bigfbuf[(harmchan * 2) + 1] = (float)harmfrq;
		dz->bigfbuf[harmchan * 2]       = (float)harmamp;
		harmamp *= dz->param[SPEKBRITE];		// Attenuate harmonics as we rise thro harmonics
	}
}

/************************************** SPREAD_PEAKS ****************************************
 *
 * Spread peaks to the adjacent channels.
 */

void spread_peaks(int *peaked,double spreaddnratio,double spreadupratio,dataptr dz)
{
	int bb, cc, dd, vc;
	int chanlimit;
	double frqlimit, spreadval, maxwander, wander;
	for(cc = 0, bb = -1,dd = 1, vc = 0; cc < dz->clength; bb++, cc++, dd++, vc +=2) {
		if(dd < dz->clength) {
			if(peaked[dd] && !peaked[cc]) {
				chanlimit = cc - 3;								//	chan overlap is 8, so frq OK if lies within chans cc-3 to cc+4
				if(chanlimit >= 0)
					frqlimit = chanlimit * dz->chwidth;			//	Set downward limit of new frq as centre of chan 3-chans down
				else											//	or at zero, if not enough channels downwards
					frqlimit = 0;									
				spreadval = dz->bigfbuf[FREQ] * spreaddnratio;	//	Calculate frq of spread peak
				if(spreadval <= frqlimit) {						//	If it exceeds limit
					maxwander = dz->bigfbuf[FREQ] - frqlimit;	//	Calcuate gap between peak frq and downwward limit
					wander = maxwander * spreaddnratio;			//	Calculate spread as a fraction of this gap
					spreadval = dz->bigfbuf[FREQ] - wander;		//	Subtract spread from peak frq
				}
				dz->bigfbuf[(dd*2)+1] = (float)spreadval;		//	Set new chan frq to spreadpeak val
			}
		}
		if(bb > 0) {											//	Simil, but for chan above	
			if(peaked[bb] && !peaked[cc]) {
				chanlimit = cc + 4;
				if(chanlimit < dz->clength)
					frqlimit = chanlimit * dz->chwidth;
				else
					frqlimit = dz->nyquist;
				spreadval = dz->bigfbuf[FREQ] * spreadupratio;
				if(spreadval >= frqlimit) {
					maxwander = frqlimit - dz->bigfbuf[FREQ];
					wander = maxwander * spreaddnratio;			//	must use spreadDNratio to ensure new frq is below upper limit
					spreadval = dz->bigfbuf[FREQ] + wander;
				}
				dz->bigfbuf[(bb*2)+1] = (float)spreadval;
			}
		}
	}
}

/************************** SPECVARY **********************************/

int specvary(dataptr dz)
{
	return(FINISHED);
}

/************************* SPECFORMAT *******************/

int specformat(dataptr dz)
{
	int spekchans, clength, exit_status;
	int n, inbrksize;
	double *inbrk, frq, amp;
	double chwidth, chantopfrq, chanmidfrq, chanbotfrq, chanbotamp, chanmidamp, chantopamp;
	double chanmax_amp, chanmin_amp, chanmax_frq = 0.0, chanmin_frq = 0.0, chanejmin, chanejmax;
	char valstr[400];

	dz->brk[0] = dz->parray[0];		//	Use address and pointers assigned to brktable 0
	dz->brksize[0] = dz->itemcnt/2;	//	A param 0 is not a brktable, so not using it.
	inbrk = dz->brk[0];				//	This allows input data to be read using brktablread function
	inbrksize = dz->itemcnt;		//	and param[0] to be reused for the output
	spekchans = dz->iparam[SPEKPOINTS] + 2;
	clength = spekchans/2;
	chwidth = dz->nyquist/(double)(clength);
	chantopfrq = chwidth/2.0;
	chanmidfrq = 0.0;
	chanbotfrq = -chantopfrq;
	chanbotamp = 0.0;
	if((exit_status = read_value_from_brktable(chantopfrq,0,dz))<0)		//	Read data from input brktable
		return(exit_status);
	chantopamp = dz->param[0];
		
	chanmax_amp = 0.0;			//	init vals to search for min/max amps within chan
	chanmin_amp  = 100.0;
	chanejmin = chanbotamp;		//	init max and min of vals at edges of channel : first channel, min at foot
	chanejmax = chantopamp;
	n = 0;
	while(n < inbrksize) {
		frq = inbrk[n++];
		amp = inbrk[n++];

		if(frq >= chantopfrq) {											//	Once frq of input data strays into next channel
			if(chanmax_amp > chanejmax || chanmax_amp == 1.0) {			//	If peak amp in channel is greater than both edges, this is a true peak	
				sprintf(valstr,"%lf %lf\n",chanmax_frq,chanmax_amp);	//	Peak points get amp and frq of peak frq
				fputs(valstr,dz->fp);
			} else if(chanmin_amp < chanejmin) {						//	If trough amp in channel is less than both edges, this is a true trough
				sprintf(valstr,"%lf %lf\n",-chanmin_frq,chanmin_amp);	//	Trough points get -ve of min frq in chan, and its amp
				fputs(valstr,dz->fp);
			} else {													//	Otherwise, frq to assign to channel is unknown
				if((exit_status = read_value_from_brktable(chanmidfrq,0,dz))<0)
					return(exit_status);								//	Find the amplitude at midpoint of channel
				chanmidamp = dz->param[0];									
				sprintf(valstr,"%lf %lf\n",0.0,chanmidamp);				//	Store frq = 0.0 (unknown) and midchannel amp
				fputs(valstr,dz->fp);
			}
			chanbotfrq = chantopfrq;									//	Move to parameters for next channel
			chanmidfrq = chanmidfrq + chwidth;
			chantopfrq = chanbotfrq + chwidth;
			chanbotamp = chantopamp;									//	Finding amplitude at top edge of new channel
			if((exit_status = read_value_from_brktable(chantopfrq,0,dz))<0)
				return(exit_status);
			chantopamp = dz->param[0];
			chanejmax = max(chanbotamp,chantopamp);						//	find max and min of vals at edges of channel
			chanejmin = min(chanbotamp,chantopamp);
			chanmax_amp = 0.0;											//	reinit vals to search for min/max amps within chan
			chanmin_amp = 100.0;
		}
		if(amp > chanmax_amp) {			//	Search for minimum and maximum amps in channel
			chanmax_amp = amp;			//	Note amp value and frq at which it occurs
			chanmax_frq = frq;
		} 
		if(amp < chanmin_amp) {
			chanmin_amp = amp;
			chanmin_frq = frq;
		}
	}
	return FINISHED;
}

/*********************** RANDOMISEAMPS ************************/

void randomiseamps(int *perm,int *peaked,dataptr dz)
{
	int randvarchans, modifiedchans, chancnt, cc, vc;
	double amp, atten, gain;											//	Get a random number of chans to attenuate
	randvarchans = (int)round(drand48() * dz->param[SPEKRANDF] * (double)dz->clength);
	if(randvarchans > 0) {
		rndintperm(perm,dz->clength);									//	Generate a new random-perm of the channels
		for(chancnt=0,modifiedchans=0;chancnt<dz->clength;chancnt++) {	
			cc = perm[chancnt];											//	Proceeding through the chans in this random order,
			if(!peaked[cc]) {											//	so long as the channel is not a peak,
				vc = cc * 2;
				amp = dz->bigfbuf[AMPP];
				atten = dz->param[SPEKRANDA] * drand48();				//	randomly attenuate its amplitude
				gain = 1.0 - atten;
				amp *= gain;											
				dz->bigfbuf[AMPP] = (float)amp;
				if(++modifiedchans >= randvarchans)						//	quitting when enough channels have been attenuated.
					break;
			}
		}
	}
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

/************************** GET_FLOAT_WITH_E_FROM_WITHIN_STRING **************************
 * takes a pointer TO A POINTER to a string. If it succeeds in finding 
 * a float it returns the float value (*val), and it's new position in the
 * string (*str).
 */

int  get_float_with_e_from_within_string(char **str,double *val)
{
	char   *p, *valstart;
	int    decimal_point_cnt = 0, has_digits = 0, has_e = 0, lastchar = 0;
	p = *str;
	while(isspace(*p))
		p++;
	valstart = p;	
	switch(*p) {
	case('-'):						break;
	case('.'): decimal_point_cnt=1;	break;
	default:
		if(!isdigit(*p))
			return(FALSE);
		has_digits = TRUE;
		break;
	}
	p++;
	while(!isspace(*p) && *p!=NEWLINE && *p!=ENDOFSTR) {
		if(isdigit(*p))
			has_digits = TRUE;
		else if(*p == 'e') {
			if(has_e || !has_digits)
				return(FALSE);
			has_e = 1;
		} else if(*p == '-') {
			if(!has_e || (lastchar != 'e'))
				return(FALSE);
		} else if(*p == '.') {
			if(has_e || (++decimal_point_cnt>1))
				return(FALSE);
		} else
			return(FALSE);
		lastchar = *p;
		p++;
	}
	if(!has_digits || sscanf(valstart,"%lf",val)!=1)
		return(FALSE);
	*str = p;
	return(TRUE);
}

/**************************** READ_VALUE_FROM_BRKTABLE *****************************/

int read_nearest_value_from_brktable(double thistime,int paramno,dataptr dz)
{
    double *endpair, *p;
    double hival, loval, hiind, loind;
    if(!dz->brkinit[paramno]) {
		dz->brkptr[paramno]   = dz->brk[paramno];
		dz->firstval[paramno] = *(dz->brk[paramno]+1);
		endpair               = dz->brk[paramno] + ((dz->brksize[paramno]-1)*2);
		dz->lastind[paramno]  = *endpair;
		dz->lastval[paramno]  = *(endpair+1);
 		dz->brkinit[paramno] = 1;
    }
	p = dz->brkptr[paramno];
	if(thistime <= *(dz->brk[paramno])) {
		dz->param[paramno] = dz->firstval[paramno];
		if(dz->is_int[paramno])							
			dz->iparam[paramno] = round(dz->param[paramno]);
		return(FINISHED);
	} else if(thistime >= dz->lastind[paramno]) {
		dz->param[paramno] = dz->lastval[paramno];
		if(dz->is_int[paramno])							
			dz->iparam[paramno] = round(dz->param[paramno]);
		return(FINISHED);
	} 
	if(thistime > *(p)) {
		while(*(p)<thistime)
			p += 2;
	} else {
		while(*(p)>=thistime)
			p -= 2;
		p += 2;
	}
	hival  = *(p+1);
	hiind  = *p;
	loval  = *(p-1);
	loind  = *(p-2);
	if(thistime - loind < hiind - thistime)
		dz->param[paramno] = loval;
	else
		dz->param[paramno] = hival;
 	dz->brkptr[paramno] = p;
    return(FINISHED);
}

/************************************* CHECK_SPEKLINE_PARAM_VALIDITY_AND_CONSISTENCY **************************/

int check_spekline_param_validity_and_consistency(dataptr dz)
{
	int n;
	double *data = dz->parray[0], minlevel, datrange, spekrange, frac, diff;
	double minfrq = data[0], maxamp = 0.0;
	double maxfrq = data[dz->itemcnt - 2];
	if(minfrq < dz->param[SPEKDATLO]) {
		sprintf(errstr,"Bottom-of-data frq (%lf) must be <= minimum frq in input data (%lf)\n",dz->param[SPEKDATLO],minfrq);
		return DATA_ERROR;
	}	
	if(maxfrq > dz->param[SPEKDATHI]) {
		sprintf(errstr,"Top-of-data frq (%lf) must be >= maximum frq in input data (%lf)\n",dz->param[SPEKDATHI],maxfrq);
		return DATA_ERROR;
	}
	dz->nyquist = dz->param[SPEKSRATE]/2.0;
	if(dz->param[SPEKSPKHI] > dz->nyquist) {
		sprintf(errstr,"Top-of-spectrum frq (%lf) cannot be above the nyquist (%lf)\n",dz->param[SPEKSPKHI],dz->nyquist);
		return DATA_ERROR;
	}
	if(dz->param[SPEKSPKHI] <= dz->param[SPEKSPKLO]) {
		sprintf(errstr,"Top-of-spectrum frq (%lf) cannot be <= bottom-of-spectrum (%lf)\n",dz->param[SPEKSPKHI],dz->param[SPEKSPKLO]);
		return DATA_ERROR;
	}
	datrange  = dz->param[SPEKDATHI] - dz->param[SPEKDATLO];
	spekrange = dz->param[SPEKSPKHI] - dz->param[SPEKSPKLO];
	for(n=0;n<dz->itemcnt;n+=2) {			//	Scale in range to outrange
		frac = (data[n] - dz->param[SPEKDATLO])/datrange;
		if(dz->param[SPEKWARP] != 1.0)		//	Warp range if requested
			frac = pow(frac,dz->param[SPEKWARP]);
		data[n] = (frac * spekrange) + dz->param[SPEKSPKLO];
	}
	if(dz->param[SPEKAWARP] != 1.0) {
		for(n=1;n<dz->itemcnt;n+=2) {	//	Find maxamp
			if(data[n] > maxamp)
				maxamp = data[n];
		}
		for(n=1;n<dz->itemcnt;n+=2) {	//	Contract diff between level and max
			diff = maxamp - data[n];
			diff /= dz->param[SPEKAWARP];
			data[n] = maxamp - diff;
		}
	}
	if(dz->mode == 0) {
		for(n=1;n<dz->itemcnt;n+=2)			//	Scale amps to maxamp parameter
			data[n] *= dz->param[SPEKMAX];

		fprintf(stdout,"INFO: Output Spectrum\n");
		for(n=0;n<dz->itemcnt;n+=2)
			fprintf(stdout,"INFO: %lf\t%lf\n",data[n],data[n+1]);
		fflush(stdout);
		minlevel = 1.0/(double)dz->infile->srate;
		dz->param[SPEKBRITE] = dbtolevel(dz->param[SPEKBRITE]);
		if(dz->param[SPEKBRITE] <= minlevel) {
			sprintf(errstr,"Rolloff forces harmonics to effectively zero level: no effect on input sound.\n");
			return(DATA_ERROR);
		}
	}
	return FINISHED;
}

/************************************* SPECLINES **************************/

int speclines(dataptr dz) {
	int exit_status, cc, vc, thisfrq, thisamp, n, harmchan;
	double *data = dz->parray[0];
	double thistime = 0.0, val, harmamp, harmfrq;
	double frq = 0, chbot, chtop;
	for(cc = 0, vc = 0; cc < dz->clength; cc++, vc +=2) {	//	Write first zero level window
		dz->bigfbuf[AMPP] = 0.0f;
		dz->bigfbuf[FREQ] = (float)frq;
		frq += dz->chwidth;
	}
	if((exit_status = write_samps(dz->bigfbuf,dz->wanted,dz))<0)
		return(exit_status);

	thistime += dz->frametime;
	
	while(thistime < dz->param[SPEKDUR]) {
		thisfrq = 0;
		thisamp = 1;
		chbot = -dz->chwidth;
		chtop = dz->chwidth;
		memset((char *)dz->bigfbuf,0,dz->wanted * sizeof(float));
		for(cc = 0, vc = 0; cc < dz->clength; cc++, vc +=2) {		//	Insert line spectra
			if(chbot <= data[thisfrq] && chtop > data[thisfrq]) {
				dz->bigfbuf[AMPP] = (float)data[thisamp];
				dz->bigfbuf[FREQ] = (float)data[thisfrq];
				thisfrq += 2;
				thisamp += 2;
				if(thisfrq >= dz->itemcnt)
					break;
			}
			chbot += dz->chwidth;			
			chtop += dz->chwidth;
		}
		thisfrq = 0;
		thisamp = 1;
		chbot = -dz->chwidth;
		chtop = dz->chwidth;
		for(cc = 0, vc = 0; cc < dz->clength; cc++, vc +=2) {		//	Broaden line spectra
			if(chbot <= data[thisfrq] && chtop > data[thisfrq]) {
				if(vc - 2 >= 0) {
					val = (float)(data[thisamp]/3.0);
					if(val > dz->bigfbuf[vc-2]) {
						dz->bigfbuf[vc-2] = (float)val;
						dz->bigfbuf[vc-1] = (float)(data[thisfrq]);
					}
				}
				if(vc - 4 >= 0) {
					val = (float)(data[thisamp]/10.0);
					if(val > dz->bigfbuf[vc-4]) {
						dz->bigfbuf[vc-4] = (float)val;
						dz->bigfbuf[vc-3] = (float)(data[thisfrq]);
					}
				}
				if(vc + 2 < dz->wanted) {
					val = (float)(data[thisamp]/3.0);
					if(val > dz->bigfbuf[vc+2]) {
						dz->bigfbuf[vc+2] = (float)val;
						dz->bigfbuf[vc+3] = (float)(data[thisfrq]);
					}
				}
				if(vc + 4 < dz->wanted) {
					val = (float)(data[thisamp]/10.0);
					if(val > dz->bigfbuf[vc+4]) {
						dz->bigfbuf[vc+4] = (float)val;
						dz->bigfbuf[vc+5] = (float)(data[thisfrq]);
					}
				}
				thisfrq += 2;
				thisamp += 2;
				if(thisfrq >= dz->itemcnt)
					break;
			}
			chbot += dz->chwidth;
			chtop += dz->chwidth;
		}
		if(dz->iparam[SPEKHARMS] > 1) {
			harmamp = dz->param[SPEKBRITE];
			for(n = 2;n <= dz->iparam[SPEKHARMS];n++) {					//	Add harmonics
				for(cc = 0, vc = 0; cc < dz->clength; cc++, vc +=2) {
					if(dz->bigfbuf[AMPP] > 0.0) {
						harmfrq = dz->bigfbuf[FREQ] * n;
						harmchan = (int)round(harmfrq/dz->chwidth);
						if(harmchan >= dz->clength || harmfrq >= dz->nyquist)
							break;
						if(dz->bigfbuf[AMPP] * harmamp > dz->bigfbuf[harmchan * 2]) {
							dz->bigfbuf[(harmchan * 2) + 1] = (float)harmfrq;
							dz->bigfbuf[harmchan * 2]       = (float)(dz->bigfbuf[AMPP] * harmamp);
						}
					}
					harmamp *= dz->param[SPEKBRITE];		// Attenuate harmonics as we rise thro harmonics
				}
			}
		}
		if((exit_status = write_samps(dz->bigfbuf,dz->wanted,dz))<0)
			return(exit_status);
		thistime += dz->frametime;
	}
	dz->infile->origstype = 0;
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

/************************************* SPECLINESFILT **************************/

int speclinesfilt(dataptr dz)
{
	double thismidi, thisamp;
	int  n;
	double *data = dz->parray[0];
	char temp[20000], temp2[128];

	strcpy(temp,"0");
	for(n=0;n<dz->itemcnt;n+=2) {	
		thismidi = unchecked_hztomidi(data[n]);
		thisamp = data[n+1];
		sprintf(temp2,"  %lf  %lf",thismidi,thisamp);
		strcat(temp,temp2);
	}
	strcat(temp,"\n");
	if(fputs(temp,dz->fp)<0) {
		sprintf(errstr,"FAILED TO WRITE FILTER DATA TO FILE\n");
		return SYSTEM_ERROR;
	}
	sprintf(temp,"%lf",dz->param[SPEKDUR]);
	for(n=0;n<dz->itemcnt;n+=2) {	
		thismidi = unchecked_hztomidi(data[n]);
		thisamp = data[n+1];
		sprintf(temp2,"  %lf  %lf",thismidi,thisamp);
		strcat(temp,temp2);
	}
	strcat(temp,"\n");
	if(fputs(temp,dz->fp)<0) {
		sprintf(errstr,"FAILED TO WRITE FILTER DATA TO FILE\n");
		return SYSTEM_ERROR;
	}
	return FINISHED;
}
