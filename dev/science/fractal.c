// see HEREH
//	TEST cmdline =
//	_cdprogs\fractal wave 1 sin_cycle.wav testfrac.wav fracshape.brk 4 -m2
//	Only getting 1/2-way through the pattern ... why??
//	look at "seedata.txt" to check times advance!!!!
//	_cdprogs\fractal spectrum zzz.ana testfrac.ana fracshape5.brk

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


#ifdef unix
#define round(x) lround((x))
#endif

#define FRAC_SPLICELEN	50			//	Ms	
#define FRAC_FORMANTS	4			//	No of formants used

#define	FRAC_SHRINK		0
#define	FRAC_OUTIMES	1

#define	 mintime		timemark			//	Minimum possible duration of any fractal element	
#define	 startfrq		frametime			//	Frq of first pitch in (Mode 1) fractal_pitch-pattern
#define	 maxtrnsp		is_flat				//	Maximum possible upward-transpostion of any fractal element
#define	 arraysize		rampbrksize			//	Array size for totally fractalised pattern
#define	 fracdur		minbrk				//	Input duration of the fractal template
#define  minstep		scalefact			//	Minimum element-duration in fractal template
#define  minfracratio	halfchwidth			//	Time shrinkage of smallest element in fractal template, when proceeding from one fractal level to next
#define	 maxfractalise	ringsize			//	Maximum degree of fractalisation
#define	 timevary		is_transpos			//	Inicates that fractal patterning varies through time
#define  fracsize		fzeroset			//	Number of elements in current totally-fractalised pattern
#define  convertor		chwidth
#define  total_splice	specenv_type		//	Total number of samples used in (any) end-splice
#define  splicesmps		deal_with_chan_data	//	No of grouped-samples in (any) end-splice
#define  maxtransshrink	is_sharp			//	Time-shrinkage of the element with maximum upward transposition, when proceeding from one fractal level to next
#define  mintransshrink	minnum				//	Time-shrinkage of the element with maximum downward transposition, when proceeding from one fractal level to next

#define	FRACDUR		0
#define	FRACMAX		1
#define	FRACTSTR	2
#define	FRACISTR	3

char errstr[2400];

int anal_infiles = 1;
int	sloom = 0;
int sloombatch = 0;

const char* cdp_version = "6.1.0";

//CDP LIB REPLACEMENTS
static int check_fractal_param_validity_and_consistency(dataptr dz);
static int setup_fractal_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_fractal_param_ranges_and_defaults(dataptr dz);
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
static int handle_the_special_data(char *str,dataptr dz);
static int create_fractal_sndbufs(dataptr dz);
static void get_max_fractalisation(dataptr dz);
static int fractalise(dataptr dz);
static int fractal_initialise_specenv(int *arraycnt,dataptr dz);
static int fractal_set_specenv_frqs(int arraycnt,dataptr dz);
static int fractal_setup_octaveband_steps(double **interval,dataptr dz);
static int fractal_setup_low_octave_bands(int arraycnt,dataptr dz);
static int fractal_extract_specenv(dataptr dz);
static int fractal_tranpose_within_formant_envelope(int vc,double transpose,dataptr dz);
static int fractal_reposition_partials_in_appropriate_channels(double transpose,dataptr dz);
static int fractal_spectrnsf(double transpose,dataptr dz);
static int fractal_getspecenvamp(double *thisamp,double thisfrq,dataptr dz);
static int fractal_get_channel_corresponding_to_frq(int *chan,double thisfrq,dataptr dz);

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
	int exit_status;
	dataptr dz = NULL;
	char **cmdline;
	int  cmdlinecnt;
	int arraycnt = 0;
	char specialname[1200];
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
		if(dz->process == FRACTAL) {
			dz->maxmode = 2;
			if((exit_status = get_the_mode_from_cmdline(cmdline[0],dz))<0) {
				print_messages_and_close_sndfiles(exit_status,is_launched,dz);
				return(exit_status);
			}
			cmdline++;
			cmdlinecnt--;
		} else
			dz->maxmode = 0;
		// setup_particular_application =
		if((exit_status = setup_fractal_application(dz))<0) {
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
	if((exit_status = setup_fractal_param_ranges_and_defaults(dz))<0) {
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
	if(dz->process == FRACSPEC) {
		dz->nyquist     = dz->infile->origrate/2.0;
		dz->frametime   = (float)(1.0/dz->infile->arate);
	    dz->clength		= dz->wanted / 2;
		dz->chwidth 	= dz->nyquist/(double)(dz->clength-1);
		dz->halfchwidth = dz->chwidth/2.0;
		if((exit_status = fractal_initialise_specenv(&arraycnt,dz))<0) {	
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);	
			return(FAILED);
		}
		dz->formant_bands = FRAC_FORMANTS;
		if((exit_status = fractal_set_specenv_frqs(arraycnt,dz))<0) {	
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);	
			return(FAILED);
		}
	}
//	handle_extra_infiles() : redundant
	// handle_outfile() = 
	if((exit_status = handle_the_outfile(&cmdlinecnt,&cmdline,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}

//	handle_formants()			redundant
//	handle_formant_quiksearch()	redundant
//	handle_special_data()		redundant
 
	strcpy(specialname,cmdline[0]);
	cmdlinecnt--;
	cmdline++;
	if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {		// CDP LIB
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = handle_the_special_data(specialname,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
//	check_param_validity_and_consistency ..
	if((exit_status = check_fractal_param_validity_and_consistency(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	is_launched = TRUE;
	if(dz->process == FRACTAL)
		dz->bufcnt = 4;
	else
		dz->bufcnt = 2;
	if((exit_status = create_fractal_sndbufs(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	//param_preprocess()						redundant
	//spec_process_file =
	if((exit_status = fractalise(dz))<0) {
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

/************************* SETUP_FRACTAL_APPLICATION *******************/

int setup_fractal_application(dataptr dz)
{
	int exit_status;
	aplptr ap;
	if((exit_status = establish_application(dz))<0)		// GLOBAL
		return(FAILED);
	ap = dz->application;
	// SEE parstruct FOR EXPLANATION of next 2 functions
	if(dz->process == FRACTAL) {
		if (dz->mode == 0)
			exit_status = set_param_data(ap,FRACSHAPE,1,0,"0");
		else
			exit_status = set_param_data(ap,FRACSHAPE,1,1,"d");
	} else
		exit_status = set_param_data(ap,FRACSHAPE,1,0,"0");


	
	
	if(exit_status<0)
		return(FAILED);
	if(dz->process == FRACTAL) {
		if(dz->mode == 0) 
			exit_status = set_vflgs(ap,"mti",3,"IDD","so",2,0,"00");
		else
			exit_status = set_vflgs(ap,"mti",3,"IDD","s",1,0,"0");
	} else
		exit_status = set_vflgs(ap,"mti",3,"IDD","sn",2,0,"00");
	if(exit_status<0)
		return(FAILED);
	// set_legal_infile_structure -->
	dz->has_otherfile = FALSE;
	// assign_process_logic -->
	if(dz->process == FRACSPEC) {
		dz->input_data_type = ANALFILE_ONLY;
		dz->process_type	= BIG_ANALFILE;
		dz->outfiletype  	= ANALFILE_OUT;
		dz->maxmode = 0;
	} else {
		dz->input_data_type = SNDFILES_ONLY;
		dz->process_type	= UNEQUAL_SNDFILE;	
		dz->outfiletype  	= SNDFILE_OUT;
		dz->maxmode = 2;
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
		}
		if((exit_status = cdparse(cmdline[0],infile_info))<0) {
			sprintf(errstr,"Failed to parse input file %s\n",cmdline[0]);
			return(PROGRAM_ERROR);
		}
		if(dz->process == FRACTAL) {
			if(infile_info->filetype != SNDFILE)  {
				sprintf(errstr,"File %s is not of correct type\n",cmdline[0]);
				return(DATA_ERROR);
			} 
			if(dz->mode == 1 && infile_info->channels != 1)  {
				sprintf(errstr,"File %s is not of correct type (must be mono for mode %d)\n",cmdline[0],dz->mode+1);
				return(DATA_ERROR);
			}
		} else if(infile_info->filetype != ANALFILE)  {
			sprintf(errstr,"File %s is not of correct type\n",cmdline[0]);
			return(DATA_ERROR);
		}
		if((exit_status = copy_parse_info_to_main_structure(infile_info,dz))<0) {
			sprintf(errstr,"Failed to copy file parsing information\n");
			return(PROGRAM_ERROR);
		}
		free(infile_info);
	}
	return(FINISHED);
}

/************************* SETUP_FRACTAL_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_fractal_param_ranges_and_defaults(dataptr dz)
{
	int exit_status;
	aplptr ap = dz->application;
	// set_param_ranges()
	ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
	// NB total_input_param_cnt is > 0 !!!
	if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
		return(FAILED);
	// get_param_ranges()
	if(dz->process == FRACTAL && dz->mode == 1) {
		ap->lo[FRACDUR]	= 1;
		ap->hi[FRACDUR]	= FRAC_MAXDUR;
		ap->default_val[FRACDUR] = 10;
	}
	ap->lo[FRACMAX]	= 0;
	ap->hi[FRACMAX]	= FRAC_MAXFRACT;
	ap->default_val[FRACMAX] = 0;
	ap->lo[FRACTSTR]	= 1;
	ap->hi[FRACTSTR]	= 2;
	ap->default_val[FRACTSTR] = 1;
	ap->lo[FRACISTR]	= 0;
	ap->hi[FRACISTR]	= 8;
	ap->default_val[FRACISTR] = 0;
	if(dz->process == FRACTAL)	
		dz->maxmode = 2;
	else
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
			if((exit_status = setup_fractal_application(dz))<0)
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
	fprintf(stderr,
	"USAGE: fractal NAME (mode) infile outfile parameters\n"
	"\n"
	"where NAME can be any one of\n"
	"\n"
	"wave  spectrum\n"
	"\n"
	"Type 'fractal wave'  for more info on fractal wave option... ETC.\n");
	return(USAGE_ONLY);
}


/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	if(!strcmp(prog_identifier_from_cmdline,"wave"))				
		dz->process = FRACTAL;
	else if(!strcmp(prog_identifier_from_cmdline,"spectrum"))
		dz->process = FRACSPEC;
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
	if(!strcmp(str,"wave")) {
		fprintf(stderr,
	    "USAGE:\n"
	    "fractal wave 1 inf outf shape     [-mmaxfrac] [-tstr] [-iwarp] [-s] [-o]\n"
	    "fractal wave 2 inf outf shape dur [-mmaxfrac] [-tstr] [-iwarp] [-s]\n"
		"\n"
		"Fractally distort (1 or more channel) input sound (Mode 1).\n"
		"OR generate fractal wave from (mono) input wavecycle (Mode 2)\n"
		"Fractal is time-pattern of transpositions (and, in Mode 1\n"
		"causes corresponding time contractions and expansions).\n"
		"Transposition happens over total duration specified in \"SHAPE\"\n"
		"and is then repeated over every resulting sub-unit of pattern,\n"
		"over every sub-sub-unit, etc. until smallest time-unit reached.\n"
		"\n"
		"SHAPE   Breakpoint textfile of\n"
		"        (Mode 1) time-(semitone)transposition pairs, (Range -12 to 12)\n"
		"        (Mode 2) time-MIDIpitch pairs, (Range 0 to 127)\n"
		"        defining pitch-time contour of the (largest) fractal shape.\n"
		"        Times must start at zero and increase,\n"
		"        with final time indicating duration of pattern.\n"
		"        (Value at final time will be ignored).\n"
		"\n"
		"DUR     (Mode 2) Output duration (secs) (max 2 hours).\n"
		"MAXFRAC Max degree of fractalisation.  (Time-variable). If not set (or zero),\n"
		"        fractalisation proceeds until min possible wavelength reached.\n"
		"STR     Time stretch of fractal pattern (Time-variable : values >= 1).\n"
		"        If set to zero, no timestretching is done.\n"
		"WARP    Interval warping of fractal pattern (Time-variable).\n"
		"        If set to zero, no warping is done.\n"
		"\n"
		"-s      Shrink pitch-intervals as fractal time-scales shrink.\n"
		"-o      Brkpnt data read using time in outfile (default: Use time in infile).\n"
		"\n");
	} else if(!strcmp(str,"spectrum")) {
		fprintf(stderr,
	    "USAGE:\n"
	    "fractal spectrum inf outf shape [-mmaxfrac] [-tstr] [-iwarp] [-s] [-n]\n"
		"\n"
		"Fractally distort input spectrum by transposition.\n"
		"Transposition happens over total duration specified in \"SHAPE\"\n"
		"and is then repeated over every resulting sub-unit of pattern,\n"
		"over every sub-sub-unit, etc. until smallest time-unit reached.\n"
		"\n"
		"SHAPE   Breakpoint textfile of\n"
		"        Time-(semitone)transposition pairs, (Range -12 to 12)\n"
		"        defining pitch-time contour of the (largest) fractal shape.\n"
		"        Times must start at zero and increase,\n"
		"        with final time indicating duration of pattern.\n"
		"        (Value at final time will be ignored).\n"
		"\n"
		"MAXFRAC Max degree of fractalisation.  (Time-variable). If not set (or zero),\n"
		"        fractalisation proceeds until min possible wavelength reached.\n"
		"STR     Time stretch of fractal pattern (Time-variable : values >= 1).\n"
		"        If set to zero, no timestretching is done.\n"
		"WARP    Interval warping of fractal pattern (Time-variable).\n"
		"        If set to zero, no warping is done.\n"
		"\n"
		"-s      Shrink pitch-intervals as fractal time-scales shrink.\n"
		"-n      Transposition only (default, retain formant envelope).\n"
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
	int exit_status, n, m, cnt;
	FILE *fp;
	char temp[200], *p;
	double dummy = 0, lasttime = 0.0, lastmintrans = 0, lastmaxtrans = 0, mintrans = 0, maxtrans = 0, fracmax = 0.0;
	double firstmidi = 0, lastmaxpitch = 0, lastminpitch = 0, maxpitch = 0, minpitch = 0, max_step, min_step, srate = (double)dz->infile->srate;
	double *fractemplate, *fracintvl, *fractimes;
	double mindur_maxtrans, mindur_mintrans, maxelement, minelement, thisdur, min_step_now, max_step_now;

	if(dz->process == FRACTAL)
		dz->mintime = (float)(4.0/srate);					//	Minimum duration of fractal pattern (> 4 samples)
	else {
		dz->frametime  = (float)(1.0/dz->infile->arate);
		dz->mintime = dz->frametime;						//	Minimum duration of fractal pattern (> 1 analysis window)
	}
	dz->maxtrnsp =  256.0;									//	8 octaves
	dz->maxfractalise = 0;
	dz->minstep = HUGE;
	if((fp = fopen(str,"r"))==NULL) {
		sprintf(errstr,"Cannot open file %s to read clip lengths.\n",str);
		return(DATA_ERROR);
	}
	cnt = 0;
	lasttime = 0;
	while(fgets(temp,200,fp)!=NULL) {
		p = temp;
		while(isspace(*p))
			p++;
		if(*p == ';' || *p == ENDOFSTR)	//	Allow comments in file
			continue;
		while(get_float_from_within_string(&p,&dummy)) {

			if (EVEN(cnt)) {								//	TIMES MUST START AT ZERO AND INCREASE
				if(cnt == 0) {
					if(dummy != 0.0) {
						sprintf(errstr,"Invalid initial time (%lf) in file %s. (Should be zero).\n",dummy,str);
						return DATA_ERROR;
					}
				} else {
					if(dummy <= lasttime) {
						sprintf(errstr,"Times do not increase at (%lf %lf) in file %s.\n",lasttime,dummy,str);
						return DATA_ERROR;
					}
					dz->minstep = min(dz->minstep,dummy - lasttime);//	Find smallest time-step in fractal-pattern
				}
				lasttime = dummy;
			} else {										//	VALUES MUSY LIE WITHIN RANGE
				if(dz->process == FRACTAL && dz->mode == 1) {		//	as pitch-pattern
					if(dummy < FRAC_MINMIDI || dummy > FRAC_MAXMIDI) {
						sprintf(errstr,"MIDIpitch (%lf) out of range (%lf to %lf) in file %s.\n",dummy,FRAC_MINMIDI,FRAC_MAXMIDI,str);
						return DATA_ERROR;
					}	
					if(cnt == 1) {		
						firstmidi = dummy;					//	Note initial MIDI pitch
						lastmaxpitch = dummy;				//	And search for max and min pitches
						lastminpitch = dummy;				//	NB maximum and minimum search IGNORES last value in table
					} else {
						maxpitch = lastmaxpitch;
						minpitch = lastminpitch;			
						lastmaxpitch = max(lastmaxpitch,dummy);
						lastminpitch = min(lastminpitch,dummy);
					}
				} else {									//	as semitone-transposition-pattern	
					if(dummy < FRAC_MINTRNS || dummy > FRAC_MAXTRNS) {
						sprintf(errstr,"Transposition (%lf semitones) out of range (%lf to %lf) in file %s.\n",dummy,FRAC_MINTRNS,FRAC_MAXTRNS,str);
						return DATA_ERROR;
					}
					if(cnt == 1) {
						lastmintrans = dummy;				//	Search for max and min transpositions
						lastmaxtrans = dummy;				//	NB maximum and minimum search IGNORES last value in table
					} else {
						mintrans = lastmintrans;
						maxtrans = lastmaxtrans;
						lastmintrans = min(lastmintrans,dummy);
						lastmaxtrans = max(lastmaxtrans,dummy);
					}
				}
			}
			cnt++;
		}
	}
	if(cnt == 0) {											//	Check data count
		sprintf(errstr,"No data found in file %s.\n",str);
		return DATA_ERROR;
	}
	if(ODD(cnt)) {
		sprintf(errstr,"Values not correctly paired in file %s.\n",str);
		return DATA_ERROR;
	}
	if(cnt < 4) {
		sprintf(errstr,"Insufficient values for a fractal shape (needs 2 times and 2 vals = 4 total) in file %s.\n",str);
		return DATA_ERROR;
	}
	dz->fracdur = lasttime;									//	Last time in data indicates total duration of fractal-shape
	if(dz->process == FRACTAL && dz->mode == 1)
		dz->startfrq = (float)miditohz(firstmidi);			//	In mode 1 (fractal-template of pitches), find initial frq of pattern

	dz->itemcnt = cnt/2;

	if((dz->parray = (double **)malloc(6 * sizeof(double *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create fractalisation calculation arrays.\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[3] = (double *)malloc((dz->itemcnt * 2) * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store fractalisation data.\n");		//	fractemplate : original (input) fractal template data		
		return(MEMORY_ERROR);												  
	}
	fractemplate = dz->parray[3];	//	Original fractal template time-val pairs

	cnt = 0;
	fseek(fp,0,0);
	while(fgets(temp,200,fp)!=NULL) {
		p = temp;
		while(isspace(*p))
			p++;
		if(*p == ';' || *p == ENDOFSTR)	//	Allow comments in file
			continue;
		while(get_float_from_within_string(&p,&dummy))
			fractemplate[cnt++] = dummy;
	}
	if(dz->process == FRACTAL && dz->mode == 1) {
		for(n=1;n<dz->itemcnt*2;n+=2)		//	Convert pitch input to transpositions of initial pitch
			fractemplate[n] -= firstmidi;
	}

	//	CHECK TIME LIMITS

	if(dz->process == FRACTAL && dz->mode == 1) {
		if(dz->fracdur > FRAC_MAXDUR) {												
			fprintf(stdout,"WARNING: Pattern total duration (%lf) greater than %.2lf hours, in file %s.\n",dz->fracdur,FRAC_MAXDUR/3600.0,str);
			fflush(stdout);
		}
	}
	if(dz->brksize[FRACTSTR]) {								//	If fractal-shape time-stretched anywhere, get maximal tstretch,
		if((exit_status = get_maxvalue_in_brktable(&fracmax,FRACTSTR,dz))<0)
			return exit_status;
		dz->param[FRACTSTR] = (int)round(fracmax);
	}

	//	CHECK MAXIMUM FRACTALISATION

	dz->minfracratio = dz->minstep/dz->fracdur;
	get_max_fractalisation(dz);

	if(dz->maxfractalise < 1) {
		sprintf(errstr,"Smallest time-unit in pattern (%lf) too short to allow fractalisation.\n",dz->minstep);
		return DATA_ERROR;
	}
	if(dz->brksize[FRACMAX]) {								//	If maximum fractalisation is preset by user
		if((exit_status = get_maxvalue_in_brktable(&fracmax,FRACMAX,dz))<0)
			return exit_status;
		dz->iparam[FRACMAX] = (int)round(fracmax);
	}														//	compare it with the max possible fractalisation
	if(dz->iparam[FRACMAX] >  dz->maxfractalise) {
		fprintf(stdout,"WARNING: Max fractalisation will be restricted to %d because of time proportions in given shape data.\n",dz->maxfractalise);
		fflush(stdout);
	} else if (dz->iparam[FRACMAX] > 0 && dz->iparam[FRACMAX] < dz->maxfractalise)
		dz->maxfractalise = dz->iparam[FRACMAX];

	if(dz->brksize[FRACISTR]) {
		if((exit_status = get_maxvalue_in_brktable(&fracmax,FRACISTR,dz))<0)
			return exit_status;
		dz->param[FRACISTR] = fracmax;
	}

	//	CHECK RESTRICTIONS ON FRACTALISATION TRANSPOSITION-DEPTH CAUSED BY CHAINED-TRANSPOSITIONS
	
	//	FinD time-shrinkage at maximum amd minimum pitch (transposition) elements of fractal shape

	if(dz->vflag[FRAC_SHRINK]) {
		mindur_maxtrans = HUGE;
		mindur_mintrans = HUGE;
		if(dz->process == FRACTAL && dz->mode == 1) {
			maxelement = maxpitch - firstmidi;
			minelement = minpitch - firstmidi;
		} else {
			maxelement = maxtrans;
			minelement = mintrans;
		}
		for(n = 1; n < (dz->itemcnt -1) * 2; n+= 2) {
			if(fractemplate[n] == maxelement) {
				thisdur = fractemplate[n+1] - fractemplate[n-1];
				mindur_maxtrans = min(mindur_maxtrans,thisdur);
			}
			if(fractemplate[n] == minelement) {
				thisdur = fractemplate[n+1] - fractemplate[n-1];
				mindur_mintrans = min(mindur_mintrans,thisdur);
			}
		}
		if(mindur_maxtrans == HUGE || mindur_mintrans == HUGE) {
			sprintf(errstr,"Problem determining duration of max:min elements in fractal.\n");
			return PROGRAM_ERROR;
		}
		dz->maxtransshrink = mindur_maxtrans/dz->fracdur;
		dz->mintransshrink = mindur_mintrans/dz->fracdur;
	}

	//	Check what happens to maximum and minimum transpositions, after maximium fractalisation.

	if(dz->process == FRACTAL && dz->mode == 1)
		max_step = maxpitch - firstmidi;				//	Base-pitch assumed to be first pitch of pattern
	else
		max_step = maxtrans;
	if(dz->param[FRACISTR] > 1.0)
		max_step *= dz->param[FRACISTR];
	if(max_step > 0.0) {								//	If highest pitch in fractal-shape is above initial pitch
		if(dz->vflag[FRAC_SHRINK]) {
			max_step_now = max_step;
			max_step = 0;
			for(n = 0;n < dz->maxfractalise;n++) {
				max_step += max_step_now;
				max_step_now *= dz->maxtransshrink;
			}
		} else
			max_step *= dz->maxfractalise;				//	calculate how mich this will step upwards in successive fractalisations
		if(dz->process == FRACTAL && dz->mode == 1) {
			maxpitch += max_step ;							
			if(maxpitch > MIDIMAX) {
				sprintf(errstr,"Max pitch after max fractalisation (%d times) (MIDI %.2lf) exceeds limit (127).\n",dz->maxfractalise,maxpitch);
				return DATA_ERROR;
			}
		} else {										//	Convert to frq ratio
			maxtrans = pow(2.0,(max_step/SEMITONES_PER_OCTAVE));
			if(maxtrans > dz->maxtrnsp) {
				sprintf(errstr,"Max transpos after max fractalisation (%d times) (%lf octaves) exceeds limit (8).\n",dz->maxfractalise,LOG2(maxtrans));
				return DATA_ERROR;
			}
		}
	}
	if(dz->process == FRACTAL && dz->mode == 1)
		min_step = minpitch - firstmidi;
	else
		min_step = mintrans;
	if(dz->param[FRACISTR] > 1.0)
		min_step *= dz->param[FRACISTR];
	if(min_step < 0.0) {								//	If lowest pitch in fractal-shape is below initial pitch
		if(dz->vflag[FRAC_SHRINK]) {
			min_step_now = min_step;
			min_step = 0;
			for(n = 0;n < dz->maxfractalise;n++) {
				min_step += min_step_now;
				min_step_now *= dz->mintransshrink;
			}
		} else
			min_step *= dz->maxfractalise;				//	calculate how mich this will step downwardsupwards in successive fractalisations
		if(dz->process == FRACTAL && dz->mode == 1) {
			minpitch += min_step;
			if(minpitch < 0) {
				sprintf(errstr,"Min pitch after max fractalisation (%d times) (MIDI %lf) falls below low limit (0).\n",dz->maxfractalise,minpitch);
				return DATA_ERROR;
			}
		} else {
			mintrans = pow(2.0,(mintrans/SEMITONES_PER_OCTAVE));
			mintrans = pow(mintrans,dz->maxfractalise);
			if(mintrans < 1.0/dz->maxtrnsp) {
				sprintf(errstr,"Max transpos down after max fractalisation (%d times) (%lf octaves) exceeds limit (-8).\n",dz->maxfractalise,LOG2(mintrans));
				return DATA_ERROR;
			}
		}
	}

	//	CREATE WORKING ARRAYS, now we know maximum depth of fractalisation (dz->maxfractalise)

	dz->arraysize = dz->itemcnt - 1;										//	Number of fractal elements
	dz->arraysize = (int)round(pow(dz->arraysize,dz->maxfractalise));		//	Number of entries needed for (largest) fractalisation
	dz->arraysize *= 2;														//	.. then include space for timing entries ....
	dz->arraysize += 64;													//	SAFETY

	if((dz->parray[0] = (double *)malloc(dz->arraysize * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create fractalisation array.\n");
		return(MEMORY_ERROR);												//	fracpattern : Stores final conmplete fractalisation of data
	}
	if((dz->parray[1] = (double *)malloc(dz->arraysize * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create swap buffer 1.\n");
		return(MEMORY_ERROR);												//	swapbuffer1 : 1st itermediate buffers to calc fractalisation, stage by stage
	}
	if((dz->parray[2] = (double *)malloc(dz->arraysize * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create swap buffer 2.\n");
		return(MEMORY_ERROR);												//	swapbuffer2 : 2nd itermediate buffers to calc fractalisation, stage by stage
	}
	if((dz->parray[4] = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store transposing data.\n");	//	fracintvl : Mode 0 : Intvls in fractal template
		return(MEMORY_ERROR);												//				Mode 1 : Intvls btwn pitches in orig fractal pitch-template
	}
	if((dz->parray[5] = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store element duations.\n");
		return(MEMORY_ERROR);												//	fractimes : Onset times of each element in fractal template
	}

	fracintvl    = dz->parray[4];	//	Intervals between pitches in original fractal pitch-template
	fractimes    = dz->parray[5];	//	Fractal template element-start-times

	for(n=0, m=0; n < (dz->itemcnt-1) * 2; n +=2, m++) {		//	Stores dz->itemcnt-1 items
		fractimes[m] = fractemplate[n];
		fracintvl[m] = fractemplate[n+1];
	}
	dz->itemcnt--;												//	Last element of fractal template was needed only to specify total pattern's duration. So now ignore
																//	dz->itemcnt is number of elements in fractal-template

	return FINISHED;
}

/************************************* CREATE_FRACTAL_PATTERN ***************************/

int create_fractal_pattern(dataptr dz)
{
	int fractalcnt = dz->itemcnt, total_fractalcnt, k, n, nn, t, v, ocnt = 0;
	double *fracpattern = dz->parray[0];	//	Stores final conmplete fractalisation of data
	double *swapbuffer1 = dz->parray[1];	//	Intermediate buffers to calc fractalisation, stage by stage
	double *swapbuffer2 = dz->parray[2];
	double *thisbuf = NULL, *thatbuf = NULL; 
	double *fractemplate = dz->parray[3];	//	Original fractal template time-val pairs
	double *fracintvl    = dz->parray[4];	//	Intervals between pitches in original fractal pitch-template
	double *fractimes    = dz->parray[5];	//	Fractal template element-start-times
	double starttime, startval, duration, shrinkage;

	memset((char *)fracpattern,0,dz->arraysize * sizeof(double));	
	memset((char *)swapbuffer1,0,dz->arraysize * sizeof(double));	
	memset((char *)swapbuffer2,0,dz->arraysize * sizeof(double));
	memcpy((char *)swapbuffer1,(char *)fractemplate,(dz->itemcnt+1) * 2 * sizeof(double));	//	Get original fractal-shape, with wraparound time
																							//	mode 0 = semitone transpos : mode1 = MIDIpitch 
	total_fractalcnt = fractalcnt;															//	If fractalise only once, this is enough
	if(dz->maxfractalise == 1)
		thatbuf = swapbuffer1;
	else {
		for(k = 1; k < dz->maxfractalise;k++) {
			ocnt = 0;
			if(ODD(k))	{
				thisbuf = swapbuffer1;
				thatbuf = swapbuffer2;
			} else {		
				thisbuf = swapbuffer2;
				thatbuf = swapbuffer1;
			}
			memset((char *)thatbuf,0,dz->arraysize * sizeof(double));	
			for(n = 0, t = 0, v = 1; n < total_fractalcnt; n++,t+=2,v+=2) {		//	Fractalise every element of the "total_fractalcnt" elements existing at this stage.
				starttime  = thisbuf[t];
				startval   = thisbuf[v];
				duration   = thisbuf[t+2] - thisbuf[t];							//	There is a wrap-around time-point in orig fractal-template
				shrinkage  = duration/dz->fracdur;
				for(nn = 0; nn < fractalcnt; nn++) {
					if(ocnt + 2 >= dz->arraysize) {
						sprintf(errstr,"Array overrun in generating fractal.\n");
						return PROGRAM_ERROR;
					}
					thatbuf[ocnt++] = starttime + (fractimes[nn] * shrinkage);
					if(dz->vflag[FRAC_SHRINK])
						thatbuf[ocnt++] = startval + (fracintvl[nn] * shrinkage);
					else
						thatbuf[ocnt++] = startval + fracintvl[nn];
				}
			}
			total_fractalcnt = ocnt/2;											//	No of fractal elements after this fractalisation now increaaed to new "total_fractalcnt"
			if(ocnt >= dz->arraysize - 1) {
				sprintf(errstr,"Array overrun in generating last timepoint of fractal.\n");
				return PROGRAM_ERROR;
			}
			thatbuf[ocnt++] = fractemplate[dz->itemcnt*2];						//	Retain wraparound time-point
		}
	}
	for(n = 0, t = 0, v = 1; n < total_fractalcnt; n++,t+=2,v+=2) {
		fracpattern[t] = thatbuf[t];
		if((dz->brksize[FRACISTR] > 0) || (dz->param[FRACISTR] > 0.0))			//	If fractal is interval warped, do it here
			thatbuf[v] *= dz->param[FRACISTR];
		fracpattern[v] = pow(2.0,(thatbuf[v]/SEMITONES_PER_OCTAVE));			//	Convert to 8va-transpos
	}
	fracpattern[t] = thatbuf[t];												//	Store wraparound time-point		

	if(dz->param[FRACTSTR] > 0.0) {
		for(t=0;t <= total_fractalcnt * 2; t+=2)								//	If fractal is time-warped, do the timewarp
			fracpattern[t] *= dz->param[FRACTSTR];
	}
	dz->fracsize = total_fractalcnt;											//	Remember length of fractal pattern
	return FINISHED;
}

/************************************* CHECK_FRACTAL_PARAM_VALIDITY_AND_CONSISTENCY ***************************/

int check_fractal_param_validity_and_consistency(dataptr dz)
{
	int exit_status;
	double minwarp = 0.0;
	dz->timevary = 0;
	if(dz->brksize[FRACMAX] || dz->brksize[FRACTSTR] || dz->brksize[FRACISTR])
		dz->timevary = 1;
	if(dz->brksize[FRACTSTR]) {
		if((exit_status = get_minvalue_in_brktable(&minwarp,FRACTSTR,dz))<0)
			return exit_status;
		if(minwarp < 1.0) {
			sprintf(errstr,"Values in timewarp table must be >= 1.0\n");
			return DATA_ERROR;
		}
	} else if(dz->param[FRACTSTR] > 0.0 && dz->param[FRACTSTR] < 1.0) {
		sprintf(errstr,"Non-zero timewarp value must be >= 1.\n");
		return DATA_ERROR;
	}
	if(dz->process == FRACTAL) {
		dz->splicesmps = (int)ceil(FRAC_SPLICELEN * MS_TO_SECS * (double)dz->infile->srate);
		dz->total_splice = dz->splicesmps * dz->infile->channels;
		if(dz->mode == 1) {
			if(dz->insams[0] < dz->total_splice) {
				fprintf(stdout,"WARNING: Infile too short to enable endsplice to be made.\n");
				fflush(stdout);
				dz->total_splice = 0;	//	Don't attempt endsplice
			}
		} else
			dz->total_splice = 0;		//	No endsplice needed in mode 0
	} else {
		dz->splicesmps = 0;				//	Probably redundant
		dz->total_splice = 0;
	}
	return FINISHED;
}

/************************************* FRACTALISE ***************************/

int fractalise(dataptr dz)
{
	int exit_status, chans = dz->infile->channels, done = 0, ch, nxt, n, m;
	double intime = 0.0, outtime = 0.0, ztime, localtime, srate = (double)dz->infile->srate;
	int ibufpos = 0, gpibufpos = 0, obufpos = 0, fracindex = 0, bufcnt = 0, abs_input_start_samp = 0, nextfracsampcnt, zsamps;
	int   gpbuflen = dz->buflen/chans, abs_window_cnt = 0, next_abs_window_cnt = 0;
	double dibufpos = 0.0, localsampcnt, incr, frac, diff, spliceval, abs_time = 0, nexttime = 0.0, transpose = 1.0;
	double *fracpattern = dz->parray[0];
	float *ibuf, *obuf, *ovflwbuf = NULL;
	double *val;
	double *fractemplate = dz->parray[3];
	if(dz->process == FRACTAL) {
		ibuf = dz->sampbuf[0];
		/* input overflow in = dz->sampbuf[1] */ 
		obuf = dz->sampbuf[2];
		ovflwbuf = dz->sampbuf[3];
	} else {
		ibuf = dz->sampbuf[0];
		obuf = dz->sampbuf[1];
	}
	if((val = (double *)malloc(chans * sizeof(double))) == NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create intermediate value storage.\n");
		return(MEMORY_ERROR);
	}
	if(!dz->timevary) {										//	If fractal does not vary
		if((exit_status = create_fractal_pattern(dz)) < 0)
			return exit_status;								//	Create fixed fractal pattern
	}
	if(dz->process == FRACTAL) {
								//	FRACTAL reads a double buffer, so that we have wraparound points to use
		if(dz->mode == 0) {								//	(If mode 1, source used as "table", and is already in buffer)	
			dz->buflen *= 2;
			if((exit_status = read_samps(ibuf,dz))<0)
				return(exit_status);						
			abs_input_start_samp = 0;
			dz->buflen /= 2;
		}
		for(ch = 0;ch < chans; ch++)						//	Read initial src/table values to output value store
			val[ch] = ibuf[ch];
	} else {												//	FRACSPEC reads a single buffer of analysis samples	
		dz->flbufptr[0] = ibuf;
		dz->flbufptr[1] = obuf;
		if((exit_status = read_samps(ibuf,dz))<0)
			return(exit_status);
		memcpy((char *)obuf,(char *)ibuf,dz->wanted * sizeof(float));
		abs_time = dz->frametime;							//	First window is copied (below)
		abs_window_cnt = 1;
		dz->flbufptr[0] += dz->wanted;						//	flbufptrs advance
		dz->flbufptr[1] += dz->wanted;
	}
	while(!done) {
		if(dz->timevary) {									//	If fractal varies in time
			if(dz->mode == 1 || (dz->process == FRACTAL && dz->vflag[FRAC_OUTIMES])) {	//	Read brkpoint data using either time in infile or time in outfile
				if((exit_status = read_values_from_all_existing_brktables(outtime,dz))<0)
					return exit_status;
			} else {
				if((exit_status = read_values_from_all_existing_brktables(intime,dz))<0)
					return exit_status;
			}
			if(dz->iparam[FRACMAX] > 0)						//	Find maximum fractalisation		
				dz->maxfractalise = dz->iparam[FRACMAX];
			else
				get_max_fractalisation(dz);
			if(dz->maxfractalise > 0)						//	Create the fractal pattern, and remember length of fractal-pattern (dz->fracsize)
				create_fractal_pattern(dz);
		}
		ztime = fractemplate[dz->itemcnt * 2];
		if(dz->maxfractalise == 0) {
			if(dz->process == FRACTAL) {
				zsamps  = (int)round(ztime * srate);
				gpibufpos = (int)round(dibufpos);				//	Approximation of interpolating pointer involved here
				ibufpos = gpibufpos * chans;
				for(n = 0; n <zsamps; n++) {
					if(ibufpos >= dz->buflen) {
						if(dz->mode == 0) {					//	If reading countinuous input, advance by buflen
							memset((char *)ibuf,0,dz->buflen * 2 * sizeof(float));
							bufcnt++;
							if(dz->buflen * bufcnt >= dz->insams[0]) {
								done = 1;							//	Exit if attempted samps_read looks beyound input file end
								break;
							}
							sndseekEx(dz->ifd[0],dz->buflen * bufcnt,0);
							dz->buflen *= 2;						//	 and read 2 * buflen (so we have wraparound)
							abs_input_start_samp += dz->ssampsread/2;
							if((exit_status = read_samps(ibuf,dz))<0)
								return(exit_status);
							dz->buflen /= 2;
							if(dz->ssampsread == 0) {				//	Exit if no more samples to read (should be redundant as trap at snd seek above)
								done = 1;
								break;
							}
						}
						ibufpos -= dz->buflen;
					}
					if(obufpos >= dz->buflen + dz->total_splice) {	//	Allow enough samples on end of output to perform splice. (ALPHA)
						if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
							return(exit_status);					//	Write to outfile, then copy back ovflow into obuf
						memset((char *)obuf,0,dz->buflen * sizeof(float));
						memcpy((char *)obuf,(char *)ovflwbuf,dz->buflen * sizeof(float));
						memset((char *)ovflwbuf,0,dz->buflen * sizeof(float));
						obufpos -= dz->buflen;						//	and reset obufptr
					}
					for(ch = 0;ch<chans;ch++)
						obuf[obufpos++] = ibuf[ibufpos++];
				}
				gpibufpos  = ibufpos/chans;
				dibufpos = (double)gpibufpos;
				intime  += ztime;									//	overflow point in fractal template contains its duration, so advance time in input file
				outtime += ztime;									//	time in output file
			} else {	//	 FRACSPEC
				next_abs_window_cnt = (int)round(ztime/dz->frametime);
				while(abs_window_cnt < next_abs_window_cnt) {
					memcpy((char *)dz->flbufptr[1],(char *)dz->flbufptr[0],dz->wanted * sizeof(float));
					dz->flbufptr[0] += dz->wanted;
					dz->flbufptr[1] += dz->wanted;
					if(dz->flbufptr[0] >= obuf) {
						if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
							return(exit_status);
						memset((char *)ibuf,0,dz->buflen * sizeof(float));
						memset((char *)obuf,0,dz->buflen * sizeof(float));
						if((exit_status = read_samps(ibuf,dz))<0)
							return(exit_status);
						if(dz->ssampsread == 0) {
							done = 1;
							break;
						}
						dz->flbufptr[0] = ibuf;
						dz->flbufptr[1] = obuf;
					}
					abs_window_cnt++;
					abs_time += dz->frametime;
					if(abs_window_cnt >= dz->wlength) {
						done = 1;
						break;
					}
				}
			}
		} else {
			if(dz->process == FRACTAL) {
				localsampcnt = 0;									//	This is a count in gp-samples
				fracindex = 0;
				while(fracindex < dz->fracsize * 2) {				//	For each unit of the fractal (2 entries - time:val - per fractal element)
					nextfracsampcnt = (int)round(fracpattern[fracindex + 2] * srate);
					while(localsampcnt > nextfracsampcnt) {
						fracindex += 2;
						if(fracindex >= dz->fracsize)
							break;
						nextfracsampcnt = (int)round(fracpattern[fracindex + 2] * srate);
					}
					incr = fracpattern[fracindex + 1];				//	Get current input-read increment
					while(localsampcnt < nextfracsampcnt) {			//	While we're still using the current increment
						for(ch = 0;ch < chans; ch++)
							obuf[obufpos++] = (float)val[ch];		//	Copy current value from input to output
						if(obufpos >= dz->buflen + dz->total_splice) {//	Allow enough samples on end of output to perform splice. (ALPHA)
							if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
								return(exit_status);				//	Write to outfile, then copy back ovflow into obuf
							memset((char *)obuf,0,dz->buflen * sizeof(float));
							memcpy((char *)obuf,(char *)ovflwbuf,dz->buflen * sizeof(float));
							memset((char *)ovflwbuf,0,dz->buflen * sizeof(float));
							obufpos -= dz->buflen;					//	and reset obufptr
						}
						if(dz->mode == 1) {
							dibufpos += incr * dz->convertor;		//	Advance in table-read
							gpibufpos = (int)floor(dibufpos);
							ibufpos = gpibufpos;					//	Mono
						}else {
							dibufpos += incr;						//	Advance pointer in infile src
							gpibufpos = (int)floor(dibufpos);
							ibufpos = gpibufpos * chans;			//	In mode 1, if run off end of input file, quit
							if(abs_input_start_samp + ibufpos >= dz->insams[0]) {				
								done = 1;
								break;
							}
						}
						frac = dibufpos - (double)gpibufpos;			
						if(gpibufpos >= gpbuflen) {				
							if(dz->mode == 0) {						//	If reading countinuous input, advance by buflen
								memset((char *)ibuf,0,dz->buflen * 2 * sizeof(float));
								bufcnt++;
								if(dz->buflen * bufcnt >= dz->insams[0]) {
									done = 1;						//	Exit if attempted samps_read looks beyound input file end
									break;
								}
								sndseekEx(dz->ifd[0],dz->buflen * bufcnt,0);
								dz->buflen *= 2;					//	 and read 2 * buflen (so we have wraparound)
								abs_input_start_samp += dz->ssampsread/2;
								if((exit_status = read_samps(ibuf,dz))<0)
									return(exit_status);
								dz->buflen /= 2;
								if(dz->ssampsread == 0) {			//	Exit if no more samples to read (should be redundant as trap at snd seek above)
									done = 1;
									break;
								}
							}
							gpibufpos -= gpbuflen;
							dibufpos  -= (double)gpbuflen;
							ibufpos = gpibufpos * chans;
						}											//	Working in absolute sample-frame get interpolated value(s) from src/table
						for(ch = 0,nxt = chans;ch < chans;ch++,nxt++) {
							val[ch]  = ibuf[ibufpos + ch];
							diff     = ibuf[ibufpos + nxt] - val[ch];
							val[ch] += diff * frac;
						}
						localsampcnt += incr;						//	Advance by given samp-time incr
						if(localsampcnt >= nextfracsampcnt)			//	If we reach time of next fractal (we will therefore exit inner loop)
							fracindex += 2;							//	incr fracindex-value, hence advance in fractal pattern in outer loop
					}
					if(done)
						break;
				}
				intime += fracpattern[dz->fracsize];				//	overflow point in fractal table contains its duration, so advance time in input file
				outtime = (double)((dz->total_samps_written + obufpos)/chans)/srate;	//	time in output file
				if(dz->mode == 1) {
					if(outtime >= dz->param[FRACDUR])
						done = 1;
				} else {
					if(intime >= dz->duration)
						done = 1;
				}
			} else {	//	 FRACSPEC
				localtime = 0.0;
				fracindex = 0;
				while(fracindex < dz->fracsize * 2) {				//	For each unit of the fractal (2 entries - time:val - per fractal element)
					nexttime  = fracpattern[fracindex + 2];
					transpose = fracpattern[fracindex + 1];
					while(localtime < nexttime) {
						if((exit_status = fractal_spectrnsf(transpose,dz))<0)
							return(exit_status);
						memcpy((char *)dz->flbufptr[1],(char *)dz->flbufptr[0],dz->wanted * sizeof(float));
						dz->flbufptr[0] += dz->wanted;
						dz->flbufptr[1] += dz->wanted;
						if(dz->flbufptr[0] >= obuf) {	//	obuf abutts to end of ibuf
							if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
								return(exit_status);
							memset((char *)ibuf,0,dz->buflen * sizeof(float));
							memset((char *)obuf,0,dz->buflen * sizeof(float));
							if((exit_status = read_samps(ibuf,dz))<0)
								return(exit_status);
							if(dz->ssampsread == 0) {
								done = 1;
								break;
							}
							dz->flbufptr[0] = ibuf;
							dz->flbufptr[1] = obuf;
						}
						abs_window_cnt++;
						abs_time  += dz->frametime;
						localtime += dz->frametime;
						if(abs_time >= dz->duration) {
							done = 1;
							break;
						}
					}
					fracindex += 2;
				if(done)
					break;
				}

			}
		}
	}
	if(obufpos > 0) {
		if(dz->process == FRACTAL) {
			if(dz->mode == 1 && dz->total_splice > 0) {
				for(n=0,m = obufpos - chans;n < dz->splicesmps;n++,m -= chans) {
					spliceval = (double)n/(double)dz->splicesmps;	
					for(ch = 0;ch < chans;ch++)					//	Splice end of output
						obuf[m+ch] = (float)(obuf[m+ch] * spliceval);
				}
			}
		}
		if((exit_status = write_samps(obuf,obufpos,dz))<0)
			return(exit_status);							//	Write last samples to outfile
	}
	return FINISHED;
}

/**************************** GET_MAX_FRACTALISATION ****************************/

void get_max_fractalisation(dataptr dz)
{
	double minlen = dz->minstep;							//	minimum duration in fractal-shape	
	dz->maxfractalise = 0;
	if(dz->param[FRACTSTR] > 1.0)							//	If fractal-shape gets stretched anywhere, expand minlen appropriately
		minlen *= dz->param[FRACTSTR];
	while(minlen > dz->mintime) {							//	Calculate the max possible fractalisation
		minlen *= dz->minfracratio;
		dz->maxfractalise++;
	}
}

/**************************** CREATE_FRACTAL_SNDBUFS ****************************/

int create_fractal_sndbufs(dataptr dz)
{
	int exit_status, n;
	int bigbufsize, secsize, m;
	int framesize = F_SECSIZE * dz->infile->channels;
	double srate = (double)dz->infile->srate;

	if(dz->process == FRACTAL) {
		if (dz->mode == 1)	//	Mode 1, source file is used as a table to read cyclically
			bigbufsize = dz->insams[0] * sizeof(float);
		else
			bigbufsize = (int)Malloc(-1);
	 secsize = bigbufsize/framesize;
		if(secsize * framesize < bigbufsize) {
			secsize++;
			bigbufsize = secsize * framesize;
		}
	} else
		bigbufsize = dz->wanted * BUF_MULTIPLIER * sizeof(float);
	dz->buflen = bigbufsize/sizeof(float);
	if(bigbufsize * dz->bufcnt <= 0) {
		if(dz->process == FRACTAL && dz->mode == 1)
			sprintf(errstr,"Infile too large to create sound buffers.\n");
		else
			sprintf(errstr,"Insufficent memory to create sound buffers.\n");
		return(PROGRAM_ERROR);
	}
	if((dz->bigbuf = (float *)malloc(bigbufsize * dz->bufcnt)) == NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
		return(PROGRAM_ERROR);
	}
	if((dz->sampbuf = (float **)malloc(sizeof(float *) * (dz->bufcnt+1)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffers.\n");
		return(MEMORY_ERROR);
	}
	if((dz->sbufptr = (float **)malloc(sizeof(float *) * dz->bufcnt))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffer pointers.\n");
		return(MEMORY_ERROR);
	}
	if(dz->process == FRACSPEC) {
		if((dz->flbufptr = (float **)malloc(sizeof(float *) * dz->bufcnt))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for float sample buffers.\n");
			return(MEMORY_ERROR);
		}
		for(n = 0;n <dz->bufcnt; n++)
			dz->flbufptr[n] = NULL;
	}

	for(n=0;n<dz->bufcnt;n++)
		dz->sbufptr[n] = dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
	dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
	// FRACTAL
	// dz->sampbuf[0] ibuf
	// dz->sampbuf[1] iovflwbuf
	// dz->sampbuf[2] obuf
	// dz->sampbuf[3] ovflwbuf		dz->sampbuf[4] end of ovflwbuf
	// FRACSPEC
	// dz->sampbuf[0] ibuf
	// dz->sampbuf[1] obuf

	if(dz->process == FRACTAL && dz->mode == 1) {
		if((exit_status = read_samps(dz->sampbuf[0],dz))<0)	//	Read source into buffer
			return(exit_status);
		for(n=0, m = dz->insams[0];n < dz->infile->channels;n++,m++)
			dz->sampbuf[0][m] = 0;							//	wraparound points go into iovflwbuf
		dz->convertor = (double)dz->insams[0]/srate;		//	Table-read convertor for a 1Hz read
		dz->convertor *= dz->startfrq;						//	Table-read convertor for a fractal-startfrq read
	}
	return FINISHED;
}

/********************************** SPECTRNSF **********************************
 *
 * transpose spectrum, but retain original spectral envelope.
 */

int fractal_spectrnsf(double transpose,dataptr dz)
{
	int exit_status;
	double pre_totalamp, post_totalamp;
	int cc, vc;
	rectify_window(dz->flbufptr[0],dz);
	if((exit_status = fractal_extract_specenv(dz))<0)
		return(exit_status);
	if((exit_status = get_totalamp(&pre_totalamp,dz->flbufptr[0],dz->wanted))<0)
		return(exit_status);
	if(dz->vflag[1]) {
		for(cc = 0, vc = 0; cc < dz->clength; cc++, vc += 2)
			dz->flbufptr[0][FREQ] = (float)(fabs(dz->flbufptr[0][FREQ])*transpose);
	} else {
		for(cc = 0, vc = 0; cc < dz->clength; cc++, vc += 2) {
			if((exit_status = fractal_tranpose_within_formant_envelope(vc,transpose,dz))<0)
				return(exit_status);
		}
	}
	if((exit_status = fractal_reposition_partials_in_appropriate_channels(transpose,dz))<0)
		return(exit_status);
	if((exit_status = get_totalamp(&post_totalamp,dz->flbufptr[0],dz->wanted))<0)
		return(exit_status);
	return normalise(pre_totalamp,post_totalamp,dz);
}

	/************************** TRANPOSE_WITHIN_FORMANT_ENVELOPE *****************************/

int fractal_tranpose_within_formant_envelope(int vc,double transpose,dataptr dz)
{
	int exit_status;
	double thisspecamp, newspecamp, thisamp, formantamp_ratio; 
	if((exit_status = fractal_getspecenvamp(&thisspecamp,(double)dz->flbufptr[0][FREQ],dz))<0)
		return(exit_status);
	dz->flbufptr[0][FREQ] = (float)(fabs(dz->flbufptr[0][FREQ])*transpose);
	if(dz->flbufptr[0][FREQ] < dz->nyquist) {
		if(thisspecamp < VERY_TINY_VAL)
			dz->flbufptr[0][AMPP] = 0.0f;
		else {
			if((exit_status = fractal_getspecenvamp(&newspecamp,(double)dz->flbufptr[0][FREQ],dz))<0)
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

int fractal_reposition_partials_in_appropriate_channels(double transpose,dataptr dz)
{
	int exit_status;
	int truecc,truevc;
	int cc, vc;
	for(vc = 0, cc= 0; vc < dz->wanted; vc+=2, cc++) {						/* 1 */
		dz->windowbuf[0][AMPP] = 0.0f;
		dz->windowbuf[0][FREQ] = (float)(cc * dz->chwidth);
	}
	if(transpose > 1.0f) {			/* 2 */
		for(cc=dz->clength-1,vc = dz->wanted-2; cc>=0; cc--, vc-=2) {
			if(dz->flbufptr[0][FREQ] < dz->nyquist && dz->flbufptr[0][AMPP] > 0.0f) {
				if((exit_status = fractal_get_channel_corresponding_to_frq(&truecc,(double)dz->flbufptr[0][FREQ],dz))<0)
					return(exit_status);
				truevc = truecc * 2;
				if((exit_status = move_data_into_appropriate_channel(vc,truevc,dz->flbufptr[0][AMPP],dz->flbufptr[0][FREQ],dz))<0)
					return(exit_status);
			}
		}
		for(vc = 0; vc < dz->wanted; vc++)
			dz->flbufptr[0][vc] = dz->windowbuf[0][vc];	
	} else if(transpose < 1.0f){		/* 3 */
		for(cc=0,vc = 0; cc < dz->clength; cc++, vc+=2) {
			if(dz->flbufptr[0][FREQ] < dz->nyquist && dz->flbufptr[0][FREQ]>0.0) {
				if((exit_status = fractal_get_channel_corresponding_to_frq(&truecc,(double)dz->flbufptr[0][FREQ],dz))<0)
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

/******************* ZERO_OUTOFRANGE_CHANNELS *****************/

int fractal_zero_outofrange_channels(double *totalamp,double lofrq_limit,double hifrq_limit,dataptr dz)
{
	int cc, vc;
	*totalamp = 0.0;
	for(cc = 0,vc = 0; cc < dz->clength; cc++, vc += 2) {
		if(dz->flbufptr[0][FREQ] < lofrq_limit || dz->flbufptr[0][FREQ] > hifrq_limit)
			dz->flbufptr[0][AMPP] = 0.0f;
		else
			*totalamp += dz->flbufptr[0][AMPP];
	}
	return(FINISHED);
}

/**************************** GETSPECENVAMP *************************/

int fractal_getspecenvamp(double *thisamp,double thisfrq,dataptr dz)
{
	double pp, ratio, ampdiff;
	int z = 1;
	if(thisfrq<0.0)	{ 	/* NOT SURE THIS IS CORRECT */
		*thisamp = 0.0;	/* SHOULD WE PHASE INVERT & RETURN A -ve AMP ?? */
		return(FINISHED);	
	}
	if(thisfrq<=1.0)
		pp = 0.0;
	else
		pp = log10(thisfrq); 
	while( dz->specenvpch[z] < pp){
		z++;
		/*RWD may need to trap on size of array? */
		if(z == dz->infile->specenvcnt -1)
			break;
	}
	ratio    = (pp - dz->specenvpch[z-1])/(dz->specenvpch[z] - dz->specenvpch[z-1]);
	ampdiff  = dz->specenvamp[z] - dz->specenvamp[z-1];
	*thisamp = dz->specenvamp[z-1] + (ampdiff * ratio);
	*thisamp = max(0.0,*thisamp);
	return(FINISHED);
}

/********************** EXTRACT_SPECENV *******************/

int fractal_extract_specenv(dataptr dz)
{
	int    n, cc, vc, specenvcnt_less_one;
	int    botchan, topchan;
	double botfreq, topfreq;
	double bwidth_in_chans;
 	specenvcnt_less_one = dz->infile->specenvcnt - 1;
	vc = 0;
	//	RECTIFY_CHANNEL_FRQ_ORDER
	for(n=0;n<dz->infile->specenvcnt;n++)
		dz->specenvamp[n] = (float)0.0;
	topfreq = 0.0f;
	n = 0;
	while(n < specenvcnt_less_one) {
		botfreq  = topfreq;
		botchan  = (int)((botfreq + dz->halfchwidth)/dz->chwidth); /* TRUNCATE */
		botchan -= 4;	//	CHAN_SRCHRANGE_F
		botchan  =  max(botchan,0);
		topfreq  = dz->specenvtop[n];
		topchan  = (int)((topfreq + dz->halfchwidth)/dz->chwidth); /* TRUNCATE */
		topchan += 4;	//	CHAN_SRCHRANGE_F;
		topchan  =  min(topchan,dz->clength);
		for(cc = botchan,vc = botchan * 2; cc < topchan; cc++,vc += 2) {
			if(dz->flbufptr[0][FREQ] >= botfreq && dz->flbufptr[0][FREQ] < topfreq)
				dz->specenvamp[n] = (float)(dz->specenvamp[n] + dz->flbufptr[0][AMPP]);
		}
		bwidth_in_chans   = (double)(topfreq - botfreq)/dz->chwidth;
		dz->specenvamp[n] = (float)(dz->specenvamp[n]/bwidth_in_chans);
		n++;
	}
	return(FINISHED);
}

/************************** INITIALISE_SPECENV *********************
 *
 * 	MAR 1998: not sure if the follwoing comment is relevant any more
 *	but I wont risk changing it at this stage.
 *
 * 	WANTED and CLENGTH are calculated from scratch here, as dz->wanted
 *	gets set equal to dz->specenvcnt for calculations on formant data,
 * 	while dz->clength may not yet be set!!		
 */

int fractal_initialise_specenv(int *arraycnt,dataptr dz)
{
	*arraycnt   = (dz->infile->channels/2) + 1;
	if((dz->specenvfrq = (float *)malloc((*arraycnt) * sizeof(float)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for formant frq array.\n");
		return(MEMORY_ERROR);
	}
	if((dz->specenvpch = (float *)malloc((*arraycnt) * sizeof(float)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for formant pitch array.\n");
		return(MEMORY_ERROR);
	}
	/*RWD  zero the data */
	memset(dz->specenvpch,0,*arraycnt * sizeof(float));

	if((dz->specenvamp = (float *)malloc((*arraycnt) * sizeof(float)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for formant aplitude array.\n");
		return(MEMORY_ERROR);
	}
	if((dz->specenvtop = (float *)malloc((*arraycnt) * sizeof(float)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for formant frq limit array.\n");
		return(MEMORY_ERROR);
	}
	if((dz->windowbuf = (float **)malloc(sizeof(float *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for extra float sample buffers.\n");
		return(MEMORY_ERROR);
	}
	if((dz->windowbuf[0] = (float *)malloc(dz->wanted * sizeof(float)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for extra float sample buffer 0.\n");
		return(MEMORY_ERROR);
	}
	return(FINISHED);
}

/************************************ FRACTAL_SET_SPECENV_FRQS ****************************/

int fractal_set_specenv_frqs(int arraycnt,dataptr dz)
{
	int exit_status;
	double bandbot;
	double *interval;
	int m, n, k = 0, cc;
	//	PICHWISE_FORMANTS
	if((exit_status = fractal_setup_octaveband_steps(&interval,dz))<0)
		return(exit_status);
	if((exit_status = fractal_setup_low_octave_bands(arraycnt,dz))<0)
		return(exit_status);
	k  = 4; //	TOP_OF_LOW_OCTAVE_BANDS
	cc = 8;	//	CHAN_ABOVE_LOW_OCTAVES
	while(cc <= dz->clength) {
		m = 0;
		if((bandbot = dz->chwidth * (double)cc) >= dz->nyquist)
			break;
		for(n=0;n<dz->formant_bands;n++) {
			if(k >= arraycnt) {
				sprintf(errstr,"Formant array too small: fractal_set_specenv_frqs()\n");
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

int fractal_setup_octaveband_steps(double **interval,dataptr dz)
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

int fractal_setup_low_octave_bands(int arraycnt,dataptr dz)
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
			sprintf(errstr,"Insufficient low octave band setups in fractal_setup_low_octave_bands()\n");
			return(PROGRAM_ERROR);
		}
	}
	return(FINISHED);
}

/**************************** GET_CHANNEL_CORRESPONDING_TO_FRQ ***************************/

int fractal_get_channel_corresponding_to_frq(int *chan,double thisfrq,dataptr dz)
{	
	if(dz->chwidth <= 0.0) {
		sprintf(errstr,"chwidth not set in fractal_get_channel_corresponding_to_frq()\n");
		return(PROGRAM_ERROR);
	}
	if(thisfrq < 0.0) {
		sprintf(errstr,"-ve frequency in fractal_get_channel_corresponding_to_frq()\n");
		return(PROGRAM_ERROR);
	}
	if(thisfrq > dz->nyquist) {
		sprintf(errstr,"frequency beyond nyquist in fractal_get_channel_corresponding_to_frq()\n");
		return(PROGRAM_ERROR);
	}
 	*chan = (int)((fabs(thisfrq) + dz->halfchwidth)/dz->chwidth);  /* TRUNCATE */
	if(*chan >= dz->clength) {
		sprintf(errstr,"chan (%d) beyond clength-1 (%d) returned: fractal_get_channel_corresponding_to_frq()\n",
		*chan,(dz->clength)-1);
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

