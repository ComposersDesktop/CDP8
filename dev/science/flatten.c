//	HEREH : ADD PARAMETER TO AVOID int DECAY TAIL BEING FLATTENED
//	compile and test on files on loom

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

#ifdef unix
#define round(x) lround((x))
#endif


#define WINS_PER_ELEMENT	(3)		//	No of envelope windows per element-to-find
#define	MAXLEV				(0.95)	//	Maximum output level

char errstr[2400];

int anal_infiles = 1;
int	sloom = 0;
int sloombatch = 0;

const char* cdp_version = "6.1.0";

//CDP LIB REPLACEMENTS
static int check_flatten_param_validity_and_consistency(dataptr dz);
static int setup_flatten_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_flatten_param_ranges_and_defaults(dataptr dz);
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
static int create_flatten_sndbufs(dataptr dz);
static int flatten_param_preprocess(dataptr dz);
static int is_phase_change(float *ibuf,int *all_zero,int *phasechange,int wstart,int wend);
static int find_phasechange_position(float *ibuf,int *zcrospos,int wstart,int wend);
static int flatten(dataptr dz);

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
		dz->maxmode = 0;
		// setup_particular_application =
		if((exit_status = setup_flatten_application(dz))<0) {
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
	if((exit_status = setup_flatten_param_ranges_and_defaults(dz))<0) {
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
//	handle_special_data()		redundant
 
	if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {		// CDP LIB
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
//	check_param_validity_and_consistency....
	if((exit_status = check_flatten_param_validity_and_consistency(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	is_launched = TRUE;
	dz->bufcnt = 2;
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

	if((exit_status = create_flatten_sndbufs(dz))<0) {							// CDP LIB
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	//param_preprocess ...
	if((exit_status = flatten_param_preprocess(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	//spec_process_file =
	if((exit_status = flatten(dz))<0) {
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

/************************* SETUP_FLATTEN_APPLICATION *******************/

int setup_flatten_application(dataptr dz)
{
	int exit_status;
	aplptr ap;
	if((exit_status = establish_application(dz))<0)		// GLOBAL
		return(FAILED);
	ap = dz->application;
	// SEE parstruct FOR EXPLANATION of next 2 functions
	if((exit_status = set_param_data(ap,0   ,2,2,"dd"))<0)
		return(FAILED);
	if((exit_status = set_vflgs(ap,"t",1,"d","",0,0,""))<0)
		return(FAILED);
	// set_legal_infile_structure -->
	dz->has_otherfile = FALSE;
	// assign_process_logic -->
	dz->input_data_type = SNDFILES_ONLY;
	dz->process_type	= EQUAL_SNDFILE;	
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
		} else if(infile_info->channels != MONO)  {
			sprintf(errstr,"File %s is not a mono soundfile\n",cmdline[0]);
			return(DATA_ERROR);
		} else if((exit_status = copy_parse_info_to_main_structure(infile_info,dz))<0) {
			sprintf(errstr,"Failed to copy file parsing information\n");
			return(PROGRAM_ERROR);
		}
		free(infile_info);
	}
	return(FINISHED);
}

/************************* SETUP_FLATTEN_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_flatten_param_ranges_and_defaults(dataptr dz)
{
	int exit_status;
	aplptr ap = dz->application;
	// set_param_ranges()
	ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
	// NB total_input_param_cnt is > 0 !!!
	if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
		return(FAILED);
	// get_param_ranges()
	ap->lo[0]	= 0.001;
	ap->hi[0]	= 100.0;
	ap->default_val[0]	= 0.1;
	ap->lo[1]	= 20.0;
	ap->hi[1]	= 50000.0;
	ap->default_val[1]	= 20.0;
	ap->lo[2]	= 0.0;
	ap->hi[2]	= dz->duration;
	ap->default_val[2]	= 0.0;
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
			if((exit_status = setup_flatten_application(dz))<0)
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
	usage2("flatten");
	return(USAGE_ONLY);
}

/**************************** CHECK_FLATTEN_PARAM_VALIDITY_AND_CONSISTENCY *****************************/

int check_flatten_param_validity_and_consistency(dataptr dz)
{
	if(dz->param[0] * 2 > dz->duration) {
		sprintf(errstr,"Elementsize (%lf) is too large for input source (duration %lf).\n",dz->param[0],dz->duration);
		return(DATA_ERROR);
	}
	if(dz->param[2] >= dz->duration) {
		sprintf(errstr,"Tail (%lf) is too large for input source (duration %lf).\n",dz->param[2],dz->duration);
		return(DATA_ERROR);
	}
	return FINISHED;
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
	if(!strcmp(prog_identifier_from_cmdline,"flatten"))				dz->process = FLATTEN;
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
	if(!strcmp(str,"flatten")) {
		fprintf(stderr,
	    "USAGE:\n"
	    "flatten flatten infile outfile elementsize shoulder [-ttail]\n"
		"\n"
		"Equalise level of sound elements in mono src.\n"
		"\n"
		"ELEMENTSIZE Approx size of elements (e.g. syllables) in src.\n"
		"            (Range 0.001 to 100).\n"
		"SHOULDER    Risetime in segment to changed level (mS).\n"
		"            (Range 20 to ELEMENTSIZE/2).\n"
		"            Will never be longer than distance from seg edge to peak.\n"
		"TAIL        Portion of end of sound to be treated as a whole segment.\n"
		"            (Range 0 to < duration).\n"
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

/******************************** GATE ********************************/

int flatten(dataptr dz)
{
	int exit_status, done, all_zero, phasechange, sub_all_zero, sub_phasechange, minwindow, passno;
	double srate = (double)dz->infile->srate, maxval, minval, lastenv, maxsamp, maxmax;
	double magprechange = 0.0, magpostchange = 0.0, incr, mag, lastmag = 0.0, max_samp, local_max_samp, val, normaliser = 1.0;
	float *ibuf = dz->sampbuf[0], *obuf = dz->sampbuf[1];
	int bufpos = 0, n, m, k, j, wsize, bigger_wsize, this_wsize, subwsize, finalsubwsize, this_subwsize;
	int envcnt, trofcnt, nutrofcnt, trofpos, startsamp, endsamp, sub_startsamp, last_startsamp, last_endsamp, zcrospos;
	int dove, maxat, seglen, peaksamp, samps_to_peak, samps_from_peak, doveup, dovedn, tail;
	double *env = dz->parray[0], *maxamp;
	int *trof = dz->lparray[0], *peakloc;

	dz->tempsize = dz->insams[0];

	dove  = (int)round(dz->param[1] * MS_TO_SECS * srate);		//	level change time, in samples
	wsize = (int)round((dz->param[0]/(double)WINS_PER_ELEMENT) * srate);
	tail  = (int)round(dz->param[2] * srate);
	tail  = dz->insams[0] - tail;

	maxval = 0.0;
	envcnt = 0;
	bufpos = 0;
	env = dz->parray[0];
	if((exit_status = read_samps(ibuf,dz))<0)
		return(exit_status);

	//	Read envelope at relevant elementsize
	
	done = 0;
	n = 0;
	k = 0;
	this_wsize = wsize;
	while(dz->ssampsread > 0) {
		while(n < this_wsize) {
			if(fabs(ibuf[bufpos]) >  maxval)
				maxval = fabs(ibuf[bufpos]);
			if(++bufpos >= dz->ssampsread) {
				if((exit_status = read_samps(ibuf,dz))<0)
					return(exit_status);
				if(dz->ssampsread == 0) {
					done = 1;
					break;
				}
				bufpos = 0;
			}
			n++;
			k++;
		}
		if(done)
			break;
		env[envcnt++] = maxval;
		n = 0;
		maxval = 0.0;
		if(k + wsize > tail)
			this_wsize = dz->insams[0] - k;
	}
	if(n)		//	If part-window at end, save envelope value for it
		env[envcnt++] = maxval;

	//	Find troughs: at this stage the trof value is the envelope-cnt number

	trofcnt = 0;
	trof[trofcnt++] = 0;
	lastenv = -1.0;

	done = 0;
	n = 0;
	while(!done) {
		while(env[n] >= lastenv) {
			lastenv = env[n];
			if(++n >= envcnt) {
				done = 1;
				break;
			}
		}
		if(done)
			break;
		while(env[n] <= lastenv) {
			lastenv = env[n];
			if(++n >= envcnt) {
				done = -1;
				break;
			}
		}
		if(done)
			break;
		trof[trofcnt++] = n;		//	Remember position of trof in envelope
	}
	if(done < 0)					//	If we were descending in level at end of envelope
		trof[trofcnt++] = n-1;		//	Last env value is a trof

	//	For each trof, find the true minimum position

	nutrofcnt = 0;
	for(n = 0; n < trofcnt;n++) {
		trofpos = trof[n] * wsize;
		sndseekEx(dz->ifd[0],trofpos,0);
		startsamp = 0;
		if(n == trofcnt - 1 && tail > 0)
			endsamp = dz->insams[0];
		else
			endsamp = wsize;
		bigger_wsize = wsize;
		bufpos = 0;
		if((exit_status = read_samps(ibuf,dz)) < 0)						//	NB buflen > wsize
		   return(exit_status);
		all_zero = 0;
		phasechange = 0;
		if(!is_phase_change(ibuf,&all_zero,&phasechange,startsamp,endsamp))		//	If larger window has no phase-change
			continue;															//	no zero-crossing here, so no zero-trof, continue
																		
		//	recursively split window into 3 and find minimum window				//	Otherwise MUST be a phasechange in some subwindow!!	

		sub_all_zero = 0;
		last_startsamp = -1;
		last_endsamp = -1;
		while(bigger_wsize > WINS_PER_ELEMENT * 2) {
			all_zero = 0;
			phasechange = 0;
			subwsize = (int)round(bigger_wsize/(double)WINS_PER_ELEMENT);		//	Get next smaller window, (1/3 of larger window)
																				//	There may not be an exact number of subwindows within the window, 
			finalsubwsize = bigger_wsize - (subwsize * (WINS_PER_ELEMENT - 1));	//	so final subwindow must be length-adjusted
			this_subwsize = subwsize;
			minval = HUGE;
			minwindow = -1;
			for(k = 0; k < WINS_PER_ELEMENT; k++) {								//	For all subwindows
				if(k == WINS_PER_ELEMENT - 1)
					this_subwsize = finalsubwsize;
				sub_all_zero = 0;
				sub_phasechange = 0;
				sub_startsamp = startsamp + (k * subwsize);
				if(!is_phase_change(ibuf,&sub_all_zero,&sub_phasechange,sub_startsamp,sub_startsamp + this_subwsize)) {
					if(sub_all_zero) {											//	IF any subwindow is all_zeros, use this as minimum
						minwindow = k;											//	 and quit			
						break;
					} else														//	Otherwise, if no phase-change in window
						continue;												//	no zero-crossing to do cut, so ignore this window
				}
				maxval = 0.0;
				for(j = 0; j < this_subwsize; j++) {
					if(fabs(ibuf[bufpos]) >  maxval)
						maxval = fabs(ibuf[bufpos]);							//	Find maxval in each subwindows
					if(++bufpos >= dz->buflen) {
						sprintf(errstr,"Error in buffer accounting for subwindows.\n");
						return PROGRAM_ERROR;
					}
				}
				if(maxval < minval) {											//	Find the minimum of these maxima
					minval = maxval;											//	To find trof window
					minwindow = k;
				}
			}
			if(minwindow < 0) {
				sprintf(errstr,"Minimum window not set.\n");
				return PROGRAM_ERROR;
			}
			last_startsamp = startsamp;											//	Remember edges of this window
			last_endsamp   = endsamp;
			startsamp += minwindow * subwsize;									//	Startsamp for nextpass = start of subwindow with min level
			if(minwindow == WINS_PER_ELEMENT - 1)
				endsamp = startsamp + finalsubwsize;
			else
				endsamp = startsamp + subwsize;
			bufpos = startsamp; 
			bigger_wsize = endsamp - startsamp;									//	Now make the subwindow the window to be subdivided further
		}
		if(sub_all_zero)														//	If min window is ALL zero
			trof[nutrofcnt++] = trofpos + (startsamp + endsamp)/2;				//	place trof in middle of window
		else {																	//	else there must be a phasechange in the min window
			if(last_startsamp < 0) {
				sprintf(errstr,"last_startsamp not set.\n");
				return PROGRAM_ERROR;
			}
			if((exit_status = find_phasechange_position(ibuf,&zcrospos,last_startsamp,last_endsamp))< 0)
				return exit_status;
			trof[nutrofcnt++] = trofpos + zcrospos;
		}
	}
	trofcnt = nutrofcnt;

	if((maxamp = (double *)malloc(trofcnt * sizeof(double)))==NULL) {
		sprintf(errstr,"Insufficient memory to store amplitudes of segments.\n");
		return(MEMORY_ERROR);
	}
	if((peakloc = (int *)malloc(trofcnt * sizeof(int)))==NULL) {
		sprintf(errstr,"Insufficient memory to store peak locations of segments.\n");
		return(MEMORY_ERROR);
	}

	//	Find maxima between trofs

	sndseekEx(dz->ifd[0],0,0);
	if((exit_status = read_samps(ibuf,dz)) < 0)
	   return(exit_status);
	startsamp = 0;
	bufpos = 0;
	for(n = 0; n < trofcnt; n++) {
		if(n == trofcnt - 1)
			endsamp = dz->insams[0];
		else
			endsamp = trof[n+1];
		maxsamp = 0.0;
		maxat = startsamp;
		for(k = startsamp; k < endsamp;k++) {
			if(fabs(ibuf[bufpos]) > maxsamp) {
				maxsamp = fabs(ibuf[bufpos]);
				maxat = k;
			}
			if(++bufpos >= dz->ssampsread) {
				if((exit_status = read_samps(ibuf,dz)) < 0)
				   return(exit_status);
				if(dz->ssampsread == 0)
					break;
				bufpos = 0;
			}
		}
		maxamp[n]  = maxsamp;
		peakloc[n] = maxat;
		startsamp = endsamp;
	}

	//	Find loudest segment

	maxmax = maxamp[0];
	for(n=1;n<trofcnt;n++) {
		if(maxamp[n] > maxmax)
			maxmax = maxamp[n];
	}

	//	Get normalisers for all segments

	for(n = 0; n < trofcnt;n++) {
		if(flteq(maxamp[n],0.0))
			maxamp[n] = 1.0;
		else 
			maxamp[n] = maxmax/maxamp[n];	
	}
	//	Flatten the source

	for(passno = 0;passno < 3; passno++) {
//		if(sloom) {
			switch(passno) {
			case(0):
				fprintf(stdout,"INFO: Adjusting segment levels.\n");
				break;
			case(1):
				fprintf(stdout,"INFO: Adjusting overall level.\n");
				break;
			case(2):
				fprintf(stdout,"INFO: Writing output.\n");
				break;
			}
			fflush(stdout);
//		}
		dz->total_samps_written = 0;
		sndseekEx(dz->ifd[0],0,0);
		if((exit_status = read_samps(ibuf,dz)) < 0)
		   return(exit_status);
		startsamp = 0;
		bufpos = 0;
		max_samp = 0.0;

		for(n = 0; n < trofcnt; n++) {
			if(n > 0)
				magprechange = maxamp[n] - maxamp[n-1]; 
			if(n < trofcnt - 1) {
				magpostchange = maxamp[n+1] - maxamp[n]; 
				endsamp = trof[n+1];
			} else {
				endsamp = dz->insams[0];
				magpostchange = 0.0;
			}
			seglen = endsamp - startsamp;
			peaksamp = peakloc[n];
			samps_to_peak = peaksamp - startsamp;
			doveup = min(dove,samps_to_peak);					//	Length of transition from previous amp boost
			samps_from_peak = endsamp - peaksamp;
			dovedn = min(dove,samps_from_peak);					//	Length of transition to next amp boost
			local_max_samp = 0.0;
			for(k = startsamp,j = 0,m = seglen-1; k < endsamp;k++,j++,m--) {
				if(n > 0 && j < doveup) {						//	j goes  0   1   2   3   4   5   6   7   8 ... 
					incr = (double)j/(double)doveup;			//	If transit is 3 samps
					mag  = (magprechange/2) * incr;				//	incr   0/3 1/3 2/3 
					mag += lastmag;
					val = ibuf[bufpos] * mag;
					switch(passno) {
					case(0):
						local_max_samp = max(local_max_samp,fabs(val));
						break;
					case(1):
						max_samp = max(max_samp,fabs(val));
						break;
					case(2):
						obuf[bufpos] = (float)(val * normaliser);
						break;
					}
				} else if(n < trofcnt - 1 && m < dovedn) {		//	m goes  8   7   6   5   4   3   2   1   0
					incr = 1.0 - ((double)m/(double)dovedn);	//	If transit is 4 samps
					mag  = (magpostchange/2) * incr;			//	m/dovedn goes			   3/4 2/4 1/4 0/4 		
					mag += maxamp[n];							//	incr goes				   1/4 2/4 3/4  1
					lastmag = mag;
					val = ibuf[bufpos] * mag;

					switch(passno) {
					case(0):
						local_max_samp = max(local_max_samp,fabs(val));
						break;
					case(1):
						max_samp = max(max_samp,fabs(val));
						break;
					case(2):
						obuf[bufpos] = (float)(val * normaliser);
						break;
					}
				} else {
					val = ibuf[bufpos] * maxamp[n];
					switch(passno) {
					case(0):
						local_max_samp = max(local_max_samp,fabs(val));
						break;
					case(1):
						max_samp = max(max_samp,fabs(val));
						break;
					case(2):
						obuf[bufpos] = (float)(val * normaliser);
						break;
					}
				}
				if(++bufpos >= dz->ssampsread) {
					if(passno == 2) {
						if((exit_status = write_samps(obuf,dz->ssampsread,dz))<0)
							return(exit_status);
					} else {
						dz->total_samps_written += dz->ssampsread;
						dz->process = GREV;
						display_virtual_time(dz->total_samps_written,dz);
						dz->process = FLATTEN;
					}
					if((exit_status = read_samps(ibuf,dz)) < 0)
						return(exit_status);
					if(dz->ssampsread == 0)
						break;
					bufpos = 0;
				}
			}
			if(passno == 0) {
				if(!flteq(local_max_samp,0.0))
					maxamp[n] *= MAXLEV/local_max_samp;
			}
			startsamp = endsamp;
		}
		if(passno == 1) {
			if(max_samp > MAXLEV)
				normaliser = MAXLEV/max_samp;
			else
				normaliser = 1.0;
		}
	}
	return FINISHED;
}

/****************************** FLATTEN_PARAM_PREPROCESS ******************************/

#define SAFETY	64

int flatten_param_preprocess(dataptr dz)
{
	int arraysize = (int)ceil(dz->duration/dz->param[0]);
	if((dz->lparray = (int **)malloc(sizeof(int *)))==NULL) {
		fprintf(stdout,"ERROR: INSUFFICIENT MEMORY to store Filter Data.\n");
		fflush(stdout);
		return(FAILED);
	}
	if((dz->lparray[0] = (int *)malloc((arraysize + SAFETY) * sizeof(int)))==NULL) {	//	Array for trof locations
		fprintf(stdout,"ERROR: INSUFFICIENT MEMORY to store Filter Data.\n");
		fflush(stdout);
		return(FAILED);
	}
	if((dz->parray = (double **)malloc(2 * sizeof(double *)))==NULL) {
		fprintf(stdout,"ERROR: INSUFFICIENT MEMORY to store Filter Data.\n");
		fflush(stdout);
		return(FAILED);
	}
	if((dz->parray[0] = (double *)malloc((arraysize * (WINS_PER_ELEMENT+1)) * sizeof(double)))==NULL) {	//	Array for initial envelope
		fprintf(stdout,"ERROR: INSUFFICIENT MEMORY to store Filter Data.\n");
		fflush(stdout);
		return(FAILED);
	}
	memset((char *)dz->parray[0],0,(arraysize * (WINS_PER_ELEMENT+1)) * sizeof(double));

	if((dz->parray[1] = (double *)malloc(4 * sizeof(double)))==NULL) {
		fprintf(stdout,"ERROR: INSUFFICIENT MEMORY to store Filter Data.\n");			//	Array for shrunk envelopes
		fflush(stdout);
		return(FAILED);
	}
	return FINISHED;
}

/*************************** CREATE_FLATTEN_SNDBUFS **************************/

int create_flatten_sndbufs(dataptr dz)
{
	int n;
	int bigbufsize, secsize;
	int framesize;
	double srate = (double)dz->infile->srate;
	framesize = F_SECSIZE * dz->infile->channels;
	if(dz->sbufptr == 0 || dz->sampbuf==0) {
		sprintf(errstr,"buffer pointers not allocated: create_sndbufs()\n");
		return(PROGRAM_ERROR);
	}
	dz->buflen = (int)ceil((dz->param[0]/2.0) * srate);				//	Buffer must be large-enough to accomodate 1 while envelope-window
	secsize = dz->buflen/framesize;										//	Which is roughly 1/3 of elementsize = dz->param[0]
	if(secsize * framesize < dz->buflen)
		secsize++;
	dz->buflen = secsize * framesize;
	bigbufsize = dz->buflen * sizeof(float);
	if((dz->bigbuf = (float *)malloc(bigbufsize  * dz->bufcnt)) == NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
		return(PROGRAM_ERROR);
	}
	for(n=0;n<dz->bufcnt;n++)
		dz->sbufptr[n] = dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
	dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
	return(FINISHED);
}

/*************************** IS_PHASE_CHANGE **************************/

int is_phase_change(float *ibuf,int *all_zero,int *phasechange,int wstart,int wend)
{
	int phase = 0;
	int k = wstart;
	while(ibuf[k] == 0.0) {
		if(++k >= wend) {
			*all_zero = 1;
			return FALSE;
		}
	}
	if(ibuf[k] > 0)
		phase = 1;
	else
		phase = -1;
	k++;
	while(k < wend) {
		if(phase > 1) {
			if(ibuf[k] < 0.0) {
				*phasechange = 1;
				return TRUE;		//	phase changed
			}
		} else {
			if(ibuf[k] > 0.0) {		//	phase changed
				*phasechange = 1;
				return TRUE;
			}
		}
		k++;
	}
	return FALSE;
}

/*************************** FIND_PHASECHANGE_POSITION **************************/

int find_phasechange_position(float *ibuf,int *zcrospos,int wstart,int wend)
{
	int phase = 0;
	int k = wstart;
	while(ibuf[k] == 0.0) {
		if(++k >= wend) {
			sprintf(errstr,"Error: no phasechange found in window.\n");
			return PROGRAM_ERROR;
		}
	}
	if(ibuf[k] > 0)
		phase = 1;
	else
		phase = -1;
	k++;
	while(k < wend) {
		if(phase > 1) {
			if(ibuf[k] < 0.0) {
				*zcrospos = k;
				break;
			}
		} else {
			if(ibuf[k] > 0.0) {		//	phase changed
				*zcrospos = k;
				break;
			}
		}
		k++;
	}
	return FINISHED;
}

