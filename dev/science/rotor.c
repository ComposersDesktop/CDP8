/*	SYNTHESIS FROM ROTATING ARMATURES ....
 *
 *				params			  0		  1			2		3			4			5		  6			7			 7		8
 *				params		   ROT_CNT ROT_PMIN ROT_PMAX ROT_NSTEP	ROT_PCYC  ROT_TCYC	ROT_PHAS	 ROT_DUR	 ROT_GSTEP ROT_DOVE
 *	rotor rotor 1   infile env notecnt minmidi maxmidi	maxnotedur  protspeed drotspeed inital-phase out_duration step	  -dx
 *	rotor rotor 2-3 infile env notecnt minmidi maxmidi	maxnotedur  protspeed drotspeed inital-phase out_duration		  -dx		
 *																															x in ms
 *	Take a line with points marked at regular(?) intervals, y (height) determines pitch.
 *  Rotate the line about centre, so (if clockwise) pitchset falls slightly then more then more,
 *  until reaches maximum, then rises rapidly rises less etc till all down to same etc
 *
 *												@
 *		
 *									 @					  @
 *												@
 *				      @				  @					 @
 *						@					
 *	@  @  @  @  @		  @			   @		@		@	ETC
 *							@		    	
 *							  @			@			   @
 *												@
 *										 @			  @
 *
 *												@
 *	 repet-note		   falling		falling	  chord	 rising	
 *
 *
 *
 *	Take a line with points marked at regular(?) intervals, "x - minx" determines time.
 *  Rotate the line about centre.
 *
 *												@
 *		
 *									 @					  @
 *												@
 *				      @				  @					 @
 *						@					
 *	@  @  @  @  @		  @			   @		@		@	ETC
 *	1  2  3	 4	5	  		@		    	
 *	zero					  @			@			   @
 *	time										@
 *	slow			  1 2 3	4 5			 @			  @
 *	tempo			  zero			 12345			  12345	
 *					  time			 zero		@	  zero
 *					  faster		 time		zero  time
 *									 very		time  very
 *									 fast		Tutti fast
 *
 *
 *
 *	infile = waveform to read for synth
 *	env = envelope to impose on synthd sounds
 *	notecnt = number of notes in set.
 *	maxnotestep = max step between notes in slowest set
 *	protspeed = speed of rotation of pitch line
 *	drotspeed = speed of rotation of time line  = number of pitchlines before we're back to where we started
 *	initial-phase = (initial) phase difference between pitch and time rotations
 *		(this will change if rot speeds are different)
 *	step = time step between each pitch-set in mode 1
 *	mode 2 uses an extra non-sounding event at end of time row, to determine where
 *		next timerow begins, hence it dilates and contracts timewise with rotation of time-rotor.
 *	mode 3 superimposes first note of next row on last note of previous.
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
#include <science.h>
#include <ctype.h>
#include <sfsys.h>
#include <string.h>
#include <srates.h>

#define is_stereo	is_rectified
#define ROOT2		(1.4142136)

#ifdef unix
#define round(x) lround((x))
#endif

char errstr[2400];

int anal_infiles = 1;
int	sloom = 0;
int sloombatch = 0;

const char* cdp_version = "7.0.0";

//CDP LIB REPLACEMENTS
static int check_rotor_param_validity_and_consistency(dataptr dz);
static int setup_rotor_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_rotor_param_ranges_and_defaults(dataptr dz);
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
static int handle_the_special_data(char *str,dataptr dz);
static int create_rotor_sndbufs(dataptr dz);
static int rotor_param_preprocess(dataptr dz);
static int rotor(dataptr dz) ;
static int write_event(double time,double thispitch,double tabincr,int tabsize,int *obufpos,double normaliser,double line_angle,double pos,dataptr dz);
static int get_event_level(double time,double thispitch,double tabincr,int tabsize,int *obufpos,double *normaliser,double line_angle,double pos,dataptr dz);
static int read_value_from_brkarray(double *env,int *nextind,double *val,double thistime,dataptr dz);
static void time_display(int samps_sent,dataptr dz);
static int write_rotor_samps(float *obuf,int samps_sent,dataptr dz);

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
		if((exit_status = setup_rotor_application(dz))<0) {
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
	if((exit_status = setup_rotor_param_ranges_and_defaults(dz))<0) {
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
//	handle_special_data .....
	if((exit_status = handle_the_special_data(cmdline[0],dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	cmdlinecnt--;
	cmdline++;
	if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {		// CDP LIB
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
//	check_param_validity_and_consistency....
	if((exit_status = check_rotor_param_validity_and_consistency(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = open_the_outfile(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	is_launched = TRUE;
	dz->bufcnt = 5;
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

	if((exit_status = create_rotor_sndbufs(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	//param_preprocess ....
	if((exit_status = rotor_param_preprocess(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	//spec_process_file =
	if((exit_status = rotor(dz))<0) {
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
	int has_extension = 0;
	char *filename = (*cmdline)[0], *p;
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
	p = filename + strlen(filename);
	p--;
	while(p != filename) {
		if(*p == '.') {
			has_extension = 1;
			break;
		}
		p--;
	}
	strcpy(dz->outfilename,filename);
	if(!has_extension)
		strcat(dz->outfilename,".wav");
	(*cmdline)++;
	(*cmdlinecnt)--;
	return(FINISHED);
}

/************************ OPEN_THE_OUTFILE *********************/

int open_the_outfile(dataptr dz)
{
	int exit_status;
	if(dz->is_stereo)
		dz->infile->channels = 2;
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

/************************* SETUP_ROTOR_APPLICATION *******************/

int setup_rotor_application(dataptr dz)
{
	int exit_status;
	aplptr ap;

	if((exit_status = establish_application(dz))<0)		// GLOBAL
		return(FAILED);
	ap = dz->application;
	// SEE parstruct FOR EXPLANATION of next 2 functions
	if(dz->mode == 0)
		exit_status = set_param_data(ap,ROTORDAT,9,9,"iDDDIIddD");
	else
		exit_status = set_param_data(ap,ROTORDAT,9,8,"iDDDIIdd0");
	if(exit_status<0)
		return(FAILED);
	if((exit_status = set_vflgs(ap,"d",1,"d","s",1,0,"0"))<0)
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

/************************* SETUP_ROTOR_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_rotor_param_ranges_and_defaults(dataptr dz)
{
	int exit_status;
	aplptr ap = dz->application;
	// set_param_ranges()
	ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
	// NB total_input_param_cnt is > 0 !!!
	if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
		return(FAILED);
	// get_param_ranges()
	ap->lo[ROT_CNT]	= 3;
	ap->hi[ROT_CNT]	= 127;
	ap->default_val[ROT_CNT] = 7;
	ap->lo[ROT_PMIN] = 0;
	ap->hi[ROT_PMIN] = 127;
	ap->default_val[ROT_PMIN] = 48;
	ap->lo[ROT_PMAX] = 0;
	ap->hi[ROT_PMAX] = 127;
	ap->default_val[ROT_PMAX] = 72;
	ap->lo[ROT_NSTEP]	= 0;
	ap->hi[ROT_NSTEP]	= 4;
	ap->default_val[ROT_NSTEP] = .1;
	ap->lo[ROT_PCYC]	= 4;
	ap->hi[ROT_PCYC]	= 256;
	ap->default_val[ROT_PCYC] = 16;
	ap->lo[ROT_TCYC]	= 4;
	ap->hi[ROT_TCYC]	= 256;
	ap->default_val[ROT_TCYC] = 16;
	ap->lo[ROT_PHAS]	= 0;
	ap->hi[ROT_PHAS]	= 1;
	ap->default_val[ROT_PHAS] = 0;
	ap->lo[ROT_DUR]	= 1;
	ap->hi[ROT_DUR]	= 32767;
	ap->default_val[ROT_DUR] = 20;
	if(dz->mode == 0) {
		ap->lo[ROT_GSTEP]	= .1;
		ap->hi[ROT_GSTEP]	= 60;
		ap->default_val[ROT_GSTEP] = 4;
	}
	ap->lo[ROT_DOVE]	= 0;
	ap->hi[ROT_DOVE]	= 5;
	ap->default_val[ROT_DOVE] = 0;
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
			if((exit_status = setup_rotor_application(dz))<0)
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

int usage1()
{
	usage2("rotor");
	return(USAGE_ONLY);
}

/**************************** CHECK_ROTOR_PARAM_VALIDITY_AND_CONSISTENCY *****************************/

int check_rotor_param_validity_and_consistency(dataptr dz)
{
	if(!dz->brksize[ROT_PMIN] && !dz->brksize[ROT_PMAX]) {
		if(flteq(dz->param[ROT_PMIN],dz->param[ROT_PMAX])) {
			sprintf(errstr,"Zero pitchrange (%lf to %lf) specified.\n",dz->param[ROT_PMIN],dz->param[ROT_PMAX]);
			return(DATA_ERROR);
		} else if(dz->param[ROT_PMIN] > dz->param[ROT_PMAX]) {
			fprintf(stdout,"WARNING: Inverted or pitchrange (%lf to %lf) specified.\n",dz->param[ROT_PMIN],dz->param[ROT_PMAX]);
			fflush(stdout);
		}
	}
	if(dz->vflag[0])
		dz->is_stereo = 1;
	else
		dz->is_stereo = 0;
	return FINISHED;
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	if(!strcmp(prog_identifier_from_cmdline,"rotor"))				dz->process = ROTOR;
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
	if(!strcmp(str,"rotor")) {
		fprintf(stderr,
	    "USAGE:\n"
	    "rotor rotor 1   fi fo env cnt minp maxp step prot trot phas dur gstp [-ddove] [-s]\n"
	    "rotor rotor 2-3 fi fo env cnt minp maxp step prot trot phas dur [-ddove] [-s]\n"
		"\n"
		"Generate note-sets that grow and shrink in pitch-range and speed (and spatial-width).\n"
		"\n"
		"Mode 1: Note-set start-times determined by param \"gstp\".\n"
		"Mode 2: Next Note-set start-time, depends on spacings WITHIN current set.\n"
		"Mode 3: First event of next note-set overlaid on last event of previous set.\n"
		"\n"
	    "FI     File to be read at different speeds to generate output events.\n"
		"       (should start and end at sample value 0.0, OR use \"dove\")\n"
	    "FO     Output file(can be mono or stereo).\n"
		"ENV    Envelope to be imposed over output events.\n"
		"       Envelope duration determines duration of all events.\n"
		"CNT    Number of events in each (changing) set (Range 3 to 127).\n"
		"MINP   Minimum (MIDI) pitch of events (Range 0 to 127).\n"
		"MAXP   Maximum (MIDI) pitch of events (Range 0 to 127).\n"
		"STEP   Maximum timestep between event-onsets (Range 0 to 4 secs).\n"
		"PROT   Number of notesets before pitch-sequence returns to orig (Range 4 to 256).\n"
		"TROT   Number of speeds, before speed returns to original (Range 4 to 256).\n"
		"PHAS   Initial phase difference between prot and trot (range 0 - 1).\n"
		"DUR    Duration of output to generate (Range 1 to 32767).\n"
		"GSTP   (Mode 1 only) timestep between each note-group (Range 1 to 60).\n"
		"DOVE   Size (mS) of start/end dovetails of insound  (Range 0 to 5).\n"
		"\n"
		"-s     Stereo output: output grows and shrinks in spatial width.\n");
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

/**************************** HANDLE_THE_SPECIAL_DATA ****************************/

int handle_the_special_data(char *str,dataptr dz)
{
	FILE *fp;
	double dummy, lasttime = 0.0;
	char temp[200], *p;
	int istime = 1;
	int cnt = 0;

	if((fp = fopen(str,"r"))==NULL) {
		sprintf(errstr,"Cannot open file %s to read envelope data.\n",str);
		return(DATA_ERROR);
	}
	while(fgets(temp,200,fp)!=NULL) {
		p = temp;
		while(isspace(*p))
			p++;
		if(*p == ';' || *p == ENDOFSTR)	//	Allow comments in file
			continue;
		while(get_float_from_within_string(&p,&dummy)) {
			if(istime) {
				if(cnt == 0) {
					if(dummy != 0.0) {
						sprintf(errstr,"Initial time in data in file %s must be zero.\n",str);
						return(DATA_ERROR);
					}
				} else {
					if(dummy <= lasttime) {
						sprintf(errstr,"Times do not advance between %lf and %lf in file %s\n",lasttime,dummy,str);
						return(DATA_ERROR);
					}
				}
				lasttime = dummy;
			} else if(dummy > 1.0 || dummy < 0.0) {
				sprintf(errstr,"Found envelope value (%lf) out of range (0 to 1) in file %s\n",dummy,str);
				return(DATA_ERROR);
			}
			istime = !istime;
			cnt++;
		}
	}
	if(cnt == 0) {
		sprintf(errstr,"No data found in file %s\n",str);
		return(DATA_ERROR);
	}
	if(!EVEN(cnt)) {
		sprintf(errstr,"Data not paired correctly in file %s\n",str);
		return(DATA_ERROR);
	}
	if(cnt < 4) {
		sprintf(errstr,"Insufficient data found in file %s : Needs at least 2 time-value pairs.\n",str);
		return(DATA_ERROR);
	}
	dz->frametime = (float)lasttime;									//	Remember duration of envelope
	dz->rampbrksize = (int)round(dz->frametime * dz->infile->srate);	//	Remember duration of envelope in samples

	if((dz->parray = (double **)malloc(sizeof(double *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store transposition data.\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[0] = (double *)malloc(cnt * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store transposition data.\n");
		return(MEMORY_ERROR);
	}
	cnt = 0;
	fseek(fp,0,0);
	while(fgets(temp,200,fp)!=NULL) {
		p = temp;
		while(isspace(*p))
			p++;
		if(*p == ';' || *p == ENDOFSTR)	//	Allow comments in file
			continue;
		while(get_float_from_within_string(&p,&dummy)) {
			dz->parray[0][cnt] = dummy;
			cnt++;
		}
	}
	fclose(fp);
	dz->itemcnt = cnt;
	return FINISHED;
}

/*************************** CREATE_ROTOR_SNDBUFS **************************/

int create_rotor_sndbufs(dataptr dz)
{
	int n, exit_status;
	int bigbufsize, inbufsize, evbufsize, maxrotstepsamps, maxrotcnt;
	double maxrotstep, maxrotcntd;
	if(dz->sbufptr == 0 || dz->sampbuf==0) {
		sprintf(errstr,"buffer pointers not allocated: create_sndbufs()\n");
		return(PROGRAM_ERROR);
	}
	if(dz->brksize[ROT_NSTEP]) {
		if((exit_status = get_maxvalue_in_brktable(&maxrotstep,ROT_NSTEP,dz))<0)
			return exit_status;
	} else 
		maxrotstep = dz->param[ROT_NSTEP];
	if(dz->brksize[ROT_CNT]) {													//	Output may baktrak, noteset to noteset
		if((exit_status = get_maxvalue_in_brktable(&maxrotcntd,ROT_CNT,dz))<0)
			return exit_status;
		maxrotcnt = (int)round(maxrotcntd);
	} else 
		maxrotcnt = dz->iparam[ROT_CNT];
	maxrotstepsamps = (int)ceil(maxrotstep * dz->infile->srate);				// maximum size of note
	dz->buflen = maxrotcnt * maxrotstepsamps;									// maximum size of noteset
	if(dz->is_stereo)
		dz->buflen *= 2;
	inbufsize = dz->insams[0] + 1;	//	Add wrap-around point

	evbufsize = dz->rampbrksize;	//	Store size of envelope, in samples
	evbufsize += 2;					//	1 for wraparound, 1 for safety!!

	if(dz->is_stereo)
		bigbufsize = inbufsize + (evbufsize * 4) + (dz->buflen * 2); //	In mode 0, may need to baktrak, but never more than 1 complete (max)setlen
	else
		bigbufsize = inbufsize + (evbufsize * 3) + (dz->buflen * 2);	//	Need space for outbuf & overflowbuf
	if((dz->bigbuf = (float *)malloc(bigbufsize * sizeof(float))) == NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
		return(PROGRAM_ERROR);
	}

	// MONO
	//	  obuf		ovflwbuf			eventbuf		  envelopebuf		inbuf
	//		0			1					2			  	  3				  4
	//	|-------|------------------|------------------|------------------|------------|
	//			
	//   buflen	     evbufsize			evbufsize			evbufsize		insams[0]
	//				  
	// STEREO				  
	//	  obuf				ovflwbuf					eventbuf		  envelopebuf		  inbuf
	//		0					1							2			  	  3					4
	//	|-------|------------------------------------|------------------|------------------|------------|
	//   buflen			evbufsize * 2					evbufsize			evbufsize		insams[0]
	//
	//
	n = 0;
	dz->sbufptr[n] = dz->sampbuf[n] = dz->bigbuf;					 
	n++;															 //	0 = Output buffer
	dz->sbufptr[n] = dz->sampbuf[n] = dz->sampbuf[n-1] + (dz->buflen * 2); //	size buflen * 2
	n++;															 //	1 = overflow buffer
	if(dz->is_stereo)												 //			size evbufsize * outchans
		dz->sbufptr[n] = dz->sampbuf[n] = dz->sampbuf[n-1] + (evbufsize * 2);
	else
		dz->sbufptr[n] = dz->sampbuf[n] = dz->sampbuf[n-1] + evbufsize;	 //	2 = created event
	n++;															 //			size evbufsize
	dz->sbufptr[n] = dz->sampbuf[n] = dz->sampbuf[n-1] + evbufsize;	 //	3 = envelope of event
	n++;															 //			size evbufsize
	dz->sbufptr[n] = dz->sampbuf[n] = dz->sampbuf[n-1] + evbufsize;	 //	4 = insndbuf
	return(FINISHED);
}

/************************************* ROTOR_PARAM_PREPROCESS ***********************************
 *
 * (1)	Read input file to buffer, withg wraparound point, for reading as a waveform table.
 * (2)	Convert input envelope to a sample scale array in another buffer.
 */

int rotor_param_preprocess(dataptr dz) 
{
	int exit_status;
	double *env = dz->parray[0];
	int n, m;
	double srate = (double)dz->infile->srate, val, thistime;
	int origbuflen = dz->buflen, nextind, dovecnt;
	float *ibuf = dz->sampbuf[4];
	float *ebuf = dz->sampbuf[3];
	dz->buflen = dz->insams[0];						//	Read input sound to ibuf
	if((exit_status = read_samps(ibuf,dz))<0)
		return(exit_status);
	if(dz->param[ROT_DOVE] > 0) {
		dovecnt = (int)round(dz->param[ROT_DOVE] * MS_TO_SECS * dz->infile->srate);
		if(dovecnt * 2 >= dz->buflen) {
			sprintf(errstr,"Dovetails too large for input sound.\n");
			return DATA_ERROR;
		}
		for(n= 0;n < dovecnt; n++) {				//	Dovetail start
			val = (double)n/(double)dovecnt;
			ibuf[n] = (float)(ibuf[n] * val);
		}											//	Dovetail end
		for(n= dz->buflen - 1,m = 0;m < dovecnt; n--,m++) {
			val = (double)m/(double)dovecnt;
			ibuf[n] = (float)(ibuf[n] * val);
		}
	}
	ibuf[dz->buflen] = 0;							//	Wrap-around zero-point
	dz->buflen = origbuflen;

	nextind = 2;									//	Read input envelope array into a sample-scale array in a buffer
	for(n = 0; n < dz->rampbrksize;	n++) {
		thistime = (double)n/srate;
		if((exit_status = read_value_from_brkarray(env,&nextind,&val,thistime,dz))<0)
			return exit_status;
		ebuf[n] = (float)val;
	}
	ebuf[n] = 0.0f;								//	Wrap-around zero point
	return FINISHED;
}

/**************************** READ_VALUE_FROM_BRKARRAY *****************************/

int read_value_from_brkarray(double *env,int *nextind,double *val,double time,dataptr dz)
{
    double thistim, nexttim, thisval, nextval, valdiff, timdiff, timfrac;
	nexttim = env[*nextind];
	while(time > nexttim) {
		if((*nextind += 2) >= dz->itemcnt) {
			sprintf(errstr, "Overshot end of envelope brktable while converting to sample-buffer.\n");
			return PROGRAM_ERROR;
		}
		nexttim = env[*nextind];
	}
	thistim = env[*nextind - 2];
	thisval = env[*nextind - 1];
	nextval = env[*nextind + 1];
	valdiff = nextval - thisval;
	timdiff = nexttim - thistim;
	timfrac = (time - thistim)/timdiff;
	valdiff *= timfrac;
	*val = thisval + valdiff;
	return FINISHED;
}

/**************************** ROTOR *****************************/

int rotor(dataptr dz) 
{
	int exit_status, pitch_orient = 1;
	int obufpos, ovflwsize;
	float *obuf = dz->sampbuf[0];
	int stepcnt, notecnt = dz->iparam[ROT_CNT], kk, tsets_per_cycle;	//	If there are 5 positions before line returns to orig position.
	double drotspeed, protspeed, maxtime, duration, maxrange, centre, total_time, local_time, line_angle, p_line_angle;
	double pitchrange, halfrange, rangebot, thispitch, timestep, thispos, normaliser = 0.0;
	int m, n;
	int tabsize = dz->insams[0];
	double tabincr = (double)tabsize/(double)dz->infile->srate; //	tabincr to read table once per second, i.e. at 1Hz
	int ochans = 1;
	if(dz->is_stereo)
		ochans++;
	ovflwsize = dz->rampbrksize * ochans;
	stepcnt = notecnt - 1;					//	e.g. with 5 notes, there are 4 gaps
	duration = dz->param[ROT_DUR];			//	Total duration of output

	dz->tempsize = (int)round(duration * dz->infile->srate) * ochans;	//	Establish scale for loom progress_bar

	//	INITIALISE CONSTANTS
	for(kk=0;kk<2;kk++) {
		memset((char *)obuf,0,((dz->buflen * 2) + ovflwsize) * sizeof(float));
		obufpos = 0;
		total_time = 0.0;
		line_angle = 0.0;
		dz->total_samps_written = 0;
		if(kk == 0)
			time_display(dz->total_samps_written,dz);
		p_line_angle = dz->iparam[ROT_PHAS] * TWOPI;	//	Set initla phase of pitch-rotor

		if(dz->brksize[ROT_TCYC]) {
			if((exit_status = read_value_from_brktable(total_time,ROT_TCYC,dz))<0)
				return(exit_status);
		}
		tsets_per_cycle = dz->iparam[ROT_TCYC];			//	If there are 5 positions before line returns to orig position.
		drotspeed= 1.0/tsets_per_cycle;					//	then there is (r=)1/5th of a rotation per line-set.
		drotspeed *= TWOPI;								//	Convert to radians.
		if(dz->brksize[ROT_PCYC]) {
			if((exit_status = read_value_from_brktable(total_time,ROT_PCYC,dz))<0)
				return(exit_status);
		}
		protspeed = 1.0/dz->iparam[ROT_PCYC];			//	How much of a cycle per note-set
		protspeed *= TWOPI;								//	Convert to radians.

		if(dz->brksize[ROT_PMAX]) {
			if((exit_status = read_value_from_brktable(total_time,ROT_PMAX,dz))<0)
				return(exit_status);
		}
		if(dz->brksize[ROT_PMIN]) {
			if((exit_status = read_value_from_brktable(total_time,ROT_PMIN,dz))<0)
				return(exit_status);
		}
		maxrange = dz->param[ROT_PMAX] - dz->param[ROT_PMIN];
		centre   = dz->param[ROT_PMIN] + maxrange/2.0;	//	Set initial pitch-range params

		if(dz->brksize[ROT_NSTEP]) {
			if((exit_status = read_value_from_brktable(total_time,ROT_NSTEP,dz))<0)
				return(exit_status);
		}
		maxtime = dz->param[ROT_NSTEP];					//	Set initial maximum timestep between notes.
		
		if(kk == 0) {
			fprintf(stdout,"INFO: Checking output level.\n");
			fflush(stdout);
		} else {
			if(sloom)
				fprintf(stdout,"INFO: Writing output.\n");
			else
				fprintf(stdout,"\nINFO: Writing output.\n");
			fflush(stdout);
		}

		while(total_time < duration) {
			for(m = 0; m < tsets_per_cycle;m++) {
				local_time = 0.0;
				timestep = fabs(maxtime * cos(line_angle));			//	Time-step to next event when line is tilted at angle
				pitchrange = maxrange * sin(p_line_angle);			//	Range shrunk (or inverted) by sin-function.
				halfrange = pitchrange/2.0;							//	If inverted, halfrange is -ve
				halfrange *= pitch_orient;							//	Inverts range on passing through 2PI
				rangebot = centre - halfrange;						//	and "rangebot" is at top
				for(n = 0;n < stepcnt; n++) {

					//	CACULATE PITCH OF EVENT FROM ROTATING ARM, AND POSITION ON ARM
					
					thispos   = (double)n/(double)stepcnt;			//	relative position in range (normalised 0-1)
					thispitch  = thispos * pitchrange;				//	but "thispitch" here is -ve
					thispitch *= pitch_orient;
					thispitch += rangebot;							//	So true pitch is subtracted from top of range

					//	WRITE OUTPUT EVENT
					
					if(kk == 0) {
						if((exit_status = get_event_level(total_time+local_time,thispitch,tabincr,tabsize,&obufpos,&normaliser,line_angle,thispos,dz))<0)	//	Check output level
							return exit_status;
					} else {
						if((exit_status = write_event(total_time+local_time,thispitch,tabincr,tabsize,&obufpos,normaliser,line_angle,thispos,dz))<0) //	Write all events except last
							return exit_status;
					}

					//	ADVANCE TIME, VIA TIME-ROTATOR

					local_time += timestep;
				}

				//	WRITE FINAL EVENT OF TIME-SET

				thispos   = (double)n/(double)stepcnt;
				thispitch = thispos * pitchrange;
				thispitch *= pitch_orient;
				thispitch += rangebot;
				if(kk == 0) {
					if((exit_status = get_event_level(total_time+local_time,thispitch,tabincr,tabsize,&obufpos,&normaliser,line_angle,thispos,dz))<0)	//	Check output level
						return exit_status;
				} else {
					if((exit_status = write_event(total_time+local_time,thispitch,tabincr,tabsize,&obufpos,normaliser,line_angle,thispos,dz))<0)	//	Write last_event
						return exit_status;
				}

				//	AT END OF A COMPLETE SET, Read any time-varying params

				//	PITCH ROTOR CONTINUES TO ROTATE

				p_line_angle += protspeed;							//	Advance pitch-rotator angle
				if(p_line_angle >= TWOPI) {							//	If pitchrotor cycle completed
					if(dz->brksize[ROT_PCYC]) {
						if((exit_status = read_value_from_brktable(total_time,ROT_PCYC,dz))<0)
							return(exit_status);
						protspeed = 1.0/dz->iparam[ROT_PCYC];		//	How much of a cycle per note-set
						protspeed *= TWOPI;							//	Convert to radians.
					}
					p_line_angle -= TWOPI;
					pitch_orient = -pitch_orient;
				}
				if(p_line_angle < PI/2.0 || p_line_angle >= 3 * PI/2.0)
					pitch_orient = 1;
				else
					pitch_orient = -1;

				//	Update any (other) timer-varying params, at end of a noteset
				
				if(dz->brksize[ROT_PMIN] || dz->brksize[ROT_PMAX]) {
					if(dz->brksize[ROT_PMIN]) {
						if((exit_status = read_value_from_brktable(total_time,ROT_PMIN,dz))<0)
							return(exit_status);
					}
					if(dz->brksize[ROT_PMAX]) {
						if((exit_status = read_value_from_brktable(total_time,ROT_PMAX,dz))<0)
							return(exit_status);
					}
					maxrange = dz->param[ROT_PMAX] - dz->param[ROT_PMIN];
					centre   = dz->param[ROT_PMIN] + maxrange/2.0;
				}
				if(dz->brksize[ROT_NSTEP]) {
					if((exit_status = read_value_from_brktable(total_time,ROT_NSTEP,dz))<0)
						return(exit_status);
					maxtime = dz->param[ROT_NSTEP];
				}

				if(dz->brksize[ROT_TCYC]) {
					if((exit_status = read_value_from_brktable(total_time,ROT_TCYC,dz))<0)
						return(exit_status);
					//	Cannot alter the tsets_per_cycle inside this loop (do it after exiting loop, below)
					drotspeed= 1.0/dz->iparam[ROT_TCYC];
					drotspeed *= TWOPI;
				}
				if((line_angle += drotspeed) >= TWOPI)
					line_angle -= TWOPI;

				//	locate start of next TSET

				switch(dz->mode) {
				case(0):
					if(dz->brksize[ROT_GSTEP]) {
						if((exit_status = read_value_from_brktable(total_time,ROT_GSTEP,dz))<0)
							return(exit_status);
					}												//	Get step to next note-set as input param
					total_time += dz->param[ROT_GSTEP];
					break;
				case(1):											//	All events have already been written
					total_time += local_time + timestep;
					break;
				case(2):											//	Keep group time where last group was placed
					total_time += local_time;
					break;											//	(1st event of next set superimposed on last event this set)	
				}
				if(total_time >= duration)
					break;
			}

			//	Add the end of a complete rotation of groups-of-notesets, read any time-varying time-rotation data
			
			if(dz->brksize[ROT_TCYC]) {
				if((exit_status = read_value_from_brktable(total_time,ROT_TCYC,dz))<0)
					return(exit_status);
				tsets_per_cycle = dz->iparam[ROT_TCYC];
				//	We already know drotspeed from reading table above
			}
		}
		if(kk == 0) {
			if(obufpos > 0) {
				for(n=0;n<obufpos;n++) {
					if(fabs(obuf[n]) > normaliser)
						normaliser = fabs(obuf[n]);
				}
			}
			normaliser = 0.95/normaliser;
		} else {
			if(obufpos > 0) {
				if(normaliser < 1.0) {
					for(n=0;n<obufpos;n++)
						obuf[n] = (float)(obuf[n] * normaliser);
				}
				if((exit_status = write_rotor_samps(obuf,obufpos,dz))<0)
					return(exit_status);
			}
		}
	}
	return FINISHED;
}

/******************************************* WRITE_EVENT *******************************/

int write_event(double time,double thispitch,double tabincr,int tabsize,int *obufpos,double normaliser,double line_angle,double pos,dataptr dz)
{
	int exit_status;
	float *obuf  = dz->sampbuf[0];
	float *nbuf = dz->sampbuf[2];
	float *ebuf = dz->sampbuf[3];				//	Create envelope of length of required event, at srate
	float *ibuf = dz->sampbuf[4];
	double frq = miditohz(thispitch);
	int eventsamps = dz->rampbrksize,n,k,ovflwsize;
	double tabpos = 0.0, frac, diff, relpos, reldist, temp, lpos, rpos,thisval;
	int thispos, nextpos, bufpos;
	int ochans = 1;

	if(dz->is_stereo)
		ochans++;
	ovflwsize = dz->rampbrksize * ochans;
	tabincr *= frq;								//	Frq-related table-read increment
	for(n = 0; n< eventsamps;n++) {
		thispos = (int)floor(tabpos);			//	Read input sample by interpolation
		nextpos = thispos+1;					//	with incr determined by pitch/frq
		frac = tabpos - thispos;
		diff =  ibuf[nextpos] - ibuf[thispos];
		diff *= frac;
		thisval = ibuf[thispos] + diff;
		nbuf[n] = (float)(thisval * ebuf[n]);	//	Scale by input envelope
		tabpos += tabincr;
		if(tabpos >= tabsize)
			tabpos -= tabsize;
	}
	bufpos = (int)round(time * dz->infile->srate) * ochans;
	bufpos -= dz->total_samps_written;
	while(bufpos >= (dz->buflen * 2) + ovflwsize) {		//	In case bufpos jumps ahead beyond buffer
		if(normaliser < 1.0) {					//	Only write (1) buflen if we've also filled the overflow buffer
			for(k=0;k<dz->buflen;k++)	{		//	so that we can potentially backtrack over the buflen
				obuf[k] = (float)(obuf[k] * normaliser);
			}
		}
		if((exit_status = write_rotor_samps(obuf,dz->buflen,dz))<0)
			return(exit_status);
		memcpy((char *)obuf,(char *)(obuf+dz->buflen),(ovflwsize + dz->buflen) * sizeof(float));
		memset((char *)(obuf + dz->buflen + ovflwsize),0,dz->buflen * sizeof(float));
		bufpos -= dz->buflen;
	}

	if(dz->is_stereo) {			//	Change position range from (normalised) 0 to 1
		pos *= 2.0;				//	to 0 to 2
		pos -= 1.0;				//	to -1 to +1
		pos *= cos(line_angle);	// Scale according to line angle : (-1 to 1) at cos(0)==1	 --> (-1 to 1)
		if(pos < 0)				//								   (-1 to 1) at cos(PI/2)==0 --> (0 to 0)  squeezed to centre
			relpos = -pos;		//								   (-1 to 1) at cos(PI)==-1  --> (1 to -1) range inverted : ETC
		else					//	Do hole-in-middle compensation
			relpos = pos;
		temp = 1.0 + (relpos * relpos);	
		reldist = ROOT2 / sqrt(temp);
		temp = (pos + 1.0) / 2.0;
		rpos = temp * reldist;
		lpos = (1.0 - temp ) * reldist;

		for(n = 0; n< eventsamps;n++) {			//	Add new event into output stream
			if(bufpos >= (dz->buflen * 2) + ovflwsize) {
				if(normaliser < 1.0) {
					for(k=0;k<dz->buflen;k++) {
						obuf[k] = (float)(obuf[k] * normaliser);
					}
				}
				if((exit_status = write_rotor_samps(obuf,dz->buflen,dz))<0)
					return(exit_status);
				memcpy((char *)obuf,(char *)(obuf+dz->buflen),(ovflwsize + dz->buflen) * sizeof(float));
				memset((char *)(obuf + dz->buflen + ovflwsize),0,dz->buflen * sizeof(float));
				bufpos -= dz->buflen;
			}
			obuf[bufpos] = (float)(obuf[bufpos] + (nbuf[n] * lpos));
			bufpos++;
			obuf[bufpos] = (float)(obuf[bufpos] + (nbuf[n] * rpos));
			bufpos++;
		}

	} else {
		for(n = 0; n< eventsamps;n++) {					//	Add new event into output stream
			if(bufpos >= (dz->buflen * 2) + ovflwsize) {
				if(normaliser < 1.0) {
					for(k=0;k<dz->buflen;k++)
						obuf[k] = (float)(obuf[k] * normaliser);
				}
				if((exit_status = write_rotor_samps(obuf,dz->buflen,dz))<0)
					return(exit_status);
				memcpy((char *)obuf,(char *)(obuf+dz->buflen),(ovflwsize + dz->buflen) * sizeof(float));
				memset((char *)(obuf + dz->buflen + ovflwsize),0,dz->buflen * sizeof(float));
				bufpos -= dz->buflen;
			}
			obuf[bufpos] = (float)(obuf[bufpos] + nbuf[n]);
			bufpos++;
		}
	}
	*obufpos = bufpos;
	return FINISHED;
}

/******************************************* GET_EVENT_LEVEL *******************************/

int get_event_level(double time,double thispitch,double tabincr,int tabsize,int *obufpos,double *normaliser,double line_angle,double pos,dataptr dz)
{
	float *obuf  = dz->sampbuf[0];
	float *nbuf = dz->sampbuf[2];
	float *ebuf  = dz->sampbuf[3];					//	Create envelope of length of required event, at srate
	float *ibuf  = dz->sampbuf[4];
	double frq = miditohz(thispitch);
	int eventsamps = dz->rampbrksize,n,k, ovflwsize;
	double tabpos = 0.0, frac, diff, thisval, relpos, temp, reldist, rpos,lpos;
	int thispos, nextpos, bufpos;
	int ochans = 1;
	if(dz->is_stereo)
		ochans++;
	ovflwsize = dz->rampbrksize * ochans;
	tabincr *= frq;									//	Frq-related table-read increment
	for(n = 0; n< eventsamps;n++) {
		thispos = (int)floor(tabpos);				//	Read input sample by interpolation
		nextpos = thispos+1;						//	with incr determined by pitch/frq
		frac = tabpos - thispos;
		diff =  ibuf[nextpos] - ibuf[thispos];
		diff *= frac;
		thisval = ibuf[thispos] + diff;
		nbuf[n] = (float)(thisval * ebuf[n]);		//	Scale by input envelope
		tabpos += tabincr;
		if(tabpos >= tabsize)
			tabpos -= tabsize;
	}
	bufpos = (int)round(time * dz->infile->srate) * ochans;
	bufpos -= dz->total_samps_written;

	while(bufpos >= (dz->buflen * 2) + ovflwsize) {		//	In case bufpos jumps ahead beyond buffer
		for(k=0;k<dz->buflen;k++) {
			if(fabs(obuf[k]) > *normaliser)
				*normaliser = fabs(obuf[k]);
		}
		dz->total_samps_written += dz->buflen;
		time_display(dz->total_samps_written,dz);
		memcpy((char *)obuf,(char *)(obuf+dz->buflen),(ovflwsize + dz->buflen) * sizeof(float));
		memset((char *)(obuf + dz->buflen + ovflwsize),0,dz->buflen * sizeof(float));
		bufpos -= dz->buflen;
	}
	if(dz->is_stereo) {
		pos *= 2.0;				//	to 0 to 2
		pos -= 1.0;				//	to -1 to +1
		pos *= cos(line_angle);	// Scale according to line angle : (-1 to 1) at cos(0)==1	 --> (-1 to 1)
		if(pos < 0)				//								   (-1 to 1) at cos(PI/2)==0 --> (0 to 0)  squeezed to centre
			relpos = -pos;		//								   (-1 to 1) at cos(PI)==-1  --> (1 to -1) range inverted : ETC
		else					//	Do hole-in-middle compensation
			relpos = pos;
		temp = 1.0 + (relpos * relpos);	
		reldist = ROOT2 / sqrt(temp);
		temp = (pos + 1.0) / 2.0;
		rpos = temp * reldist;
		lpos = (1.0 - temp ) * reldist;
		for(n = 0; n< eventsamps;n++) {					//	Add new event into output stream
			if(bufpos >= (dz->buflen * 2) + ovflwsize) {
				for(k=0;k<dz->buflen;k++) {
					if(fabs(obuf[k]) > *normaliser)
						*normaliser = fabs(obuf[k]);
				}
				dz->total_samps_written += dz->buflen;
				time_display(dz->total_samps_written,dz);
				memcpy((char *)obuf,(char *)(obuf+dz->buflen),(ovflwsize + dz->buflen) * sizeof(float));
				memset((char *)(obuf + dz->buflen + ovflwsize),0,dz->buflen * sizeof(float));
				bufpos -= dz->buflen;
			}
			obuf[bufpos] = (float)(obuf[bufpos] + (nbuf[n] * lpos));
			bufpos++;
			obuf[bufpos] = (float)(obuf[bufpos] + (nbuf[n] * rpos));
			bufpos++;
		}
	} else {
		for(n = 0; n< eventsamps;n++) {					//	Add new event into output stream
			if(bufpos >= (dz->buflen * 2) + ovflwsize) {
				for(k=0;k<dz->buflen;k++) {
					if(fabs(obuf[k]) > *normaliser)
						*normaliser = fabs(obuf[k]);
				}
				dz->total_samps_written += dz->buflen;
				time_display(dz->total_samps_written,dz);
				memcpy((char *)obuf,(char *)(obuf+dz->buflen),(ovflwsize + dz->buflen) * sizeof(float));
				memset((char *)(obuf + dz->buflen + ovflwsize),0,dz->buflen * sizeof(float));
				bufpos -= dz->buflen;
			}
			obuf[bufpos] = (float)(obuf[bufpos] + nbuf[n]);
			bufpos++;
		}
	}
	*obufpos = bufpos;
	return FINISHED;
}

/******************************* TIME_DISPLAY **************************/

void time_display(int samps_sent,dataptr dz)
{
	if(sloom)
		dz->process = MTOS;
	display_virtual_time(samps_sent,dz);
	if(sloom)
		dz->process = ROTOR;
}

/******************************* WRITE_ROTOR_SAMPS **************************/

int write_rotor_samps(float *obuf,int samps_sent,dataptr dz)
{
	int exit_status;
	if(sloom)				//	Ensures correct setting of progress bar
		dz->process = MTOS;
	if((exit_status = write_samps(obuf,samps_sent,dz))<0)
		return(exit_status);
	if(sloom)
		dz->process = ROTOR;
	return FINISHED;
}
