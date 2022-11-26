//	This algorithm finds the pitch (if any) in each window,
//	marking no-pitch-found with a -1 value.
//	It discards any windows falling below a threshold amplitude.
//	(It can ,optionally, smooth the output data, and remove "blips"
//	i.e. "pitches lasting for no more than 1 window".
//	but this is not essential, as the statistical survey to follow 
//	should normally eliminate these anomalies.
//
//	It then allots the found pitches to 1/8th semitone bins,
//	and finds the bin with the most entries.
//	This can be done by counting the windows that fall into the bin,
//	or by summing the total-loudnesses of the windows that fall into the bin.
//	The bin with the highest score/weight is taken to be the most prominent pitch.
//
//	Note that, if the pitch is wrong but falls on a prominent harmonic,
//	and the tuning is to a harmonic field with a large number of pitches to tune to,
//	such an error may still lead to an appropriate transposition,
//	as the real root with be transposed along with the prominent harmonic.

#include <stdio.h>
#include <stdlib.h>
#include <structures.h>
#include <tkglobals.h>
#include <pnames.h>
#include <filetype.h>
#include <processno.h>
#include <modeno.h>
#include <arrays.h>
#include <flags.h>
#include <logic.h>
#include <fmnts.h>
#include <formants.h>
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

#ifdef unix
#define round lround
#endif

#define  searchsize rampbrksize

#define TUNING_ARRAY	0
#define TIMING_ARRAY	0
#define FOUND_PITCHES	1
#define TOTALAMPS	    2
#define PITCHWEIGHT	    3
#define WEIGHTEDSCORE	4

#define AMPBINSCNT 32

#define MINSEGLEN (0.03 + FLTERR)		//	2 * 15mS splices and margin-of-error for segments in Loom VBOX 

static double st_intun, st_noise;
static int st_wndws;

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
static int  setup_the_application(dataptr dz);
static int  setup_the_param_ranges_and_defaults(dataptr dz);
static int	check_the_param_validity_and_consistency(dataptr dz);
static int  get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz);
static int  setup_and_init_input_brktable_constants(dataptr dz,int brkcnt);
static int  get_the_mode_no(char *str, dataptr dz);

/* BYPASS LIBRARY GLOBAL FUNCTION TO GO DIRECTLY TO SPECIFIC APPLIC FUNCTIONS */

static int handle_tuning_data(int *cmdlinecnt,char ***cmdline,dataptr dz);
static int handle_segmentation_data(int *cmdlinecnt,char ***cmdline,dataptr dz);
static int fill_tuning_array(dataptr dz);
static int do_spectune(dataptr dz);
static int do_specpedal(dataptr dz);
static int specget(double *pitch,chvptr *partials,dataptr dz);
static int close_to_frq_already_in_ring(chvptr *there,double frq1,dataptr dz);
static int substitute_in_ring(int vc,chvptr here,chvptr there,dataptr dz);
static int insert_in_ring(int vc, chvptr here, dataptr dz);
static int put_ring_frqs_in_ascending_order(chvptr **partials,float *minamp,dataptr dz);
static void find_pitch(double *pitch,chvptr *partials,double lo_loud_partial,double hi_loud_partial,float minamp,dataptr dz);
static int equivalent_pitches(double frq1, double frq2, dataptr dz);
static int is_peak_at(double frq,int window_offset,float minamp,dataptr dz);
static int enough_partials_are_harmonics(chvptr *partials,double pich_pich,dataptr dz);
static int is_a_harmonic(double frq1,double frq2,dataptr dz);
static int local_peak(int thiscc,double frq, float *thisbuf, dataptr dz);
static int spectrnsf(double transpos,dataptr dz);
static int spectrnsp(double transpos,dataptr dz);
static int tranpose_within_formant_envelope(double transpos,int vc,dataptr dz);
static int reposition_partials_in_appropriate_channels(double transpos, dataptr dz);
static int tidy_up_the_pitch_data(double *pitches,int pitchcnt,dataptr dz);
static int test_glitch_backwards(int gltchstart,int gltchend,char *smooth,double max_pglide,int wlength,double *pitches);
static int test_glitch_forwards(int gltchstart,int gltchend,char *smooth,double max_pglide,double *pitches);
static void remove_unsmooth_pitches(char *smooth,int wlength,double *pitches);
static int test_glitch_sets(char *smooth,double max_pglide,int wlength,double *pitches);
static int is_smooth_from_after(int n,char *smooth,double max_pglide,double *pitches);
static int is_smooth_from_before(int n,char *smooth,double max_pglide,double *pitches);
static int is_finalpitch_smooth(char *smooth,double max_pglide,int wlength,double *pitches);
static int is_initialpitch_smooth(char *smooth,double max_pglide,double *pitches);
static int is_smooth_from_both_sides(int n,double max_pglide,double *pitches);
static int anti_noise_smoothing(int wlength,double *pitches,float frametime);
static int pitch_found(double *pitches,int pitchcnt);
static int mark_zeros_in_pitchdata(double *pitches,int pitchcnt,dataptr dz);
static int eliminate_blips_in_pitch_data(double *pitches,int pitchcnt,dataptr dz);
static int alternative_find_pitch(double *pitch,chvptr *partials,dataptr dz);
static int setup_formants(dataptr dz);
static int set_specenv_frqs(int arraycnt,dataptr dz);
static int setup_octaveband_steps(double **interval,dataptr dz);
static int setup_low_octave_bands(int arraycnt,dataptr dz);
extern int special_operation_on_window_zero(dataptr dz);
extern int allocate_single_buffer_plus_peakarray(dataptr dz);
extern int get_totalamp_and_maxamp(double *totalamp,float *maxamp,float *sbuf,int wanted);
extern int get_peaks(dataptr dz) ;
extern int tunepeaks(int *tuned,dataptr dz);

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
	if(dz->mode == 3) {	//	No output file
		if(sloom) {
			cmdlinecnt--;
			cmdline++;
		}
	} else {
		if((exit_status = handle_the_outfile(&cmdlinecnt,&cmdline,is_launched,dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		if(dz->mode != 4 && dz->mode != 5) {
			if(!dz->vflag[2]) {
				//	handle_formants()			formants for pitchshift-with-fixed-formants
				if((exit_status = setup_formants(dz))<0) {
					print_messages_and_close_sndfiles(exit_status,is_launched,dz);
					return(FAILED);
				}
			}
		}
	}
//	handle_formant_quiksearch()	redundant
//	handle_special_data()		redundant except
	if(dz->mode != 3) {
		if(dz->mode > 0) {
			if(dz->mode == 5) {
				if((exit_status = handle_segmentation_data(&cmdlinecnt,&cmdline,dz))<0) {
					print_messages_and_close_sndfiles(exit_status,is_launched,dz);
					return(FAILED);
				}
			} else {
				if((exit_status = handle_tuning_data(&cmdlinecnt,&cmdline,dz))<0) {
					print_messages_and_close_sndfiles(exit_status,is_launched,dz);
					return(FAILED);
				}
			}
		} else {
			if((exit_status = fill_tuning_array(dz))<0) {
				print_messages_and_close_sndfiles(exit_status,is_launched,dz);
				return(FAILED);
			}
		}
	}
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
	dz->extra_bufcnt =  1;	
	if(dz->mode == 4)
		dz->bptrcnt = 4;
	else
		dz->bptrcnt = 1;

	if((exit_status = establish_spec_bufptrs_and_extra_buffers(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if(dz->mode == 4) {
		if((exit_status  = allocate_single_buffer_plus_peakarray(dz))< 0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
	} else {
		if((exit_status  = allocate_single_buffer(dz))< 0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		if((dz->windowbuf[0] = (float *)malloc(dz->wanted * sizeof(float)))==NULL) {
			sprintf(errstr,"Insufficient memory for extra channel-shuffling buffer.\n");
			return(MEMORY_ERROR);
		}
		if(dz->mode == 5)
			dz->searchsize = dz->wlength;
		else
			dz->searchsize = dz->iparam[ST_ETIME] - dz->iparam[ST_STIME] + 1;

		if ((dz->parray[FOUND_PITCHES] = (double *)malloc((dz->searchsize + 4) * sizeof(double)))==NULL) {
			sprintf(errstr,"Insufficient memory to store found pitches.\n");
			return(MEMORY_ERROR);
		}
		if ((dz->parray[TOTALAMPS] = (double *)malloc((dz->searchsize + 4) * sizeof(double)))==NULL) {
			sprintf(errstr,"Insufficient memory to store window amplitudes.\n");
			return(MEMORY_ERROR);
		}
		if ((dz->parray[PITCHWEIGHT] = (double *)malloc((dz->searchsize + 4) * sizeof(double)))==NULL) {
			sprintf(errstr,"Insufficient memory to store pitched-window amplitudes.\n");
			return(MEMORY_ERROR);
		}
		if ((dz->parray[WEIGHTEDSCORE] = (double *)malloc((dz->searchsize + 4) * sizeof(double)))==NULL) {
			sprintf(errstr,"Insufficient memory to store pitches amplitude weightings.\n");
			return(MEMORY_ERROR);
		}
		if ((dz->iparray[0] = (int *)malloc((dz->searchsize + 4) * sizeof(int)))==NULL) {
			sprintf(errstr,"Insufficient memory for pitch bins.\n");
			return(MEMORY_ERROR);
		}
		if ((dz->iparray[1] = (int *)malloc((dz->searchsize + 4) * sizeof(int)))==NULL) {
			sprintf(errstr,"Insufficient memory for pitch bin counters.\n");
			return(MEMORY_ERROR);
		}

		//param_preprocess()						redundant
		if((exit_status = setup_ring(dz) )<0) {										// CDP LIB
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
	}
	//spec_process_file =
	if(dz->mode==4)
		exit_status  = do_specpedal(dz);
	else
		exit_status  = do_spectune(dz);
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
	dz->infilecnt = 1;
	//establish_bufptrs_and_extra_buffers():
	dz->array_cnt=5;
	if((dz->parray  = (double **)malloc(dz->array_cnt * sizeof(double *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for internal double arrays.\n");
		return(MEMORY_ERROR);
	}
	dz->iarray_cnt=2;
	if((dz->iparray  = (int **)malloc(dz->iarray_cnt * sizeof(int *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for internal integer arrays.\n");
		return(MEMORY_ERROR);
	}
	dz->parray[TUNING_ARRAY] = NULL;
	dz->parray[FOUND_PITCHES] = NULL;
	dz->parray[2] = NULL;
	dz->iparray[0] = NULL;
	dz->iparray[1] = NULL;
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
	switch(dz->mode) {
	case(5):	exit_status = set_param_data(ap,SEGMENTLIST,0,0,""); break;
	case(4):	exit_status = set_param_data(ap,TUNINGLIST,1,1,"i"); break;
	case(3):	// fall thro
	case(0):	exit_status = set_param_data(ap,0		  ,0,0,"");	break;
	case(1):	exit_status = set_param_data(ap,TUNINGLIST,0,0,"");	break;
	case(2):	exit_status = set_param_data(ap,TUNINGLIST,0,0,"");	break;
	}
	if(exit_status < 0)
		return(FAILED);
	switch(dz->mode) {
	case(5):	exit_status = set_vflgs(ap,"mlhiwn",6,"idddid","rb" ,2,0,"00");	 break;
	case(4):	exit_status = set_vflgs(ap,"",0,"","" ,0,0,"");	 break;
	case(3):	exit_status = set_vflgs(ap,"mlhseiwn",8,"idddddid","rb" ,2,0,"00");	 break;
	default:	exit_status = set_vflgs(ap,"mlhseiwn",8,"idddddid","rbf",3,0,"000"); break;
	}
	if(exit_status < 0)
		return(FAILED);
	// set_legal_infile_structure -->
	dz->has_otherfile = FALSE;
	// assign_process_logic -->
	dz->input_data_type = ANALFILE_ONLY;
	if(dz->mode == 3) {
		dz->process_type	= OTHER_PROCESS;	
		dz->outfiletype  	= NO_OUTPUTFILE;
	} else if(dz->mode == 5) {
		dz->process_type	= TO_TEXTFILE;	
		dz->outfiletype  	= TEXTFILE_OUT;
	} else {
		dz->process_type	= EQUAL_ANALFILE;	
		dz->outfiletype  	= ANALFILE_OUT;
	}
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
	if(dz->mode == 4) {
		ap->lo[ST_CNT]				= 1;
		ap->hi[ST_CNT]				= 128;
		ap->default_val[ST_CNT]	= 32;
	} else {
		ap->lo[ST_MATCH]			= 1;
		ap->hi[ST_MATCH]			= 8;
		ap->default_val[ST_MATCH]	= ST_ACCEPTABLE_MATCH;
		ap->lo[ST_LOPCH]			= 4;
		ap->hi[ST_LOPCH]			= 127;
		ap->default_val[ST_LOPCH]	= 4;
		ap->lo[ST_HIPCH]    		= 4;
		ap->hi[ST_HIPCH]    		= 127;
		ap->default_val[ST_HIPCH]	= 127;
		if (dz->mode == 5) {
			ap->lo[ST_INTUN5]			= 0;
			ap->hi[ST_INTUN5]			= 6;
			ap->default_val[ST_INTUN5]	= 1;
			ap->lo[ST_WNDWS5]			= 0;
			ap->hi[ST_WNDWS5]			= dz->wlength;
			ap->default_val[ST_WNDWS5]	= BLIPLEN;
			ap->lo[ST_NOISE5]			= 0.0;
			ap->hi[ST_NOISE5]			= SIGNOIS_MAX;
			ap->default_val[ST_NOISE5]	= SILENCE_RATIO;
		} else {
			ap->lo[ST_STIME]			= 0;
			ap->hi[ST_STIME]			= dz->duration;
			ap->default_val[ST_STIME]	= 0;
			ap->lo[ST_ETIME]			= 0;
			ap->hi[ST_ETIME]			= dz->duration;
			ap->default_val[ST_ETIME]	= dz->duration;
			ap->lo[ST_INTUN]			= 0;
			ap->hi[ST_INTUN]			= 6;
			ap->default_val[ST_INTUN]	= 1;
			ap->lo[ST_WNDWS]			= 0;
			ap->hi[ST_WNDWS]			= dz->wlength;
			ap->default_val[ST_WNDWS]	= BLIPLEN;
			ap->lo[ST_NOISE]			= 0.0;
			ap->hi[ST_NOISE]			= SIGNOIS_MAX;
			ap->default_val[ST_NOISE]	= SILENCE_RATIO;
		}
	}
	dz->maxmode = 6;
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
	int n, m, lolim;
	int warning_given = 0;

	if(dz->mode == 4)
		return FINISHED;

	if(dz->param[ST_LOPCH] >= dz->param[ST_HIPCH]) {
		sprintf(errstr,"Upper and lower pitch limits are not compatible.\n");
		return(DATA_ERROR);
	}
	if(dz->mode != 3 && dz->mode != 5) {
		if(dz->mode == 1) {

			//	FOR HARMONIC SET CASE

			for(n = 0;n < dz->itemcnt; n++) {
				if(dz->parray[TUNING_ARRAY][n] < dz->param[ST_LOPCH] || dz->parray[TUNING_ARRAY][n] > dz->param[ST_HIPCH]) {
					if(!warning_given) {
						fprintf(stdout,"WARNING: TUNING PITCH %lf IS OUTSIDE THE SPECIFIED RANGE (%lf to %lf)\n",
							dz->parray[TUNING_ARRAY][n],dz->param[ST_LOPCH],dz->param[ST_HIPCH]);
						fflush(stdout);
						warning_given = 1;
					}
				}
			}
		} else {

			//	FOR HARMONIC FIELD AND TEMPERED SCALE CASES : RESTRICT TUNING-PITCHES INSIDE SPECIFIED RANGE

			for(n = 0;n < dz->itemcnt; n++) {
				if(dz->parray[TUNING_ARRAY][n] >= dz->param[ST_LOPCH])
					break;
			}
			lolim = n;
			for(n = lolim;n < dz->itemcnt; n++) {
				if(dz->parray[TUNING_ARRAY][n] > dz->param[ST_HIPCH])
					break;
			}
			dz->itemcnt = n;			//	COnstrict MIDI search array from top (if ness)

			if(lolim > 0) {				//	Shuffle array down
				for(n = lolim, m = 0; n < dz->itemcnt; n++,m++) 
					dz->parray[TUNING_ARRAY][m] = dz->parray[TUNING_ARRAY][n];
				dz->itemcnt -= lolim;	//	Constrict MIDI search array from bottom
			}
		} 
	}
	if (dz->mode != 5) {
		if(dz->param[ST_STIME] >= dz->param[ST_ETIME]) {
			sprintf(errstr,"Upper and lower searchtime limits are not compatible.\n");
			return(DATA_ERROR);
		}
		dz->iparam[ST_STIME] = (int)(round(dz->param[ST_STIME]/dz->frametime));		//	Convert search limits from seconds to windowcount
		dz->iparam[ST_ETIME] = (int)(round(dz->param[ST_ETIME]/dz->frametime));
	}
	if (dz->mode == 5) {
		st_intun = pow(SEMITONE_INTERVAL,fabs(dz->param[ST_INTUN5]));	//	Convert in-tune semitones to frq ratio 
		st_noise = dz->param[ST_NOISE5];
		st_wndws = dz->iparam[ST_WNDWS];
	} else {
		st_intun = pow(SEMITONE_INTERVAL,fabs(dz->param[ST_INTUN]));	//	Convert in-tune semitones to frq ratio 
		st_noise = dz->param[ST_NOISE];
		st_wndws = dz->iparam[ST_WNDWS];
	}
	st_noise /= 20.0;													//  convert sig-to-nois ratio from dB to gain
	st_noise = pow(10.0,st_noise);
	st_noise = 1.0/st_noise;
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
	if      (!strcmp(prog_identifier_from_cmdline,"tune"))			dz->process = SPECTUNE;
	else {
		fprintf(stderr,"Unknown program identification string '%s'\n",prog_identifier_from_cmdline);
		return(USAGE_ONLY);
	}
	return(FINISHED);
}

/******************************** USAGE1 ********************************/

int usage1(void)
{
	fprintf(stderr,
	"\nFURTHER OPERATIONS ON ANALYSIS FILES\n\n"
	"USAGE: spectune tune mode infile (outfile) parameters: \n"
	"\n"
	"Type 'spectune tune' for more info.\n");
	return(USAGE_ONLY);
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
	if(!strcmp(str,"tune")) {
		fprintf(stdout,
	    "USAGE: spectune tune 1 infile outfile\n"
		"[-mmatch] [-llop] [-hhip] [-stim] [-etim] [-iintune] [-wwins] [-nnois] [-r][-b][-f]\n"
		"OR:    spectune tune 2-3 infile outfile tuning\n"
		"[-mmatch] [-llop] [-hhip] [-stim] [-etim] [-iintune] [-wwins] [-nnois] [-r][-b][-f]\n"
		"OR:    spectune tune 4 infile outfile\n"
		"[-mmatch] [-llop] [-hhip] [-stim] [-etim] [-iintune] [-wwins] [-nnois] [-r][-b]\n"
		"OR:    spectune tune 5 infile outfile tuning peakcnt\n"
		"OR:    spectune tune 6 infile outfile segmentation\n"
		"[-mmatch] [-llop] [-hhip] [-iintune] [-wwins] [-nnois] [-r][-b]\n"
		"\n"
		"Find (most prominent) pitch in input file\n"
		"then transpose input file to ....\n"
		"MODE 1: nearest tempered pitch.\n"
		"MODE 2: nearest of pitches listed in \"tuning\" file.\n"
		"MODE 3: nearest pitch, or its octave equivalents, listed in \"tuning\" file.\n"
		"MODE 4: Report the pitch found : no sound output.\n"
		"MODE 5: Tune sound to a given (possibly time-varying) pedal pitch and its harmonics.\n"
		"MODE 6: Report median pitch in all segments indicated.\n"
		"\n"
		"MATCH   No of partials lying on harmonic series, to affirm pitch (Default %d).\n"
		"LOP     Minimum (MIDI) pitch to accept (default : 4 = c 10Hz).\n"
		"HIP     Maximum (MIDI) pitch to accept (default : 127).\n"
		"STIM    Time in file where pitch search begins (default : 0.0).\n"
		"ETIM    Time in file where pitch search ends (default : end-of-file).\n"
		"INTUNE  How closely tuned must harmonics be (in semitones) (Default 1).\n"
		"WINS    Minimum no consective pitched windows, to confirm pitch found (Default %d).\n"
		"NOIS    Sig-to-noise ratio (dB). (Default %.0lf db).\n"
		"        Windows more than \"NOIS\" dB below loudest window are ignored.\n"
		"PEAKCNT How many spectral peaks to tune (1 - 128 max).\n"
		"\n"
		"-r  Ignore relative loudness of pitched windows when assessing most prominent pitch.\n"
		"-b  Smooth pitch data & eliminate blips, before searching for most prominent pitch.\n"
		"-f  Ignore the formant envelope, when transposing.\n"
		"\n"
		"TUNING  Textfile list of (possibly fractional) MIDI pitches.\n"
		"\n"
		"SEGMENTATION  Times of starts/ends of segments in input sound.\n"
		"\n"
		"Input (and output sound) files are analysis files.\n",ST_ACCEPTABLE_MATCH,BLIPLEN,SILENCE_RATIO);
	} else
		fprintf(stdout,"Unknown option '%s'\n",str);
	return(USAGE_ONLY);
}

int usage3(char *str1,char *str2)
{
	fprintf(stderr,"Insufficient parameters on command line.\n");
	return(USAGE_ONLY);
}

/**************************** DO_SPECTUNE ****************************/

int do_spectune(dataptr dz)
{
	int exit_status, wc, done, windows_in_buf, median;
	int n,m, medianscore, medianpos, sumcnt, samps_read, pitchcnt = 0, startwin, endwin, j, k, mediancnt, tdiff;
	double *tunings = dz->parray[TUNING_ARRAY], *pitches = dz->parray[FOUND_PITCHES], medianweight, *timings = NULL;
	double *pitchweight = dz->parray[PITCHWEIGHT], *weightedscore = dz->parray[WEIGHTEDSCORE], *windowamp = dz->parray[TOTALAMPS],*medianpitches = NULL;
	int	*pitchbin = dz->iparray[0], *pitchscore = dz->iparray[1];
	double pitch, sum, medianpitch, diff, nudiff, transpos, totalamp = 0.0, pitchstep;
	chvptr *partials;
	int st_stime, st_etime;
	if(dz->mode == 5) {
		st_stime = 0;
		st_etime = dz->wlength;
	} else {
		st_stime = dz->iparam[ST_STIME];
		st_etime = dz->iparam[ST_ETIME];
	}
	done = 0;
	if((partials = (chvptr *)malloc(MAXIMI * sizeof(chvptr)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for partials array.\n");
		return(MEMORY_ERROR);
	}
	while((samps_read = fgetfbufEx(dz->bigfbuf, dz->buflen,dz->ifd[0],0)) > 0) {
		if(samps_read < 0) {
			sprintf(errstr,"Failed to read data from input file.\n");
			return(SYSTEM_ERROR);
		}
		dz->flbufptr[0] = dz->bigfbuf;
		windows_in_buf = samps_read/dz->wanted;    
		for(wc=0; wc<windows_in_buf; wc++, dz->total_windows++) {
			if(dz->total_windows < st_stime) {
				dz->flbufptr[0] += dz->wanted;
				continue;
			}
			if(dz->total_windows > st_etime) {
				done = 1;
				break;
			}
			if((exit_status = get_totalamp(&totalamp,dz->flbufptr[0],dz->wanted))<0)
				return(exit_status);
			dz->parray[TOTALAMPS][pitchcnt] = totalamp;
			if(dz->total_windows==0)
				pitches[pitchcnt++] = 0.0;		//	First window has no pitch
			else {
				if((exit_status = specget(&pitch,partials,dz))<0)
					return(exit_status);
				pitches[pitchcnt++] = pitch;
			}
			dz->flbufptr[0] += dz->wanted;
		}
		if(done)
			break;
	}

	//	CLEAN UP PITCH DATA

	if((exit_status = tidy_up_the_pitch_data(pitches,pitchcnt,dz))<0)
		return exit_status;

	//	ASSIGN FOUND PITCHES TO 1/8-TONE BANDS

	for(n=0;n<pitchcnt;n++) {
		if(pitches[n] > 0) {
			if(hztomidi(&pitch,pitches[n]) < 0) {
				pitch = -1.0;
			}
		} else
			pitch = -1.0;
		pitches[n] = pitch;
	}
	
	if(dz->mode == 5) {
		timings = dz->parray[TIMING_ARRAY];
		if ((medianpitches = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL) {
			sprintf(errstr,"Insufficient memory to store tunings data.\n");
			return(MEMORY_ERROR);
		}
	
		startwin = 0;
		for(k=1;k<dz->itemcnt;k++) {
			endwin   = (int)round(timings[k]/dz->frametime);
			for(n = startwin;n<endwin;n++) {
				//	IF pitch is out of range (or marked as no-pitch-found i.e. <= 0.0) set bin to zero
				if(pitches[n] < dz->param[ST_LOPCH] || pitches[n] > dz->param[ST_HIPCH]) {
					pitchbin[n] = 0;
					pitchweight[n] = -1;
				}
				else {	//	ELSE assign pitch to nearest 8th-tone bin : NB values are stored as an integer count of 1/8th tones
					pitchbin[n] = (int)(round(pitches[n] * 8.0));
					pitchweight[n] = windowamp[n];
				}
				pitchscore[n] = 0;	//	Flag all bins as not counted
				weightedscore[n] = 0.0;
			}

			if(dz->vflag[0]) {

				//	COUNT THE PITCH CLASSES

				for(n=startwin;n<endwin;n++) {
					if(pitchscore[n])			//	If pitchbin marked as already counted (non-zero), go to next item
						continue;
					if(pitchbin[n] == 0) {
						pitchscore[n] = -1;	//	If pitchbin is not pitched (or not in range) mark as already counted
						continue;
					}
					pitchscore[n] = 1;		//	Start counting
					for(m=n+1;m<endwin;m++) {
						if(pitchscore[m])		//	Marked as already counted, go to next item
							continue;
						if(pitchbin[m] == 0) {	
							pitchscore[m] = -1;	//	If pitchbin is not pitched (or not in range) mark as already counted
							continue;
						}
						if(pitchbin[n] == pitchbin[m]) {
							pitchscore[n]++;	//	Count recurrences of value in bin[n]
							pitchscore[m] = -1;	//	Flag other occurence as already counted
						}
					}
				}

				//	FIND THE MEDIAN PITCHBIN

				medianscore = -1;
				medianpos = -1;
				for(n=startwin;n<endwin;n++) {
					if(pitchscore[n] > medianscore) {
						medianscore = pitchscore[n];
						medianpos = n;
					}
				}
				if(medianpos < 0)
					medianpitches[k-1] = -1;
				else 
					medianpitches[k-1] = pitchbin[medianpos];

			} else {

			//	WEIGH THE PITCH CLASSES

				for(n=startwin;n<endwin;n++) {
					if(weightedscore[n] > 0.0)	//	If pitchbin marked as already weighed (non-negative), go to next item
						continue;
					if(pitchbin[n] == 0) {
						weightedscore[n] = -1;	//	If pitchbin is not pitched (or not in range) mark as not-to-be-weighed
						continue;
					}
					weightedscore[n] = pitchweight[n];	//	Start weighing
					for(m=n+1;m<endwin;m++) {
						if(weightedscore[m] > 0.0)		//	Marked as already weighed, go to next item
							continue;
						if(pitchbin[m] == 0) {	
							weightedscore[m] = -1;		//	If pitchbin is not pitched (or not in range) mark as not-to-be-weighed
							continue;
						}
						if(pitchbin[n] == pitchbin[m]) {
							weightedscore[n] += pitchweight[m];	//	Add weight of recurrencing pitch value to bin[n]
							weightedscore[m] = -1;				//	Flag other occurence as already weighed
						}
					}
				}

				//	FIND THE MEDIAN PITCHWEIGHT

				medianweight = -1;
				medianpos = -1;
				for(n=startwin;n<endwin;n++) {
					if(weightedscore[n] > medianweight) {
						medianweight = weightedscore[n];
						medianpos = n;
					}
				}
			}
			if(medianpos < 0)
				median = -1;		//	No median pitch found
			else 
				median = pitchbin[medianpos];

			
			//	AVERAGE ALL PITCHES FALLING IN THE MEDIAN PITCHBIN
			
			if(median < 0)
				medianpitches[k-1] = -1;
			else {
				sum = 0.0;
				sumcnt = 0;
				for(n=startwin;n<endwin;n++) {
					if(pitchbin[n] == median) {
						sum += pitches[n];
						sumcnt++;
					}
				}
				medianpitches[k-1] = sum/(double)sumcnt;
			}
			startwin = endwin;
		}
		j = 0;
		mediancnt = dz->itemcnt - 1;				//	1 less segment than segment boundaries
		for(k = 0; k < mediancnt;k++) {
			if(medianpitches[k] < 0.0)
				j++;
		}
		if(j == mediancnt) {
			sprintf(errstr,"NO MEDIAN PITCHES FOUND.\n");
			return DATA_ERROR;
		}
		if(medianpitches[0] < 0.0) {				//	If initial pitch null
			k = 1;
			while(k < mediancnt) {					//	Find first valid pitch
				if(medianpitches[k] >= 0) {
					j = 0;							//	Copy this to all prior segments
					while(j < k) {
						medianpitches[j] = medianpitches[k];
						j++;
					}
					break;
				}
				k++;
			}
		}
		if(medianpitches[mediancnt - 1] < 0.0) {	//	If last pitch null
			k = mediancnt - 2;
			while(k >= 0) {							//	Find last valid pitch
				if(medianpitches[k] >= 0) {
					j = k+1;						//	Copy this to all later segments
					while(j < mediancnt) {
						medianpitches[j] = medianpitches[k];
						j++;
					}
					break;
				}
				k--;
			}
		}

		for(k = 0; k < mediancnt;k++) {
			if(medianpitches[k] < 0.0) {							//	Find any internal null pitches, and interp across
				j = k + 1;
				while(medianpitches[j] < 0.0)
					j++;														 
				pitchstep = medianpitches[j] - medianpitches[k-1];	//	    P1 x P2 -> P1 + 1/2diff
				tdiff = j - (k-1);									//	  P1 x x P2	-> P1 + 1/3diff
				pitchstep *= 1.0/(double)tdiff;						//	P1 x x x P2 -> P1 + 1/4diffETC
				medianpitches[k] = medianpitches[k-1] + pitchstep;
			}
		}
		for(k = 0; k < mediancnt;k++)
			fprintf(dz->fp,"%lf\n",medianpitches[k]);
		return FINISHED;
	
	} else {

		for(n=0;n<pitchcnt;n++) {
					//	IF pitch is out of range (or marked as no-pitch-found i.e. <= 0.0) set bin to zero
			if(pitches[n] < dz->param[ST_LOPCH] || pitches[n] > dz->param[ST_HIPCH]) {
				pitchbin[n] = 0;
				pitchweight[n] = -1;
			}
			else {	//	ELSE assign pitch to nearest 8th-tone bin : NB values are stored as an integer count of 1/8th tones
				pitchbin[n] = (int)(round(pitches[n] * 8.0));
				pitchweight[n] = windowamp[n];
			}
			pitchscore[n] = 0;	//	Flag all bins as not counted
			weightedscore[n] = 0.0;
		}

		if(dz->vflag[0]) {

			for(n=0;n<pitchcnt;n++) {
						//	IF pitch is out of range (or marked as no-pitch-found i.e. <= 0.0) set bin to zero
				if(pitches[n] < dz->param[ST_LOPCH] || pitches[n] > dz->param[ST_HIPCH]) {
					pitchbin[n] = 0;
					pitchweight[n] = -1;
				}
				else {	//	ELSE assign pitch to nearest 8th-tone bin : NB values are stored as an integer count of 1/8th tones
					pitchbin[n] = (int)(round(pitches[n] * 8.0));
					pitchweight[n] = windowamp[n];
				}
				pitchscore[n] = 0;	//	Flag all bins as not counted
				weightedscore[n] = 0.0;
			}

			//	COUNT THE PITCH CLASSES

			for(n=0;n<pitchcnt;n++) {
				if(pitchscore[n])			//	If pitchbin marked as already counted (non-zero), go to next item
					continue;
				if(pitchbin[n] == 0) {
					pitchscore[n] = -1;	//	If pitchbin is not pitched (or not in range) mark as already counted
					continue;
				}
				pitchscore[n] = 1;		//	Start counting
				for(m=n+1;m<pitchcnt;m++) {
					if(pitchscore[m])		//	Marked as already counted, go to next item
						continue;
					if(pitchbin[m] == 0) {	
						pitchscore[m] = -1;	//	If pitchbin is not pitched (or not in range) mark as already counted
						continue;
					}
					if(pitchbin[n] == pitchbin[m]) {
						pitchscore[n]++;	//	Count recurrences of value in bin[n]
						pitchscore[m] = -1;	//	Flag other occurence as already counted
					}
				}
			}

			//	FIND THE MEDIAN PITCHBIN

			medianscore = -1;
			medianpos = -1;
			for(n=0;n<pitchcnt;n++) {
				if(pitchscore[n] > medianscore) {
					medianscore = pitchscore[n];
					medianpos = n;
				}
			}
			if(medianpos < 0) {
				sprintf(errstr,"NO MEDIAN PITCH FOUND.\n");
				return(DATA_ERROR);
			}
			median = pitchbin[medianpos];

		} else {

		//	WEIGH THE PITCH CLASSES

			for(n=0;n<pitchcnt;n++) {
						//	IF pitch is out of range (or marked as no-pitch-found i.e. <= 0.0) set bin to zero
				if(pitches[n] < dz->param[ST_LOPCH] || pitches[n] > dz->param[ST_HIPCH]) {
					pitchweight[n] = -1;
				}
				else {	//	ELSE assign pitch to nearest 8th-tone bin : NB values are stored as an integer count of 1/8th tones
					pitchbin[n] = (int)(round(pitches[n] * 8.0));
					pitchweight[n] = windowamp[n];
				}
				weightedscore[n] = 0.0;		// Reset all weights to zero
			}

			for(n=0;n<pitchcnt;n++) {
				if(weightedscore[n] > 0.0)	//	If pitchbin marked as already weighed (non-negative), go to next item
					continue;
				if(pitchbin[n] == 0) {
					weightedscore[n] = -1;	//	If pitchbin is not pitched (or not in range) mark as not-to-be-weighed
					continue;
				}
				weightedscore[n] = pitchweight[n];	//	Start weighing
				for(m=n+1;m<pitchcnt;m++) {
					if(weightedscore[m] > 0.0)		//	Marked as already weighed, go to next item
						continue;
					if(pitchbin[m] == 0) {	
						weightedscore[m] = -1;		//	If pitchbin is not pitched (or not in range) mark as not-to-be-weighed
						continue;
					}
					if(pitchbin[n] == pitchbin[m]) {
						weightedscore[n] += pitchweight[m];	//	Add weight of recurrencing pitch value to bin[n]
						weightedscore[m] = -1;				//	Flag other occurence as already weighed
					}
				}
			}

			//	FIND THE MEDIAN PITCHWEIGHT

			medianweight = -1;
			medianpos = -1;
			for(n=0;n<pitchcnt;n++) {
				if(weightedscore[n] > medianweight) {
					medianweight = weightedscore[n];
					medianpos = n;
				}
			}
			if(medianpos < 0) {
				sprintf(errstr,"NO MEDIAN PITCH FOUND.\n");
				return(DATA_ERROR);
			}
			median = pitchbin[medianpos];
		}
		
		//	AVERAGE ALL PITCHES FALLING IN THE MEDIAN PITCHBIN
		
		sum = 0.0;
		sumcnt = 0;
		for(n=0;n<pitchcnt;n++) {
			if(pitchbin[n] == median) {
				sum += pitches[n];
				sumcnt++;
			}
		}
		medianpitch = sum/(double)sumcnt;
	}

	if(dz->mode == 3) {
		fprintf(stdout,"INFO: %lf\n",medianpitch);
		fflush(stdout);
		return FINISHED;
	}
	
	//	FIND CLOSEST PITCH IN TUNING SET

	
	diff = fabs(medianpitch - tunings[0]);
	for(n = 1;n < dz->itemcnt;n++) {
		nudiff = fabs(medianpitch - tunings[n]);
		if(nudiff > diff)
			break;
		diff = nudiff;
	}
	n--;

	transpos = miditohz(tunings[n])/miditohz(medianpitch);

	fprintf(stdout,"INFO: Transposing from MIDI %.2lf to %lf : by ratio %lf\n",medianpitch,tunings[n],transpos);
	fflush(stdout);
	
	//	TRANSPOSE THE DATA

	if(sndseekEx(dz->ifd[0],0,0)<0) {
		sprintf(errstr,"SEEK FAILED BEFORE TRANSPOSITION STARTED.\n");
		return(DATA_ERROR);
	}

	dz->total_windows = 0;
	while((samps_read = fgetfbufEx(dz->bigfbuf, dz->buflen,dz->ifd[0],0)) > 0) {
		if(samps_read < 0) {
			sprintf(errstr,"Failed to re-read data from input file, to do transposition.\n");
			return(SYSTEM_ERROR);
		}
		dz->flbufptr[0] = dz->bigfbuf;
		windows_in_buf = samps_read/dz->wanted;    
		
		for(wc=0; wc<windows_in_buf; wc++, dz->total_windows++) {
			if(dz->total_windows==0) 		//	First window has no pitch
				special_operation_on_window_zero(dz);
			else {
				if(dz->vflag[2]) {
					if((exit_status = spectrnsp(transpos,dz))<0)
						return(exit_status);
				} else {
					if((exit_status = spectrnsf(transpos,dz))<0)
						return(exit_status);
				}
			}
			if((exit_status = write_samps(dz->flbufptr[0],dz->wanted,dz))<0)
				return(exit_status);
			dz->flbufptr[0] += dz->wanted;
		}
	}
	return FINISHED;
}

/************************ GET_THE_MODE_NO *********************/

int get_the_mode_no(char *str, dataptr dz)
{
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

/************************ HANDLE_TUNING_DATA *********************/

int handle_tuning_data(int *cmdlinecnt,char ***cmdline,dataptr dz)
{
	int oct;
	aplptr ap = dz->application;
	int n, m, total;
	char temp[200], *q, *filename = (*cmdline)[0];
	double *p, tempval, midival;
	if(!sloom) {
		if(*cmdlinecnt <= 0) {
			sprintf(errstr,"Insufficient parameters on command line.\n");
			return(USAGE_ONLY);
		}
	}
	ap->data_in_file_only 	= TRUE;
	ap->special_range 		= TRUE;
	ap->min_special 		= 4;
	ap->max_special 		= 127;
	if((dz->fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,"Cannot open datafile %s\n",filename);
		return(DATA_ERROR);
	}
	n = 0;
	while(fgets(temp,200,dz->fp)!=NULL) {
		q = temp;
		if(is_an_empty_line_or_a_comment(q))
			continue;
		n++;
	}
	if(n==0) {
		sprintf(errstr,"No data in file %s\n",filename);
		return(DATA_ERROR);
	}
	dz->itemcnt = n;
	if(dz->mode == 2)
		dz->itemcnt *= 12;	//	Only need 11 octaves (12 for safety)
	if ((dz->parray[TUNING_ARRAY] = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL) {
		sprintf(errstr,"Insufficient memory to store tuning data\n");
		return(MEMORY_ERROR);
	}
	p = dz->parray[TUNING_ARRAY];
	if(fseek(dz->fp,0,0)<0) {
		sprintf(errstr,"fseek() failed in handle_pitchdata()\n");
		return(SYSTEM_ERROR);
	}
	n = 0;
	while(fgets(temp,200,dz->fp)!=NULL) {
		q = temp;
		if(is_an_empty_line_or_a_comment(temp))
			continue;
		if(n >= dz->itemcnt) {
			sprintf(errstr,"Accounting problem reading pitchdata\n");
			return(PROGRAM_ERROR);
		}
		while(get_float_from_within_string(&q,p)) {
			if(*p <= ap->min_special || *p >= ap->max_special) {
				sprintf(errstr,"Pitch out of range (%lf - %lf) : line %d: file %s\n",ap->min_special,ap->max_special,n+1,filename);
				return(DATA_ERROR);
			}
			p++;
		}
		n++;
	}
	if(fclose(dz->fp)<0) {
		fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
		fflush(stdout);
	}

	total = n;

	if(dz->mode == 2) {

		//	FOR HARMONIC-FIELD CASE : TRANSPOSE ALL PITCHES INTO LOWEST OCTAVE

		for(n = 0; n < total; n++) {
			while(dz->parray[TUNING_ARRAY][n] > ap->min_special)
				dz->parray[TUNING_ARRAY][n] -= 12;
			dz->parray[TUNING_ARRAY][n] += 12;
		}
	}

	//	SORT PITCHES INTO ASCENDING ORDER

	for(n = 0; n < total-1; n++) {
		for(m = n; m < total; m++) {
			if(dz->parray[TUNING_ARRAY][m] < dz->parray[TUNING_ARRAY][n]) {
				tempval = dz->parray[TUNING_ARRAY][m];
				dz->parray[TUNING_ARRAY][m] = dz->parray[TUNING_ARRAY][n];
				dz->parray[TUNING_ARRAY][n] = tempval;
			}
		}
	}

	if(dz->mode == 2) {

		//	FOR HARMONIC-FIELD CASE : FILL THE MIDI SPACE
		m = total;
		oct = 1;
		midival = 0.0;
		while(midival <= 127.0) {
			for(n = 0; n < total; n++) {
				midival = dz->parray[TUNING_ARRAY][n] + (12.0 * oct);
				if(midival > 127.0)
					break;
				if(m >= dz->itemcnt) {
					sprintf(errstr,"Tuning array overran when expanded.\n");
					return(DATA_ERROR);
				}
				dz->parray[TUNING_ARRAY][m++] = midival;
			}
			oct++;
		}
		dz->itemcnt = m;	//	Remember size of tuning-space
	}
	(*cmdline)++;		
	(*cmdlinecnt)--;
	return(FINISHED);
}

/************************ FILL_TUNING_ARRAY *********************/

int fill_tuning_array(dataptr dz)
{
	int n, m;
	dz->itemcnt = 124;	//	Only need 11 octaves
	if ((dz->parray[TUNING_ARRAY] = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL) {
		sprintf(errstr,"Insufficient memory to store tuning data\n");
		return(MEMORY_ERROR);
	}
	for(n = 0,m = 4;m <= 127;n++,m++)
		dz->parray[TUNING_ARRAY][n] = (double)m;
	dz->itemcnt = n;	//	Remember size of tuning-space
	return(FINISHED);
}

/****************************** SPECGET *******************************
 *
 * (1)	Ignore partials below low limit of pitch.
 * (2)  If this channel data is louder than any existing piece of data in ring.
 *		(Ring data is ordered loudness-wise)...
 * (3)	If this freq is too close to an existing frequency..
 * (4)	and if it is louder than that existing frequency data..
 * (5)	Substitute in in the ring.
 * (6)	Otherwise, (its a new frq) insert it into the ring.
 */
 
int specget(double *pitch,chvptr *partials,dataptr dz)
{
	int exit_status = FINISHED;
	int vc;
	chvptr here, there;
	float minamp;
	double loudest_partial_frq, nextloudest_partial_frq, lo_loud_partial, hi_loud_partial, sum;	
	if((exit_status = initialise_ring_vals(MAXIMI,-1.0,dz))<0)
		return(exit_status);
	if((exit_status = rectify_frqs(dz->flbufptr[0],dz))<0)
		return(exit_status);
	sum = 0.0;
	for(vc=0;vc<dz->wanted;vc+=2) {
		sum += dz->flbufptr[0][AMPP];
		here = dz->ringhead;
		if(dz->flbufptr[0][FREQ] > SPEC_MINFRQ) {		  			/* 1 */
			do {								
				if(dz->flbufptr[0][AMPP] > here->val) {			   	/* 2 */
					if((exit_status = close_to_frq_already_in_ring(&there,(double)dz->flbufptr[0][FREQ],dz))<0)
						return(exit_status);
					if(exit_status==TRUE) {
						if(dz->flbufptr[0][AMPP] > there->val) {		/* 4 */
							if((exit_status = substitute_in_ring(vc,here,there,dz))<0) /* 5 */
								return(exit_status);
						}
					} else	{										/* 6 */
						if((exit_status = insert_in_ring(vc,here,dz))<0)
							return(exit_status);
					}				
					break;
				}
			} while((here = here->next)!=dz->ringhead);
		}
	}
	loudest_partial_frq     = dz->flbufptr[0][dz->ringhead->loc + 1];
	nextloudest_partial_frq = dz->flbufptr[0][dz->ringhead->next->loc + 1];
	if(loudest_partial_frq < nextloudest_partial_frq) {
		lo_loud_partial = loudest_partial_frq;
		hi_loud_partial = nextloudest_partial_frq;
	} else {
		lo_loud_partial = nextloudest_partial_frq;
		hi_loud_partial = loudest_partial_frq;
	}
	if((exit_status = put_ring_frqs_in_ascending_order(&partials,&minamp,dz))<0)
		return(exit_status);
	if((exit_status = alternative_find_pitch(pitch,partials,dz))<0)
		return(exit_status);
//	find_pitch(pitch,partials,lo_loud_partial,hi_loud_partial,minamp,dz);
	return exit_status;
}

/**************************** CLOSE_TO_FRQ_ALREADY_IN_RING *******************************/

int close_to_frq_already_in_ring(chvptr *there,double frq1,dataptr dz)
{
#define EIGHT_OVER_SEVEN	(1.142857143)

	double frq2, frqratio;
	*there = dz->ringhead;
	do {
		if((*there)->val > 0.0) {
			frq2 = dz->flbufptr[0][(*there)->loc + 1];
			if(frq1 > frq2)
				frqratio = frq1/frq2;
			else
			    frqratio = frq2/frq1;
			if(frqratio < EIGHT_OVER_SEVEN)
				return(TRUE);
		}
	} while((*there = (*there)->next) != dz->ringhead);
	return(FALSE);
}

/******************************* SUBSITUTE_IN_RING **********************/

int substitute_in_ring(int vc,chvptr here,chvptr there,dataptr dz)
{
	chvptr spare, previous;
	if(here!=there) {
		if(there==dz->ringhead) {
			sprintf(errstr,"IMPOSSIBLE! in substitute_in_ring()\n");
			return(PROGRAM_ERROR);
		}
		spare = there;
		there->next->last = there->last; /* SPLICE REDUNDANT STRUCT FROM RING */
		there->last->next = there->next;
		previous = here->last;
		previous->next = spare;			/* SPLICE ITS ADDRESS-SPACE BACK INTO RING */
		spare->last = previous;			/* IMMEDIATELY BEFORE HERE */
		here->last = spare;
		spare->next = here;
		if(here==dz->ringhead)			/* IF HERE IS RINGHEAD, MOVE RINGHEAD */
			dz->ringhead = spare;
		here = spare;					/* POINT TO INSERT LOCATION */
	}
	here->val = dz->flbufptr[0][AMPP]; 	/* IF here==there */
	here->loc = vc;	    				/* THIS WRITES OVER VAL IN EXISTING RING LOCATION */
	return(FINISHED);
}

/*************************** INSERT_IN_RING ***************************/


int insert_in_ring(int vc, chvptr here, dataptr dz)
{
	chvptr previous, newend, spare;
	if(here==dz->ringhead) {
		dz->ringhead = dz->ringhead->last;
		spare = dz->ringhead;
	} else {
		if(here==dz->ringhead->last)
			spare = here;
		else {
			spare  = dz->ringhead->last;
			newend = dz->ringhead->last->last; 	/* cut ENDADR (spare) out of ring */
			dz->ringhead->last = newend;
			newend->next = dz->ringhead;
			previous = here->last;
			here->last = spare;					/* reuse spare address at new loc by */ 
			spare->next = here;  				/* inserting it back into ring before HERE */
			previous->next = spare;
			spare->last = previous;
		}
	}
	spare->val = dz->flbufptr[0][vc];			/* Store new val in spare ring location */
	spare->loc = vc;
	return(FINISHED);
}

/************************** PUT_RING_FRQS_IN_ASCENDING_ORDER **********************/

int put_ring_frqs_in_ascending_order(chvptr **partials,float *minamp,dataptr dz)
{
	int k;
	chvptr start, ggot, here = dz->ringhead;
	float minpitch;
	*minamp = (float)MAXFLOAT;
	for(k=0;k<MAXIMI;k++) {
		if((*minamp = min(dz->flbufptr[0][here->loc],*minamp))>=(float)MAXFLOAT) {
			sprintf(errstr,"Problem with amplitude out of range: put_ring_frqs_in_ascending_order()\n");
			return(PROGRAM_ERROR);
		}
		(here->loc)++;		/* CHANGE RING TO POINT TO FRQS, not AMPS */
		here->val = dz->flbufptr[0][here->loc];
		here = here->next;
	}
	here = dz->ringhead;
	minpitch = dz->flbufptr[0][here->loc];
	for(k=1;k<MAXIMI;k++) {
		start = ggot = here;
		while((here = here->next)!=start) {		/* Find lowest frq */
			if(dz->flbufptr[0][here->loc] < minpitch) {	
				minpitch = dz->flbufptr[0][here->loc];
				ggot = here;
			}
		}
		(*partials)[k-1] = ggot;				/* Save its address */
		here = ggot->next;						/* Move to next ring site */
		minpitch = dz->flbufptr[0][here->loc];	/* Preset minfrq to val there */
		ggot->last->next = here;				/* Unlink ringsite ggot */
		here->last = ggot->last;
	}
	(*partials)[k-1] = here;	     			/* Remaining ringsite is maximum */

	here = dz->ringhead = (*partials)[0];   	/* Reconstruct ring */
	for(k=1;k<MAXIMI;k++) {
		here->next = (*partials)[k];
		(*partials)[k]->last = here;
		here = here->next;
	}
	here->next = dz->ringhead;					/* Close up ring */
	dz->ringhead->last = here;
	return(FINISHED);
}

/******************************	 FIND_PITCH **************************/

#define MAXIMUM_PARTIAL (64)
//
//void find_pitch(double *pitch,chvptr *partials,double lo_loud_partial,double hi_loud_partial,float minamp,dataptr dz)
//{
//	static int firsttime = 1;
//	int n, m, mm, k, kk, maximi_less_one = MAXIMUM_PARTIAL - 1, endd = 0;
//	double whole_number_ratio, comparison_frq, thisfrq, thisamp, pich_pich = -1.0;
//	for(n=1;n<maximi_less_one;n++) {
//		for(m=n+1;m<MAXIMUM_PARTIAL;m++) {	/* NOV 7 */
//			whole_number_ratio = (double)m/(double)n;
//			comparison_frq     = lo_loud_partial * whole_number_ratio;
//			if(equivalent_pitches(comparison_frq,hi_loud_partial,dz))
//				endd = (MAXIMUM_PARTIAL/m) * n;		/* explanation at foot of file */
//			else if(comparison_frq > hi_loud_partial)
//				break;
//
//			for(k=n;k<=endd;k+=n) {
//				pich_pich = lo_loud_partial/(double)k;
//				if(pich_pich>dz->nyquist/MAXIMI)
//					continue;
//				if(pich_pich<SPEC_MINFRQ)
//						break;
//				if(is_peak_at(pich_pich,0,minamp,dz)){
//					if(enough_partials_are_harmonics(partials,pich_pich,dz)) {
//						for(mm=0;mm<MAXIMI;mm++) {
//							kk = partials[mm]->loc;
//							if((thisfrq = dz->flbufptr[0][kk]) < pich_pich)
//								continue;
//							if(is_a_harmonic(thisfrq,pich_pich,dz)) {		/* for this algo, lowest partial must be within pitchrange specified */
//								if(thisfrq > dz->nyquist/MAXIMI)
//									return;
//								else
//									break;
//							}
//						}
//						for(mm=0;mm<MAXIMI;mm++) {
//							kk = partials[mm]->loc;
//							if((thisfrq = dz->flbufptr[0][kk]) < pich_pich)
//								continue;
//							if(is_a_harmonic(thisfrq,pich_pich,dz)) {
//								thisamp = dz->flbufptr[0][kk-1];
//							}
//						}
//
//					}	
//				}
//			}
//		}
//	}
//	if(pich_pich < miditohz(dz->param[ST_LOPCH]) || pich_pich > miditohz(dz->param[ST_HIPCH]))
//		pich_pich = 0.0;
//	if(pich_pich > 0) {
//		if(hztomidi(pitch,pich_pich) < 0) {
//			*pitch = -1.0;
//		} else
//			*pitch = pich_pich;
//	} else
//		*pitch = -1.0;
//}
//
/**************************** EQUIVALENT_PITCHES *************************/

int equivalent_pitches(double frq1, double frq2, dataptr dz)
{
	double ratio;
	int   iratio;
	double intvl;

	ratio = frq1/frq2;
	iratio = round(ratio);

	if(iratio!=1)
		return(FALSE);

	if(ratio > iratio)
		intvl = ratio/(double)iratio;
	else
		intvl = (double)iratio/ratio;
	if(intvl > st_intun)	
		return FALSE;
	return TRUE;
}

/*************************** IS_PEAK_AT ***************************/
//
//#define PEAK_LIMIT (.05)
//
//int is_peak_at(double frq,int window_offset,float minamp,dataptr dz)
//{
//	float *thisbuf;
//	int cc, vc, searchtop, searchbot;
//	if(window_offset) {								/* BAKTRAK ALONG BIGBUF, IF NESS */
//		thisbuf = dz->flbufptr[0] - (window_offset * dz->wanted);
//		if((int)thisbuf < 0 || thisbuf < dz->bigfbuf || thisbuf >= dz->flbufptr[1])
//			return(FALSE);
//	} else
//		thisbuf = dz->flbufptr[0];
//	cc = (int)((frq + dz->halfchwidth)/dz->chwidth);		 /* TRUNCATE */
//	searchtop = min(dz->clength,cc + CHANSCAN + 1);
//	searchbot = max(0,cc - CHANSCAN);
//	for(cc = searchbot ,vc = searchbot*2; cc < searchtop; cc++, vc += 2) {
//		if(!equivalent_pitches((double)thisbuf[vc+1],frq,dz)) {
//			continue;
//		}
//		if(thisbuf[vc] < minamp * PEAK_LIMIT)
//			continue;
//		if(local_peak(cc,frq,thisbuf,dz))		
//			return TRUE;
//	}
//	return FALSE;
//}
//
/**************************** ENOUGH_PARTIALS_ARE_HARMONICS *************************/
//
//int enough_partials_are_harmonics(chvptr *partials,double pich_pich,dataptr dz)
//{
//	int n, good_match = 0;
//	double thisfrq;
//	for(n=0;n<MAXIMI;n++) {
//		if((thisfrq = dz->flbufptr[0][partials[n]->loc]) < pich_pich)
//			continue;
//		if(is_a_harmonic(thisfrq,pich_pich,dz)){		
//			if(++good_match >= dz->iparam[ST_MATCH])
//				return TRUE;
//		}
//	}
//	return FALSE;
//}
//
/**************************** IS_A_HARMONIC *************************/
//
//int is_a_harmonic(double frq1,double frq2,dataptr dz)
//{
//	double ratio = frq1/frq2;
//	int   iratio = round(ratio);
//	double intvl;
//
//	ratio = frq1/frq2;
//	iratio = round(ratio);
//
//	if(ratio > iratio)
//		intvl = ratio/(double)iratio;
//	else
//		intvl = (double)iratio/ratio;
//	if(intvl > dz->param[ST_INTUN])
//		return(FALSE);
//	return(TRUE);
//}
//
/***************************** LOCAL_PEAK **************************/
//
//int local_peak(int thiscc,double frq, float *thisbuf, dataptr dz)
//{
//	int thisvc = thiscc * 2;
//	int cc, vc, searchtop, searchbot;
//	double frqtop = frq * SEMITONE_INTERVAL;
//	double frqbot = frq / SEMITONE_INTERVAL;
//	searchtop = (int)((frqtop + dz->halfchwidth)/dz->chwidth);		/* TRUNCATE */
//	searchtop = min(dz->clength,searchtop + PEAKSCAN + 1);
//	searchbot = (int)((frqbot + dz->halfchwidth)/dz->chwidth);		/* TRUNCATE */
//	searchbot = max(0,searchbot - PEAKSCAN);
//	for(cc = searchbot ,vc = searchbot*2; cc < searchtop; cc++, vc += 2) {
//		if(thisbuf[thisvc] < thisbuf[vc])
//			return(FALSE);
//	}
//	return(TRUE);
//}
//
/********************************** SPECTRNSF **********************************
 *
 * transpose spectrum, but retain original spectral envelope.
 */

int spectrnsf(double transpos,dataptr dz)
{
	int exit_status;
	double pre_totalamp, post_totalamp;
	int cc, vc;
	rectify_window(dz->flbufptr[0],dz);
	if((exit_status = extract_specenv(0,0,dz))<0)
		return(exit_status);
	if((exit_status = get_totalamp(&pre_totalamp,dz->flbufptr[0],dz->wanted))<0)
		return(exit_status);
	for(cc = 0, vc = 0; cc < dz->clength; cc++, vc += 2) {
		if((exit_status = tranpose_within_formant_envelope(transpos,vc,dz))<0)
			return(exit_status);
	}
	if((exit_status = reposition_partials_in_appropriate_channels(transpos,dz))<0)
		return(exit_status);
	if((exit_status = get_totalamp(&post_totalamp,dz->flbufptr[0],dz->wanted))<0)
		return(exit_status);
	return normalise(pre_totalamp,post_totalamp,dz);
}

/********************************** SPECTRNSP **********************************
 *
 * transpose spectrum, (spectral envelope also moves).
 */
int spectrnsp(double transpos,dataptr dz)
{
	int exit_status;
	double pre_totalamp, post_totalamp;
	int cc, vc;
	rectify_window(dz->flbufptr[0],dz);
	if((exit_status = get_totalamp(&pre_totalamp,dz->flbufptr[0],dz->wanted))<0)
		return(exit_status);
	for(cc = 0, vc = 0; cc < dz->clength; cc++, vc += 2)
		dz->flbufptr[0][FREQ] = (float)(dz->flbufptr[0][FREQ]*transpos);
	if((exit_status = reposition_partials_in_appropriate_channels(transpos,dz))<0)
		return(exit_status);
	if((exit_status = get_totalamp(&post_totalamp,dz->flbufptr[0],dz->wanted))<0)
		return(exit_status);
	return normalise(pre_totalamp,post_totalamp,dz);
}

/************************** TRANPOSE_WITHIN_FORMANT_ENVELOPE *****************************/

int tranpose_within_formant_envelope(double transpos,int vc,dataptr dz)
{
	int exit_status;
	double thisspecamp, newspecamp, thisamp, formantamp_ratio; 
	if((exit_status = getspecenvamp(&thisspecamp,(double)dz->flbufptr[0][FREQ],0,dz))<0)
		return(exit_status);
	dz->flbufptr[0][FREQ] = (float)(fabs(dz->flbufptr[0][FREQ])*transpos);
	if(dz->flbufptr[0][FREQ] < dz->nyquist) {
		if(thisspecamp < VERY_TINY_VAL)
			dz->flbufptr[0][AMPP] = 0.0f;
		else {
			if((exit_status = getspecenvamp(&newspecamp,(double)dz->flbufptr[0][FREQ],0,dz))<0)
				return(exit_status);
			if(newspecamp < VERY_TINY_VAL)
				dz->flbufptr[0][AMPP] = 0.0f;
			else {
				formantamp_ratio = newspecamp/thisspecamp;
				if((thisamp = dz->flbufptr[0][AMPP] * formantamp_ratio) < VERY_TINY_VAL)
					dz->flbufptr[0][AMPP] = 0.0f;
				else
					dz->flbufptr[0][AMPP] = (float)thisamp;
			}
		}
	}
	return(FINISHED);
}

/************************ REPOSITION_PARTIALS_IN_APPROPRIATE_CHANNELS *************************
 *
 * (1)	At each pass, preset store-buffer channel amps to zero.
 * (2)	Move frq data into appropriate channels, carrying the
 *		amplitude information along with them.
 *		Work down spectrum 	for upward transposition, and
 * (3)	up spectrum for downward transposition,
 *		so that we do not overwrite transposed data before we move it.
 * (4)	Put new frqs back into src buff.
 */

int reposition_partials_in_appropriate_channels(double transpos,dataptr dz)
{
	int exit_status;
	int truecc,truevc;
	int cc, vc;
	for(vc = 0; vc < dz->wanted; vc+=2)						/* 1 */
		dz->windowbuf[0][vc] = 0.0f;
	if(transpos > 1.0f) {			/* 2 */
		for(cc=dz->clength-1,vc = dz->wanted-2; cc>=0; cc--, vc-=2) {
			if(dz->flbufptr[0][FREQ] < dz->nyquist && dz->flbufptr[0][AMPP] > 0.0f) {
				if((exit_status = get_channel_corresponding_to_frq(&truecc,(double)dz->flbufptr[0][FREQ],dz))<0)
					return(exit_status);
				truevc = truecc * 2;
				if((exit_status = move_data_into_appropriate_channel(vc,truevc,dz->flbufptr[0][AMPP],dz->flbufptr[0][FREQ],dz))<0)
					return(exit_status);
												/* upward transpos, chandata tends to thin */
			}									/* case(TRUE) tries for fuller spectrum    */
		}
		for(vc = 0; vc < dz->wanted; vc++)
			dz->flbufptr[0][vc] = dz->windowbuf[0][vc];	
	} else if(transpos < 1.0f){		/* 3 */
		for(cc=0,vc = 0; cc < dz->clength; cc++, vc+=2) {
			if(dz->flbufptr[0][FREQ] < dz->nyquist && dz->flbufptr[0][FREQ]>0.0) {
				if((exit_status = get_channel_corresponding_to_frq(&truecc,(double)dz->flbufptr[0][FREQ],dz))<0)
					return(exit_status);
				truevc = truecc * 2;
				if((exit_status = move_data_into_appropriate_channel(vc,truevc,dz->flbufptr[0][AMPP],dz->flbufptr[0][FREQ],dz))<0)
					return(exit_status);
			}
		}
		for(vc = 0; vc < dz->wanted; vc++)
			dz->flbufptr[0][vc] = dz->windowbuf[0][vc];		/* 4 */
	}
	return(FINISHED);
}

/***************************** TIDY_UP_PITCH_DATA ***********************/

int tidy_up_the_pitch_data(double *pitches,int pitchcnt,dataptr dz)
{
	int exit_status;

	if(dz->vflag[1]) {
		if((exit_status = anti_noise_smoothing(pitchcnt,pitches,dz->frametime))<0)
			return(exit_status);
	}
	if((exit_status = mark_zeros_in_pitchdata(pitches,pitchcnt,dz))<0)
		return(exit_status);
	if(dz->vflag[1]) {
		if((exit_status = eliminate_blips_in_pitch_data(pitches,pitchcnt,dz))<0)
			return(exit_status);
	}
	return pitch_found(pitches,pitchcnt);
}

/********************** ANTI_NOISE_SMOOTHING *********************/
/*RWD used in conditional test, so better as a real var:*/
static const int MIN_SMOOTH_SET = 3;
#define MAX_GLISRATE   (16.0)	/* Assumptions: pitch can't move faster than 16 octaves per sec: MAX_GLISRATE */ 
								/* Possible movement from window-to-window = MAX_GLISRATE * dz->frametime */

int anti_noise_smoothing(int wlength,double *pitches,float frametime)
{
	char *smooth;
	double max_pglide;
	int n;
	if(wlength < MIN_SMOOTH_SET + 1)
		return(FINISHED);

	max_pglide = pow(2.0,MAX_GLISRATE * frametime);

	if((smooth = (char *)malloc((size_t)wlength))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for smoothing array.\n");
		return(MEMORY_ERROR);
	}
	for(n=1;n<wlength-1;n++)
		smooth[n] = (char)is_smooth_from_both_sides(n,max_pglide,pitches);
	smooth[0]         = (char)is_initialpitch_smooth(smooth,max_pglide,pitches);
	smooth[wlength-1] = (char)is_finalpitch_smooth(smooth,max_pglide,wlength,pitches);
	for(n=MIN_SMOOTH_SET-1;n<wlength;n++) {
		if(!smooth[n])
			smooth[n] = (char)is_smooth_from_before(n,smooth,max_pglide,pitches);
	}
	for(n=0;n<=wlength-MIN_SMOOTH_SET;n++) {
		if(!smooth[n])
			smooth[n] = (char)is_smooth_from_after(n,smooth,max_pglide,pitches);
	}
	test_glitch_sets(smooth,max_pglide,wlength,pitches);
	remove_unsmooth_pitches(smooth,wlength,pitches);
	free(smooth);
	return(FINISHED);
}		

/********************** IS_SMOOTH_FROM_BOTH_SIDES *********************
 *
 * verify a pitch if it has continuity with the pitches on either side.
 */

int is_smooth_from_both_sides(int n,double max_pglide,double *pitches)
{
	double thispitch, pitch_before, pitch_after;
	double pre_interval, post_interval;
	if((thispitch = pitches[n]) < FLTERR)
		return FALSE;
	if((pitch_before = pitches[n-1]) < FLTERR)
		return  FALSE;
	if((pitch_after = pitches[n+1]) < FLTERR)
		return  FALSE;
	pre_interval  = pitch_before/thispitch;
	if(pre_interval < 1.0)
		pre_interval = 1.0/pre_interval;
	post_interval = pitch_after/thispitch;
	if(post_interval < 1.0)
		post_interval = 1.0/post_interval;
	if(pre_interval  > max_pglide
	|| post_interval > max_pglide)
		return  FALSE;
	return  TRUE;
}

/********************** IS_INITIALPITCH_SMOOTH *********************
 *
 * verify first pitch if it has continuity with an ensuing verified pitch.
 */

int is_initialpitch_smooth(char *smooth,double max_pglide,double *pitches)
{
	int n;
	double thispitch, post_interval;
	if((thispitch = pitches[0]) < FLTERR)
		return FALSE;
	for(n=1;n < MIN_SMOOTH_SET;n++) {
		if(smooth[n]) {
			post_interval = pitches[n]/pitches[0];
			if(post_interval < 1.0)
				post_interval = 1.0/post_interval;
			if(post_interval <= pow(max_pglide,(double)n))
				return TRUE;
		}
	}	
	return(FALSE);
}
			
/********************** IS_FINALPITCH_SMOOTH *********************
 *
 * verify final pitch if it has continuity with a preceding verified pitch.
 */

int is_finalpitch_smooth(char *smooth,double max_pglide,int wlength,double *pitches)
{
	double pre_interval, thispitch;
	int n;
	int last = wlength - 1;
	if((thispitch = pitches[last]) < FLTERR)
		return FALSE;
	for(n=1;n < MIN_SMOOTH_SET;n++) {
		if(smooth[last-n]) {
			pre_interval = pitches[last-n]/pitches[last];
			if(pre_interval < 1.0)
				pre_interval = 1.0/pre_interval;
			if(pre_interval <= pow(max_pglide,(double)n))
				return TRUE;
		}
	}
	return(FALSE);
}
			
/********************** IS_SMOOTH_FROM_BEFORE *********************
 *
 * verify a pitch which has continuity with a preceding set of verified pitches.
 */

int is_smooth_from_before(int n,char *smooth,double max_pglide,double *pitches)
{
	double thispitch, pitch_before, pre_interval;
	int m;
	if((thispitch = pitches[n]) < FLTERR)
		return FALSE;
	for(m=1;m<MIN_SMOOTH_SET;m++) {		/* If there are (MIN_SMOOTH_SET-1) smooth pitches before */
		if(!smooth[n-m])
			return(FALSE);
	}
	pitch_before = pitches[n-1];		/* Test the interval with the previous pitch */
	pre_interval  = pitch_before/thispitch;
	if(pre_interval < 1.0)
		pre_interval = 1.0/pre_interval;
	if(pre_interval  > max_pglide)   
		return  FALSE;				   	/* And if it's acceptably smooth */
	return  TRUE;					   	/* mark this pitch as smooth also */
}

/********************** IS_SMOOTH_FROM_AFTER *********************
 *
 * verify a pitch which has continuity with a following set of verified pitches.
 */

int is_smooth_from_after(int n,char *smooth,double max_pglide,double *pitches)
{
	double thispitch, pitch_after, post_interval;
	int m;
	if((thispitch = pitches[n]) < FLTERR)
		return FALSE;
	for(m=1;m<MIN_SMOOTH_SET;m++) {	/* If there are (MIN_SMOOTH_SET-1) smooth pitches after */
		if(!smooth[n+m])
		return(FALSE);
	}
	pitch_after = pitches[n+1];		/* Test the interval with the next pitch */
	post_interval  = pitch_after/thispitch;
	if(post_interval < 1.0)
		post_interval = 1.0/post_interval;
	if(post_interval  > max_pglide)   
		return  FALSE;				/* And if it's acceptably smooth */
	return  TRUE;					/* mark this pitch as smooth also */
}

/********************** TEST_GLITCH_SETS *********************
 *
 * This function looks for any sets of values that appear to be glitches
 * amongst the real pitch data.
 * It is possible some items are REAL pitch data isolated BETWEEN short glitches.
 * This function checks for these cases.
 */

int test_glitch_sets(char *smooth,double max_pglide,int wlength,double *pitches)
{
	int exit_status;
	int gotglitch = FALSE;
	int n, gltchend, gltchstart = 0;
	for(n=0;n<wlength;n++) {
		if(gotglitch) {			/* if inside a glitch */
			if(smooth[n]) {		/* if reached its end, mark the end, then process the glitch */
				gltchend = n;
				if((exit_status = test_glitch_forwards(gltchstart,gltchend,smooth,max_pglide,pitches))<0)
					return(exit_status);
				if((exit_status = test_glitch_backwards(gltchstart,gltchend,smooth,max_pglide,wlength,pitches))<0)
					return(exit_status);
				gotglitch = 0;
			}
		} else {				/* look for a glitch and mark its start */
			if(!smooth[n]) {
				gotglitch = 1;
				gltchstart = n;
			}
		}
	}
	if(gotglitch) {				/* if inside a glitch at end of data, process glitch */
		gltchend = n;
		test_glitch_forwards(gltchstart,gltchend,smooth,max_pglide,pitches);
	}
	return(FINISHED);
}

/********************* REMOVE_UNSMOOTH_PITCHES ***********************
 *
 * delete all pitches which have no verified continuity with surrounding pitches.
 */

void remove_unsmooth_pitches(char *smooth,int wlength,double *pitches)
{
	int n;
	for(n=0;n<wlength;n++) {
		if(!smooth[n])		
			pitches[n] = (float)NOT_PITCH;
	}
}

/********************** TEST_GLITCH_FORWARDS *********************
 *
 * searching from start of glitch, look for isolated true pitches
 * amongst glitch data.
 */

#define LAST_SMOOTH_NOT_SET (-1)

int test_glitch_forwards(int gltchstart,int gltchend,char *smooth,double max_pglide,double *pitches)
{
	int n, glcnt;
	int last_smooth, previous;
	double pre_interval;
	if((previous = gltchstart - 1) < 0)
		return FINISHED;
	if(pitches[previous] < FLTERR) {
		sprintf(errstr,"Error in previous smoothing logic: test_glitch_forwards()\n");
		return(PROGRAM_ERROR);
	}
	last_smooth = previous;
	n = gltchstart+1;								/* setup params for local search of glitch */
	glcnt = 1;
	while(n < gltchend) {							/* look through the glitch */
		if(pitches[n] > FLTERR) {					/* if glitch location holds a true pitch */
			pre_interval = pitches[n]/pitches[previous]; 
			if(pre_interval < 1.0)
				pre_interval = 1.0/pre_interval;	/* compare against previous verified pitch */
			if(pre_interval <= pow(max_pglide,(double)(n-previous))) {
				smooth[n] = TRUE;	   				/* if comparable: mark this pitch as verified */
				last_smooth = n;
			}
		}											
		n++;										/* Once more than a max-glitch-set has been scanned */							  				
													/* or the end of the entire glitch is reached */
		if(++glcnt >= MIN_SMOOTH_SET || n >= gltchend) {
			if(last_smooth == previous)
				break;								/* If no new verifiable pitch found, give up */
			previous = last_smooth;
			n = last_smooth + 1;					/* Otherwise start a new local search from newly verified pitch */
			glcnt = 1;									
		}
	}
	return(FINISHED);
}

/********************** TEST_GLITCH_BACKWARDS *********************
 *
 * searching from end of glitch, look for isolated true pitches
 * amongst glitch data.
 */

int test_glitch_backwards(int gltchstart,int gltchend,char *smooth,double max_pglide,int wlength,double *pitches)
{
	int n, glcnt, next, next_smooth;
	double post_interval;
	if((next = gltchend) >= wlength)
		return FINISHED;
	if(pitches[next] < FLTERR) {
		sprintf(errstr,"Error in previous smoothing logic: test_glitch_backwards()\n");
		return(PROGRAM_ERROR);
	}
	next_smooth = next;
	n = gltchend-2;			 						/* setup params for local search of glitch */
	glcnt = 1;
	while(n >= gltchstart) {						/* look through the glitch */
		if(pitches[n] > FLTERR) {					/* if glitch location holds a true pitch */
			post_interval = pitches[n]/pitches[next]; 
			if(post_interval < 1.0)
				post_interval = 1.0/post_interval;	/* compare against previous verified pitch */
			if(post_interval <= pow(max_pglide,(double)(next - n))) {
				smooth[n] = TRUE;	   				/* if comparable: mark this pitch as verified */
				next_smooth = n;
			}
		}							  				
		n--;						  				/* Once more than a max-glitch-set has been scanned */
													/* or the start of the entire glitch is reached */							  				
		if(++glcnt >= MIN_SMOOTH_SET || n < gltchstart) {	  				
			if(next_smooth == next)
				break;								/* If no new verifiable pitch found, give up */
			next = next_smooth;
			n = next_smooth - 1;
			glcnt = 1;								/* Otherwise start a new local search */
		}
	}
	return(FINISHED);
}

/**************************** PITCH_FOUND ****************************/

int pitch_found(double *pitches,int pitchcnt)
{
	int n;
    for(n=0;n<pitchcnt;n++) {
		if(pitches[n] > NOT_PITCH)
			return(FINISHED);
	}
	sprintf(errstr,"No valid pitch found.\n");
	return(GOAL_FAILED);
}
 
/********************************** MARK_ZEROS_IN_PITCHDATA ************************
 *
 * Disregard data on windows which are SILENCE_RATIO below maximum level.
 */

int mark_zeros_in_pitchdata(double *pitches,int pitchcnt,dataptr dz)
{
	int n;
	double maxlevel = 0.0, minlevel;
	for(n=0;n<pitchcnt;n++) {
		if(dz->parray[TOTALAMPS][n] > maxlevel)
			maxlevel = dz->parray[TOTALAMPS][n];
	}
	minlevel = maxlevel * st_noise;
	for(n=0;n<pitchcnt;n++) {
		if(dz->parray[TOTALAMPS][n] < minlevel)
			pitches[n] = (float)NOT_SOUND;
	}
	return(FINISHED);
}

/************************** ELIMINATE_BLIPS_IN_PITCH_DATA ****************************
 *
 * (1) 	Eliminate any group of 'dz->param[PICH_VALID]' pitched windows, bracketed by
 *	unpitched windows, as unreliable data.
 */

int eliminate_blips_in_pitch_data(double *pitches,int pitchcnt,dataptr dz)
{
	int n, m, k, wlength_less_bliplen;
	int OK = 1;
	if(st_wndws<=0)
		return(FINISHED);
	wlength_less_bliplen = pitchcnt - st_wndws;
	for(n=1;n<wlength_less_bliplen;n++) {
		if(pitches[n] > 0.0) {
			if(pitches[n-1] < 0.0) {
				for(k = 1; k <= st_wndws; k++) {
					if(pitches[n+k] < 0.0) {
						for(m=0;m<k;m++)
							pitches[n+m] = (float)NOT_PITCH;
						n += k;
						continue;
					}
				}
			}
		}
	}
	n = wlength_less_bliplen;
	if((pitches[n] > 0.0) && (pitches[n-1] < 0.0)) {
/* UNREACHABLE at level4 ??? */
		for(k = 1; k < st_wndws; k++) {
			if(pitches[n+k] < 0.0)
				OK = 0;
			break;
		}
	}
	if(!OK)  {
		for(n=wlength_less_bliplen;n<pitchcnt;n++)
			pitches[n] = (float)NOT_PITCH;
	}
	return(FINISHED);
}

/******************************	 ALTERNATIVE_FIND_PITCH **************************/

#define MAXIMUM_ROOT_DIVISION (2)

int alternative_find_pitch(double *pitch,chvptr *partials,dataptr dz)
{
	int n, m, kk, mx = MAXIMI * MAXIMUM_ROOT_DIVISION, *matches, match, matchcnt = 0, maxcnt;
	double whole_number_ratio, comparison_frq, thisfrq, pich_pich = -1.0, *posspich;
	chvptr thispartial, thatpartial;

	if ((posspich = (double *)malloc(mx * sizeof(double)))==NULL) {
		sprintf(errstr,"Insufficient memory to store possible pitches.\n");
		return(MEMORY_ERROR);
	}
	if ((matches = (int *)malloc(mx * sizeof(int)))==NULL) {
		sprintf(errstr,"Insufficient memory to store tuning matches.\n");
		return(MEMORY_ERROR);
	}
	for(m=1;m<MAXIMUM_ROOT_DIVISION;m++) {
		whole_number_ratio = 1.0/(double)m;			//	Consider each partial as a root OR 2 * root
		thispartial = dz->ringhead;
		do {										//	For every partial (thispartial)
			match = 0;								//	Use it (or its divisor) as the comparison frq
			thisfrq = thispartial->val * whole_number_ratio;	
			thatpartial = dz->ringhead;				//	For every partial (thatpartial)
			do {
				kk = 1;
				comparison_frq = thisfrq * kk;		//	Consider all harmonics of thispartial
				while(comparison_frq < thatpartial->val + thisfrq) {	//	until we exceed the freq of thatpartial
					if(equivalent_pitches(comparison_frq,thatpartial->val,dz))
						match++;					//	If thatpartial is a harmonic of thispartial, Count the match
					kk++;
					comparison_frq = thisfrq * kk;
				}
				thatpartial = thatpartial->next;
			} while(thatpartial != dz->ringhead);
			if(match >= dz->iparam[ST_MATCH]) {		//	If sufficient matches found
				posspich[matchcnt] = thisfrq;		//	Store fundamental pitch 
				matches[matchcnt]  = match;			//	And store count of harmonics-matches
				matchcnt++;
			}
			thispartial = thispartial->next;
		} while(thispartial != dz->ringhead);
	}		
	pich_pich = 0.0;	//	Default : no pitch
	if(matchcnt) {		//	If possible pitches found
		maxcnt = 0;		//	Find the highest number of harmonics-matches
		for(n=0;n<matchcnt;n++) {
			if(matches[n] > maxcnt)
				maxcnt = matches[n];
		}				//	If more than one most-matched-pitch, take the highest frequency
		for(n=0;n<matchcnt;n++) {
			if((matches[n] == maxcnt) && (posspich[n] > pich_pich))
				pich_pich = posspich[n];
		}
	}
	if(pich_pich < miditohz(dz->param[ST_LOPCH]) || pich_pich > miditohz(dz->param[ST_HIPCH]))
		pich_pich = 0.0;
	if(pich_pich <= 0)
		*pitch = -1.0;
	else
		*pitch = pich_pich;
	return FINISHED;
}

/************************** READ_FORMANTBAND_DATA_AND_SETUP_FORMANTS *********************/

int setup_formants(dataptr dz)
{
	int exit_status;
	int max_fbands = 0, arraycnt;
	aplptr ap = dz->application;
	if((exit_status = establish_formant_band_ranges(dz->infile->channels,ap))<0)
		return(exit_status);

	dz->specenv_type = PICHWISE_FORMANTS;			
	if(dz->specenv_type==PICHWISE_FORMANTS && ap->no_pitchwise_formants) {
		sprintf(errstr,"-p flag can't be used: Too few analysis data channels.\n");
		return(DATA_ERROR);
	}
	dz->formant_bands = 4;
	max_fbands = ap->max_pichwise_fbands;
	if((exit_status = initialise_specenv(&arraycnt,dz))<0)
		return(exit_status);
	if((exit_status = set_specenv_frqs(arraycnt,dz))<0)
		return(exit_status);
	dz->descriptor_samps = dz->infile->specenvcnt * DESCRIPTOR_DATA_BLOKS;
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
	double bandbot;
	double *interval;
	int m, n, k = 0, cc;
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
	dz->specenvfrq[k] = (float)dz->nyquist;
	dz->specenvpch[k] = (float)log10(dz->nyquist);
	dz->specenvtop[k] = (float)dz->nyquist;
	dz->specenvamp[k] = (float)0.0;
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
	int n;
	if(arraycnt < LOW_OCTAVE_BANDS) {
		sprintf(errstr,"Insufficient array space for low_octave_bands\n");
		return(PROGRAM_ERROR);
	}
	for(n=0;n<LOW_OCTAVE_BANDS;n++) {
		switch(n) {
		case(0):
			dz->specenvfrq[0] = (float)1.0;					/* frq whose log is 0 */
			dz->specenvpch[0] = (float)0.0;
			dz->specenvtop[0] = (float)dz->chwidth;			/* 8VA wide bands */
			break;
		case(1):
			dz->specenvfrq[1] = (float)(dz->chwidth * 1.5); /* Centre Chs 1-2 */	
			dz->specenvpch[1] = (float)log10(dz->specenvfrq[1]);
			dz->specenvtop[1] = (float)(dz->chwidth * 2.0);
			break;
		case(2):
			dz->specenvfrq[2] = (float)(dz->chwidth * 3.0); /* Centre Chs 2-4 */
			dz->specenvpch[2] = (float)log10(dz->specenvfrq[2]);
			dz->specenvtop[2] = (float)(dz->chwidth * 4.0);
			break;
		case(3):
			dz->specenvfrq[3] = (float)(dz->chwidth * 6.0); /* Centre Chs 4-8 */	
			dz->specenvpch[3] = (float)log10(dz->specenvfrq[3]);
			dz->specenvtop[3] = (float)(dz->chwidth * 8.0);
			break;
		default:
			sprintf(errstr,"Insufficient low octave band setups in setup_low_octave_bands()\n");
			return(PROGRAM_ERROR);
		}
	}
	return(FINISHED);
}

/***************** SPECIAL_OPERATION_ON_WINDOW_ZERO ************/

int special_operation_on_window_zero(dataptr dz)
{
	int vc;
	for(vc = 0; vc < dz->wanted; vc += 2)
		dz->windowbuf[0][FREQ] = dz->flbufptr[0][FREQ];
	return(TRUE);
}

/**************************** DO_SPECPEDAL ****************************/

int do_specpedal(dataptr dz)
{
	int exit_status, wc, done, windows_in_buf, wastuned = 0, tuned = 0;
	int samps_read;
	double time = 0.0, totalamp = 0.0;
	float maxamp;
	chvptr *partials;
	done = 0;
	if((partials = (chvptr *)malloc(MAXIMI * sizeof(chvptr)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for partials array.\n");
		return(MEMORY_ERROR);
	}
	while((samps_read = fgetfbufEx(dz->bigfbuf, dz->buflen,dz->ifd[0],0)) > 0) {
		if(samps_read < 0) {
			sprintf(errstr,"Failed to read data from input file.\n");
			return(SYSTEM_ERROR);
		}
		dz->flbufptr[0] = dz->bigfbuf;
		windows_in_buf = samps_read/dz->wanted;    
		for(wc=0; wc<windows_in_buf; wc++, dz->total_windows++) {
			if(dz->total_windows == 0) {
				dz->flbufptr[0] += dz->wanted;
				time += dz->frametime;
				continue;
			}
			if((exit_status = get_totalamp_and_maxamp(&totalamp,&maxamp,dz->flbufptr[0],dz->wanted))<0)
				return(exit_status);
			if(totalamp <= 0.0) {
				dz->flbufptr[0] += dz->wanted;
				time += dz->frametime;
				continue;
			}
			if((exit_status = get_peaks(dz))<0)
				return(exit_status);
			if((exit_status = tunepeaks(&tuned,dz))<0)
				return(exit_status);
			wastuned += tuned;
			
			dz->flbufptr[0] += dz->wanted;

			time += dz->frametime;
		}
		if((exit_status = write_samps(dz->bigfbuf,samps_read,dz))<0)
			return(exit_status);
	}
	fprintf(stdout,"INFO: Tuned %d of %d windows\n",wastuned,dz->total_windows);
	fflush(stdout);

	return FINISHED;
}

/****************************************** ALLOCATE_SINGLE_BUFFER_PLUS_PEAKARRAY *****************************/

int allocate_single_buffer_plus_peakarray(dataptr dz)
{
	unsigned int buffersize;
	if(dz->bptrcnt < 4) {
		sprintf(errstr,"bufptr not established in allocate_single_buffer_plus_peakarray()\n");
		return(PROGRAM_ERROR);
	}
	buffersize = dz->wanted * BUF_MULTIPLIER;
	dz->buflen = buffersize;
	buffersize += dz->wanted;		//	Extra space for peak assessment
	buffersize += 1;				//	Safety
	if((dz->bigfbuf	= (float*) malloc(buffersize * sizeof(float)))==NULL) {  
		sprintf(errstr,"INSUFFICIENT MEMORY for sound buffers.\n");
		return(MEMORY_ERROR);
	}
	dz->big_fsize = dz->buflen;
	dz->bigfbuf[dz->big_fsize] = 0.0f;	/* safety value */
	dz->flbufptr[2] = dz->bigfbuf + dz->buflen + 1;
	return(FINISHED);
}

/******************************** GET_PEAKS **********************************/

int get_peaks(dataptr dz) 
{
	float *inbuf = dz->flbufptr[0];
	float *pkbuf = dz->flbufptr[2];
	float lastamp;
	double maxamp;
	int cc, vc, lastvc, kk, thismax;
	memset((char *)pkbuf,0,dz->wanted * sizeof(float));	//	Preset all peak amplitudes to zeros

	if(inbuf[2] < inbuf[0])							//	Peak at start
		pkbuf[0] = inbuf[0];						//	set peak level to input amp
	else if (inbuf[0] > 0.0) {
		if(flteq(inbuf[2],inbuf[0]))				//	Possible peak-plateau at start
			pkbuf[0] = inbuf[0];					//	ditto
	}
	for(cc = 1, vc = 2, lastvc = 0; cc < dz->clength - 1; cc++, vc+=2, lastvc+=2) {

		if(inbuf[vc] > inbuf[lastvc]) {				//	If current amp louder than last
			pkbuf[vc] = inbuf[vc];					//  This amp possibly a peak, so assign it the input amp value
			lastamp = pkbuf[lastvc];					
			pkbuf[lastvc] = 0;						//	last amp could not be a peak, so set to zero
			if(lastamp > 0.0) {
				kk = vc - 4;						//	Check preceeding peaks, in case level plateaued
				while(kk >= 0) {					//	and remove plateau-peak values, as they are NOT a peak
					if(flteq(pkbuf[kk],lastamp))
						pkbuf[kk] = 0.0;
					kk-=2;
				}
			}
		} else if(flteq(inbuf[vc],inbuf[lastvc]))	//	peak-plateau, each plateau channel gets input amp val
			pkbuf[vc] = inbuf[vc];
		// else, not a peak, remains zero-valued
	}
	if(inbuf[vc] > inbuf[lastvc]) {				//	Peak AT END
		pkbuf[vc] = inbuf[vc];					//  This amp a peak, so assign it the input amp value
		lastamp = pkbuf[lastvc];					
		pkbuf[lastvc] = 0;						//	last amp could not be a peak, so set to zero
		if(lastamp > 0.0) {
			kk = vc - 4;						//	Check preceeding peaks, in case level plateaued
			while(kk >= 0) {					//	and remove plateau-peak values, as they are NOT a peak
				if(flteq(pkbuf[kk],lastamp))
					pkbuf[kk] = 0.0;
				kk-=2;
			}
		}
	} else if(pkbuf[lastvc] > 0.0) {
		if(flteq(inbuf[vc],inbuf[lastvc]))		//	Possible peak-plateau at end
			pkbuf[vc] = inbuf[vc];
	}
	
	//	GET THE PEAKCNT LOUDEST PEAKS

	for(kk = 0; kk < dz->iparam[ST_CNT];kk++) {
		maxamp = -10000;
		thismax = 0;
		for(vc = 0; vc < dz->wanted; vc += 2) {
			if(pkbuf[vc] > maxamp) {
				maxamp = pkbuf[vc];
				thismax = vc;
			}
		}
		if(maxamp <= 0)
			break;
		pkbuf[thismax+1] = pkbuf[thismax];		//	Move loudest value out of array (to adjacent odd-numbered array position)
		pkbuf[thismax] = 0.0;					//	so next pass gets next-loudest peak
	}
	for(vc = 0; vc < dz->wanted; vc += 2) {
		if(pkbuf[vc+1] > 0.0) {					//	Where peak level has been stored in odd-vals of array	
			pkbuf[vc] = 1.0;					//	Mark as peak (in even array-vals)
			pkbuf[vc+1] = inbuf[vc+1];			//	Remember frqs of peaks (in odd-array vals)
		}
	}											//	Look for duplication of peak freqs

	for(kk = 0; kk < dz->wanted; kk += 2) {				//	For all input freqs
		if(pkbuf[kk] == 0.0) {							//	If not marked as peak
			for(vc = 0; vc < dz->wanted; vc += 2) {	
				if(pkbuf[vc] > 0.0) {					//	Test against all frqs of peaks
														//	and if equal to a peak frq,
					if(equivalent_pitches((double)inbuf[kk+1],pkbuf[vc+1],dz))
						pkbuf[kk] = 1.0;				//	mark as another peak
					else if(inbuf[kk+1] > pkbuf[vc+1])
						break;
				}
			}
		}
	}
	return FINISHED;
}

/************************** GET_TOTALAMP_AND_MAXAMP ***********************/

int get_totalamp_and_maxamp(double *totalamp,float *maxamp,float *sbuf,int wanted)
{
	int vc;
	*totalamp = 0.0;
	*maxamp = -MAXFLOAT;
	for(vc = 0; vc < wanted; vc += 2) {
		*totalamp += sbuf[AMPP];
		if(sbuf[AMPP] > *maxamp)
			*maxamp = sbuf[AMPP];
	}
	return(FINISHED);
}

/************************** TUNEPEAKS ***********************/

int tunepeaks(int *tuned,dataptr dz)
{
	float *pkbuf = dz->flbufptr[2];
	float *inbuf = dz->flbufptr[0];
	double *tunings = dz->parray[TUNING_ARRAY];
	double actualfrq, centrefrq, topfrq, botfrq, newfrq, newfrqbas, lastnewfrq, botgap, topgap;
	float temp;
	int cc, vc, kk, octcnt, peakchan, peakvc;
	double closest, close, closestfrq = 0.0;
	*tuned = 0;
	for(cc = 0, vc = 0;cc < dz->clength; cc++, vc+=2) {
		if(pkbuf[vc] > 0.0) {										//	At each peak
			actualfrq = inbuf[vc+1];
			centrefrq = cc * dz->chwidth;							//	Find frq range of associated channel
			botfrq = max(centrefrq - dz->halfchwidth,0.0);
			topfrq = min(centrefrq + dz->halfchwidth,dz->nyquist);
			closest = HUGE;
			for(kk=0;kk<dz->itemcnt;kk++) {
				octcnt = 1;
				newfrqbas = tunings[kk];
				newfrq = miditohz(newfrqbas);
				lastnewfrq = 0.0;
				while(newfrq < actualfrq) {							//	Find nearest harmonic of tuning-set actual-freq in peak channel
					lastnewfrq = newfrq;
					octcnt++;
					newfrq = miditohz(newfrqbas + (octcnt * SEMITONES_PER_OCTAVE));
				}
				if(lastnewfrq > 0.0) {
					botgap = actualfrq - lastnewfrq;
					topgap = newfrq - actualfrq;								
					if(botgap < topgap)
						newfrq = lastnewfrq;
				}
				close = fabs(newfrq - actualfrq);
				if(close < closest) {
					closest = close;
					closestfrq = newfrq;
				}
			}
			newfrq = closestfrq;
			if(newfrq <= topfrq && newfrq >= botfrq) {				//	If newfrq lies within channel,
				inbuf[vc+1] = (float)newfrq;						//	retune it
				*tuned = 1;
			} else {												//	Otherwise
				peakchan = (int)round(newfrq/dz->chwidth);			//  find channel where that frq could sit
				peakvc = peakchan*2; 
				if(pkbuf[peakvc] == 0.0) {							//	If new channel is not already a peak
					pkbuf[peakvc+1] = (float)newfrq;				//	Insert new freq into selected channel
					*tuned = 1;
					temp = inbuf[peakvc];							//	Swap level of new channel for level of peak
					inbuf[peakvc] = inbuf[vc];
					inbuf[vc] = temp;
					pkbuf[peakvc] = 1.0;							//	Swap peak and non-peak markers of channels
					pkbuf[vc] = 0.0;
				} else {											//	BUT if it is already a peak
					if(cc == 0)										//	reduce level of channel that is NOT being retuned
						inbuf[vc] = inbuf[vc+2];					//	so its no longer a peak
					else if(cc == dz->clength - 1)
						inbuf[vc] = inbuf[vc-2];
					else
						inbuf[vc] = min(inbuf[vc-2],inbuf[vc+2]);
					pkbuf[vc] = 0.0;

				}
			}
		}
	}
	return FINISHED;
}

/************************ HANDLE_SEGMENTATION_DATA *********************/

int handle_segmentation_data(int *cmdlinecnt,char ***cmdline,dataptr dz)
{
	aplptr ap = dz->application;
	int n;
	char temp[200], *q, *filename = (*cmdline)[0];
	double *p, lasttime;
	FILE *sfp;
	if(!sloom) {
		if(*cmdlinecnt <= 0) {
			sprintf(errstr,"Insufficient parameters on command line.\n");
			return(USAGE_ONLY);
		}
	}
	ap->data_in_file_only 	= TRUE;
	ap->special_range 		= FALSE;
	if((sfp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,"Cannot open datafile %s\n",filename);
		return(DATA_ERROR);
	}
	n = 0;
	while(fgets(temp,200,sfp)!=NULL) {
		q = temp;
		if(is_an_empty_line_or_a_comment(q))
			continue;
		n++;
	}
	if(n==0) {
		sprintf(errstr,"No data in file %s\n",filename);
		return(DATA_ERROR);
	}
	dz->itemcnt = n;
	if ((dz->parray[TIMING_ARRAY] = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL) {
		sprintf(errstr,"Insufficient memory to store slice times\n");
		return(MEMORY_ERROR);
	}
	p = dz->parray[TIMING_ARRAY];
	if(fseek(sfp,0,0)<0) {
		sprintf(errstr,"fseek() failed in handle_segmentation_data()\n");
		return(SYSTEM_ERROR);
	}
	n = 0;
	lasttime = -1.0;
	while(fgets(temp,200,sfp)!=NULL) {
		q = temp;
		if(is_an_empty_line_or_a_comment(temp))
			continue;
		if(n >= dz->itemcnt) {
			sprintf(errstr,"Accounting problem reading timedata.\n");
			return(PROGRAM_ERROR);
		}
		while(get_float_from_within_string(&q,p)) {
			if(n == 0 && *p != 0.0) {
				sprintf(errstr,"Segmentation times must begin at zero (First time in file %s is %lf)\n",filename,*p);
				return(DATA_ERROR);
			}
			if(*p < 0.0) {
				sprintf(errstr,"Time (%lf) less than zero on line %d: file %s\n",*p,n+1,filename);
				return(DATA_ERROR);
			}
			if(*p <= lasttime) {
				sprintf(errstr,"Time (%lf) out of order (last time %lf) on lines %d & %d: file %s\n",*p,lasttime,n,n+1,filename);
				return(DATA_ERROR);
			}
			if(n == dz->itemcnt-1)
				*p = dz->duration;
			else if(*p >= dz->duration) {
				sprintf(errstr,"Time out of range (0 to sound duration %lf) : line %d: file %s\n",dz->duration,n+1,filename);
				return(DATA_ERROR);
			}
			if(*p - lasttime < 0.03 + FLTERR) {
				sprintf(errstr,"Segment length (%lf between %lf to %lf) too short (min %lf) : line %d: file %s\n",*p - lasttime,lasttime,*p,MINSEGLEN,n+1,filename);
				return(DATA_ERROR);
			}
			lasttime = *p;
			p++;
			n++;
		}
	}
	if(fclose(sfp)<0) {
		fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
		fflush(stdout);
	}
	(*cmdline)++;		
	(*cmdlinecnt)--;
	return(FINISHED);
}
