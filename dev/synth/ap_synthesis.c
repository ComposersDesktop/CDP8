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
    License along with the CDP System; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
 *
 */

/* RWD/TW 24/02/18: removed usage ref to "Outputs..." names */

#include <stdio.h>
#include <stdlib.h>
#include <structures.h>
#include <cdpmain.h>
#include <tkglobals.h>
#include <pnames.h>
#include <synth.h>
#include <processno.h>
#include <modeno.h>
#include <globcon.h>
#include <logic.h>
#include <filetype.h>
#include <mixxcon.h>
#include <speccon.h>
#include <flags.h>
#include <arrays.h>
#include <formants.h>
#include <sfsys.h>
#include <osbind.h>
#include <string.h>
#include <math.h>
#include <srates.h>
//TW UPDATES
#include <ctype.h>
#include <special.h>

#ifndef HUGE
#define HUGE 3.40282347e+38F
#endif

/********************************************************************************************/
/********************************** FORMERLY IN pconsistency.c ******************************/
/********************************************************************************************/

static int  synth_consistency(dataptr dz);
static void init_specsynth(dataptr dz);
static int specsynth_consistency(dataptr dz);
//TW UPDATE: new functions
static int read_clicktrack_data(char *,dataptr dz);
static int gettempo(char *str,double *tempo,double *pulse_unit,double *accel,int linecnt,int *GP,int *is_abstime);
static int getmeter(char *str,int *metre_num,int *metre_denom,int linecnt,int GP,int is_abstime,double *abstime,int *accent);
static int getstyle(char *str,char *style,int metre_num,int linecnt,int GP,int is_abstime);
static int getcount(char *str,int *barcnt,int linecnt,int GP,int is_abstime,double *timeincr);
static int generate_clickdata(int init,int *arraysize,double tempo,double pulse_unit,int metre_num,int metre_denom,
							  char *style,int barcnt,double accel,double *time,double *lasttime,
							  int GP,int is_abstime,double abstime,int accent,double timeincr,int *tcnt,int *tarray,
							  double *mintimestep,dataptr dz);
static int default_style(char *style,int metre_num);
static int generate_clicktable(dataptr dz);
static int click_consistency(dataptr dz);
static int getlineno(char *str,int tcnt,int linecnt);
static int read_chord_data(char *str,dataptr dz);

/***************************************************************************************/
/****************************** FORMERLY IN aplinit.c **********************************/
/***************************************************************************************/

/***************************** ESTABLISH_BUFPTRS_AND_EXTRA_BUFFERS **************************/

int establish_bufptrs_and_extra_buffers(dataptr dz)
{
	int exit_status;
	int is_spec = FALSE;
	dz->extra_bufcnt = -1;	/* ENSURE EVERY CASE HAS A PAIR OF ENTRIES !! */
	dz->bptrcnt = 0;
	dz->bufcnt  = 0;
	switch(dz->process) {
	case(SYNTH_SPEC):
		is_spec = TRUE;			
		dz->extra_bufcnt = 0;	dz->bptrcnt = 5;
		break;
	case(SYNTH_WAVE):		
	case(MULTI_SYN):		
	case(SYNTH_NOISE):	
	case(SYNTH_SIL):			
//TW UPDATE
	case(CLICK):			
		dz->extra_bufcnt = 0;	dz->bufcnt = 1;
		break;
	default:
		sprintf(errstr,"Unknown program type [%d] in establish_bufptrs_and_extra_buffers()\n",dz->process);
		return(PROGRAM_ERROR);
	}

	if(dz->extra_bufcnt < 0) {
		sprintf(errstr,"bufcnts have not been set: establish_bufptrs_and_extra_buffers()\n");
		return(PROGRAM_ERROR);
	}
	if(is_spec)
		return establish_spec_bufptrs_and_extra_buffers(dz);
	else if((dz->process==HOUSE_SPEC && dz->mode==HOUSE_CONVERT) || dz->process==INFO_DIFF) {
		if((exit_status = establish_spec_bufptrs_and_extra_buffers(dz))<0)
			return(exit_status);
	}
	return establish_groucho_bufptrs_and_extra_buffers(dz);
}

/***************************** SETUP_INTERNAL_ARRAYS_AND_ARRAY_POINTERS **************************/

int setup_internal_arrays_and_array_pointers(dataptr dz)
{
	int n;		 
	dz->ptr_cnt    = -1;		/* base constructor...process */
	dz->array_cnt  = -1;
	dz->iarray_cnt = -1;
	dz->larray_cnt = -1;
	switch(dz->process) {
//TW UPDATE
	case(CLICK):		   dz->array_cnt=3; dz->iarray_cnt=0; dz->larray_cnt=0; dz->ptr_cnt= 0; dz->fptr_cnt = 0; break;
	case(SYNTH_WAVE):
	case(SYNTH_NOISE):	   dz->array_cnt=1; dz->iarray_cnt=0; dz->larray_cnt=0; dz->ptr_cnt= 0; dz->fptr_cnt = 0; break;
	case(MULTI_SYN):	   dz->array_cnt=4; dz->iarray_cnt=0; dz->larray_cnt=1; dz->ptr_cnt= 0; dz->fptr_cnt = 0; break;	
	case(SYNTH_SPEC):
	case(SYNTH_SIL):	   dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt=0; dz->ptr_cnt= 0; dz->fptr_cnt = 0; break;
	}

/*** WARNING ***
ANY APPLICATION DEALING WITH A NUMLIST INPUT: MUST establish AT LEAST 1 double array: i.e. dz->array_cnt = at least 1
**** WARNING ***/


	if(dz->array_cnt < 0 || dz->iarray_cnt < 0 || dz->larray_cnt < 0 || dz->ptr_cnt < 0 || dz->fptr_cnt < 0) {
		sprintf(errstr,"array_cnt not set in setup_internal_arrays_and_array_pointers()\n");	   
		return(PROGRAM_ERROR);
	}

	if(dz->array_cnt > 0) {  
		if((dz->parray  = (double **)malloc(dz->array_cnt * sizeof(double *)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for internal double arrays.\n");
			return(MEMORY_ERROR);
		}
		for(n=0;n<dz->array_cnt;n++)
			dz->parray[n] = NULL;
	}
	if(dz->iarray_cnt > 0) {
		if((dz->iparray = (int     **)malloc(dz->iarray_cnt * sizeof(int *)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for internal int arrays.\n");
			return(MEMORY_ERROR);
		}
		for(n=0;n<dz->iarray_cnt;n++)
			dz->iparray[n] = NULL;
	}
	if(dz->larray_cnt > 0) {	  
		if((dz->lparray = (int    **)malloc(dz->larray_cnt * sizeof(int *)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for internal long arrays.\n");
			return(MEMORY_ERROR);
		}
		for(n=0;n<dz->larray_cnt;n++)
			dz->lparray[n] = NULL;
	}
	if(dz->ptr_cnt > 0)   {  	  
		if((dz->ptr    	= (double  **)malloc(dz->ptr_cnt  * sizeof(double *)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for internal pointer arrays.\n");
			return(MEMORY_ERROR);
		}
		for(n=0;n<dz->ptr_cnt;n++)
			dz->ptr[n] = NULL;
	}
	if(dz->fptr_cnt > 0)   {  	  
		if((dz->fptr = (float **)malloc(dz->fptr_cnt * sizeof(float *)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for internal float-pointer arrays.\n");
			return(MEMORY_ERROR);
		}
		for(n=0;n<dz->fptr_cnt;n++)
			dz->fptr[n] = NULL;
	}
	return(FINISHED);
}

/****************************** ASSIGN_PROCESS_LOGIC *********************************/

int assign_process_logic(dataptr dz)
{						 
	switch(dz->process) {
//TW UPDATE
	case(CLICK):		   		setup_process_logic(NO_FILE_AT_ALL,	UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(MULTI_SYN):		
	case(SYNTH_WAVE):	   		setup_process_logic(NO_FILE_AT_ALL,	UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(SYNTH_NOISE):	   		setup_process_logic(NO_FILE_AT_ALL,	UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(SYNTH_SIL):	   		setup_process_logic(NO_FILE_AT_ALL,	UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(SYNTH_SPEC):	   		setup_process_logic(NO_FILE_AT_ALL,	BIG_ANALFILE,		NO_OUTPUTFILE,	dz);	break;
	default:
		sprintf(errstr,"Unknown process: assign_process_logic()\n");
		return(PROGRAM_ERROR);
		break;
	}
	if(dz->has_otherfile) {
		switch(dz->input_data_type) {
		case(ALL_FILES):
		case(TWO_SNDFILES):
		case(SNDFILE_AND_ENVFILE):
		case(SNDFILE_AND_BRKFILE):
		case(SNDFILE_AND_UNRANGED_BRKFILE):
		case(SNDFILE_AND_DB_BRKFILE):
			break;
		case(MANY_SNDFILES):
			if(dz->process==INFO_TIMELIST)
				break;
			/* fall thro */
		default:
			sprintf(errstr,"Most processes accepting files with different properties\n"
						   "can only take 2 sound infiles.\n");
			return(PROGRAM_ERROR);
		}
	}
	return(FINISHED);
}

/***************************** SET_LEGAL_INFILE_STRUCTURE **************************
 *
 * Allows 2nd infile to have different props to first infile.
 */

void set_legal_infile_structure(dataptr dz)
{
	switch(dz->process) {
	default:
		dz->has_otherfile = FALSE;
		break;
	}
}

/****************************** FORMERLY IN internal.c *********************************/
/***************************************************************************************/

/****************************** SET_LEGAL_INTERNALPARAM_STRUCTURE *********************************/

int set_legal_internalparam_structure(int process,int mode,aplptr ap)
{
	int exit_status = FINISHED;
	switch(process) {
//TW UPDATE
	case(CLICK):			exit_status = set_internalparam_data("i",ap); 		break;
	case(SYNTH_SPEC):
	case(MULTI_SYN):		
	case(SYNTH_WAVE):		exit_status = set_internalparam_data("",ap); 		break;
	case(SYNTH_NOISE):		exit_status = set_internalparam_data("0",ap); 		break;
	case(SYNTH_SIL):		exit_status = set_internalparam_data("00",ap); 		break;
	default:
		sprintf(errstr,"Unknown process in set_legal_internalparam_structure()\n");
		return(PROGRAM_ERROR);
	}
	return(exit_status);		
}

/********************************************************************************************/
/********************************** FORMERLY IN specialin.c *********************************/
/********************************************************************************************/

/********************** READ_SPECIAL_DATA ************************/

int read_special_data(char *str,dataptr dz)	   
{
	aplptr ap = dz->application;

	switch(ap->special_data) {
//TW UPDATE
	case(CLICKTRACK):
		dz->infile->srate = CLICK_SRATE;
		return read_clicktrack_data(str,dz);
	case(CHORD_SYN):
		dz->infile->srate = CLICK_SRATE;
		return read_chord_data(str,dz);
	default:
		sprintf(errstr,"Unknown special_data type: read_special_data()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);	/* NOTREACHED */
}

/********************************************************************************************/
/********************************** FORMERLY IN preprocess.c ********************************/
/********************************************************************************************/

/****************************** PARAM_PREPROCESS *********************************/

int param_preprocess(dataptr dz)	
{
	int exit_status = FINISHED, n;
//TW UPDATE
	double x, convertor;

	switch(dz->process) {
//TW UPDATE
	case(CLICK):
		if((exit_status = generate_clicktable(dz))<0)
			return(exit_status);
		break;
	case(SYNTH_WAVE):		
	case(SYNTH_NOISE):		
	case(SYNTH_SIL):
		break;		
	case(MULTI_SYN):
		convertor = (double)dz->iparam[SYN_TABSIZE] / (double)dz->iparam[SYN_SRATE];
		for(n = 0;n<dz->itemcnt;n++)
			dz->parray[1][n] *= convertor;
		break;
	case(SYNTH_SPEC):
		x = (dz->param[SS_FOCUS] * 4.0) - 2.0;
		dz->param[SS_FOCUS] = pow(10,x);
		x = (dz->param[SS_FOCUS2] * 4.0) - 2.0;
		dz->param[SS_FOCUS2] = pow(10,x);
		break;		
	default:
		sprintf(errstr,"PROGRAMMING PROBLEM: Unknown process in param_preprocess()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/********************************************************************************************/
/********************************** FORMERLY IN procgrou.c **********************************/
/********************************************************************************************/

/**************************** GROUCHO_PROCESS_FILE ****************************/

int groucho_process_file(dataptr dz)   /* FUNCTIONS FOUND IN PROCESS.C */
{	
	switch(dz->process) {
//TW UPDATE
	case(CLICK):		
	case(SYNTH_WAVE):		
	case(MULTI_SYN):		
	case(SYNTH_NOISE):		
	case(SYNTH_SIL):	
		return do_synth(dz);
	case(SYNTH_SPEC):
		return do_stereo_specsynth(dz);
	default:
		sprintf(errstr,"Unknown case in process_file()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);	/* NOTREACHED */
}

/********************************************************************************************/
/********************************** FORMERLY IN pconsistency.c ******************************/
/********************************************************************************************/

/****************************** CHECK_PARAM_VALIDITY_AND_CONSISTENCY *********************************/

int check_param_validity_and_consistency(dataptr dz)
{
	int exit_status;
	handle_pitch_zeros(dz);
	switch(dz->process) {
	case(SYNTH_WAVE):	
	case(MULTI_SYN):		
	case(SYNTH_NOISE):	
	case(SYNTH_SIL):	
		if((exit_status = synth_consistency(dz))<0)
			return exit_status;
		dz->infile->channels = dz->iparam[SYN_CHANS];
		dz->infile->srate    = dz->iparam[SYN_SRATE];
		dz->infile->stype = SAMP_SHORT;
		return create_sized_outfile(dz->wordstor[0],dz);
	case(SYNTH_SPEC):	
		init_specsynth(dz);
//TW UPDATE
		return specsynth_consistency(dz);
	case(CLICK):	
//TW UPDATE
		if((exit_status = click_consistency(dz))<0)
			return exit_status;
		dz->infile->channels = 1;
		dz->infile->srate    = 44100;
		dz->infile->stype = SAMP_SHORT;
		return create_sized_outfile(dz->wordstor[0],dz);
	}
	return(FINISHED);
}

/****************************** SYNTH_CONSISTENCY *********************************/

int synth_consistency(dataptr dz)
{
	if(BAD_SR(dz->iparam[SYN_SRATE])) {
		sprintf(errstr,"Invalid sample rate.\n");
		return(DATA_ERROR);
	}
	dz->iparam[SYN_TABSIZE] /= 4;
	dz->iparam[SYN_TABSIZE] *= 4;
	
	return(FINISHED);
}


/****************************** SPECSYNTH_CONSISTENCY *********************************/

int specsynth_consistency(dataptr dz)
{
	if(dz->vflag[SS_PICHSPRD]) {
		if(dz->param[SS_SPREAD] > dz->nyquist/SPEC_MINFRQ) {
			sprintf(errstr,"SPREAD value is too large for pitch transposition (rather than frq) data.\n");
			return(DATA_ERROR);
		}
	}
	return(FINISHED);
}

/********************************************************************************************/
/********************************** FORMERLY IN buffers.c ***********************************/
/********************************************************************************************/

/**************************** ALLOCATE_LARGE_BUFFERS ******************************/

int allocate_large_buffers(dataptr dz)
{
	int exit_status, tempchans;
	switch(dz->process) {
//TW UPDATE
	case(SYNTH_WAVE):		case(SYNTH_NOISE):	case(SYNTH_SIL):	case(CLICK):	case(MULTI_SYN):
		tempchans = dz->infile->channels;
		if(dz->process == CLICK)
			dz->infile->channels = 1;
		else
			dz->infile->channels = dz->iparam[SYN_CHANS];
		if((exit_status = create_sndbufs(dz))<0)
			return exit_status;
		dz->infile->channels = tempchans;
		return FINISHED;
	case(SYNTH_SPEC):
		return allocate_triple_buffer(dz);
	default:
		sprintf(errstr,"Unknown program no. in allocate_large_buffers()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);	/* NOTREACHED */
}

/********************************************************************************************/
/********************************** FORMERLY IN cmdline.c ***********************************/
/********************************************************************************************/

int get_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	if     (!strcmp(prog_identifier_from_cmdline,"wave"))		dz->process = SYNTH_WAVE;
	else if(!strcmp(prog_identifier_from_cmdline,"noise"))		dz->process = SYNTH_NOISE;
	else if(!strcmp(prog_identifier_from_cmdline,"silence"))	dz->process = SYNTH_SIL;
	else if(!strcmp(prog_identifier_from_cmdline,"spectra"))	dz->process = SYNTH_SPEC;
	else if(!strcmp(prog_identifier_from_cmdline,"clicks"))		dz->process = CLICK;
	else if(!strcmp(prog_identifier_from_cmdline,"chord"))		dz->process = MULTI_SYN;
	else {
		sprintf(errstr,"Unknown program identification string '%s'\n",prog_identifier_from_cmdline);
		return(USAGE_ONLY);
	}
//TW UPDATE
	return(FINISHED);
}

/********************************************************************************************/
/********************************** FORMERLY IN usage.c *************************************/
/********************************************************************************************/

/******************************** USAGE1 ********************************/

int usage1(void)
{
	sprintf(errstr,
	"USAGE: synth NAME (mode) outfile parameters\n"
	"\n"
	"where NAME can be any one of\n"
	"\n"
//TW UPDATE
	"wave    noise   silence    spectra    clicks    chord\n"
	"\n"
	"Type 'synth wave'  for more info on synth wave option... ETC.\n");
	return(USAGE_ONLY);
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
	if(!strcmp(str,"wave")) {
    	sprintf(errstr,
    	"GENERATE SIMPLE WAVEFORMS\n\n"
		"USAGE: synth wave mode outfile sr chans dur freq [-aamp] [-ttabsize]\n\n"
		"MODES ARE\n"
		"1) sine wave\n"
		"2) square wave\n"
		"3) sawtooth wave\n"
		"4) ramp wave\n\n"
		"SR      (sample rate) can be 48000, 24000, 44100, 22050, 32000, or 16000\n"
		"CHANS   can be 1, 2 or 4\n"
		"DUR     is duration of output snd, in seconds.\n"
		"FREQ    is frq of output sond, in Hz\n"
		"AMP     is amplitude of output sound: 0.0 < Range <= 1.0 (max & default).\n"
		"TABSIZE is size of table storing waveform.\n"
		"        defaults to 256: input value always rounded to multiple of 4.\n\n"
		"Frq and Amp may vary through time.\n");
	} else if(!strcmp(str,"chord")) {
    	sprintf(errstr,
    	"GENERATE CHORD ON SIMPLE WAVEFORM\n\n"
		"USAGE: synth chord mode outfile datafile sr chans dur  [-aamp] [-ttabsize]\n\n"
		"MODES ARE\n"
		"1) DATAFILE has list of midi data\n"
		"2) DATAFILE has list of frequency data\n"
		"SR      (sample rate) can be 48000, 24000, 44100, 22050, 32000, or 16000\n"
		"CHANS   can be 1, 2 or 4\n"
		"DUR     is duration of output snd, in seconds.\n"
		"AMP     is amplitude of output sound: 0.0 < Range <= 1.0 (max & default).\n"
		"TABSIZE is size of table storing waveform.\n"
		"        defaults to 4096: input value always rounded to multiple of 4.\n\n");
	} else if(!strcmp(str,"noise")) {
    	sprintf(errstr,
    	"GENERATE NOISE\n\n"
		"USAGE: synth noise outfile sr chans dur [-aamp]\n\n"
		"SR      (sample rate) can be 48000, 24000, 44100, 22050, 32000, or 16000\n"
		"CHANS   can be 1, 2 or 4\n"
		"DUR     is duration of output snd, in seconds.\n"
		"AMP     is amplitude of output sound: 0.0 < Range <= 1.0 (max & default).\n\n"
		"Amp may vary through time.\n");
	} else if(!strcmp(str,"silence")) {
    	sprintf(errstr,
    	"MAKE SILENT SOUNDFILE\n\n"
		"USAGE: synth silence outfile sr chans dur\n\n"
		"SR      (sample rate) can be 48000, 24000, 44100, 22050, 32000, or 16000\n"
		"CHANS   can be 1, 2 or 4\n"
		"DUR     is duration of output snd, in seconds.\n");
//TW UPDATE : new options
	} else if(!strcmp(str,"spectra")) {
    	sprintf(errstr,
    	"GENERATE BOTH CHANNELS OF A STEREO SPECTRAL BAND\n\n"
		"USAGE: synth spectra outfilename dur frq spread max-foc min-foc\n"
		"                     timevar srate [-p]\n\n"
		"DUR      duration of output snd, in seconds.\n"
		"FRQ      centre frequency of the band.\n"
		"SPREAD   width of band, in Hz (default) or as transposition ratio.\n"
		"'Foc' = focus, degree to which band energy is concentrated at centre frequency.\n"
		"MAX-FOC  range 0 1\n"
		"MIN-FOC  range 0 1\n"
		"TIMEVAR  Degree to which the band varies over time. Range 0-1.\n"
		"SRATE    Sample rate of output audio file.\n\n"
		"-p       If flag set, 'SPREAD' is a transposition ratio.\n\n");
	} else if(!strcmp(str,"clicks")) {
    	sprintf(errstr,
    	"CLICKTRACK    USAGE: synth clicks mode outname clickfile [-sstart -eend -zzero]\n\n"
		"      make clicktrack from 'start' to 'end'; music starting at dataline 'zero'.\n"
		"      MODE 1: start and end are TIMES:    MODE 2: start & end are data linenos.\n"
		"    CLICKFILE contains sequence of datalines. Each dataline has a line number \n"
 		"    followed by 2, 3 or 4 data items, separated by spaces, which can be....\n"
		"TIME  141.52   (Set time to 141.52 secs: must be later than previous event)\n"
		"GP  1  23.7    (Pause of 23.7sec, with accent [1] at start: '0' for no accent)\n"
		"       OR the following set of items:-\n"
 		"1) TEMPO : in one of the forms (with no spaces around '='):-\n"
 		"       1=144         (crotchet = 144)   1.5=100    (dotted crotchet = 100)\n"
 		"       0.5=144to85.3 (tempo change from quaver = 144 to 85.3)\n"
 		"2) BARRING :  in the form 4:4 or 6:8 or 7:16 etc. (no spaces around the ':')\n"
 		"3) COUNT :  i.e. the (integer) number of bars in this format.\n"
		"------- and, optionally ---------\n"
 		"4) ACCENT PATTERN  1.....  strong beat followed by 5 weak beats\n"
 		"                   1..1..  strong beat (start), secondary (at 4), weak between\n"
 		"                   100100  strong & secondary beats, but NO intermediate beats\n"
		"IF THE ACCENT PATTERN IS OMITTED, default patterns are used. These are:-\n"
		"An accent on the start of each bar PLUS\n"
		"a) in 6:8, 9:8, 12:8, 15:8 etc, or 6:4, 6:16, 6:32, 9:4, 9:16 etc.\n"
		"      sound on every unit (4,8,16,32 etc.), secondary accents every 3rd beat\n"
		"b) in All other meters: NO secondary accents\n");

	} else
		sprintf(errstr,"Unknown option '%s'\n",str);
	return(USAGE_ONLY);
}

/******************************** USAGE3 ********************************/

int usage3(char *str1,char *str2)
{
	sprintf(errstr,"Insufficient parameters on command line.\n");
	return(USAGE_ONLY);
}

/******************************** INNER_LOOP (redundant)  ********************************/

int inner_loop
(int *peakscore,int *descnt,int *in_start_portion,int *least,int *pitchcnt,int windows_in_buf,dataptr dz)
{
	return(FINISHED);
}

/******************************** INIT_SPECSYNTH (redundant)  ********************************/

void init_specsynth(dataptr dz)
{
//TW Extensive rewrite
	dz->outfile->origrate = dz->infile->origrate  = (int)round(dz->param[SS_SRATE]);
	if(dz->floatsam_output) {
		dz->outfile->stype = SAMP_FLOAT;
		dz->infile->stype = SAMP_FLOAT;
		dz->outfile->origstype = SAMP_FLOAT;
		dz->infile->origstype  = SAMP_FLOAT;
	} else {
		dz->outfile->origstype = SAMP_SHORT;
		dz->infile->origstype  = SAMP_SHORT;
		dz->outfile->stype = SAMP_SHORT;
		dz->infile->stype = SAMP_SHORT;
	}
	dz->wanted			  = SPECSYN_MLEN;
	dz->outfile->channels = SPECSYN_MLEN;
	dz->infile->channels  = SPECSYN_MLEN;
	dz->outfile->Mlen	   = dz->infile->Mlen	   = (dz->outfile->channels-2);	
	dz->wanted = dz->infile->channels;
	dz->clength = dz->wanted/2;
	dz->nyquist = dz->outfile->origrate / 2;		
	dz->outfile->Dfac  = dz->infile->Dfac  = (dz->outfile->channels-2) / 8;
	dz->outfile->arate = dz->infile->arate = (float) dz->outfile->origrate / (float)dz->outfile->Dfac;
	dz->outfile->srate = dz->infile->srate = (int) dz->outfile->arate;
	dz->frametime = (float)(1.0/dz->outfile->arate);
	dz->wlength = (int)round(dz->param[SS_DUR] * dz->outfile->arate);
	dz->chwidth = dz->nyquist/(double)dz->clength;
	
}

//TW UPDATE : NEW FUNCTIONS below
/******************************** READ_CLICKTRACK_DATA ********************************/

#define CLICKLINE  (0)
#define CLICKTEMPO (1)
#define CLICKMETER (2)
#define CLICKCOUNT (3)
#define CLICKSTYLE (4)

int read_clicktrack_data(char *str,dataptr dz)
{
	int exit_status, init = 1;
	int cnt, linecnt = 0, GP, is_abstime, accent;
	char temp[200], *q, *p;
	int arraysize = BIGARRAY * 10, tarray = BIGARRAY * 10, tcnt = 0;/* ARRAYS V. BIG AS free() fails inside realloc() or on its own */
	double tempo = -1.0, accel = -1.0, time = 0.0, pulse_unit = 0.0, timeincr, lasttime = -HUGE, abstime = 0.0;
	int metre_num = -1, metre_denom = 1, barcnt = 0;
	char style[200];
	double  mintimestep = 0.0;
	FILE *fp;

	if((fp = fopen(str,"r"))==NULL) {			
		sprintf(errstr,"Cannot open clicktrack data file %s\n",str);
		return(DATA_ERROR);
	}
	while(fgets(temp,200,fp)!=NULL) {	 /* READ AND TEST BRKPNT VALS */
		q = temp;
		cnt = 0;
		while(isspace(*q))
			q++;
		GP = 0;
		is_abstime = 0;
		accent = 0;
		timeincr = 0.0;
		if(*q == ENDOFSTR || *q == ';')	/* ignore blank lines and comments */
			continue;
		while(get_word_from_string(&q,&p)) {
			switch(cnt) {
			case(CLICKLINE):
				if((exit_status = getlineno(p,tcnt+1,linecnt+1)) < 0)
					return(exit_status);
				break;
			case(CLICKTEMPO):
				if((exit_status = gettempo(p,&tempo,&pulse_unit,&accel,linecnt,&GP,&is_abstime)) < 0)
					return(exit_status);
				break;
			case(CLICKMETER):
				if((exit_status = getmeter(p,&metre_num,&metre_denom,linecnt,GP,is_abstime,&abstime,&accent)) < 0)
					return(exit_status);
				break;
			case(CLICKCOUNT):
				if((exit_status = getcount(p,&barcnt,linecnt,GP,is_abstime,&timeincr)) < 0)
					return(exit_status);
				break;
			case(CLICKSTYLE):
				if((exit_status = getstyle(p,style,metre_num,linecnt,GP,is_abstime)) < 0)
					return(exit_status);
				break;
			default:
				sprintf(errstr,"Too many data items on line %d\n",linecnt+1);
				return(DATA_ERROR);
			}
			cnt++;
		}
		if(cnt < 5) {
			switch(cnt) {
			case(3):
				if(!is_abstime) {
					sprintf(errstr,"Too few data items on line %d\n",linecnt+1);
					return(DATA_ERROR);
				}
				break;
			case(4):
				if(!GP && ((exit_status = default_style(style,metre_num)) < 0))
					return(exit_status);
				break;
			default:
				sprintf(errstr,"Too few data items on line %d\n",linecnt+1);
				return(DATA_ERROR);
			}
		}
		if ((exit_status = generate_clickdata(init,&arraysize,tempo,pulse_unit,metre_num,metre_denom,style,barcnt,
				accel,&time,&lasttime,GP,is_abstime,abstime,accent,timeincr,&tcnt,&tarray,&mintimestep,dz)) < 0)
			return(exit_status);
		init = 0;
		linecnt++;
	}
	dz->parray[2][tcnt] = lasttime;
	dz->iparam[CLICKTIME] = linecnt;
	dz->duration = (double)linecnt;	/* tells cdparams about range of param 3 */
	return(FINISHED);
}

/******************************** READ_CHORD_DATA ********************************/

int read_chord_data(char *str,dataptr dz)
{
	int cnt = 0, exit_status;
	char temp[200], *q;
	double maxmidi = 127.0, minmidi;
	double maxfrq = miditohz(127);
	double minfrq = 1.0/MAX_SYN_DUR;
	double d;
	FILE *fp;
	if((exit_status = hztomidi(&minmidi,minfrq)) < 0)
		return exit_status;

	if((fp = fopen(str,"r"))==NULL) {			
		sprintf(errstr,"Cannot open chord data file %s\n",str);
		return(DATA_ERROR);
	}
	dz->ringsize = 1;
	while(fgets(temp,200,fp)!=NULL) {	 /* READ AND TEST BRKPNT VALS */
		q = temp;
		while(get_float_from_within_string(&q,&d)) {
			switch(dz->mode) {
			case(0):	/* MIDI */
				if(d < minmidi || d > maxmidi) {
					sprintf(errstr,"Value %d (%lf) is out of range.\n",cnt+1,d);
					return(DATA_ERROR);
				}
				break;
			case(1):	/* FRQ */
				if(d < minfrq || d > maxfrq) {
					sprintf(errstr,"Value %d (%lf) is out of range.\n",cnt+1,d);
					return(DATA_ERROR);
				}
				break;
			case(2):	/* MIDI, CHORD SEQUENCE */
				if(d < -99.0)
					dz->ringsize++;
				else if(d < minmidi || d > maxmidi) {
					sprintf(errstr,"Value %d (%lf) is out of range.\n",cnt+1,d);
					return(DATA_ERROR);
				}
				break;
			}
			cnt++;
		}
	}
	if(cnt == 0) {
		sprintf(errstr,"No data found in file.\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[1] = (double *)malloc(cnt * sizeof(double)))==NULL) {
		sprintf(errstr,"Insufficient memory to store MIDI values.\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[2] = (double *)malloc(cnt * sizeof(double)))==NULL) {
		sprintf(errstr,"Insufficient memory to store table step values.\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[3] = (double *)malloc(cnt * sizeof(double)))==NULL) {
		sprintf(errstr,"Insufficient memory to store sample interpolators.\n");
		return(MEMORY_ERROR);
	}
	if((dz->lparray[0] = (int *)malloc(cnt * sizeof(int)))==NULL) {
		sprintf(errstr,"Insufficient memory to store samplecounters.\n");
		return(MEMORY_ERROR);
	}
	fseek(fp,0,0);
	cnt = 0;
	while(fgets(temp,200,fp)!=NULL) {	 /* READ AND TEST BRKPNT VALS */
		q = temp;
		while(get_float_from_within_string(&q,&d)) {
			switch(dz->mode) {
			case(0):
				d = miditohz(d);
				break;
			case(2):
				if(d < -99.0)
					d = 0.0;
				else
					d = miditohz(d);
				break;
			}
			dz->parray[1][cnt] = d;
			cnt++;
		}
	}
	dz->itemcnt = cnt;
	for(cnt=0;cnt<dz->itemcnt;cnt++) {
		dz->parray[2][cnt] = 0.0;	/* initialise all table increments and positions */
		dz->parray[3][cnt] = 0.0;
		dz->lparray[0][cnt] = 0;
	}
	return(FINISHED);
}

/*********************************** GETTEMPO ***********************************
 *
 * Expected format 1=144 OR 1.5=134.567 [or "G" (or "g") as part of "G P"]
 */

int gettempo(char *str,double *tempo,double *pulse_unit,double *accel,int linecnt,int *GP,int *is_abstime)
{
	char *p;
	p = str;
	if(!strcmp(str,"GP")) {
		*GP = 1;
		return(FINISHED);
	} else if(!strcmp(str,"TIME")) {
		if(linecnt == 0) {
			sprintf(errstr,"'TIME' dataline too soon: You cannot reset the Absolute time before you start the clickdata.\n");
			return(DATA_ERROR);
		}
		*is_abstime = 1;
		return(FINISHED);
	} else if(!strcmp(str,".")) {
		if(*accel >= 0.0) {
			sprintf(errstr,"Bad tempo data at line %d : Cannot use a DOT (for same tempo) when previous tempo was changing\n",linecnt+1);
			return(DATA_ERROR);
		} else if(*tempo < 0.0) {
			sprintf(errstr,"Bad tempo data at line %d : Cannot use a DOT (for same tempo) when no previous tempo established\n",linecnt+1);
			return(DATA_ERROR);
		}
		return(FINISHED);
	}
	if(!strgetfloat_within_string(&p,pulse_unit)) {
		sprintf(errstr,"Bad tempo data at line %d : Cannot get length of pulse unit (e.g. 1 = crotchet)\n",linecnt+1);
		return(DATA_ERROR);
	}
	if(!flteq(*pulse_unit,1.0) && !flteq(*pulse_unit,2.0) && !flteq(*pulse_unit,3.0) && !flteq(*pulse_unit,4.0) 
	&& !flteq(*pulse_unit,1.5) && !flteq(*pulse_unit,0.75) && !flteq(*pulse_unit,0.375) && !flteq(*pulse_unit,0.1875)
	&& !flteq(*pulse_unit,0.5) && !flteq(*pulse_unit,0.25) && !flteq(*pulse_unit,0.125))  {
		sprintf(errstr,"Bad tempo data at line %d : Invalid pulse unit (%.3lf)\n",linecnt+1,*pulse_unit);
		return(DATA_ERROR);
	}
	if(*p++ != '=') {
		sprintf(errstr,"Bad tempo data at line %d : '=' missing, or spaces before '='\n",linecnt+1);
		return(DATA_ERROR);
	}
	if(!strgetfloat_within_string(&p,tempo)) {
		sprintf(errstr,"Bad tempo data at line %d : Failed to find metronome mark value\n",linecnt+1);
		return(DATA_ERROR);
	}
	*accel = -1.0;
	if(strlen(p) > 0) {
		if(!strncmp(p,"to",2)) {
			p += 2;
			if(!strgetfloat_within_string(&p,accel)) {
				sprintf(errstr,"Bad tempo data at line %d : Failed to find tempo at end of tempo-change\n",linecnt+1);
				return(DATA_ERROR);
			}
		} else {
			sprintf(errstr,"Bad tempo data at line %d : Spurious characters at end (can be 'to' followed by 2nd tempo only)\n",linecnt+1);
			return(DATA_ERROR);
		}
	}
	return(FINISHED);
}

/*********************************** GETMETER ***********************************
 *
 * Expected format 3:8,  [OR  P (or p) as part of "G P"]
 */

int getmeter(char *str,int *metre_num,int *metre_denom,int linecnt,int GP,int is_abstime,double *abstime,int *accent)
{
	char *q,*p = str;
	int k, j;
	if(GP) {
		if(!strcmp(str,"1")) {
			*accent = 1;
			return(FINISHED);
		} else if(!strcmp(str,"0"))
			return(FINISHED);
		else {
			sprintf(errstr,"Bad GP data at line %d : no accent indicator (0 or 1)\n",linecnt+1);
			return(DATA_ERROR);
		}
	} else if(is_abstime) {
		if(sscanf(str,"%lf",abstime)!=1) {
			sprintf(errstr,"Bad TIME data at line %d (found '%s')\n",linecnt+1,str);
			return(DATA_ERROR);
		}
		return(FINISHED);
	} else if(!strcmp(str,".")) {
		if(*metre_num < 0) {
			sprintf(errstr,"Bad meter data at line %d : Cannot use a DOT (for same meter) when no previous meter exists\n",linecnt+1);
			return(DATA_ERROR);
		}
		return(FINISHED);
	}
	while(*p != ':') {
		if(!isdigit(*p)) {
			sprintf(errstr,"Bad meter data at line %d : no numerator, or non-numeric character or space in numerator before ':' or missing ':'\n",linecnt+1);
			return(DATA_ERROR);
		}
		p++;
	}
	*p = ENDOFSTR;
	if(sscanf(str,"%d",metre_num)!=1) {
		sprintf(errstr,"Bad meter data at line %d : Failed to find numerator of meter\n",linecnt+1);
		return(DATA_ERROR);
	}
	p++;
	q = p;
	while(*p != ENDOFSTR) {
		if(!isdigit(*p)) {
			sprintf(errstr,"Bad meter data at line %d : non-numeric (or no) characters after ':'\n",linecnt+1);
			return(DATA_ERROR);
		}
		p++;
	}
	if(sscanf(q,"%d",metre_denom)!=1) {
		sprintf(errstr,"Bad meter data at line %d : Failed to find denominator of meter (no spaces permitted after ':')\n",linecnt+1);
		return(DATA_ERROR);
	}
	if((k = *metre_denom) < 2) {
		sprintf(errstr,"Bad meter data at line %d : denominator of meter is not a multiple of 2\n",linecnt+1);
		return(DATA_ERROR);
	}
	while(k > 2)  {
		j = k/2;
		if(k != j * 2) {
			sprintf(errstr,"Bad meter data at line %d : denominator of meter is not a multiple of 2\n",linecnt+1);
			return(DATA_ERROR);
		}
		k = j;
	}
	return(FINISHED);
}

/*********************************** GETSTYLE ***********************************************
 *
 * Expected format 1..... or 1..1.. or 100100  where number of chars = denominator of meter
 */

int getstyle(char *str,char *style,int metre_num,int linecnt,int GP,int is_abstime)
{
	char *p = str;
	if(GP || is_abstime)
		return(FINISHED);
	while(*p != ENDOFSTR) {
		if(*p != '1' && *p != '.' && *p != '0') {
			sprintf(errstr,"Bad style data at line %d : Invalid character (%c) in style string (must be '1','0' or '.'\n",linecnt+1,*p);
			return(DATA_ERROR);
		}
		p++;
	}
	
	if((p - str) != metre_num) {
		sprintf(errstr,"Bad style data at line %d : Wrong number of characters in style-string (must be %d in this meter)\n",linecnt+1,metre_num);
		return(DATA_ERROR);
	}
	strcpy(style,str);
	return(FINISHED);
}

/*********************************** GETLINENO ***********************************************
 *
 * Expected format = digit (increasing from 1)
 */

int getlineno(char *str,int tcnt,int linecnt)
{
	int lineno;
	if(sscanf(str,"%d",&lineno)!=1) {
		sprintf(errstr,"Failed to read line number, at line %d (found '%s')\n",linecnt,str);
		return(DATA_ERROR);
	}
	if(lineno != linecnt) {
		if(lineno == 0) {
			sprintf(errstr,"Lines should be numbered from 1 upwards\n");
			return(DATA_ERROR);
		} else {
			sprintf(errstr,"Line numbers are not in sequence, at line %d\n",linecnt);
			return(DATA_ERROR);
		}
	}
	if(lineno != tcnt) {
		sprintf(errstr,"Anomaly in counting lines, at line %d\n",linecnt);
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/*********************************** GETCOUNT ***********************************************
 *
 * Expected format is an INTEGER
 */

int getcount(char *str,int *barcnt,int linecnt,int GP,int is_abstime,double *timeincr)
{
	char *p = str;
	if(is_abstime)
		return(FINISHED);
	else if(GP) {
		if(sscanf(str,"%lf",timeincr)!=1) {
			sprintf(errstr,"Failed to read duration of General Pause at line %d\n",linecnt+1);
			return(DATA_ERROR);
		} 
		return(FINISHED);
	}
	while(*p != ENDOFSTR) {
		if(!isdigit(*p)) {
			sprintf(errstr,"Bad bar-count data at line %d : Non-numeric character encountered (must be whole number of bars)\n",linecnt+1);
			return(DATA_ERROR);
		}
		p++;
	}
	if(sscanf(str,"%d",barcnt)!=1) {
		sprintf(errstr,"Bad bar-count data at line %d : Failed to read bar-count\n",linecnt+1);
		return(DATA_ERROR);
	}
	if(*barcnt < 1) {
		sprintf(errstr,"Bad bar-count (%d) data at line %d\n",*barcnt,linecnt+1);
		return(DATA_ERROR);
	}
	return(FINISHED);
}

/*********************************** GENERATE_CLICKDATA ***********************************************/

#define ACCELPOW (0.6)
#define RITPOW (1.0/ACCELPOW)

int generate_clickdata
(int init,int *arraysize,double tempo,double pulse_unit,int metre_num,int metre_denom,char *style,int barcnt,double accel,
 double *time,double *lasttime,int GP,int is_abstime,double abstime,int accent,double timeincr,int *tcnt,int *tarray,
 double *mintimestep,dataptr dz)
{
	int n, eventcnt;
	double timestep, unit_tempo, *cliks, pulse_mutiplier;
	double orig_timestep = 0.0, final_timestep, total_beats = 1.0, tstepinc = 0.0, thisincr;
	char *p = style;
	int do_accel, steptype = 1;
	if(init) {
		if((dz->parray[0] = (double *)malloc(*arraysize * sizeof(double)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to make clicktrack data array.\n");
			return(MEMORY_ERROR);
		}
		dz->itemcnt = 0;
		*mintimestep = (double)((CLICKLEN + 1) + 4 /* safety */)/(double)dz->infile->srate;
		if((dz->parray[2] = (double *)malloc(*tarray * sizeof(double)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to store clicktrack data line times.\n");
			return(MEMORY_ERROR);
		}
	}
	cliks = dz->parray[0];
	dz->parray[2][(*tcnt)++] = *time;
	if(*tcnt > *tarray) {
		*tarray += BIGARRAY;
		if((dz->parray[2] = (double *)realloc((char *)dz->parray[2],(*tarray) * sizeof(double)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to store clicktrack data line times.\n");
			return(MEMORY_ERROR);
		}
	}
	if(GP) {
		if(accent) {
			if(dz->itemcnt + 2 >= *arraysize) {
				*arraysize += BIGARRAY;
				if((dz->parray[0] = (double *)realloc((char *)dz->parray[0],*arraysize * sizeof(double)))==NULL) {
					sprintf(errstr,"INSUFFICIENT MEMORY to make expand clicktrack data array.\n");
					return(MEMORY_ERROR);
				}
				cliks = dz->parray[0];
			}		
			cliks[dz->itemcnt++] = *time;
			cliks[dz->itemcnt++] = CLICKAMP1;
		}
		*lasttime = *time;
		*time += timeincr;
		return(FINISHED);
	}
	if(is_abstime) {
		if(abstime < *lasttime) {
			sprintf(errstr,"ABSOLUTE TIME %lf is OUT OF SEQUENCE.\n",abstime);	/*RWD 12:2003 added abstime arg */
			return(DATA_ERROR);
		}
		*time = abstime;
		return(FINISHED);
	}
	pulse_mutiplier = (double)metre_denom/4.0;	/* could be .5 if beats are minims */
	unit_tempo = tempo * pulse_unit;			/* crotchets per minute */
	unit_tempo *= pulse_mutiplier;				/* basic pulse-divisions per minute */
	unit_tempo /= 60.0;							/* basic pulse-divisions per second */
	if((timestep = (1.0/unit_tempo)) < *mintimestep) {
		sprintf(errstr,"Tempo too fast to separate clicks in output, at time %lf secs.\n",*time);
		return(DATA_ERROR);
	}
	eventcnt = 0;
	if(accel > 0.0) {						/* in accel or rit case, change tempo logwise */
		if((final_timestep = timestep * (tempo/accel)) < *mintimestep) {
			sprintf(errstr,"Tempo too fast to separate clicks in output, at time %lf secs.\n",*time);
			return(DATA_ERROR);
		}
		total_beats   = barcnt * metre_num;
		if((tstepinc = final_timestep - timestep) < 0) {
			steptype = -1;
			tstepinc = -tstepinc;
		}
		orig_timestep = timestep;
		do_accel = 1;
	} else 
		do_accel = 0;

	for(n = 0; n < barcnt; n++) {			/* for each bar */
		if(dz->itemcnt + (metre_num * 2) >= *arraysize) {
			*arraysize += BIGARRAY;
			if((dz->parray[0] = (double *)realloc((char *)dz->parray[0],*arraysize * sizeof(double)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY to make expand clicktrack data array.\n");
				return(MEMORY_ERROR);
			}
			cliks = dz->parray[0];
		}		
		while(*p != ENDOFSTR) {				/* for each beat in specified pattern */
			switch(*p) {
			case('1'):					
				cliks[dz->itemcnt++] = *time;
				if(p == style)			/* stress first beat */
					cliks[dz->itemcnt++] = CLICKAMP1;
				else					/* half-stress subsidiary accents */
					cliks[dz->itemcnt++] = CLICKAMP2;
				break;
			case('.'):					/* sound other beats with no stress */
				cliks[dz->itemcnt++] = *time;
				cliks[dz->itemcnt++] = CLICKAMP3;
				break;
			/* case('0'):				  ignore unsounded beats */
			}
			if(do_accel) {					/* if tempo changing, increment timestep logwise */
				if(steptype < 0)
					thisincr = pow(((double)eventcnt/(double)total_beats),ACCELPOW) * tstepinc * steptype;
				else
					thisincr = pow(((double)eventcnt/(double)total_beats),RITPOW) * tstepinc * steptype;
				timestep = orig_timestep + thisincr;
				eventcnt++;
			}
			*lasttime = *time;
			*time += timestep;				/* advance absolute time to next beat */
			p++;							/* advance along accent pattern */
		}									
		p = style;							/* at end of pattern, return to start of pattern */
	}
	return(FINISHED);
}

/*********************************** DEFAULT_STYLE ***********************************************/

int default_style(char *style,int metre_num)
{
	char *p = style;
	int triple = 0, n;
	if((((metre_num/3) * 3) == metre_num)  && (metre_num > 3))
		triple = 1;
	*p++ = '1';
	for(n = 1; n < metre_num;n++) {
		if(triple & (n%3 == 0))
			*p = '1';
		else
			*p = '.';
		p++;
	}
	*p = ENDOFSTR;
	return(FINISHED);
}

/*********************************** GENERATE_CLICKTABLE ***********************************************/

int generate_clicktable(dataptr dz)
{
	int n;
	double rand1, rand2, maxval;
	if((dz->parray[1] = (double *)malloc((CLICKLEN +1) * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store click table.\n");
		return(MEMORY_ERROR);
	}
	initrand48();
	maxval = 0.0;
	for(n=0;n < CLICKLEN;n++) {
		rand1 = (drand48() * 2.0) - 1.0;	/* in case I decide to modify it every time !! */
		rand2 = (drand48() * .02) - .01;
		dz->parray[1][n] = sqrt(sin(PI/(double)n)) * ((double)n * 2.0 * PI/(6.0+ rand1)) * rand2 * (MAXSHORT * .98);
						/* 1/2 sin envel, steepened *    sin oscil every 6 or so samps    * noise *  maxsamp adjusted for noise wander */ 
		maxval= max(maxval,fabs(dz->parray[1][n]));
	}
	maxval = MAXSHORT/maxval;
	for(n=0;n < CLICKLEN;n++)
		dz->parray[1][n] *= maxval;
	dz->parray[1][n] = 0.0;
	return(FINISHED);
}

/*********************************** CLICK_CONSISTENCY ***********************************************/

int click_consistency(dataptr dz) 
{
	double lasttime;
	switch(dz->mode) {
	case(CLICK_BY_TIME):
		if(dz->param[CLIKSTART] >= dz->param[CLIKEND]) {
			sprintf(errstr,"Start time for Click Generation is beyond End time.\n");
			return(DATA_ERROR);
		}
		lasttime = dz->parray[0][dz->itemcnt - 2];
		if(dz->param[CLIKSTART] > lasttime - FLTERR) {
			sprintf(errstr,"Start time for Click Generation is beyond end of your data.\n");
			return(DATA_ERROR);
		}
		dz->param[CLIKEND] = min(dz->param[CLIKEND],lasttime);
		break;
	case(CLICK_BY_LINE):
		if(dz->iparam[CLIKSTART] > dz->iparam[CLIKEND]) {
			sprintf(errstr,"Start line for Click Generation is at or beyond End line.\n");
			return(DATA_ERROR);
		}
		if(dz->iparam[CLIKSTART] > dz->iparam[CLICKTIME]) {
			sprintf(errstr,"Start line for Click Generation is beyond end of your data.\n");
			return(DATA_ERROR);
		}
		dz->iparam[CLIKSTART]--;
		dz->iparam[CLIKEND]--;
		dz->iparam[CLIKEND] = min(dz->iparam[CLIKEND],(dz->iparam[CLICKTIME] - 1));
		break;
	}
	dz->iparam[CLIKOFSET]--;	/* convert from "1-N" to "0 to N-1" */
	return(FINISHED);
}
