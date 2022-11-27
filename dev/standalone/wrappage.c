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
 *	INPUT CAN BE MULTICHANNEL BUT WILL BE MIXED TO MONO
 *	Code based on SAUSAGE with spatialisation altered
*
 *	ABOUT THIS PROCESS
 *
 *	There are 3 + (infiles * 2) buffers invloved here.....
 *	WRAP_SBUF into which the input data is read  from the file.
 *	THIS_BUF(x) several buffers, one for each infile,
 *      into which the input data for each file is mixed down to mono, where necessary. Otherwise, initial reads go directly to this buffer.
 *		This buffer has an overflow sector of GRS_BUF_SMPXS samples.
 *		The 1st read fills the buffer and the overflow.After the first read these overflow samples are copied to the bufstart,
 *		and a new input buffer start is marked, overflow-samples into the GRS_BUF at GRS_IBUF.
 *		THIS_IBUF(x) are then the points at which further samples are read into the input buffers.
 *	GRS_LBUF is buffer into which grains are copied for processing to the output, (they are timestretched, pshifted or and spliced).
 *	WRAP_OBUF is the buffer into which the finished grains are copied ready for output.
 *
 *	GRS_INCHANS is the number of channels in the input grains i.e. after they have (or have not) been mixed down to mono.
 *		In this program, this is ALWAYS MONO
 *	GRS_OUTCHANS is the number of channels in the output.
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
#include <osbind.h>
#include <standalone.h>
#include <ctype.h>
#include <sfsys.h>
#include <string.h>
#include <srates.h>
#include <limits.h>
#include <float.h>
#include <sfsys.h>
#include <osbind.h>

#if defined unix || defined __GNUC__
#define round(x) lround((x))
#endif
#ifndef HUGE
#define HUGE 3.40282347e+38F
#endif

char errstr[2400];

int anal_infiles = 0;
int	sloom = 0;
int sloombatch = 0;

const char* cdp_version = "7.1.0";


//CDP LIB REPLACEMENTS
static int check_wrappage_param_validity_and_consistency(dataptr dz);
static int setup_wrappage_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_wrappage_param_ranges_and_defaults(dataptr dz);
static int handle_the_outfile(int *cmdlinecnt,char ***cmdline,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
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
static int setup_and_init_input_brktable_constants(dataptr dz,int brkcnt);

#define WRAP_SBUF	(0)
#define WRAP_OBUF	(1)
#define WRAP_IBUF	(2)
#define	THIS_BUF(x)		(((x)*2)+2)
#define	THIS_IBUF(x)	(((x)*2)+3)

#define SAFETY   ((int)4096)
#define OSAFETY  ((int)256)

static int    wrappage(dataptr dz);
static int    granulate(int *thissnd,int *aipc,int *aopc,float **iiiiptr,float **LLLLptr,
				double inv_sr,int *samptotal,float **maxwrite,int *nctr,dataptr dz);
static double dehole(double pos);
static int    write_samps_granula(int k,int *nctr,dataptr dz);
static float  interp_gval_with_amp(float *s,double flcnt_frac,int chans,float ampp);
static float  interp_gval(float *s,double flcnt_frac,int chans);
static int    set_instep(int ostep_per_chan,dataptr dz);
static int    set_outstep(int gsize_per_chan,dataptr dz);
static int    set_range(int absiicnt,dataptr dz);
static int   do_scatter(int ostep_per_chan,dataptr dz);
static int    set_ivalue(int flag,int paramno,int hparamno,int rangeno,dataptr dz);
static double set_dvalue(int flag,int paramno,int hparamno,int rangeno,dataptr dz);
static int    renormalise(int nctr,dataptr dz);

static int  read_samps_wrappage(int firsttime,dataptr dz);
static int  read_a_specific_large_buf(int k,dataptr dz);
static int  read_a_specific_normal_buf_with_wraparound(int k,dataptr dz);
static int  baktrak_wrappage(int thissnd,int samptotal,int absiicnt_per_chan,float **iiptr,dataptr dz);
static int  reset_wrappage(int thissnd,int resetskip,dataptr dz);
static int  get_next_insnd(dataptr dz);
static void perm_wrappage(int cnt,dataptr dz);
static void insert(int n,int t,int cnt_less_one,dataptr dz);
static void prefix(int n,int cnt_less_one,dataptr dz);
static void shuflup(int k,int cnt_less_one,dataptr dz);

static int grab_an_appropriate_block_of_wrappage_memory(int *this_bloksize,int standard_block,int bufdivisor,dataptr dz);

static int make_multichan_grain(float *b,float **iiptr,float aamp,int gsize_per_chan,double *transpos,dataptr dz);
static void	do_multichan_splices(int gsize_per_chan,int bspl,int espl,dataptr dz);
static void do_multichan_btab_splice(dataptr dz);
static void do_multichan_bsplice(int gsize_per_chan,dataptr dz,int bspl);
static void do_multichan_etab_splice(int gsize_per_chan,dataptr dz);
static void do_multichan_esplice(int gsize_per_chan,dataptr dz,int espl);
static int write_multichan_grain(double rpos,int chan, int chanb,float **maxwrite,float **Fptr,float **FFptr,int gsize_per_chan,int *nctr,dataptr dz);
static double get_wrappage_position(dataptr dz);
static int create_wrappage_buffers(dataptr dz);
static int wrappage_preprocess(dataptr dz);
static int initialise_unused_wrappage_vflags(dataptr dz);
static void param_check_and_convert(int *zerovel,dataptr dz);
static int check_spatialisation_data(dataptr dz);
static void convert_pitch(int paramno,dataptr dz);
static int initialise_data(dataptr dz);
static int set_internal_flags(dataptr dz);
static void initialise_channel_configuration(dataptr dz);
static int granula_setup(dataptr dz);
static int check_for_zeroes(int paramno,dataptr dz);
static int calc_overflows(dataptr dz);
static void set_ranges(dataptr dz);
static int make_splice_tables(dataptr dz);
static void convert_value_to_int(int paramno,double convertor,dataptr dz);
static int check_for_zeroes(int paramno,dataptr dz);
static void set_this_range(int rangeno,int hino, int lono, int flagno, dataptr dz);
static void set_this_drange(int rangeno,int hino, int lono, int flagno, dataptr dz);
static int calc_max_scatter(int mgi,int *max_scatlen, dataptr dz);
static void adjust_overflows(int mscat, dataptr dz);
static int store_the_filename(char *filename,dataptr dz);
static void get_centre(double thistime,dataptr dz);
static int handle_the_special_data(int *cmdlinecnt,char ***cmdline,dataptr dz);
static int read_centre_data(char *filename,dataptr dz);
static int read_timevarying_data(double itime, double otime, dataptr dz);

#define wrapcentre	scalefact	// USe dz->scalefact to store wrapcentre

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
	dz->maxmode = 0;
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
		// setup_particular_application =
		if((exit_status = setup_wrappage_application(dz))<0) {
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
	// open_first_infile		CDP LIB
	if((exit_status = open_first_infile(cmdline[0],dz))<0) {	
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);	
		return(FAILED);
	}
	cmdlinecnt--;
	cmdline++;

//	handle_extra_infiles() ........
	if((exit_status = handle_extra_infiles(&cmdline,&cmdlinecnt,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);		
		return(FAILED);
	}
//NEW
	// setup_param_ranges_and_defaults() =
	if((exit_status = setup_wrappage_param_ranges_and_defaults(dz))<0) {
		exit_status = print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	// handle_outfile() = 
	if((exit_status = handle_the_outfile(&cmdlinecnt,&cmdline,dz))<0) {		//	Just takes its name for now
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}

//	handle_formants()			redundant
//	handle_formant_quiksearch()	redundant
// handle_special_data()		redundant
	if((exit_status = handle_the_special_data(&cmdlinecnt,&cmdline,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {		// CDP LIB
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	
//	check_param_validity_and_consistency....
	if((exit_status = check_wrappage_param_validity_and_consistency(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	is_launched = TRUE;
	//param_preprocess()
	if((exit_status = wrappage_preprocess(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = create_wrappage_buffers(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	//spec_process_file =
	if((exit_status = wrappage(dz))<0) {
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

/**************************************************/
/* GENERAL FUNCTIONS, REPLACING CDP LIB FUNCTIONS */
/**************************************************/

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
	switch(dz->process) {
	case(PSOW_INTERLEAVE):
	case(PSOW_REPLACE):
		dz->infilecnt = 2;
		break;
	default:
		dz->infilecnt = 1;
		break;
	}
	if((exit_status = setup_internal_arrays_and_array_pointers(dz))<0)	 
		return(exit_status);
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
	int exit_status;
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
	if((exit_status = store_the_filename(filename,dz))<0)
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

/************************* SETUP_WRAPPAGE_APPLICATION *******************/

int setup_wrappage_application(dataptr dz)
{
	int exit_status;
	aplptr ap;
	if((exit_status = establish_application(dz))<0)		// GLOBAL
		return(FAILED);
	ap = dz->application;
	// SEE parstruct FOR EXPLANATION of next 2 functions
	if((exit_status = set_param_data(ap,WRAP_FOCUS,20,20,"iDDDDDDDDDDDDDDDDDDd")) < 0)
		return(FAILED);
	if((exit_status = set_vflgs(ap,"b",1,"i","eo",2,0,"00")) < 0)
		return(FAILED);
	if((exit_status = set_internalparam_data("ddddiiiiiiiiiiiiiiiii",ap)) < 0)
		return(FAILED);
	// set_legal_infile_structure -->
	dz->has_otherfile = FALSE;
	// assign_process_logic -->
	dz->input_data_type = ONE_OR_MANY_SNDFILES;
	dz->process_type	= UNEQUAL_SNDFILE;	
	dz->outfiletype  	= SNDFILE_OUT;

	return application_init(dz);	//GLOBAL
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

/************************* SETUP_WRAPPAGE_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_wrappage_param_ranges_and_defaults(dataptr dz)
{
	int exit_status, n, chans = dz->infile->channels;
	int infilesize_in_samps;
	double sr = (double)dz->infile->srate, duration;
	aplptr ap = dz->application;
	infilesize_in_samps = dz->insams[0];
	for(n = 1;n < dz->infilecnt;n++)
		infilesize_in_samps = min(dz->insams[1],infilesize_in_samps);
	infilesize_in_samps /= chans;
	duration = (double)infilesize_in_samps/sr;
	// set_param_ranges()
	ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
	// NB total_input_param_cnt is > 0 !!!s
	if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
		return(FAILED);
	// get_param_ranges()
	ap->lo[WRAP_OUTCHANS]	= 2.0;						
	ap->hi[WRAP_OUTCHANS]	= 16.0;
	ap->default_val[WRAP_OUTCHANS]	= 2.0;						
	ap->lo[WRAP_SPREAD]     = 0.0;							
	ap->hi[WRAP_SPREAD]     = 16.0;									
	ap->default_val[WRAP_SPREAD]   	= 16.0;		
	ap->lo[WRAP_DEPTH]		= 0.0;						
	ap->hi[WRAP_DEPTH]		= 16.0;
	ap->default_val[WRAP_DEPTH]   	= 4.0;		
	ap->lo[WRAP_VELOCITY]  	= 0.0;
	ap->hi[WRAP_VELOCITY]  	= WRAP_MAX_VELOCITY;							
	ap->default_val[WRAP_VELOCITY]  = 1.0;
	ap->lo[WRAP_HVELOCITY]  = 0.0;
	ap->hi[WRAP_HVELOCITY]  = WRAP_MAX_VELOCITY;								
	ap->default_val[WRAP_HVELOCITY] = 1.0;
   	ap->lo[WRAP_DENSITY]   	= FLTERR;
    ap->hi[WRAP_DENSITY] 	= WRAP_MAX_DENSITY;					
    ap->default_val[WRAP_DENSITY]  	= WRAP_DEFAULT_DENSITY;			
	ap->lo[WRAP_HDENSITY]   = (1.0/sr);
	ap->hi[WRAP_HDENSITY]   = (double)MAXSHORT/2.0;						
	ap->default_val[WRAP_HDENSITY]  = WRAP_DEFAULT_DENSITY;
	ap->lo[WRAP_GRAINSIZE]  = WRAP_MIN_SPLICELEN * 2.0;														   
	ap->hi[WRAP_GRAINSIZE]  = (infilesize_in_samps/sr) * SECS_TO_MS;				
	ap->default_val[WRAP_GRAINSIZE] = WRAP_DEFAULT_GRAINSIZE;
	ap->lo[WRAP_HGRAINSIZE] = WRAP_MIN_SPLICELEN * 2.0;
	ap->hi[WRAP_HGRAINSIZE] = (infilesize_in_samps/sr) * SECS_TO_MS;				
	ap->default_val[WRAP_HGRAINSIZE]= WRAP_DEFAULT_GRAINSIZE;
	ap->hi[WRAP_PITCH]    	= LOG2(dz->nyquist/WRAP_MIN_LIKELY_PITCH) * SEMITONES_PER_OCTAVE;	
	ap->lo[WRAP_PITCH]    	= -(ap->hi[WRAP_PITCH]);			
	ap->default_val[WRAP_PITCH]   	= 0.0;			
	ap->hi[WRAP_HPITCH]     = LOG2(dz->nyquist/WRAP_MIN_LIKELY_PITCH) * SEMITONES_PER_OCTAVE;	
	ap->lo[WRAP_HPITCH]     = -(ap->hi[WRAP_HPITCH]);			
	ap->default_val[WRAP_HPITCH]   	= 0.0;			
	ap->lo[WRAP_AMP]       	= 0.0;
	ap->hi[WRAP_AMP]       	= 1.0;									
	ap->default_val[WRAP_AMP]       = 1.0;
	ap->lo[WRAP_HAMP]       = 0.0;
	ap->hi[WRAP_HAMP]       = 1.0;									
	ap->default_val[WRAP_HAMP]      = 1.0;									
	ap->lo[WRAP_BSPLICE]   	= WRAP_MIN_SPLICELEN;
	ap->hi[WRAP_BSPLICE]   	= ap->hi[WRAP_GRAINSIZE]/2.0;			
	ap->default_val[WRAP_BSPLICE]   = WRAP_DEFAULT_SPLICELEN;			
	ap->lo[WRAP_HBSPLICE]   = WRAP_MIN_SPLICELEN;
	ap->hi[WRAP_HBSPLICE]   = ap->hi[WRAP_GRAINSIZE]/2.0;			
	ap->default_val[WRAP_HBSPLICE]  = WRAP_DEFAULT_SPLICELEN;
	ap->lo[WRAP_ESPLICE]   	= WRAP_MIN_SPLICELEN;
	ap->hi[WRAP_ESPLICE]   	= ap->hi[WRAP_GRAINSIZE]/2.0;			
	ap->default_val[WRAP_ESPLICE]   = WRAP_DEFAULT_SPLICELEN;			
	ap->lo[WRAP_HESPLICE]   = WRAP_MIN_SPLICELEN;
	ap->hi[WRAP_HESPLICE]   = ap->hi[WRAP_GRAINSIZE]/2.0;			
	ap->default_val[WRAP_HESPLICE]  = WRAP_DEFAULT_SPLICELEN;
	ap->lo[WRAP_SRCHRANGE] 	= 0.0;
	ap->hi[WRAP_SRCHRANGE] 	= (duration * 2.0) * SECS_TO_MS;						
	ap->default_val[WRAP_SRCHRANGE] = 0.0;			
	ap->lo[WRAP_SCATTER]    = 0.0;
	ap->hi[WRAP_SCATTER]    = 1.0;									
	ap->default_val[WRAP_SCATTER] = WRAP_DEFAULT_SCATTER;
	ap->lo[WRAP_OUTLEN]   	= 0.0;
	ap->hi[WRAP_OUTLEN]   	= BIG_TIME;						
	ap->default_val[WRAP_OUTLEN]  = 0.0;
	ap->lo[WRAP_BUFXX]   	= 0.0;
	ap->hi[WRAP_BUFXX]   	= 64.0;						
	ap->default_val[WRAP_BUFXX]  = 0.0;

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
			if((exit_status = setup_wrappage_application(dz))<0)
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

/************************* SETUP_INTERNAL_ARRAYS_AND_ARRAY_POINTERS *******************/

int setup_internal_arrays_and_array_pointers(dataptr dz)
{
	int n;		 
	dz->array_cnt = 4;
	dz->iarray_cnt = 2;
	dz->larray_cnt = 0;
	dz->ptr_cnt = 0;
	dz->fptr_cnt = 4;

	if((dz->parray  = (double **)malloc(dz->array_cnt * sizeof(double *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for internal double arrays.\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<dz->array_cnt;n++)
		dz->parray[n] = NULL;

	if((dz->iparray = (int     **)malloc(dz->iarray_cnt * sizeof(int *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for internal int arrays.\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<dz->iarray_cnt;n++)
		dz->iparray[n] = NULL;

	if((dz->fptr = (float  **)malloc(dz->fptr_cnt * sizeof(float *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for internal float-pointer arrays.\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<dz->fptr_cnt;n++)
		dz->fptr[n] = NULL;
	return(FINISHED);
}

/**************************** CHECK_WRAPPAGE_PARAM_VALIDITY_AND_CONSISTENCY *****************************/

int check_wrappage_param_validity_and_consistency(dataptr dz)
{
	int exit_status;
	int zerovel      = FALSE;
	int total_flags  = WRAP_SFLAGCNT+WRAP_FLAGCNT;
	int n;
	int min_infilesize = INT_MAX;

	initrand48();

	if(dz->application->vflag_cnt < WRAP_MAX_VFLAGS
	&&(exit_status = initialise_unused_wrappage_vflags(dz))<0) 
		return(exit_status);	/* Create flags inaccessible to user in certain modes */

	for(n=0;n<dz->infilecnt;n++)	
		min_infilesize = min(min_infilesize,dz->insams[n]);
	if(min_infilesize == INT_MAX) {
		sprintf(errstr,"Problem estimating min infilesize.\n");
		return(PROGRAM_ERROR);
	}
    dz->iparam[WRAP_ORIG_SMPSIZE] = min_infilesize/dz->infile->channels;

	if((dz->iparray[WRAP_FLAGS]=(int *)malloc(total_flags * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for wrappage internal flags.\n");
		return(MEMORY_ERROR);		/* establish internal flagging */
	}
	for(n=0;n<total_flags;n++)
		dz->iparray[WRAP_FLAGS][n] = 0;

	param_check_and_convert(&zerovel,dz);

	if(zerovel && dz->param[WRAP_OUTLEN]==0.0) {
		sprintf(errstr,	"Zero VELOCITY found: Outfile length must be specified.\n");
		return(USER_ERROR);
	}
	dz->out_chans = dz->iparam[WRAP_OUTCHANS];	

	if((exit_status = check_spatialisation_data(dz))<0)
		return(exit_status);

	convert_pitch(WRAP_PITCH,dz);
	convert_pitch(WRAP_HPITCH,dz);

	dz->iparam[WRAP_ARRAYSIZE] = BIGARRAY;
	if((exit_status = initialise_data(dz))<0) 			/* before setup_environment */
		return(exit_status);
	if((exit_status = set_internal_flags(dz))<0)
		return(exit_status);
	initialise_channel_configuration(dz);
	dz->outfile->channels = dz->iparam[WRAP_OUTCHANS];	/* preset channel count for OUTPUT */

	if((exit_status = granula_setup(dz))<0)
		return(exit_status);							/* has to be done before buffers.c */
	dz->extra_bufcnt = 1;
	dz->bufcnt = 2;
	dz->bufcnt += (dz->infilecnt * 2);
	if((dz->sampbuf  = (float **)malloc(dz->bufcnt * sizeof(float *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY (1)to reallocate space for wrappage.\n");
		return(MEMORY_ERROR);
	}
	if((dz->sbufptr  = (float **)malloc(dz->bufcnt * sizeof(float *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY (2)to reallocate space for wrappage.\n");
		return(MEMORY_ERROR);
	}
	if((dz->extrabuf = (float **)malloc(dz->extra_bufcnt * sizeof(float *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY (3)to reallocate space for wrappage.\n");
		return(MEMORY_ERROR);
	}
	dz->outfile->channels = dz->iparam[0];
	return(FINISHED);
}

/******************************************* GET_THE_PROCESS_NO *****************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	if     (!strcmp(prog_identifier_from_cmdline,"wrappage"))		dz->process = WRAPPAGE;
	else {
		sprintf(errstr,"Unknown program identification string '%s'\n",prog_identifier_from_cmdline);
		return(USAGE_ONLY);
	}
	return FINISHED;
}

/******************************** USAGE1 ********************************/

int usage1(void)
{
	return usage2("wrappage");
}

/******************************** USAGE2 ********************************/

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
	if (!strcmp(str,"wrappage")) {
		fprintf(stdout,	
		"GRANULAR RECONSTITUTION OF ONE OR MORE SOUNDFILES OVER MULTICHANNEL SPACE.\n\n"
		"USAGE       (name of outfile must NOT end with a '1')\n"
		"wrappage wrappage infile [infile2 ...] outfile centre outchans spread depth\n"
		"veloc hveloc dens hdens gsize hgsize pshift hpshift amp hamp bsplice hbsplice\n"
		"esplice hesplice  range jitter outlength [-bmult] [-e] [-o]\n\n"
		"CENTRE:    central position of output image in outputfile. (Range 0-outchans).\n"
		"           Centre values below 1 are positions between last chan and 1st chan.\n"
		"           Numeric, or a file of triples \"time   centre   direction\". Direction \n"
		"           (1 clockwise, -1 anticlock) is of centre's move from previous position.\n"
		"OUTCHANS:  Number of channels in output file.\n"
		"SPREAD:    Width (from far left to far right) of spatialisation around centre.\n"
		"DEPTH:     No of channels with sound, behind leading edges of spread.\n"
		"           (Will always be forced to <= half the spread value).\n"
		"VELOC:     speed of advance in infiles, relative to outfile. (>=0)\n"
		"           inverse of timestretch, (permits infinite timestretch).\n"
		"DENS:      grain overlap (>0 : <1 leaves intergrain silence)\n"
		"           Extremely small values don't perform predictably.\n"
		"GSIZE:     grainsize in MS (must be > 2 * splicelen) (Default %.0lf)\n"
		"PSHIFT:    pitchshift in +|- (fractions of) semitones.\n"
		"AMP:       gain on grains (range 0-1) (Default 1.0)\n"
		"           use if amp variation required (over a range &/or in time)\n"
		"BSPLICE:   grain-startsplice length,in MS (Default %.0lf)\n"
		"ESPLICE:   grain-endsplice length,in MS (Default %.0lf)\n"
		"RANGE:     of search for nextgrain (in mS), before infile 'now'  (Default 0).\n"
		"JITTER:    Randomisation of grain position (Range 0-1) Default (%.2lf).\n"
		"OUTLENGTH: max outfile length (if end of data not reached).\n"
		"           Set to zero (Default) to ignore.\n"
		"           BUT if VELOCITY is ANYWHERE 0: OUTLENGTH must be given.\n"
		"\n"
		"HVELOC,HDENS,HGSIZE,HPSHIFT,HAMP,HBSPLICE,HESPLICE\n"
		"allow a range of values to be specified for any of these params. e.g. With\n"
		"PSHIFT & HPSHIFT set, random pitchshift chosen between these limits.\n"
		"AND NB PSHIFT & HPSHIFT can both vary through time.\n\n"
		"All params, except OUTLENGTH, OUTCHANS, can vary through time.\n"
		"\n"
		"MULT      Long silences in output may require larger buffers to be used\n"
		"          \"Mult\" enlarges output buffers \"mult\" times. \n"
		"-e        Use exponential splices. Default, linear.\n"
		"-o        Velocity params ALWAYS read relative to time in INPUT file.\n"
		"          Other params normally read relative to time in INPUT file.\n"
		"          Flag forces them to be read relative to time in OUTPUT file.\n"
		"          \n",WRAP_DEFAULT_GRAINSIZE,WRAP_DEFAULT_SPLICELEN,WRAP_DEFAULT_SPLICELEN,WRAP_DEFAULT_SCATTER);
	} else
		fprintf(stdout,"Unknown option '%s'\n",str);
	return(USAGE_ONLY);
}

/******************************** USAGE3 ********************************/

int usage3(char *str1,char *str2)
{
	fprintf(stderr,"Insufficient parameters on command line.\n");
	return(USAGE_ONLY);
}

/************************* redundant functions: to ensure libs compile OK *******************/

int assign_process_logic(dataptr dz)
{
	return(FINISHED);
}

void set_legal_infile_structure(dataptr dz)
{}

int establish_bufptrs_and_extra_buffers(dataptr dz)
{
	return(FINISHED);
}

int inner_loop
(int *peakscore,int *descnt,int *in_start_portion,int *least,int *pitchcnt,int windows_in_buf,dataptr dz)
{
	return(FINISHED);
}

int read_special_data(char *str,dataptr dz)	
{
	return(FINISHED);
}

int get_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	return(FINISHED);
}

int set_legal_internalparam_structure(int process,int mode,aplptr ap)
{
	
	return(FINISHED);
}

/*************************** CREATE_WRAPPAGE_BUFFERS **************************/

int create_wrappage_buffers(dataptr dz)
{
	size_t standard_block = (size_t) Malloc(-1);
	int this_bloksize;
	int   exit_status, n, chans = dz->infile->channels;
	int   overall_size, bufdivisor = 0;
	float *tailend;
	int  multichan_buflen = 0, multichan_bufxs = 0, outbuflen;

	if((dz->extrabuf[WRAP_GBUF] = (float *)malloc(dz->iparam[WRAP_GLBUF_SMPXS] * sizeof(float)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create grain buffer.\n");	 /* GRAIN BUFFER */
		return(MEMORY_ERROR);
	}
				/* CALCULATE NUMBER OF BUFFER CHUNKS REQUIRED : bufdivisor */
	if(dz->iparam[WRAP_CHANNELS]>0)
		bufdivisor += chans;							/* chans for multichan-infile, before reducing to mono */
	bufdivisor += dz->infilecnt;						/* 1 MONO buffer for each infile */
	for(n=0;n<dz->iparam[WRAP_OUTCHANS];n++)								
		bufdivisor += 1 + (sizeof(int)/sizeof(float));	/* 1 float and 1 long buf for output, both * sizeof outchans */
	this_bloksize = (int) standard_block;
	for(;;) {
		if((exit_status = grab_an_appropriate_block_of_wrappage_memory(&this_bloksize,(int) standard_block,bufdivisor,dz))<0)
			return(exit_status);

					/* CALCULATE AND ALLOCATE TOTAL MEMORY REQUIRED : overall_size */
	
		overall_size = (dz->buflen * bufdivisor) + (dz->iparam[WRAP_BUF_SMPXS] * dz->infilecnt) + dz->iparam[WRAP_LBUF_SMPXS];
		if(dz->iparam[WRAP_CHANNELS])				
			overall_size += chans * dz->iparam[WRAP_BUF_SMPXS];					/* IF multichan, also allow for bufxs in multichan inbuf */

		if(overall_size * sizeof(float)<0) {
			sprintf(errstr,"INSUFFICIENT MEMORY for sound buffers.\n");	/* arithmetic overflow */
			return(MEMORY_ERROR);
		}
		if((dz->bigbuf=(float *)malloc(overall_size * sizeof(float)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
			return(MEMORY_ERROR);
		}
					/* SET SIZE OF inbuf, outbuf, AND Lbuf (FOR CALCS) */
		outbuflen   = dz->buflen;
		outbuflen *= dz->out_chans;
		if(dz->iparam[WRAP_CHANNELS]) {
			multichan_buflen = dz->buflen * chans;
			multichan_bufxs  = dz->iparam[WRAP_BUF_SMPXS] * chans;
		}										 
		dz->iparam[WRAP_LONGS_BUFLEN] = outbuflen;			/* Longs buffer is same size as obuf */
		if(dz->iparam[WRAP_LBUF_SMPXS] > dz->iparam[WRAP_LONGS_BUFLEN])
			continue;
		break;
	}
				/* DIVIDE UP ALLOCATED MEMORY IN SPECIALISED BUFFERS */	
																	
	if(dz->iparam[WRAP_CHANNELS]) {				 					/* sbuf : extra multichan input buffer, if required */						 
		dz->sampbuf[WRAP_SBUF]   = dz->bigbuf;									 
		dz->sampbuf[THIS_BUF(0)] = dz->sampbuf[WRAP_SBUF] + multichan_buflen + multichan_bufxs;			 
	} else												 
		dz->sampbuf[THIS_BUF(0)] = dz->bigbuf;						
	dz->sbufptr[THIS_BUF(0)]  = dz->sampbuf[THIS_BUF(0)] + dz->buflen;	
	dz->sampbuf[THIS_IBUF(0)] = dz->sampbuf[THIS_BUF(0)] + dz->iparam[WRAP_BUF_SMPXS]; 
	tailend     		      = dz->sbufptr[THIS_BUF(0)] + dz->iparam[WRAP_BUF_SMPXS];  
	for(n=1;n<dz->infilecnt;n++) {
		dz->sampbuf[THIS_BUF(n)]  = tailend;								   /* Lbuf: buffer for calculations */
		dz->sbufptr[THIS_BUF(n)]  = dz->sampbuf[THIS_BUF(n)] + dz->buflen;	
		dz->sampbuf[THIS_IBUF(n)] = dz->sampbuf[THIS_BUF(n)] + dz->iparam[WRAP_BUF_SMPXS]; 
		tailend     		      = dz->sbufptr[THIS_BUF(n)] + dz->iparam[WRAP_BUF_SMPXS]; 
	}
	dz->fptr[WRAP_LBUF]     = tailend;								   
	dz->fptr[WRAP_LBUFEND]  = dz->fptr[WRAP_LBUF] + dz->iparam[WRAP_LONGS_BUFLEN];
	dz->fptr[WRAP_LTAILEND] = dz->fptr[WRAP_LBUFEND] + dz->iparam[WRAP_LBUF_SMPXS];
	dz->fptr[WRAP_LBUFMID]  = dz->fptr[WRAP_LBUF]    + dz->iparam[WRAP_LBUF_SMPXS];
	dz->sampbuf[WRAP_OBUF]  = dz->fptr[WRAP_LTAILEND];		 		    

								/* INITIALISE BUFFERS */

	memset((char *)dz->bigbuf,0,overall_size * sizeof(float));
	return(FINISHED);
}

/*  INPUT BUFFERS :-		
 *
 *	|-----------BUFLEN-----------|
 *
 *	buf      ibuf		   bufend    tailend
 *	|_________|__________________|buf_smpxs|  ..... (obuf->)
 *					             /
 *	|buf_smpxs|	          <<-COPY_________/
 *
 *		      |-----------BUFLEN-----------|
 *
 *
 *
 *  OUTPUT LONGS BUFFER:-
 *
 *	Lbuf	   Lbufmid	  Lbufend
 *	|____________|_______________|_Lbuf_smpxs_|
 *				                 /
 *	|_Lbuf_smpxs_|         <<-COPY___________/	
 *
 */

/****************************** WRAPPAGE_PREPROCESS ****************************/

int wrappage_preprocess(dataptr dz)
{
	int exit_status, orig_infilecnt;
	if((dz->iparray[WRAP_PERM] = (int *)malloc(dz->infilecnt * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for permutation array for wrappage.\n");
		return(MEMORY_ERROR);
	}
	dz->iparray[WRAP_PERM][0] = dz->infilecnt + 1;	/* impossible value to initialise perm */
	orig_infilecnt = dz->infile->channels;
	dz->infile->channels = dz->iparam[0];
	if((dz->outfilename = (char *)malloc(strlen("cdptest1.wav") + 1)) == NULL) {
		sprintf(errstr,"Insufficient memory for temporary file name.\n");
		return MEMORY_ERROR;
	}
	strcpy(dz->outfilename,"cdptest1.wav");
	if((exit_status = create_sized_outfile(dz->outfilename,dz))<0)
		return(exit_status);
	dz->infile->channels = orig_infilecnt;
	return(FINISHED);
}

/************************** CHECK_SPATIALISATION_DATA ************************************/

int check_spatialisation_data(dataptr dz)
{
	int imaxspaceh = 0, n;
	double maxspaceh = 0.0;
	double *p;

	if(!dz->zeroset) {
		maxspaceh = -DBL_MAX;
		p = dz->parray[WRAP_CENTRE];
		p++;
		for(n=0;n<dz->ringsize;n++) {
			if(*p > maxspaceh)
				maxspaceh = *p;
			p+=3;
		}
	} else
		maxspaceh = dz->wrapcentre;
	imaxspaceh = (int)ceil(maxspaceh);

	if(imaxspaceh > dz->out_chans)  {
		sprintf(errstr,"SPATIALIASATION CENTRE MAX VALUE (%lf) INCOMPATIBLE WITH OUTPUT CHANNEL-CNT (%d)\n",maxspaceh,dz->iparam[WRAP_OUTCHANS]);
		return(DATA_ERROR);
	}
	return(FINISHED);
}

/********************************* WRAPPAGE *************************************/

int wrappage(dataptr dz)
{
	int    exit_status;
	int    firsttime = TRUE, thissnd = 0;
	int    nctr = 0;	 /* normalisation vals counter */
	int   absicnt_per_chan = 0, absocnt_per_chan = 0;
	float  *iptr;
	float   *Fptr = dz->fptr[WRAP_LBUF];
 	int   vals_to_write, total_ssampsread;
	double sr = (double)dz->infile->srate;
	double inverse_sr = 1.0/sr;
	float   *maxwrite = dz->fptr[WRAP_LBUF];	/* pointer to last sample created */
	if(sloom)
		dz->total_samps_read = 0;
	dz->itemcnt = 0;

	if((exit_status = read_samps_wrappage(firsttime,dz))<0)
		return(exit_status);
	iptr = dz->sampbuf[THIS_BUF(thissnd)];
	total_ssampsread = dz->ssampsread;	
	display_virtual_time(0L,dz);
	do {
		if((exit_status = granulate(&thissnd,&absicnt_per_chan,&absocnt_per_chan,&iptr,&Fptr,
									inverse_sr,&total_ssampsread,&maxwrite,&nctr,dz))<0)
			return(exit_status);
	} while(exit_status==CONTINUE);

	vals_to_write = maxwrite - dz->fptr[WRAP_LBUF];
	while(vals_to_write > dz->iparam[WRAP_LONGS_BUFLEN]) {					   
		if((exit_status = write_samps_granula(dz->iparam[WRAP_LONGS_BUFLEN],&nctr,dz))<0)
			return(exit_status);
		memmove((char *)dz->fptr[WRAP_LBUF],(char *)dz->fptr[WRAP_LBUFEND],
			dz->iparam[WRAP_LBUF_SMPXS] * sizeof(float));
		memset((char *)dz->fptr[WRAP_LBUFMID],0,dz->iparam[WRAP_LONGS_BUFLEN] * sizeof(float));
		vals_to_write -= dz->iparam[WRAP_LONGS_BUFLEN];
	}
	if(vals_to_write > 0) {
		if((exit_status = write_samps_granula(vals_to_write,&nctr,dz))<0)
			return(exit_status);
	}
	if(dz->total_samps_written <= 0) {
		sprintf(errstr,"SOURCE POSSIBLY TOO SHORT FOR THIS OPTION: Try 'Full Monty'\n");
		return(GOAL_FAILED);
	}
	display_virtual_time(0,dz);
	dz->infile->channels = dz->iparam[WRAP_OUTCHANS];	// setup NOW for headwrite, as headwrite uses dz->infile->channels as its OUTCHAN count
	return renormalise(nctr,dz);
}

/***************************** RENORMALISE ****************************
 *
 * (1)	Find smallest normalising factor S (creating gretest level reduction).
 * (2)	For each normalising factor N, find value which will bring it
 *	down to S. That is S/N. Reinsert these values in the normalising
 *	factor array.
 *	REnormalising with these factors, will ensure whole file is normalised
 *	using same factor (S), which is also the smallest factor required.
 * (3)	Seek to start of outfile.
 * (4)	Set infile pointer to same as outfile pointer, so that read_samps()
 *	now reads from OUTFILE.
 * (5)	Reset samps_read counter.
 * (6)	Size of buffers we read depends on whether we output a stereo
 *	file or a mono file.
 * (7) 	Reset normalisation-factor array pointer (m).
 *	While we still have a complete buffer to read..
 * (8)  Read samps into output buffer (as, if output is stereo, this
 *	may be larger than ibuf, and will accomodate the data).
 * (9)	Renormalise all values.
 * (10)	Increment pointer to normalisation factors.
 * (11)	Seek to start of current buffer, in file.
 * (12)	Overwrite data in file.
 * (13)	Re-seek to end of current buffer, in file.
 * (14)	Calcualte how many samps left over at end, and if any...
 * (16)	Read last (short) buffer.
 * (17)	Set buffer size to actual number of samps left.
 * (18)	Renormalise.
 * (19)	Seek to start of ACTUAL buffer in file.
 * (20)	Write the real number of samps left in buffer.
 * (21) Check the arithmetic.
 * (22) Restore pointer to original infile, so finish() works correctly.
 */
  
int renormalise(int nctr,dataptr dz)
{
	int exit_status;
	int n, m = 0;
	int total_samps_read = 0;
	int samp_total = dz->total_samps_written, samps_remaining, last_total_samps_read;
	double min_norm  = dz->parray[WRAP_NORMFACT][0];	/* 1 */
	float  *s = NULL;
	double nf;
	int   cnt;
//TW NEW MECHANISM writes renormalised vals from original file created (tempfile) into true outfile		
	if(sndcloseEx(dz->ifd[0]) < 0) {
		sprintf(errstr, "WARNING: Can't close input soundfile\n");
		return(SYSTEM_ERROR);
	}
	if(sndcloseEx(dz->ofd) < 0) {
		sprintf(errstr, "WARNING: Can't close output soundfile\n");
		return(SYSTEM_ERROR);
	}
	if((dz->ifd[0] = sndopenEx(dz->outfilename,0,CDP_OPEN_RDWR)) < 0) {	   /*RWD Nov 2003 need RDWR to enable sndunlink to work */
		sprintf(errstr,"Failure to reopen file %s for renormalisation.\n",dz->outfilename);
		return(SYSTEM_ERROR);
	}
	if((sndseekEx(dz->ifd[0],0,0)<0)){        
        sprintf(errstr,"sndseek() failed\n");
        return SYSTEM_ERROR;
    }
	if(!sloom){
        // RWD
        //printf("dz->wordstor[0] = %s\n",dz->wordstor[0]);
		//dz->wordstor[0][strlen(dz->wordstor[0]) -9] = ENDOFSTR;
    }
	if((exit_status = create_sized_outfile(dz->wordstor[0],dz))<0) {
		sprintf(errstr,"Failure to create file %s for renormalisation.\n",dz->wordstor[0]);
		return(exit_status);
	}
	s = dz->sampbuf[WRAP_OBUF];
	dz->total_samps_written = 0;
	for(m=1;m<nctr;m++) {
		if((dz->parray[WRAP_NORMFACT])[m] < min_norm)
			min_norm  = (dz->parray[WRAP_NORMFACT])[m];
	}

	if(min_norm < 1.0) {
		sprintf(errstr,"Renormalising by %lf\n",min_norm);
		print_outmessage_flush(errstr);
		for(m=0;m<nctr;m++)								/* 2 */
			(dz->parray[WRAP_NORMFACT])[m] = min_norm/(dz->parray[WRAP_NORMFACT])[m];
	}
 //   if(!sloom) {
 //       sprintf(errstr,"\nCopying to output\n");
 //       print_outmessage_flush(errstr);
 //   }
//TW new mechanism: lines not needed
	dz->total_samps_read = 0;						/* 5 */
	display_virtual_time(0L,dz);					/* works on dz->total_samps_read here, so param irrelevant */

	dz->buflen *= dz->outfile->channels;			/* 6 */												
	m = 0;											/* 7 */
	cnt = dz->buflen;
    
	while(dz->total_samps_read + dz->buflen < samp_total) {
		//if(!sloom) {
		//	sprintf(errstr,"\nCopying to output\n");
		//	print_outmessage_flush(errstr);
		//}
		if((exit_status = read_samps(s,dz))<0) {
			close_and_delete_tempfile(dz->outfilename,dz);
			return(exit_status);
		}											/* 8 */
		total_samps_read += dz->ssampsread;
		if(min_norm < 1.0) {
			nf  = (dz->parray[WRAP_NORMFACT])[m];
			for(n=0;n<cnt;n++)							/* 9 */
				s[n] = /*round*/ (float)((double)s[n] * nf);
			m++;										/* 10 */
		}
		dz->process = BRASSAGE;	/* Forces correct time-bar display */
		if((exit_status = write_samps(s,dz->buflen,dz))<0) {
			close_and_delete_tempfile(dz->outfilename,dz);
			return(exit_status);					/* 12 */
		}
		dz->process = WRAPPAGE;
	}												/* 14 */
	if((samps_remaining = samp_total - dz->total_samps_read)>0) {
		last_total_samps_read =  dz->total_samps_read;
		if((exit_status = read_samps(s,dz))<0) {
			close_and_delete_tempfile(dz->outfilename,dz);
			return(exit_status);					/* 16 */
		}
		dz->buflen = samps_remaining;	/* 17 */
		if(min_norm < 1.0) {
			nf  = (dz->parray[WRAP_NORMFACT])[m];
			for(n=0;n<dz->buflen;n++)					/* 18 */
				s[n] = /*round*/(float) ((double)s[n] * nf);
		}
		if(samps_remaining > 0) {
			dz->process = BRASSAGE;
			if((exit_status = write_samps(s,samps_remaining,dz))<0) {
				close_and_delete_tempfile(dz->outfilename,dz);
				return(exit_status);					/* 20 */
			}
			dz->process = WRAPPAGE;
		}
	}
	if(dz->total_samps_written != samp_total) {		/* 21 */
		sprintf(errstr,"ccounting problem: Renormalise()\n");
		close_and_delete_tempfile(dz->outfilename,dz);
		return(PROGRAM_ERROR);
	}
	close_and_delete_tempfile(dz->outfilename,dz);
	return(FINISHED);
}

/*************************** GRANULATE *********************************
 * iptr     = advancing base-pointer in input buffer.
 * iiptr    = true read-pointer in input buffer.
 * absicnt_per_chan  = advancing-base-counter in input stream, counting TOTAL samps.
 * absiicnt_per_chan = true counter in input stream, counting TOTAL samps to write pos.
 * Fptr     = advancing base-pointer in output buffer.
 * FFptr    = true write pointer in output buffer.
 * absocnt_per_chan  = advancing-base-counter in output stream, measuring OUTsize.
 */

/* rwd: some major changes here to clear bugs */
/* tw:  some more major changes here to clear implementation errors */

//TW removed redundant 'sr' parameter
int granulate(int *thissnd,int *aipc,int *aopc,float **iiiiptr,float **LLLLptr,
		double inv_sr,int *samptotal,float **maxwrite,int *nctr,dataptr dz)
{
	int exit_status;
	int  absicnt_per_chan = *aipc, absocnt_per_chan = *aopc;
	float *iiptr, *iptr = *iiiiptr, *endbufptr = NULL, *startbufptr = NULL, *thisbuf = NULL;
	float  *FFptr, *Fptr = *LLLLptr;
	int  isauspos, iisauspos;
	float   aamp = -1;
	int   bspl = 0, espl = 0, lastsnd;
	int   firsttime = FALSE;
	int   gsize_per_chan, ostep_per_chan, istep_per_chan, absiicnt_per_chan, rang_per_chan;
	int  smpscat_per_chan;
	int   resetskip = 0;
	double  itime = (double)absicnt_per_chan * inv_sr;
	double  otime = (double)absocnt_per_chan * inv_sr;
	double	transpos = 1.0, position;
	int saved_total_samps_read = 0;
	int chana, chanb;

	if((exit_status = read_timevarying_data(itime,otime,dz))<0)
		return(exit_status);
	gsize_per_chan = set_ivalue(dz->iparray[WRAP_FLAGS][WRAP_GRAINSIZE_FLAG],WRAP_GRAINSIZE,WRAP_HGRAINSIZE,WRAP_GRANGE,dz);

	if(absicnt_per_chan + gsize_per_chan >= dz->iparam[WRAP_ORIG_SMPSIZE])
		return(FINISHED);

	ostep_per_chan = set_outstep(gsize_per_chan,dz);

	if(dz->iparam[WRAP_OUTLEN]>0 && (absocnt_per_chan>=dz->iparam[WRAP_OUTLEN]))
		return(FINISHED); 	/* IF outfile LENGTH SPECIFIED HAS BEEN MADE: EXIT */

	FFptr = Fptr;
	if(dz->iparray[WRAP_FLAGS][WRAP_SCATTER_FLAG])	{ /* If grains scattered, scatter FFptr */
		smpscat_per_chan = do_scatter(ostep_per_chan,dz);	
		FFptr += (smpscat_per_chan * dz->iparam[WRAP_OUTCHANS]);	
	}
	if(FFptr < dz->fptr[WRAP_LBUF]) {
		sprintf(errstr,"Array overrun 1: granula()\n");
		return(PROGRAM_ERROR); /* FFptr can be outside the Lbuffer because Fptr is outside it */
	}
	if(FFptr >= dz->fptr[WRAP_LTAILEND]) {
		if((exit_status = write_samps_granula(dz->iparam[WRAP_LONGS_BUFLEN],nctr,dz))<0)
			return(exit_status);
/* IF CURRENT POINTER AT END OF LBUF WRITE SAMPS, WRAP AROUND POINTERS */
		memmove((char *)dz->fptr[WRAP_LBUF],(char *)dz->fptr[WRAP_LBUFEND],
			dz->iparam[WRAP_LBUF_SMPXS] * sizeof(float));
		memset((char *)dz->fptr[WRAP_LBUFMID],0,dz->iparam[WRAP_LONGS_BUFLEN] * sizeof(float));
		FFptr    -= dz->iparam[WRAP_LONGS_BUFLEN];	
		if((Fptr -= dz->iparam[WRAP_LONGS_BUFLEN])  < dz->fptr[WRAP_LBUF]) {
			sprintf(errstr,"Array overrun 2: granula()\n");
			return(PROGRAM_ERROR);
		}
		*maxwrite -= dz->iparam[WRAP_LONGS_BUFLEN];
	}

	istep_per_chan    = set_instep(ostep_per_chan,dz);	
	if(istep_per_chan==0 && dz->iparam[WRAP_OUTLEN]==0) {
		sprintf(errstr,"velocity or instep has become so small that file will be infinitely long!!\n"
					   "Try slightly increasing your very small values.\n");
		return(GOAL_FAILED);
	}
	iiptr    = iptr;

	lastsnd   = *thissnd;
	isauspos  = iptr  - dz->sampbuf[THIS_BUF(lastsnd)];
	iisauspos = iiptr - dz->sampbuf[THIS_BUF(lastsnd)];
	*thissnd  = get_next_insnd(dz);
	iiptr     = dz->sampbuf[THIS_BUF(*thissnd)] + iisauspos;
	iptr      = dz->sampbuf[THIS_BUF(*thissnd)] + isauspos;

	absiicnt_per_chan = absicnt_per_chan;
	if(dz->iparray[WRAP_FLAGS][WRAP_RANGE_FLAG]) {
		rang_per_chan      = set_range(absiicnt_per_chan,dz);		/* RESET iiptr etc WITHIN SEARCHRANGE */	
		absiicnt_per_chan -= rang_per_chan;
		iiptr             -= rang_per_chan * dz->iparam[WRAP_INCHANS];
	}
	endbufptr   = dz->sbufptr[THIS_BUF(*thissnd)];	
	startbufptr = dz->sampbuf[THIS_BUF(*thissnd)];		
	if(iiptr >= endbufptr) {
		while(iiptr >= endbufptr) {
			if((read_samps_wrappage(firsttime,dz))<0)					/* IF iiptr OFF END OF IBUF */
				return(exit_status);
			*samptotal += dz->ssampsread;
			if(dz->ssampsread<=0)
				return(FINISHED);
					/* READ SAMPS, WRAP BACK POINTER */
			iiptr -= dz->buflen; 	
			iptr  -= dz->buflen;
		}
	} else if(iiptr < startbufptr) {								/* IF RANGE TAKES US BAK OUT OF THIS BUF, */

		if(sloom)
			saved_total_samps_read = dz->total_samps_read;	/* saved so display_virtual_time() works during baktrak!! */
		if((resetskip = *samptotal - dz->iparam[WRAP_SAMPS_IN_INBUF])<0) {	/* SET RESETSKIP TO START OF CURRENT BUFFER */
			sprintf(errstr,"Error in baktraking: granula()\n");
			return(PROGRAM_ERROR);
		}
		if((exit_status = baktrak_wrappage(*thissnd,*samptotal,absiicnt_per_chan,&iiptr,dz))<0)
			return(exit_status);									/* SEEK BACKWDS, & RESET iiptr In NEW BUF */
		if(sloom)
			dz->total_samps_read = saved_total_samps_read;		/* restore so its not changed by the baktraking!! */
	}
	if(dz->iparray[WRAP_FLAGS][WRAP_AMP_FLAG])
		/* RWD was set_ivalue*/
		aamp = (float) set_dvalue(dz->iparray[WRAP_FLAGS][WRAP_AMP_FLAG],WRAP_AMP,WRAP_HAMP,WRAP_ARANGE,dz);
									/* GET SPLICE VALUES */
	bspl = set_ivalue(dz->iparray[WRAP_FLAGS][WRAP_BSPLICE_FLAG],WRAP_BSPLICE,WRAP_HBSPLICE,WRAP_BRANGE,dz);
	espl = set_ivalue(dz->iparray[WRAP_FLAGS][WRAP_ESPLICE_FLAG],WRAP_ESPLICE,WRAP_HESPLICE,WRAP_ERANGE,dz);
	thisbuf = dz->sampbuf[THIS_BUF(*thissnd)];

	if(!make_multichan_grain(thisbuf,&iiptr,aamp,gsize_per_chan,&transpos,dz))	
		return(FINISHED);											/* COPYGRAIN TO GRAINBUF,INCLUDING ANY PSHIFT */
		do_multichan_splices(gsize_per_chan,bspl,espl,dz);			/* DO SPLICES IN GRAINBUF */
	position = get_wrappage_position(dz);
	chana = (int)floor(position);
	position  -= (double)chana;	//	position is relative position between 2 adjacent out-chans
	chanb = chana + 1;			//	chanb is adjacent to chana
	if(chana > dz->out_chans)	//	chana beyond last lspkr wraps around to 1st lspkr
		chana = 1;		
	else if(chana < 1)			//	chana below 1st loudspeaker wraps around to last-lspkr
		chana = dz->out_chans;
	if((exit_status = write_multichan_grain(position,chana,chanb,maxwrite,&Fptr,&FFptr,gsize_per_chan,nctr,dz))<0)
		return(exit_status);
	if(sloom)
		saved_total_samps_read = dz->total_samps_read;	/* saved so not disturbed by restoring of baktrakd-from buffer */

	if(resetskip && (exit_status = reset_wrappage(*thissnd,resetskip,dz))<0)	
		return(exit_status);
	if(sloom)
		dz->total_samps_read = saved_total_samps_read;
	iptr += (istep_per_chan * dz->iparam[WRAP_INCHANS]);		/* MOVE FORWARD IN postmixdown input-buffer, which is either MONO or multichannel */
	absicnt_per_chan += istep_per_chan;		/* rwd: moved here from top of input section */

	Fptr += ostep_per_chan * dz->iparam[WRAP_OUTCHANS];	/* Move forward in output buffer, which can be mono, spatialised-stereo or multichannel */
		
			/*      so we don't lose first grain */
	absocnt_per_chan += ostep_per_chan;		

	*aipc = absicnt_per_chan;	/* RETURN VALUES OF RETAINED VARIABLES */
	*aopc = absocnt_per_chan;
	*iiiiptr = iptr;
	*LLLLptr = Fptr;
	return(CONTINUE);
}

/***************************** MAKE_MULTICHAN_GRAIN ***************************
 * iicnt = RELATIVE read-pointer in input buffer (starts at 0).
 */

int make_multichan_grain(float *b,float **iiptr,float aamp,int gsize_per_chan,double *transpos,dataptr dz)
{
	int n, k;
	int chans = dz->iparam[WRAP_INCHANS];		// WRAP_INCHANS = no of chans in input grain, i.e. mono if its been mixed down to mono
	double flcnt;								//	but = dz->infile->channels, if not
	int iicnt, real_gsize = gsize_per_chan * chans;
	double flcnt_frac;
	float *s = *iiptr, *gbufptr = dz->extrabuf[WRAP_GBUF];
	if(aamp>=0) {
		if(!dz->iparray[WRAP_FLAGS][WRAP_PITCH_FLAG]) {
			if(real_gsize >= dz->iparam[WRAP_SAMPS_IN_INBUF])	  /* JUNE 1996 */
				return(0);
			for(n=0;n<real_gsize;n++)   /* COPY GRAIN TO GRAINBUF & RE-LEVEL ETC */
				*gbufptr++ = (*s++ * aamp);
		} else {			
			iicnt = (s - b)/chans;
			*transpos = set_dvalue(dz->iparray[WRAP_FLAGS][WRAP_PITCH_FLAG],WRAP_PITCH,WRAP_HPITCH,WRAP_PRANGE,dz);
			*transpos = pow(2.0,*transpos);
			if((((int)(gsize_per_chan * *transpos)+1) + iicnt) * chans >= dz->iparam[WRAP_SAMPS_IN_INBUF])	  /* JUNE 1996 */
				return(0);
			flcnt = (double)iicnt;
			flcnt_frac = flcnt - (double)iicnt;
			for(n=0;n<gsize_per_chan;n++) {
				for(k = 0; k < chans;k++) {
					*gbufptr++ = interp_gval_with_amp(s,flcnt_frac,dz->iparam[WRAP_INCHANS],aamp);
					s++;
				}
				flcnt += *transpos;  
				iicnt = (int)flcnt;
				s = b + (iicnt * chans);
				flcnt_frac = flcnt - (double) iicnt;	    	    
			}
		}
	} else {		/* NO CHANGE IN AMPLITUDE */
		if(!dz->iparray[WRAP_FLAGS][WRAP_PITCH_FLAG]) {
			if(real_gsize >= dz->iparam[WRAP_SAMPS_IN_INBUF])	  /* JUNE 1996 */
				return(0);
			for(n=0;n<real_gsize;n++)
				*gbufptr++ = *s++;
		} else {
			iicnt = (s - b)/chans;
			*transpos = set_dvalue(dz->iparray[WRAP_FLAGS][WRAP_PITCH_FLAG],WRAP_PITCH,WRAP_HPITCH,WRAP_PRANGE,dz);
			*transpos = pow(2.0,*transpos);
			if((((int)(gsize_per_chan * *transpos)+1) + iicnt) * chans >= dz->iparam[WRAP_SAMPS_IN_INBUF])	  /* JUNE 1996 */
				return(0);
			flcnt = (double)iicnt;
 			flcnt_frac = flcnt - (double)iicnt;
			for(n=0;n<gsize_per_chan;n++) {
				for(k=0;k < chans;k++) {
					*gbufptr++ = interp_gval(s,flcnt_frac,dz->iparam[WRAP_INCHANS]);
					s++;
				}
				flcnt += *transpos;  
				iicnt = (int)flcnt;	    	    
				s = b + (iicnt * chans);
				flcnt_frac = flcnt - (double)iicnt;	    	    
			}
		}
	}
	*iiptr = s;
	return(1);
}

/****************************** DEHOLE ************************/

#define NONLIN  0.5
#define DEVIATE 0.25

double dehole(double pos)
{
	double comp = 1.0 - (fabs((pos * 2.0) - 1.0));	/* 1 */
	comp = pow(comp,NONLIN);						/* 2 */
	comp *= DEVIATE;								/* 3 */
	comp += (1.0 - DEVIATE);						/* 4 */
	return(comp);
}


/**************************** DO_SCATTER *****************************
 *  scatter forward by fraction of randpart of 1 event separation.
 *
 *		  Possible scatter  |      	|-------------->|     
 *		    Range selected  |		|------>      	|	
 *	Random choice within range  |		|--->      	|	
 */

int do_scatter(int ostep_per_chan,dataptr dz)
{
	double scat  = dz->param[WRAP_SCATTER] * drand48();
	int   smpscat_per_chan = round((double)ostep_per_chan * scat);
	return(smpscat_per_chan);
}				

/******************************** WRITE_SAMPS_GRANULA ****************************
 *
 * Normalise each buffer before writing to obuf, then to file.
 * Save the normalisation factor in array normfact[].
 */

int write_samps_granula(int k,int *nctr,dataptr dz)
{
	int exit_status;
	/*int*/double val = 0.0 /*0L, max_long = 0L*/;
	int n;
	double thisnorm = 1.0, max_double = 0.0;
	float *s = NULL;
	float  *l = dz->fptr[WRAP_LBUF];
	s = dz->sampbuf[WRAP_OBUF];

	for(n=0;n<k;n++) {
		if((val = fabs(l[n])) > max_double)
			max_double = val;
	}
	if(/*max_long*/max_double > F_MAXSAMP) {
		thisnorm = (double)F_MAXSAMP/max_double;
		for(n=0;n<k;n++)
			s[n] =  (float) ((double)(l[n]) * thisnorm);
	} else {
		for(n=0;n<k;n++)
			s[n] =  l[n];
	}
	dz->process = BRASSAGE;	/* Forces correct time-bar display */
	if((exit_status = write_samps(s,k,dz))<0)
		return(exit_status);
	dz->process = WRAPPAGE;
	(dz->parray[WRAP_NORMFACT])[(*nctr)++] = thisnorm;
	if(*nctr >= dz->iparam[WRAP_ARRAYSIZE]) {
		dz->iparam[WRAP_ARRAYSIZE] += BIGARRAY;
		if((dz->parray[WRAP_NORMFACT] = 
		(double *)realloc((char *)(dz->parray[WRAP_NORMFACT]),dz->iparam[WRAP_ARRAYSIZE]*sizeof(double)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to reallocate normalisation array.\n");
			return(MEMORY_ERROR);
		}
	}
	return(FINISHED);
}

/************************** DO_MULTICHAN_SPLICES *****************************/

void do_multichan_splices(int gsize_per_chan,int bspl,int espl,dataptr dz)
{
	if(dz->iparam[WRAP_IS_BTAB] && (dz->iparam[WRAP_BSPLICE] < gsize_per_chan/2))
		do_multichan_btab_splice(dz);
	else
		do_multichan_bsplice(gsize_per_chan,dz,bspl);
	if(dz->iparam[WRAP_IS_ETAB] && (dz->iparam[WRAP_ESPLICE] < gsize_per_chan/2))
		do_multichan_etab_splice(gsize_per_chan,dz);
	else
		do_multichan_esplice(gsize_per_chan,dz,espl);
}

/************************** DO_MULTICHAN_BTAB_SPLICE ****************************/

void do_multichan_btab_splice(dataptr dz)
{
	int n, j, k = dz->iparam[WRAP_BSPLICE];
	double *d;
	float *gbufptr = dz->extrabuf[WRAP_GBUF];   
	
	if(k==0)
		return;
	d = dz->parray[WRAP_BSPLICETAB];
	for(n=0;n<k;n++) {
		for(j=0;j<dz->iparam[WRAP_INCHANS];j++) {
			*gbufptr = (float)/*round*/(*gbufptr * *d); 
			gbufptr++;
		}
		d++;
	}
}

/************************** DO_MULTICHAN_BSPLICE ****************************
 *
 * rwd: changed to avoid f/p division, added exponential option
 */
void do_multichan_bsplice(int gsize_per_chan,dataptr dz,int bspl)
{
	double dif,val,lastval,length,newsum,lastsum,twodif;
	int n, j, k = min(bspl,gsize_per_chan);
	float *gbufptr = dz->extrabuf[WRAP_GBUF];
	int chans = dz->iparam[WRAP_INCHANS];
	if(k==0)
		return;
	val = 0.0;
	length = (double)k;
	if(!dz->vflag[WRAP_EXPON]){
		dif = 1.0/length;
		lastval = dif;
		for(j= 0; j<chans;j++)
			*gbufptr++ = (float)val;
		for(j= 0; j<chans;j++) {
			*gbufptr = (float)/*round*/(*gbufptr * lastval);
			gbufptr++;   
		}
		for(n=2;n<k;n++) {
			val = lastval + dif;
			lastval = val;
			for(j= 0; j<chans;j++) {
				*gbufptr = (float) /*round*/(*gbufptr * val);
				gbufptr++;
			}
		}
	}  else {			/* fast quasi-exponential */
		dif = 1.0/(length*length);
		twodif = dif * 2.0;
		lastsum = 0.0;
		lastval = dif;
		for(j=0;j<chans;j++)
			*gbufptr++ = (float)val;/* mca - round or truncate? */
		for(j=0;j<chans;j++) {
			*gbufptr = (float) /*round*/(*gbufptr * lastval);	 /*** fixed MAY 1998 ***/
			gbufptr++;
		}
		for(n=2;n<k;n++) {
 			newsum = lastsum + twodif;
 			val = lastval + newsum + dif;
			for(j=0;j<chans;j++) {
				*gbufptr = (float) /*round*/(*gbufptr * val);
				gbufptr++;
			}
			lastval = val;
			lastsum = newsum;
		}
	}
}

/************************** DO_MULTICHAN_ETAB_SPLICE ****************************/

void do_multichan_etab_splice(int gsize_per_chan,dataptr dz)
{
	int n, j, k = dz->iparam[WRAP_ESPLICE];
	double *d;
	int chans = dz->iparam[WRAP_INCHANS];
	float *gbufptr = dz->extrabuf[WRAP_GBUF] + ((gsize_per_chan - k) * chans);   
	if(k==0)
		return;
	d = dz->parray[WRAP_ESPLICETAB];
	for(n=0;n<k;n++) {
		for(j=0;j<chans;j++) {
			*gbufptr = (float) /*round*/(*gbufptr * *d); 
			gbufptr++;
		}
		d++;
	}
}

/************************** DO_MULTICHAN_ESPLICE ****************************/
/* rwd: changed to avoid f/p division, added exponential option */

void do_multichan_esplice(int gsize_per_chan,dataptr dz,int espl)
{
	double dif,val,lastval,length,newsum,lastsum,twodif;
	int n, j, k = min(espl,gsize_per_chan);
	int chans = dz->iparam[WRAP_INCHANS];
	int real_gsize = gsize_per_chan * chans;
	float *gbufptr = dz->extrabuf[WRAP_GBUF] + real_gsize;
	if(k==0)
		return;
	val = 0.0;
	length = (double) k;
	if(!dz->vflag[WRAP_EXPON]) {
		dif = 1.0/length;
		lastsum = dif;
		for(j=0;j<chans;j++) {
			gbufptr--;
			*gbufptr = (float)val;
		}
		for(j=0;j<chans;j++) {
			gbufptr--;
			*gbufptr = (float) /*round*/(*gbufptr * lastsum);   
		}
		for(n=k-3;n>=0;n--) {
			val = lastsum + dif;
			lastsum = val;
			for(j=0;j<chans;j++) {
				gbufptr--;
				*gbufptr = (float) /*round*/(*gbufptr * val);
			}
		}
	} else {			/* fast quasi-exponential */ 
		dif = 1.0/(length * length);
		twodif = dif * 2.0;
		lastsum = 0.0;
		lastval = dif;
		for(j=0;j<chans;j++) {
			gbufptr--;
			*gbufptr = (float)val;
		}
		for(j=0;j<chans;j++) {
			gbufptr--;
			*gbufptr = (float) /*round*/(*gbufptr * lastval);
		}
		for(n=k-3;n>=0;n--) {
			newsum =lastsum + twodif;
			val = lastval + newsum + dif;
			for(j=0;j<chans;j++) {
				gbufptr--;
				*gbufptr = (float)/*round*/(*gbufptr * val);
			}
			lastval = val;
			lastsum = newsum;
		}
	}
}

/************************** INTERP_GVAL_WITH_AMP ****************************/

float interp_gval_with_amp(float *s,double flcnt_frac,int chans,float ampp)
{
	/*long*/float tthis = /*(long)*/ *s * ampp;
	/*long*/float next = /*(long)*/ *(s+chans) * ampp;
	//long val  = this + round((double)(next-this) * flcnt_frac);
	//return(short)((val+TWO_POW_14) >> 15);
	return (float) (tthis + ((next-tthis) * flcnt_frac));
}

/************************** INTERP_GVAL ****************************/

float interp_gval(float *s,double flcnt_frac,int chans)
{
	float tthis = *s;
	float next = *(s+chans);
	float val  = (float)(tthis + /*round*/((double)(next-tthis) * flcnt_frac));
	return(val);
}
	
/*************************** SET_RANGE ******************************/

int set_range(int absiicnt,dataptr dz)
{
	int val;
	val = min(dz->iparam[WRAP_SRCHRANGE],absiicnt);
	val = round((double)val * drand48());
	return(val);
}

/*************************** SET_OUTSTEP ******************************/

int set_outstep(int gsize_per_chan,dataptr dz)		
{							
	double dens;
	int val = 0;
	/* TW 4 :2002 */
	dens = set_dvalue(dz->iparray[WRAP_FLAGS][WRAP_DENSITY_FLAG],WRAP_DENSITY,WRAP_HDENSITY,WRAP_DRANGE,dz);
	val  = round((double)gsize_per_chan/(double)dens);
	return(val);
}

/*************************** SET_INSTEP ******************************/

int set_instep(int ostep_per_chan,dataptr dz)	
{												/* rwd: added range error traps */
	double velocity;
	int istep_per_chan = 0;
 	velocity = set_dvalue(dz->iparray[WRAP_FLAGS][WRAP_VELOCITY_FLAG],WRAP_VELOCITY,WRAP_HVELOCITY,WRAP_VRANGE,dz);
	istep_per_chan =  round(velocity * (double)ostep_per_chan);
	return(istep_per_chan);
}

/*************************** SET_IVALUE ******************************/

int set_ivalue(int flag,int paramno,int hparamno,int rangeno,dataptr dz)
{
	int val = 0;
	switch(flag) {
	case(NOT_SET):
	case(FIXED):
	case(VARIABLE):
		val = dz->iparam[paramno];
		break;
	case(RANGE_VLO):
	case(RANGE_VHI):
	case(RANGE_VHILO):
		dz->iparam[rangeno] = dz->iparam[hparamno] - dz->iparam[paramno];
		/* fall thro */
	case(RANGED):
		val = round((drand48() * dz->iparam[rangeno]) + (double)dz->iparam[paramno]);
		break;
	}    
	return(val);
}

/*************************** SET_DVALUE ******************************/

double set_dvalue(int flag,int paramno,int hparamno,int rangeno,dataptr dz)
{
	double val = 0;
	switch(flag) {
	case(NOT_SET):
	case(FIXED):
	case(VARIABLE):
		val = dz->param[paramno];
		break;
	case(RANGE_VLO):
	case(RANGE_VHI):
	case(RANGE_VHILO):
		dz->param[rangeno] = dz->param[hparamno] - dz->param[paramno];
		/* fall thro */
	case(RANGED):
		val = (drand48() * dz->param[rangeno]) + dz->param[paramno];
		break;
	}    
	return(val);
}

/******************************* READ_SAMPS_SAUSAGE *******************************/

int read_samps_wrappage(int firsttime,dataptr dz)
{
	int exit_status, n;
	int samps_read = INT_MAX;	/* i.e. larger than possible */
	for(n=0;n<dz->infilecnt;n++) {
		if(firsttime) {
			dz->total_samps_read = 0;
			if((exit_status = read_a_specific_large_buf(n,dz))<0)
				return(exit_status);
			samps_read = min(samps_read,dz->ssampsread);
		} else {
			if((exit_status = read_a_specific_normal_buf_with_wraparound(n,dz))<0)
				return(exit_status);
			samps_read = min(samps_read,dz->ssampsread);
		}
	}
	dz->total_samps_read += samps_read;
	if(samps_read==INT_MAX) {
		sprintf(errstr,"Problem reading sound buffers.\n");
		return(PROGRAM_ERROR);
	}
	if(firsttime)
		dz->iparam[WRAP_SAMPS_IN_INBUF] = dz->ssampsread;
	else
		dz->iparam[WRAP_SAMPS_IN_INBUF] = dz->ssampsread + dz->iparam[WRAP_BUF_SMPXS];
	return(FINISHED);
}

/*************************** READ_A_SPECIFIC_LARGE_BUF ******************************/

int read_a_specific_large_buf(int j,dataptr dz)
{
	int bigbufsize = dz->buflen;	   /* RWD odd one, this... */
	int n, m, samps_read, ibufcnt;
	int bufno = THIS_BUF(j), chans = dz->infile->channels;
	double sum;
	bigbufsize += dz->iparam[WRAP_BUF_SMPXS];
	if(chans > 1) {
		bigbufsize *= chans;
		if((samps_read = fgetfbufEx(dz->sampbuf[WRAP_SBUF],bigbufsize,dz->ifd[j],0))<0) {
			sprintf(errstr,"Failed to read samps from file %d.\n",j+1);
			return(SYSTEM_ERROR);
		}
		dz->ssampsread = samps_read;
		bigbufsize /= chans;			   /* RWD still in samps...buflen + smpxs */
		dz->ssampsread /= chans;
		ibufcnt = 0;
		for(n=0;n<dz->ssampsread;n++) {
			sum = 0.0;
			for(m=0;m<chans;m++)
				sum += dz->sampbuf[WRAP_SBUF][ibufcnt++];
			dz->sampbuf[bufno][n] = (float)(sum/(double)chans);
		}
	} else {
		if((samps_read = fgetfbufEx(dz->sampbuf[bufno],bigbufsize,dz->ifd[j],0))<0) {
			sprintf(errstr,"Failed to read samps from file %d.\n",j+1);
			return(SYSTEM_ERROR);
		}
		dz->ssampsread = samps_read;
	}
	return(FINISHED);
}

/*************************** READ_A_SPECIFIC_NORMAL_BUF_WITH_WRAPAROUND ******************************/

int read_a_specific_normal_buf_with_wraparound(int j,dataptr dz)
{
	int bigbufsize = dz->buflen;
	int n,m,samps_read, ibufcnt;
	double sum;
	int bufno  = THIS_BUF(j);
	int ibufno = THIS_IBUF(j), chans = dz->infile->channels;
	memmove((char *)dz->sampbuf[bufno],(char *)(dz->sbufptr[bufno]),dz->iparam[WRAP_BUF_SMPXS] * sizeof(float));
	if(chans > 1) {
		bigbufsize *= chans;
		if((samps_read = fgetfbufEx(dz->sampbuf[WRAP_SBUF],bigbufsize,dz->ifd[j],0))<0) {
			sprintf(errstr,"Failed to read from file %d.\n",j+1);
			return(SYSTEM_ERROR);
		}
		dz->ssampsread = samps_read;
		bigbufsize /= chans;
		dz->ssampsread /= chans;
		ibufcnt = 0;
		for(n=0;n<dz->ssampsread;n++) {
			sum = 0.0;
			for(m=0;m<chans;m++)
				sum += dz->sampbuf[WRAP_SBUF][ibufcnt++];
			dz->sampbuf[ibufno][n] = (float)(sum/(double)chans);
		}
	} else {
		if((samps_read = fgetfbufEx(dz->sampbuf[ibufno],bigbufsize,dz->ifd[j],0))<0) {
			sprintf(errstr,"Failed to read from file %d.\n",j+1);
			return(SYSTEM_ERROR);
		}
		dz->ssampsread = samps_read;
	}
	return(FINISHED);
}

/*************************** BAKTRAK_SAUSAGE ******************************
 *
 *     <------------ baktrak(b) ---------------->
 *
 *     <--(b-x)-->
 *		  		  <------------ x -------------->
 *		 		 |-------- current bufer --------|
 *
 *     |-------- new buffer --------|
 *      <------------x------------->
 *				
 */

int baktrak_wrappage(int thissnd,int samptotal,int absiicnt_per_chan,float **iiptr,dataptr dz)
{
	int  exit_status,  chans = dz->infile->channels;
	int bktrk, new_position;
	unsigned int reset_size = dz->buflen + dz->iparam[WRAP_BUF_SMPXS];
	bktrk = samptotal - (absiicnt_per_chan * dz->iparam[WRAP_INCHANS]);
	*iiptr      = dz->sampbuf[THIS_BUF(thissnd)];

	if((new_position = samptotal - bktrk)<0) {
		fprintf(stdout,"WARNING: Non-fatal program error:\nRange arithmetic problem - 2, in baktraking.\n");
		fflush(stdout);
		new_position = 0;
		*iiptr = dz->sampbuf[THIS_BUF(thissnd)];
	}
    if(chans > 1)   /* IF we've converted from MULTICHAN file */
		new_position *= chans;
	if(sndseekEx(dz->ifd[thissnd],new_position,0)<0) {
		sprintf(errstr,"sndseek error: baktrak_wrappage()\n");
		return(SYSTEM_ERROR);
	}
	memset((char *)dz->sampbuf[THIS_BUF(thissnd)],0,reset_size * sizeof(float));
    if(chans > 1) {
		reset_size *= chans;
    	memset((char *)dz->sampbuf[WRAP_SBUF],0,reset_size * sizeof(float));
	}
	if((exit_status = read_a_specific_large_buf(thissnd,dz))<0)
		return(exit_status);
	dz->iparam[WRAP_SAMPS_IN_INBUF] = dz->ssampsread;
	return(FINISHED);
}

/****************************** RESET_SAUSAGE *******************************/

int reset_wrappage(int thissnd,int resetskip,dataptr dz)
{
	int exit_status, chans = dz->infile->channels;
	unsigned int reset_size = dz->buflen + dz->iparam[WRAP_BUF_SMPXS];

	memset((char *)dz->sampbuf[THIS_BUF(thissnd)],0,reset_size * sizeof(float));
	if(chans > 1) {
		reset_size *= chans;
    	memset((char *)dz->sampbuf[WRAP_SBUF],0,reset_size * sizeof(float));
		resetskip *= chans;
}
	if(sndseekEx(dz->ifd[thissnd],resetskip,0)<0) {
		sprintf(errstr,"sndseek error: reset_wrappage()\n");
		return(SYSTEM_ERROR);
	}
	if((exit_status = read_a_specific_large_buf(thissnd,dz))<0)
		return(exit_status);
	dz->iparam[WRAP_SAMPS_IN_INBUF] = dz->ssampsread;  
	return(FINISHED);
}

/****************************** GET_NEXT_INSND *******************************/

int get_next_insnd(dataptr dz)
{
	if(dz->itemcnt <= 0) {
		perm_wrappage(dz->infilecnt,dz);
		dz->itemcnt = dz->infilecnt;
	}
	dz->itemcnt--;
	return(dz->iparray[WRAP_PERM][dz->itemcnt]);
}

/****************************** PERM_SAUSAGE *******************************/

void perm_wrappage(int cnt,dataptr dz)
{
	int n, t;
	for(n=0;n<cnt;n++) {
		t = (int)(drand48() * (double)(n+1));	 /* TRUNCATE */
		if(t==n)
			prefix(n,cnt-1,dz);
		else
			insert(n,t,cnt-1,dz);
	}
}

/****************************** INSERT ****************************/

void insert(int n,int t,int cnt_less_one,dataptr dz)
{   
	shuflup(t+1,cnt_less_one,dz);
	dz->iparray[WRAP_PERM][t+1] = n;
}

/****************************** PREFIX ****************************/

void prefix(int n,int cnt_less_one,dataptr dz)
{   
	shuflup(0,cnt_less_one,dz);
	dz->iparray[WRAP_PERM][0] = n;
}

/****************************** SHUFLUP ****************************/

void shuflup(int k,int cnt_less_one,dataptr dz)
{   
	int n;
	for(n = cnt_less_one; n > k; n--)
		dz->iparray[WRAP_PERM][n] = dz->iparray[WRAP_PERM][n-1];
}

/*************************** GRAB_AN_APPROPRIATE_BLOCK_OF_WRAPPAGE_MEMORY **************************/

int grab_an_appropriate_block_of_wrappage_memory(int *this_bloksize,int standard_block,int bufdivisor,dataptr dz)
{
	do {
		if((dz->buflen = *this_bloksize) * sizeof(float) < 0) {	/* arithmetic overflow */
			sprintf(errstr,"INSUFFICIENT MEMORY for sound buffers.\n");
			return(MEMORY_ERROR);
		}
		*this_bloksize += standard_block;
		dz->buflen -= (dz->infilecnt * dz->iparam[WRAP_BUF_SMPXS]) + dz->iparam[WRAP_LBUF_SMPXS];	
															/* Allow for overflow areas */
		if(dz->iparam[WRAP_CHANNELS])				
			dz->buflen -= dz->infile->channels * dz->iparam[WRAP_BUF_SMPXS];	/* Allow for overflow space in additional multichan inbuf */
		dz->buflen /= bufdivisor;							/* get unit buffersize */
		if(dz->iparam[WRAP_CHANNELS])						/* If reading multichano: chans * SECSIZE reduces to single mono SECSIZE */
			dz->buflen = (dz->buflen / dz->infile->channels) * dz->infile->channels;
	} while(dz->buflen <= 0);
	if(dz->iparam[WRAP_BUFXX] > 1) {
		if((*this_bloksize *= dz->iparam[WRAP_BUFXX]) < 0) {
			sprintf(errstr,"Cannot create a buffer this large.\n");
			return(DATA_ERROR);
		}
		if((dz->buflen = *this_bloksize) * sizeof(float) < 0) {	/* arithmetic overflow */
			sprintf(errstr,"Cannot create a buffer this large.\n");
			return(MEMORY_ERROR);
		}
		*this_bloksize += standard_block;
		dz->buflen -= (dz->infilecnt * dz->iparam[WRAP_BUF_SMPXS]) + dz->iparam[WRAP_LBUF_SMPXS];	
															/* Allow for overflow areas */
		if(dz->iparam[WRAP_CHANNELS])				
			dz->buflen -= dz->infile->channels * dz->iparam[WRAP_BUF_SMPXS];	/* Allow for overflow space in additional multichan inbuf */
		dz->buflen /= bufdivisor;							/* get unit buffersize */
		if(dz->iparam[WRAP_CHANNELS])						/* If reading multichano: chans * SECSIZE reduces to single mono SECSIZE */
			dz->buflen = (dz->buflen / dz->infile->channels) * dz->infile->channels;
	}
	return(FINISHED);
}

/************************* WRITE_MULTICHAN_GRAIN ****************************/

int write_multichan_grain(double rpos,int chana,int chanb,float **maxwrite,float **Fptr,float **FFptr,int gsize_per_chan,int *nctr,dataptr dz)
{
	int exit_status;
	int   n, to_doo, exess;
	int   real_gsize = gsize_per_chan * dz->out_chans;
	double lpos;
	double adjust = dehole(rpos);
	float  *writend, *fptr = *Fptr, *ffptr = *FFptr;
	float  *gbufptr = dz->extrabuf[WRAP_GBUF];
	lpos  = (1.0 - rpos) * adjust;
	rpos *= adjust;
	chana--;
	chanb--;
	if((writend = (ffptr + real_gsize)) > *maxwrite)
		*maxwrite = writend;
	if(ffptr + real_gsize >= dz->fptr[WRAP_LTAILEND]) {
		to_doo = dz->fptr[WRAP_LTAILEND] - ffptr;
		if((exess  = real_gsize - to_doo) >= dz->iparam[WRAP_LONGS_BUFLEN]) {
 			sprintf(errstr,"Array overflow: write_multichan_grain()\n");
			return(PROGRAM_ERROR);
		}
		to_doo /= dz->out_chans;
		for(n = 0; n < to_doo; n++) {
			*(ffptr + chana) += (float) (*gbufptr * lpos);
			*(ffptr + chanb) += (float) (*gbufptr * rpos);
			gbufptr++;
			ffptr += dz->out_chans;
		}
		if((exit_status = write_samps_granula(dz->iparam[WRAP_LONGS_BUFLEN],nctr,dz))<0)
			return(exit_status);
		memmove((char *)dz->fptr[WRAP_LBUF],(char *)dz->fptr[WRAP_LBUFEND],
			(size_t)dz->iparam[WRAP_LBUF_SMPXS] * sizeof(float));
		memset((char *)dz->fptr[WRAP_LBUFMID],0,dz->iparam[WRAP_LONGS_BUFLEN] * sizeof(float));
		ffptr -= dz->iparam[WRAP_LONGS_BUFLEN];
		fptr  -= dz->iparam[WRAP_LONGS_BUFLEN];
		*maxwrite -= dz->buflen; /* APR 1996 */
		exess /= dz->out_chans;
		for(n = 0; n < exess; n++) {
			*(ffptr + chana) += (float) (*gbufptr * lpos);
			*(ffptr + chanb) += (float) (*gbufptr * rpos);
			gbufptr++;
			ffptr += dz->out_chans;
		}
		*Fptr  = fptr;
		*FFptr = ffptr;
		return(FINISHED);
	}
	for(n=0;n<gsize_per_chan;n++) {
		*(ffptr + chana) += (float) (*gbufptr * lpos);
		*(ffptr + chanb) += (float) (*gbufptr * rpos);
		gbufptr++;
		ffptr += dz->out_chans;
	}
	*Fptr  = fptr;
	*FFptr = ffptr;
	return(FINISHED);
}

/************************* GET_WRAPPAGE_POSITION ****************************
 *
 *	The output position is calculated from centre, spread and depth.
 *	 
 *		 depth							 depth
 *		 _____							 _____		effective_with = depth;
 *		|	  |							|	  |		position random between 1 & -1;
 *		|	  |			centre			|	  |		position *= depth; (range -depth TO +depth)
 *		|------------------|------------------|		if(position < 0.0)
 *		|	halfspread			halfspread	  |			position = centre - half_spread + depth + position.
 *		|_____________________________________|		else
 *						spread							position = centre + half_spread - depth + position.
 */

double get_wrappage_position(dataptr dz)
{
	double pos;
	double spread = min(dz->iparam[WRAP_OUTCHANS],dz->param[WRAP_SPREAD]);
	double halfspread = spread/2.0;
	double depth = min(spread/2.0,dz->param[WRAP_DEPTH]);
	double centre = dz->wrapcentre - 1.0;		//	Convert from 1-N frame to 0 to (N-1) frame
	pos = (drand48() * 2.0) - 1.0;
	pos *= depth;
	if(pos < 0.0)
		pos += (centre - halfspread + depth);
	else
		pos += (centre + halfspread - depth);
	while(pos >= dz->param[WRAP_OUTCHANS])
		pos -= dz->param[WRAP_OUTCHANS];
	while(pos < 0.0)
		pos += dz->param[WRAP_OUTCHANS];
	pos += 1.0;												//	Convert from 0 to (N-1) frame to 1-N frame
	if(pos > dz->param[WRAP_OUTCHANS])
		pos -= dz->param[WRAP_OUTCHANS];				// Positions beyond top chan, are between chan N and chan 1
	return pos;
}

/************************* INITIALISE_UNUSED_BRASSAGE_VFLAGS *************************/

int initialise_unused_wrappage_vflags(dataptr dz)
{
	int n;
	if(dz->vflag==NULL) {
		if((dz->vflag  = (char *)malloc(WRAP_MAX_VFLAGS * sizeof(char)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for wrappage flags.\n");
			return(MEMORY_ERROR);
		}
	} else {
		if((dz->vflag  = (char *)realloc(dz->vflag,WRAP_MAX_VFLAGS * sizeof(char)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to reallocate wrappage flags.\n");
			return(MEMORY_ERROR);
		}
	}
	for(n=dz->application->vflag_cnt;n<WRAP_MAX_VFLAGS;n++) 
		dz->vflag[n]  = FALSE;
	return(FINISHED);
}

/************************** PARAM_CHECK_AND_CONVERT ************************************/

void param_check_and_convert(int *zerovel,dataptr dz)
{
	double sr = (double)dz->infile->srate;
	int paramno, total_params = dz->application->max_param_cnt + dz->application->option_cnt;
	for(paramno = 0;paramno < total_params; paramno++) {
		switch(paramno) {
		case(WRAP_VELOCITY):
		case(WRAP_HVELOCITY):
			if(*zerovel == FALSE)
				*zerovel = check_for_zeroes(paramno,dz);
			break;	
		case(WRAP_AMP):
		case(WRAP_HAMP):
			/* not required for floats */
			/*convert_value_to_int(paramno,TWO_POW_15,dz);*/
			break;
		case(WRAP_OUTLEN):
			convert_value_to_int(paramno,sr,dz);
			break;
		case(WRAP_SRCHRANGE):
		case(WRAP_GRAINSIZE):
		case(WRAP_HGRAINSIZE):
		case(WRAP_BSPLICE):
		case(WRAP_HBSPLICE):
		case(WRAP_ESPLICE):
		case(WRAP_HESPLICE):
			convert_value_to_int(paramno,MS_TO_SECS * sr,dz);
			break;						 
		}
	}
}

/************************** CONVERT_PITCH ************************************/

void convert_pitch(int paramno,dataptr dz)
{
	double *p, *end;
	if(dz->brksize[paramno]) {
		p   = dz->brk[paramno] + 1;		
		end = dz->brk[paramno] + (dz->brksize[paramno] * 2);
		while(p < end) {
			*p *= OCTAVES_PER_SEMITONE;
			p += 2;
		}
	} else
		dz->param[paramno] = dz->param[paramno] * OCTAVES_PER_SEMITONE;
}

/******************************* INITIALISE_DATA **************************/

int initialise_data(dataptr dz)
{
	/* NEED TO BE INITALISED (??) AS THEY ARE INTERNAL */
	dz->param[WRAP_VRANGE] 			= 0.0;
	dz->param[WRAP_DRANGE] 			= 0.0;
	dz->param[WRAP_PRANGE]			= 0.0;
	dz->param[WRAP_SPRANGE]			= 0.0;
	dz->iparam[WRAP_ARANGE] 		= 0;
	dz->iparam[WRAP_GRANGE] 		= 0;
	dz->iparam[WRAP_BRANGE]			= 0;
	dz->iparam[WRAP_ERANGE]			= 0;
	dz->iparam[WRAP_LONGS_BUFLEN]	= 0;
	dz->iparam[WRAP_BUF_XS] 		= 0;
	dz->iparam[WRAP_LBUF_XS] 		= 0;
	dz->iparam[WRAP_BUF_SMPXS] 		= 0;
	dz->iparam[WRAP_LBUF_SMPXS]		= 0;
	dz->iparam[WRAP_GLBUF_SMPXS]	= 0;
	dz->iparam[WRAP_SAMPS_IN_INBUF] = 0;

	dz->iparam[WRAP_IS_BTAB]		= FALSE;
	dz->iparam[WRAP_IS_ETAB]		= FALSE;
																		
    if((dz->parray[WRAP_NORMFACT] = (double *)malloc(dz->iparam[WRAP_ARRAYSIZE] * sizeof(double)))==NULL) {
		sprintf(errstr,"initialise_data()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/************************** SET_INTERNAL_FLAGS ************************************/

int set_internal_flags(dataptr dz)
{
	if(dz->brksize[WRAP_HVELOCITY])  dz->iparray[WRAP_FLAGS][WRAP_VELOCITY_FLAG]  |= WRAP_VARIABLE_HI_BOUND;
	else 							dz->iparray[WRAP_FLAGS][WRAP_VELOCITY_FLAG]  |= WRAP_HI_BOUND;

	if(dz->brksize[WRAP_HDENSITY])   dz->iparray[WRAP_FLAGS][WRAP_DENSITY_FLAG]   |= WRAP_VARIABLE_HI_BOUND;
	else 							dz->iparray[WRAP_FLAGS][WRAP_DENSITY_FLAG]   |= WRAP_HI_BOUND;

	if(dz->brksize[WRAP_HGRAINSIZE]) dz->iparray[WRAP_FLAGS][WRAP_GRAINSIZE_FLAG] |= WRAP_VARIABLE_HI_BOUND;
	else  							dz->iparray[WRAP_FLAGS][WRAP_GRAINSIZE_FLAG] |= WRAP_HI_BOUND;

	if(dz->brksize[WRAP_HPITCH])  	dz->iparray[WRAP_FLAGS][WRAP_PITCH_FLAG]     |= WRAP_VARIABLE_HI_BOUND;
	else 							dz->iparray[WRAP_FLAGS][WRAP_PITCH_FLAG]     |= WRAP_HI_BOUND;

	if(dz->brksize[WRAP_HAMP])  		dz->iparray[WRAP_FLAGS][WRAP_AMP_FLAG]       |= WRAP_VARIABLE_HI_BOUND;
	else 							dz->iparray[WRAP_FLAGS][WRAP_AMP_FLAG]       |= WRAP_HI_BOUND;

	if(dz->brksize[WRAP_HBSPLICE])  	dz->iparray[WRAP_FLAGS][WRAP_BSPLICE_FLAG]   |= WRAP_VARIABLE_HI_BOUND;
	else 							dz->iparray[WRAP_FLAGS][WRAP_BSPLICE_FLAG]   |= WRAP_HI_BOUND;

	if(dz->brksize[WRAP_HESPLICE])  	dz->iparray[WRAP_FLAGS][WRAP_ESPLICE_FLAG]   |= WRAP_VARIABLE_HI_BOUND;
	else 							dz->iparray[WRAP_FLAGS][WRAP_ESPLICE_FLAG]   |= WRAP_HI_BOUND;

	if(dz->brksize[WRAP_VELOCITY])	dz->iparray[WRAP_FLAGS][WRAP_VELOCITY_FLAG]  |= WRAP_VARIABLE_VAL;
	else							dz->iparray[WRAP_FLAGS][WRAP_VELOCITY_FLAG]  |= WRAP_FIXED_VAL;

	if(dz->brksize[WRAP_DENSITY])	dz->iparray[WRAP_FLAGS][WRAP_DENSITY_FLAG]   |= WRAP_VARIABLE_VAL;
	else							dz->iparray[WRAP_FLAGS][WRAP_DENSITY_FLAG]   |= WRAP_FIXED_VAL;

	if(dz->brksize[WRAP_GRAINSIZE])	dz->iparray[WRAP_FLAGS][WRAP_GRAINSIZE_FLAG] |= WRAP_VARIABLE_VAL;
	else							dz->iparray[WRAP_FLAGS][WRAP_GRAINSIZE_FLAG] |= WRAP_FIXED_VAL;

	if(dz->brksize[WRAP_PITCH])		dz->iparray[WRAP_FLAGS][WRAP_PITCH_FLAG]     |= WRAP_VARIABLE_VAL;
	else							dz->iparray[WRAP_FLAGS][WRAP_PITCH_FLAG]     |= WRAP_FIXED_VAL;

	if(dz->brksize[WRAP_AMP])		dz->iparray[WRAP_FLAGS][WRAP_AMP_FLAG]       |= WRAP_VARIABLE_VAL;
	else							dz->iparray[WRAP_FLAGS][WRAP_AMP_FLAG]       |= WRAP_FIXED_VAL;

	if(dz->brksize[WRAP_BSPLICE])	dz->iparray[WRAP_FLAGS][WRAP_BSPLICE_FLAG]   |= WRAP_VARIABLE_VAL;
	else							dz->iparray[WRAP_FLAGS][WRAP_BSPLICE_FLAG]   |= WRAP_FIXED_VAL;

	if(dz->brksize[WRAP_ESPLICE])	dz->iparray[WRAP_FLAGS][WRAP_ESPLICE_FLAG]   |= WRAP_VARIABLE_VAL;
	else							dz->iparray[WRAP_FLAGS][WRAP_ESPLICE_FLAG]   |= WRAP_FIXED_VAL;

	if(dz->brksize[WRAP_SRCHRANGE])	dz->iparray[WRAP_FLAGS][WRAP_RANGE_FLAG]     |= WRAP_VARIABLE_VAL;
	else							dz->iparray[WRAP_FLAGS][WRAP_RANGE_FLAG]     |= WRAP_FIXED_VAL;

	if(dz->brksize[WRAP_SCATTER])	dz->iparray[WRAP_FLAGS][WRAP_SCATTER_FLAG]   |= WRAP_VARIABLE_VAL;
	else							dz->iparray[WRAP_FLAGS][WRAP_SCATTER_FLAG]   |= WRAP_FIXED_VAL;

	return(FINISHED);
}
/**************************** INITIALISE_CHANNEL_CONFIGURATION *******************************/

void initialise_channel_configuration(dataptr dz)
{
	if(dz->infile->channels > MONO) {
		dz->iparam[WRAP_CHANNELS] = TRUE;
	} else {
		dz->iparam[WRAP_CHANNELS] = FALSE;
	}
	dz->iparam[WRAP_INCHANS]  = MONO;			/* input is mono, grain is mono */
}

/*************************** GRANULA_SETUP ***********************/

int granula_setup(dataptr dz)
{
	int exit_status;
	
	if((exit_status = make_splice_tables(dz))<0)
		return(exit_status);
	set_ranges(dz);
	if((exit_status = calc_overflows(dz))<0)
		return(exit_status);

	dz->iparam[WRAP_BUF_SMPXS]   *= dz->iparam[WRAP_INCHANS];		//	extra space in INPUT buffer (must accept all input channels)
	dz->iparam[WRAP_GLBUF_SMPXS] *= dz->iparam[WRAP_INCHANS];		//	extra space in the GRAIN buffer is either mono 
																//	(where channel is extracted, or all channels mixed to mono) 
																//	or it is multichannel: WRAP_INCHANS knows this
	dz->iparam[WRAP_LBUF_SMPXS]  *= dz->iparam[WRAP_OUTCHANS];	//	The output grain can be multichan via spatialisation, or via using multichan input
																//	or multichan if multichan input goes direct to multichan output
	return(FINISHED);
}

/************************** CHECK_FOR_ZEROES ************************************/

int check_for_zeroes(int paramno,dataptr dz)
{
	double *p, *end;
	if(dz->brksize[paramno]) {
		p   = dz->brk[paramno] + 1;		
		end = dz->brk[paramno] + (dz->brksize[paramno] * 2);
		while(p < end) {
			if(flteq(*p,0.0))
				return TRUE;
			p++;
		}
	} else  {
		if(flteq(dz->param[paramno],0.0))
			return TRUE;
	}
	return (FALSE);
}

/************************** CONVERT_VALUE_TO_INT ************************************/

void convert_value_to_int(int paramno,double convertor,dataptr dz)
{
	double *p, *end;
	if(dz->brksize[paramno]) {
		p   = dz->brk[paramno] + 1;		
		end = dz->brk[paramno] + (dz->brksize[paramno] * 2);
		while(p < end) {
			*p = (double)round(*p * convertor);
			p += 2;
		}
		dz->is_int[paramno] = TRUE;
	} else
		dz->iparam[paramno] = round(dz->param[paramno] * convertor);
}

/************************ MAKE_SPLICE_TABLES ******************************/

int make_splice_tables(dataptr dz)
{						 	/* rwd: changed to eliminate f/p division */
	int n;					/* plus quasi-exponential option */
	double dif,val,lastval;
	double length, newsum,lastsum,twodif;
	double *local_btabptr;	/* better safe than sorry! */
	double *local_etabptr;

	if(dz->iparray[WRAP_FLAGS][WRAP_BSPLICE_FLAG]<=1) {
		if(dz->iparam[WRAP_BSPLICE]==0) {
			dz->iparam[WRAP_IS_BTAB] = TRUE;	/* even though it's empty */
			return(FINISHED);
		}
		if((dz->parray[WRAP_BSPLICETAB] = (double *)malloc(dz->iparam[WRAP_BSPLICE] * sizeof(double)))==NULL) {
			sprintf(errstr,"make_splice_tables(): 1\n");
			return(PROGRAM_ERROR);
		}
		local_btabptr = dz->parray[WRAP_BSPLICETAB];
		val = 0.0;
		length = (double)dz->iparam[WRAP_BSPLICE];
		if(!dz->vflag[WRAP_EXPON]) {
			dif = 1.0/length;
			lastval = dif;
			*local_btabptr++ = val;
			*local_btabptr++ = lastval;
			for(n=2;n<dz->iparam[WRAP_BSPLICE];n++) {
				val = lastval + dif;
				lastval = val;
				*local_btabptr++ = val;
			}
		} else {			/* do quasi-exponential splice */
			dif = 1.0/(length*length);
			twodif = dif*2.0;
			lastsum = 0.0;
			lastval = dif;
			*local_btabptr++ = val;
			*local_btabptr++ = lastval;
			for(n=2;n<dz->iparam[WRAP_BSPLICE];n++) {
				newsum = lastsum + twodif;
				val = lastval + newsum + dif;
				*local_btabptr++ = val;
				lastval = val;
				lastsum = newsum;
			}
		}   
		dz->iparam[WRAP_IS_BTAB] = TRUE;
	}
	if(dz->iparray[WRAP_FLAGS][WRAP_ESPLICE_FLAG]<=1) {
		if(dz->iparam[WRAP_ESPLICE]==0) {
			dz->iparam[WRAP_IS_ETAB] = TRUE;	/* even thogh it's empty! */
			return(FINISHED);
		}
		if((dz->parray[WRAP_ESPLICETAB] = (double *)malloc(dz->iparam[WRAP_ESPLICE] * sizeof(double)))==NULL) {
			sprintf(errstr,"make_splice_tables(): 2\n");
			return(PROGRAM_ERROR);
		}
		local_etabptr = dz->parray[WRAP_ESPLICETAB] + dz->iparam[WRAP_ESPLICE];
		val = 0.0;
		length = (double)dz->iparam[WRAP_ESPLICE];
		if(!dz->vflag[WRAP_EXPON]) {
			dif = 1.0/length;
			lastsum = dif;
			*--local_etabptr = val;
			*--local_etabptr = lastsum;
			for(n=dz->iparam[WRAP_ESPLICE]-3;n>=0;n--){
				val = lastsum + dif;
				lastsum = val;	
				*--local_etabptr = val;
			}
		} else {
			dif = 1.0/(length*length);
			twodif = dif*2.0;
			lastsum = 0.0;
			lastval = dif;
			*--local_etabptr = val;
			*--local_etabptr = lastval;
			for(n=dz->iparam[WRAP_ESPLICE]-3;n>=0;n--) {
				newsum = lastsum + twodif;
				val = lastval + newsum + dif;
				*--local_etabptr = val;
				lastval = val;
				lastsum = newsum;
			}
		}   
		dz->iparam[WRAP_IS_ETAB] = TRUE;
    }
	return(FINISHED);
}

/************************* SET_RANGES *****************************/

void set_ranges(dataptr dz)
{
	int n;
	for(n=0;n<WRAP_SFLAGCNT;n++) {		
		if(dz->iparray[WRAP_FLAGS][n]==RANGED) {
			switch(n) {
			case(WRAP_VELOCITY_FLAG):
				set_this_drange(WRAP_VRANGE,WRAP_HVELOCITY,WRAP_VELOCITY,WRAP_VELOCITY_FLAG,dz);
				break;
			case(WRAP_DENSITY_FLAG):
				set_this_drange(WRAP_DRANGE,WRAP_HDENSITY,WRAP_DENSITY,WRAP_DENSITY_FLAG,dz);
				break;
			case(WRAP_GRAINSIZE_FLAG):
				set_this_range(WRAP_GRANGE,WRAP_HGRAINSIZE,WRAP_GRAINSIZE,WRAP_GRAINSIZE_FLAG,dz);
 				break;
			case(WRAP_BSPLICE_FLAG):
				set_this_range(WRAP_BRANGE,WRAP_HBSPLICE,WRAP_BSPLICE,WRAP_BSPLICE_FLAG,dz);
				break;
			case(WRAP_ESPLICE_FLAG):
				set_this_range(WRAP_ERANGE,WRAP_HESPLICE,WRAP_ESPLICE,WRAP_ESPLICE_FLAG,dz);
				break;
			case(WRAP_PITCH_FLAG):
				set_this_drange(WRAP_PRANGE,WRAP_HPITCH,WRAP_PITCH,WRAP_PITCH_FLAG,dz);
				break;
			case(WRAP_AMP_FLAG):
				/*set_this_range*/set_this_drange(WRAP_ARANGE,WRAP_HAMP,WRAP_AMP,WRAP_AMP_FLAG,dz);
				break;
			}
		}
	}
}

/************************ CALC_OVERFLOWS *************************
 *
 * We will attempt to put data into output buffer until we reach its 'end'.
 * But out grain will extend GRAINSIZE beyond this> So we must have
 * an overflow area on the buffer equal to the maximum grainsize.
 *
 * On the input side, we will attempt to read from input buffer until we
 * get to its end, BUT, grainsize extends beyond this. Also, to create
 * an output of GRAINSIZE we need to use an input size of 
 * (grainsize*transposition). So overflow on input buffer = maximum val
 * of (grainsize*transposition).
 * Lbuf_smpxs  = overflow in Lbuf
 * gLbuf_smpxs = MAXIMUM GRAINSIZE
 * buf_smpxs   = overflow in inbuf
 */


int calc_overflows(dataptr dz)	/* DO THIS AFTER THE CONVERSION OF PITCH from SEMITONES */
{
	int exit_status;
	int max_scatter;
	double mt = 1.0;	/* MAXIMUM TRANSPOSITION */
	double mg = 1.0;	/* MAXIMUM GRAINSIZE     */
	double max1, max2;
	int mgi;
	switch(dz->iparray[WRAP_FLAGS][WRAP_PITCH_FLAG]) {
	case(NOT_SET):   
		mt = 1.0;
		break;
	case(FIXED):     
		mt = dz->param[WRAP_PITCH];   		  								
		break;
	case(VARIABLE):
		if((exit_status = get_maxvalue_in_brktable(&max1,WRAP_PITCH,dz))<0)
			return(exit_status);
	  	mt = max1;
	  	break;
	case(RANGED):    
		mt = max(dz->param[WRAP_HPITCH],dz->param[WRAP_PITCH]);		   		
		break;
	case(RANGE_VLO):
		if((exit_status = get_maxvalue_in_brktable(&max1,WRAP_PITCH,dz))<0)
			return(exit_status);
		mt = max(max1,dz->param[WRAP_HPITCH]);  											
		break;
	case(RANGE_VHI): 
		if((exit_status = get_maxvalue_in_brktable(&max2,WRAP_HPITCH,dz))<0)
			return(exit_status);
		mt = max(max2,dz->param[WRAP_PITCH]);  											
		break;
	case(RANGE_VHILO):
		if((exit_status = get_maxvalue_in_brktable(&max1,WRAP_PITCH,dz))<0)
			return(exit_status);
		if((exit_status = get_maxvalue_in_brktable(&max2,WRAP_HPITCH,dz))<0)
			return(exit_status);
		mt = max(max1,max2);		
		break;
	}
	mt = pow(2.0,mt);	/* CONVERT OCTAVES TO TRANSPOSITION RATIO */

	switch(dz->iparray[WRAP_FLAGS][WRAP_GRAINSIZE_FLAG]) {
	case(NOT_SET):
	case(FIXED):     
		mg = (double)dz->iparam[WRAP_GRAINSIZE];			       	   						
	   	break;
	case(VARIABLE):  
		if((exit_status = get_maxvalue_in_brktable(&max1,WRAP_GRAINSIZE,dz))<0)
			return(exit_status);
		mg = max1;		  					
		break;
	case(RANGED):    
// BUG: OCtober 2009
		mg = max(dz->iparam[WRAP_HGRAINSIZE],dz->iparam[WRAP_GRAINSIZE]);  			       							
		break;
	case(RANGE_VLO): 
		if((exit_status = get_maxvalue_in_brktable(&max1,WRAP_GRAINSIZE,dz))<0)
			return(exit_status);
		mg = max(max1,(double)dz->iparam[WRAP_HGRAINSIZE]);		
		break;
	case(RANGE_VHI): 
		if((exit_status = get_maxvalue_in_brktable(&max2,WRAP_HGRAINSIZE,dz))<0)
			return(exit_status);
		mg = max(max2,(double)dz->iparam[WRAP_GRAINSIZE]);		
		break;
	case(RANGE_VHILO):
		if((exit_status = get_maxvalue_in_brktable(&max1,WRAP_GRAINSIZE,dz))<0)
			return(exit_status);
		if((exit_status = get_maxvalue_in_brktable(&max2,WRAP_HGRAINSIZE,dz))<0)
			return(exit_status);
		mg = max(max1,max2);  
		break;
	}
	mgi = round(mg)+1;
	dz->iparam[WRAP_GLBUF_SMPXS] = mgi;						/* Overflow in outbuf = MAX-grainsize */
	dz->iparam[WRAP_BUF_SMPXS]   = round((double)mgi * mt);	/* Overflow in inbuf  = MAX-grainsize * transpos */
	if((exit_status = calc_max_scatter(mgi,&max_scatter,dz))<0)
		return(exit_status);
	adjust_overflows(max_scatter,dz);
	return(FINISHED);
}

/************************ SET_THIS_RANGE ******************************/

void set_this_range(int rangeno,int hino, int lono, int flagno, dataptr dz)
{
	dz->iparam[rangeno]  = dz->iparam[hino] - dz->iparam[lono];
	if(dz->iparam[rangeno]==0)
		dz->iparray[WRAP_FLAGS][flagno] = FIXED;
}

/************************ SET_THIS_DRANGE ******************************/

void set_this_drange(int rangeno,int hino, int lono, int flagno, dataptr dz)
{
	dz->param[rangeno]  = dz->param[hino] - dz->param[lono];
	if(flteq(dz->param[rangeno],0.0))
		dz->iparray[WRAP_FLAGS][flagno] = FIXED;
}

/******************************** CALC_MAX_SCATTER ****************************/

int calc_max_scatter(int mgi,int *max_scatlen, dataptr dz)		 /* mgi = MAX GRAIN SIZE */
{

#define WRAP_ROUNDUP	(0.9999)

	int exit_status;
	int os = 0;
	double sc = 0.0;
	double min1, min2, minn/*, sr = (double)dz->infile->srate*/;
	int k = dz->iparray[WRAP_FLAGS][WRAP_DENSITY_FLAG];	
	switch(k) {
	case(NOT_SET):
	case(FIXED):
	case(RANGED):      
		os = (int)(((double)mgi/dz->param[WRAP_DENSITY]) + WRAP_ROUNDUP);	/* ROUND UP */						
		break;
	case(VARIABLE):    
		if((exit_status = get_minvalue_in_brktable(&min1,WRAP_DENSITY,dz))<0)
			return(exit_status);
		os = (int)(((double)mgi/min1) + WRAP_ROUNDUP);		  						
		break;
	case(RANGE_VLO):   
		if((exit_status = get_minvalue_in_brktable(&min1,WRAP_DENSITY,dz))<0)
			return(exit_status);
		minn = min(min1,dz->param[WRAP_HDENSITY]);			
		os = (int)(((double)mgi/minn) + WRAP_ROUNDUP); 	
		break;
	case(RANGE_VHI):   
		if((exit_status = get_minvalue_in_brktable(&min2,WRAP_HDENSITY,dz))<0)
			return(exit_status);
		minn = min(min2,dz->param[WRAP_DENSITY]);			
		os = (int)(((double)mgi/minn) + WRAP_ROUNDUP); 	
		break;
	case(RANGE_VHILO): 
		if((exit_status = get_minvalue_in_brktable(&min1,WRAP_DENSITY,dz))<0)
			return(exit_status);
		if((exit_status = get_minvalue_in_brktable(&min2,WRAP_HDENSITY,dz))<0)
			return(exit_status);
		minn = min(min1,min2);			
		os = (int)(((double)mgi/minn) + WRAP_ROUNDUP); 	
		break;
	}
	switch(dz->iparray[WRAP_FLAGS][WRAP_SCATTER_FLAG]) {
	case(NOT_SET):
	case(FIXED):     
		sc = dz->param[WRAP_SCATTER];   		   	   		
		break;
	case(VARIABLE):  
		if((exit_status = get_maxvalue_in_brktable(&sc,WRAP_SCATTER,dz))<0)
			return(exit_status);
		break;
	}
	*max_scatlen = (int)(((double)os * sc) + WRAP_ROUNDUP); /* ROUND UP */
	return(FINISHED);
}

/************************ ADJUST_OVERFLOWS ***************************
 * WRAP_LBUF_SMPXS		   = overflow in calculation buffer Lbuf 
 * iparam[WRAP_GLBUF_SMPXS] = MAXIMUM GRAINSIZE 
 * WRAP_BUF_SMPXS		   = overflow in inbuf
 */
 /* RW NB we eliminate almost everything! */
void adjust_overflows(int mscat, dataptr dz)
{
	dz->iparam[WRAP_BUF_SMPXS]   += SAFETY;							/* ADD SAFETY MARGINS !! */
	dz->iparam[WRAP_GLBUF_SMPXS] += OSAFETY;
	dz->iparam[WRAP_LBUF_SMPXS]   = dz->iparam[WRAP_GLBUF_SMPXS] + mscat;
}

/****************************** STORE_FILENAME *********************************/

int store_the_filename(char *filename,dataptr dz)
{
	if(dz->wordstor != NULL) {
		sprintf(errstr,"Cannot store filename: wordstor already allocated.\n");
		return(PROGRAM_ERROR);
	}
	if((dz->wordstor = (char **)malloc(sizeof(char *)))==NULL) {
		sprintf(errstr,"Cannot store filename.\n");
		return(MEMORY_ERROR);
	}
	if((dz->wordstor[0] = (char *)malloc((strlen(filename)+1) * sizeof(char)))==NULL) {
		sprintf(errstr,"Cannot store filename.\n");
		return(MEMORY_ERROR);
	}
	if(strcpy(dz->wordstor[0],filename)!=dz->wordstor[0]) { 
		sprintf(errstr,"Failed to copy filename to store.\n");
		return(PROGRAM_ERROR);
	}
	dz->all_words++;
	return(FINISHED);
}

/************************ HANDLE_THE_SPECIAL_DATA *********************/

int handle_the_special_data(int *cmdlinecnt,char ***cmdline,dataptr dz)
{
	int exit_status;
	aplptr ap = dz->application;

	if(!sloom) {
		if(*cmdlinecnt <= 0) {
			sprintf(errstr,"Insufficient parameters on command line.\n");
			return(USAGE_ONLY);
		}
	}
	ap->min_special 	= 0;
	ap->max_special 	= 16;
	ap->min_special2  	= -1;
	ap->max_special2  	= 1;
	if((exit_status = read_centre_data((*cmdline)[0],dz))<0)
		return(exit_status);
	(*cmdline)++;		
	(*cmdlinecnt)--;
	return(FINISHED);
}

/************************ READ_CENTRE_DATA *********************/

int read_centre_data(char *filename,dataptr dz)
{
	double *time, *position, *direction, lasttime, dummy;
	int cnt;
	char temp[200], *q;
	aplptr ap;
	FILE *fp;
	ap = dz->application;
	dz->zeroset = 0;
	if(sloom) {
		if(filename[0] == NUMERICVAL_MARKER) {
			q = filename + 1;
			dz->wrapcentre = atof(q);
			dz->zeroset = 1;
		}
	} else if(isdigit(filename[0]) || ((filename[0] == '.') && isdigit(filename[1]))) {
		dz->wrapcentre = atof(filename);
		dz->zeroset = 1;
	}
	if(dz->zeroset)
		return FINISHED;
	if((fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,	"Can't open file %s to read data.\n",filename);
		return(DATA_ERROR);
	}
	cnt = 0;
	while(fgets(temp,200,fp)==temp) {
		q = temp;
		while(get_float_from_within_string(&q,&dummy)) {
			cnt++;
		}
	}	    
	if(cnt == 0) {
		sprintf(errstr,"No data in file %s\n",filename);
		return(DATA_ERROR);
	}
	if(((dz->ringsize = cnt/3) * 3) != cnt) {
		sprintf(errstr,"Data not grouped correctly in file %s\n",filename);
		return(DATA_ERROR);
	}
	if((dz->parray[WRAP_CENTRE] = (double *)malloc(cnt * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for data in file %s.\n",filename);
		return(MEMORY_ERROR);
	}
	time	 = dz->parray[WRAP_CENTRE];
	position = time+1;
	direction	 = time+2;
	rewind(fp);
	lasttime = -1.0;
	cnt = 0;
	while(fgets(temp,200,fp)==temp) {
		q = temp;
		while(get_float_from_within_string(&q,&dummy)) {
			switch(cnt) {
			case(0):
				if(dummy < 0.0 || dummy <= lasttime) {
					sprintf(errstr,"Times do not advance correctly in file %s\n",filename);
					return(DATA_ERROR);
				}
				*time = dummy;
				time += 3;
				break;
			case(1):
				if(dummy < ap->min_special || dummy > ap->max_special) {
					sprintf(errstr,"Invalid position value (%lf) in file %s\n",dummy,filename);
					return(DATA_ERROR);
				}
				*position = dummy;
				position += 3;
				break;
			case(2):
				if(!(flteq(dummy,ap->min_special2) || flteq(dummy,ap->max_special2))) {
					sprintf(errstr,"Invalid direction value (%lf) in file %s\n",dummy,filename);
					return(DATA_ERROR);
				}
				*direction = dummy;
				direction += 3;
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
	return(FINISHED);
}

/************************ GET_CENTRE *********************/

void get_centre(double thistime,dataptr dz)
{
	double *time, *position, *direction, *nexttime, *lasttime, *nextpos, *lastpos;
	double lastplace, nextplace, time_step, time_frac, position_step, new_position;
	int k;
	//	DATA is time - position - direction triples  : total size is dz->ringsize;
	time = dz->parray[WRAP_CENTRE];
	k = 0;
	while(*time < thistime) {
		k += 3;
		if(k >= dz->ringsize) {	//	Fell off end of data
			position = time + 1;
			dz->wrapcentre = *position;
			return;
		}
		time += 3;
	}
	nexttime = time;
	nextpos = time + 1;
	if(k - 3 < 0) {				//	Time is at or before first entry in data
		dz->wrapcentre = *nextpos;
		return;
	} 
	time -= 3;
	lasttime  = time;
	lastpos   = time + 1;
	direction = time + 2;
	lastplace = *lastpos;
	nextplace = *nextpos;
	time_step = *nexttime - *lasttime;
	time_frac = (thistime - *lasttime)/time_step;
	if(*direction > 0) {
		if(nextplace < lastplace)
			nextplace += dz->iparam[WRAP_OUTCHANS];
		position_step =  nextplace - lastplace;
		position_step *= time_frac;
		new_position = lastplace + position_step;
		while(new_position > dz->iparam[WRAP_OUTCHANS])
			new_position -= dz->iparam[WRAP_OUTCHANS];
	} else {
		if(nextplace > lastplace)
			lastplace += dz->iparam[WRAP_OUTCHANS];
		position_step =  lastplace - nextplace;
		position_step *= time_frac;
		new_position = lastplace - position_step;
		while(new_position > dz->iparam[WRAP_OUTCHANS])
			new_position -= dz->iparam[WRAP_OUTCHANS];
	}
	dz->wrapcentre = new_position;
}

/************************ READ_TIMEVARYING_DATA *********************/

int read_timevarying_data(double itime, double otime, dataptr dz)
{
	int exit_status, k;
	if(!dz->zeroset) {
		if(dz->vflag[WRAP_OTIME])
			get_centre(otime,dz);
		else
			get_centre(itime,dz);
	}
	for(k = WRAP_SPREAD; k <= WRAP_SCATTER;k++) {
		if(dz->brksize[k]) {
			switch(k) {
			case(WRAP_VELOCITY):
			case(WRAP_HVELOCITY):
				if((exit_status = read_value_from_brktable(itime,k,dz))<0)
					return(exit_status);
				break;
			default:
				if(dz->vflag[WRAP_OTIME]) {
					if((exit_status = read_value_from_brktable(otime,k,dz))<0)
						return(exit_status);
				} else {
					if((exit_status = read_value_from_brktable(itime,k,dz))<0)
						return(exit_status);
				}
				break;
			}
		}
	}
	return FINISHED;
}
