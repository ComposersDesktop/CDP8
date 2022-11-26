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
#include <science.h>
#include <ctype.h>
#include <sfsys.h>
#include <string.h>
#include <srates.h>

#define	TWEET_PDATA	0
#define	TWEET_MIN	1	
#define WINDIV (6.0)
#define FOFCOSWINDOW (.002)
#define MINSPEECHGLITCH (0.15)				// Assume minimum time for speaking voice to leap > 8va and back is > 150 mS
#define PITCHERROR (1.5)
#define ALMOST_OCT (1.9)

#ifdef unix
#define round(x) lround((x))
#endif

char errstr[2400];

int anal_infiles = 1;
int	sloom = 0;
int sloombatch = 0;

const char* cdp_version = "6.1.0";

//CDP LIB REPLACEMENTS
static int setup_tweet_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_tweet_param_ranges_and_defaults(dataptr dz);
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
static int get_the_mode_from_cmdline(char *str,dataptr dz);
static int setup_and_init_input_brktable_constants(dataptr dz,int brkcnt);
static int handle_the_special_data(int *cmdlinecnt,char ***cmdline,dataptr dz);
static int get_exclude_data(char *filename,dataptr dz);
static int extract_formant_shape(double *env,int segdur,int maxoutdur,dataptr dz);
static int recreate_tweet_sndbufs(dataptr dz);
static int create_tweet_sndbufs(dataptr dz);
static int silence_lolevel_fofs(int *outsegs,int *silence,int outcnt,dataptr dz);
static int smooth_cuts(int *cut,int *cutcnt,int cutstart,dataptr dz);
static int find_min_energy_downward_zero_crossing_point
			(int *n,int *trofpnt,int trofpntcnt,double *scanarray,int *cutcnt,int *cut,int cutstart,dataptr dz);
static int find_the_local_peaks(int *here,int *there,float *buf,int *n,int trofpntcnt,int *trofpnt,
			int *startsamp,int *endsamp,int losamp, int *cut, int cutcnt, double *localpeak, double *scanarray,
			int *localpeakcnt,int *first_downcross,dataptr dz);
static int	mark_cut(int *cutcnt,int *cut,int localpeakcnt,double *startarray,int here,int there,
			 int startsamp,int first_downcross,double starttime,int msg,dataptr dz);
static int get_envelope_troughs(int *trofpnt,int *trofpntcnt,int envcnt,dataptr dz);
static int get_min_wsize(int *minwsize, dataptr dz);
static int extract_pitch_dependent_env_from_sndfile(int minwsize,int *maxwsize,dataptr dz);
static int check_tweet_param_validity_and_consistency(dataptr dz);
static int tweet_param_preprocess(dataptr dz);
static float getmaxsampr(int startsamp, int sampcnt,float *buffer);
static void smooth_oct_glitches(dataptr dz);
static int read_validpitch_wsize_in_samps_from_brktable(double thistime,int kk,dataptr dz);
static int count_zerocrossings(int here,int there,float *buf);
static int triangulate_env(int *here,int *there,int ideal_place,float *buf);
static int next_zero_cross(int here,dataptr dz);
static int previous_zero_cross(int here,int firstzero,dataptr dz);
static double autocorrelate(int n,int m,float *buf);
static int auto_correlate(int start,int *at,int end,int realend,int minlen,double pitchseg,int kk,dataptr dz);
static int next_down_zcross(int here,int hibound,float *buf);
static int last_down_zcross(int here,int lobound,float *buf);
static int silence_zero_signal_areas(int *outsegs,int *silence,int outcnt,dataptr dz);
static int impulse(int seglen,double time,dataptr dz);
static int tweet(dataptr dz);

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
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
		dz->maxmode = 3;
		if((exit_status = get_the_mode_from_cmdline(cmdline[0],dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(exit_status);
		}
		cmdline++;
		cmdlinecnt--;
		// setup_particular_application =
		if((exit_status = setup_tweet_application(dz))<0) {
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
	if((exit_status = setup_tweet_param_ranges_and_defaults(dz))<0) {
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

//	handle_extra_infiles() : redundant
	// handle_outfile() = 
	if((exit_status = handle_the_outfile(&cmdlinecnt,&cmdline,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}

//	handle_formants()			redundant
//	handle_formant_quiksearch()	redundant
//	handle_special_data ..........
	if((exit_status = handle_the_special_data(&cmdlinecnt,&cmdline,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {		// CDP LIB
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
//	check_param_validity_and_consistency....
	if((exit_status = check_tweet_param_validity_and_consistency(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	//param_preprocess ......
	if((exit_status = tweet_param_preprocess(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	is_launched = TRUE;
	dz->bufcnt = 3;
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

	if((exit_status = create_tweet_sndbufs(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	//spec_process_file =
	if((exit_status = tweet(dz))<0) {
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
	dz->infilecnt = 1;
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

/************************* SETUP_TWEET_APPLICATION *******************/

int setup_tweet_application(dataptr dz)
{
	int exit_status;
	aplptr ap;
	if((exit_status = establish_application(dz))<0)		// GLOBAL
		return(FAILED);
	ap = dz->application;
	// SEE parstruct FOR EXPLANATION of next 2 functions
	if(dz->mode == 2)
		exit_status = set_param_data(ap,0   ,4,2,"Dd00");
	else
		exit_status = set_param_data(ap,0   ,4,4,"DdID");
	if(exit_status<0)
		return(FAILED);
	if((exit_status = set_vflgs(ap,"",0,"","w",1,0,"0"))<0)
		return(FAILED);
	// set_legal_infile_structure -->
	dz->has_otherfile = FALSE;
	// assign_process_logic -->
	dz->input_data_type = SNDFILES_ONLY;
	dz->process_type	= UNEQUAL_SNDFILE;	
	dz->outfiletype  	= SNDFILE_OUT;
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

/************************* SETUP_TWEET_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_tweet_param_ranges_and_defaults(dataptr dz)
{
	int exit_status;
	aplptr ap = dz->application;
	// set_param_ranges()
	ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
	// NB total_input_param_cnt is > 0 !!!
	if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
		return(FAILED);
	// get_param_ranges()
	ap->lo[TWT_PDAT]  = -2.0;			//	Pitch data
	ap->hi[TWT_PDAT]  = dz->nyquist;
	ap->default_val[TWT_PDAT] = 440.0;
	ap->lo[TWT_MIN]	  = -60.0;
	ap->hi[TWT_MIN]	  = 0.0;
	ap->default_val[TWT_MIN]  = 0.0;
	if(dz->mode != 2) {
		ap->lo[TWT_PKCNT] = 1;
		if(dz->mode == 1)
			ap->hi[TWT_PKCNT] = 4000;
		else
			ap->hi[TWT_PKCNT] = 200;
		ap->default_val[TWT_PKCNT] = 4;
		ap->lo[TWT_CHIRP] = 0;
		ap->hi[TWT_CHIRP] = 30;
		ap->default_val[TWT_CHIRP] = 0;
	}
	dz->maxmode = 3;
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
			if((exit_status = setup_tweet_application(dz))<0)
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
	usage2("tweet");
	return(USAGE_ONLY);
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

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	if(!strcmp(prog_identifier_from_cmdline,"tweet"))				dz->process = TWEET;
	else {
		sprintf(errstr,"Unknown program identification string '%s'\n",prog_identifier_from_cmdline);
		return(USAGE_ONLY);
	}
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

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
	if(!strcmp(str,"tweet")) {
		fprintf(stderr,
	    "USAGE:\n"
	    "tweet tweet 1 infile outfile exclude pitchdata minlevel pkcnt chirp [-w]\n"
	    "tweet tweet 2 infile outfile exclude pitchdata minlevel frq chirp [-w]\n"
	    "tweet tweet 3 infile outfile exclude pitchdata minlevel [-w]\n"
		"\n"
		"Replace FOFs in vocal sound by synthetic tweets or (mode 3) noise.\n"
		"\n"
		"PITCHDATA   A breakpoint file with time and frq information.\n"
		"            (Use \"repitch getpitch 2\").\n"
		"            May contain zeros (indicating moments of no-signal)\n"
		"            but it must contain SOME significant frequency information.\n"
		"EXCLUDE     Areas in src from which FOFs will NOT be extracted.\n"
		"            If not wanted, enter \"0\". Otherwise pairs of times in textfile.\n"
		"MINLEVEL    A level in dBs below level of loudest FOF found,\n"
		"            FOFs below this level rejected. NB Zero = NO FOFS rejected.\n"
		"PKCNT       Number of peaks in impulse. (1-200) (frq changes with FOF size)\n"
		"FRQ         Freq of peaks in impulse.   (1-200) (frq fixed).\n"
		"CHIRP       Glissing of impulse. (Range 0 to 30) (Care, output may clip).\n"
		"\n"
		"-w    FOFs are windowed (cosine smooth of edges)\n");
	} else
		fprintf(stdout,"Unknown option '%s'\n",str);
	return(USAGE_ONLY);
}

int usage3(char *str1,char *str2)
{
	fprintf(stderr,"Insufficient parameters on command line.\n");
	return(USAGE_ONLY);
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

/************************* TWEET *******************************/

int tweet(dataptr dz)
{
	int exit_status;
/*	double maxenv = 10000.0;*/
	int n, m, k, j, cutcnt, ipos, trofpntcnt = 0;
	int outcnt, fofcnt,exclustt, excluend, maxoutdur=0, inbufstart, inbufend, obufpos, splen;
	int envcnt, minwsize, maxwsize = 0, bigarray, cutstart = 0, thiscut, lastcut=0;
	int segdur, segstart, segend;
	int *trofpnt, *cuttime, *outsegs, *excludes = NULL;
	double *scanarray, *env, srate = (double)dz->infile->srate;
	float *ibuf = dz->sampbuf[0], *obuf, *chpbuf;
	int *silence = NULL, done, init;
	int jj, kk;
	int cosedgelen = (int)round(FOFCOSWINDOW * dz->infile->srate);
	int min_fofsamps_for_windowing = (cosedgelen * 2) + 1;
	double *cosedge, val, time, splic;
	if((cosedge = (double *)malloc(cosedgelen * sizeof(double)))==NULL) {
		sprintf(errstr,"Insufficient memory for FOF windowing.\n");
		return(MEMORY_ERROR);
	}
	for(jj=0,kk=1;jj<cosedgelen;jj++,kk++) {
		val = (double)kk/(double)cosedgelen;
		val = (cos(val * PI) + 1.0)/2.0;
		cosedge[jj] = 1.0 - val;
	}
	if(dz->itemcnt > 0)								//	Get exclude-data, if it is being used
		excludes = dz->lparray[0];

	if((exit_status = get_min_wsize(&minwsize,dz)) < 0)
		return exit_status;
	if(((envcnt = dz->insams[0]/minwsize) * minwsize)!=dz->insams[0])
		envcnt++;
	if((dz->env=(float *)malloc(((envcnt+20) * 2) * sizeof(float)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for envelope array.\n");
		return(MEMORY_ERROR);
	}
	bigarray = envcnt+20;
	memset(dz->env,0,(envcnt+20) * 2 * sizeof(float));	/* PRESET env VALS TO ZERO */
					/* PART ENVELOPE vals CAN THEN BE STORED, where env-extract overlaps a buffer boundary */
	fprintf(stdout,"INFO: Extracting pitch-related envelope from file.\n");
	fflush(stdout);
	if((exit_status = extract_pitch_dependent_env_from_sndfile(minwsize,&maxwsize,dz))<0)
		return(exit_status);
	envcnt = dz->envend - dz->env;

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
	if((outsegs = (int *)malloc(bigarray * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY TO ANALYSE ENVELOPE.\n");
		return(MEMORY_ERROR);
	}
	if((silence = (int *)malloc(bigarray/2 * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY TO MARK SILENCES.\n");
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
		if((exit_status = find_min_energy_downward_zero_crossing_point(&n,trofpnt,trofpntcnt,scanarray,&cutcnt,cuttime,cutstart,dz)) < 0)
			return(exit_status);
		if(!dz->zeroset)
			cutcnt++;
	}
	if((exit_status = smooth_cuts(cuttime,&cutcnt,cutstart,dz))<0)
		return(exit_status);

	n = 0;
	m = 0;
	thiscut = 0;
	done = 0;
	outcnt = 0;
	init = 0;

	if(dz->itemcnt > 0) {		// REMOVE FOFS IN EXCLUDED AREAS
		while(n < dz->itemcnt) {
			exclustt = excludes[m];
			excluend = excludes[m+1];
			while(cuttime[thiscut] < exclustt) {
				if(init) {
					silence[outcnt/2] = 0;
					outsegs[outcnt++] = cuttime[lastcut];
					outsegs[outcnt++] = cuttime[thiscut];
				}
				lastcut = thiscut;
				init = 1;
				if(++thiscut >= cutcnt) {
					done = 1;
					break;
				}
			}
			if(done)
				break;
			silence[outcnt/2] = 1;
			outsegs[outcnt++] = cuttime[lastcut];					//	Mark as silent
			while(cuttime[thiscut] < excluend) {
				thiscut++;
				if(thiscut >= cutcnt) {
					done = 1;
					break;
				}
			}
			if(done)
				break;
			outsegs[outcnt++] = cuttime[thiscut];
			lastcut = thiscut;
			if(++thiscut >= cutcnt)
				break;
			n++;
			m += 2;

		}
	}
	while(thiscut < cutcnt) {
		if(init) {
			silence[outcnt/2] = 0;
			outsegs[outcnt++] = cuttime[lastcut];
			outsegs[outcnt++] = cuttime[thiscut];
		}
		lastcut = thiscut;
		init = 1;
		thiscut++;
	}
	if((exit_status = silence_zero_signal_areas(outsegs,silence,outcnt,dz)) < 0)
		return(exit_status);
	if(dz->param[1] < 0.0) {
		if((exit_status = silence_lolevel_fofs(outsegs,silence,outcnt,dz)) < 0)
			return(exit_status);
	}
	fofcnt = outcnt/2;

	n = 0;
	maxoutdur = 0;
	for(n=0,j=0,k=1;n<fofcnt;n++,j+=2,k+=2)
		maxoutdur = max(outsegs[k] - outsegs[j],maxoutdur);
	dz->buflen = max(maxoutdur + 64,dz->buflen);				//	Recalc buflen for for output phase

	if((exit_status = recreate_tweet_sndbufs(dz))<0)
		return exit_status;

	ibuf   = dz->sampbuf[0];
	obuf   = dz->sampbuf[1];
	chpbuf = dz->sampbuf[2];
	
	if((env = (double *)malloc(maxoutdur * sizeof(double))) == NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY establishing level check array.\n");
		return(MEMORY_ERROR);
	}
	if((sndseekEx(dz->ifd[0],0,0)<0)){
        sprintf(errstr,"sndseek() failed\n");
        return SYSTEM_ERROR;
    }
	dz->total_samps_read = 0;
	if((exit_status = read_samps(ibuf,dz))<0)
		return(exit_status);
	inbufstart = 0;
	inbufend = dz->ssampsread;
	obufpos = 0;
	n = 0;
	j = 0;
	k = 1;
	time = 0.0;
	memset((char*)obuf,0,dz->buflen * sizeof(float));
	while(n<fofcnt) {
		segstart = outsegs[j];
		segend   = outsegs[k];
		segdur   = segend - segstart;
		if(silence[n]) {				//	Output silence
			for(jj=0;jj < segdur;jj++) {
				obuf[obufpos] = 0.0f;
				if(++obufpos >= dz->buflen) {
					if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
						return exit_status;
					obufpos = 0;
				}
			}
		} else {																	//	Check we have all of segment in current inbuf
			if(segstart < inbufstart || segend >= inbufend) {						//	and if not, get a new buffer
				if((sndseekEx(dz->ifd[0],segstart,0)<0)){
					sprintf(errstr,"sndseek() failed\n");
					return SYSTEM_ERROR;
				}
				dz->total_samps_read = segstart;
				if((exit_status = read_samps(ibuf,dz))<0)
					return(exit_status);
				inbufstart = segstart;
			}
			ipos = segstart - inbufstart;
			segend = ipos + segdur;
			if(segend > dz->buflen) {
				sprintf(errstr,"FOF too long for buffer.\n");
				return(PROGRAM_ERROR);
			}
			memcpy((char*)chpbuf,(char*)(ibuf + ipos),segdur * sizeof(float));		//	Copy FOF to chpbuf

			if(dz->vflag[FOF_UNWIN]) {	//											//	If ness cosin-splice the FOF
				if(segdur >= min_fofsamps_for_windowing)
					splen = cosedgelen;
				else
					splen = segdur/2;
				for(jj = 0,kk = segdur - 1;jj < segdur;jj++,kk--) {
					val = chpbuf[jj];
					if(jj < splen) {
						splic = (double)(jj+1)/(double)splen;
						splic = (cos(splic * PI) + 1.0)/2.0;
						splic = 1.0 - splic;
						val *= splic;
					} else if(kk < splen) {
						splic = (double)(kk+1)/(double)splen;
						splic = (cos(splic * PI) + 1.0)/2.0;
						splic = 1.0 - splic;
						val *= splic;
					}
					chpbuf[jj] = (float)val;
				}
			}
			if((exit_status = extract_formant_shape(env,segdur,maxoutdur,dz))<0)	//	Extract outer envelope of formant
				return exit_status;
			if((exit_status = impulse(segdur,time,dz))<0)							//	Generate chirp to fit
				return exit_status;
			for(kk=0;kk<segdur;kk++)												//	superimpose formant shape on chirp
				chpbuf[kk] = (float)(chpbuf[kk] * env[kk]);
			
			for(jj=0;jj < segdur;jj++) {
				obuf[obufpos] = chpbuf[jj];
				if(++obufpos >= dz->buflen) {
					if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
						return exit_status;
					obufpos = 0;
				}
			}
		}
		n++;
		j += 2;
		k += 2;
		time = (double)(dz->total_samps_written + obufpos)/srate;
	}
	if(obufpos > 0) {
		if((exit_status = write_samps(obuf,obufpos,dz))<0)
			return(exit_status);
	}
	return FINISHED;
}

/************************* EXTRACT_PITCH_DEPENDENT_ENV_FROM_SNDFILE *******************************/

int extract_pitch_dependent_env_from_sndfile(int minwsize,int *maxwsize,dataptr dz)
{
	int exit_status;
	double tconvertor = 1.0/(double)dz->infile->srate;
	double fconvertor = (double)dz->infile->srate/WINDIV;
	int  start_samp = 0, envwindow_sampsize, big_envwindow_sampsize, here = 0, start_buf = 0;
	float *env = dz->env, *ibuf = dz->sampbuf[0], *ovflwbuf = dz->sampbuf[1];
	double time = 0.0;

	if((dz->ssampsread = fgetfbufEx(ibuf, dz->buflen * 2,dz->ifd[0],0)) < 0) {
		sprintf(errstr,"Can't read samples from input soundfile\n");		//	NB Uses a double length buf for reading envelope
		return(SYSTEM_ERROR);
	}
	if((exit_status = read_value_from_brktable(time,TWT_PDAT,dz))<0)
		return(exit_status);
	if(dz->param[TWT_PDAT] < 0.0)		/* no-pitch */
		envwindow_sampsize = minwsize;
	else
		envwindow_sampsize = (int)round(fconvertor/dz->param[TWT_PDAT]);
	while(here < dz->insams[0]) {
		if(here - start_buf >= dz->buflen) {
			memset((char *)ibuf,0,dz->buflen * sizeof(float));
			memcpy((char *)ibuf,(char *)ovflwbuf,dz->buflen * sizeof(float));
			start_buf += dz->buflen;
			if(dz->total_samps_read < dz->insams[0]) {
				if((dz->ssampsread = fgetfbufEx(ovflwbuf, dz->buflen,dz->ifd[0],0)) < 0) {
					sprintf(errstr,"Can't read samples from input soundfile\n");
					return(SYSTEM_ERROR);
				}
			}
		}	
		big_envwindow_sampsize = envwindow_sampsize * 2;
		*env++ = (float)time;
		if(here + big_envwindow_sampsize >= dz->insams[0]) {
			here -= envwindow_sampsize;
			big_envwindow_sampsize = dz->insams[0] - here;
			*env++ = getmaxsampr(start_samp,big_envwindow_sampsize,ibuf);
			break;
		}
		start_samp = here - start_buf;
		*env++ = getmaxsampr(start_samp,big_envwindow_sampsize,ibuf);
		here += envwindow_sampsize;
		time   = (float)(here * tconvertor);
		if((exit_status = read_value_from_brktable(time,TWT_PDAT,dz))<0)
			return(exit_status);
		if(dz->param[TWT_PDAT] < 0.0)		/* no-pitch */
			envwindow_sampsize = minwsize;
		else
			envwindow_sampsize = (int)round(fconvertor/dz->param[TWT_PDAT]);
		if(envwindow_sampsize > *maxwsize)
			*maxwsize = envwindow_sampsize;
	}
	*maxwsize *= 2;	/* using overlapping windows */
	dz->envend = env;
	memset((char *)ibuf,0,dz->buflen * 2 * sizeof(float));
	return FINISHED;
}

/************************** GET_MIN_WSIZE **************************/

int get_min_wsize(int *minwsize, dataptr dz) {
	int n,m;
	int wsize;
	double fconvertor = (double)dz->infile->srate/WINDIV, frq;
	*minwsize = dz->insams[0];
	for(n=0,m=1;n<dz->brksize[TWEET_PDATA];n++,m+=2) {
		frq = dz->brk[TWEET_PDATA][m];
		if(frq > 0.0) {
			wsize = (int)round(fconvertor/frq);
			if(wsize < *minwsize)
				*minwsize = wsize;
		}
	}
	if(*minwsize >= dz->insams[0])
		return(DATA_ERROR);
	return FINISHED;
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

/************************** FIND_MIN_ENERGY_DOWNWARD_ZERO_CROSSING_POINT **************************/

int find_min_energy_downward_zero_crossing_point(int *n,int *trofpnt,int trofpntcnt,
			double *scanarray,int *cutcnt,int *cut,int cutstart,dataptr dz)
{
	int exit_status;
	int check;
	float starttime, val, *buf = dz->sampbuf[0];
	int wsize, thissamp, hisamp, losamp, j, zc_cnt, at;
	int endsamp, startsamp, seglen;
	int newpos, diff, losampinbuf, hisampinbuf, here, there, localpeakcnt;
	int k, first_downcross, mindiff, goalseglen, newseglen, thiscut;
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
	wsize = read_validpitch_wsize_in_samps_from_brktable(starttime,0,dz);
	thissamp = (int)round(starttime * (double)dz->infile->srate);
	losamp = thissamp;
	if((exit_status = read_value_from_brktable(starttime,0,dz))<0)	/* get pitch at start of window */
		return(exit_status);
	if(losamp >= dz->insams[0])
		return(FINISHED);
	if(losamp >= endsamp || losamp < startsamp) {
		newpos = (losamp/F_SECSIZE) * F_SECSIZE;
		if((sndseekEx(dz->ifd[0],newpos,0)<0)){
            sprintf(errstr,"sndseek() failed\n");
            return SYSTEM_ERROR;
        }
		dz->total_samps_read = newpos;
		if((exit_status = read_samps(buf,dz))<0)
			return(exit_status);
		endsamp = dz->total_samps_read;
		startsamp = endsamp - dz->ssampsread;
	}
	hisamp = min(dz->insams[0],thissamp + wsize);	/* get end-of-search area from  wtime & half-wsize */
	if(hisamp >= dz->insams[0])
		hisamp = dz->insams[0];
	if(hisamp > endsamp) {
		newpos = (losamp/F_SECSIZE) * F_SECSIZE;			/* if end-of--search beyond end of current buf */
        /* adjust buffer to contain whole search area */
		if((sndseekEx(dz->ifd[0],newpos,0)<0)){
            sprintf(errstr,"sndseek() failed\n");
            return SYSTEM_ERROR;
        }
        dz->total_samps_read = newpos;
		if((exit_status = read_samps(buf,dz))<0)
			return(exit_status);
		endsamp = dz->total_samps_read;
		startsamp = endsamp - dz->ssampsread;
	}
	losampinbuf = losamp - startsamp;					/* get search ends relative to buffer */
	hisampinbuf = hisamp - startsamp;
	if(dz->param[0] > 0) {
		pitchwindow = (int)round((double)dz->infile->srate/dz->param[0]);
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

	losamp = here + startsamp;

	dz->zeroset = 0;
	if((exit_status = find_the_local_peaks(&here,&there,buf,n,trofpntcnt,trofpnt,
		 &startsamp,&endsamp,losamp,cut,*cutcnt,scanarray,scanarray,&localpeakcnt,&first_downcross,dz) )<0)
		 return(exit_status);
	if(exit_status == FINISHED)
		dz->zeroset = 1;
	if(dz->zeroset)
		return(FINISHED);
	if((exit_status = mark_cut(cutcnt,cut,localpeakcnt,scanarray,here,there,startsamp,first_downcross,starttime,1,dz))<0)
		return(exit_status);
	if((*cutcnt)-1 < cutstart)		/* FIND LENGTH OF SEGMENT GENERATED */
		seglen = cut[*cutcnt];
	else
		seglen = cut[*cutcnt] - cut[(*cutcnt)-1];
	if(seglen == 0) {		/* ELIMINATE ZERO-LENGTH SEGMENTS (CUTS AT SAME TIME) */
		(*cutcnt)--;
	} else if(*cutcnt > cutstart && dz->param[0] > 0) {
							/* CHECK THE LENGTH OF THE WINDOW AGAINST WHAT WE'D PREDICT FROM THE PITCH VALUE */	
		check = 1;
		while(check) {
			official_pitchlen = (double)dz->infile->srate/dz->param[0];
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
									/* TOO int ITEM */
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
                if((sndseekEx(dz->ifd[0],lastcut,0)<0)){
                    sprintf(errstr,"sndseek() failed\n");
                    return SYSTEM_ERROR;
                }
				dz->total_samps_read = lastcut;
				if((exit_status = read_samps(buf,dz))<0)
					return(exit_status);
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
				if((sndseekEx(dz->ifd[0],startsamp,0)<0)){
                    sprintf(errstr,"sndseek() failed\n");
                    return SYSTEM_ERROR;
                }
				dz->total_samps_read = startsamp;
				if((exit_status = read_samps(buf,dz))<0)
					return(exit_status);
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
			if(isup == -1)
				zc_cnt++;
			isup = 1;
			here++;
			if(here >= there) {
				done = 1;
				break;
			}
			val = buf[here];
		}
		if(done)
			break;
		while(val < 0.0) {
			if(isup == 1)
				zc_cnt++;
			isup = -1;
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

/******************************** FIND_THE_LOCAL_PEAKS ********************************/

int find_the_local_peaks(int *here,int *there,float *buf,int *n,int trofpntcnt,int *trofpnt,
		 int *startsamp,int *endsamp,int losamp, int *cut, int cutcnt, double *localpeak, double *scanarray,
		 int *localpeakcnt,int *first_downcross,dataptr dz)
{
	int exit_status, notfound = 0, finished = 0, below, bufinc = 0;
	int maxat=0;
	float maxval=0.0f, val;
	int wsize, hisamp,losampinbuf,hisampinbuf;
	int orighere, origthere;
	*first_downcross = -1;
	wsize = *there - *here;
	while(!finished) {
		hisamp = min(dz->insams[0],losamp + wsize);		/* get end-of-search area from  wtime & half-wsize */
		if(hisamp >= dz->insams[0])
			hisamp = dz->insams[0];
		losampinbuf = losamp - *startsamp;					/* get search ends relative to buffer */
		hisampinbuf = hisamp - *startsamp;
		if(hisampinbuf >= dz->buflen) {
			if((sndseekEx(dz->ifd[0],losamp,0)<0)){
                sprintf(errstr,"sndseek() failed\n");
                return SYSTEM_ERROR;
            }
			dz->total_samps_read = losamp;
			if((exit_status = read_samps(dz->sampbuf[0],dz))<0)
				return(exit_status);
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
				*first_downcross = *here;					/* mark 1st down-zerocross in search area */
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
			dz->zeroset = 1;
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
		if(*first_downcross < 0)
			dz->zeroset = 1;
		if((maxval > 0.0) && (*first_downcross < 0))
			*first_downcross = *here;

		*localpeak++ = (double)maxval;
		*localpeak++ = (double)(maxat + *startsamp);
	}	
	*localpeakcnt = (localpeak - scanarray)/2;
	if(*localpeakcnt == 1) {
		if(!dz->zeroset) {			/* If only 1 peak, the cut points are at the first_downcross if NOT zeroset */
			*here = *first_downcross;
		} else {
			*here = orighere;		// REDUNDANT, as, if dz->zeroset then "mark_cut" is not called.
			*there = origthere;
		}
	}
	return CONTINUE;
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
		cut[*cutcnt] = here + startsamp;			/* return location of zero crossing after peak */
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

/***************************** SMOOTH_CUTS *****************************/

int smooth_cuts(int *cut,int *cutcnt,int cutstart,dataptr dz)
{
	int exit_status;
	int n, k, seg0, seg1, seg2, minseg, end, start, *thiscut, srchlen, minlen;
	int pitchseglim, pitchseglo=0, realend, lastcutval;
	double pitchseg, lastpitchseg=0.0, time, big_seg, this_seg, last_seg, big_int, this_int, last_int;
	int last_cut, this_cut, next_cut;
	double last_ratio, next_ratio;

	if((sndseekEx(dz->ifd[0],0,0)<0)){
        sprintf(errstr,"sndseek() failed\n");
        return SYSTEM_ERROR;
    }
	dz->total_samps_read = 0;
	if((exit_status = read_samps(dz->sampbuf[0],dz))<0)
		return(exit_status);
	for(n=3 + cutstart;n<*cutcnt;n++) {
		lastcutval = 0;
		end     = cut[n];
		realend = end;
		time = (double)cut[n-3]/(double)dz->infile->srate;
		if((exit_status = read_value_from_brktable(time,0,dz))<0)
			return(exit_status);									/* Get pitch at start of cut segment */
		if(dz->param[0] > 0.0) {									/* if it's a true pitch ... */
			pitchseg = (double)dz->infile->srate/dz->param[0];		/* Find corresponding segment length */
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
		if(n >= 3) {
			last_cut = cut[n-2] - cut[n-3];
			this_cut = cut[n-1] - cut[n-2];
			next_cut = cut[n]   - cut[n-1];
			if(((last_cut > this_cut) && (next_cut > this_cut)) || ((last_cut < this_cut) && (next_cut < this_cut))) {
				if(last_cut > this_cut) {
					last_ratio = (double)last_cut/(double)this_cut;
					next_ratio = (double)next_cut/(double)this_cut;
				} else {
					last_ratio = (double)this_cut/(double)last_cut;
					next_ratio = (double)this_cut/(double)next_cut;
				}
				if((last_ratio > 1.3) || (next_ratio > 1.3)) {
					if((exit_status = auto_correlate(start,thiscut,end,realend,minlen,pitchseg,0,dz))<0)
						return(exit_status);
				}
			}
		}
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

/****************************** SILENCE_ZERO_SIGNAL_AREAS *********************************/

int silence_zero_signal_areas(int *outsegs,int *silence,int outcnt,dataptr dz)
{
	double *frqdata = dz->brk[0];
	int n, segcnt = 0;
	int zeropnt[2]={0}, zerocnt = 0, brklen = dz->brksize[0] * 2, frqindx, timindx, nufrqindx, nutimindx;
	n = 0;
	timindx = 0;
	frqindx = 1;
	while (n < dz->brksize[0]) {
		zerocnt = 0;
		if(frqdata[frqindx] < 0.0) {
			zeropnt[0] = (int)round(frqdata[timindx] * (double)dz->infile->srate);
			zerocnt = 1;
			nufrqindx =frqindx + 2;
			nutimindx =timindx + 2;
			while(nufrqindx < brklen) {
				if(frqdata[nufrqindx] < 0.0) {	
					zeropnt[1] = (int)round(frqdata[nutimindx] * (double)dz->infile->srate);
					zerocnt = 2;
					frqindx = nufrqindx;
					timindx = nutimindx;
					n++;
					nufrqindx += 2;
					nutimindx += 2;
				} else {
					break;
				}
			}
			while(outsegs[segcnt] < zeropnt[0]) {
				if((segcnt+=2) >= outcnt)
					return FINISHED;
			}
			segcnt-=2;
			silence[segcnt/2] = 1;
			if (zerocnt > 1) {
				while(outsegs[segcnt] <= zeropnt[1]) {
					silence[segcnt/2] = 1;
					segcnt += 2;
					if(segcnt >= outcnt)
						return FINISHED;
				}
				
			}
		}
		n++;
		timindx += 2;
		frqindx += 2;
	}
	return(FINISHED);
}

/****************************** SILENCE_LOLEVEL_FOFS *********************************/

int silence_lolevel_fofs(int *outsegs,int *silence,int outcnt,dataptr dz)
{
	int exit_status;
	int n, m, totalsamps = 0, endsegindx, levelcnt, ipos, levelsiz = (outcnt)/2;
	double maxsamp = 0.0, maxlocalsamp = 0.0, *level, lolim;
	float *ibuf = dz->sampbuf[0];
	if((level = (double *)malloc(levelsiz * sizeof(double))) == NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY establishing level check array.\n");
		return(MEMORY_ERROR);
	}
	if((sndseekEx(dz->ifd[0],0,0)<0)){
        sprintf(errstr,"sndseek() failed\n");
        return SYSTEM_ERROR;
    }
	dz->total_samps_read = 0;
	if((exit_status = read_samps(ibuf,dz))<0)
		return(exit_status);
	endsegindx = 1;
	levelcnt = 0;
	while(dz->ssampsread > 0) {
		ipos = 0;
		while(ipos < dz->ssampsread) {
			maxsamp = max(maxsamp,fabs(ibuf[ipos]));
			maxlocalsamp = max(maxlocalsamp,fabs(ibuf[ipos]));
			ipos++;
			totalsamps++;
			if(totalsamps >= outsegs[endsegindx]) {
				if(levelcnt < levelsiz) {
					level[levelcnt++] = maxlocalsamp;
					maxlocalsamp = 0.0;
					endsegindx += 2;
				}
			}
		}
		if((exit_status = read_samps(ibuf,dz))<0)
			return(exit_status);
	}
	if((sndseekEx(dz->ifd[0],0,0)<0)){
        sprintf(errstr,"sndseek() failed\n");
        return SYSTEM_ERROR;
    }
	dz->total_samps_read = 0;
	if((exit_status = read_samps(ibuf,dz))<0)
		return(exit_status);
	lolim = maxsamp * dbtogain(dz->param[1]);
	n = 0;
	m = 0;
	while(n < outcnt) {
		if(level[m] < lolim)
			silence[m] = 1;
		n += 2;
		m++;
	}
	return(FINISHED);
}

/*************************** CREATE_TWEET_SNDBUFS **************************/

int create_tweet_sndbufs(dataptr dz)
{
	int bigbufsize;
	if(dz->sbufptr == 0 || dz->sampbuf==0) {
		sprintf(errstr,"buffer pointers not allocated: create_fofex_sndbufs()\n");
		return(PROGRAM_ERROR);
	}
	bigbufsize = (int)Malloc(-1);
	dz->buflen = bigbufsize/sizeof(float);
	if((dz->bigbuf = (float *)malloc(bigbufsize * 2)) == NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create initial sound buffer.\n");
		return(PROGRAM_ERROR);
	}
	dz->sampbuf[0] = dz->bigbuf;				//	ibuf
	dz->sampbuf[1] = dz->bigbuf + dz->buflen;	//	ovflw
	return(FINISHED);
}

/*************************** RECREATE_TWEET_SNDBUFS **************************/

int recreate_tweet_sndbufs(dataptr dz)
{
	int n;
	int bigbufsize;
	free(dz->bigbuf);
	dz->bufcnt = 3;
	bigbufsize = (int)Malloc(-1);
	bigbufsize = max(dz->buflen * sizeof(float),(unsigned int)bigbufsize);
	dz->buflen = bigbufsize/sizeof(float);
	if((dz->bigbuf = (float *)malloc(bigbufsize  * (dz->bufcnt+1))) == NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to recreate sound buffers.\n");
		return(PROGRAM_ERROR);
	}
	for(n=0;n<dz->bufcnt;n++)
		dz->sbufptr[n] = dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
	dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
	return(FINISHED);
}

/*************************** EXTRACT_FORMANT_SHAPE **************************/

int extract_formant_shape(double *env,int segdur,int maxoutdur,dataptr dz)
{
	int phase = 0, maxposfirst, phasecnt;
	float *chpbuf = dz->sampbuf[2];
	double maxval, lastval, diff, frac, ratio;
	int n = 0, m, k, step, maxpos = 0, maxposend, lastpos;
	memset((char *)env,0,maxoutdur * sizeof(double));
	while(chpbuf[n] == 0.0) {
		n++;
		if(n >= segdur)
			return FINISHED;
	}
	if(chpbuf[n] > 0.0)
		phase = 1;
	else
		phase = -1;
	maxposfirst = -1;
	phasecnt = 0;
	maxval = 0.0;
	while(n < segdur) {
		switch(phase) {
		case(1):								//	Find absolute maximum sample in a wavset
			if(chpbuf[n] > 0.0) {
				if(chpbuf[n] > maxval) {
					maxval = chpbuf[n];
					maxpos = n;
				}
			} else {
				if(fabs(chpbuf[n]) > maxval) {
					maxval = fabs(chpbuf[n]);
					maxpos = n;
				}
				phase = -phase;
				phasecnt++;
			}
			break;
		case(-1):
			if(chpbuf[n] < 0.0) {
				if(fabs(chpbuf[n]) > maxval) {
					maxval = fabs(chpbuf[n]);
					maxpos = n;
				}
			} else {
				if(chpbuf[n] > maxval) {
					maxval = chpbuf[n];
					maxpos = n;
				}
				phase = -phase;
				phasecnt++;
			}
			break;
		}
		if(phasecnt == 2) {
			if(maxposfirst < 0)
				maxposfirst = maxpos;
			env[maxpos] = maxval;			//	and store it in corresponding position in envelope
			maxval = 0.0;
			phasecnt = 0;
		}
		n++;
	}
	env[maxpos] = maxval;
	maxposend   = maxpos;
	k = 0;
	lastpos = 0;
	lastval = 0.0;
	n = 0;
	while(n < segdur) {						//	Interpolate between stored (non-zero) vals
		if(env[n] > 0.0) {
			step = n - lastpos;
			diff = env[n] - lastval; 
			for(m= 0,k = lastpos;m < step; k++,m++) {
				ratio = (double)m/(double)step;
				frac = diff * ratio;
				env[k] = lastval + frac;
			}
			lastval = env[n];
			lastpos = n;
		}
		n++;
	}
	step = n - lastpos;						//	Interpolate down to zero end
	diff = 0.0 - lastval; 
	for(m= 0,k = lastpos;m < step; k++,m++) {
		ratio = (double)m/(double)step;
		frac = diff * ratio;
		env[k] = lastval + frac;
	}
	for(n=0;n < maxposfirst;n++)			//	Check start and end to see if signal rises above interpolated envelope vals
		env[n] = max(env[n],fabs(chpbuf[n]));
	for(n=maxposend;n < segdur;n++)
		env[n] = max(env[n],fabs(chpbuf[n]));
	return FINISHED;
}

/******************************** IMPULSE ********************************/

int impulse(int seglen,double time,dataptr dz)
{
	int exit_status;
	int n, m, k, pkts = 0, impulse_halflen;
	float *chpbuf = dz->sampbuf[2];
	double sharpness, range, val, maxsamp, normaliser, srate = (double)dz->infile->srate;
	double halfsum, sumup, sumdn, adjustup, adjustdn;

	memset((char *)chpbuf,0,dz->buflen * sizeof(float));
	if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
		return exit_status;
	impulse_halflen  = seglen/2;
	if(seglen < 4)
		return FINISHED;						//	if seglen too short, "output" silence
	switch(dz->mode) {
	case(0):
		pkts = dz->iparam[TWT_PKCNT];
		break;
	case(1):
		pkts = (int)round((double)dz->iparam[TWT_PKCNT] * ((double)seglen/srate));
		break;
	}
	maxsamp = 0.0;

	if(dz->mode == 2) {							//	Noise
		sumup = 0.0;
		sumdn = 0.0;
		for(n = 0;n<impulse_halflen*2;n++) {
			val = (drand48() * 2.0) - 1.0;
			if(val > 0.0)
				sumup += val;
			else if (val < 0.0)
				sumdn -= val;
			chpbuf[n] = (float)val;
		}
		halfsum = (sumup + sumdn)/2.0; 
		adjustup = halfsum/sumdn;
		adjustdn = halfsum/sumdn;
		for(n = 0;n<impulse_halflen*2;n++) {
			if(chpbuf[n] > 0.0)
				chpbuf[n] = (float)(chpbuf[n] * adjustup);
			else if(chpbuf[n] < 0.0)
				chpbuf[n] = (float)(chpbuf[n] * adjustdn);
			maxsamp = max(maxsamp,fabs(chpbuf[n]));
		}
		normaliser = 1.0/maxsamp;
	} else {
		if(seglen < pkts * 4)
			range = (double)(seglen/4);			//	A cycle must have at least 4 samples
		else
			range = (double)pkts;
		range *= PI;
		sharpness = 1.0/dz->param[TWT_CHIRP]; 
		for(n = 0,m=impulse_halflen,k=impulse_halflen-1;n<impulse_halflen;n++,m++,k--) {
			val = (double)n/(double)impulse_halflen;
			val = pow(val,sharpness);
			val *= range;
			val = sin(val);
			chpbuf[m] = (float)-val;
			chpbuf[k] = (float)val;
			maxsamp = max(maxsamp,fabs(chpbuf[k]));
		}
	}
	normaliser = 1.0/maxsamp;
	for(n=0;n < impulse_halflen * 2;n++)
		chpbuf[n] = (float)(chpbuf[n] * normaliser);
	return FINISHED;
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
	if((exit_status = get_exclude_data((*cmdline)[0],dz))<0)
		return(exit_status);
	(*cmdline)++;		
	(*cmdlinecnt)--;
	return(FINISHED);
}

/*************************** GET_EXCLUDE_DATA ***********************/

int get_exclude_data(char *filename,dataptr dz)
{
	FILE *fp;
	double dummy = 0.0, lasttime, srate = (double)dz->infile->srate;
	int cnt, badcnt, *times;
	char temp[200], temp2[4], *q;
	if(sloom) {
		temp[0] = NUMERICVAL_MARKER;		//	Reads a zero instead of a filename
		temp[1] = ENDOFSTR;
		temp2[0] = filename[0];
		temp2[1] = ENDOFSTR;
		if(!strcmp(temp2,temp)) {
			dz->itemcnt = 0;
			return FINISHED;
		}
	} else {
		if(!strcmp(filename,"0")) {
			dz->itemcnt = 0;
			return FINISHED;
		}
	}
	if((fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,	"Can't open file %s to read data.\n",filename);
		return(DATA_ERROR);
	}
	cnt = 0;
	badcnt = 0;
	lasttime = -1.0;
	while(fgets(temp,200,fp)==temp) {
		q = temp;
		if(*q == ';')	//	Allow comments in file
			continue;
		while(get_float_from_within_string(&q,&dummy)) {
			if(cnt == 0 && dummy < 0.0) {
				sprintf(errstr,"Time (%lf) is less than zero in file %s.\n",dummy,filename);
				return(DATA_ERROR);
			}
			if(dummy <= lasttime) {
				sprintf(errstr,"Times (%lf & %lf) in file %s are not in increasing order.\n",lasttime,dummy,filename);
				return(DATA_ERROR);
			}
			if(dummy > dz->duration) {
				badcnt++;			//	Rounding error could make a valid end-of-file time not exactly tally with "duration"
				if(badcnt > 1) {	//	So allow 1 overflow time, but not more than 1
					sprintf(errstr,"Time (%lf),in file %s, is beyond end of infile (%lf).\n",dummy,filename,dz->duration);
					return(DATA_ERROR);
				}
			}
			lasttime = dummy;
			cnt++;
		}
	}	    
	if(cnt == 0) {
		sprintf(errstr,"No data in file %s\n",filename);
		return(DATA_ERROR);
	}
	if(((dz->itemcnt = cnt/2) * 2) != cnt) {
		sprintf(errstr,"Data not paired correctly in file %s\n",filename);
		return(DATA_ERROR);
	}
	if((dz->lparray[0] = (int *)malloc(cnt * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for data in file %s.\n",filename);
		return(MEMORY_ERROR);
	}
	times = dz->lparray[0];
	rewind(fp);
	cnt = 0;
	while(fgets(temp,200,fp)==temp) {
		q = temp;
		if(*q == ';')	//	Allow comments in file
			continue;
		while(get_float_from_within_string(&q,&dummy))
			times[cnt++] = (int)round(dummy * srate);
	}	    
	if(flteq(dummy,dz->duration))			//	Force any eof time to be at sample-end of infile
		times[cnt - 1] = dz->insams[0];
	if(fclose(fp)<0) {
		fprintf(stdout,"WARNING: Failed to close file %s.\n",filename);
		fflush(stdout);
	}
	return(FINISHED);
}

/******************************** TWEET_PARAM_PREPROCESS ********************************/

int tweet_param_preprocess(dataptr dz)
{
	int n, m;
	if(dz->brksize[TWT_CHIRP]) {											//	Divide by 10 then Convert to a power of 10	
		for(n=0,m=1;n < dz->brksize[TWT_CHIRP];n++,m+=2)					//	Vals 0	10	20	30
			dz->brk[TWT_CHIRP][m] = pow(10.0,(dz->brk[TWT_CHIRP][m]/10.0));	//	->	 0	1	2	3
	} else																//	->	 1	10	100	1000
		dz->param[TWT_CHIRP] = pow(10.0,(dz->param[TWT_CHIRP]/10.0));
	return FINISHED;
}

/**************************** CHECK_TWEET_PARAM_VALIDITY_AND_CONSISTENCY *****************************/

int check_tweet_param_validity_and_consistency(dataptr dz)
{
	if(dz->brksize[0] == 0) {
		sprintf(errstr,"PITCH PARAMETER MUST BE IN A BREAKPOINT FILE.\n");
		return(DATA_ERROR);
	}
	/* EXTRA SMOOTHING OF PITCH DATA */
	smooth_oct_glitches(dz);		
	return FINISHED;
}

/************************* SMOOTH_OCT_GLITCHES *******************/

void smooth_oct_glitches(dataptr dz)
{
	int n, m, jumpcnt, start, end, k, datalen, endtime_index, jump[2];
	int octglitch;
	double interval, *pitchdata = dz->brk[0], time[2];
	datalen = dz->brksize[0] * 2;
	endtime_index = datalen - 2;
	jumpcnt = 0;
	m = 1;
	n = 3;
	while(n < datalen) {
		interval = pitchdata[n]/pitchdata[m];
		if(interval > 2.0) {
			jump[jumpcnt] = n;
			time[jumpcnt] = pitchdata[n-1];
			jumpcnt++;
		}
		else if(interval < 0.5) {
			jump[jumpcnt] = -n;
			time[jumpcnt] = pitchdata[n-1];
			jumpcnt++;
		}
		if(jumpcnt == 2) {
			octglitch = 0;
			if((jump[0] < 0) && (jump[1] > 0)) {
				octglitch = -1;
			} else if((jump[0] > 0) && (jump[1] < 0) ) {
				octglitch = 1;
			}
			if(octglitch && (time[1] - time[0] < MINSPEECHGLITCH)) {
				start = (int)abs(jump[0]);
				end   = (int)abs(jump[1]);
				k = start;
				if(octglitch > 0) {
					while(k < end) {
						pitchdata[k] /= 2.0;
						k += 2;
					}
				} else {
					while(k < end) {
						pitchdata[k] *= 2.0;
						k += 2;
					}
				}
				jumpcnt = 0;
			} else {
				jump[0] = jump[1];
				time[0] = time[1];
				jumpcnt = 1;
			}
		}
		n += 2;
		m += 2;
	}
	if(jumpcnt) {		
		if(time[0] < MINSPEECHGLITCH) {										// SINGLE LEAP AT START
			k = 1;
			end = (int)abs(jump[0]);
			if(jump[0] > 0) {
				while(k < end) {
					pitchdata[k] *= 2.0;
					k += 2;
				}
			} else {
				while(k < end) {
					pitchdata[k] /= 2.0;
					k += 2;
				}
			}
		} else if(pitchdata[endtime_index] - time[0] < MINSPEECHGLITCH)  {	// SINGLE LEAP LEFT AT END
			k = (int)abs(jump[0]);
			end = datalen;
			if(jump[0] > 0) {
				while(k < end) {
					pitchdata[k] /= 2.0;
					k += 2;
				}
			} else {
				while(k < end) {
					pitchdata[k] *= 2.0;
					k += 2;
				}
			}
		}
	}
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
