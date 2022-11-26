// _cdprogs\motor motor 1 motorsrc.wav test.wav 10 20 .5 .6 .8 .5

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


#define gp_maxinsmps	rampbrksize
#define gp_maxpulsesmps	total_windows
#define minpulsesmps	temp_sampsize
#define symwarning		fzeroset
#define inbufcnt		is_mapping

#define	MOT_SNGLE	0
#define	MOT_SLICE	1
#define	MOT_MULTI	2

#ifdef unix
#define round(x) lround((x))
#endif

char errstr[2400];

int anal_infiles = 1;
int	sloom = 0;
int sloombatch = 0;

const char* cdp_version = "6.1.0";

//CDP LIB REPLACEMENTS
static int check_motor_param_validity_and_consistency(dataptr dz);
static int setup_motor_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_motor_param_ranges_and_defaults(dataptr dz);
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
static int motor(dataptr dz);
static int create_motor_sndbufs(dataptr dz);
static int motor_param_preprocess(dataptr dz);
static void rndpermm(int permlen,int *permm);
static void insert(int m,int t,int permlen,int *permm);
static void prefix(int m,int permlen,int *permm);
static void shuflup(int k,int permlen, int *permm);
static int generate_inner_pulse(float *ebuf,float *ibuf,float *obuf,int ibufpos,int obufpos,int gp_eventsamps,int gp_tail,double incr,
								double trem,int output,dataptr dz);
static int calculate_cresc_and_decresc_counts_of_inner_events(int *cresc_cnt,int *decresc_cnt,double *cresctime, double *decresctime,
															  double sym,double symrnd,double pulsdur,double frq,dataptr dz);
static void calculate_fwd_and_bkwd_sampsteps_in_infile(int *in_upstep,int *in_dnstep,double srate,int chans,
													   int gp_sampsread,int cresc_cnt,int decresc_cnt,double inner_dur,double edge,int ibufno,dataptr dz);
static void calculate_inputsamps_to_read_and_length_of_tail(int *gp_eventsamps,int *gp_tail,double srate,double inner_dur,double fratio,double edge,dataptr dz);
static int calculate_max_read_events_in_any_env_pulse(int *max_cresccnt,int *max_innercnt,double *frq,double *edge,int arraysize,int *permm,int permcnt,
													   int *gp_sampsread,dataptr dz);

static int select_infile_to_use(int *bufcntr,int *permm,int permcnt,dataptr dz);
static int handle_the_special_data(char *str,int *max_gp_seg,dataptr dz);

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
	int exit_status, modetype;
	dataptr dz = NULL;
	char **cmdline;
	int  cmdlinecnt;
	int n, max_gp_seg = 0;
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
		dz->maxmode = 9;
		if((exit_status = get_the_mode_from_cmdline(cmdline[0],dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(exit_status);
		}
		cmdline++;
		cmdlinecnt--;
		// setup_particular_application =
		if((exit_status = setup_motor_application(dz))<0) {
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

	modetype = dz->mode % 3;

	// parse_infile_and_hone_type() = 
	if((exit_status = parse_infile_and_check_type(cmdline,dz))<0) {
		exit_status = print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	// setup_param_ranges_and_defaults() =
	if((exit_status = setup_motor_param_ranges_and_defaults(dz))<0) {
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

//	handle_extra_infiles()		CDP LIB
	if((exit_status = handle_extra_infiles(&cmdline,&cmdlinecnt,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	dz->gp_maxinsmps = 0;
	if(modetype != MOT_SLICE) {
		dz->inbufcnt = dz->infilecnt;
		for(n=0;n<dz->inbufcnt;n++)	//	Largest buffer for input data is size of largest infile
			dz->gp_maxinsmps = max(dz->gp_maxinsmps,dz->insams[n]);
		dz->gp_maxinsmps /= dz->infile->channels;
	}
	// handle_outfile() = 
	if((exit_status = handle_the_outfile(&cmdlinecnt,&cmdline,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}

//	handle_formants()			redundant
//	handle_formant_quiksearch()	redundant
//	handle_special_data....
	if(modetype == MOT_SLICE) {
		if((exit_status = handle_the_special_data(cmdline[0],&max_gp_seg,dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		dz->inbufcnt = dz->itemcnt;
		dz->gp_maxinsmps = max_gp_seg;	//	Largest buffer for input data is size of largest cut seg
		cmdlinecnt--;
		cmdline++;
	}
	if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {		// CDP LIB
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
//	check_param_validity_and_consistency....
	if((exit_status = check_motor_param_validity_and_consistency(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	//param_preprocess =
	if((exit_status = motor_param_preprocess(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	is_launched = TRUE;
	dz->bufcnt = dz->inbufcnt + 3;
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

	if((exit_status = create_motor_sndbufs(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	//spec_process_file =
	if((exit_status = motor(dz))<0) {
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
	
	switch(dz->input_data_type) {
	case(SNDFILES_ONLY):	
		dz->infilecnt = 1;	
		break;
	case(MANY_SNDFILES):	
		dz->infilecnt = -2;	
		break;
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

/************************* SETUP_MOTOR_APPLICATION *******************/

int setup_motor_application(dataptr dz)
{
	int exit_status, modetype = dz->mode % 3;
	aplptr ap;
	if((exit_status = establish_application(dz))<0)		// GLOBAL
		return(FAILED);
	ap = dz->application;
	// SEE parstruct FOR EXPLANATION of next 2 functions
	
	switch(modetype) {
	case(MOT_SNGLE):	exit_status = set_param_data(ap,0        ,6,6,"dDDDDD");	break;
	case(MOT_SLICE):	exit_status = set_param_data(ap,MOTORDATA,6,6,"dDDDDD");	break;
	case(MOT_MULTI):	exit_status = set_param_data(ap,0        ,6,6,"dDDDDD");	break;
	}
	if((exit_status = set_param_data(ap,0   ,6,6,"DDDDDd"))<0)
		return(FAILED);
	switch(modetype) {
	case(MOT_SNGLE):	exit_status = set_vflgs(ap,"fpjtyebvs",9,"DDDDDDDDi","a",1,0,"0");		break;
	case(MOT_SLICE):	//	 fall thro
	case(MOT_MULTI):	exit_status = set_vflgs(ap,"fpjtyebvs",9,"DDDDDDDDi","ac",2,0,"00");	break;
	}
	if(exit_status<0)
		return(FAILED);
	// set_legal_infile_structure -->
	dz->has_otherfile = FALSE;
	// assign_process_logic -->
	switch(modetype) {
	case(MOT_SNGLE): //	 fall thro
	case(MOT_SLICE): dz->input_data_type = SNDFILES_ONLY;	break;
	case(MOT_MULTI): dz->input_data_type = MANY_SNDFILES;	break;
	}
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
		} else if((exit_status = copy_parse_info_to_main_structure(infile_info,dz))<0) {
			sprintf(errstr,"Failed to copy file parsing information\n");
			return(PROGRAM_ERROR);
		}
		free(infile_info);
	}
	return(FINISHED);
}

/************************* SETUP_MOTOR_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_motor_param_ranges_and_defaults(dataptr dz)
{
	int exit_status;
	aplptr ap = dz->application;
	// set_param_ranges()
	ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
	// NB total_input_param_cnt is > 0 !!!
	if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
		return(FAILED);
	// get_param_ranges()
	ap->lo[MOT_DUR]	= 1.0;
	ap->hi[MOT_DUR]	= 7200.0;
	ap->default_val[MOT_DUR] = 20.0;
	ap->lo[MOT_FRQ]	= 2;
	ap->hi[MOT_FRQ]	= 100;
	ap->default_val[MOT_FRQ] = MOT_FRQ_DFLT;
	ap->lo[MOT_PULSE]	= 0.1;
	ap->hi[MOT_PULSE]	= 10.0;
	ap->default_val[MOT_PULSE] = 1.0;
	ap->lo[MOT_FRATIO]	= 0.0;
	ap->hi[MOT_FRATIO]	= 1.0;
	ap->default_val[MOT_FRATIO] = 0.5;
	ap->lo[MOT_PRATIO]	= 0.0;
	ap->hi[MOT_PRATIO]	= 1.0;
	ap->default_val[MOT_PRATIO] = 1.0;
	ap->lo[MOT_SYM]	= 0.0;
	ap->hi[MOT_SYM]	= 1.0;
	ap->default_val[MOT_SYM] = 0.5;
	ap->lo[MOT_FRND] = 0.0;
	ap->hi[MOT_FRND] = 1.0;
	ap->default_val[MOT_FRND] = 0.0;
	ap->lo[MOT_PRND] = 0.0;
	ap->hi[MOT_PRND] = 1.0;
	ap->default_val[MOT_PRND] = 0.0;
	ap->lo[MOT_JIT] = 0.0;
	ap->hi[MOT_JIT] = 3.0;
	ap->default_val[MOT_JIT] = 0.0;
	ap->lo[MOT_TREM] = 0.0;
	ap->hi[MOT_TREM] = 1.0;
	ap->default_val[MOT_TREM] = 0.0;
	ap->lo[MOT_SYMRND] = 0.0;
	ap->hi[MOT_SYMRND] = 1.0;
	ap->default_val[MOT_SYMRND] = 0.0;
	ap->lo[MOT_EDGE] = 0;
	ap->hi[MOT_EDGE] = 20.0;
	ap->default_val[MOT_EDGE] = 0;
	ap->lo[MOT_BITE] = 0.1;
	ap->hi[MOT_BITE] = 10.0;
	ap->default_val[MOT_BITE] = 3.0;
	ap->lo[MOT_VARY] = 0;
	ap->hi[MOT_VARY] = 1.0;
	ap->default_val[MOT_VARY] = 0.0;
	ap->lo[MOT_SEED] = 0;
	ap->hi[MOT_SEED] = 256;
	ap->default_val[MOT_SEED] = 0;
	dz->maxmode = 9;
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
			if((exit_status = setup_motor_application(dz))<0)
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
	usage2("motor");
	return(USAGE_ONLY);
}

/**************************** CHECK_MOTOR_PARAM_VALIDITY_AND_CONSISTENCY *****************************/

int check_motor_param_validity_and_consistency(dataptr dz)
{
	int exit_status;
	double maxratio, maxpulse, minfrq, minpulsedur, maxfrqdur;

	if(dz->brksize[MOT_FRATIO]) {
		if((exit_status = get_maxvalue_in_brktable(&maxratio,MOT_FRATIO,dz))<0)
			return exit_status;
	} else
		maxratio = dz->param[MOT_FRATIO];
	if(maxratio == 0.0) {
		sprintf(errstr,"Zero event-ratio will create a silent output.\n");
		return DATA_ERROR;
	}
	if(dz->brksize[MOT_PRATIO]) {
		if((exit_status = get_maxvalue_in_brktable(&maxratio,MOT_PRATIO,dz))<0)
			return exit_status;
	} else
		maxratio = dz->param[MOT_PRATIO];
	if(maxratio == 0.0) {
		sprintf(errstr,"Zero pulsed-enveloped-ratio will create a silent output.\n");
		return DATA_ERROR;
	}
	if((dz->brksize[MOT_VARY] || dz->param[MOT_VARY] > 0.0) && dz->vflag[MOT_FXDSTP]) {
		sprintf(errstr,"Fixed step and varying step in src-read cannot both be used.\n");
		return DATA_ERROR;
	}
	if(dz->brksize[MOT_PULSE]) {
		if((exit_status = get_maxvalue_in_brktable(&maxpulse,MOT_PULSE,dz))<0)
			return exit_status;
	} else
		maxpulse = dz->param[MOT_PULSE];
	if(dz->brksize[MOT_FRQ]) {
		if((exit_status = get_minvalue_in_brktable(&minfrq,MOT_FRQ,dz))<0)
			return exit_status;
	} else
		minfrq = dz->param[MOT_FRQ];
	minpulsedur = 1.0/maxpulse;
	minpulsedur *= maxratio;
	maxfrqdur   = 1.0/minfrq;
	if(minpulsedur <= 2.0 * maxfrqdur) {
		fprintf(stdout,"ERROR: Min outerpulse dur (1/rate(%lf) = %lf) less max-offtime (shorten by %.2lf) = %lf\n",maxpulse,1/maxpulse,1/maxpulse * (1 - dz->param[MOT_PRATIO]),(1/maxpulse) - (1/maxpulse * (1 - dz->param[MOT_PRATIO])));
		fprintf(stdout,"ERROR: is less than or equal to 2 * max innerpulse dur (1/rate(%lf) = %.2lf    X2=   %.2lf).\n",minfrq,1/minfrq,1/minfrq * 2);
		fflush(stdout);
		return DATA_ERROR;
	}
	return FINISHED;
}

/**************************** MOTOR_PARAM_PREPROCESS *****************************/

int motor_param_preprocess(dataptr dz)
{
	double maxpulse, minpulse, thispulse, srate = (double)dz->infile->srate;
	int chans = dz->infile->channels;
	int n, v;
	minpulse = HUGE;
	maxpulse = 0.0;
	if(dz->brksize[MOT_PULSE]) {
		for(n=0,v=1;n <dz->brksize[MOT_PULSE];n++,v+=2) {
			thispulse = 1.0/dz->brk[MOT_PULSE][v];
			maxpulse = max(maxpulse,thispulse);			//	Find the largest pulse to fit into a buffer, to help determine buffer size
			minpulse = min(minpulse,thispulse);			//	and the smallest pulse, to later determine the max number of pulses per buffer
		}
	} else {
		maxpulse = 1.0/dz->param[MOT_PULSE];
		minpulse = 1.0/dz->param[MOT_PULSE];
	}
	dz->gp_maxpulsesmps = (int)ceil(maxpulse * srate);
	dz->minpulsesmps    = (int)floor(minpulse * srate) * chans;

	if(dz->brksize[MOT_PULSE]) {						//	Convert Frq into wavelength
		for(n=0,v=1;n <dz->brksize[MOT_PULSE];n++,v+=2)
			dz->brk[MOT_PULSE][v] = 1.0/dz->brk[MOT_PULSE][v];
	} else
		dz->param[MOT_PULSE] = 1.0/dz->param[MOT_PULSE];
	return FINISHED;
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	if(!strcmp(prog_identifier_from_cmdline,"motor"))				dz->process = MOTOR;
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
	if(!strcmp(str,"motor")) {
		fprintf(stderr,
	    "USAGE: motor motor 1,4,7 infile outfile params\n"
	    "USAGE: motor motor 2,5,8 infile outfile data params\n"
	    "OR:    motor motor 3,6,9 inf1 [inf2 inf3 ....] outfile params\n"
		"Params are...\n"
		"dur freq pulse fratio pratio sym [-ffrand] [-pprand] [-jjitter] [-ttremor]\n"
		"[-yshift] [-eedge] [-bbite] [-vvary | -a] [-sseed] [-c]\n"
		"\n"
		"Create fast (inner) pulse-stream, within slower (outer) pulsed-enveloping.\n"
		"Under every outer-pulse, set of inner events cut successively from input src(s)\n"
		"as the outer-envelope rises, then in reverse order as it falls.\n"
		"Outer-pulse, shortened by \"PRATIO\", must hold at least 2 inner-pulses.\n"
		"\n"
		"Mode 1+3:  Typical source(s) short, & widening in frq-range from start to end.\n"
		"Mode 2:    Cuts segments from single src, at slice-times specified in \"data\".\n"
		"Modes 4-6: Similar except source-reads only advance.\n"
		"Modes 7-9: Similar except source-reads either only advance or only regress.\n"
		"\n"
		"DATA   Textfile of times in infile at which to slice it into separate srcs.\n"
		"DUR    Duration of the output file.\n"
		"FREQ   Pulse-rate (Hz) of inner-pulses (range 2 to 100).\n"
		"PULSE  Pulse-rate (Hz) of outer-pulses. (range 0.1 to 10)\n"
		"FRATIO Proportion of on-time to off-time of inner-events. (range 0 to 1)\n"
		"PRATIO Proportion of on-time to off-time of outer-events. (range 0 to 1)\n"
		"SYM    Symmetry of outer-pulses.  (range 0 to 1)\n"
		"       \"sym\" marks peak of rising-falling envelope on range 0 to 1.\n"
		"       0.5 gives symmetrical cresc-decresc envelope.\n"
		"       1 gives cresc envelope:      0 gives decresc envelope.\n"
		"       0.75 gives long cresc and short decresc. etc\n"
		"FRAND  Freq(f) randomisation (Range 0-1): max variation from f/2 to 3f/2\n"
		"PRAND  Pulse(p) randomisation (Range 0-1): max variation from p/2 to 3p/2\n"
		"JITTER Range of any pitch randomisation of inner-pulses (0 - 3 semitones).\n"
		"TREMOR Range of any random amplitude attenuation of inner-pulses (0-1).\n"
		"SHIFT  Range of any randomisation of outer-pulse symmetry (Range 0 to 1).\n"
		"EDGE   Length of decay-tail of inner-pulses (multiple of dur: Range 0 to 20)\n"
		"BITE   Shape of outer-pulses. (Range 0.1 to 10: Dflt 3). 1 = Linear rise-fall\n"
		"       > 1 slow-fast rise, fast-slow fall: < 1 fast-slow rise, slow-fast fall.\n"
		"VARY   Advance-step in src-read rand-varies from 1 outer-pulse to next.(0-1).\n"
		"       0 = no variation, 1 = max variation range (from no advance to max-step).\n"
		"SEED   Different seed vals gives different randomised outputs.(Range 0 to 256)\n"
		"-a     Inner-events under outer-pulse-cresc advance by fixed step.\n"
		"       (Default: Inner-events advance to end of source, unless \"vary\" set).\n"
		"-c     (Mode 2-3 only) cycle through input srcs.(Default, randomly permute order).\n");
	} else
		fprintf(stdout,"Unknown option '%s'\n",str);
	return(USAGE_ONLY);
}

int usage3(char *str1,char *str2)
{
	fprintf(stderr,"Insufficient parameters on command line.\n");
	return(USAGE_ONLY);
}

/******************************** MOTOR ********************************/

int motor(dataptr dz)
{
	int exit_status, warned = 0, bufcntr = 0, done = 0, event_cnt, ibufno, ch, chans = dz->infile->channels;
	float **ibuf, *ebuf, *obuf, *ovflwbuf;
	int *instep, *gp_sampsread, *mot_starts, *mot_ends, arraysize, in_upstep = 0, in_dnstep = 0, write_position, gp_edgsmps, pulsesmps, start = 0, end = 0, next;
	int n, m, j, k, cresc_cnt, decresc_cnt, max_cresccnt, inner_cnt, max_innercnt, outstep, gp_eventsamps, gp_tail, ibufpos, obufpos = 0;
	double time, srate = (double)dz->infile->srate;
	double *mot_frq, *mot_sym, *mot_frnd, *mot_jit, *mot_trem, *mot_symrnd, *mot_fratio, *mot_dur, *mot_edge, *mot_bite, *inseg;
	double frq, edge, sym, frnd, jit, trem, symrnd, fratio, pulsdur, bite = 1.0, edgelen, outer_dur, real_outer_dur, rnd, endtime, cresctime = 0.0, decresctime = 0.0;
	double inner_dur, thisdur, val, incr, mindur, segdur;
	int *permm, permcnt = dz->inbufcnt;
	int *samphold, start_read, samps_to_read, gp_splicelen, gp_samps_envup, gp_samps_envdn, zerocnt;
	int modetype = dz->mode % 3, advance_regress = 0, only_advance = 0;
	if(dz->mode < 3)
		advance_regress = 1;
	else if(dz->mode < 6)
		only_advance = 1;

	if((samphold = (int *)malloc(dz->inbufcnt * sizeof(int)))==NULL) {
		sprintf(errstr,"Insufficient memory to create store for source sizes.\n");
		return(MEMORY_ERROR);
	}
	if(modetype != MOT_SLICE) {												//	Arrays already created for MOT_SLICE in read_special_data
		if((dz->parray = (double **)malloc(10 * sizeof(double *)))==NULL) {
			sprintf(errstr,"Insufficient memory to create temporary varying parameter storage.\n");
			return(MEMORY_ERROR);
		}
	}
	//	PRELIMINARIES

	if((permm = (int *)malloc(dz->inbufcnt * sizeof(int)))==NULL) {			//	Establish array for permuting order of infiles
		sprintf(errstr,"Insufficient memory to create file-order permutation store.\n");
		return(MEMORY_ERROR);
	}

	//	Establish arrays to store brkpnt values for each outer-pulse in a buffer

	arraysize = (dz->buflen/dz->minpulsesmps) + 64;								//	Array must accomodate max number of inner-pulses with an outer_envelope-pulse								
	
	for(n=0;n < 10;n++) {
		if((dz->parray[n] = (double *)malloc(arraysize * sizeof(double)))==NULL) {
			sprintf(errstr,"Insufficient memory to temporary varying parameter storage %d.\n",n+1);
			return(MEMORY_ERROR);
		}
	}
	if((dz->lparray = (int **)malloc(3 * sizeof(int *)))==NULL) {
		sprintf(errstr,"Insufficient memory to create 'int' arrays.\n");
		return(MEMORY_ERROR);
	}
	if((dz->lparray[0] = (int *)malloc(arraysize * sizeof(int)))==NULL) {
		sprintf(errstr,"Insufficient memory to create pulse markers for events in buffer.\n");
		return(MEMORY_ERROR);
	}
	if((dz->lparray[1] = (int *)malloc(arraysize * sizeof(int)))==NULL) {
		sprintf(errstr,"Insufficient memory to create pulse markers for events in buffer.\n");
		return(MEMORY_ERROR);
	}

	//	Establish array to store sample-step between inner-events, in each infile
	
	if((dz->lparray[2] = (int *)malloc(dz->inbufcnt * sizeof(int)))==NULL) {
		sprintf(errstr,"Insufficient memory to create array to store stpes with infiles.\n");
		return(MEMORY_ERROR);
	}

	//	Establish array of input buffers

	if((ibuf = (float **)malloc(dz->inbufcnt * sizeof(float *)))==NULL) {
		sprintf(errstr,"Insufficient memory to create input sound buffers.\n");
		return(MEMORY_ERROR);
	}
	if((gp_sampsread = (int *)malloc(dz->inbufcnt * sizeof(int)))==NULL) {
		sprintf(errstr,"Insufficient memory to create input file pointers.\n");
		return(MEMORY_ERROR);
	}
	instep		= dz->lparray[2];	//	Sample-step (within each infile) to next inner-event, when "advance" flag not set
	mot_starts  = dz->lparray[0];		//	Start of each envelope-event
	mot_ends    = dz->lparray[1];		//	End of each envelope-event
	mot_sym		= dz->parray[0];		//	Parameters of the envelope
	mot_symrnd	= dz->parray[1];
	mot_bite	= dz->parray[2];
	mot_dur		= dz->parray[3];
	mot_frq		= dz->parray[4];		//	Parameters of inner-events within the envelope
	mot_frnd	= dz->parray[5];
	mot_jit		= dz->parray[6];
	mot_trem	= dz->parray[7];
	mot_fratio  = dz->parray[8];
	mot_edge	= dz->parray[9];

	for(n=0;n<dz->inbufcnt;n++)			//	Establish arrays to store input and output sound
		ibuf[n] = dz->sampbuf[n];
	ebuf     = dz->sampbuf[n++];
	obuf     = dz->sampbuf[n++];
	ovflwbuf = dz->sampbuf[n];

	if(modetype == MOT_SLICE) {			//	Read single source at different points, and store segments in buffers, splicing starts and ends
		gp_splicelen = (int)round(MOT_SPLICE * MS_TO_SECS * srate);
		inseg = dz->parray[10];			//	noting the length of each segment in gp_samples
		for(n=0,m=0;n < dz->inbufcnt;n++,m+=2) {
			start_read = (int)round(inseg[m] * srate) * chans;
			if(n == dz->itemcnt - 1)
				samps_to_read = dz->insams[0] - start_read;
			else {
				segdur = inseg[m+1] - inseg[m];
				samps_to_read = (int)round(segdur * srate) * chans;
			}
			sndseekEx(dz->ifd[0],start_read,0);
			if((dz->ssampsread = fgetfbufEx(ibuf[n],samps_to_read,dz->ifd[0],0)) < 0) {
				sprintf(errstr,"Can't read sample-set %d from input soundfile.\n",n+1);
				return(SYSTEM_ERROR);
			}
			if(dz->ssampsread != samps_to_read) {
				fprintf(stdout,"WARNING: Samps read (%d) not exactly as asked for (%d) for input seg %d\n",dz->ssampsread,samps_to_read,n+1);
				fflush(stdout);
			}
			samphold[n]     = dz->ssampsread;
			gp_sampsread[n] = dz->ssampsread/chans;
			if(n>0) {
				ibufpos = 0;
				for(k=0;k<gp_splicelen;k++) {
					val = (double)k/(double)gp_splicelen;
					for(ch=0;ch<chans;ch++) {
						ibuf[n][ibufpos] = (float)(ibuf[n][ibufpos] * val);
						ibufpos++;
					}
				}
			}
			if(n<dz->inbufcnt-1) {
				for(k=0,j=gp_sampsread[n] - 1;k<gp_splicelen;k++,j--) {
					ibufpos = j * chans;
					val = (double)k/(double)gp_splicelen;
					for(ch=0;ch<chans;ch++) {
						ibuf[n][ibufpos] = (float)(ibuf[n][ibufpos] * val);
						ibufpos++;
					}
				}
			}
		}
	} else {							//	OR Read (all) input file(s) and note their length(s) in gp_samples
		for(n=0;n<dz->inbufcnt;n++) {
			if((dz->ssampsread = fgetfbufEx(ibuf[n], dz->buflen,dz->ifd[n],0)) < 0) {
				sprintf(errstr,"Can't read samples from input soundfile %d.\n",n+1);
				return(SYSTEM_ERROR);
			}
			samphold[n]		= dz->ssampsread;
			gp_sampsread[n] = dz->ssampsread/chans;
		}
	}
	time = 0.0;
	write_position = 0;

	//	In fixed-step mode we advance by a fixed step in the input file.
	//	Need to know the maximum number of steps within the crescendo part of any envelope-pulse, so.....

	dz->symwarning = 0;

	if(dz->vflag[MOT_FXDSTP]) {
		srand(dz->iparam[MOT_SEED]);						//	Initialise randomisation
		rndpermm(permcnt,permm);							//	and do initial permutation
		if((exit_status = calculate_max_read_events_in_any_env_pulse(&max_cresccnt,&max_innercnt,&frq,&edge,arraysize,permm,permcnt,gp_sampsread,dz))<0)
			return exit_status;
		inner_dur = 1/frq;									//	Duration of final inner-event
		edgelen = inner_dur * edge;							//	Tail on final event
		gp_edgsmps = (int)ceil(edgelen * srate);			//	Allowing for possible tail on last sample-read
		for(n=0;n<dz->inbufcnt;n++)	{						//	Divide each input file into max possible number of steps to find the instep for each src

			if(advance_regress)							//	In these modes, whole src must fit under crescendo
				instep[n] = (int)floor(((samphold[n]/chans) - gp_edgsmps)/max_cresccnt) * chans;
			else											//	In these modes, whole src must fit under crescendo+decrescendo
				instep[n] = (int)floor(((samphold[n]/chans) - gp_edgsmps)/max_innercnt) * chans;
		}
	}
	srand(dz->iparam[MOT_SEED]);							//	(re)Initialise randomisation
	rndpermm(permcnt,permm);								//	and (re)do initial permutation

	while(time < dz->param[MOT_DUR]) {						//	Until we have generated the required output duration
		event_cnt = 0;

		//	LOCATE POSITION AND END-TIMES OF ALL LARGE-PULSES FITTING WITHIN CURRENT BUFFER

		while(write_position < dz->buflen) {				//	Find all large-pulses FITTING IN A BUFFER
			if(event_cnt >= arraysize) {
				sprintf(errstr,"Array overrun storing pulse start and end times in buffer.\n");
				return PROGRAM_ERROR;
			}												//	Store sample-time of start of large-pulse .....
			mot_starts[event_cnt] = (int)round(time * srate) * chans;
			mot_starts[event_cnt] -= dz->total_samps_written;//	.....within the current buffer
			
			//	Find parameters for internal-events inside each of envelope-pulse
			
			if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
				return exit_status;
			mot_frq[event_cnt]    = dz->param[MOT_FRQ];
			mot_sym[event_cnt]    = dz->param[MOT_SYM];
			mot_frnd[event_cnt]   = dz->param[MOT_FRND];
			mot_jit[event_cnt]    = dz->param[MOT_JIT];
			mot_trem[event_cnt]   = dz->param[MOT_TREM];
			mot_symrnd[event_cnt] = dz->param[MOT_SYMRND];
			mot_fratio[event_cnt] = dz->param[MOT_FRATIO];
			mot_edge[event_cnt]   = dz->param[MOT_EDGE];
			mot_bite[event_cnt]   = dz->param[MOT_BITE];

			mindur = 1.0/dz->param[MOT_FRQ];	//	Duration of inner-events
			mindur += 0.001;					//	Correct for rounding errors
			mindur *= 2;						//	Two inner pulses is minimum to fit in 1 outer-pulse

			//	Find step between envelope-pulses, and actual sounding-end of envelope-pulse

			outer_dur = dz->param[MOT_PULSE];					//	Find actual duration of large-pulse, modifying it, if randomised
			if(dz->param[MOT_PRND] > 0.0) {			
				rnd = (drand48() * 2.0) - 1.0;					//	Range -1 to 1
				rnd *= dz->param[MOT_PRND];						//	Range -r to + r
				rnd += 1.0;										//	Range 1-r to 1+r
				outer_dur *= rnd;
				outer_dur = max(outer_dur,2 * mindur);			//	Outer-Pulse cannot be shorter than 2 inner_pulses
			}
			real_outer_dur = outer_dur * dz->param[MOT_PRATIO];	//	If pulse does NOT sound for all its duration, find actual end of sound-write
			real_outer_dur = max(real_outer_dur,mindur);		//	Outer-Pulse cannot be shorter than 2 inner_pulses
			mot_dur[event_cnt] = real_outer_dur;				//	and store it
			endtime = time + real_outer_dur;					//	Store sample-time of end of large-pulse-write ....
			mot_ends[event_cnt] = (int)round(endtime * srate) * chans;
			mot_ends[event_cnt] -= dz->total_samps_written;		//	....within the current buffer
			pulsesmps = (int)round(outer_dur * srate) * chans;

			event_cnt++;										//	Advance event-counter to next event				
			write_position += pulsesmps;						//	Advance write-position in output buffer.
			time += outer_dur;									//	Advance time for next brktable-read		
			if(time >= dz->param[MOT_DUR]) {					//	If this large-pulse event runs over required total-duration-of-output,
				done = 1;										//	flag to quit, once inner-events have been generated
				break;											//	and break (so no further envel-pulses generated).
			}
		}
			//	NOW GENERATE THE INNER EVENTS INSIDE THE LARGE-PULSES
		
		for(n = 0; n <event_cnt; n++) {

			//	Get params needed for for inner-event within each outer-pulse			
			
			start = mot_starts[n];
			end	  = mot_ends[n];
			if(n < event_cnt - 1)
				next = mot_starts[n+1];						//	Note start of next pulse
			else											//	(if there is one)
				next = -1;
			frq     = mot_frq[n];
			sym     = mot_sym[n];
			frnd    = mot_frnd[n];
			jit     = mot_jit[n];
			trem    = mot_trem[n];
			symrnd  = mot_symrnd[n];
			fratio  = mot_fratio[n];
			pulsdur = mot_dur[n];
			edge    = mot_edge[n];
			bite    = mot_bite[n];							
			
			//	Using outer-event duration and symmetry, find true durations of cresc and decresc in envelope
			//	find counts of inner events in cresc and decresc portions of envelope
			
			if((exit_status = calculate_cresc_and_decresc_counts_of_inner_events(&cresc_cnt,&decresc_cnt,&cresctime,&decresctime,sym,symrnd,pulsdur,frq,dz))<0)
				return exit_status;
			
			inner_cnt = cresc_cnt + decresc_cnt;
			
			//	Select appropriate infile to read

			ibufno = select_infile_to_use(&bufcntr,permm,permcnt,dz);

			//	Get (average) separation-time of inner-events, in the output

			inner_dur = 1/frq;
			outstep = (int)round(inner_dur * srate) * chans;

			//	Calculate read-steps in infile				

			calculate_fwd_and_bkwd_sampsteps_in_infile(&in_upstep,&in_dnstep,srate,chans,gp_sampsread[ibufno],cresc_cnt,decresc_cnt,inner_dur,edge,ibufno,dz);

			//	Calculate number of input samples to read, and length of inner-event tail

			calculate_inputsamps_to_read_and_length_of_tail(&gp_eventsamps,&gp_tail,srate,inner_dur,fratio,edge,dz);

			//	Set start points in input and output buffers

			obufpos = start;

			if(jit != 0.0) {									//	Check for pitch-jitter		
				rnd = (drand48() * 2.0) - 1.0;					//	Range -1 to 1
				jit *= rnd;										//	Semitones up or down		
				jit = pow(2.0,jit/SEMITONES_PER_OCTAVE);		//	Frq-ratio up or down = incr in reading data
				incr = jit;
			} else
				incr = 1.0;

			if(advance_regress) {	//	In these modes Read advances then regresses

				ibufpos = 0;

				//	GENERATE ALL THE EVENTS IN THE CRESC PART OF ENVELOPE
				
				for(m = 0; m < cresc_cnt;m++) {
					if((exit_status = generate_inner_pulse(ebuf,ibuf[ibufno],obuf,ibufpos,obufpos,gp_eventsamps,gp_tail,incr,trem,1,dz))<0)
						return exit_status;
					if(frnd > 0.0) {								//	If inner-event timings are randomised, Recalculate step to next event
						rnd = ((drand48() * 2.0) - 1.0)/2.0;		//	Range -1/2 to 1/2
						rnd *= frnd;								//	Range -frnd(min -1/2) to frnd (max 1/2)
						thisdur = inner_dur * (1.0 + rnd);			//	Step-time increased or decreaserd by max of +- 1/2
						outstep = (int)round(thisdur * srate) * chans;
					}
					obufpos += outstep;
					ibufpos += in_upstep;
				}
				//	GENERATE ALL EVENTS IN THE DECRESC PART OF ENVELOPE
				
				for(m = 0; m < decresc_cnt;m++) {
					if((ibufpos -= in_dnstep) < 0) {
						ibufpos = 0;
						if(!warned) {
							fprintf(stdout,"WARNING: Decrescendoing part of an event overran foot of buffer.\n");
							fflush(stdout);
							warned = 1;
						}
					}
					if((exit_status = generate_inner_pulse(ebuf,ibuf[ibufno],obuf,ibufpos,obufpos,gp_eventsamps,gp_tail,incr,trem,1,dz))<0)
						return exit_status;
					if(frnd > 0.0) {
						rnd = ((drand48() * 2.0) - 1.0)/2.0;
						rnd *= frnd;
						thisdur = inner_dur * (1.0 + rnd);
						outstep = (int)round(thisdur * srate) * chans;
					}
					obufpos += outstep;
				}

			} else {	//	In these modes Read only advances, or only regresses

				if(only_advance)		//	Advances from start
					ibufpos = 0;
				else {					//	Advances from start OR Regresses from end at random
					rnd = drand48();
					if(rnd < 0.5)
						ibufpos = 0;			//	Advance
					else {
						ibufpos = (in_upstep) * (inner_cnt - 1);
						in_upstep = -in_upstep;	//	Regress
					}
				}
				for(m = 0; m < inner_cnt;m++) {
					if((exit_status = generate_inner_pulse(ebuf,ibuf[ibufno],obuf,ibufpos,obufpos,gp_eventsamps,gp_tail,incr,trem,1,dz))<0)
						return exit_status;
					if(frnd > 0.0) {								//	If inner-event timings are randomised, Recalculate step to next event
						rnd = ((drand48() * 2.0) - 1.0)/2.0;		//	Range -1/2 to 1/2
						rnd *= frnd;								//	Range -frnd(min -1/2) to frnd (max 1/2)
						thisdur = inner_dur * (1.0 + rnd);			//	Step-time increased or decreaserd by max of +- 1/2
						outstep = (int)round(thisdur * srate) * chans;
					}
					obufpos += outstep;
					ibufpos += in_upstep;
				}
			}

			//	THEN DO ENVELOPE OVER CRESC+DECRESC EVENTS

			gp_samps_envup = (int)round(cresctime * srate);	//	Number of sample-groups in the cresc and decresc
			gp_samps_envdn = (int)round(decresctime * srate);
																//	Adjust for rounding errors
			while(start + ((gp_samps_envup + gp_samps_envdn) * chans) > end)
				gp_samps_envdn--;
			obufpos = start;									//	Returning to start of the outer-event		
			for(m = 0; m < gp_samps_envup; m++) {
				val = (double)m/(double)gp_samps_envup;
				val = pow(val,bite);							//	Impose "bite" curvature on outer-nvelope
				for(ch=0;ch<chans;ch++) {
					obuf[obufpos] = (float)(obuf[obufpos] * val);
					obufpos++;
				}
			}
			for(m = gp_samps_envdn - 1; m >= 0; m--) {
				val = (double)m/(double)gp_samps_envdn;
				val = pow(val,bite);
				for(ch=0;ch<chans;ch++) {
					obuf[obufpos] = (float)(obuf[obufpos] * val);
					obufpos++;
				}
			}
			zerocnt = (dz->buflen * 2) - obufpos;				//	zero the rest of the outbuffer
			if(zerocnt > 0)										//	in case any writes to it have not fallen under envelope,
				memset((char *)(obuf+obufpos),0,zerocnt * sizeof(float));
		}
		if(done)												//	If already got total duration, quit
			break;
		if((exit_status = write_samps(obuf,dz->buflen,dz))<0)	//	otherwise ...
			return exit_status;									//	Write buffer-full of sound
		memcpy((char *)obuf,(char *)ovflwbuf,dz->buflen * sizeof(float));
		memset((char *)ovflwbuf,0,dz->buflen * sizeof(float));	//	.... and copy back any overflow
		write_position -= dz->buflen;							//	Reset position of write in buffer.
	}
	if((exit_status = write_samps(obuf,obufpos,dz))<0)			//	Write any remaining samples
		return exit_status;
	return FINISHED;
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

/******************************** CREATE_MOTOR_SNDBUFS ********************************/

int create_motor_sndbufs(dataptr dz)
{
	int n, chans = dz->infile->channels;
	int bigbufsize, secsize, framesize = F_SECSIZE * chans;
	double max_transpos;

	dz->buflen = max(dz->gp_maxinsmps,dz->gp_maxpulsesmps);
	max_transpos = pow(2.0,dz->application->hi[MOT_JIT]/SEMITONES_PER_OCTAVE);	//	Allow for maximal transposition of output
	dz->buflen = (int)ceil((double)dz->buflen * max_transpos);
	dz->buflen = (int)ceil((double)dz->buflen * 1.5);							//	Allow for maximal warp of inner-pulse-lengths
	dz->buflen *= chans;
	secsize = dz->buflen/framesize;
	if(secsize * framesize != dz->buflen)
		secsize++;
	dz->buflen = secsize * framesize;
	bigbufsize = (dz->buflen * dz->bufcnt) * sizeof(float);
	if((dz->bigbuf = (float *)malloc(bigbufsize)) == NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create total sound buffers.\n");
		return(PROGRAM_ERROR);
	}
	dz->sbufptr[0] = dz->sampbuf[0] = dz->bigbuf;							//	1 Inbuf for each infile
	for(n=1;n < dz->bufcnt;n++)												//	1 untransposed inner-pulse buf
		dz->sbufptr[n] = dz->sampbuf[n] = dz->sampbuf[n-1] + dz->buflen;	//	1 motor-pulses output buf
	dz->sampbuf[n] = dz->sampbuf[n-1] + dz->buflen;							//	1 motor-pulses overflow
	return(FINISHED);
}

/*************************** RNDPERMM ********************************/

void rndpermm(int permlen,int *permm)
{
	int n, t;
	for(n=0;n<permlen;n++) {		/* 1 */
		t = (int)(drand48() * (double)(n+1));		/* 2 */
		if(t==n)
			prefix(n,permlen,permm);
		else
			insert(n,t,permlen,permm);
	}
}

/***************************** INSERT **********************************
 *
 * Insert the value m AFTER the T-th element in permm[pindex].
 */

void insert(int m,int t,int permlen,int *permm)
{   
	shuflup(t+1,permlen,permm);
	permm[t+1] = m;
}

/***************************** PREFIX ************************************
 *
 * Insert the value m at start of the permutation permm[pindex].
 */

void prefix(int m,int permlen,int *permm)
{
	shuflup(0,permlen,permm);
	permm[0] = m;
}

/****************************** SHUFLUP ***********************************
 *
 * move set members in permm[pindex] upwards, starting from element k.
 */

void shuflup(int k,int permlen, int *permm)
{
	int n, *i;
	int z = permlen - 1;
	i = permm + z;
	for(n = z;n > k;n--) {
		*i = *(i-1);
		i--;
	}
}

/****************************** GENERATE_INNER_PULSE ***********************************/

int generate_inner_pulse(float *ebuf,float *ibuf,float *obuf,int ibufpos,int obufpos,int gp_eventsamps,int gp_tail,double incr,double trem,int output,dataptr dz)
{
	int chans = dz->infile->channels, splen, ch;
	int k, kk, m, mm, thispos, nextpos, gp_fadelen;
	double splic, rnd, amp, debufpos, frac, val, diff, srate = (double)dz->infile->srate;
	int gp_splicelen = (int)round((MOT_SPLICE * MS_TO_SECS) * srate);

	if(output) {
		memset((char *)ebuf,0,dz->buflen * sizeof(float));	//	Copy total event samples from ibuf
		memcpy((char *)ebuf,(char *)(ibuf + ibufpos),gp_eventsamps * chans * sizeof(float));

		splen = min(gp_splicelen,gp_eventsamps/2);			//	Find appropriate splicelen				
		for(k = 0; k < splen;k++) {							//	Do on-splice
			kk = k * chans;
			splic = (double)k/(double)splen;
			for(ch = 0;ch < chans;ch++)
				ebuf[kk + ch] = (float)(ebuf[kk + ch] * splic);
		}
		if(gp_tail == 0) {									//	If No tail
			for(k = 0, m = gp_eventsamps - 1; k < splen;k++,m--) {
				mm = m * chans;								//	Do off-splice
				splic = (double)k/(double)splen;
				for(ch = 0;ch < chans;ch++)
					ebuf[mm + ch] = (float)(ebuf[mm + ch] * splic);
			}
		} else {											//	Else, fade through splicelen and tail
			gp_fadelen = splen + gp_tail;
			for(k = 0, m = gp_eventsamps - 1; k < gp_fadelen;k++,m--) {
				mm = m * chans;
				splic = (double)k/(double)gp_fadelen;
				splic = pow(splic,MOT_EXPDECAY);			//	Exponential decay
				for(ch = 0;ch < chans;ch++)
					ebuf[mm + ch] = (float)(ebuf[mm + ch] * splic);
			}
		}
	}

	//	NOW ADD THE EVENT TO THE OUTPUT, VARYING PITCH AND AMPLITUDE IF NESS

	if(trem > 0.0) {									//	If amplitude varies
		rnd = drand48();								//	Range 0 to 1
		rnd *= trem;									//	Range 0 to trem
		amp = 1.0 - rnd;								//	Amp = 1 minus rnd-variation
	} else
		amp = 1.0;
	if(output) {
		debufpos = 0.0;
		while(debufpos < gp_eventsamps) {					//	Read input using increment (for possible pitch-shift)
			thispos = (int)floor(debufpos);
			frac = debufpos - (double)thispos;
			thispos *= chans;
			nextpos = thispos + chans;
			for(ch = 0;ch < chans; ch++) {
				val  = ebuf[thispos++];
				diff = ebuf[nextpos++] - val;
				val += diff * frac;							
				val *= amp;									//	Do any required amplitude atenuation
				obuf[obufpos] = (float)(obuf[obufpos] + val);//	Add sample into output (in case there are event overlaps)
				obufpos++;
			}
			debufpos += incr;
			if(obufpos >= dz->buflen * 2) {
				sprintf(errstr,"Error in output buffer logic. May be due to jitter.\n");
				return PROGRAM_ERROR;
			}
		}
	}
	return FINISHED;
}

/********************************** CALCULATE_CRESC_AND_DECRESC_COUNTS_OF_INNER_EVENTS ************************************/

int calculate_cresc_and_decresc_counts_of_inner_events(int *cresc_cnt,int *decresc_cnt,double *cresctime,double *decresctime,
													   double sym,double symrnd,double pulsdur,double frq,dataptr dz)
{
	double rnd, offset, adjust;
	int total;
	int bum = 0;
	if(symrnd > 0.0) {									//	If symmetry randomised
		rnd = (drand48() * 2.0) - 1.0;					//	Rand value in range -1 to 1
		rnd *= symrnd;									//	Rand value in range -symrand to symrand
		if(rnd < 0.0) {
			rnd = (-rnd) * sym;							//	Push backward the symmetry position by a random amount
			sym -= rnd;
		} else {										//	OR Push forward the symmetry position by a random amount
			rnd *= (1.0 - sym);
			sym += rnd;
		}
	}
	*cresctime   = pulsdur * sym;						//	use length and symmetry to work out how many events needed in cresc and decresc
	*decresctime = pulsdur - *cresctime;
	*cresc_cnt   = (int)floor(*cresctime * frq);
	*decresc_cnt = (int)floor(*decresctime * frq);
	total = *cresc_cnt + *decresc_cnt;
	if(total < 2) {
		if(!dz->symwarning) {
			fprintf(stdout,"WARNING: Insufficient inner-events underneath envelope : adjusting symmetry.\n");
			fflush(stdout);
			dz->symwarning = 1;
		}
		offset = sym - 0.5;								//	distance from midpoint to sym-peak
		adjust = -offset;								//	step back to midpoint
		adjust /= 10.0;									//	1/10 of this distance
		while(total < 2) {
			sym += adjust;								//	Move symmetry towards midpoint, in small steps	
			if(adjust > 0.0 && sym > 0.5)				//	recalculating cresctime & decresctime, to get values that work 
				bum = 1;
			else if(adjust < 0.0 && sym < 0.5)			//	But if no values work ... fail!!!!
				bum = 1;
			if(bum) {
				sprintf(errstr,"Insufficient inner-events underneath envelope. sym = %lf pulsdur = %lf\n",sym,pulsdur);
				return PROGRAM_ERROR;
			}
			*cresctime   = pulsdur * sym;
			*decresctime = pulsdur - *cresctime;
			*cresc_cnt   = (int)floor(*cresctime * frq);
			*decresc_cnt = (int)floor(*decresctime * frq);
			total = *cresc_cnt + *decresc_cnt;
		}
	}
	if(*cresc_cnt == 0) {
		(*cresc_cnt)++;
		(*decresc_cnt)--;
		*cresctime   = *cresc_cnt/frq;
		*decresctime = *decresc_cnt/frq;
	} else if (*decresc_cnt == 0) {
		(*decresc_cnt)++;
		(*cresc_cnt)--;
		*cresctime   = *cresc_cnt/frq;
		*decresctime = *decresc_cnt/frq;
	} 
	return FINISHED;
}

/********************************** SELECT_INFILE_TO_USE ************************************/

int	select_infile_to_use(int *bufcntr,int *permm,int permcnt,dataptr dz)
{		
	int ibufno = 0;
	int modetype = dz->mode % 3;
	if(modetype != MOT_SNGLE) {						//	Find appropriate inbuf, if multiple input files, or many cut segments
		if(dz->vflag[MOT_CYCLIC])
			ibufno = *bufcntr;						//	either next one cyclically	
		else
			ibufno = permm[*bufcntr];				//	or next one in perm
		if(++(*bufcntr) >= dz->inbufcnt) {			//	and advance infile counter "bufcntr"
			if(!dz->vflag[MOT_CYCLIC])
				rndpermm(permcnt,permm);
			*bufcntr = 0;
		}
	}
	return ibufno;
}

/********************************** CALCULATE_FWD_AND_BKWD_SAMPSTEPS_IN_INFILE ************************************/

void calculate_fwd_and_bkwd_sampsteps_in_infile(int *in_upstep,int *in_dnstep,double srate,int chans,int gp_sampsread,
												int cresc_cnt,int decresc_cnt,double inner_dur,double edge,int ibufno,dataptr dz)
{			
	double edgelen, upsteptime, advance, dnsteptime, rnd;
	int gp_edgsmps, smp_advance, smp_regress;
	int *instep = dz->lparray[2];
	int advance_regress = 0;
	if(dz->mode < 3)
		advance_regress = 1;

	if(dz->vflag[MOT_FXDSTP]) {						//	Envelope inner-events always advance by a fixed amount	
													//	From known pre-calculated timestep between reads, for this infile	
		*in_upstep = instep[ibufno];				//	Fixed step calculated so this is same in all modes (for modes > 3 in_dnstep not used)
		if(advance_regress) {
			upsteptime = (double)((*in_upstep)/chans)/srate;
			advance    = upsteptime * cresc_cnt;		//	End time in infile after all reads
			dnsteptime = advance/(double)decresc_cnt;	//	Length of time-steps back to start of file, in decresendo-steps
			*in_dnstep = (int)round(dnsteptime * srate) * chans;
		}

	} else {										//	Envelope inner-events always advance to end of infile data

		edgelen = inner_dur * edge;					//	how many infile steps to advance to end, and to return to start
		gp_edgsmps = (int)ceil(edgelen * srate);	//	Allow for overlay of last sample due to its tail

		if(dz->param[MOT_VARY] > 0.0) {				//	If advance in src randomly varies, vary apparent length of source
			rnd = drand48() * dz->param[MOT_VARY];	//	Range 0 to mot_vary
			rnd	= 1.0 - rnd;						//	Range 1 to (1-mot_vary);
			gp_sampsread = (int)round((double)gp_sampsread * rnd);
		}
		if(advance_regress) {
			*in_upstep = (int)floor((gp_sampsread - gp_edgsmps)/cresc_cnt) * chans;
			*in_dnstep = (int)floor((gp_sampsread - gp_edgsmps)/decresc_cnt) * chans;
		} else
			*in_upstep = (int)floor((gp_sampsread - gp_edgsmps)/(cresc_cnt + decresc_cnt)) * chans;
	}

	smp_advance = (*in_upstep) * cresc_cnt;
	smp_regress = (*in_dnstep) * decresc_cnt;
	while(smp_advance - smp_regress < 0) {			//	After stepping up then down, should be back at file start
		*in_dnstep -= chans;						//	IF overshoot zero, reduce down-step by 1 gp_sample at a time, until fixed
		smp_regress= *in_dnstep * decresc_cnt;
	}
}

/********************************** CALCULATE_INPUTSAMPS_TO_READ_AND_LENGTH_OF_TAIL ************************************/

void calculate_inputsamps_to_read_and_length_of_tail(int *gp_eventsamps,int *gp_tail,double srate,double inner_dur,double fratio,double edge,dataptr dz)
{
	double cliplen, eventdur;
	int gp_clipsamps;
	cliplen = inner_dur * fratio;					//	Actual (main) sounding part of inner-event
	eventdur = cliplen + (cliplen * edge);			//	Actual time needed in order to have required tail
	gp_clipsamps = (int)round(cliplen * srate);	//	Sample length of inner-event (without tail)
	*gp_eventsamps= (int)round(eventdur * srate);	//	Sample length of inner-event WITH tail
	*gp_tail = *gp_eventsamps - gp_clipsamps;		//	Length of event tail
}

/*********************************	CALCULATE_MAX_READ_EVENTS_IN_ANY_ENV_PULSE ****************************************/

int calculate_max_read_events_in_any_env_pulse(int *max_cresccnt,int *max_innercnt,double *frq,double *edge,int arraysize,int *permm,int permcnt,int *gp_sampsread,dataptr dz)
{
	int exit_status, chans = dz->infile->channels, done = 0, ibufno, bufcntr;
	int n, event_cnt, write_position, cresc_cnt = 0, decresc_cnt = 0, pulsesmps;
	double time, cresctime = 0.0, decresctime = 0.0, srate = (double)dz->infile->srate;
	double *mot_sym = dz->parray[0], *mot_symrnd = dz->parray[1], *mot_dur = dz->parray[3], *mot_frnd = dz->parray[5];
	double *mot_jit	= dz->parray[6], *mot_trem = dz->parray[7]; 
	double outer_dur, rnd, real_outer_dur, sym, symrnd, frnd, jit, trem;
	float *ebuf, *obuf;
	ebuf = dz->sampbuf[dz->inbufcnt];
	obuf = dz->sampbuf[dz->inbufcnt+1];
	time = 0.0;
	write_position = 0;
	while(time < dz->param[MOT_DUR]) {						//	Until we have generated the required output duration
		event_cnt = 0;

		//	LOCATE POSITION AND END-TIMES OF ALL LARGE-PULSES FITTING WITHIN CURRENT BUFFER

		while(write_position < dz->buflen) {				//	Find all large-pulses FITTING IN A BUFFER

			if(event_cnt >= arraysize) {
				sprintf(errstr,"Array overrun storing pulse start and end times in buffer.\n");
				return PROGRAM_ERROR;
			}												//	Store sample-time of start of large-pulse .....
	
			//	Find parameters for internal-events inside each of envelope-pulse
			
			if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
				return exit_status;
			mot_sym[event_cnt]    = dz->param[MOT_SYM];
			mot_symrnd[event_cnt] = dz->param[MOT_SYMRND];
			*frq  = dz->param[MOT_FRQ];
			*edge = dz->param[MOT_EDGE];

			//	Find step between envelope-pulses, and actual sounding-end of envelope-pulse

			outer_dur = dz->param[MOT_PULSE];				//	Find actual duration of large-pulse, modifying it, if randomised
			if(dz->param[MOT_PRND] > 0.0) {			
				rnd = (drand48() * 2.0) - 1.0;				//	Range -1 to 1
				rnd *= dz->param[MOT_PRND];					//	Range -r to + r
				rnd += 1.0;									//	Range 1-r to 1+r
				outer_dur *= rnd;
				outer_dur = max(outer_dur,2.0/dz->param[MOT_FRQ]);	//	Outer-Pulse cannot be shorter than 2 inner_pulses
			}
			real_outer_dur = outer_dur * dz->param[MOT_PRATIO];//	If pulse does NOT sound for all its duration, find actual end of sound-write
			real_outer_dur = max(outer_dur,2.0/dz->param[MOT_FRQ]);	//	Outer-Pulse cannot be shorter than 2 inner_pulses
			mot_dur[event_cnt] = real_outer_dur;			//	and store it
			pulsesmps = (int)round(outer_dur * srate) * chans;
			event_cnt++;									//	Advance event-counter to next event				
			write_position += pulsesmps;					//	Advance write-position in output buffer.
			time += outer_dur;								//	Advance time for next brktable-read		
			if(time >= dz->param[MOT_DUR]) {				//	If this large-pulse event runs over required total-duration-of-output,
				done = 1;									//	flag to quit, once inner-events have been generated
				break;										//	and break (so no further envel-pulses generated).
			}
		}
			//	NOW PSEUDO-GENERATE THE INNER EVENTS INSIDE THE LARGE-PULSES
		
		for(n = 0; n <event_cnt; n++) {

			//	Get params needed for for inner-event within each outer-pulse			
			
			sym       = mot_sym[n];
			symrnd    = mot_symrnd[n];
			outer_dur = mot_dur[n];
			frnd      = mot_frnd[n];
			jit       = mot_jit[n];
			trem      = mot_trem[n];
			
			//	Using outer-event duration and symmetry, find true durations of cresc and decresc in envelope
			//	find counts of inner events in cresc and decresc portions of envelope
			
			if((exit_status = calculate_cresc_and_decresc_counts_of_inner_events(&cresc_cnt,&decresc_cnt,&cresctime,&decresctime,sym,symrnd,outer_dur,*frq,dz))<0)
				return exit_status;
			*max_innercnt = max(*max_innercnt,cresc_cnt + decresc_cnt); 
			*max_cresccnt = max(*max_cresccnt,cresc_cnt);
			
			//	Select appropriate infile to read MAY USE RAND

			ibufno = select_infile_to_use(&bufcntr,permm,permcnt,dz);
			
			//	USE ANY RAND FUNCTIONS THAT MAY BE USED LATER, TO KEEP RAND OUTPUT REPRODUCIBLE

			if(dz->param[MOT_VARY] > 0.0)
				rnd = drand48();
			if(jit != 0.0)
				rnd = drand48();

			if(dz->mode >= 6)
				rnd = drand48();

			//	USE ANY RAND FUNCTIONS THAT MAY BE USED IN GENERATING THE EVENTS IN THE CRESC PART OF ENVELOPE
			for(n = 0; n < cresc_cnt;n++) {
				if(trem > 0.0)
					rnd = drand48();
				if(frnd > 0.0)
					rnd = drand48();
			}
			//	USE ANY RAND FUNCTIONS THAT MAY BE USED IN GENERATING THE EVENTS IN THE DECRESC PART OF ENVELOPE
			
			for(n = 0; n < decresc_cnt;n++) {
				if(trem > 0.0)
					rnd = drand48();
				if(frnd > 0.0)
					rnd = drand48();
			}
		}
		write_position -= dz->buflen;						//	Reset position of write in buffer.
	}
	return FINISHED;
}

/**************************** HANDLE_THE_SPECIAL_DATA ****************************/

int handle_the_special_data(char *str,int *max_gp_seg,dataptr dz)
{
	int done = 0, outcnt, n, m;
	double dummy = 0.0, lasttime, maxsegdur, srate = (double)dz->infile->srate;
	double splicedur = MOT_SPLICE * MS_TO_SECS;
	double dblsplicedur = splicedur * 2;
	double overlap = (MOT_DOVE + MOT_SPLICE) * MS_TO_SECS;
	FILE *fp;
	int cnt = 0, linecnt;
	char temp[800], *p;

	if((fp = fopen(str,"r"))==NULL) {
		sprintf(errstr,"Cannot open file %s to read times.\n",str);
		return(DATA_ERROR);
	}
	linecnt = 0;
	lasttime = -1.0;
	while(fgets(temp,200,fp)!=NULL) {
		p = temp;
		while(isspace(*p))
			p++;
		if(*p == ';' || *p == ENDOFSTR)	//	Allow comments in file
			continue;
		while(get_float_from_within_string(&p,&dummy)) {
			if(cnt == 0) {
				if(dummy <= dblsplicedur) {
					sprintf(errstr,"Invalid time (%lf) (closer to start than 2 splicedurs = %.3lf) at line %d in file %s.\n",dummy,dblsplicedur,linecnt+1,str);
					return(DATA_ERROR);
				}
			} else if(dummy <= lasttime + dblsplicedur) {
				sprintf(errstr,"Times (%lf & %lf) not increasing by 2 splicedurs (%.3lf) line %d in file %s.\n",lasttime, dummy,dblsplicedur,linecnt,str);
				return(DATA_ERROR);
			} else if(dummy >= dz->duration - dblsplicedur) {
				fprintf(stdout,"WARNING: Time (%lf) too near or beyond end of source-file, at line %d in file %s.\n",dummy,linecnt+1,str);
				fprintf(stdout,"WARNING: Ignoring data at and after this time.\n");
				fflush(stdout);
				done = 1;
				break;
			}
			lasttime = dummy;
			cnt++;
		}
		if(done)
			break;
		linecnt++;
	}
	if(cnt == 0) {
		sprintf(errstr,"No valid data found in file %s.\n",str);
		return(DATA_ERROR);
	}
	dz->itemcnt = cnt;
	outcnt = (dz->itemcnt + 1) * 2;		//	Slice times expanded into edit-time-pairs in source
	if((dz->parray = (double **)malloc(11 * sizeof(double *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create time-data storage. (1)\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[10] = (double *)malloc(outcnt * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create time-data storage. (2)\n");
		return(MEMORY_ERROR);
	}
	fseek(fp,0,0);
	cnt = 0;
	done = 0;
	while(fgets(temp,200,fp)!=NULL) {
		p = temp;
		while(isspace(*p))
			p++;
		if(*p == ';' || *p == ENDOFSTR)	//	Allow comments in file
			continue;
		while(get_float_from_within_string(&p,&dummy)) {
			dz->parray[10][cnt] = dummy;
			if(++cnt >= dz->itemcnt) {
				done = 1;
				break;
			}
		}
		if(done)
			break;
	}
	dz->parray[10][outcnt-1] = dz->duration;
	for(n=outcnt-2,m = dz->itemcnt-1; m >= 0; n-=2,m--) {
		dz->parray[10][n]   = dz->parray[10][m] - overlap;
		dz->parray[10][n-1] = dz->parray[10][m] + overlap;
	}
	dz->parray[10][n]   = 0.0;

	//	orig storage	0	1	2	3
	//	prog vals		A	B	C	D	

	//	final storage	0	1	2	3	4	5	6	7	8	9
	//	final vals		0	A+	-A	B+	B-	C+	C-	D+	D-	dur

	dz->itemcnt = outcnt/2;	//	dz->itemcnt is now the number of cut segments
	maxsegdur = 0.0;
	for(n=0,m=0;n < dz->itemcnt;n++,m+=2)
		maxsegdur = max(maxsegdur,dz->parray[10][m+1] - dz->parray[10][m]);
	*max_gp_seg = (int)ceil(maxsegdur * srate);
	fclose(fp);
	return FINISHED;
}
