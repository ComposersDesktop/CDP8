/*
 * Copyright (c) 1983-2013 Trevor Wishart and Composers Desktop Project Ltd
 * http://www.trevorwishart.co.uk
 * http://www.composersdesktop.com
 *
 This file is part of the CDP System.

    The CDP System is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    The CDP System is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License aint with the CDP System; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
 *
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
#include <ctype.h>
#include <sfsys.h>
#include <string.h>
#include <srates.h>
#ifdef unix
#include <aaio.h>
#endif

#define	S_OFF 0
#define	S_ON  1
#define SIGNAL_TO_LEFT  (0)
#define SIGNAL_TO_RIGHT (1)
#define ROOT2		(1.4142136)

#define SYN_FROMROOT 0
#define SYN_TOROOT   1
#define SYN_SPACED   2
#define SYN_X        3
#define SYN_JUMP     4

#define SYNDUFF_MIN

#if defined unix || defined __GNUC__
#define round(x) lround((x))
#endif


#ifndef HUGE
#define HUGE 3.40282347e+38F
#endif

char errstr[2400];

int anal_infiles = 1;
int	sloom = 0;
int sloombatch = 0;

const char* cdp_version = "7.1.0";

//CDP LIB REPLACEMENTS
static int setup_synthesizer_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int setup_synthesis_param_ranges_and_defaults(dataptr dz);
static int handle_the_outfile(int *cmdlinecnt,char ***cmdline,dataptr dz);
static int open_the_outfile(dataptr dz);
static int handle_the_special_data(char *str,dataptr dz);
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

static int synthesis_param_preprocess(int **perm,int **permon,int **permoff,int **superperm,double *minrate,int *maxsteps,dataptr dz);
static int synthesis(int *perm,int *permon,int *permoff,int *superperm,double minrate,int maxsteps,dataptr dz);
static void incr_sinptr(int n,double time,double onehzincr,dataptr dz);
static double read_level(int n,double time,dataptr dz);
static int create_synthesizer_sndbufs(dataptr dz);
static int generate_packet_envelope (dataptr dz);
static double read_packet_envelope(int kk,double incr,dataptr dz);
static int modify_packet_envelope(dataptr dz);
static void rndintperm(int *perm,int cnt);
static void get_current_partial_vals(double time,double *pvals,int totalpartials,dataptr dz);
static void pancalc(double position,double *leftgain,double *rightgain);
static void sort_partials_into_ascending_frq_order(int total_partialcnt,double *pvals,double *sinptr,
				double **llev,double **rlev,int **onoff,int **lmost,int **origspl,int *splordr,dataptr dz);
static void resort_partials_into_original_frq_order(int total_partialcnt,double *pvals,double *sinptr,
				double **llev,double **rlev,int **onoff,int **lmost,int **origspl,int *splordr,dataptr dz);
static void xclusive(int *perm,int *permon,int *permoff,int max_partials_cnt,int partials_in_play, int **onoff,int stepcnt);
static double emergepos(int emergchan,int chans,double time,double timespan);
static double convergepos(int converchan,int chans,double time,double convergetime,double dur);
static void spacebox_apply(double pos, double lev,int chans,int *lmost, int *rmost,double *rlev,double *llev,int spacetyp);
static void output_special_spatialisation_sample(float *obuf,int sampcnt,int switchpos,int chans,double val,double valr,int lmost,int rmost,int spacetyp);
static void spacebox(double *pos, int *switchpos, double posstep, int chans, int spacetyp, int configno, int configcnt,int *superperm);

static double sinread(double *tabpos,double frq,dataptr dz);
static void duffing_osc(double *val,double *vel, double delta_t,double *tabpos,dataptr dz);
static int duffing(dataptr dz);

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
	int exit_status;
	dataptr dz = NULL;
	char **cmdline, sfnam[400];
	int  cmdlinecnt;
	aplptr ap;
	int is_launched = FALSE;
	int *perm, *permon, *permoff, *superperm;
	int maxsteps = 0;
	double minrate = 0.0;
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
		dz->maxmode = 4;
		if((exit_status = get_the_mode_from_cmdline(cmdline[0],dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(exit_status);
		}
		cmdline++;
		cmdlinecnt--;
		// setup_particular_application =
		if(dz->mode == 2 && cmdlinecnt < 8) {
			usage2("synthesis");
			return(FAILED);
		}
		if((exit_status = setup_synthesizer_application(dz))<0) {
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
	dz->infile->channels = 1;
	// parse_infile_and_hone_type() = 
	// setup_param_ranges_and_defaults() =
	if((exit_status = setup_synthesis_param_ranges_and_defaults(dz))<0) {
		exit_status = print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	// open_first_infile() : redundant
	// handle_extra_infiles() : redundant
	// handle_outfile() = 
	if((exit_status = handle_the_outfile(&cmdlinecnt,&cmdline,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}

//	handle_formants()			redundant
//	handle_formant_quiksearch()	redundant
	if(dz->mode != 3) {
		strcpy(sfnam,cmdline[0]);
		cmdlinecnt--;
		cmdline++;
	}
	if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {		// CDP LIB
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
//	check_param_validity_and_consistency() redundant
	if(dz->mode == 3) {
		if((dz->parray = (double **)malloc(sizeof(double *)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to create sinetable array.\n");
			return(MEMORY_ERROR);
		}
	} else {
		if((exit_status = handle_the_special_data(sfnam,dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
	}
	is_launched = TRUE;
	if((exit_status = create_synthesizer_sndbufs(dz))<0) {										// CDP LIB
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = synthesis_param_preprocess(&perm,&permon,&permoff,&superperm,&minrate,&maxsteps,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if(dz->mode == 2 && dz->iparam[SYNTH_CHANS] > 1)
		dz->infile->channels = dz->iparam[SYNTH_CHANS];
	if((exit_status = open_the_outfile(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	//spec_process_file =
	if(dz->mode == 3) {
//	NB sintable is in dz->parray[0]
//	dz->rampbrksize = sample-length of final splice
		if((exit_status = duffing(dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
	} else {
		if((exit_status = synthesis(perm,permon,permoff,superperm,minrate,maxsteps,dz))<0) {
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
	p = filename;					//	Drop file extension
	while(*p != ENDOFSTR) {
		if(*p == '.') {
			*p = ENDOFSTR;
			break;
		}
		p++;
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

/************************* SETUP_SYNTHESIZER_APPLICATION *******************/

int setup_synthesizer_application(dataptr dz)
{
	int exit_status;
	aplptr ap;
	if((exit_status = establish_application(dz))<0)		// GLOBAL
		return(FAILED);
	ap = dz->application;
	// SEE parstruct FOR EXPLANATION of next 2 functions
	switch(dz->mode) {
	case(0): 
		if((exit_status = set_param_data(ap,SYN_PARTIALS,3,3,"idD"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","",0,0,""))<0)
			return(exit_status);
		break;
	case(1): 
		if((exit_status = set_param_data(ap,SYN_PARTIALS,3,3,"idD"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"nc",2,"DD","f",1,0,"0"))<0)
			return(exit_status);
		break;
	case(2): 
		if((exit_status = set_param_data(ap,SYN_PARTIALS,6,6,"idDiiD"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"udfsneEcCtr",11,"ddddiididid","azmxj",5,0,"00000"))<0)
			return(exit_status);
		break;
	case(3): 
		if((exit_status = set_param_data(ap,0,6,6,"idDDdd"))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","",0,0,""))<0)
			return(exit_status);
		break;
	}
	// set_legal_infile_structure -->
	dz->has_otherfile = FALSE;
	// assign_process_logic -->
	dz->input_data_type = NO_FILE_AT_ALL;
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

/************************* SETUP_SYNTHESIS_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_synthesis_param_ranges_and_defaults(dataptr dz)
{
	int exit_status;
	aplptr ap = dz->application;
	// set_param_ranges()
	ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
	// NB total_input_param_cnt is > 0 !!!
	if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
		return(FAILED);
	// get_param_ranges()
	ap->lo[SYNTHSRAT]	= 16000;
	ap->hi[SYNTHSRAT]	= 96000;
	ap->default_val[SYNTHSRAT] = 44100.0;
	ap->lo[SYNTH_DUR]	= 0.0;
	ap->hi[SYNTH_DUR]	= 32767.0;
	ap->default_val[SYNTH_DUR]	= 1.0;
	ap->lo[SYNTH_FRQ]	= .001;
	ap->hi[SYNTH_FRQ]	= 10000;
	ap->default_val[SYNTH_FRQ] = 440;
	if(dz->mode == 1) {
		ap->lo[SYNTH_SQZ]	= 0.0;
		ap->hi[SYNTH_SQZ]	= 1000.0;
		ap->default_val[SYNTH_SQZ]	= 1.0;
		ap->lo[SYNTH_CTR]	= -1.0;
		ap->hi[SYNTH_CTR]	= 1.0;
		ap->default_val[SYNTH_CTR]	= 0.0;
	} else if(dz->mode == 2) {
		ap->lo[SYNTH_CHANS]	= 1;
		ap->hi[SYNTH_CHANS]	= 16.0;
		ap->default_val[SYNTH_CHANS] = 1.0;
		ap->lo[SYNTH_MAX]	= 1.0;
		ap->hi[SYNTH_MAX]	= 8.0;
		ap->default_val[SYNTH_MAX]	= 3.0;
		ap->lo[SYNTH_RATE]	= 0.004;
		ap->hi[SYNTH_RATE]	= 100;
		ap->default_val[SYNTH_RATE]	= 0.1;
		ap->lo[SYNTH_RISE]	= 0.0;
		ap->hi[SYNTH_RISE]	= 100;
		ap->default_val[SYNTH_RISE]	= 0;
		ap->lo[SYNTH_FALL]	= 0.0;
		ap->hi[SYNTH_FALL]	= 100;
		ap->default_val[SYNTH_FALL]	= 0;
		ap->lo[SYNTH_STDY]	= 0.0;
		ap->hi[SYNTH_STDY]	= 3600;
		ap->default_val[SYNTH_STDY]	= 0;
		ap->lo[SYNTH_SPLEN]	= 2;
		ap->hi[SYNTH_SPLEN]	= 50;
		ap->default_val[SYNTH_SPLEN] = 5;
		ap->lo[SYNTH_NUM]	= 0;
		ap->hi[SYNTH_NUM]	= 1000;
		ap->default_val[SYNTH_NUM] = 0;
		ap->lo[SYNTH_EFROM]	= 0;
		ap->hi[SYNTH_EFROM]	= 16.0;
		ap->default_val[SYNTH_EFROM] = 0;
		ap->lo[SYNTH_ETIME]	= 0;
		ap->hi[SYNTH_ETIME]	= 32767.0;
		ap->default_val[SYNTH_ETIME] = 0;
		ap->lo[SYNTH_CTO]	= 0;
		ap->hi[SYNTH_CTO]	= 16.0;
		ap->default_val[SYNTH_CTO] = 0;
		ap->lo[SYNTH_CTIME]	= 0;
		ap->hi[SYNTH_CTIME]	= 32767.0;
		ap->default_val[SYNTH_CTIME] = 0;
		ap->lo[SYNTH_STYPE]	= 0;
		ap->hi[SYNTH_STYPE]	= 14;
		ap->default_val[SYNTH_STYPE] = 0;
		ap->lo[SYNTH_RSPEED]	= -20;
		ap->hi[SYNTH_RSPEED]	= 20;
		ap->default_val[SYNTH_RSPEED] = 0;
	} else if(dz->mode == 3) {
		ap->lo[SYNTH_FRQ]	= .1;
		ap->hi[SYNTH_FRQ]	= 200;
		ap->default_val[SYNTH_FRQ] = 70;
		ap->lo[SYNTH_DAMP]	= .15;
		ap->hi[SYNTH_DAMP]	= 2;
		ap->default_val[SYNTH_DAMP] = .5;
		ap->lo[SYNTH_K]	= -10;
		ap->hi[SYNTH_K]	= 10;
		ap->default_val[SYNTH_K] = 1;
		ap->lo[SYNTH_B]	= 20;
		ap->hi[SYNTH_B]	= 50;
		ap->default_val[SYNTH_B] = 30;
	}
	dz->maxmode = 4;
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
			if((exit_status = setup_synthesizer_application(dz))<0)
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
	usage2("synthesis");
	return(USAGE_ONLY);
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	if(!strcmp(prog_identifier_from_cmdline,"synthesis"))				dz->process = SYNTHESIZER;
	else {
		fprintf(stderr,"Unknown program identification string '%s'\n",prog_identifier_from_cmdline);
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
	int k;
	if(!strcmp(str,"synthesis")) {
		fprintf(stderr,
	    "USAGE:\n"
	    "newsynth synthesis 1 outf spectrum srate dur frq\n"
	    "newsynth synthesis 2 outf spectrum srate dur frq [-nnarrowing] [-ccentring]\n"
	    "newsynth synthesis 3 outf spectrum srate dur frq chans maxrange rate\n"
		"    [-urise] [-dfall] [-fsteady] [-ssplice] [-nN] [-a] [-z] [-x]\n"
		"    [-tspacetype] [-rrotspeed]\n"
		"    [[-m] [-j] [efrom -Etime] [cto -Ctime]]\n"
	    "newsynth synthesis 4 outf          srate dur frq damping k b\n"
		"\n"
		"Synthesize complex spectra.\n"
		"\n"
		"MODE 1 Generates tones with any number of (possibly varying) partials.\n"
		"MODE 2 Generates wave-packet streams with any number of (possibly varying) partials.\n"
		"MODE 3 Multichan Mode1 where partials (spread over N octaves) fade in-out randomly.\n"
		"MODE 4 Duffing damped oscillator (frq, amplitude and damping can vary through time).\n"
		"\n"
		"Press any key to see further information.\n");
		while(!kbhit())
			;
		if(kbhit()) {
			fprintf(stderr,
			"SPECTRUM  Listing of partial ratios and relative levels, against time.\n"
			"      Data: text file of data lines. Every line has same number of entries.\n"
			"      1ST ENTRY on each line is time. Times must start at zero and increase.\n"
			"      ALL EVEN NUMBERED ENTRIES are partial numbers.\n"
			"      For tone-generation, the first partial number on each line must be 1.\n"
			"      Partial numbers must increase from entry to entry.\n"
			"      ALL OTHER ODD NUMBERED ENTRIES are partial levels, and may have any value.\n"
			"      -ve values invert the phase of the partial.\n"
			"SRATE     Sample rate of synthesized sound.\n"
			"DUR       Duration of synthesized sound.\n"
			"FRQ       Possibly time-varying Fundamental frq of output (0.001 to 10000Hz) OR\n"
			"          (Mode 4) of forcing oscillation (1-200Hz).\n"
			"DAMPING   (Mode 4) Possibly time-varying damping of forced oscillation (0.15 to 2).\n"
			"K, B      Coefficients determe nature of damping. (k -10 to 10 : b 20 to 50)\n"
			"NARROWING Narrowing of packet envelope (0 - 1000).\n"
			"          Values below 1.0 broaden the packet.\n"
			"          Values very close to zero may produce clicks (square-wave envelope).\n"
			"          Very high vals with very high frqs may produce click-impulses or silence.\n"
			"CENTRING  Centring of peak of packet envelope.\n"
			"          0  peak at centre: -1 peak at start: 1 peak at end.\n"
			"CHANS     Number of output channels.\n"
			"MAXRANGE  Max range of transposition of spectral components (in whole 8vas).\n"
			"STEP      Average time between changes to partial-content of output.\n"
			"RISE      Time to expand to maximum range.\n"
			"FALL      Time to return to initial range, before end.\n"
			"STEADY    Duration of steady state at sound end.\n"
			"SPLICE    Splices for partial entry and exit, in mS.\n"
			"-nN       (Number) Same fixed number (N) of partials chosen for each event.\n"
			"-a        Initial rise in number of partials from only-the-fundamental.\n"
			"-z        Fall in number of partials , during \"steady state\" to fundamental.\n"
			"-x        (Xclusive) change all partials (as far as poss) from event to event.\n"
			"-m        (Move) Distribute partials in space.\n"
			"-j        (Jump) All partials assigned to same location for any one event.\n"
			"SPACETYPE Type of output spatialisation.\n"
			"ROTSPEED  rotation speed (for certain spatialisation types).\n"
			"-e  -E    (Emerge) sound emerges from channel \"from\" over time \"time\" at start.\n"
			"-c  -C    (Converge) Sound converges to channel \"to\" over time \"time\" at end.\n"
			"NB: Flags -j,-e,-E,-c,-C only operational if -m set.\n"          
			"NB: Flags with NO params must be placed AFTER any flags WITH params, on the cmdline.\n"          
			"\n"
			"Hit key 's' to continue to \"SPACETYPE\" information, or 'e' to exit.\n");
		}
		while((k = getch())!='s' && k != 'e')
			;
		if(k == 's') {
			fprintf(stderr,
			"\n"	
			"SPACETYPE options : For 8-channel output only.\n"	
			"\n"	
			"1   Positions alternate between Left and Right sides, but are otherwise random.\n"	
			"2   Positions alternate between Front and Back, but are otherwise random.\n"
			"3   Rotating clockwise or anticlockwise.\n"	
			"4   Random permutations of all 8 channels.\n"	
			"5   ... plus all possible pairs of channels.\n"	
			"6   ... plus all possible meaningful small and large triangles.\n"	
			"7   ... plus square, diamond and all-at-once.\n"	
			"      In types 4 to 7, all members of perm used before next perm starts.\n"	
			"8   Alternate between all-left and all-right.\n"	
			"9   Alternate between all-front and all-back.\n"	
			"10  Alternate between all-square and all-diamond.\n"	
			"11  Rotate triangle formed by lspkrs 2-apart clockwise.\n"	
			"12  Rotate triangle formed by lspkrs 3-apart clockwise.\n"	
			"13  Rotate triangle formed by lspkrs 2-apart anticlockwise.\n"	
			"14  Rotate triangle formed by lspkrs 3-apart anticlockwise.\n");
		}
	} else
		fprintf(stdout,"Unknown option '%s'\n",str);
	return(USAGE_ONLY);
}

int usage3(char *str1,char *str2)
{
	fprintf(stderr,"Insufficient parameters on command line.\n");
	return(USAGE_ONLY);
}

/**************************** SYNTHESIS_PARAM_PREPROCESS *************************/

int synthesis_param_preprocess (int **perm,int **permon,int **permoff,int **superperm,double *minrate,int *maxsteps,dataptr dz)
{
	int exit_status, sinarray, pntarray = 0, chans, configno;
	int n, m;
	double *sintab, *thispartials, srate, nyquist, endsplice, time, top, maxrate;
	int partialscnt = dz->itemcnt, entrycnt = dz->ringsize;

	if(BAD_SR(dz->param[SYNTHSRAT])) {
		sprintf(errstr,"Invalid sample rate (%lf) entered.\n",dz->param[SYNTHSRAT]);
		return(MEMORY_ERROR);
	}
	dz->infile->srate = dz->iparam[SYNTHSRAT];
	srate = (double)dz->iparam[SYNTHSRAT];
	nyquist = srate/2.0;
	chans = dz->iparam[SYNTH_CHANS];
	//	Establish end splice length

	dz->iparam[SYNTH_DUR] = (int)round(dz->param[SYNTH_DUR] * srate);
	if(dz->mode == 2) {
		if(dz->brksize[SYNTH_RATE]) {
			if((exit_status = get_maxvalue(SYNTH_RATE,&maxrate,dz))<0)
				return PROGRAM_ERROR;
			if((exit_status = get_minvalue_in_brktable(minrate,SYNTH_RATE,dz))<0)
				return PROGRAM_ERROR;
		} else {
			maxrate = dz->param[SYNTH_RATE];
			*minrate = dz->param[SYNTH_RATE];
		}
		if(maxrate >= dz->param[SYNTH_DUR]/2.0) {
			sprintf(errstr,"(max) Rate (%lf) must be less than half duration (%lf).\n",maxrate,dz->param[SYNTH_DUR]);
			return(DATA_ERROR);
		}
		if(dz->iparam[SYNTH_STYPE] > 0) {
			if(*minrate <= dz->param[SYNTH_SPLEN] * MS_TO_SECS * 2.0) {
				sprintf(errstr,"(min) Rate (%lf) must be greater than 2 * splice (%lf) For special spatialisation types\n",*minrate,dz->param[SYNTH_SPLEN] * MS_TO_SECS * 2.0);
				return(DATA_ERROR);
			}
		} else {
			if(*minrate <= dz->param[SYNTH_SPLEN] * MS_TO_SECS) {
				sprintf(errstr,"(min) Rate (%lf) must be greater than splicelength (%lf).\n",*minrate,dz->param[SYNTH_SPLEN] * MS_TO_SECS);
				return(DATA_ERROR);
			}
		}
		if(dz->vflag[SYN_SPACED] && dz->iparam[SYNTH_CHANS] < 2) {		//	Can't spatialise to mono output!!
			fprintf(stdout,"WARNING: Mono output: Can't spatialise the output.\n");
			fflush(stdout);;
			dz->vflag[SYN_SPACED] = 0;
		}
		endsplice = 50.0 * MS_TO_SECS;
		dz->rampbrksize = (int)floor(endsplice * srate);
	} else {
		endsplice = 50.0 * MS_TO_SECS;									//	Go for big splice
		if(dz->param[SYNTH_DUR] <= endsplice * 2)
			endsplice = min(dz->param[SYNTH_DUR]/4.0,5.0 * MS_TO_SECS);	//	Else go for small splice
		dz->rampbrksize = (int)floor(endsplice * srate);				//	Establish size of final splice
	}
	//	Check no partials go over nyquist
	if(dz->mode != 3) {
		for(n=0;n < partialscnt;n++) {
			thispartials = dz->parray[n];
			for(m=0;m<entrycnt;m+=2) {
				if(dz->brksize[SYNTH_FRQ]) {
					time = thispartials[m];
					if((exit_status = read_value_from_brktable(time,SYNTH_FRQ,dz))<0)
						return(exit_status);
				}
				top = thispartials[m+1] * dz->param[SYNTH_FRQ];
				if(top >= nyquist) {
					sprintf(errstr,"Partial %lf at time %lf (frq %lf) is above the nyquist (%lf)\n",thispartials[m+1],thispartials[m],dz->param[SYNTH_FRQ],nyquist);
					return(DATA_ERROR);
				}
			}
		}
	}

	//	Establish sine-table
	if(dz->mode == 3)
		sinarray = 0;
	else {
		sinarray = partialscnt * 2;
		pntarray = sinarray + 1;
	}
	if((dz->parray[sinarray] = (double *)malloc((SYNTH_TABSIZE + 1) * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for sine table.\n");
		return(MEMORY_ERROR);
	}
	sintab = dz->parray[sinarray];
	for(n=0;n<SYNTH_TABSIZE;n++)
		sintab[n] = sin(PI * 2.0 * ((double)n/(double)SYNTH_TABSIZE));
	sintab[n] = sintab[0];							/* wrap around point */

	//	Pointers into sintable for all partials

	if(dz->mode == 3)
		return FINISHED;
	else if(dz->mode == 2) {

/*
 *																			  | |
 * MODE 2 arrays															  |positions
 * pcnt = partialcnt mpcnt = maxpartial cnt (partials + all transpositions) | | |
 *																		  |	current
 *																		  | frqs|
 *	parray	|----------|----------|-|-|-----------------|-----------------|-|-|-|
 *          | tvarying pno+plevel |s|s|	  left_level	| right-level	  step| |
 *			|	(Mpcnt*2)         |i|i|	      mpcnt		|	mpcnt         times |
 *			|					  |n|n|					|				  | | | |
 *	address	0			   mpcnt*2| |p|(mpcnt*2)+2		|(mpcnt*3)+2	  | (mpcnt*4)+3
 *			|					  |	|t|					|				  (mpcnt*4)+2
 *			|					  |	|r|					|				  | | (mpcnt*4)+4
 *	lengths |  linelen of srcdata | | | maxsteps		|	maxsteps	  | |m|	|
 *								  |s|t|									  |t|p|t|
 *	(slen = sintablen)			  |l|o|									  |o|c|o|
 *	(totl = estimate of no		  |e|t|									  |t|n|t|
 *		    of timesteps used)	  |n|l|									  |l|t|l|
 *
 *
 * iparray	|-----------------|-----------------|-----------------|-| switchpos
 *			| on-off flags	  | leftmost chan	|		spo		  |s|porder
 *			|	(mpcnt)		  |	   (mpcnt)		|	  (mpcnt)	  |p| | (mpcnt*3)+2
 *			|				  |					|				  |l| (mpcnt*3)+1 
 *  address	0				mpcnt			  mpcnt*2			  (mpcnt*3)
 *			|				  |					|				  |c| |
 *	lengths |	maxsteps	  |  maxsteps		|	maxsteps	  |nmpcnt
 *																  |t| mpcnt
 *																  |r| |
 *	(splcntrs = splice counters)								  |s| |
 *	(spo = orig values of splice counters)		
 */

//	A sine-table pointer for every partial and every partial transposition		
		if((dz->parray[pntarray] = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for sine table pointers.\n");
			return(MEMORY_ERROR);
		}
//	An array for every partials on-off markers, every leftmost-chan, every splice-counter-orig-vals, plus actual splice-cntrs + porder
		if((dz->iparray = (int **)malloc(((partialscnt * 3) + 3) * sizeof(int *)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for integer array fpr partial on-off markers.\n");
			return(MEMORY_ERROR);
		}
		*maxsteps = (int)ceil(dz->param[SYNTH_DUR]/(*minrate)) + 100; // SAFETY

		for(n=0,m=dz->temp_sampsize;n<partialscnt;n++,m++) {
//	An array of on-off switching vals at steptimes, for every partial and partial-transposition
			if((dz->iparray[n] = (int *)malloc((*maxsteps) * sizeof(int)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY for partial on-off marker array %d.\n",n);		//	base address = 0
				return(MEMORY_ERROR);
			}
//	An array of leftmost output channel at steptimes, for every partial and partial-transposition
			if((dz->iparray[n+partialscnt] = (int *)malloc((*maxsteps) * sizeof(int)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY for partial on-off marker array %d.\n",n);		//	base address = partialscnt
				return(MEMORY_ERROR);
			}
//	An array of original_vals of splice_counters, at steptimes for every partial and partial-transposition
			if((dz->iparray[n+(partialscnt*2)] = (int *)malloc((*maxsteps) * sizeof(int)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY for partial on-off marker array %d.\n",n);		//	base address = partialscnt
				return(MEMORY_ERROR);
			}
// An array levels (or of left levels), at every steptime, for each partial and partial-transposition			//	base address = dz->temp_sampsize
			if((dz->parray[m] = (double *)malloc((*maxsteps) * sizeof(double)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY for partial on-off gain and step times vals, array %d.\n",n);
				return(MEMORY_ERROR);
			}
// An array of right levels, at every steptime, for each partial and partial-transposition			//	base address = dz->temp_sampsize + partialscnt
			if((dz->parray[m+partialscnt] = (double *)malloc((*maxsteps) * sizeof(double)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY for partial on-off gain and step times vals, array %d.\n",n);
				return(MEMORY_ERROR);
			}
		}
// An array of steptimes
		if((dz->parray[m+partialscnt] = (double *)malloc((*maxsteps) * sizeof(double)))==NULL) {		//	address = dz->temp_sampsize + (partialscnt * 2)
			sprintf(errstr,"INSUFFICIENT MEMORY for partial on-off gain and step times vals, array %d.\n",n);
			return(MEMORY_ERROR);
		}
// An array of current frqs of partials
		if((dz->parray[m+partialscnt+1] = (double *)malloc(partialscnt * sizeof(double)))==NULL) { //	address = dz->temp_sampsize + (partialscnt * 2) + 1
			sprintf(errstr,"INSUFFICIENT MEMORY for partial on-off gain and step times vals, array %d.\n",n);
			return(MEMORY_ERROR);
		}
 // An array of current spatial positions
		if((dz->parray[m+partialscnt+2] = (double *)malloc((*maxsteps) * sizeof(double)))==NULL) { //	address = dz->temp_sampsize + (partialscnt * 2) + 2
			sprintf(errstr,"INSUFFICIENT MEMORY for step positions.\n");
			return(MEMORY_ERROR);
		}

// An array of splice-counters for every partial
		if((dz->iparray[partialscnt*3] = (int*)malloc(partialscnt * sizeof(int)))==NULL) {			//	address = (partialscnt * 3)
			sprintf(errstr,"INSUFFICIENT MEMORY for partial on-off gain and step times vals, array %d.\n",n);
			return(MEMORY_ERROR);
		}
// An array of to remember the partials order, for 2nd run
		if((dz->iparray[(partialscnt*3)+1] = (int*)malloc(partialscnt * sizeof(int)))==NULL) {			//	address = (partialscnt * 3)+1
			sprintf(errstr,"INSUFFICIENT MEMORY for partial on-off gain and step times vals, array %d.\n",n);
			return(MEMORY_ERROR);
		}
		for(n=0;n<partialscnt;n++)	//	Store original order
			dz->iparray[(partialscnt*3)+1][n] = n;

// An array of to remember the switchpos
		if((dz->iparray[(partialscnt*3)+2] = (int*)malloc((*maxsteps) * sizeof(int)))==NULL) {			//	address = (partialscnt * 3)+2
			sprintf(errstr,"INSUFFICIENT MEMORY for switching between channels %d.\n",n);
			return(MEMORY_ERROR);
		}

// A permutation array for randomly permuting partials
		if((*perm = (int *)malloc(partialscnt*sizeof(int)))==NULL) {
			sprintf(errstr,"NO MEMORY FOR PARTIALS PERMUTATIONS\n");
			return(DATA_ERROR);
		}
		if((*permon = (int *)malloc(partialscnt*sizeof(int)))==NULL) {
			sprintf(errstr,"NO MEMORY FOR PARTIALS PERMUTATIONS\n");
			return(DATA_ERROR);
		}
		if((*permoff = (int *)malloc(partialscnt*sizeof(int)))==NULL) {
			sprintf(errstr,"NO MEMORY FOR PARTIALS PERMUTATIONS\n");
			return(DATA_ERROR);
		}
		configno = chans;
		configno += (chans * ((chans/2) - 1)) + chans/2;
		configno += chans * 2;
		configno += 3;
		if((*superperm = (int *)malloc(configno*sizeof(int)))==NULL) {
			sprintf(errstr,"NO MEMORY FOR PARTIALS PERMUTATIONS\n");
			return(DATA_ERROR);
		}
	} else {
		if((dz->parray[pntarray] = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for sine table.\n");
			return(MEMORY_ERROR);
		}
	}
	for(n=0;n<dz->itemcnt;n++)						//	Zero sine-table pointers for all partials
		dz->parray[pntarray][n] = 0.0;
	if(dz->mode == 1) {
		if((exit_status = generate_packet_envelope(dz))<0)
			return(exit_status);
	} else if(dz->mode == 2) {
		if(dz->param[SYNTH_RISE] + dz->param[SYNTH_FALL] + dz->param[SYNTH_STDY] > dz->param[SYNTH_DUR]) {
			sprintf(errstr,"Rise, Fall and Steady-state parameters not compatible with output duration.\n");
			return(DATA_ERROR);
		}
		dz->param[SYNTH_STDY] = dz->param[SYNTH_DUR] - dz->param[SYNTH_STDY];
		dz->param[SYNTH_FALL] = dz->param[SYNTH_STDY] - dz->param[SYNTH_FALL];


		if(dz->param[SYNTH_ETIME] + dz->param[SYNTH_CTIME] >= dz->param[SYNTH_DUR]) {
			sprintf(errstr,"Emerge and Converge times, combined, must be LESS than Output duration.\n");
			return(DATA_ERROR);
		}
		if(dz->iparam[SYNTH_NUM] > partialscnt) {
			sprintf(errstr,"Number of partials in play must be <= total number of partials and their transpositions (%d)\n",partialscnt);
			return(DATA_ERROR);
		}
		if(dz->iparam[SYNTH_EFROM] > dz->iparam[SYNTH_CHANS] || dz->iparam[SYNTH_CTO] > dz->iparam[SYNTH_CHANS]) {
			sprintf(errstr,"Channel to emerge from or converge to must be <= output channel count.\n");
			return(DATA_ERROR);
		}
		if(dz->iparam[SYNTH_EFROM] > 0 && dz->param[SYNTH_ETIME] == 0.0) {
			fprintf(stdout,"WARNING: Emergence time set to zero: Ignoring emergence channel.\n");
			fflush(stdout);
			dz->iparam[SYNTH_EFROM] = 0;
		}
		if(dz->iparam[SYNTH_EFROM] == 0 && dz->param[SYNTH_ETIME] > 0.0) {
			fprintf(stdout,"WARNING: Emergence channel not set: Ignoring emergence duration.\n");
			fflush(stdout);
			dz->param[SYNTH_ETIME] = 0.0;
		}
		if(dz->iparam[SYNTH_CTO] > 0 && dz->param[SYNTH_CTIME] == 0.0) {
			fprintf(stdout,"WARNING: Convergence time set to zero: Ignoring convergence channel.\n");
			fflush(stdout);
			dz->iparam[SYNTH_CTO] = 0;
		}
		if(dz->iparam[SYNTH_CTO] == 0 && dz->param[SYNTH_CTIME] > 0.0) {
			fprintf(stdout,"WARNING: Convergence channel not set: Ignoring convergence duration.\n");
			fflush(stdout);
			dz->param[SYNTH_ETIME] = 0.0;
		}
		if(!dz->vflag[SYN_SPACED] && (dz->iparam[SYNTH_CTO] > 0 || dz->iparam[SYNTH_EFROM] > 0)) {
			fprintf(stdout,"WARNING: Spatialisation flag not set: ignoring emerge/converge parameters.\n");
			fflush(stdout);
			dz->iparam[SYNTH_CTO] = 0;
			dz->iparam[SYNTH_EFROM] = 0;
			dz->param[SYNTH_ETIME] = 0.0;
			dz->param[SYNTH_CTIME] = 0.0;
		}
		dz->param[SYNTH_CTIME] = dz->param[SYNTH_DUR] - dz->param[SYNTH_CTIME];
		if(dz->vflag[SYN_JUMP] && !dz->vflag[SYN_SPACED]) {
			fprintf(stdout,"WARNING: Spatialisation flag not set: ignoring Jump flag.\n");
			fflush(stdout);
			dz->vflag[SYN_JUMP] = 0;
		}
		if(dz->iparam[SYNTH_STYPE] < 0) {
			if(chans != 8) {
				sprintf(errstr,"Special Spatialisation types Only available for 8-channel output.\n");
				return(DATA_ERROR);
			}
			if(dz->iparam[SYNTH_EFROM] || dz->iparam[SYNTH_CTO]) {
				fprintf(stdout,"WARNING: Emergence/convergence  not available with Special Spatialisation types.\n");
				fflush(stdout);
				dz->iparam[SYNTH_CTO] = 0;
				dz->iparam[SYNTH_EFROM] = 0;
				dz->param[SYNTH_ETIME] = 0.0;
				dz->param[SYNTH_CTIME] = 0.0;
				
			}
			if(dz->vflag[SYN_JUMP]) {
				sprintf(errstr,"Special Spatialisation types incompatible with Jump flag. Choose one or the other.\n");
				return(DATA_ERROR);
			}
			if(dz->iparam[SYNTH_STYPE] == SB_ROTATE && dz->param[SYNTH_RSPEED] == 0.0) {
				sprintf(errstr,"No rotation speed given for Special Spatialisation type %d, \"Rotation\".\n",SB_ROTATE);
				return(DATA_ERROR);
			}
			if(dz->iparam[SYNTH_STYPE] != SB_ROTATE && dz->param[SYNTH_RSPEED] > 0.0) {
				sprintf(errstr,"Special Spatialisation type %d, \"Rotation\" not set: Ignoring Rotation Speed.\n",SB_ROTATE);
				fflush(stdout);
			}
			dz->vflag[SYN_SPACED] = 1;
		}
	}
	return(FINISHED);
}

/******************************** SYNTHESIS ********************************
 *
 * MODE 2 arrays
 * pcnt = partialcnt mpcnt = maxpartial cnt (partials + all transpositions) | | |
 *																		  |	current
 *																		  | frqs|
 *	parray	|----------|----------|-|-|-----------------|-----------------|-|-|-|
 *          | tvarying pno+plevel |s|s|	  left_level	| right-level	  step|p|
 *			|	(Mpcnt*2)         |i|i|	      mpcnt		|	mpcnt         timeso|
 *			|					  |n|n|					|				  | | |s|
 *	address	0			   mpcnt*2| |p|(mpcnt*2)+2		|(mpcnt*3)+2	  | (mpcnt*4)+3
 *			|					  |	|t|					|				  (mpcnt*4)+2
 *			|					  |	|r|					|				  | | (mpcnt*4)+4
 *	lengths |  linelen of srcdata | | | maxsteps		|	maxsteps	  | |m|	|
 *								  |s|t|									  |t|p|t|
 *	(slen = sintablen)			  |l|o|									  |o|c|o|
 *	(totl = estimate of no		  |e|t|									  |t|n|t|
 *		    of timesteps used)	  |n|l|									  |l|t|l|
 *
 * iparray	|-----------------|-----------------|-----------------|-| switchpos
 *			| on-off flags	  | leftmost chan	|		spo		  |s|porder
 *			|	(mpcnt)		  |	   (mpcnt)		|	  (mpcnt)	  |p| | (mpcnt*3)+2
 *			|				  |					|				  |l| (mpcnt*3)+1 
 *  address	0				mpcnt			  mpcnt*2			  (mpcnt*3)
 *			|				  |					|				  |c| |
 *	lengths |	maxsteps	  |  maxsteps		|	maxsteps	  |nmpcnt
 *																  |t| mpcnt
 *																  |r| |
 *	(splcntrs = splice counters)								  |s| |
 *	(spo = orig values of splice counters)		
 */

int synthesis(int *perm,int *permon,int *permoff,int *superperm, double minrate,int maxsteps,dataptr dz)
{
	int exit_status, n, chans, rangxs = 0, max_partials_cnt, partials_in_play, rmost, k, terminate = 0, israngechange = 0;
	int loindex, hiindex, packet_dur, kk, stepcnt = 0, totaloutsamps, base_sampcnt;
	double loval, hival, valdiff, timefrac, val, valr = 0.0, vall = 0.0, level, maxval = 1.0, onehzincr, packet_incr, envv, pos = 0.0;
	float *obuf = dz->sampbuf[0];
	double srate = (double)dz->infile->srate, thisstep, xsrange_frac = 0.0, thisrangxs, rangetop, posstep = 0.0;
	int partialcnt = dz->itemcnt, total_partialcnt = 0, sintable = partialcnt * 2, splen = 0, spacetyp = dz->iparam[SYNTH_STYPE];
	double *sintab = dz->parray[sintable], *sinptr = dz->parray[sintable+1];
	double **llev = NULL, **rlev = NULL;
	double *steptimes = NULL, *pvals = NULL, *position = NULL;
	int **onoff = NULL, **lmost = NULL, **origspl = NULL, *splcntr = NULL, *splordr = NULL, *swpos = NULL;
	int inendsplice, instartsplice, total_samps_synthed = 0, jlmost = 0, switchpos = 0;
	int configcnt = 0, configno = 0, l_most = 0, r_most = 0, special_onoff = 0, indownsplice = 0;
	int sampcnt = 0, startspliceend = dz->rampbrksize, endsplicestart = dz->iparam[1] - dz->rampbrksize;
	double time = 0.0, spliceincr, spliceval, localspliceval, normaliser, falldur = 0.0, enddur = 0.0, nexttime = -1.0, leftgain = 0.0, rightgain = 0.0;

	onehzincr = (double)SYNTH_TABSIZE/srate;
	spliceincr = 1.0/(double)dz->rampbrksize;
	spliceval = 0.0;
	instartsplice = 1;
	inendsplice = 0;
	totaloutsamps = dz->iparam[1];
	if(dz->mode == 2) {
		chans = dz->iparam[SYNTH_CHANS];
		if(spacetyp > 0) {
			switch(spacetyp) {
			case(SB_SUPERSPACE4):				//	Square, diamond and All-at-once
				configno = 3;					//	For 8 chan = 3 + (8*2) + [((8*(4-1))+4] + 8 = 3 + 16 + 28 + 8 = 55
				// fall thro
			case(SB_SUPERSPACE3):				//	all possible meaningful small and large triangles
				configno += chans * 2;
				// fall thro
			case(SB_SUPERSPACE2):				//	all possible pairs
				configno += (chans * ((chans/2) - 1)) + chans/2;
				// fall thro
			case(SB_SUPERSPACE):				//	all single chans
				configno += chans;
				break;
			}
		}
		totaloutsamps *= chans;
		endsplicestart = dz->iparam[1] - (int)floor(50 * MS_TO_SECS * srate);	//	Force long splice at end
		endsplicestart *= chans;
		startspliceend = (int)floor(50 * MS_TO_SECS * srate);					//	Force long splice at start
		startspliceend *= chans;
		instartsplice = 1;
		inendsplice = 0;
		splen = (int)round(dz->param[SYNTH_SPLEN] * MS_TO_SECS * srate);
		total_partialcnt = partialcnt;
		llev = dz->parray + (partialcnt * 2) + 2;
		rlev = dz->parray + (partialcnt * 3) + 2;
		steptimes = dz->parray[(partialcnt * 4) + 2];
		pvals     = dz->parray[(partialcnt * 4) + 3];
		position  = dz->parray[(partialcnt * 4) + 4];
		onoff   = dz->iparray;
		lmost   = dz->iparray + partialcnt;
		origspl = dz->iparray + (partialcnt * 2);
		splcntr = dz->iparray[partialcnt * 3];
		splordr = dz->iparray[(partialcnt * 3) + 1];
		swpos   = dz->iparray[(partialcnt * 3) + 2];
		falldur = dz->param[SYNTH_STDY] - dz->param[SYNTH_FALL];
		enddur  = dz->param[SYNTH_DUR] - dz->param[SYNTH_STDY];
		rangxs = dz->iparam[SYNTH_MAX] - 1;		//	Max no partial-transpositions (apart from orig vals)
		for(n=0;n < partialcnt;n++) {
			onoff[n][0] = S_OFF;//  all partials initially flagged off
			lmost[n][0] = 0;	//  all leftmost-outchan initially set to left - SAFETY
			origspl[n][0] = 0;	//	all original-settings of splice-counters to zero
			splcntr[n] = 0;		//  all splicecounters initially set to zero - SAFETY
			llev[n][0] = 0.0;	//	all partial gains initially set to zero - SAFETY
			rlev[n][0] = 0.0;
		}
		nexttime = 0.0;			//	initialise "nexttime" to trigger 1st setting of partials
		steptimes[0] = nexttime;//	initial steptime set to zero
		stepcnt = 0;
	} else {
		chans = 1;
	}
	fprintf(stdout,"INFO: First pass: assessing level.\n");
	fflush(stdout);
	memset((char *)obuf,0,dz->buflen * sizeof(float));
	while(total_samps_synthed < totaloutsamps) {
		time = (double)(total_samps_synthed/chans)/srate;
		if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
			return exit_status;
		switch(dz->mode) {
		case(0):
			for(n=0;n<dz->itemcnt;n++) {
				loindex = (int)floor(sinptr[n]);	//	Read from sintable, using partial-increment
				hiindex = loindex + 1;
				loval   = sintab[loindex];
				hival   = sintab[hiindex];
				valdiff = hival - loval;
				timefrac = sinptr[n] - (double)loindex;
				val = loval + (valdiff * timefrac);
				level = read_level(n,time,dz);		//	Read corresponding level
				val *= level;
				obuf[sampcnt] = (float)(obuf[sampcnt] + val);
				incr_sinptr(n,time,onehzincr,dz);	//	Track (modify if ness) the partial-incr value for this partial
			}
			if(instartsplice) {
				obuf[sampcnt] = (float)(obuf[sampcnt] * spliceval);
				spliceval += spliceincr;
				spliceval = min(spliceval,1.0);
			} else if(inendsplice) {
				obuf[sampcnt] = (float)(obuf[sampcnt] * spliceval);
				spliceval -= spliceincr;
				spliceval = max(spliceval,0.0);
			}
			maxval = max(maxval,fabs(obuf[sampcnt]));
			if(++sampcnt >= dz->buflen) {
				memset((char *)obuf,0,dz->buflen * sizeof(float));
				sampcnt = 0;
			}
			total_samps_synthed++;
			if(!inendsplice && (total_samps_synthed >= endsplicestart)) {
				inendsplice = 1;
				spliceval = 1.0;
			} 
			if(instartsplice && (total_samps_synthed >= startspliceend))
				instartsplice = 0;
			break;
		case(1):
			if(dz->brksize[SYNTH_SQZ] || dz->brksize[SYNTH_CTR]) {
				if(!(flteq(dz->param[SYNTH_SQZ],1.0)) || !(flteq(dz->param[SYNTH_CTR],0.0)))
					modify_packet_envelope(dz);
			}
			packet_dur  = (int)round((1.0/dz->param[SYNTH_FRQ]) * srate);
			packet_incr = (double)TREMOLO_TABSIZE/(double)(packet_dur - 1);	//	Forces last read to be at end of packet envelope (zero)
			for(n=0;n<dz->itemcnt;n++)
				sinptr[n] = 0.0;
			for(kk = 0; kk<packet_dur;kk++) {
				for(n=0;n<dz->itemcnt;n++) {
					if(!dz->vflag[0])
						time = (double)(total_samps_synthed + n)/srate;
					loindex = (int)floor(sinptr[n]);
					hiindex = loindex + 1;
					loval   = sintab[loindex];
					hival   = sintab[hiindex];
					valdiff = hival - loval;
					timefrac = sinptr[n] - (double)loindex;
					val = loval + (valdiff * timefrac);
					level = read_level(n,time,dz);
					val *= level;
					obuf[sampcnt] = (float)(obuf[sampcnt] + val);
					incr_sinptr(n,time,onehzincr,dz);
				}
				envv = read_packet_envelope(kk,packet_incr,dz);
				obuf[sampcnt] = (float)(obuf[sampcnt] * envv);
				maxval = max(maxval,fabs(obuf[sampcnt]));
				if(++sampcnt >= dz->buflen) {
					sampcnt = 0;
					memset((char *)obuf,0,dz->buflen * sizeof(float));
				}
				total_samps_synthed++;
			}
			break;
		case(2):
			if(time >= steptimes[stepcnt]) {					//	If we've reached the next partials-change time
				if(sloom && ((stepcnt % 200) == 0)) {
					fprintf(stdout,"INFO: at %.1lf secs\n",time);
					fflush(stdout);
				}
				if(spacetyp > 0) {
					if(configcnt == 0)
						rndintperm(superperm,configno);
					if(++configcnt >= configno)
						configcnt = 0;
				}
				
				thisstep = (drand48() * 2.0) - 1.0;				//	-1 to 1
				thisstep *= dz->param[SYNTH_RATE]/2.0;			// -(1/2) rate to +(1/2) rate
				thisstep += dz->param[SYNTH_RATE];				// (1/2) rate to 1+(1/2) rate
				nexttime = time + thisstep;
				stepcnt++;
				if(spacetyp == SB_ROTATE)
					posstep = thisstep * dz->param[SYNTH_RSPEED] * chans;
				if(stepcnt >= maxsteps - 1) {					//	If we run out of memory (as steps have random length) despite safety margin
					terminate = 1;								//	Force all partials to turn off, and terminate at end of fade
					totaloutsamps = total_samps_synthed + (splen * chans);
				}
				steptimes[stepcnt] = nexttime;
																//	Find current value of all partials, + sort to ascending order
				get_current_partial_vals(time,pvals,total_partialcnt,dz);
				sort_partials_into_ascending_frq_order(total_partialcnt,pvals,sinptr,llev,rlev,onoff,lmost,origspl,splordr,dz);

				// FIND THE RANGE OF PARTIALS WHICH CAN BE USED

				if(terminate) {	//	TURN EVERYTHING OFF!!
					for(n=0;n<total_partialcnt;n++) {
						onoff[n][stepcnt] = S_OFF;
						if(onoff[n][stepcnt-1] == S_ON) {
							origspl[n][stepcnt] = splen;	//	Partial is switched off
							splcntr[n] = splen;				//	Set up dnsplice, retaining previous level
							llev[n][stepcnt] = llev[n][stepcnt-1];
							lmost[n][stepcnt] = lmost[n][stepcnt-1];
							if(dz->vflag[SYN_SPACED])		//	Retain previous level(s)
								rlev[n][stepcnt] = rlev[n][stepcnt-1];
							else
								rlev[n][stepcnt] = 0.0;
						} else if(onoff[n][stepcnt-1] == S_OFF) {
							origspl[n][stepcnt] = 0;		//	Partial already OFF
							splcntr[n] = 0;					//	SAFETY
							lmost[n][stepcnt] = lmost[n][stepcnt-1];
						}
					}
				} else {
					if(dz->iparam[SYNTH_MAX] > 1) {					//	Find which partials can be used at this time....
						israngechange = 0;
						if(time < dz->param[SYNTH_RISE]) {			//	If we're in the initial fade-up or final fade-down
							xsrange_frac = (dz->param[SYNTH_RISE] - time)/dz->param[SYNTH_RISE];	//	Frac Range 1-0
							israngechange = 1;
						} else if(time > dz->param[SYNTH_FALL] && time < dz->param[SYNTH_STDY]) {	//	Calc the active octave-range at this point
							xsrange_frac = (time - dz->param[SYNTH_FALL])/falldur;					//	Frac Range 0-1
							israngechange = 2;
						} else if (time >= dz->param[SYNTH_STDY]) {
							if(dz->vflag[SYN_TOROOT])				//	If we're in final steady-state or final fade
								xsrange_frac = (time - dz->param[SYNTH_STDY])/enddur;				//	Frac Range 0-1
							else
								xsrange_frac = 1.0;													//	Flat
							israngechange = 3;
						}
						if(israngechange) {
							xsrange_frac = 1.0 - xsrange_frac;										//	Frac Range 1-0
							if((israngechange == 1) && dz->vflag[SYN_FROMROOT]) {
								thisrangxs = dz->iparam[SYNTH_MAX] * xsrange_frac;	//	Expand from root to max-range
								rangetop   = thisrangxs * dz->scalefact;
							} else if((israngechange == 3) && dz->vflag[SYN_TOROOT]) {
								thisrangxs = xsrange_frac;							//	Contract from min-range to root
								rangetop   = thisrangxs * dz->scalefact;
							} else {
								thisrangxs = rangxs * xsrange_frac;					//	Contract xs-range
								rangetop   = (1.0 + thisrangxs) * dz->scalefact;	//	Find current top of range
							}														//	Multiplying by original range of partials (scalefact)
							for(n = 0;n<total_partialcnt;n++) {
								if(pvals[n] >= rangetop)			//	Find max partial we can use
									break;
							}
							max_partials_cnt = n+1;					//	Max range of partials-and-transpositions we might use
							max_partials_cnt = min(max_partials_cnt,total_partialcnt);	//	FAILSAFE
						} else										//	Otherwise, use maximum octave range
							max_partials_cnt = total_partialcnt;
					} else											
						max_partials_cnt = total_partialcnt;			
																	//	P-and-ts we'll actually use at this moment (random)
					if(dz->iparam[SYNTH_NUM] > 0)
						partials_in_play = min(dz->iparam[SYNTH_NUM],max_partials_cnt);
					else
						partials_in_play = (int)floor(drand48() * (double)max_partials_cnt) + 1;
					
					//	If Jump flag set, do spatialisation for ALL partials FIRST
					
					special_onoff = 0;
					if(dz->vflag[SYN_SPACED]) {
						if(dz->vflag[SYN_JUMP]) {				
							if(dz->iparam[SYNTH_EFROM] && (time < dz->param[SYNTH_ETIME]))
								pos = emergepos(dz->iparam[SYNTH_EFROM],chans,time,dz->param[SYNTH_ETIME]);
							else if(dz->iparam[SYNTH_CTO] && (time > dz->param[SYNTH_CTIME]))
								pos = convergepos(dz->iparam[SYNTH_CTO],chans,time,dz->param[SYNTH_CTIME],dz->param[SYNTH_DUR]);
							else
								pos = chans * drand48();
							jlmost = (int)floor(pos);
							pos -= (double)jlmost;
							pos = (pos * 2.0) - 1.0;
							pancalc(pos,&leftgain,&rightgain);
						} else if(spacetyp > 0) {
							spacebox(&pos,&switchpos,posstep,chans,spacetyp,configno,configcnt,superperm);
							position[stepcnt] = pos;
							swpos[stepcnt] = switchpos;
							if((position[stepcnt] != position[stepcnt-1]) || (swpos[stepcnt] != swpos[stepcnt-1]))
								special_onoff = 1;		//	Where partial changes position, will need to fade-out then refade-in
						}
					}
					// Randomly-> CHOOSE PARTIALS ON or OFF, ESTABLISH RELATIVE LEVEL, SET SPATIAL POSITION (if flagged)

					if(partials_in_play == max_partials_cnt) {		//	If partials fill available range
						for(n=0;n<partials_in_play;n++) {			//	All partials in range are on
							onoff[n][stepcnt] = S_ON;
							if(onoff[n][stepcnt-1] == S_OFF) {		//	If previously off
								origspl[n][stepcnt] = splen;		//	Mark as fade-up
								splcntr[n] = splen;					//	Set splice-counter to count back down to zero
								llev[n][stepcnt] = (drand48() * 0.5) + 0.5;	//	Set new (rand)level [llev stands in for mono level]
								if(dz->vflag[SYN_SPACED] && (spacetyp == 0)) {		//	If SPATIALISED 
									
									if(dz->vflag[SYN_JUMP]) {		//	If Jump flag in use, leftmost chan and levels already set
										lmost[n][stepcnt] = jlmost;
										rlev[n][stepcnt]  = llev[n][stepcnt] * rightgain;
										llev[n][stepcnt] *= leftgain;
									} else {						//	Else create position for each individual partial
										if(dz->iparam[SYNTH_EFROM] && (time < dz->param[SYNTH_ETIME]))
											pos = emergepos(dz->iparam[SYNTH_EFROM],chans,time,dz->param[SYNTH_ETIME]);
										else if(dz->iparam[SYNTH_CTO] && (time > dz->param[SYNTH_CTIME]))
											pos = convergepos(dz->iparam[SYNTH_CTO],chans,time,dz->param[SYNTH_CTIME],dz->param[SYNTH_DUR]);
										else
											pos = chans * drand48();//	Create spatial position at random	(range 0 - chans)
										lmost[n][stepcnt] = (int)floor(pos);		//	Find leftmost lspkr
										pos -= (double)lmost[n][stepcnt];			//	Range 0-1
										pos = (pos * 2.0) - 1.0;					//	Range (-1 to 1)
										pancalc(pos,&leftgain,&rightgain);			//	Calc relative levels of left and right signals
										rlev[n][stepcnt] = llev[n][stepcnt] * rightgain;
										llev[n][stepcnt] *= leftgain;				//	Readjust output levels
									}
								} else {
									lmost[n][stepcnt] = lmost[n][stepcnt-1];
									rlev[n][stepcnt] = 0.0;
								}
							} else {								//	Else, already on
								llev[n][stepcnt] = llev[n][stepcnt-1];//	Retain previous level(s)
								lmost[n][stepcnt] = lmost[n][stepcnt-1];
								if(dz->vflag[SYN_SPACED])
									rlev[n][stepcnt] = rlev[n][stepcnt-1];
								else
									rlev[n][stepcnt] = 0.0;
								if(special_onoff) {
									splcntr[n] = splen * 2;
									origspl[n][stepcnt] = splen * 2;
								} else {
									origspl[n][stepcnt] = 0;
									splcntr[n] = 0;					//	SAFETY
								}
							}
						}
						while(n < total_partialcnt) {				//	For all remaining (unused) partials
							onoff[n][stepcnt] = S_OFF;
							if(onoff[n][stepcnt-1] == S_ON) {		//	If partial was on
								origspl[n][stepcnt] = splen;		//	Mark it as fading out
								splcntr[n] = splen;					//	Set splice-counter to count down to zero
								llev[n][stepcnt] = llev[n][stepcnt-1];
								lmost[n][stepcnt] = lmost[n][stepcnt-1];
								if(dz->vflag[SYN_SPACED])			//	retaining previous level(s)
									rlev[n][stepcnt] = rlev[n][stepcnt-1];
								else
									rlev[n][stepcnt] = 0.0;
							} else {								//	Else it was previously off
								origspl[n][stepcnt] = 0;
								splcntr[n] = 0;						//	SAFETY
								origspl[n][stepcnt] = 0;
								lmost[n][stepcnt] = lmost[n][stepcnt-1];
							}
							n++;
						}
					} else {
						rndintperm(perm,max_partials_cnt);			//	Randomly permute all possible partials
						if(dz->vflag[SYN_X])						//	If Exclusuve, Force currently OFF-partials to top of list
							xclusive(perm,permon,permoff,max_partials_cnt,partials_in_play,onoff,stepcnt);
						for(n=0;n<partials_in_play;n++)				//	Switch first p_in_p partials in perm, ON
							onoff[perm[n]][stepcnt] = S_ON;	
						while(n < max_partials_cnt) {				//	and switch remainder of those in range off
							onoff[perm[n]][stepcnt] = S_OFF;
							n++;
						}
						while(n < total_partialcnt) {				//	and switch remainder off
							onoff[n][stepcnt] = S_OFF;
							n++;
						}

						// ALGO ASSUMES THAT, BY THE TIME WE REACH NEXT STEP, splice has ended

						for(n=0;n<total_partialcnt;n++) {			//	Switch first p_in_p partials in perm, ON
							if(onoff[n][stepcnt] == S_ON) {
								if(onoff[n][stepcnt-1] == S_ON) {	//	Partial remains on
									llev[n][stepcnt] = llev[n][stepcnt-1];
									lmost[n][stepcnt] = lmost[n][stepcnt-1];
									if(dz->vflag[SYN_SPACED])		//	Retain previous level(s)
										rlev[n][stepcnt] = rlev[n][stepcnt-1];
									else
										rlev[n][stepcnt] = 0.0;
									if(special_onoff) {
										origspl[n][stepcnt] = splen * 2;
										splcntr[n] = splen * 2;
									} else {
										origspl[n][stepcnt] = 0;
										splcntr[n] = 0;					//	SAFETY
									}
																	
								} else if(onoff[n][stepcnt-1] == S_OFF) {
									origspl[n][stepcnt] = splen;
									splcntr[n] = splen;				//	Partial is switched on				
									llev[n][stepcnt] = (drand48() * 0.5) + 0.5;	//	Set new (rand)level
									if(dz->vflag[SYN_SPACED] && (spacetyp == 0)) {		//	If SPATIALISED...etc
										if(dz->vflag[SYN_JUMP]) {	//	If Jump flag in use, leftmost chan and levels already set
											lmost[n][stepcnt] = jlmost;
											rlev[n][stepcnt]  = llev[n][stepcnt] * rightgain;
											llev[n][stepcnt] *= leftgain;
										} else {					//	Else create position for each individual partial
											if(dz->iparam[SYNTH_EFROM] && (time < dz->param[SYNTH_ETIME]))
												pos = emergepos(dz->iparam[SYNTH_EFROM],chans,time,dz->param[SYNTH_ETIME]);
											else if(dz->iparam[SYNTH_CTO] && (time > dz->param[SYNTH_CTIME]))
												pos = convergepos(dz->iparam[SYNTH_CTO],chans,time,dz->param[SYNTH_CTIME],dz->param[SYNTH_DUR]);
											else
												pos = chans * drand48();
											lmost[n][stepcnt] = (int)floor(pos);
											pos -= (double)lmost[n][stepcnt];
											pos = (pos * 2.0) - 1.0;
											pancalc(pos,&leftgain,&rightgain);
											rlev[n][stepcnt] = llev[n][stepcnt] * rightgain;
											llev[n][stepcnt] *= leftgain;
										}
									} else {
										rlev[n][stepcnt] = 0.0;
										lmost[n][stepcnt] = lmost[n][stepcnt-1];
									}
								}
							} else { //	Marked as OFF
								if(onoff[n][stepcnt-1] == S_ON) {
									origspl[n][stepcnt] = splen;	//	Partial is switched off
									splcntr[n] = splen;				//	Set up dnsplice, retaining previous level
									llev[n][stepcnt] = llev[n][stepcnt-1];
									lmost[n][stepcnt] = lmost[n][stepcnt-1];
									if(dz->vflag[SYN_SPACED])		//	Retain previous level(s)
										rlev[n][stepcnt] = rlev[n][stepcnt-1];
									else
										rlev[n][stepcnt] = 0.0;
								} else if(onoff[n][stepcnt-1] == S_OFF) {
									origspl[n][stepcnt] = 0;		//	Partial already OFF
									splcntr[n] = 0;					//	SAFETY
									lmost[n][stepcnt] = lmost[n][stepcnt-1];
								}
							}
						}
					}
				}
			}
			//	USING THE ON/OFF, RELATIVE LEVEL, SPLICING, AND SPATIALISATION INFO, WRITE VARIOUS PARTIALS

			base_sampcnt = sampcnt;
			for(n=0;n<total_partialcnt;n++) {
				sampcnt = base_sampcnt;
				if(onoff[n][stepcnt]) {					//	If partial is NOW on
					loindex = (int)floor(sinptr[n]);	//	Read from sintable, using partial-increment
					hiindex = loindex + 1;
					loval   = sintab[loindex];
					hival   = sintab[hiindex];
					valdiff = hival - loval;
					timefrac = sinptr[n] - (double)loindex;
					val = loval + (valdiff * timefrac);
					level = read_level(n,time,dz);		//	Read corresponding level
					indownsplice = 0;
					if(splcntr[n] > 0) {				//	Get any splice contribution
						if(splcntr[n] > splen) {		//	This indicates an OFF/ON splice
							localspliceval = (double)(splcntr[n] - splen)/(double)splen;
							indownsplice = 1;			//	Down-splice
						} else {
							indownsplice = 0;			//	Up-splice
							localspliceval = (double)(splen - splcntr[n])/(double)splen;
						}
						val *= localspliceval;			//	Upfade, splcntr falling, splen-splcntr rising 
						splcntr[n]--;					//	Advance splicecnt towards zero
					}
					if(dz->vflag[SYN_SPACED]) {			//	If spatialisation, get spatial contributions
						if(spacetyp > 0) {
							if(indownsplice) {
								pos = position[stepcnt-1];
								switchpos = swpos[stepcnt-1];
								spacebox_apply(pos,llev[n][stepcnt-1],chans,&l_most,&r_most,&valr,&vall,spacetyp);
							} else {
								pos = position[stepcnt];
								switchpos = swpos[stepcnt];
								spacebox_apply(pos,llev[n][stepcnt],chans,&l_most,&r_most,&valr,&vall,spacetyp);
							}
							valr = val * valr;
							val  = val * vall;
						} else {						//	If spatialisation, get spatial contributions
							valr = val * rlev[n][stepcnt];
							val *= llev[n][stepcnt];
						}
					} else
						val *= llev[n][stepcnt];		//	Or just incorporate calculated atten for this element
					if(dz->vflag[SYN_SPACED]) {			//	if spatialised, find rightmost channel from leftmost
						if(spacetyp > 0) {	
							output_special_spatialisation_sample(obuf,sampcnt,switchpos,chans,val,valr,l_most,r_most,spacetyp);
							sampcnt += chans;
						} else {
							rmost = (lmost[n][stepcnt] + 1) % chans;
							for(k = 0;k< chans;k++) {
								if(k == lmost[n][stepcnt])	//	Add output only to the 2 relevant channels
									obuf[sampcnt] = (float)(obuf[sampcnt] + val);
								else if(k == rmost)
									obuf[sampcnt] = (float)(obuf[sampcnt] + valr);
								sampcnt++;
							}
						}
					} else {							//	If NOT spatialised, add output to all outchans
						for(k = 0;k < chans;k++) {
							obuf[sampcnt] = (float)(obuf[sampcnt] + val);
							sampcnt++;
						}
					}

				} else {								//	Partial is OFF
					
					if(splcntr[n] > 0) {				//	BUT IF its still a fade-out, Get any splice contribution
						loindex = (int)floor(sinptr[n]);
						hiindex = loindex + 1;
						loval   = sintab[loindex];
						hival   = sintab[hiindex];
						valdiff = hival - loval;
						timefrac = sinptr[n] - (double)loindex;
						val = loval + (valdiff * timefrac);
						level = read_level(n,time,dz);
						localspliceval = (double)splcntr[n]/(double)splen;	//	Downfade, splcntr falling
						val *= localspliceval;
						splcntr[n]--;					//	Advance splicecnt towards zero
						if(dz->vflag[SYN_SPACED]) {
							if(spacetyp > 0) {
								pos = position[stepcnt-1];
								switchpos = swpos[stepcnt-1];
								spacebox_apply(pos,llev[n][stepcnt-1],chans,&l_most,&r_most,&valr,&vall,spacetyp);
								valr = val * valr;
								val  = val * vall;
							} else {
								valr = val * rlev[n][stepcnt];
								val *= llev[n][stepcnt];
							}
						} else
							val *= llev[n][stepcnt];
						if(dz->vflag[SYN_SPACED]) {
							if(spacetyp > 0) {	
								output_special_spatialisation_sample(obuf,sampcnt,switchpos,chans,val,valr,l_most,r_most,spacetyp);
								sampcnt += chans;
							} else {
								rmost = (lmost[n][stepcnt] + 1) % chans;
								for(k = 0;k < chans;k++) {
									if(k == lmost[n][stepcnt])
										obuf[sampcnt] = (float)(obuf[sampcnt] + val);
									else if(k == rmost)
										obuf[sampcnt] = (float)(obuf[sampcnt] + valr);
									sampcnt++;
								}
							}
						} else {
							for(k = 0;k < chans;k++) {
								obuf[sampcnt] = (float)(obuf[sampcnt] + val);
								sampcnt++;
							}
						}
					}
				}
				incr_sinptr(n,time,onehzincr,dz);
			}
			if(instartsplice) {							//	Do big splice at start of output
				sampcnt = base_sampcnt;
				for(k = 0;k < chans;k++) {
					obuf[sampcnt] = (float)(obuf[sampcnt] * spliceval);
					sampcnt++;
				}
				spliceval += spliceincr;
				spliceval = min(spliceval,1.0);
			} else if(inendsplice) {					//	Do big splice at end of output
				sampcnt = base_sampcnt;
				for(k = 0;k < chans;k++) {
					obuf[sampcnt] = (float)(obuf[sampcnt] * spliceval);
					sampcnt++;
				}
				spliceval -= spliceincr;
				spliceval = max(spliceval,0.0);
			}
			sampcnt = base_sampcnt;						//	Find maxval over all channels
			for(k = 0;k< chans;k++) {
				maxval = max(maxval,fabs(obuf[sampcnt]));
				sampcnt++;
			}
			if(sampcnt >= dz->buflen) {					//	Check if buffer full - refresh
				memset((char *)obuf,0,dz->buflen * sizeof(float));
				sampcnt = 0;
			}
			total_samps_synthed += chans;				//	Find out if (still) in startsplice or endsplice
			if(!inendsplice && (total_samps_synthed >= endsplicestart)) {
				inendsplice = 1;
				spliceval = 1.0;
			} 
			if(instartsplice && (total_samps_synthed >= startspliceend))
				instartsplice = 0;
			break;
		}
	}
	if(sloom) {
		fprintf(stdout,"INFO: at %.1lf secs\n",time);
		fflush(stdout);
	}
	normaliser = 0.85/maxval;
	time = 0.0;
	spliceval = 0.0;
	instartsplice = 1;
	inendsplice = 0;
	total_samps_synthed = 0;
	sampcnt = 0;
	for(n=0;n<dz->itemcnt;n++)					//	Zero sine-table pointers for all partials
		sinptr[n] = 0.0;
	if(dz->mode == 2) {
		for(n=0;n < partialcnt;n++) {
			onoff[n][0] = S_OFF;//  all partials initially flagged off
			lmost[n][0] = 0;	//  all leftmost-outchan initially set to left - SAFETY
			origspl[n][0] = 0;	//	all original-settings of splice-counters to zero
			splcntr[n] = 0;		//  all splicecounters initially set to zero - SAFETY
			llev[n][0] = 0.0;	//	all partial gains initially set to zero - SAFETY
			rlev[n][0] = 0.0;
		}
		stepcnt = 0;
		terminate = 0;
		resort_partials_into_original_frq_order(total_partialcnt,pvals,sinptr,llev,rlev,onoff,lmost,origspl,splordr,dz);
	}
	fprintf(stdout,"INFO: Second pass: synthesis.\n");
	fflush(stdout);
	memset((char *)obuf,0,dz->buflen * sizeof(float));
	while(total_samps_synthed < totaloutsamps) {
		time = (double)(total_samps_synthed/chans)/srate;
		if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
			return exit_status;
		switch(dz->mode) {
		case(0):
			for(n=0;n<dz->itemcnt;n++) {
				loindex = (int)floor(sinptr[n]);	//	Read from sintable, using partial-increment
				hiindex = loindex + 1;
				loval   = sintab[loindex];
				hival   = sintab[hiindex];
				valdiff = hival - loval;
				timefrac = sinptr[n] - (double)loindex;
				val = loval + (valdiff * timefrac);
				level = read_level(n,time,dz);		//	Read corresponding level
				val *= level;
				obuf[sampcnt] = (float)(obuf[sampcnt] + val);
				incr_sinptr(n,time,onehzincr,dz);	//	Track (modify if ness) the partial-incr value for this partial
			}
			obuf[sampcnt] = (float)(obuf[sampcnt] * normaliser);
			if(instartsplice) {
				obuf[sampcnt] = (float)(obuf[sampcnt] * spliceval);
				spliceval += spliceincr;
				spliceval = min(spliceval,1.0);
			} else if(inendsplice) {
				obuf[sampcnt] = (float)(obuf[sampcnt] * spliceval);
				spliceval -= spliceincr;
				spliceval = max(spliceval,0.0);
			}
			total_samps_synthed++;
			if(!inendsplice && (total_samps_synthed >= endsplicestart)) {
				inendsplice = 1;
				spliceval = 1.0;
			} 
			if(instartsplice && (total_samps_synthed >= startspliceend))
				instartsplice = 0;
			if(++sampcnt >= dz->buflen) {
				if((exit_status = write_samps(obuf,sampcnt,dz))<0)
					return(exit_status);
				sampcnt = 0;
				memset((char *)obuf,0,dz->buflen * sizeof(float));
			}
			break;
		case(1):										//	At start of each packet, set up packet shape, size and increment
			if(!(flteq(dz->param[SYNTH_SQZ],1.0)) || !(flteq(dz->param[SYNTH_CTR],0.0)))
				modify_packet_envelope(dz);				//	Packet duration determined by fundamental frq
			packet_dur  = (int)round((1.0/dz->param[SYNTH_FRQ]) * srate);
			packet_incr = (double)TREMOLO_TABSIZE/(double)(packet_dur - 1);
			for(n=0;n<dz->itemcnt;n++)					//	Zero sine-table pointers for all partials, at start of packet
				sinptr[n] = 0.0;
			for(kk = 0; kk<packet_dur;kk++) {
				for(n=0;n<dz->itemcnt;n++) {			//	If not holding partial values steady WITHIN packets, 
					if(!dz->vflag[0])					//	use absolute time to update partial frqs and levels.
						time = (double)(total_samps_synthed + n)/srate;	//	Otherwise use packet_start-time,
					loindex = (int)floor(sinptr[n]);	//	Read from sintable, using partial-increment
					hiindex = loindex + 1;
					loval   = sintab[loindex];
					hival   = sintab[hiindex];
					valdiff = hival - loval;
					timefrac = sinptr[n] - (double)loindex;
					val = loval + (valdiff * timefrac);
					level = read_level(n,time,dz);		//	Read corresponding level
					val *= level;
					obuf[sampcnt] = (float)(obuf[sampcnt] + val);
					incr_sinptr(n,time,onehzincr,dz);	//	Track (modify if ness) the partial-incr value for this partial
				}
														//	Once all partial-samples added, impose packet envelope
				envv = read_packet_envelope(kk,packet_incr,dz);
				obuf[sampcnt] = (float)(obuf[sampcnt] * envv * normaliser);
				total_samps_synthed++;
				if(++sampcnt >= dz->buflen) {
					if((exit_status = write_samps(obuf,sampcnt,dz))<0)
						return(exit_status);
					sampcnt = 0;
					memset((char *)obuf,0,dz->buflen * sizeof(float));
				}
			}
			break;
		case(2):
			if(sloom)									//	Forces correct read-out of time-bar
				dz->insams[0] = dz->iparam[SYNTH_DUR] * chans;
			if(time >= steptimes[stepcnt]) {			//	If we've reached the next partials-change time
				stepcnt++;								//	Advance to next vals

				get_current_partial_vals(time,pvals,total_partialcnt,dz);
				sort_partials_into_ascending_frq_order(total_partialcnt,pvals,sinptr,llev,rlev,onoff,lmost,origspl,splordr,dz);

				for(n=0;n<total_partialcnt;n++)			//	Set any splice counters needed
					splcntr[n] = origspl[n][stepcnt];
			}
			base_sampcnt = sampcnt;
			for(n=0;n<total_partialcnt;n++) {
				sampcnt = base_sampcnt;
				if(onoff[n][stepcnt]) {					//	If partial is on
					loindex = (int)floor(sinptr[n]);	//	Read from sintable, using partial-increment
					hiindex = loindex + 1;
					loval   = sintab[loindex];
					hival   = sintab[hiindex];
					valdiff = hival - loval;
					timefrac = sinptr[n] - (double)loindex;
					val = loval + (valdiff * timefrac);
					level = read_level(n,time,dz);		//	Read corresponding level
					indownsplice = 0;
					if(splcntr[n] > 0) {				//	Get any splice contribution
						if(splcntr[n] > splen) {		//	This indicates an OFF/ON splice
							localspliceval = (double)(splcntr[n] - splen)/(double)splen;
							indownsplice = 1;			//	Down-splice
						} else {	
							localspliceval = (double)(splen - splcntr[n])/(double)splen;
							indownsplice = 0;			//	Up-splice
						}
						val *= localspliceval;			//	Upfade, splcntr falling, splen-splcntr rising 
						splcntr[n]--;					//	Advance splicecnt towards zero
					}
					if(dz->vflag[SYN_SPACED]) {			//	If spatialisation, get spatial contributions
						if(spacetyp > 0) {
							if(indownsplice) {
								pos = position[stepcnt-1];
								switchpos = swpos[stepcnt-1];
								spacebox_apply(pos,llev[n][stepcnt-1],chans,&l_most,&r_most,&valr,&vall,spacetyp);
							} else {
								pos = position[stepcnt];
								switchpos = swpos[stepcnt];
								spacebox_apply(pos,llev[n][stepcnt],chans,&l_most,&r_most,&valr,&vall,spacetyp);
							}
							valr = val * valr;
							val =  val * vall;
						} else {						//	If spatialisation, get spatial contributions
							valr = val * rlev[n][stepcnt];
							val  = val * llev[n][stepcnt];	
						}					
					} else
						val  = val * llev[n][stepcnt];	//	Or just incorporate calculated atten for this element
					if(dz->vflag[SYN_SPACED]) {			//	if spatialised, find rightmost channel from leftmost
						if(spacetyp > 0) {	
							output_special_spatialisation_sample(obuf,sampcnt,switchpos,chans,val,valr,l_most,r_most,spacetyp);
							sampcnt += chans;
						} else {
							rmost = (lmost[n][stepcnt] + 1) % chans;
							for(k = 0;k< chans;k++) {
								if(k == lmost[n][stepcnt])	//	Add output only to the 2 relevant channels
									obuf[sampcnt] = (float)(obuf[sampcnt] + val);
								else if(k == rmost)
									obuf[sampcnt] = (float)(obuf[sampcnt] + valr);
								sampcnt++;
							}
						}
					} else {							//	If NOT spatialised, add output to all outchans
						for(k = 0;k < chans;k++) {
							obuf[sampcnt] = (float)(obuf[sampcnt] + val);
							sampcnt++;
						}
					}

				} else {								//	Partial is OFF
					if(splcntr[n] > 0) {				//	BUT IF its still a fade-out, Get any splice contribution
						loindex = (int)floor(sinptr[n]);
						hiindex = loindex + 1;
						loval   = sintab[loindex];
						hival   = sintab[hiindex];
						valdiff = hival - loval;
						timefrac = sinptr[n] - (double)loindex;
						val = loval + (valdiff * timefrac);
						level = read_level(n,time,dz);
						localspliceval = (double)splcntr[n]/(double)splen;	//	Downfade, splcntr falling
						val *= localspliceval;
						splcntr[n]--;					//	Advance splicecnt towards zero
						if(dz->vflag[SYN_SPACED]) {		//	If spatialisation, get spatial contributions FROM PREVIOUS STEP
							if(spacetyp > 0) {
								pos = position[stepcnt-1];
								switchpos = swpos[stepcnt-1];
								spacebox_apply(pos,llev[n][stepcnt-1],chans,&l_most,&r_most,&valr,&vall,spacetyp);
								valr = val * valr;
								val =  val * vall;
							} else {					//	If spatialisation, get spatial contributions
								valr = val * rlev[n][stepcnt-1];
								val  = val * llev[n][stepcnt-1];	
							}
						} else
							val  = val * llev[n][stepcnt-1];	
						if(dz->vflag[SYN_SPACED]) {		//	As fadeout of last sound, keep PREVIOUS STEP spatial coords
							if(spacetyp > 0) {	
								output_special_spatialisation_sample(obuf,sampcnt,switchpos,chans,val,valr,l_most,r_most,spacetyp);
								sampcnt += chans;
							} else {
								rmost = (lmost[n][stepcnt-1] + 1) % chans;
								for(k = 0;k < chans;k++) {
									if(k == lmost[n][stepcnt-1])
										obuf[sampcnt] = (float)(obuf[sampcnt] + val);
									else if(k == rmost)
										obuf[sampcnt] = (float)(obuf[sampcnt] + valr);
									sampcnt++;
								}
							}
						} else {
							for(k = 0;k < chans;k++) {
								obuf[sampcnt] = (float)(obuf[sampcnt] + val);
								sampcnt++;
							}
						}
					}
				}
				incr_sinptr(n,time,onehzincr,dz);	//	Track (modify if ness) the partial-incr value for this partial
			}
			if(instartsplice) {							//	Do big splice at start of output
				sampcnt = base_sampcnt;
				for(k = 0;k < chans;k++) {
					obuf[sampcnt] = (float)(obuf[sampcnt] * spliceval);
					sampcnt++;
				}
				spliceval += spliceincr;
				spliceval = min(spliceval,1.0);
			} else if(inendsplice) {					//	Do big splice at end of output
				sampcnt = base_sampcnt;
				for(k = 0;k < chans;k++) {
					obuf[sampcnt] = (float)(obuf[sampcnt] * spliceval);
					sampcnt++;
				}
				spliceval -= spliceincr;
				spliceval = max(spliceval,0.0);
			}
			sampcnt = base_sampcnt;						//	Normalise output
			for(k = 0;k < chans;k++) {
				obuf[sampcnt] = (float)(obuf[sampcnt] * normaliser);
				sampcnt++;
			}
			if(sampcnt >= dz->buflen) {					//	Check if buffer full - write_samps and refresh
				if((exit_status = write_samps(obuf,sampcnt,dz))<0)
					return(exit_status);
				memset((char *)obuf,0,dz->buflen * sizeof(float));
				sampcnt = 0;
			}
			total_samps_synthed += chans;				//	Find out if (still) in startsplice or endsplice
			if(!inendsplice && (total_samps_synthed >= endsplicestart)) {
				inendsplice = 1;
				spliceval = 1.0;
			} 
			if(instartsplice && (total_samps_synthed >= startspliceend))
				instartsplice = 0;
			break;
		}
	}
	if(sampcnt) {
		if((exit_status = write_samps(obuf,sampcnt,dz))<0)
			return(exit_status);
	}
	return FINISHED;
}

/**************************** INCR_SINPTR ****************************/

void incr_sinptr(int n,double time,double onehzincr,dataptr dz)
{
	int m;
	double 	hival, loval, hitime, lotime, timediff, timefrac, valdiff, partialval, thisincr;
	double *sinptr = dz->parray[(dz->itemcnt * 2) + 1];
	double *thispartial = dz->parray[n];
	m = 0;
	while(thispartial[m] < time) {
		m += 2;
		if(m >= dz->ringsize)
			break;
	}
	if(m==0)
		partialval = thispartial[1];
	else if(m < dz->ringsize) {
		hival  = thispartial[m+1];
		loval  = thispartial[m-1];
		hitime = thispartial[m];
		lotime = thispartial[m-2];
		timediff = hitime - lotime;
		timefrac = (time - lotime)/timediff;
		valdiff  = hival - loval;
		partialval = loval + (valdiff * timefrac);
	} else
		partialval = thispartial[dz->ringsize-1];

	//	Convert partial numbers to table-increments

	thisincr = partialval * onehzincr;
	thisincr *= dz->param[SYNTH_FRQ];
	sinptr[n] += thisincr;
	if(sinptr[n] >= SYNTH_TABSIZE)
		sinptr[n] -= (double)SYNTH_TABSIZE;
}

/**************************** GET_CURRENT_PARTIAL_VALS ****************************/

void get_current_partial_vals(double time,double *pvals,int partialcnt,dataptr dz)
{
	int m, n;
	double 	hival, loval, hitime, lotime, timediff, timefrac, valdiff, partialval;
	double *thispartial;
	for(n = 0;n < partialcnt;n++ ) {	
		thispartial = dz->parray[n];
		m = 0;
		while(thispartial[m] < time) {
			m += 2;
			if(m >= dz->ringsize)
				break;
		}
		if(m==0)
			partialval = thispartial[1];
		else if(m < dz->ringsize) {
			hival  = thispartial[m+1];
			loval  = thispartial[m-1];
			hitime = thispartial[m];
			lotime = thispartial[m-2];
			timediff = hitime - lotime;
			timefrac = (time - lotime)/timediff;
			valdiff  = hival - loval;
			partialval = loval + (valdiff * timefrac);
		} else
			partialval = thispartial[dz->ringsize-1];
		pvals[n] = partialval;
	}
}

/**************************** READ_LEVEL ****************************/

double read_level(int n,double time,dataptr dz)
{
	int m;
	double 	hival, loval, hitime, lotime, timediff, timefrac, valdiff, level;
	double *thislevel = dz->parray[n + dz->itemcnt];
	m = 0;
	while(thislevel[m] < time) {
		m += 2;
		if(m >= dz->ringsize)
			break;
	}
	if(m==0) {
		level = thislevel[1];
	} else if(m < dz->ringsize) {
		hival  = thislevel[m+1];
		loval  = thislevel[m-1];
		hitime = thislevel[m];
		lotime = thislevel[m-2];
		timediff = hitime - lotime;
		timefrac = (time - lotime)/timediff;
		valdiff  = hival - loval;
		level = loval + (valdiff * timefrac);
	} else {
		level = thislevel[dz->ringsize-1];
	}
	return level;
}

/**************************** HANDLE_THE_SPECIAL_DATA ****************************/

int handle_the_special_data(char *str,dataptr dz)
{
	double dummy = 0.0, lasttime = 0.0, lastpartial = 1.0, maxval = 0.0, normaliser;
	int entrycnt = 0, partialcnt, n, timepos, valpos, pno_cnt, lev_cnt = 0;
	int zz, nupno_cnt = 0, nulev_cnt,lstart, m, k, nn, mm;
	double *sortptr;
	int totalpartials = 0, tablecnt, lno_cnt;

	FILE *fp;
	int cnt, linecnt;
	char temp[8000], *p;
	if((fp = fopen(str,"r"))==NULL) {
		sprintf(errstr,"Cannot open file %s to read times.\n",str);
		return(DATA_ERROR);
	}
	linecnt = 0;
	while(fgets(temp,8000,fp)!=NULL) {
		p = temp;
		while(isspace(*p))
			p++;
		if(*p == ';' || *p == ENDOFSTR)	//	Allow comments in file
			continue;
		cnt = 0;
		while(get_float_from_within_string(&p,&dummy)) {
			switch(cnt) {
			case(0):
				if(linecnt == 0) {
					if(dummy != 0) {
						sprintf(errstr,"First time in partials data (%lf) must be zero.\n",dummy);
						return(DATA_ERROR);
					} else
						lasttime = dummy;
				} else {
					if(dummy <= lasttime) {
						sprintf(errstr,"Times do not advance at line %d in partials data.\n",linecnt+1);
						return(DATA_ERROR);
					}
				}
				break;
			default:
				if(ODD(cnt)) {	// ODD entries, partial numbers
					if(dummy < 1.0) {
						sprintf(errstr,"Invalid partial (%lf) (less than 1) on line %d.\n",dummy,linecnt+1);
						return(DATA_ERROR);
					}
					if(cnt == 1) {
						if(dz->mode == 0 && dummy != 1.0) {
							sprintf(errstr,"Invalid first partial (%lf) (must be 1 in this tone-generation mode)\n",dummy);
							return(DATA_ERROR);
						}
						lastpartial = dummy;
					}
					else {
						if(dummy <= lastpartial) {
							sprintf(errstr,"Partial numbers do not increase through line %d.\n",linecnt+1);
							return(DATA_ERROR);
						}
						lastpartial = dummy;
					}
				} else  // EVEN values are levels, which can be -ve (inverted phase)
					maxval = max(maxval,fabs(dummy));
				break;
			}
			cnt++;
		}
		if(cnt < 3 || EVEN(cnt)) {
			sprintf(errstr,"Invalid number of entries (%d) on line %d\n",cnt,linecnt+1);
			return(DATA_ERROR);
		}
		if(linecnt == 0)
			entrycnt = cnt;
		else if(cnt != entrycnt) {
			sprintf(errstr,"Line %d has different number of entries (%d) to previous lines which have (%d)\n",linecnt+1,cnt,entrycnt);
			return(DATA_ERROR);
		}
		linecnt++;
	}
	if(linecnt == 0) {
		sprintf(errstr,"No data found in partials data file.\n");
		return(DATA_ERROR);
	}
	if(flteq(maxval,0.0)) {
		sprintf(errstr,"No significant level found in partials data file.\n");
		return(DATA_ERROR);
	}
	normaliser = 1.0/maxval;
	partialcnt = (entrycnt - 1)/2;
	if(dz->mode == 2) {

/*
 * MODE 2 arrays
 * mpcnt = maxpartial cnt (partials + all transpositions)				  |	| |
 *																		  |	current
 *																		  | frqs
 *	parray	|----------|----------|-|-|-----------------|-----------------|-|-|-|
 *          | tvarying pno+plevel |s|s|	  left_level	| right-level	  step|p|
 *			|	(Mpcnt*2)         |i|i|	      mpcnt		|	mpcnt         timeso|
 *			|					  |n|n|					|				  | | |s|
 *	address	0			   mpcnt*2| |p|(mpcnt*2)+2		|(mpcnt*3)+2	  | (mpcnt*4)+3
 *			|					  |	|t|					|				  (mpcnt*4)+2
 *			|					  |	|r|					|				  | | (mpcnt*4)+4
 *	lengths |  linelen of srcdata | | | maxsteps		|	maxsteps	  | |m|
 *								  |s|t|									  |t|p|
 *	(slen = sintablen)			  |l|o|									  |o|c|
 *	(totl = estimate of no		  |e|t|									  |t|n|
 *		    of timesteps used)	  |n|l|									  |l|t|
 */
		totalpartials = partialcnt * dz->iparam[SYNTH_MAX];
		dz->array_cnt = (totalpartials * 2) + 2;//	An array for every partial-and-partial-transposition, every pandp-level, 
												//	and Sin-table + sintab-incr-pointers
		dz->temp_sampsize = dz->array_cnt;
		dz->array_cnt += (totalpartials * 2) + 3;	//	An array for the left and right level of every partial-and-partial-transposition.
													//	and One array for the steptimes, and one array for the frqs of partials at current-time
		dz->itemcnt = totalpartials;				//	Array for every partial-pno and partial-level + Array for position at every step.
	} else {
		dz->array_cnt = (partialcnt * 2) + 5;	//	An array for every partial-pno, every partial-level, 
												//	+ snd-sintable + sintab-incr-pointers + packet envelope + 2 packet-envelope-temp-arrays
		dz->itemcnt = partialcnt;				//	Array for every partial-pno and partial-level.
	}
	if((dz->parray = (double **)malloc(dz->array_cnt * sizeof(double *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create partial data arrays.\n");
		return(MEMORY_ERROR);
	}
	if(dz->mode == 2)
		zz = totalpartials * 2; 
	else
		zz = partialcnt * 2;
	for(n=0;n <zz;n++) {			//	2 entries (time and value) for every line in the data.
		if((dz->parray[n] = (double *)malloc((linecnt * 2) * sizeof(double)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to store partial data.\n");
			return(MEMORY_ERROR);
		}
	}
	fseek(fp,0,0);
	timepos = 0;			//	Pointer to time-values in all arrays
	valpos  = 1;			//	Pointer to val-at-time in all arrays
	pno_cnt = 0;			//	Pointer to partial-pno table
	lstart  = partialcnt;	//	Start of partial-level table
	if(dz->mode == 2)
		lstart  *= dz->iparam[SYNTH_MAX];

	while(fgets(temp,8000,fp)!=NULL) {
		p = temp;
		if(*p == ';')	//	Allow comments in file
			continue;
		cnt = 0;
		while(get_float_from_within_string(&p,&dummy)) {
			switch(cnt) {
			case(0):
				for(n=0;n <partialcnt;n++)				//	Put time in all pno arrays
					dz->parray[n][timepos] = dummy;
				for(m = lstart,n=0;n <partialcnt;n++,m++)//	Put time in all level arrays
					dz->parray[m][timepos] = dummy;
				pno_cnt = 0;							//	Point to start of pnos, and levels
				lev_cnt = lstart;
				break;
			default:
				if(ODD(cnt))							//	Put pno in appropriate pno-array
					dz->parray[pno_cnt++][valpos] = dummy;
				else									//	Put level in appropriate level-array
					dz->parray[lev_cnt++][valpos] = dummy * normaliser;
				break;
			}
			cnt++;
		}
		if(cnt) {
			timepos += 2;								//	Advance pointers in pno and level tables		
			valpos  +=2;
		}
	}
	if(fclose(fp)<0) {
		fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",str);
		fflush(stdout);
	}
	
	if(dz->mode == 2) {

		dz->scalefact = dz->parray[partialcnt-1][1];	// Remember the original range

		tablecnt = partialcnt;		//	Total number of original partial-no (or level) tables
		entrycnt = timepos;			//	Total number of entries in each table

		if(dz->iparam[SYNTH_MAX] > 1) {
	
			//	COPY ORIGINAL PARTIAL-NO AND LEVEL TABLES INTO HIGHER OCTAVES

		nupno_cnt = partialcnt;			//	Start of new partial-transpositions tables
			nulev_cnt = lstart + partialcnt;//	Pointer to new levels tables
			for(n=1;n<dz->iparam[SYNTH_MAX];n++) {									//	For every additional 8va
				for(pno_cnt=0,lno_cnt=lstart;pno_cnt<tablecnt;pno_cnt++,lno_cnt++) {//	For every original partial-table, and level-table
					for(k=0;k<entrycnt;k+=2) {										//	For every entry in original tables
						dz->parray[nupno_cnt][k]   = dz->parray[pno_cnt][k];		//	At same time
						dz->parray[nupno_cnt][k+1] = (dz->parray[pno_cnt][k+1]) * (n+1); //	Create new table, partials up n octs
						dz->parray[nulev_cnt][k]   = dz->parray[lno_cnt][k];		//	At same time
						dz->parray[nulev_cnt][k+1] = dz->parray[lno_cnt][k+1];		//	Create new table with same levels
					}
					nupno_cnt++;
					nulev_cnt++;
				}
			}

			//	SORT PARTIAL-NOS INTO ASCENDING ORDER

			for(n = 0, m = lstart; n < nupno_cnt-1; n++,m++) {
				for(nn = n+1, mm = m+1; nn < nupno_cnt;nn++, mm++) {
					if(dz->parray[nn][1] < dz->parray[n][1]) {						//	Sort of first partialval in array	
						sortptr = dz->parray[nn];
						dz->parray[nn] = dz->parray[n];
						dz->parray[n] = sortptr;
						sortptr = dz->parray[mm];
						dz->parray[mm] = dz->parray[m];
						dz->parray[m] = sortptr;
					}
				}
			}
		}

	}
	dz->ringsize = linecnt * 2;							//	Store lengths of partial tables (1 time and 1 value entry from each dataline)
	return(FINISHED);
}

/**************************** CREATE_SYNTHESIZER_SNDBUFS ****************************/

int create_synthesizer_sndbufs(dataptr dz)
{
	int n;
	size_t bigbufsize;
	long framesize;
	framesize = F_SECSIZE * dz->infile->channels;
	if(dz->mode == 2)
		framesize = F_SECSIZE * dz->iparam[SYNTH_CHANS];
	dz->bufcnt = 1;
	if((dz->sampbuf = (float **)malloc(sizeof(float *) * (dz->bufcnt+1)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffers.\n");
		return(MEMORY_ERROR);
	}
	if((dz->sbufptr = (float **)malloc(sizeof(float *) * dz->bufcnt))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffer pointers.\n");
		return(MEMORY_ERROR);
	}
	bigbufsize = (size_t)Malloc(-1);
	bigbufsize /= dz->bufcnt;
	if(bigbufsize <=0)
		bigbufsize  = framesize * sizeof(float);

	dz->buflen = bigbufsize / sizeof(float);	
	dz->buflen = (dz->buflen / framesize)  * framesize;
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

/****************************** GET_THE_MODE_FROM_CMDLINE *********************************/

int get_the_mode_from_cmdline(char *str,dataptr dz)
{
	char temp[200], *p;
	if(sscanf(str,"%s",temp)!=1) {
		fprintf(stderr,"Cannot read mode of program.\n");
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

/**************************** GENERATE_PACKET_ENVELOPE *************************/

int generate_packet_envelope (dataptr dz)
{
	int n, halftabsize = TREMOLO_TABSIZE/2;
	int isneg = 0, tablopos, tabhipos, cosarray = (dz->itemcnt * 2) + 2;
	double *costab, *temptab, *origtab, diff, tabrem, tabincr, lotabincr, hitabincr, readpos, frac;
	if((dz->parray[cosarray] = (double *)malloc((TREMOLO_TABSIZE + 1) * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for sine table.\n");
		return(MEMORY_ERROR);
	}
	costab = dz->parray[cosarray];
	if((dz->parray[cosarray+1] = (double *)malloc((TREMOLO_TABSIZE + 1) * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for sine table.\n");
		return(MEMORY_ERROR);
	}
	temptab = dz->parray[cosarray + 1];
	if((dz->parray[cosarray+2] = (double *)malloc((TREMOLO_TABSIZE + 1) * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for sine table.\n");
		return(MEMORY_ERROR);
	}
	origtab = dz->parray[cosarray + 2];
	for(n=0;n<TREMOLO_TABSIZE;n++) {
		costab[n] = cos(PI * 2.0 * ((double)n/(double)TREMOLO_TABSIZE));
		costab[n] += 1.0;
		costab[n] /= 2.0;
		costab[n] = 1.0 - costab[n];
		origtab[n] = costab[n];
	}
	costab[n]  = 0.0; /* wrap around point */
	origtab[n] = 0.0; /* wrap around point */
	if(!dz->brksize[SYNTH_SQZ] && !dz->brksize[SYNTH_CTR]) {
		if(flteq(dz->param[SYNTH_SQZ],1.0)) {
			for(n=0;n<=TREMOLO_TABSIZE;n++)
				temptab[n] = origtab[n];
		} else {
			for(n=0;n<=TREMOLO_TABSIZE;n++)
				temptab[n] = pow(origtab[n],dz->param[SYNTH_SQZ]);
		}
		if(flteq(dz->param[SYNTH_CTR],1.0)) {
			for(n=0;n<=TREMOLO_TABSIZE;n++)
				costab[n] = temptab[n];
		} else {
			if(dz->param[SYNTH_CTR] < 0.0) {
				frac = 1.0 + dz->param[SYNTH_CTR];
				isneg = 1;
			} else 
				frac = 1.0 - dz->param[SYNTH_CTR];
			if(isneg) {
				lotabincr = 1.0/frac;
				hitabincr = 1.0/(2.0 - frac);
			} else {
				lotabincr = 1.0/(2.0 - frac);
				hitabincr = 1.0/frac;
			}
			readpos = 0;
			tabincr = lotabincr;
			for(n=0;n<TREMOLO_TABSIZE;n++) {
				if(readpos >= halftabsize) {
					tabincr = hitabincr;
				}
				tablopos = (int)floor(readpos);
				tabhipos = min(tablopos + 1,TREMOLO_TABSIZE);
				tabrem    = readpos - (double)tablopos;
				diff      = temptab[tabhipos] - temptab[tablopos];
				costab[n] = temptab[tablopos] + (diff * tabrem);
				readpos += tabincr;
			}
		}
	}
	return(FINISHED);
}

/**************************** MODIFY_PACKET_ENVELOPE *************************/

int modify_packet_envelope (dataptr dz)
{
	int n, halftabsize = TREMOLO_TABSIZE/2;
	int isneg = 0, tablopos, tabhipos, cosarray = (dz->itemcnt * 2) + 2;
	double *costab, *temptab, *origtab, diff, tabrem, tabincr, lotabincr, hitabincr, readpos, frac;

	costab = dz->parray[cosarray];
	temptab = dz->parray[cosarray + 1];
	origtab = dz->parray[cosarray + 2];
	if(flteq(dz->param[SYNTH_SQZ],1.0)) {
		for(n=0;n<=TREMOLO_TABSIZE;n++)
			temptab[n] = origtab[n];
	} else {
		for(n=0;n<=TREMOLO_TABSIZE;n++)
			temptab[n] = pow(origtab[n],dz->param[SYNTH_SQZ]);
	}
	if(flteq(dz->param[SYNTH_CTR],1.0)) {
		for(n=0;n<=TREMOLO_TABSIZE;n++)
			costab[n] = temptab[n];
	} else {
			if(dz->param[SYNTH_CTR] < 0.0) {
				frac = 1.0 + dz->param[SYNTH_CTR];
				isneg = 1;
			} else 
				frac = 1.0 - dz->param[SYNTH_CTR];
			if(isneg) {
				lotabincr = 1.0/frac;
				hitabincr = 1.0/(2.0 - frac);
			} else {
				lotabincr = 1.0/(2.0 - frac);
				hitabincr = 1.0/frac;
			}
			readpos = 0;
			tabincr = lotabincr;
			for(n=0;n<TREMOLO_TABSIZE;n++) {
				if(readpos >= halftabsize) {
					tabincr = hitabincr;
				}
				tablopos = (int)floor(readpos);
				tabhipos = min(tablopos + 1,TREMOLO_TABSIZE);
				tabrem    = readpos - (double)tablopos;
				diff      = temptab[tabhipos] - temptab[tablopos];
				costab[n] = temptab[tablopos] + (diff * tabrem);
				readpos += tabincr;
			}
	}
	return(FINISHED);
}

/**************************** READ_PACKET_ENVELOPE *************************/

double read_packet_envelope(int kk,double incr,dataptr dz)
{
	double *costab, tabpos, tabrem, diff, envv;
	int tablopos, tabhipos, cosarray = (dz->itemcnt * 2) + 2;
	costab = dz->parray[cosarray];
	tabpos = (double)kk * incr;
	tablopos = (int)floor(tabpos);
	tabhipos = min(tablopos + 1,TREMOLO_TABSIZE);
	tabrem   = tabpos - (double)tablopos;
	diff = costab[tabhipos] - costab[tablopos];
	envv = costab[tablopos] + (diff * tabrem);
	return envv;
}

/*********************** RNDINTPERM ************************/

void rndintperm(int *perm,int cnt)
{
	int n,t,k;
	memset((char *)perm,0,cnt * sizeof(int));
	for(n=0;n<cnt;n++) {
		t = (int)(drand48() * (double)(n+1)); /* TRUNCATE */
		if(t==n) {
			for(k=n;k>0;k--)
				perm[k] = perm[k-1];
			perm[0] = n;
		} else {
			for(k=n;k>t;k--)
				perm[k] = perm[k-1];
			perm[t] = n;
		}
	}
}

/************************************ PANCALC *******************************/

void pancalc(double position,double *leftgain,double *rightgain)
{
	int dirflag;
	double temp;
	double relpos;
	double reldist, invsquare;

	if(position < 0.0)
		dirflag = SIGNAL_TO_LEFT;		/* signal on left */
	else
		dirflag = SIGNAL_TO_RIGHT;

	if(position < 0) 
		relpos = -position;
	else 
		relpos = position;
	if(relpos <= 1.0){		/* between the speakers */
		temp = 1.0 + (relpos * relpos);
		reldist = ROOT2 / sqrt(temp);
		temp = (position + 1.0) / 2.0;
		*rightgain = temp * reldist;
		*leftgain = (1.0 - temp ) * reldist;
	} else {				/* outside the speakers */
		temp = (relpos * relpos) + 1.0;
		reldist  = sqrt(temp) / ROOT2;   /* relative distance to source */
		invsquare = 1.0 / (reldist * reldist);
		if(dirflag == SIGNAL_TO_LEFT){
			*leftgain = invsquare;
			*rightgain = 0.0;
		} else {   /* SIGNAL_TO_RIGHT */
			*rightgain = invsquare;
			*leftgain = 0;
		}
	}
}

/************************************ SORT_PARTIALS_INTO_ASCENDING_FRQ_ORDER *******************************/

void sort_partials_into_ascending_frq_order(int total_partialcnt,double *pvals,double *sinptr,double **llev,double **rlev,int **onoff,int **lmost,int **origspl,int *splordr,dataptr dz)
{
	double *sortptr, temp;
	int n, m, nn, mm, itemp;
	int *iptr;
	for(n = 0, m = total_partialcnt; n < total_partialcnt-1; n++,m++) {		//	m indexes levels
		for(nn = n+1, mm = m+1; nn < total_partialcnt;nn++, mm++) {

			if(pvals[nn] < pvals[n]) {		//	Sort on partialval

				// Shuffle arrays so they're in ascending frq order, of CURRENT frqs

				sortptr = dz->parray[nn];
				dz->parray[nn] = dz->parray[n];
				dz->parray[n] = sortptr;
				sortptr = dz->parray[mm];
				dz->parray[mm] = dz->parray[m];
				dz->parray[m] = sortptr;

				// Shuffle associated sinptrs

				temp = sinptr[nn];
				sinptr[nn] = sinptr[n];
				sinptr[n] = temp;

				// Shuffle associated (left-)level pointers

				sortptr = llev[nn];
				llev[nn] = llev[n];
				llev[n] = sortptr;

				// Shuffle associated right-level pointers

				sortptr = rlev[nn];
				rlev[nn] = rlev[n];
				rlev[n] = sortptr;

				// Shuffle associated onoff flags

				iptr = onoff[nn];
				onoff[nn] = onoff[n];
				onoff[n] = iptr;

				// Shuffle associated lmost-spkr info

				iptr = lmost[nn];
				lmost[nn] = lmost[n];
				lmost[n] = iptr;

				// Shuffle associated splicectr origins

				iptr = origspl[nn];
				origspl[nn] = origspl[n];
				origspl[n] = iptr;

				// Finally swap frqs into correct order

				temp = pvals[nn];
				pvals[nn] = pvals[n];
				pvals[n] = temp;

				// And keep track of reordering, for 2nd pass

				itemp = splordr[nn];
				splordr[nn] = splordr[n];
				splordr[n] = itemp;

			}
		}
	}
}

/************************************ RESORT_PARTIALS_INTO_ORIGINAL_FRQ_ORDER *******************************/

void resort_partials_into_original_frq_order(int total_partialcnt,double *pvals,double *sinptr,double **llev,double **rlev,int **onoff,int **lmost,int **origspl,int *splordr,dataptr dz)
{
	double *sortptr, temp;
	int n, m, nn, mm;
	int *iptr;
	for(n = 0, m = total_partialcnt; n < total_partialcnt-1; n++,m++) {		//	m indexes levels
		for(nn = n+1, mm = m+1; nn < total_partialcnt;nn++, mm++) {

			if(splordr[nn] < splordr[n]) {		//	Sort on original order value

				// Shuffle arrays so they're in original order

				sortptr = dz->parray[nn];
				dz->parray[nn] = dz->parray[n];
				dz->parray[n] = sortptr;
				sortptr = dz->parray[mm];
				dz->parray[mm] = dz->parray[m];
				dz->parray[m] = sortptr;

				// Shuffle associated sinptrs

				temp = sinptr[nn];
				sinptr[nn] = sinptr[n];
				sinptr[n] = temp;

				// Shuffle associated (left-)level pointers

				sortptr = llev[nn];
				llev[nn] = llev[n];
				llev[n] = sortptr;

				// Shuffle associated right-level pointers

				sortptr = rlev[nn];
				rlev[nn] = rlev[n];
				rlev[n] = sortptr;

				// Shuffle associated onoff flags

				iptr = onoff[nn];
				onoff[nn] = onoff[n];
				onoff[n] = iptr;

				// Shuffle associated lmost-spkr info

				iptr = lmost[nn];
				lmost[nn] = lmost[n];
				lmost[n] = iptr;

				// Shuffle associated splicectr origins

				iptr = origspl[nn];
				origspl[nn] = origspl[n];
				origspl[n] = iptr;

				// Finally swap frqs into correct order

				temp = pvals[nn];
				pvals[nn] = pvals[n];
				pvals[n] = temp;

			}
		}
	}
}

/**************************************** XCLUSIVE **************************************
 *
 *	Resort an existing permuutation (of partials chosen)
 *  so they already ON partials occur after all the corrently-OFF partials
 */

void xclusive(int *perm,int *permon,int *permoff,int max_partials_cnt,int partials_in_play, int **onoff,int stepcnt)
{
	int permoncnt = 0, permoffcnt = 0, n, ptl;
	if(partials_in_play == max_partials_cnt)
		return;
	for(n = 0;n < max_partials_cnt;n++) {
		ptl = perm[n];
		if(onoff[ptl][stepcnt])				//	If this partial is already ON
			permon[permoncnt++] = ptl;		//	Store the ON-partials, in order they were in initial perm
		else
			permoff[permoffcnt++] = ptl;	//	If this partial is OFF
	}										//	Store the OFF-partials, in order they were in initial perm

	for(n=0;n<permoffcnt;n++)				//	Place the OFF partials first in the permlist,
		perm[n] = permoff[n];				//	But otherwise preserving perm order.
	while(n < max_partials_cnt) {
		perm[n] = permon[n];
		n++;
	}
}

/**************************************** EMERGEPOS **************************************
 *
 *	Find spatial position, where image emerging from single channel to gradually fill all channels
 */

double emergepos(int emergchan,int chans,double time,double timespan)
{
	double frac, chanspan, pos, lmost;
	emergchan--;
	frac = time/timespan;						//	Fraction of emerge-time covered
	if(frac < 0.33)
		chanspan = 0;
	else {
		frac = pow((frac - 0.33),1.5);
		chanspan = (double)chans * frac;		//	Fraction of total-channels available
	}
	pos = drand48() * chanspan;					//	Position randomly within chanspan
	lmost = (double)emergchan - (chanspan/2.0);	//	Find leftmost position (relative to emergence chan)
	pos += lmost;								//	Find true position
	if(pos < 0.0)								//	Adjust for %N chans
		pos += (double)chans;
	else if(pos >= chans)						//	Adjust for %N chans
		pos -= (double)chans;
	return pos;
}

/**************************************** EMERGEPOS **************************************
 *
 *	Find spatial position, where image converging to single channel from all channels
 */

double convergepos(int converchan,int chans,double time,double convergetime,double dur)
{
	double frac, chanspan, pos, lmost;			//	Fraction of converge-time covered
	int ipos;
	converchan--;
	frac = (time - convergetime)/(dur - convergetime);
	frac = 1.0 - frac;							//	Amount of convergence
	if(frac < 0.33)
		chanspan = 0;
	else {
		frac = pow((frac - 0.33),2.0);
		chanspan = (double)chans * frac;		//	Fraction of total-channels available
	}
	pos = drand48() * chanspan;					//	Position randomly within chanspan
	ipos = (int)round(pos/0.1);
	pos = ipos * 0.1;
	lmost = (double)converchan - (chanspan/2.0);//	Find leftmost position (relative to convergence chan)
	pos += lmost;								//	Find true position
	if(pos < 0.0)								//	Adjust for %N chans
		pos += (double)chans;
	else if(pos >= chans)						//	Adjust for %N chans
		pos -= (double)chans;
	return pos;
}

/**************************************** SPACEBOX **************************************/

void spacebox(double *pos, int *switchpos, double posstep, int chans, int spacetyp, int configno, int configcnt,int *superperm)
{
	switch(spacetyp) {
	case(SB_LRRAND):				//	Alternate Left and Right sides, random position
		*pos = chans/2 * drand48();	//	Random choice of half of chan positions
		if(*switchpos)				//	If switch on, put in 2nd half	
			*pos += chans/2;
		*switchpos = -(*switchpos); 
		break;
	case(SB_FBRAND):				//	Alternate Front and Back sides, random position
		*pos = chans/2 * drand48();	//	Simil for front and back
		if(*switchpos) {
			*pos += 2;
			if(*pos >= chans)
				*pos -= chans;
		} else {
			*pos += 6;
			if(*pos >= chans)
				*pos -= chans;
		}
		*switchpos = -(*switchpos); 
		break;
	case(SB_ROTATE):				//	Rotating clockwise or anticlockwise
		*pos += posstep;
		if(*pos >= chans)
			*pos -= chans;
		else if(*pos < 0.0)
			*pos += chans;
		break;
	case(SB_SUPERSPACE):
	case(SB_SUPERSPACE2):
	case(SB_SUPERSPACE3):
	case(SB_SUPERSPACE4):			//	Get item in current permutaion of possibilities
		*switchpos = superperm[configcnt];
		break;	
	case(SB_LR):					//	Alternate all-left/all-right	Switch between the 2 alternatives
	case(SB_FB):					//	Alternate all-back/all-front
	case(SB_FRAMESWITCH):			//	Switch all-square/all-diamond	
		*switchpos = !(*switchpos);
		break;
	case(SB_TRIROT1):				//	Rotate triangle formed by spkrs 2-apart clockwise
	case(SB_TRIROT2):				//	Rotate triangle formed by spkrs 3-apart clockwise
		(*switchpos)++;				//	Advance apex of triangle
		if(*switchpos >= chans)
			*switchpos -= chans;
		break;
	case(SB_ANTITRIROT1):			//	Rotate triangle formed by spkrs 2-apart anticlockwise
	case(SB_ANTITRIROT2):			//	Rotate triangle formed by spkrs 2-apart anticlockwise
		(*switchpos)--;				//	Regress apex of triangle
		if(*switchpos < chans)
			*switchpos += chans;
		break;
	}
}
		
/**************************************** SPACEBOX_APPLY **************************************/

void spacebox_apply(double pos, double lev,int chans,int *lmost, int *rmost,double *rlev,double *llev,int spacetyp)
{
	double leftgain, rightgain;
	switch(spacetyp) {
	case(SB_LRRAND):		//	These options use true stereo between adjacent speakers
	case(SB_FBRAND):		//	Find levels and left/right lspkrs
	case(SB_ROTATE):
		*lmost = (int)floor(pos);
		pos -= (double)(*lmost);
		pos = (pos * 2.0) - 1.0;
		pancalc(pos,&leftgain,&rightgain);
		*rlev  = lev * rightgain;
		*llev  = lev * leftgain;
		*rmost = (*lmost + 1) % chans;
		break;
	case(SB_LR):
	case(SB_FB):
	case(SB_TRIROT1):
	case(SB_ANTITRIROT1):
	case(SB_TRIROT2):
	case(SB_ANTITRIROT2):
	case(SB_FRAMESWITCH):
	case(SB_SUPERSPACE):
	case(SB_SUPERSPACE2):
	case(SB_SUPERSPACE3):
	case(SB_SUPERSPACE4):
		*llev = lev;		//	Input level is distributed (as is) amongst various lspkrs
		break;
	}
}

/**************************************** OUTPUT_SPECIAL_SPATIALISATION_SAMPLE **************************************/

void output_special_spatialisation_sample(float *obuf,int sampcnt,int switchpos,int chans,double val,double valr,int lmost,int rmost,int spacetyp)
{
	int k, tri1, tri2, tri3, a, b;
	switch(spacetyp) {
	case(SB_LR):
		if(switchpos) {
			for(k = (chans/2)+1;k < chans;k++)
				obuf[sampcnt+k] = (float)(obuf[sampcnt+k] + val);
		} else {
			for(k = 1;k < chans/2;k++)
				obuf[sampcnt+k] = (float)(obuf[sampcnt+k] + val);
		}
		break;		
	case(SB_FB):
		if(switchpos) {
			for(k = 0;k < chans;k++) {
				if(k < 2 || k == 7)
					obuf[sampcnt+k] = (float)(obuf[sampcnt+k] + val);
			}
		} else {
			for(k = 3;k < 6;k++)
				obuf[sampcnt+k] = (float)(obuf[sampcnt+k] + val);
		}
		break;		
	case(SB_TRIROT1):
	case(SB_ANTITRIROT1):
		tri1 = switchpos;
		tri2 = (switchpos + 2) % chans;
		tri3 = (switchpos + 6) % chans;
		for(k = 0;k< chans;k++) {
			if(k == tri1 || k == tri2 || k == tri3)	//	Add output only to the 2 relevant channels
				obuf[sampcnt] = (float)(obuf[sampcnt] + val);
			sampcnt++;
		}
		break;		
	case(SB_TRIROT2):
	case(SB_ANTITRIROT2):
		tri1 = switchpos;
		tri2 = (switchpos + 3) % chans;
		tri3 = (switchpos + 5) % chans;
		for(k = 0;k< chans;k++) {
			if(k == tri1 || k == tri2 || k == tri3)	//	Add output only to the 2 relevant channels
				obuf[sampcnt] = (float)(obuf[sampcnt] + val);
			sampcnt++;
		}
		break;		
	case(SB_FRAMESWITCH):
		if(switchpos) {
			for(k = 0;k< chans;k++) {	//	SQUARE
				if(ODD(k))
					obuf[sampcnt] = (float)(obuf[sampcnt] + val);
				sampcnt++;
			}
		} else {
			for(k = 0;k< chans;k++) {	//	DIAMOND
				if(EVEN(k))
					obuf[sampcnt] = (float)(obuf[sampcnt] + val);
				sampcnt++;
			}
		}
		break;
	case(SB_SUPERSPACE):
	case(SB_SUPERSPACE2):
	case(SB_SUPERSPACE3):
	case(SB_SUPERSPACE4):
		if(switchpos <= 7) {					//  0 - 7	Single chans
			obuf[sampcnt+switchpos] = (float)(obuf[sampcnt+switchpos] + val);
		} else if(switchpos <=35) {				//	8 - 35
			switchpos -= 8;						//	0 - 27
			if(switchpos >=24) {				//	24 - 27
				switchpos -= 24;				//	0 - 3 paired with its opposite
				obuf[sampcnt+switchpos] = (float)(obuf[sampcnt+switchpos] + val);
				switchpos += chans/2;			//	4 - 7
				obuf[sampcnt+switchpos] = (float)(obuf[sampcnt+switchpos] + val);
			} else {							//	0 - 23	
				a = switchpos/3;				//	0-7 = a
				b = switchpos - (a*3);			//	0-2
				b++;							//	1-3
				b = (a + b) % chans;			//	a+(1-3)
				obuf[sampcnt+a] = (float)(obuf[sampcnt+a] + val);
				obuf[sampcnt+b] = (float)(obuf[sampcnt+b] + val);
			}
		} else if(switchpos <= 43) {			//	36 - 43	TRIANGLE 1
			switchpos -=36;						//	0 - 7
			tri1 = switchpos;					//	0,1,2...
			tri2 = (switchpos + 2) % chans;		//	2,3,4...
			tri3 = (switchpos + 6) % chans;		//	7,6,0...
			for(k = 0;k< chans;k++) {
				if(k == tri1 || k == tri2 || k == tri3)	//	Add output only to the 2 relevant channels
					obuf[sampcnt] = (float)(obuf[sampcnt] + val);
				sampcnt++;
			}
		} else if(switchpos <= 51) {			//	44 - 51	TRIANGLE 2
			switchpos -= 44;					//	0 - 7
			tri1 = switchpos;					//	0,1,2,...
			tri2 = (switchpos + 3) % chans;		//	3,4,5...
			tri3 = (switchpos + 5) % chans;		//	5,6,7...
			for(k = 0;k< chans;k++) {
				if(k == tri1 || k == tri2 || k == tri3)	//	Add output only to the 2 relevant channels
					obuf[sampcnt] = (float)(obuf[sampcnt] + val);
				sampcnt++;
			}
		} else if(switchpos == 52) {			//	SQUARE
			for(k = 0;k< chans;k++) {
				if(EVEN(k))						//	0,2,4,6
					obuf[sampcnt] = (float)(obuf[sampcnt] + val);
				sampcnt++;
			}
			break;		
		} else if(switchpos == 53) {			//	DIAMOND
			for(k = 0;k< chans;k++) {
				if(ODD(k))						//	1,3,5,7
					obuf[sampcnt] = (float)(obuf[sampcnt] + val);
				sampcnt++;
			}
			break;		
		} else {								//	54 ALL
			for(k = 0;k< chans;k++) {			//	0,1,2,3,4,5,6,7
				obuf[sampcnt] = (float)(obuf[sampcnt] + val);
				sampcnt++;
			}
			break;		
		}
		break;
	default:		//	STEREO POSITIONED BETWEEN SOME PAIR OF CHANNELS
		for(k = 0;k< chans;k++) {
			if(k == lmost)	//	Add output only to the 2 relevant channels
				obuf[sampcnt] = (float)(obuf[sampcnt] + val);
			else if(k == rmost)
				obuf[sampcnt] = (float)(obuf[sampcnt] + valr);
			sampcnt++;
		}
	}		
}

/**************************** DUFFING *************************/

int duffing(dataptr dz)
{
	int exit_status, passno;
	int sampcnt, bufpos, n;
	double time, maxsamp = 0.0, spliceamp, normaliser = 1.0, srate = (double)dz->iparam[SYNTHSRAT], tabpos, val, vel;
	double delta_t = 1.0/srate;		//	Time-increment between sample-generation
	float *obuf = dz->sampbuf[0];
	int outlen = (int)round(dz->infile->srate * dz->param[SYNTH_DUR]);
	double spliceincr = 1.0/dz->rampbrksize;
	int splicestart = outlen - dz->rampbrksize;
	dz->scalefact = (double)SYNTH_TABSIZE/srate;			//	Constant in sintable read
	for(passno = 0;passno < 2;passno++) {
		switch(passno) {
		case(0): fprintf(stdout,"INFO: Assessing level.\n");			break;
		case(1): fprintf(stdout,"INFO: Generating output sound.\n");	break;
		}
		fflush(stdout);
		sampcnt = 0;
		bufpos = 0;
		tabpos = 0.0;										//	Initial position in table reading VELOCITY of point
		val = 0.0;											//	Initial POSITION of point
		vel = 0.0;											//	Initial velocity of point
		if(splicestart < 0)									//	If outduration too short to include whole endsplice
			spliceamp = outlen/dz->rampbrksize;				//	Preset start-amp at appropriate level within endsplice
		else
			spliceamp = 1.0;
		while(sampcnt < outlen) {
			obuf[bufpos] = (float)val;
			if(sampcnt >= splicestart) {						//	If in  endsplice, do end splice
				obuf[bufpos] = (float)(obuf[bufpos] * spliceamp);
				spliceamp -= spliceincr;
				spliceamp = max(spliceamp,0.0);
			}
			bufpos++;
			if(bufpos >= dz->buflen)  {
				switch(passno) {
				case(0):
					for(n=0;n<dz->buflen;n++)
						maxsamp = max(maxsamp,fabs(obuf[n]));
					break;
				case(1):
					for(n=0;n<dz->buflen;n++)
						obuf[n] = (float)(obuf[n] * normaliser);
					if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
						return(exit_status);
					break;
				}
				bufpos = 0;
			}
			time = (double)sampcnt/srate;
			if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
				return exit_status;
			duffing_osc(&val,&vel,delta_t,&tabpos,dz);
			sampcnt++;
		}
		if(bufpos > 0) {
			switch(passno) {
			case(0):
				for(n=0;n<bufpos;n++)
					maxsamp = max(maxsamp,fabs(obuf[n]));
				if(maxsamp <= 0.0) {
					sprintf(errstr,"NO SIGNIFICANT SOUND-LEVEL PRODUCED.\n");
					return(GOAL_FAILED);
				} else if(maxsamp > 0.95) {
					if(maxsamp >= HUGE) {
						fprintf(stdout,"INFO: Output blew up\n");
						fflush(stdout);
						exit(1);
					}
					normaliser = 1.0/maxsamp;
					fprintf(stdout,"INFO: Max Level %lf Normalising output by %lf\n",maxsamp,normaliser);
					fflush(stdout);
				}
				break;
			case(1):
				for(n=0;n<bufpos;n++)
					obuf[n] = (float)(obuf[n] * normaliser);
				if((exit_status = write_samps(obuf,bufpos,dz))<0)
					return(exit_status);
				break;
			}
		}
	}
	return FINISHED;
}

/**************************** SINREAD *************************/

double sinread(double *tabpos,double frq,dataptr dz)
{
	double tabincr, val, valdiff, timefrac, *sintab = dz->parray[0]; 
	int lopos, hipos;
	lopos = (int)floor(*tabpos);
	hipos = (int)ceil(*tabpos);
	timefrac = *tabpos - (double)lopos;
	val = sintab[lopos];
	valdiff = sintab[hipos] - val;
	val += valdiff * timefrac;
	tabincr = frq * dz->scalefact;
	*tabpos += tabincr;
	if(*tabpos >= SYNTH_TABSIZE)
		*tabpos -= SYNTH_TABSIZE;
	return val;
}


/**************************** DUFFING_OSC *************************
 *
 *
 *	dx/dt = y	velocity is rate of change of position (x)
 *
 *				3
 *	dy/dt = x - x  - Dy + Fcoswt	acceleration is driven partly by sinusoid-forcing and partly by system-damping
 *
 *	where Fcoswt is a driving sinusoidal oscillation
 *	and x - x3 - Dy is the damping due to the Duffing double well.
 */

void duffing_osc(double *val,double *vel, double delta_t,double *tabpos,dataptr dz)
{
	double delta_vel, damped_acc, forced_acc;
	damped_acc = (dz->param[SYNTH_K] * (*val)) - (dz->param[SYNTH_B] * pow((*val),3.0));					//	Duffing damping of acceleration
	damped_acc -= dz->param[SYNTH_DAMP] * (*vel);
	forced_acc = sinread(tabpos,dz->param[SYNTH_FRQ],dz);	//	Sinusoidally varying acceleration
	forced_acc *= 1000000;									//	scaled in amplitude
	delta_vel = (damped_acc + forced_acc) * delta_t;		//	Change in velocity caused by acceleration
	*val += *vel * delta_t;									//	position changed due to velocity
	*vel += delta_vel;										//	velocity changed due to aceleration
}
