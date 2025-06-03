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
#include <envlcon.h>
#include <srates.h>
#ifdef unix
#include <aaio.h>
#endif

#ifdef unix
#define round(x) lround((x))
#endif

#define envcnt	wlength
#define trofcnt rampbrksize
#define targetcnt ringsize
#define NEGATION 45

#define EXTSPEAK_PKSRCHWIDTH 3

#define XSPK_MAXLEVEL	0.9
char errstr[2400];

#define	PATN_ARRAY	0
#define	PERM_ARRAY	1
#define	TRGT_ARRAY	2

int anal_infiles = 1;
int	sloom = 0;
int sloombatch = 0;

const char* cdp_version = "6.1.0";

//CDP LIB REPLACEMENTS
static int check_extspeak_param_validity_and_consistency(int XSPK_RRND,int XSPK_ORISZ,dataptr dz);
static int setup_extspeak_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_extspeak_param_ranges_and_defaults(dataptr dz);
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
static int precalculate_peaks_array_and_splice(dataptr dz);
static int extspeak(int XSPK_RRND,int XSPK_ORISZ,dataptr dz);
static int windows_in_sndfile(dataptr dz);
static int getenv_of_buffer(int samps_to_process,float **env,dataptr dz);
static double getmaxsamp(int startsamp, int sampcnt,float *buffer);
static int istrof(float *env,float *envend,float *q,int width);
static void randperm(int z,int setlen,dataptr dz);
static void hinsert(int z,int m,int t,int setlen,dataptr dz);
static void hprefix(int z,int m,int setlen,dataptr dz);
static void hshuflup(int z,int k,int setlen,dataptr dz);
static int handle_the_extra_infiles(int *cmdlinecnt,char ***cmdline,dataptr dz);
static int insert_new_syllable(int peaklen,int patno,int splicelen,double gain,double origlevel,double normaliser,
							   int *obufpos,float *obuf,float *ovflwbuf,dataptr dz);
static int store_patterndatafile_name(int *cmdlinecnt,char ***cmdline,dataptr dz);
static int handle_pattern_data(int patternsize,dataptr dz);
static int generate_xspk_pattern(int XSPK_RRND, int patternsize,dataptr dz);
static int create_sndbufs_for_extspeak(dataptr dz);
static int get_int_from_within_string(char **str,int *val, int minus_one_ok);
static int open_checktype_getsize_and_compare_header(char *filename,int fileno,fileptr *fp2,dataptr dz);
static int find_orig_syllab_maxima(int splicelen,dataptr dz);
static int find_normaliser(double *normaliser,int samps_per_sec,int splicelen,dataptr dz);
static double get_syllable_time(int thistrofat,int lasttrofat,int peaklen,int n,int samps_per_sec,dataptr dz);
static double dbtolevel(double val);
static int getcutdata(int *cmdlinecnt,char ***cmdline,dataptr dz);
static int get_cutpat_data(int *cmdlinecnt,char ***cmdline,dataptr dz);
static int expand_pattern_data(int patternsize,dataptr dz);
static int get_cuttapa_data(int *cmdlinecnt,char ***cmdline,dataptr dz);
static int get_cuttarg_data(int *cmdlinecnt,char ***cmdline,dataptr dz);
static int exttargetspeak(int XSPK_RRND,int XSPK_ORISZ,dataptr dz);
static int find_normaliser_target(double *normaliser,int samps_per_sec,int splicelen,dataptr dz);

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
	int exit_status;
	dataptr dz = NULL;
	char **cmdline;
	int  cmdlinecnt, XSPK_RRND, XSPK_ORISZ;
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
	dz->trofcnt = 0;
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
		dz->maxmode = 18;
		if((exit_status = get_the_mode_from_cmdline(cmdline[0],dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(exit_status);
		}
		cmdline++;
		cmdlinecnt--;
		// setup_particular_application =
		if((exit_status = setup_extspeak_application(dz))<0) {
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
	if((exit_status = setup_extspeak_param_ranges_and_defaults(dz))<0) {
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

	if((dz->iparray = (int **)malloc(3 * sizeof(int *))) == NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store trof-locations and other data.\n");
		return(MEMORY_ERROR);
	}

//	handle_extra_infiles
	if((exit_status = handle_the_extra_infiles(&cmdlinecnt,&cmdline,dz))<0) {
		fprintf(stdout,"\n **** May be TOO MANY PARAMS on commandline ****\n\n");
		fflush(stdout);
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if(dz->infilecnt < 2) {
		sprintf(errstr,"ERROR: At least TWO input files required for this process.\n");
		exit_status = DATA_ERROR;
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}

	// handle_outfile() =
	if((exit_status = handle_the_outfile(&cmdlinecnt,&cmdline,dz))<0) {
		fprintf(stdout,"\n **** If the outfile does NOT already exist ****\n **** May be TOO FEW PARAMS on commandline ****\n\n");
		fflush(stdout);
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}

//	handle_formants()			redundant
//	handle_formant_quiksearch()	redundant
	switch(dz->mode) {
	case(2):	//	fall thro
	case(5):
		if((exit_status = store_patterndatafile_name(&cmdlinecnt,&cmdline,dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		break;
	case(6):	//	fall thro
	case(7):	//	fall thro
	case(9):	//	fall thro
	case(10):
		if((exit_status = getcutdata(&cmdlinecnt,&cmdline,dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		break;
	case(8):	//	fall thro
	case(11):
		if((exit_status = get_cutpat_data(&cmdlinecnt,&cmdline,dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		break;
	case(12):	//	fall thro
	case(13):	//	fall thro
	case(15):	//	fall thro
	case(16):
		if((exit_status = get_cuttarg_data(&cmdlinecnt,&cmdline,dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		break;
	case(14):	//	fall thro
	case(17):
		if((exit_status = get_cuttapa_data(&cmdlinecnt,&cmdline,dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		break;
	}
	if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {		// CDP LIB
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if(dz->mode == 3 || dz->mode == 9)
		XSPK_RRND =	3;
	else if(dz->mode == 12)
		XSPK_RRND =	3;
	else if(dz->mode == 15)
		XSPK_RRND =	2;
	else
		XSPK_RRND = XSPK_RAND;
	if(dz->mode >=12 && dz->mode < 15)
		XSPK_ORISZ = 2;
	else
		XSPK_ORISZ = XSPK_ORIGSZ;

	is_launched = TRUE;
	dz->bufcnt = dz->infilecnt + 2;
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

	//	1 double array for splice, 1 for maxlevels of orig syllables & 1 for maxlevels of new syllables
	if((dz->parray = (double **)malloc(3 * sizeof(double *))) == NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create splice buffer (1).\n");
		return(MEMORY_ERROR);
	}
	//	2 float arrays for trofstore
	if((dz->fptr=(float **)malloc(2 * sizeof(float *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store envelope (1).\n");
		return(MEMORY_ERROR);
	}
//	create_sndbufs
	if((exit_status = create_sndbufs_for_extspeak(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = precalculate_peaks_array_and_splice(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if(dz->vflag[XSPK_ENV] || (dz->mode >= 3 && dz->mode < 6) || (dz->mode >= 9 && dz->mode < 12) || dz->mode >= 15) {
		if((dz->parray[2] = (double *)malloc(dz->infilecnt * sizeof(double))) == NULL) {//	Storage max levels of new input syllables
			sprintf(errstr,"INSUFFICIENT MEMORY to maximum levels of input syllables.\n");
			return(MEMORY_ERROR);
		}
	}
	//	check_param_validity_and_consistency....
	if((exit_status = check_extspeak_param_validity_and_consistency(XSPK_RRND,XSPK_ORISZ,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	//param_preprocess()						redundant
	//spec_process_file =
	if(dz->mode >= 12)  {
		if((exit_status = exttargetspeak(XSPK_RRND,XSPK_ORISZ,dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
	} else {
		if((exit_status = extspeak(XSPK_RRND,XSPK_ORISZ,dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
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

/************************* SETUP_EXTSPEAK_APPLICATION *******************/

int setup_extspeak_application(dataptr dz)
{
	int exit_status;
	aplptr ap;
	if((exit_status = establish_application(dz))<0)		// GLOBAL
		return(FAILED);
	ap = dz->application;
	switch(dz->mode) {
	case(9):	//	fall thro
	case(10):	//	fall thro
	case(6):	//	fall thro
	case(7): exit_status = set_param_data(ap,XSPK_CUTS	  ,6,5,"0iiIDi");	break;
	case(11):	//	fall thro
	case(8): exit_status = set_param_data(ap,XSPK_CUTPAT  ,6,4,"0iiID0");	break;
	case(3):	//	fall thro
	case(4):	//	fall thro
	case(0):	//	fall thro
	case(1): exit_status = set_param_data(ap,0			  ,6,6,"iiiIDi");	break;
	case(5):	//	fall thro
	case(2): exit_status = set_param_data(ap,XSPK_PATTERN ,6,5,"iiiID0");	break;
	case(12):	//	fall thro
	case(13):	//	fall thro
	case(15):	//	fall thro
	case(16): exit_status = set_param_data(ap,XSPK_CUTARG ,6,3,"0i00Di");	break;
	case(14):	//	fall thro
	case(17): exit_status = set_param_data(ap,XSPK_CUPATA ,6,2,"0i00D0");	break;
	}
	if(exit_status < 0)
		return(FAILED);
	switch(dz->mode) {
	case(6):	//	fall thro
	case(0):
		exit_status = set_vflgs(ap,"",0,"","tekior",6,0,"000000");
		break;
	case(7):	//	fall thro
	case(8):	//	fall thro
	case(1):	//	fall thro
	case(2):
		exit_status = set_vflgs(ap,"",0,"","tekio" ,5,0,"00000");
		break;
	case(9):	//	fall thro
	case(3):
		exit_status = set_vflgs(ap,"",0,"","tekr"   ,4,0,"0000");
		break;
	case(10):	//	fall thro
	case(11):	//	fall thro
	case(4):	//	fall thro
	case(5):
		exit_status = set_vflgs(ap,"",0,"","tek"    ,3,0,"000");
		break;
	case(12):
		exit_status = set_vflgs(ap,"",0,"","teor"   ,4,0,"0000");
		break;
	case(15):	//	fall thro
		exit_status = set_vflgs(ap,"",0,"","ter"    ,3,0,"000");
		break;
	case(13):	//	fall thro
	case(14):
		exit_status = set_vflgs(ap,"",0,"","teo"    ,3,0,"000");
		break;
	case(16):	//	fall thro
	case(17):
		exit_status = set_vflgs(ap,"",0,"","te"     ,2,0,"00");
		break;
	}
	if(exit_status < 0)
		return(FAILED);
	// set_legal_infile_structure -->
	dz->has_otherfile = FALSE;
	// assign_process_logic -->
	dz->input_data_type = MANY_SNDFILES;
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

/************************* SETUP_EXTSPEAK_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_extspeak_param_ranges_and_defaults(dataptr dz)
{
	int exit_status;
	aplptr ap = dz->application;
	// set_param_ranges()
	ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
	// NB total_input_param_cnt is > 0 !!!
	if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
		return(FAILED);
	if(dz->mode < 6) {
		ap->lo[XSPK_WINSZ]	= 5;			//	param 0
		ap->hi[XSPK_WINSZ]	= 1000;
		ap->default_val[XSPK_WINSZ]	= 50;
	}
	ap->lo[XSPK_SPLEN]	= 2;				//	param 1
	ap->hi[XSPK_SPLEN]	= 100;
	ap->default_val[XSPK_SPLEN]	= 5;
	if(dz->mode < 12) {
		ap->lo[XSPK_OFFST]	= 0;			//	param 2
		ap->hi[XSPK_OFFST]	= 100;
		ap->default_val[XSPK_OFFST]	= 0;
		ap->lo[XSPK_N]	= 0;				//	param 3
		ap->hi[XSPK_N]	= MAX_PATN;
		ap->default_val[XSPK_N]	= 1;
	}
	ap->lo[XSPK_GAIN]	= -96;				//	param 4
	ap->hi[XSPK_GAIN]	= 0;
	ap->default_val[XSPK_GAIN]	= 0;
	if(dz->mode != 2 && dz->mode != 5 && dz->mode != 8 && dz->mode != 11 && dz->mode != 14 && dz->mode != 17) {
		ap->lo[XSPK_SEED]	= 0;			//	param 5
		ap->hi[XSPK_SEED]	= 64;
		ap->default_val[XSPK_SEED]	= 0;
	}
	dz->maxmode = 18;
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
			if((exit_status = setup_extspeak_application(dz))<0)
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
	usage2("extspeak");
	return(USAGE_ONLY);
}

/**************************** CHECK_EXTSPEAK_PARAM_VALIDITY_AND_CONSISTENCY *****************************/

int check_extspeak_param_validity_and_consistency(int XSPK_RRND,int XSPK_ORISZ,dataptr dz)
{
//	NEEDS TO BE DONE WHEN TROFCNT IS KNOWN AND SPLICELEN IN GPSAMPS KNOWN !!!!
	int lastsamp = 0, n, k, sampsize;
	int chans = dz->infile->channels, srate = dz->infile->srate;
	int offset = dz->iparam[XSPK_OFFST];
	int *trof = dz->lparray[0];
	int splicelen, minseg, bad;
	splicelen = dz->iparam[XSPK_SPLEN] * chans;
	minseg = (splicelen * 2) + chans;
	if(dz->trofcnt == 0) {
		sprintf(errstr,"Trof array not established before testing parameters.\n");
		return PROGRAM_ERROR;
	}
	if(offset >= dz->trofcnt - 2) {
		sprintf(errstr,"ERROR: Offset (%d) too large for number of peaks found (%d).\n",offset,dz->trofcnt);
		return DATA_ERROR;
	}
	bad = 0;
	for(n = 0; n <= dz->trofcnt; n++) {
		sampsize = trof[n] - lastsamp;
		if(sampsize < minseg) {
			bad = 1;
			if(n == 0)
				fprintf(stdout,"syllab %d too short (%d mS) for two Splices (%d mS)\nbetween times 0 and %lf\n",
				n+1,(int)round(((double)(sampsize/chans)/(double)srate) * SECS_TO_MS),(int)round(dz->param[XSPK_SPLEN] * 2),(double)trof[n]/(double)(srate * chans));
			else {
				sampsize -= splicelen;
				fprintf(stdout,"syllab %d too short (%d mS) for two Splices (%d mS)\nbetween times %lf and %lf\n",
				n+1,(int)round(((double)(sampsize/chans)/(double)srate) * SECS_TO_MS),(int)round(dz->param[XSPK_SPLEN] * 2),(double)(trof[n-1]/chans)/(double)srate,(double)(trof[n]/chans)/(double)srate);
			}
		}
		lastsamp = trof[n] - splicelen;
	}
	if(bad)
		return DATA_ERROR;
	if(dz->trofcnt <= 1) {
		sprintf(errstr,"Too few trofs found (%d) for this process and offset (%d)\n",dz->trofcnt,dz->iparam[2]);
		return DATA_ERROR;
	}
	if(dz->mode < 3 || (dz->mode >= 6  && dz->mode < 9)) {
		if(dz->vflag[XSPK_INJECT]) {
			if(dz->vflag[XSPK_ORISZ] || dz->vflag[XSPK_ENV] || dz->vflag[XSPK_TRANSPOSE] ) {
				if(dz->vflag[XSPK_ORISZ]) {
					if(dz->vflag[XSPK_ENV] && dz->vflag[XSPK_TRANSPOSE])
						fprintf(stdout,"WARNING: \"Original Size\" flag redundant : \"Envelope\" & \"Transpose\" Flags not operational when INSERTING the original syllables.\n");
					else if(dz->vflag[XSPK_ENV])
						fprintf(stdout,"WARNING: \"Original Size\" flag redundant : \"Envelope\" Flag not operational when INSERTING the original syllables.\n");
					else if(dz->vflag[XSPK_TRANSPOSE])
						fprintf(stdout,"WARNING: \"Original Size\" flag redundant : \"Transpose\" Flag not operational when INSERTING the original syllables.\n");
					else
						fprintf(stdout,"WARNING: \"Original Size\" flag redundant when INSERTING the original syllables.\n");
				} else {
					if(dz->vflag[XSPK_ENV] && dz->vflag[XSPK_TRANSPOSE])
						fprintf(stdout,"WARNING: \"Envelope\" & \"Transpose\" Flags not operational when INSERTING the original syllables.\n");
					else if(dz->vflag[XSPK_ENV])
						fprintf(stdout,"WARNING: \"Envelope\" Flag not operational when INSERTING the original syllables.\n");
					else
						fprintf(stdout,"WARNING: \"Transpose\" Flag not operational when INSERTING the original syllables.\n");
				}
				fflush(stdout);
			}
			dz->vflag[XSPK_TRANSPOSE] = 0;
			dz->vflag[XSPK_ENV]       = 0;
			dz->vflag[XSPK_ORISZ]    = 1;
		}
	}
	if(dz->mode < 3 || (dz->mode >= 6  && dz->mode < 9) || (dz->mode >= 12  && dz->mode < 15)) {
		if(dz->vflag[XSPK_ORISZ] && dz->vflag[XSPK_TRANSPOSE]) {
			fprintf(stdout,"WARNING: \"Keep original size\" Flag makes \"Transposition\" Flag redundant.\n");
			fflush(stdout);
		}
	}
	if((dz->mode==0 || dz->mode==3 || dz->mode==6 || dz->mode==9) && dz->vflag[XSPK_RRND] && dz->infilecnt < 3) {
		switch(dz->mode) {
		case(10):	//	fall thro
		case(7):	//	fall thro
		case(4):	//	fall thro
		case(1):
			sprintf(errstr,"INFO: No randomisation possible with less than 2 (extra) input file.\n");
			return DATA_ERROR;
		case(9):	//	fall thro
		case(6):	//	fall thro
		case(3):	//	fall thro
		case(0):
			if(dz->infilecnt < 3) {
				sprintf(errstr,"WARNING: No randomisation possible with only less than 2 (extra) input file.\n");
				return DATA_ERROR;
			}
			break;
		}
	}
	if(dz->brksize[XSPK_GAIN]) {
		for(n = 0,k = 1; n < dz->brksize[XSPK_GAIN];n++,k+=2)
			dz->brk[XSPK_GAIN][k] = dbtolevel(dz->brk[XSPK_GAIN][k]);
	} else
		dz->param[XSPK_GAIN] = dbtolevel(dz->param[XSPK_GAIN]);

	return FINISHED;
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	if(!strcmp(prog_identifier_from_cmdline,"extspeak"))				dz->process = EXTSPEAK;
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
    if(!strcmp(str,"extspeak")) {
        fprintf(stderr,
        "\nUSAGE:  extspeak extspeak 1-18 infile0 infile1 [infile2..] outfile params\n"
        "PARAMS for each mode\n"
        "1)          wsize splice offset N gain seed [-t] [-e] [-k] [-i] [-o] [-r]\n"
        "2)          wsize splice offset N gain seed [-t] [-e] [-k] [-i] [-o] \n"
        "3)  pattern wsize splice offset N gain      [-t] [-e] [-k] [-i] [-o] \n"
        "4)          wsize splice offset N gain seed [-t] [-e] [-k]           [-r]\n"
        "5)          wsize splice offset N gain seed [-t] [-e] [-k]\n"
        "6)  pattern wsize splice offset N gain      [-t] [-e] [-k]\n"
        "7)  cuts          splice offset N gain seed [-t] [-e] [-k] [-i] [-o] [-r]\n"
        "8)  cuts          splice offset N gain seed [-t] [-e] [-k] [-i] [-o] \n"
        "9)  cutspat       splice offset N gain      [-t] [-e] [-k] [-i] [-o] \n"
        "10) cuts          splice offset N gain seed [-t] [-e] [-k]           [-r]\n"
        "11) cuts          splice offset N gain seed [-t] [-e] [-k]\n"
        "12) cutspat       splice offset N gain      [-t] [-e] [-k]\n"
        "13) cuts+targets  splice          gain seed [-t] [-e]           [-o] [-r]\n"
        "14) cuts+targets  splice          gain seed [-t] [-e]           [-o]\n"
        "15) cuts+tgts+pat splice          gain      [-t] [-e]           [-o]\n"
        "16) cuts+targets  splice          gain seed [-t] [-e]                [-r]\n"
        "17) cuts+targets  splice          gain seed [-t] [-e]\n"
        "18) cuts+tgts+pat splice          gain      [-t] [-e]\n"
        "MODES 1-3,6-9   REPLACE  - or INJECT BETWEEN - syllabs of infile0 ...\n"
        "MODES 12-15     REPLACE SPECIFIED syllabs of infile0 ...\n"
        "MODES 4-6,10-12 MIX INTO selected syllabs of infile0 ...\n"
        "MODES 16-18     MIX INTO SPECIFIED syllabs of infile0 ...\n"
        "      ... NEW SYLLABLES, each taken from one of the other input files.\n"
        "MODES 1/4/7/10/13/16  Select syllabs from each file in turn OR (-r) at random.\n"
        "MODES 2/5/8/11/14/17  Rand-permute order of syllabs. Once ALL used, permute again.\n"
        "MODES 3/6/9/12/15/18  Select infile following specified ordering-pattern.\n"
        "\n"
        "PATTERN Pattern in a Textfile : the specific sequence of syllables to be inserted.\n"
        "        e.g. \"1 5 2\" means, use infile1 infile5 infile2 in that order (then repeat).\n"
        "CUTS    Modes 7,8,10,11 List of Times, in a Textfile : location of syllable-starts\n"
        "        in infile0 (zero time, and time at very end of file, not required).\n"
        "CUTSPAT Modes 9 and 12 List of Times, in a Textfile : location of syllable-starts\n"
        "        FOLLOWED BY single line with \"#\", followed by PATTERN Data (see above).\n"
        "CUTS+TARGETS    List of Times, in a Textfile : location of syllable-starts\n"
        "        FOLLOWED BY single line with \"#\", followed by SYLLABLES TO TARGET Data.\n"
        "CUTS+TGTS+PAT   List of Times, in a Textfile : location of syllable-starts\n"
        "        FOLLOWED BY single line with \"#\", followed by SYLLABLES TO TARGET Data\n"
        "        FOLLOWED BY single line with \"#\", followed by PATTERN Data (see above).\n"
        "If there is NO CUTS/CUTSPAT, infile0 divided into \"syllables\" by automatic process.\n"
        "\n"
        "FOR MORE INFORMATION ----- hit Enter to continue\n");
        getchar();

        fprintf(stderr,
        "\n"
        "In \"Syllables to Target\" data, the COUNT OF SYLLABLES in infile0\n"
        "is ONE MORE than the number of edit-points you have listed\n"
        "(assuming there are no edits at zero or at the file end).\n"
        "\n"
        "       Each edit point marks the END of a syllable\n"
        "       and the final syllable is AFTER the last edit.\n"
        "\n"
        "To ensure to target the VERY LAST syllable, the value \"-1\" can be used.\n"
        "\n"
        "\n"
        "PARAMETERS\n"
        "\n"
        "WSIZE   Size of envelope-search window to AUTOfind syllabs in file0, in mS (try 50).\n"
        "SPLICE  Splice length for extracting and joining syllables (try 15).\n"
        "OFFSET  Number of syllables at start of infile0 to output unchanged.\n"
        "N       N means write 1 original (file0) syllable for every N new syllabs inserted\n"
        "        ...... EXCEPT IF flag \"-k\" is set, when ......\n"
        "        N means KEEP N original (file0) syllables for every 1 new syllab inserted.\n"
        "GAIN    Gain of inserted syllables (Range -96dB to 0dB).\n"
        "SEED    Initialisation for random reorderings, or permutations.\n"
        "        If Seed > 0, using SAME seed AGAIN gives IDENTICAL random output.\n"
        "Parameters N and GAIN may vary over time.\n"
        "\n"
        "FOR MORE INFORMATION ----- hit Enter to continue\n");
        getchar();

        fprintf(stderr,
        "\n"
        "EXAMPLE USAGE:\n"
        "          For N=2 (i.e. REPLACE or KEEP 2 in 3 syllables)\n"
        "          AND 9 Original Syllables =  ---------\n"
        "\n"
        "No flags:      Keep 1, Replace (N=)2  -XX-XX-XX\n"
        "-k flags:      Keep (N=)2, Replace 1  --X--X--X\n"
        "-i flag:       Keep 1, Inject (N=)2   -XX-XX-XX-XX-XX-XX-XX-XX-XX\n"
        "-i & -k flags: Keep (N=)2, Inject 1   --X--X--X--X-\n"
        "\n"
        "FLAGS.\n"
        "-r      RAND: Select next inserted syllable ENTIRELY at random.\n"
        "-k      KEEP:  N becomes count of syllabs to KEEP rather than to REPLACE or INJECT.\n"
        "-i      INJECT new syllable(s) BETWEEN existing infile0 syllables.\n"
        "        Default : new syllabs OVERWRITE existing syllables, and are resized to fit.\n"
        "\n"
        "FOLLOWING FLAG ONLY RELEVANT where you REPLACE the original syllables.\n"
        "-o      ORIGINAL SIZE: Don't Resize new syllable to size of originals.\n"
        "        (Output file will therefore not be same length/rhythm as infile0).\n"
        "\n"
        "FOLLOWING FLAGS ONLY RELEVANT where you REPLACE or MIX INTO the original syllables.\n"
        "-t      TRANSPOSE: If Resizing, Transpose/Time-stretch new syllab to adjust length.\n"
        "        Default: Cut to Size or Pad with Silence.\n"
        "-e      Follow the ENVELOPE of the the original syllables in file0\n"
        "        by scaling level of inserted syllab to that of replaced/mixed-into syllable.\n"
        "        This scaling is independent of the \"GAIN\" value (which is ALSO applied).\n"
        "\n"
        "FOR MORE INFORMATION ----- hit Enter to continue\n");
        getchar();

        fprintf(stderr,
        "CONTOUR TYPES.\n"
        "\n"
        "                                        _\n"
        "                                     / | |    _\n"
        "      _ ................ Add ..... /   |_|   | |\n"
        "     | |                               | |   | |\n"
        "     | |                     _         | |   | |\n"
        "     | |    _               | |        | |   |_|\n"
        "     | |_  | |         _    | |        | |_  | |\n"
        "    _| | |_| |_   +   | |   | |   =   _| | |_| |_\n"
        "   | | | | | | |     _| |___| |_     | | | | | | |\n"
        "\n"
        "                         Gain+          _\n"
        "      _ ................ Add ..... ../ |_|\n"
        "     | |                               | |    _\n"
        "     | |                   .....       | |   | |\n"
        "     | |    _                          | |   |_|\n"
        "     | |_  | |       .....  ._.        | |_  | |\n"
        "    _| | |_| |_   +   ._.   | |   =   _| | |_| |_\n"
        "   | | | | | | |     _| |___| |_     | | | | | | |\n"
        "\n"
        "                                        _\n"
        "                                     / | |\n"
        "                                    /  | |\n"
        "                                   /   | |\n"
        "                                  /    | |\n"
        "                                 /     | |\n"
        "      _ .... -e ...... _ ..Add../      |_|    _\n"
        "     | |  Envelope    |^|              | |   | |\n"
        "     | |              |^|  .....       | |   | |\n"
        "     | |    _ ........|^|... _ ...     | |   |_|\n"
        "     | |_  | |        |^|   | |        | |_  | |\n"
        "    _| | |_| |_  +    |_|   | |   =   _| | |_| |_\n"
        "   | | | | | | |     _| |___| |_     | | | | | | |\n"
        "\n");
    } else {
        fprintf(stdout,"Unknown option '%s'\n", str);
        return USAGE_ONLY;
    }
    return USAGE_ONLY;
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

/****************************** EXTSPEAK *********************************/

int extspeak(int XSPK_RRND,int XSPK_ORISZ,dataptr dz)
{
	int exit_status, chans = dz->infile->channels, srate = dz->infile->srate, orig, done;
	float *ibuf = dz->sampbuf[0], *obuf = dz->sampbuf[dz->infilecnt], *ovflwbuf = dz->sampbuf[dz->infilecnt + 1];
	int gpsplicelen = dz->iparam[XSPK_SPLEN];
	int splicelen = gpsplicelen * chans, upsplice;
	int obufpos = 0, lasttrofat = 0, thistrofat, peaklen, splicend;
	int *trof = dz->lparray[0];
	int mintrof;
	int n, m, j, k, z;
	double *splicebuf = dz->parray[0], time = 0, maxgain = 0.0, *origmax = dz->parray[1], *syllmax = dz->parray[2];
	int samps_per_sec = srate * chans;
	double val, gain, normaliser = 1.0;
	int *pattern;
	double Nmaxd, origlevel = 1.0;
	int max_inserts_per_orig, patternsize, patterncnt, patno, syllread, q;
	for(n = 1; n< dz->infilecnt;n++) {
		if((syllread = fgetfbufEx(dz->sampbuf[n], dz->insams[n],dz->ifd[n],0)) < 0) {
			sprintf(errstr,"Can't read samples from soundfile %d\n",n);      // RWD was %n
			return(SYSTEM_ERROR);
		}
		if(syllread != dz->insams[n]) {
			sprintf(errstr,"Fail to read all samples from soundfile %d\n",n);   // RWD ditto
			return(SYSTEM_ERROR);
		}
		if(dz->vflag[XSPK_ENV] || (dz->mode >= 3 && dz->mode < 6) || (dz->mode >= 9 && dz->mode < 12)|| dz->mode >= 15) {
			maxgain = 0.0;
			for(q = 0; q < dz->insams[n]; q++)
				maxgain = max(maxgain,fabs(dz->sampbuf[n][q]));
			syllmax[n] = maxgain;
		}
	}
	if(dz->mode != 2 && dz->mode != 5 && dz->mode != 8 && dz->mode != 11) {
		if(dz->iparam[XSPK_SEED] > 0)
			srand(dz->iparam[XSPK_SEED]);
		else
			initrand48();
	}
	mintrof = dz->iparam[XSPK_OFFST];
	if(dz->brksize[XSPK_N])  {
		if((exit_status = get_maxvalue_in_brktable(&Nmaxd,XSPK_N,dz))<0)
			return exit_status;
	} else
		Nmaxd = dz->param[XSPK_N];
	if(dz->vflag[XSPK_KEEP])								// One inserted item for every N origs
		max_inserts_per_orig = 1;							//	Max possible 1 for every 1
	else
		max_inserts_per_orig = (int)ceil(Nmaxd);			//	Max possible = max_inserts_per_orig
	patternsize = (dz->trofcnt+1) * max_inserts_per_orig;	//	Max possible pattern size

	if((exit_status = generate_xspk_pattern(XSPK_RRND,patternsize,dz))<0)
		return exit_status;
	if((dz->mode >= 3 && dz->mode < 6) || dz->mode >= 9) {
		fprintf(stdout,"INFO: Calculating Normalisation.\n");
		fflush(stdout);
		if((exit_status = find_orig_syllab_maxima(splicelen,dz)) < 0)
			return exit_status;
		if((exit_status = find_normaliser(&normaliser,samps_per_sec,splicelen,dz)) < 0)
			return exit_status;
	}
	pattern = dz->iparray[PATN_ARRAY];
	orig = 0;
	done = 0;
	lasttrofat = 0;
	splicend = splicelen - 1;
	patterncnt = 0;
	m = mintrof - 1;
	if(m >= 0) {
		thistrofat = trof[m];
		peaklen = thistrofat;			//	We must be at start of file : therefore no obufpos baktrak & no upsplice
		for(k = 0, j = peaklen - 1; k < thistrofat; k++,j--) {
			if (j < splicelen)
				obuf[obufpos] = (float)(ibuf[k] * splicebuf[splicend--] * normaliser);	//	do downslice
			else
				obuf[obufpos] = (float)(ibuf[k] * normaliser);							//  else copy input
			if(++obufpos >= dz->buflen * 2) {
				if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
					return(exit_status);
				memcpy((char *)obuf,(char *)ovflwbuf,dz->buflen * sizeof(float));
				memset((char *)ovflwbuf,0,dz->buflen * sizeof(float));
				obufpos -= dz->buflen;
			}
		}
		lasttrofat = thistrofat;
	}
	m++;
	for(n = m; n <= dz->trofcnt; n++)  {
		if(n > 0)
			lasttrofat = trof[n-1];
		thistrofat  = trof[n];
		peaklen = thistrofat - lasttrofat;
		splicend = splicelen - 1;
		time = get_syllable_time(thistrofat,lasttrofat,peaklen,n,samps_per_sec,dz);
		if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
			return exit_status;
		gain = dz->param[XSPK_GAIN];
		if(lasttrofat > 0) {			//	If we're NOT at file start
			obufpos -= splicelen;		//	baktrak to splice to end of last segment written
			peaklen += splicelen;		//	and length of peak is therefore one splicelen longer
			upsplice = splicelen;
		} else
			upsplice = 0;				//	Prevents initial splice on start of file-segment
		splicend = splicelen - 1;
		if(dz->vflag[XSPK_ENV])
			origlevel = origmax[n];
		switch(dz->vflag[XSPK_KEEP]) {
		case(0):						//	N  = the number of syllables to insert
										//	First Copy ONE original syllable
			for(k = 0, j = peaklen - 1, m = thistrofat - peaklen; k < peaklen; k++,m++,j--) {
				if(k < upsplice)
					val = ibuf[m] * splicebuf[k];							//	do upslice
				else if (j < splicelen)
					val = ibuf[m] * splicebuf[splicend--];					//	do downslice
				else
					val = ibuf[m];											//  else as is
				obuf[obufpos] = (float)(obuf[obufpos] + (val * normaliser));//	then normalise and add to buffer
				if(++obufpos >= dz->buflen * 2) {
					if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
						return(exit_status);
					memcpy((char *)obuf,(char *)ovflwbuf,dz->buflen * sizeof(float));
					memset((char *)ovflwbuf,0,dz->buflen * sizeof(float));
					obufpos -= dz->buflen;
				}
			}

			for(z = 0; z < dz->iparam[XSPK_N]; z++) {	//	Then Add N NEW syllables
				if((dz->mode >= 3 && dz->mode < 6) || dz->mode >= 9 || !dz->vflag[XSPK_INJECT]) {
					if(++n > dz->trofcnt) {				//	Advance count of original syllables
						done = 1;						//	and If out of original syllables, quit
						break;
					}
					lasttrofat = thistrofat;
					thistrofat  = trof[n];
					peaklen = thistrofat - lasttrofat;
					peaklen += splicelen;
				}
				obufpos -= splicelen;
				upsplice = splicelen;
				patno = pattern[patterncnt++];
				if((dz->mode < 3 || (dz->mode >= 6 && dz->mode < 9)) && dz->vflag[XSPK_ORISZ])	//	IF not Resizing : write-in ALL of new syllable
					peaklen = dz->insams[patno];			//	ELSE it's truncated/extended to size of original syllable
				splicend = peaklen - 1;
				if((exit_status = insert_new_syllable(peaklen,patno,splicelen,gain,origlevel,normaliser,&obufpos,obuf,ovflwbuf,dz))<0)
					return exit_status;
			}
			break;
		case(1):					//	N  = the number of syllables to keep
			if(orig < dz->iparam[XSPK_N]) {		//	Add an original syllable, Keeping count of how many added
				for(k = 0, j = peaklen - 1, m = thistrofat - peaklen; k < peaklen; k++,m++,j--) {
					if(k < upsplice)
						val = ibuf[m] * splicebuf[k];							//	do upslice
					else if (j < splicelen)
						val = ibuf[m] * splicebuf[splicend--];					//	do downslice
					else
						val = ibuf[m];											//  else as is
					obuf[obufpos] = (float)(obuf[obufpos] + (val * normaliser));//	then normalise and add to buffer
					if(++obufpos >= dz->buflen * 2) {
						if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
							return(exit_status);
						memcpy((char *)obuf,(char *)ovflwbuf,dz->buflen * sizeof(float));
						memset((char *)ovflwbuf,0,dz->buflen * sizeof(float));
						obufpos -= dz->buflen;
					}
				}
				orig++;									//	Count how many orig syllabs added
			} else {				//	If all N orig syllabs have already been added
				patno = pattern[patterncnt++];		//	Add JUST 1 NEW syllable
				if((dz->mode < 3 || (dz->mode >= 6 && dz->mode < 9)) && dz->vflag[XSPK_ORISZ])
					peaklen = dz->insams[patno];
				splicend = peaklen - 1;
				if((exit_status = insert_new_syllable(peaklen,patno,splicelen,gain,origlevel,normaliser,&obufpos,obuf,ovflwbuf,dz))<0)
					return exit_status;
				orig = 0;							//	Reset orig-syllabs counter
			}
			break;
		}
		if(done)
			break;
	}
	if(obufpos) {
		if((exit_status = write_samps(obuf,obufpos,dz))<0)
			return(exit_status);
	}
	return FINISHED;
}
				
/****************************** EXTTARGETSPEAK *********************************/

int exttargetspeak(int XSPK_RRND,int XSPK_ORISZ,dataptr dz)
{
	int exit_status, chans = dz->infile->channels, srate = dz->infile->srate;
	float *ibuf = dz->sampbuf[0], *obuf = dz->sampbuf[dz->infilecnt], *ovflwbuf = dz->sampbuf[dz->infilecnt + 1];
	int gpsplicelen = dz->iparam[XSPK_SPLEN];
	int splicelen = gpsplicelen * chans, upsplice;
	int obufpos = 0, lasttrofat = 0, thistrofat, peaklen, splicend;
	int *trof = dz->lparray[0];
	int n, m, j, k;
	double *splicebuf = dz->parray[0], time = 0, maxgain = 0.0, *origmax = dz->parray[1], *syllmax = dz->parray[2];
	int samps_per_sec = srate * chans;
	double val, gain, normaliser = 1.0;
	int *pattern, *target;
	double origlevel = 1.0;
	int patterncnt, target_cnt, patno, syllread, q;
	for(n = 1; n< dz->infilecnt;n++) {
		if((syllread = fgetfbufEx(dz->sampbuf[n], dz->insams[n],dz->ifd[n],0)) < 0) {
			sprintf(errstr,"Can't read samples from soundfile %d\n",n);      // RWD was %n
			return(SYSTEM_ERROR);
		}
		if(syllread != dz->insams[n]) {
			sprintf(errstr,"Fail to read all samples from soundfile %d\n",n); // RWD ditto
			return(SYSTEM_ERROR);
		}
		if(dz->vflag[XSPK_ENV] || dz->mode >= 15) {
			maxgain = 0.0;
			for(q = 0; q < dz->insams[n]; q++)
				maxgain = max(maxgain,fabs(dz->sampbuf[n][q]));
			syllmax[n] = maxgain;
		}
	}
	if(dz->mode != 14 && dz->mode != 17) {
		if(dz->iparam[XSPK_SEED] > 0)
			srand(dz->iparam[XSPK_SEED]);
		else
			initrand48();
		if((exit_status = generate_xspk_pattern(XSPK_RRND,dz->trofcnt+1,dz))<0)
			return exit_status;
	}
	if(dz->mode >= 15) {
		fprintf(stdout,"INFO: Calculating Normalisation.\n");
		fflush(stdout);
		if((exit_status = find_orig_syllab_maxima(splicelen,dz)) < 0)
			return exit_status;
		if((exit_status = find_normaliser_target(&normaliser,samps_per_sec,splicelen,dz)) < 0)
			return exit_status;
	}
	target  = dz->iparray[TRGT_ARRAY];
	pattern = dz->iparray[PATN_ARRAY];
	lasttrofat = 0;
	splicend = splicelen - 1;
	patterncnt = 0;
	target_cnt = 0;
	for(n = 0; n <= dz->trofcnt; n++)  {
		if(n > 0)
			lasttrofat = trof[n-1];
		thistrofat  = trof[n];
		peaklen = thistrofat - lasttrofat;
		splicend = splicelen - 1;
		if(dz->brksize[XSPK_GAIN]) {
			time = get_syllable_time(thistrofat,lasttrofat,peaklen,n,samps_per_sec,dz);
			if((exit_status = read_value_from_brktable(time,XSPK_GAIN,dz))<0)
				return exit_status;
		}
		gain = dz->param[XSPK_GAIN];
		if(lasttrofat > 0) {			//	If we're NOT at file start
			obufpos -= splicelen;		//	baktrak to splice to end of last segment written
			peaklen += splicelen;		//	and length of peak is therefore one splicelen longer
			upsplice = splicelen;
		} else
			upsplice = 0;				//	Prevents initial splice on start of file-segment
		splicend = splicelen - 1;
		if(dz->vflag[XSPK_ENV])
			origlevel = origmax[n];
		if(target_cnt < dz->targetcnt && n == target[target_cnt]) {
			patno = pattern[patterncnt++];
			if((dz->mode < 15) && dz->vflag[XSPK_ORISZ])	//	IF not Resizing : write-in ALL of new syllable
				peaklen = dz->insams[patno];			//	ELSE it's truncated/extended to size of original syllable
			splicend = peaklen - 1;
			if((exit_status = insert_new_syllable(peaklen,patno,splicelen,gain,origlevel,normaliser,&obufpos,obuf,ovflwbuf,dz))<0)
				return exit_status;
			target_cnt++;
		} else {
			for(k = 0, j = peaklen - 1, m = thistrofat - peaklen; k < peaklen; k++,m++,j--) {
				if(k < upsplice)
					val = ibuf[m] * splicebuf[k];							//	do upslice
				else if (j < splicelen)
					val = ibuf[m] * splicebuf[splicend--];					//	do downslice
				else
					val = ibuf[m];											//  else as is
				obuf[obufpos] = (float)(obuf[obufpos] + (val * normaliser));//	then normalise and add to buffer
				if(++obufpos >= dz->buflen * 2) {
					if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
						return(exit_status);
					memcpy((char *)obuf,(char *)ovflwbuf,dz->buflen * sizeof(float));
					memset((char *)ovflwbuf,0,dz->buflen * sizeof(float));
					obufpos -= dz->buflen;
				}
			}
		}
	}
	if(obufpos) {
		if((exit_status = write_samps(obuf,obufpos,dz))<0)
			return(exit_status);
	}
	return FINISHED;
}
				
/*************************************** EXTRACT_ENV_FROM_SNDFILE **************************/

int extract_env_from_sndfile(dataptr dz)
{
	int exit_status;
	float *envptr;
	dz->envcnt = windows_in_sndfile(dz);
	if((dz->fptr[0]=(float *)malloc((dz->envcnt+20) * sizeof(float)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store envelope (2).\n");
		return(MEMORY_ERROR);
	}
	envptr = dz->fptr[0];
	dz->ssampsread = 1;
	while(dz->ssampsread > 0)	{
		if((dz->ssampsread = fgetfbufEx(dz->sampbuf[0], dz->insams[0],dz->ifd[0],0)) < 0) {
			sprintf(errstr,"Can't read samples from soundfile 1: extract_env_from_sndfile()\n");
			return(SYSTEM_ERROR);
		}
		if(sloom)
			display_virtual_time(dz->total_samps_read,dz);
		if((exit_status = getenv_of_buffer(dz->ssampsread,&envptr,dz))<0)
			return exit_status;
	}
	dz->fptr[1] = envptr;		//	envend
	if((exit_status = sndseekEx(dz->ifd[0],0,0))<0) {
		sprintf(errstr,"Failed to return to start of file.\n");
		return SYSTEM_ERROR;
	}
	dz->total_samps_read = 0;
	return(FINISHED);
}

/************************* GETENV_OF_BUFFER [READENV] *******************************/

int getenv_of_buffer(int samps_to_process,float **envptr,dataptr dz)
{
	int  start_samp = 0;
	int envwindow_sampsize = dz->iparam[XSPK_WINSZ];
	float *ibuf = dz->sampbuf[0];
	float *env = *envptr;
	double convertor = 1.0/F_ABSMAXSAMP;
	while(samps_to_process >= envwindow_sampsize) {
		*env++       = (float) (getmaxsamp(start_samp,envwindow_sampsize,ibuf) * convertor);
		start_samp  += envwindow_sampsize;
		samps_to_process -= envwindow_sampsize;
	}
	if(samps_to_process)	/* Handle any final short buffer */
		*env++ = (float)(getmaxsamp(start_samp,samps_to_process,ibuf) * convertor);
	*envptr = env;
	return FINISHED;
}

/****************************** WINDOWS_IN_SNDFILE [GET_ENVSIZE] ******************************/
 
int windows_in_sndfile(dataptr dz)
{
	int envcnt, winsize = dz->iparam[XSPK_WINSZ];
	if(((envcnt = dz->insams[0]/winsize) * winsize)!=dz->insams[0])
		envcnt++;
	return envcnt;
}

/*************************** GETMAXSAMP *****************************/

double getmaxsamp(int startsamp, int sampcnt,float *buffer)
{
	int  i, endsamp = startsamp + sampcnt;
	double thisval, thismaxsamp = 0.0;
	for(i = startsamp; i<endsamp; i++) {
		if((thisval =  fabs(buffer[i]))>thismaxsamp)
			thismaxsamp = thisval;
	}
	return thismaxsamp;
}

/*************************** EXTTROFSGET *************************/
 
int exttrofsget(dataptr dz)
{
 	int upwards, troffcnt = 0, thistrofat, lasttrofat, env_cnt, peaklen;
	int *trof;
 	float *p, *q, *ibuf = dz->sampbuf[0];
	double *maxarray, maxlevel;
	int envwindow_sampsize = dz->iparam[XSPK_WINSZ], n, k;
	int minseglen = (dz->iparam[XSPK_SPLEN] * 2) * dz->infile->channels;		//	Minimum segment to cut is larger than 2 splices.
	float *env, *envend;

	env = dz->fptr[0];
	envend = dz->fptr[1];
					//	 2 gpsplices     * chans;
	p = env+1;
 	q = env;
	if (*p > *q)
 		upwards = TRUE;
 	else
 		upwards = FALSE;
	troffcnt  = 0;
	lasttrofat = 0;
	dz->envcnt = 0;
 	while(p < envend) {
 		if(upwards) {
 			if(*p < *q) {
 				upwards = FALSE;
 			}
 		} else {
			if(*p > *q) {												//	Peak-segments (separated by trofs)
				if(istrof(env,envend,q,EXTSPEAK_PKSRCHWIDTH)) {
					thistrofat = dz->envcnt * envwindow_sampsize;
					peaklen = thistrofat - lasttrofat;
					if(peaklen > minseglen) {							//	Peak-segments must be longer than 2 splices
						troffcnt++;										//	This also skips getting a trof AT 0 time
						lasttrofat = thistrofat;
					}
				}
 				upwards = TRUE;
 			}
		}
 		p++;
 		q++;
		dz->envcnt++;
 	}
	if((dz->lparray[0] = (int *)malloc((troffcnt + 1) * sizeof(int))) == NULL) {	//	Allow for trof at EOF
		sprintf(errstr,"INSUFFICIENT MEMORY to store peak-locations.\n");
		return(MEMORY_ERROR);
	}
	if(dz->mode == 1 || dz->mode == 4) {
		if((dz->iparray[PERM_ARRAY] = (int *)malloc((dz->infilecnt - 1) * sizeof(int))) == NULL) {	//	Storage for permed syllable order
			sprintf(errstr,"INSUFFICIENT MEMORY to store permutation array.\n");
			return(MEMORY_ERROR);
		}
	}
	trof = dz->lparray[0];
	dz->trofcnt = troffcnt;
 	p = env+1;
 	q = env;
	env_cnt = 0;
	if (*p > *q)
 		upwards = TRUE;
 	else
 		upwards = FALSE;
	troffcnt = 0;
	lasttrofat = 0;
	while(env_cnt < dz->envcnt) {
 		if(upwards) {
 			if(*p < *q) {
 				upwards = FALSE;
 			}
 		} else {
			if(*p > *q) {													//	Peak-segments (separated by trofs)
				if(istrof(env,envend,q,EXTSPEAK_PKSRCHWIDTH)) {
					thistrofat = env_cnt * envwindow_sampsize;
					peaklen = thistrofat - lasttrofat;
					if(peaklen > minseglen) {
						trof[troffcnt++] = env_cnt * dz->iparam[XSPK_WINSZ];//	Peak-segments must be longer than 2 splices
						lasttrofat = thistrofat;							//	This also skips getting a trof AT 0 time
					}
				}
 				upwards = TRUE;
 			}
		}
 		p++;
 		q++;
		env_cnt++;
 	}
	trof[troffcnt] = dz->insams[0];		//	Add trof at EOF
													//Only get here if dz->mode < 6
	if(dz->vflag[XSPK_ENV] || dz->mode >= 3) {					//	Get syllable maxima on infile0
		if((dz->parray[1] = (double *)malloc((dz->trofcnt + 1) * sizeof(double))) == NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to create splice buffer (2).\n");
			return(MEMORY_ERROR);
		}
		maxarray = dz->parray[1];
		n = 0;
		lasttrofat = 0;
		while(n <= dz->trofcnt) {
			thistrofat = trof[n];
			k = lasttrofat;
			maxlevel = -HUGE;
			while(k < thistrofat) {
				maxlevel = max(maxlevel,fabs(ibuf[k]));
				k++;
			}
			maxarray[n] = maxlevel;
			lasttrofat = thistrofat;
			n++;
		}
	}
	return(FINISHED);
}

/*************************** CREATE_SNDBUFS_FOR_EXTSPEAK **************************
 *
 *	Only AFTER params have been read.
 */

int create_sndbufs_for_extspeak(dataptr dz)
{
	int bigbufsize, minobufsize, safety = 16;
	int n, k,chans = dz->infile->channels;
	double srate = (double)dz->infile->srate;
	/* All other cases */
	if(dz->bufcnt == dz->infilecnt + 3);		//	includes end-of-all bufs pointer
	if(dz->sbufptr == 0 || dz->sampbuf == 0) {
		sprintf(errstr,"buffer pointers not allocated: create_sndbufs_for_extspeak()\n");
		return(PROGRAM_ERROR);
	}
	/* minobufsize = 2 * sizeof splice */
	minobufsize = ((int)ceil((dz->param[XSPK_SPLEN] * MS_TO_SECS) * 2.0 * srate) + safety) * chans;
	minobufsize = max(minobufsize,512 * chans);
	bigbufsize  = 0;
	for(n = 0; n < dz->infilecnt; n++)
		bigbufsize += dz->insams[n];
	bigbufsize += minobufsize * 2;
	bigbufsize *=  sizeof(float);
	if((dz->bigbuf = (float *)malloc((bigbufsize))) == NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
		return(MEMORY_ERROR);
	}
	dz->sbufptr[0] = dz->sampbuf[0] = dz->bigbuf;
	for(n=0,k=1;n<dz->infilecnt;n++,k++)								//	Inbufs = size of infles
		dz->sbufptr[k] = dz->sampbuf[k] = dz->sampbuf[n] + dz->insams[n];
	dz->sbufptr[k] = dz->sampbuf[k] = dz->sampbuf[n] + minobufsize;		// outbuf and ovflwbuf are minobufsize length
	k++;
	n++;
	dz->sbufptr[k] = dz->sampbuf[k] = dz->sampbuf[n] + minobufsize;
	dz->buflen = minobufsize;
	return(FINISHED);
}

/*************************** ISTROF **********************************/
 
int istrof(float *env,float *envend,float *q,int width)
{
 	int up, down;
 	float *upq, *downq, *r;
 	if(width<2)
 		return(TRUE);
 	down = up = width/2;
	if(EVEN(width))
 		down = up - 1;		/* set search params above and below q */
 	downq = q - down;
 	upq   = q + up;
 	upq   = min(envend-1,upq);	/* allow for ends of envelope table */
 	downq = max(env,downq);
 	for(r = downq; r<=upq; r++) {
 		if(*q > *r)
 			return(FALSE);
 	}
 	return(TRUE);			/* if r is minimum of all in peak, return 1 */
 }
 
/*************************** PRECALCULATE_PEAKS_ARRAY_AND_SPLICE **********************************/
 
int precalculate_peaks_array_and_splice(dataptr dz)
{
	int exit_status, chans = dz->infile->channels;
	int gpsplicelen, splicelen, n, m, k;
	double splicincr, *splicebuf;
	int srate = dz->infile->srate;
	int *trof;
	double *maxarray, maxlevel;
	int thistrofat, lasttrofat;
	float *ibuf;

	dz->iparam[XSPK_SPLEN] = (int)round(dz->param[XSPK_SPLEN] * MS_TO_SECS * (double)srate);
	if(dz->mode < 6)
		dz->iparam[XSPK_WINSZ] = (int)round(dz->param[XSPK_WINSZ] * MS_TO_SECS * (double)srate) * chans;
	gpsplicelen = dz->iparam[XSPK_SPLEN];
	splicelen = gpsplicelen * chans;
	splicincr = 1.0/(double)gpsplicelen;
	if((dz->parray[0] = (double *)malloc((splicelen * sizeof(double)))) == NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create splice buffer (2).\n");
		return(MEMORY_ERROR);
	}
	splicebuf = dz->parray[0];
	for(n= 0, m = 0;n < gpsplicelen; n++, m += chans) {
		for(k = 0; k < chans; k++)
			splicebuf[m+k] = (double)n * splicincr;
	}
	if(dz->mode < 6) {
		if((exit_status = extract_env_from_sndfile(dz))< 0)
			return exit_status;
		if((dz->lparray=(int **)malloc(2 * sizeof(int *)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to store peaks.\n");
			return(MEMORY_ERROR);
		}
		if((dz->lparray[0]=(int *)malloc(dz->envcnt * sizeof(int)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to store peaks (2).\n");
			return(MEMORY_ERROR);
		}
		trof = dz->lparray[0];

		if((exit_status = exttrofsget(dz))< 0)
			return exit_status;
	} else {
		trof = dz->lparray[0];		//	Array was created when special-data file "cuts" was read
		ibuf = dz->sampbuf[0];
		if((dz->ssampsread = fgetfbufEx(dz->sampbuf[0],dz->insams[0],dz->ifd[0],0)) < 0) {
			sprintf(errstr,"Can't read samples from soundfile 1: precalculate_peaks_array_and_splice()\n");
			return(SYSTEM_ERROR);
		}
		if(dz->vflag[XSPK_ENV] || (dz->mode >= 9 && dz->mode < 12) || dz->mode >= 15) {			//	Get syllable maxima on infile0
			if((dz->parray[1] = (double *)malloc((dz->trofcnt + 1) * sizeof(double))) == NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY to create splice buffer (2).\n");
				return(MEMORY_ERROR);
			}
			maxarray = dz->parray[1];
			n = 0;
			lasttrofat = 0;
			while(n <= dz->trofcnt) {
				thistrofat = trof[n];
				k = lasttrofat;
				maxlevel = -HUGE;
				while(k < thistrofat) {
					maxlevel = max(maxlevel,fabs(ibuf[k]));
					k++;
				}
				maxarray[n] = maxlevel;
				lasttrofat = thistrofat;
				n++;
			}
		}
	}
	return FINISHED;
}

/********************************* RANDPERM *************************************/

void randperm(int z,int setlen,dataptr dz)
{
	int n, t;
	for(n=0;n<setlen;n++) {
		t = (int)floor(drand48() * (n+1));
		if(t>=n)
			hprefix(z,n,setlen,dz);
		else
			hinsert(z,n,t,setlen,dz);
	}
}

/***************************** HINSERT **********************************
 *
 * Insert the value m AFTER the T-th element in iparray[].
 */

void hinsert(int z,int m,int t,int setlen,dataptr dz)
{
	hshuflup(z,t+1,setlen,dz);
	dz->iparray[z][t+1] = m;
}

/***************************** HPREFIX ************************************
 *
 * Insert the value m at start of the permutation iparray[].
 */

void hprefix(int z,int m,int setlen,dataptr dz)
{
	hshuflup(z,0,setlen,dz);
	dz->iparray[z][0] = m;
}

/****************************** HSHUFLUP ***********************************
 *
 * move set members in iparray[] upwards, starting from element k.
 */

void hshuflup(int z,int k,int setlen,dataptr dz)
{
	int n, *i;
	int y = setlen - 1;
	i = (dz->iparray[z]+y);
	for(n = y;n > k;n--) {
		*i = *(i-1);
		i--;
	}
}

/****************************** GENERATE_XSPK_PATTERN ***********************************/

int generate_xspk_pattern(int XSPK_RRND,int patternsize,dataptr dz)
{
	int exit_status, lastperm, firstperm, n, patterncnt, syllabcnt, done = 0;
	int *pattern, *permm;
	if(dz->mode != 8 && dz->mode != 11 && dz->mode != 14 && dz->mode != 17) {		//	In these modes, pattern has already been read
		if((dz->iparray[PATN_ARRAY] = (int *)malloc(patternsize  * sizeof(int))) == NULL) {//	Storage for permed or swapped syllable order
			sprintf(errstr,"INSUFFICIENT MEMORY to store pattern of new syllables.\n");
			return(MEMORY_ERROR);
		}
	}
	pattern = dz->iparray[PATN_ARRAY];
	syllabcnt = dz->infilecnt - 1;	//	Count of files with syllables-for-insertion
	patterncnt = 0;
	switch(dz->mode) {
	case(12):	//	fall thro
	case(15):	//	fall thro
	case(9):	//	fall thro
	case(6):	//	fall thro
	case(3):	//	fall thro
	case(0):						//	Syllable pattern is in original order, or entirely random
		if(dz->vflag[XSPK_RRND]) {	//	Entirely random
			while(patterncnt < patternsize)
				pattern[patterncnt++] = (int)floor(drand48() * syllabcnt) + 1;
		} else {					//	No flags set : seq of infiles, (repeated) e.g. for 3 files 1 2 3 1 2 3 ....
			while(patterncnt < patternsize) {
				for(n = 0; n < syllabcnt; n++) {
					pattern[patterncnt] = n + 1;
					if(++patterncnt >= patternsize) {
							done = 1;
							break;
					}
				}
				if(done)
					break;
			}
		}
		break;
	case(16):	//	fall thro
	case(13):	//	fall thro
	case(10):	//	fall thro
	case(7):	//	fall thro
	case(4):	//	fall thro
	case(1):										//	Rand-permutation of ALL input syllables, then a different ditto, etc
		permm = dz->iparray[PERM_ARRAY];
		randperm(PERM_ARRAY,syllabcnt,dz);			//	For syllabcnt 3, perms values 0,1,2 into "permm"
		lastperm = permm[syllabcnt - 1];
		while(patterncnt < patternsize) {
			for(n = 0; n < syllabcnt; n++) {
				pattern[patterncnt] = permm[n] + 1;	//	Read permd value, but add 1 to index syllab buffers from 1 upwards
				if(++patterncnt >= patternsize) {
						done = 1;
						break;
				}
			}
			if(done)
				break;
			do {
				randperm(1,syllabcnt,dz);
				firstperm = permm[0];
			} while(firstperm == lastperm);
			lastperm = permm[syllabcnt - 1];		//	Ensure no syllable repeated betwren end of one perm & start of next
		}
		break;
	case(5):	//	fall thro
	case(2):						//	Syllable pattern is input as a file : read it, and expand to fill patternsize
		if((exit_status = handle_pattern_data(patternsize,dz))<0)
			return exit_status;
		break;
	case(8):	//	fall thro
	case(11):						//	Syllable pattern HAS ALREADY BEEN input as a file : expand it to fill patternsize
		if((exit_status = expand_pattern_data(patternsize,dz))<0)
			return exit_status;
		break;
	}
	return FINISHED;
}

/************************ HANDLE_PATTERN_DATA *********************/

int handle_pattern_data(int patternsize,dataptr dz)
{
	aplptr ap = dz->application;
	int patterncnt, linecnt, done, k, j;
	char temp[2000], *q, *filename = dz->wordstor[0];
	int *pattern = dz->iparray[PATN_ARRAY];
	int *p;
	ap->data_in_file_only = TRUE;
	ap->special_range 	  = TRUE;
	ap->min_special 	  = 1;
	ap->max_special 	  = dz->infilecnt - 1;

	if((dz->fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,"Cannot open pattern datafile %s\n",filename);
		return(DATA_ERROR);
	}
	p = pattern;
	done = 0;
	linecnt = 0;
	patterncnt = 0;
	while(fgets(temp,200,dz->fp)!=NULL) {
		q = temp;
		if(is_an_empty_line_or_a_comment(temp)) {
			linecnt++;
			continue;
		}
		while(get_int_from_within_string(&q,p,0)) {
			if(*p < ap->min_special || *p > ap->max_special) {
				sprintf(errstr,"Pattern number (%d) out of range (from %d to count of NEW syllables in infile list = %d) : file %s : line %d\n",
					(int)round(*p),(int)round(ap->min_special),dz->infilecnt - 1,filename,linecnt+1);
				return(DATA_ERROR);
			}
			if(++patterncnt >= patternsize) {
				done = 1;
				break;
			}
			p++;
		}
		if(done)
			break;
		linecnt++;
	}
	if(fclose(dz->fp)<0) {
		fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
		fflush(stdout);
	}
	if(patterncnt == 0) {
		sprintf(errstr,"No data found in file %s\n",filename);
		return(DATA_ERROR);
	}
	if(patterncnt < patternsize) {			//	Repeat pattern to fill pattern array
		for(k = patterncnt, j = 0; k < patternsize; k++,j++)
			pattern[k] = pattern[j];
	}
	return(FINISHED);
}

/************************************ STORE_PATTERNDATAFILE_NAME ***************************
 *
 *	Store the NAME of the special-data file, to open later.
 */

int store_patterndatafile_name(int *cmdlinecnt,char ***cmdline,dataptr dz)
{
	int exit_status;
	char *filename = (*cmdline)[0];
	if(!sloom) {
		if(*cmdlinecnt <= 0) {
			sprintf(errstr,"Insufficient parameters on command line.\n");
			return(USAGE_ONLY);
		}
	}
	if((dz->fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,"Cannot open pattern datafile %s\n",filename);
		return(DATA_ERROR);
	}
	fclose(dz->fp);
	if(dz->wordstor!=NULL)
		free_wordstors(dz);
	dz->all_words = 0;
	if((exit_status = store_filename(filename,dz))<0)
		return(exit_status);
	(*cmdline)++;
	(*cmdlinecnt)--;
	return(FINISHED);
}

/************************************ INSERT_NEW_SYLLABLE ***************************/

int insert_new_syllable(int peaklen,int patno,int splicelen,double gain,double origlevel,double normaliser,int *obufpos, float *obuf, float *ovflwbuf,dataptr dz)
{
	int exit_status, k, j, m, z, c, gpsmpcnt, chans = dz->infile->channels;
	double frac, diff, flincr, flpos, val;
	double *splicebuf = dz->parray[0];
	float *ibuf = dz->sampbuf[patno], *origibuf, ival;
	double *syllmax =  dz->parray[2];
	int ibufpos;
	int splicend = splicelen - 1;

	if(dz->vflag[XSPK_ENV])				//	Scale to level of original syllable
		gain *= origlevel/syllmax[patno];	//	pattern vals run from 1 (not from zero), and level also stored at 1 upwards

	if((dz->mode >= 3 && dz->mode < 6) || (dz->mode >= 9 && dz->mode < 12) || dz->mode >= 15) {
		origibuf = dz->sampbuf[0];
		ibufpos = dz->total_samps_written + *obufpos;
		switch(dz->vflag[XSPK_TRANSPOSE]) {
		case(0):						//	CUT OR PAD WITH ZEROS
			for(k = 0, j = peaklen - 1; k < peaklen; k++,j--) {
				if(k >= dz->insams[patno])												// add normalised orig into existing buffer
					ival = 0.0f;
				else
					ival = ibuf[k];
				if(k < splicelen)														//	do upslice of new syllab BUT ADD ~~NO~~ old syllab (its already there in downsplice)
					val = (origibuf[ibufpos++]  + (gain * ival)) * splicebuf[k];
				else if (j < splicelen)													//	do downslice of new syllab
					val = (origibuf[ibufpos++] + (gain * ival)) * splicebuf[splicend--];
				else																	//	just add, and normalise
					val = origibuf[ibufpos++] + (gain * ival);
				obuf[*obufpos] = (float)(obuf[*obufpos] + (val * normaliser));
				if(++(*obufpos) >= dz->buflen * 2) {
					if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
						return(exit_status);
					memcpy((char *)obuf,(char *)ovflwbuf,dz->buflen * sizeof(float));
					memset((char *)ovflwbuf,0,dz->buflen * sizeof(float));
					*obufpos -= dz->buflen;
				}
			}
			break;
		case(1):						//	TRANSPOSE
			flpos = 0.0;															//	Flincr increments as GPsampcnt
			flincr = (double)(dz->insams[patno])/(double)peaklen;
			flpos = 0.0;															//	Flincr increments as GPsampcnt
			for(k = 0,gpsmpcnt = 0,j = peaklen - chans;k < peaklen;gpsmpcnt++,k+=chans,j-=chans) {
				flpos = ((double)k/(double)peaklen) * (double)(dz->insams[patno]/chans);
				m = (int)floor(flpos);											//	Find the fractional part of the GPchans-xnt
				frac = flpos - (double)m;										//	Convert to the (channel-base) in the true sampcnt;
				m *= chans;
				for(c = 0; c < chans; c++) {									//	Do interp between ALL channels
					z = m + chans;
					val = ibuf[m];
					diff = ibuf[z] - ibuf[m];
					val +=  diff * frac;
					if(k < splicelen)											//	do upslice of new syllab
						val = (origibuf[ibufpos++]  + (gain * val)) * splicebuf[k];
					else if (j < splicelen)										//	do downslice of new syllab mixing with orig
						val = (origibuf[ibufpos++] + (gain * val)) * splicebuf[splicend--];
					else														//	just add, and normalise
						val = origibuf[ibufpos++] + (gain * val);
					obuf[*obufpos] = (float)(obuf[*obufpos] + (val * normaliser));
					m++;
					(*obufpos)++;
				}
				if(*obufpos >= dz->buflen * 2) {
					if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
						return(exit_status);
					memcpy((char *)obuf,(char *)ovflwbuf,dz->buflen * sizeof(float));
					memset((char *)ovflwbuf,0,dz->buflen * sizeof(float));
					*obufpos -= dz->buflen;
				}
			}
			break;
		}
		return FINISHED;
	}
	switch(dz->vflag[XSPK_TRANSPOSE]) {
	case(0):						//	CUT OR PAD WITH ZEROS
		for(k = 0, j = peaklen - 1; k < peaklen; k++,j--) {
			if(k >= dz->insams[patno])												// add nothing into existing buffer
				;
			else if(k < splicelen)													//	do upslice
				obuf[*obufpos] = (float)(obuf[*obufpos] + (gain * (ibuf[k] * splicebuf[k])));
			else if (j < splicelen)													//	do downslice
				obuf[*obufpos] = (float)(obuf[*obufpos] + (gain * (ibuf[k] * splicebuf[splicend--])));
			else
				obuf[*obufpos] = (float)(obuf[*obufpos] + (gain * ibuf[k]));		//	just copy
			if(++(*obufpos) >= dz->buflen * 2) {
				if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
					return(exit_status);
				memcpy((char *)obuf,(char *)ovflwbuf,dz->buflen * sizeof(float));
				memset((char *)ovflwbuf,0,dz->buflen * sizeof(float));
				*obufpos -= dz->buflen;
			}
		}
		break;
	case(1):						//	TRANSPOSE
		for(k = 0,gpsmpcnt = 0,j = peaklen - chans;k < peaklen;gpsmpcnt++,k+=chans,j-=chans) {
			flpos = ((double)k/(double)peaklen) * (double)(dz->insams[patno]/chans);
			m = (int)floor(flpos);											//	Find the fractional part of the GPchans-xnt
			frac = flpos - (double)m;										//	Convert to the (channel-base) in the true sampcnt;
			m *= chans;
			for(c = 0; c < chans; c++) {									//	Do interp between ALL channels
				z = m + chans;
				val = ibuf[m];
				diff = ibuf[z] - ibuf[m];
				val  += (diff * frac);
				if(k < splicelen)
					obuf[*obufpos] = (float)(obuf[*obufpos] + (gain * val * splicebuf[k]));
				else if (j < splicelen)										//	do upslice
					obuf[*obufpos] = (float)(obuf[*obufpos] + (gain * val * splicebuf[splicend--]));
				else														//	do downslice
					obuf[*obufpos] = (float)(obuf[*obufpos] + (gain * val));
				m++;														//	just add in
				(*obufpos)++;
			}
			if(*obufpos >= dz->buflen * 2) {
				if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
					return(exit_status);
				memcpy((char *)obuf,(char *)ovflwbuf,dz->buflen * sizeof(float));
				memset((char *)ovflwbuf,0,dz->buflen * sizeof(float));
				*obufpos -= dz->buflen;
			}
		}
		break;
	}
	return FINISHED;
}

/************************ HANDLE_EXTRA_INFILES *********************/

int handle_the_extra_infiles(int *cmdlinecnt,char ***cmdline,dataptr dz)
{
					/* OPEN ANY FURTHER INFILES, CHECK COMPATIBILITY, STORE DATA AND INFO */
	int  exit_status;
	int  n;
	char *filename;
	fileptr fp2 = NULL;
	for(n=1;n<dz->infilecnt;n++) {
		filename = (*cmdline)[0];
		if((exit_status = open_checktype_getsize_and_compare_header(filename,n,&fp2,dz))<0)
			return(exit_status);
		(*cmdline)++;
		(*cmdlinecnt)--;
	}
	return(FINISHED);
}

/************************** GET_INT_FROM_WITHIN_STRING **************************
 * takes a pointer TO A POINTER to a string. If it succeeds in finding
 * aN INT it returns the int value (*val), and it's new position in the
 * string (*str).
 */

int get_int_from_within_string(char **str,int *val,int minus_one_ok)
{
	char *p, *valstart;
	int  has_digits = 0;
	p = *str;
	while(isspace(*p))
		p++;
	valstart = p;
	if(minus_one_ok) {
		if(*p == NEGATION) {
			p++;
			if(*p != 1 + INT_TO_ASCII)	//	Only neg number allowed is -1
				return(FALSE);
		}
	}
	if(!isdigit(*p))
		return(FALSE);
	has_digits = TRUE;
	p++;
	while(!isspace(*p) && *p!=NEWLINE && *p!=ENDOFSTR) {
		if(isdigit(*p))
			has_digits = TRUE;
		p++;
	}
	if(!has_digits || sscanf(valstart,"%d",val)!=1)
		return(FALSE);
	*str = p;
	return(TRUE);
}

/************************** OPEN_CHECKTYPE_GETSIZE_AND_COMPARE_HEADER *****************************/

int open_checktype_getsize_and_compare_header(char *filename,int fileno,fileptr *fp2,dataptr dz)
{
	int exit_status;
	infileptr ifp;
	double maxamp, maxloc;
	int maxrep;
	int getmax = 0, getmaxinfo = 0;
	fileptr fp1 = dz->infile;

	if((dz->ifd[fileno] = sndopenEx(filename,0,CDP_OPEN_RDONLY)) < 0) {
		sprintf(errstr,"cannot open input file %s to read data.\n",filename);
		return(DATA_ERROR);
	}
	if((ifp = (infileptr)malloc(sizeof(struct filedata)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store data on later infile. (1)\n");
		return(MEMORY_ERROR);
	}
	if((*fp2 = (fileptr)malloc(sizeof(struct fileprops)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store data on later infile. (2)\n");
		return(MEMORY_ERROR);
	}
	if((exit_status = readhead(ifp,dz->ifd[fileno],filename,&maxamp,&maxloc,&maxrep,getmax,getmaxinfo))<0)
		return(exit_status);
	copy_to_fileptr(ifp,*fp2);
	if((*fp2)->filetype != SNDFILE) {
		sprintf(errstr,"%s is not a sound file.\n",filename);
		return(DATA_ERROR);
	}
	if((*fp2)->filetype != fp1->filetype) {
		sprintf(errstr,"Incompatible file type in input file %s.\n",filename);
		return(DATA_ERROR);
	}
	if((*fp2)->srate != fp1->srate) {
		sprintf(errstr,"Incompatible sample-rate in input file %s.\n",filename);
		return(DATA_ERROR);
	}
	if((*fp2)->channels != fp1->channels) {
		sprintf(errstr,"Incompatible channel-count in input file %s.\n",filename);
		return(DATA_ERROR);
	}
	if((dz->insams[fileno] = sndsizeEx(dz->ifd[fileno]))<0) {	    /* FIND SIZE OF FILE */
		sprintf(errstr, "Can't read size of input file %s.\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/***************************************** FIND_ORIG_SYLLAB_MAXIMA *******************************/

int find_orig_syllab_maxima(int splicelen,dataptr dz)
{
	int thistrofat, lasttrofat, n, k;
	int *trof = dz->lparray[0];
	double maxlevel, *origmax = dz->parray[1];
	float *ibuf = dz->sampbuf[0];
	lasttrofat = 0;
	for(n = 0; n <= dz->trofcnt;n++) {
		thistrofat = trof[n];
		lasttrofat = max(0,lasttrofat - splicelen);
		maxlevel = 0.0;
		for(k = lasttrofat; k < thistrofat; k++)
			maxlevel = max(maxlevel,ibuf[k]);
		origmax[n] = maxlevel;
		lasttrofat = thistrofat;
	}
	return FINISHED;
}

/***************************************** FIND_NORMALISER *******************************/

int find_normaliser(double *normaliser,int samps_per_sec,int splicelen,dataptr dz)
{
	int exit_status, patterncnt, patno, z, n, done, orig;
	double newmaxlevel, time = 0.0;
	int lasttrofat, thistrofat, peaklen;
	int *trof = dz->lparray[0];
	int  *pattern = dz->iparray[PATN_ARRAY];
	double thislevel, *origlevel = dz->parray[1], *syllmax = dz->parray[2];
	done = 0;
	orig = 0;
	patterncnt = 0;
	lasttrofat = 0;
	newmaxlevel = 0.0;
	z = 0;
	while(z < dz->iparam[XSPK_OFFST]) {
		newmaxlevel = max(newmaxlevel,origlevel[z]);
		z++;
	}
	for(n = z; n <= dz->trofcnt; n++)  {
		if(n > 0)
			lasttrofat = trof[n-1];
		lasttrofat = max(0,lasttrofat - splicelen);
		thistrofat = trof[n];
		peaklen = thistrofat - lasttrofat;
		time = get_syllable_time(thistrofat,lasttrofat,peaklen,n,samps_per_sec,dz);
		if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
			return exit_status;
		switch(dz->vflag[XSPK_KEEP]) {
		case(0):					//	N  = the number of syllables to insert
			newmaxlevel = max(newmaxlevel,origlevel[n]);	//	Just read level of file0 syllable
			for(z = 0; z < dz->iparam[XSPK_N]; z++) {		//	Then Consider N NEW syllables
				if(++n >= dz->trofcnt) {					//	Advance count of original syllables
					done = 1;								//	and If out of original syllables, quit
					break;
				}
				if(dz->brksize[XSPK_GAIN]) {
					lasttrofat = thistrofat;			//	Reset time to read any gain table
					thistrofat = trof[n];
					time = get_syllable_time(thistrofat,lasttrofat,peaklen,n,samps_per_sec,dz);
					if((exit_status = read_value_from_brktable(time,XSPK_GAIN,dz))<0)
						return exit_status;
				}
				patno = pattern[patterncnt++];
				if(dz->vflag[XSPK_ENV])
					thislevel = origlevel[n] + (origlevel[n] * dz->param[XSPK_GAIN]);					//	File0-level + (scaled fileN-level X gain)
				else
					thislevel = origlevel[n] + (syllmax[patno] * dz->param[XSPK_GAIN]);					//	File0-level + (fileN-level X gain)
				newmaxlevel = max(newmaxlevel,thislevel);
			}
			break;
		case(1):					//	N  = the number of syllables to keep
			if(orig < dz->iparam[XSPK_N]) {			//	Add an original syllable, Keeping count of how many added
				newmaxlevel = max(newmaxlevel,origlevel[n]);
				orig++;								//	Count how many orig syllabs added
			} else {								//	If all N orig syllabs have already been added
				if(++n >= dz->trofcnt) {			//	Advance count of original syllables
					done = 1;						//	and If out of original syllables, quit
					break;
				}
				if(dz->brksize[XSPK_GAIN]) {
					lasttrofat = thistrofat;
					thistrofat = trof[n];
					time = get_syllable_time(thistrofat,lasttrofat,peaklen,n,samps_per_sec,dz);
					if((exit_status = read_value_from_brktable(time,XSPK_GAIN,dz))<0)
						return exit_status;
				}
				patno = pattern[patterncnt++];		//	Add JUST 1 NEW syllable
				if(dz->vflag[XSPK_ENV])
					thislevel = origlevel[n] + (origlevel[n] * dz->param[XSPK_GAIN]);				//	File0-level + (scaled fileN-level X gain)
				else
					thislevel = origlevel[n] + (syllmax[patno] * dz->param[XSPK_GAIN]);				//	File0-level + (fileN-level X gain)
				newmaxlevel = max(newmaxlevel,thislevel);
				orig = 0;							//	Reset orig-syllabs counter
			}
			break;
		}
		if(done)
			break;
	}
	if(newmaxlevel > XSPK_MAXLEVEL)
		*normaliser = XSPK_MAXLEVEL/newmaxlevel;
	return FINISHED;
}

/***************************************** FIND_NORMALISER_TARGET *******************************/

int find_normaliser_target(double *normaliser,int samps_per_sec,int splicelen,dataptr dz)
{
	int exit_status, patterncnt, patno, n;
	double newmaxlevel, time = 0.0;
	int lasttrofat, thistrofat, peaklen;
	int *trof = dz->lparray[0];
	int  target_cnt, *pattern = dz->iparray[PATN_ARRAY], *target = dz->iparray[TRGT_ARRAY];
	double thislevel, *origlevel = dz->parray[1], *syllmax = dz->parray[2];
	lasttrofat = 0;
	target_cnt = 0;
	patterncnt = 0;
	newmaxlevel = 0.0;
	for(n = 0; n <= dz->trofcnt; n++)  {
		if(n > 0)
			lasttrofat = trof[n-1];
		lasttrofat = max(0,lasttrofat - splicelen);
		thistrofat = trof[n];
		peaklen = thistrofat - lasttrofat;
		if(target_cnt < dz->targetcnt &&  n == target[target_cnt]) {
			if(dz->brksize[XSPK_GAIN]) {
				time = get_syllable_time(thistrofat,lasttrofat,peaklen,n,samps_per_sec,dz);
				if((exit_status = read_value_from_brktable(time,XSPK_GAIN,dz))<0)
					return exit_status;
			}
			patno = pattern[patterncnt++];
			if(dz->vflag[XSPK_ENV])
				thislevel = origlevel[n] + (origlevel[n] * dz->param[XSPK_GAIN]);					//	File0-level + (scaled fileN-level X gain)
			else
				thislevel = origlevel[n] + (syllmax[patno] * dz->param[XSPK_GAIN]);					//	File0-level + (fileN-level X gain)
			newmaxlevel = max(newmaxlevel,thislevel);
			target_cnt++;
		} else
			newmaxlevel = max(newmaxlevel,origlevel[n]);
	}
	if(newmaxlevel > XSPK_MAXLEVEL)
		*normaliser = XSPK_MAXLEVEL/newmaxlevel;
	return FINISHED;
}

/*************************************** GET_SYLLABLE_TIME ****************************************/

double get_syllable_time(int thistrofat,int lasttrofat,int peaklen,int n,int samps_per_sec,dataptr dz)
{
	double time;
	if(lasttrofat == 0)				//	time is zero at first syllab of file0
		time = 0,0;
	else if(n == dz->trofcnt)		//	time is EOF at last syllab of file0
		time = (double)thistrofat/(double)samps_per_sec;
	else							//	ELSE time is mid-point of syllable
		time = (double)(thistrofat + (peaklen/2))/(double)samps_per_sec;
	return time;
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

/******************************** GETCUTDATA ***********************/

int getcutdata(int *cmdlinecnt,char ***cmdline,dataptr dz)
{
	aplptr ap = dz->application;
	int troffcnt, linecnt, chans = dz->infile->channels, srate = dz->infile->srate;
	char temp[2000], *q, *filename = (*cmdline)[0];
	int *trof;
	double *p, dummy;
	ap->data_in_file_only = TRUE;
	ap->special_range 	  = TRUE;
	ap->min_special 	  = 0;
	ap->max_special 	  = dz->duration;

	if((dz->fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,"Cannot open pattern datafile %s\n",filename);
		return(DATA_ERROR);
	}
	p = &dummy;
	linecnt = 0;
	troffcnt = 0;
	while(fgets(temp,200,dz->fp)!=NULL) {
		q = temp;
		if(is_an_empty_line_or_a_comment(temp)) {
			linecnt++;
			continue;
		}
		while(get_float_from_within_string(&q,p)) {
			if(*p < ap->min_special || *p > ap->max_special) {
				sprintf(errstr,"Cut position (%lf) out of range (%lf to %lf): file %s : line %d\n",
					*p,ap->min_special,ap->max_special,filename,linecnt+1);
				return(DATA_ERROR);
			}
			troffcnt++;
		}
		linecnt++;
	}
	if(troffcnt == 0) {
		if(fclose(dz->fp)<0) {
			fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
			fflush(stdout);
		}
		sprintf(errstr,"No data found in file %s\n",filename);
		return(DATA_ERROR);
	}
	if((dz->lparray=(int **)malloc(2 * sizeof(int *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store trofs (1).\n");
		return(MEMORY_ERROR);
	}
	if((dz->lparray[0] = (int *)malloc((troffcnt + 1) * sizeof(int))) == NULL) {		//	Storage for syllable edit-point data
		sprintf(errstr,"INSUFFICIENT MEMORY to store trofs (2).\n");
		return(MEMORY_ERROR);
	}
	if(dz->mode == 7 || dz->mode == 10) {
		if((dz->iparray[PERM_ARRAY] = (int *)malloc((dz->infilecnt - 1) * sizeof(int))) == NULL) {	//	Storage for permed syllable order
			sprintf(errstr,"INSUFFICIENT MEMORY to store permutation array.\n");
			return(MEMORY_ERROR);
		}
	}
	if(fseek(dz->fp,0,0)< 0) {
		sprintf(errstr,"Failed to return to start of file %s.\n",filename);
		return SYSTEM_ERROR;
	}
	troffcnt = 0;
	trof = dz->lparray[0];
	while(fgets(temp,200,dz->fp)!=NULL) {
		q = temp;
		if(is_an_empty_line_or_a_comment(temp))
			continue;
		while(get_float_from_within_string(&q,p))
			trof[troffcnt++] = (int)(*p * (double)srate) * chans;
	}
	if(fclose(dz->fp)<0) {
		fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
		fflush(stdout);
	}
	if(trof[troffcnt - 1] >= dz->insams[0]-chans)	//	Ignore any splice at End-of-file
		troffcnt--;									//	by moving it to the troffcnt spot
	else
		trof[troffcnt] = dz->insams[0];
	dz->trofcnt = troffcnt;
	(*cmdline)++;
	(*cmdlinecnt)--;
	return FINISHED;
}

/************************ GET_CUTPAT_DATA *********************/

int get_cutpat_data(int *cmdlinecnt,char ***cmdline,dataptr dz)
{
	aplptr ap = dz->application;
	int troffcnt, patterncnt, linecnt, gottrof, chans = dz->infile->channels, srate = dz->infile->srate;
	char temp[2000], *q, *filename = (*cmdline)[0];
	int maxpatternsize, *trof;
	double *dp, ddummy;
	int *ip, idummy, *pattern;
	ap->data_in_file_only = TRUE;
	ap->special_range 	  = TRUE;
	ap->min_special 	  = 0;
	ap->max_special		  = dz->duration;
	ap->min_special2	  = 1;
	ap->max_special2	  = dz->infilecnt - 1;

	if((dz->fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,"Cannot open splicing & pattern datafile %s\n",filename);
		return(DATA_ERROR);
	}
	dp = &ddummy;
	ip = &idummy;
	gottrof = 0;
	linecnt = 1;
	troffcnt = 0;
	patterncnt = 0;
	while(fgets(temp,200,dz->fp)!=NULL) {
		q = temp;
		if(temp[0] == '#') {
			if(gottrof) {
				sprintf(errstr,"Too many  \"#\" lines in the file\n");
				return(DATA_ERROR);
			}
			if(strlen(temp) != 1) {
				q++;
				while(*q != ENDOFSTR) {			//	Catch error like #1 1 2 3 4, with pattern item(s) on same line as #
					if(!isspace(*q)) {
						sprintf(errstr,"Spurious characters found on \"#\" line in file %s\n",filename);
						if(fclose(dz->fp)<0) {
							fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
							fflush(stdout);
						}
						return DATA_ERROR;
					}
					q++;
				}
			}
			gottrof = 1;
			linecnt++;
			continue;
		}
		else if(is_an_empty_line_or_a_comment(temp)) {
			linecnt++;
			continue;
		}
		if(gottrof) {		//	Looking for pattern numbers
			while(get_int_from_within_string(&q,ip,0)) {
				if(*ip < ap->min_special2 || *ip > ap->max_special2) {
					sprintf(errstr,"Pattern number (%d) out of range (from %d to count of NEW syllables in infile list = %d) : file %s : line %d\n",
					*ip,(int)round(ap->min_special2),(int)round(ap->max_special2),filename,linecnt);
					return(DATA_ERROR);
				}
				patterncnt++;
			}
		} else {		//	Looking for file edit times
 			while(get_float_from_within_string(&q,dp)) {
				if(*dp < ap->min_special || *dp > ap->max_special) {
					sprintf(errstr,"Cut position (%lf) out of range (%lf to %lf): file %s : line %d\n",
						*dp,ap->min_special,ap->max_special,filename,linecnt);
					return(DATA_ERROR);
				}
				troffcnt++;
			}
		}
		linecnt++;
	}
	if(troffcnt == 0 || patterncnt == 0) {
		if(fclose(dz->fp)<0) {
			fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
			fflush(stdout);
		}
		if(troffcnt == 0 && patterncnt == 0) {
			sprintf(errstr,"No data found in file %s\n",filename);
			return(DATA_ERROR);
		} else if(troffcnt == 0) {
			sprintf(errstr,"No file edit times found in file %s\n",filename);
			return(DATA_ERROR);
		} else {
			if(!gottrof) {
				sprintf(errstr,"No separator \"#\" found in file %s\n",filename);
				return(DATA_ERROR);
			} else {
				sprintf(errstr,"No pattern data found in file %s\n",filename);
				return(DATA_ERROR);
			}
		}
	}
	if((dz->lparray=(int **)malloc(2 * sizeof(int *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store cut times (1).\n");
		return(MEMORY_ERROR);
	}
	if((dz->lparray[0] = (int *)malloc((troffcnt + 1) * sizeof(int))) == NULL) {		//	Storage for syllable edit-point data
		sprintf(errstr,"INSUFFICIENT MEMORY to store source cut times (2).\n");
		return(MEMORY_ERROR);
	}
	if(dz->mode == 7 || dz->mode == 10) {
		if((dz->iparray[PERM_ARRAY] = (int *)malloc((dz->infilecnt - 1) * sizeof(int))) == NULL) {	//	Storage for permed syllable order
			sprintf(errstr,"INSUFFICIENT MEMORY to store permutation array.\n");
			return(MEMORY_ERROR);
		}
	}
	maxpatternsize = (troffcnt+1) * (1 + MAX_PATN);
	if((dz->iparray[PATN_ARRAY] = (int *)malloc(maxpatternsize  * sizeof(int))) == NULL) {	//	Storage for pattern of syllables
		sprintf(errstr,"INSUFFICIENT MEMORY to store pattern of new syllables.\n");
		return(MEMORY_ERROR);
	}
	if(fseek(dz->fp,0,0)< 0) {
		sprintf(errstr,"Failed to return to start of file %s.\n",filename);
		return SYSTEM_ERROR;
	}
	dp = &ddummy;
	ip = &idummy;
	gottrof = 0;
	linecnt = 0;
	trof = dz->lparray[0];
	pattern = dz->iparray[PATN_ARRAY];
	troffcnt = 0;
	patterncnt = 0;
	while(fgets(temp,200,dz->fp)!=NULL) {
		q = temp;
		if(temp[0] == '#') {
			gottrof = 1;
			continue;
		}
		if(is_an_empty_line_or_a_comment(temp))
			continue;
		if(gottrof) {		//	Looking for pattern numbers
			while(get_int_from_within_string(&q,ip,0))
				pattern[patterncnt++] = *ip;
		} else {		//	Looking for file edit times
 			while(get_float_from_within_string(&q,dp))
				trof[troffcnt++] = (int)(*dp * (double)srate) * chans;
		}
	}
	if(fclose(dz->fp)<0) {
		fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
		fflush(stdout);
	}
	if(trof[troffcnt - 1] >= dz->insams[0]-chans)	//	Ignore any splice at End-of-file
		troffcnt--;									//	by moving it to the troffcnt spot
	else
		trof[troffcnt] = dz->insams[0];
	dz->trofcnt = troffcnt;
	dz->itemcnt = patterncnt;
	(*cmdline)++;
	(*cmdlinecnt)--;
	return(FINISHED);
}

/************************ EXPAND_PATTERN_DATA *********************/

int expand_pattern_data(int patternsize,dataptr dz)
{
	int k, j;
	int *pattern = dz->iparray[PATN_ARRAY];
	if(dz->itemcnt < patternsize) {			//	Repeat pattern to fill pattern array : Array already has max possible size for patterns
		for(k = dz->itemcnt, j = 0; k < patternsize; k++,j++)
			pattern[k] = pattern[j % dz->itemcnt];
	}
	return FINISHED;
}

/************************ GET_CUTTARG_DATA *********************/

int get_cuttarg_data(int *cmdlinecnt,char ***cmdline,dataptr dz)
{
	aplptr ap = dz->application;
	int troffcnt, target_cnt, linecnt, gottrof, chans = dz->infile->channels, srate = dz->infile->srate;
	char temp[2000], *q, *filename = (*cmdline)[0];
	int *trof;
	double *dp, ddummy;
	int *ip, idummy, *target, n, m, k;
	ap->data_in_file_only = TRUE;
	ap->special_range 	  = TRUE;
	ap->min_special 	  = 0;
	ap->max_special		  = dz->duration;
	ap->min_special2	  = 1;
	ap->max_special2	  = dz->infilecnt - 1;

	if((dz->fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,"Cannot open splicing & target datafile %s\n",filename);
		return(DATA_ERROR);
	}
	dp = &ddummy;
	ip = &idummy;
	gottrof = 0;
	linecnt = 1;
	troffcnt = 0;
	target_cnt = 0;
	while(fgets(temp,200,dz->fp)!=NULL) {
		q = temp;
		if(temp[0] == '#') {
			if(gottrof) {
				sprintf(errstr,"Too many  \"#\" lines in the file\n");
				return(DATA_ERROR);
			}
			if(strlen(temp) != 1) {
				q++;
				while(*q != ENDOFSTR) {			//	Catch error like #1 1 2 3 4, with target item(s) on same line as #
					if(!isspace(*q)) {
						sprintf(errstr,"Spurious characters found on \"#\" line in file %s\n",filename);
						if(fclose(dz->fp)<0) {
							fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
							fflush(stdout);
						}
						return DATA_ERROR;
					}
					q++;
				}
			}
			gottrof = 1;
			linecnt++;
			continue;
		}
		else if(is_an_empty_line_or_a_comment(temp)) {
			linecnt++;
			continue;
		}
		if(gottrof) {		//	Looking for target numbers
			while(get_int_from_within_string(&q,ip,1)) {
				if((*ip != -1) && (*ip == 0 || *ip > troffcnt+1)) {
					sprintf(errstr,"Target number (%d) out of range (from 1 to count of syllabs within file0 (%d)  = %d) OR -1 (for last syllab)le: file %s : line %d\n",
					*ip,troffcnt+1,filename,linecnt);
					return(DATA_ERROR);
				}
				target_cnt++;
			}
		} else {		//	Looking for file edit times
 			while(get_float_from_within_string(&q,dp)) {
				if(*dp < ap->min_special || *dp > ap->max_special) {
					sprintf(errstr,"Cut position (%lf) out of range (%lf to %lf): file %s : line %d\n",
						*dp,ap->min_special,ap->max_special,filename,linecnt);
					return(DATA_ERROR);
				}
				troffcnt++;
			}
		}
		linecnt++;
	}
	if(troffcnt == 0 || target_cnt == 0) {
		if(fclose(dz->fp)<0) {
			fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
			fflush(stdout);
		}
		if(troffcnt == 0 && target_cnt == 0) {
			sprintf(errstr,"No data found in file %s\n",filename);
			return(DATA_ERROR);
		} else if(troffcnt == 0) {
			sprintf(errstr,"No file edit times found in file %s\n",filename);
			return(DATA_ERROR);
		} else {
			if(!gottrof) {
				sprintf(errstr,"No separator \"#\" found in file %s\n",filename);
				return(DATA_ERROR);
			} else {
				sprintf(errstr,"No target data found in file %s\n",filename);
				return(DATA_ERROR);
			}
		}
	}
	if((dz->lparray=(int **)malloc(2 * sizeof(int *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store source cut times (1).\n");
		return(MEMORY_ERROR);
	}
	if((dz->lparray[0] = (int *)malloc((troffcnt + 1) * sizeof(int))) == NULL) {		//	Storage for syllable edit-point data
		sprintf(errstr,"INSUFFICIENT MEMORY to store source cut times (2).\n");
		return(MEMORY_ERROR);
	}
	if((dz->iparray[TRGT_ARRAY] = (int *)malloc((troffcnt+1) * sizeof(int))) == NULL) {	//	Storage for targeted syllables
		sprintf(errstr,"INSUFFICIENT MEMORY to store target syllables.\n");
		return(MEMORY_ERROR);
	}
	if(dz->mode == 13 || dz->mode == 16) {
		if((dz->iparray[PERM_ARRAY] = (int *)malloc((dz->infilecnt - 1) * sizeof(int))) == NULL) {	//	Storage for permed syllable order
			sprintf(errstr,"INSUFFICIENT MEMORY to store permutation array.\n");
			return(MEMORY_ERROR);
		}
	}
	if(fseek(dz->fp,0,0)< 0) {
		sprintf(errstr,"Failed to return to start of file %s.\n",filename);
		return SYSTEM_ERROR;
	}
	dp = &ddummy;
	ip = &idummy;
	gottrof = 0;
	linecnt = 0;
	trof = dz->lparray[0];
	target  = dz->iparray[TRGT_ARRAY];
	troffcnt = 0;
	target_cnt = 0;
	while(fgets(temp,200,dz->fp)!=NULL) {
		q = temp;
		if(temp[0] == '#') {
			gottrof = 1;
			continue;
		}
		if(is_an_empty_line_or_a_comment(temp))
			continue;
		if(gottrof) {		//	Looking for target numbers
			while(get_int_from_within_string(&q,ip,1)) {
				if(*ip < 0)
					*ip = troffcnt + 1;
				target[target_cnt++] = *ip;
			}
		} else {		//	Looking for file edit times
 			while(get_float_from_within_string(&q,dp))
				trof[troffcnt++] = (int)(*dp * (double)srate) * chans;
		}
	}
	if(fclose(dz->fp)<0) {
		fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
		fflush(stdout);
	}
	if(trof[troffcnt - 1] >= dz->insams[0]-chans)	//	Ignore any splice at End-of-file
		troffcnt--;									//	by moving it to the troffcnt spot
	else
		trof[troffcnt] = dz->insams[0];
	dz->trofcnt = troffcnt;
	for(n = 0; n < target_cnt-1;n++) {				//	Sort targets to ascending order, and erase duplicates
		for(m = n+1; m < target_cnt; m++) {
			if(target[n] == target[m]) {
				fprintf(stdout,"syllable %d targeted more than once.\n",target[n]);
				fflush(stdout);
				if(m < target_cnt - 1) {
					for(k = m+1;k < target_cnt;k++)
						target[k-1] = target[k];	//	Erase by shufldown
				}
				target_cnt--;
				m--;
			} else if(target[m] < target[n]) {
				k = target[m];
				target[m] = target[n];
				target[n] = k;
			}
		}
	}
	for(n = 0; n < target_cnt;n++)					//	Make targets count from zero
		target[n]--;
	dz->targetcnt = target_cnt;
	(*cmdline)++;
	(*cmdlinecnt)--;
	return(FINISHED);
}

/************************ GET_CUTTAPA_DATA *********************/

int get_cuttapa_data(int *cmdlinecnt,char ***cmdline,dataptr dz)
{
	aplptr ap = dz->application;
	int troffcnt, target_cnt, patterncnt, linecnt, gothash, chans = dz->infile->channels, srate = dz->infile->srate;
	char temp[2000], *q, *filename = (*cmdline)[0];
	int *trof;
	double *dp, ddummy;
	int *ip, idummy, *pattern, *target, n, m, k;
	ap->data_in_file_only = TRUE;
	ap->special_range 	  = TRUE;
	ap->min_special 	  = 0;
	ap->max_special		  = dz->duration;
	ap->min_special2	  = 1;
	ap->max_special2	  = dz->infilecnt - 1;

	if((dz->fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,"Cannot open splicing,pattern and targeting datafile %s\n",filename);
		return(DATA_ERROR);
	}
	dp = &ddummy;
	ip = &idummy;
	gothash = 0;
	linecnt = 1;
	troffcnt = 0;
	target_cnt = 0;
	patterncnt = 0;
	while(fgets(temp,200,dz->fp)!=NULL) {
		q = temp;
		if(temp[0] == '#') {
			if(gothash == 2) {
				sprintf(errstr,"Too many \"#\" signs in datafile %s\n",filename);
				return(DATA_ERROR);
			}
			if(strlen(temp) != 1) {
				q++;
				while(*q != ENDOFSTR) {			//	Catch error like #1 1 2 3 4, with pattern item(s) on same line as #
					if(!isspace(*q)) {
						if(gothash)
							sprintf(errstr,"Spurious characters found on second \"#\" line in file %s\n",filename);
						else
							sprintf(errstr,"Spurious characters found on first \"#\" line in file %s\n",filename);
						if(fclose(dz->fp)<0) {
							fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
							fflush(stdout);
						}
						return DATA_ERROR;
					}
					q++;
				}
			}
			gothash++;
			linecnt++;
			continue;
		}
		else if(is_an_empty_line_or_a_comment(temp)) {
			linecnt++;
			continue;
		}
		if(gothash == 2) {		//	Looking for target numbers
			while(get_int_from_within_string(&q,ip,1)) {
				if((*ip != -1) && (*ip == 0 || *ip > troffcnt+1)) {
					sprintf(errstr,"Target number (%d) out of range (from 1 to count of syllabs within file0 = %d) OR -1 for last syllable: file %s : line %d\n",
					*ip,troffcnt+1,filename,linecnt);
					return(DATA_ERROR);
				}
				target_cnt++;
			}
		} else if(gothash == 1) {		//	Looking for pattern
			while(get_int_from_within_string(&q,ip,0)) {
				if(*ip < ap->min_special2 || *ip > ap->max_special2) {
					sprintf(errstr,"Pattern number (%d) out of range (from %d to count of NEW syllables (%d) in infile list = %d) : file %s : line %d\n",
					*ip,dz->infilecnt-1,(int)round(ap->min_special2),(int)round(ap->max_special2),filename,linecnt);
					return(DATA_ERROR);
				}
				patterncnt++;
			}
		} else {		//	Looking for file edit times
 			while(get_float_from_within_string(&q,dp)) {
				if(*dp < ap->min_special || *dp > ap->max_special) {
					sprintf(errstr,"Cut position (%lf) out of range (%lf to %lf): file %s : line %d\n",
						*dp,ap->min_special,ap->max_special,filename,linecnt);
					return(DATA_ERROR);
				}
				troffcnt++;
			}
		}
		linecnt++;
	}
	if(troffcnt == 0 || patterncnt == 0 || target_cnt == 0) {
		if(fclose(dz->fp)<0) {
			fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
			fflush(stdout);
		}
		if(troffcnt == 0 && patterncnt == 0 && target_cnt == 0) {
			sprintf(errstr,"No data found in file %s\n",filename);
			return(DATA_ERROR);
		} else if(troffcnt == 0) {
			sprintf(errstr,"No file edit times found in file %s\n",filename);
			return(DATA_ERROR);
		} else {
			if(gothash == 0) {
				sprintf(errstr,"No separator \"#\" found in file %s\n",filename);
				return(DATA_ERROR);
			} else if(gothash == 1) {
				sprintf(errstr,"No 2nd separator \"#\" found in file %s\n",filename);
				return(DATA_ERROR);
			} else if(target_cnt == 0) {
				sprintf(errstr,"No target data found in file %s\n",filename);
				return(DATA_ERROR);
			} else {
				sprintf(errstr,"No pattern data found in file %s\n",filename);
				return(DATA_ERROR);
			}
		}
	}
	if((dz->lparray=(int **)malloc(2 * sizeof(int *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store cut times (1).\n");
		return(MEMORY_ERROR);
	}
	if((dz->lparray[0] = (int *)malloc((troffcnt + 1) * sizeof(int))) == NULL) {		//	Storage for syllable edit-point data
		sprintf(errstr,"INSUFFICIENT MEMORY to store source cut times (2).\n");
		return(MEMORY_ERROR);
	}
	if((dz->iparray[PATN_ARRAY] = (int *)malloc((troffcnt+1) * sizeof(int))) == NULL) {	//	Storage for pattern of syllables
		sprintf(errstr,"INSUFFICIENT MEMORY to store target syllables.\n");
		return(MEMORY_ERROR);
	}
	if((dz->iparray[TRGT_ARRAY] = (int *)malloc((troffcnt+1) * sizeof(int))) == NULL) {	//	Storage for target syllables
		sprintf(errstr,"INSUFFICIENT MEMORY to store target syllables.\n");
		return(MEMORY_ERROR);
	}
	if(fseek(dz->fp,0,0)< 0) {
		sprintf(errstr,"Failed to return to start of file %s.\n",filename);
		return SYSTEM_ERROR;
	}
	dp = &ddummy;
	ip = &idummy;
	gothash = 0;
	linecnt = 0;
	trof = dz->lparray[0];
	pattern = dz->iparray[PATN_ARRAY];
	target  = dz->iparray[TRGT_ARRAY];
	troffcnt = 0;
	patterncnt = 0;
	target_cnt = 0;
	while(fgets(temp,200,dz->fp)!=NULL) {
		q = temp;
		if(temp[0] == '#') {
			gothash++;
			continue;
		}
		if(is_an_empty_line_or_a_comment(temp))
			continue;
		if(gothash == 2) {			//	Looking for target list
			while(get_int_from_within_string(&q,ip,1)) {
				if(*ip < 0)
					*ip = troffcnt+1;
				target[target_cnt++] = *ip;
			}
		} else if(gothash == 1) {		//	Looking for pattern numbers
			while(get_int_from_within_string(&q,ip,0))
				pattern[patterncnt++] = *ip;
		} else {		//	Looking for file edit times
 			while(get_float_from_within_string(&q,dp))
				trof[troffcnt++] = (int)(*dp * (double)srate) * chans;
		}
	}
	if(fclose(dz->fp)<0) {
		fprintf(stdout,"WARNING: Failed to close datafile %s.\n",filename);
		fflush(stdout);
	}
	if(trof[troffcnt - 1] >= dz->insams[0]-chans)	//	Ignore any splice at End-of-file
		troffcnt--;									//	by moving it to the troffcnt spot
	else
		trof[troffcnt] = dz->insams[0];
	dz->trofcnt = troffcnt;
	for(n = 0; n < target_cnt-1;n++) {				//	Sort targets to ascending order, and erase duplicates
		for(m = n+1; m < target_cnt; m++) {
			if(target[n] == target[m]) {
				fprintf(stdout,"syllable %d targeted more than once.\n",target[n]);
				fflush(stdout);
				if(m < target_cnt - 1) {
					for(k = m+1;k < target_cnt;k++)
						target[k-1] = target[k];	//	Erase by shufldown
				}
				target_cnt--;
				m--;
			} else if(target[m] < target[n]) {		//	Swap out-of-order targets
				k = target[m];
				target[m] = target[n];
				target[n] = k;
			}
		}
	}
	for(n = 0; n < target_cnt;n++)					//	Make targets count from zero
		target[n]--;
	dz->targetcnt = target_cnt;
	if(patterncnt < dz->trofcnt + 1) {				//	Expand pattern to max possible size
		for(n = 0, k = patterncnt; k < dz->trofcnt+1;n++,k++)
			pattern[k] = pattern[n % patterncnt];
	}
	(*cmdline)++;
	(*cmdlinecnt)--;
	return(FINISHED);
}
