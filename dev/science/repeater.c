//	_cdprogs\repeater repeater 3 alan_bellydancefc.wav test.wav repeater2.txt 8 .66 .66 -r2 -p.5

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

#define SAFETY 64
#define maxmaxbuf	 total_windows
#define envwindowlen ringsize
#define arraysize	 rampbrksize
#define	REPCLIP	0.95	//	level to normalise to
#define	REPMINDEL 0.02	//	minimum delay to produce oscillaor effect

#ifdef unix
#define round(x) lround((x))
#endif

char errstr[2400];

int anal_infiles = 1;
int	sloom = 0;
int sloombatch = 0;

const char* cdp_version = "6.1.0";

//CDP LIB REPLACEMENTS
static int setup_repeater_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_repeater_param_ranges_and_defaults(dataptr dz);
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
static int create_repeater_sndbufs(double maxseglen, double maxovlp, dataptr dz);
static int handle_the_special_data(char *str,double *maxseglen,double *maxovlp,dataptr dz);
static int repeater(dataptr dz);
static int setup_repeater_param_ranges_and_defaults(dataptr dz);
static int setup_repeater_application(dataptr dz);
static int write_and_reset_obuf(int samps_to_write,int *obufpos,dataptr dz);
static int reset_ibuf_and_read(int *ibufpos,dataptr dz) ;
static int normalise_buffer(int samplen,dataptr dz);
static int calc_output_dur(int *dursamps,dataptr dz);

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
	int exit_status;
	dataptr dz = NULL;
	char **cmdline;
	int  cmdlinecnt;
	int n;
	double maxseglen, maxovlp;
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
		if((exit_status = setup_repeater_application(dz))<0) {
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
	if((exit_status = setup_repeater_param_ranges_and_defaults(dz))<0) {
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
//	handle_special_data() = 
	if((exit_status = handle_the_special_data(cmdline[0],&maxseglen,&maxovlp,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	cmdlinecnt--;
	cmdline++;
	if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {		// CDP LIB
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
//	check_param_validity_and_consistency()	redundant
	is_launched = TRUE;
	dz->bufcnt = 7;
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

//	create_sndbufs() =
	if((exit_status = create_repeater_sndbufs(maxseglen,maxovlp,dz))<0) {			// CDP LIB
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	//param_preprocess() redundant
	//spec_process_file =
	if((exit_status = repeater(dz))<0) {
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

/************************* SETUP_REPEATER_APPLICATION *******************/

int setup_repeater_application(dataptr dz)
{
	int exit_status;
	aplptr ap;
	if((exit_status = establish_application(dz))<0)		// GLOBAL
		return(FAILED);
	ap = dz->application;
	// SEE parstruct FOR EXPLANATION of next 2 functions
	if(dz->mode < 2) 
		exit_status = set_param_data(ap,REPEATDATA,3,0,"000");
	else
		exit_status = set_param_data(ap,REPEATDATA,3,3,"DDD");
	if(exit_status <0)
		return(FAILED);
	if((exit_status = set_vflgs(ap,"rp",3,"DDi","",0,0,""))<0)
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
		} else if((exit_status = copy_parse_info_to_main_structure(infile_info,dz))<0) {
			sprintf(errstr,"Failed to copy file parsing information\n");
			return(PROGRAM_ERROR);
		}
		free(infile_info);
	}
	return(FINISHED);
}

/************************* SETUP_REPEATER_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_repeater_param_ranges_and_defaults(dataptr dz)
{
	int exit_status;
	aplptr ap = dz->application;
	// set_param_ranges()
	ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
	// NB total_input_param_cnt is > 0 !!!
	if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
		return(FAILED);
	// get_param_ranges()
	if(dz->mode == 2) {
		ap->lo[REP_ACCEL] = 1.0;
		ap->hi[REP_ACCEL] = 10.0;
		ap->default_val[REP_ACCEL]	= 2.0;
		ap->lo[REP_WARP] = 0.1;
		ap->hi[REP_WARP] = 10.0;
		ap->default_val[REP_WARP]	= 0.66;
		ap->lo[REP_FADE] = 0.1;
		ap->hi[REP_FADE] = 10.0;
		ap->default_val[REP_FADE]	= .33;
	}
	ap->lo[REP_RAND]	= 1.0;
	if(dz->mode == 1)
		ap->hi[REP_RAND] = 8.0;
	else
		ap->hi[REP_RAND] = 2.0;
	ap->default_val[REP_RAND]	= 1.0;
	ap->lo[REP_TRNSP] = 0.0;
	ap->hi[REP_TRNSP] = 12.0;
	ap->default_val[REP_TRNSP]	= 0.0;
	ap->lo[REP_SEED] = 0;
	ap->hi[REP_SEED] = 256;
	ap->default_val[REP_SEED]	= 0;
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
			if((exit_status = setup_repeater_application(dz))<0)
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
	usage2("repeater");
	return(USAGE_ONLY);
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	if(!strcmp(prog_identifier_from_cmdline,"repeater"))			dz->process = REPEATER;
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
	if(!strcmp(str,"repeater")) {
		fprintf(stderr,
	    "USAGE: repeater repeater\n"
	    "1-2 infile outfile datafile                 [-rrand] [-prand] [-sseed]\n"
	    "3   infile outfile datafile accel warp fade [-rrand] [-prand] [-sseed]\n"
		"\n"
		"Play source, with specified elements repeating.\n"
		"MODE 3	produces dimming, accelerating output, like bouncing object.\n"
		"\n"
		"MODES 1 & 3 DATAFILE has sets of 4-values, being....\n"
		"       \"Start-time\"   \"End-time\"   \"Repeat-cnt\"   \"Delay\"\n"
		"       with one set-of-values for each element to be repeated.\n"
		"       Elements can overlap, or baktrak in src, & must be at >= %.3lf secs.\n"
		"       \"Delay\", is time between start of 1 repeated element & start of next.\n"
		"       Delay zero will produce a delay equal to the segment length.\n"
		"       Otherwise, delays < 0.05 secs may produce output like oscillator.\n"
		"\n"
		"MODE 2 DATAFILE has sets of 4-values, being....\n"
		"       \"Start-time\"   \"End-time\"   \"Repeat-cnt\"   \"Offset\"\n"
		"       Similar to MODE 1 but using \"Offset\" instead of \"Delay\".\n"
		"       \"Offset\", for any repeating segment,\n"
		"       is the gap between end of one repeated element and start of next.\n"
		"\n"
		"RAND   Randomise delay:\n"
		"       Mode 1&3: Extend each delay-time by a random multiple.\n"
		"                 Multiplier generated in range you specify (between 1 & 2).\n"
		"       Mode 2:   Extend each offset-time by a random multiple.\n"
		"                 Multiplier generated in range you specify (between 1 & 8).\n"
		"       Value 1 gives NO randomisation. \"RAND\" may vary through time.\n"
		"\n"
		"PRAND  Randomise pitch of repeats within given semitone range (between 0 & 12)\n"
		"       \"PRAND\" may vary through time.\n"
		"\n"
		"SEED   An integer value. repeated runs of process with same input\n"
		"       and same seed value will give identical output.\n"
		"\n"
		"ACCEL  Delay (& segment) shortening by end of repeats\n"
		"       e.g. accel = 2 gradually shortens delay to 1/2 its duration.\n"
		"WARP   Warps delay change. 1 no warp. > 1 shortens less initially, more later.\n"
		"FADE   Decay curve. 1 linear, >1 fast then slow decay, <1 slow then fast.\n",(int)round(REPSPLEN * 2) * MS_TO_SECS,(int)round(REPSPLEN * 2) * MS_TO_SECS);
	} else
		fprintf(stderr,"Unknown option '%s'\n",str);
	return(USAGE_ONLY);
}

int usage3(char *str1,char *str2)
{
	fprintf(stderr,"Insufficient parameters on command line.\n");
	return(USAGE_ONLY);
}

/******************************** REPEATER ********************************/

int repeater(dataptr dz)
{
	int exit_status, chans = dz->infile->channels, ch, at_start, overlap, varypitch; 
	float *ibuf = dz->sampbuf[0], *iovflwbuf = dz->sampbuf[1], *obuf = dz->sampbuf[2], *repbuf = dz->sampbuf[4], *reprepbuf = dz->sampbuf[5], *segbuf;
	double splicelen = REPSPLEN * MS_TO_SECS, srate = (double)dz->infile->srate, maxtransdown = 1.0, maxexpand = 1.0;
	int gp_splicesamps = (int)round(splicelen * srate), possible_gp_samps_to_read, maxrepbufpos, samps_written, outsamps = 0, lastrepbufpos;
	double dgp_splicesamps = (double)gp_splicesamps, val, rnd, thistime, incr, md, frac, diff;
	int total_splicesamps = gp_splicesamps * chans, last_gp_absendsamp, ibufpos, obufpos, repbufpos, bufpos_in_iovflw, totalreps, rep, n, m, k, baktrak;
	int samps_to_read, delaysamps, gp_abssttsamp, gp_absendsamp, repeats, gp_delaysamps, gp_samps_to_read, samps_to_write, startdelay = 0, datacnt, thisdata;
	double *segdata = dz->parray[0];
	int isshorten;
	double lenchange, lenchangeincr = 0.0, lenfact, thisfade, endspliceval;
	int inital_gp_delaysamps = 0, initial_gp_samps_to_read = 0, gp_endsplice = 0, gp_endsplice_stt = 0, endsplice = 0, endsplice_stt = 0;

	srand((int)dz->iparam[REP_SEED]);

	if(sloom) {
		if((exit_status = calc_output_dur(&outsamps,dz))<0)
			return exit_status;
		dz->tempsize = outsamps;
	}
	if(dz->mode != 1) {								//	Setup enveloping arrays, for signal normalisation
		dz->envwindowlen = gp_splicesamps * 2;		//	half-windowlen must be no larger than splicesamps (ensuring envelope is val 1 throughout splice)
		dz->envwindowlen *= chans;
		dz->arraysize = dz->buflen2/dz->envwindowlen;
		dz->arraysize += SAFETY;
		if((dz->parray[1] = (double *)malloc(dz->arraysize * sizeof(double)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to create envelope array.\n");
			return(MEMORY_ERROR);
		}
		if((dz->iparray = (int **)malloc(sizeof(int *)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to create envelope max locations array (1).\n");
			return(MEMORY_ERROR);
		}
		if((dz->iparray[0] = (int *)malloc(dz->arraysize * sizeof(int)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to create envelope max locations array (2).\n");
			return(MEMORY_ERROR);
		}
	}
	at_start = 1;
	last_gp_absendsamp = 0;

	if((exit_status = read_samps(iovflwbuf,dz))<0)		//	Initially read into input-overflow-buf
		return exit_status;
	ibufpos = dz->buflen;								//	and point into it
	obufpos = 0;
	totalreps = dz->itemcnt/4;							//	For every repeat unit in data
	for(rep=0, datacnt = 0; rep< totalreps;rep++,datacnt+=4) {
		thisdata = datacnt;								//	Get the repeat-params
		thistime = segdata[thisdata];
		gp_abssttsamp = (int)round(segdata[thisdata++] * srate);
		gp_absendsamp = (int)round(segdata[thisdata++] * srate);
		repeats	      = (int)round(segdata[thisdata++]);
		gp_delaysamps = (int)round(segdata[thisdata] * srate);

		if((exit_status = read_values_from_all_existing_brktables(thistime,dz))<0)
			return exit_status;
		varypitch = 0;
		if(dz->param[REP_TRNSP] != 0.0) {
			varypitch = 1;
			maxtransdown = pow(2.0,-dz->param[REP_TRNSP]/SEMITONES_PER_OCTAVE);
			maxexpand = 1.0/maxtransdown;
		}
		gp_samps_to_read = gp_abssttsamp - last_gp_absendsamp;
		if(gp_samps_to_read > 0) {						//	If next segment is beyond end of last segment, read from infile.
													
		//	READ any of infile BETWEEN SEGS

			while(obufpos >= dz->buflen + total_splicesamps) {
				if((exit_status = write_and_reset_obuf(dz->buflen,&obufpos,dz))<0)
					return(exit_status);				//	check if obuf has overflowed
			}
			gp_samps_to_read += gp_splicesamps;			//	Allow for endsplice-down in read-segment by reading extra from input
			for(n=0,m = gp_samps_to_read - 1;n < gp_samps_to_read; n++,m--) {
				if(n < gp_splicesamps) {
					if(at_start)
						val = 1.0;
					else
						val = (double)n/dgp_splicesamps;//	Copy to output, with splices at start, if not at start of infile
				} else if (m < gp_splicesamps)
					val = (double)m/dgp_splicesamps;	//	and splice at end
				else
					val = 1.0;
				for(ch = 0;ch < chans; ch++) {
					obuf[obufpos] = (float)(obuf[obufpos] + (ibuf[ibufpos] * val));
					obufpos++;
					ibufpos++;
				}
				if(obufpos >= dz->buflen + total_splicesamps) {
					if((exit_status = write_and_reset_obuf(dz->buflen,&obufpos,dz))<0)
						return(exit_status);
				}
				if(ibufpos >= dz->buflen * 2) {
					if((exit_status = reset_ibuf_and_read(&ibufpos,dz))<0)
						return(exit_status);
					if(dz->ssampsread == 0) {
						sprintf(errstr,"Reached end of input prematurely\n");
						return PROGRAM_ERROR;
					}
				}
			}
			at_start = 0;
			obufpos -= total_splicesamps;				//	Baktrack by splicelen in output;
			ibufpos -= total_splicesamps;				//	Baktrack by splicelen in input, for start-read of segment.

		} else {
														//	ELSE go directly to the new segment
			baktrak = (last_gp_absendsamp - gp_abssttsamp) * chans;
			if(baktrak > 0) {							//	If this seg starts BEFORE END but AFTER START of previous seg, baktrak in current buf.
				ibufpos -= baktrak;						//	ibuf has a prebuf as big as the largest possible segment, so baktracking will fall inside inbuf.

				if(ibufpos < 0) {						//	However, if this segment starts BEFORE START of previous segment, baktraking may go beyond start of prebuf,
					sndseekEx(dz->ifd[0],0,0);			//	in which case return to start of file ...
					dz->total_samps_read = 0;			
					dz->ssampsread = 0;					// ...and read...
					memset((char *)ibuf,0,2 * dz->buflen * sizeof(float));
					if((exit_status = read_samps(iovflwbuf,dz))<0)		
						return exit_status;
					ibufpos = dz->buflen;				 //	...until we find required sample-position
					while(dz->total_samps_read < gp_abssttsamp * chans) {
						if((exit_status = reset_ibuf_and_read(&ibufpos,dz))<0)
							return(exit_status);
						if(dz->ssampsread == 0) {
							sprintf(errstr,"Reached end of input prematurely during baktrak in infile.\n");
							return PROGRAM_ERROR;
						}
					}
					if(dz->total_samps_read == dz->ssampsread)
						at_start = 1;
				}	
			}
		}

		//	READ SEGMENT

		gp_samps_to_read = gp_absendsamp - gp_abssttsamp;//	Read the rep-segment into rep-buffer

		if(dz->mode != 1) {
			if(gp_delaysamps == 0)
				gp_delaysamps = gp_samps_to_read;		//	Special delay value, zero, delays sample by its complete length
		}
		gp_samps_to_read += gp_splicesamps;

		if(varypitch && (maxexpand > 1.0))				//	IF segments are transposed downwards, check if they will overlap
			possible_gp_samps_to_read = (int)ceil((double)gp_samps_to_read * maxexpand);
		else
			possible_gp_samps_to_read = gp_samps_to_read;

		if(dz->mode != 1 && (gp_delaysamps < possible_gp_samps_to_read)) {
			overlap = 1;								//	Delays WILL (POSSIBLY) OVERLAP: write ALL into reprepbuf
			segbuf = reprepbuf;
			memset((char *)segbuf,0,dz->buflen2 * sizeof(float));
		} else {										//	Delays do not overlap, write 1 segment in repbuf
			segbuf = repbuf;
			overlap = 0;
		}												//	Zero the segment buffer
		memset((char *)repbuf,0,dz->buflen2 * sizeof(float));
		
		repbufpos = 0;									//	Write one repeat segment (at original pitch) to appropriate buffer

		for(n=0,m = gp_samps_to_read - 1;n < gp_samps_to_read; n++,m--) {
			if(n < gp_splicesamps) {
				if(at_start)
					val = 1.0;
				else
					val = (double)n/dgp_splicesamps;	//	Copy to relevant segment-buffer, with splice at start, if not at start of infile
			} else if (m < gp_splicesamps)
				val = (double)m/dgp_splicesamps;		//	and splice at end
			else
				val = 1.0;
			for(ch = 0;ch < chans; ch++) {
				segbuf[repbufpos] = (float)(ibuf[ibufpos] * val);
				repbufpos++;
				ibufpos++;
			}
			if(repbufpos >= dz->buflen2) {
				sprintf(errstr,"Input segment has overflowed segment buffer.\n");
				return PROGRAM_ERROR;
			}
			if(ibufpos >= 2 * dz->buflen) {
				if((exit_status = reset_ibuf_and_read(&ibufpos,dz))<0)
					return(exit_status);
			}
		}
		at_start = 0;	
														
		samps_to_read = gp_samps_to_read * chans;
		delaysamps	  = gp_delaysamps * chans;
		isshorten = 0;
		thisfade = 1.0;
		endspliceval = 1.0;
		if(overlap) {															//	Delays OVERLAP: write ALL into repbuf, from single copy in reprepbuf

			if(dz->mode == 2) {
				if(dz->param[REP_ACCEL] != 1.0) {
					lenchange = 1.0 - (1.0/dz->param[REP_ACCEL]);
					lenchangeincr = lenchange/(double)(repeats - 1);
					isshorten = 1;
				}
				inital_gp_delaysamps = gp_delaysamps;
				initial_gp_samps_to_read = gp_samps_to_read;
			}
			maxrepbufpos = 0;
			lastrepbufpos = 0;
			for(n=0;n<repeats;n++) {											//	Copy repeating segments into segment-buffer
				if(dz->mode == 2) {
					if(isshorten) {
						lenfact = 1.0 - pow(lenchangeincr * (double)n,dz->param[REP_WARP]);
						gp_delaysamps = (int)round(inital_gp_delaysamps * lenfact);
						delaysamps = gp_delaysamps * chans;
						gp_samps_to_read = (int)round(initial_gp_samps_to_read * lenfact);
						gp_endsplice = min(gp_samps_to_read,gp_splicesamps);
						gp_endsplice_stt = gp_samps_to_read - gp_endsplice;
						endsplice = gp_endsplice * chans;
						endsplice_stt = gp_endsplice_stt * chans;
						samps_to_read = gp_samps_to_read * chans;
					}
					thisfade = pow((double)(repeats - n)/(double)repeats,dz->param[REP_FADE]) ;
				}
				if(dz->param[REP_RAND] > 1.0) {
					val = dz->param[REP_RAND] - 1.0;							//	0 to maxval-1 (1-2 -> 0-1: 1-8 -> 0-7)
					rnd =  drand48();											//	0 to 1
					rnd *= val;													//	0 to 1*rnd OR 0 to 7*rnd (rnd < 1)
					rnd += 1.0;													//	1 to 2*rnd OR 1 to 8*rnd (rnd < 1)
					delaysamps = (int)round(gp_delaysamps * rnd) * chans;
				}
				repbufpos = lastrepbufpos + delaysamps;							//	Advancing by delay-time
				lastrepbufpos = repbufpos;
				endspliceval = 1.0;

				if(varypitch && (n > 0)) {										//	If there's pitch-variation
					val  = (drand48() * 2.0) - 1.0;								//	Get random value in range 0 to +- given semitone-range
					val *= dz->param[REP_TRNSP];								
					incr = pow(2.0,val/SEMITONES_PER_OCTAVE);					//	Convert to an increment for table read
					md = 0;
					while(md < gp_samps_to_read) {								//	Transpose repeated segment before copying segment buffer
						m = (int)floor(md);
						frac = md - (double)m;
						if(isshorten && (md > gp_endsplice_stt))
							endspliceval = 1.0 - ((md - (double)gp_endsplice_stt)/(double)gp_endsplice);
						for(ch=0,k=m*chans;ch<chans;ch++,k++) {
							val  = segbuf[k];
							diff = segbuf[k+chans] - val;
							val += diff*frac;
							if(dz->mode == 2)
								val *= thisfade*endspliceval;
							repbuf[repbufpos] = (float)(repbuf[repbufpos] + val);
							if(++repbufpos >= dz->buflen2) {
								sprintf(errstr,"segment buffer too short to contain repeated overlapping segments (1).\n");
								return PROGRAM_ERROR;		
							}
						}
						md += incr;
					}
					maxrepbufpos = max(maxrepbufpos,repbufpos);
				} else {
					for(m=0; m< samps_to_read;m++) {
						if(isshorten && (m > endsplice_stt))
							endspliceval = 1.0 - ((m - (double)endsplice_stt)/(double)endsplice);
						repbuf[repbufpos] = (float)(repbuf[repbufpos] + (segbuf[m] * thisfade * endspliceval));	//	Add repeating units back into segment buffer
						if(++repbufpos >= dz->buflen2) {
							sprintf(errstr,"segment buffer too short to contain repeated overlapping segments (1).\n");
							return PROGRAM_ERROR;		
						}
					}
					maxrepbufpos = max(maxrepbufpos,repbufpos);
				}
			}
			samps_to_write = maxrepbufpos;

			if((exit_status = normalise_buffer(samps_to_write,dz))<0)			//	Normalise the output in such a way
						return(exit_status);									//	that start and end normalisations are 1.0
		
			for(n = 0; n < samps_to_write; n++) { 									
				obuf[obufpos] = (float)(obuf[obufpos] + repbuf[n]);				//	Add whole set of repeating units to output
				if(++obufpos >= dz->buflen + total_splicesamps) {
					if((exit_status = write_and_reset_obuf(dz->buflen,&obufpos,dz))<0)
						return(exit_status);
				}
			}

		} else {	//	Delays will NOT overlap


			if(dz->mode == 2) {
				if(dz->param[REP_ACCEL] != 1.0) {
					lenchange = 1.0 - (1.0/dz->param[REP_ACCEL]);
					lenchangeincr = lenchange/(double)(repeats - 1);
					isshorten = 1;
				}
				inital_gp_delaysamps = gp_delaysamps;
				initial_gp_samps_to_read = gp_samps_to_read;
			}

			if(dz->mode == 1)													//	In mode 1
				startdelay = 0;													//	Delaytime starts at END of segment
			for(n=0;n<repeats;n++) {
				if(dz->mode == 2) {
					if(isshorten) {
						lenfact = 1.0 - pow(lenchangeincr * (double)n,dz->param[REP_WARP]);
						gp_delaysamps = (int)round(inital_gp_delaysamps * lenfact);
						gp_samps_to_read = (int)round(initial_gp_samps_to_read * lenfact);
						gp_endsplice = min(gp_samps_to_read,gp_splicesamps);
						gp_endsplice_stt = gp_samps_to_read - gp_endsplice;
						endsplice = gp_endsplice * chans;
						endsplice_stt = gp_endsplice_stt * chans;
						samps_to_read = gp_samps_to_read * chans;
					}
					thisfade = pow((double)(repeats - n)/(double)repeats,dz->param[REP_FADE]) ;
				}
				samps_written = 0;
				repbufpos = 0;
				endspliceval = 1.0;
				if(varypitch && (n > 0)) {										//	If there's pitch-variation
					val  = (drand48() * 2.0) - 1.0;	
					val *= dz->param[REP_TRNSP];						
					incr = pow(2.0,val/SEMITONES_PER_OCTAVE);	
					md = 0;
					while(md < gp_samps_to_read) {								//	Transpose repeated segment before copying to output buffer
						m = (int)floor(md);
						frac = md - (double)m;
						if(isshorten && (md > gp_endsplice_stt))
							endspliceval = 1.0 - ((md - (double)gp_endsplice_stt)/(double)gp_endsplice);
						for(ch=0,k=m*chans;ch<chans;ch++,k++) {
							val  = segbuf[k];
							diff = segbuf[k+chans] - val;
							val += diff*frac;
							if(dz->mode == 2)
								val *= thisfade*endspliceval;
							obuf[obufpos] = (float)(obuf[obufpos] + val);
							if(++obufpos >= dz->buflen + total_splicesamps) {	//	(startsplice overlaps with existing obuf data, so use "add")
								if((exit_status = write_and_reset_obuf(dz->buflen,&obufpos,dz))<0)
									return(exit_status);
							}
							samps_written++;
						}
						md += incr;
					}
				} else {
					while(repbufpos < samps_to_read) {
						if(isshorten && (repbufpos > endsplice_stt))
							endspliceval = 1.0 - ((repbufpos - (double)endsplice_stt)/(double)endsplice);
						if(dz->mode == 2)
							repbuf[repbufpos] = (float)(repbuf[repbufpos] * thisfade * endspliceval);	//	Do any fades or endsplicing
						obuf[obufpos] = (float)(obuf[obufpos] + repbuf[repbufpos++]);//	Add repeating unit to output
						if(++obufpos >= dz->buflen + total_splicesamps) {			//	(startsplice overlaps with existing obuf data, so use "add")
							if((exit_status = write_and_reset_obuf(dz->buflen,&obufpos,dz))<0)
								return(exit_status);
						}
						samps_written++;
					}
				}
				if(dz->param[REP_RAND] > 1.0) {
					val = dz->param[REP_RAND] - 1.0;
					rnd =  drand48();
					rnd *= val;
					rnd += 1.0;
					delaysamps = (int)round(gp_delaysamps * rnd) * chans;
				} else
					delaysamps = gp_delaysamps * chans;
				if(dz->mode != 1)													//	In modes 0 & 2
					startdelay = samps_written;										//	Delaytime starts at START of segment
				
				for(m=startdelay;m < delaysamps; m++) {
					if(++obufpos >= dz->buflen + total_splicesamps) {
						if((exit_status = write_and_reset_obuf(dz->buflen,&obufpos,dz))<0)
							return(exit_status);
					}
				}
			}
		}
		obufpos -= total_splicesamps;					//	Baktrack by splicelen in output;
		ibufpos -= total_splicesamps;					//	Restore ibufpos to true end-of-segment time, ready for next read.
			
		last_gp_absendsamp = gp_absendsamp;				//	Set sample position of end of segment read
	}

	bufpos_in_iovflw = ibufpos - dz->buflen;			//	ibufpos is normally in iovflwbuf (>= dz->buflen) unless it's baktracked				
	if(bufpos_in_iovflw < dz->ssampsread) {				//	If there are still input samples remaining to be read (these are always in iovflw)
		n = 0;
		while(bufpos_in_iovflw < dz->ssampsread) {
			if(n < gp_splicesamps)
				val = (double)n/dgp_splicesamps;		//	Copy to repeat-buffer, with splices at start
			else 
				val = 1.0;
			for(ch = 0;ch < chans; ch++) {
				obuf[obufpos] = (float)(obuf[obufpos] + (ibuf[ibufpos] * val));
				obufpos++;
				ibufpos++;
				bufpos_in_iovflw++;
			}
			if(obufpos >= dz->buflen + total_splicesamps) {
				if((exit_status = write_and_reset_obuf(dz->buflen,&obufpos,dz))<0)
					return(exit_status);
			}
			if(ibufpos >= 2 * dz->buflen) {
				if((exit_status = reset_ibuf_and_read(&ibufpos,dz))<0)
					return(exit_status);
				bufpos_in_iovflw = 0;
			}
			n++;
		}
	} else
		obufpos += total_splicesamps;					//	Restore obufpos to its true position

	dz->process = GREV;
	if((exit_status = write_samps(obuf,obufpos,dz))<0)
		return(exit_status);
	dz->process = REPEATER;
	return FINISHED;
}

/**************************** HANDLE_THE_SPECIAL_DATA ****************************/

int handle_the_special_data(char *str,double *maxseglen,double *maxovlp,dataptr dz)
{
	int n, k, cnt, curtail, idummy, linecnt, warned = 0, chans = dz->infile->channels;
	FILE *fp;
	char temp[200], *p;
	double dummy = 0, lasttime, lastendtime = 0.0, seglen, srate = (double)dz->infile->srate;
	double splicelen, twosplicelen, lastdur = 0.0;
	double max_ovlpbuf;									//	Finds size of buffer needed for any overlapping sets of delayed segments
	
	int splicesamps, repeats = 0;

	*maxovlp = 0.0;
	splicelen    = REPSPLEN * MS_TO_SECS;				//	Find minimum permissible size of segments == >two-splicelengths
	splicesamps  = (int)ceil(splicelen * srate);
	splicelen = (double)(splicesamps + chans)/srate;	//	Add a sample (for each chan) for splicelen, for safety
	twosplicelen = (double)((splicesamps + chans) * 2)/srate;

	if((fp = fopen(str,"r"))==NULL) {
		sprintf(errstr,"Cannot open file \"%s\" to read repeater data.\n",str);
		return(DATA_ERROR);
	}
	cnt = 0;
	lasttime = 0.0;
	curtail = 0;
	linecnt = 0;
	while(fgets(temp,200,fp)!=NULL) {
		p = temp;
		while(isspace(*p))
			p++;
		if(*p == ';' || *p == ENDOFSTR)	//	Allow comments in file
			continue;
		cnt = 0;
		while(get_float_from_within_string(&p,&dummy)) {
			if(curtail == 1)
				break;					//	if in midst of current seg (cnt > 0), but fell off end of data, break now before linecnt is incremented
			k = cnt % 4;
			if(k == 0 && curtail == 2)	//	if at start of a new seg (cnt == 0), but we reached end of file in last segment, break now
				break;
			switch(k) {
			case(0):
				if(dummy < 0.0) {
					sprintf(errstr,"Segment start-time (%lf) less than zero at line %d in file \"%s\".\n",dummy,linecnt+1,str);
					return DATA_ERROR;
				}
				if(dummy >= dz->duration - splicelen) {
					if(linecnt == 0) {
						sprintf(errstr,"1st segment start-time (%lf) close to or > infile-end (%lf) in file \"%s\".\n",dummy,dz->duration,str);
						return DATA_ERROR;
					} else {
						fprintf(stdout,"WARNING: line %d in file \"%s\" : start-time (%lf) close to or > infile-end (%lf).\n",linecnt+1,str,dummy,dz->duration);
						fprintf(stdout,"WARNING: Ignoring repetition-data at and beyond this time in file \"%s\".\n",str);
						fflush(stdout);
						curtail = 1;
					}
				}
				lasttime = dummy;
				break;
			case(1):
				if(dummy - lasttime <= twosplicelen) {
					sprintf(errstr,"Segment on line %d in file \"%s\", dur %lf, too short for splicing (min dur %lf).\n",linecnt+1,str,dummy - lasttime,twosplicelen);
					return DATA_ERROR;
				}
				if(dummy >= dz->duration) {
					if(dz->duration - lasttime <= twosplicelen) {
						fprintf(stdout,"WARNING: Segment on line %d in file \"%s\", ends after infile-end and is hence too short.\n",linecnt+1,str,twosplicelen);
						fprintf(stdout,"WARNING: Ignoring this and later segments.\n");
						fflush(stdout);
						curtail = 1;				//	cnt is complete after end of previous viable segment
						break;
					} else {
						fprintf(stdout,"WARNING: line %d in file \"%s\" : segment end-time (%lf) beyond infile-end (%lf).\n",linecnt+1,str,dummy,dz->duration);
						fprintf(stdout,"WARNING: Curtailing segment to finish at end of src-file (and ignoring any subsequent segments).\n");
						fflush(stdout);
						curtail = 2;				//	read rest of this seg, but ignore any further segs
					}
				}
				lastendtime = dummy;
				lastdur = dummy - lasttime;
				break;
			case(2):
				idummy = (int)round(dummy);
				if(dummy != (double)idummy) {
					sprintf(errstr,"Non-integer repeat value on line %d  in file \"%s\".\n",linecnt+1,str);
					return DATA_ERROR;
				}
				if(idummy < 2 && idummy != 0) {
					sprintf(errstr,"Repeat value less than 2 on line %d in file \"%s\".\n",linecnt+1,str);
					return DATA_ERROR;
				}
				repeats= idummy;
				break;
			case(3):
				switch(dz->mode) {
				case(0):
				case(2):
					if(dummy < REPMINDEL && dummy != 0.0) {
						if(!warned) {
							fprintf(stdout,"WARNING: (Non-zero) Delay (%.3lf) <= %.3lf on line %d in file \"%s\".\n",dummy,REPMINDEL,linecnt+1,str);
							fprintf(stdout,"WARNING: This may produce unexpected output, like an oscillator.\n");
							fflush(stdout);																//	---------------------------
							warned = 1;
						}
					}																				// |   |---------------------------
					if(dummy < lastdur + splicelen) {				//	If delayed repeats overlap	// delay   |---------------------------
						max_ovlpbuf = (dummy * repeats) + lastdur + splicelen;						// |   |   |   |---------------------------	__
						*maxovlp = max(*maxovlp,max_ovlpbuf);		//	Remember maxoverlap dur		// |           |	          			   |  |
					}												//	For bufsize calculations.	// |delay*rpts |	+ (last)dur			   |+splicelen
					break;
				case(1): //	Delayed repeats never overlap.
					break;
				}
				break;
			default:
				sprintf(errstr,"Too many values (%d) on line %d in file \"%s\": Need only 4.\n",cnt,linecnt+1,str);
				return DATA_ERROR;
			}
			cnt++;
		}
		if(cnt < 4) {
			sprintf(errstr,"Too few values (%d) on line %d in file \"%s\": Need 4.\n",cnt,linecnt+1,str);
			return DATA_ERROR;
		}
		linecnt++;
	}
	if(linecnt == 0) {
		sprintf(errstr,"No viable repetition data found in file \"%s\".\n",str);
		return(DATA_ERROR);
	}
	dz->itemcnt = linecnt * 4;

	if((dz->parray = (double **)malloc(2 * sizeof(double *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create Repetition data array.\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[0] = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create Repetition data array.\n");
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
		while(get_float_from_within_string(&p,&dummy))
			dz->parray[0][cnt++] = dummy;
		if(cnt >= dz->itemcnt)
			break;
	}
	lastendtime = dz->parray[0][dz->itemcnt - 3];
	if(lastendtime > dz->duration)
		dz->parray[0][dz->itemcnt - 3] = dz->duration;
	*maxseglen = 0.0;
	for(n = 0; n < dz->itemcnt;n+=4) {
		seglen = dz->parray[0][n+1] - dz->parray[0][n];
		*maxseglen = max(seglen,*maxseglen);
	}
	return FINISHED;
}

/******************************** CREATE_REPEATER_SNDBUFS ********************************/

int create_repeater_sndbufs(double maxseglen, double maxovlp, dataptr dz)
{
	int exit_status, chans = dz->infile->channels;
	int bigbufsize, secsize, maxovlpsamps;
	int framesize = F_SECSIZE * chans;
	double maxrand = 0.0, maxtransp = 0.0, maxtranspdn, maxexpand = 1.0, srate = (double)dz->infile->srate;
	int splicespace;

	if(dz->sbufptr == 0 || dz->sampbuf == 0) {
		sprintf(errstr,"buffer pointers not allocated: create_sndbufs()\n");
		return(PROGRAM_ERROR);
	}
	splicespace = (int)ceil(REPSPLEN * MS_TO_SECS * srate) * chans;//	Allow for inbuf splice-backtraks and splice-beyond-endtime as segments cut

	//	BUFLEN2 to contain segments for repeating
	if(dz->brksize[REP_RAND]) {											
		if((exit_status = get_maxvalue_in_brktable(&maxrand,REP_RAND,dz))<0)
			return exit_status;
	} else if(dz->param[REP_RAND] > 1.0)							//	Start with the max seglen to handle overlapping segments
		maxrand = dz->param[REP_RAND];
	if(maxrand > 1.0)												//	Allow for maximum possible expansion of "overlapping" segs, by random expansion!!
		maxovlp *= maxrand;									
	if(dz->brksize[REP_TRNSP]) {											
		if((exit_status = get_maxvalue_in_brktable(&maxtransp,REP_TRNSP,dz))<0)
			return exit_status;
	} else															//	Find the maximum transposition
		maxtransp = dz->param[REP_TRNSP];
	if(maxtransp > 0.0) {											//	If segments are transposed, they could be transposed downwards (and therefore be longer)
		maxtranspdn = pow(2.0,-maxtransp/SEMITONES_PER_OCTAVE);		//	Convert semitones to frq-ratio for fownwarfd transposition.
		maxexpand = 1.0/maxtranspdn;								//	Transposing down an 8va (frq ratio 1/2) makes sound 2 * longer, so take reciprocal
		maxovlp *= maxexpand;										//	Increase length of overlap-buffer by this amount as all segs could be expanded and overlap
		maxseglen *= maxexpand;										//	Increase length of single-segment-buffer by this amount
	}
	maxovlpsamps  = (int)ceil(maxovlp * srate) * chans;
	dz->buflen2 = (int)ceil(maxseglen * srate) * chans;			//	Size = max seglen
	dz->buflen2 = max(dz->buflen2,maxovlpsamps);					//	Or = max length of any overlapping repeats captured in segment buffer
	dz->buflen2 += splicespace * 2;									//	Must be large enough to fit and splice area at end of segment

	bigbufsize = (int)Malloc(-1);
	dz->buflen = bigbufsize/sizeof(float);							//	dz->buflen2 will accomodate largest cut-segment
	dz->buflen = max(dz->buflen,dz->buflen2) + (splicespace * 2);	//	must be large enough to fit largest cut-segment and splice baktrak
	secsize = dz->buflen/framesize;
	if(secsize * framesize != dz->buflen)
		secsize++;
	dz->buflen = secsize * framesize;
	secsize = dz->buflen2/framesize;
	if(secsize * framesize != dz->buflen2)
		secsize++;
	dz->buflen2 = secsize * framesize;

	if(dz->buflen <= 0) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create input and output sound buffers.\n");
		return(PROGRAM_ERROR);
	}
	if(dz->buflen2 <= 0) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create delay-segments sound buffer.\n");
		return(PROGRAM_ERROR);
	}
	bigbufsize = ((dz->buflen * 4) + (dz->buflen2 * 2)) * sizeof(float);
	if((dz->bigbuf = (float *)malloc(bigbufsize)) == NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create total sound buffers.\n");
		return(PROGRAM_ERROR);
	}
	dz->sbufptr[0] = dz->sampbuf[0] = dz->bigbuf;					//	Inbuf
	dz->sbufptr[1] = dz->sampbuf[1] = dz->sampbuf[0] + dz->buflen;	//	Inbuf overflow
	dz->sbufptr[2] = dz->sampbuf[2] = dz->sampbuf[1] + dz->buflen;	//	Outbuf
	dz->sbufptr[3] = dz->sampbuf[3] = dz->sampbuf[2] + dz->buflen;	//	Ovflwbuf
	dz->sbufptr[4] = dz->sampbuf[4] = dz->sampbuf[3] + dz->buflen;	//	segment-store
	dz->sbufptr[5] = dz->sampbuf[5] = dz->sampbuf[4] + dz->buflen2;	//	Repeated-segment-store
	dz->sampbuf[6] = dz->sampbuf[5] + dz->buflen2;
	return(FINISHED);
}

/******************************** WRITE_AND_RESET_OBUF ********************************/

int write_and_reset_obuf(int samps_to_write,int *obufpos,dataptr dz)
{
	int exit_status;
	float *obuf = dz->sampbuf[2],  *ovflwbuf = dz->sampbuf[3];

	dz->process = GREV;
	if((exit_status = write_samps(obuf,samps_to_write,dz))<0)
		return(exit_status);
	dz->process = REPEATER;
	memset((char *)obuf,0,dz->buflen * sizeof(float));
	memcpy((char *)obuf,(char *)ovflwbuf,dz->buflen * sizeof(float));
	memset((char *)ovflwbuf,0,dz->buflen * sizeof(float));
	*obufpos -= dz->buflen;
	return FINISHED;
}

/******************************** RESET_IBUF_AND_READ ********************************/

int reset_ibuf_and_read(int *ibufpos,dataptr dz)
{
	int exit_status;
	float *ibuf = dz->sampbuf[0], *iovflwbuf = dz->sampbuf[1];
	memcpy((char *)ibuf,(char *)iovflwbuf,dz->buflen * sizeof(float));	//	Copy input overflow back into ibuf
	memset((char *)iovflwbuf,0,dz->buflen * sizeof(float));				//	Set overflow to zero
	if((exit_status = read_samps(iovflwbuf,dz))<0)						//	Read into overflow
		return(exit_status);
	*ibufpos = dz->buflen;												//	Reset ibufpos to start of iovflwbuf
	return FINISHED;
}

/******************************** NORMALISE_BUFFER ********************************/

int normalise_buffer(int samplen,dataptr dz)
{
	double *env = dz->parray[1], maxsamp, thiseval, nexteval, diff, eval;
	float *buf = dz->sampbuf[4];
	int *loc = dz->iparray[0], chans = dz->infile->channels, ch;
	int e, m, k, envsize, maxloc, windowstart, thispos, goalpos, samppos, gap, win_in_buf;
	int needs_enveloping = 0, ethis, enext, done, shortwindow = 0;
	int halfwindow = dz->envwindowlen/2;
	memset((char *)env,0,dz->arraysize * sizeof(double));
	win_in_buf = samplen/dz->envwindowlen;						//	Number of complete windows in buf
	shortwindow  = samplen - (win_in_buf * dz->envwindowlen);	//	Length of any short window
	
	//	To force the final window to be full-length, we will make the penultimate window the short window
	samppos = 0;
	do {
		samppos = 0;
		for(e = 0; e < win_in_buf;e++) {						//	For all the normalisable samples, advance by windowlen blocks			
			maxsamp = 0.0;
			maxloc = 0;
			for(m=0,k=samppos;m < dz->envwindowlen;m++,k++) {	//	In each normal window, find the maxsamp
				if(fabs(buf[k]) > maxsamp) { 
					maxsamp = fabs(buf[k]);
					maxloc = m;
				}
			}
			samppos += dz->envwindowlen;
			if(e >= dz->arraysize) {
				sprintf(errstr,"envelope arraysize exceeded.\n");
				return PROGRAM_ERROR;
			}
			env[e] = maxsamp;									//	And store the envelope val
			loc[e] = (maxloc/chans) * chans;					//	And position of maximum to chan-grp boundary
		}
		if(shortwindow) {
			maxsamp = 0.0;
			maxloc  = 0;
			for(m=0,k=samppos;m < shortwindow;m++,k++) {
				if(fabs(buf[k]) > maxsamp) { 
					maxsamp = fabs(buf[k]);
					maxloc = m;
				}
			}
			loc[e] = (maxloc/chans) * chans;
			e++;
		}
		envsize = e;
		needs_enveloping = 0;
		for(e = 0;e < envsize;e++) {							//	Check where signal exceeds max (REPCLIP)
			if(env[e] > REPCLIP) {								//	and force (re-)envelope to reduce level here
				env[e] = REPCLIP/env[e];
				needs_enveloping = 1;							//	AND note the re-envelopeing is necessary
			} else												//	otherwise leave envelope level at 1.0 (no change)	
				env[e] = 1.0;
		}
		if(needs_enveloping) {									//	If enveloping required
			if(env[0] < 1.0) {									//	If 1st window overloads, do a presmooth
				for(samppos=0;samppos < loc[0];samppos++)
					buf[samppos] = (float)(buf[samppos] * env[0]);
			}
			if(env[envsize-1] < 1.0) {							//	If last window overloads, do a presmooth
				thispos = loc[envsize-1] + (dz->envwindowlen * (envsize-2));
				goalpos = samplen;
				for(samppos=thispos;samppos < goalpos;samppos++)
					buf[samppos] = (float)(buf[samppos] * env[envsize-1]);
			}
			ethis = -1;											//	Interpolate the re-envelope vals, in order to envelope the src, in situ
			enext = 0;
			done = 0;
			for(windowstart = 0; windowstart < samplen; windowstart+=dz->envwindowlen) {
				ethis++;
				enext++;
				thiseval = env[ethis];
				nexteval = env[enext];
				if(thiseval < 1.0 && nexteval == 1.0) {
					thispos = windowstart + loc[ethis];			//	Interp from maximum in this-window to middle of non-normalised next-window
					goalpos = windowstart + dz->envwindowlen + halfwindow;
				} else if(thiseval == 1.0 && nexteval < 1.0) {
					thispos = windowstart + halfwindow;			//	Interp from middle of non-normalised this-window to maximum in next
					if(enext >= envsize)
						goalpos = samplen;
					else
						goalpos = windowstart + dz->envwindowlen + loc[enext];
				} else if(thiseval < 1.0 && nexteval < 1.0) {
					thispos = windowstart + loc[ethis];			//	Interp from max in this window to max in next
					if(enext >= envsize)
						goalpos = samplen;
					else
						goalpos = windowstart + dz->envwindowlen + loc[enext];
				} else { // (thiseval == 1.0 && nexteval == 1.0) do nothing
					continue;
				}
				samppos = thispos;
				gap = (goalpos - thispos)/chans;
				diff = nexteval - thiseval;
				for(m=0;m < gap;m++) {
					if(samppos >= dz->buflen) {
						done = 1;
						break;
					}
					eval = (double)m/(double)gap;
					eval *= diff;
					eval += thiseval;
					for(ch= 0;ch < chans;ch++) {
						buf[samppos] = (float)(buf[samppos] * eval);
						samppos++;
					}
				}
				if(done)
					break;
			}
		}
	} while(needs_enveloping);									//	Do this recursively until nothing is too loud	
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

/****************************** CALC_OUTPUT_DUR *********************************/

int calc_output_dur(int *dursamps,dataptr dz)
{
	int exit_status, chans = dz->infile->channels;
	double *segdata =  dz->parray[0];
	double stttime, endtime, repeats, delay, seglen, repsdur = 0.0, advance;
	double lastendtime = 0.0, totaldur = 0.0, maxrand = 1.0, srate = (double)dz->infile->srate;
	int n, m;
	if(dz->brksize[REP_RAND]) {											
		if((exit_status = get_maxvalue_in_brktable(&maxrand,REP_RAND,dz))<0)
			return exit_status;
	} else if(dz->param[REP_RAND] > 1.0)
		maxrand = dz->param[REP_RAND];
	for(n = 0; n < dz->itemcnt;n+=4) {
		m = n;
		stttime = segdata[m++];
		endtime = segdata[m++];
		repeats = segdata[m++];
		delay   = segdata[m++];

		if((advance = stttime - lastendtime) > 0.0)		//	If we advance in input
			totaldur += advance;						//	add duration of advance-step to total output duration

		seglen  = endtime - stttime;
		switch(dz->mode) {
		case(0):										//	Find approx duration covered by repeats of segment
		case(2):
			repsdur = (repeats * delay) + seglen;
			break;
		case(1):
			repsdur = (seglen + delay) * repeats;
			break;
		}
		if(maxrand > 1.0)								//	Allow for max possible randomisation-increase
			repsdur *= maxrand;									
		totaldur += repsdur;							//	and add to total output duration
		lastendtime = endtime;
	}
	if((advance = dz->duration - lastendtime) > 0.0)	//	IF not yet at end of file
		totaldur += advance;							//	add duration of step to end-of-file to total output dur


	*dursamps = (int)round(totaldur * srate) * chans;
	return FINISHED;
}
