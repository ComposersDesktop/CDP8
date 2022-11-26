#include <stdio.h>
#include <stdlib.h>
#include <structures.h>
#include <tkglobals.h>
#include <pnames.h>
#include <flags.h>
#include <filetype.h>
#include <processno.h>
#include <modeno.h>
#include <logic.h>
#include <globcon.h>
#include <cdpmain.h>
#include <math.h>
#include <osbind.h>
#include <standalone.h>
#include <science.h>
#include <ctype.h>
#include <sfsys.h>
#include <string.h>
#include <srates.h>
#include <filtcon.h>
#include <arrays.h>
#include <string.h>
#include <special.h>
#include <memory.h>

#ifndef cdp_round
extern int cdp_round(double a);
#endif

#ifdef unix
#define round(x) lround((x))
#else
#define round(x) cdp_round((x))
#endif

#define MINUS96DB (0.000016)
#define SYNFLT_TAIL	1000
#define SYNFDOVE	15			//	Size of dovetails at start and end of noise source
#define SYNFLEV		0.90		//	Level limit for both synth and filter output

#define outsams rampbrksize
#define	synfgain is_sharp
#define	synfdove is_flat


#ifdef unix
#define round(x) lround((x))
#endif

char errstr[2400];

int anal_infiles = 1;
int	sloom = 0;
int sloombatch = 0;

const char* cdp_version = "8.0.0";

//CDP LIB REPLACEMENTS
static int  check_synfilt_param_validity_and_consistency(double *flt_inv_sr,dataptr dz);
static int  setup_synfilt_application(dataptr dz);
static int  parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int  setup_synflt_param_ranges_and_defaults(dataptr dz);
static int  handle_the_outfile(int *cmdlinecnt,char ***cmdline,dataptr dz);
static int  open_the_outfile(dataptr dz);
static int  setup_and_init_input_param_activity(dataptr dz,int tipc);
static int  setup_input_param_defaultval_stores(int tipc,aplptr ap);
static int  establish_application(dataptr dz);
static int  initialise_vflags(dataptr dz);
static int  setup_parameter_storage_and_constants(int storage_cnt,dataptr dz);
static int  initialise_is_int_and_no_brk_constants(int storage_cnt,dataptr dz);
static int  mark_parameter_types(dataptr dz,aplptr ap);
static int  assign_file_data_storage(int infilecnt,dataptr dz);
static int  get_tk_cmdline_word(int *cmdlinecnt,char ***cmdline,char *q);
static int  get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz);
static int  get_the_mode_from_cmdline(char *str,dataptr dz);
static int  setup_and_init_input_brktable_constants(dataptr dz,int brkcnt);
static int  synfilter_pconsistency(int flt_wordcnt,int flt_entrycnt,int *flt_cnt,int flt_timeslots,int *flt_frq_index,dataptr dz);
static int  force_start_and_end_val(dataptr dz);
static void initialise_filter_table_read(int param,dataptr dz);
static int  allocate_tvarying_filter_arrays(int flt_timeslots,int *flt_cnt,int harmonics_cnt,dataptr dz);
static int  put_tvarying_filter_data_in_arrays(double *fbrk,int flt_worcdnt,int flt_entrycnt,int flt_cnt,int flt_timeslots,dataptr dz);
static int  initialise_fltbankv_internal_params(int fltcnt,int *flt_frq_index,dataptr dz);
static int  synfilter_preprocess(int flt_cnt,double flt_inv_sr,dataptr dz);
static int  allocate_filter_internalparam_arrays(int fltcnt,dataptr dz);
static int  initialise_filter_params(int flt_cnt,double flt_inv_sr,dataptr dz);
static int  getmaxlinelen(int *maxcnt,FILE *fp);
static int  check_seq_and_range_of_filter_data(double *fbrk,int flt_entrycnt,int total_wordcnt,double *endtime,dataptr dz);
static void get_syncoeffs1(int n,double flt_inv_sr,dataptr dz);
static void get_syncoeffs2(int n,dataptr dz);
static int  read_the_special_data(char *str,int *flt_wordcnt,int *flt_entrycnt,int *flt_cnt,int *flt_timeslots,dataptr dz);
static int  get_data_from_tvary_infile(char *filename,int *flt_wordcnt,int *flt_entrycnt,int *flt_cnt,int *flt_timeslots,dataptr dz);
static int  get_data_from_fsyn_infile(char *filename,int *flt_wordcnt,int *flt_entrycnt,int *flt_cnt,int *flt_timeslots,dataptr dz);
static int  getmaxlinelen(int *maxcnt,FILE *fp);
static int  check_filter_data(int flt_entrycnt,int *flt_wordcnt,int *flt_timeslots,dataptr dz);
static int  allocate_filter_frq_amp_arrays(int fltcnt,dataptr dz);

static int  filter_process(double flt_inv_sr,int flt_cnt,int flt_timeslots,dataptr dz);

static void filtering(int n,int chans,float *buf,double *a,double *b,double *y,double *z,double *d,double *e,double *ampl,int flt_cnt,int *flt_ovflw,
					  int running,dataptr dz);
static double check_float_limits(double sum, int *flt_ovflw,dataptr dz);
static int 	newq(double *flt_q_incr, int *flt_sams, dataptr dz);
static int 	newfval(int *fsams,int flt_cnt,int flt_timeslots,int *flt_frq_index,int *flt_times_cnt,dataptr dz);
static int 	do_fvary_filters(double flt_inv_sr,int flt_cnt,int *flt_times_cnt,int *flt_sams,double *flt_q_incr,int *flt_blokcnt,
							 int *flt_ovflw,int flt_timeslots,int *flt_frq_index,int running,dataptr dz);
static void	gen_noise(float *buf,dataptr dz);

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
	int exit_status;
	dataptr dz = NULL;
	char **cmdline, sfnam[400];
	int  cmdlinecnt;
	int n;
	aplptr ap;
	int is_launched = FALSE;

	double flt_inv_sr;
	int flt_cnt, flt_timeslots, flt_wordcnt, flt_entrycnt, flt_frq_index;

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
		dz->maxmode = 2;
		if((exit_status = get_the_mode_from_cmdline(cmdline[0],dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(exit_status);
		}
		cmdline++;
		cmdlinecnt--;
		// setup_particular_application =
		if((exit_status = setup_synfilt_application(dz))<0) {
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

	if((exit_status = setup_internal_arrays_and_array_pointers(dz))<0) {
		exit_status = print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	// parse_infile_and_hone_type() 
	// setup_param_ranges_and_defaults() =
	if((exit_status = setup_synflt_param_ranges_and_defaults(dz))<0) {
		exit_status = print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
//	open_first_infile() : redundant
//	handle_extra_infiles() : redundant
	// handle_outfile() = 
	if((exit_status = handle_the_outfile(&cmdlinecnt,&cmdline,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}

//	handle_formants()			redundant
//	handle_formant_quiksearch()	redundant
//	handle_special_data() = copy name of special-data file here
	strcpy(sfnam,cmdline[0]);
	cmdlinecnt--;
	cmdline++;
 
	if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {		// CDP LIB
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
//	check_param_validity_and_consistency....
	if((exit_status = check_synfilt_param_validity_and_consistency(&flt_inv_sr,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
// srate and chans obtained from params
	if((exit_status = open_the_outfile(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
//	Can only run these once srate and chans set
	if((exit_status = read_the_special_data(sfnam,&flt_wordcnt,&flt_entrycnt,&flt_cnt,&flt_timeslots,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(exit_status);
	}
	if((exit_status = check_filter_data(flt_entrycnt,&flt_wordcnt,&flt_timeslots,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(exit_status);
	}
	if((exit_status = synfilter_pconsistency(flt_wordcnt,flt_entrycnt,&flt_cnt,flt_timeslots,&flt_frq_index,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(exit_status);
	}
	is_launched = TRUE;
	dz->bufcnt = 1;
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

	if((exit_status = create_sndbufs(dz))<0) {							// CDP LIB
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	//param_preprocess() = 
	if((exit_status = synfilter_preprocess(flt_cnt,flt_inv_sr,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = filter_process(flt_inv_sr,flt_cnt,flt_timeslots,dz)) < 0) {
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

/************************* SETUP_SYNFILT_APPLICATION *******************/

int setup_synfilt_application(dataptr dz)
{
	int exit_status;
	aplptr ap;
	if((exit_status = establish_application(dz))<0)		// GLOBAL
		return(FAILED);
	ap = dz->application;
	// SEE parstruct FOR EXPLANATION of next 2 functions
	if(dz->mode== 0)
		exit_status = set_param_data(ap,SYN_FILTERBANK,6,6,"iiDidi");
	else
		exit_status = set_param_data(ap,TIMEVARYING_FILTERBANK,6,6,"iiDidi");
	if(exit_status < 0)
		return(FAILED);
	if((exit_status = set_vflgs(ap,"",0,"","do",2,0,"00"))<0)
		return(FAILED);
	// set_legal_infile_structure -->
	dz->has_otherfile = FALSE;
	// assign_process_logic -->
	dz->input_data_type = NO_FILE_AT_ALL;
	dz->process_type	= UNEQUAL_SNDFILE;	
	dz->outfiletype  	= SNDFILE_OUT;
	return application_init(dz);	//GLOBAL
}

/************************* SETUP_SYNFLT_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_synflt_param_ranges_and_defaults(dataptr dz)
{
	int exit_status;
	aplptr ap = dz->application;
	// set_param_ranges()
	ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
	// NB total_input_param_cnt is > 0 !!!
	if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
		return(FAILED);
	// get_param_ranges()

	ap->lo[SYNFLT_SRATE]   = 44100.0;
	ap->hi[SYNFLT_SRATE]   = 96000.0;
	ap->default_val[SYNFLT_SRATE] = 44100.0;
	ap->lo[SYNFLT_CHANS]   = 1;
	ap->hi[SYNFLT_CHANS]   = 2;
	ap->default_val[SYNFLT_CHANS] = 1;
	ap->lo[SYNFLT_Q]	   = MINQ;
	ap->hi[SYNFLT_Q]	   = MAXQ;
	ap->default_val[SYNFLT_Q] = FLT_DEFAULT_Q;
	ap->lo[SYNFLT_HARMCNT] = 1.0;
	ap->hi[SYNFLT_HARMCNT] = FLT_MAXHARMS;
	ap->default_val[SYNFLT_HARMCNT]	= FLT_DEFAULT_HCNT;
	ap->lo[SYNFLT_ROLLOFF] = MIN_DB_ON_16_BIT;
	ap->hi[SYNFLT_ROLLOFF] = 0.0;
	ap->default_val[SYNFLT_ROLLOFF] = FLT_DEFAULT_ROLLOFF;
	ap->lo[SYNFLT_SEED] = 0;
	ap->hi[SYNFLT_SEED] = 32767;
	ap->default_val[SYNFLT_SEED] = 0;
	dz->maxmode = 2;
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
			if((exit_status = setup_synfilt_application(dz))<0)
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

/****************************** SETUP_INTERNAL_ARRAYS_AND_ARRAY_POINTERS *********************************/

int setup_internal_arrays_and_array_pointers(dataptr dz)
{
	int n;		 
	dz->array_cnt = 19; 
	dz->larray_cnt = 1; 
	if((dz->parray  = (double **)malloc(dz->array_cnt * sizeof(double *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for internal double arrays.\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<dz->array_cnt;n++)
		dz->parray[n] = NULL;
	if((dz->lparray = (int    **)malloc(dz->larray_cnt * sizeof(int *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for internal int arrays.\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<dz->larray_cnt;n++)
		dz->lparray[n] = NULL;
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


/******************************** USAGE1 ********************************/

int usage1(void)
{
	usage2("synfilt");
	return(USAGE_ONLY);
}

/**************************** CHECK_SYNFILT_PARAM_VALIDITY_AND_CONSISTENCY *****************************/

int check_synfilt_param_validity_and_consistency(double *flt_inv_sr,dataptr dz)
{
	dz->param[SYNFLT_ROLLOFF] = dbtogain(dz->param[SYNFLT_ROLLOFF]);
	if(BAD_SR(dz->iparam[SYNFLT_SRATE])) {
		sprintf(errstr,"Invalid sample rate.\n");
		return(DATA_ERROR);
	}
	dz->infile->srate = dz->iparam[SYNFLT_SRATE];
	dz->infile->channels = dz->iparam[SYNFLT_CHANS];
	*flt_inv_sr = 1.0/(double)dz->infile->srate;
	return FINISHED;
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	if(!strcmp(prog_identifier_from_cmdline,"synfilt"))				dz->process = SYNFILT;
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
	if(!strcmp(str,"synfilt")) {
		fprintf(stderr,
	   	"NOISE FILTERED BY TIME_VARYING FILTERBANK,WITH TIME-VARIABLE Q\n\n"
	    "USAGE: synfilt synfilt\n"
	    "mode outfile data dur srate chans Q gain hcnt rolloff seed [-d] [-o] [-n]\n\n"
		"\n"
		"MODES ARE...\n"
		"1)  Single (varying) pitch : Enter filter-pitch as time MIDI-value pairs.\n"
		"2)  Simultaneous pitches : Enter filter-pitches as varibank style datafile.\n\n"
		"          Datafile has lines of data for filter bands at successive times.\n"
		"          Each line contains the following items\n"
		"                Time:   MIDIPitch1 Amp1   [MIDIPitch2 Amp2    etc....].\n"
		"          Pitch and Amp values must be paired:\n"
		"          any number of pairs can be used in a line,\n"
		"          BUT each line must have SAME number of pairs on it.\n"
		"          (To eliminate a band in any line(s), set its amplitude to 0.0).\n"
		"          Time values (in secs) must be in ascending order (and >=0.0)\n"
		"          and the MAXIMUM TIME must be greater than 0.03 secs (30mS).\n"
		"          Amp values may be numeric, or dB values (e.g. -4.1dB).\n"
		"          Comment-lines may be used: start these with ';'.\n\n"
		"          Im both modes, duration of output set by last entry in datafile.\n\n"
		"Q         Q (tightness) of filter : Range(%lf to %.1lf).\n"		
		"SRATE     Sample rate of output file.\n"		
		"CHANS     Output mono (1) or stereo (2).\n"		
		"HCNT      No of harmonics of each pitch to use: Default 1.\n"
		"          High harmonics of high pitches may be beyond nyquist.\n"
		"          (No-of-pitches times no-of-harmonics determines program speed).\n"
		"ROLLOFF   Level drop (in dB) from one harmonic to next. Range(0 to %.1lf)\n"
		"SEED      Initialises random-noise generation.\n"
		"-d        double filtering.\n"
		"-o        Drop out if filter overflows.\n", MINQ, MAXQ,MIN_DB_ON_16_BIT);    //RWD added args
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

/************************************* SYNFILTER_PCONSISTENCY *********************************/

int synfilter_pconsistency(int flt_wordcnt,int flt_entrycnt,int *flt_cnt,int flt_timeslots,int *flt_frq_index,dataptr dz)
{
	int exit_status;
	initrand48();
	/* preset internal counters, or defaulted variables */

	if(dz->brksize[SYNFLT_Q]) {
		if((exit_status = force_start_and_end_val(dz))<0)
			return(exit_status);
		initialise_filter_table_read(SYNFLT_Q,dz);
	}
	dz->param[SYNFLT_ROLLOFF] = dbtogain(dz->param[SYNFLT_ROLLOFF]);
	if((exit_status = allocate_tvarying_filter_arrays(flt_timeslots,flt_cnt,dz->iparam[SYNFLT_HARMCNT],dz))<0)
		return(exit_status);
	if((exit_status = put_tvarying_filter_data_in_arrays(dz->parray[FLT_FBRK],flt_wordcnt,flt_entrycnt,*flt_cnt,flt_timeslots,dz))<0)
		return(exit_status);
	if((exit_status = initialise_fltbankv_internal_params(*flt_cnt,flt_frq_index,dz))<0)
		return(exit_status);
	return(FINISHED);
}

/*************************************** FORCE_START_AND_END_VAL **************************************/

int force_start_and_end_val(dataptr dz)
{
 	double lasttime, filedur, firsttime, *p;
	int   k, n;
	firsttime = *(dz->brk[SYNFLT_Q]);
	if(firsttime < 0.0) {
		sprintf(errstr,"First time in Q file is -ve: Can't proceed\n");
		return(DATA_ERROR);
	}
	if(flteq(firsttime,0.0))
		*(dz->brk[SYNFLT_Q]) = 0.0;
	else {							/* FORCE VALUE AT TIME 0 */
	 	dz->brksize[SYNFLT_Q]++;
		if((dz->brk[SYNFLT_Q] = (double *)realloc(dz->brk[SYNFLT_Q],dz->brksize[SYNFLT_Q] * 2 * sizeof(double)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to reallocate filter Q array.\n");
			return(MEMORY_ERROR);
		}
		k = dz->brksize[SYNFLT_Q] * 2;
		for(n=k-1;n>=2;n--)
			dz->brk[SYNFLT_Q][n] = dz->brk[SYNFLT_Q][n-2];
		dz->brk[SYNFLT_Q][0] = 0.0;
		dz->brk[SYNFLT_Q][1] = dz->brk[SYNFLT_Q][3];
	}

	lasttime = *(dz->brk[SYNFLT_Q] + ((dz->brksize[SYNFLT_Q]-1) * 2));
	filedur  = (double)(dz->outsams/dz->infile->channels)/(double)dz->infile->srate;
	if(lasttime >= filedur + SYNFLT_TAIL)
		return(FINISHED);			/* FORCE Q VALUE AT (BEYOND) END OF FILE */
	dz->brksize[SYNFLT_Q]++;
	if((dz->brk[SYNFLT_Q] = (double *)realloc(dz->brk[SYNFLT_Q],dz->brksize[SYNFLT_Q] * 2 * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to reallocate filter array.\n");
		return(MEMORY_ERROR);
	}
	p = (dz->brk[SYNFLT_Q] + ((dz->brksize[SYNFLT_Q]-1) * 2));
	*p++ = filedur + SYNFLT_TAIL + 1.0;
	*p = *(p-2);
	return(FINISHED);
}

/************************************* INITIALISE_FILTER_TABLE_READ *********************************/

void initialise_filter_table_read(int param,dataptr dz)
{
	dz->lastind[param] = (double)round((*dz->brk[param]) * dz->infile->srate);
	dz->lastval[param] = *(dz->brk[param]+1);
	dz->brkptr[param]  = dz->brk[param] + 2;
}

/**************************** ALLOCATE_TVARYING_FILTER_ARRAYS *******************************/

int allocate_tvarying_filter_arrays(int flt_timeslots,int *flt_cnt,int harmonics_cnt,dataptr dz)
{
	(*flt_cnt) *= harmonics_cnt;

	if((dz->lparray[FLT_SAMPTIME] = (int *)calloc(flt_timeslots * sizeof(int),sizeof(char)))==NULL
	|| (dz->parray[FLT_INFRQ]     = (double *)calloc((*flt_cnt) * flt_timeslots * sizeof(double),sizeof(char)))==NULL
	|| (dz->parray[FLT_INAMP]     = (double *)calloc((*flt_cnt) * flt_timeslots * sizeof(double),sizeof(char)))==NULL
	|| (dz->parray[FLT_FINCR]     = (double *)calloc((*flt_cnt) * sizeof(double),sizeof(char)))==NULL
	|| (dz->parray[FLT_AINCR]     = (double *)calloc((*flt_cnt) * sizeof(double),sizeof(char)))==NULL
	|| (dz->parray[FLT_LASTFVAL]  = (double *)calloc((*flt_cnt) * sizeof(double),sizeof(char)))==NULL
	|| (dz->parray[FLT_LASTAVAL]  = (double *)calloc((*flt_cnt) * sizeof(double),sizeof(char)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for filter coefficients.\n");
		return(MEMORY_ERROR);
	}
	return(FINISHED);
}

/**************************** PUT_TVARYING_FILTER_DATA_IN_ARRAYS *******************************/

int put_tvarying_filter_data_in_arrays(double *fbrk,int flt_wordcnt,int flt_entrycnt,int flt_cnt,int flt_timeslots,dataptr dz)
{
	int timescnt = 0, freqcnt = 0, ampcnt = 0, n, m;
	double atten;
	int total_frq_cnt = flt_cnt * flt_timeslots;
	int j;
	int srate = dz->infile->srate;
	if(dz->parray[FLT_INFRQ]==NULL) {
		sprintf(errstr,"FLT_INFRQ array not established: put_tvarying_filter_data_in_arrays()\n");
		return(PROGRAM_ERROR);
	} 
	if(dz->parray[FLT_INAMP]==NULL) {
		sprintf(errstr,"FLT_INAMP array not established: put_tvarying_filter_data_in_arrays()\n");
		return(PROGRAM_ERROR);
	} 
	if(dz->lparray[FLT_SAMPTIME]==NULL) {
		sprintf(errstr,"FLT_SAMPTIME array not established: put_tvarying_filter_data_in_arrays()\n");
		return(PROGRAM_ERROR);
	} 
	for(n=0;n<flt_wordcnt;n++) {
		m = n%flt_entrycnt;
		if(m==0) {
			if(timescnt >= flt_timeslots) {
				sprintf(errstr,"Error 0 in filter counting: put_tvarying_filter_data_in_arrays()\n");
				return(PROGRAM_ERROR);
			} 
			dz->lparray[FLT_SAMPTIME][timescnt++] = round(fbrk[n] * dz->infile->srate);

		} else if(ODD(m)) {
			for(j=1;j<=dz->iparam[SYNFLT_HARMCNT];j++) {
				if(freqcnt >= total_frq_cnt) {
					sprintf(errstr,"Error 1 in filter counting: put_tvarying_filter_data_in_arrays()\n");
					return(PROGRAM_ERROR);
				} 
				if((dz->parray[FLT_INFRQ][freqcnt] = fbrk[n] * (double)j) > FLT_MAXFRQ) {
					sprintf(errstr,"Filter Harmonic %d of %.1lfHz = %.1lfHz beyond filter limit %.1lf.\n",
					j,fbrk[n],dz->parray[FLT_INFRQ][freqcnt],FLT_MAXFRQ);
					return(DATA_ERROR);
				}
				freqcnt++;
			}
		} else {
			atten = 1.0;
			for(j=1;j<=dz->iparam[SYNFLT_HARMCNT];j++) {
				if(ampcnt >= total_frq_cnt) {
					sprintf(errstr,"Error 2 in filter counting: put_tvarying_filter_data_in_arrays()\n");
					return(PROGRAM_ERROR);
				} 
				dz->parray[FLT_INAMP][ampcnt] = fbrk[n] * atten;
				ampcnt++;
				atten *= dz->param[SYNFLT_ROLLOFF];
			}
		}
	}
	if(freqcnt != total_frq_cnt || ampcnt != freqcnt || timescnt != flt_timeslots) {
		sprintf(errstr,"Filter data accounting problem: read_time_varying_filter_data()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/**************************** INITALISE_FLTBANKV_INTERNAL_PARAMS **********************/

int initialise_fltbankv_internal_params(int flt_cnt,int *flt_frq_index,dataptr dz)
{
	int exit_status;
	int n;
	if((exit_status = allocate_filter_frq_amp_arrays(flt_cnt,dz))<0)
		return(exit_status);
	for(n = 0;n<flt_cnt;n++) {
		dz->parray[FLT_FRQ][n]      = dz->parray[FLT_INFRQ][n];
		dz->parray[FLT_AMP][n]      = dz->parray[FLT_INAMP][n];
		dz->parray[FLT_LASTFVAL][n] = dz->parray[FLT_FRQ][n];
		dz->parray[FLT_LASTAVAL][n] = dz->parray[FLT_AMP][n];
	}
	*flt_frq_index = flt_cnt;
	dz->iparam[FLT_TIMES_CNT] = 1;
	return(FINISHED);
}

/****************************** FILTER_PROCESS *************************/

int filter_process(double flt_inv_sr,int flt_cnt,int flt_timeslots,dataptr dz)
{
	int exit_status = FINISHED;
	int flt_frq_index = flt_cnt, flt_times_cnt = 1, flt_blokcnt = 0, flt_sams = 0, flt_ovflw = 0, startup;
	double flt_q_incr = 0.0, srate = (double)dz->infile->srate;
	int tail_extend = 0, was_tail_extend = 0, bufspace, extraspace = 0;
	int chans = dz->infile->channels, sndendset = 0, gotend = 0, tail_done = 0;
	double inmaxsamp = 0.0, outmaxsamp = 0.0, maxsamp = 0.0;
	double tailmaxsamp = 0.0, tailfade = 1.0, tailincr = 1.0;
	int tailmaxpos = 0;
	float *buf = dz->sampbuf[0];
	int n, m, k, sndend = 0;
	int framend, framestart, framesize = F_SECSIZE, framecnt = dz->buflen/framesize;
	int synfsplic, downsplicestart, gap_from_end;
	double splincr, splice;
	
	synfsplic = (int)round(SYNFDOVE * MS_TO_SECS * srate);
	downsplicestart = dz->outsams - (synfsplic * chans);
	splincr  = 1.0/(double)synfsplic;
	splice = 0.0;

	dz->synfgain = 1.0;			//	Initialise filter process gain
	dz->scalefact = 1.0;		//	Initial noise gain set to 1.0
	dz->tempsize = dz->outsams;	//	Temporarily set param for write_display
	dz->process = GREV;
	display_virtual_time(0,dz);
	dz->process = SYNFILT;

	//	GENERATE THE NOISE SOURCE TO CHECK ITS LEVEL

	fprintf(stdout,"INFO: Assessing synth source level.\n");
	fflush(stdout);
	srand((int)dz->iparam[SYNFLT_SEED]);
	while(dz->samps_left > 0) {
		gen_noise(buf,dz);
		for(n = 0;n < dz->ssampsread;n++)
			inmaxsamp = max(inmaxsamp,fabs(buf[n]));
		dz->total_samps_written = dz->outsams - dz->samps_left;
		dz->process = GREV;
		display_virtual_time(dz->total_samps_written,dz);
		dz->process = SYNFILT;
	}
	if(inmaxsamp <= 0.0) {
		sprintf(errstr,"No level found in input signal\n");
		return DATA_ERROR;
	}
	if(inmaxsamp > SYNFLEV)
		dz->scalefact = SYNFLEV/inmaxsamp;			//	Reduce synthesis gain

	//	RUN FILTER TO ASSESS LEVEL OF OUTPUT, AND HENCE SET AN APPROPRIATE GAIN

	dz->total_samps_written = 0;			//	Reset output counters
	dz->samps_left = dz->outsams;
	dz->total_samps_read = 0;

	if((exit_status = newfval(&(dz->iparam[FLT_FSAMS]),flt_cnt,flt_timeslots,&flt_frq_index,&flt_times_cnt,dz))<0)
		return(exit_status);
	dz->process = GREV;
	display_virtual_time(0,dz);
	dz->process = SYNFILT;
	fprintf(stdout,"INFO: Assessing output level.\n");
	fflush(stdout);
	srand((int)dz->iparam[SYNFLT_SEED]);	//	Reset random generator
	startup = 1;
	while(dz->samps_left > 0) {
		memset((char *)dz->sampbuf[0],0,(size_t) (dz->buflen * sizeof(float)));
		if(tail_extend)
			dz->ssampsread = 0;			
		else {
			gen_noise(buf,dz);
			if(startup) {							//	Fade-up splice at start of synth material
				for(n = 0;n < synfsplic*chans; n+=chans) {
					for(m = 0; m < chans; m++)
						buf[n+m] = (float)(buf[n+m] * splice);
					splice += splincr;
				}
				startup = 0;						//	Fade-down splice at end of synth material
			} else if((gap_from_end = dz->total_samps_read - downsplicestart) >= 0) {
				n = (dz->total_samps_read % dz->buflen) - gap_from_end;
				splice = 1.0;
				k = 0;
				while(n < 0) {
					splice -= splincr;
					n += chans;
					k++;
				}
				while(k < synfsplic) {
					for(m=0;m<chans;m++)
						buf[n+m] = (float)(buf[n+m] * splice);
					splice -= splincr;
					k++;
					if((n += chans) >= dz->buflen)
						break;
				}
			}
			if(dz->samps_left <= 0) {
				was_tail_extend = 1;
				tail_extend = 1;
			}
		}
		if(tail_extend) {
			bufspace = dz->buflen;
			dz->ssampsread = dz->buflen;				
			extraspace += dz->buflen;						//	Extra space needed for progress-bar display
		}
		if((exit_status = do_fvary_filters(flt_inv_sr,flt_cnt,&flt_times_cnt,&flt_sams,&flt_q_incr,&flt_blokcnt,&flt_ovflw,flt_timeslots,&flt_frq_index,0,dz)) <0)
			return(exit_status);
		if(tail_extend) {
			sndend = dz->buflen;
			framend = dz->buflen;
			for(k = framecnt; k > 0; k--) {					//	Search backwards thro buffer, frame by frame
				framestart = framend - framesize;
				maxsamp = 0.0;
				for(n = framend-chans;n >= framestart;n-=chans) {	
					for(m=0;m<chans;m++) {					//	Search backwards thro frame, samp-grup by samp-group
						if(fabs(buf[n+m]) > maxsamp) {
							if(!sndendset) {				//	If samples cease to be zero
								sndend = n + chans;			//	Mark start of end-zeros in buffer
								sndendset = 1;				//	and flag that snd end has been found
							}
							maxsamp = fabs(buf[n+m]);
						}
					}
					if(maxsamp < MINUS96DB) {				//	If max level in frame falls below -96dB
						if(sndendset) {						//	If we found a place in buffer after which samples were all zero
							dz->ssampsread = sndend;		//	Mark this as end of output, and quit the main filtering loop
							tail_extend = 0;				//	by setting tail_extend to zero
							dz->samps_left = 0; // SAFETY	
							break;
						} else								//	If we didn't find place .... 	
							sndend = framestart;			//	Then all samples in the frame are zero.
					}										//	So move snd end to start of current frame.
				}											//	.. and search backwards thro previous frame
				if(tail_extend == 0) {
					extraspace -= dz->buflen - sndend;		//	Reduce extra space needed for progress-bar display
					break;
				}
				framend = framestart;
			}
		}
		if(tail_extend)
			tail_extend++;
		if(dz->ssampsread > 0) {
			for(n = 0;n < dz->ssampsread;n++)
				outmaxsamp = max(outmaxsamp,(double)fabs(buf[n]));
			dz->process = GREV;
			dz->total_samps_written = dz->outsams - dz->samps_left;
			display_virtual_time(dz->total_samps_written,dz);
			dz->process = SYNFILT;
		}
	}
	if(outmaxsamp <= 0.0) {
		sprintf(errstr,"No level found in output signal.\n");
		return DATA_ERROR;
	}
	dz->synfgain = (inmaxsamp/outmaxsamp) * SYNFLEV;	/* Normalise */

	if(dz->synfgain <= dbtogain(-90.0)) {
		sprintf(errstr,"FILTER BLEW UP: REDUCE NUMBER OF HARMONICS, OR INCREASE Q\n");
		return GOAL_FAILED;
	}

	//	IF FILTER-PROCESS AMPLIFIES INPUT, PRE-CHECK ANY POSSIBLE RUNAWAY LOUDNESS IN TAIL.

	if(outmaxsamp < inmaxsamp) {

		dz->total_samps_written = 0;			//	Reset output counters
		dz->samps_left = dz->outsams;
		dz->total_samps_read = 0;

		for(n = 0;n<flt_cnt;n++) {
			dz->parray[FLT_FRQ][n]      = dz->parray[FLT_INFRQ][n];
			dz->parray[FLT_AMP][n]      = dz->parray[FLT_INAMP][n];
			dz->parray[FLT_LASTFVAL][n] = dz->parray[FLT_FRQ][n];
			dz->parray[FLT_LASTAVAL][n] = dz->parray[FLT_AMP][n];
		}
		flt_frq_index = flt_cnt;
		flt_times_cnt = 1;
		sndendset	= 0;
		sndend		= 0;
		tail_extend = 0;
		was_tail_extend = 0;
		gotend		= 0;
	
		if((exit_status = newfval(&(dz->iparam[FLT_FSAMS]),flt_cnt,flt_timeslots,&flt_frq_index,&flt_times_cnt,dz))<0)
			return(exit_status);
		dz->process = GREV;
		display_virtual_time(0,dz);
		dz->process = SYNFILT;
		fprintf(stdout,"INFO: Reassessing output level, as process will amplify signal.\n");
		fflush(stdout);
		srand((int)dz->iparam[SYNFLT_SEED]);	//	Reset random generator
		startup = 1;
		while(dz->samps_left > 0 || tail_extend) {
			memset((char *)dz->sampbuf[0],0,(size_t) (dz->buflen * sizeof(float)));
			if(tail_extend)
				dz->ssampsread = 0;			
			else {
				gen_noise(buf,dz);
				if(startup) {					//	Fade-up splice at start of synth material
					for(n = 0;n < synfsplic*chans; n+=chans) {
						for(m = 0; m < chans; m++)
							buf[n+m] = (float)(buf[n+m] * splice);
						splice += splincr;
					}
					startup = 0;				//	Fade-down splice at end of synth material
				} else if((gap_from_end = dz->total_samps_read - downsplicestart) >= 0) {
					n = (dz->total_samps_read % dz->buflen) - gap_from_end;
					splice = 1.0;
					k = 0;
					while(n < 0) {
						splice -= splincr;
						n += chans;
						k++;
					}
					while(k < synfsplic) {
						for(m=0;m<chans;m++)
							buf[n+m] = (float)(buf[n+m] * splice);
						splice -= splincr;
						k++;
						if((n += chans) >= dz->buflen)
							break;
					}
				}
				if(dz->samps_left <= 0) {
					was_tail_extend = 1;
					tail_extend = 1;
				}
			}
			if(tail_extend) {
				bufspace = dz->buflen;
				dz->ssampsread = dz->buflen;				
			}
			if((exit_status = do_fvary_filters(flt_inv_sr,flt_cnt,&flt_times_cnt,&flt_sams,&flt_q_incr,&flt_blokcnt,&flt_ovflw,flt_timeslots,&flt_frq_index,0,dz)) <0)
				return(exit_status);
			if(tail_extend) {
				for(n = 0;n < dz->buflen;n++) {
					if(fabs(buf[n]) > tailmaxsamp) {
						tailmaxsamp = fabs(buf[n]);
						tailmaxpos  = n + ((tail_extend - 1) * dz->buflen);
					}
				}
				sndend = dz->buflen;
				framend = dz->buflen;
				for(k = framecnt; k > 0; k--) {					//	Search backwards thro buffer, frame by frame
					framestart = framend - framesize;
					maxsamp = 0.0;
					for(n = framend-chans;n >= framestart;n-=chans) {	
						for(m=0;m<chans;m++) {					//	Search backwards thro frame, samp-grup by samp-group
							if(fabs(buf[n+m]) > maxsamp) {
								if(!sndendset) {				//	If samples cease to be zero
									sndend = n + chans;			//	Mark start of end-zeros in buffer
									sndendset = 1;				//	and flag that snd end has been found
								}
								maxsamp = fabs(buf[n+m]);
							}
						}
						if(maxsamp < MINUS96DB) {				//	If max level in frame falls below -96dB
							if(sndendset) {						//	If we found a place in buffer after which samples were all zero
								dz->ssampsread = sndend;		//	Mark this as end of output, and quit the main filtering loop
								tail_extend = 0;				//	by setting tail_extend to zero
								dz->samps_left = 0; // SAFETY	
								break;
							} else								//	If we didn't find place .... 	
								sndend = framestart;			//	Then all samples in the frame are zero.
						}										//	So move snd end to start of current frame.
					}											//	.. and search backwards thro previous frame
					if(tail_extend == 0) {
						tail_done = 1;
						break;
					}
					framend = framestart;
				}
			}
			if(tail_extend)
				tail_extend++;
			if(dz->ssampsread > 0) {
				if(!tail_extend && !tail_done) {
					for(n = 0;n < dz->ssampsread;n++) {
						outmaxsamp = max(outmaxsamp,(double)fabs(buf[n]));
						tailmaxsamp = outmaxsamp;
					}
				}
				dz->process = GREV;
				dz->total_samps_written = dz->outsams - dz->samps_left;
				display_virtual_time(dz->total_samps_written,dz);
				dz->process = SYNFILT;
			}
		}
		if(tailmaxsamp > outmaxsamp)		//	If filter tail grows loud, set up a fader process
			tailincr = pow(outmaxsamp/tailmaxsamp,(double)(1.0/tailmaxpos));

        fprintf(stdout,"WARNING: * !$$~* EXPLODING TAIL !$$~* \n");          //RWD Xcode/clang doesn't like £ character
		fflush(stdout);
	}

	//	RUN THE FILTER


	dz->total_samps_written = 0;			//	Reset output counters
	dz->samps_left = dz->outsams;
	dz->total_samps_read = 0;

	for(n = 0;n<flt_cnt;n++) {
		dz->parray[FLT_FRQ][n]      = dz->parray[FLT_INFRQ][n];
		dz->parray[FLT_AMP][n]      = dz->parray[FLT_INAMP][n];
		dz->parray[FLT_LASTFVAL][n] = dz->parray[FLT_FRQ][n];
		dz->parray[FLT_LASTAVAL][n] = dz->parray[FLT_AMP][n];
	}
	dz->tempsize = dz->outsams + extraspace;	//	Reset param for write_display
	flt_frq_index = flt_cnt;
	flt_times_cnt = 1;
	sndendset	= 0;
	sndend		= 0;
	tail_extend = 0;
	was_tail_extend = 0;
	gotend		= 0;

	dz->process = GREV;
	display_virtual_time(0,dz);
	dz->process = SYNFILT;
	fprintf(stdout,"INFO: Running filter.\n");
	fflush(stdout);
	if((exit_status = newfval(&(dz->iparam[FLT_FSAMS]),flt_cnt,flt_timeslots,&flt_frq_index,&flt_times_cnt,dz))<0)
		return(exit_status);
	srand((int)dz->iparam[SYNFLT_SEED]);	//	Reset random generator
	startup = 1;
	while(dz->samps_left > 0 || tail_extend) {
		memset((char *)buf,0,(size_t) (dz->buflen * sizeof(float)));
		if(tail_extend)
			dz->ssampsread = 0;			
		else {
			gen_noise(buf,dz);
			if(startup) {							//	Fade-up splice at start of synth material
				for(n = 0;n < synfsplic*chans; n+=chans) {
					for(m = 0; m < chans; m++)
						buf[n+m] = (float)(buf[n+m] * splice);
					splice += splincr;
				}
				startup = 0;					//	Fade-down splice at end of synth material
			} else if((gap_from_end = dz->total_samps_read - downsplicestart) >= 0) {
				n = (dz->total_samps_read % dz->buflen) - gap_from_end;
				splice = 1.0;
				k = 0;
				while(n < 0) {
					splice -= splincr;
					n += chans;
					k++;
				}
				while(k < synfsplic) {
					for(m=0;m<chans;m++)
						buf[n+m] = (float)(buf[n+m] * splice);
					splice -= splincr;
					k++;
					if((n += chans) >= dz->buflen)
						break;
				}
			}
			if(dz->samps_left <= 0) {
				was_tail_extend = 1;
				tail_extend = 1;
				fprintf(stdout,"INFO: Writing Filter tail.\n");
				fflush(stdout);
			}
        }
		if(tail_extend) {
			bufspace = dz->buflen;
			dz->ssampsread = dz->buflen;				
		}
		if((exit_status = do_fvary_filters(flt_inv_sr,flt_cnt,&flt_times_cnt,&flt_sams,&flt_q_incr,&flt_blokcnt,&flt_ovflw,flt_timeslots,&flt_frq_index,1,dz))<0)
			return(exit_status);
		if(tail_extend) {
			if(tailincr != 1.0) {							//	If tail is to nbe daded
				for(n=0;n<dz->buflen;n++) {					//	Envelope it here
					buf[n] = (float)(buf[n] * tailfade);
					tailfade *= tailincr;
				}
			}
			sndend = dz->buflen;
			framend = dz->buflen;
			for(k = framecnt; k > 0; k--) {					//	Search backwards thro buffer, frame by frame
				framestart = framend - framesize;
				maxsamp = 0.0;
				for(n = framend-chans;n >= framestart;n-=chans) {	
					for(m=0;m<chans;m++) {					//	Search backwards thro frame, samp-grup by samp-group
						if(fabs(buf[n+m]) > maxsamp) {
							if(!sndendset) {				//	If samples cease to be zero
								sndend = n + chans;			//	Mark start of end-zeros in buffer
								sndendset = 1;				//	and flag that snd end has been found
							}
							maxsamp = fabs(buf[n+m]);
						}
					}
					if(maxsamp < MINUS96DB) {				//	If max level in frame falls below -96dB
						if(sndendset) {						//	If we found a place in buffer after which samples were all zero
							dz->ssampsread = sndend;		//	Mark this as end of output, and quit the main filtering loop
							tail_extend = 0;				//	by setting tail_extend to zero
							dz->samps_left = 0; // SAFETY	
							break;
						} else								//	If we didn't find place .... 	
							sndend = framestart;			//	Then all samples in the frame are zero.
					}										//	So move snd end to start of current frame.
				}											//	.. and search backwards thro previous frame

				if(tail_extend == 0) {
					break;
				}
				framend = framestart;
			}
			if((maxsamp < MINUS96DB) && !sndendset) {		//	Entire buffer is "zero"
				tail_extend = 0;							//	Force exit by seting tail_extend to zero
				dz->samps_left = 0; // SAFETY				//	DO NOT set samps_read to 0, as there may be input samples still to write	
			}
		}
		if(tail_extend)
			tail_extend++;
		if(dz->ssampsread > 0) {
			dz->process = GREV;	//	Forces correct progress-bar write
			if((exit_status = write_samps(dz->sampbuf[0],dz->ssampsread,dz))<0)
				return(exit_status);
			dz->process = SYNFILT;
		}
	}				
    if(flt_ovflw > 0)  {
		fprintf(stdout,"INFO: Number of overflows: %d\n",flt_ovflw);
		fflush(stdout);
	}
	return(FINISHED);
}

/*************************** DO_FVARY_FILTERS *****************************/

int do_fvary_filters
(double flt_inv_sr,int flt_cnt,int *flt_times_cnt,int *flt_sams,double *flt_q_incr,int *flt_blokcnt,int *flt_ovflw,
 int flt_timeslots,int *flt_frq_index,int running,dataptr dz)
{
	int exit_status;
	int    n, m, fno, chans = dz->infile->channels;
	float *buf = dz->sampbuf[0];

	double *fincr = dz->parray[FLT_FINCR];
	double *aincr = dz->parray[FLT_AINCR];
 
 	double *ampl = dz->parray[FLT_AMPL];
	double *a = dz->parray[FLT_A];
	double *b = dz->parray[FLT_B];
	double *y = dz->parray[FLT_Y];
	double *z = dz->parray[FLT_Z];
	double *d = dz->parray[FLT_D];
	double *e = dz->parray[FLT_E];
 	int fsams = dz->iparam[FLT_FSAMS];
	if (dz->vflag[DROP_OUT_AT_OVFLOW]) {
		for (n = 0; n < dz->ssampsread; n += chans) { 
			if(fsams <= 0) {
				if((exit_status = newfval(&fsams,flt_cnt,flt_timeslots,flt_frq_index,flt_times_cnt,dz))<0)
					return(exit_status);
			}
			if(dz->brksize[SYNFLT_Q]) {
				if((*flt_sams -= chans) <= 0) {
					if(!newq(flt_q_incr,flt_sams,dz)) {
						sprintf(errstr,"Ran out of Q values: do_fvary_filters()\n");
						return(PROGRAM_ERROR);
					}
					*flt_sams *= chans;
				}
			}
			if((*flt_blokcnt -= chans) <= 0) {
				for (fno = 0; fno < flt_cnt; fno++) {
					get_syncoeffs1(fno,flt_inv_sr,dz);
					get_syncoeffs2(fno,dz);
				}
				if(dz->brksize[SYNFLT_Q])
					dz->param[SYNFLT_Q] *= *flt_q_incr;
				
				for(m=0;m<flt_cnt;m++) {
					dz->parray[FLT_FRQ][m] *= fincr[m];
					dz->parray[FLT_AMP][m] *= aincr[m];
				}
				*flt_blokcnt = BSIZE * chans;
			}
			filtering(n,chans,buf,a,b,y,z,d,e,ampl,flt_cnt,flt_ovflw,running,dz);
			if(*flt_ovflw > 0) {
				sprintf(errstr,"Filter overflowed\n");
				return(GOAL_FAILED);
			}
			fsams--;
		}
	} else {
		for (n = 0; n < dz->ssampsread; n += chans) { 
			if(fsams <= 0) {
				if((exit_status = newfval(&fsams,flt_cnt,flt_timeslots,flt_frq_index,flt_times_cnt,dz))<0)
					return(exit_status);
			}
			if(dz->brksize[SYNFLT_Q]) {
				if((*flt_sams -= chans) <= 0) {
					if(!newq(flt_q_incr,flt_sams,dz)) {
			 			sprintf(errstr,"Ran out of Q values: do_fvary_filters()\n");
						return(PROGRAM_ERROR);
					}
					*flt_sams *= chans;
				}
			}
			if((*flt_blokcnt -= chans) <= 0) {
				for (fno = 0; fno < flt_cnt; fno++) {
					get_syncoeffs1(fno,flt_inv_sr,dz);
					get_syncoeffs2(fno,dz);
				}
				if(dz->brksize[SYNFLT_Q])
					dz->param[SYNFLT_Q] *= *flt_q_incr;

				for(m=0;m<flt_cnt;m++) {
					dz->parray[FLT_FRQ][m] *= fincr[m];
					dz->parray[FLT_AMP][m] *= aincr[m];
				}
				*flt_blokcnt = BSIZE * chans;
			}
			filtering(n,chans,buf,a,b,y,z,d,e,ampl,flt_cnt,flt_ovflw,running,dz);
			fsams--;
		}
	}
 	dz->iparam[FLT_FSAMS] = fsams;
	return(CONTINUE);
}

/******************************* NEWQ ***************************
 *
 * VAL     is the base value from which we calculate.
 * VALINCR is the value increment per block of samples.
 * SAMPCNT is the number of samples from 1 brkpnt val to next.
 */					   

int newq(double *flt_q_incr, int *flt_sams, dataptr dz)
{
	double *p;				   
	double ratio, one_over_steps;
	double thistime;
	double thisval;
	p = dz->brkptr[SYNFLT_Q];
	if(p - dz->brk[SYNFLT_Q] >= dz->brksize[SYNFLT_Q] * 2)
		return(FALSE);
	thistime = (double)round((*p++) * dz->infile->srate);
	thisval  = *p++;
	*flt_sams    = round(thistime - dz->lastind[SYNFLT_Q]);
	/* steps = no_of_samples/sampsize_of_blok: therefore.. */
	one_over_steps = (double)BSIZE/(double)(*flt_sams);	
	ratio                = (thisval/dz->lastval[SYNFLT_Q]);
	*flt_q_incr = pow(ratio,(one_over_steps));
	dz->param[SYNFLT_Q]     = dz->lastval[SYNFLT_Q];
	dz->lastval[SYNFLT_Q]   = thisval;
	dz->lastind[SYNFLT_Q]   = thistime;
	dz->brkptr[SYNFLT_Q]    = p;
	return(TRUE);
}

/******************************* NEWFVAL ***************************
 *
 * VAL is the base value from which we calculate.
 * VALINCR is the value increment per block of samples.
 * FSAMS is the number of samples (per channel) from 1 brkpnt val to next.
 * brk is the particular table we're accessing.
 */

int newfval(int *fsams,int flt_cnt,int flt_timeslots,int *flt_frq_index,int *flt_times_cnt,dataptr dz)
{
	int   thistime, lasttime;
	double rratio, one_over_steps;
	int   n,m,k;
	double thisval;
	double *lastfval = dz->parray[FLT_LASTFVAL];
	double *lastaval = dz->parray[FLT_LASTAVAL];
	double *aincr    = dz->parray[FLT_AINCR];
	double *fincr    = dz->parray[FLT_FINCR];
	int total_frqcnt = flt_cnt * flt_timeslots;
	
	if(*flt_times_cnt>flt_timeslots) {
		sprintf(errstr,"Ran off end of filter data: newfval()\n"); 
		return(PROGRAM_ERROR);
	}
	k = *flt_times_cnt;
	lasttime  = dz->lparray[FLT_SAMPTIME][k-1];
	thistime  = dz->lparray[FLT_SAMPTIME][k];
	*fsams = thistime - lasttime;
	/* steps  = fsams/BSIZE: therefore ... */	
	one_over_steps  = (double)BSIZE/(double)(*fsams);	
	if(*flt_frq_index >= total_frqcnt)
		return(FINISHED);
	for(n=0, m= *flt_frq_index;n<flt_cnt;n++,m++) {
	/* FREQUENCY */
		thisval = dz->parray[FLT_INFRQ][m];
		if(flteq(lastfval[n],thisval))
			fincr[n] = 1.0;
		else {
			rratio = (thisval/lastfval[n]);
			fincr[n] = pow(rratio,one_over_steps);
		}
		dz->parray[FLT_FRQ][n] = lastfval[n];
		lastfval[n] = thisval;
	/* AMPLITUDE */
		thisval = dz->parray[FLT_INAMP][m];
		if(flteq(thisval,lastaval[n]))
			aincr[n] = 1.0;
		else {
			rratio = (thisval/lastaval[n]);
			aincr[n] = pow(rratio,one_over_steps);
		}
		dz->parray[FLT_AMP][n] = lastaval[n];
		lastaval[n] = thisval;
	}
	*flt_frq_index += flt_cnt;
	(*flt_times_cnt)++;
	return(FINISHED);
}

/************************** FILTERING ****************************/

void filtering(int n,int chans,float *buf,double *a,double *b,double *y,double *z,
				double *d,double *e,double *ampl,int flt_cnt,int *flt_ovflw,int running,dataptr dz)
{
	double input, sum, xx;
	int chno, this_samp, fno, i;

	for(chno = 0; chno < chans; chno++) {
		this_samp = n + chno;
		input = (double)buf[this_samp];
		sum   = 0.0;
		for (fno = 0; fno < flt_cnt; fno++) {
			i    = (fno * chans) + chno;
			xx   = input + (a[fno] * y[i]) + (b[fno] * z[i]);
			z[i] = y[i];
			y[i] = xx;
			if(dz->vflag[FLT_DBLFILT]) {
				xx   += (a[fno] * d[i]) + (b[fno] * e[i]);
				e[i] = d[i];
				d[i] = xx;
			}
			sum += (xx * ampl[fno]);
		}
		sum *= dz->synfgain;
		if(running)
			sum = check_float_limits(sum,flt_ovflw,dz);
		buf[this_samp] = (float) sum;
	}
}

/************************ CHECK_FLOAT_LIMITS **************************/

//TODO: if shorts o/p - do clipping; if floatsams, report but don't change!
double check_float_limits(double sum,int *flt_ovflw,dataptr dz)
{
	double peak = fabs(sum);
#ifdef NOTDEF
	//do this when 'modify loudness' can handle floatsams!
	if(dz->true_outfile_stype== SAMP_FLOAT){
		
		if(peak > 1.0){
			(*flt_ovflw)++;
			dz->peak_fval = max(dz->peak_fval,peak);
		}		
	}
	else {
#endif
		if (sum > 1.0) {
//TW SUGGEST KEEP THIS; prevents FILTER BLOWING UP: see notes
			dz->synfgain *= 0.9999;
			(*flt_ovflw)++;
			dz->peak_fval = max(dz->peak_fval,peak);
			//return(1.0);
			if(dz->clip_floatsams)
				sum = 1.0;
		}
		if (sum < -1.0) {
//TW SUGGEST KEEP THIS; prevents FILTER BLOWING UP: see notes

			dz->synfgain *= 0.9999;
			(flt_ovflw)++;
			dz->peak_fval = max(dz->peak_fval,peak);
			//return(-1.0);
			if(dz->clip_floatsams)
				sum = -1.0;
		}
#ifdef NOTDEF
	}
#endif
	return sum;
}

/************************ GEN_NOISE **************************/

void gen_noise(float *buf,dataptr dz)
{
	int n;
	dz->ssampsread = min(dz->buflen,dz->samps_left);
	for(n=0;n<dz->ssampsread;n++)
		buf[n] = (float)(((drand48() * 2.0) - 1.0) * dz->scalefact);
	dz->total_samps_read += dz->ssampsread;
	dz->samps_left -= dz->ssampsread;
}

/************************** SYNFILTER_PREPROCESS **************************/

int synfilter_preprocess(int flt_cnt,double flt_inv_sr,dataptr dz)
{
	int exit_status;
	if((exit_status = allocate_filter_internalparam_arrays(flt_cnt,dz))<0)
		return(exit_status);
	if((exit_status = initialise_filter_params(flt_cnt,flt_inv_sr,dz))<0)
		return(exit_status);
	return(FINISHED);
}

/**************************** ALLOCATE_FILTER_INTERNALPARAM_ARRAYS *******************************/

int allocate_filter_internalparam_arrays(int fltcnt,dataptr dz)
{
	int chans = dz->infile->channels;
	if((dz->parray[FLT_AMPL] = (double *)calloc(fltcnt * sizeof(double),sizeof(char)))==NULL
	|| (dz->parray[FLT_A]    = (double *)calloc(fltcnt * sizeof(double),sizeof(char)))==NULL
	|| (dz->parray[FLT_B]    = (double *)calloc(fltcnt * sizeof(double),sizeof(char)))==NULL
	|| (dz->parray[FLT_WW]   = (double *)calloc(fltcnt * sizeof(double),sizeof(char)))==NULL
	|| (dz->parray[FLT_COSW] = (double *)calloc(fltcnt * sizeof(double),sizeof(char)))==NULL
	|| (dz->parray[FLT_SINW] = (double *)calloc(fltcnt * sizeof(double),sizeof(char)))==NULL
	|| (dz->parray[FLT_Y]    = (double *)calloc(fltcnt * chans * sizeof(double),sizeof(char)))==NULL
	|| (dz->parray[FLT_Z]    = (double *)calloc(fltcnt * chans * sizeof(double),sizeof(char)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for arrays of filter parameters.\n");
		return(MEMORY_ERROR);
	}
    if(dz->vflag[FLT_DBLFILT]) {
		if((dz->parray[FLT_D]    = (double *)calloc(fltcnt * chans * sizeof(double),sizeof(char)))==NULL
		|| (dz->parray[FLT_E]    = (double *)calloc(fltcnt * chans * sizeof(double),sizeof(char)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for double filtering parameters.\n");
			return(MEMORY_ERROR);
		}
	}
	return(FINISHED);
}

/************************ INITIALISE_FILTER_PARAMS ***************************/

int initialise_filter_params(int flt_cnt,double flt_inv_sr,dataptr dz)
{
	int n, chno, k;
	int chans = dz->infile->channels;
	for(n=0;n<flt_cnt;n++) {
		get_syncoeffs1(n,flt_inv_sr,dz);
		get_syncoeffs2(n,dz);
		for(chno=0;chno<chans;chno++) {
			k = (n*chans)+chno;
			if(dz->vflag[FLT_DBLFILT]) {
				dz->parray[FLT_D][k] = 0.0;
				dz->parray[FLT_E][k] = 0.0;
			}
			dz->parray[FLT_Y][k]  = 0.0;
			dz->parray[FLT_Z][k]  = 0.0;
		}	
	}
	return(FINISHED);
}

/*********************** GET_COEFFS1 *************************/

void get_syncoeffs1(int n,double flt_inv_sr,dataptr dz)
{
	dz->parray[FLT_WW][n]   = 2.0 * PI * dz->parray[FLT_FRQ][n] * flt_inv_sr;
	dz->parray[FLT_COSW][n] = cos(dz->parray[FLT_WW][n]);
	dz->parray[FLT_SINW][n] = sin(dz->parray[FLT_WW][n]);
}

/*********************** GET_COEFFS2 ***************************/

void get_syncoeffs2(int n,dataptr dz)
{
	double g, r;
	r    = exp( -(dz->parray[FLT_WW][n])/(2.0 * dz->param[SYNFLT_Q]));
	dz->parray[FLT_A][n] = 2.0 * r * dz->parray[FLT_COSW][n];
	dz->parray[FLT_B][n] = -(r) * r;
	g    = 1.0 / ((1.0  + dz->parray[FLT_B][n]) * dz->parray[FLT_SINW][n]);
	dz->parray[FLT_AMPL][n] = dz->parray[FLT_AMP][n]/g;
	if(dz->vflag[FLT_DBLFILT])
		dz->parray[FLT_AMPL][n] /= g;
}

/********************** READ_THE_SPECIAL_DATA ************************/

int read_the_special_data(char *str,int *flt_wordcnt,int *flt_entrycnt,int *flt_cnt,int *flt_timeslots,dataptr dz)	   
{
//	int exit_status = FINISHED;
	aplptr ap = dz->application;

	dz->application->min_special = unchecked_hztomidi(FLT_MINFRQ);
	dz->application->max_special = MIDIMAX;
	switch(ap->special_data) {
	case(SYN_FILTERBANK):			return get_data_from_fsyn_infile(str,flt_wordcnt,flt_entrycnt,flt_cnt,flt_timeslots,dz);
	case(TIMEVARYING_FILTERBANK):	return get_data_from_tvary_infile(str,flt_wordcnt,flt_entrycnt,flt_cnt,flt_timeslots,dz);
	default:
		sprintf(errstr,"Unknown special_data type: read_special_data()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/**************************** ALLOCATE_FILTER_FRQ_AMP_ARRAYS *******************************/

int allocate_filter_frq_amp_arrays(int fltcnt,dataptr dz)
{
	if((dz->parray[FLT_AMP] = (double *)calloc(fltcnt * sizeof(double),sizeof(char)))==NULL
	|| (dz->parray[FLT_FRQ] = (double *)calloc(fltcnt * sizeof(double),sizeof(char)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for filter amp and frq arrays.\n");
		return(MEMORY_ERROR);
	}
	return(FINISHED);
}

/**************************** GET_DATA_FROM_TVARY_INFILE *******************************/

int get_data_from_tvary_infile(char *filename,int *flt_wordcnt,int *flt_entrycnt,int *flt_cnt,int *flt_timeslots,dataptr dz)
{
	int exit_status, addrow = 0;
	char *temp, *p, *thisword;
	int maxlinelen, frqcnt;
	int total_wordcnt = 0, n, k;
	int  columns_in_this_row, columns_in_row = 0, number_of_rows = 0;
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
				return(exit_status);
			}
			if(number_of_rows == 0 && total_wordcnt == 0) {		/* Need ot insert value agt time 0.0 */
				if(!flteq(val,0.0))
					addrow = 1;
			}
			columns_in_this_row++;
			total_wordcnt++;
		}
		if(number_of_rows==0) {
			if((columns_in_row = columns_in_this_row)<3) {
				sprintf(errstr,"Insufficient filter data in row 1 of file %s.\n",filename);
				return(DATA_ERROR);
			} else if (ODD(columns_in_row - 1)) {
				sprintf(errstr,"Frq and Amp data not paired correctly (or no Time) in row 1 of file %s.\n",filename);
				return(DATA_ERROR);
			}
		} else if(columns_in_this_row!=columns_in_row) {
			if(columns_in_this_row < columns_in_row)
				sprintf(errstr,"Not enough entries in row %d of file %s\n",number_of_rows+1,filename);
			else
				sprintf(errstr,"Too many entries in row %d of file %s\n",number_of_rows+1,filename);
			return(DATA_ERROR);
		}
		number_of_rows++;
	}
	if(columns_in_row<3) {
		sprintf(errstr,"Insufficient data in each row, to define filters.\n");
		return(DATA_ERROR);
	}
	if(number_of_rows<2) {
		sprintf(errstr,"Insufficient data in. Must be at least 2 lines in file %s.\n",filename);  //RWD final ) in wrong place!
		return(DATA_ERROR);
	}
	frqcnt = columns_in_row - 1;
	if(ODD(frqcnt)) {
		sprintf(errstr,"amplitude and freq data not correctly paired in rows.\n");
		return(DATA_ERROR);
	}
	*flt_wordcnt = total_wordcnt;
	if(addrow) {
		*flt_wordcnt += columns_in_row;
		number_of_rows++;
	}
	if((dz->parray[FLT_FBRK] = (double *)malloc(*flt_wordcnt * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to allocate filter brktable.\n");
		return(MEMORY_ERROR);
	}
	*flt_entrycnt = columns_in_row;
	*flt_cnt       = frqcnt/2;
	*flt_timeslots = number_of_rows;
	fseek(dz->fp,0,0);
	total_wordcnt = 0;
	if(addrow)
		total_wordcnt += columns_in_row;		//	Leave space for zero-time line

	while(fgets(temp,maxlinelen,dz->fp)!=NULL) {
		columns_in_this_row = 0;
		if(is_an_empty_line_or_a_comment(temp))
			continue;
		p = temp;
		while(get_word_from_string(&p,&thisword)) { 
			if((exit_status = get_level(thisword,&val))<0) {	/* reads vals or dB vals */
				return(exit_status);
			}
			dz->parray[FLT_FBRK][total_wordcnt] = val;
			total_wordcnt++;
		}
	}
	if(fclose(dz->fp)<0) {
		fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
		fflush(stdout);
	}
	if(addrow) {					//	Add row at time 0, by duplicating vals in next row
		k = columns_in_row;
		for(n=0; n < columns_in_row;n++,k++)
			dz->parray[FLT_FBRK][n] = dz->parray[FLT_FBRK][k];
		dz->parray[FLT_FBRK][0] = 0.0;
	}
	return(FINISHED);
}

/**************************** GET_DATA_FROM_FSYN_INFILE *******************************/

int get_data_from_fsyn_infile(char *filename,int *flt_wordcnt,int *flt_entrycnt,int *flt_cnt,int *flt_timeslots,dataptr dz)
{
	int exit_status, addrow = 0;
	char *temp, *p, *thisword;
	int maxlinelen;
	int total_wordcnt = 0;
	int  columns_in_this_row, number_of_rows = 0;
	double zval = 0.0;

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
				return(exit_status);
			}
			if(number_of_rows == 0) {
				if(total_wordcnt == 0) {
					if(!flteq(val,0.0))
						addrow = 1;
				} else
					zval = val;
			}
			columns_in_this_row++;
			total_wordcnt++;
		}
		if(columns_in_this_row!=2) {
			sprintf(errstr,"Wrong number of entries in row %d of file %s\n",number_of_rows+1,filename);
			return(DATA_ERROR);
		}
		number_of_rows++;
	}
	if(number_of_rows < 2) {
		sprintf(errstr,"Insufficient data in file %s (at least 2 lines required)\n",filename);
		return(DATA_ERROR);
	}
	if(ODD(total_wordcnt)) {
		sprintf(errstr,"Data in file %s not paired correctly\n",filename);
		return(DATA_ERROR);
	}
	if(addrow) {
		number_of_rows++;
		total_wordcnt += 2;						//	Space for extra row at time 0
	}
	*flt_wordcnt = (total_wordcnt/2) * 3;		//	Amplitude info (1.0) is added as a srd row
	if((dz->parray[FLT_FBRK] = (double *)malloc(*flt_wordcnt * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to allocate filter brktable.\n");
		return(MEMORY_ERROR);
	}
	fseek(dz->fp,0,0);
	total_wordcnt = 0;
	if(addrow) {								//	Add line at zero time, if ness
		dz->parray[FLT_FBRK][total_wordcnt++] = 0.0;
		dz->parray[FLT_FBRK][total_wordcnt++] = zval;
		dz->parray[FLT_FBRK][total_wordcnt++] = 1.0;
	}
	while(fgets(temp,maxlinelen,dz->fp)!=NULL) {
		if(is_an_empty_line_or_a_comment(temp))
			continue;
		p = temp;
		while(get_word_from_string(&p,&thisword)) { 
			if((exit_status = get_level(thisword,&val))<0) {	/* reads vals or dB vals */
				return(exit_status);
			}
			dz->parray[FLT_FBRK][total_wordcnt] = val;
			total_wordcnt++;
		}
		dz->parray[FLT_FBRK][total_wordcnt] = 1.0;				/* once line is read, add a standard amp of 1.0 */
		total_wordcnt++;
	}
	*flt_entrycnt = 3;	/*  number of entries in line is standard 3 */
	*flt_cnt      = 1;	/*	number of frqs in each line is 1 */
	*flt_timeslots = number_of_rows;
	if(fclose(dz->fp)<0) {
		fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
		fflush(stdout);
	}
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

/**************************** CHECK_FILTER_DATA *******************************/

int check_filter_data(int flt_entrycnt,int *flt_wordcnt,int *flt_timeslots,dataptr dz)
{
	int    exit_status;	
	int   n, lastfilt;
	double endtime;
	int    total_wordcnt = *flt_wordcnt;
	if(dz->parray[FLT_FBRK][0] < 0.0) {
		sprintf(errstr,"Negative time value (%lf) on line 1.\n",dz->parray[FLT_FBRK][0]);
		return(DATA_ERROR);
	}
	if(flteq(dz->parray[FLT_FBRK][0],0.0))		
		dz->parray[FLT_FBRK][0] = 0.0;			/* FORCE A FILTER SETTING AT TIME ZERO */
	else {				 			
		if((dz->parray[FLT_FBRK] = 
		(double *)realloc(dz->parray[FLT_FBRK],(total_wordcnt+flt_entrycnt) * sizeof(double)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to reallocate filter brktable.\n");
			return(MEMORY_ERROR);
		}
		for(n=total_wordcnt-1; n>=0; n--)
			dz->parray[FLT_FBRK][n + flt_entrycnt] = dz->parray[FLT_FBRK][n];
		total_wordcnt += flt_entrycnt;
		dz->parray[FLT_FBRK][0] = 0.0;
		(*flt_timeslots)++;
	}
	if((exit_status = check_seq_and_range_of_filter_data(dz->parray[FLT_FBRK],flt_entrycnt,total_wordcnt,&endtime,dz))<0)
		return(exit_status);
								/* FORCE A FILTER SETTING AT (BEYOND) END OF FILE */

	if(endtime <= SYNFDOVE * 2 * MS_TO_SECS) {
		sprintf(errstr,"Data too short (timewise) for synthesis: must be > 30mS.\n");
		return DATA_ERROR;
	}
	lastfilt = total_wordcnt - flt_entrycnt;

	dz->outsams = (int)round(endtime * (double)dz->iparam[SYNFLT_SRATE]) * dz->iparam[SYNFLT_CHANS];
	dz->samps_left = dz->outsams;

	if(*flt_timeslots < 2) {
		sprintf(errstr,"Error in timeslot logic: check_filter_data()\n");
		return(PROGRAM_ERROR);
	}
	*flt_wordcnt = total_wordcnt;
	return(FINISHED);
}

/**************************** CHECK_SEQ_AND_RANGE_OF_FILTER_DATA *******************************/

int check_seq_and_range_of_filter_data(double *fbrk,int flt_entrycnt,int flt_wordcnt,double *endtime,dataptr dz)
{
	double lasttime = 0.0;
	int n, m, lineno;
	for(n=1;n<flt_wordcnt;n++) {
		m = n%flt_entrycnt;
		lineno = (n/flt_entrycnt)+1;	/* original line-no : ignoring comments */
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
			fbrk[n] = miditohz(fbrk[n]);
		} else
			fbrk[n] = max(fbrk[n],MINFILTAMP);	/* Zero filter amp, forced to +ve, but effectively zero */
	}
	*endtime = lasttime;
	return(FINISHED);
}
