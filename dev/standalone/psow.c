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
 *	INPUT MUST BE MONO
 *	& use PTOBRK to create the pitcb brkpoint file with no-SIG markers
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

const char* cdp_version = "7.1.0";

#define WINDIV (6.0)
#define BUMZEROCNT (20)		/* No of zeros that can occur WITHIN a genuine signal */
#define SMOOTHWIN (40)		/* Max no samples to smooth if glitch at end */
#define PITCHERROR (1.5)
#define ALMOST_OCT (1.9)
/* windows are three times smaller than the pitch-cycle */

//CDP LIB REPLACEMENTS
static int check_psow_param_validity_and_consistency(dataptr dz);
static int setup_psow_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_psow_param_ranges_and_defaults(dataptr dz);
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
static int establish_maxmode(dataptr dz);
static int get_the_mode_from_cmdline(char *str,dataptr dz);

static int find_max_nopitch_stretch(dataptr dz);
static int	create_psow_sndbufs(int minbuf,dataptr dz);
static int	psoa_weird(dataptr dz);
static int	extract_pitch_dependent_env_from_sndfile(int minwsize,int k,int *maxwsize,dataptr dz);
static float getmaxsampr(int startsamp, int sampcnt,float *buffer);
static int	get_min_wsize(int *minwsize,int k,dataptr dz);
static int	find_min_energy_downward_zero_crossing_point
			(int *n,int *trofpnt,int trofpntcnt,double *scanarray,int *cutcnt,int *cut,int kk,int cutstart,dataptr dz);
static int read_validpitch_wsize_in_samps_from_brktable(double thistime,int kk,dataptr dz);
static int next_zero_cross(int here,dataptr dz);
static int previous_zero_cross(int here,int last,dataptr dz);
static int	get_envelope_troughs(int *trofpnt,int *trofpntcnt,int envcnt,dataptr dz);
static int  eliminate_too_short_events(int *zcnt,int *final_pos,int *sigcnt,int opos,int last,float *obuf,int chans,dataptr dz);
static int  smooth_bad_events(int *zcnt,int *final_pos,int *sig_cnt,int *stsmoothed,int opos,int last,float *buf,int chans,dataptr dz);
static int	smooth_bad_grains(int seglen,int bufno, dataptr dz);
static int triangulate_env(int *here,int *there,int ideal_place,float *buf);
static int count_zerocrossings(int here,int there,float *buf);
static int	mark_cut(int *cutcnt,int *cut,int localpeakcnt,double *startarray,int here,int there,
			 int startsamp,int first_downcross,double starttime,int msg,dataptr dz);
static int	find_the_local_peaks(int *here,int *there,float *buf,int *n,int trofpntcnt,int *trofpnt,
		 int *startsamp,int *endsamp,int losamp, int *cut, int cutcnt, double *localpeak, double *scanarray, 
		 int *localpeakcnt,int *first_downcross,int kk,dataptr dz);

static int		smooth_cuts(int *cut,int *cutcnt,int kk,int cutstart,dataptr dz);
static int		auto_correlate(int start,int *at,int end,int realend,int minlen,double pitchseg,int kk,dataptr dz);
static double	autocorrelate(int n,int m,float *buf);
static int		next_down_zcross(int here,int hibound,float *buf);
static int		last_down_zcross(int here,int lobound,float *buf);
static void		keep_filename_tail_only(dataptr dz);
static void		create_next_filename(char *outfilename,int n);

static int	psoa_interp(dataptr dz);
static int	is_valid_grain(float *buf,int filelen);
static int	chop_zero_signal_areas_into_compatible_units(int *cutcnt,int *cuttime,int cutstart,int kk,dataptr dz);
static int	get_time_nearest_true_pitch(double time,double *pitch,int kk,dataptr dz);
static int	do_pitchsync_grain_interp(int start0,int end0,int start1,int end1,int *opos,dataptr dz);
static int	count_wavesets(int *cnt,int *maxsize,int start,int end,float *buf);
static int	get_waveset_end(int start,int end,int *thisend,float *buf);
static int	find_repcnt(int startlen,int endlen,int samplen,dataptr dz);

static double tremolo(int seglen,double tremfrq,double tremdepth,dataptr dz);
static int	  vibrato(int seglen,double vibfrq,double vibdep, dataptr dz);

static int	  zerofof(int start,int end,float *buf);

static int fof_stretch(int n,double time,int here_in_buf,int there_in_buf,float *ibuf,float *obuf,
			int *opos,double *maxlevel,double gain,dataptr dz);

//static int interp_maxfofs(int k,int here_in_buf, int there_in_buf,float *ibuf,float *obuf,int *cuttime,
//			int startsamp,double gain,int *opos,int *n,double maxlevel,int cutcnt,int *seglen,dataptr dz);

static double linear_splint(double *fofenv,int *fofloc,int envcnt,double time,int *lo);
static int superimpose_fofs_on_synthtones(double *fofenv,int *fofloc,double *sintab,int here_in_buf,int there_in_buf,
				float *ibuf,float *obuf,int *opos,int samptime,int envcnt,dataptr dz);

#define SINTABLEN	(4096.0)
#define ROOT2		(1.4142136)
#define	PSOW_POW	(.25)

static int read_synth_data(char *filename,dataptr dz);
static int read_time_varying_synth_data(char *filename,dataptr dz);
static int get_data_from_tvary_infile(char *filename,dataptr dz);
static int check_synth_data(int wordcnt_in_line,dataptr dz);
static int getmaxlinelen(int *maxcnt,FILE *fp);;
static int allocate_osc_frq_amp_arrays(int fltcnt,dataptr dz);
static int check_seq_and_range_of_oscillator_data(double *fbrk,int wordcnt_in_line,dataptr dz);
static int allocate_tvarying_osc_arrays(dataptr dz);
static int put_tvarying_osc_data_in_arrays(double *fbrk,dataptr dz);
static int initialise_psowsynth_internal_params(dataptr dz);
static int newoscval(int samptime,dataptr dz);
static int handle_the_special_data(int *cmdlinecnt,char ***cmdline,dataptr dz);
static int setup_the_special_data_ranges(int mode,int srate,double duration,double nyquist,int wlength,int channels,aplptr ap);
static int create_normalising_envelope(dataptr dz);
static void getfofenv(float *ibuf,double *fofenv,int *fofloc,int maxseglen,int here_in_buf,int there_in_buf,int *envcnt);
static int superimpose_fofs_on_input(double *fofenv,int *fofloc,int here_in_buf,int there_in_buf,
			float *ibuf,float *ibuf2,float *obuf,int *opos,int startsamp,
			int *envtoptime,double *envval,double *envincr,int envcnt,dataptr dz);
static void spacecalc(double position,double *leftgain,double *rightgain);
static void insert_edge_cuts(int *cuttime,int *cutcnt,int *cutstart,dataptr dz);
static float *do_weight(int *cnt0,int *cnt1,float *ibuf,float *ibuf3,float *readbuf,dataptr dz);
static int read_reinforcement_data(char *filename,dataptr dz);
static int harmonic_is_duplicated(int j,int m,int *hno);
static void sort_harmonics_to_ascending_order(dataptr dz);
static int read_inharmonics_data(char *filename,dataptr dz);
static int convert_pse_semit_to_octratio(dataptr dz);

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
	int exit_status;
	dataptr dz = NULL;
	char **cmdline;
	int  cmdlinecnt;
	int minbuf, n;
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

		if((exit_status = establish_maxmode(dz))<0)
			return(FAILED);
		if(dz->maxmode > 0) {
			if((exit_status = get_the_mode_from_cmdline(cmdline[0],dz))<0) {
				print_messages_and_close_sndfiles(exit_status,is_launched,dz);
				return(exit_status);
			}
			cmdline++;
			cmdlinecnt--;
		}
		// setup_particular_application =
		if((exit_status = setup_psow_application(dz))<0) {
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
	if((exit_status = setup_psow_param_ranges_and_defaults(dz))<0) {
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
	if(dz->process == PSOW_INTERP || dz->process == PSOW_IMPOSE || dz->process == PSOW_INTERLEAVE || dz->process == PSOW_REPLACE) {
		if((exit_status = handle_other_infile(1,cmdline[0],dz))<0)
			return(exit_status);
		cmdline++;
		cmdlinecnt--;
	}
	// handle_outfile() = 
	if(sloom || (dz->process != PSOW_LOCATE)) {
		if((exit_status = handle_the_outfile(&cmdlinecnt,&cmdline,dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
	}

//	handle_formants()			redundant
//	handle_formant_quiksearch()	redundant
	// handle_special_data() = 
	if(dz->process == PSOW_SYNTH || dz->process == PSOW_REINF) {
		if((exit_status = handle_the_special_data(&cmdlinecnt,&cmdline,dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
	}
	if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {		// CDP LIB
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
//	check_param_validity_and_consistency....
	if((exit_status = check_psow_param_validity_and_consistency(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	is_launched = TRUE;
	switch(dz->process) {
	case(PSOW_STRETCH): dz->bufcnt = 4; break;
	case(PSOW_DUPL):	dz->bufcnt = 4; break;
	case(PSOW_DEL):		dz->bufcnt = 3; break;
	case(PSOW_STRFILL): dz->bufcnt = 5; break;
	case(PSOW_FREEZE):	dz->bufcnt = 3; break;
	case(PSOW_CHOP):	dz->bufcnt = 3; break;
	case(PSOW_INTERP):	dz->bufcnt = 5; break;
	case(PSOW_FEATURES):dz->bufcnt = 4; break;
	case(PSOW_SYNTH):	dz->bufcnt = 5; break;
	case(PSOW_IMPOSE):	dz->bufcnt = 6; break;
	case(PSOW_SPLIT):	dz->bufcnt = 4; break;
	case(PSOW_SPACE):	dz->bufcnt = 6; break;
	case(PSOW_INTERLEAVE): dz->bufcnt = 6; break;
	case(PSOW_REPLACE): dz->bufcnt = 4; break;
	case(PSOW_EXTEND):
	case(PSOW_EXTEND2):  dz->bufcnt = 5; break;
	case(PSOW_LOCATE):  dz->bufcnt = 2; break;
	case(PSOW_CUT):		dz->bufcnt = 4; break;
	case(PSOW_REINF):	dz->bufcnt = 5; break;
	}
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

	if(dz->process == PSOW_INTERP)
		minbuf = dz->infile->srate;	/* 1 second */
	else
		minbuf = find_max_nopitch_stretch(dz);
	if((exit_status = create_psow_sndbufs(minbuf,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	//param_preprocess()						redundant
	//spec_process_file =
	if(dz->process == PSOW_INTERP) {
		if((exit_status = psoa_interp(dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
	} else if((exit_status = psoa_weird(dz))<0) {
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
	strcpy(dz->outfilename,filename);	   
	if(dz->process != PSOW_LOCATE) {
		if(dz->process == PSOW_SPACE)
			dz->infile->channels = 2;
		if((exit_status = create_sized_outfile(filename,dz))<0)
			return(exit_status);
		if(dz->process == PSOW_SPACE)
			dz->infile->channels = 1;
		if(dz->process == PSOW_CHOP) {
			dz->all_words = 0;									
			if((exit_status = store_filename(filename,dz))<0)
				return(exit_status);
		}
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

/************************* SETUP_PSOW_APPLICATION *******************/

int setup_psow_application(dataptr dz)
{
	int exit_status;
	aplptr ap;
	if((exit_status = establish_application(dz))<0)		// GLOBAL
		return(FAILED);
	ap = dz->application;
	// SEE parstruct FOR EXPLANATION of next 2 functions
	switch(dz->process) {
	case(PSOW_STRETCH):
		if((exit_status = set_param_data(ap,0   ,3,3,"DDi"      ))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","",0,0,""))<0)
			return(FAILED);
		break;
	case(PSOW_DUPL):
		if((exit_status = set_param_data(ap,0   ,3,3,"DIi"      ))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","",0,0,""))<0)
			return(FAILED);
		break;
	case(PSOW_DEL):
		if((exit_status = set_param_data(ap,0   ,3,3,"DIi"      ))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","",0,0,""))<0)
			return(FAILED);
		break;
	case(PSOW_STRFILL):
		if((exit_status = set_param_data(ap,0   ,4,4,"DDiD"      ))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","",0,0,""))<0)
			return(FAILED);
		break;
	case(PSOW_FREEZE):
		if((exit_status = set_param_data(ap,0   ,8,8,"DddiDDDd"  ))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","",0,0,""))<0)
			return(FAILED);
		break;
	case(PSOW_CHOP):
		if((exit_status = set_param_data(ap,0   ,2,2,"DD"  ))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","",0,0,""))<0)
			return(FAILED);
		break;
	case(PSOW_INTERP): 
		if((exit_status = set_param_data(ap,0   ,7,7,"dddDDDD"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","",0,0,""))<0)
			return(FAILED);
		break;
	case(PSOW_FEATURES): 
		if((exit_status = set_param_data(ap,0   ,11,11,"DiDDDDDdIDI"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","a",1,0,"0"))<0)
			return(FAILED);
		break;
	case(PSOW_SYNTH): 
		switch(dz->mode) {
		case(0):
		case(1):		
			exit_status = set_param_data(ap,SYNTHBANK				  ,2,2,  "DD");
			break;
		case(2):
		case(3):		
			exit_status = set_param_data(ap,TIMEVARYING_SYNTHBANK	  ,2,2,  "DD");
			break;
		case(4):		
			exit_status = set_param_data(ap,0						  ,2,2,  "DD");
			break;
		}
		if(exit_status < 0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","",0,0,"0"))<0)
			return(FAILED);
		break;
	case(PSOW_IMPOSE): 
		if((exit_status = set_param_data(ap,0 ,4,4,"DDdd"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","",0,0,"0"))<0)
			return(FAILED);
		break;
	case(PSOW_SPLIT): 
		if((exit_status = set_param_data(ap,0 ,4,4,"Didd"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","",0,0,"0"))<0)
			return(FAILED);
		break;
	case(PSOW_SPACE): 
		if((exit_status = set_param_data(ap,0 ,5,5,"DiDDD"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","",0,0,"0"))<0)
			return(FAILED);
		break;
	case(PSOW_INTERLEAVE): 
		if((exit_status = set_param_data(ap,0 ,6,6,"DDiDDD"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","",0,0,"0"))<0)
			return(FAILED);
		break;
	case(PSOW_REPLACE): 
		if((exit_status = set_param_data(ap,0 ,3,3,"DDi"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","",0,0,"0"))<0)
			return(FAILED);
		break;
	case(PSOW_EXTEND): 
		if((exit_status = set_param_data(ap,0 ,8,8,"DddiDDDD"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","s",1,0,"0"))<0)
			return(FAILED);
		break;
	case(PSOW_EXTEND2): 
		if((exit_status = set_param_data(ap,0 ,6,6,"iidDDi"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","",0,0,"0"))<0)
			return(FAILED);
		break;
	case(PSOW_LOCATE): 
		if((exit_status = set_param_data(ap,0 ,2,2,"Dd"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","",0,0,"0"))<0)
			return(FAILED);
		break;
	case(PSOW_CUT): 
		if((exit_status = set_param_data(ap,0 ,2,2,"Dd"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","",0,0,"0"))<0)
			return(FAILED);
		break;
	case(PSOW_REINF): 
		switch(dz->mode) {
		case(0):
			if((exit_status = set_param_data(ap,PSOW_REINFORCEMENT ,1,1,"D"))<0)
				return(FAILED);
			if((exit_status = set_vflgs(ap,"",0,"","ds",2,1,"d0"))<0)
				return(FAILED);
			break;
		case(1):
			if((exit_status = set_param_data(ap,PSOW_INHARMONICS ,1,1,"D"))<0)
				return(FAILED);
			if((exit_status = set_vflgs(ap,"",0,"","w",1,1,"d"))<0)
				return(FAILED);
			break;
		}
		break;
	}
	// set_legal_infile_structure -->
	dz->has_otherfile = FALSE;
	// assign_process_logic -->
	switch(dz->process) {
	case(PSOW_INTERP):
	case(PSOW_IMPOSE):
	case(PSOW_INTERLEAVE):
	case(PSOW_REPLACE):
		dz->input_data_type = TWO_SNDFILES;
		break;
	default:
		dz->input_data_type = SNDFILES_ONLY;
		break;
	}
	if(dz->process == PSOW_LOCATE) {
		dz->process_type	= OTHER_PROCESS;	
		dz->outfiletype  	= NO_OUTPUTFILE;
	} else {
		dz->process_type	= UNEQUAL_SNDFILE;	
		dz->outfiletype  	= SNDFILE_OUT;
	}
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

/************************* SETUP_PSOW_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_psow_param_ranges_and_defaults(dataptr dz)
{
	int exit_status;
	aplptr ap = dz->application;
	// set_param_ranges()
	ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
	// NB total_input_param_cnt is > 0 !!!s
	if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
		return(FAILED);
	// get_param_ranges()
	ap->lo[0]			= -2.0;
	ap->hi[0]			= dz->nyquist;
	ap->default_val[0]	= 440.0;
	switch(dz->process) {
	case(PSOW_STRETCH):
		ap->lo[1]			= 0.1;
		ap->hi[1]			= 10.00;
		ap->default_val[1]	= 1.0;
		ap->lo[2]			= 1;
		ap->hi[2]			= 256;
		ap->default_val[2]	= 1;
		dz->maxmode = 0;
		break;
	case(PSOW_DUPL):
		ap->lo[1]			= 2;
		ap->hi[1]			= 256;
		ap->default_val[1]	= 2;
		ap->lo[2]			= 1;
		ap->hi[2]			= 256;
		ap->default_val[2]	= 1;
		dz->maxmode = 0;
		break;
	case(PSOW_DEL):
		ap->lo[1]			= 2;
		ap->hi[1]			= 20;
		ap->default_val[1]	= 2;
		ap->lo[2]			= 1;
		ap->hi[2]			= 256;
		ap->default_val[2]	= 1;
		dz->maxmode = 0;
		break;
	case(PSOW_STRFILL):
		ap->lo[1]			= 1.0;
		ap->hi[1]			= 10.00;
		ap->default_val[1]	= 1.0;
		ap->lo[2]			= 1;
		ap->hi[2]			= 32767;
		ap->default_val[2]	= 1;
		ap->lo[3]			= -24;
		ap->hi[3]			= 24;
		ap->default_val[3]	= 0;
		dz->maxmode = 0;
		break;
	case(PSOW_FREEZE):
		ap->lo[PS_TIME]			 = 0.0;
		ap->hi[PS_TIME]			 = dz->duration;
		ap->default_val[PS_TIME] = dz->duration/2.0;
		ap->lo[PS_DUR]			 = 0.0;
		ap->hi[PS_DUR]			 = 32767.0;
		ap->default_val[PS_DUR]	 = 1.0;
		ap->lo[PS_SEGS]			 = 1;
		ap->hi[PS_SEGS]			 = 256;
		ap->default_val[PS_SEGS] = 1;
		ap->lo[PS_DENS]			 = .001;
		ap->hi[PS_DENS]			 = 100;
		ap->default_val[PS_DENS] = 1.0;
		ap->lo[PS_TRNS]			 = .125;
		ap->hi[PS_TRNS]			 = 8.0;
		ap->default_val[PS_TRNS] = 1.0;
		ap->lo[PS_RAND]			 = 0.0;
		ap->hi[PS_RAND]			 = 1.0;
		ap->default_val[PS_RAND] = 0.0;
		ap->lo[PS_GAIN]			 = 0.0;
		ap->hi[PS_GAIN]			 = 1.0;
		ap->default_val[PS_GAIN] = 1.0;
		dz->maxmode = 0;
		break;
	case(PSOW_CHOP):
		ap->lo[1]			= 1.0;
		ap->hi[1]			= dz->insams[0];
		ap->default_val[1]	= 1.0;
		dz->maxmode = 0;
		break;
	case(PSOW_INTERP):
		ap->lo[PS_SDUR]	= 0.0;
		ap->hi[PS_SDUR]	= 32767.0;
		ap->default_val[PS_SDUR] = 1.0;
		ap->lo[PS_IDUR]	= 0.0;
		ap->hi[PS_IDUR]	= 32767.0;
		ap->default_val[PS_IDUR] = 1.0;
		ap->lo[PS_EDUR]	= 0.0;
		ap->hi[PS_EDUR]	= 32767.0;
		ap->default_val[PS_EDUR] = 1.0;
		ap->lo[PS_VIBFRQ]	= 0.0;
		ap->hi[PS_VIBFRQ]	= 20;
		ap->default_val[PS_VIBFRQ] = 6.5;
		ap->lo[PS_VIBDEPTH]	= 0.0;
		ap->hi[PS_VIBDEPTH]	= 3.0;
		ap->default_val[PS_VIBDEPTH] = 0.0;
		ap->lo[PS_TREMFRQ]	= 0.0;
		ap->hi[PS_TREMFRQ]	= 30.0;
		ap->default_val[PS_TREMFRQ] = 8.34712;
		ap->lo[PS_TREMDEPTH]	= 0;
		ap->hi[PS_TREMDEPTH]	= 10.0;
		ap->default_val[PS_TREMDEPTH] = 0.2;
		dz->maxmode = 0;
		break;
	case(PSOW_FEATURES):
		ap->lo[PSF_SEGS] = 1;
		ap->hi[PSF_SEGS] = 256;
		ap->default_val[PSF_SEGS] = 1;
		ap->lo[PSF_TRNS] = -48;
		ap->hi[PSF_TRNS] = 96;
		ap->default_val[PSF_TRNS] = 0;
		ap->lo[PS_VIBFRQ] = 0.0;
		ap->hi[PS_VIBFRQ] = 20;
		ap->default_val[PS_VIBFRQ] = 6.5;
		ap->lo[PS_VIBDEPTH] = 0.0;
		ap->hi[PS_VIBDEPTH]	= 3.0;
		ap->default_val[PS_VIBDEPTH] = 0.0;
		ap->lo[PSF_SPEC] = -24;
		ap->hi[PSF_SPEC] = 24;
		ap->default_val[PSF_SPEC] = 0;
		ap->lo[PSF_RAND] = 0.0;
		ap->hi[PSF_RAND] = 1.0;
		ap->default_val[PSF_RAND] = 0.0;
		ap->lo[PSF_GAIN] = 0.0;
		ap->hi[PSF_GAIN] = 1.0;
		ap->default_val[PSF_GAIN] = 1.0;
		ap->lo[PSF_SUB] = 0;
		ap->hi[PSF_SUB] = 8;
		ap->default_val[PSF_SUB] = 0;
		ap->lo[PSF_SUBGAIN] = 0;
		ap->hi[PSF_SUBGAIN] = 1;
		ap->default_val[PSF_SUBGAIN] = 0.5;
		ap->lo[PSF_FOFSTR] = 1.0;
		ap->hi[PSF_FOFSTR] = 4096;
		ap->default_val[PSF_FOFSTR] = 1.0;
		dz->maxmode = 2;
		break;
	case(PSOW_SYNTH):
		ap->lo[PS_DEPTH]  = 0.0;			/* depth */
		ap->hi[PS_DEPTH]  = 1.0;
		ap->default_val[PS_DEPTH]  = 1.0;
		dz->maxmode = 5;
		break;
	case(PSOW_IMPOSE):
		ap->lo[PS_DEPTH]  = 0.0;			/* depth */
		ap->hi[PS_DEPTH]  = 1.0;
		ap->default_val[PS_DEPTH]  = 1.0;
		ap->lo[PS_WSIZE]  = 1.0;			/* windowsize (ms)  */
		ap->hi[PS_WSIZE]  = 200.0;
		ap->default_val[PS_WSIZE]  = 50.0;
		ap->lo[PS_GATE]  = -96.0;			/* gate (db) */
		ap->hi[PS_GATE]  = 0.0;
		ap->default_val[PS_GATE]  = -60.0;
		dz->maxmode = 0;
		break;
	case(PSOW_SPLIT):
		ap->lo[PS_SUBNO]  = 3.0;			/* suharm no */
		ap->hi[PS_SUBNO]  = 8.0;
		ap->default_val[PS_SUBNO]  = 3.0;
		ap->lo[PS_UTRNS]  = 0.0;			/* up transpos (semitones) */
		ap->hi[PS_UTRNS]  = 48.0;
		ap->default_val[PS_UTRNS]  = 0.0;
		ap->lo[PS_ATTEN]  = 0.0;			/* relative level */
		ap->hi[PS_ATTEN]  = 8.0;
		ap->default_val[PS_ATTEN]  = 1.0;
		dz->maxmode = 0;
		break;
	case(PSOW_SPACE):
		ap->lo[PS_SUBNO]  = 2.0;			/* suharm no */
		ap->hi[PS_SUBNO]  = 5.0;
		ap->default_val[PS_SUBNO]  = 2.0;
		ap->lo[PS_SEPAR]  = -1.0;			/* relative level */
		ap->hi[PS_SEPAR]  = 1.0;
		ap->default_val[PS_SEPAR]  = 0.0;
		ap->lo[PS_RELEV]  = 0.001;			/* LR relative level */
		ap->hi[PS_RELEV]  = 1000.0;
		ap->default_val[PS_RELEV]  = 1.0;
		ap->lo[PS_RELV2]  = 0.0;			/* HI frq suppression */
		ap->hi[PS_RELV2]  = 1.0;
		ap->default_val[PS_RELV2]  = 0.0;
		dz->maxmode = 0;
		break;
	case(PSOW_INTERLEAVE):
		ap->lo[1]			= -2.0;			/* pitchdata for file 2 */
		ap->hi[1]			= dz->nyquist;
		ap->default_val[1]	= 440.0;
		ap->lo[PS_GCNT]		= 1.0;			/* fofs per bunch */
		ap->hi[PS_GCNT]		= 16.0;
		ap->default_val[PS_GCNT]  = 1.0;
		ap->lo[PS_BIAS]  = -1;				/* bias to pitch of one or other */
		ap->hi[PS_BIAS]  = 1;
		ap->default_val[PS_BIAS]  = 0;
		ap->lo[PS_RELV2]  = 0.0001;			/* relative level */
		ap->hi[PS_RELV2]  = 10000.0;
		ap->default_val[PS_RELV2]  = 1.0;
		ap->lo[PS_WEIGHT]  = 0.0625;		/* weighting to one sound or other */
		ap->hi[PS_WEIGHT]  = 16.0;
		ap->default_val[PS_WEIGHT]  = 1.0;
		dz->maxmode = 0;
		break;
	case(PSOW_REPLACE):
		ap->lo[1]			= -2.0;			/* pitchdata for file 2 */
		ap->hi[1]			= dz->nyquist;
		ap->default_val[1]  = 449.0;
		ap->lo[2]			= 1;			/* fofs per bunch */
		ap->hi[2]			= 16;
		ap->default_val[2]  = 1.0;
		dz->maxmode = 0;
		break;
	case(PSOW_EXTEND):
		ap->default_val[0] = 440.0;			/* pitch */
		ap->lo[PS_TIME]	 = 0.0;			/* grabtime */
		ap->hi[PS_TIME]	 = dz->duration;	
		ap->default_val[PS_TIME]  = 0.0;			/* grabtime */
		ap->lo[PS_DUR]	 = dz->duration;	/* TOTAL duration of output */
		ap->hi[PS_DUR]	 = 32767.0;
		ap->default_val[PS_DUR]   = dz->duration + 1.0;	
		ap->lo[PS_SEGS]	 = 1;
		ap->hi[PS_SEGS]	 = 256;
		ap->default_val[PS_SEGS]  = 1;
		ap->lo[PSE_VFRQ] = 0.0;
		ap->hi[PSE_VFRQ] = 20;
		ap->default_val[PSE_VFRQ] = 6.5;
		ap->lo[PSE_VDEP] = 0.0;
		ap->hi[PSE_VDEP] = 3.0;
		ap->default_val[PSE_VDEP] = 0.0;
		ap->lo[PSE_TRNS] = -48.0;
		ap->hi[PSE_TRNS] = 24.0;
		ap->default_val[PSE_TRNS] = 0.0;
		ap->lo[PSE_GAIN] = 0.0;
		ap->hi[PSE_GAIN] = 10.0;
		ap->default_val[PSE_GAIN] = 1.0;
		dz->maxmode = 0;
		break;
	case(PSOW_EXTEND2):
		ap->lo[0]  = 0;
		ap->hi[0]  = dz->duration;
		ap->default_val[0] = 0.0;
		ap->lo[1]  = 0;
		ap->hi[1]  = dz->duration;
		ap->default_val[1] = 0.0;
		ap->lo[PS_DUR]	 = dz->duration;	/* TOTAL duration of output */
		ap->hi[PS_DUR]	 = 32767.0;
		ap->default_val[PS_DUR]   = dz->duration + 1.0;	
		ap->lo[PS2_VFRQ] = 0.0;
		ap->hi[PS2_VFRQ] = 20;
		ap->default_val[PS2_VFRQ] = 6.5;
		ap->lo[PS2_VDEP] = 0.0;
		ap->hi[PS2_VDEP] = 3.0;
		ap->default_val[PS2_VDEP] = 0.0;
		ap->lo[PS2_NUJ] = -24.0;
		ap->hi[PS2_NUJ] = 24.0;
		ap->default_val[PS2_NUJ] = 0;
		dz->maxmode = 0;
		break;
	case(PSOW_LOCATE):
		ap->lo[1]	 = 0.0;				/* time */
		ap->hi[1] = dz->duration;	
		ap->default_val[1] = 0.0;
		dz->maxmode = 0;
		break;
	case(PSOW_CUT):
		ap->lo[1]	 = 0.0;				/* time */
		ap->hi[1]	 = dz->duration;		
		ap->default_val[1] = 0.0;
		dz->maxmode = 2;
		break;
	case(PSOW_REINF):
		dz->maxmode = 2;
		if(dz->mode == 0) {
			ap->lo[ISTR] = 0;
			ap->hi[ISTR] = 1000;		
			ap->default_val[ISTR] = 0;
		} else {
			ap->lo[ISTR] = 1.0;
			ap->hi[ISTR] = 256.0;		
			ap->default_val[ISTR] = 4.0;
		}
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
			if((exit_status = setup_psow_application(dz))<0)
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

/****************************** SET_LEGAL_INTERNALPARAM_STRUCTURE *********************************/

int set_legal_internalparam_structure(int process,int mode,aplptr ap)
{
	int exit_status = FINISHED;
	if(process == PSOW_SYNTH) {
		switch(mode) {
		case(0):
		case(1):	
			exit_status = set_internalparam_data("00iiiiii",ap); break;
		case(2):
		case(3):	
			exit_status = set_internalparam_data("iiiiii",ap);	 break;
		case(4):	
			break;
		}
	}
	return(exit_status);		
}

/************************* SETUP_INTERNAL_ARRAYS_AND_ARRAY_POINTERS *******************/

int setup_internal_arrays_and_array_pointers(dataptr dz)
{
	int n;
	dz->array_cnt  = 0; 
	dz->larray_cnt = 0; 
	switch(dz->process) {
	case(PSOW_SYNTH):
		dz->array_cnt = 5; 
		dz->larray_cnt = 1; 
		break;
	case(PSOW_REINF):
		if(dz->mode == 0) {
			dz->array_cnt = 1; 
			dz->larray_cnt = 1; 
		} else {
			dz->array_cnt = 2; 
		}
		break;
	}
	if(dz->array_cnt) {
		if((dz->parray  = (double **)malloc(dz->array_cnt * sizeof(double *)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for internal double arrays.\n");
			return(MEMORY_ERROR);
		}
		for(n=0;n<dz->array_cnt;n++)
			dz->parray[n] = NULL;
	}
	if(dz->larray_cnt) {
		if((dz->lparray  = (int **)malloc(dz->larray_cnt * sizeof(int *)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for internal long arrays.\n");
			return(MEMORY_ERROR);
		}
		for(n=0;n<dz->larray_cnt;n++)
			dz->lparray[n] = NULL;
	}
	return(FINISHED);
}

/************************* READ_SPECIAL_DATA *******************/

int read_special_data(char *str,dataptr dz)	
{
	aplptr ap = dz->application;
	switch(ap->special_data) {
	case(SYNTHBANK):				return read_synth_data(str,dz);
	case(TIMEVARYING_SYNTHBANK):	return read_time_varying_synth_data(str,dz);
	case(PSOW_REINFORCEMENT):		return read_reinforcement_data(str,dz);
	case(PSOW_INHARMONICS):			return read_inharmonics_data(str,dz);
	}
	return(FINISHED);
}

/************************ HANDLE_THE_SPECIAL_DATA *********************/

int handle_the_special_data(int *cmdlinecnt,char ***cmdline,dataptr dz)
{
	int exit_status;
	aplptr ap = dz->application;

	if(ap->special_data) {
		if(!sloom) {
			if(*cmdlinecnt <= 0) {
				sprintf(errstr,"Insufficient parameters on command line.\n");
				return(USAGE_ONLY);
			}
		}
		if((exit_status = setup_the_special_data_ranges
		(dz->mode,dz->infile->srate,dz->duration,dz->nyquist,dz->wlength,dz->infile->channels,ap))<0)
			return(exit_status);

		if((exit_status = read_special_data((*cmdline)[0],dz))<0)
			return(exit_status);
		(*cmdline)++;		
		(*cmdlinecnt)--;
	}
	return(FINISHED);
}

/************************ SETUP_SPECIAL_DATA_RANGES *********************/

int setup_the_special_data_ranges(int mode,int srate,double duration,double nyquist,int wlength,int channels,aplptr ap)
{
	switch(ap->special_data) {
	case(SYNTHBANK):
	case(TIMEVARYING_SYNTHBANK):
		ap->special_range 		= TRUE;
		switch(mode) {
		case(0):
		case(2):
			ap->min_special 	= 0.1;
			ap->max_special 	= nyquist/2.0;
			break;
		case(1):
		case(3):
			ap->min_special 	= unchecked_hztomidi(0.1);
			ap->max_special 	= MIDIMAX;
			break;
		}
		ap->default_special		= 440.0;
		ap->other_special_range = TRUE;
		ap->min_special2 		= 0.0;
		ap->max_special2 		= 1.0;
		break;
	case(PSOW_REINFORCEMENT):
		ap->min_special 	= 2.0;
		ap->max_special 	= 256.0;
		ap->min_special2 	= FLTERR;
		ap->max_special2 	= 16.0;
		break;
	case(PSOW_INHARMONICS):
		ap->min_special 	= 1.0;
		ap->max_special 	= 256.0;
		ap->min_special2 	= FLTERR;
		ap->max_special2 	= 16.0;
		break;
	default:
		sprintf(errstr,"Unknown special_data type: setup_the_special_data_ranges()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/**************************** CHECK_PSOW_PARAM_VALIDITY_AND_CONSISTENCY *****************************/

int check_psow_param_validity_and_consistency(dataptr dz)
{
	int exit_status;
	int n;
	double time, time2;
	if(dz->process != PSOW_INTERP && dz->process != PSOW_EXTEND2) {
		if(dz->brksize[0] == 0) {
			sprintf(errstr,"PITCH PARAMETER MUST BE IN A BREAKPOINT FILE.\n");
			return(DATA_ERROR);
		}
		n = (dz->brksize[0] - 1) * 2;
		time  = dz->brk[0][n];
		time2 = (double)dz->insams[0]/(double)dz->infile->srate;
		if(fabs(time - time2) > .05) {
			sprintf(errstr,"PITCH DATA FILE DOES NOT CORRESPOND IN LENGTH (%lf) TO SOUND FILE (%lf).\n",time,time2);
			return(DATA_ERROR);
		}
	}
	switch(dz->process) {
	case(PSOW_FREEZE):
		if(!dz->brksize[PS_GAIN] && (dz->param[PS_GAIN] <= 0.0)) {
			sprintf(errstr,"ZERO output gain specified.\n");
			return(DATA_ERROR);
		}
		/* fall thro */
	case(PSOW_EXTEND):
		if((exit_status = read_value_from_brktable(dz->param[1],0,dz))<0)
			return(exit_status);
		if(flteq(dz->param[0],NOT_SOUND)) {
			sprintf(errstr,"There is no significant signal at time %lf in the file.\n",dz->param[1]);
			return(DATA_ERROR);
		} else if(flteq(dz->param[0],NOT_PITCH)) {
			fprintf(stdout,"WARNING: At time %lf there is no known pitch.\n",dz->param[1]);
			fflush(stdout);
		}
		if((exit_status = convert_pse_semit_to_octratio(dz))<0)
			return(exit_status);
		break;
	case(PSOW_EXTEND2):
		if(dz->param[1] <= dz->param[0]) {
			sprintf(errstr,"Start and End times for grain are incopatible.\n");
			return(DATA_ERROR);
		}
		dz->iparam[0] = (int)round(dz->param[0] * dz->infile->srate);
		dz->iparam[1] = (int)round(dz->param[1] * dz->infile->srate);
		break;
	case(PSOW_SYNTH):
		switch(dz->mode) {
		case(2):
		case(3):
			if((exit_status = allocate_tvarying_osc_arrays(dz))<0)
				return(exit_status);
			if((exit_status = put_tvarying_osc_data_in_arrays(dz->parray[PS_FBRK],dz))<0)
				return(exit_status);
			if((exit_status = initialise_psowsynth_internal_params(dz))<0)
				return(exit_status);
			break;
		}	
		break;
	case(PSOW_IMPOSE):
		dz->param[PS_GATE] = dbtogain(dz->param[PS_GATE]);
		break;
	case(PSOW_SPLIT):
		dz->param[PS_UTRNS] = pow(2.0,(dz->param[PS_UTRNS]/SEMITONES_PER_OCTAVE));
		break;
	case(PSOW_REINF):
		if(dz->mode==0)
			dz->iparam[ISTR] = (int)round(dz->param[ISTR] * MS_TO_SECS * (double)dz->infile->srate) * dz->infile->channels;
		break;
	}
	return FINISHED;
}

/*************************** CREATE_SNDBUFS **************************/

int create_psow_sndbufs(int minbuf,dataptr dz)
{
	int n;
	size_t bigbufsize;
	if(dz->sbufptr == 0 || dz->sampbuf==0) {
		sprintf(errstr,"buffer pointers not allocated: create_sndbufs()\n");
		return(PROGRAM_ERROR);
	}
	bigbufsize = (size_t) Malloc(-1);
	bigbufsize = max(bigbufsize,minbuf * sizeof(float) * dz->bufcnt);
	bigbufsize /= dz->bufcnt;
	if(bigbufsize <=0)
		bigbufsize  =  F_SECSIZE * sizeof(float);	  	  /* RWD keep ths for now */
	dz->buflen = (int)(bigbufsize / sizeof(float));	
	/*RWD also cover n-channels usage */
	bigbufsize = (size_t)(dz->buflen * sizeof(float));
	if((dz->bigbuf = (float *)malloc(bigbufsize  * dz->bufcnt)) == NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
		return(PROGRAM_ERROR);
	}
	if(dz->process==PSOW_SPACE) {
		dz->sampbuf[0] = dz->bigbuf;
		dz->sampbuf[1] = dz->bigbuf + dz->buflen;
		dz->sampbuf[2] = dz->bigbuf + (2 * dz->buflen);
		dz->sampbuf[3] = dz->bigbuf + (4 * dz->buflen);
	} else {
		for(n=0;n<dz->bufcnt;n++)
			dz->sbufptr[n] = dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
		dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
	}
	return(FINISHED);
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	if      (!strcmp(prog_identifier_from_cmdline,"stretch"))			dz->process = PSOW_STRETCH;
	else if (!strcmp(prog_identifier_from_cmdline,"dupl"))				dz->process = PSOW_DUPL;
	else if (!strcmp(prog_identifier_from_cmdline,"delete"))			dz->process = PSOW_DEL;
	else if (!strcmp(prog_identifier_from_cmdline,"strtrans"))			dz->process = PSOW_STRFILL;
	else if (!strcmp(prog_identifier_from_cmdline,"grab"))				dz->process = PSOW_FREEZE;
	else if (!strcmp(prog_identifier_from_cmdline,"chop"))				dz->process = PSOW_CHOP;
	else if (!strcmp(prog_identifier_from_cmdline,"interp"))			dz->process = PSOW_INTERP;
	else if (!strcmp(prog_identifier_from_cmdline,"features"))			dz->process = PSOW_FEATURES;
	else if (!strcmp(prog_identifier_from_cmdline,"synth"))				dz->process = PSOW_SYNTH;
	else if (!strcmp(prog_identifier_from_cmdline,"impose"))			dz->process = PSOW_IMPOSE;
	else if (!strcmp(prog_identifier_from_cmdline,"split"))				dz->process = PSOW_SPLIT;
	else if (!strcmp(prog_identifier_from_cmdline,"space"))				dz->process = PSOW_SPACE;
	else if (!strcmp(prog_identifier_from_cmdline,"interleave"))		dz->process = PSOW_INTERLEAVE;
	else if (!strcmp(prog_identifier_from_cmdline,"replace"))			dz->process = PSOW_REPLACE;
	else if (!strcmp(prog_identifier_from_cmdline,"sustain"))			dz->process = PSOW_EXTEND;
	else if (!strcmp(prog_identifier_from_cmdline,"sustain2"))			dz->process = PSOW_EXTEND2;
	else if (!strcmp(prog_identifier_from_cmdline,"locate"))			dz->process = PSOW_LOCATE;
	else if (!strcmp(prog_identifier_from_cmdline,"cutatgrain"))		dz->process = PSOW_CUT;
	else if (!strcmp(prog_identifier_from_cmdline,"reinforce"))			dz->process = PSOW_REINF;
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
	"\nOPERATIONS ON PITCH-SYNCHRONOUS GRAINS (FOFS)\n\n"
	"USAGE: psow NAME (mode) infile(s) outfile parameters: \n"
	"\n"
	"where NAME can be any one of\n"
	"\n"
	"stretch    dupl    delete    strtrans    features   grab    sustain   sustain2\n"
	"chop   interp    synth   impose    split    space    interleave   replace\n"
	"locate   cutatgrain   reinforce\n\n"
	"Type 'psow stretch' for more info on psow stretch..ETC.\n");
	return(USAGE_ONLY);
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
	if(!strcmp(str,"stretch")) {
		fprintf(stderr,
	    "USAGE:\n"
	    "psow stretch infile outfile pitch-brkpnt-data timestretch segcnt\n"
		"\n"
		"Timestretch/transpose a sound by repositioning the pitch-syncd grains.\n"
		"The grains themselves are not time-stretched.\n"
		"\n"
		"PITCH-BRKPNT-DATA  a breakpoint file with time and frq information.\n"
		"                   File may contain zeros (indicating moments of no-signal)\n"
		"                   but NOT pitch-zeros (indicating moments of no-pitch).\n"
		"                   It must contain SOME significant frequency information.\n"
		"TIMESTRETCH        Proportion by which sound is stretched (or shrunk).\n"
		"SEGCNT             no of grains in a chunk retained as-is,\n"
		"                   while gaps between segments are stretched.\n"
		"\n");
	} else if(!strcmp(str,"dupl")) {
		fprintf(stdout,
	    "USAGE:\n"
	    "psow dupl infile outfile pitch-brkpnt-data repeat-cnt segcnt\n"
		"\n"
		"Timestretch/transpose a sound by duplicating the pitch-syncd grains.\n"
		"\n"
		"PITCH-BRKPNT-DATA  a breakpoint file with time and frq information.\n"
		"                   File may contain zeros (indicating moments of no-signal)\n"
		"                   but NOT pitch-zeros (indicating moments of no-pitch).\n"
		"                   It must contain SOME significant frequency information.\n"
		"REPEAT-CNT         Number of repetitions of each chunk.\n"
		"SEGCNT             no of grains in a chunk .\n"
		"\n");
	} else if(!strcmp(str,"delete")) {
		fprintf(stdout,
	    "USAGE:\n"
	    "psow del infile outfile pitch-brkpnt-data propkeep segcnt\n"
		"\n"
		"Time shrink sound by deleting proportion of pitch-syncd grains.\n"
		"\n"
		"PITCH-BRKPNT-DATA  a breakpoint file with time and frq information.\n"
		"                   File may contain zeros (indicating moments of no-signal)\n"
		"                   but NOT pitch-zeros (indicating moments of no-pitch).\n"
		"                   It must contain SOME significant frequency information.\n"
		"PROPKEEP           Proportion of chunks to keep.\n"
		"                   '2' keeps 1 in 2:    '7' Keeps 1 in 7:   etc.\n"
		"SEGCNT             no of grains in a chunk.\n"
		"\n");
	} else if(!strcmp(str,"strtrans")) {
		fprintf(stdout,
	    "USAGE:\n"
		"psow strtrans infile outfile pitch-brkpnt-data timestretch segcnt trans\n"
		"\n"
		"Timestretch/transpose a sound by repositioning the pitch-syncd grains,\n"
		"and overlapping them.\n"
		"\n"
		"PITCH-BRKPNT-DATA  a breakpoint file with time and frq information.\n"
		"                   File may contain zeros (indicating moments of no-signal)\n"
		"                   but NOT pitch-zeros (indicating moments of no-pitch).\n"
		"                   It must contain SOME significant frequency information.\n"
		"TIMESTRETCH        Proportion by which sound is stretched (or shrunk).\n"
		"SEGCNT             no of grains in a chunk retained as-is,\n"
		"                   while gaps between segments are stretched.\n"
		"TRANS              Transposition in semitones, corresponds to overlap between\n"
		"                   succesive segments.\n"
		"                   NB This parameter interacts with 'Timestretch' parameter\n"
		"                   in unpredicable ways.\n"
		"\n");
	} else if(!strcmp(str,"grab")) {
		fprintf(stdout,
	    "USAGE:\n"
		"psow grab infil outfil pitch-brkpnt-data time dur segcnt spectrans density rand gain\n"
		"\n"
		"Grab a pitch-syncd grain from a file, and use it to create a new sound\n"
		"\n"
		"PITCH-BRKPNT-DATA  a breakpoint file with time and frq information.\n"
		"                   File may contain zeros (indicating moments of no-signal)\n"
		"                   but NOT pitch-zeros (indicating moments of no-pitch).\n"
		"                   It must contain SOME significant frequency information.\n"
		"TIME               Time at which to grab grain(s).\n"
		"DUR                Output file duration. Dur ZERO grabs SINGLE grain(chunk).\n"
		"SEGCNT             no of grains in a chunk.\n"
		"DENSITY            Rate at which the chunks in the outfile succeed one another.\n"
		"                   1: grains follow on, 2: grains overlap by 2, 3: by 3 etc\n"
		"                   0.5: grains separated by equivalent silence etc.\n"
		"                   overlap by 2 can transpose up by 8va, without changing spectrum.\n"
		"SPECTRANS          Transposition of spectrum (not fundamental) in semitones.\n"
		"RAND               Randomisation of position of grainchunks in output.\n"
		"                   Randomisation introduces noisiness into output sound.\n"
		"GAIN               Overall gain: may need to reduce this if density > 1 in output.\n"
		"\n");
	} else if(!strcmp(str,"chop")) {
		fprintf(stdout,
	    "USAGE:\n"
		"psow chop infile outfile pitch-brkpnt-data time-grain-pairs\n"
		"\n"
		"Chop sound into sections between specified grain(chunks)\n"
		"\n"
		"PITCH-BRKPNT-DATA  a breakpoint file with time and frq information.\n"
		"                   File may contain zeros (indicating moments of no-signal)\n"
		"                   but NOT pitch-zeros (indicating moments of no-pitch).\n"
		"                   It must contain SOME significant frequency information.\n"
		"TIME-GRAIN-PAIRS   File contains pairs of values: time   graincnt.\n"
		"                   'Time' is time of grain where file is cut.\n"
		"                   File is always cut at start-of-grain boundary.\n"
		"                   Next segment begins AFTER the specified grainchunk.\n"
		"                   'Graincnt' is no. of grains in chunk, at specified time,\n"
		"                   before the next cut-section starts.\n"
		"\n"
		"This process might be used in conjunction with 'psow grab' or 'interp',\n"
		"Grabbing a grain at a particular time in the sound, extending it,\n"
		"then reinserting it into the original sound by using the chopped components.\n"
		"\n");
	} else if(!strcmp(str,"interp")) {
		fprintf(stdout,
	    "USAGE: psow interp infile1 infile2 outfile startdur interpdur enddur\n"
		"                  vibfrq  vibdepth  tremfrq  tremdepth\n"
		"\n"
		"Interpolate between 2 pitch-synchronised grains, to produce new sound.\n"
		"\n"
		"STARTDUR   Duration to sustain initial grain.\n"
		"INTERPDUR  Duration of interpolation.\n"
		"ENDDUR     Duration to sustain final grain.\n"
		"VIBFRQ     Frequency of any added vibrato.\n"
		"VIBDEPTH   Depth (semitones) of any added vibrato.\n"
		"TREMFRQ    Frequency of any added tremolo.\n"
		"TREMDEPTH  Depth of any added tremolo.\n"
		"\n"
		"Process assumes your files each contain a single pitchsync grain.\n"
		"obtained using 'psow grab' with output duration 0.0\n"
		"\n");
	} else if(!strcmp(str,"features")) {
		fprintf(stdout,
	    "USAGE: psow features 1-2 infile1 outfile pitch-brkpnt-data segcnt\n"
		"              trans  vibfrq  vibdepth  spectrans  hoarseness  attenuation\n"
		"              subharmno  subharmamp fof-stretching [-a]\n"
		"Impose new features on vocal-type sound, preserving or modifying FOF-grains.\n"
		"\n"
		"PITCH-BRKPNT-DATA  a breakpoint file with time and frq information.\n"
		"                   File may contain zeros (indicating moments of no-signal)\n"
		"                   but NOT pitch-zeros (indicating moments of no-pitch).\n"
		"                   It must contain SOME significant frequency information.\n"
		"SEGCNT         No of grains in a chunk retained as-is.\n"
		"TRANS          Pitch transposition (semitones). Works differently in 2 modes.\n"
		"               1) Transpose accompanied by timewarp (pitch up, snd shorter).\n"
		"               2) Transposed pitch accompanied by additional lower pitch.\n"
		"VIBFRQ         Frequency of any added vibrato.\n"
		"VIBDEPTH       Depth (semitones) of any added vibrato.\n"
		"SPECTRANS      Transposition of spectrum (not fundamental) in semitones.\n"
		"HOARSENESS     Degree of hoarseness of voice (Range 0 to 1).\n"
		"ATTENUATION    Attenuation (maybe necessary when 'fof-stretching') (Range 0-1)\n"
		"SUBHARMNO      Amount by which fundamental divided, 0 or 1 gives NO subharm.\n"
		"SUBHARMAMP     Level of any subharmonic introduced.\n"
		"FOF-STRETCHING Time-extension of FOFs: does NOT stretch sound (Range 1 to 512)\n"
		"-a             Alternative algorithm for fof_stretch.\n");
	} else if(!strcmp(str,"synth")) {
		fprintf(stdout,
	    "USAGE: psow synth 1-5 infile1 outfile [oscdatafile] pitch-brkpnt-data depth\n"
		"\n"
		"Impose vocal FOFs on a stream of synthesized sound.\n"
		"\n"
		"OSCDATAFILE        (Amplitude values range is 0.0 to 1.0)\n"
		"  Mode 1: Each line has pair of values for frequency and amplitude.\n"
		"  Mode 2: Each line has pair of values for midipitch and amplitude.\n"
		"  Mode 3: Each line has frq and amp data in the 'filter varibank' format.\n"
		"  Mode 4: Each line has midipitch & amp data in 'filter varibank' format.\n"
		"  Mode 5: No 'oscdatafile' ... synthetic source is noise.\n"
		"PITCH-BRKPNT-DATA  a breakpoint file with time and frq information.\n"
		"                   File may contain zeros (indicating moments of no-signal)\n"
		"                   but NOT pitch-zeros (indicating moments of no-pitch).\n"
		"                   It must contain SOME significant frequency information.\n"
		"DEPTH              Depth of application of FOFS to the synthesized sound.\n"
		"\n");
	} else if(!strcmp(str,"impose")) {
		fprintf(stdout,
	    "USAGE: psow synth infile1 infile2 outfile pitch-brkpnt-data depth wsize gate\n"
		"\n"
		"Impose vocal FOFs in 1st sound onto the 2nd sound.\n"
		"\n"
		"PITCH-BRKPNT-DATA  a breakpoint file with time and frq information.\n"
		"                   File may contain zeros (indicating moments of no-signal)\n"
		"                   but NOT pitch-zeros (indicating moments of no-pitch).\n"
		"                   It must contain SOME significant frequency information.\n"
		"DEPTH  Depth of application of FOFS to the 2nd sound.\n"
		"WSIZE  Windowsize (mS) to envelope track 2nd sound (for normalisation).\n"
		"GATE   Level (decibels) in 2nd sound at which it's assumed to be zero.\n"
		"\n");
	} else if(!strcmp(str,"split")) {
		fprintf(stdout,
	    "USAGE: psow split infile1 outfile pitch-brkpnt-data subharmno uptrans balance\n"
		"\n"
		"Split vocal FOFs into subharmonic and upwardly transposed pitch.\n"
		"\n"
		"PITCH-BRKPNT-DATA  a breakpoint file with time and frq information.\n"
		"                   File may contain zeros (indicating moments of no-signal)\n"
		"                   but NOT pitch-zeros (indicating moments of no-pitch).\n"
		"                   It must contain SOME significant frequency information.\n"
		"SUBHARMNO  Subharmonic number (divides frequency of source) (Range 3-8).\n"
		"UPTRANS    Upward transposition in semitones (Range 0 - 48).\n"
		"BALANCE    Level of up-transposed components relative to subharms (0-8).\n"
		"\n");
	} else if(!strcmp(str,"space")) {
		fprintf(stdout,
	    "USAGE: psow space infile1 outfile\n"
		"          pitch-brkpnt-data subno separation balance hisuppress\n"
		"\n"
		"Split alternating FOFs between different spatial locations.\n"
		"\n"
		"PITCH-BRKPNT-DATA  a breakpoint file with time and frq information.\n"
		"                   File may contain zeros (indicating moments of no-signal)\n"
		"                   but NOT pitch-zeros (indicating moments of no-pitch).\n"
		"                   It must contain SOME significant frequency information.\n"
		"SUBNO       Subharmonic number (divides frequency of source) (Range 2-5).\n"
		"SEPARATION  spatial separation of alternate FOFs (range -1 to 1).\n"
		"     0:  no separation, all output is stereo-centred.\n"
		"     1:  alternate FOFS to widest spread, starting with far right position.\n"
		"     -1: alternate FOFS to widest spread, starting with far left position.\n"
		"     intermediate vals give intermediate degrees of spatial separation.\n"
		"BALANCE    of left:right components (0-8).\n"
		"     1.0:  leftward & rightward levels equal.\n"
		"     >1:   leftward signal divided by balance. (Bias to right)\n"
		"     <1:   rightward signal multiplied by balance. (Bias to left)\n"
		"HISUPPRESS Suppression of high-frequency components (0-1).\n"
		"\n");
	} else if(!strcmp(str,"interleave")) {
		fprintf(stdout,
	    "USAGE: psow interleave\n"
		"       infile1 infile2 outfile pbrk1 pbrk2 grplen bias bal weight\n"
		"\n"
		"Interleave FOFs from two different files.\n"
		"\n"
		"PBRK1  a breakpoint file with time & frq info about infile1.\n"
		"       File may contain zeros (indicating moments of no-signal)\n"
		"       but NOT pitch-zeros (indicating moments of no-pitch).\n"
		"       It must contain SOME significant frequency information.\n"
		"PBRK2  Similar file for infile 2.\n"
		"GRPLEN number of FOFS in each manipulated segment.\n"
		"BIAS   Is outpitch biased to one or other of infiles ?.\n"
		"       0:  no bias.  1:  biased to 1st.   -1: biased to 2nd.\n"
		"       intermediate vals give intermediate degrees of bias.\n"
		"BAL    level balance of components of 2 input files in output.\n"
		"       1.0:  equally loud.  >1:   1st louder.  <1:   2nd louder.\n"
		"WEIGTH relative number of components of 2 input files in output.\n"
		"       1.0:  equal. >1: more of 1st. <1: more of 2nd.\n"
		"\n");
	} else if(!strcmp(str,"replace")) {
		fprintf(stdout,
	    "USAGE: psow replace infile1 infile2 outfile pbrk1 pbrk2 grpcnt\n"
		"\n"
		"Combine fofs of first sound with pitch of 2nd.\n"
		"\n"
		"PBRK1  a breakpoint file with time & frq info about infile1.\n"
		"       File may contain zeros (indicating moments of no-signal)\n"
		"       but NOT pitch-zeros (indicating moments of no-pitch).\n"
		"       It must contain SOME significant frequency information.\n"
		"PBRK2  Similar file for infile 2.\n"
		"GRPCNT Number of FOFS in a chunk.\n"
		"\n"
		"This process assumes that BOTH the input files have pitched-grains\n"
		"\n");
	} else if(!strcmp(str,"sustain")) {
		fprintf(stdout,
	    "USAGE:\n"
		"psow sustain infil outfil pch-brkdata time dur segcnt\n"
		"      vibfrq vibdepth transpos gain [-s]\n"
		"\n"
		"Freeze and Sustain a sound on a specified pitch-syncd grain.\n"
		"\n"
		"PCH-BRKDATA  a breakpoint file with time and frq information.\n"
		"             File may contain zeros (indicating moments of no-signal)\n"
		"             but NOT pitch-zeros (indicating moments of no-pitch).\n"
		"             It must contain SOME significant frequency information.\n"
		"TIME         Time at which to freeze grain(s).\n"
		"DUR          Output file duration. (Greater than input file's duration).\n"
		"SEGCNT       no of grains in a chunk.\n"
		"VIBFRQ       Frequency of any added vibrato (added only to expanded grain).\n"
		"VIBDEPTH     Depth (semitones) of any added vibrato.\n"
		"TRANSPOS     Transposition (semitones) of grain.\n"
		"             Time zero in the brkpnt file for transposition parameter (ONLY)\n"
		"             = start of expanded grain, & NOT (necessarily) start of sound.\n"
		"GAIN         loudness contour of the entire output.\n"
		"-s           Smooth the grabbed fofs\n"
		"\n");
	} else if(!strcmp(str,"sustain2")) {
		fprintf(stdout,
	    "USAGE:\n"
		"psow sustain2 infil outfil start end  dur vibfrq vibdepth nudge\n"
		"\n"
		"Freeze and Sustain a sound on a specified grain.\n"
		"\n"
		"START          Time at which to cut grain.\n"
		"END            Time OF end of grain.\n"
		"DUR            Output file duration. (Greater than input file's duration).\n"
		"VIBFRQ         Frequency of any added vibrato.\n"
		"VIBDEPTH       Depth (semitones) of any added vibrato.\n"
		"NUDGE			Move selected grain position by 'nudge' zerocrossings\n"
		"\n");
	} else if(!strcmp(str,"locate")) {
		fprintf(stdout,
	    "USAGE: psow locate infil pitch-brkpnt-data time\n"
		"\n"
		"Gives exact time of grain-start nearest to specified time.\n"
		"\n"
		"PITCH-BRKPNT-DATA  a breakpoint file with time and frq information.\n"
		"                   File may contain zeros (indicating moments of no-signal)\n"
		"                   but NOT pitch-zeros (indicating moments of no-pitch).\n"
		"                   It must contain SOME significant frequency information.\n"
		"TIME           Time at which find grain.\n"
		"\n");
	} else if(!strcmp(str,"cutatgrain")) {
		fprintf(stdout,
	    "USAGE:\n"
		"psow cutatgrain 1-2 infil outfil pitch-brkpnt-data time\n"
		"\n"
		"Cuts the file at the specified time.\n"
		"Mode 1: Retains file BEFORE (exact) specified grain time.\n"
		"Mode 2: Retains file AT AND AFTER (exact) specified grain.\n"
		"\n"
		"PITCH-BRKPNT-DATA  a breakpoint file with time and frq information.\n"
		"                   File may contain zeros (indicating moments of no-signal)\n"
		"                   but NOT pitch-zeros (indicating moments of no-pitch).\n"
		"                   It must contain SOME significant frequency information.\n"
		"TIME       time at which to cut the file.\n"
		"\n");
	} else if(!strcmp(str,"reinforce")) {
		fprintf(stdout,
	    "USAGE:\n"
		"psow reinforce 1 infil outfil reinforcement-data pitch-brkpnt-data [-ddelay] [-s]\n"
		"psow reinforce 2 infil outfil reinforcement-data pitch-brkpnt-data [-wweight]\n"
		"\n"
		"MODE 1: Reinforce the harmonic content of the sound.\n"
		"MODE 2: Reinforce sound with inharmonic partials.\n"
		"\n"
		"REINFORCEMENT-DATA File of pairs of values, which represent\n"
		"     a harmonic number (2-256) + level relative to src (>0.0 - 16.0).\n"
		"     In MODE 2, the 'harmonics' may be fractional (range >1 - 256).\n"
		"\n"
		"PITCH-BRKPNT-DATA  a breakpoint file with time and frq information.\n"
		"                   File may contain zeros (indicating moments of no-signal)\n"
		"                   but NOT pitch-zeros (indicating moments of no-pitch).\n"
		"                   It must contain SOME significant frequency information.\n"
		"\n"
		"-s      FOFs generated for higher harmonics which coincide with\n"
		"        FOFs of lower harmonics, are omitted.\n"
		"delay   mS delay of onset of the added harmonics.\n"
		"weight  Sustain inharmonic components. Higher weight, longer sustain.\n"
		"        A very high weight may cause buffers to overflow. Default 4.0\n"
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

int get_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	return(FINISHED);
}

/**********************************************************/
/*	UBER-FUNCTION THAT EXTRACTS FOFS, AND CALLS PROCESSES */
/**********************************************************/

/************************* PSOA_WEIRD *******************************/

int psoa_weird(dataptr dz)
{
	int exit_status, iswarned = 0, one_grain = 0, display_time;
/*	double maxenv = 10000.0;*/
	int n, m, k, j, cutcnt, opos, ipos, writesize, seglen=0, next_opos, advance, repets;
	int *trofpnt=NULL, *cuttime=NULL;
	double *scanarray, time, dhere, dstep, thisplace, transinc, sum, diff, valdiff, maxamp, rand, gain=0.0, transpos, thisval;
	double maxlevel, spectranspos, envincr, envval, average_seglen, valleft, valright, spacepos=0.0, suppression, dur, timestep;
	int trofpntcnt = 0, cnt, startpos, orig_startpos=0, minwsize2, envcnt2=0;
	float *ibuf=NULL, *ibuf2=NULL, *ibuf3=NULL, *ibuf4=NULL, *obuf=NULL, *obuf2=NULL, *ovflw=NULL, *xbuf=NULL, val;
	int envcnt, minwsize, maxwsize = 0, here, here_in_buf=0, there_in_buf=0, lastwrite, startsamp, startsamp2, endsamp, overflow;
	int xbufpos, maxwritesize, bufstart, outseglen, transtep, startopos, endopos, startz, endz, vibshift;
	int bigarray, lastwritend, ssampsread2=0, limit, envtoptime, total_seglen, segcnt, cutstart = 0, cnt0, cnt1;
	int namelen, spaceswitch, done_freeze, got;
	char *outfilename;
	double *fofenv, *sintab, srate = (double)dz->infile->srate, *amp, atten, *ihno=NULL, maxsamp, normaliser, ingraintime;
	int *fofloc, segstep, bakstep;
	int maxseglen, seglen0, seglen1, startcut=0, endcut=0, offset, nextcut, lastcut, startbuf = 0, last_sampsread=0;
	int zcnt = 0, final_pos = 0, sig_cnt = 0, zcntb = 0, final_posb = 0, sig_cntb = 0, *hno=NULL;

	int grainlen /*= there_in_buf - here_in_buf*/;
	double sinstep, thissin;
	int stsmoothed[2];
	switch(dz->process) {
	case(PSOW_FEATURES):
	case(PSOW_SYNTH):
	case(PSOW_SPLIT):
	case(PSOW_SPACE):
	case(PSOW_REPLACE):
	case(PSOW_EXTEND):
	case(PSOW_EXTEND2):
	case(PSOW_CUT):
	case(PSOW_REINF):
		ibuf  = dz->sampbuf[0];
		ibuf2 = dz->sampbuf[1];
		obuf  = dz->sampbuf[2];
		obuf2 = dz->sampbuf[3];
		break;
	case(PSOW_LOCATE):
		ibuf  = dz->sampbuf[0];
		ibuf2 = dz->sampbuf[1];
		break;
	case(PSOW_IMPOSE):
	case(PSOW_INTERLEAVE):
		ibuf  = dz->sampbuf[0];
		ibuf2 = dz->sampbuf[1];
		ibuf3 = dz->sampbuf[2];
		ibuf4 = dz->sampbuf[3];
		obuf  = dz->sampbuf[4];
		obuf2 = dz->sampbuf[5];
		break;
	default:
		ibuf  = dz->sampbuf[0];
		obuf  = dz->sampbuf[1];
		obuf2 = dz->sampbuf[2];
		break;
	}
	if(dz->process==PSOW_EXTEND2) {
		offset = max(0,dz->iparam[0] - dz->buflen/2);
		if(offset > 0){
            if((sndseekEx(dz->ifd[0],offset,0)<0)){
                sprintf(errstr,"sndseek() failed\n");
                return SYSTEM_ERROR;
            }
        }
		if((exit_status = read_samps(dz->sampbuf[0],dz))<0)
			return exit_status;
		here = dz->iparam[0] - offset;
		if(dz->iparam[0] == 0) {
			here = 0;
			if(dz->iparam[PS2_NUJ] < 0) {
				sprintf(errstr,"Nudge incompatible with given start position");
				return(DATA_ERROR);
			}
		} else {
			got = 0;
			nextcut = next_down_zcross(here,dz->buflen,dz->sampbuf[0]);
			lastcut = last_down_zcross(here,0,dz->sampbuf[0]);
			if(lastcut < 0) {
				if(offset == 0)
					lastcut = 0;
			}
			if(nextcut < 0) {
				here = lastcut;
				got = 1;
			}
			if(lastcut == 0) {
				if(nextcut >= 0) {
					if(nextcut - here > here)
						here = nextcut;
					else
						here = 0;
				}
				got = 1;
			} else if(lastcut < 0) {
				here = nextcut;
				got = 1;
			}
			if(here < 0) {
				sprintf(errstr,"Failed to find zero-cross point near time specified\n");
				return(DATA_ERROR);
			}
			if(!got) {
				if(nextcut - here < here -lastcut) 
					here = nextcut;
				else
					here = lastcut;
			}
			k = dz->iparam[PS2_NUJ]; 
			while(k > 0) {
				here = next_down_zcross(here,dz->buflen,dz->sampbuf[0]);
				if(here < 0) {
					sprintf(errstr,"Nudged zero-cross not found in buffer.\n");
					return(DATA_ERROR);
				}
				k--;
			}
			while(k < 0) {
				here = last_down_zcross(here,0,dz->sampbuf[0]);
				if(here < 0) {
					sprintf(errstr,"Nudged zero-cross not found in buffer.\n");
					return(DATA_ERROR);
				}
				k++;
			}
		}
		startcut = here + offset;

		if(dz->iparam[1] >= dz->insams[0]) {
			here = dz->insams[0];
			if(dz->iparam[PS2_NUJ] > 0) {
				sprintf(errstr,"Nudge incompatible with given end position");
				return(DATA_ERROR);
			}
		} else {
			got = 0;
			here = dz->iparam[1] - offset;
			nextcut = next_down_zcross(here,dz->buflen,dz->sampbuf[0]);
			lastcut = last_down_zcross(here,0,dz->sampbuf[0]);
			if(nextcut < 0) {
				here = lastcut;
				got = 1;
			} else if(lastcut < 0) {
				here = nextcut;
				got = 1;
			}
			if(here < 0) {
				sprintf(errstr,"Failed to find zero-cross point near time specified\n");
				return(DATA_ERROR);
			}
			if(!got) {
				if(nextcut - here < here -lastcut)  {
					here = nextcut;
				} else {
					here = lastcut;
				}
			}
			k = dz->iparam[PS2_NUJ]; 
			while(k > 0) {
				here = next_down_zcross(here,dz->buflen,dz->sampbuf[0]);
				if(here < 0) {
					sprintf(errstr,"Nudged zero-cross not found in buffer.\n");
					return(DATA_ERROR);
				}
				k--;
			}
			while(k < 0) {
				here = last_down_zcross(here,0,dz->sampbuf[0]);
				if(here < 0) {
					sprintf(errstr,"Nudged zero-cross not found in buffer.\n");
					return(DATA_ERROR);
				}
				k++;
			}
		}
		endcut = here + offset;
		if((seglen = endcut-startcut) <= 0) {
			sprintf(errstr,"Times too close to find a grain.\n");
			return(DATA_ERROR);
		}
		memset((char *)dz->sampbuf[0],0,dz->buflen * sizeof(float));
        if((sndseekEx(dz->ifd[0],0,0)<0)){
            sprintf(errstr,"sndseek() failed\n");
            return SYSTEM_ERROR;
        }
		dz->total_samps_read = 0;
		dz->samps_left = dz->insams[0];
	} else {
		if((exit_status = get_min_wsize(&minwsize,0,dz)) < 0)
			return exit_status;
		if(((envcnt = dz->insams[0]/minwsize) * minwsize)!=dz->insams[0])
			envcnt++;
		if((dz->env=(float *)malloc(((envcnt+20) * 2) * sizeof(float)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for envelope array.\n");
			return(MEMORY_ERROR);
		}
		bigarray = envcnt+20;
		if(dz->process == PSOW_INTERLEAVE || dz->process == PSOW_REPLACE) {
			if((exit_status = get_min_wsize(&minwsize2,1,dz)) < 0)
				return exit_status;
			if(((envcnt2 = dz->insams[1]/minwsize2) * minwsize2)!=dz->insams[1])
				envcnt2++;
			bigarray += envcnt2 + 20;
		}
		memset(dz->env,0,(envcnt+20) * 2 * sizeof(float));	/* PRESET env VALS TO ZERO */
						/* PART ENVELOPE vals CAN THEN BE STORED, where env-extract overlaps a buffer boundary */
		fprintf(stdout,"INFO: Extracting pitch-related envelope from file.\n");
		fflush(stdout);
		if((exit_status = extract_pitch_dependent_env_from_sndfile(minwsize,0,&maxwsize,dz))<0)
			return(exit_status);
		envcnt = dz->envend - dz->env;

	// NB EACH ENVELOPE-ITEM CONSISTS OF TWO VALS (time & env-val).
	// REVISED HERE TAKING THIS INTO ACCOUNT

		fprintf(stdout,"INFO: Locating envelope peaks.\n");
		fflush(stdout);
		if((trofpnt = (int *)malloc(envcnt/2 * sizeof(int)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY TO ANALYSE ENVELOPE.\n");
			return(MEMORY_ERROR);
		}
		if((cuttime = (int *)malloc(bigarray/2 * sizeof(int)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY TO ANALYSE ENVELOPE.\n");
			return(MEMORY_ERROR);
		}
		if((exit_status = get_envelope_troughs(trofpnt,&trofpntcnt,envcnt,dz))<0)
			return(exit_status);
		if((scanarray = (double *)malloc((maxwsize + 20) * sizeof(double)))==NULL) {
			sprintf(errstr,"Insufficient memory for array to scan for zero-crossings.\n");
			return(MEMORY_ERROR);
		}
		if((sndseekEx(dz->ifd[0],0,0)<0)){
            sprintf(errstr,"sndseek() failed\n");
            return SYSTEM_ERROR;
        }
		dz->total_samps_read = 0;
		dz->samps_left = dz->insams[0];
		if(sloom)
			display_virtual_time(dz->total_samps_read,dz);
		fprintf(stdout,"INFO: Calculating cut points.\n");
		fflush(stdout);
		if((exit_status = read_samps(ibuf,dz))<0)
			return(exit_status);
		k = 0;
		cutcnt = 0;
		if(dz->env[trofpnt[0]] <= 0.0)	/* if first trof is at zero, skip that one */
			k++;
		for(n = k;n<trofpntcnt;n++) {	/* find places to cut the source */
			if((exit_status = find_min_energy_downward_zero_crossing_point(&n,trofpnt,trofpntcnt,scanarray,&cutcnt,cuttime,0,cutstart,dz)) < 0)
				return(exit_status);
			cutcnt++;
		}
		if((exit_status = smooth_cuts(cuttime,&cutcnt,0,cutstart,dz))<0)
			return(exit_status);
#ifdef PSOWTEST
		for(n = 0;n < cutcnt;n++) {
			fprintf(stdout,"INFO: cuttime[%d] = %d\n",n,cuttime[n]);
			fflush(stdout);
		}
#endif
		if((exit_status = chop_zero_signal_areas_into_compatible_units(&cutcnt,cuttime,cutstart,0,dz)) < 0)
			return(exit_status);
		
		if(dz->process == PSOW_INTERLEAVE || dz->process == PSOW_REPLACE) {
			free(dz->env);
			if((dz->env=(float *)malloc(((envcnt2+20) * 2) * sizeof(float)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY for 2nd envelope array.\n");
				return(MEMORY_ERROR);
			}
			memset(dz->env,0,(envcnt2+20) * 2 * sizeof(float));	/* PRESET env VALS TO ZERO */
							/* PART ENVELOPE vals CAN THEN BE STORED, where env-extract overlaps a buffer boundary */
			fprintf(stdout,"INFO: Extracting pitch-related envelope from 2nd file.\n");
			fflush(stdout);
			maxwsize = 0;
			if((exit_status = extract_pitch_dependent_env_from_sndfile(minwsize2,1,&maxwsize,dz))<0)
				return(exit_status);
			envcnt2 = dz->envend - dz->env;

		// NB EACH ENVELOPE-ITEM CONSISTS OF TWO VALS (time & env-val).
		// REVISED HERE TAKING THIS INTO ACCOUNT

			fprintf(stdout,"INFO: Locating envelope peaks for 2nd file.\n");
			fflush(stdout);
			free(trofpnt);
			if((trofpnt = (int *)malloc(envcnt2/2 * sizeof(int)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY TO ANALYSE ENVELOPE OF 2ND FILE.\n");
				return(MEMORY_ERROR);
			}
			trofpntcnt = 0;
			if((exit_status = get_envelope_troughs(trofpnt,&trofpntcnt,envcnt2,dz))<0)
				return(exit_status);
			free(scanarray);
			if((scanarray = (double *)malloc((maxwsize + 20) * sizeof(double)))==NULL) {
				sprintf(errstr,"Insufficient memory for array to scan for zero-crossings in 2nd file.\n");
				return(MEMORY_ERROR);
			}
			if((sndseekEx(dz->ifd[1],0,0)<0)){
                sprintf(errstr,"sndseek() failed\n");
                return SYSTEM_ERROR;
            }
			dz->total_samps_read = 0;
			dz->samps_left = dz->insams[1];
			if(sloom)
				display_virtual_time(dz->total_samps_read,dz);
			fprintf(stdout,"INFO: Calculating cut points in 2nd file.\n");
			fflush(stdout);
			if((dz->ssampsread = fgetfbufEx(dz->sampbuf[0], dz->buflen,dz->ifd[1],0)) < 0) {
				sprintf(errstr,"Can't read samples from input soundfile 2\n");
				return(SYSTEM_ERROR);
			}
			dz->total_samps_read += dz->ssampsread;
			k = 0;
			cutstart = cutcnt;
			if(dz->env[trofpnt[0]] <= 0.0)	/* if first trof is at zero, skip that one */
				k++;
			for(n = k;n<trofpntcnt;n++) {	/* find places to cut the source */
				if((exit_status = find_min_energy_downward_zero_crossing_point(&n,trofpnt,trofpntcnt,scanarray,&cutcnt,cuttime,1,cutstart,dz)) < 0)
					return(exit_status);
				cutcnt++;
			}
			if((exit_status = smooth_cuts(cuttime,&cutcnt,1,cutstart,dz))<0)
				return(exit_status);
			if((exit_status = chop_zero_signal_areas_into_compatible_units(&cutcnt,cuttime,cutstart,1,dz)) < 0)
				return(exit_status);
		}
	}
	opos = 0;
	switch(dz->process) {
	case(PSOW_FREEZE):
		j = (int)round(dz->param[1] * srate);	/* Time at which to look for frozen segment */
		for(n = 0; n < cutcnt;n++) {
			if(cuttime[n] >= j) {
				if(cuttime[n] == j)
					n++;
				break;
			}
		}
		n--;
		startsamp = cuttime[n];
		j = dz->iparam[PS_SEGS];								/* no. of segs to use */
		if((k = n+j) > cutcnt) {
			sprintf(errstr,"Insufficient segments after time %f secs\n",dz->param[1]);
			return(GOAL_FAILED);
		}
		if(k == cutcnt) {
			endsamp = dz->insams[0];
			if(k-1 > n)
				fprintf(stdout,"INFO: grabbing grain-segments %d at time %lf to end of file\n",n,(double)startsamp/srate);
			else
				fprintf(stdout,"INFO: grabbing grain-segment %d at time %lf to end of file\n",n,(double)startsamp/srate);
			fflush(stdout);
		} else {
			endsamp = cuttime[k];
			if(k-1 > n)
				fprintf(stdout,"INFO: grabbing grain-segments %d at time %lf to %d\n",n,(double)startsamp/srate,k-1);
			else 
				fprintf(stdout,"INFO: grabbing grain-segment %d at time %lf\n",n,(double)startsamp/srate);
			fflush(stdout);
		}
		seglen = endsamp - startsamp;
		if(seglen > dz->buflen) {
			sprintf(errstr,"Specified chunk is too large to fit in buffer\n");
			return(PROGRAM_ERROR);
		}
        if((sndseekEx(dz->ifd[0],startsamp,0)<0)){
            sprintf(errstr,"sndseek() failed\n");
            return SYSTEM_ERROR;
        }
		if((exit_status = read_samps(ibuf,dz))<0)
			return(exit_status);
		for(n=0;n<seglen;n++)
			ibuf[n] = (float)(ibuf[n] * dz->param[PS_GAIN]);	/* adjust gain of input */
		maxwritesize = (int)round(dz->param[PS_DUR] * srate);
		if(maxwritesize == 0) {
			one_grain = 1;
			maxwritesize = 1;
		}
		writesize = 0;
		here_in_buf = 0;
		maxamp = 0.0;
		while(writesize < maxwritesize) {
			time = (double)writesize/srate;
			if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
				return(exit_status);
			if(!flteq(dz->param[PS_TRNS],1.0)) {
				thisplace = 0.0;
				transinc = dz->param[PS_TRNS];
				obuf[opos++] = ibuf[0];
				while(thisplace < (double)seglen) {
					thisplace += transinc;
					j = (int)floor(thisplace);
					k = j+1;
					diff = thisplace - (double)j;
					valdiff = ibuf[k] - ibuf[j];
					valdiff *= diff;
					val = (float)(ibuf[j] + valdiff);
					if((k >= seglen) && (val < 0.0))			/* Seg ends just before seg crosses zero downwards. */
						val = 0.0;								/* If interpolating from final samp to first samp after, */	
																/* once interpolated value crosses zero, break */
					obuf[opos++] += val;
				}
			} else {
				for(n=0;n<seglen;n++)
					obuf[opos++] += ibuf[n];
			}
			if(one_grain)
				break;
			dstep = (double)seglen/dz->param[PS_DENS];
			if(dz->param[PS_RAND] > 0.0) {
				rand = drand48() - 0.5;		/* rand number between -0.5 & 0.5 */
				rand *= dz->param[PS_RAND];	/* reduced according to degree of randomness */
				rand *= dstep;
				dstep += rand;
			}
			if((j = (int)round(dstep)) <= 0) {
				if(!iswarned) {
					fprintf(stdout,"WARNING: Density too high at %lf secs: using max density %d\n",time,seglen);
					fflush(stdout);
					iswarned = 1;
				}
				j = 1;
			}
			here_in_buf += j;
			writesize   += j;
			if(here_in_buf > dz->buflen * 2) {
				sprintf(errstr,"Output buffer overflowed.\n");
				return(PROGRAM_ERROR);
			}
			if(here_in_buf + dz->total_samps_written >= maxwritesize)
				break;
			if(here_in_buf >= dz->buflen) {
				for(n=0;n<dz->buflen;n++)
					maxamp = max(fabs(obuf[n]),maxamp);
				if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
					return(exit_status);
				memset((char*)obuf,0,dz->buflen * sizeof(float));
				memcpy((char*)obuf,(char*)obuf2,(opos - dz->buflen) * sizeof(float));
				memset((char*)obuf2,0,dz->buflen * sizeof(float));
				here_in_buf -= dz->buflen;
			}
			opos = here_in_buf;
		}
		if(opos > 0) {
			if((exit_status = write_samps(obuf,opos,dz))<0)
				return(exit_status);
		}
		if(maxamp > 1.0) {
			fprintf(stdout,"ERROR: Output overloaded: set gain (at least) to %.2lf\n",1.0/maxamp);
			fflush(stdout);
		}
		return FINISHED;
	case(PSOW_CHOP):
					/* CHECK INPUT DATA, NOW SEGS ARE LOCATED */

		if((dz->brksize[1] + 1) > 999) {					/* Check for too many segments */
			sprintf(errstr,"Too many segments to cut (%d)\n",dz->brksize[1]);
			return(GOAL_FAILED);
		}
		for(n=0,m=0;n<dz->brksize[1];n++,m+=2) {			/* Check input data */
			here   = (int)round(dz->brk[1][m] * srate);
			seglen = (int)round(dz->brk[1][m+1]);
			for(j=0;j<cutcnt;j++) {
				if(cuttime[j] >= here) {
					if(cuttime[j] == here)
						j++;
					break;
				}
			}
			j--;
			if(j == 0)				/* Time before first segend (before first cut) */
				k = 0;				/* Chop at end of first seg (a chop before this does nothing) */
			else if(j == cutcnt)	/* Time in last seg (after last cut) */
				k = cutcnt - 1;		/* Chop before last seg */
			else
				k = j;				/* Time within segement after cut j: chop out this segment */
			j = k;
			k += seglen;
			if(k > cutcnt) {
				sprintf(errstr,"Insufficient segments (%d asked for) after time %lf\n",seglen,dz->brk[1][m]);
				return(GOAL_FAILED);
			}
			if(seglen > 1) 
				fprintf(stdout,"INFO: cutting around grain-segments %d at time %lf to %d\n",j,dz->brk[1][m],k-1);
			else
				fprintf(stdout,"INFO: cutting around grain-segment %d at time %lf\n",j,dz->brk[1][m]);
		}
					/* COPY FILENAME, FOR GENERIC USE FOR ALL OUTFILES */

		if(!sloom)
			keep_filename_tail_only(dz);					
		namelen = strlen(dz->wordstor[0]);					
		if((outfilename = (char *)malloc((namelen + 5) * sizeof(char)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for outfilename.\n");
			return(MEMORY_ERROR);
		}				
		strcpy(outfilename,dz->wordstor[0]);

					/* START READING INFLE  */

		dz->total_samps_read = 0;
		dz->samps_left = dz->insams[0];
        if((sndseekEx(dz->ifd[0],0,0)<0)){
            sprintf(errstr,"sndseek() failed\n");
            return SYSTEM_ERROR;
        }
		if((exit_status = read_samps(ibuf,dz))<0)
			return(exit_status);
		startsamp = 0;
		bufstart = 0;
		there_in_buf = 0;

					/* CUT SEGMENTS FROM THE INFILE */

		for(n=0,m=0;n<dz->brksize[1];n++,m+=2) {			/* Read cutpoints */
			memset((char *)obuf,0,dz->buflen * sizeof(float));
			opos = 0;
			here   = (int)round(dz->brk[1][m] * dz->infile->srate);
			seglen = (int)round(dz->brk[1][m+1]);
			for(j=0;j<cutcnt;j++) {
				if(cuttime[j] >= here) {
					if(cuttime[j] == here)
						j++;
					break;
				}
			}
			j--;
			if(j == 0)				
				k = -1;
			else if(j == cutcnt)	
				k = cutcnt - 1;
			else					
				k = j;
			here = cuttime[k];
			while(here > dz->total_samps_read) {				/* If not enough samps in buf to reach "here" */
				for(j=there_in_buf;j < dz->ssampsread;j++) {	/* Write rest of samps in buffer  ETC. */
					obuf[opos++] = ibuf[j];
					if(opos >= dz->ssampsread) {
						if((exit_status = write_samps(obuf,opos,dz))<0)
							return(exit_status);
						opos = 0;
					}
				}
				if((exit_status = read_samps(ibuf,dz))<0)
					return(exit_status);
				there_in_buf = 0;
				bufstart += dz->buflen;
			}
			here_in_buf = here - bufstart;
			for(j=there_in_buf;j < here_in_buf;j++) {		/* Write samps between last start and here */
				obuf[opos++] = ibuf[j];
				if(opos >= dz->ssampsread) {
					if((exit_status = write_samps(obuf,opos,dz))<0)
						return(exit_status);
					opos = 0;
				}
			}
			if(opos > 0) {
				if((exit_status = write_samps(obuf,opos,dz))<0)
					return(exit_status);
			}
					/* WRITE HEADER AND CLOSE FILE */

			if((exit_status = headwrite(dz->ofd,dz))<0) {	
				free(outfilename);
				return(exit_status);
			}
			if((exit_status = reset_peak_finder(dz))<0)
				return(exit_status);
			if(sndcloseEx(dz->ofd) < 0) {
				fprintf(stdout,"WARNING: Can't close output soundfile %s\n",outfilename);
				fflush(stdout);
			}
			dz->ofd = -1;

					/* CREATE NEXT OUTFILE */

			strcpy(outfilename,dz->wordstor[0]);
			create_next_filename(outfilename,(int)n);
			if((exit_status = create_sized_outfile(outfilename,dz))<0) {
				sprintf(errstr,"Failed to make segment %d and beyond\n",n+1);
				return(exit_status);
			}

					/* SKIP OVER THE GRAIN(S) */

			startsamp = here + seglen;
			while(startsamp > dz->total_samps_read) {
				if((exit_status = read_samps(ibuf,dz))<0)
					return(exit_status);
				bufstart += dz->buflen;
			}
			there_in_buf = startsamp - bufstart;
		}
					/* WRITE SEGMENT REMAINING AT FILE END */

		memset((char *)obuf,0,dz->buflen * sizeof(float));
		opos = 0;
		while(dz->ssampsread > 0) {
			for(j = there_in_buf; j < dz->ssampsread; j++) {
				obuf[opos++] = ibuf[j];
				if(opos >= dz->buflen) {
					if((exit_status = write_samps(obuf,opos,dz))<0)
						return(exit_status);
					opos = 0;
				}
			}
			if((exit_status = read_samps(ibuf,dz))<0)
				return(exit_status);
			there_in_buf = 0;
		}
		if(opos > 0) {
			if((exit_status = write_samps(obuf,opos,dz))<0)
				return(exit_status);
		}
		return FINISHED;
	}
	here = 0;
	dz->total_samps_read = 0;
	dz->samps_left = dz->insams[0];
	if((sndseekEx(dz->ifd[0],0,0)<0)){
        sprintf(errstr,"sndseek() failed\n");
        return SYSTEM_ERROR;
    }
	memset((char*)ibuf,0,dz->buflen * sizeof(float));
	if((exit_status = read_samps(ibuf,dz))<0)
		return(exit_status);
	if(dz->process != PSOW_LOCATE) {
		fprintf(stdout,"INFO: Creating the outfile.\n");
		fflush(stdout);
	}
	switch(dz->process) {
	case(PSOW_FEATURES):
		memset((char*)ibuf2,0,dz->buflen * sizeof(float));
		if((exit_status = read_samps(ibuf2,dz))<0)
			return(exit_status);
		j = dz->iparam[1];
		break;
	case(PSOW_SYNTH):
	case(PSOW_IMPOSE):
	case(PSOW_SPLIT):
	case(PSOW_SPACE):
		memset((char*)ibuf2,0,dz->buflen * sizeof(float));
		if((exit_status = read_samps(ibuf2,dz))<0)
			return(exit_status);
		j = 1;
		break;
	case(PSOW_REPLACE):
		memset((char*)ibuf2,0,dz->buflen * sizeof(float));
		if((exit_status = read_samps(ibuf2,dz))<0)
			return(exit_status);
		j = dz->iparam[2];
		break;
	case(PSOW_EXTEND2):
		last_sampsread = dz->ssampsread;
		/* fall thro */
	case(PSOW_EXTEND):
		memset((char*)ibuf2,0,dz->buflen * sizeof(float));
		if((exit_status = read_samps(ibuf2,dz))<0)
			return(exit_status);
		j = dz->iparam[3];
		break;
	case(PSOW_INTERLEAVE):
		memset((char*)ibuf2,0,dz->buflen * sizeof(float));
		if((exit_status = read_samps(ibuf2,dz))<0)
			return(exit_status);
		if((exit_status = read_samps(ibuf2,dz))<0)
			return(exit_status);
        if((sndseekEx(dz->ifd[1],0,0)<0)){
            sprintf(errstr,"sndseek() failed\n");
            return SYSTEM_ERROR;
        }
		memset((char*)ibuf3,0,dz->buflen * sizeof(float));
		if((ssampsread2 = fgetfbufEx(ibuf3, dz->buflen,dz->ifd[1],0)) < 0) {
			sprintf(errstr,"Can't read samples from input soundfile 2\n");
			return(SYSTEM_ERROR);
		}
		memset((char*)ibuf4,0,dz->buflen * sizeof(float));
		if((ssampsread2 = fgetfbufEx(ibuf4, dz->buflen,dz->ifd[1],0)) < 0) {
			sprintf(errstr,"Can't read samples from input soundfile 2\n");
			return(SYSTEM_ERROR);
		}
		j = dz->iparam[2];
		break;
	case(PSOW_REINF):
		memset((char*)ibuf2,0,dz->buflen * sizeof(float));
		if((exit_status = read_samps(ibuf2,dz))<0)
			return(exit_status);
		j = 1;
		break;
	default:
		j = dz->iparam[2];
		break;
	}
	if(dz->process != PSOW_EXTEND2) {
		if(j >= cutcnt) {
			sprintf(errstr,"Insufficient Grains in source (%d) to create blocksize requested (%d).\n",cutcnt,j);
			return(DATA_ERROR);
		}
	}
	lastwrite = 0;
	startsamp = 0;
	stsmoothed[0] = 0;
	stsmoothed[1] = 0;
	if(dz->process != PSOW_LOCATE) {
		memset((char*)obuf,0,dz->buflen * sizeof(float));
		memset((char*)obuf2,0,dz->buflen * sizeof(float));
	}
	switch(dz->process) {
	case(PSOW_STRETCH):
		maxsamp = 0.0;
		ovflw = dz->sampbuf[3];
		memset((char*)ovflw,0,dz->buflen * sizeof(float));
		for(n = j-1; n < cutcnt;n+=j) {
 			here_in_buf = here - startsamp;
			there_in_buf = cuttime[n] - startsamp;
			if(dz->brksize[1]) {
				time = here/srate;
				if((exit_status = read_value_from_brktable(time,1,dz))<0)
					return(exit_status);
			}
			opos = (int)round(here * dz->param[1]);	/* absolute output position */
			opos -= lastwrite;							/* output position relative to buf start */
			if((overflow = opos - (dz->buflen * 2))>0) {
				if((exit_status = eliminate_too_short_events(&zcnt,&final_pos,&sig_cnt,dz->buflen,0,obuf,1,dz)) < 0)
					return(exit_status);
				if((exit_status = smooth_bad_events(&zcntb,&final_posb,&sig_cntb,stsmoothed,dz->buflen,0,obuf,1,dz)) < 0)
					return(exit_status);
				for(m = 0;m < dz->buflen;m++)
					maxsamp = max(maxsamp,fabs(obuf[m]));
				lastwrite += dz->buflen; 
				memcpy((char*)obuf,(char*)obuf2,dz->buflen * sizeof(float));
				memset((char*)obuf2,0,dz->buflen * sizeof(float));
				memcpy((char*)obuf2,(char*)ovflw,overflow * sizeof(float));
				memset((char*)ovflw,0,dz->buflen * sizeof(float));
				opos -= dz->buflen;
			}
			if((opos + (there_in_buf - here_in_buf)) >= dz->buflen * 3) {
				sprintf(errstr,"OVERFLOWED OUTPUT BUFFER.\n");
				return(MEMORY_ERROR);
			}
			while(there_in_buf > dz->buflen) {
				there_in_buf = dz->buflen;
				for(k = here_in_buf; k < there_in_buf;k++)
					obuf[opos++] += ibuf[k];
				if((exit_status = read_samps(ibuf,dz))<0)
					return(exit_status);
				startsamp += dz->buflen;
				here_in_buf = 0;
				there_in_buf = cuttime[n] - startsamp;
			}
			for(k = here_in_buf; k < there_in_buf;k++)
				obuf[opos++] += ibuf[k];
			here = cuttime[n];
		}
		if(opos > 0) {
			if((exit_status = eliminate_too_short_events(&zcnt,&final_pos,&sig_cnt,opos,1,obuf,1,dz)) < 0)
				return(exit_status);
			if((exit_status = smooth_bad_events(&zcntb,&final_posb,&sig_cntb,stsmoothed,opos,1,obuf,1,dz)) < 0)
				return(exit_status);
			for(m = 0;m < opos;m++)
				maxsamp = max(maxsamp,fabs(obuf[m]));
		}
		if(maxsamp > 1.0)
			normaliser = 1.0/maxsamp;
		else
			normaliser = 1.0;
		memset((char*)obuf,0,dz->buflen * sizeof(float));
		memset((char*)obuf2,0,dz->buflen * sizeof(float));
        if((sndseekEx(dz->ifd[0],0,0)<0)){
            sprintf(errstr,"sndseek() failed\n");
            return SYSTEM_ERROR;
        }
		here = 0;
		ovflw = dz->sampbuf[3];
		memset((char*)ovflw,0,dz->buflen * sizeof(float));
		for(n = j-1; n < cutcnt;n+=j) {
 			here_in_buf = here - startsamp;
			there_in_buf = cuttime[n] - startsamp;
			if(dz->brksize[1]) {
				time = here/srate;
				if((exit_status = read_value_from_brktable(time,1,dz))<0)
					return(exit_status);
			}
			opos = (int)round(here * dz->param[1]);	/* absolute output position */
			opos -= lastwrite;							/* output position relative to buf start */
			if((overflow = opos - (dz->buflen * 2))>0) {
				if((exit_status = eliminate_too_short_events(&zcnt,&final_pos,&sig_cnt,dz->buflen,0,obuf,1,dz)) < 0)
					return(exit_status);
				if((exit_status = smooth_bad_events(&zcntb,&final_posb,&sig_cntb,stsmoothed,dz->buflen,0,obuf,1,dz)) < 0)
					return(exit_status);
				for(m = 0;m < dz->buflen;m++)
					obuf[m] = (float)(obuf[m] * normaliser);
				if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
					return(exit_status);
				lastwrite += dz->buflen; 
				memcpy((char*)obuf,(char*)obuf2,dz->buflen * sizeof(float));
				memset((char*)obuf2,0,dz->buflen * sizeof(float));
				memcpy((char*)obuf2,(char*)ovflw,overflow * sizeof(float));
				memset((char*)ovflw,0,dz->buflen * sizeof(float));
				opos -= dz->buflen;
			}
			if((opos + (there_in_buf - here_in_buf)) >= dz->buflen * 3) {
				sprintf(errstr,"OVERFLOWED OUTPUT BUFFER.\n");
				return(MEMORY_ERROR);
			}
			while(there_in_buf > dz->buflen) {
				there_in_buf = dz->buflen;
				for(k = here_in_buf; k < there_in_buf;k++)
					obuf[opos++] += ibuf[k];
				if((exit_status = read_samps(ibuf,dz))<0)
					return(exit_status);
				startsamp += dz->buflen;
				here_in_buf = 0;
				there_in_buf = cuttime[n] - startsamp;
			}
			for(k = here_in_buf; k < there_in_buf;k++)
				obuf[opos++] += ibuf[k];
			here = cuttime[n];
		}
		if(opos > 0) {
			if((exit_status = eliminate_too_short_events(&zcnt,&final_pos,&sig_cnt,opos,1,obuf,1,dz)) < 0)
				return(exit_status);
			if((exit_status = smooth_bad_events(&zcntb,&final_posb,&sig_cntb,stsmoothed,opos,1,obuf,1,dz)) < 0)
				return(exit_status);
			for(m = 0;m < opos;m++)
				obuf[m] = (float)(obuf[m] * normaliser);
				if((exit_status = write_samps(obuf,opos,dz))<0)
					return(exit_status);
		}
		return(FINISHED);
	case(PSOW_DUPL):
		maxwritesize = 0;
		for(n = j-1; n < cutcnt;n+=j) {
			here_in_buf  = here - startsamp;
			there_in_buf = cuttime[n] - startsamp;
			writesize    = there_in_buf - here_in_buf;
			if(writesize > maxwritesize)
				maxwritesize = writesize;
			here = cuttime[n];
		}
		here = 0;
		if(maxwritesize > dz->buflen) {
			free(dz->sampbuf[3]);
			if((dz->sampbuf[3] = (float *)((maxwritesize + 2) * sizeof(float)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY TO STORE GRAIN-BLOCKS\n");
				return(MEMORY_ERROR);
			}
		}
		xbuf = dz->sampbuf[3];
		for(n = j-1; n < cutcnt;n+=j) {
			if(dz->brksize[1]) {
				time = here/srate;
				if((exit_status = read_value_from_brktable(time,1,dz))<0)
					return(exit_status);
			}
			here_in_buf  = here - startsamp;
			there_in_buf = cuttime[n] - startsamp;
			writesize    = there_in_buf - here_in_buf;
			xbufpos      = 0;
			while(there_in_buf > dz->buflen) {
				there_in_buf = dz->buflen;
				for(k = here_in_buf; k < there_in_buf;k++)
					xbuf[xbufpos++] = ibuf[k];
				if((exit_status = read_samps(ibuf,dz))<0)
					return(exit_status);
				startsamp += dz->buflen;
				here_in_buf  = 0;
				there_in_buf = cuttime[n] - startsamp;
			}
			for(k = here_in_buf; k < there_in_buf;k++)
				xbuf[xbufpos++] = ibuf[k];
			for(m=0;m<dz->iparam[1];m++) {
				for(k = 0;k < xbufpos;k++) {
					obuf[opos++] = xbuf[k];
					if(opos >= dz->buflen * 2) {
						if((exit_status = eliminate_too_short_events(&zcnt,&final_pos,&sig_cnt,dz->buflen,0,obuf,1,dz)) < 0)
							return(exit_status);
						if((exit_status = smooth_bad_events(&zcntb,&final_posb,&sig_cntb,stsmoothed,dz->buflen,0,obuf,1,dz)) < 0)
							return(exit_status);
						if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
							return(exit_status);
						memcpy((char *)obuf,(char *)obuf2,dz->buflen * sizeof(float));
						memset((char *)obuf2,0,dz->buflen * sizeof(float));
						opos = 0;
					}
				}
			}
			here = cuttime[n];
		}
		break;
	case(PSOW_DEL):
		for(n = j; n < cutcnt;n+=j) {
			here_in_buf  = here - startsamp;
			there_in_buf = cuttime[n] - startsamp;
			while(there_in_buf > dz->buflen) {
				there_in_buf = dz->buflen;
				for(k = here_in_buf; k < there_in_buf;k++) {
					obuf[opos++] = ibuf[k];
					if(opos >= dz->buflen * 2) {
						if((exit_status = eliminate_too_short_events(&zcnt,&final_pos,&sig_cnt,dz->buflen,0,obuf,1,dz)) < 0)
							return(exit_status);
						if((exit_status = smooth_bad_events(&zcntb,&final_posb,&sig_cntb,stsmoothed,dz->buflen,0,obuf,1,dz)) < 0)
							return(exit_status);
						if((exit_status = write_samps(obuf,dz->buflen ,dz))<0)
							return(exit_status);
						memcpy((char *)obuf,(char *)obuf2,dz->buflen * sizeof(float));
						opos = 0;
					}
				}
				if((exit_status = read_samps(ibuf,dz))<0)
					return(exit_status);
				startsamp += dz->buflen;
				here_in_buf  = 0;
				there_in_buf = cuttime[n] - startsamp;
			}
			for(k = here_in_buf; k < there_in_buf;k++) {
				obuf[opos++] = ibuf[k];
				if(opos >= dz->buflen * 2) {
					if((exit_status = eliminate_too_short_events(&zcnt,&final_pos,&sig_cnt,dz->buflen,0,obuf,1,dz)) < 0)
						return(exit_status);
					if((exit_status = smooth_bad_events(&zcntb,&final_posb,&sig_cntb,stsmoothed,dz->buflen,0,obuf,1,dz)) < 0)
						return(exit_status);
					if((exit_status = write_samps(obuf,dz->buflen ,dz))<0)
						return(exit_status);
					memcpy((char *)obuf,(char *)obuf2,dz->buflen * sizeof(float));
					opos = 0;
				}
			}
			if(dz->brksize[1]) {
				time = here/srate;
				if((exit_status = read_value_from_brktable(time,1,dz))<0)
					return(exit_status);
			}
			n += (dz->iparam[1] - 1) * j;	/* SKIP GRAINS */
			here = cuttime[n];
		}
		break;
	case(PSOW_STRFILL):
		if(!dz->brksize[3])
			dz->param[3] /= SEMITONES_PER_OCTAVE; 
		ovflw = dz->sampbuf[3];
		xbuf  = dz->sampbuf[4];
		memset((char*)ovflw,0,dz->buflen * sizeof(float));
		memset((char*)xbuf,0,dz->buflen * sizeof(float));
		for(n = j-1; n < cutcnt;n+=j) {
 			here_in_buf = here - startsamp;
			there_in_buf = cuttime[n] - startsamp;
			if((seglen = there_in_buf - here_in_buf) > dz->buflen) {
				sprintf(errstr,"Too long grain encountered at %f secs\n",here/srate);
				return(GOAL_FAILED);
			}
			if(dz->brksize[1]) {
				time = here/srate;
				if((exit_status = read_value_from_brktable(time,1,dz))<0)
					return(exit_status);
			}
			opos = (int)round(here * dz->param[1]);	/* absolute output position */
			opos -= lastwrite;							/* output position relative to buf start */
			if((overflow = opos - (dz->buflen * 2))>0) {
				if((exit_status = eliminate_too_short_events(&zcnt,&final_pos,&sig_cnt,dz->buflen,0,obuf,1,dz)) < 0)
					return(exit_status);
				if((exit_status = smooth_bad_events(&zcntb,&final_posb,&sig_cntb,stsmoothed,dz->buflen,0,obuf,1,dz)) < 0)
					return(exit_status);
				if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
					return(exit_status);
				lastwrite += dz->buflen; 
				memcpy((char*)obuf,(char*)obuf2,dz->buflen * sizeof(float));
				memset((char*)obuf2,0,dz->buflen * sizeof(float));
				memcpy((char*)obuf2,(char*)ovflw,overflow * sizeof(float));
				memset((char*)ovflw,0,dz->buflen * sizeof(float));
				opos -= dz->buflen;
			}
			if((opos + seglen) >= dz->buflen * 3) {
				sprintf(errstr,"OVERFLOWED OUTPUT BUFFER.\n");
				return(MEMORY_ERROR);
			}
			/* Fill the temporary buffer with the input segment */
			xbufpos = 0;
			while(there_in_buf > dz->buflen) {
				there_in_buf = dz->buflen;
				for(k = here_in_buf; k < there_in_buf;k++)
					xbuf[xbufpos++] = ibuf[k];
				if((exit_status = read_samps(ibuf,dz))<0)
					return(exit_status);
				startsamp += dz->buflen;
				here_in_buf = 0;
				there_in_buf = cuttime[n] - startsamp;
			}
			for(k = here_in_buf; k < there_in_buf;k++)
				xbuf[xbufpos++] = ibuf[k];
			if((exit_status = smooth_bad_grains(xbufpos,4,dz))<0)
				return(exit_status);
			/* FIND THE TIME WHERE NEXT SEGMENT WILL START */
			time = cuttime[n]/srate;
			if(dz->brksize[1]) {
				if((exit_status = read_value_from_brktable(time,1,dz))<0)
					return(exit_status);
			}
			if(dz->brksize[3]) {
				if((exit_status = read_value_from_brktable(time,3,dz))<0)
					return(exit_status);
				dz->param[3] /= SEMITONES_PER_OCTAVE; 
			}
			next_opos = (int)round(cuttime[n] * dz->param[1]);	/* absolute output next-position */
			next_opos -= lastwrite;
			advance = next_opos - opos;				/* gap between start of this seg and start of next */
			transinc = pow(2.0,(double)dz->param[3]);
			dstep = (double)seglen/transinc;
			sum = 0.0;
			repets = 0;
			while(sum < (double)advance) {
				sum += dstep;
				repets++;
			}
			/* FIND HOW MANY SEGMENTS NEEDED TO FILL THE GAP, WITH (at least) SLIGHT OVERLAP OF SEGS */

			thisplace = (double)opos;
			for(k = 0; k < repets;k++) {
				opos = round(thisplace);
						/* ADD SOME RAND but don't go below zero !!! */
				if((overflow = opos - (dz->buflen * 2))>0) {
					if((exit_status = eliminate_too_short_events(&zcnt,&final_pos,&sig_cnt,dz->buflen,0,obuf,1,dz)) < 0)
						return(exit_status);
					if((exit_status = smooth_bad_events(&zcntb,&final_posb,&sig_cntb,stsmoothed,dz->buflen,0,obuf,1,dz)) < 0)
						return(exit_status);
					if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
						return(exit_status);
					lastwrite += dz->buflen; 
					memcpy((char*)obuf,(char*)obuf2,dz->buflen * sizeof(float));
					memset((char*)obuf2,0,dz->buflen * sizeof(float));
					memcpy((char*)obuf2,(char*)ovflw,overflow * sizeof(float));
					memset((char*)ovflw,0,dz->buflen * sizeof(float));
					opos -= dz->buflen;
					thisplace -= (double)dz->buflen;
				}
				for(m=0;m<seglen;m++)
					obuf[opos++] += xbuf[m];
				thisplace += dstep;
			}
			here = cuttime[n];
		}
		break;
	case(PSOW_FEATURES):
		lastwritend = 0;
		maxlevel = 0.0;
		cnt = 0;
		for(n = j-1; n < cutcnt;n+=j) {
			gain = dz->param[PSF_GAIN];
			transpos = 0.0;
			if(n<j) {
				seglen = cuttime[n];
				here = 0;
			} else
				seglen = cuttime[n] - cuttime[n-j]; 
			startopos = opos + dz->total_samps_written;
			here_in_buf = here - startsamp;
			there_in_buf = cuttime[n] - startsamp;
			time = startopos/srate;
			if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
				return(exit_status);
			if(dz->iparam[PSF_SUB] > 1) {
				if(cnt % dz->iparam[PSF_SUB] != 0)
					gain *= 1.0 - dz->param[PSF_SUBGAIN];
			}
			cnt++;
			if(!flteq(dz->param[PSF_TRNS],0.0))
				transpos = pow(2.0,(dz->param[PSF_TRNS]/SEMITONES_PER_OCTAVE));
			if((overflow = opos - dz->buflen)>0) {
				if((exit_status = eliminate_too_short_events(&zcnt,&final_pos,&sig_cnt,dz->buflen,0,obuf,1,dz)) < 0)
					return(exit_status);
				if((exit_status = smooth_bad_events(&zcntb,&final_posb,&sig_cntb,stsmoothed,dz->buflen,0,obuf,1,dz)) < 0)
					return(exit_status);
				if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
					return(exit_status);
				lastwritend -= dz->buflen;
				lastwrite += dz->buflen; 
				memset((char*)obuf,0,dz->buflen * sizeof(float));
				if(lastwritend > 0)
					memcpy((char*)obuf,(char*)obuf2,lastwritend * sizeof(float));
				memset((char*)obuf2,0,dz->buflen * sizeof(float));
				opos -= dz->buflen;
			}
			if((opos + (there_in_buf - here_in_buf)) >= dz->buflen * 2) {
				sprintf(errstr,"OVERFLOWED OUTPUT BUFFER.\n");
				return(MEMORY_ERROR);
			}
			if(here_in_buf >= dz->buflen * 2) {
				sprintf(errstr,"OVERFLOWED INPUT BUFFER.\n");
				return(MEMORY_ERROR);
			}
			while(here_in_buf > dz->buflen) {
				memset((char *)ibuf,0,dz->buflen * sizeof(float));
				if(dz->ssampsread > 0) {
					memcpy((char *)ibuf,(char *)ibuf2,dz->buflen * sizeof(float));
					memset((char *)ibuf2,0,dz->buflen * sizeof(float));
					if((exit_status = read_samps(ibuf2,dz))<0)
						return(exit_status);
				}
				startsamp += dz->buflen;
				here_in_buf -= dz->buflen;
				there_in_buf = cuttime[n] - startsamp;
			}
			if(zerofof(here_in_buf,there_in_buf,ibuf)) {
				for(k = here_in_buf; k < there_in_buf;k++)
					opos++;
			} else {
				if(dz->iparam[PSF_FOFSTR] > 1) {
// INTERPOLATE MAX FOFS ... expt: doesn't really work -- too many glitches so far
//					if((exit_status = interp_maxfofs(k,here_in_buf,there_in_buf,ibuf,obuf,cuttime,
//					startsamp,gain,&opos,&n,maxlevel,cutcnt,&seglen,dz))<0)
//						return(exit_status);

					if((exit_status = fof_stretch(n,time,here_in_buf,there_in_buf,ibuf,obuf,&opos,&maxlevel,gain,dz))<0)
						return(exit_status);
			
				} else if(!flteq(dz->param[PSF_SPEC],0.0)) {
					dhere = here_in_buf;
					spectranspos = pow(2.0,(dz->param[PSF_SPEC]/SEMITONES_PER_OCTAVE));
					while(dhere < there_in_buf) {
						k = (int)floor(dhere);
						diff = dhere - (double)k;
						valdiff = ibuf[k+1] - ibuf[k];
						valdiff *= diff;
						sum = ibuf[k] + valdiff;
						sum *= gain;
						obuf[opos] = (float)(obuf[opos] + sum);
						maxlevel = max(obuf[opos],maxlevel);
						opos++;
						dhere += spectranspos;
					}
					if(opos > lastwritend)
						lastwritend = opos;
				} else {
					for(k = here_in_buf; k < there_in_buf;k++) {
						sum = ibuf[k] * gain;
						obuf[opos] = (float)(obuf[opos] + sum);
						maxlevel = max(obuf[opos],maxlevel);
						opos++;
					}
				}
			}
			if(opos > lastwritend)
				lastwritend = opos;
			endopos = startopos + seglen;
			opos = endopos - dz->total_samps_written;
			if(!flteq(transpos,0.0)) {
				outseglen = (int)round((double)seglen/transpos);
				transtep = outseglen - seglen;
				seglen = outseglen;
				opos += transtep;
				if(dz->mode != 0) {									
					if(opos + dz->total_samps_written < cuttime[n])	/* i.e. dupl the last FOF until we catch up with */
						n--;										/* FOFs in orig */
				}
			}
			here = cuttime[n];
			if(dz->param[PS_VIBDEPTH] > 0.0)
				opos += vibrato(seglen,dz->param[PS_VIBFRQ],dz->param[PS_VIBDEPTH],dz);

		
			if(dz->param[PSF_RAND] > 0.0) {
				rand = drand48() - 0.5;			/* rand number between -0.5 & 0.5 */
				rand *= dz->param[PSF_RAND];	/* reduced according to degree of randomness */
				rand *= (double)seglen;
				opos += (int)round(rand);
			}
		}
		if(lastwritend > 0) {
			if((exit_status = eliminate_too_short_events(&zcnt,&final_pos,&sig_cnt,lastwritend,1,obuf,1,dz)) < 0)
				return(exit_status);
			if((exit_status = smooth_bad_events(&zcntb,&final_posb,&sig_cntb,stsmoothed,lastwritend,1,obuf,1,dz)) < 0)
				return(exit_status);
			if((exit_status = write_samps(obuf,lastwritend,dz))<0)
				return(exit_status);
		}
		if(maxlevel > 1.0) {
			fprintf(stdout,"WARNING: Signal distorted: maxlevel = %lf\n",maxlevel);
			fflush(stdout);
		}
		return FINISHED;
	case(PSOW_SYNTH):
		lastwritend = 0;
		maxseglen = cuttime[0];
		for(n=1;n<cutcnt;n++)
			maxseglen = max(maxseglen,cuttime[n] - cuttime[n-1]);
		maxseglen = max(maxseglen,dz->insams[0] - cuttime[cutcnt-1]);
		if((fofenv = (double *)malloc(maxseglen * sizeof(double)))==NULL) {
			sprintf(errstr,"Insufficient memory for FOF envelope vals store\n");
			return(MEMORY_ERROR);
		}
		if((fofloc = (int *)malloc(maxseglen * sizeof(int)))==NULL) {
			sprintf(errstr,"Insufficient memory for FOF envelope times store\n");
			return(MEMORY_ERROR);
		}
		if((sintab = (double *)malloc((unsigned int)round(SINTABLEN+1) * sizeof(double)))==NULL) {
			sprintf(errstr,"Insufficient memory for SINE table\n");
			return(MEMORY_ERROR);
		}
		dstep = TWOPI/SINTABLEN;
		for(n=0;n<SINTABLEN;n++)
			sintab[n] = sin((double)n * dstep);
		sintab[n] = 0.0;	/* wraparound */
		for(n = 1; n <= cutcnt;n++) {
			if(sloom) {
				display_time = round(((double)n/(double)cutcnt) * PBAR_LENGTH);
				fprintf(stdout,"TIME: %d\n",display_time);
				fflush(stdout);
			}
			if(n<1) {
				seglen = cuttime[n];
				here = 0;
			} else if(n==cutcnt)
				seglen = dz->insams[0] - cuttime[n]; 
			else
				seglen = cuttime[n] - cuttime[n-1]; 
			startopos = opos + dz->total_samps_written;
			here_in_buf = here - startsamp;
			there_in_buf = cuttime[n] - startsamp;
			time = startopos/srate;
			if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
				return(exit_status);
			if((overflow = opos - dz->buflen)>0) {
				if((exit_status = eliminate_too_short_events(&zcnt,&final_pos,&sig_cnt,dz->buflen,0,obuf,1,dz)) < 0)
					return(exit_status);
				if((exit_status = smooth_bad_events(&zcntb,&final_posb,&sig_cntb,stsmoothed,dz->buflen,0,obuf,1,dz)) < 0)
					return(exit_status);
				if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
					return(exit_status);
				lastwritend -= dz->buflen;
				lastwrite += dz->buflen; 
				memset((char*)obuf,0,dz->buflen * sizeof(float));
				if(lastwritend > 0)
					memcpy((char*)obuf,(char*)obuf2,lastwritend * sizeof(float));
				memset((char*)obuf2,0,dz->buflen * sizeof(float));
				opos -= dz->buflen;
			}
			if((opos + (there_in_buf - here_in_buf)) >= dz->buflen * 2) {
				sprintf(errstr,"OVERFLOWED OUTPUT BUFFER.\n");
				return(MEMORY_ERROR);
			}
			if(here_in_buf >= dz->buflen * 2) {
				sprintf(errstr,"OVERFLOWED INPUT BUFFER.\n");
				return(MEMORY_ERROR);
			}
			while(here_in_buf > dz->buflen) {
				memset((char *)ibuf,0,dz->buflen * sizeof(float));
				if(dz->ssampsread > 0) {
					memcpy((char *)ibuf,(char *)ibuf2,dz->buflen * sizeof(float));
					memset((char *)ibuf2,0,dz->buflen * sizeof(float));
					if((exit_status = read_samps(ibuf2,dz))<0)
						return(exit_status);
				}
				startsamp += dz->buflen;
				here_in_buf -= dz->buflen;
				there_in_buf = cuttime[n] - startsamp;
			}
			if(zerofof(here_in_buf,there_in_buf,ibuf)) {
				for(k = here_in_buf; k < there_in_buf;k++)
					opos++;
			} else {
				getfofenv(ibuf,fofenv,fofloc,maxseglen,here_in_buf,there_in_buf,&envcnt);
				if((exit_status = superimpose_fofs_on_synthtones(fofenv,fofloc,sintab,
					here_in_buf,there_in_buf,ibuf,obuf,&opos,cuttime[n-1],envcnt,dz))<0)
				return(exit_status);
			}
			if(opos > lastwritend)
				lastwritend = opos;
			endopos = startopos + seglen;
			opos = endopos - dz->total_samps_written;
			here = cuttime[n];
		}
		if(lastwritend > 0) {
			if((exit_status = eliminate_too_short_events(&zcnt,&final_pos,&sig_cnt,lastwritend,1,obuf,1,dz)) < 0)
				return(exit_status);
			if((exit_status = smooth_bad_events(&zcntb,&final_posb,&sig_cntb,stsmoothed,lastwritend,1,obuf,1,dz)) < 0)
				return(exit_status);
			if((exit_status = write_samps(obuf,lastwritend,dz))<0)
				return(exit_status);
		}
		return FINISHED;
	case(PSOW_IMPOSE):
		limit = dz->insams[1];
		if((exit_status = create_normalising_envelope(dz))<0)
			return(exit_status);
		envtoptime = 2;
		envincr  = dz->env[3] - dz->env[1];
		envincr /= dz->env[2] - dz->env[0];
		envincr /= srate;
		envval = dz->env[1];
        if((sndseekEx(dz->ifd[1],0,0)<0)){
            sprintf(errstr,"sndseek() failed\n");
            return SYSTEM_ERROR;
        }
		memset((char*)ibuf3,0,dz->buflen * sizeof(float));
		if((ssampsread2 = fgetfbufEx(ibuf3, dz->buflen,dz->ifd[1],0)) < 0) {
			sprintf(errstr,"Can't read samples from 2nd input soundfile.\n");
			return(SYSTEM_ERROR);
		}
		memset((char*)ibuf4,0,dz->buflen * sizeof(float));
		if((ssampsread2 = fgetfbufEx(ibuf4, dz->buflen,dz->ifd[1],0)) < 0) {
			sprintf(errstr,"Can't read samples from 2nd input soundfile.\n");
			return(SYSTEM_ERROR);
		}
		lastwritend = 0;
		maxseglen = cuttime[0];
		for(n=1;n<cutcnt;n++)
			maxseglen = max(maxseglen,cuttime[n] - cuttime[n-1]);
		maxseglen = max(maxseglen,dz->insams[0] - cuttime[cutcnt-1]);
		if((fofenv = (double *)malloc(maxseglen * sizeof(double)))==NULL) {
			sprintf(errstr,"Insufficient memory for FOF envelope vals store\n");
			return(MEMORY_ERROR);
		}
		if((fofloc = (int *)malloc(maxseglen * sizeof(int)))==NULL) {
			sprintf(errstr,"Insufficient memory for FOF envelope times store\n");
			return(MEMORY_ERROR);
		}
		for(n = 1; n <= cutcnt;n++) {
			if(cuttime[n] > limit)
				break;
			if(sloom) {
				display_time = round(((double)n/(double)cutcnt) * PBAR_LENGTH);
				fprintf(stdout,"TIME: %d\n",display_time);
				fflush(stdout);
			}
			if(n<1) {
				seglen = cuttime[n];
				here = 0;
			} else if(n==cutcnt)
				seglen = dz->insams[0] - cuttime[n]; 
			else
				seglen = cuttime[n] - cuttime[n-1]; 
			startopos = opos + dz->total_samps_written;
			here_in_buf = here - startsamp;
			there_in_buf = cuttime[n] - startsamp;
			time = startopos/srate;
			if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
				return(exit_status);
			if((overflow = opos - dz->buflen)>0) {
				if((exit_status = eliminate_too_short_events(&zcnt,&final_pos,&sig_cnt,dz->buflen,0,obuf,1,dz)) < 0)
					return(exit_status);
				if((exit_status = smooth_bad_events(&zcntb,&final_posb,&sig_cntb,stsmoothed,dz->buflen,0,obuf,1,dz)) < 0)
					return(exit_status);
				if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
					return(exit_status);
				lastwritend -= dz->buflen;
				lastwrite += dz->buflen; 
				memset((char*)obuf,0,dz->buflen * sizeof(float));
				if(lastwritend > 0)
					memcpy((char*)obuf,(char*)obuf2,lastwritend * sizeof(float));
				memset((char*)obuf2,0,dz->buflen * sizeof(float));
				opos -= dz->buflen;
			}
			if((opos + (there_in_buf - here_in_buf)) >= dz->buflen * 2) {
				sprintf(errstr,"OVERFLOWED OUTPUT BUFFER.\n");
				return(MEMORY_ERROR);
			}
			if(here_in_buf >= dz->buflen * 2) {
				sprintf(errstr,"OVERFLOWED INPUT BUFFER.\n");
				return(MEMORY_ERROR);
			}
			while(here_in_buf > dz->buflen) {
				memset((char *)ibuf,0,dz->buflen * sizeof(float));
				memset((char *)ibuf3,0,dz->buflen * sizeof(float));
				if(dz->ssampsread > 0) {
					memcpy((char *)ibuf,(char *)ibuf2,dz->buflen * sizeof(float));
					memset((char *)ibuf2,0,dz->buflen * sizeof(float));
					if((exit_status = read_samps(ibuf2,dz))<0)
						return(exit_status);
				}
				if(ssampsread2 > 0) {
					memcpy((char *)ibuf3,(char *)ibuf4,dz->buflen * sizeof(float));
					memset((char *)ibuf4,0,dz->buflen * sizeof(float));
					if((ssampsread2 = fgetfbufEx(ibuf4,dz->buflen,dz->ifd[1],0)) < 0) {
						sprintf(errstr,"Can't read samples from 2nd input soundfile.\n");
						return(SYSTEM_ERROR);
					}
				}
				startsamp += dz->buflen;
				here_in_buf -= dz->buflen;
				there_in_buf = cuttime[n] - startsamp;
			}
			if(zerofof(here_in_buf,there_in_buf,ibuf)) {
				for(k = here_in_buf; k < there_in_buf;k++)
					opos++;
			} else { 
				getfofenv(ibuf,fofenv,fofloc,maxseglen,here_in_buf,there_in_buf,&envcnt);
				if((exit_status = superimpose_fofs_on_input(fofenv,fofloc,
					here_in_buf,there_in_buf,ibuf,ibuf3,obuf,&opos,startsamp,&envtoptime,&envval,&envincr,envcnt,dz))<0)
					return(exit_status);
			}
			if(opos > lastwritend)
				lastwritend = opos;
			endopos = startopos + seglen;
			opos = endopos - dz->total_samps_written;
			here = cuttime[n];
		}
		if(lastwritend > 0) {
			if((exit_status = eliminate_too_short_events(&zcnt,&final_pos,&sig_cnt,lastwritend,1,obuf,1,dz)) < 0)
				return(exit_status);
			if((exit_status = smooth_bad_events(&zcntb,&final_posb,&sig_cntb,stsmoothed,lastwritend,1,obuf,1,dz)) < 0)
				return(exit_status);
			if((exit_status = write_samps(obuf,lastwritend,dz))<0)
				return(exit_status);
		}
		if(dz->total_samps_written <= 0) {
			sprintf(errstr,"No output written: 2nd sound is too short ??\n");
			return(GOAL_FAILED);
		}
		return FINISHED;
	case(PSOW_SPLIT):
		lastwritend = 0;
		ipos = 0;
		opos = 0;
		startsamp = 0;
		for(n = dz->iparam[PS_SUBNO]-1; n <= cutcnt;n+=dz->iparam[PS_SUBNO]) {
			if((overflow = opos - dz->buflen)>0) {			/* write outbuf */
				if((exit_status = eliminate_too_short_events(&zcnt,&final_pos,&sig_cnt,dz->buflen,0,obuf,1,dz)) < 0)
					return(exit_status);
				if((exit_status = smooth_bad_events(&zcntb,&final_posb,&sig_cntb,stsmoothed,dz->buflen,0,obuf,1,dz)) < 0)
					return(exit_status);
				if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
					return(exit_status);
				lastwritend -= dz->buflen;
				lastwrite += dz->buflen; 
				memset((char*)obuf,0,dz->buflen * sizeof(float));
				if(lastwritend > 0)
					memcpy((char*)obuf,(char*)obuf2,lastwritend * sizeof(float));
				memset((char*)obuf2,0,dz->buflen * sizeof(float));
				opos -= dz->buflen;
			}
			m = n - dz->iparam[PS_SUBNO]; 
			if(m < 0)
				total_seglen = cuttime[n];
			else
				total_seglen = cuttime[n] - cuttime[m];
			if((opos + total_seglen) >= dz->buflen * 2) {
				sprintf(errstr,"OVERFLOWED OUTPUT BUFFER.\n");
				return(MEMORY_ERROR);
			}
			if((m > 0) && (cuttime[m] - startsamp > dz->buflen)) {		/* refill inbuf */
				memset((char *)ibuf,0,dz->buflen * sizeof(float));
				if(dz->ssampsread > 0) {
					memcpy((char *)ibuf,(char *)ibuf2,dz->buflen * sizeof(float));
					memset((char *)ibuf2,0,dz->buflen * sizeof(float));
					if((exit_status = read_samps(ibuf2,dz))<0)
						return(exit_status);
				}
				startsamp += dz->buflen;
				ipos -= dz->buflen;
				there_in_buf = cuttime[n] - startsamp;
			}
			if(ipos + total_seglen >= dz->buflen * 2) {
				sprintf(errstr,"OVERFLOWED INPUT BUFFER.\n");
				return(MEMORY_ERROR);
			}
			segcnt = 0;
			average_seglen = 0.0;
			for(m = n - dz->iparam[PS_SUBNO];m < n;m++) {
				if(m < 0)
					startz = 0;
				else
					startz = cuttime[m] - startsamp;
				endz = cuttime[m+1] - startsamp;
				if(!zerofof(startz,endz,ibuf)) {
					average_seglen += (double)(endz - startz);
					segcnt++;
				}
			}
			if(segcnt > 0) {
				average_seglen = (double)average_seglen/(double)segcnt;
				transtep = (int)round(average_seglen/dz->param[PS_UTRNS]);
				cnt = 0;
				orig_startpos = opos;
				startpos = opos;
				for(m = n - dz->iparam[PS_SUBNO];m < n;m++,cnt++) {
					opos = startpos;
					if(m < 0) {
						here_in_buf = 0;
						there_in_buf = cuttime[0];
					} else {
						here_in_buf  = cuttime[m]   - startsamp;
						there_in_buf = cuttime[m+1] - startsamp;
					}
					if(!zerofof(here_in_buf,there_in_buf,ibuf)) {
						if(dz->param[PS_ATTEN] <= 1.0) {
							if(cnt > 0)
								gain = dz->param[PS_ATTEN];		/* attenuate uptransposed FOFs */
							else
								gain = 1.0;
						} else {
							if(cnt == 0)
								gain /= dz->param[PS_ATTEN];		/* attenuate subharmonics */
							else
								gain = 1.0;
						}
						for(k = here_in_buf;k<there_in_buf;k++) {
							thisval = ibuf[k] * gain;
							obuf[opos] = (float)(obuf[opos] + thisval);
							opos++;
						}									
						lastwritend = max(opos,lastwritend);
					}
					startpos += transtep;						/* uptranspose FOFs */
				}
			}
			opos = orig_startpos + total_seglen;		/* retain pitch of next subharm */
			ipos += total_seglen;
		}
		if(lastwritend > 0) {
			if((exit_status = eliminate_too_short_events(&zcnt,&final_pos,&sig_cnt,lastwritend,1,obuf,1,dz)) < 0)
				return(exit_status);
			if((exit_status = smooth_bad_events(&zcntb,&final_posb,&sig_cntb,stsmoothed,lastwritend,1,obuf,1,dz)) < 0)
				return(exit_status);
			if((exit_status = write_samps(obuf,lastwritend,dz))<0)
				return(exit_status);
		}
		if(dz->total_samps_written <= 0) {
			sprintf(errstr,"No output written: 2nd sound is too short ??\n");
			return(GOAL_FAILED);
		}
		return FINISHED;
	case(PSOW_SPACE):
		lastwritend = 0;
		opos = 0;
		startsamp = 0;
		spaceswitch = 0;
		if(cuttime[cutcnt-1] != dz->insams[0])					/* force segment at end */
			cuttime[cutcnt++] = dz->insams[0];					/* force segment at start */
		for(n=cutcnt-1;n>=0;n--)								
			cuttime[n+1]  = cuttime[n];
		cuttime[0] = 0;
		cutcnt++;
		memset((char*)obuf,0,dz->buflen * 2 * sizeof(float));
		memset((char*)obuf2,0,dz->buflen * 2 * sizeof(float));
		for(n = 1; n < cutcnt;n++) {
			if((here_in_buf = cuttime[n-1] - startsamp) >= dz->buflen) {
				memset((char *)ibuf,0,dz->buflen * sizeof(float));
				if(dz->ssampsread > 0) {
					memcpy((char *)ibuf,(char *)ibuf2,dz->buflen * sizeof(float));
					memset((char *)ibuf2,0,dz->buflen * sizeof(float));
					if((exit_status = read_samps(ibuf2,dz))<0)
						return(exit_status);
				}
				startsamp += dz->buflen;
				here_in_buf -= dz->buflen;
			}
			there_in_buf = cuttime[n] - startsamp;
			if((overflow = opos - (dz->buflen*2))>0) {			/* write outbuf */
				if((exit_status = eliminate_too_short_events(&zcnt,&final_pos,&sig_cnt,dz->buflen*2,0,obuf,2,dz)) < 0)
					return(exit_status);
				if((exit_status = smooth_bad_events(&zcntb,&final_posb,&sig_cntb,stsmoothed,dz->buflen*2,0,obuf,2,dz)) < 0)
					return(exit_status);
				if((exit_status = write_samps(obuf,dz->buflen*2,dz))<0)
					return(exit_status);
				lastwritend -= dz->buflen*2;
				lastwrite += dz->buflen*2; 
				memset((char*)obuf,0,dz->buflen * 2 * sizeof(float));
				if(lastwritend > 0)
					memcpy((char*)obuf,(char*)obuf2,lastwritend * sizeof(float));
				memset((char*)obuf2,0,dz->buflen * 2 * sizeof(float));
				opos -= dz->buflen * 2;
			}
			time = (double)cuttime[n-1]/srate;
			if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
				return(exit_status);
			switch(spaceswitch) {
			case(0):
				if(dz->param[PS_SEPAR] >= 0.0)		/* even events move rightwards */
					spacepos = dz->param[PS_SEPAR];
				else								/* even events move leftwards */
					spacepos = -(dz->param[PS_SEPAR]);
				break;
			case(1):
				if(dz->param[PS_SEPAR] >= 0.0)		/* odd events move leftwards */
					spacepos = -(dz->param[PS_SEPAR]);
				else								/* odd events move rightwards */
					spacepos = dz->param[PS_SEPAR];
				break;
			case(2):
				spacepos = 0.0;
				break;
			case(3):
				if(dz->param[PS_SEPAR] >= 0.0)		/* even events move rightwards */
					spacepos = dz->param[PS_SEPAR]/2.0;
				else								/* even events move leftwards */
					spacepos = -(dz->param[PS_SEPAR]/2.0);
				break;
			case(4):
				if(dz->param[PS_SEPAR] >= 0.0)		/* odd events move leftwards */
					spacepos = -(dz->param[PS_SEPAR]/2.0);
				else								/* odd events move rightwards */
					spacepos = dz->param[PS_SEPAR]/2.0;
				break;
			}
			spacecalc(spacepos,&valleft,&valright);
			suppression = (dz->param[PS_RELV2] * (double)spaceswitch/(double)(dz->iparam[PS_SUBNO] - 1));
			suppression = pow(suppression,PSOW_POW);
			valleft  *= 1.0 - suppression;
			valright *= 1.0 - suppression;
			spaceswitch++;
			spaceswitch %= dz->iparam[PS_SUBNO];
			
			if(dz->param[PS_RELEV] < 1.0)			/* de-emphasize rightward sig */
				valright *= dz->param[PS_RELEV];
			else if(dz->param[PS_RELEV] > 0.0)		/* de-emphasize leftward sig */
				valleft /= dz->param[PS_RELEV];
			for(k=here_in_buf;k<there_in_buf;k++) {
				obuf[opos++] = (float)(ibuf[k] * valleft);
				obuf[opos++] = (float)(ibuf[k] * valright);
			}
		}
		lastwritend = opos;
		if(lastwritend > 0) {
			if((exit_status = eliminate_too_short_events(&zcnt,&final_pos,&sig_cnt,lastwritend,1,obuf,2,dz)) < 0)
				return(exit_status);
			if((exit_status = smooth_bad_events(&zcntb,&final_posb,&sig_cntb,stsmoothed,lastwritend,1,obuf,2,dz)) < 0)
				return(exit_status);
			if((exit_status = write_samps(obuf,lastwritend,dz))<0)
				return(exit_status);
		}
		return FINISHED;
	case(PSOW_INTERLEAVE):
		insert_edge_cuts(cuttime,&cutcnt,&cutstart,dz);
		limit = min(cutstart,cutcnt - cutstart);	/* work to end of smallest infile, counted in FOFs */
		lastwritend = 0;
		opos = 0;
		startsamp = 0;
		startsamp2 = 0;
		cnt0 = 0;
		cnt1 = 0;
		for(n = j-1, m = cutstart+j-1; n < limit;n+=j,m+=j) {
			startopos = opos;
			gain = 1.0;
			if(n == j-1) {
				seglen0 = cuttime[n];
				seglen1 = cuttime[m];
				time = 0.0;
				xbuf = ibuf;
			} else {
				seglen0 = cuttime[n] - cuttime[n-j];
				seglen1 = cuttime[m] - cuttime[m-j];
				time = (double)cuttime[n-j]/srate;
				xbuf = do_weight(&cnt0,&cnt1,ibuf,ibuf3,xbuf,dz);
			}
			diff = seglen1 - seglen0;
			if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
				return(exit_status);
			if(xbuf == ibuf) {	/* FILE 1 */
				if(n == j-1)
					here_in_buf = 0;
				else
					here_in_buf = cuttime[n-j] - startsamp;
				while(here_in_buf >= dz->buflen) {
					memset((char *)ibuf,0,dz->buflen * sizeof(float));
					if(dz->ssampsread > 0) {
						memcpy((char *)ibuf,(char *)ibuf2,dz->buflen * sizeof(float));
						memset((char *)ibuf2,0,dz->buflen * sizeof(float));
						if((exit_status = read_samps(ibuf2,dz))<0)
							return(exit_status);
					}
					startsamp += dz->buflen;
					here_in_buf -= dz->buflen;
				}
				there_in_buf = cuttime[n] - startsamp;
				if(dz->param[PS_RELV2] < 1.0)	/* 1st sound de-emphasized */
					gain = dz->param[PS_RELV2];
				if(dz->param[PS_BIAS] > 0.0)
					advance = seglen0;				/* keep length of seg from snd 1 */
				else							/* bias towards length of 2nd snd's seg */
					advance = seglen0 - (int)round(diff * dz->param[PS_BIAS]);
			} else {
				if(m == cutstart+j-1)
					here_in_buf = 0;
				else
					here_in_buf = cuttime[m-j] - startsamp2;
				while(here_in_buf >= dz->buflen) {
					memset((char *)ibuf3,0,dz->buflen * sizeof(float));
					if(ssampsread2 > 0) {
						memcpy((char *)ibuf3,(char *)ibuf4,dz->buflen * sizeof(float));
						memset((char *)ibuf4,0,dz->buflen * sizeof(float));
						if((ssampsread2 = fgetfbufEx(ibuf4,dz->buflen,dz->ifd[1],0)) < 0) {
							sprintf(errstr,"Can't read samples from 2nd input soundfile.\n");
							return(SYSTEM_ERROR);
						}
					}
					startsamp2 += dz->buflen;
					here_in_buf -= dz->buflen;
				}
				there_in_buf = cuttime[m] - startsamp2;
				if(dz->param[PS_RELV2] > 1.0)	/* 2nd sound de-emphasized */
					gain = 1.0/dz->param[PS_RELV2];
				if(dz->param[PS_BIAS] < 0.0)
					advance = seglen1;				/* keep length of seg from snd 2 */
				else							/* bias towards length of 1st snd's seg */
					advance = seglen1 - (int)round(diff * dz->param[PS_BIAS]);	
												/* diff is reversed from situation with sound 1 (+ becomes -) */
												/* but PS_BIAS is -ve .... so these negs cancel */
			}
			if((overflow = opos - (dz->buflen))>0) {			/* write outbuf */
				if((exit_status = eliminate_too_short_events(&zcnt,&final_pos,&sig_cnt,dz->buflen,0,obuf,1,dz)) < 0)
					return(exit_status);
				if((exit_status = smooth_bad_events(&zcntb,&final_posb,&sig_cntb,stsmoothed,dz->buflen,0,obuf,1,dz)) < 0)
					return(exit_status);
				if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
					return(exit_status);
				lastwritend -= dz->buflen;
				lastwrite += dz->buflen; 
				memset((char*)obuf,0,dz->buflen * sizeof(float));
				if(lastwritend > 0)
					memcpy((char*)obuf,(char*)obuf2,lastwritend * sizeof(float));
				memset((char*)obuf2,0,dz->buflen * sizeof(float));
				opos -= dz->buflen;
			}
			for(k=here_in_buf;k<there_in_buf;k++) {
				thisval = xbuf[k] * gain;
				obuf[opos] = (float)(obuf[opos] + thisval);
				opos++;
			}
			lastwritend = opos;
			opos = startopos + advance;
		}
		if(lastwritend > 0) {
			if((exit_status = eliminate_too_short_events(&zcnt,&final_pos,&sig_cnt,lastwritend,1,obuf,1,dz)) < 0)
				return(exit_status);
			if((exit_status = smooth_bad_events(&zcntb,&final_posb,&sig_cntb,stsmoothed,lastwritend,1,obuf,1,dz)) < 0)
				return(exit_status);
			if((exit_status = write_samps(obuf,lastwritend,dz))<0)
				return(exit_status);
		}
		return FINISHED;
	case(PSOW_REPLACE):
		lastwritend = 0;
		opos = 0;
		startsamp = 0;
		limit = min(cutstart,cutcnt - cutstart);	/* work to end of smallest infile, counted in FOFs */
		for(n = j-1, m = cutstart+j-1; n < limit;n+=j,m+=j) {
			if(n == j-1) {
				seglen = cuttime[m];
				here_in_buf = 0;
			} else {
				seglen = cuttime[m] - cuttime[m-j];
				here_in_buf = cuttime[n-j] - startsamp;
			}
			if(here_in_buf >= dz->buflen) {
				memset((char *)ibuf,0,dz->buflen * sizeof(float));
				if(dz->ssampsread > 0) {
					memcpy((char *)ibuf,(char *)ibuf2,dz->buflen * sizeof(float));
					memset((char *)ibuf2,0,dz->buflen * sizeof(float));
					if((exit_status = read_samps(ibuf2,dz))<0)
						return(exit_status);
				}
				startsamp += dz->buflen;
				here_in_buf -= dz->buflen;
			}
			there_in_buf = cuttime[n] - startsamp;
			if((overflow = opos - (dz->buflen*2))>0) {			/* write outbuf */
				if((exit_status = eliminate_too_short_events(&zcnt,&final_pos,&sig_cnt,dz->buflen,0,obuf,1,dz)) < 0)
					return(exit_status);
				if((exit_status = smooth_bad_events(&zcntb,&final_posb,&sig_cntb,stsmoothed,dz->buflen,0,obuf,1,dz)) < 0)
					return(exit_status);
				if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
					return(exit_status);
				lastwritend -= dz->buflen;
				lastwrite += dz->buflen; 
				memset((char*)obuf,0,dz->buflen * sizeof(float));
				if(lastwritend > 0)
					memcpy((char*)obuf,(char*)obuf2,lastwritend * sizeof(float));
				memset((char*)obuf2,0,dz->buflen * sizeof(float));
				opos -= dz->buflen;
			}
			startopos = opos;
			for(k=here_in_buf;k<there_in_buf;k++) {
				obuf[opos] = (float)(obuf[opos] + ibuf[k]);
				opos++;
			}
			lastwritend = opos;
			opos = startopos + seglen;
		}
		if(lastwritend > 0) {
			if((exit_status = eliminate_too_short_events(&zcnt,&final_pos,&sig_cnt,lastwritend,1,obuf,1,dz)) < 0)
				return(exit_status);
			if((exit_status = smooth_bad_events(&zcntb,&final_posb,&sig_cntb,stsmoothed,lastwritend,1,obuf,1,dz)) < 0)
				return(exit_status);
			if((exit_status = write_samps(obuf,lastwritend,dz))<0)
				return(exit_status);
		}
		return FINISHED;
	case(PSOW_EXTEND):
		xbuf = dz->sampbuf[4];
		lastwritend = 0;
		opos = 0;
		if(cuttime[cutcnt-1] != dz->insams[0])	/* force segment at end */
			cuttime[cutcnt++] = dz->insams[0];
		for(n=cutcnt-1;n>=0;n--)								
			cuttime[n+1]  = cuttime[n];
		cuttime[0] = 0;							/* force segment at start */
		cutcnt++;
		startsamp = (int)round(dz->param[1] * srate);	/* Time at which to look for frozen segment */
		for(n = j; n < cutcnt;n+=j) {
			if(cuttime[n] >= startsamp) {
				if(cuttime[n] == startsamp)
					n+=j;
				break;
			}
		}
		n-= j;
		if(n+j >= cutcnt) {
			sprintf(errstr,"Segment cut too close to end to include %d grains.\n",j);
			return(GOAL_FAILED);
		}
		seglen = cuttime[n+j] - cuttime[n];
		dur = (dz->param[PS_DUR] - dz->duration) * srate;
		repets = (int)round(dur/(double)seglen);
		if(repets <= 1) {
			sprintf(errstr,"File will not be extended,with this value of Output File Duration.\n");
			return(GOAL_FAILED);
		}
		startz = n;
		startsamp = 0;
		done_freeze = 0;
		for(n = j; n < cutcnt;n+=j) {
			seglen = cuttime[n] - cuttime[n-j]; 
			startopos = opos + dz->total_samps_written;
			here_in_buf  = cuttime[n-j] - startsamp;
			there_in_buf = cuttime[n] - startsamp;
			time = (double)startopos/srate;
			if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
				return(exit_status);
			if((overflow = opos - dz->buflen)>0) {
				if((exit_status = eliminate_too_short_events(&zcnt,&final_pos,&sig_cnt,dz->buflen,0,obuf,1,dz)) < 0)
					return(exit_status);
				if((exit_status = smooth_bad_events(&zcntb,&final_posb,&sig_cntb,stsmoothed,dz->buflen,0,obuf,1,dz)) < 0)
					return(exit_status);
				if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
					return(exit_status);
				lastwritend -= dz->buflen;
				lastwrite += dz->buflen; 
				memset((char*)obuf,0,dz->buflen * sizeof(float));
				if(lastwritend > 0)
					memcpy((char*)obuf,(char*)obuf2,lastwritend * sizeof(float));
				memset((char*)obuf2,0,dz->buflen * sizeof(float));
				opos -= dz->buflen;
			}
			if((opos + (there_in_buf - here_in_buf)) >= dz->buflen * 2) {
				sprintf(errstr,"OVERFLOWED OUTPUT BUFFER.\n");
				return(MEMORY_ERROR);
			}
			if(here_in_buf >= dz->buflen * 2) {
				sprintf(errstr,"OVERFLOWED INPUT BUFFER.\n");
				return(MEMORY_ERROR);
			}
			while(here_in_buf > dz->buflen) {
				memset((char *)ibuf,0,dz->buflen * sizeof(float));
				if(dz->ssampsread > 0) {
					memcpy((char *)ibuf,(char *)ibuf2,dz->buflen * sizeof(float));
					memset((char *)ibuf2,0,dz->buflen * sizeof(float));
					if((exit_status = read_samps(ibuf2,dz))<0)
						return(exit_status);
				}
				startsamp += dz->buflen;
				here_in_buf  -= dz->buflen;
				there_in_buf -= dz->buflen;
			}
			if((n < startz) || done_freeze) {
				if(zerofof(here_in_buf,there_in_buf,ibuf)) {
					for(k = here_in_buf; k < there_in_buf;k++)
						opos++;
				} else if (dz->brksize[PSE_GAIN]) {
					for(k = here_in_buf; k < there_in_buf;k++) {
						startopos = opos + dz->total_samps_written;
						time = (double)startopos/srate;
						if((exit_status = read_value_from_brktable(time,PSE_GAIN,dz))<0)
							return(exit_status);
						obuf[opos] = (float)(obuf[opos] + (ibuf[k] * dz->param[PSE_GAIN]));
						opos++;
					}
				} else {
					for(k = here_in_buf; k < there_in_buf;k++) {
						obuf[opos] = (float)(obuf[opos] + ibuf[k]);
						opos++;
					}
				}
				if(opos > lastwritend)
					lastwritend = opos;
				if(dz->param[PSE_VDEP] > 0.0) {
					vibshift = vibrato(seglen,dz->param[PSE_VFRQ],dz->param[PSE_VDEP],dz);
					opos += vibshift;
				}
			} else  {
				ingraintime = 0;									/* Start measuring time in brkpnt for grain frq transpositions */
				grainlen = there_in_buf - here_in_buf;
				if(dz->vflag[0]) {
					sinstep =  PI/(double)grainlen;
					thissin = 0.0;
					for(k=here_in_buf,m=0;k<there_in_buf;k++,m++) {
						xbuf[m] = (float)(sin(thissin) * ibuf[k]);
						thissin += sinstep;
					}
				} else {
					for(k=here_in_buf,m=0;k<there_in_buf;k++,m++)
						xbuf[m] = ibuf[k];
				}
				limit = (int)round((dz->param[1] + dz->param[2] - dz->duration) * (double)dz->infile->srate);   // End of freeze section
				for(k = 0;k<repets;k++) {
					if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
						return(exit_status);
					if(dz->brksize[PSE_TRNS] > 0) {
						if((exit_status = read_value_from_brktable(ingraintime,PSE_TRNS,dz))<0)
							return(exit_status);
					}
					segstep = (int)floor((double)seglen/dz->param[PSE_TRNS]);	/*	segstep corresponds to wavelen of transposed seg */
					bakstep = seglen - segstep;									/*	This results in next seg being written 'bakstep' earlier */
					opos -= bakstep;
					timestep = (double)segstep/(double)srate;
					ingraintime += timestep;
					time += timestep;
					if((overflow = opos - dz->buflen)>0) {
						if((exit_status = eliminate_too_short_events(&zcnt,&final_pos,&sig_cnt,dz->buflen,0,obuf,1,dz)) < 0)
							return(exit_status);
						if((exit_status = smooth_bad_events(&zcntb,&final_posb,&sig_cntb,stsmoothed,dz->buflen,0,obuf,1,dz)) < 0)
							return(exit_status);
						if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
							return(exit_status);
						lastwritend -= dz->buflen;
						lastwrite += dz->buflen; 
						memset((char*)obuf,0,dz->buflen * sizeof(float));
						if(lastwritend > 0)
							memcpy((char*)obuf,(char*)obuf2,lastwritend * sizeof(float));
						memset((char*)obuf2,0,dz->buflen * sizeof(float));
						opos -= dz->buflen;
					}
					if(dz->total_samps_written + opos > limit) {
						done_freeze = 1;
						break;
					}
					if (dz->brksize[PSE_GAIN]) {
						for(m=0;m<seglen;m++) {
							startopos = opos + dz->total_samps_written;
							time = (double)startopos/srate;
							if((exit_status = read_value_from_brktable(time,PSE_GAIN,dz))<0)
								return(exit_status);
							obuf[opos] = (float)(obuf[opos] + (xbuf[m] * dz->param[PSE_GAIN]));
							opos++;
						}
					} else {				
						for(m=0;m<seglen;m++) {
							obuf[opos] = (float)(obuf[opos] + xbuf[m]);
							opos++;
						}
					}
					if(opos > lastwritend)
						lastwritend = opos;
					if(dz->param[PSE_VDEP] > 0.0) {							/* vibrato rate relates to seg-advance (segstep) (???) */
						vibshift = vibrato(segstep,dz->param[PSE_VFRQ],dz->param[PSE_VDEP],dz);
						opos += vibshift;
					}
				}						
				done_freeze = 1;
			} 
		}
		if(lastwritend > 0) {
//			if((exit_status = eliminate_too_short_events(&zcnt,&final_pos,&sig_cnt,lastwritend,1,obuf,1,dz)) < 0)
//				return(exit_status);
			if((exit_status = smooth_bad_events(&zcntb,&final_posb,&sig_cntb,stsmoothed,lastwritend,1,obuf,1,dz)) < 0)
				return(exit_status);
			if((exit_status = write_samps(obuf,lastwritend,dz))<0)
				return(exit_status);
		}
		return FINISHED;
	case(PSOW_EXTEND2):
		xbuf = dz->sampbuf[4];
		lastwritend = 0;
		opos = 0;
		dur = (dz->param[PS_DUR] - dz->duration) * srate;
		repets = (int)round(dur/(double)seglen);
		if(repets <= 1) {
			sprintf(errstr,"File will not be extended,with this value of Output File Duration.\n");
			return(GOAL_FAILED);
		}
		startbuf = 0;
		while(dz->total_samps_read < startcut) {
			if((exit_status = write_samps(ibuf,dz->buflen,dz))<0)
				return(exit_status);
			startbuf += dz->buflen; 
			memcpy((char *)ibuf,(char *)ibuf2,dz->buflen * sizeof(float));
			memset((char *)ibuf2,0,dz->buflen * sizeof(float));
			last_sampsread = dz->ssampsread;
			if((exit_status = read_samps(ibuf2,dz))<0)
				return(exit_status);
		}
		if(startcut - startbuf > dz->buflen) {
			if((exit_status = write_samps(ibuf,dz->buflen,dz))<0)
				return(exit_status);
			startbuf += dz->buflen; 
			memcpy((char *)ibuf,(char *)ibuf2,dz->buflen * sizeof(float));
			memset((char *)ibuf2,0,dz->buflen * sizeof(float));
			last_sampsread = dz->ssampsread;
			if((exit_status = read_samps(ibuf2,dz))<0)
				return(exit_status);
			startbuf += dz->buflen;
		}
		here_in_buf  = startcut - startbuf;
		there_in_buf = endcut - startbuf;
		for(n=0;n<here_in_buf;n++)
			obuf[opos++] = ibuf[n];
		lastwritend = opos;
		for(k=here_in_buf,m=0;k<there_in_buf;k++,m++)
			xbuf[m] = ibuf[k];
		for(k = 0;k<repets;k++) {
			if((overflow = opos - dz->buflen)>0) {
				if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
					return(exit_status);
				lastwritend -= dz->buflen;
				memset((char*)obuf,0,dz->buflen * sizeof(float));
				if(lastwritend > 0)
					memcpy((char*)obuf,(char*)obuf2,lastwritend * sizeof(float));
				memset((char*)obuf2,0,dz->buflen * sizeof(float));
				opos -= dz->buflen;
			}
			startopos = opos;
			for(m=0;m<seglen;m++) {
				obuf[opos] = (float)(obuf[opos] + xbuf[m]);
				opos++;
			}
			if(opos > lastwritend)
				lastwritend = opos;
			time = (double)(startopos + dz->total_samps_written)/srate;
			if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
				return(exit_status);
			if(dz->param[PS2_VDEP] > 0.0)
				opos += vibrato(seglen,dz->param[PS2_VFRQ],dz->param[PS2_VDEP],dz);
		}
		for(n=there_in_buf;n<last_sampsread;n++) {
			obuf[opos++] = ibuf[n];
			lastwritend = opos;
			if((overflow = opos - dz->buflen)>0) {
				if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
					return(exit_status);
				lastwritend -= dz->buflen;
				memset((char*)obuf,0,dz->buflen * sizeof(float));
				if(lastwritend > 0)
					memcpy((char*)obuf,(char*)obuf2,lastwritend * sizeof(float));
				memset((char*)obuf2,0,dz->buflen * sizeof(float));
				opos -= dz->buflen;
			}
		}
		while(dz->ssampsread > 0) {
			memcpy((char *)ibuf,(char *)ibuf2,dz->buflen * sizeof(float));
			memset((char *)ibuf2,0,dz->buflen * sizeof(float));
			last_sampsread = dz->ssampsread;
			if((exit_status = read_samps(ibuf2,dz))<0)
				return(exit_status);
			for(n=0;n<last_sampsread;n++) {
				obuf[opos++] = ibuf[n];
				lastwritend = opos;
				if((overflow = opos - dz->buflen)>0) {
					if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
						return(exit_status);
					lastwritend -= dz->buflen;
					memset((char*)obuf,0,dz->buflen * sizeof(float));
					if(lastwritend > 0)
						memcpy((char*)obuf,(char*)obuf2,lastwritend * sizeof(float));
					memset((char*)obuf2,0,dz->buflen * sizeof(float));
					opos -= dz->buflen;
				}
			}
		}
		if(lastwritend > 0) {
			if((exit_status = write_samps(obuf,lastwritend,dz))<0)
				return(exit_status);
		}
		return FINISHED;
	case(PSOW_LOCATE):
		startsamp = (int)round(dz->param[1] * srate);
		for(n=0;n<cutcnt;n++) {
			if(cuttime[n] >= startsamp) {
				diff = startsamp - cuttime[n];
				if(n == cutcnt-1)
					endsamp = dz->insams[0];
				else
					endsamp = cuttime[n+1];
				if(endsamp - startsamp < diff)
					n++;
				if(n >= cutcnt) {
					sprintf(errstr,"Time is too close to end of file.\n");
					return(GOAL_FAILED);
				}
				time = (double)cuttime[n]/srate;
				fprintf(stdout,"INFO: TIME %lf IS NEAREST GRAIN START %lf\n",dz->param[1],time);
				break;
			}
		}
		fflush(stdout);
		return FINISHED;
	case(PSOW_CUT):
		startsamp = (int)round(dz->param[1] * srate);
		time = -1.0;
		for(n=0;n<cutcnt;n++) {
			if(cuttime[n] >= startsamp) {
				diff = startsamp - cuttime[n];
				if(n == cutcnt-1)
					endsamp = dz->insams[0];
				else
					endsamp = cuttime[n+1];
				if(endsamp - startsamp < diff)
					n++;
				if(n >= cutcnt) {
					sprintf(errstr,"Time is too close to end of file.\n");
					return(GOAL_FAILED);
				}
				time = (double)cuttime[n]/srate;
				fprintf(stdout,"INFO: TIME %lf IS NEAREST GRAIN START %lf\n",dz->param[1],time);
				break;
			}
		}
		if(time < 0.0) {
			sprintf(errstr,"Cut Time is not within the input sound.");
			return(GOAL_FAILED);
		}
		fflush(stdout);
		opos = 0;
		startsamp = (int)round(time * srate);
		if(startsamp <= 0) {
			sprintf(errstr,"Cut is at very start of file. No change to source sound");
			return(GOAL_FAILED);
		}
		if(startsamp >= dz->insams[0]) {
			fprintf(stdout,"WARNING: Grain number beyond end of file.\n");
			fflush(stdout);
			startsamp = cutcnt-1;
		}
		switch(dz->mode) {
		case(0):
			while(startsamp >= dz->buflen) {
				if((exit_status = write_samps(ibuf,dz->buflen,dz))<0)
					return(exit_status);
				if((exit_status = read_samps(ibuf,dz))<0)
					return(exit_status);
				startsamp -= dz->buflen;
			}	
			if(startsamp > 0) {
				if((exit_status = write_samps(ibuf,startsamp,dz))<0)
					return(exit_status);
			}
			break;
		case(1):
			while(startsamp >= dz->buflen) {
				if((exit_status = read_samps(ibuf,dz))<0)
						return(exit_status);
				startsamp -= dz->buflen;
			}	
			while(dz->ssampsread > 0) {
				for(n=startsamp;n<dz->ssampsread;n++)
					obuf[opos++] = ibuf[n];
				if((exit_status = read_samps(ibuf,dz))<0)
					return(exit_status);
				if(opos >= dz->buflen) {
					if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
						return(exit_status);
					memset((char*)obuf,0,dz->buflen * sizeof(float));
					memcpy((char*)obuf,(char*)obuf2,dz->buflen * sizeof(float));
					opos -= dz->buflen;
				}
				startsamp = 0;
			}
			break;
		}
		if(opos > 0) {
			if((exit_status = write_samps(obuf,opos,dz))<0)
				return(exit_status);
		}
		return FINISHED;
	case(PSOW_REINF):
		maxamp = 0.0;
		ovflw = dz->sampbuf[4];
		if(dz->mode == 0) {
			hno = dz->lparray[0];
			amp = dz->parray[0];
		} else {
			ihno = dz->parray[0];
			amp = dz->parray[1];
		}
		lastwritend = 0;
		memset((char*)ovflw,0,dz->buflen * sizeof(float));
		if(dz->mode == 0 && dz->vflag[0])
			sort_harmonics_to_ascending_order(dz);
		fprintf(stdout,"INFO: Assessing final level\n");
		fflush(stdout);
		for(n = 0; n < cutcnt;n++) {
 			here_in_buf = here - startsamp;
			there_in_buf = cuttime[n] - startsamp;
			seglen = there_in_buf - here_in_buf;
			opos = here;	/* absolute output position */
			opos -= lastwrite;							/* output position relative to buf start */
			if((overflow = opos - (dz->buflen * 2))>0) {
				if((exit_status = eliminate_too_short_events(&zcnt,&final_pos,&sig_cnt,dz->buflen,0,obuf,1,dz)) < 0)
					return(exit_status);
				if((exit_status = smooth_bad_events(&zcntb,&final_posb,&sig_cntb,stsmoothed,dz->buflen,0,obuf,1,dz)) < 0)
					return(exit_status);
				for(j=0;j<dz->buflen;j++)
					maxamp = max(fabs(obuf[j]),maxamp);
				lastwrite += dz->buflen; 
				display_virtual_time(lastwrite,dz);
				memcpy((char*)obuf,(char*)obuf2,dz->buflen * sizeof(float));
				memset((char*)obuf2,0,dz->buflen * sizeof(float));
				memcpy((char*)obuf2,(char*)ovflw,overflow * sizeof(float));
				memset((char*)ovflw,0,dz->buflen * sizeof(float));
				lastwritend -= dz->buflen;
				opos -= dz->buflen;
			}
			if(dz->mode == 0) {
				if((opos + (seglen * 2) + (dz->iparam[ISTR] * dz->itemcnt)) >= dz->buflen * 3) {
					sprintf(errstr,"OVERFLOWED OUTPUT BUFFER.\n");
					return(MEMORY_ERROR);
				}
			} else {
				if((opos + (seglen * (1 + dz->param[ISTR]))) >= dz->buflen * 3) {
					sprintf(errstr,"OVERFLOWED OUTPUT BUFFER.\n");
					return(MEMORY_ERROR);
				}
			}
			startopos = opos;
			display_virtual_time(here,dz);
			for(k = here_in_buf; k < there_in_buf;k++)
				obuf[opos++] += ibuf[k];
			if(opos > lastwritend)
				lastwritend = opos;
			if(dz->mode == 0) {		/* HARMONIC */
				for(m=0;m<dz->itemcnt;m++) {
					dstep = (double)seglen/(double)hno[m];
					gain  = amp[m];				
					dhere  = (double)startopos + ((m+1) * dz->iparam[ISTR]);
					j = 0;
					while(j < hno[m]) {
						opos = (int)round(dhere);
						if(dz->mode == 0 && dz->vflag[0]) {
							if(j==0 || ((m > 0) && harmonic_is_duplicated(j,m,hno))) {
								dhere += dstep;
								j++;
								continue;
							}
						}
						for(k = here_in_buf; k < there_in_buf;k++)
							obuf[opos++] += (float)(ibuf[k] * gain);
						if(opos > lastwritend)
							lastwritend = opos;
						dhere += dstep;
						j++;
					}
				}
			} else {				/* INHARMONIC */
				for(m=0;m<dz->itemcnt;m++) {
					dstep = (double)seglen/ihno[m];
					gain  = amp[m];				
					dhere  = (double)startopos;
					j = 0;
					while((double)j < (ihno[m] * dz->param[ISTR])) {
						opos = (int)round(dhere);
						if(j > 0) {
							for(k = here_in_buf; k < there_in_buf;k++)
								obuf[opos++] += (float)(ibuf[k] * gain);
							if(opos > lastwritend)
								lastwritend = opos;
						}
						dhere += dstep;
						j++;
					}
				}
			}
			here = cuttime[n];
			if(there_in_buf >= dz->buflen) {
				memcpy((char*)ibuf,(char*)ibuf2,dz->buflen * sizeof(float));
				memset((char*)ibuf2,0,dz->buflen * sizeof(float));
				if((exit_status = read_samps(ibuf2,dz))<0)
					return(exit_status);
				startsamp += dz->buflen;
			}
		}
		opos = 	lastwritend;
		if(opos > 0) {
			if((exit_status = eliminate_too_short_events(&zcnt,&final_pos,&sig_cnt,opos,1,obuf,1,dz)) < 0)
				return(exit_status);
			if((exit_status = smooth_bad_events(&zcntb,&final_posb,&sig_cntb,stsmoothed,opos,1,obuf,1,dz)) < 0)
				return(exit_status);
			display_virtual_time(dz->insams[0],dz);
			for(j=0;j<opos;j++)
				maxamp = max(fabs(obuf[j]),maxamp);
		}
		if(maxamp > .999)
			atten = .999/maxamp;
		else
			atten = 1.0;
        if((sndseekEx(dz->ifd[0],0,0)<0)){
            sprintf(errstr,"sndseek() failed\n");
            return SYSTEM_ERROR;
        }
		dz->total_samps_read = 0;
		dz->samps_left = dz->insams[0];
		here = 0;
		lastwritend = 0;
		memset((char*)ovflw,0,dz->buflen * sizeof(float));
		memset((char*)obuf,0,dz->buflen * 2 * sizeof(float));
		memset((char*)ibuf,0,dz->buflen * 2 * sizeof(float));
		if((exit_status = read_samps(ibuf,dz))<0)
			return(exit_status);
		if((exit_status = read_samps(ibuf2,dz))<0)
			return(exit_status);
		display_virtual_time(0,dz);
		fprintf(stdout,"INFO: Creating the output.\n");
		fflush(stdout);
		for(n = 0; n < cutcnt;n++) {
 			here_in_buf = here - startsamp;
			there_in_buf = cuttime[n] - startsamp;
			seglen = there_in_buf - here_in_buf;
			opos = here;	/* absolute output position */
			opos -= lastwrite;							/* output position relative to buf start */
			if((overflow = opos - (dz->buflen * 2))>0) {
				if((exit_status = eliminate_too_short_events(&zcnt,&final_pos,&sig_cnt,dz->buflen,0,obuf,1,dz)) < 0)
					return(exit_status);
				if((exit_status = smooth_bad_events(&zcntb,&final_posb,&sig_cntb,stsmoothed,dz->buflen,0,obuf,1,dz)) < 0)
					return(exit_status);
				for(j=0;j<dz->buflen;j++)
					obuf[j] = (float)(obuf[j] * atten);
				if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
					return(exit_status);
				lastwrite += dz->buflen; 
				memcpy((char*)obuf,(char*)obuf2,dz->buflen * sizeof(float));
				memset((char*)obuf2,0,dz->buflen * sizeof(float));
				memcpy((char*)obuf2,(char*)ovflw,overflow * sizeof(float));
				memset((char*)ovflw,0,dz->buflen * sizeof(float));
				lastwritend -= dz->buflen;
				opos -= dz->buflen;
			}
			if((opos + (seglen * 2)) >= dz->buflen * 3) {
				sprintf(errstr,"OVERFLOWED OUTPUT BUFFER.\n");
				return(MEMORY_ERROR);
			}
			startopos = opos;
			display_virtual_time(here,dz);
			for(k = here_in_buf; k < there_in_buf;k++)
				obuf[opos++] += ibuf[k];
			if(opos > lastwritend)
				lastwritend = opos;
			if(dz->mode == 0) {	/* HARMONIC */
				for(m=0;m<dz->itemcnt;m++) {
					dstep = (double)seglen/(double)hno[m];
					gain  = amp[m];				
					dhere  = (double)startopos + ((m+1) * dz->iparam[ISTR]);
					j = 0;
					while(j < hno[m]) {
						opos = (int)round(dhere);
						if(dz->mode == 0 && dz->vflag[0]) {
							if(j==0 || ((m > 0) && harmonic_is_duplicated(j,m,hno))) {
								dhere += dstep;
								j++;
								continue;
							}
						}
						for(k = here_in_buf; k < there_in_buf;k++)
							obuf[opos++] += (float)(ibuf[k] * gain);
						if(opos > lastwritend)
							lastwritend = opos;
						dhere += dstep;
						j++;
					}
				}
			} else {	/* INHARMONIC */
				for(m=0;m<dz->itemcnt;m++) {
					dstep = (double)seglen/ihno[m];
					gain  = amp[m];				
					dhere  = (double)startopos;
					j = 0;
					while((double)j < (ihno[m] * dz->param[ISTR])) {
						opos = (int)round(dhere);
						if(j > 0) {
							for(k = here_in_buf; k < there_in_buf;k++)
								obuf[opos++] += (float)(ibuf[k] * gain);
							if(opos > lastwritend)
								lastwritend = opos;
						}
						dhere += dstep;
						j++;
					}
				}
			}
			here = cuttime[n];
			if(there_in_buf >= dz->buflen) {
				memcpy((char*)ibuf,(char*)ibuf2,dz->buflen * sizeof(float));
				memset((char*)ibuf2,0,dz->buflen * sizeof(float));
				if((exit_status = read_samps(ibuf2,dz))<0)
					return(exit_status);
				startsamp += dz->buflen;
			}
		}
		opos = 	lastwritend;
		if(opos > 0) {
			if((exit_status = eliminate_too_short_events(&zcnt,&final_pos,&sig_cnt,opos,1,obuf,1,dz)) < 0)
				return(exit_status);
			if((exit_status = smooth_bad_events(&zcntb,&final_posb,&sig_cntb,stsmoothed,opos,1,obuf,1,dz)) < 0)
				return(exit_status);
			for(j=0;j<opos;j++)
				obuf[j] = (float)(obuf[j] * atten);
			if((exit_status = write_samps(obuf,opos,dz))<0)
				return(exit_status);
		}
		return FINISHED;
	}
	if(opos > 0) {
		if((exit_status = eliminate_too_short_events(&zcnt,&final_pos,&sig_cnt,opos,1,obuf,1,dz)) < 0)
			return(exit_status);
		if((exit_status = smooth_bad_events(&zcntb,&final_posb,&sig_cntb,stsmoothed,opos,1,obuf,1,dz)) < 0)
			return(exit_status);
		if((exit_status = write_samps(obuf,opos,dz))<0)
			return(exit_status);
	}
	return FINISHED;
}

/************************* EXTRACT_PITCH_DEPENDENT_ENV_FROM_SNDFILE *******************************/

int extract_pitch_dependent_env_from_sndfile(int minwsize,int k,int *maxwsize,dataptr dz)
{
	int exit_status;
	double tconvertor = 1.0/(double)dz->infile->srate;
	double fconvertor = (double)dz->infile->srate/WINDIV;
	int  start_samp = 0, envwindow_sampsize, big_envwindow_sampsize, here = 0, start_buf = 0;
	float *env = dz->env;
	double time = 0.0;

	if((dz->ssampsread = fgetfbufEx(dz->sampbuf[0], dz->buflen,dz->ifd[k],0)) < 0) {
		sprintf(errstr,"Can't read samples from input soundfile %d\n",k+1);
		return(SYSTEM_ERROR);
	}
	if(dz->ssampsread < dz->insams[k]) {
		if((dz->ssampsread = fgetfbufEx(dz->sampbuf[1], dz->buflen,dz->ifd[k],0)) < 0) {
			sprintf(errstr,"Can't read samples from input soundfile %d\n",k+1);
			return(SYSTEM_ERROR);
		}
	}
	if((exit_status = read_value_from_brktable(time,k,dz))<0)
		return(exit_status);
	if(dz->param[k] < 0.0)		/* no-pitch */
		envwindow_sampsize = minwsize;
	else
		envwindow_sampsize = (int)round(fconvertor/dz->param[k]);
	while(here < dz->insams[k]) {
		if(here - start_buf >= dz->buflen) {
			memset((char *)dz->sampbuf[0],0,dz->buflen * sizeof(float));
			memcpy((char *)dz->sampbuf[0],(char *)dz->sampbuf[1],dz->buflen * sizeof(float));
			start_buf += dz->buflen;
			if(dz->total_samps_read < dz->insams[k]) {
				if((dz->ssampsread = fgetfbufEx(dz->sampbuf[1], dz->buflen,dz->ifd[k],0)) < 0) {
					sprintf(errstr,"Can't read samples from input soundfile %d\n",k+1);
					return(SYSTEM_ERROR);
				}
			}
		}	
		big_envwindow_sampsize = envwindow_sampsize * 2;
		*env++ = (float)time;
		if(here + big_envwindow_sampsize >= dz->insams[k]) {
			here -= envwindow_sampsize;
			big_envwindow_sampsize = dz->insams[k] - here;
			*env++ = getmaxsampr(start_samp,big_envwindow_sampsize,dz->sampbuf[0]);
			break;
		}
		start_samp = here - start_buf;
		*env++ = getmaxsampr(start_samp,big_envwindow_sampsize,dz->sampbuf[0]);
		here += envwindow_sampsize;
		time   = (float)(here * tconvertor);
		if((exit_status = read_value_from_brktable(time,k,dz))<0)
			return(exit_status);
		if(dz->param[k] < 0.0)		/* no-pitch */
			envwindow_sampsize = minwsize;
		else
			envwindow_sampsize = (int)round(fconvertor/dz->param[k]);
		if(envwindow_sampsize > *maxwsize)
			*maxwsize = envwindow_sampsize;
	}
	*maxwsize *= 2;	/* using overlapping windows */
	dz->envend = env;
	memset((char *)dz->sampbuf[0],0,dz->buflen * sizeof(float));
	memset((char *)dz->sampbuf[1],0,dz->buflen * sizeof(float));
	return FINISHED;
}

/*************************** GETMAXSAMPR ******************************/

float getmaxsampr(int startsamp, int sampcnt,float *buffer)
{
	int  i, endsamp = startsamp + sampcnt;
	float thisval, thismaxsamp = -1;
	for(i = startsamp; i<endsamp; i++) {
		if((thisval = (float)fabs(buffer[i]))>thismaxsamp)
			thismaxsamp = thisval;
	}
	return(thismaxsamp);
}

/************************** GET_MIN_WSIZE **************************/

int get_min_wsize(int *minwsize, int k, dataptr dz) {
	int n,m;
	int wsize;
	double fconvertor = (double)dz->infile->srate/WINDIV, frq;
	*minwsize = dz->insams[k];
	for(n=0,m=1;n<dz->brksize[k];n++,m+=2) {
		frq = dz->brk[k][m];
		if(frq > 0.0) {
			wsize = (int)round(fconvertor/frq);
			if(wsize < *minwsize)
				*minwsize = wsize;
		}
	}
	if(*minwsize >= dz->insams[k])
		return(DATA_ERROR);
	return FINISHED;
}

/************************** FIND_MIN_ENERGY_DOWNWARD_ZERO_CROSSING_POINT **************************/

int find_min_energy_downward_zero_crossing_point(int *n,int *trofpnt,int trofpntcnt,
			double *scanarray,int *cutcnt,int *cut,int kk,int cutstart,dataptr dz)
{
	int exit_status;
	int check;
	float starttime, val, *buf = dz->sampbuf[0];
	int wsize, thissamp, hisamp, losamp, j, zc_cnt, at;
	int endsamp, startsamp, seglen;
	int newpos, diff, losampinbuf, hisampinbuf, here, there, localpeakcnt;
	int k, first_downcross=0, mindiff, goalseglen, newseglen, thiscut;
	double official_pitchlen;
	int pitchseg_lolimit, pitchseg_hilimit;
	int done;
	int pitchwindow, ideal_place, last_startsamp, thiscutinbuf, lastcut, lastcutinbuf;
	int seg1, seg2;
	double rat;
	endsamp = dz->total_samps_read;
	startsamp = endsamp - dz->ssampsread;
	j = trofpnt[*n];
	starttime = dz->env[j];
	wsize = read_validpitch_wsize_in_samps_from_brktable(starttime,kk,dz);
	thissamp = (int)round(starttime * (double)dz->infile->srate);
	losamp = thissamp;
	if((exit_status = read_value_from_brktable(starttime,kk,dz))<0)	/* get pitch at start of window */
		return(exit_status);
	if(losamp >= dz->insams[kk])
		return(FINISHED);
	while(losamp >= endsamp) {
		if((dz->ssampsread = fgetfbufEx(dz->sampbuf[0], dz->buflen,dz->ifd[kk],0)) < 0) {
			sprintf(errstr,"Can't read samples from input soundfile %d\n",kk+1);
			return(SYSTEM_ERROR);
		}
		dz->total_samps_read += dz->ssampsread;
		dz->samps_left -= dz->ssampsread;
		endsamp = dz->total_samps_read;
		startsamp = endsamp - dz->ssampsread;
	}
	hisamp = min(dz->insams[kk],thissamp + wsize);	/* get end-of-search area from  wtime & half-wsize */
	if(hisamp >= dz->insams[kk])
		hisamp = dz->insams[kk];
	if(hisamp > endsamp) {
		newpos = (losamp/SECSIZE) * SECSIZE;			/* if end-of--search beyond end of current buf */
        if((sndseekEx(dz->ifd[kk],newpos,0)<0)){        /* adjust buffer to contain whole search area */
            sprintf(errstr,"sndseek() failed\n");
            return SYSTEM_ERROR;
        }
		diff = dz->total_samps_read - newpos;
		dz->total_samps_read -= diff;
		dz->samps_left += diff;
		if((dz->ssampsread = fgetfbufEx(dz->sampbuf[0], dz->buflen,dz->ifd[kk],0)) < 0) {
			sprintf(errstr,"Can't read samples from input soundfile %d\n",kk+1);
			return(SYSTEM_ERROR);
		}
		dz->total_samps_read += dz->ssampsread;
		dz->samps_left -= dz->ssampsread;
		endsamp = dz->total_samps_read;
		startsamp = endsamp - dz->ssampsread;
	}
	losampinbuf = losamp - startsamp;					/* get search ends relative to buffer */
	hisampinbuf = hisamp - startsamp;
	if(dz->param[kk] > 0) {
		pitchwindow = (int)round((double)dz->infile->srate/dz->param[kk]);
		ideal_place = cut[(*cutcnt) - 1] + pitchwindow;
		ideal_place -= startsamp;
	} else {
		ideal_place = -1; 
	}
	here  = losampinbuf;
	there = hisampinbuf;
	zc_cnt = count_zerocrossings(losampinbuf,hisampinbuf,buf);
	while(zc_cnt > 5)	/* Divide segment into 4 quarters, making 3 overlapping areas, while any area has >= 5 zcrossings */
		zc_cnt = triangulate_env(&here,&there,ideal_place,buf);	/* and find area with least energy */
	
	losamp = here;
//NEW
	losamp = here + startsamp;
	if((exit_status = find_the_local_peaks(&here,&there,buf,n,trofpntcnt,trofpnt,
		 &startsamp,&endsamp,losamp,cut,*cutcnt,scanarray,scanarray,&localpeakcnt,&first_downcross,kk,dz) )<0)
		 return(exit_status);
	if(exit_status == FINISHED)
		 return(exit_status);

	if((exit_status = mark_cut(cutcnt,cut,localpeakcnt,scanarray,here,there,startsamp,first_downcross,starttime,1,dz))<0)
		return(exit_status);
	if((*cutcnt)-1 < cutstart)		/* FIND LENGTH OF SEGMENT GENERATED */
		seglen = cut[*cutcnt];
	else
		seglen = cut[*cutcnt] - cut[(*cutcnt)-1];
	if(seglen == 0) {		/* ELIMINATE ZERO-LENGTH SEGMENTS (CUTS AT SAME TIME) */
		(*cutcnt)--;
	} else if(*cutcnt > cutstart && dz->param[kk] > 0) {
							/* CHECK THE LENGTH OF THE WINDOW AGAINST WHAT WE'D PREDICT FROM THE PITCH VALUE */	
		check = 1;
		while(check) {
			official_pitchlen = (double)dz->infile->srate/dz->param[kk];
			pitchseg_lolimit = (int)round(official_pitchlen/PITCHERROR);
			pitchseg_hilimit = (int)round(official_pitchlen * PITCHERROR);
											/* IF SEGLEN IS ACCEPTABLE, BREAK */
			if(seglen > pitchseg_lolimit && seglen < pitchseg_hilimit) {
				break;
			}
			if(seglen < pitchseg_lolimit) {	/* TOO SHORT ITEM, DELETE FROM CUTLIST */
				(*cutcnt)--;
				break;
			}
									/* TOO LONG ITEM */
					/* BRUTE FORCE METHOD, CUT AT ANY DOWN ZEROCROSS NEAR THE RIGHT TIME */
							/* TRY TO MAKE A CUT HALF-WAY THROUGH THE SEGMENT */
			at = 0;
			mindiff = seglen;				
			goalseglen = seglen/2;					/* set the goal segement length to half the current seglen */
													/* Geteach zero-crossing in this segment */
			last_startsamp = -1;
			thiscutinbuf = cut[*cutcnt] - startsamp;
			lastcut = cut[(*cutcnt) - 1];
			lastcutinbuf = lastcut - startsamp;
			if(lastcutinbuf < 0) {
				last_startsamp = startsamp;
                if((sndseekEx(dz->ifd[kk],lastcut,0)<0)){        
                    sprintf(errstr,"sndseek() failed\n");
                    return SYSTEM_ERROR;
                }
				dz->total_samps_read = lastcut;
				if((dz->ssampsread = fgetfbufEx(dz->sampbuf[0], dz->buflen,dz->ifd[kk],0)) < 0) {
					sprintf(errstr,"Can't read samples from input soundfile %d\n",kk+1);
					return(SYSTEM_ERROR);
				}
				dz->total_samps_read += dz->ssampsread;
				dz->samps_left = dz->insams[kk] - dz->total_samps_read;
				startsamp = dz->total_samps_read - dz->ssampsread;
				thiscutinbuf  -= lastcutinbuf;	/* lastcutinbuf is -ve */
				lastcutinbuf = 0;
			}
			done = 0;
			k = lastcutinbuf;
			val = buf[++k];
			while(k < thiscutinbuf) {
				if(val > 0.0) {
					while(val >= 0.0) {
						k++;
						if(k >= thiscutinbuf) {
							done = 1;
							break;
						}
						val = buf[k];
					}
				}
				if(done)
					break;
				if(val < 0.0) {
					newseglen = k - lastcutinbuf;
					diff = newseglen - goalseglen;
					if(abs(diff) < mindiff) {			/* if a segment cut from here is closer to the goallength, mark it */
						mindiff = abs(diff);
						at = k + startsamp;
					} else if(diff > 0)	{				/* if not closer, and already above goal length, finished */
						break;
					}
					while(val < 0.0) {
						k++;
						if(k >= thiscutinbuf) {
							done = 1;
							break;
						}
						val = buf[k];
					}
				}
				if(done)
					break;
				if(val == 0.0) {
					while(val == 0.0) {
						k++;
						if(k >= thiscutinbuf)
							break;
						val = buf[k];
					}
				}
			}
			if(last_startsamp >= 0) {				/* rejiggle bufs */
                if((sndseekEx(dz->ifd[kk],startsamp,0)<0)){        
                    sprintf(errstr,"sndseek() failed\n");
                    return SYSTEM_ERROR;
                }
				dz->total_samps_read = startsamp;
				if((dz->ssampsread = fgetfbufEx(dz->sampbuf[0], dz->buflen,dz->ifd[kk],0)) < 0) {
					sprintf(errstr,"Can't read samples from input soundfile %d\n",kk+1);
					return(SYSTEM_ERROR);
				}
				dz->total_samps_read += dz->ssampsread;
				dz->samps_left = dz->insams[kk] - dz->total_samps_read;
			}
			if(at) {								/* if appropriate cutpoint found, mark it */
				seg1 = (at - startsamp) - lastcutinbuf;
				seg2 = thiscutinbuf - (at - startsamp);
				rat = (double)seg1/(double)seg2;
				if(rat  < 2.0 && rat > .5) {	/* if new cut is reasonable */
					thiscut = cut[*cutcnt];		/* insert new cuts */
					cut[*cutcnt] = at;
					(*cutcnt)++;
					cut[*cutcnt] = thiscut;
				}
			}
			break;
		}
	}
	return FINISHED;
}

/**************************** READ_VALIDPITCH_WSIZE_IN_SAMPS_FROM_BRKTABLE *****************************/

int read_validpitch_wsize_in_samps_from_brktable(double thistime,int kk,dataptr dz)
{
    double dnval = 0.0, upval = 0.0, val, time;
    double diff, mindiff = HUGE;
	double dntimediff = -1.0, uptimediff = -1.0;
	int n, m, wsize, here = 0;
	for(n = 0,m = 0;n < dz->brksize[kk];n++,m+=2) {
		time = dz->brk[kk][m];
		if((diff = fabs(time - thistime)) < mindiff) {
			mindiff = diff;
			here = m;
		}
		n++;
	}
	time = dz->brk[kk][here];
	val  = dz->brk[kk][here+1];
	if(val <= 0.0) {	/* no pitch */
		m = here;
		m -= 2;
		while(m >= 0) {
			if((dnval = dz->brk[kk][m+1]) > 0.0) {
				dntimediff = time - dz->brk[kk][m];
				break;
			}
			m -= 2;

		}
		m = here;
		m += 2;
		while(m < (dz->brksize[kk] * 2)) {
			if((upval = dz->brk[kk][m+1]) > 0.0) {
				uptimediff = dz->brk[kk][m] - time;
				break;
			}
			m += 2;
		}
		if(dntimediff < 0.0)
			val = upval;
		else if(uptimediff < 0.0)
			val = dnval; 
		else if(uptimediff < dntimediff)
			val = upval;
		else
			val = dnval; 
	}
	wsize = (int)round((double)dz->infile->srate/val);
	return wsize;
}

/**************************** NEXT_ZERO_CROSS *****************************/

int next_zero_cross(int here,dataptr dz)
{
	float *buf = dz->sampbuf[0];
	float val = buf[here];
	while((val = buf[here]) >= 0.0)
		here++;
	return here;
}

/**************************** PREVIOUS_ZERO_CROSS *****************************/

int previous_zero_cross(int here,int firstzero,dataptr dz)
{
	float *buf = dz->sampbuf[0];
	float val;
	while((val = buf[here]) < 0.0) {
		here--;
		if(here <= firstzero)
			return firstzero;
	}
	here++;
	return here;
}

/*************************** FIND_MAX_NOPITCH_STRETCH **************************/

int find_max_nopitch_stretch(dataptr dz)
{
	int n, m, seccnt;
	double start = -1.0, end = -1.0, maxblank = 0.0, thisblank;
	for(n=0,m=1;n<dz->brksize[0];n++,m+=2) {
		if(start >= 0.0) {
			if(dz->brk[0][m] < 0.0)
				end = dz->brk[0][m-1];
			else {
				if(end > 0.0) {
					thisblank = end - start;
					if(thisblank > maxblank)
						maxblank = thisblank;
				}
				start = -1.0;
				end = -1.0;
			}
		} else if(dz->brk[0][m] < 0.0)
			start = dz->brk[0][m-1];
	}
	n = (int)ceil((maxblank + 200) * (double)dz->infile->srate);
	seccnt = n/SECSIZE;
	if(seccnt * SECSIZE < n)
		seccnt++;
	n = seccnt * SECSIZE;
	return n;
}

/******************************** GET_ENVELOPE_TROUGHS ********************************/

int get_envelope_troughs(int *trofpnt,int *trofpntcnt,int envcnt,dataptr dz)
{
	int n;
	int lasttrofpntcnt = 0, realtrofpntcnt = 0, reallasttrofpntcnt = 0;
	float lastenval;
	int zerotrof;
	trofpnt[0] = 0;
	n = 1;						/* time-val pairs : vals are odd numbers */
	lastenval = dz->env[n];
	if(lastenval <= 0.0) {
		trofpnt[0] = 0;			/* mark start of zero trough */
		while(dz->env[n] <= 0.0) {
			n+= 2;
			if(n >= envcnt) {
				sprintf(errstr,"NO PEAKS FOUND IN ENVELOPE\n");
				return(GOAL_FAILED);
			}
		}
		n -= 2;
		if(n > 1)  {			/* if zero trough persists, also mark its end */
			trofpnt[1] = n-1;	/* times are even numbers : n is odd */
			lastenval = dz->env[n];
			*trofpntcnt = 1;
			lasttrofpntcnt = 1;
		}
		n += 2;
	} else {
		n += 2;
		while(n < envcnt) {			/* GET FIRST ENVELOPE TROUGH */
			if(dz->env[n] > lastenval) {
				trofpnt[0] = 0;
				lastenval = dz->env[n];
				n+=2;
				break;
			} else if (dz->env[n] < lastenval) {
				trofpnt[0] = n-1;	/* times are even numbers : n is odd */
				lastenval = dz->env[n];
				n+=2;
				break;
			}
			lastenval = dz->env[n];
			n+=2;
		}
	}
	if(n >= envcnt) {
		sprintf(errstr,"NO PEAKS FOUND IN ENVELOPE\n");
		return(GOAL_FAILED);
	}
	zerotrof = 0;
	while(n < envcnt) {			/* GET ENVELOPE TROUGHS */
		if(dz->env[n] > lastenval) {
			if(zerotrof) {						/* mark and count end of a zero section */
				*trofpntcnt = lasttrofpntcnt + 1;
				trofpnt[*trofpntcnt] = n-3;		/* times are even numbers : n is odd; previous time is a zeroval */
				lasttrofpntcnt = *trofpntcnt;	/* count end-of-a-zero-section: but don't count as a real (new) trof */
				zerotrof = 0;				
			}
			*trofpntcnt = lasttrofpntcnt + 1;
			realtrofpntcnt = reallasttrofpntcnt + 1;
		} else if (dz->env[n] < lastenval) {	/* can't be in a zero block, as zero is min val */
			trofpnt[*trofpntcnt] = n-1;			/* times are even numbers : n is odd */
			lasttrofpntcnt = *trofpntcnt;
			reallasttrofpntcnt = realtrofpntcnt;
		} else if(dz->env[n] <= 0.0) {			/* dz->env[n] == previous value SO zero value is continued */
			zerotrof = 1;
		}
		lastenval = dz->env[n];
		n+=2;
	}
	if(realtrofpntcnt < 2) {
		sprintf(errstr,"NO SIGNIFICANT PEAKS FOUND IN ENVELOPE\n");
		return(GOAL_FAILED);
	}
	return(FINISHED);
}

/******************************** CHOP_ZERO_SIGNAL_AREAS_INTO_COMPATIBLE_UNITS ********************************/

int chop_zero_signal_areas_into_compatible_units(int *cutcnt,int *cuttime,int cutstart,int kk,dataptr dz)
{
	int exit_status, iszero;
	int n, m, k, seglen, zeromax, zerostart=0, maxzerostart, maxzeroend = 0, thispitch, bufstart;
	int zerolen, segcnt;
	double time, dseglen, incr;
	float *buf = dz->sampbuf[0];
	time = 0.0;
	for(n=cutstart+1;n < *cutcnt;n++) {
		time = (double)cuttime[n-1]/(double)dz->infile->srate;
		if((exit_status = read_value_from_brktable(time,kk,dz))<0)
			return(exit_status);
		if(dz->param[kk] < 0.0) {
			if((exit_status = get_time_nearest_true_pitch(time,&(dz->param[kk]),kk,dz))<0)
				return(exit_status);
		}
		thispitch = (int)round((double)dz->infile->srate/dz->param[kk]);
		seglen = cuttime[n] - cuttime[n-1];
		if(seglen > thispitch * 2) {
			iszero = 0;
			zeromax = 0;
            if((sndseekEx(dz->ifd[kk],cuttime[n-1],0)<0)){        
                sprintf(errstr,"sndseek() failed\n");
                return SYSTEM_ERROR;
            }
			if((dz->ssampsread = fgetfbufEx(dz->sampbuf[0], dz->buflen,dz->ifd[kk],0)) < 0) {
				sprintf(errstr,"Can't read samples from input soundfile %d\n",kk+1);
				return(SYSTEM_ERROR);
			}
			dz->total_samps_read += dz->ssampsread;
			dz->samps_left -= dz->ssampsread;
			bufstart = cuttime[n-1];
			maxzerostart = -1;
			for(m = 0;m < seglen; m++) {
				if(buf[m] == 0.0) {
					if(!iszero) {
						zerostart = m;
						iszero = 1;
					}
				} else {
					if(iszero) {
						if((m - zerostart) > zeromax) {
							zeromax = m - zerostart;
							maxzerostart = zerostart;
							maxzeroend   = m;
						}
						iszero = 0;
					}
				}	
			}
			if(maxzerostart >= 0) {
				while(buf[maxzeroend] >= 0.0)
					maxzeroend++;
				if(maxzeroend > seglen || maxzeroend > dz->ssampsread)	/* can't find appropriate down_zcross within segment */
					continue;
				if(maxzerostart <= thispitch)
					maxzerostart = thispitch;
				if(maxzeroend < ALMOST_OCT * thispitch)					/* insufficient zeros to divide the seg */
					continue;
				for(k = *cutcnt;k >= n;k--)								/* insert first extra point */
					cuttime[k+1] = cuttime[k];
				cuttime[n] = maxzerostart + bufstart; 
				(*cutcnt)++;
				n++;
				zerolen = maxzeroend - maxzerostart;					/* if enough space for further cuts, make them */
				if((segcnt = (int)round((double)zerolen/(double)thispitch)) > 0) {
					dseglen = (double)zerolen/(double)segcnt;
					incr = dseglen;
					zerostart = maxzerostart;
					maxzerostart = zerostart + (int)floor(dseglen);
					while(maxzerostart < (maxzeroend - 1)) {	/* SAFETY */
						for(k = *cutcnt;k >= n;k--)
							cuttime[k+1] = cuttime[k];
						cuttime[n] = maxzerostart + bufstart; 
						(*cutcnt)++;
						n++;
						dseglen += incr;
						maxzerostart = zerostart + (int)floor(dseglen);
					}
				}
			}
		}
	}
	return(FINISHED);
}

/******************************** GET_TIME_NEAREST_TRUE_PITCH ********************************/

int get_time_nearest_true_pitch(double time,double *pitch,int kk,dataptr dz)
{	
	int n, no_up = 0, no_dn = 0, start;
	double uptime=0.0, dntime, uppitch=0.0, dnpitch;
	double *thisbrk = dz->brk[kk];

	for(n=0;n<dz->brksize[kk] * 2;n+=2) {
		if(thisbrk[n] >= time)
			break;
	}
	if(n >= dz->brksize[kk] * 2) {
		n--;
		while(thisbrk[n] < 0.0)
			n -= 2;
		if(n<0) {
			sprintf(errstr,"Pitch not found in pitch breakpoint file %d.\n",kk+1);
			return(DATA_ERROR);
		}
		*pitch = thisbrk[n];
		return FINISHED;
	}
	n++;
	start = n;
	while(thisbrk[n] < 0.0) {
		n+= 2;
		if(n >= dz->brksize[kk] * 2) {
			no_up = 1;
			break;
		}
	}
	if(!no_up) {
		uppitch = thisbrk[n];
		uptime  = thisbrk[n-1];
	}
	n = start;
	while(thisbrk[n] < 0.0) {
		n-= 2;
		if(n < 0) {
			no_dn = 1;
			break;
		}
	}
	if(no_up && no_dn) {
		sprintf(errstr,"Pitch not found in pitch breakpoint file %d.\n",kk+1);
		return(DATA_ERROR);
	}
	if(no_dn)
		*pitch = uppitch;
	else {
		dnpitch = thisbrk[n];
		dntime  = thisbrk[n-1];
		if(no_up)
			*pitch = dnpitch;
		else if((uptime - time) < (time - dntime))
			*pitch = uppitch;
		else
			*pitch = dnpitch;
	}
	return FINISHED;
}

/******************************** SMOOTH_BAD_GRAINS ********************************/

int smooth_bad_grains(int seglen,int bufno, dataptr dz)
{
	int dosmooth;
	int k, startend;
	float *buf = dz->sampbuf[bufno];
	double incr, gain, diff;
	int smoothlen = SMOOTHWIN;
	if(seglen < SMOOTHWIN * 2)
		smoothlen = seglen/2;
	dosmooth = 0;
	diff = fabs(buf[1] - buf[0]);
	for(k = 0;k < smoothlen - 1;k++) {
			/* look for sudden gradient changes relative to size of sample */
		if((diff > 0.001) && (fabs(buf[k]) < (diff * 4.0))) {
			dosmooth = 1;
			break;
		}
		diff = fabs(buf[k+1] - buf[k]);
	}
	if(dosmooth) {
		gain = 1.0;
		incr = 1.0/(double)smoothlen;
		for(k = smoothlen - 1;k >= 0;k--) {
			gain -= incr;
			buf[k] = (float)(buf[k] * gain);
		}
	}
	startend = seglen-smoothlen;
	dosmooth = 0;
	diff = fabs(buf[startend] - buf[startend + 1]);
	for(k = startend;k < seglen-2;k++) {
			/* look for sudden gradient changes relative to size of sample */
		if((diff > 0.001) && (fabs(buf[k+1]) < (diff * 4.0))) {
			dosmooth = 1;
			break;
		}
		diff = fabs(buf[k] - buf[k+1]);
	}
	if(dosmooth) {
		gain = 1.0;
		incr = 1.0/(double)smoothlen;
		for(k = startend;k < seglen;k++) {
			gain -= incr;
			buf[k] = (float)(buf[k] * gain);
		}
	}
	return FINISHED;
}

/******************************** COUNT_ZEROCROSSINGS ********************************/

int count_zerocrossings(int here,int there,float *buf)
{
	int done = 0, isup = 0;
	int zc_cnt = 0;
	float val = buf[here];
	while(here < there) {
		while(flteq(val,0.0)) {	/* only accessed if segment starts with zeros */
			here++;
			if(here >= there)  {
				done = 1;
				break;
			}
			val = buf[here];
		}
		if(done)
			break;
		while(val >= 0.0) {
			if(isup != 1) {
				zc_cnt++;
				isup = 1;
			}
			here++;
			if(here >= there) {
				done = 1;
				break;
			}
			val = buf[here];
		}
		if(done)
			break;
		while(val <= 0.0) {
			if(isup != -1) {
				zc_cnt++;
				isup = -1;
			}
			here++;
			if(here >= there) {
				done = 1;
				break;
			}
			val = buf[here];
		}
	}
	return zc_cnt;
}

/******************************** TRIANGULATE_ENV ********************************
 *
 * Segment divided into 4 quarters, to define 3 overlapping areas...
 *
 *			 1st quarter  2nd quarter  3rd quarter  4th quarter
 *			 ___________  ___________  ___________  ___________
 *			|			||			 ||			  ||		   |
 *
 *			|______Env0______________||_____Env2_______________|
 *
 *						 |_____Env1_______________|
 *
 *		Find area with minimum energy.
 */

int triangulate_env(int *here,int *there,int ideal_place,float *buf)
{
	int localhere = *here, localthere = *there;
	int seglen, this_zc_cnt[3], max_zc_cnt;
	int quartlen, quart[5], n, m;
	int  oversize, use_1st_quart = 0, use_last_quart = 0;
	double qsum[4], esum[3];
	seglen = localthere - localhere;
	quartlen = seglen/4;
	oversize = seglen - (quartlen * 4);
	if(*here > ideal_place)
		use_1st_quart = 1;
	if(*there < ideal_place)
		use_last_quart =1;
	if(oversize) {			/* func only called if >=5 zero crossings: so >= 9 samples: so 'there' never <= 'here' */
		localhere++;			/* truncate examined area by 1 sample */
		if(oversize > 1)
			localthere--;		/* truncate examined area by 1 more sample */
		if(oversize > 2)
			localhere++;		/* truncate examined area by 1 more sample */
	}
	quart[0] = localhere;		/* divide segment into 4 equal parts */
	for(n = 0; n < 4; n++)
		quart[n+1] = quart[n] + quartlen;
	if(use_1st_quart)
		max_zc_cnt = count_zerocrossings(quart[0],quart[2],buf);
	else if(use_last_quart)
		max_zc_cnt = count_zerocrossings(quart[2],quart[4],buf);
	else {
		max_zc_cnt = -1;
		for(n = 0; n < 3; n++) {	/* Find zcross count in each of 3 overalpping Envs */
			this_zc_cnt[n] = count_zerocrossings(quart[n],quart[n+2],buf);
			if(this_zc_cnt[n] > max_zc_cnt)
				max_zc_cnt = this_zc_cnt[n];
		}
	}
	if(max_zc_cnt < 5)			/* If none of the Env-segs has >= 5 zcrossings, return withot doing anything */
		return max_zc_cnt;		/* causing calling loop to finish */
	if(use_1st_quart) {
		*here  = quart[0];
		*there = quart[2];
		return max_zc_cnt;
	} else if(use_last_quart) {
		*here  = quart[2];
		*there = quart[4];
		return max_zc_cnt;
	}
	for(n = 0; n < 4; n++) {	/* Sum the abs-samples in each Quarter */
		qsum[n] = 0.0;
		for(m = quart[n];m <quart[n+1];m++)
			qsum[n] += fabs(buf[m]);
	}

	for(n = 0; n < 3; n++)		/* Sum the abs-samples in each Env */
		esum[n] = qsum[n] + qsum[n+1];

	if((flteq(esum[0],esum[1])) && (flteq(esum[0],esum[2]))) {	/* esums are all equal */
		if(this_zc_cnt[1] >= 5) {			/* return middle seg limits, unless too few zcrossings */
			*here  = quart[1];
			*there = quart[3];
			return this_zc_cnt[1];
		} else if(this_zc_cnt[0] >= 5) {	/* else return start seg limits, unless too few zcrossings */
			*here  = quart[0];
			*there = quart[2];
			return this_zc_cnt[0];
		} else {							/* else return end seg limits */
			*here  = quart[2];
			*there = quart[4];
			return this_zc_cnt[2];
		}
	} else if((esum[0] <= esum[1]) && (esum[0] <= esum[2])) {	/* esum[0] in Env0 has minimum energy */
		if(this_zc_cnt[0] >= 5) {	/* If Env0 has > 5 zcrossings */
			*here  = quart[0];		/* define this as the new working segment in which to find min energy */				
			*there = quart[2];		/* Else return without doing anything */
		}							/* (Calling process then drops out & uses segment we had at start of this func) */
		return this_zc_cnt[0];
	} else if((esum[1] <= esum[0]) && (esum[1] <= esum[2])) {	/* esum[1] in Env1 has minimum energy */
		if(this_zc_cnt[1] >= 5) {
			*here  = quart[1];
			*there = quart[3];
		}
		return this_zc_cnt[1];
	} /* else */												/* esum[2] in Env2 has minimum energy */
	if(this_zc_cnt[2] >= 5) {
		*here  = quart[2];
		*there = quart[4];
	}
	return this_zc_cnt[2];
}

/******************************** MARK_CUT ********************************/

int mark_cut(int *cutcnt,int *cut,int localpeakcnt,double *startarray,int here,int there,
			 int startsamp,int first_downcross,double starttime,int msg,dataptr dz)
{
	double localpeak1, localpeak2, localpeak3, maxlocalpeak, minenergy, energy;
	int localpeakat1, localpeakat2 , at = 0, k, m, thissampinbuf;
	int up;
	double *localpeak = startarray;
	switch(localpeakcnt) {
	case(0):
		if(msg)
			sprintf(errstr,"FAILED TO LOCATE CUT POINT NEAR %lf secs: NO LOCAL PEAK FOUND: IMPOSSIBLE!!\n",starttime);
		return(PROGRAM_ERROR);
	case(1):										/* SINGLE PEAK */
		if(*localpeak > 0.0)						/* +ve peak, downward-cross is at end */
			cut[*cutcnt] = there + startsamp;		/* return location of zero crossing after peak */
		else
			cut[*cutcnt] = here + startsamp;		/* -ve peak, zerocross at start, return location zerocros before peak */
		break;
	case(2):										/* TWO PEAKS */
		if(*localpeak > 0.0) {						/* FIRST PEAK IS ABOVE ZERO: find downcross after this */
			here = (int)round(*(localpeak+1));		/* location of 1st local peak */
			here -= startsamp;						/* location in buf */
			here =  next_zero_cross(here,dz);		/* find following down-zerocross */
			cut[*cutcnt] =  here + startsamp;		/* return its absolute location */
		} else {									/* FIRST PEAK IS BELOW ZERO */
			if(fabs(*localpeak) < *(localpeak+2))	/* initial peak is lower energy */
				cut[*cutcnt] = here + startsamp;		/* return absolute-position of its start */
			cut[*cutcnt] = there + startsamp;		/*final peak is lower energy, return absolute-pos of its end */
		}
		break;
	case(3):
		localpeak1 = *localpeak++;					/* THREE PEAKS */
		localpeakat1 = (int)round(*localpeak++);	/* Find location of (abs) max of the 3 */
		maxlocalpeak = *localpeak;
		at = 1;
		localpeak2 = *localpeak++;
		localpeakat2 = (int)round(*localpeak++);
		if(localpeak2 > maxlocalpeak) {
			maxlocalpeak = localpeak2;
			at = 2;
		}
		localpeak3 = *localpeak++;
		if(localpeak3 > maxlocalpeak) {
			maxlocalpeak = localpeak3;
			at = 3;
		}
		up = 0;										/* Is first peak +ve or -ve */
		if(localpeak1 > 0.0)
			up = 1;
		switch(at) {		/* Select on basis of where max peak is */
		case(1):			/* 1st peak is max */
			if(up) {
				cut[*cutcnt] = there + startsamp;				/* +ve cycle, return end of last peak */
			} else {
				here = localpeakat2 - startsamp;
				here = next_zero_cross(here,dz);				/* -ve cycle, return zero after peak2 */
				cut[*cutcnt] = here + startsamp;
			}
			break;
		case(2):			/* 2nd peak is max */
			if(localpeak1 > localpeak3) {				/* 3rd peak is minimum */
				if(up) {
					cut[*cutcnt] = there + startsamp;			/* +ve cycle, return end of peak3 */
				} else {
					here = localpeakat2 - startsamp;
					here = next_zero_cross(here,dz);			/* -ve cycle, return start of peak3 */
					cut[*cutcnt] = here + startsamp;
				}
			} else {									/* 1st peak is minimum */
				if(up) {
					here = localpeakat1 - startsamp;
					here = next_zero_cross(here,dz);			/* +ve cycle, return end of peak1 */
					cut[*cutcnt] = here + startsamp;					
				} else {
					cut[*cutcnt] = here + startsamp;				/* -ve cycle, return start of peak1 */
				}
			}
			break;
		case(3):			/* 3rd peak is max */
			if(up) {
				here = localpeakat1 - startsamp;
				here = next_zero_cross(here,dz);				/* +ve cycle, return end of peak1 */
				cut[*cutcnt] = here + startsamp;
			} else {
				cut[*cutcnt] = here + startsamp;					/* -ve cycle, return start of peak1 */
			}
			break;
		}
		break;
	default:						/* MORE THAN 3 PEAKS */
		localpeak = startarray;
		minenergy = HUGE;										/* find minimum energy of each PAIR of peaks */
		for(k = 0,m = 2; m < localpeakcnt * 2; k+=2,m+=2) {
			energy = fabs(localpeak[k]) + fabs(localpeak[m]);
			if(energy < minenergy) {
				minenergy = energy;
				at = k;
			}
		}
		thissampinbuf  = (int)round(localpeak[at+1]);
		thissampinbuf -= startsamp;
		if(localpeak[at] > 0.0)	{								/* if min energy is at a pair that starts +ve */
			thissampinbuf = next_zero_cross(thissampinbuf,dz);
			cut[*cutcnt]   = thissampinbuf + startsamp;			/* return end of that pair's 1st peak */
		} else {
			thissampinbuf = previous_zero_cross(thissampinbuf,first_downcross,dz);
			cut[*cutcnt]   = thissampinbuf + startsamp;			/* else return start of that pair's 1st peak */
		}
	}
	return(FINISHED);
}

/******************************** FIND_THE_LOCAL_PEAKS ********************************/

int find_the_local_peaks(int *here,int *there,float *buf,int *n,int trofpntcnt,int *trofpnt,
		 int *startsamp,int *endsamp,int losamp, int *cut, int cutcnt, double *localpeak, double *scanarray, 
		 int *localpeakcnt,int *first_downcross,int kk,dataptr dz) 
{
	int notfound = 0, finished = 0, below, bufinc = 0;
	int maxat=0;
	float maxval=0.0, val;
	int wsize, hisamp,losampinbuf,hisampinbuf;
	int orighere, origthere;

	wsize = *there - *here;
	while(!finished) {
		hisamp = min(dz->insams[kk],losamp + wsize);		/* get end-of-search area from  wtime & half-wsize */
		if(hisamp >= dz->insams[kk])
			hisamp = dz->insams[kk];
		losampinbuf = losamp - *startsamp;					/* get search ends relative to buffer */
		hisampinbuf = hisamp - *startsamp;
		if(hisampinbuf >= dz->buflen) {
            if((sndseekEx(dz->ifd[kk],losamp,0)<0)){        
                sprintf(errstr,"sndseek() failed\n");
                return SYSTEM_ERROR;
            }
			dz->total_samps_read -= dz->ssampsread;
			dz->total_samps_read += losamp;
			dz->samps_left = dz->insams[kk] - dz->total_samps_read;
			if((dz->ssampsread = fgetfbufEx(dz->sampbuf[0], dz->buflen,dz->ifd[kk],0)) < 0) {
				sprintf(errstr,"Can't read samples from input soundfile %d\n",kk+1);
				return(SYSTEM_ERROR);
			}
			dz->total_samps_read += dz->ssampsread;
			dz->samps_left -= dz->ssampsread;
			*endsamp = dz->total_samps_read;
			*startsamp = *endsamp - dz->ssampsread;
			losampinbuf = losamp - *startsamp;					/* get search ends relative to buffer */
			hisampinbuf = hisamp - *startsamp;
			bufinc = 0;
		}
		*here = losampinbuf;
		/* IGNORE EDGES OF WINDOW before the first, and after the last, zero crossing */
					/* MOVE START SEARCH TO FIRST ZERO CROSSING */
		if(!bufinc) {
			val = buf[*here];
			*first_downcross = -1;
			if(val >= 0.0) {
				while(val >= 0.0) {
					(*here)++;
					if(*here >= hisampinbuf) {
						notfound = 1;							/* no zero-cross found */
						break;
					}
					val = buf[*here];
				}												
				if(notfound) {
					wsize += 20;								/* no zero-cross found, return to loop head */
					bufinc = 1;
					continue;									/* and move end of search window */
				}
			// VAL is just after zero cross downwards
				*first_downcross = *here;							/* mark 1st down-zerocross in search area */
			} else if (val <= 0.0) {
				while(val <= 0.0) {
					(*here)++;
					if(*here >= hisampinbuf) {
						notfound = 1;							/* no zero-cross found */
						break;
					}
					val = buf[*here];
				}
				if(notfound) {
					wsize += 20;								/* no zero-cross found, return to loop head */
					bufinc = 1;
					continue;	
				}
				// VAL is just after zero cross upwards
			}
		}
					/* MOVE END OF SEARCH TO LAST ZERO CROSSING */
		*there = hisampinbuf - 1;
		val = buf[*there];
		if(val >= 0.0) {
			below = 0;
			while(val >= 0.0) {
				(*there)--;
				if(*there < losampinbuf) {
					break;
				}
				val = buf[*there];
			}
		} else if(val <= 0.0) {
			below = 1;
			while(val <= 0.0) {
				(*there)--;
				if(*there < losampinbuf) {
					break;
				}
				val = buf[*there];
			}
		}
		if(*there <= *here) {								/* There is ONLY ONE zero-crossing found */
			if(buf[*here] >= 0.0) {							/* This is an upward-crossing zero, NOT a downward crossing zero */
				notfound = 1;								/* no DOWN-zero-cross found, return to loop head */
				wsize += 20;
				bufinc = 1;
				continue;									/* and move end of search window */
			}
			cut[cutcnt] = *here + *startsamp;				/* return the single downward-crossing zero */
			*first_downcross = *here;
			return FINISHED;
		}													/* ELSE more than one zero-cross found */
															/* therefore at least one intervening (+ or -) peak  exists */
		(*there)++;											/* move end search to the value AFTER the last zero-crossing */
		finished = 1;										/* break from search loop */
	}
	orighere = *here;
	origthere = *there;
				/* STORE PEAKS BETWEEN ZERO-CROSSINGS */
	val = buf[*here];
	while(*here < *there) {
		if(val >= 0.0) {
			maxval = val;
			maxat = *here;
			while(val >= 0.0) {
				(*here)++;
				if(*here >= *there)
					break;
				val = buf[*here];
				if(val > maxval) {
					maxval = val;
					maxat  = *here;
				}
			}
		} else if(val <= 0.0) {
			maxval = val;
			maxat = *here;
			while(val <= 0.0) {
				(*here)++;
				if(*here >= *there)
					break;
				val = buf[*here];
				if(val < maxval) {
					maxval = val;
					maxat  = *here;
				}
			}
		}
		if((maxval > 0.0) && (*first_downcross < 0))
			*first_downcross = *here;
		*localpeak++ = (double)maxval;
		*localpeak++ = (double)(maxat + *startsamp);
	}	
	*localpeakcnt = (localpeak - scanarray)/2;
	if(*localpeakcnt == 1) {			/* If only 1 peak, the cut points are at its start or end */
		*here = orighere;
		*there = origthere;
	}
	 return CONTINUE;
}

/***************************** SMOOTH_CUTS *****************************/

int smooth_cuts(int *cut,int *cutcnt,int kk,int cutstart,dataptr dz)
{
	int exit_status;
	int n, k, seg0, seg1, seg2, minseg, end, start, *thiscut, srchlen, minlen;
	int pitchseglim, pitchseglo=0, realend, lastcutval;
	double pitchseg, lastpitchseg=0.0, time, big_seg, this_seg, last_seg, big_int, this_int, last_int;
	
    if((sndseekEx(dz->ifd[kk],0,0) < 0)){        
        sprintf(errstr,"sndseek() failed\n");
        return SYSTEM_ERROR;
    }
	dz->samps_left = dz->insams[kk];
	dz->total_samps_read = 0;

	if((dz->ssampsread = fgetfbufEx(dz->sampbuf[0], dz->buflen,dz->ifd[kk],0)) < 0) {
		sprintf(errstr,"Can't read samples from input soundfile %d\n",kk+1);
		return(SYSTEM_ERROR);
	}
	dz->total_samps_read += dz->ssampsread;
	dz->samps_left -= dz->ssampsread;
	for(n=3 + cutstart;n<*cutcnt;n++) {
		lastcutval = 0;
		end     = cut[n];
		realend = end;
		time = (double)cut[n-3]/(double)dz->infile->srate;
		if((exit_status = read_value_from_brktable(time,kk,dz))<0)
			return(exit_status);									/* Get pitch at start of cut segment */
		if(dz->param[kk] > 0.0) {									/* if it's a true pitch ... */
			pitchseg = (double)dz->infile->srate/dz->param[kk];		/* Find corresponding segment length */
			pitchseglim = (int)round(pitchseg * ALMOST_OCT);		/* Go to almost an 8va up */
			pitchseglo  = (int)round(pitchseg/ALMOST_OCT);			/* Go to almost an 8va down */
			pitchseglim += cut[n-3];								/* Prevent autocorrelation-search going beyond 8va */
			if(end > pitchseglim) {									
				end = pitchseglim;									/* Typically, end of search is ~2* expected pitchseg len */
				lastcutval = cut[n-2];								/* & we save origval in case new val is to be INTERPOLATED. */
			}														/* But if 3segs is smaller, set endofsearch to  endof 3segs */
		} else
			pitchseg = -1.0;
		start   = cut[n-3];
		thiscut = &(cut[n-2]);
		seg0	= cut[n-2] - cut[n-3];
		seg1	= cut[n-1] - cut[n-2];
		seg2	= cut[n]   - cut[n-1];
		minseg = min(seg0,seg1);
		minseg = min(minseg,seg2);
		srchlen = cut[n]   - cut[n-3];
		if((minlen = minseg/2) <= 0)			/* if previous segs tiny, e.g. just 1 seg, this val gets 0 */
			minlen = min(srchlen/6,pitchseglo);	/* use either ~8va below pitch, or half the average of 3seglen */
		if((exit_status = auto_correlate(start,thiscut,end,realend,minlen,pitchseg,kk,dz))<0)
			return(exit_status);
		if(cut[n-2] >= cut[n-1]) {		/* If next cut is now too near to new cut ... */
			for(k = n;k<*cutcnt;k++)	/* e.g. 0 20 30 40 --> 0 20 40 40 .... omit duplicate  */
				cut[k-1] = cut[k];		/* OR   0 20 30 40 --> 0 20 50 40 .... omit skipped val */
			(*cutcnt)--;
			n--;
		} else if(lastcutval > 0) {		/* If new cut will fit between previous, and original value here */
										/*  e.g. originally 20 [200] 220 240 --> 20 40 220 240 (200 stored as lastcutval) */
			if(lastcutval - cut[n-2] > pitchseglo) {
				for(k = *cutcnt;k>=n-1;k--)	/* then reinsert orig val .... */
					cut[k+1] = cut[k];		/*  e.g. 20 40 220 240  --> 20 40 200 220 240 */
				cut[n-1] = lastcutval;
				(*cutcnt)++;
			}
		}								/* Eliminate segs that are 8va up (twice too short) */
		if((n > cutstart+3) && (lastpitchseg > 0.0) && (pitchseg > 0.0)) {
			pitchseg = (pitchseg + lastpitchseg)/2.0;
			this_seg = (double)(cut[n-2] - cut[n-3]);
			if((big_seg   = (double)(cut[n-2] - cut[n-4])) > pitchseg)
				big_int = big_seg/pitchseg;
			else
				big_int = pitchseg/big_seg;
			if(this_seg > pitchseg)
				this_int = this_seg/pitchseg;
			else
				this_int = pitchseg/this_seg;
			if((last_seg = (double)(cut[n-3] - cut[n-4])) > pitchseg)
				last_int = last_seg/pitchseg;
			else
				last_int = pitchseg/last_seg;
			if((big_int < this_int) && (big_int < last_int)) {
				for(k = n-2;k < *cutcnt;k++)
					cut[k-1] = cut[k];
				(*cutcnt)--;
				n--;
			}
		}
		lastpitchseg = pitchseg;
	}
	return FINISHED;
}

/******************************** AUTO_CORRELATE ********************************
 *                 *at
 *               (cut to			   search	 realend
 *     start      adjust)               end		 of 3segs
 * 		 ___________ ______ _____________i___________
 *		|			|	   |			 i			|
 *  									 i
 *
 *
 *
 *	start    = start of the segment.
 *  end      = end of the search area for correlation between segs.
 *  *at      = starts out as the original cut position, and may be changed by this process.
 *  realend  = end of the group of 3 segs: if no cutsite found before 'end', extend search towards 'realend'
 *            (cutsites are always at downward-crossing zeros)
 *  minlen   = minimum length of adjacent stretches of sound to compare
 *	pitchseg = length of seg corresponding to pitch (if any) at this place.
 *             if the new cutpoint is further from the pitchseg val than the original cut (*at)
 *			   the search can be restricted to a smaller range then defiend by 'end'.
 */
int auto_correlate(int start,int *at,int end,int realend,int minlen,double pitchseg,int kk,dataptr dz)
{
	float *buf = dz->sampbuf[0];
	int startbuf = 0, startinbuf, atinbuf, orig_atinbuf, maxinbuf, n, m, oldlen, newlen;
	int newlen2=0, atinbuf2, orig_maxinbuf;
	double max_auto, max_auto2, thisauto, time;
	int finished, unpitched = 0, extended = 0;
	if(end > dz->total_samps_read) {
        if((sndseekEx(dz->ifd[kk],start,0)<0)){        
            sprintf(errstr,"sndseek() failed\n");
            return SYSTEM_ERROR;
        }
		dz->total_samps_read = start;
		dz->samps_left       = dz->insams[kk] - start;
		if((dz->ssampsread = fgetfbufEx(dz->sampbuf[0], dz->buflen,dz->ifd[kk],0)) < 0) {
			sprintf(errstr,"Can't read samples from input soundfile %d\n",kk+1);
			return(SYSTEM_ERROR);
		}
		dz->total_samps_read += dz->ssampsread;
		dz->samps_left -= dz->ssampsread;
	}
//NEW
	startbuf = dz->total_samps_read - dz->ssampsread;
	startinbuf = start - startbuf; 
	atinbuf    = *at   - startbuf;
	maxinbuf   = end   - startbuf;
	orig_atinbuf  = atinbuf;
	orig_maxinbuf = maxinbuf;
	finished = 0;
//	oldlen = *at - startinbuf;
	oldlen = atinbuf - startinbuf;
	do {
		while(!finished) {
			atinbuf2 = -1;
			n = startinbuf;
			m = startinbuf + minlen;
			m = next_down_zcross(m,maxinbuf,buf);	/* starting with a segment which ends at first down_zcross after minlen */
			if(m > maxinbuf || m < 0)
				break;
			max_auto = autocorrelate(n,m,buf);		/* autocorrelate two minsize segments */
			max_auto2 = max_auto;
			atinbuf = m;
			for(;;) {								/* gradually expand size to next (etc) down_zcross, & autocorrelate */
				if((m = next_down_zcross(m,maxinbuf,buf)) < 0)
					break;
				if((thisauto = autocorrelate(n,m,buf)) > max_auto) {
					max_auto = thisauto;
					atinbuf = m;
				} else if(thisauto > max_auto2) {
					max_auto2 = thisauto;
					atinbuf2 = m;					/* Save best and 2nd best lengths (tested by correlation) */
				}
			}		
			if(pitchseg < 0.0) {					/* If an unpitched seg, use closest pitch to set arbitrary 'ideal' seg size */
				unpitched = 1;
				time = (double)(*at)/(double)dz->infile->srate;
				pitchseg = (double)read_validpitch_wsize_in_samps_from_brktable(time,kk,dz);
			}
			newlen = atinbuf - startinbuf;
			if(atinbuf2 >= 0)
				newlen2 = atinbuf2 - startinbuf;
			*at = atinbuf + startbuf;	/* Compare best-correlated  cuts, with existing cut */
			if(fabs((double)newlen - pitchseg) <= fabs((double)oldlen - pitchseg)) {
				if(atinbuf2 >= 0) {		/* If 2nd best correlation point exists & is closer to pitch than best, use it */
					if(fabs((double)newlen2 - pitchseg) < fabs((double)newlen - pitchseg))
						*at = atinbuf2 + startbuf;	
				}						/* Else, If best new value is closer to pitch than original: use it */
				return FINISHED;
										/* If best cut(s) are worse than existing cut, */	
			} else if(extended) {		/* if already EXTENDED the search, no point in looking at smaller range */
				*at = orig_atinbuf + startbuf;	/* Reset, and break from inner ('while') loop */
				break;					
			} else {					/* Otherwise, set end of search to the down_zcross PRIOR to the one found */
				maxinbuf = last_down_zcross(maxinbuf,startinbuf,buf);
				if(maxinbuf <= 0) {		/* If can't get any smaller, break from inner 'while' loop */
					*at = orig_atinbuf + startbuf;
					break;
				}						/* Otherwise continue search in 'while' loop */
			}		
			if(unpitched)
				pitchseg = -1.0;
		}								/* If inner loop search has failed */
		if(end < realend) {				/* if the search end is less than the realend of the 3segments, EXTEND the search */	
			maxinbuf = next_down_zcross(orig_maxinbuf,dz->buflen,buf);
			if(maxinbuf < 0)
				break;
			end = maxinbuf + startbuf;
			orig_maxinbuf = maxinbuf;
			extended = 1;
		}
	} while(end < realend);
	return FINISHED;
}

/******************************** AUTOCORRELATE ********************************/

double autocorrelate(int n,int m,float *buf)
{
	int j, k;
	double sum = 0.0;
	for(j = n,k = m;j<m;j++,k++)
		sum += (buf[j] * buf[k]);
	sum = sum/(double)(m-n);
	return sum;
}

/******************************** NEXT_DOWN_ZCROSS ********************************/

int next_down_zcross(int here,int hibound,float *buf)
{	
	double val = buf[here];
	while(val <= 0.0) {
		here++;
		if(here >= hibound)
			return -1;
		val = buf[here];
	}
	while(val > 0.0) {
		here++;
		if(here >= hibound)
			return -1;
		val = buf[here];
	}
	return here;
}

/******************************** LAST_DOWN_ZCROSS ********************************/

int last_down_zcross(int here,int lobound,float *buf)
{	
	double val = buf[here];
	while(val <= 0.0) {
		here--;
		if(here < lobound)
			return -1;
		val = buf[here];
	}
	while(val >= 0.0) {
		here--;
		if(here < lobound)
			return -1;
		val = buf[here];
	}
	while(val < 0.0) {
		here--;
		if(here < lobound)
			return -1;
		val = buf[here];
	}
	here++;
	return here;
}

/************************************/
/* CLEAN UP THE OUTPUT IF NECESSARY */
/************************************/

/******************************** ELIMINATE_TOO_SHORT_EVENTS ********************************/

int eliminate_too_short_events(int *zcnt,int *final_pos,int *sigcnt,int opos,int last,float *obuf,int chans,dataptr dz)
{
	int too_long_zeros = 0, badsig_possible = 0, finished;
	int n, k, sigstart=0, sigend=0;
	int zeros = *zcnt, sigs = *sigcnt, bumzeros;
	int tooshortsig = dz->infile->srate/50;
	int toolongsil  = dz->infile->srate/25;
	if(zeros <= BUMZEROCNT * chans) {
		bumzeros = zeros;	/* bumzeros are zeros within signals */
		zeros = 0;
	} else {
		bumzeros = 0;
	}
	if(*final_pos > 0)
		*final_pos -= dz->buflen;
	for(n=*final_pos; n < opos; n++) {
		if(flteq(obuf[n],0.0)) {
			if(zeros == 0) {
				bumzeros++;
				if(bumzeros <= BUMZEROCNT * chans) {
					continue;
				} else {
					zeros = BUMZEROCNT * chans;
					bumzeros = 0;
				}
			}
			if(sigs) {
				if((sigs < tooshortsig * chans) && too_long_zeros) {
					sigend = n;
					badsig_possible = 1;
					too_long_zeros = 0;
				} else {
					badsig_possible = 0;
				}
				sigs = 0;
			}
			zeros++;
			if(!too_long_zeros) {
				if(zeros >= toolongsil * chans) {
					if(badsig_possible) {
						for(k = sigstart; k < sigend;k++)
							obuf[k] = 0;
						badsig_possible = 0;
					}
					too_long_zeros = 1;
				}
			}
		} else {
			if(bumzeros) {
				sigs += bumzeros;
				bumzeros = 0;
			}
			if(!sigs) {
				bumzeros = 0;
				badsig_possible = 0;
				sigstart = n;
				sigs = 0;
				zeros = 0;
			}
			sigs++;
		}
	}
	if(last) {			/* if end of input file : Eliminate any too short item near end */
		if(badsig_possible) {
			for(k = sigstart; k < sigend;k++)
				obuf[k] = 0;
		}
		return FINISHED;
	}

			/* PROCESS START OF SIGNAL IN CONTINUATION-BUFFER, in case badsig_possible overlaps bufend  */

	if(badsig_possible) {				/* if bad signal possibility, must be in stretch of zeros */
		while(flteq(obuf[n],0.0)) {
			zeros++;
			if(zeros >= toolongsil * chans) {	/* if post-zeros too long, zero bad sig, and break */	
				for(k = sigstart; k < sigend;k++)
					obuf[k] = 0;
				break;
			}
			n++;						/* else, once signal becomes non-zero, breaks from while */
		}
		*zcnt = zeros;		/* store final zero count, for next pass */
		*final_pos = n;
		*sigcnt = sigs;	
		return FINISHED;
	}
	finished = 0;
	while(!finished) {					
		if(flteq(obuf[n],0.0)) {
			if(sigs) {
				if((sigs < tooshortsig * chans) && too_long_zeros) {
					sigend = n;
					badsig_possible = 1;
					too_long_zeros = 0;
				} else {
					finished = 1;		/* No bad signal here after bufend, exit */
				}
				sigs = 0;
			}
			zeros++;
			if(!too_long_zeros) {
				if(zeros >= toolongsil * chans) {
					if(badsig_possible) {
						for(k = sigstart; k < sigend;k++)
							obuf[k] = 0;	/* Eliminate bad sig, and exit */
					}
					finished = 1;		/* Finished checking for badsig at bufend, exit */
				}
			}
		} else {
			if(!sigs) {
				finished = 1;			/* start of new sig section: */
			}							/* any previous badsigs eliminated in zeros block, so exit */
			sigs++;
			if(sigs >= tooshortsig * chans)		/* too much sig to be bad sig: exit */
				finished = 1;
		}
		n++;
	}
//NEW
	*zcnt = zeros;		/* store final zero count, for next pass */
	*final_pos = n;
	*sigcnt = sigs;
	return FINISHED;
}

/******************************** SMOOTH_BAD_EVENTS ********************************/

int smooth_bad_events(int *zcnt,int *final_pos,int *sig_cnt,int *stsmoothed,int opos,int last,float *buf,int chans,dataptr dz)
{
	int finished, dosmooth, start_smoothed, q;
	int n=0, k, fadelen, checklen, start, end;
	int zeros, sigs, bumzeros, startzeros, startbumzeros, startsigs;
	double incr, gain, diff;
	int new_sig_total = 0, new_z_total = 0;

	zeros = *zcnt;
	if(zeros <= BUMZEROCNT * chans) {
		bumzeros = zeros/chans;		/* bumzeros are zeros within signals */
		zeros = 0;
	} else {
		bumzeros = 0;
		zeros /= chans;
	}
	startzeros    = zeros;
	startbumzeros = bumzeros;
	startsigs     = *sig_cnt/chans;
	if(*final_pos > 0)
		*final_pos -= dz->buflen;
	for(q=0;q<chans;q++) {
		zeros    = startzeros;
		bumzeros = startbumzeros;
		sigs     = startsigs;
		start_smoothed = stsmoothed[q];
		for(n=q + *final_pos; n < opos; n+=chans) {
			if(flteq(buf[n],0.0)) {
				if(zeros == 0) {
					bumzeros++;
					if(bumzeros <= BUMZEROCNT) {
						continue;
					} else {
						zeros = BUMZEROCNT;
						bumzeros = 0;
					}
				}
				if(sigs) {
					checklen = min(sigs,SMOOTHWIN);
					end = n - (BUMZEROCNT * chans);
					start = end - (checklen * chans);
					dosmooth = 0;
					diff = fabs(buf[start] - buf[start+chans]);
					for(k = start+chans;k < end;k+=chans) {
							/* look for sudden gradient changes relative to size of sample */
						if((diff > 0.001) && (fabs(buf[k]) < (diff * 4.0))) {
							dosmooth = 1;
							break;
						}
						diff = fabs(buf[k+chans] - buf[k]);
					}
					if(dosmooth) {
						fadelen = min(sigs,SMOOTHWIN);
						start = end - (fadelen * chans);
						gain = 1.0;
						incr = 1.0/(double)fadelen;
						for(k = start+chans;k < end;k+=chans) {
							gain -= incr;
							buf[k] = (float)(buf[k] * gain);
						}
					}
				}
				sigs = 0;
				start_smoothed = 0;
				zeros++;
			} else {
				if(bumzeros) {
					sigs += bumzeros;
					bumzeros = 0;
				}
				if(!sigs) {
					bumzeros = 0;
					sigs = 0;
					zeros = 0;
				}
				sigs++;
				if(!start_smoothed && (sigs > SMOOTHWIN)) {
					dosmooth = 0;
					start = n - (SMOOTHWIN * chans);
					diff = fabs(buf[start] - buf[start+chans]);
					for(k = start+chans;k < n;k+=chans) {
						if((diff > 0.001) && (fabs(buf[k-chans]) < (diff * 4.0))) {
							dosmooth = 1;
							break;
						}
						diff = fabs(buf[k+chans] - buf[k]);
					}
					if(dosmooth) {
						fadelen = min(sigs,SMOOTHWIN);
						start = n - (SMOOTHWIN * chans);
						gain = 1.0;
						incr = 1.0/(double)fadelen;
						for(k = n-chans;k >= start;k-=chans) {
							gain -= incr;
							buf[k] = (float)(buf[k] * gain);
						}
					}
					start_smoothed = 1;
				}
			}
		}
				/* PROCESS START OF SIGNAL IN CONTINUATION-BUFFER, in case badsig_possible overlaps bufend  */
		if(!last) {
			finished = 0;
			while(!finished) {					
				if(flteq(buf[n],0.0)) {
					if(zeros == 0) {
						bumzeros++;
						if(bumzeros <= BUMZEROCNT) {
							continue;
						} else {
							zeros = BUMZEROCNT;
							bumzeros = 0;
						}
					}
					checklen = min(sigs,SMOOTHWIN);
					end = n - (BUMZEROCNT * chans);
					start = end - (checklen * chans);
					dosmooth = 0;
					diff = fabs(buf[start] - buf[start+chans]);
					for(k = start+chans;k < end;k+=chans) {
							/* if the samp-to-samp step > 0.001 (-60dB) */
							/* If the step is more than 4 times the sizeof the sample, complain */
						if((diff > 0.001) && (fabs(buf[k]) < (diff * 4.0))) {
							dosmooth = 1;
							break;
						}
						diff = fabs(buf[k+chans] - buf[k]);
					}
					if(dosmooth) {
						fadelen = min(sigs,SMOOTHWIN);
						start = end - (fadelen * chans);
						gain = 1.0;
						incr = 1.0/(double)fadelen;
						for(k = start+chans;k < end;k+=chans) {
							gain -= incr;
							buf[k] = (float)(buf[k] * gain);
						}
					}
					finished = 1;
				} else {
					if(bumzeros) {
						sigs += bumzeros;
						bumzeros = 0;
					}
					if(!sigs) {
						bumzeros = 0;
						sigs = 0;
						zeros = 0;
					}
					sigs++;
					if(!start_smoothed && (sigs > SMOOTHWIN)) {
						dosmooth = 0;
						start = n - (SMOOTHWIN * chans);
						diff = fabs(buf[start] - buf[start+chans]);
						for(k = start+chans;k < n;k+=chans) {
							if((diff > 0.001) && (fabs(buf[k-chans]) < (diff * 4.0))) {
								dosmooth = 1;
								break;
							}
							diff = fabs(buf[k+chans] - buf[k]);
						}
						if(dosmooth) {
							start = n - (SMOOTHWIN * chans);
							gain = 1.0;
							incr = 1.0/(double)SMOOTHWIN;
							for(k = n-chans;k >= start;k-=chans) {
								gain -= incr;
								buf[k] = (float)(buf[k] * gain);
							}
						}
						start_smoothed = 1;
					}
					if(n > (dz->buflen + SMOOTHWIN) * chans)	/* The signal tail all falls in next buffer */
						finished = 1;
				}
				n+=chans;
			}
		}
		new_sig_total += sigs;
		new_z_total   += zeros;
		stsmoothed[q] = start_smoothed;
	}
	*sig_cnt = new_sig_total;
	*zcnt    = new_z_total;
	*final_pos = n;
	return FINISHED;
}

/********************************/
/*			PSOW_CHOP			*/
/********************************/

/******************************** KEEP_FILENAME_TAIL_ONLY ********************************/

void keep_filename_tail_only(dataptr dz)
{
	int namelen = strlen(dz->wordstor[0]);
	char *q = dz->wordstor[0];
	char *r = dz->wordstor[0] + namelen;
	char *p = r - 1;
	while((*p != '\\') && (*p != '/') && (*p != ':')) {
		p--	;
		if(p < dz->wordstor[0])
			break;
	}
	if(p > dz->wordstor[0]) {
		p++;
		while(p <= r)
			*q++ = *p++;
	}
}

/******************************** CREATE_NEXT_FILENAME ********************************/

void create_next_filename(char *outfilename,int n)
{
	n++;
	if(!sloom) {
		if(n<10)
			insert_new_chars_at_filename_end(outfilename,"_00");
		else if(n<100)
			insert_new_chars_at_filename_end(outfilename,"_0");
		else
			insert_new_chars_at_filename_end(outfilename,"_");
		insert_new_number_at_filename_end(outfilename,n,0);
	} else
		insert_new_number_at_filename_end(outfilename,n,1);
}

/********************************/
/*			PSOA_INTERP			*/
/********************************/

/******************************** PSOA_INTERP ********************************/

int psoa_interp(dataptr dz) 
{
	int exit_status;
	int start0, end0, start1, end1, n, m, seglen0, seglen1, repets, sampcnt;
	float *ibuf0 = dz->sampbuf[0];
	float *ibuf1 = dz->sampbuf[1];
	float *obuf0 = dz->sampbuf[2];
	float *obuf1 = dz->sampbuf[3];
	float *ovflw = dz->sampbuf[4];
	int opos = 0;
	double gain, time;
	initrand48();
	memset((char *)obuf0,0,dz->buflen * sizeof(float));
	memset((char *)obuf1,0,dz->buflen * sizeof(float));
	memset((char *)ovflw,0,dz->buflen * sizeof(float));

	/* GET 1ST GRAIN */

	if((exit_status = read_samps(ibuf0,dz))<0)
		return exit_status;
	start0 = 0;
	end0 = dz->insams[0];
	if(!is_valid_grain(ibuf0,dz->insams[0])) {
		sprintf(errstr,"File 1 is not a valid pitch-sync grain file.\n");
		return(DATA_ERROR);
	}
	seglen0 = end0 - start0;

	/* IN CASES EXPANDING A FIRST GRAIN, Do the expansion */

	sampcnt = (int)round(dz->param[PS_SDUR] * (double)dz->infile->srate);
	if(sampcnt == 0)
		repets = 1;
	else
		repets = (int)round((double)sampcnt/(double)seglen0);
	for(n=0;n<repets;n++) {
		time = (double)(dz->total_samps_written + opos)/(double)dz->infile->srate;
		if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
			return(exit_status);
		if(dz->param[PS_TREMDEPTH] < 0.01)
			dz->param[PS_TREMDEPTH] = 0.0;
		if(dz->param[PS_TREMDEPTH] > 0.0)
			gain = tremolo(end0 - start0,dz->param[PS_TREMFRQ],dz->param[PS_TREMDEPTH],dz);
		else
			gain = 1.0;
		for(m=start0;m < end0;m++) {
			obuf0[opos++] = (float)(ibuf0[m] * gain);
			if(opos >= dz->buflen * 2) {
				if((exit_status = write_samps(obuf0,dz->buflen,dz))<0)
					return(exit_status);
				memcpy((char *)obuf0,(char *)obuf1,dz->buflen * sizeof(float));
				memset((char *)obuf1,0,dz->buflen * sizeof(float));
				opos = dz->buflen;
			}
		}
		if(dz->param[PS_VIBDEPTH] > 0.0)
			opos += vibrato(seglen0,dz->param[PS_VIBFRQ],dz->param[PS_VIBDEPTH],dz);
	}

	/* FIND 2ND GRAIN */

	if((dz->ssampsread = fgetfbufEx(ibuf1, dz->buflen,dz->ifd[1],0)) < 0) {
		sprintf(errstr,"Can't read samples from input soundfile 2\n");
		return(SYSTEM_ERROR);
	}
	start1 = 0;
	end1 = dz->insams[1];
	if(!is_valid_grain(ibuf1,dz->insams[1])) {
		sprintf(errstr,"File 2 is not a valid pitch-sync grain file.\n");
		return(DATA_ERROR);
	}
	seglen1 = end1 - start1;

	if((exit_status = do_pitchsync_grain_interp(start0,end0,start1,end1,&opos,dz))<0)
		return(exit_status);

	/* IN CASES EXPANDING A FINAL GRAIN, Expand it */

	sampcnt = (int)round(dz->param[PS_EDUR] * (double)dz->infile->srate);
	if(sampcnt == 0)
		repets = 1;
		repets = (int)round((double)sampcnt/(double)seglen1);
	for(n=0;n<repets;n++) {
		time = (double)(dz->total_samps_written + opos)/(double)dz->infile->srate;
		if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
			return(exit_status);
		if(dz->param[PS_TREMDEPTH] < 0.01)
			dz->param[PS_TREMDEPTH] = 0.0;
		if(dz->param[PS_TREMDEPTH] > 0.0)
			gain = tremolo(end0 - start0,dz->param[PS_TREMFRQ],dz->param[PS_TREMDEPTH],dz);
		else
			gain = 1.0;
		for(m=start1;m < end1;m++) {
			obuf0[opos] = (float)(obuf0[opos] + (ibuf1[m] * gain));
			opos++;
			if(opos >= dz->buflen * 2) {
				if((exit_status = write_samps(obuf0,dz->buflen,dz))<0)
					return(exit_status);
				memcpy((char *)obuf0,(char *)obuf1,dz->buflen * sizeof(float));
				memset((char *)obuf1,0,dz->buflen * sizeof(float));
				memcpy((char *)obuf1,(char *)ovflw,dz->buflen * sizeof(float));
				memset((char *)ovflw,0,dz->buflen * sizeof(float));
				opos = dz->buflen;
			}
		}
		if(dz->param[PS_VIBDEPTH] > 0.0) {
			if(n != repets-1)
				opos += vibrato(seglen1,dz->param[PS_VIBFRQ],dz->param[PS_VIBDEPTH],dz);
		}
	}

	/* WRITE REMAINDER OF OUTPUT FILE */

	if(opos > 0) {
		if((exit_status = write_samps(obuf0,opos,dz))<0)
			return(exit_status);
	}
	return FINISHED;
}

/******************************** IS_VALID_GRAIN ********************************/

int is_valid_grain(float *buf,int filelen)
{
	int n = 0, finished = 0;
	if(buf[0] > 0)
		return 0;
	while(!finished) {
		while(buf[n] <= 0) {
			if(n >= filelen)
				return 0;
			n++;
		}
		while(buf[n] >= 0) {
			if(n >= filelen) {
				finished = 1;
				break;
			}
			n++;
		}
	}
	if(n != filelen) {
		return 0;
	}
	return 1;
}

/******************************** DO_PITCHSYNC_GRAIN_INTERP ********************************/

int do_pitchsync_grain_interp(int start0,int end0,int start1,int end1,int *opos,dataptr dz)
{
	int exit_status;
	float *ibuf0 = dz->sampbuf[0];
	float *ibuf1 = dz->sampbuf[1];
	float *obuf0 = dz->sampbuf[2];
	float *obuf1 = dz->sampbuf[3];
	float *ovflw = dz->sampbuf[4];
	double ratio, dlen, outgrain_position_incr, diff, valdiff, step0, step1, dhere0, dhere1, val0, val1, val;
	float **array0, **array1;
	int cnt0 = 0, cnt1 = 0, maxsize0 = 0, maxsize1 = 0, maxsize, maxcnt, n, m, j, baselen0, baselen1;
	int seglen0, seglen1, thisstart0, thisstart1, thisend0, thisend1, repcnt, wavcnt, change, thislen;
	int here0, here1, startopos;
	int *len0, *len1;
	int wavchange, thiswavlen;
	double wavset_len_incr_step, wavset_len_incr, wavset_interp_ratio, wavset_interp_ratio_step, gain, time;
	seglen0 = end0 - start0;
	seglen1 = end1 - start1;
	/* COUNT NO OF WAVESETS IN EACH GRAIN, AND FIND MAXSIZE */
	
	if((exit_status = count_wavesets(&cnt0,&maxsize0,start0,end0,ibuf0))<0)
		return(exit_status);
	if((exit_status = count_wavesets(&cnt1,&maxsize1,start1,end1,ibuf1))<0)
		return(exit_status);
	maxsize = max(maxsize0,maxsize1);
	maxcnt = max(cnt0,cnt1);

	/* ESTABLISH ARRAYS TO STORE EACH WAVESET, and each waveset's length */
	
	if((array0 = (float  **)malloc(maxcnt * sizeof(float *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for 1st interpolation buffer array.\n");
		return(MEMORY_ERROR);
	}
	if((array1 = (float  **)malloc(maxcnt * sizeof(float *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for 2nd interpolation buffer array.\n");
		return(MEMORY_ERROR);
	}
	if((len0 = (int *)malloc(maxcnt * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for 1st interpolation buffer array.\n");
		return(MEMORY_ERROR);
	}
	if((len1 = (int *)malloc(maxcnt * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for 1st interpolation buffer array.\n");
		return(MEMORY_ERROR);
	}
	baselen0 = 0;
	baselen1 = 0;


	/* GET SIZES OF WAVESET STORES */

	if(cnt0 > cnt1) {			/* If more wavesets in 1st, store zeroed arrays for corresponding wavesets of 2nd, */
		thisstart0 = start0;	/* calculating length by using ratio of sizes of wavesets in the 1st. */
		thisstart1 = start1;	
		for(n=0;n<cnt0;n++) {
			if((exit_status = get_waveset_end(thisstart0,end0,&thisend0,ibuf0))<0)
				return(exit_status);
			len0[n] = thisend0 - thisstart0;
			thisstart0 = thisend0;
			if(n >= cnt1) {
				if(baselen0==0) {
					baselen0 = len0[n-1];
					baselen1 = len1[n-1];
				}
				ratio    = (double)len0[n]/(double)baselen0;
				len1[n]  = (int)round((double)baselen1 * ratio);
			} else {	
				if((exit_status = get_waveset_end(thisstart1,end1,&thisend1,ibuf1))<0)
					return(exit_status);
				len1[n] = thisend1 - thisstart1;
				thisstart1 = thisend1;
			}
		}
	} else if(cnt1 > cnt0) {	/* If more wavesets in 2nd, store zeroed arrays for corresponding wavesets of 1st, */
		thisstart0 = start0;	/* calculating length by using ratio of sizes of wavesets in the 2nd. */
		thisstart1 = start1;
		for(n=0;n<cnt1;n++) {
			if((exit_status = get_waveset_end(thisstart1,end1,&thisend1,ibuf1))<0)
				return(exit_status);
			len1[n] = thisend1 - thisstart1;
			thisstart1 = thisend1;
			if(n >= cnt0) {
				if(baselen0==0) {
					baselen0 = len0[n-1];
					baselen1 = len1[n-1];
				}
				ratio    = (double)len1[n]/(double)baselen1;
				len0[n]  = (int)round((double)baselen0 * ratio);
			} else {	
				if((exit_status = get_waveset_end(thisstart0,end0,&thisend0,ibuf0))<0)
					return(exit_status);
				len0[n] = thisend0 - thisstart0;
				thisstart0 = thisend0;
			}
		}
	} else {					/* Simple case, equal no of wavesets : store them */
		thisstart0 = start0;	/* calculating length by using ratio of sizes of wavesets in the 2nd. */
		thisstart1 = start1;
		for(n=0;n<cnt1;n++) {
			if((exit_status = get_waveset_end(thisstart0,end0,&thisend0,ibuf0))<0)
				return(exit_status);
			len0[n] = thisend0 - thisstart0;
			thisstart0 = thisend0;

			if((exit_status = get_waveset_end(thisstart1,end1,&thisend1,ibuf1))<0)
				return(exit_status);
			len1[n] = thisend1 - thisstart1;
			thisstart1 = thisend1;
		}
	}
	for(n=0;n<maxcnt;n++) {
		if((array0[n] = (float  *)malloc((len0[n] + 1) * sizeof(float)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for grain1 interpolation buffer %d.\n",n+1);
			return(MEMORY_ERROR);
		}
		if((array1[n] = (float  *)malloc((len1[n] + 1) * sizeof(float)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for grain2 interpolation buffer %d.\n",n+1);
			return(MEMORY_ERROR);
		}
	}

	/* STORE THE INITIAL WAVESETS */

	if(cnt0 > cnt1) {			/* If more wavesets in 1st, store zeroed arrays for corresponding wavesets of 2nd, */
		thisstart0 = start0;	/* calculating length by using ratio of sizes of wavesets in the 1st. */
		thisstart1 = start1;	
		for(n=0;n<cnt0;n++) {
			if((exit_status = get_waveset_end(thisstart0,end0,&thisend0,ibuf0))<0)
				return(exit_status);
			memcpy((char *)array0[n],(char *)(ibuf0 + thisstart0),len0[n] * sizeof(float));
			thisstart0 = thisend0;
			if(n >= cnt1) {
				memset((char *)array1[n],0,len1[n] * sizeof(float));
			} else {	
				if((exit_status = get_waveset_end(thisstart1,end1,&thisend1,ibuf1))<0)
					return(exit_status);
				memcpy((char *)array1[n],(char *)(ibuf1 + thisstart1),len1[n] * sizeof(float));
				thisstart1 = thisend1;
			}
			array0[n][len0[n]] = 0.0f;	/* wraparound points */
			array1[n][len1[n]] = 0.0f;	/* wraparound points */
		}
	} else if(cnt1 > cnt0) {	/* If more wavesets in 2nd, store zeroed arrays for corresponding wavesets of 1st, */
		thisstart0 = start0;	/* calculating length by using ratio of sizes of wavesets in the 2nd. */
		thisstart1 = start1;
		for(n=0;n<cnt1;n++) {
			if((exit_status = get_waveset_end(thisstart1,end1,&thisend1,ibuf1))<0)
				return(exit_status);
			memcpy((char *)array1[n],(char *)(ibuf1 + thisstart1),len1[n] * sizeof(float));
			thisstart1 = thisend1;
			if(n >= cnt0) {
				memset((char *)array0[n],0,len0[n] * sizeof(float));
			} else {	
				if((exit_status = get_waveset_end(thisstart0,end0,&thisend0,ibuf0))<0)
					return(exit_status);
				memcpy((char *)array0[n],(char *)(ibuf0 + thisstart0),len0[n] * sizeof(float));
				thisstart0 = thisend0;
			}
			array0[n][len0[n]] = 0.0f;	/* wraparound points */
			array1[n][len1[n]] = 0.0f;	/* wraparound points */
		}
	} else {					/* Simple case, equal no of wavesets : store them */
		thisstart0 = start0;	/* calculating length by using ratio of sizes of wavesets in the 2nd. */
		thisstart1 = start1;
		for(n=0;n<cnt1;n++) {
			if((exit_status = get_waveset_end(thisstart0,end0,&thisend0,ibuf0))<0)
				return(exit_status);
			memcpy((char *)array0[n],(char *)(ibuf0 + thisstart0),len0[n] * sizeof(float));
			thisstart0 = thisend0;
			if((exit_status = get_waveset_end(thisstart1,end1,&thisend1,ibuf1))<0)
				return(exit_status);
			memcpy((char *)array1[n],(char *)(ibuf1 + thisstart1),len1[n] * sizeof(float));
			thisstart1 = thisend1;
			array0[n][len0[n]] = 0.0f;	/* wraparound points */
			array1[n][len1[n]] = 0.0f;	/* wraparound points */
		}
	}
	repcnt = find_repcnt(seglen0,seglen1,0,dz);
	wavcnt = max(cnt0,cnt1);
	dlen = seglen0;
	change = (seglen1 - seglen0);
	outgrain_position_incr   = (double)change/(double)repcnt;
	wavset_interp_ratio_step = 1.0/(double)repcnt;
	wavset_interp_ratio      = wavset_interp_ratio_step;

	startopos = *opos;
	for(n=0;n<repcnt;n++) {
		*opos = startopos;
		thislen = (int)round(dlen);
		time = (double)(dz->total_samps_written + *opos)/(double)dz->infile->srate;
		if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
			return(exit_status);
		if(dz->param[PS_TREMDEPTH] < 0.01)
			dz->param[PS_TREMDEPTH] = 0.0;
		if(dz->param[PS_TREMDEPTH] > 0.0)
			gain = tremolo(end0 - start0,dz->param[PS_TREMFRQ],dz->param[PS_TREMDEPTH],dz);
		else
			gain = 1.0;
		for(m=0;m<wavcnt;m++) {
			here0 = 0;
			here1 = 0;
			dhere0 = (double)here0;
			dhere1 = (double)here1;
			wavchange = len1[m] - len0[m];
			wavset_len_incr_step = (double)wavchange/(double)repcnt;
			wavset_len_incr = wavset_len_incr_step * (double)n;
			thiswavlen = (int)round((double)len0[m] + wavset_len_incr);
			step0 = (double)len0[m]/(double)thiswavlen;
			step1  = (double)len1[m]/(double)thiswavlen;
			val0 = array0[m][here0] * (1.0 - wavset_interp_ratio);
			val1 = array1[m][here1] * wavset_interp_ratio;
			val   = val0 + val1;
			val *= gain;
			obuf0[*opos] = (float)(obuf0[*opos] + val);
			(*opos)++;
			for(j=1;j<thiswavlen;j++) {
				dhere0 += step0;						/* get value from 1st waveset, by interp Nn stored waveset */
				here0  = (int)floor(dhere0);			/* as outlen may be different to stored length */
				diff   = dhere0 - (double)here0;
				valdiff= array0[m][here0 + 1] - array0[m][here0];
				val0   = (double)array0[m][here0];
				val0  += (diff * valdiff);
				val0 *=  (1.0 - wavset_interp_ratio);	/* Get proportion of the value approp to TIME in interp */

				dhere1 += step1;						/* Simil with 2nd waveset */
				here1 = (int)floor(dhere1);
				diff   = dhere1 - (double)here1;
				valdiff= array1[m][here1 + 1] - array1[m][here1];
				val1   = (double)array1[m][here1];
				val1  += (diff * valdiff);
				val1  *= wavset_interp_ratio;

				val   = val0 + val1;
				val *= gain;
				obuf0[*opos] = (float)(obuf0[*opos] + val);
				(*opos)++;
			}
		}
		dlen += outgrain_position_incr;
		wavset_interp_ratio += wavset_interp_ratio_step;
		startopos += thislen;
		if(dz->param[PS_VIBDEPTH] > 0.0)
			startopos += vibrato(thislen,dz->param[PS_VIBFRQ],dz->param[PS_VIBDEPTH],dz);
		if(startopos >= dz->buflen * 2) {
			if((exit_status = write_samps(obuf0,dz->buflen,dz))<0)
				return(exit_status);
			memcpy((char *)obuf0,(char *)obuf1,dz->buflen * sizeof(float));
			memset((char *)obuf1,0,dz->buflen * sizeof(float));
			memcpy((char *)obuf1,(char *)ovflw,dz->buflen * sizeof(float));
			memset((char *)ovflw,0,dz->buflen * sizeof(float));
			startopos -= dz->buflen;
		}
	}
	*opos = startopos;
	return(FINISHED);
}

/******************************** COUNT_WAVESETS ********************************/

int count_wavesets(int *cnt,int *maxsize,int start,int end,float *buf) 
{
	int thisstart, n, size;
	thisstart = start;
	for(n=start;n<end;n++) {
		while(buf[n] < 0) {
			n++;
			if(n >= end) {
				sprintf(errstr,"Anomalous segment end.\n");
				return(DATA_ERROR);
			}
		}
		while(buf[n] >= 0) {
			n++;
			if(n >= end)
				break;
		}
		(*cnt)++;
		size = n - thisstart;
		*maxsize = max(*maxsize,size);
		thisstart = n;
	}
	return(FINISHED);
}

/******************************** GET_WAVESET_END ********************************/

int get_waveset_end(int start,int end,int *thisend,float *buf) 
{
	int n;
	for(n=start;n<end;n++) {
		while(buf[n] < 0) {
			n++;
			if(n >= end) {
				sprintf(errstr,"Anomalous segment end.\n");
				return(DATA_ERROR);
			}
		}
		while(buf[n] >= 0) {
			n++;
			if(n >= end) {
				break;
		}
		}
		*thisend = n;
		break;
	}
	return(FINISHED);
}

/******************************** FIND_REPCNT ********************************/

int find_repcnt(int startlen,int endlen,int samplen,dataptr dz)
{
	int maxlen, minlen, repcnt, diff, sum, n, thisrep=0;
	double incr, dlen;
	int thisgap, thatgap;
	if(samplen == 0)
		samplen = (int)round(dz->param[1] * (double)dz->infile->srate);
	thatgap = samplen * 2;
	maxlen = max(startlen,endlen);							/* get the largest segment */
	minlen = min(startlen,endlen);							/* get the largest segment */
	repcnt = (int)round((double)samplen/(double)maxlen);	/* divide whole length by this to get min no of repetitions */
	diff = maxlen - minlen;

	for(;;) {
		dlen = (double)minlen;								/* the unrounded no of samps counted */
		incr = (double)diff/repcnt;							/* the size increment from grain to grain */
		sum = 0;
		for(n=0;n<repcnt;n++) {								/* add grains, incrementing their lengths as we go */
			sum += (int)round(dlen);						/* and find total length */
			incr = (double)diff/repcnt;
			dlen += incr;
		}
		if(sum == samplen) {								/* If we hit target length break */ 
			thisrep = repcnt;
			break;
		} else if(sum > samplen) {							/* if TOO BIG */
			thisgap = sum - samplen;						/* find difference from target */
			if(thisgap >= thatgap) {							/* if bigger than or equal to last diff, we've found best fit */
				break;
			}
			thatgap = thisgap;								/* if smaller than last diff, remember diff */
			thisrep = repcnt;								/* and remember this no of repeptitions */
			repcnt--;										/* decrement the repetition count */
		} else  {	/* sum < samplen */						/* if TOO SMALL */
			thisgap = samplen - sum;						/* find difference from target */
			if(thisgap >= thatgap) {							/* if bigger than or equal to last diff, we've found best fit */
				break;
			}
			thatgap = thisgap;								/* if smaller than last diff, remember diff */
			thisrep = repcnt;								/* and remember this no of repeptitions */
			repcnt++;										/* increment the repetition count */
		} 
	}
	return thisrep;
}

/******************************** VIBRATO ********************************/

int vibrato(int seglen,double vibfrq,double vibdep,dataptr dz)
{
	double shift, advance;
	static int val = 0;
	static double pos = 0;
	int zz, len;
	len = (int)round((double)dz->infile->srate/vibfrq);	//	Effective sample length of vibrato cycle
	if(val == 0)
		advance = 0.0;
	else
		advance = (double)seglen/(double)len;				//	Fractional stop through vibrato table
	advance *= TWOPI;
	pos += advance;
	pos = fmod(pos,TWOPI);									//	Position in vibrato cycle	
	shift = pow(2.0,vibdep/SEMITONES_PER_OCTAVE) - 1.0;		//	Frequency shift corresponfing to position in vibrato cycle
	zz = (int)round(sin(pos) * shift * seglen);			//	Shift in position of FOF to correspond to vibrato
	val += seglen;
	return zz;

}

/******************************** TREMOLO ********************************/

double tremolo(int seglen,double tremfrq,double tremdepth,dataptr dz)
{
	double pos, gain;
	static int val = 0;
	int len;
	len = (int)round((double)dz->infile->srate/(double)tremfrq);
	pos = (double)val/(double)len;
	pos *= TWOPI;
	pos = fmod(pos,TWOPI);
	gain = (cos(pos) - 1.0) * 0.5;		/* range 0 to -1 */
	gain *= tremdepth;					/* range 0 to -tremdepth */
	gain = 1.0 - gain;					/* range 1 to 1-tremdepth */
	val += seglen;
	if(tremdepth > 1.0)
		gain /= tremdepth; 
	return gain;

}

/*****************/
/* PSOW_FEATURES */
/*****************/

/******************************** ZEROFOF ********************************/

int zerofof(int start,int end,float *buf)
{
	int n;
	float val;
	for(n = start; n < end; n++) {
		val = buf[n];
		if(val > 0.0)
			return 0;
		if(val < 0.0)
			return 0;
	}
	return 1;
}

/******************************** FOF_STRETCH ********************************/

int fof_stretch(int n,double time,int here_in_buf,int there_in_buf,float *ibuf,float *obuf,
		int *opos,double *maxlevel,double gain,dataptr dz)
{
	int maxloc, maxwavstart, maxwavend, repets, k;
	double maxamp, val;

	if((dz->iparam[PSF_FOFSTR] > 256) && (n % 100 == 0)) {
		fprintf(stdout,"INFO: Input time %lf\n",time);
		fflush(stdout);
	}
	maxamp = fabs(ibuf[here_in_buf]);
	maxloc = here_in_buf;
	for(k = here_in_buf; k < there_in_buf;k++) {
		if(fabs(ibuf[k]) > maxamp) {
			maxamp = fabs(ibuf[k]);
			maxloc = k;
		}
	}
	k = here_in_buf;
	maxwavstart = k;
	while((maxwavend = next_down_zcross(k,there_in_buf,ibuf))<there_in_buf) {
		if(maxwavend < 0)
			maxwavend = there_in_buf;
		if(maxwavend > maxloc)
			break;
	if(dz->vflag[0] == 1)
			maxwavstart = k;
		k = maxwavend;
		
	}
	for(k = here_in_buf;k < maxwavstart;k++) {
		val = ibuf[k] * gain;
		obuf[*opos] = (float)(obuf[*opos] + val);
		*maxlevel = max(obuf[*opos],*maxlevel);
		(*opos)++;
	}
	for(repets = 0; repets < dz->iparam[PSF_FOFSTR]; repets++) {
		for(k = maxwavstart;k < maxwavend;k++) {
			val = ibuf[k] * gain;
			obuf[*opos] = (float)(obuf[*opos] + val);
			*maxlevel = max(obuf[*opos],*maxlevel);
			(*opos)++;
			if(*opos >= dz->buflen * 2) {
				sprintf(errstr,"OVERFLOWED OUTPUT BUFFER.\n");
				return(MEMORY_ERROR);
			}
		}
	}
	for(k = maxwavend;k < there_in_buf;k++) {
		val = ibuf[k] * gain;
		obuf[*opos] = (float)(obuf[*opos] + val);
		*maxlevel = max(obuf[*opos],*maxlevel);
		(*opos)++;
		if(*opos >= dz->buflen * 2) {
			sprintf(errstr,"OVERFLOWED OUTPUT BUFFER.\n");
			return(MEMORY_ERROR);
		}
	}
	return(FINISHED);
}

/*******************************************/
/* SUPERIMPOSE FOFS ON SYNTHTONES OR INPUT */
/*******************************************/

/******************************** GETFOFENV ********************************/

void getfofenv(float *ibuf,double *fofenv,int *fofloc,int maxseglen,int here_in_buf,int there_in_buf,int *envcnt)
{
	double maxamp;
	int seglen, maxloc, n;
	seglen = there_in_buf - here_in_buf;
	memset((char *)fofenv,0,maxseglen * sizeof(double));
	memset((char *)fofloc,0,maxseglen * sizeof(int));
	maxloc = here_in_buf;
	maxamp = 0.0;
	*envcnt = 0;
	for(n = here_in_buf; n < there_in_buf;n++) {
		maxamp = 0.0;
		while(ibuf[n] <= 0.0) {				/* find absmax in each half-cycle */
			if(fabs(ibuf[n]) > maxamp) {
				maxamp = fabs(ibuf[n]);
				maxloc = n;
			}
			n++;
		}
		fofenv[*envcnt] = maxamp;			/* and store as points on envelope curve */
		fofloc[*envcnt] = maxloc - here_in_buf;
		(*envcnt)++;
		maxamp = 0.0;
		maxloc = n;
		while(ibuf[n] >= 0.0) {
			if(ibuf[n] > maxamp) {
				maxamp = ibuf[n];
				maxloc = n;
			}
			n++;
		}
		fofenv[*envcnt]   = maxamp;
		fofloc[*envcnt] = maxloc - here_in_buf;
		(*envcnt)++;
	}
	/* Add edge zeros */
	for(n=(*envcnt)-1;n >=0;n--) {
		fofenv[n+1] = fofenv[n];
		fofloc[n+1] = fofloc[n];
	}
	fofenv[0] = 0.0;			
	fofloc[0] = 0;
	(*envcnt)++;
	if(!flteq(fofenv[(*envcnt)-1],0.0)) {
		fofenv[*envcnt] = 0.0;
		fofloc[*envcnt] = there_in_buf - here_in_buf;
		(*envcnt)++;
	}
}

/******************************** LINEAR_SPLINT ********************************/

double linear_splint(double *fofenv,int *fofloc,int envcnt,double time,int *lo)
{
	int hi;
	double val, timediff, valdiff, timestep;
	if((double)fofloc[*lo] <= time) {
		while((double)fofloc[*lo] <= time)
			(*lo)++;
		(*lo)--;
	}
	hi = (*lo) + 1;
	timediff = time - (double)fofloc[*lo];
	timestep = (double)(fofloc[hi] - fofloc[*lo]);
	valdiff  = fofenv[hi] - fofenv[*lo];
	val  = fofenv[*lo];
	val += (valdiff * timediff)/timestep;
	return max(val,0.0);
}

/******************************** SUPERIMPOSE_FOFS_ON_SYNTHTONES ********************************/

int superimpose_fofs_on_synthtones(double *fofenv,int *fofloc,double *sintab,int here_in_buf,int there_in_buf,
				float *ibuf,float *obuf,int *opos,int samptime,int envcnt,dataptr dz)
{
	int exit_status;
	int seglen, n, m, tabpos, lo;
	double maxamp, timediff, valdiff, normaliser, fofval;
	double wavtabincr, dtabpos;
	double vallo, valhi, val;
	float *sbuf = dz->sampbuf[4];
	double *frq = dz->parray[PS_OSCFRQ], *amp = dz->parray[PS_OSCAMP];
	seglen = there_in_buf - here_in_buf;
	memset((char *)sbuf,0,dz->buflen  * sizeof(float));	/* clear synthesis buffer */
	switch(dz->mode) {
	case(2):											/* TIMEVARYING OSCILLATOR BANK */
	case(3):
		if((exit_status = newoscval(samptime,dz))<0)
			return(exit_status);
		/* fall thro */
	case(0):											/* FIXED OSCILLATOR BANK */
	case(1):
		for(n=0;n < dz->iparam[PS_SCNT];n++) {			/* Generate multi-syn synthed waveform */
			wavtabincr = (SINTABLEN * frq[n])/(double)dz->infile->srate;
			dtabpos = 0;
			tabpos = 0;
			for(m=0;m<=seglen;m++) {
				tabpos = (int)floor(dtabpos);
				timediff = dtabpos - (double)tabpos;
				tabpos %= (int)SINTABLEN;
				vallo = sintab[tabpos];
				valhi = sintab[tabpos+1];
				valdiff = valhi - vallo;
				val   = vallo + (valdiff * timediff);
				val  *= amp[n];							/* use amplitude of each component */
				sbuf[m] = (float)(sbuf[m] + val);
				dtabpos += wavtabincr;
			}
		}
		break;
	case(4):			/* NOISE */
		for(n=0;n<=seglen;n++)
			sbuf[n] = (float)((drand48() * 2.0) - 1.0);
		break;
	}
	maxamp = 0.0;
	for(n=0;n<seglen;n++)						/* Find synthetic sound's maxamp */
		maxamp = max(maxamp,fabs(sbuf[n]));
	normaliser = 1.0/maxamp;
	for(n=0;n<seglen;n++)						/* Normalise synthetic sound */
		sbuf[n] = (float)(sbuf[n] * normaliser);
	lo = 0;
	for(n=0;n<seglen;n++) {
		val  = sbuf[n];
		fofval = linear_splint(fofenv,fofloc,envcnt,(double)n,&lo);
		if(dz->param[PS_DEPTH] < 1.0) {
			vallo  = val * (1.0 - dz->param[PS_DEPTH]);
			valhi  = val * dz->param[PS_DEPTH];			/* if fof isn't depth of whole sound */
			valhi *= fofval;
			val = vallo + valhi;
		} else
			val *= fofval;
		obuf[*opos] = (float)val;
		(*opos)++;
	}
	return(FINISHED);
}

/******************************* NEWOSCVAL ****************************/

int newoscval(int samptime,dataptr dz)
{
	int   thistime, lasttime, nowdiff, timediff;
	double rratio;
	int   n,m,k;
	double thisval, lastval, timeratio;
	int total_frqcnt = dz->iparam[PS_SCNT] * dz->iparam[PS_TIMESLOTS];

	k = dz->iparam[PS_TIMES_CNT];
	thistime  = dz->lparray[PS_SAMPTIME][k];
	while(thistime < samptime) {
		dz->iparam[PS_TIMES_CNT]++;
		dz->iparam[PS_FRQ_INDEX] += dz->iparam[PS_SCNT];
		if(dz->iparam[PS_TIMES_CNT]>dz->iparam[PS_TIMESLOTS]) {
			sprintf(errstr,"Ran off end of oscillator data: newoscval()\n"); 
			return(PROGRAM_ERROR);
		}
		k = dz->iparam[PS_TIMES_CNT];
		thistime = dz->lparray[PS_SAMPTIME][k];
	}
	lasttime  = dz->lparray[PS_SAMPTIME][k-1];
	if(dz->iparam[PS_FRQ_INDEX] >= total_frqcnt) {
		sprintf(errstr,"Ran out of frq, amp values too soon: newoscval()\n");
		return(PROGRAM_ERROR);
	}
	timediff = thistime - lasttime;	
	nowdiff  = samptime - lasttime;
	timeratio = (double)nowdiff/(double)timediff;
	for(n=0, m= dz->iparam[PS_FRQ_INDEX];n<dz->iparam[PS_SCNT];n++,m++) {
	/* FREQUENCY */
		thisval = dz->parray[PS_INFRQ][m];
		lastval = dz->parray[PS_INFRQ][m - dz->iparam[PS_SCNT]];
		if(flteq(lastval,thisval))
			dz->parray[PS_OSCFRQ][n] = thisval;
		else {
			rratio = (thisval/lastval);
			dz->parray[PS_OSCFRQ][n] = lastval * pow(rratio,timeratio);
		}
	/* AMPLITUDE */
		thisval = dz->parray[PS_INAMP][m];
		lastval = dz->parray[PS_INAMP][m - dz->iparam[PS_SCNT]];
		if(flteq(thisval,lastval))
			dz->parray[PS_OSCAMP][n] = thisval;
		else {
			rratio = (thisval/lastval);
			dz->parray[PS_OSCAMP][n] = lastval * pow(rratio,timeratio);
		}
	}
	return(FINISHED);
}

/******************************** SUPERIMPOSE_FOFS_ON_INPUT ********************************/

int superimpose_fofs_on_input(double *fofenv,int *fofloc,int here_in_buf,int there_in_buf,float *ibuf,float *ibuf3,float *obuf,
			int *opos,int startsamp,int *envtoptime,double *envval,double *envincr,int envcnt,dataptr dz)
{
	int changenv = 0;
	int seglen, pos_in_inbuf, pos_in_seg,lo = 0;
	double fofval, time;
	double vallo, valhi, val, srate = dz->infile->srate;
	double endtime = (double)(startsamp + there_in_buf)/srate;
	if(endtime > dz->env[*envtoptime])
		changenv = 1;
	seglen = there_in_buf - here_in_buf;
	for(pos_in_seg=0,pos_in_inbuf=here_in_buf;pos_in_seg<=seglen;pos_in_seg++,pos_in_inbuf++) {
		if(changenv && (time = (double)(startsamp + pos_in_inbuf)/srate) >= dz->env[*envtoptime]) {
			*envval = dz->env[(*envtoptime)+1];
			*envtoptime += 2;
			*envincr  = dz->env[(*envtoptime)+1] - dz->env[(*envtoptime)-1];
			*envincr /= dz->env[*envtoptime]   - dz->env[(*envtoptime)-2];
			*envincr /= srate;
		} else
			*envval += *envincr;
		fofval = linear_splint(fofenv,fofloc,envcnt,(double)pos_in_seg,&lo);
		val = ibuf3[pos_in_inbuf] * (*envval);
		if(dz->param[PS_DEPTH] < 1.0) {
			vallo  = val * (1.0 - dz->param[PS_DEPTH]);
			valhi  = val * dz->param[PS_DEPTH];			/* if fof isn't depth of whole sound */
			valhi *= fofval;
			val = vallo + valhi;
		} else
			val *= fofval;
		obuf[*opos] = (float)val;
		(*opos)++;
	}
	return(FINISHED);
}

/**************************** CREATE_NORMALISING_ENVELOPE **********************/

int create_normalising_envelope(dataptr dz)
{
	int wsize, bigarray, j, n, k, startsamp;
	double maxval = 0.0, time, srate = (double)dz->infile->srate;
	int donebuf;
	float *ibuf = dz->sampbuf[2];
	wsize = (int)floor(dz->param[PS_WSIZE] * MS_TO_SECS * srate);
	bigarray = (dz->insams[1]/wsize) + 20;
	free(dz->env);
	if((dz->env=(float *)malloc(bigarray * 2 * sizeof(float)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for envelope array.\n");
		return(MEMORY_ERROR);
	}
	startsamp = 0;
	k = 0;
	while((dz->ssampsread = fgetfbufEx(ibuf, dz->buflen,dz->ifd[1],0)) > 0) {
		if(dz->ssampsread < 0) {
			sprintf(errstr,"Can't read samples from 2nd input soundfile.\n");
			return(SYSTEM_ERROR);
		}
		donebuf = 0;
		j = 0;
		while(j < dz->ssampsread) {
			for(n=0;n<wsize;n++) {
				maxval = max(fabs(ibuf[j++]),maxval);
				if(j >= dz->ssampsread) {
					donebuf = 1;
					break;
				}
			}
			if(donebuf)
				break;
			time = (double)(startsamp + j)/srate;
			dz->env[k++] = (float)time;
			dz->env[k++] = (float)maxval;
			maxval = 0.0;
		}
		startsamp += dz->ssampsread;
	}
	dz->env[k++] = (float)((double)dz->insams[1]/srate);
	dz->env[k++] = 0.0f;
	if(k < 4) {
		sprintf(errstr,"Failed to find envelope of 2nd soundfile: File too short ??\n");
		return(GOAL_FAILED);
	}
	for(n=1;n<k;n+=2) {
		if(dz->env[n] < dz->param[PS_GATE])	/* eliminate zeros */
			dz->env[n] = 0.0f;
		else								/* force normalisation curve */
			dz->env[n] = (float)(1.0/dz->env[n]);
	}
	return(FINISHED);
}

/*****************************************/
/* PSOW_SYNTH:  DEAL WITH SYNTHESIS DATA */
/*****************************************/

/************************** READ_SYNTH_DATA ********************************/

int read_synth_data(char *filename,dataptr dz)
{
	int exit_status;
	int n = 0, valcnt;
	char temp[200], *p, *thisword;
	double *frq, *amp;
	int is_frq = TRUE;
	if((dz->fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,"Cannot open datafile %s\n",filename);
		return(DATA_ERROR);
	}
	while(fgets(temp,200,dz->fp)!=NULL) {
		p = temp;
		if(is_an_empty_line_or_a_comment(p))
			continue;
		n++;
	}
	if(n==0) {
		sprintf(errstr,"No data in file %s\n",filename);
		return(DATA_ERROR);
	}
	dz->iparam[PS_SCNT] = n;
	if((exit_status = allocate_osc_frq_amp_arrays(dz->iparam[PS_SCNT],dz))<0)
		return(exit_status);
	frq = dz->parray[PS_OSCFRQ];
	amp = dz->parray[PS_OSCAMP];
	if(fseek(dz->fp,0,0)<0) {
		sprintf(errstr,"fseek() failed in read_synth_data()\n");
		return(SYSTEM_ERROR);
	}
	n = 0;
	while(fgets(temp,200,dz->fp)!=NULL) {
		p = temp;
		if(is_an_empty_line_or_a_comment(temp))
			continue;
		if(n >= dz->iparam[PS_SCNT]) {
			sprintf(errstr,"Accounting problem reading Frq & Amp: read_synth_data()\n");
			return(PROGRAM_ERROR);
		}
		valcnt = 0;
		while(get_word_from_string(&p,&thisword)) {
			if(valcnt>=2) {
				sprintf(errstr,"Too many values on line %d: file %s\n",n+1,filename);
				return(DATA_ERROR);
			}
			if(is_frq) {
				if(sscanf(thisword,"%lf",&(frq[n]))!=1) {
					sprintf(errstr,"Problem reading Frq data: line %d: file %s\n",n+1,filename);
					return(DATA_ERROR);
				}
				if(frq[n]<dz->application->min_special || frq[n]>dz->application->max_special) {
					sprintf(errstr,"frq (%.3lf) on line %d out of range (%.1lf to %.1lf):file %s\n",
					frq[n],n+1,dz->application->min_special,dz->application->max_special,filename);
					return(DATA_ERROR);
				}
				if(dz->mode==1 || dz->mode==3)
					frq[n] = miditohz(frq[n]);
			} else {
				if((exit_status = get_level(thisword,&(amp[n])))<0)
					return(exit_status);
				if(amp[n]<0.0 || amp[n]>1.0) {
					sprintf(errstr,"amp (%lf) out of range (0 to 1) on line %d: file %s\n",amp[n],n+1,filename);
					return(DATA_ERROR);
				}
			}
			is_frq = !is_frq;
			valcnt++;
		}
		if(valcnt<2) {
			sprintf(errstr,"Not enough values on line %d: file %s\n",n+1,filename);
			return(DATA_ERROR);
		}
		n++;
	}
	if(fclose(dz->fp)<0) {
		fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
		fflush(stdout);
	}
	return(FINISHED);
}

/************************** READ_TIME_VARYING_SYNTH_DATA ********************************/

int read_time_varying_synth_data(char *filename,dataptr dz)
{
	int exit_status;
//	int cnt = 0;
	if((exit_status = get_data_from_tvary_infile(filename,dz))<0)
		return(exit_status);
	if((exit_status = check_synth_data(dz->iparam[PS_ENTRYCNT],dz))<0)
		return(exit_status);
	return(FINISHED);
}

/**************************** GET_DATA_FROM_TVARY_INFILE *******************************/

int get_data_from_tvary_infile(char *filename,dataptr dz)
{
	int exit_status;
	char *temp, *p, *thisword;
	int maxlinelen, frqcnt;
	int total_wordcnt = 0;
	int  columns_in_this_row, columns_in_row = 0, number_of_rows = 0;
	int n, m, k, old_wordcnt;
	double filedur = (double)dz->insams[0]/(double)dz->infile->srate;
	double far_time = filedur + 1.0;
	double val;
	if((dz->fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,"Cannot open datafile %s\n",filename);
		return(DATA_ERROR);
	}
	if((exit_status = getmaxlinelen(&maxlinelen,dz->fp))<0)
		return(exit_status);
	if((fseek(dz->fp,0,0))<0) {
		sprintf(errstr,"Seek failed in get_data_from_tvary_infile()\n");
		return(SYSTEM_ERROR);
	}
	if((temp = (char *)malloc((maxlinelen+2) * sizeof(char)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for temporary line storage.\n");
		return(MEMORY_ERROR);
	}
	while(fgets(temp,maxlinelen,dz->fp)!=NULL) {
		columns_in_this_row = 0;
		if(is_an_empty_line_or_a_comment(temp))
			continue;
		p = temp;
		while(get_word_from_string(&p,&thisword)) { 
			if((exit_status = get_level(thisword,&val))<0) {	/* reads vals or dB vals */
				free(temp);
				return(exit_status);
			}
			if((total_wordcnt==0 && ((dz->parray[PS_FBRK] = (double *)malloc(sizeof(double)))==NULL))
			|| (dz->parray[PS_FBRK] = (double *)realloc(dz->parray[PS_FBRK],(total_wordcnt+1) * sizeof(double)))==NULL) {
				free(temp);
				sprintf(errstr,"INSUFFICIENT MEMORY to reallocate oscillator brktable.\n");
				return(MEMORY_ERROR);
			}
			dz->parray[PS_FBRK][total_wordcnt] = val;
			columns_in_this_row++;
			total_wordcnt++;
		}
		if(number_of_rows==0) {
			if((columns_in_row = columns_in_this_row)<3) {
				sprintf(errstr,"Insufficient oscillator data in row 1 of file %s.\n",filename);
				free(temp);
				return(DATA_ERROR);
			} else if (ODD(columns_in_row - 1)) {
				sprintf(errstr,"Frq and Amp data not paired correctly (or no Time) in row 1 of file %s.\n",filename);
				free(temp);
				return(DATA_ERROR);
			}
		} else if(columns_in_this_row!=columns_in_row) {
			free(temp);
			if(columns_in_this_row < columns_in_row)
				sprintf(errstr,"Not enough entries in row %d of file %s\n",number_of_rows+1,filename);
			else
				sprintf(errstr,"Too many entries in row %d of file %s\n",number_of_rows+1,filename);
			return(DATA_ERROR);
		}
		number_of_rows++;
	}
	dz->iparam[PS_WORDCNT] = total_wordcnt;
	free(temp);
	if(columns_in_row<3) {
		sprintf(errstr,"Insufficient data in each row, to define oscillator.\n");
		return(DATA_ERROR);
	}
	dz->iparam[PS_ENTRYCNT] = columns_in_row;
	frqcnt = columns_in_row - 1;
	if(ODD(frqcnt)) {
		sprintf(errstr,"amplitude and freq data not correctly paired in rows.\n");
		return(DATA_ERROR);
	}
	dz->iparam[PS_SCNT]       = frqcnt/2;
	dz->iparam[PS_TIMESLOTS] = number_of_rows;
	if(fclose(dz->fp)<0) {
		fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
		fflush(stdout);
	}
	if(dz->parray[PS_FBRK][0] !=0.0) {
		if(flteq(dz->parray[PS_FBRK][0],0.0))
			dz->parray[PS_FBRK][0] = 0.0;
		else {
			dz->iparam[PS_TIMESLOTS]++;
			old_wordcnt = dz->iparam[PS_WORDCNT];
			k = old_wordcnt-1;
			dz->iparam[PS_WORDCNT] += dz->iparam[PS_ENTRYCNT];
			m = dz->iparam[PS_WORDCNT] - 1;
			if((dz->parray[PS_FBRK] = (double *)realloc(dz->parray[PS_FBRK],dz->iparam[PS_WORDCNT] * sizeof(double)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY to reallocate oscillator brktable.\n");
				return(MEMORY_ERROR);
			}
			for(n=0;n<old_wordcnt;n++,m--,k--)
				dz->parray[PS_FBRK][m] = dz->parray[PS_FBRK][k];
			dz->parray[PS_FBRK][0] = 0.0;
			total_wordcnt = dz->iparam[PS_WORDCNT];
		}
	}

	if(dz->parray[PS_FBRK][total_wordcnt - dz->iparam[PS_ENTRYCNT]] < far_time) {
		dz->iparam[PS_TIMESLOTS]++;
		m = dz->iparam[PS_WORDCNT];
		k = m - dz->iparam[PS_ENTRYCNT];
		dz->iparam[PS_WORDCNT] += dz->iparam[PS_ENTRYCNT];
		if((dz->parray[PS_FBRK] = (double *)realloc(dz->parray[PS_FBRK],dz->iparam[PS_WORDCNT] * sizeof(double)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to reallocate oscillator brktable.\n");
			return(MEMORY_ERROR);
		}
		dz->parray[PS_FBRK][m] = far_time;
		m++;
		k++;
		for(n=1;n<dz->iparam[PS_ENTRYCNT];n++,m++,k++)
			dz->parray[PS_FBRK][m] = dz->parray[PS_FBRK][k];
	}
/* NEW CODE */
	return(FINISHED);
}

/**************************** CHECK_SYNTH_DATA *******************************/

int check_synth_data(int wordcnt_in_line,dataptr dz)
{
	int    exit_status;	
	int   n, lastosc, new_total_wordcnt;
	double endtime;
	int    total_wordcnt = dz->iparam[PS_WORDCNT];
	if(dz->parray[PS_FBRK][0] < 0.0) {
		sprintf(errstr,"Negative time value (%lf) on line 1.\n",dz->parray[PS_FBRK][0]);
		return(DATA_ERROR);
	}
	if(flteq(dz->parray[PS_FBRK][0],0.0))		
		dz->parray[PS_FBRK][0] = 0.0;			/* FORCE AN OSCILLATOR SETTING AT TIME ZERO */
	else {				 			
		if((dz->parray[PS_FBRK] = 
		(double *)realloc(dz->parray[PS_FBRK],(total_wordcnt+wordcnt_in_line) * sizeof(double)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to reallocate oscillator brktable.\n");
			return(MEMORY_ERROR);
		}
		for(n=total_wordcnt-1; n>=0; n--)
			dz->parray[PS_FBRK][n + wordcnt_in_line] = dz->parray[PS_FBRK][n];
		total_wordcnt += wordcnt_in_line;
		dz->parray[PS_FBRK][0] = 0.0;
		dz->iparam[PS_TIMESLOTS]++;
	}
	dz->iparam[PS_WORDCNT] = total_wordcnt;
	if((exit_status = check_seq_and_range_of_oscillator_data(dz->parray[PS_FBRK],wordcnt_in_line,dz))<0)
		return(exit_status);
								/* FORCE AN OSCILLATOR SETTING AT (BEYOND) END OF FILE */
	lastosc = total_wordcnt - wordcnt_in_line;
	endtime =  (double)dz->insams[0]/(double)dz->infile->srate;
	if(dz->parray[PS_FBRK][lastosc] <= endtime) {
		if((dz->parray[PS_FBRK] = 
		(double *)realloc(dz->parray[PS_FBRK],(total_wordcnt + wordcnt_in_line) * sizeof(double)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to reallocate oscillator brktable.\n");
			return(MEMORY_ERROR);
		}
		new_total_wordcnt = total_wordcnt + wordcnt_in_line;
		for(n=total_wordcnt;n<new_total_wordcnt;n++)
			dz->parray[PS_FBRK][n] = dz->parray[PS_FBRK][n - wordcnt_in_line];
		dz->parray[PS_FBRK][total_wordcnt] = endtime + 1.0;
		total_wordcnt = new_total_wordcnt;
		dz->iparam[PS_TIMESLOTS]++;
	}
	if(dz->iparam[PS_TIMESLOTS]<2) {
		sprintf(errstr,"Error in timeslot logic: check_synth_data()\n");
		return(PROGRAM_ERROR);
	}
	dz->iparam[PS_WORDCNT] = total_wordcnt;
	return(FINISHED);
}

/**************************** GETMAXLINELEN *******************************/

int getmaxlinelen(int *maxcnt,FILE *fp)
{
	int thiscnt = 0;
	char c;
	*maxcnt = 0;
	while((c= (char)fgetc(fp))!=EOF) {
		if(c=='\n' || c == ENDOFSTR) {
			*maxcnt = max(*maxcnt,thiscnt);
			thiscnt = 0;
		} else
			thiscnt++;			
	}
	*maxcnt = (int)max(*maxcnt,thiscnt);
	*maxcnt += 4;	/* NEWLINE, ENDOFSTR and safety!! */
	return(FINISHED);
}			

/**************************** ALLOCATE_OSC_FRQ_AMP_ARRAYS *******************************/

int allocate_osc_frq_amp_arrays(int fltcnt,dataptr dz)
{
	/*RWD 9:2001 must have empty arrays */
	if((dz->parray[PS_OSCFRQ] = (double *)calloc(fltcnt * sizeof(double),sizeof(char)))==NULL
	|| (dz->parray[PS_OSCAMP] = (double *)calloc(fltcnt * sizeof(double),sizeof(char)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for oscilattor amp and frq arrays.\n");
		return(MEMORY_ERROR);
	}
	return(FINISHED);
}

/**************************** CHECK_SEQ_AND_RANGE_OF_OSCILLATOR_DATA *******************************/

int check_seq_and_range_of_oscillator_data(double *fbrk,int wordcnt_in_line,dataptr dz)
{
	double lasttime = 0.0;
	int n, m, lineno;
	for(n=1;n<dz->iparam[PS_WORDCNT];n++) {
		m = n%wordcnt_in_line;
		lineno = (n/wordcnt_in_line)+1;	/* original line-no : ignoring comments */
		if(m==0) {
			if(fbrk[n] <= lasttime) {
				sprintf(errstr,"Time is out of sequence on line %d\n",lineno);
				return(DATA_ERROR);
			}
			lasttime = fbrk[n];
		} else if(ODD(m)) {
			if(fbrk[n]<dz->application->min_special || fbrk[n]>dz->application->max_special) {
				sprintf(errstr,"frq_or_midi value [%.3lf] out of range (%.1f - %.1f) on line %d\n",
				fbrk[n],dz->application->min_special,dz->application->max_special,lineno);
					return(DATA_ERROR);
			}
			if(dz->mode==1 || dz->mode==3)
				fbrk[n] = miditohz(fbrk[n]);
		} else
			fbrk[n] = max(fbrk[n],0.0);
	}
	return(FINISHED);
}

/**************************** ALLOCATE_TVARYING_OSC_ARRAYS *******************************/

int allocate_tvarying_osc_arrays(dataptr dz)
{
	if((dz->lparray[PS_SAMPTIME] = (int *)calloc(dz->iparam[PS_TIMESLOTS] * sizeof(int),sizeof(char)))==NULL
	|| (dz->parray[PS_INFRQ]     = (double *)calloc(dz->iparam[PS_SCNT] * dz->iparam[PS_TIMESLOTS] * sizeof(double),sizeof(char)))==NULL
	|| (dz->parray[PS_INAMP]     = (double *)calloc(dz->iparam[PS_SCNT] * dz->iparam[PS_TIMESLOTS] * sizeof(double),sizeof(char)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for oscillator coefficients.\n");
		return(MEMORY_ERROR);
	}
	return(FINISHED);
}

/**************************** PUT_TVARYING_OSC_DATA_IN_ARRAYS *******************************/

int put_tvarying_osc_data_in_arrays(double *fbrk,dataptr dz)
{
	int timescnt = 0, freqcnt = 0, ampcnt = 0, n, m;
	int total_frq_cnt = dz->iparam[PS_SCNT] * dz->iparam[PS_TIMESLOTS];
	int entrycnt = dz->iparam[PS_ENTRYCNT];
	int wordcnt  = dz->iparam[PS_WORDCNT];
	if(dz->parray[PS_INFRQ]==NULL) {
		sprintf(errstr,"PS_INFRQ array not established: put_tvarying_osc_data_in_arrays()\n");
		return(PROGRAM_ERROR);
	} 
	if(dz->parray[PS_INAMP]==NULL) {
		sprintf(errstr,"PS_INAMP array not established: put_tvarying_osc_data_in_arrays()\n");
		return(PROGRAM_ERROR);
	} 
	if(dz->lparray[PS_SAMPTIME]==NULL) {
		sprintf(errstr,"PS_SAMPTIME array not established: put_tvarying_osc_data_in_arrays()\n");
		return(PROGRAM_ERROR);
	} 
	for(n=0;n<wordcnt;n++) {
		m = n%entrycnt;
		if(m==0)
			dz->lparray[PS_SAMPTIME][timescnt++] = round(fbrk[n] * dz->infile->srate);
		else if(ODD(m))
			dz->parray[PS_INFRQ][freqcnt++] = fbrk[n];
		else
			dz->parray[PS_INAMP][ampcnt++] = fbrk[n];
	}
	if(freqcnt != total_frq_cnt || ampcnt != freqcnt || timescnt != dz->iparam[PS_TIMESLOTS]) {
		sprintf(errstr,"Oscillator data accounting problem: put_tvarying_osc_data_in_arrays()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/**************************** INITIALISE_PSOWSYNTH_INTERNAL_PARAMS **********************/

int initialise_psowsynth_internal_params(dataptr dz)
{
	int exit_status;
	int n;
	if((exit_status = allocate_osc_frq_amp_arrays(dz->iparam[PS_SCNT],dz))<0)
		return(exit_status);
	for(n = 0;n<dz->iparam[PS_SCNT];n++) {
		dz->parray[PS_OSCFRQ][n]      = dz->parray[PS_INFRQ][n];
		dz->parray[PS_OSCAMP][n]      = dz->parray[PS_INAMP][n];
	}
	dz->iparam[PS_FRQ_INDEX] = dz->iparam[PS_SCNT];
	dz->iparam[PS_TIMES_CNT] = 1;
	return(FINISHED);
}

/**************/
/* PSOW_SPACE */
/**************/

/******************************** SPACECALC ********************************/

void spacecalc(double position,double *leftgain,double *rightgain)
{
	double temp;
	double relpos;
	double reldist;
	if(position < 0) 
		relpos = -position;
	else 
		relpos = position;
	temp = 1.0 + (relpos * relpos);
	reldist = ROOT2 / sqrt(temp);
	temp = (position + 1.0) / 2.0;
	*rightgain = temp * reldist;
	*leftgain = (1.0 - temp ) * reldist;
}

/*******************/
/* PSOW_INTERLEAVE */
/*******************/

/******************************** INSERT_EDGE_CUTS ********************************/

void insert_edge_cuts(int *cuttime,int *cutcnt,int *cutstart,dataptr dz)
{
	int n;
	if(cuttime[(*cutstart)-1] != dz->insams[0]) {		/* force segment end of 1st file */
		for(n=(*cutcnt)-1;n>=*cutstart;n--)								
			cuttime[n+1]  = cuttime[n];	
		(*cutcnt)++;
		cuttime[*cutstart] = 0;
		(*cutstart)++;
	}
	for(n=(*cutcnt)-1;n>=0;n--)							/* force zero at start of 1st file */		
		cuttime[n+1]  = cuttime[n];
	cuttime[0] = 0;
	(*cutcnt)++;
	(*cutstart)++;

	if(cuttime[(*cutcnt)-1] != dz->insams[1]) {			/* force segment end of 2ND file */
		cuttime[(*cutcnt)++] = dz->insams[1];
		(*cutcnt)++;
	}
	for(n=(*cutcnt)-1;n>=(*cutstart);n--)				/* force zero at start of 2ND file */		
		cuttime[n+1]  = cuttime[n];
	(*cutcnt)++;
	cuttime[*cutstart] = 0;
}

/******************************** DO_WEIGHT ********************************/

float *do_weight(int *cnt0,int *cnt1,float *ibuf,float *ibuf3,float *readbuf,dataptr dz)
{
	int weight;
	if(dz->param[PS_WEIGHT] > 1.0) {
		weight = (int)round(dz->param[PS_WEIGHT]);
		if(weight > *cnt0) {
			readbuf = ibuf;
			(*cnt0)++;
			*cnt1 = 0;
		} else {
			readbuf = ibuf3;
			(*cnt1)++;
			*cnt0 = 0;
		}
	} else if(dz->param[PS_WEIGHT] < 1.0) {
		weight = (int)round(1.0/dz->param[PS_WEIGHT]);
		if(weight > *cnt1) {
			readbuf = ibuf3;
			(*cnt1)++;
			*cnt0 = 0;
		} else {
			readbuf = ibuf;
			(*cnt0)++;
			*cnt1 = 0;
		}
	} else if(readbuf == ibuf) {
		readbuf = ibuf3;
		*cnt0 = 0;
		*cnt1 = 1;
	} else {
		readbuf = ibuf;
		*cnt0 = 1;
		*cnt1 = 0;
	}
	return readbuf;
}


#ifdef PSOWTEST
tester(int wherelo,int wherehi,int opos,int here,int n,int j,int *cuttime,int srate)
{
	if((opos >= wherelo) && (opos <= wherehi)) {
		fprintf(stdout,"INFO: opos %d FROM cut[%d] to cut[%d] at %d && %d len = %d time = %lf\n",
		opos,n-j,n,here,cuttime[n],cuttime[n]-here,(double)cuttime[n]/(double)srate);
		fflush(stdout);
	}
}
#endif

/******************************** INTERP_MAXFOFS ********************************/
#ifdef NOTDEF
int interp_maxfofs(int k,int here_in_buf, int there_in_buf,float *ibuf,float *obuf,int *cuttime,
				   int startsamp,double gain,int *opos,int *n,double maxlevel,int cutcnt,int *seglen,dataptr dz)
{
	double maxamp, sum;
	int maxloc, maxwav0start, maxwav0end, maxwav1start, maxwav1end, z, kk, thisseglen;
	int thislen, thatlen, lendiff, totlen, repcnt, i0, i1;
	double lenstep, dlen, dstep0, dstep1, di0, di1;
	double tdiff0, tdiff1, val0, upval, valdiff0, val1, valdiff1;
	int lastfofzero = 0, thisfofzero = 0;

	maxamp = 0.0;
	maxloc = k;
	for(k = here_in_buf; k < there_in_buf;k++) {
		if(fabs(ibuf[k]) > maxamp) {
			maxamp = fabs(ibuf[k]);
			maxloc = k;
		}
	}
	thisseglen = *seglen;
	if(maxamp == 0.0)
		lastfofzero = 1;
	if(!lastfofzero) {
		k = here_in_buf;
		maxwav0start = k;
		while((maxwav0end = next_down_zcross(k,there_in_buf,ibuf))<there_in_buf) {
			if(maxwav0end < 0)
				maxwav0end = there_in_buf;
			if(maxwav0end > maxloc)
				break;
			maxwav0start = k;
			k = maxwav0end;
		}
		for(k = here_in_buf;k < maxwav0start;k++) {
			sum = ibuf[k] * gain;
			obuf[*opos] = (float)(obuf[*opos] + sum);
			maxlevel = max(obuf[*opos],maxlevel);
			(*opos)++;
		}
	}
	z = dz->iparam[PSF_FOFSTR] - 1;
	thisfofzero = 0;
	while(z > 0) {
		here_in_buf = there_in_buf;
		(*n)++;
		if(*n >= cutcnt) {
			there_in_buf = dz->insams[0];
			z = 0;
		} else
			there_in_buf = cuttime[*n] - startsamp;
		thisseglen = there_in_buf - here_in_buf;
		*seglen += thisseglen;
		if(zerofof(here_in_buf,there_in_buf,ibuf))
			thisfofzero = 1;
		if(!thisfofzero) {
			maxamp = 0.0;
			maxloc = here_in_buf;
			for(k = here_in_buf; k < there_in_buf;k++) {
				if(fabs(ibuf[k]) > maxamp) {
					maxamp = fabs(ibuf[k]);
					maxloc = k;
				}
			}
			k = here_in_buf;
			maxwav1start = k;
			while((maxwav1end = next_down_zcross(k,there_in_buf,ibuf))<there_in_buf) {
				if(maxwav1end < 0)
					maxwav1end = there_in_buf;
				if(maxwav1end > maxloc)
					break;
				maxwav1start = k;
				k = maxwav1end;
			}
		}
		if(!lastfofzero  && !thisfofzero) {
			kk = maxwav0start;
			thislen  = maxwav0end - maxwav0start;
			thatlen = maxwav1end - maxwav1start;
			lendiff = thatlen - thislen;
			totlen  = maxwav1start - maxwav0start;
			repcnt = find_repcnt(thislen,thatlen,totlen,dz);
			lenstep = (double)lendiff/(double)repcnt;
			dlen = (double)thislen;
			dstep0 = 1.0;
			dstep1 = (double)thislen/(double)thatlen;
			for(kk=0;kk<repcnt;kk++) {
				di0 = (double)maxwav0start;
				di1 = (double)maxwav1start;
				for(k = 0;k < thislen;k++) {
					i0 = (int)floor(di0);
					i1 = (int)floor(di1);
					tdiff0 = di0 - (double)i0;
					tdiff1 = di1 - (double)i1;
					val0 = (double)ibuf[i0];
					upval = ibuf[i0+1];
					if(i0+1 > maxwav0end-1) {
						if(upval < 0.0)
							upval = 0.0;
					}
					valdiff0 = upval - val0;
					val0 += valdiff0 * tdiff0;
					val1 = (double)ibuf[i1];
					upval = ibuf[i1+1];
					if(i1+1 > maxwav1end-1) {
						if(upval < 0.0)
							upval = 0.0;
					}
					valdiff1 = upval - val1;
					val1 += valdiff1 * tdiff1;
					sum = (val0 + val1)/2.0;
					sum *= gain;
					obuf[*opos] = (float)(obuf[*opos] + sum);
					maxlevel = max(obuf[*opos],maxlevel);
					(*opos)++;
					if(*opos >= dz->buflen * 2) {
						sprintf(errstr,"OVERFLOWED OUTPUT BUFFER.\n");
						return(MEMORY_ERROR);
					}
					di0 += dstep0;
					di1 += dstep1;
				}
				dlen += lenstep;
				thislen = (int)round(dlen);
				dstep0 = (double)(maxwav0end-maxwav0start)/(double)thislen;
				dstep1 = (double)thatlen/(double)thislen;
			}	
			maxwav0start = maxwav1start;
			maxwav0end = maxwav1end;
		} else if(lastfofzero && thisfofzero) {
			*opos += thisseglen;
			if(*opos >= dz->buflen * 2) {
				sprintf(errstr,"OVERFLOWED OUTPUT BUFFER.\n");
				return(MEMORY_ERROR);
			}
		} else if(lastfofzero) {
			for(k = here_in_buf;k < there_in_buf;k++) {
				sum = ibuf[k] * gain;
				obuf[*opos] = (float)(obuf[*opos] + sum);
				maxlevel = max(obuf[*opos],maxlevel);
				(*opos)++;
				if(*opos >= dz->buflen * 2) {
					sprintf(errstr,"OVERFLOWED OUTPUT BUFFER.\n");
					return(MEMORY_ERROR);
				}
			}	
		} else if(thisfofzero) {
			for(k = maxwav0start;k < there_in_buf;k++) {
				sum = ibuf[k] * gain;
				obuf[*opos] = (float)(obuf[*opos] + sum);
				maxlevel = max(obuf[*opos],maxlevel);
				(*opos)++;
				if(*opos >= dz->buflen * 2) {
					sprintf(errstr,"OVERFLOWED OUTPUT BUFFER.\n");
					return(MEMORY_ERROR);
				}
			}
		}
		lastfofzero = thisfofzero;
		z--;
	}
	if(!thisfofzero && !lastfofzero) {
		for(k = maxwav1end;k < there_in_buf;k++) {
			sum = ibuf[k] * gain;
			obuf[*opos] = (float)(obuf[*opos] + sum);
			maxlevel = max(obuf[*opos],maxlevel);
			(*opos)++;
			if(*opos >= dz->buflen * 2) {
				sprintf(errstr,"OVERFLOWED OUTPUT BUFFER.\n");
				return(MEMORY_ERROR);
			}
		}
	}
	return(FINISHED);
}
#endif
/****************************** ESTABLISH_MAXMODE *********************************/

int establish_maxmode(dataptr dz) {
	switch(dz->process) {
	case(PSOW_STRETCH):
	case(PSOW_DUPL):
	case(PSOW_DEL):
	case(PSOW_STRFILL):
	case(PSOW_FREEZE):
	case(PSOW_CHOP):
	case(PSOW_INTERP):
	case(PSOW_IMPOSE):
	case(PSOW_SPLIT):
	case(PSOW_SPACE):
	case(PSOW_INTERLEAVE):
	case(PSOW_REPLACE):
	case(PSOW_EXTEND):
	case(PSOW_LOCATE):
	case(PSOW_EXTEND2):
		dz->maxmode = 0;
		break;
	case(PSOW_FEATURES):
		dz->maxmode = 2;
		break;
	case(PSOW_SYNTH):
		dz->maxmode = 5;
		break;
	case(PSOW_REINF):
	case(PSOW_CUT):
		dz->maxmode = 2;
		break;
	default:
		sprintf(errstr,"FAILED TO FIND MAX MODE.\n");
		return(PROGRAM_ERROR);
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

/**************************** READ_REINFORCEMENT_DATA *******************************/

int read_reinforcement_data(char *filename,dataptr dz)
{
	int n = 0, m, valcnt;
	char temp[200], *p, *thisword;
	double *amp;
	int *hno;
	int is_hno = TRUE;

	if((dz->fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,"Cannot open datafile %s\n",filename);
		return(DATA_ERROR);
	}
	while(fgets(temp,200,dz->fp)!=NULL) {
		p = temp;
		if(is_an_empty_line_or_a_comment(p))
			continue;
		n++;
	}
	if(n==0) {
		sprintf(errstr,"No data in file %s\n",filename);
		return(DATA_ERROR);
	}
	dz->itemcnt = n;
	if ((dz->lparray[0] = (int *)malloc(dz->itemcnt * sizeof(int)))==NULL) {
		sprintf(errstr,"Insufficient memory to store reinforcement harmonics data\n");
		return(MEMORY_ERROR);
	}
	if ((dz->parray[0] = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL) {
		sprintf(errstr,"Insufficient memory to store reinforcement level data\n");
		return(MEMORY_ERROR);
	}
	hno = dz->lparray[0];
	amp = dz->parray[0];
	if(fseek(dz->fp,0,0)<0) {
		sprintf(errstr,"fseek() failed in read_reinforcement_data()\n");
		return(SYSTEM_ERROR);
	}
	n = 0;
	while(fgets(temp,200,dz->fp)!=NULL) {
		p = temp;
		if(is_an_empty_line_or_a_comment(temp))
			continue;
		if(n >= dz->itemcnt) {
			sprintf(errstr,"Accounting problem reading reinforcement_data\n");
			return(PROGRAM_ERROR);
		}
		valcnt = 0;
		while(get_word_from_string(&p,&thisword)) {
			if(valcnt>=2) {
				sprintf(errstr,"Too many values on line %d: file %s\n",n+1,filename);
				return(DATA_ERROR);
			}
			if(is_hno) {
				if(sscanf(thisword,"%d",&(hno[n]))!=1) {
					sprintf(errstr,"Problem reading Harmonic number data: line %d: file %s\n",n+1,filename);
					return(DATA_ERROR);
				}
				if(hno[n]<dz->application->min_special || hno[n]>dz->application->max_special) {
					sprintf(errstr,"harmonic number (%d) on line %d out of range (%d to %d):file %s\n",
					hno[n],n+1,(int)round(dz->application->min_special),(int)round(dz->application->max_special),filename);
					return(DATA_ERROR);
				}
				if(n > 0) {
					for(m=0;m<n;m++) {
						if(hno[m] == hno[n]) {
							sprintf(errstr,"Harmonic number %d is duplicated in file %s\n",hno[m],filename);
							return(DATA_ERROR);
						}
					}
				}
			} else {
				if(sscanf(thisword,"%lf",&(amp[n]))!=1) {
					sprintf(errstr,"Problem reading Gain for harmonic %d: line %d: file %s\n",hno[n],n+1,filename);
					return(DATA_ERROR);
				}
				if(amp[n]<dz->application->min_special2 || amp[n]>dz->application->max_special2) {
					sprintf(errstr,"amp (%lf) out of range (%lf to %.3lf) on line %d: file %s\n",
					amp[n],dz->application->min_special2,dz->application->max_special2,n+1,filename);
					return(DATA_ERROR);
				}
			}
			is_hno = !is_hno;
			valcnt++;
		}
		if(valcnt<2) {
			sprintf(errstr,"Not enough values on line %d: file %s\n",n+1,filename);
			return(DATA_ERROR);
		}
		n++;
	}
	if(fclose(dz->fp)<0) {
		fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
		fflush(stdout);
	}
	return(FINISHED);
}

/**************************** HARMONIC_IS_DUPLICATED *******************************/

int harmonic_is_duplicated(int j,int m,int *hno)
{
	int cnt = 0, zz;
	while(cnt < m) {
		zz = hno[m]/hno[cnt];
		if(zz * hno[cnt] == hno[m]) {	/* harmonic is integer multiple of a previous harmonic */
			if(j % zz == 0)
				return 1;
		}
		cnt++;
	}
	return 0;
}

/**************************** SORT_HARMONICS_TO_ASCENDING_ORDER *******************************/

void sort_harmonics_to_ascending_order(dataptr dz) {
	int *hno, temphno;
	double *amp, tempamp;
	int n, m, subcnt;
	if(dz->itemcnt == 1)
		return;
	hno = dz->lparray[0];
	amp = dz->parray[0];
	subcnt = dz->itemcnt - 1;
	n = 0;
	while(n < subcnt) {
		m = n + 1;
		while(m < dz->itemcnt) {
			if(hno[m] < hno[n]) {
				temphno = hno[n];
				tempamp = amp[n];
				hno[n] = hno[m];
				amp[n] = amp[m];
				hno[m] = temphno;
				amp[m] = tempamp;
			}
			m++;
		}
		n++;
	}
}

/**************************** READ_INHARMONICS_DATA *******************************/

int read_inharmonics_data(char *filename,dataptr dz)
{
	int n = 0, m, valcnt;
	char temp[200], *p, *thisword;
	double *amp, *ihno;
	int is_hno = TRUE;

	if((dz->fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,"Cannot open datafile %s\n",filename);
		return(DATA_ERROR);
	}
	while(fgets(temp,200,dz->fp)!=NULL) {
		p = temp;
		if(is_an_empty_line_or_a_comment(p))
			continue;
		n++;
	}
	if(n==0) {
		sprintf(errstr,"No data in file %s\n",filename);
		return(DATA_ERROR);
	}
	dz->itemcnt = n;
	if ((dz->parray[0] = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL) {
		sprintf(errstr,"Insufficient memory to store reinforcement harmonics data\n");
		return(MEMORY_ERROR);
	}
	if ((dz->parray[1] = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL) {
		sprintf(errstr,"Insufficient memory to store reinforcement level data\n");
		return(MEMORY_ERROR);
	}
	ihno = dz->parray[0];
	amp  = dz->parray[1];
	if(fseek(dz->fp,0,0)<0) {
		sprintf(errstr,"fseek() failed in read_reinforcement_data()\n");
		return(SYSTEM_ERROR);
	}
	n = 0;
	while(fgets(temp,200,dz->fp)!=NULL) {
		p = temp;
		if(is_an_empty_line_or_a_comment(temp))
			continue;
		if(n >= dz->itemcnt) {
			sprintf(errstr,"Accounting problem reading reinforcement_data\n");
			return(PROGRAM_ERROR);
		}
		valcnt = 0;
		while(get_word_from_string(&p,&thisword)) {
			if(valcnt>=2) {
				sprintf(errstr,"Too many values on line %d: file %s\n",n+1,filename);
				return(DATA_ERROR);
			}
			if(is_hno) {
				if(sscanf(thisword,"%lf",&(ihno[n]))!=1) {
					sprintf(errstr,"Problem reading Harmonic number data: line %d: file %s\n",n+1,filename);
					return(DATA_ERROR);
				}
				if(ihno[n]<=dz->application->min_special || ihno[n]>dz->application->max_special) {
					sprintf(errstr,"harmonic number (%lf) on line %d out of range (>%lf to %lf):file %s\n",
					ihno[n],n+1,dz->application->min_special,dz->application->max_special,filename);
					return(DATA_ERROR);
				}
				if(n > 0) {
					for(m=0;m<n;m++) {
						if(flteq(ihno[m],ihno[n])) {
							sprintf(errstr,"Harmonic number %lf is duplicated in file %s\n",ihno[m],filename);
							return(DATA_ERROR);
						}
					}
				}
			} else {
				if(sscanf(thisword,"%lf",&(amp[n]))!=1) {
					sprintf(errstr,"Problem reading Gain for harmonic %lf: line %d: file %s\n",ihno[n],n+1,filename);
					return(DATA_ERROR);
				}
				if(amp[n]<dz->application->min_special2 || amp[n]>dz->application->max_special2) {
					sprintf(errstr,"amp (%lf) out of range (%lf to %.3lf) on line %d: file %s\n",
					amp[n],dz->application->min_special2,dz->application->max_special2,n+1,filename);
					return(DATA_ERROR);
				}
			}
			is_hno = !is_hno;
			valcnt++;
		}
		if(valcnt<2) {
			sprintf(errstr,"Not enough values on line %d: file %s\n",n+1,filename);
			return(DATA_ERROR);
		}
		n++;
	}
	if(fclose(dz->fp)<0) {
		fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
		fflush(stdout);
	}
	return(FINISHED);
}

/********************* CONVERT_PSE_SEMIT_TO_OCTRATIO *********************/

int convert_pse_semit_to_octratio(dataptr dz)
{
	int n, m;
	if(dz->brksize[PSE_TRNS] > 0) {
		for(n=0,m = 1;n < dz->brksize[PSE_TRNS];n++,m+=2)
			dz->brk[PSE_TRNS][m] = pow(2.0,(dz->brk[PSE_TRNS][m]/SEMITONES_PER_OCTAVE));
	} else
		dz->param[PSE_TRNS] = pow(2.0,(dz->param[PSE_TRNS]/SEMITONES_PER_OCTAVE));
	return FINISHED;
}

