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
#include <float.h>
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

#if defined unix || defined __GNUC__
#define round(x) lround((x))
#endif
#ifndef HUGE
#define HUGE 3.40282347e+38F
#endif

char errstr[2400];

int anal_infiles = 1;
int	sloom = 0;
int sloombatch = 0;
/* TW update 2012 */
const char* cdp_version = "7.1.0";

//CDP LIB REPLACEMENTS
static int setup_mchanpan_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int check_the_param_validity_and_consistency(dataptr dz);
static int setup_mchanpan_param_ranges_and_defaults(dataptr dz);
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
//static double dbtolevel(double val);
static int mchanpan(dataptr dz);
static int allocate_mchanpan_buffer(dataptr dz);
static void mchan_pancalc(double position,double *leftgain,double *rightgain,double *farleftgain,double *farrightgain,int *leftchan,int pantype,int focusparam,dataptr dz);
static int newposition(int *brkindex,double *position,double *posincr,int *total_sams,int *true_goalchan,int *pantype,dataptr dz);
static int read_mchanpan_data(char *filename,dataptr dz);
static int read_mchancross_data(char *filename,dataptr dz);
static int count_events(int silpar,int arrayno,int *arraycnt,dataptr dz);
static void spread_set(dataptr dz);
static void spread_pan(double *centre,dataptr dz);
static int read_antiphon_data(char *filename,dataptr dz);
static int setup_antiphon_arrays(char *str,dataptr dz);
static int generate_antiphonal_events_array(dataptr dz);
static void do_span(int *map,double span,float *ibuf,float *obuf,int bufpos,double rolloff,int chans);
static int establish_arrays(dataptr dz);
static void spread_pan_array_cnt(double time,int last,dataptr dz);
static int spread_pan_process(double time,int last,dataptr dz);
static int write_break_data(dataptr dz);

static int handle_the_special_data(int *cmdlinecnt,char ***cmdline,dataptr dz);
static int zing(dataptr dz);
static int stepspread(dataptr dz);
static int panspread(dataptr dz);
static int antiphonal_events(dataptr dz);
static int antiphony(dataptr dz);
static int crosspan(dataptr dz);
static int panprocess(dataptr dz);
static int mchanpan2(dataptr dz);

static void permute_chans(int outchans,dataptr dz);
static void insertch(int n,int t,int outchans,dataptr dz);
static void prefixch(int n,int outchans,dataptr dz);
static void shuflupch(int k,int outchans,dataptr dz);
static int adjacence(int endchan,dataptr dz);

#define BSIZE	(128)
#define ROOT2	(1.4142136)
#define ASCTOINT (96)

#define	ATCENTRE	(0)
#define	TOCENTRE	(1)
#define	FROMCENTRE	(2)
#define	CROSSCENTRE	(3)
#define	ATEDGE		(4)


#define antiphlen0 itemcnt
#define antiphlen1 ringsize
#define	antievents rampbrksize

/*	GENERAL SCHEMA FOR ROTATION PANNING ON > 2 LOUDSPEAKERS
 * 
 *  (homc = hole in middle compensation)
 * 
 *		0.05		0.9			0.05		0
 *		x			x	cross	x			x
 *			~		\	  +		/       ~
 *			    ~	  \	 homc /     ~
 *					~    \   /   ~   cross
 *					    ~  X ~       linear
 *				    ~    /   \   ~
 *			    ~	  /       \		~
 *			~		/           \		~
 *		0			0.05		0.9			0.05
 *		x			x			x			x
 *
 *
 * If there are only 3 channels
 *     0.05			0.9			0.05
 *		x			x	cross	x
 *						homc
 *     0.05			0.05		0.9
 *		x			x			x	cross
 *									homc
 *	   0.9			0.05		0.05	
 *		x			x			x
 *
 * So, effectively, the 3rd channel does not change in level
 * But we get the same effect by crossfading between
 * Chan N (0.05) and chan (N+3)%3 (==N in this case) (0.05)
 * So same algo applies to 3-channel case.
 *
 * HOMC: this may vary, depending on which lspkrs one's panning between ???
 *
 * For 4 lpskrs, pan position can be at lspkr 1 2 3 4
 * or in between (1.5, 2.7, 4.9)
 *
 * Best way to think of this is as (lspkr - 1) % chans
 * i.e. 0 1 2 3  or in between (.5 1.7 3.9)
 * USing "mod" allows us to have an increasing lspkr position (on e.g. 4 lpskrs)
 * which circles the space, so (1->4->5), so the parameter is 0->3->4 % 4 = 0->3->0
 *
 * So fileformat can be
 * Time		lspkrposition
 *	0.0		1
 *	2.2		3
 *	2.6		4.5
 *	10		1.3
 *
 * Rotation can be generated using a time-increasing parameter, to generate the file
 * such that eveytime position == whole number, create a time-position pair, and using 
 * ((lspkr - 1)%chancnt + 1) to give correct lspkr.
 *
 *	For any position, e.g. 3.5 the output is
 *	LEFTCHAN = floor(3.5)  = chan 3
 *	RIGHTCHAN = LEFTCHAN + 1;
 *  HOMC param controlled by 3.5 - floor(3.5) = stereoposition
 *  level on L-1  = 0.05 * (1.0 - K)
 *  level on R+1  = 0.05 * K
 *	level on any remaining loudspeakers = 0
 *
 */

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])		// THIS ALL NEEDS TO BE FIXED !!! */
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
	dz->maxmode = 10;
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
		if((exit_status = get_the_mode_from_cmdline(cmdline[0],dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		cmdline++;
		cmdlinecnt--;
		// setup_particular_application =
		if((exit_status = setup_mchanpan_application(dz))<0) {
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
	if(dz->mode == 7) {
		if((exit_status = establish_arrays(dz))<0) {
			exit_status = print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
	}	
	// setup_param_ranges_and_defaults() =
	if((exit_status = setup_mchanpan_param_ranges_and_defaults(dz))<0) {
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
		if(dz->mode == 5) {
			if((exit_status = handle_extra_infiles(&cmdline,&cmdlinecnt,dz))<0) {
				print_messages_and_close_sndfiles(exit_status,is_launched,dz);
				return(FAILED);
			}
		} else {
			sprintf(errstr,"This process uses only one input file\n");
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);	
			return(FAILED);
		}

	}
//	handle_extra_infiles() : redundant
	// handle_outfile() = 
	if(dz->mode != 7) {
		if((exit_status = handle_the_outfile(&cmdlinecnt,&cmdline,dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
	}

//	handle_formants()			redundant
//	handle_formant_quiksearch()	redundant
//	handle_special_data()		redundant
 
	if(dz->mode == 0 || dz->mode == 1 || dz->mode == 4 || dz->mode == 5 || dz->mode == 6) {
		if((exit_status = handle_the_special_data(&cmdlinecnt,&cmdline,dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
	}
	if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {		// CDP LIB
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
//	check_param_validity_and_consistency 
	if((exit_status = check_the_param_validity_and_consistency(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if(dz->mode != 7) {
		if((exit_status = open_the_outfile(dz))<0) {		//	outfile opened here as output chancnt is a parameter
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
	}
	is_launched = TRUE;
	if(dz->mode != 7) {
		if(dz->mode == 6)
			dz->bufcnt = dz->infile->channels * 2;
		else
			dz->bufcnt = (dz->infile->channels * dz->infilecnt) + dz->iparam[0];

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

		if((exit_status = allocate_mchanpan_buffer(dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
	}
	//param_preprocess()						redundant
	//spec_process_file =
	switch(dz->mode) {
	case(0):
		if((exit_status = mchanpan(dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		break;
	case(1):
	case(9):
		if((exit_status = zing(dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		break;
	case(2):
		if((exit_status = stepspread(dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		break;
	case(3):
		if((exit_status = panspread(dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		break;
	case(4):
		if((exit_status = antiphonal_events(dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		break;
	case(5):
		if((exit_status = antiphony(dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		break;
	case(6):
		if((exit_status = crosspan(dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		break;
	case(7):
		if((exit_status = panprocess(dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		exit_status = print_messages_and_close_sndfiles(FINISHED,is_launched,dz);	// CDP LIB
		free(dz);
		return(SUCCEEDED);
	case(8):
		if((exit_status = mchanpan2(dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		break;
	}
	if(dz->mode != 6)
		dz->infile->channels = dz->iparam[0];	// ready for output header
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
//	dz->infilecnt = 1;
	if(dz->mode != 7) {
		if((exit_status = setup_internal_arrays_and_array_pointers(dz))<0)	 
			return(exit_status);
	}
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
	char *filename = (*cmdline)[0];
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
	if((dz->outfilename = (char *)malloc(strlen(filename) + 1))==NULL) {
		sprintf(errstr,"No memory to store output filename.\n");
		return(DATA_ERROR);
	}
	strcpy(dz->outfilename,filename);
	(*cmdline)++;
	(*cmdlinecnt)--;
	return(FINISHED);
}

/************************ OPEN_THE_OUTFILE *********************/

int open_the_outfile(dataptr dz)
{
	int exit_status, k;
	k = dz->infile->channels;
	if(dz->mode != 6)
		dz->infile->channels = dz->iparam[0];
	if((exit_status = create_sized_outfile(dz->outfilename,dz))<0)
		return(exit_status);
	dz->infile->channels = k;
	return(FINISHED);
}

/************************ HANDLE_THE_SPECIAL_DATA *********************/

int handle_the_special_data(int *cmdlinecnt,char ***cmdline,dataptr dz)
{
	int exit_status;
	aplptr ap = dz->application;

//	if(ap->special_data) {
		if(!sloom) {
			if(*cmdlinecnt <= 0) {
				sprintf(errstr,"Insufficient parameters on command line.\n");
				return(USAGE_ONLY);
			}
		}
		switch(dz->mode) {
		case(0):
		case(1):
			ap->min_special 	= 0;
			ap->max_special 	= 16;
			ap->min_special2  	= -1;
			ap->max_special2  	= 1;
			if((exit_status = read_mchanpan_data((*cmdline)[0],dz))<0)
				return(exit_status);
			break;
		case(4):
		case(5):
			if((exit_status = read_antiphon_data((*cmdline)[0],dz))<0)
				return(exit_status);
			break;
		case(6):
			if((exit_status = read_mchancross_data((*cmdline)[0],dz))<0)
				return(exit_status);
			break;
		}
		(*cmdline)++;		
		(*cmdlinecnt)--;
//	}
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
		}
		switch(dz->mode) {
		case(0):
		case(1):
		case(2):
		case(9):
			if(infile_info->channels != 1) {
				sprintf(errstr,"File %s is not of correct type (must be mono)\n",cmdline[0]);
				return(DATA_ERROR);
			}
			break;
		case(3):
			if(infile_info->channels > 2) {
				sprintf(errstr,"File %s is not of correct type (must be mono or stereo)\n",cmdline[0]);
				return(DATA_ERROR);
			}
			break;
		case(6):
		case(7):
			if(infile_info->channels < 2) {
				sprintf(errstr,"File %s is not of correct type (cannot be mono)\n",cmdline[0]);
				return(DATA_ERROR);
			}
			break;
		}
		if((exit_status = copy_parse_info_to_main_structure(infile_info,dz))<0) {
			sprintf(errstr,"Failed to copy file parsing information\n");
			return(PROGRAM_ERROR);
		}
		free(infile_info);
	}
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
			if((exit_status = setup_mchanpan_application(dz))<0)
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

/************************* SETUP_INTERNAL_ARRAYS_AND_ARRAY_POINTERS *******************/

int setup_internal_arrays_and_array_pointers(dataptr dz)
{
	switch(dz->mode) {
	case(0):
		dz->array_cnt  = 1; 
		if((dz->parray  = (double **)malloc(dz->array_cnt * sizeof(double *)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for internal long arrays.\n");
			return(MEMORY_ERROR);
		}
		dz->parray[0]  = NULL;
		break;
	case(1):
		dz->larray_cnt = 2; 
		if((dz->lparray  = (int **)malloc(dz->larray_cnt * sizeof(int *)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for internal long arrays.\n");
			return(MEMORY_ERROR);
		}
		dz->lparray[0]  = NULL;
		dz->lparray[1]  = NULL;
		break;
	case(2):
		dz->larray_cnt = 1; 
		dz->array_cnt = 2; 
		if((dz->lparray  = (int **)malloc(dz->larray_cnt * sizeof(int *)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for internal long arrays.\n");
			return(MEMORY_ERROR);
		}
		dz->lparray[0]  = NULL;
		if((dz->parray  = (double **)malloc(dz->array_cnt * sizeof(double *)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for internal double arrays.\n");
			return(MEMORY_ERROR);
		}
		dz->parray[0]  = NULL;
		dz->parray[1]  = NULL;
		break;
	case(3):
		dz->array_cnt = 1; 
		if((dz->parray  = (double **)malloc(dz->array_cnt * sizeof(double *)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for internal double arrays.\n");
			return(MEMORY_ERROR);
		}
		dz->parray[0]  = NULL;
		break;
	case(4):
	case(5):
		dz->larray_cnt = 3; 
		if((dz->lparray  = (int **)malloc(dz->larray_cnt * sizeof(int *)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for internal long arrays.\n");
			return(MEMORY_ERROR);
		}
		dz->lparray[0]  = NULL;
		dz->lparray[1]  = NULL;
		dz->lparray[2]  = NULL;
		break;
	case(6):
		dz->larray_cnt = 2; 
		if((dz->lparray  = (int **)malloc(dz->larray_cnt * sizeof(int *)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for internal long arrays.\n");
			return(MEMORY_ERROR);
		}
		dz->lparray[0]  = NULL;
		dz->lparray[1]  = NULL;
		break;
	case(9):
		dz->larray_cnt = 1; 
		if((dz->lparray  = (int **)malloc(dz->larray_cnt * sizeof(int *)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for internal long arrays.\n");
			return(MEMORY_ERROR);
		}
		dz->lparray[0]  = NULL;

		dz->iarray_cnt = 1; 
		if((dz->iparray  = (int **)malloc(dz->iarray_cnt * sizeof(int *)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for internal int arrays.\n");
			return(MEMORY_ERROR);
		}
		dz->iparray[0]  = NULL;
		break;
	}
	return(FINISHED);
}

/******************************** ESTABLISH_ARRAYS ********************************/

int establish_arrays(dataptr dz)
{
	int n;
	dz->array_cnt = 2 + dz->infile->channels;
	if((dz->parray  = (double **)malloc(dz->array_cnt * sizeof(double *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for internal double arrays.\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n < dz->array_cnt;n++)
		dz->parray[n]  = NULL;

	dz->larray_cnt = 4;
	if((dz->lparray  = (int **)malloc(dz->larray_cnt * sizeof(int *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for internal long arrays.\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n < dz->larray_cnt;n++)
		dz->lparray[n]  = NULL;
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

/******************************** USAGE1 ********************************/

int usage1(void)
{
	usage2("mchanpan");
	return(USAGE_ONLY);
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
	if(!strcmp(str,"mchanpan")) {
		fprintf(stderr,
	    "USAGE: mchanpan mchanpan 1 inf outf panfile outchans [-ffocus]\n"
	    "USAGE: mchanpan mchanpan 2 inf outf switchdata outchans [-ffocus -mminsil]\n"
	    "USAGE: mchanpan mchanpan 3 inf outf outchans centre spread depth rolloff minsil [-s]\n"
	    "USAGE: mchanpan mchanpan 4 inf outf outchans centre spread depth rolloff\n"
	    "USAGE: mchanpan mchanpan 5 inf outf antiphon outchans minsil\n"
	    "USAGE: mchanpan mchanpan 6 inf1 [inf2 ..] outf antiphon outchans eventdur gap splice\n"
	    "USAGE: mchanpan mchanpan 7 inf outf pandata rolloff\n"
	    "USAGE: mchanpan mchanpan 9 inf outf outchans startchan speed focus [-a]\n"
	    "USAGE: mchanpan mchanpan 10 inf outf outchans [-ffocus -mminsil -ggrouping] [-a] [-r]\n"
		"\n"
		"Mode 1: Move a mono sound file around a multichannel space.\n"
		"Mode 2: Switch (silence-separated) mono events in file from one chan to another.\n"
		"Mode 3: Spread (s-s) mono events stepwise from one set of chans, to another set.\n"
		"Mode 4: Spread source gradually from a centre, across several channels.\n"
		"Mode 5: Switch events antiphonally between 2 specified sets of channels.\n"
		"Mode 6: Switch sounds antiphonally between 2 specified sets of channels.\n"
		"                  Mode 6 can take 1 or more inputfiles.\n"
		"Mode 7: Pan from one channel configuration to another, passing through centre.\n"
		"Mode 9: Rotate a mono sound file around a multichannel space.\n"
		"Mode 10: Switch (silence-separated) mono events randomly from one chan to another.\n"
		"\n"
		"PANFILE  Output pan-data in triples: time  pan-position  pantype\n"
		"         Pan-position vals lie between (channel) 1 and a max no-of-channels >= 3.\n"
		"         positions between 0 and 1 are also possible (see below).\n"
		"\n"
		"         Pantype values can be\n"
		"         0  = direct pan: pan from 1 to 4 goes directly between lspkrs 1->4\n"
		"         1  = clockwise rotation: pan 1 to 4 goes around lspkrs 1->2->3->4\n"
		"         -1 = anticlock rotation: pan 1 to 4 (with 8 chans) goes 1->8->7->6->5->4\n"
		"\n"
		"         Direct pans must start at single lspkr:\n"
		"         and position vals must be non-zero integers.\n"
		"\n"
		"         Rotations can start/stop anywhere, so position values can be fractional.\n"
		"         Vals between 0 & 1 are positions between max lspkr (e.g. 8) and lspkr 1\n"
		"\n"
		"SWITCHDATA list of outchans, in a textfile. Sound switches from one outchan to next.\n"
		"         If end of list reached, it starts again from its beginning.\n"
		"\n"
		"PANDATA  Output pan-data in lines, each having: time  pan-positions\n"
		"         Pan-positions is a list of ALL the input channels, in any order.\n"
		"         Each input channel in turn is mapped to each numbered channel in the list.\n"
		"         Passing from one mapping to next, sound in all chans spreads out until it's\n"
		"         output equally from all chans, then contracts to its next outchan position.\n"
		"         To force this maximal spread elsewehere,\n"
		"         enter time & list of zeros (1 for each chan).\n"
		"\n"
		"ANTIPHON two strings of letters, with separator ('-') represents antiphonal chans.\n"
		"         e.g. abcd-efgh = antiphon between chans 1,2,3,4 and chans 5,6,7,8.\n"
		"\n"
		"OUTCHANS number of channels in the output file.\n"
		"\n"
		"STARTCHAN mode 8: Channel from which rotation begins.\n"
		"\n"
		"SPEED    mode 8: Speed of rotation (cycles per sec): can vary through time.\n"
		"\n"
		"FOCUS    If focus = 1.0, position (e.g.) '2' puts all signal in lspkr 2\n"
		"         If focus = 0.9, position '2' puts 0.9 of signal in lspkr 2\n"
		"         and the remainder in the 2 adjacent lspkrs (1 and 3).\n"
		"\n"
		"MINSIL   min duration (mS) of zero-value samps to count as silence between peaks.\n"
		"CENTRE   centre of spreading. Time-variable.\n"
		"SPREAD   Chan-spread of output (far left to far right of centre). Time-variable.\n"
		"         Minimum is 1 in Mode 3 and 0 in Mode 4.\n"
		"DEPTH    max no of chans (to left, and to right) utilised, behind leading edges.\n"
		"         Signal always reaches maximum spread, but may have hole in middle. \n"
		"ROLLOFF  level fall as signal spread over several chans. Range 0-1.\n"
		"         0 = no fall in level, 1 = level divided by number of chans in use.\n"
		"EVENTDUR Time before switching to next outchannel set(can timevary).\n"
		"GAP      Time gap between switching events (can timevary).\n"
		"SPLICE   Duration (in mS) of splices cutting events.\n"
		"GROUPING (Max) number of events at outchan, before switch to next outchan.\n"
		"-s       Output steps wider by 1 channel (to both L & R) on every event,\n"
		"         as far as the spread value given (must be integer).\n"
		"         Depth value must also be integer.\n"
		"-a       Mode 8: ANTICLOCKWISE rotation (default clockwise).\n"
		"         Mode 10: no steps allowed between ADJACENT output channels.\n"
		"-r       Mode 10: Randomise number of grouped events at each outchan.\n");
	} else
		fprintf(stderr,"Unknown option '%s'\n",str);
	return(USAGE_ONLY);
}

int usage3(char *str1,char *str2)
{
	fprintf(stderr,"Insufficient parameters on command line.\n");
	return(USAGE_ONLY);
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	if(!strcmp(prog_identifier_from_cmdline,"mchanpan")) {
		dz->process = MCHANPAN; 
	} else {
		sprintf(errstr,"Unknown program identification string '%s'\n",prog_identifier_from_cmdline);
		return(USAGE_ONLY);
	}
	return(FINISHED);
}

/************************************ READ_MCHANPAN_DATA ************************************/

int read_mchanpan_data(char *filename,dataptr dz)
{
	double *p, *time, *position, *type, lasttime, maxpos, dummy;
	int n, k, outchans, itype, ipos, cnt;
	int *snake;
	char temp[200], *q;
	aplptr ap;
	FILE *fp;

	ap = dz->application;
	if((fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,	"Can't open file %s to read data.\n",filename);
		return(DATA_ERROR);
	}
	cnt = 0;
	while(fgets(temp,200,fp)==temp) {
		q = temp;
		if(*q == ';')	//	Allow comments in file
			continue;
		while(get_float_from_within_string(&q,&dummy)) {
			cnt++;
		}
	}	    
	if(cnt == 0) {
		sprintf(errstr,"No data in file %s\n",filename);
		return(DATA_ERROR);
	}
	switch(dz->mode) {
	case(0):
		if(((dz->ringsize = cnt/3) * 3) != cnt) {
			sprintf(errstr,"Data not grouped correctly in file %s\n",filename);
			return(DATA_ERROR);
		}
		if((dz->parray[0] = (double *)malloc(cnt * sizeof(double)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for data in file %s.\n",filename);
			return(MEMORY_ERROR);
		}
		time	 = dz->parray[0];
		position = time+1;
		type	 = time+2;
		rewind(fp);
		lasttime = -1.0;
		cnt = 0;
		while(fgets(temp,200,fp)==temp) {
			q = temp;
			if(*q == ';')	//	Allow comments in file
				continue;
			while(get_float_from_within_string(&q,&dummy)) {
				switch(cnt) {
				case(0):
					if(dummy < 0.0 || dummy <= lasttime) {
						sprintf(errstr,"Times do not advance correctly in file %s.\n",filename);
						return(DATA_ERROR);
					}
					*time = dummy;
					time += 3;
					break;
				case(1):
					if(dummy < ap->min_special || dummy > ap->max_special) {
						sprintf(errstr,"Invalid position value (%lf) in file %s.\n",dummy,filename);
						return(DATA_ERROR);
					}
					*position = dummy;
					position += 3;
					break;
				case(2):
					if(dummy < ap->min_special2 || dummy > ap->max_special2) {
						sprintf(errstr,"Invalid pantytpe value (%lf) in file %s.\n",dummy,filename);
						return(DATA_ERROR);
					}
					k = (int)round(dummy);
					if((double)k != dummy) {
						sprintf(errstr,"Invalid pantytpe value (%lf) in file %s.\n",dummy,filename);
						return(DATA_ERROR);
					}
					*type = dummy;
					type += 3;
					break;
				}
				cnt++;
				cnt %= 3;
			}
		}
		if(fclose(fp)<0) {
			fprintf(stdout,"WARNING: Failed to close file %s.\n",filename);
			fflush(stdout);
		}
		maxpos = -DBL_MAX;
		p = dz->parray[0];
		p++;
		for(n=0;n<dz->ringsize;n++) {
			if(*p > maxpos)
				maxpos = *p;
			p+=3;
		}
		outchans = (int)ceil(maxpos);
//		if(outchans < 3) {
//			sprintf(errstr,"Less than 3 output channels required: this process is for panning over 3 or more channels only.\n");
//			return(DATA_ERROR);
//		}
		dz->itemcnt = outchans;

		time	 = dz->parray[0];
		position = time+1;
		type	 = time+2;
		for(n=0;n<dz->ringsize;n++) {
			itype = (int)round(*type);
			if(itype == 0) {
				ipos = (int)round(*position);
				if((double)ipos != *position) {
					sprintf(errstr,"Non-integer position (%lf) associated with a direct pan, at line %d.\n",*position,n+1);
					return(DATA_ERROR);
				}
			}
			time += 3;
			position += 3;
			type += 3;
		}
		break;
	case(1):
		if((dz->lparray[0] = (int *)malloc(cnt * sizeof(int)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for data in file %s.\n",filename);
			return(MEMORY_ERROR);
		}
		snake = dz->lparray[0];
		dz->itemcnt = 0;
		rewind(fp);
		cnt = 0;
		while(fgets(temp,200,fp)==temp) {
			q = temp;
			if(*q == ';')	//	Allow comments in file
				continue;
			while(get_float_from_within_string(&q,&dummy)) {
				if((snake[cnt] = (int)round(dummy)) < 1) {
					sprintf(errstr,"Invalid  channel number (%d) in snake data.\n",snake[cnt]);
					return(MEMORY_ERROR);
				}
				dz->itemcnt = max(dz->itemcnt,snake[cnt]);	//	dz->itemcnt stores max channel referred to in listing
				cnt++;
			}
		}
		dz->ringsize = cnt;									//	dz->ringsize stores number of items in listing
		if(fclose(fp)<0) {
			fprintf(stdout,"WARNING: Failed to close file %s.\n",filename);
			fflush(stdout);
		}
		break;
	}
	return(FINISHED);
}

/************************************ READ_MCHANCROSS_DATA ************************************/

int read_mchancross_data(char *filename,dataptr dz)
{
	double time = 0.0, lasttime = -1.0, dummy, srate = (double)dz->infile->srate;
	int n, m, k, chans = dz->infile->channels, cnt;
	int *events, *maps;
	int eventcnt, mapcnt, thischan;
	char temp[200], *q;
	FILE *fp;

	fprintf(stdout,"INFO: Checking panning data\n");
	fflush(stdout);
	if((fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,	"Can't open file %s to read data.\n",filename);
		return(DATA_ERROR);
	}
	cnt = 0;
	while(fgets(temp,200,fp)==temp) {
		q = temp;
		if(*q == ';')	//	Allow comments in file
			continue;
		while(get_float_from_within_string(&q,&dummy)) {
			cnt++;
		}
	}	    
	if(cnt == 0) {
		sprintf(errstr,"No data in file %s\n",filename);
		return(DATA_ERROR);
	}
	k = dz->infile->channels + 1;
	if ((dz->itemcnt = cnt/k) * k != cnt) {
		sprintf(errstr,	"Data in file %s not grouped correctly.\n",filename);
		return(DATA_ERROR);
	}
	dz->ringsize = cnt - dz->itemcnt;
	if((dz->lparray[0] = (int *)malloc((dz->itemcnt + 2) * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for data in file %s.\n",filename);
		return(MEMORY_ERROR);
	}
	if((dz->lparray[1] = (int *)malloc((dz->ringsize + (2 * chans)) * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for data in file %s.\n",filename);
		return(MEMORY_ERROR);
	}
	events = dz->lparray[0];
	maps = dz->lparray[1];
	rewind(fp);
	cnt = 0;
	eventcnt = 0;
	mapcnt = 0;
	while(fgets(temp,200,fp)==temp) {
		q = temp;
		if(*q == ';')	//	Allow comments in file
			continue;
		while(get_float_from_within_string(&q,&dummy)) {
			if(cnt % k == 0) {
				time = dummy;
				if(eventcnt == 0) {
					if(time < 0.0) {
						sprintf(errstr,"Invalid time (%lf) in file %s\n",time, filename);
						return(DATA_ERROR);
					} 
				} else if(time <= lasttime) {
					sprintf(errstr,"Times do not increase between (%lf) and %lf in file %s\n",lasttime, time, filename);
					return(DATA_ERROR);
				}
				if(eventcnt >= dz->itemcnt) {
					sprintf(errstr,"Accounting error 1 in reading crosspan data\n");
					return(PROGRAM_ERROR);
				}
				events[eventcnt++] = (int)round(time * srate) * chans;
				lasttime = time;
			} else {
				thischan = (int)round(dummy);
				if(thischan < 0 || thischan > chans) {
					sprintf(errstr,"Invalid channel (%d) at time %lf in file %s\n",thischan, time, filename);
					return(DATA_ERROR);
				}
				thischan--;
				if(mapcnt >= dz->ringsize) {
					sprintf(errstr,"Accounting error 2 in reading crosspan data\n");
					return(PROGRAM_ERROR);
				}
				maps[mapcnt++] = thischan;
			}
			cnt++;
		}
	}
	if(fclose(fp)<0) {
		fprintf(stdout,"WARNING: Failed to close file %s.\n",filename);
		fflush(stdout);
	}
	for(mapcnt=0,k=0;mapcnt<dz->ringsize;mapcnt+=chans,k++) {
		if(maps[mapcnt] >= 0) {
			for(n=0;n<chans;n++) {
				if(maps[mapcnt+n] < 0) {
					sprintf(errstr,"Zero channel entries can only be used for EVERY line in a map (see time %lf)\n",(double)(events[k]/chans)/srate);
					return(DATA_ERROR);
				}
			}
			for(n=0;n<chans-1;n++) {
				for(m = n+1;m<chans;m++) {
					if(maps[mapcnt+n] == maps[mapcnt+m]) {
						sprintf(errstr,"Channel %d duplicated at time %lf\n",maps[mapcnt+n]+1,(double)(events[k]/chans)/srate);
						return(DATA_ERROR);
					}
				}
			}
		} else {
			for(n=0;n<chans;n++) {
				if(maps[mapcnt+n] >= 0) {
					sprintf(errstr,"Zero channel entries can only be used for EVERY line in a map (see time %lf)\n",(double)(events[k]/chans)/srate);
					return(DATA_ERROR);
				}
			}
		}
	}
	if(events[0] > 0) {								//	If ness, insert an event at tome 0	
		for(n=dz->itemcnt-1;n>=0;n--)
			events[n+1] = events[n];				//	Copy all times to next higher place
		events[0] = 0;								//	Insert time 0 at foot of array
		dz->itemcnt++;								//	Increment count of events	
		for(n=dz->ringsize - 1;n>=0;n-=chans)		//	Copy all maps to next higher map-place
			maps[n+chans] = maps[n];				//	This also leaves a copy of lowest map where it is
		dz->ringsize += chans;						//	Increment count of map items by length of maps
	}
	if(events[dz->itemcnt - 1] < dz->insams[0]) {	//	If ness, insert event at end of file
		events[dz->itemcnt] = dz->insams[0];
		dz->itemcnt++;
		for(n=0;n<chans;n++)						//	As a copy of the final data map
			maps[dz->ringsize + n] = maps[dz->ringsize - chans + n];
		dz->ringsize += chans;
	}
	return(FINISHED);
}

/************************* SETUP_MCHANPAN_APPLICATION *******************/

int setup_mchanpan_application(dataptr dz)
{
	int exit_status;
	aplptr ap;
	if((exit_status = establish_application(dz))<0)		// GLOBAL
		return(FAILED);
	ap = dz->application;
	// SEE parstruct FOR EXPLANATION of next 2 functions
	switch(dz->mode) {
	case(0):
		if((exit_status = set_param_data(ap,MCHANDATA   ,1,1,"i"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","f",1,1,"d"))<0)
			return(FAILED);
		break;
	case(1):
		if((exit_status = set_param_data(ap,MCHANDATA2  ,1,1,"i"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","fm",2,2,"dd"))<0)
			return(FAILED);
		break;
	case(2):
		if((exit_status = set_param_data(ap,0,6,6,"iDDDDd"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","s",1,0,"0"))<0)
			return(FAILED);
		break;
	case(3):
		if((exit_status = set_param_data(ap,0,5,5,"iiDDD"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","",0,0,""))<0)
			return(FAILED);
		break;
	case(4):
		if((exit_status = set_param_data(ap,ANTIPHON,2,2,"id"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","",0,0,""))<0)
			return(FAILED);
		break;
	case(5):
		if((exit_status = set_param_data(ap,ANTIPHON,4,4,"iDDd"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","",0,0,""))<0)
			return(FAILED);
		break;
	case(6):
		if((exit_status = set_param_data(ap,CROSSPAN,1,1,"D"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","",0,0,""))<0)
			return(FAILED);
		break;
	case(7):
		if((exit_status = set_param_data(ap,0,2,2,"DD"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","",0,0,""))<0)
			return(FAILED);
		break;
	case(8):
		if((exit_status = set_param_data(ap,0,4,4,"iiDd"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","a",1,0,"0"))<0)
			return(FAILED);
		break;
	case(9):
		if((exit_status = set_param_data(ap,0  ,1,1,"i"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"fmg",3,"ddI","ar",2,0,"00"))<0)
			return(FAILED);
		break;
	}
	// set_legal_infile_structure -->
	dz->has_otherfile = FALSE;
	// assign_process_logic -->
	if(dz->mode == 7) {
		dz->input_data_type = SNDFILES_ONLY;
		dz->process_type	= OTHER_PROCESS;	
		dz->outfiletype  	= NO_OUTPUTFILE;
		return application_init(dz);	//GLOBAL
	}
	if(dz->mode == 5)
		dz->input_data_type = ONE_OR_MANY_SNDFILES;
	else
		dz->input_data_type = SNDFILES_ONLY;
	dz->process_type	= UNEQUAL_SNDFILE;	
	dz->outfiletype  	= SNDFILE_OUT;
	return application_init(dz);	//GLOBAL
}

/************************* SETUP_MCHANPAN_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_mchanpan_param_ranges_and_defaults(dataptr dz)
{
	int exit_status;
	aplptr ap = dz->application;
	// set_param_ranges()
	ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
	// NB total_input_param_cnt is > 0 !!!
	if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
		return(FAILED);
	// get_param_ranges()
	if(dz->mode == 6) {
		ap->lo[0]	= 0;
		ap->hi[0]	= 1;
		ap->default_val[0]	= 1;
	} else {
		ap->lo[0]	= 3;
		ap->hi[0]	= 16;
		ap->default_val[0]	= 8;
	}
	switch(dz->mode) {
	case(9):
		ap->lo[3]	= 1;
		ap->hi[3]	= 100;
		ap->default_val[3]	= 1;
		/* fall thro */
	case(1):
		ap->lo[2]	= (2.0/(double)dz->infile->srate) * SECS_TO_MS;
		ap->hi[2]	= (32767.0/(double)dz->infile->srate) * SECS_TO_MS;
		ap->default_val[2]	= SILMIN;
		/* fall thro */
	case(0):
		ap->lo[1]	= 0;
		ap->hi[1]	= 1;
		ap->default_val[1]	= 1;
		break;
	case(2):
		ap->lo[5]	= (2.0/(double)dz->infile->srate) * SECS_TO_MS;		//	inter-event silence
		ap->hi[5]	= (32767.0/(double)dz->infile->srate) * SECS_TO_MS;
		ap->default_val[5]	= SILMIN;
		/* fall thro */
	case(3):
		ap->lo[1]	= 0.0;			// centre
		ap->hi[1]	= 8.0;
		ap->default_val[1]	= 1;
		ap->lo[2]	= 0.0;			// spread
		ap->hi[2]	= 16.0;
		ap->default_val[2]	= 8;
		ap->lo[3]	= 0.0;			// maxdepth
		ap->hi[3]	= 8;
		ap->default_val[3]	= 4;
		ap->lo[4]	= 0.0;			// rolloff
		ap->hi[4]	= 1.0;
		ap->default_val[4]	= 0.0;
		break;
	case(4):
		ap->lo[1]	= (2.0/(double)dz->infile->srate) * SECS_TO_MS;
		ap->hi[1]	= (32767.0/(double)dz->infile->srate) * SECS_TO_MS;
		ap->default_val[1]	= SILMIN;
		break;
	case(5):
		ap->lo[1]	= 0.0;
		ap->hi[1]	= 60.0;
		ap->default_val[1]	= 0.5;
		ap->lo[2]	= 0.0;
		ap->hi[2]	= 60.0;
		ap->default_val[2]	= 0.0;
		ap->lo[3]	= 0.0;
		ap->hi[3]	= 1000.0;
		ap->default_val[3]	= 15.0;
		break;
	case(7):
		ap->lo[0]	= 0.0;
		ap->hi[0]	= 16.0;
		ap->default_val[0]	= 1.0;
		ap->lo[1]	= 0.0;
		ap->hi[1]	= 16.0;
		ap->default_val[1]	= 0.0;
		break;
	case(8):
		ap->lo[1]	= 0;
		ap->hi[1]	= 16;
		ap->default_val[1]	= 1;
		ap->lo[2]	= 0;
		ap->hi[2]	= 64;
		ap->default_val[2]	= 1;
		ap->lo[3]	= 0;
		ap->hi[3]	= 1;
		ap->default_val[3]	= 1;
		break;
	}
	if(!sloom)
		put_default_vals_in_all_params(dz);
	return(FINISHED);
}

/************************************************** MCHANPAN ********************************************/

#define DIRECT 0
#define ROTATION 1

int mchanpan(dataptr dz)
{
	int exit_status;
	int 	i, chan, got_newposition, step=0;
	int		brkindex = 1, leftchan = 1, rightchan=0, farleftchan=0, farrightchan=0, outchans = dz->iparam[0];
	int		startbesideleft=0, startbesideright=0, goalbesideleft=0, goalbesideright=0;
	int		true_goalchan = (int)round(dz->parray[0][1]), goalchan = true_goalchan - 1, startchan=0, pantype=0;
	int	block = 0, sams = 0, total_sams = 0;
	float  *inbuf = dz->sampbuf[0], *bufptr;
	double	leftgain,rightgain, farleftgain,farrightgain, outchansd = (double)outchans;
	double	lcoef = 0.0, rcoef = 0.0, llcoef = 0.0, rrcoef = 0.0, maxllcoef=0.0, factor;			
	double	position = 0.0, posincr = 0.0, chan_multiplier;
	display_virtual_time(0L,dz);
	do {
		if((exit_status = read_samps(inbuf,dz))<0) {
			sprintf(errstr,"Failed to read data from sndfile.\n");
			return(DATA_ERROR);
		}
		bufptr = dz->sampbuf[1];	/* set output buffer pointer */

		for (i = 0 ; i < dz->ssampsread; i++ ) {
			got_newposition = 0;
			if(sams-- <= 0) {				/* We count down in samples to the next breakpoint position value */
				startchan = goalchan;		/* The old gola-channel becomes the new start-channel, as we arrived at goal */
				sams	  = newposition(&brkindex,&position,&posincr,&total_sams,&true_goalchan,&pantype,dz);
				got_newposition = 1;
				goalchan  = true_goalchan - 1;
				if(pantype == DIRECT) {					/* Define channels alongside actual start and goal chans, for use where focus < 1.0 */
					startbesideleft = startchan - 1;	/* and sound is spread to adjacent loudspeakers */
					if(startbesideleft < 0)				/* NB these parameters run from 0 to outchans-less-1, where input positions run from 1 to outchans */
						startbesideleft = outchans - 1;
					startbesideright = startchan + 1;
					if(startbesideright >= outchans)
						startbesideright = 0;
					goalbesideleft = goalchan - 1;
					if(goalbesideleft < 0)
						goalbesideleft = outchans - 1;
					goalbesideright = goalchan + 1;
					if(goalbesideright >= outchans)
						goalbesideright = 0;
				}
			}											/* gain data is only recalculated at each sample-block, OR when a new-position has been set */
			if((block-- <= 0) || got_newposition) {		
				mchan_pancalc(position,&leftgain,&rightgain,&farleftgain,&farrightgain,&leftchan,pantype,1,dz);
				if(pantype == ROTATION) {
					leftchan--;												/* output bufs numbered from 0 */
					farleftchan  = (leftchan + outchans - 1) % outchans;	/* output chans are adjacent, L and R refer to lspkrs to left and right of current position */
					rightchan    = (leftchan + 1) % outchans;				/* farleft and farright are speakers next to left and right spkrs, for any unfocused signal */
					farrightchan = (rightchan + 1) % outchans;
				}
				lcoef  = leftgain;
				rcoef  = rightgain;
				llcoef = farleftgain; 
				rrcoef = farrightgain;
				if(pantype == DIRECT) {
					if(got_newposition)	{						/* WHERE THE FOCUS IS < 1.0, (and hence maxllcoef > 0) sounds spills to adjacent channels. */
						maxllcoef = llcoef;						/* lcoef, (level on startchan) does not fade to zero, nor does rcoef (level on goalchan) start at zero. */
						step = abs(goalchan - startchan);		/* llcoeff (level on chans adj to startchan) however ALWAYS falls to zero, at end of pan. */
					}
					if((step > 1) && (step != outchans - 1)) {
						if(maxllcoef > 0) {						/* In pans between non-adjacent loudspeakers, we use the complete fade-out of the llcoef */
							factor = llcoef/maxllcoef;			/* to create a 'factor' which ensures that lcoef fades completely at end of pan */
							lcoef *= factor;					/* and that rcoef (controlling goal-position signal) starts from zero */
							rcoef *= (1.0 - factor);
						}										/* For adjacent-loudspeaker pans (e.g. 2->3) */
					}											/* llcoef & rrcoef behave differently and themselves compensate for the failure to fade to zero. */
				}					
				block += BSIZE;
				position += posincr;				/* position is incremented for next block-read */
				switch(pantype) {
				case(DIRECT):						/* In Direct pan case */
					if(position > 1.0)				/* position = fraction of distance between any lspkr-pair (range  0-1) */
						position = 1.0;
					else if(position < 0.0)
						position = 0.0;
					break;
				case(ROTATION):						/* In Rotation pan case */
					if(position > outchansd)		/* position = lspkr no (or some fraction inbetween) (range 1 - outchans) */
						position -= outchansd;		/* Convention is that vals 1 to 8 refers to positions at or between lspkrs 1-8 */
					else if(position < 0.0)			/* while values below 1 refer to positions between lspkr 8 and lspkr 1, in the 'circle' */
						position += outchansd;		/* We adjust here, when position-incrementation takes position beyond these limits */
						break;
				}
	    	}
			switch(pantype) {
			case(DIRECT):
				for(chan=0;chan<outchans;chan++) {
					chan_multiplier = 0.0;
					if(chan == startchan)												/* output chans are not ness adjacent */
						chan_multiplier += lcoef;										/* 'left' &  'right' refer to start and goal of any pair of pand speakers */
					if((chan == startbesideleft) && (startbesideleft != goalchan))		/* startbesideleft & startbesideright are lspkrs to left and right of start position */
						chan_multiplier += llcoef;										/* goalbesideleft & goalbesideright are lspkrs to left and right of goal position */
					if((chan == startbesideright) && (startbesideright != goalchan))	/* these will only get signal, if focus < 1 */
						chan_multiplier += llcoef;										/* i.e. 2 --> 5 may imply 1 --> 4 and 3 --> 6, with lower level signal */
					if(chan == goalchan)
						chan_multiplier += rcoef;										/* Exceptions (!= goalchan ETC) deal with case where pan is between adjacent lspkrs, */
					if((chan == goalbesideleft) && (goalbesideleft != startchan))		/* and focus is < 1.0, and hence the unfocused signal and focused signal overlap. */
						chan_multiplier += rrcoef;										/* In General case, e.g. (1)2(3) --> (5)6(7), but in special case (1)2(3) --> (2)3(4) */
					if((chan == goalbesideright) && (goalbesideright != startchan))		
						chan_multiplier += rrcoef;										/* Amount of input signal going to this particular output channel */
					*bufptr += (float)(chan_multiplier * inbuf[i]);						/* is a sum of contributions depending on whether the channel is the start or goal channel */
					bufptr++;															/* or, where focus < 1.0, it is adjacent to the start or goal channel */	
				}
				break;
			case(ROTATION):
				for(chan=0;chan<outchans;chan++) {
					chan_multiplier = 0.0;
					if(chan == farleftchan)
						chan_multiplier += llcoef;
					if(chan == leftchan)
						chan_multiplier += lcoef;
					if(chan == rightchan)
						chan_multiplier += rcoef;
					if(chan == farrightchan)
						chan_multiplier += rrcoef;										/* Amount of input signal going to this particular output channel */
					*bufptr += (float)(chan_multiplier * inbuf[i]);						/* is a sum of contributions depending on whether the channel is the start or goal channel */
					bufptr++;															/* or, where focus < 1.0, it is adjacent to these start or goal channel */
				}
				break;
			}
		}
		if(dz->ssampsread > 0) {
			if((exit_status = write_samps(dz->sampbuf[1],dz->ssampsread * outchans,dz))<0)
				return(exit_status);
		}
		memset((char *)dz->sampbuf[1],0,dz->buflen * dz->iparam[0] * sizeof(float));
	} while(dz->samps_left > 0);
	return(FINISHED);
}

/************************************ NEWPOSITION *******************************/

int newposition(int *brkindex,double *position,double *posincr,int *total_sams,int *true_goalchan,int *pantype,dataptr dz)
{
	double diff, steps, nextval;
	double *thisbrk = dz->parray[0];
	int here, sams;
	double outchans = (double)dz->iparam[0];
	if(*brkindex < dz->ringsize) {
		here  = (*brkindex) * 3;
		sams  = round(thisbrk[here] * dz->infile->srate) - *total_sams;
		steps = (double)sams/(double)BSIZE;	
		*pantype = (int)round(thisbrk[here-1]);
		switch(*pantype) {
		case(0):
			*true_goalchan = (int)round(thisbrk[here+1]);	/* gets next lspkr to move to */
			*position = 0;									/* starts at current lspkr (notionally 0) */
			*posincr = 1.0/steps;							/* proceeds to next lspkr  (notionally 1) */
			(*brkindex)++;
			break;
		case(1):											/* interpolates between position values in file, in increasing order */
			*position  = thisbrk[here-2];
			nextval = thisbrk[here+1];						/* gets next position in file */
			diff  = fabs(nextval - (*position));			/* steps between breakpnt positions, thro all intermediate positions */
			if(nextval < *position)
				diff = outchans - diff;					/* in order to move thro lspkrs in increasing order, 3 to 1 means 3->4->5->6->7->8->1 */
			*posincr = diff/steps;							/* position gradually increments */
			(*brkindex)++;
			break;
		case(-1):											/* interpolates between position values in file, in DEcreasing order */
			*position  = thisbrk[here-2];
			nextval = thisbrk[here+1];						/* gets next position */
			diff  = fabs(nextval - (*position));			/* steps between breakpnt positions, thro all intermediate positions, in decreasing order */
			if(nextval > *position)
				diff = outchans - diff;					/* in order to move thro lspkrs in DEcreasing order, 1 to 3 means 1->8->7->6->5->4->3 */
			*posincr = -(diff/steps);						/* position gradually DEcrements */
			(*brkindex)++;
			break;
		}
	} else {
		*posincr = 0.0;
		sams = dz->insams[0] - *total_sams;
	}
	*total_sams += sams;
	if(*pantype != 0)
		*pantype = 1;	/* clockwise and anticlockwise rotation use the same algorithm */
	return(sams);	
}

/************************************ MCHAN_PANCALC *******************************/

void mchan_pancalc(double position,double *leftgain,double *rightgain,double *farleftgain,double *farrightgain,int *leftchan,int pantype,int focusparam,dataptr dz)
{
	double temp;
	double relpos;
	double holecompensate, stereoposition, zerocentredposition;
	double focus = dz->param[focusparam];

	*leftchan = (int)floor(position);
	stereoposition = position - (double)(*leftchan);	/* range   0 to 1 */
	zerocentredposition = (stereoposition * 2.0) - 1.0;	/* range  -1 to 1 */
	if(zerocentredposition < 0) 
		relpos = -zerocentredposition;					/* range   0 to 1 : position relative to centre of stereo */
	else 
		relpos = zerocentredposition;
	temp = 1.0 + (relpos * relpos);						/* calculate hole in middle compensation */
	holecompensate = ROOT2 / sqrt(temp);
	*rightgain = stereoposition * holecompensate;
	*leftgain = (1.0 - stereoposition) * holecompensate;
	*rightgain = ((*rightgain) * focus) + dz->scalefact;	/* adjust levels from range 0-1 into range 'focus' */
	*leftgain  = ((*leftgain)  * focus) + dz->scalefact;
	*farleftgain  = dz->scalefact * (1.0 - stereoposition);	/* calculate further-out lspkr-levels */
	*farrightgain = dz->scalefact * stereoposition;
	if(pantype == 1 && *leftchan == 0)						/* rotations only */
		*leftchan = dz->iparam[0];							/* positions below lspkr 1 are between lpskr 'outchans' and lspkr 1 */
}

/**************************** ALLOCATE_MCHANPAN_BUFFER ******************************/

int allocate_mchanpan_buffer(dataptr dz)
{
	int n, m;
	size_t bigbufsize;
	if(dz->sbufptr == 0 || dz->sampbuf==0) {
		sprintf(errstr,"buffer pointers not allocated: create_sndbufs()\n");
		return(PROGRAM_ERROR);
	}
	bigbufsize = (size_t) Malloc(-1);
	bigbufsize /= dz->bufcnt;
	if(bigbufsize <=0) {
		bigbufsize  =  F_SECSIZE * sizeof(float);	  	  /* RWD keep ths for now */

	}
	dz->buflen = bigbufsize / sizeof(float);	
	bigbufsize = dz->buflen * sizeof(float);
	if((dz->bigbuf = (float *)malloc(bigbufsize  * dz->bufcnt)) == NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
		return(PROGRAM_ERROR);
	}
	//	We grab (N * buflen) for each N-channel infile buffer
	for(m=0,n=0;n<dz->bufcnt;m++,n+=dz->infile->channels)
		dz->sbufptr[m] = dz->sampbuf[m] = dz->bigbuf + (dz->buflen * n);
		//	And the final buffer space for the outfile buffer

	dz->sampbuf[m] = dz->bigbuf + (dz->buflen * n);
	dz->buflen *= dz->infile->channels;		//	If infile is > mono, read to a bigger buffer
	return(FINISHED);
}

/**************************** CHECK_THE_PARAM_VALIDITY_AND_CONSISTENCY ****************************/

int check_the_param_validity_and_consistency(dataptr dz)
{
	int exit_status;
	double maxval, outchans = (double)dz->iparam[0];
	switch(dz->mode) {
	case(1):
		dz->iparam[2] = (int)round(dz->param[2] * MS_TO_SECS * (double)dz->infile->srate);
		/* fall thro */
	case(0):
		if(outchans < dz->itemcnt) {
			sprintf(errstr,"Data file accesses more channels than specified in the 'outchans' parameter.\n");
			return(DATA_ERROR);
		}
		dz->scalefact = (1.0 - dz->param[1])/2.0;		/* Set level of signal distributed to adjacent lspkrs */
		break;
	case(2):
		if(dz->vflag[0]) {
			if(dz->brksize[1] || dz->brksize[2] || dz->brksize[3]) {
				sprintf(errstr,"Brkpoint files for centre spread and depth, cannot be used when STEP flag is set\n");
				return(DATA_ERROR);
			}
			dz->iparam[1] = (int)round(dz->param[1]);
			dz->iparam[2] = (int)round(dz->param[2]);
			dz->iparam[3] = (int)round(min(dz->param[3],outchans/2.0));
		}
		dz->iparam[5] = (int)round(dz->param[5] * MS_TO_SECS * (double)dz->infile->srate);
		/* fall thro */
	case(3):
		if(dz->brksize[1]) {
			if((exit_status = get_maxvalue_in_brktable(&maxval,1,dz))<0)
				return(exit_status);
			dz->param[1] = maxval;
		}
		if(dz->param[1] > outchans) {
			sprintf(errstr,"(Max) Spread-centre value (%lf) incompatible with output channels count (%d)\n",dz->param[1],dz->iparam[0]);
			return(DATA_ERROR);
		}
		if(dz->mode == 3) {
			if(dz->brksize[1]) {
				sprintf(errstr,"Brkpoint files for centre cannot be used.\n");
				return(DATA_ERROR);
			}
			dz->iparam[1] = (int)round(dz->param[1]);
			if(dz->iparam[1] != dz->param[1]) {
				sprintf(errstr,"Fractional values for centre cannot be used.\n");
				return(DATA_ERROR);
			}
		}
		break;
	case(4):
		dz->iparam[1] = (int)round(dz->param[1] * MS_TO_SECS * (double)dz->infile->srate) * dz->infile->channels;
		if((exit_status = setup_antiphon_arrays(dz->wordstor[0],dz)) < 0)
			return(exit_status);
		break;
	case(5):
		if(dz->param[1] < .05) {
			sprintf(errstr,"INFO: Antiphonal steps too small\n");
			return(DATA_ERROR);
		}
		if((exit_status = setup_antiphon_arrays(dz->wordstor[0],dz)) < 0)
			return(exit_status);
		if((exit_status = generate_antiphonal_events_array(dz)) < 0)
			return(exit_status);
		break;
	case(7):
		if(dz->param[0] > dz->infile->channels) {
			sprintf(errstr,"Centre beyond channel count.\n");
			return(DATA_ERROR);
		}
		if(dz->param[1] > dz->infile->channels) {
			sprintf(errstr,"Spread too large for channel count.\n");
			return(DATA_ERROR);
		}
		break;
	case(8):
		dz->scalefact = (1.0 - dz->param[3])/2.0;		/* Set level of signal distributed to adjacent lspkrs */
		if(dz->param[1] > dz->param[0]) {
			sprintf(errstr,"Start Channel is too large for given channel count.\n");
			return(DATA_ERROR);
		}
		break;
	case(9):
		dz->iparam[2] = (int)round(dz->param[2] * MS_TO_SECS * (double)dz->infile->srate);
		dz->scalefact = (1.0 - dz->param[1])/2.0;		/* Set level of signal distributed to adjacent lspkrs */
		if((dz->iparray[0] = (int *)malloc(dz->iparam[0] * sizeof(int)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to make channel perm array.\n");
			return(MEMORY_ERROR);
		}
		break;
	}
	return(FINISHED);
}

/**************************** ZING ****************************
 *
 *	Bounce silence-separated events in a source around a sequence of output lspkr positions.
 *
 */

int zing(dataptr dz)
{	
	int exit_status, done, permno = 0, grpno = 0, grpcnt=0, endchan = -1, eventsarray;
	float *ibuf = dz->sampbuf[0], *obuf = dz->sampbuf[1];
	int *snake = dz->lparray[0], *events;
	int *perm=NULL;
	int ibufpos = 0, abs_samp_pos ,buf_start, segstart, segend, k;
	int  snakepos = 0, outchans = dz->iparam[0];
	int silsegscnt = 0, segscnt;
	double smear, focus, time = 0.0, srate = (double)dz->infile->srate;
	int thisoutchan, thisleftoutchan=0, thisrightoutchan=0;

	focus = dz->param[1];
	smear = dz->scalefact;
	if(dz->mode == 9) {	//	Random sequence
		perm = dz->iparray[0];
		if(dz->brksize[3]) {
			if((exit_status = read_value_from_brktable(0.0,3,dz))<0)
				return(exit_status);
		}
		grpcnt = dz->iparam[3];
		if(dz->vflag[1])
			grpcnt = (int)floor(drand48() * (double)grpcnt) + 1;
	}
	memset((char *)obuf,0,dz->buflen * outchans * sizeof(float));

	/* COUNT SILENCE-SPEARATED EVENTS IN DATA */
	
	if(dz->mode	== 9)
		eventsarray = 0;
	else
		eventsarray = 1;
	if((exit_status = count_events(2,eventsarray,&silsegscnt,dz))<0)
		return(exit_status);
	events = dz->lparray[eventsarray];
	k = silsegscnt/2; 
	if(k < 2) {
		sprintf(errstr,"Only one event found. No panning will take place. Try adjusting the length of silences you are using.\n");
		return(DATA_ERROR);
	} else {
		fprintf(stdout,"INFO: %d distinct events found.\n",k);
		fflush(stdout);
	}
	/* PAN, SWITCHING OUTCHAN FOR EACH PEAK */

	fprintf(stdout,"INFO: Panning sound.\n");
	fflush(stdout);
	if((sndseekEx(dz->ifd[0],0,0)<0)){
        sprintf(errstr,"sndseek() failed\n");
        return SYSTEM_ERROR;
    }
	reset_filedata_counters(dz);
	if((exit_status = read_samps(ibuf,dz))<0) {
		sprintf(errstr,"Failed to read data from sndfile.\n");
		return(DATA_ERROR);
	}
	if(dz->ssampsread == 0) {
		sprintf(errstr,"No data found in soundfile.\n");
		return(DATA_ERROR);
	}
	buf_start = 0;
	segscnt = 0;
	if(dz->mode == 9)  {
		do {
			permute_chans(outchans,dz);
		} while(adjacence(endchan,dz));
		endchan = perm[outchans - 1];
		thisoutchan = perm[permno];
		grpno++;
	} else {
		thisoutchan = snake[snakepos++] - 1;	/* outchans stored as 1 to N, values used as 0 to (N-1) */
		snakepos %= dz->ringsize;				/* cycle round lspkr positions in param-list */
	}
	if(smear  > 0.0) {
		thisleftoutchan = thisoutchan - 1;
		if(thisleftoutchan < 0)
			thisleftoutchan = outchans - 1;
		thisrightoutchan = thisoutchan + 1;
		if(thisrightoutchan >= outchans)
			thisrightoutchan = 0;
	}
	segstart = events[segscnt++];
	segend   = events[segscnt++];
	done = 0;
	while(!done) {
		while(dz->total_samps_read <= segstart) {
			if((exit_status = write_samps(obuf,dz->ssampsread * outchans,dz))<0)
				return(exit_status);
			memset((char *)obuf,0,dz->buflen * outchans * sizeof(float));
			buf_start = dz->total_samps_read;
			if((exit_status = read_samps(ibuf,dz))<0) {
				sprintf(errstr,"Failed to read data from sndfile.\n");
				return(DATA_ERROR);
			}
			if(dz->ssampsread == 0) {
				done = 1;
				break;
			}
		}
		if(done)
			break;
		ibufpos = segstart - buf_start;
		abs_samp_pos = segstart;
		while(abs_samp_pos < segend) {
			k = (ibufpos * outchans) + thisoutchan;
			obuf[k] = (float)(obuf[k] + (ibuf[ibufpos] * focus));
			if(smear > 0.0) {
				k = (ibufpos * outchans) + thisleftoutchan;
				obuf[k]  = (float)(obuf[k] + (ibuf[ibufpos] * smear));
				k = (ibufpos * outchans) + thisrightoutchan;
				obuf[k]  = (float)(obuf[k] + (ibuf[ibufpos] * smear));
			}
			ibufpos++;
			if(ibufpos >= dz->ssampsread) {
				if((exit_status = write_samps(obuf,dz->ssampsread * outchans,dz))<0)
					return(exit_status);
				memset((char *)obuf,0,dz->buflen * outchans * sizeof(float));
				buf_start = dz->total_samps_read;
				if((exit_status = read_samps(ibuf,dz))<0) {
					sprintf(errstr,"Failed to read data from sndfile.\n");
					return(DATA_ERROR);
				}
				if(dz->ssampsread == 0) {
					done = 1;
					break;
				}
				ibufpos = 0;
			}
			abs_samp_pos++;
		}
		if(done)
			break;
		if(segscnt >= silsegscnt)
			break;
		if(dz->mode == 9) {
			if(grpno >= grpcnt) {
				if(dz->brksize[3]) {
					time = (double)(buf_start + ibufpos)/srate;
					if((exit_status = read_value_from_brktable(time,3,dz))<0)
						return(exit_status);
				}
				grpcnt = dz->iparam[3];
				if(dz->vflag[1])
					grpcnt = (int)floor(drand48() * (double)grpcnt) + 1;
				grpno = 0;
				permno++;
			}
			if(permno >= outchans) {
				do {
					permute_chans(outchans,dz);
				} while((perm[0] == endchan) || adjacence(endchan,dz));
				endchan = perm[outchans - 1];
				permno = 0;
			}
			thisoutchan = perm[permno];
			grpno++;
		} else {
			thisoutchan = snake[snakepos++] - 1;
			snakepos %= dz->ringsize;
		}
		if(smear  > 0.0) {
			thisleftoutchan = thisoutchan - 1;
			if(thisleftoutchan < 0)
				thisleftoutchan = outchans - 1;
			thisrightoutchan = thisoutchan + 1;
			if(thisrightoutchan >= outchans)
				thisrightoutchan = 0;
		}
		segstart = events[segscnt++];
		segend   = events[segscnt++];
	}
	if(ibufpos > 0) {
		if((exit_status = write_samps(dz->sampbuf[1],ibufpos * outchans,dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}

/****************************** GET_MODE *********************************/

int get_the_mode_from_cmdline(char *str,dataptr dz)
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

/*************************** COUNT_EVENTS ****************************/
	
int count_events(int silpar,int arrayno,int *eventsegscnt,dataptr dz)
{
	int exit_status;
	float *ibuf = dz->sampbuf[0];
	int ibufpos, last_buf_start, silstart = -1, silend;
	int silsegscnt, insil, k;
	int *silences, *events;
	int event_at_end = 1;

	/* COUNT (VALID-LENGTH) SILENCES IN DATA */

	fprintf(stdout,"INFO: Counting silences between events.\n");
	fflush(stdout);
	insil = 0;
	silsegscnt = 0;
	while(dz->samps_left > 0) {
		if((exit_status = read_samps(ibuf,dz))<0) {
			sprintf(errstr,"Failed to read data from sndfile.\n");
			return(DATA_ERROR);
		}
		for(ibufpos=0;ibufpos<dz->ssampsread;ibufpos++) {
			if(ibuf[ibufpos] == 0.0)
				insil++;
			else {
				if(insil) {
					if(insil >= dz->iparam[silpar]) {
						silsegscnt++;
					}
				}
				insil = 0;
			}
		}
	}
	if(insil) 
		silsegscnt++;
	if(silsegscnt == 0) {
		sprintf(errstr,"NO SILENCES FOUND IN FILE.\n");
		return(MEMORY_ERROR);
	}
	silsegscnt++;			/* in case less silences than sounds */
	if((sndseekEx(dz->ifd[0],0,0)<0)){
        sprintf(errstr,"sndseek() failed\n");
        return SYSTEM_ERROR;
    }
	reset_filedata_counters(dz);
	last_buf_start = 0;
	/* STORE LOCATION OF STARTS AND ENDS OF SILENCE BLOCKS */

	if((dz->lparray[arrayno] = (int *)malloc((silsegscnt * 2) * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY TO STORE SILENCE LOCATIONS.\n");
		return(MEMORY_ERROR);
	}
	silsegscnt = 0;
	insil = 0;
	silences = dz->lparray[arrayno];
	fprintf(stdout,"INFO: Marking silence-separated Events.\n");
	fflush(stdout);
	while(dz->samps_left > 0) {
		if((exit_status = read_samps(ibuf,dz))<0) {
			sprintf(errstr,"Failed to read data from sndfile.\n");
			return(DATA_ERROR);
		}
		for(ibufpos=0;ibufpos<dz->ssampsread;ibufpos++) {
			if(ibuf[ibufpos] == 0.0) {
				if(insil == 0)
					silstart = last_buf_start + ibufpos;
				insil++;
			} else {
				if(insil) {
					if(insil >= dz->iparam[silpar]) {
						silend = last_buf_start + ibufpos;
						silences[silsegscnt++] = silstart;
						silences[silsegscnt++] = silend;
					}
				}
				insil = 0;
			}
		}
		last_buf_start += dz->ssampsread;
	}
	if(silstart < 0) {
		sprintf(errstr,"No signal found in sndfile.\n");
		return(DATA_ERROR);
	}
	if(insil) {		/* if silence at end */
		silend = dz->insams[0];
		silences[silsegscnt++] = silstart;
		silences[silsegscnt++] = silend;
		event_at_end = 0;
	}
	events = dz->lparray[arrayno];	/* we write over the original data store */
	if(silences[0] == 0) {					/* If first silence is at time zero */
		for(k=1;k<silsegscnt;k++)			/* map gaps between silences backwards into (=) events ( losing first silence in process) */
			events[k-1] = silences[k];		/* --X--X-- to XX-XX- */
		if(event_at_end)					/* if last silence does not end at file end, it starts a final event */
			events[silsegscnt-1] = dz->insams[0];	/* so add the end of file as end of final event, and silsegcnt stays same. --X--X-- thro XX-XX- to  XX-XX-XX  */
		else								/* otherwise final silent end, does NOT start a new event, so delete last entry */			
			silsegscnt -= 2;				/* --X--X-- thro XX-XX- to XX-XX: subtract a start-end pair from count */ 

	} else {								/* first event at time zero */	
		for(k=silsegscnt;k>0;k--)			/* map gaps between silences forwards into (=) events (eventually using extra address space assigned at end) */
			events[k] = silences[k-1];		
		events[0] = 0;						/* Add start of file as start of firsst event. --X--X-- to XX-XX-XX- */
													
		if(event_at_end) {					/* if last silence does not end at file end, it starts a final event */
			events[silsegscnt+1] = dz->insams[0];	/* so add the end of file as end of final event */
			silsegscnt += 2;				/* --X--X-- thro XX-XX-XX- to  XX-XX-XX-XX add a start-end pair to count */
		} else {
			/* last silence is at end of file, and does not start an event .... --X--X-- thro XX-XX-XX- to XX-XX-XX  silsegscnt stays as is, last entry forgotten */	
		}
	}
	*eventsegscnt = silsegscnt;
	fprintf(stdout,"INFO: Found %d events.\n",silsegscnt/2);
	fflush(stdout);
	return FINISHED;
}

/*************************** STEPSPREAD ****************************/

int stepspread(dataptr dz)
{
	int exit_status, outchans = dz->iparam[0], n, m;
	int eventsegscnt, buf_start, ibufpos, obufpos, eventstart, eventend;
	int *events;
	double *spreads, *levels;
	float *ibuf = dz->sampbuf[0], *obuf = dz->sampbuf[1];
	double time = 0.0, srate = (double)dz->infile->srate;

	if((exit_status = count_events(5,0,&eventsegscnt,dz))<0)
		return(exit_status);
	events = dz->lparray[0];

	for(n=1,m=2;n<eventsegscnt;n++,m+=2)
		events[n] = events[m];
	eventsegscnt /= 2;	// We only need the starts of the events

	if((dz->parray[0] = (double *)malloc(eventsegscnt * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY TO STORE EVENT SPREAD VALS.\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[1] = (double *)malloc(outchans * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY TO STORE OUT-LEVELS.\n");
		return(MEMORY_ERROR);
	}
	spreads = dz->parray[0];
	levels = dz->parray[1];
	if(dz->vflag[0]) {
		m = 1;
		for(n=0;n < eventsegscnt;n++) {
			spreads[n] = min(m,dz->iparam[2]);
			spreads[n] = min(spreads[n],outchans);
			m+=2;
		}
	} else {
		n = 0;
		time = (double)(events[n])/srate;
		if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
			return(exit_status);
		spreads[n++] = min(dz->param[2],outchans);
		if(dz->brksize[2]) {
			while(n < eventsegscnt) {
				time = (double)(events[n])/srate;
				if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
					return(exit_status);
				spreads[n] = min(dz->param[2],outchans);
				n++;
			}
		} else {
			while(n < eventsegscnt)
				spreads[n++] = spreads[0];
		}
	}
	fflush(stdout);
	if((sndseekEx(dz->ifd[0],0,0)<0)){
        sprintf(errstr,"sndseek() failed\n");
        return SYSTEM_ERROR;
    }
	reset_filedata_counters(dz);

	events[0] = 0;		// We start at the start of the file
	buf_start = 0;
	if((exit_status = read_samps(ibuf,dz))<0) {
		sprintf(errstr,"Failed to read data from sndfile.\n");
		return(DATA_ERROR);
	}
	memset((char *)obuf,0,dz->buflen * outchans * sizeof(float));
	ibufpos = 0;
	obufpos = 0;
	n = 1;

	while(n <= eventsegscnt) {
		eventstart = events[n-1] - buf_start;
		if(n == eventsegscnt) {
			eventend = dz->insams[0] - buf_start;
		} else 
			eventend   = events[n] - buf_start;
		time     = (double)eventstart/srate;
		if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
			return(exit_status);
		dz->iparam[1] = (int)round(dz->param[1]) - 1;		// centre is an integer numbered from 0 to N-1
		dz->param[2] = spreads[n-1];		//  precalcd
		spread_set(dz);			
		while(ibufpos < eventstart) {
			if(ibufpos >= dz->buflen) {
				if((exit_status = write_samps(obuf,dz->buflen * outchans,dz))<0)
					return(exit_status);
				memset((char *)obuf,0,dz->buflen * outchans * sizeof(float));
				buf_start  += dz->buflen;
				eventstart -= dz->buflen;
				eventend   -= dz->buflen;
				ibufpos = 0;
				obufpos = 0;
				if((exit_status = read_samps(ibuf,dz))<0) {
					sprintf(errstr,"Failed to read data from sndfile.\n");
					return(DATA_ERROR);
				}
			}
			for(m=0;m<outchans;m++) 
				obuf[obufpos + m] = (float)0.0;
			ibufpos++;
			obufpos += outchans;
		}
		while(ibufpos < eventend) {
			if(ibufpos >= dz->buflen) {
				if((exit_status = write_samps(obuf,dz->buflen * outchans,dz))<0)
					return(exit_status);
				memset((char *)obuf,0,dz->buflen * outchans * sizeof(float));
				buf_start += dz->buflen;
				eventend  -= dz->buflen;
				ibufpos = 0;
				obufpos = 0;
				if((exit_status = read_samps(ibuf,dz))<0) {
					sprintf(errstr,"Failed to read data from sndfile.\n");
					return(DATA_ERROR);
				}
			}
			for(m=0;m<outchans;m++) 
				obuf[obufpos + m] = (float)(ibuf[ibufpos] * levels[m]);
			ibufpos++;
			obufpos += outchans;
		}
		n++;
	}
	if(ibufpos > 0) {
		if((exit_status = write_samps(obuf,ibufpos * outchans,dz))<0)
			return(exit_status);
	}
	return FINISHED;
}

/*************************** SPREAD_SET ****************************/

void spread_set(dataptr dz)
{
	int outchans   = dz->iparam[0];
	int centre     = dz->iparam[1];
	double spread  = dz->param[2];
	double depth   = min(dz->param[3],(double)outchans/2.0);
	double rolloff = dz->param[4];
	double *levels = dz->parray[1];
	int stepped = dz->vflag[0];
	int K, N, j, maxchan, minchan, chan, ochan;
	double range, fraclevel, maxlevel, hole;
	
	if(stepped)
		depth  = dz->iparam[3];
	N = (int)floor(spread); 

	/* Initialise all levels to zero */

	for(chan = 0;chan < outchans;chan++)
		levels[chan] = 0.0;


	/* Establish maxlevel (determined by rolloff) */

	range = 1.0 - (1.0/(double)N);
	maxlevel = 1.0 - (rolloff * range);
	
	/* Do channels with full level first */

	K = N/2;
	if(EVEN(N))
		K--;
	minchan = centre - K;
	maxchan = centre + K;
	for(chan = minchan;chan <=maxchan;chan++) {
		ochan = chan;
		while(ochan < 0)
			ochan += outchans;
		while(ochan >= outchans)
			ochan -= outchans;
		levels[ochan] = maxlevel;
	}

	/* IF hole in middle */

	if((hole = (spread/2.0) - depth) > 0) {					

	/*	Zero channels in centre of hole */

		K = (int)floor(hole);
		for(j =0;j < K;j++) {
			ochan = centre + j;
			while(ochan >= outchans)
				ochan -= outchans;
			levels[ochan] = 0;
			ochan = centre - j;
			while(ochan < 0)
				ochan += outchans;
			levels[ochan] = 0;
		}

		/*	Reduce levels adjacent to hole, if ness */

		if(!stepped) {
			fraclevel = hole - (double)K;
			fraclevel = 1.0 - fraclevel;
			fraclevel *= maxlevel;
			ochan = centre + j;
			while(ochan >= outchans)
				ochan -= outchans;
			levels[ochan] = fraclevel;
			ochan = centre - j;
			while(ochan < 0)
				ochan += outchans;
			levels[ochan] = fraclevel;
		}
	}
		/* Finally, Do fractional channels on leading edge of spread */

	if(stepped && (spread != outchans)) {
		return;
	}
	fraclevel = (spread - (double)N);
	if(ODD(N))
		fraclevel = fraclevel/2.0;
	else
		fraclevel = (fraclevel + 1.0)/2.0;
	fraclevel *= maxlevel;
	maxchan++;
	while(maxchan >= outchans)
		maxchan -= outchans;
	while(maxchan < 0)
		maxchan += outchans;
	minchan--;
	while(minchan >= outchans)
		minchan -= outchans;
	while(minchan < 0)
		minchan += outchans;
	levels[maxchan] = (float)(levels[maxchan] + fraclevel);
	levels[minchan] = (float)(levels[minchan] + fraclevel);
}

/*************************** PANSPREAD ****************************/

int panspread(dataptr dz)
{
	int exit_status, outchans = dz->iparam[0], sectcnt, m, k;
	int buf_start, ibufpos, obufpos = 0;
	double *levels;
	int inchans = dz->infile->channels;
	float *ibuf = dz->sampbuf[0], *obuf = dz->sampbuf[1];
	double time = 0.0, srate = (double)dz->infile->srate;
	int obuflen = (dz->buflen/inchans) * outchans;
	double stereo_pos;
	double centre=0.0;
	int halfstage = outchans/2;
	int c_left, c_rite;
	double left_contrib_to_c_left, rite_contrib_to_c_left, left_contrib_to_c_rite, rite_contrib_to_c_rite, val;

	if((dz->parray[0] = (double *)malloc(outchans * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY TO STORE OUT-LEVELS.\n");
		return(MEMORY_ERROR);
	}
	levels = dz->parray[0];
	buf_start = 0;
	if((exit_status = read_samps(ibuf,dz))<0) {
		sprintf(errstr,"Failed to read data from sndfile.\n");
		return(DATA_ERROR);
	}
	memset((char *)obuf,0,obuflen * sizeof(float));
	sectcnt = 0;
	while(dz->ssampsread > 0) {
		for(ibufpos = 0, obufpos = 0;ibufpos < dz->ssampsread;ibufpos+=inchans,obufpos += outchans) {
			if(sectcnt % 256 == 0) {
				time = (double)sectcnt/srate;
				if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
					return(exit_status);
				spread_pan(&centre,dz);
			}
			if(dz->infile->channels == 2) {
				c_left = (int)floor(centre);
				c_rite = c_left + 1;
				if(c_rite >= outchans)
					c_rite -= outchans;
				for(k=1;k<halfstage;k++) {			//	All loudpseakers to left of central pair get left signal
					m = c_left - k;
					if(m < 0)
						m += outchans;
					obuf[obufpos + m] = (float)(ibuf[ibufpos] * levels[m]);
				}
				for(k=1;k<halfstage;k++) {			//	All loudpseakers to right of central pair get right signal
					m = c_rite + k;
					if(m >= outchans)
						m -= outchans;
					obuf[obufpos + m] = (float)(ibuf[ibufpos+1] * levels[m]);
				}									//	Central pair get a mix of left and right signals
				stereo_pos = centre - (double)c_left;
				left_contrib_to_c_left = (1.0 + (2.0 * stereo_pos))/2.0;
				left_contrib_to_c_left = min(1.0,left_contrib_to_c_left);
				rite_contrib_to_c_left = (1.0 - (2.0 * stereo_pos))/2.0;
				rite_contrib_to_c_left = max(0.0,rite_contrib_to_c_left);
				val = (ibuf[ibufpos] * left_contrib_to_c_left) + (ibuf[ibufpos+1] * rite_contrib_to_c_left);
				val *= levels[c_left];
				obuf[obufpos + c_left] = (float)val;

				left_contrib_to_c_rite = ((2.0 * stereo_pos) - 1.0)/2.0;
				left_contrib_to_c_rite = max(0.0,left_contrib_to_c_rite);
				rite_contrib_to_c_rite = (3.0 - (2.0 * stereo_pos))/2.0;
				rite_contrib_to_c_rite = min(1.0,rite_contrib_to_c_rite);
				val = (ibuf[ibufpos] * left_contrib_to_c_rite) + (ibuf[ibufpos+1] * rite_contrib_to_c_rite);
				val *= levels[c_rite];
				obuf[obufpos + c_rite] = (float)val;
			} else {
				for(m=0;m<outchans;m++) 
					obuf[obufpos + m] = (float)(ibuf[ibufpos] * levels[m]);
			}
			sectcnt++;
		}
		if((exit_status = write_samps(obuf,obufpos,dz))<0)
			return(exit_status);
		memset((char *)obuf,0,obuflen * sizeof(float));
		if((exit_status = read_samps(ibuf,dz))<0) {
			sprintf(errstr,"Failed to read data from sndfile.\n");
			return(DATA_ERROR);
		}
	}
	if(obufpos > 0) {
		if((exit_status = write_samps(obuf,obufpos,dz))<0)
			return(exit_status);
	}
	return FINISHED;
}

/*
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

void spread_pan(double *thiscentre,dataptr dz)
{
	int outchans   = dz->iparam[0];
	double centre  = dz->param[1];
	double spread  = min(dz->param[2],(double)outchans), halfspread = spread/2.0;
	double depth   = min(dz->param[3],(double)outchans/2.0);
	double rolloff = dz->param[4];
	double *levels = dz->parray[0];
	double spredej_left, spredej_right;
	int spredej_left_leftchan, spredej_right_leftchan, spredej_right_rightchan, k, j, ochan;
	double range, maxlevel, hole, stereopos_left, stereopos_right, relpos, temp, holecompensate;
	double holej_right, holej_left, zleft, zright, floor_zleft, floor_zright;
	double left_leftchan_level, right_rightchan_level, spredej_left_rightchan;
	double kk, jj, holing, mingap;

	if(depth <= 0.0)
		depth = FLTERR;
	
	if((halfspread = spread/2.0) == 0.0)
		halfspread = FLTERR;

	if((centre = centre - 1.0) < 0.0)
		centre += (double)outchans;

	for(ochan = 0;ochan < outchans;ochan++)
		levels[ochan] = 0.0;

	/* Establish maxlevel (determined by rolloff) */

	if(spread < 1.0)
		range = 0.0;
	else
		range = 1.0 - (1.0/(double)spread);
	maxlevel = 1.0 - (rolloff * range);

	// Set all channels fully within the spread to maxlevel

	spredej_left = centre - halfspread;
	while(spredej_left  < 0)
		spredej_left += (double)outchans;
	spredej_right = centre + halfspread;
	while(spredej_right >= outchans)
		spredej_right -= (double)outchans;
	spredej_left_leftchan  = (int)floor(spredej_left);
	if((spredej_left_rightchan  = spredej_left_leftchan + 1) >= outchans)
		spredej_left_rightchan = 0;
	spredej_right_leftchan = (int)floor(spredej_right);

	hole = spread - (depth * 2.0);

	if(spread >= outchans) {
		for(k = 0;k<outchans; k++)
			levels[k] = maxlevel;
		if(hole <= 0.0)
			return;
	} else if(spread > 1.0) {

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
	} else {

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

	halfspread = spread/2.0;

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
	/* !!!!!!!!!!!!! */
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

/*************************** READ_ANTIPHON_DATA ****************************/

int read_antiphon_data(char *str,dataptr dz)
{
	int exit_status;	
	dz->all_words = 0;
	if((exit_status = store_filename(str,dz))<0)
		return(exit_status);
	return FINISHED;
}

/*************************** CHECK_ANTIPHON_DATA ****************************/

int setup_antiphon_arrays(char *str,dataptr dz)
{
	char *p, *q, *antiphon0, *antiphon1 = NULL;
	int got_separator = 0, maxchan = 0, strcnt, j, n, k;
	int inchans = dz->infile->channels;
	int outchans = dz->iparam[0], distribution;
	int *antiphon_a, *antiphon_b;
	antiphon0 = str;
	p = str;
			/* 
			 * Expects astring like 'abc-def' 
			 * Looks for the separator '-' and replaces it be ENDOFSTR
			 * splittingbthe source string into 2 i.e. abc & def
			 * These then become antiphon0 and antiphon1
			 */
	while (*p != ENDOFSTR) {
		if(*p == '-') {
			got_separator = 1;
			if(p-str == 0) {
				sprintf(errstr,"Invalid Antiphon Data : no channels before separator\n");
				return(DATA_ERROR);
			}
			antiphon1 = p+1;
			if(*antiphon1 == ENDOFSTR) {
				sprintf(errstr,"Invalid Antiphon Data : no channels after separator\n");
				return(DATA_ERROR);
			}
			*p = ENDOFSTR;
		} else if(!isalpha(*p)) {
			sprintf(errstr,"Invalid Antiphon Data : invalid character (%c)\n",*p);
			return(DATA_ERROR);
		}
		p++;
	}
	if(!got_separator) {
		sprintf(errstr,"Invalid Antiphon Data : no separator character (-)\n");
		return(DATA_ERROR);
	}
	p = antiphon0;
			/* 
			 * Check for character duplication (not allowed)
			 * in antiphon0 and antiphon1
			 */
	while(*p != ENDOFSTR) {
		q = p + 1;
		if(*q == ENDOFSTR)
			break;
		while(*q != ENDOFSTR) {
			if(*p == *q) {
				sprintf(errstr,"Invalid Antiphon Data : repeated channel (%c)\n",*p);
				return(DATA_ERROR);
			}
			q++;
		}
		p++;
	}
	p = antiphon1;
	while(*p != ENDOFSTR) {
		q = p + 1;
		if(*q == ENDOFSTR)
			break;
		while(*q != ENDOFSTR) {
			if(*p == *q) {
				sprintf(errstr,"Invalid Antiphon Data : repeated channel (%c)\n",*p);
				return(DATA_ERROR);
			}
			q++;
		}
		p++;
	}
	p = antiphon0;
	strcnt = 0;
			/* 
			 * Find the maximum channel referred to in "antiphon0-ENDOFSTR-antiphon1-ENDOFSTR"
			 * Counts the ENDOFSTR markers, existing at the 2nd.
			 */

	while(strcnt < 2) {
		switch(*p) {
		case('a'):	maxchan = max(1,maxchan);	break;
		case('b'):	maxchan = max(2,maxchan);	break;
		case('c'):	maxchan = max(3,maxchan);	break;
		case('d'):	maxchan = max(4,maxchan);	break;
		case('e'):	maxchan = max(5,maxchan);	break;
		case('f'):	maxchan = max(6,maxchan);	break;
		case('g'):	maxchan = max(7,maxchan);	break;
		case('h'):	maxchan = max(8,maxchan);	break;
		case('i'):	maxchan = max(9,maxchan);	break;
		case('j'):	maxchan = max(10,maxchan);	break;
		case('k'):	maxchan = max(11,maxchan);	break;
		case('l'):	maxchan = max(12,maxchan);	break;
		case('m'):	maxchan = max(13,maxchan);	break;
		case('n'):	maxchan = max(14,maxchan);	break;	
		case('o'):	maxchan = max(15,maxchan);	break;
		case('p'):	maxchan = max(16,maxchan);	break;
		case(ENDOFSTR):	
			strcnt++;
			break;
		default:
			sprintf(errstr,"Invalid Antiphon Character: (a-p, lower case, only)\n");
			return(DATA_ERROR);
		}
		p++;
	}
	if(maxchan > outchans) {
		sprintf(errstr,"Maximum channel in antiphon data (%d) incompatible with outchannel count (%d)\n",maxchan,outchans);
		return(DATA_ERROR);
	}
			/* 
			 * Finds length of each antiphon string.
			 */
	dz->antiphlen0 = strlen(antiphon0);
	dz->antiphlen1 = strlen(antiphon1);
			/* 
			 * If more than 1 channel in input (say N), and K output channels
			 * Checks that K = N or 2N or 3N etc
			 * (i.e. integral number of output channels per input channel)
			 */
	if(inchans > 1) {
		if((dz->antiphlen0/inchans) * inchans != dz->antiphlen0) {
			sprintf(errstr,"1st antiphon channel count not an integral multiple of inchans (%d).\n",inchans);
			return(DATA_ERROR);
		}
		if((dz->antiphlen1/inchans) * inchans != dz->antiphlen1) {
			sprintf(errstr,"2nd antiphon channel count not an integral multiple of inchans (%d).\n",inchans);
			return(DATA_ERROR);
		}
	}
			/* 
			 * Generates storage for the real antiphon arrays, TWICE as long as the antiphon strings.
			 * These will store the end  and start of each antiphon event (therefore 2 er event)
			 */

	if((dz->lparray[0] = (int *)malloc((dz->antiphlen0 * 2) * sizeof(int)))==NULL) {
		sprintf(errstr,"Insufficient memory for 1st antiphon data.\n");
		return(DATA_ERROR);
	}
	if((dz->lparray[1] = (int *)malloc((dz->antiphlen1 * 2) * sizeof(int)))==NULL) {
		sprintf(errstr,"Insufficient memory for 2nd antiphon data.\n");
		return(DATA_ERROR);
	}
	antiphon_a = dz->lparray[0];
	antiphon_b = dz->lparray[1];
			/* 
			 * For N input channels, and K output chans,
			 * each inchan is distributed to K/N output channels.
			 * (K/N is an integer: checked above)
			 */
	distribution = dz->antiphlen0/inchans;
	j = 0;
			/* 
			 * Associate each inchannel
			 * with K/N output channels of antiphon0
			 */
	p = antiphon0;
	for(n=0;n<inchans;n++) {
		for(k=0;k < distribution;k++) {
			antiphon_a[j++] = n;
			antiphon_a[j++] = *p - ASCTOINT;
			p++;
		}
	}
			/* 
			 * Similar operation for antiphon1
			 */
	distribution = dz->antiphlen1/inchans;
	j = 0;
	p = antiphon1;
	for(n=0;n<inchans;n++) {
		for(k=0;k < distribution;k++) {
			antiphon_b[j++] = n;
			antiphon_b[j++] = *p - ASCTOINT;
			p++;
		}
	}
	return FINISHED;
}

/*************************** GENERATE_ANTIPHONAL_EVENTS_ARRAY ****************************/

int generate_antiphonal_events_array(dataptr dz)
{
	int exit_status, inchans = dz->infile->channels;
	int maxsamps = dz->insams[0], n, cnt, sampcnt, eventscnt, start;
	double maxdur, time, srate = (double)dz->infile->srate;
	int *events;
	int splicelen = (int)round(dz->param[3] * MS_TO_SECS * srate) * inchans;
	if(dz->infilecnt > 1) {
		for(n=1;n<dz->infilecnt;n++)
			maxsamps = max(maxsamps,dz->insams[n]);
	}
	maxdur = (double)(maxsamps/inchans)/srate;

	/* Count how many antiphonal 'events' there will be */
	
	if(dz->brksize[1]) {		// eventdur
		time = 0.0;
		cnt = 0;
		while(time < maxdur) {
			cnt++;
			if((exit_status = read_value_from_brktable(time,1,dz))<0)
				return(exit_status);
			time += dz->param[1];
		}
	} else {
		cnt = (int)floor(maxdur/dz->param[1]);
	}
	if((dz->lparray[2] = (int *)malloc(cnt * 2 * sizeof(int))) == NULL) {
		sprintf(errstr,"Insufficient memory for events array.\n");
		return MEMORY_ERROR;
	}
	events = dz->lparray[2];

	/* Store timing of every antiphonal 'event' end+startofnext (i.e. bbccdd) */
	
	time = 0.0;
	cnt = 0;
	while(time < maxdur) {
		if(dz->brksize[1]) {
			if((exit_status = read_value_from_brktable(time,1,dz))<0)
				return(exit_status);
		}
		time += dz->param[1];
		if(time < maxdur) {
			sampcnt = (int)round(time * srate) * inchans;
			events[cnt++] = sampcnt;
			events[cnt++] = sampcnt;
		}
	}
	eventscnt = cnt;
	cnt = 0;
	if(dz->brksize[2] == 0)
		dz->iparam[2] = (int)round(dz->param[2] * srate) * inchans;
	while(cnt < eventscnt) {			// eventgaps
		time = (double)(events[cnt]/inchans)/srate;
		if(dz->brksize[2]) {
			if((exit_status = read_value_from_brktable(time,2,dz))<0)
				return(exit_status);
			dz->iparam[2] = (int)round(dz->param[2] * srate) * inchans;
		}
		events[cnt] -= dz->iparam[2];	//	subtract gap value from event end
		cnt += 2;
	}
	start = 0;
	cnt = 0;
	while(cnt < eventscnt) {			// check no events too short
		if(events[cnt++] - start <= splicelen) {
			sprintf(errstr,"Events too short for splices at %lf\n",(double)(start/inchans)/srate);
			return(DATA_ERROR);
		}
		start = events[cnt++];
	}
	dz->antievents = eventscnt;
	return FINISHED;
}

/*************************** ANTIPHONAL_EVENTS ****************************/

int antiphonal_events(dataptr dz)
{
	int exit_status;
	int inchans = dz->infile->channels, outchans = dz->iparam[0], arrayno, ichan, ochan;
	int obuflen = (dz->buflen/inchans) * outchans, silsegscnt, buf_start;
	int segscnt, segstart, segend, arraysize, outwrite, ibufpos, obufpos = 0, abs_samp_pos;
	int *events, *array, k, j, grpcnt;
	float *ibuf = dz->sampbuf[0], *obuf = dz->sampbuf[1];
	if((exit_status = count_events(1,2,&silsegscnt,dz))<0)
		return(exit_status);

	events = dz->lparray[2];
	if(dz->infile->channels > 1) {
		for(k = 0,j = 1;k < silsegscnt; k+=2,j+=2) {
			events[k] = (events[k]/inchans) * inchans;					// Event start rounded down to chan-group boundary (if ness)
			if((grpcnt = events[j]/inchans) * inchans != events[j]) {
				grpcnt++;												// Event end rounded up to chan-group boundary (if ness)
				events[j] = grpcnt * inchans;
			}
		}
	}
	k = silsegscnt/2; 
	if(k < 2) {
		sprintf(errstr,"Only one event found. No panning will take place. Try adjusting the length of silences you are using.\n");
		return(DATA_ERROR);
	} else {
		fprintf(stdout,"INFO: %d distinct events found.\n",k);
		fflush(stdout);
	}
	/* PAN, SWITCHING ANTIPHONALLY FOR EACH PEAK */

	fprintf(stdout,"INFO: Panning sound.\n");
	fflush(stdout);
	if((sndseekEx(dz->ifd[0],0,0)<0)){
        sprintf(errstr,"sndseek() failed\n");
        return SYSTEM_ERROR;
    }
	reset_filedata_counters(dz);

	buf_start = 0;
	segscnt = 0;
	segstart = events[segscnt++];
	segend   = events[segscnt++];
	arrayno = 0;
	array = dz->lparray[0];
	arraysize = dz->antiphlen0;
	while(dz->samps_left) {
		while(dz->total_samps_read <= segstart) {
			outwrite = (dz->ssampsread/inchans) * outchans;
			if((exit_status = write_samps(obuf,outwrite,dz))<0)
				return(exit_status);
			memset((char *)obuf,0,obuflen * sizeof(float));
			buf_start = dz->total_samps_read;
			if((exit_status = read_samps(ibuf,dz))<0) {
				sprintf(errstr,"Failed to read data from sndfile.\n");
				return(DATA_ERROR);
			}
		}
		ibufpos = segstart - buf_start;
		abs_samp_pos = segstart;
		while(abs_samp_pos < segend) {
			obufpos = (ibufpos/inchans) * outchans;
			for(ichan = 0,ochan = 1; ichan < arraysize;ichan+=2, ochan+= 2)
				obuf[obufpos + array[ochan]] = (float)(obuf[obufpos + array[ochan]] + ibuf[ibufpos + array[ichan]]);	// out chan ochan gets in chan ichan added in
			ibufpos += inchans;
			obufpos += outchans;
			abs_samp_pos += inchans;
			if(ibufpos >= dz->ssampsread) {
				outwrite = (dz->ssampsread/inchans) * outchans;
				if((exit_status = write_samps(obuf,outwrite,dz))<0)
					return(exit_status);
				memset((char *)obuf,0,obuflen * sizeof(float));
				buf_start = dz->total_samps_read;
				if((exit_status = read_samps(ibuf,dz))<0) {
					sprintf(errstr,"Failed to read data from sndfile.\n");
					return(DATA_ERROR);
				}
				ibufpos = 0;
				obufpos = 0;
			}
		}
		if(segscnt >= silsegscnt)
			break;
		if(arrayno == 0) {
			arrayno = 1;
			array = dz->lparray[1];
			arraysize = dz->antiphlen1;
		} else {
			arrayno = 0;
			array = dz->lparray[0];
			arraysize = dz->antiphlen0;
		}
		segstart = events[segscnt++];
		segend   = events[segscnt++];
	}
	if(obufpos > 0) {
		if((exit_status = write_samps(obuf,obufpos,dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}

/*************************** ANTIPHONY ****************************/

int antiphony(dataptr dz)
{
	int exit_status;
	double srate = (double)dz->infile->srate;
	int inchans = dz->infile->channels, outchans = dz->iparam[0], antiphonstate, thisfile, nextfile;
	int splicelen = (int)round(dz->param[3] * MS_TO_SECS * srate) * inchans;		//	splice length is measured in input-file frame
	int obuflen = (dz->buflen/inchans) * outchans, n, samps_read;
	int *events = dz->lparray[2], *array0, *array1, arraysize0, arraysize1;
	float *obuf = dz->sampbuf[dz->infilecnt], *ibuf1, *ibuf2;
	int abs_samp_pos, ibufpos, obufpos, cnt;
	int lasteventend, lasteventdnsplicend, thisevent, upsplicend, ichan, ochan, outwrite;
	double dnsplicval, upsplicval, splicincr;
	int totalsamps_to_process;
	
	splicincr = 1.0/(double)(splicelen/inchans);

	totalsamps_to_process = 0;
	dz->ssampsread = 0;
	for(n=0;n< dz->infilecnt;n++) {
		memset(dz->sampbuf[n],0,dz->buflen * sizeof(float));	//	If nothing is read, because data in this file is finished, buffer will have zeros zeros
		if((samps_read = fgetfbufEx(dz->sampbuf[n],dz->buflen,dz->ifd[n],0)) < 0) {
			sprintf(errstr,"Sound read error.\n");
			return(SYSTEM_ERROR);
		}
		if(samps_read > dz->ssampsread)
			dz->ssampsread = samps_read;
		if(dz->insams[n] > totalsamps_to_process)
			totalsamps_to_process = dz->insams[n];
	}
	memset(obuf,0,obuflen * sizeof(float));

	antiphonstate = 0;				//	Establish 1st 2 antiphon states (need both, as antiphons may overlap during crossfade).
	array0 = dz->lparray[0];
	arraysize0 = dz->antiphlen0;
	array1 = dz->lparray[1];
	arraysize1 = dz->antiphlen1;

	thisfile = 0;					//	Establish 1st 2 files to use  (need both, if infilecnt > 1, as antiphons may overlap during crossfade).
	ibuf1 = dz->sampbuf[thisfile];
	nextfile = (thisfile + 1) % dz->infilecnt;
	ibuf2 = dz->sampbuf[nextfile];

	abs_samp_pos = 0;
	ibufpos = 0;
	obufpos = 0;
	cnt = 0;

	lasteventend = events[cnt++];
	lasteventdnsplicend = lasteventend + splicelen;
	thisevent = events[cnt++];

	while(cnt < dz->antievents) {
		upsplicend = thisevent + splicelen;	//	abs samps end of upslice in infile(s)

		while(abs_samp_pos < lasteventend) {
			for(ichan = 0,ochan = 1; ichan < arraysize0;ichan+=2, ochan+= 2)
				obuf[obufpos + array0[ochan]] = (float)(obuf[obufpos + array0[ochan]] + ibuf1[ibufpos + array0[ichan]]);	// oout chan ochan gets in chan ichan added in
			ibufpos += inchans;
			obufpos += outchans;
			abs_samp_pos += inchans;
			if(ibufpos >= dz->ssampsread) {
				outwrite = (dz->ssampsread/inchans) * outchans;
				if((exit_status = write_samps(obuf,outwrite,dz))<0)
					return(exit_status);
				memset((char *)obuf,0,obuflen * sizeof(float));
				dz->ssampsread = 0;
				for(n=0;n< dz->infilecnt;n++) {
					memset(dz->sampbuf[n],0,dz->buflen * sizeof(float));
					if((samps_read = fgetfbufEx(dz->sampbuf[n],dz->buflen,dz->ifd[n],0)) < 0) {
						sprintf(errstr,"Sound read error.\n");
						return(SYSTEM_ERROR);
					}			//	If nothing is read, because data in this file is finished, buffer is still full of zeros
					if(samps_read > dz->ssampsread)
						dz->ssampsread = samps_read;	// Continue until longest file is empty
				}
				if(dz->ssampsread == 0) {
					fprintf(stdout,"WARNING: Termination before all events executed.\n");
					fflush(stdout);
					break;
				}
				ibufpos = 0;
				obufpos = 0;
			}
		}
		dnsplicval = 1.0;
		upsplicval = 0.0;
		while(abs_samp_pos < upsplicend) {
			if(abs_samp_pos < lasteventdnsplicend) {
				dnsplicval = max(0.0,dnsplicval - splicincr);
				for(ichan = 0,ochan = 1; ichan < arraysize0;ichan+=2, ochan+= 2)
					obuf[obufpos + array0[ochan]] = (float)(obuf[obufpos + array0[ochan]] + ibuf1[ibufpos + array0[ichan]] * dnsplicval);
			}
			if(abs_samp_pos >= thisevent) {
				upsplicval = min(1.0,upsplicval + splicincr);
				for(ichan = 0,ochan = 1; ichan < arraysize1;ichan+=2, ochan+= 2)
					obuf[obufpos + array1[ochan]] = (float)(obuf[obufpos + array1[ochan]] + ibuf2[ibufpos + array1[ichan]] * upsplicval);
			}
			ibufpos += inchans;
			obufpos += outchans;
			abs_samp_pos += inchans;
			if(ibufpos >= dz->ssampsread) {
				outwrite = (dz->ssampsread/inchans) * outchans;
				if((exit_status = write_samps(obuf,outwrite,dz))<0)
					return(exit_status);
				memset((char *)obuf,0,obuflen * sizeof(float));
				dz->ssampsread = 0;
				for(n=0;n< dz->infilecnt;n++) {
					memset(dz->sampbuf[n],0,dz->buflen * sizeof(float));
					if((samps_read = fgetfbufEx(dz->sampbuf[n],dz->buflen,dz->ifd[n],0)) < 0) {
						sprintf(errstr,"Sound read error.\n");
						return(SYSTEM_ERROR);
					}			//	If nothing is read, because data in this file is finished, buffer is still full of zeros
					if(samps_read > dz->ssampsread)
						dz->ssampsread = samps_read;
				}
				if(dz->ssampsread == 0) {
					fprintf(stdout,"WARNING: Termination before all events executed.\n");
					fflush(stdout);
					break;
				}
				ibufpos = 0;
				obufpos = 0;
			}
		}
		if(antiphonstate == 0) {			//	swap antiphon arrays
			array0 = dz->lparray[1];
			arraysize0 = dz->antiphlen1;
			array1 = dz->lparray[0];
			arraysize1 = dz->antiphlen0;
			antiphonstate = 1;
		} else {
			array0 = dz->lparray[0];
			arraysize0 = dz->antiphlen0;
			array1 = dz->lparray[1];
			arraysize1 = dz->antiphlen1;
			antiphonstate = 0;
		}
		thisfile = nextfile;				//	move to next infile, if more than one
		ibuf1 = dz->sampbuf[thisfile];
		nextfile = (thisfile + 1) % dz->infilecnt;
		ibuf2 = dz->sampbuf[nextfile];

		lasteventend = events[cnt++];					//	abs sampcnt end of lastevent in infile(s)
		lasteventdnsplicend = lasteventend + splicelen;	//	abs sampcnt end of splice on end of lastevent in infile(s)
		thisevent = events[cnt++];
	}
	lasteventend = totalsamps_to_process;
	while(abs_samp_pos < lasteventend) {				//  Do end of last event
		for(ichan = 0,ochan = 1; ichan < arraysize0;ichan+=2, ochan+= 2)
			obuf[obufpos + array0[ochan]] = (float)(obuf[obufpos + array0[ochan]] + ibuf1[ibufpos + array0[ichan]]);	// oout chan ochan gets in chan ichan added in
		ibufpos += inchans;
		obufpos += outchans;
		abs_samp_pos += inchans;
		if(ibufpos >= dz->ssampsread) {
			outwrite = (dz->ssampsread/inchans) * outchans;
			if((exit_status = write_samps(obuf,outwrite,dz))<0)
				return(exit_status);
			memset((char *)obuf,0,obuflen * sizeof(float));
			dz->ssampsread = 0;
			for(n=0;n< dz->infilecnt;n++) {
				memset(dz->sampbuf[n],0,dz->buflen * sizeof(float));
				if((samps_read = fgetfbufEx(dz->sampbuf[n],dz->buflen,dz->ifd[n],0)) < 0) {
					sprintf(errstr,"Sound read error.\n");
					return(SYSTEM_ERROR);
				}			//	If nothing is read, because data in this file is finished, buffer is still full of zeros
				if(samps_read > dz->ssampsread)
					dz->ssampsread = samps_read;	// Continue until longest file is empty
			}
			if(dz->ssampsread == 0) {
				fprintf(stdout,"WARNING: Possible termination before all events executed.\n");
				fflush(stdout);
				break;
			}
			ibufpos = 0;
			obufpos = 0;
		}
	}
	if(obufpos > 0) {
		if((exit_status = write_samps(obuf,obufpos,dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}

/*************************** CROSSPAN ****************************/

int crosspan(dataptr dz)
{
	int exit_status, chans = dz->infile->channels, n, m, typ = -1;
	int bufpos, eventcnt, lasteventcnt, abs_samp_cnt, thisevent, nextevent, lastmapcnt, mapcnt;
	int timestep;
	double timefrac, span, chandiv = 1.0/(double)chans, rolloff = dz->param[0], centrelevel;
	float *ibuf = dz->sampbuf[0], *obuf = dz->sampbuf[1];
	int *events = dz->lparray[0], *maps = dz->lparray[1], *map, *thismap, *nextmap;
	if((exit_status = read_samps(ibuf,dz))<0) {
		sprintf(errstr,"Failed to read data from sndfile.\n");
		return(DATA_ERROR);
	}
	memset((char *)obuf,0,dz->buflen * sizeof(float));
	fprintf(stdout,"INFO: Panning the sound\n");
	fflush(stdout);
	centrelevel = (1.0 - chandiv) * (1.0 - rolloff);
	centrelevel += chandiv;
	bufpos = 0;
	abs_samp_cnt = 0;
	lasteventcnt = 0;
	eventcnt = 1;
	lastmapcnt = 0;
	mapcnt = chans;
	while(eventcnt < dz->itemcnt) {
		thisevent = events[lasteventcnt];
		nextevent = events[eventcnt];
		thismap = maps + lastmapcnt;
		nextmap = maps + mapcnt;
		timestep = nextevent - thisevent;
		if(thismap[0] < 0) {
			if(nextmap[0] < 0)
				typ = ATCENTRE;		//	Both maps put all data at centre
			else					
				typ = FROMCENTRE;	//	Moves from centre to edge
		} else {
			if(nextmap[0] < 0)
				typ = TOCENTRE;		//	Moves from edge to centre
			else {
				for(n=0;n<chans;n++) {
					if(thismap[n] != nextmap[n]) {
						typ = CROSSCENTRE;	//	Moves from edge, thro centre, to edge
						break;
					}
				}
				if(n == chans)
					typ = ATEDGE;	//	At edge, and does not move
			}
		}
		while(abs_samp_cnt < nextevent) {
			if(bufpos >= dz->ssampsread) {
				if((exit_status = write_samps(obuf,dz->ssampsread,dz))<0)
					return(exit_status);
				memset((char *)obuf,0,dz->buflen * sizeof(float));
				dz->ssampsread = 0;
				if((exit_status = read_samps(ibuf,dz))<0) {
					sprintf(errstr,"Failed to read data from sndfile.\n");
					return(DATA_ERROR);
				}
				if(dz->ssampsread == 0)
					break;
				bufpos = 0;
			}
			timefrac = (double)(abs_samp_cnt - thisevent)/(double)timestep;
			switch(typ) {
			case(ATCENTRE):									//	Both maps put all data at centre
				for(n=0;n<chans;n++) {						//	Ordering of channels is irrelevant,
					for(m=0;m<chans;m++)					//	as all inchans go to all outchans
						obuf[bufpos+n] = (float)(obuf[bufpos+n] + ibuf[bufpos+m]);
					obuf[bufpos+n] = (float)(obuf[bufpos+n] * centrelevel);
				}
				break;
			case(FROMCENTRE):
				span = 1.0 - timefrac;
				map  = nextmap;
				do_span(map,span,ibuf,obuf,bufpos,rolloff,chans);
				break;
			case(TOCENTRE):
				span = timefrac;
				map  = thismap;
				do_span(map,span,ibuf,obuf,bufpos,rolloff,chans);
				break;
			case(CROSSCENTRE):
				if(timefrac <= 0.5) {						// Moves to centre
					map = thismap;
					span = timefrac * 2.0;
				} else {									//	Then away from centre
					map = nextmap;
					span = (1.0 - timefrac) * 2.0;
				}
				do_span(map,span,ibuf,obuf,bufpos,rolloff,chans);
				break;
			case(ATEDGE):									//	Mapping not changing.
				for(n=0;n<chans;n++)						//	Each outchan mapped from single inchan.
					obuf[bufpos+n] = ibuf[bufpos+thismap[n]];
				break;										//	No levelscaling required.
			default:
				sprintf(errstr,"Bug: Panning type not assigned.\n");
				return(PROGRAM_ERROR);
			}
			bufpos += chans;
			abs_samp_cnt += chans;
		}
		lasteventcnt = eventcnt;
		eventcnt++;
		lastmapcnt += chans;
		mapcnt += chans;
	}
	if(bufpos > 0) {
		if((exit_status = write_samps(obuf,bufpos,dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}

/*************************** DO_SPAN ****************************/

void do_span(int *map,double span,float *ibuf,float *obuf,int bufpos,double rolloff,int chans)
{
	int n, m, outchan;
	double chanspan = span * (double)chans;	//	number of channels spanned
	double halfspan = chanspan/2.0;			//	half the number of channels spanned
	double maxchan, minchan;
	int maxfullchan, minfullchan, trumaxfullchan, truminfullchan, truchan;
	double partial_level;					//	level scaling on any channel only partially spanned
	double sidelevel;						//	level of adjacent-to-centre chans when chanspan < 1
	double level, levelmin = 1.0;
	if(chanspan > 1.0)	
		levelmin = 1.0/chanspan;			//	minimum full-level possible, if rolloff is 1.0
	level =  (1.0 - levelmin) * (1.0 - rolloff);
	level += levelmin;
	if(chanspan < 1.0)	
		sidelevel = level * chanspan;
	else
		sidelevel = level;
	
	for(n=0;n<chans;n++) {
		outchan = map[n];
		maxchan = (double)outchan + halfspan;
		maxfullchan = (int)floor(maxchan);
		minchan = (double)outchan - halfspan;
		if(minchan < 0.0) 
			minfullchan = -(int)floor(-minchan);
		else
			minfullchan = (int)ceil(minchan);
		if((truminfullchan = minfullchan) < 0)			//	Convert outer channels to the 0 to N-1 range
			truminfullchan += chans;
		if((trumaxfullchan = maxfullchan) >= chans)
			trumaxfullchan -= chans;
		if(span > 1.0 && (truminfullchan == trumaxfullchan))	//	Avoid wrap-around causing the channel opposite outchan
			maxfullchan--;										//	Getting a double loading of signal
		for(m = minfullchan;m <= maxfullchan; m++) {			//	Put full-level signal in all chans that are completely spanned
			if((truchan = m) < 0)												
				truchan += chans;
			else if(truchan >= chans)
				truchan -= chans;
			obuf[bufpos + truchan] = (float)(obuf[bufpos + truchan] + (ibuf[bufpos + n] * level));
		}
	partial_level = maxchan - (double)maxfullchan;
		if(!flteq(partial_level, 0.0)) {						//	Span spreads partially to other outchans
			maxfullchan++;
			if((trumaxfullchan = maxfullchan) >= chans)
				maxfullchan -= chans;
			minfullchan--;
			if((truminfullchan = minfullchan) < 0)
				truminfullchan += chans;
			obuf[bufpos + truminfullchan] = (float)(obuf[bufpos + truminfullchan] + (ibuf[bufpos + n] * sidelevel * partial_level));
			if(truminfullchan != trumaxfullchan)					//	Avoid wrap-around getting double whammy
				obuf[bufpos + trumaxfullchan] = (float)(obuf[bufpos + trumaxfullchan] + (ibuf[bufpos + n] * sidelevel * partial_level));
		}
	}
}

/*************************** PANPROCESS ****************************/

#define XSAFETY 4

int panprocess(dataptr dz)
{
	int exit_status;
	int blok_cnt,  chans = dz->infile->channels, done;
	int n, j, total_sams = dz->insams[0]/chans, centrecnt=0;
	double dur = dz->duration;
	int *countlimits, *counters;
	double time = 0.0, start_time=0.0, start_pos=0.0, end_time=0.0, end_pos=0.0, timestep=0.0, timegap, timeratio, valdiff=0.0, valstep, val;

	if((dz->parray[0] = (double *)malloc(chans * sizeof(double)))==NULL) {	// level of each channel at calc point
		sprintf(errstr,"INSUFFICIENT MEMORY for storing levels.\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[1] = (double *)malloc(chans * sizeof(double)))==NULL) {	// lastlevel of each channel at calc point
		sprintf(errstr,"INSUFFICIENT MEMORY for storing previous levels.\n");
		return(MEMORY_ERROR);
	}
	if((dz->lparray[0] = (int *)malloc(chans * sizeof(int)))==NULL) {	//	counters for number of brkpnts generated for each chan
		sprintf(errstr,"INSUFFICIENT MEMORY for storing previous levels.\n");
		return(MEMORY_ERROR);
	}
	if((dz->lparray[1] = (int *)malloc(chans * sizeof(int)))==NULL) {	//	counters for current count of brkpnt vals
		sprintf(errstr,"INSUFFICIENT MEMORY for storing previous levels.\n");
		return(MEMORY_ERROR);
	}
	if((dz->lparray[2] = (int *)malloc(chans * sizeof(int)))==NULL) {	//	updown indicators to catch spread reversals
		sprintf(errstr,"INSUFFICIENT MEMORY for storing previous levels.\n");
		return(MEMORY_ERROR);
	}
	if((dz->lparray[3] = (int *)malloc(chans * sizeof(int)))==NULL) {	//	last-updown value 
		sprintf(errstr,"INSUFFICIENT MEMORY for storing previous levels.\n");
		return(MEMORY_ERROR);
	}	// counters
	
	countlimits = dz->lparray[0];
	counters	= dz->lparray[1];
	for(n = 0;n<chans; n++) {
		countlimits[n] = 0;
		counters[n]    = 0;
	}

	if(dz->brksize[0]) {
		centrecnt = 0;
		start_time = dz->brk[0][centrecnt++];
		start_pos  = dz->brk[0][centrecnt++];
		if(centrecnt >= dz->brksize[0] * 2) {
			end_pos = start_pos;
			end_time = dur;
		} else {
			end_time = dz->brk[0][centrecnt++];
			end_pos  = dz->brk[0][centrecnt++];
		}
		timestep = end_time - start_time;
		valdiff = fabs(end_pos - start_pos);	//	Force shortest route
		if(valdiff > chans/2) {					//	8 -> 2 becomes 0 ->2   7->2 becomes  -1 -> 2
			if(end_pos > start_pos)				//	But 8->5 stays as it is
				end_pos -= chans;
			else
				start_pos -= chans;
		}
		valdiff = end_pos - start_pos;
		dz->param[0] = start_pos;			
	}
	spread_pan_array_cnt(0.0,0,dz);
	blok_cnt = 0;
	for(n=0;n<total_sams;n++) {
		if((n > 0) && (blok_cnt % 256 == 0)) {
			time = (double)n/(double)dz->infile->srate;
			if(dz->brksize[0]) {
				if(time >= end_time) {
					start_time = end_time;
					start_pos  = end_pos;
					if(centrecnt >= dz->brksize[0] * 2) {
						end_pos = start_pos;
						end_time = dur;
					} else {
						end_time = dz->brk[0][centrecnt++];
						end_pos  = dz->brk[0][centrecnt++];
					}
					timestep = end_time - start_time;
					valdiff = fabs(end_pos - start_pos);	//	Force shortest route
					if(valdiff > chans/2) {					//	8 -> 2 becomes 0 ->2   7->2 becomes  -1 -> 2
						if(end_pos > start_pos)				//	But 8->5 stays as it is
							end_pos -= chans;
						else
							start_pos -= chans;
					}
					valdiff = end_pos - start_pos;
				}	
				timegap   = time - start_time;
				timeratio = timegap/timestep;
				valstep = valdiff * timeratio;
				if((val = start_pos + valstep) < 0)
					val += (double)chans;
				dz->param[0] = val;			
			}
			if(dz->brksize[1]) {
				if((exit_status = read_value_from_brktable(time,1,dz))<0)
					return(exit_status);
			}
			spread_pan_array_cnt(time,0,dz);
			blok_cnt = 0;
		}
		blok_cnt++;
	}
	done = 0;
	while(!done) {
		if(dz->brksize[0]) {
			if(time >= end_time) {
				start_time = end_time;
				start_pos  = end_pos;
				if(centrecnt >= dz->brksize[0] * 2) {
					end_pos = start_pos;
					end_time = dur;
				} else {
					end_time = dz->brk[0][centrecnt++];
					end_pos  = dz->brk[0][centrecnt++];
				}
				if((timestep = end_time - start_time) > 0.0) {
					valdiff = fabs(end_pos - start_pos);	//	Force shortest route
					if(valdiff > chans/2) {					//	8 -> 2 becomes 0 ->2   7->2 becomes  -1 -> 2
						if(end_pos > start_pos)				//	But 8->5 stays as it is
							end_pos -= chans;
						else
							start_pos -= chans;
					}
					valdiff = end_pos - start_pos;
					timegap   = time - start_time;
					timeratio = timegap/timestep;
					valstep = valdiff * timeratio;
					if((val = start_pos + valstep) < 0)
						val += chans;
					dz->param[0] = val;						// If timestep = 0.0, at end of file, 
				}											// dz->param[0] accepts same val as before	
			}												// And an array count is forced
		}
		if(dz->brksize[1]) {
			if((exit_status = read_value_from_brktable(time,1,dz))<0)
				return(exit_status);
		}
		spread_pan_array_cnt(time,1,dz);
		done = 1;
	}

	for(n=0,j=2;n<dz->infile->channels;n++,j++) {									//	breakpint data generated for each chan
		if((dz->parray[j] = (double *)malloc((countlimits[n] + XSAFETY) * sizeof(double)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for storing brkpnt data for channel %d.\n",n+1);
			return(MEMORY_ERROR);
		}
	}
	
	blok_cnt = 0;
	if(dz->brksize[0]) {
		centrecnt = 0;
		start_time = dz->brk[0][centrecnt++];
		start_pos  = dz->brk[0][centrecnt++];
		if(centrecnt >= dz->brksize[0] * 2) {
			end_pos = start_pos;
			end_time = dur;
		} else {
			end_time = dz->brk[0][centrecnt++];
			end_pos  = dz->brk[0][centrecnt++];
		}
		timestep = end_time - start_time;
		valdiff = fabs(end_pos - start_pos);	//	Force shortest route
		if(valdiff > chans/2) {					//	8 -> 2 becomes 0 ->2   7->2 becomes  -1 -> 2
			if(end_pos > start_pos)				//	But 8->5 stays as it is
				end_pos -= chans;
			else
				start_pos -= chans;
		}
		valdiff = end_pos - start_pos;
		dz->param[0] = start_pos;
	}
	if((exit_status = spread_pan_process(0.0,0,dz)) < 0)
		return(exit_status);
	for(n=0;n<total_sams;n++) {
		if((n > 0) && (blok_cnt % 256 == 0)) {
			time = (double)n/(double)dz->infile->srate;
			if(dz->brksize[0]) {
				if(time >= end_time) {
					start_time = end_time;
					start_pos  = end_pos;
					if(centrecnt >= dz->brksize[0] * 2) {
						end_pos = start_pos;
						end_time = dur;
					} else {
						end_time = dz->brk[0][centrecnt++];
						end_pos  = dz->brk[0][centrecnt++];
					}
					timestep = end_time - start_time;
					valdiff = fabs(end_pos - start_pos);	//	Force shortest route
					if(valdiff > chans/2) {					//	8 -> 2 becomes 0 ->2   7->2 becomes  -1 -> 2
						if(end_pos > start_pos)				//	But 8->5 stays as it is
							end_pos -= chans;
						else
							start_pos -= chans;
					}
					valdiff = end_pos - start_pos;
				}	
				timegap   = time - start_time;
				timeratio = timegap/timestep;
				valstep = valdiff * timeratio;
				if((val = start_pos + valstep) < 0)
					val += chans;
				dz->param[0] = val;			
			}
			if(dz->brksize[1]) {
				if((exit_status = read_value_from_brktable(time,1,dz))<0)
					return(exit_status);
			}
			if((exit_status = spread_pan_process(time,0,dz)) < 0)
				return(exit_status);
			blok_cnt = 0;
		}
		blok_cnt++;
	}
	time = (double)n/(double)dz->infile->srate;		//	Force read at end of file
	done = 0;
	while(!done) {
		if(dz->brksize[0]) {
			if(time >= end_time) {
				start_time = end_time;
				start_pos  = end_pos;
				if(centrecnt >= dz->brksize[0] * 2) {
					end_pos = start_pos;
					end_time = dur;
				} else {
					end_time = dz->brk[0][centrecnt++];
					end_pos  = dz->brk[0][centrecnt++];
				}
				if((timestep = time - start_time) > 0) {
					valdiff = fabs(end_pos - start_pos);	//	Force shortest route
					if(valdiff > chans/2) {					//	8 -> 2 becomes 0 ->2   7->2 becomes  -1 -> 2
						if(end_pos > start_pos)				//	But 8->5 stays as it is
							end_pos -= chans;
						else
							start_pos -= chans;
					}
					valdiff = end_pos - start_pos;
					timegap   = time - start_time;
					timeratio = timegap/timestep;
					valstep = valdiff * timeratio;
					if((val = start_pos + valstep) < 0)
						val += chans;
					dz->param[0] = val;						// If timestep = 0.0, at end of file, 
				}											// dz->param[0] accepts same val as before	
			}												// And a write is forced (in case no final write made)
		}
		if(dz->brksize[1]) {
			if((exit_status = read_value_from_brktable(time,1,dz))<0)
				return(exit_status);
		}
		if((exit_status = spread_pan_process(time,1,dz)) < 0)
			return(exit_status);
		done = 1;
	}
	for(j=0;j<chans;j++)
		countlimits[j] = counters[j];

	if((exit_status = write_break_data(dz)) < 0)
		return(exit_status);
	return(FINISHED);
}

/*************************** SPREAD_PAN_PROCESS ****************************/

int spread_pan_process(double time,int last,dataptr dz)
{
	int outchans   = dz->infile->channels;
	double centre  = dz->param[0];
	double spread  = dz->param[1];
	double halfspread = spread/2.0;
	double *levels = dz->parray[0];
	double *lastlevels = dz->parray[1];
	int *countlimits = dz->lparray[0];
	int *counters    = dz->lparray[1];
	int *up	 = dz->lparray[2];
	int *lastup = dz->lparray[3];
	double spredej_left, spredej_right, pos;
	int spredej_left_leftchan, spredej_right_leftchan, spredej_right_rightchan, m, k, k1, k2, j, ochan;
	double stereopos_left, stereopos_right;
	double zleft, zright, floor_zleft, floor_zright;
	double left_leftchan_level, right_rightchan_level, spredej_left_rightchan;

	for(ochan = 0;ochan<outchans; ochan++)
		levels[ochan]   = 0.0;

	if((centre = centre - 1.0) < 0.0)
		centre += (double)outchans;

	// Set all channels fully within the spread to 1.0

	spredej_left = centre - halfspread;
	while(spredej_left  < 0)
		spredej_left += (double)outchans;
	spredej_right = centre + halfspread;
	while(spredej_right >= outchans)
		spredej_right -= (double)outchans;
	spredej_left_leftchan  = (int)floor(spredej_left);
	if((spredej_left_rightchan  = spredej_left_leftchan + 1) >= outchans)
		spredej_left_rightchan = 0;
	spredej_right_leftchan = (int)floor(spredej_right);
	if(flteq(spread,0.0)) {
		k1 = (int)floor(centre);	
		pos = centre - k1;
		levels[k1] = (1.0 - pos);
		k2 = k1;
		if(++k2 >= outchans)
			k2 -= outchans;
		levels[k2] = pos;
		for(k = 0;k<outchans; k++) {
			if(k != k1 && k != k2)
				levels[k] = 0.0;
		}
	} else if(spread >= outchans) {
		for(k = 0;k<outchans; k++)
			levels[k] = 1.0;
	} else if(spread > 1.0) {

		k = spredej_left_leftchan;
		if(spredej_left_leftchan == spredej_left)
			levels[k] = 1.0;
		if(++k >= outchans)
			k -= outchans;
		while(k != spredej_right_leftchan) {
			levels[k] = 1.0;
			if(++k >= outchans)
				k -= outchans;
		}
		levels[spredej_right_leftchan] = 1.0;
	} else {

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
			levels[spredej_right_leftchan] = 1.0;
	}
	/* Do fractional channels on leading edges of spread */

	if(!flteq(spread,0.0)) {
		stereopos_left  = spredej_left - (double)spredej_left_leftchan;

		left_leftchan_level = 1 - stereopos_left;
		if(spredej_left_leftchan != spredej_right_leftchan)
			levels[spredej_left_leftchan] = left_leftchan_level;

		stereopos_right = spredej_right - (double)spredej_right_leftchan;

		right_rightchan_level = stereopos_right;
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
	}
	m = 0;
	for(k = 0,j = 2;k<outchans; k++,j++) {
		m = counters[k];
		if(time <= 0.0) {
			if(m+1 >= countlimits[k] + XSAFETY) {
				sprintf(errstr,"Array overrun (3) for channel %d.\n",k+1);
				return(PROGRAM_ERROR);
			}
			dz->parray[j][m++] = time;
			dz->parray[j][m++] = levels[k];
		} else if(last) {
			if(m+1 >= countlimits[k] + XSAFETY) {
				sprintf(errstr,"Array overrun (3) for channel %d\n",k+1);
				return(PROGRAM_ERROR);
			}
			if(time > dz->parray[j][m-2]) {
				dz->parray[j][m++] = time;
				dz->parray[j][m++] = levels[k];
			}
		} else {
			if(levels[k] <= 0.0 && lastlevels[k] > 0.0) {
				if(m+1 >= countlimits[k] + XSAFETY) {
					sprintf(errstr,"Array overrun (1) for channel %d.\n",k+1);
					return(PROGRAM_ERROR);
				}
				dz->parray[j][m++] = time;
				dz->parray[j][m++] = levels[k];
			} else if(lastlevels[k] <= 0.0 && levels[k] > 0.0) {
				if(m+1 >= countlimits[k] + XSAFETY) {
					sprintf(errstr,"Array overrun (1) for channel %d.\n",k+1);
					return(PROGRAM_ERROR);
				}
				dz->parray[j][m++] = time;
				dz->parray[j][m++] = lastlevels[k];
			} else if (levels[k] >= 1.0 && lastlevels[k] < 1.0) {
				if(m+1 >= countlimits[k] + XSAFETY) {
					sprintf(errstr,"Array overrun (2) for channel %d.\n",k+1);
					return(PROGRAM_ERROR);
				}
				dz->parray[j][m++] = time;
				dz->parray[j][m++] = levels[k];
			} else if (lastlevels[k] >= 1.0 && levels[k] < 1.0) {
				if(m+1 >= countlimits[k] + XSAFETY) {
					sprintf(errstr,"Array overrun (2) for channel %d.\n",k+1);
					return(PROGRAM_ERROR);
				}
				dz->parray[j][m++] = time;
				dz->parray[j][m++] = lastlevels[k];
			} else { 
				if(levels[k] - lastlevels[k] > 0)
					up[k] = 1;
				if(levels[k] - lastlevels[k] < 0)
					up[k] = -1;
				else
					up[k] = 0;
				if(up[k] != lastup[k]) {			// Level change can change direction of spread reverses
					if(m+1 >= countlimits[k] + XSAFETY) {
						sprintf(errstr,"Array overrun (3) for channel %d.\n",k+1);
						return(PROGRAM_ERROR);
					}
					dz->parray[j][m++] = time;
					dz->parray[j][m++] = levels[k];
				}
			}
		}
		lastlevels[k] = levels[k];
		lastup[k] = up[k];
		counters[k] = m;
	}
	return(FINISHED);
}

/*************************** SPREAD_PAN_ARRAY_CNT ****************************/

void spread_pan_array_cnt(double time,int last,dataptr dz)
{
	int outchans   = dz->infile->channels;
	double centre  = dz->param[0];
	double spread  = dz->param[1];
	double halfspread = spread/2.0;
	double *levels = dz->parray[0];
	double *lastlevels = dz->parray[1];
	int *countlimits = dz->lparray[0];
	int *up	 = dz->lparray[2];
	int *lastup = dz->lparray[3];
	double spredej_left, spredej_right, pos;
	int spredej_left_leftchan, spredej_right_leftchan, spredej_right_rightchan, k, k1, k2, ochan;
	double stereopos_left, stereopos_right;
	double zleft, zright, floor_zleft, floor_zright;
	double left_leftchan_level, right_rightchan_level, spredej_left_rightchan;

	for(ochan = 0;ochan<outchans; ochan++)
		levels[ochan] = 0.0;

	if((centre = centre - 1.0) < 0.0)
		centre += (double)outchans;

	// Set all channels fully within the spread to 1.0

	spredej_left = centre - halfspread;
	while(spredej_left  < 0)
		spredej_left += (double)outchans;
	spredej_right = centre + halfspread;
	while(spredej_right >= outchans)
		spredej_right -= (double)outchans;
	spredej_left_leftchan  = (int)floor(spredej_left);
	if((spredej_left_rightchan  = spredej_left_leftchan + 1) >= outchans)
		spredej_left_rightchan = 0;
	spredej_right_leftchan = (int)floor(spredej_right);

	if(flteq(spread,0.0)) {
		k1 = (int)floor(centre);	
		pos = centre - k1;
		levels[k1] = (1.0 - pos);
		k2 = k1;
		if(++k2 >= outchans)
			k2 -= outchans;
		levels[k2] = pos;
		for(k = 0;k<outchans; k++) {
			if(k != k1 && k != k2)
				levels[k] = 0.0;
		}
	} else if(spread >= outchans) {
		for(k = 0;k<outchans; k++)
			levels[k] = 1.0;
		return;
	} else if(spread > 1.0) {

		k = spredej_left_leftchan;
		if(spredej_left_leftchan == spredej_left)
			levels[k] = 1.0;
		if(++k >= outchans)
			k -= outchans;

		while(k != spredej_right_leftchan) {
			levels[k] = 1.0;
			if(++k >= outchans)
				k -= outchans;
		}
		levels[spredej_right_leftchan] = 1.0;
	} else {

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
			levels[spredej_right_leftchan] = 1.0;
	}
	/* Do fractional channels on leading edges of spread */

	if(!flteq(spread,0.0)) {
		stereopos_left  = spredej_left - (double)spredej_left_leftchan;

		left_leftchan_level = 1 - stereopos_left;
		if(spredej_left_leftchan != spredej_right_leftchan)
			levels[spredej_left_leftchan] = left_leftchan_level;

		stereopos_right = spredej_right - (double)spredej_right_leftchan;

		right_rightchan_level = stereopos_right;
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
	}
	for(k = 0;k<outchans; k++) {
		if(last || (time <= 0.0))
			countlimits[k] += 2;
		else {
			if(levels[k] <= 0.0 && lastlevels[k] > 0.0) {
				countlimits[k] += 2;
			} else if(lastlevels[k] <= 0.0 && levels[k] > 0.0) {
				countlimits[k] += 2;
			} else if (levels[k] >= 1.0 && lastlevels[k] < 1.0) {
				countlimits[k] += 2;
			} else if (lastlevels[k] >= 1.0 && levels[k] < 1.0) {
				countlimits[k] += 2;
			} else {
				if(levels[k] - lastlevels[k] > 0)
					up[k] = 1;
				if(levels[k] - lastlevels[k] < 0)
					up[k] = -1;
				else
					up[k] = 0;
				if(up[k] != lastup[k]) {			// Level can change if direction of spread reverses
					countlimits[k] += 2;
				}
			}
		}
		lastlevels[k] = levels[k];
		lastup[k] = up[k];
	}
}

/*************************** WRITE_BREAK_DATA ****************************/

int write_break_data(dataptr dz)
{
	char filename[48], temp[20];
	double *data;
	int j, k, outchans = dz->infile->channels;
	int i, cnt;
	FILE *fp;
	for(k = 0,j = 2;k<outchans; k++,j++) {
		data = dz->parray[j];
		strcpy(filename,"cdptest");
		sprintf(temp,"%d",k+1);
		strcat(filename,temp);
		strcat(filename,".txt");
		if((fp = fopen(filename,"w"))==NULL) {
			sprintf(errstr,"Cannot open output file %s\n",filename);
			return(DATA_ERROR);
		}
		cnt = dz->lparray[0][k];	// count of entries in dz->parray[k]
		for(i=0;i < cnt;i+=2)
			fprintf(fp,"%lf\t%lf\n",data[i],data[i+1]);
		if(fclose(fp)<0) {
			sprintf(errstr,"WARNING: Failed to close output textfile %s.\n",filename);
			return(DATA_ERROR);
		}
	}
	return(FINISHED);
}

/************************************************ MCHANPAN2 ******************************************
 *
 * Rotate mono file, giving speed in a breakpoint file.
 */

#define MCHANPAN2_OCHANS	0
#define MCHANPAN2_STARTCH	1
#define MCHANPAN2_SPEED		2

int mchanpan2(dataptr dz)
{
	int exit_status;
	int outchans = dz->iparam[MCHANPAN2_OCHANS];
	double doutchans = (double)outchans;
	double srate = (double)dz->infile->srate;
	double chans_per_sec, chans_per_samp = 0.0;
	double position = dz->param[MCHANPAN2_STARTCH], time = 0.0;
	double leftgain, rightgain, farleftgain, farrightgain;
	int leftchan, farleftchan, rightchan, farrightchan;
	int pantype = 1;
	int ibufpos = 0, obufpos = 0;
	float *ibuf = dz->sampbuf[0];
	float *obuf = dz->sampbuf[1];
	int obuflen = dz->buflen * dz->iparam[MCHANPAN2_OCHANS];
	int total_samps_processed = 0;

	memset((char *)obuf,0,obuflen * sizeof(float));
	if((exit_status = read_samps(ibuf,dz))<0) {
		sprintf(errstr,"Failed to read data from sndfile.\n");
		return(DATA_ERROR);
	}
	if (dz->brksize[MCHANPAN2_SPEED] == 0) {
		chans_per_sec = dz->param[MCHANPAN2_SPEED] * doutchans;
		chans_per_samp = chans_per_sec/srate;
		if(dz->vflag[0])
			chans_per_samp = -chans_per_samp;
	}
	while(total_samps_processed < dz->insams[0]) {
		if(total_samps_processed % 256 == 0) {
			if(dz->brksize[MCHANPAN2_SPEED]) {
				time = (double)total_samps_processed/srate;
				if((exit_status = read_value_from_brktable(time,MCHANPAN2_SPEED,dz)) < 0)
					return exit_status;
				chans_per_sec = dz->param[MCHANPAN2_SPEED] * doutchans;
				chans_per_samp = chans_per_sec/srate;
				if(dz->vflag[0])
					chans_per_samp = -chans_per_samp;
			}
		}
		position += chans_per_samp;
		if(position > doutchans)		/* position = lspkr no (or some fraction inbetween) (range 1 - outchans) */
			position -= doutchans;		/* Convention is that vals 1 to 8 refers to positions at or between lspkrs 1-8 */
		else if(position < 0.0)			/* while values below 1 refer to positions between lspkr 8 and lspkr 1, in the 'circle' */
			position += doutchans;		/* We adjust here, when position-incrementation takes position beyond these limits */
		mchan_pancalc(position,&leftgain,&rightgain,&farleftgain,&farrightgain,&leftchan,pantype,3,dz);
		leftchan--;												/* output bufs numbered from 0 */
		farleftchan  = (leftchan + outchans - 1) % outchans;	/* output chans are adjacent, L and R refer to lspkrs to left and right of current position */
		rightchan    = (leftchan + 1) % outchans;				/* farleft and farright are speakers next to left and right spkrs, for any unfocused signal */
		farrightchan = (rightchan + 1) % outchans;
		obuf[obufpos + farleftchan]  = (float)(obuf[obufpos + farleftchan]  + (ibuf[ibufpos] * farleftgain));
		obuf[obufpos + leftchan]     = (float)(obuf[obufpos + leftchan]     + (ibuf[ibufpos] * leftgain));
		obuf[obufpos + rightchan]    = (float)(obuf[obufpos + rightchan]    + (ibuf[ibufpos] * rightgain));
		obuf[obufpos + farrightchan] = (float)(obuf[obufpos + farrightchan] + (ibuf[ibufpos] * farrightgain));
		ibufpos++;
		obufpos += outchans;
		if(ibufpos >=  dz->buflen) {
			if((exit_status = read_samps(ibuf,dz))<0) {
				sprintf(errstr,"Failed to read data from sndfile.\n");
				return(DATA_ERROR);
			}
			if(dz->ssampsread == 0)
				break;
			ibufpos = 0;
		}
		if(obufpos >= obuflen) {
			if((exit_status = write_samps(obuf,obuflen,dz))<0)
				return(exit_status);
			memset((char *)obuf,0,obuflen * sizeof(float));
			obufpos = 0;
		}
		total_samps_processed++;
	}
	if(obufpos > 0) {
		if((exit_status = write_samps(obuf,obufpos,dz))<0)
			return(exit_status);
	}
	return FINISHED;
}

/*************************** PERMUTE_CHANS ***************************/

void permute_chans(int outchans,dataptr dz)
{   
	int n, t;
	for(n=0;n<outchans;n++) {
		t = (int)(drand48() * (double)(n+1));	 /* TRUNCATE */
		if(t==n)
			prefixch(n,outchans,dz);
		else
			insertch(n,t,outchans,dz);
	}
}

/****************************** INSERTCH ****************************/

void insertch(int n,int t,int outchans,dataptr dz)
{   
	shuflupch(t+1,outchans,dz);
	dz->iparray[0][t+1] = n;
}

/****************************** PREFIX ****************************/

void prefixch(int n,int outchans,dataptr dz)
{   
	shuflupch(0,outchans,dz);
	dz->iparray[0][0] = n;
}

/****************************** SHUFLUPCH ****************************/

void shuflupch(int k,int outchans,dataptr dz)
{   
	int n;
	for(n = outchans - 1; n > k; n--)
		dz->iparray[0][n] = dz->iparray[0][n-1];
}

/*************************** ADJACENCE **************************/

int adjacence(int endchan,dataptr dz)
{
	int *perm = dz->iparray[0];
	int i, j, ochans = dz->iparam[0];
	int lastchan = ochans - 1;
	if(dz->vflag[0] == 0)
		return 0;
	if(endchan >= 0) {
		if ((perm[0] == 0) && (endchan == lastchan))
			return 1;
		else if((endchan == 0) && (perm[0] == lastchan))
			return 1;
		else if(perm[0] - endchan == 1)
			return 1;
		else if(endchan - perm[0] == 1)
			return 1;
	}
	for(i=0;i < ochans;i++) {
		j = (i + 1) % ochans;
		if ((perm[i] == 0) && (perm[j] == lastchan))
			return 1;
		else if((perm[j] == 0) && (perm[i] == lastchan))
			return 1;
		else if(perm[j] - perm[i] == 1)
			return 1;
		else if(perm[i] - perm[j] == 1)
			return 1;
	}
	return 0;
}

// cmdline: mchanpan mchanpan 8 infile centre spread keyparam
//
//	MODE 7(8) output is
//	brkpnt files cdptest1 to N .txt
//	Where a key param of process can reduce process effect to zero, breakpoint replaces key param (Key param must NOT timevary)
//	Where this is not true, brkpnt use to control mix balance (keyparam = 0)
