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

static double st_intun, st_noise;
static int st_wndws;


#define do_decohere is_rectified	
#define POSMIN 200
#define DISC_RATIO	1		//	Proportion of channels to discohere
#define DISC_RAND	2		//	Randomisation of frequency in discohered channels
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
static int  get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz);
static int  setup_and_init_input_brktable_constants(dataptr dz,int brkcnt);
static int  get_the_mode_no(char *str, dataptr dz);

/* BYPASS LIBRARY GLOBAL FUNCTION TO GO DIRECTLY TO SPECIFIC APPLIC FUNCTIONS */

extern int    allocate_tstretch_buffer(dataptr dz);
extern int    check_for_enough_tstretch_brkpnt_vals(dataptr dz);
extern int    setup_internal_params_for_tstretch(dataptr dz);
extern int    retrograde_sequence_of_time_intervals(int endtime,int count,double *startptr,dataptr dz);
extern double calculate_position(int x,double param0,double param1,double param2);
extern int calc_position_output_wdws_relative_to_input_wdws_for_increasing_stretch
			(int *thatwindow,int startwindow,int count,double param0,double param1,double param2,dataptr dz);
extern int calc_position_output_wdws_relative_to_input_wdws_for_decreasing_stretch
			(int *thatwindow,int startwindow,int count,int totaldur,double param0,double param1,double param2,dataptr dz);
extern double calculate_number_of_output_windows(double startwdur,double endwdur,int totaldur);
extern int    advance_along_input_windows(int wdw_to_advance,int atend,dataptr dz);
extern int    timestretch_this_segment(int *thatwindow,int startwindow,double thiswdur,double nextwdur,int totaldur,dataptr dz);
extern int    get_both_vals_from_brktable(double *thistime,double *thisstretch,int brktab_no,dataptr dz);
extern int    divide_time_into_equal_increments(int *thatwindow,int startwindow,double dur,int count,dataptr dz);
extern int    do_timestretching(int *thatwindow,int count,dataptr dz);
extern int    do_timevariable_timestretch(int *thatwindow,dataptr dz);
extern int    do_constant_timestretch(int *thatwindow,dataptr dz);
extern int    spectstretch(dataptr dz);
extern int decohere(float *fbuf,dataptr dz);

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
		dz->maxmode = 0;
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
	if((exit_status = set_internalparam_data("di",ap))<0) {         //RWD removed extra 'exit_status'
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	// open_first_infile		CDP LIB
	if((exit_status = open_first_infile(cmdline[0],dz))<0) {	
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);	
		return(FAILED);
	}
	cmdlinecnt--;
	cmdline++;
	if((exit_status = handle_the_outfile(&cmdlinecnt,&cmdline,is_launched,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
//	handle_formants()			redundant
//	handle_special_data()		redundant except
	if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {		// CDP LIB
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	//check_param_validity_and_consistency() ....
	if((exit_status = check_for_enough_tstretch_brkpnt_vals(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(MEMORY_ERROR);
	}
	if((exit_status = setup_internal_params_for_tstretch(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	is_launched = TRUE;

	if((exit_status = allocate_tstretch_buffer(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(MEMORY_ERROR);
	}
	if((dz->iparray[0] = (int *)malloc(dz->clength * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for channel sorting array.\n");
		return(MEMORY_ERROR);
	}
	display_virtual_time(0L,dz);

	//spec_process_file =
	if((exit_status = spectstretch(dz)) < 0) {
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
	int exit_status, n;
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
	dz->iarray_cnt = 1;
	dz->array_cnt  = 2;
	dz->ptr_cnt    = 4;
	if((dz->parray  = (double **)malloc(dz->array_cnt * sizeof(double *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for internal double arrays.\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<dz->array_cnt;n++)
		dz->parray[n] = NULL;
	if((dz->iparray 	= (int **)malloc(dz->iarray_cnt  * sizeof(int *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for internal integer arrays.\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<dz->iarray_cnt;n++)
		dz->iparray[n] = NULL;
	if((dz->ptr    	= (double  **)malloc(dz->ptr_cnt  * sizeof(double *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for internal pointer arrays.\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<dz->ptr_cnt;n++)
		dz->ptr[n] = NULL;
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
	if((exit_status = set_param_data(ap,0,3,3,"Ddd"))< 0)
		return(FAILED);
	if((exit_status = set_vflgs(ap,"",0,"","",0,0,""))< 0)
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
	ap->lo[TSTR_STRETCH]  	= 0.0001;
	ap->hi[TSTR_STRETCH]  	= 10000;
	ap->default_val[TSTR_STRETCH]	= 1.0;
	ap->lo[DISC_RATIO]  	= 0.0;
	ap->hi[DISC_RATIO]  	= 1.0;
	ap->default_val[DISC_RATIO]	= 0.75;
	ap->lo[DISC_RAND]  	= 0.0;
	ap->hi[DISC_RAND]  	= 1.0;
	ap->default_val[DISC_RAND]	= 0.5;
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
	if      (!strcmp(prog_identifier_from_cmdline,"stretch"))			dz->process = SPECTSTR;
	else {
		fprintf(stderr,"Unknown program identification string '%s'\n",prog_identifier_from_cmdline);
		return(USAGE_ONLY);
	}
	return(FINISHED);
}

/******************************** USAGE1 ********************************/

int usage1(void)
{
	return(usage2("stretch"));
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
	if(!strcmp(str,"stretch")) {	  /* STRETCH */
		fprintf(stdout,
		"spectstr stretch time infile outfile timestretch d-ratio di-rand\n"
		"\n"
		"TIME-STRETCHING OF INFILE, SUPPRESSING ARTEFACTS WHEN STRETCH IS > 1.0\n"
		"\n"
		"TIMESTRETCH may itself vary over time.\n"
		"D-RATIO     proportion of channels to discohere.\n"
		"D-RAND      Frequency randomisation of discohered channels.\n");
	} else
		sprintf(errstr,"Unknown option '%s'\n",str);
	return(USAGE_ONLY);
}


int usage3(char *str1,char *str2)
{
	fprintf(stderr,"Insufficient parameters on command line.\n");
	return(USAGE_ONLY);
}

/************ SETUP_INTERNAL_PARAMS_FOR_TSTRETCH *************/

int setup_internal_params_for_tstretch(dataptr dz)
{
	int exit_status;
	if(dz->brksize[TSTR_STRETCH] && (exit_status = force_value_at_zero_time(TSTR_STRETCH,dz))<0)
		return(exit_status);
	dz->param[TSTR_TOTIME] = (double)dz->wlength * dz->frametime;
	/* dur of orig sound source */

    dz->iparam[TSTR_ARRAYSIZE]  = POSMIN;
    if((dz->parray[TSTR_PBUF] = (double *)malloc(dz->iparam[TSTR_ARRAYSIZE] * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for timestretch array.\n");
		return(MEMORY_ERROR);
	}
    dz->ptr[TSTR_PEND]  = dz->parray[TSTR_PBUF] + dz->iparam[TSTR_ARRAYSIZE];
    if((dz->parray[TSTR_QBUF] = (double *)malloc(dz->iparam[TSTR_ARRAYSIZE] * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for 2nd timestretch array.\n");
		return(MEMORY_ERROR);
	}
	return(FINISHED);
}

/********************* CHECK_FOR_ENOUGH_TSTRETCH_BRKPNT_VALS **********************/

int check_for_enough_tstretch_brkpnt_vals(dataptr dz)
{
	if(dz->brksize[TSTR_STRETCH] && dz->brksize[TSTR_STRETCH] < 2) {
		sprintf(errstr,"Not enough data in tsretch data file.\n");
		return(DATA_ERROR);
	}
	dz->do_decohere = 1;
	if(dz->param[DISC_RATIO] == 0.0 || dz->param[DISC_RAND] == 0.0)
		dz->do_decohere = 0;
	dz->iparam[DISC_RATIO] = (int)round(dz->param[DISC_RATIO] * dz->clength);
	return(FINISHED);
}

/*************************** ALLOCATE_TSTRETCH_BUFFER ****************************/

int allocate_tstretch_buffer(dataptr dz)
{
	unsigned int buffersize;
	dz->bptrcnt = 6;
    if((dz->flbufptr = (float **)malloc(dz->bptrcnt * sizeof(float *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for sound buffer pointers.\n");
		return(MEMORY_ERROR);
	}
	buffersize = dz->wanted * BUF_MULTIPLIER;
	dz->buflen = buffersize;
	buffersize *= 2;
	buffersize += dz->wanted;
    if((dz->bigfbuf = (float *)malloc((size_t)(buffersize * sizeof(float))))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for sound buffers.\n");
		return(MEMORY_ERROR);
	}
    dz->big_fsize = dz->buflen;
    dz->flbufptr[0] = dz->bigfbuf + dz->wanted;			/* inbuf */
    dz->flbufptr[1] = dz->flbufptr[0] + dz->big_fsize;	/* outbuf & inbufend */
    dz->flbufptr[2] = dz->flbufptr[1] + dz->big_fsize;	/* outbufend */

    dz->flbufptr[3] = dz->flbufptr[0];					/* 1st inbuf pointer */
    dz->flbufptr[4] = dz->flbufptr[0] + dz->wanted;		/* 2nd inbuf pointer */
    dz->flbufptr[5] = dz->flbufptr[1];					/* outbuf ptr */
	return(FINISHED);
}

/********************************** SPECTSTRETCH ***********************************/
 
int spectstretch(dataptr dz)
{
	int exit_status;
	int thatwindow;
	int samps_read;
	if((samps_read = fgetfbufEx(dz->flbufptr[0], dz->big_fsize,dz->ifd[0],0)) < 0) {
		sprintf(errstr,"No data found in input soundfile.\n");
		return(DATA_ERROR);
	}
	if(sloom)
		dz->total_samps_read = samps_read;
	dz->flbufptr[3] = dz->flbufptr[0];
	dz->flbufptr[4] = dz->flbufptr[0] + dz->wanted;
	thatwindow = 0;
	if(dz->brksize[TSTR_STRETCH]==0)
		exit_status = do_constant_timestretch(&thatwindow,dz);
	else
		exit_status = do_timevariable_timestretch(&thatwindow,dz);
	if(exit_status<0)
		return(exit_status);
	if((exit_status = do_timestretching(&thatwindow,dz->ptr[TSTR_PBUF] - dz->parray[TSTR_PBUF],dz))<0)
		return(exit_status);
	if(dz->flbufptr[5] - dz->flbufptr[1] > 0)
		return write_samps(dz->flbufptr[1],dz->flbufptr[5] - dz->flbufptr[1],dz);
	return(FINISHED);
}

/******************************* DO_CONSTANT_TIMESTRETCH ****************************
 *
 * Set up array of indices for reading source window values into new
 * windows.
 *
 * (0)	Initialise position-buffer pointer and start window.
 * (1)	Get first breakpoint pair from brkpoint table.
 * (2)	Calculate initial time as whole number of WINDOWS.
 * (3)	If time is stretched, output window is SHRUNK relative to input
 *	window. Hence val = 1/stretch.
 * (4)	Proceed in a loop of all input breakpoint values.
 * (5)	Returns if premature end of file encountered.
 */

int do_constant_timestretch(int *thatwindow,dataptr dz)
{
	double thiswdur, dcount, dur;
	int count;
	int startwindow = 0;
	dz->ptr[TSTR_PBUF] = dz->parray[TSTR_PBUF];
	thiswdur  = 1.0/dz->param[TSTR_STRETCH];					/* 3 */
	dcount = (double)dz->wlength/thiswdur;
	count = round(fabs(dcount));
	dur = (double)dz->wlength/(double)count;
	return divide_time_into_equal_increments(thatwindow,startwindow,dur,count,dz);
}

/******************************* DO_TIMEVARIABLE_TIMESTRETCH ****************************
 *
 * Set up array of indices for reading source window values into new
 * windows.
 *
 * (0)	Initialise position-buffer pointer and start window.
 * (1)	Get first breakpoint pair from brkpoint table.
 * (2)	Calculate initial time as whole number of WINDOWS.
 * (3)	If time is stretched, output window is SHRUNK relative to input
 *		window. Hence val = 1/stretch.
 * (4)	Proceed in a loop of all input breakpoint values.
 * (5)	Returns if brkpnt times ran off end of source soundfile.
 */

int do_timevariable_timestretch(int *thatwindow,dataptr dz)
{
	int exit_status;
	int time0, time1, time2, totaldur;
	double stretch0, stretch1, thiswdur, nextwdur, valdiff, dtime0, dtime1;
	int atend = FALSE;
	int startwindow = 0;												  /* 0 */
	int timestretch_called = FALSE;
	dz->ptr[TSTR_PBUF] = dz->parray[TSTR_PBUF];										
	if((exit_status = get_both_vals_from_brktable(&dtime0,&stretch0,TSTR_STRETCH,dz))<0)	/* 1 */
		return(exit_status);
	if(exit_status == FINISHED) {
		sprintf(errstr,"Not enough data in brkfile & not trapped earlier: do_timevariable_timestretch()\n");
		return(PROGRAM_ERROR); 
	}
	time0 = round(dtime0/dz->frametime);								  /* 2 */
	thiswdur  = 1.0/stretch0;												  /* 3 */
	while((exit_status = get_both_vals_from_brktable(&dtime1,&stretch1,TSTR_STRETCH,dz))==CONTINUE) {/* 4 */
		nextwdur  = 1.0/stretch1;
		if((time1 = round(dtime1/dz->frametime)) > dz->wlength) {
			time2  = time1;												  /* 5 */
			time1 = round(dz->param[TSTR_TOTIME]/dz->frametime);
			valdiff  = nextwdur - thiswdur;
			valdiff *= (double)(time1 - time0)/(double)(time2 - time0);
			nextwdur = thiswdur + valdiff;
			atend = TRUE;
		}
		totaldur = time1 - time0;
		timestretch_called = TRUE;
		if((exit_status = timestretch_this_segment(thatwindow,startwindow,thiswdur,nextwdur,totaldur,dz))<0)
			return(exit_status);
		startwindow += totaldur;
		thiswdur  = nextwdur;
		time0 = time1;
		if(atend)
			break;
	}
	if(timestretch_called == FALSE) {
		sprintf(errstr,"Not enough data in brkfile & not trapped earlier: do_timevariable_timestretch()\n");
		return(PROGRAM_ERROR);
	} 
	return(FINISHED);
}

/*************************** DO_TIMESTRETCHING ********************************
 *
 * Read/interpolate source analysis windows.
 *
 * (1)	initialise the position-buffer pointer. NB we don't use pbufptr as
 *		this is (?) still marking the current position in the output buffer.
 * (2)	For each entry in the position-array.
 * (3)	get the absolute position.
 * (4)	The window to intepolate FROM is this truncated
 *		e.g. position 3.2 translates to window 3.
 * (5)	Is this is a move on from the current window position? How far?
 * (6)	If we're in the final window, set the 'atend' flag.
 * (7)	If we need to move on, call advance-windows and set up a new PAIR of
 *		input windows.
 * (8)	Get Interpolation time-fraction.
 * (9)	Get output values by interpolation between input values.
 * (10) Write a complete out-windowbuf to output file.
 * (11)	Give on-screen user information.
 */

int do_timestretching(int *thatwindow,int count,dataptr dz)
{
	int exit_status;
	double amp0, amp1, phas0, phas1;
	int	 vc;
	int n, thiswindow, step;
	int atend = FALSE;						
	double here, frac;
	double *p = dz->parray[TSTR_PBUF];				/* 1 */   
	for(n=0;n<count;n++) {							/* 2 */
		here = *p++;								/* 3 */
		thiswindow = (int)here; /* TRUNCATE */		/* 4 */
		step = thiswindow - *thatwindow;			/* 5 */
		if(thiswindow == dz->wlength-1)				/* 6 */
			atend = TRUE;
		if(step) {
			if((exit_status = advance_along_input_windows(step,atend,dz))<0)   /* 7 */
				return(exit_status);
		}
		frac = here - (double)thiswindow;			/* 8 */
		for(vc = 0; vc < dz->wanted; vc += 2) {
			amp0  = dz->flbufptr[3][AMPP];			/* 9 */
			phas0 = dz->flbufptr[3][FREQ];
			amp1  = dz->flbufptr[4][AMPP];
			phas1 = dz->flbufptr[4][FREQ];
			dz->flbufptr[5][AMPP]  = (float)(amp0  + ((amp1  - amp0)  * frac));
			dz->flbufptr[5][FREQ]  = (float)(phas0 + ((phas1 - phas0) * frac));
		}
		if(dz->do_decohere) {
			if((exit_status = decohere(dz->flbufptr[5],dz))<0)
				return(exit_status);
		}
		if((dz->flbufptr[5] += dz->wanted) >= dz->flbufptr[2]) {
			if((exit_status = write_exact_samps(dz->flbufptr[1],dz->big_fsize,dz))<0)
				return(exit_status);
			dz->flbufptr[5] = dz->flbufptr[1];
		}
		*thatwindow = thiswindow;
	}
	return(FINISHED);
}

/**************************** DIVIDE_TIME_INTO_EQUAL_INCREMENTS *****************************
 *
 * all time intervals are equal. Divide up total time thus.
 */

int divide_time_into_equal_increments(int *thatwindow,int startwindow,double dur,int count,dataptr dz)
{
	int exit_status;
	int n, remnant;
	int end   = dz->ptr[TSTR_PEND] - dz->ptr[TSTR_PBUF];
	int start = 0;
	while(count >= end) {
		for(n = start; n < end; n++)
			*(dz->ptr[TSTR_PBUF])++ = (dur * (double)n) + (double)startwindow;
		if((exit_status = do_timestretching(thatwindow,dz->iparam[TSTR_ARRAYSIZE],dz))<0)	  
			return(exit_status);
		dz->ptr[TSTR_PBUF] = dz->parray[TSTR_PBUF];
		start = end;
		end  += dz->iparam[TSTR_ARRAYSIZE];
	}
	if((remnant = count - start)>0) {
		for(n=start;n<count;n++)
			*(dz->ptr[TSTR_PBUF])++ = (dur * (double)n) + (double)startwindow;
	}
	return(FINISHED);
}

/**************************** GET_BOTH_VALS_FROM_BRKTABLE ****************************/

int get_both_vals_from_brktable(double *thistime,double *thisstretch,int brktab_no,dataptr dz)
{
	double *brkend = dz->brk[brktab_no] + (dz->brksize[brktab_no] * 2);
	if(dz->brkptr[brktab_no]>=brkend)
		return(FINISHED);
	*thistime = *(dz->brkptr[brktab_no])++;
	if(dz->brkptr[brktab_no]>=brkend) {
		sprintf(errstr,"Anomaly in get_both_vals_from_brktable().\n");
		return(PROGRAM_ERROR);
	}
	*thisstretch = *(dz->brkptr[brktab_no])++;
	return(CONTINUE);
}

/************************** TIMESTRETCH_THIS_SEGMENT *****************************
 *
 * Takes a group of input windows, counts number of output windows
 * corresponding to this buffer, and sets up, in pbuff, array(s) of values
 * which are the positions in the input array corresponding to the output
 * array positions.
 * (1)  If there is (almost) no change in duration of segments, calculates
 *       times on simple additive basis.
 * (2)  Otherwise, uses exponential formula.
 * (3)  If NOT passed a negative number (i.e. flagged), the sequence of time
 *      intervals is reversed.
 */

int timestretch_this_segment(int *thatwindow,int startwindow,double thiswdur,double nextwdur,int totaldur,dataptr dz)
{
	int    stretch_decreasing = TRUE;
	int   count;
	double dur, dcount = calculate_number_of_output_windows(thiswdur,nextwdur,totaldur);
	double param0, param1, param2;
	if(dcount < 0.0)
		stretch_decreasing = FALSE; 
	count = round(fabs(dcount));
	if(fabs(nextwdur - thiswdur)<=FLTERR) {            	/* 1 */
		dur = (double)totaldur/(double)count;
		return divide_time_into_equal_increments(thatwindow,startwindow,dur,count,dz);
	}    
	param0  = nextwdur - thiswdur;                      /* 2 */
	param1  = param0/(double)totaldur;
	param2  = thiswdur * (double)totaldur;
	if(stretch_decreasing==TRUE)							 	/* 3 */
		return calc_position_output_wdws_relative_to_input_wdws_for_decreasing_stretch
		(thatwindow,startwindow,count,totaldur,param0,param1,param2,dz);
	else
		return calc_position_output_wdws_relative_to_input_wdws_for_increasing_stretch
		(thatwindow,startwindow,count,param0,param1,param2,dz);
}

/*************************** ADVANCE_ALONG_INPUT_WINDOWS ****************************
 *
 * Advance window frame in input.
 * (1)	If got to end of data... Advanve ONE LESS than the distance
 * 	(wdw_to_advance) from last window-pair.
 * (2)	Else, advance by cnt windows.
 * (3)	If at end of buffer, copy THIS window to start of whole buffer,
 * (4)	And read more data in AFTER that (dz->wanted samps from start).
 * (6)	If this is the last window in the source file, there is no other
 *  	window to interpolate to, so set last window to same as this.
 */

int advance_along_input_windows(int wdw_to_advance,int atend,dataptr dz)
{
	int n, count;
	int samps_read;
	if(atend)						/* 1 */
		count = wdw_to_advance-1;
	else							/* 2 */
		count = wdw_to_advance;
	for(n=0;n<count;n++) {
		dz->flbufptr[3] = dz->flbufptr[4];			   	 /* ADVANCE LASTWINDOW TO THISWINDOW */
		if((dz->flbufptr[4] += dz->wanted) > dz->flbufptr[1]) {  /* ADVANCE THISWINDOW TO NEXT */
			memmove((char *)dz->bigfbuf,(char *)dz->flbufptr[3],dz->wanted * sizeof(float));
			dz->flbufptr[3] = dz->bigfbuf;				    /* 3 */
			dz->flbufptr[4] = dz->flbufptr[0];				/* 4 */
			if((samps_read = fgetfbufEx(dz->flbufptr[4],dz->big_fsize,dz->ifd[0],0)) < dz->wanted) {
				if(n <= count-2) {
					sprintf(errstr,"Program miscounted windows: anomaly 1 at EOF? : advance_along_input_windows()\n");
					return(PROGRAM_ERROR);
				}
				if(samps_read < 0) {
					sprintf(errstr,"Program miscounted windows: anomaly 2 at EOF? : advance_along_input_windows()\n");
					return(PROGRAM_ERROR);
				}
				return(FINISHED);
			}
			if(sloom)
				dz->total_samps_read += samps_read;
		}
	}
	if(atend)							/* 6 */
		dz->flbufptr[4] = dz->flbufptr[3];
	return(CONTINUE);
}

/*************************** CALCULATE_NUMBER_OF_OUTPUT_WINDOWS ***************************
 *
 * Given a sequence of events of varying duration, where initial and 
 * final durations are known, together with start and end times of the
 * total sequence, this function calculates the total number of events.
 *
 *	NB
 * (1)	Where the segment duration is increading, log(startwdur/edndur) is -ve,
 *		so the returned count is -ve.
 * (2)	Where segment duration is NOT changing, log(startwdur/endwdur), and hence dcount, would be zero.
 *		This situation is trapped by first checking for (approx) equality of startwdur and endwdur
 *		and calculating the count in simpler manner.
 */

double calculate_number_of_output_windows(double startwdur,double endwdur,int totaldur)
{
	double durdiff, dcount;
	if(flteq((durdiff = endwdur - startwdur),0.0)) {
		dcount = (double)totaldur/endwdur;
		return(dcount);
	}
	dcount  = startwdur/endwdur;
	dcount  = log(dcount);
	dcount *= (double)totaldur/durdiff;
	return(dcount);
}

/********* CALC_POSITION_OUTPUT_WDWS_RELATIVE_TO_INPUT_WDWS_FOR_DECREASING_STRETCH ************
 *
 * THE ARRAY OF positions has to be retrograded, but if we have more than
 * a single output-arrayfull of position values, we must start calculating
 * the output position array from the last input position, then invert those
 * positions so they are first positions, and so on!!!
 * (0)	As the array is to be filled backwards, start calculating positions
 *		from end of input. So end of first pass = end of input (= count).
 * (1)	Find how many locations remain in output-position buffer.
 * (2)	Start of first pass is this no of positions before end.
 * (3)	If start is < 0 this means that all the positions in the current input
 *		pass will fit in the output buffer in its current state, so we skip the
 *		loop and go to (10), Otherwise...
 * (4)	Mark the address in the output buffer where writing-values begins.
 * (5)	Store in output buffer, the positions RELATIVE to start of this segment
 *		of input values (i.e. relative to ZERO).
 * (6)	Retrograde this time-interval set.
 * (7)	Increment these relative values by startwindow, to give absolute
 *		position values.
 * (8)	Do the output, and reinitialise the output buffer pointer to start
 *		of buffer. Reset new input-block end to START of previous block
 *		(we're working backwards).
 * (9)	Decrement start of block by size of output array.
 * (10)	If either we did not enter the loop OR we have some input values
 *		left on leaving the principle loop, calcuate output positions of
 *		these items, and store in the output buffer (which will be flushed
 *		either by next call to timestretch_this_segment() or, at end, by flushbuf()).
 */

int calc_position_output_wdws_relative_to_input_wdws_for_decreasing_stretch
(int *thatwindow,int startwindow,int count,int totaldur,double param0,double param1,double param2,dataptr dz)
{
	int exit_status;
	int n, start, end = count;								/* 0 */
	double *p, *startptr;
	int tofill = dz->ptr[TSTR_PEND] - dz->ptr[TSTR_PBUF];	/* 1 */
	start = count - tofill;									/* 2 */
	while(start>=0) {										/* 3 */
		startptr = dz->ptr[TSTR_PBUF];						/* 4 */
		for(n=start;n<end;n++)			 					/* 5 */
			*(dz->ptr[TSTR_PBUF])++ = calculate_position(n,param0,param1,param2);
															/* 6 */
		if((exit_status = retrograde_sequence_of_time_intervals(totaldur - start,end - start,startptr,dz))<0)
			return(exit_status);
		for(p = startptr;p<dz->ptr[TSTR_PEND];p++)
			*p += (double)startwindow;						/* 7 */
		if((exit_status = do_timestretching(thatwindow,dz->iparam[TSTR_ARRAYSIZE],dz))<0)	
			return(exit_status);
		dz->ptr[TSTR_PBUF] = dz->parray[TSTR_PBUF];			/* 8 */
		end    = start;
		start -= dz->iparam[TSTR_ARRAYSIZE];				/* 9 */
	}
	if(end) {												/* 10 */
		startptr = dz->ptr[TSTR_PBUF];
		for(n=0;n<end;n++)
			*(dz->ptr[TSTR_PBUF])++ = calculate_position(n,param0,param1,param2);
		if((exit_status = retrograde_sequence_of_time_intervals(totaldur,end,startptr,dz))<0)
			return(exit_status);
		for(p = startptr;p<dz->ptr[TSTR_PBUF];p++)
			*p += (double)startwindow;
	}
	return(FINISHED);
}

/********* CALC_POSITION_OUTPUT_WDWS_RELATIVE_TO_INPUT_WDWS_FOR_INCREASING_STRETCH ************
 *
 * Find positions of output samples relative to input samples, buffer 
 * by buffer.
 */
   
int calc_position_output_wdws_relative_to_input_wdws_for_increasing_stretch
(int *thatwindow,int startwindow,int count,double param0,double param1,double param2,dataptr dz)
{
	int exit_status;
	int n;
	int start = 0;
	int end  = dz->ptr[TSTR_PEND] - dz->ptr[TSTR_PBUF];
	while(count>=end) {
		for(n=start;n<end;n++)
			*(dz->ptr[TSTR_PBUF])++ = calculate_position(n,param0,param1,param2) + (double)startwindow;
		if((exit_status = do_timestretching(thatwindow,dz->iparam[TSTR_ARRAYSIZE],dz))<0)
			return(exit_status);
		dz->ptr[TSTR_PBUF] = dz->parray[TSTR_PBUF];				   
		start = end;
		end  += dz->iparam[TSTR_ARRAYSIZE];
	}
	if(count-start) {
		for(n=start;n<count;n++)
			*(dz->ptr[TSTR_PBUF])++ = calculate_position(n,param0,param1,param2) + (double)startwindow;
	}
	return(FINISHED);
}

/*************************** CALCULATE_POSITION ****************************
 *
 * Do the position calculation using the exponential formula.
 */

double calculate_position(int x,double param0,double param1,double param2)
{
	double k;
	k  = param1;
	k *= (double)x;
	k  = exp(k);
	k *= param2;
	k -= param2;
	return(k/(param0));
}

/*************************** RETROGRADE_SEQUENCE_OF_TIME_INTERVALS **************************
 *
 * Accepts a sequence of times, starting in address startptr, 
 * and retrogrades the sequence of time-intervals, storing the
 * results back in startptr onwards.
 */

int retrograde_sequence_of_time_intervals(int endtime,int count,double *startptr,dataptr dz)
{
	double newtime, lasttime, tsize;
	int n;
	double *p  = startptr;
	dz->ptr[TSTR_QBUF] = dz->parray[TSTR_QBUF] + (count-1);
	newtime = endtime;
	for(n=0;n<(count-1);n++) {
		lasttime   = *p++;
		tsize      = *p - lasttime;
		newtime   -= tsize;
		*dz->ptr[TSTR_QBUF]-- = newtime; 
	}
	if(dz->ptr[TSTR_QBUF]!=dz->parray[TSTR_QBUF]) {
		sprintf(errstr,"counting problem: retrograde_sequence_of_time_intervals()\n");
		return(PROGRAM_ERROR);
	}
	*dz->ptr[TSTR_QBUF] = *p; /* put starttime at start of intervals inverted array */
	p  = startptr;
	for(n=0;n<count;n++)
		*p++ = *dz->ptr[TSTR_QBUF]++;
	return(FINISHED);
}

/******************************************* DECOHERE ****************************************/

int decohere(float *fbuf,dataptr dz)
{
	int *store = dz->iparray[0];	
	int cc, vc, j, k;
	float ampj, ampk;
	double randval, chanfrq, frq;
	for(cc = 0; cc < dz->clength;cc++)
		store[cc] = cc;

	//	Sort channel numbers into order of ascending loudness
	
	for(k = 0; k < dz->clength - 1; k++) {
		vc = store[k] * 2;							//	vc is index of amps in buffer, cc is count of chans (chan = amp&frq vals)
		ampk = fbuf[AMPP];							//	AMPP = vc
		for(j = k+1; j < dz->clength; j++) {
			vc = store[j] * 2;
			ampj = fbuf[AMPP];
			if(ampj < ampk) {						//	If item higher up array (ampj) has lower amp than item lower down array (ampk)
				cc = store[k];						//	Swap the stored channel numbers
				store[k] = store[j];				
				store[j] = cc;
				ampk = ampj;						//	And reset amplitude ampk to the lower amp
			}
		}
	}

	//	Randomise frqs in lower amplitude chans

	for(k = 0; k < dz->iparam[DISC_RATIO]; k++) {
		cc = store[k];
		frq = (double)cc * dz->chwidth;
		randval = ((drand48() * 2.0) - 1.0)/2.0;			//	Range -1/2 to +1/2
		randval *= dz->param[DISC_RAND];					//	Range -DISC_RAND/2 to +DISC_RAND/2
		chanfrq = frq + randval;
		if(chanfrq < PITCHZERO || chanfrq >= dz->nyquist)	//	In lowest channel, avoid v. low or -ve frqs (chan centre is at zero)
			chanfrq = frq - randval;						//	In highest channel, avoid frq beyond nyquist ((chan centre at nyquist)
		vc = cc * 2;
		fbuf[FREQ] = (float)chanfrq;						//	FREQ = vc+1
	}

	return FINISHED;
}
