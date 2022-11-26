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



#include <stdio.h>
#include <stdlib.h>
#include <structures.h>
#include <cdpmain.h>
#include <tkglobals.h>
#include <pnames.h>
#include <repitch.h>
#include <processno.h>
#include <modeno.h>
#include <globcon.h>
#include <logic.h>
#include <filetype.h>
#include <mixxcon.h>
#include <flags.h>
#include <speccon.h>
#include <arrays.h>
#include <special.h>
#include <formants.h>
#include <sfsys.h>
#include <osbind.h>
#include <string.h>
#include <math.h>
#include <srates.h>
#include <pvoc.h>

//TW UPDATES
#include <vowels.h>
#include <ctype.h>


/********************************************************************************************/
/********************************** FORMERLY IN pconsistency.c ******************************/
/********************************************************************************************/

static int 	check_consistency_of_pitch_params(dataptr dz);
static int 	check_consistency_of_track_params(dataptr dz);
static void establish_datareduction(int k,dataptr dz);
static int	check_compatibility_pinvert_params(dataptr dz);
static int 	check_validity_of_prandomise_params(dataptr dz);
static int 	check_consistency_of_pcut_params(dataptr dz);
static int	check_compatibility_of_pfix_smoothing_flags(dataptr dz);
static int  check_for_valid_transposition_ratios(double,double,dataptr);

/********************************************************************************************/
/********************************** FORMERLY IN preprocess.c ********************************/
/********************************************************************************************/

static int  specpitch_preprocess(dataptr dz);
static int  adjust_parameters_for_specpitch(dataptr dz);
static int  establish_internal_arrays_for_pitchwork(dataptr dz);
static int  establish_arrays_for_pitchwork(dataptr dz);
static int  track_preprocess(dataptr dz);
static int  convert_track_params(dataptr dz);
static int  setup_internal_params_and_arrays_for_track(dataptr dz);
static int  papprox_preprocess(dataptr dz);
static int  pexag_preprocess(dataptr dz);
static int  pinvert_preprocess(dataptr dz);
static int  pquantise_preprocess(dataptr dz);
static int  adjust_params_and_setup_internal_params_for_prandomise(dataptr dz);
static int  adjust_params_and_setup_internal_params_for_psmooth(dataptr dz);
static int  adjust_params_for_ptranspose(dataptr dz);
static int  pvibrato_preprocess(dataptr dz);
static int  pfix_preprocess(dataptr dz);
static int  repitchb_preprocess(dataptr dz);

/********************************************************************************************/
/********************************** FORMERLY IN specialin.c *********************************/
/********************************************************************************************/

static int  get_transposition_value(double val,int datatype,dataptr dz);
static int  convert_constant_to_transposition_table(double val,dataptr dz);
static int  getsize_and_getdata_from_transpos_brkfile
			(char *filename,double minval, double maxval,int which_type, dataptr dz);
static int  read_pquantise_set(char *filename,dataptr dz);
static int  sort_quantising_set(dataptr dz);
static int  generate_octave_duplicates(dataptr dz);
static int  eliminate_duplicates_in_quantising_set(dataptr dz);
static int  read_interval_mapping(char *str,dataptr dz);
static int  get_mapping(char *filename,dataptr dz);
static int  sort_mapping(dataptr dz);
static int  get_and_count_data_from_textfile(char *filename,double **brktable,int *brksize);
static int get_and_count_timedata_from_textfile(char *filename,int **brktable1,int **brktable2,int *brksize,dataptr dz);
//TW UPDATES
static int  get_the_pitches(char *filename,double **times,double **pitch,int *pcnt);
static int  get_pitch(char *str,double *pitch);

/***************************************************************************************/
/****************************** FORMERLY IN aplinit.c **********************************/
/***************************************************************************************/

/***************************** ESTABLISH_BUFPTRS_AND_EXTRA_BUFFERS **************************/

int establish_bufptrs_and_extra_buffers(dataptr dz)
{
	dz->extra_bufcnt = -1;	/* ENSURE EVERY CASE HAS A PAIR OF ENTRIES !! */
	dz->bptrcnt = 0;
	dz->bufcnt  = 0;
	switch(dz->process) {
	case(PITCH):      		dz->extra_bufcnt =  3; dz->bptrcnt = 2; 	break;
	case(TRACK):      		dz->extra_bufcnt =  3; dz->bptrcnt = 2; 	break;
	case(P_APPROX):   		dz->extra_bufcnt =  0; dz->bptrcnt = 0; 	break;
	case(P_EXAG): 	  		dz->extra_bufcnt =  0; dz->bptrcnt = 0; 	break;
	case(P_INVERT):   		dz->extra_bufcnt =  0; dz->bptrcnt = 0; 	break;
	case(P_QUANTISE): 		dz->extra_bufcnt =  0; dz->bptrcnt = 0; 	break;
	case(P_RANDOMISE):		dz->extra_bufcnt =  0; dz->bptrcnt = 0; 	break;
	case(P_SMOOTH):   		dz->extra_bufcnt =  0; dz->bptrcnt = 0; 	break;
	case(P_TRANSPOSE):		dz->extra_bufcnt =  0; dz->bptrcnt = 0; 	break;
	case(P_VIBRATO):  		dz->extra_bufcnt =  0; dz->bptrcnt = 0; 	break;
	case(P_CUT): 	  		dz->extra_bufcnt =  0; dz->bptrcnt = 0; 	break;
	case(P_FIX):      		dz->extra_bufcnt =  0; dz->bptrcnt = 0; 	break;
	case(REPITCH):    		dz->extra_bufcnt =  0; dz->bptrcnt = 0; 	break;
	case(REPITCHB):   		dz->extra_bufcnt =  0; dz->bptrcnt = 0; 	break;
	case(TRNSP):      		dz->extra_bufcnt =  1; dz->bptrcnt = 1; 	break;
	case(TRNSF):      		dz->extra_bufcnt =  1; dz->bptrcnt = 1; 	break;
//TW UPDATES
	case(P_SYNTH): 			dz->extra_bufcnt =  0; dz->bptrcnt = 2; 	break;
	case(P_VOWELS): 		dz->extra_bufcnt =  0; dz->bptrcnt = 2; 	break;
	case(P_GEN): 			dz->extra_bufcnt =  0; dz->bptrcnt = 0; 	break;
	case(P_INTERP): 		dz->extra_bufcnt =  0; dz->bptrcnt = 0; 	break;
	case(P_INSERT):			dz->extra_bufcnt =  0; dz->bptrcnt = 0; 	break;
	case(P_SINSERT):		dz->extra_bufcnt =  0; dz->bptrcnt = 0; 	break;
	case(P_PTOSIL):			dz->extra_bufcnt =  0; dz->bptrcnt = 0; 	break;
	case(P_NTOSIL):			dz->extra_bufcnt =  0; dz->bptrcnt = 0; 	break;
	case(ANALENV):			dz->extra_bufcnt =  0; dz->bptrcnt = 4; 	break;
	case(P_BINTOBRK):		dz->extra_bufcnt =  0; dz->bptrcnt = 0; 	break;

	default:
		sprintf(errstr,"Unknown program type [%d] in establish_bufptrs_and_extra_buffers()\n",dz->process);
		return(PROGRAM_ERROR);
	}

	if(dz->extra_bufcnt < 0) {
		sprintf(errstr,"bufcnts have not been set: establish_bufptrs_and_extra_buffers()\n");
		return(PROGRAM_ERROR);
	}
	return establish_spec_bufptrs_and_extra_buffers(dz);
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
	case(PITCH):  	dz->array_cnt =2;  dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(TRACK):  	dz->array_cnt =2;  dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(P_APPROX): dz->array_cnt =1;  dz->iarray_cnt =1;  dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(P_EXAG): 	dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(P_INVERT): dz->array_cnt =1;  dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(P_QUANTISE): dz->array_cnt =1;	 dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(P_RANDOMISE):dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(P_SMOOTH): dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(P_TRANSPOSE):dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(P_VIBRATO):  dz->array_cnt =1;	 dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(P_CUT): 	dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(P_FIX):    dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(REPITCH): 	dz->array_cnt =1;  dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(REPITCHB):	dz->array_cnt =1;  dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(TRNSP):  	dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(TRNSF):  	dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
//TW UPDATES
	case(P_SYNTH):  dz->array_cnt = 1; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(P_VOWELS): dz->array_cnt = 1; dz->iarray_cnt = 1; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(P_GEN): 	dz->array_cnt = 2; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(P_INSERT): dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 2; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(P_SINSERT): dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 2; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(P_PTOSIL):	dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(P_NTOSIL):	dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(ANALENV):	dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(P_BINTOBRK): dz->array_cnt=1; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(P_INTERP):	dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;

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
		if((dz->fptr = (float  **)malloc(dz->fptr_cnt * sizeof(float *)))==NULL) {
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
	case(PITCH):		setup_process_logic(ANALFILE_ONLY,		  	EQUAL_ANALFILE,		ANALFILE_OUT,	dz);	break;
	case(TRACK):		setup_process_logic(ANALFILE_ONLY,		  	EQUAL_ANALFILE,		ANALFILE_OUT,	dz);	break;
	case(P_APPROX):	  	setup_process_logic(PITCHFILE_ONLY,		  	PITCH_TO_BIGPITCH,	PITCH_OUT,		dz);	break;
	case(P_EXAG):	  	setup_process_logic(PITCHFILE_ONLY,		  	PITCH_TO_PITCH,		PITCH_OUT,		dz);	break;
	case(P_INVERT):	  	setup_process_logic(PITCHFILE_ONLY,		  	PITCH_TO_PITCH,		PITCH_OUT,		dz);	break;
	case(P_QUANTISE): 	setup_process_logic(PITCHFILE_ONLY,		  	PITCH_TO_PITCH,		PITCH_OUT,		dz);	break;
	case(P_RANDOMISE):	setup_process_logic(PITCHFILE_ONLY,		  	PITCH_TO_PITCH,		PITCH_OUT,		dz);	break;
	case(P_SMOOTH):	  	setup_process_logic(PITCHFILE_ONLY,		  	PITCH_TO_PITCH,		PITCH_OUT,		dz);	break;
	case(P_TRANSPOSE):	setup_process_logic(PITCHFILE_ONLY,		  	PITCH_TO_PITCH,		PITCH_OUT,		dz);	break;
	case(P_VIBRATO):  	setup_process_logic(PITCHFILE_ONLY,		  	PITCH_TO_PITCH,		PITCH_OUT,		dz);	break;
	case(P_CUT):	  	setup_process_logic(PITCHFILE_ONLY,		  	PITCH_TO_BIGPITCH,	PITCH_OUT,		dz);	break;
//TW UPDATES
	case(P_SYNTH):		setup_process_logic(PITCHFILE_ONLY,		  	PITCH_TO_ANAL,		ANALFILE_OUT,	dz);	break;
	case(P_VOWELS):		setup_process_logic(PITCHFILE_ONLY,		  	PITCH_TO_ANAL,		ANALFILE_OUT,	dz);	break;
	case(P_INTERP):		setup_process_logic(PITCHFILE_ONLY,		  	PITCH_TO_PITCH,		PITCH_OUT,		dz);	break;
	case(P_GEN):		setup_process_logic(NO_FILE_AT_ALL,		  	PITCH_TO_BIGPITCH,	PITCH_OUT,		dz);	break;
	case(P_INSERT):		setup_process_logic(PITCHFILE_ONLY,		  	PITCH_TO_PITCH,		PITCH_OUT,		dz);	break;
	case(P_SINSERT):	setup_process_logic(PITCHFILE_ONLY,		  	PITCH_TO_PITCH,		PITCH_OUT,		dz);	break;
	case(P_PTOSIL):		setup_process_logic(PITCHFILE_ONLY,		  	PITCH_TO_PITCH,		PITCH_OUT,		dz);	break;
	case(P_NTOSIL):		setup_process_logic(PITCHFILE_ONLY,		  	PITCH_TO_PITCH,		PITCH_OUT,		dz);	break;
	case(ANALENV):		setup_process_logic(ANALFILE_ONLY,		  	UNEQUAL_ENVFILE,	ENVFILE_OUT,	dz);	break;

	case(P_BINTOBRK):	setup_process_logic(PITCHFILE_ONLY,		  	TO_TEXTFILE,		TEXTFILE_OUT,	dz);	break;
	case(P_FIX):	  	setup_process_logic(PITCHFILE_ONLY,		  	PITCH_TO_PITCH,		PITCH_OUT,		dz);	break;
	case(REPITCH):
		switch(dz->mode) {
		case(PPT): 		setup_process_logic(PITCH_AND_PITCH,	  	PITCH_TO_BIGPITCH,	PITCH_OUT,		dz);	break;
		case(PTP): 		setup_process_logic(PITCH_AND_TRANSPOS,	  	PITCH_TO_BIGPITCH,	PITCH_OUT,		dz);	break;
		case(TTT): 		setup_process_logic(TRANSPOS_AND_TRANSPOS,	PITCH_TO_BIGPITCH,	PITCH_OUT,		dz);	break;
		}
		break;
	case(REPITCHB):
		switch(dz->mode) {
		case(PPT): 		setup_process_logic(PITCH_AND_PITCH,	  	TO_TEXTFILE,		TEXTFILE_OUT,	dz);	break;
		case(PTP): 		setup_process_logic(PITCH_AND_TRANSPOS,	  	TO_TEXTFILE,		TEXTFILE_OUT,	dz);	break;
		case(TTT): 		setup_process_logic(TRANSPOS_AND_TRANSPOS,	TO_TEXTFILE,		TEXTFILE_OUT,	dz);	break;
		}
		break;
	case(TRNSP):
	case(TRNSF):
		switch(dz->mode) {
		case(TRNS_BIN):	setup_process_logic(ANAL_WITH_TRANSPOS,   	EQUAL_ANALFILE,		ANALFILE_OUT,	dz);	break;
		default:		setup_process_logic(ANALFILE_ONLY,	  	  	EQUAL_ANALFILE,		ANALFILE_OUT,	dz);	break;
		}
		break;
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

/***************************************************************************************/
/****************************** FORMERLY IN internal.c *********************************/
/***************************************************************************************/

/****************************** SET_LEGAL_INTERNALPARAM_STRUCTURE *********************************/

int set_legal_internalparam_structure(int process,int mode,aplptr ap)
{
	int exit_status = FINISHED;
	switch(process) {
	case(PITCH):
		switch(mode) {																	   
		case(PICH_TO_BIN):	exit_status = set_internalparam_data("0d",ap);				break;
		case(PICH_TO_BRK):	exit_status = set_internalparam_data( "d",ap);				break;
		}
		break;
	case(TRACK):
		switch(mode) {																	   
		case(TRK_TO_BIN): 	exit_status = set_internalparam_data("0d",ap);				break;
		case(TRK_TO_BRK): 	exit_status = set_internalparam_data( "d",ap);				break;
		}
		break;
	case(P_APPROX):   	return(FINISHED);
	case(P_EXAG): 	  	return(FINISHED);
	case(P_INVERT):   	return(FINISHED);
	case(P_QUANTISE): 	return(FINISHED);
	case(P_RANDOMISE):	exit_status = set_internalparam_data("i",ap);					break;
	case(P_SMOOTH):   	return(FINISHED);
	case(P_TRANSPOSE):	return(FINISHED);
	case(P_VIBRATO):  	return(FINISHED);
	case(P_CUT): 	  	return(FINISHED);
	case(P_FIX): 	  	exit_status = set_internalparam_data("iiii",ap);				break;
	case(REPITCH):    	return(FINISHED);
	case(REPITCHB):   	return(FINISHED);
	case(TRNSP):	  	return(FINISHED);
	case(TRNSF):	  	return(FINISHED);
//TW UPDATES
	case(P_SYNTH):		return(FINISHED);
	case(P_VOWELS):		return(FINISHED);
	case(P_GEN):		return(FINISHED);
	case(P_INSERT):		return(FINISHED);
	case(P_SINSERT):	return(FINISHED);
	case(P_PTOSIL):		return(FINISHED);
	case(P_NTOSIL):		return(FINISHED);
	case(ANALENV):		return(FINISHED);
	case(P_BINTOBRK):	return(FINISHED);
	case(P_INTERP):		return(FINISHED);

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
	int exit_status = FINISHED;
//TW UPDATE
	int n;

	aplptr ap = dz->application;
	double dummy = 0.0,  is_numeric = 0;
	switch(ap->special_data) {
	case(TRANSPOS_RATIO_OR_CONSTANT):
	case(TRANSPOS_OCTAVE_OR_CONSTANT):
 	case(TRANSPOS_SEMIT_OR_CONSTANT):
		if(!sloom) {
//TW NEW FILENAME CONVENTION
			if(!value_is_numeric(str) && file_has_invalid_startchar(str)) {
    			sprintf(errstr,"Filename has an invalid start-character [%s]\n",str);
				return(USER_ERROR);
			}
			if(value_is_numeric(str)) {
				if(sscanf(str,"%lf",&dummy)!=1) {
					sprintf(errstr,"Invalid data: read_special_data()\n");
					return(DATA_ERROR);
				}
				is_numeric = 1;
			}
		} else {
			if(str[0]==NUMERICVAL_MARKER) {				/* TK convention: values preceded by an extra '@' */
				str++;
				if(strlen(str)<=0 || sscanf(str,"%lf",&dummy)!=1) {
					sprintf(errstr,"Invalid transposition data: read_special_data()\n");
					return(DATA_ERROR);
				}
				is_numeric = 1;
			}
		}
		if(is_numeric)
			return get_transposition_value(dummy,(int)ap->special_data,dz);
		else	
			return getsize_and_getdata_from_transpos_brkfile
			(str,(double)MIN_TRANSPOS,(double)MAX_TRANSPOS,(int)ap->special_data,dz);
		break;
	case(PITCHQUANTISE_SET):  	return read_pquantise_set(str,dz);
	case(INTERVAL_MAPPING):	  	return read_interval_mapping(str,dz);
 	case(OUT_PFILE):
		if(!sloom) {
			if((dz->other_file=sndcreat_formatted(str,(dz->insams[0]/dz->wanted),SAMP_FLOAT,
			1,dz->infile->srate,CDP_CREATE_NORMAL)) < 0) {
				sprintf(errstr,"Cannot open output pitch file %s\n",str);
				return(DATA_ERROR);
			}
			dz->needpeaks = 0;
		}
		break;
	case(OUT_PBRKFILE):		  
		if(!sloom) {
			if((dz->fp = fopen(str,"w"))==NULL) {
				sprintf(errstr,"Cannot open file %s for output.\n",str);
				return(DATA_ERROR);
			}
		}
		break;
//TW UPDATES
	case(PITCH_SPECTRUM):		  
		if((exit_status = get_and_count_data_from_textfile(str,&dz->parray[PICH_SPEC],&dz->itemcnt)) < 0)
			return(exit_status);
		for(n=0;n<dz->itemcnt;n++) {
			if(dz->parray[PICH_SPEC][n] < ap->min_special || dz->parray[PICH_SPEC][n] > ap->max_special) {
				sprintf(errstr,"Partial amplitude[%d] = %lf is out of range (%lf - %lf)\n",
				n+1,dz->parray[PICH_SPEC][n],ap->min_special,ap->max_special);
				return(DATA_ERROR);
			}
		}
		break;
	case(ZERO_INSERTTIMES):
		if((exit_status = get_and_count_timedata_from_textfile(str,&dz->lparray[0],&dz->lparray[1],&dz->itemcnt,dz)) < 0)
			return(exit_status);
		break;
	case(PITCH_VOWELS):
		if((exit_status = get_the_vowels(str,&dz->parray[0],&dz->iparray[0],&dz->itemcnt,dz)) < 0)
			return(exit_status);
		break;
	case(PITCH_CREATE):
		if((exit_status = get_the_pitches(str,&dz->parray[0],&dz->parray[1],&dz->itemcnt)) < 0)
			return(exit_status);
		break;

	default:
		sprintf(errstr,"Unknown special_data type: read_special_data()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/********************** GET_TRANSPOSITION_VALUE ************************/

int get_transposition_value(double val,int datatype,dataptr dz)
{
	switch(datatype) {
	case(TRANSPOS_SEMIT_OR_CONSTANT):	val *= OCTAVES_PER_SEMITONE;		/* semitones -> octaves */
	/* fall thro */
	case(TRANSPOS_OCTAVE_OR_CONSTANT):	val = (float)pow(2.0,val);	break;	/* octaves -> frqratios */
	}
	if(val < MIN_TRANSPOS || val > MAX_TRANSPOS) {	 /* graphics - range could be preset */
		sprintf(errstr,"Transposition [%lf] out of range %lf - %lf semitones\n",val,MIN_TRANSPOS,MAX_TRANSPOS);
		return(DATA_ERROR);
	}
	return convert_constant_to_transposition_table(val,dz);
}

/****************************** CONVERT_CONSTANT_TO_TRANSPOSITION_TABLE *********************************
 *
 * convert constant to transposition table.
 */

int convert_constant_to_transposition_table(double val,dataptr dz)
{   int n = 0, arraysize = BIGARRAY;
    double ttime = 0.0;
    double infiletime  = (double)dz->wlength * dz->frametime;
    if((dz->transpos = (float *)malloc(arraysize * sizeof(float)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for transpostion array.\n");
		return(MEMORY_ERROR);
	}
    while(n < dz->wlength) {
        (dz->transpos)[n] = (float)val;
		if(++n >= arraysize) {
	    	arraysize += BIGARRAY;
    	    if((dz->transpos = (float *)realloc((char *)dz->transpos,arraysize*sizeof(float)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY to reallocate transpostion array.\n");
				return(MEMORY_ERROR);
			}
		}
		ttime += dz->frametime;
    }
    while(ttime < infiletime) {
		(dz->transpos)[n] = (float)val;
		if(++n >= arraysize) {
	    	arraysize += BIGARRAY;
    	    if((dz->transpos = (float *)realloc((char *)dz->transpos,arraysize*sizeof(float)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY to reallocate transpostion array.\n");
				return(MEMORY_ERROR);
			}
		}
		ttime += dz->frametime;
    }
    if((dz->transpos = (float *)realloc((char *)dz->transpos,n * sizeof(float)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to reallocate transpostion array.\n");
		return(MEMORY_ERROR);
	}
	return(FINISHED);
}

/********************** GETSIZE_AND_GETDATA_FROM_TRANSPOS_BRKFILE *******************/

int getsize_and_getdata_from_transpos_brkfile(char *filename,double minval, double maxval,int which_type, dataptr dz)
{
	FILE *fp;
	int exit_status;
	int brksize;
	if((fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,	"Can't open brkpntfile %s to read data.\n",filename);
		return(DATA_ERROR);
	}
	if((exit_status = 
	read_and_test_pitch_or_transposition_brkvals(fp,filename,&(dz->temp),&brksize,which_type,minval,maxval))<0)
		return(exit_status);
	if(fclose(fp)<0) {
		fprintf(stdout,"WARNING: Failed to close output textfile %s.\n",filename);
		fflush(stdout);
	}
	if((exit_status = 
	convert_brkpntdata_to_window_by_window_array(dz->temp,brksize,&(dz->transpos),dz->wlength,dz->frametime))<0)
		return(exit_status);
	return(FINISHED);
}

/********************************* READ_PQUANTISE_SET ********************************/

int read_pquantise_set(char *filename,dataptr dz)
{
	int exit_status;
	double *p;
	int n;
	if((exit_status = get_and_count_data_from_textfile(filename,&(dz->parray[PQ_QSET]),&(dz->itemcnt)))<0)
		return(exit_status);
	p = dz->parray[PQ_QSET];
	for(n=0;n<dz->itemcnt;n++) {
		if(*p < MIDIMIN || *p > MIDIMAX) {
			sprintf(errstr,"Pitch value out of range (%d.0 - %d.0) in quantisation set\n",MIDIMIN,MIDIMAX);
		return(DATA_ERROR);
		}
		p++;
	}
	return FINISHED;
}

/************************* SORT_QUANTISING_SET *****************************/

int sort_quantising_set(dataptr dz)
{
	int n, m;
	double thisval;
	for(n=1;n<dz->itemcnt;n++) {
		thisval = dz->parray[PQ_QSET][n];
		m = n-1;
		while(m >= 0 && dz->parray[PQ_QSET][m] > thisval) {
			dz->parray[PQ_QSET][m+1]  = dz->parray[PQ_QSET][m];
			m--;
		}
		dz->parray[PQ_QSET][m+1]  = thisval;
	}
	return(FINISHED);
}

/************************* GENERATE_OCTAVE_DUPLICATES *****************************/

int generate_octave_duplicates(dataptr dz)
{
	int n, m;
	double thisval, octval;
	int duplicated;
	int orig_itemcnt = dz->itemcnt;
	for(n=0;n<orig_itemcnt;n++) {
		thisval = dz->parray[PQ_QSET][n];
		octval = thisval;
		while((octval -= SEMITONES_PER_OCTAVE) >= MIDIMIN) {
			duplicated = FALSE;
		   	for(m=0;m<dz->itemcnt;m++) {
				if(flteq(dz->parray[PQ_QSET][m],octval)) {
					duplicated = TRUE;
					break;
				}
			}		
			if(!duplicated) {
				dz->itemcnt++;
				if((dz->parray[PQ_QSET] = (double *)realloc((char *)dz->parray[PQ_QSET],dz->itemcnt * sizeof(double)))==NULL) {
					sprintf(errstr,"INSUFFICIENT MEMORY for quantisation set array.\n");
					return(MEMORY_ERROR);
				}
				dz->parray[PQ_QSET][dz->itemcnt - 1] = octval;
			}
		}
		octval = thisval;
		while((octval += SEMITONES_PER_OCTAVE) <= MIDIMAX) {
			duplicated = FALSE;
		   	for(m=0;m<dz->itemcnt;m++) {
				if(flteq(dz->parray[PQ_QSET][m],octval)) {
					duplicated = TRUE;
					break;
				}
			}		
			if(!duplicated) {
				dz->itemcnt++;
				if((dz->parray[PQ_QSET] = (double *) realloc((char *)dz->parray[PQ_QSET],dz->itemcnt * sizeof(double)))==NULL) {
					sprintf(errstr,"INSUFFICIENT MEMORY to reallocate quantisation set array.\n");
					return(MEMORY_ERROR);
				}
				dz->parray[PQ_QSET][dz->itemcnt - 1] = octval;
			}
		}
	}
	return(FINISHED);
}

/************************* ELIMINATE_DUPLICATES_IN_QUANTISING_SET *****************************/

int eliminate_duplicates_in_quantising_set(dataptr dz)
{
	int n, m, k;
	for(n=1,m=0;n<dz->itemcnt;n++,m++) { /* eliminate DUPLICATE vals */
		if(flteq(dz->parray[PQ_QSET][n],dz->parray[PQ_QSET][m])) {
			for(k=n;k<dz->itemcnt;k++)
				dz->parray[PQ_QSET][k-1] = dz->parray[PQ_QSET][k];
			n--;
			m--;
			dz->itemcnt--;
		}
	}
	return(FINISHED);
}

/************************** READ_INTERVAL_MAPPING ********************************/

int read_interval_mapping(char *str,dataptr dz)
{
	int exit_status;

	if(!strcmp(str,"0"))
		dz->is_mapping = FALSE;
	else { 
		if((exit_status = get_mapping(str,dz))<0)
			return(exit_status);
		dz->is_mapping = TRUE;
	} 
	return(FINISHED);
}

/********************************* GET_MAPPING ********************************/

int get_mapping(char *filename,dataptr dz)
{
	int exit_status;
	double *p;
	int n;
	if((exit_status = get_and_count_data_from_textfile(filename,&(dz->parray[PI_INTMAP]),&(dz->itemcnt)))<0)
		return(exit_status);
	p = dz->parray[PI_INTMAP];
	for(n=0;n<dz->itemcnt;n++) {
		if(*p < -MAXINTRANGE || *p > MAXINTRANGE) {
			sprintf(errstr,
			"Mapping val (%lf) out of range (%.0lf to -%.0lf semitones) (8 8vas up or down).\n",
			*p,MAXINTRANGE,MAXINTRANGE);
			return(DATA_ERROR);
		}
		p++;
	}
	if(ODD(dz->itemcnt)) {
		sprintf(errstr,"Data not paired correctly in mapping file\n");
		return(DATA_ERROR);
	}
	dz->itemcnt /= 2;
	return sort_mapping(dz);
}

/************************* SORT_MAPPING *****************************
 *
 * sort interval mapping into ascending size of src_interval.
 */

int sort_mapping(dataptr dz)
{
	int n, m, j, k;
	double src_interval, goal_interval;
	for(n=1;n<dz->itemcnt;n++) {
		k = n*2;
		src_interval  = dz->parray[PI_INTMAP][k];
		goal_interval = dz->parray[PI_INTMAP][k+1];
		m = n-1;
		while(m >= 0 && dz->parray[PI_INTMAP][m*2] > src_interval) {
			k = (m+1)* 2;
			j =  m   * 2;
			dz->parray[PI_INTMAP][k]   = dz->parray[PI_INTMAP][j];
			dz->parray[PI_INTMAP][k+1] = dz->parray[PI_INTMAP][j+1];
			m--;
		}
		k = (m+1)*2;
		dz->parray[PI_INTMAP][k]   = src_interval;
		dz->parray[PI_INTMAP][k+1] = goal_interval;
	}
	return(FINISHED);
}

/*************************** GET_AND_COUNT_DATA_FROM_TEXTFILE ******************************
 *
 * (1)	Gets double table data from a text file.
 * (2)	counts ALL items (not pairing them).
 * (3)	Does NOT check for in-range.
 */

int get_and_count_data_from_textfile(char *filename,double **brktable,int *brksize)
{
	FILE *fp;
	double *p;
	int arraysize = BIGARRAY;
	char temp[200], *q;
	int n = 0;
	if((fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,	"Can't open textfile %s to read data.\n",filename);
		return(DATA_ERROR);
	}
	if((*brktable = (double *)malloc(arraysize * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for data.\n");
		return(MEMORY_ERROR);
	}
	p = *brktable;
	while(fgets(temp,200,fp)==temp) {
		q = temp;
		while(get_float_from_within_string(&q,p)) {
			p++;
			if(++n >= arraysize) {
				arraysize += BIGARRAY;
				if((*brktable = (double *)realloc((char *)(*brktable),arraysize * sizeof(double)))==NULL) {
					sprintf(errstr,"INSUFFICIENT MEMORY to reallocate data table.\n");
					return(MEMORY_ERROR);
				}
				p = *brktable + n;		
			}
		}
	}	    
	if(n == 0) {
		sprintf(errstr,"No data in textdata file %s\n",filename);
		return(DATA_ERROR);
	}
	if((*brktable = (double *)realloc((char *)(*brktable),n * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to reallocate data table.\n");
		return(MEMORY_ERROR);
	}
	if(fclose(fp)<0) {
		fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
		fflush(stdout);
	}
	*brksize = n;
	return(FINISHED);
}


/*************************** GET_AND_COUNT_TIMEDATA_FROM_TEXTFILE ******************************
 *
 * (1)	Gets double table data from a text file.
 * (2)	counts ALL items (not pairing them).
 * (3)	Does NOT check for in-range.
 */

int get_and_count_timedata_from_textfile(char *filename,int **brktable1,int **brktable2,int *brksize,dataptr dz)
{
	FILE *fp;
	double dummy[2];
	int *p[2];
	int arraysize = BIGARRAY;
	char temp[200], *q;
	int n = 0;
	int thistable;
	aplptr ap = dz->application;

	if((fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,	"Can't open textfile %s to read data.\n",filename);
		return(DATA_ERROR);
	}
	if((*brktable1 = (int *)malloc(arraysize * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for data.\n");
		return(MEMORY_ERROR);
	}
	if((*brktable2 = (int *)malloc(arraysize * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for data.\n");
		return(MEMORY_ERROR);
	}
	p[0] = *brktable1;
	p[1] = *brktable2;
	thistable = 0;
	while(fgets(temp,200,fp)==temp) {
		q = temp;
		while(get_float_from_within_string(&q,&(dummy[thistable]))) {
			if(thistable) {
				if(dummy[0] < ap->min_special || dummy[0] > ap->max_special) {
					sprintf(errstr,"Time (%lf) in datafile out of range (%lf %lf)\n",
					dummy[0],ap->min_special,ap->max_special);
					return(DATA_ERROR);
				}
				if(dummy[1] < ap->min_special || dummy[1] > ap->max_special) {
					sprintf(errstr,"Time (%lf) in datafile out of range (%lf %lf)\n",
					dummy[1],ap->min_special,ap->max_special);
					return(DATA_ERROR);
				}
				if(flteq(dummy[0],dummy[1])) {
					sprintf(errstr,"Times in datafile equivalent (%lf %lf)\n",dummy[0],dummy[1]);
					return(DATA_ERROR);
				}
				if(dummy[0] > dummy[1]) {
					sprintf(errstr,"Times in datafile reversed (%lf %lf)\n",dummy[0],dummy[1]);
					return(DATA_ERROR);
				}
				
				if(dz->mode == 1) {	/* info as sample-cnt, convert to time */
					dummy[0] /= (double)dz->infile->origrate;
					dummy[1] /= (double)dz->infile->origrate;
				}
				*(p[0]) = (int)round(dummy[0] * dz->infile->arate);	/* convert from time to window number */
				*(p[1]) = (int)round(dummy[1] * dz->infile->arate);
				p[0]++;
				p[1]++;
				if(++n >= arraysize) {
					arraysize += BIGARRAY;
					if((*brktable1 = (int *)realloc((char *)(*brktable1),arraysize * sizeof(int)))==NULL) {
						sprintf(errstr,"INSUFFICIENT MEMORY to reallocate data table.\n");
						return(MEMORY_ERROR);
					}
					if((*brktable2 = (int *)realloc((char *)(*brktable2),arraysize * sizeof(int)))==NULL) {
						sprintf(errstr,"INSUFFICIENT MEMORY to reallocate data table.\n");
						return(MEMORY_ERROR);
					}
					p[0] = *brktable1 + n;		
					p[1] = *brktable2 + n;		
				}
			}
			thistable = !thistable;
		}
	}	    
	if(n == 0) {
		sprintf(errstr,"No data in textdata file %s\n",filename);
		return(DATA_ERROR);
	}
	if(thistable) {
		sprintf(errstr,"data in textdata file %s not paired correctly\n",filename);
		return(DATA_ERROR);
	}
	if((*brktable1 = (int *)realloc((char *)(*brktable1),n * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to reallocate data table.\n");
		return(MEMORY_ERROR);
	}
	if((*brktable2 = (int *)realloc((char *)(*brktable2),n * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to reallocate data table.\n");
		return(MEMORY_ERROR);
	}
	if(fclose(fp)<0) {
		fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
		fflush(stdout);
	}
	*brksize = n;
	return(FINISHED);
}

//TW UPDATE : NEW FUNCTION
/******************************** GET_THE_PITCHES ********************************/

int get_the_pitches(char *filename,double **times,double **pitch,int *pcnt)
{
	FILE *fp;
	double *t, lasttime = 0.0;
	double *pitchval;
	int non_zero_start = 0;
	int arraysize = BIGARRAY, n = 0;
	char temp[200], *q, *p;
	int istime;

	if((fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,	"Can't open textfile %s to read pitch data.\n",filename);
		return(DATA_ERROR);
	}
	if((*times = (double *)malloc(arraysize * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for time data.\n");
		return(MEMORY_ERROR);
	}
	if((*pitch = (double *)malloc(arraysize * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for vowels data.\n");
		return(MEMORY_ERROR);
	}
	t = *times;
	pitchval = *pitch;
	istime = 1;
	while(fgets(temp,200,fp)==temp) {
		q = temp;
		while(get_word_from_string(&q,&p)) {
			if(istime) {
				if(sscanf(p,"%lf",t)!=1) {
					sprintf(errstr,"No time for time-pitch pair %d\n",n+1);
					return(DATA_ERROR);
				}
				if(n==0) {
					if(*t < 0.0) {
						sprintf(errstr,"First time is less than zero in pitch data\n");
						return(DATA_ERROR);
					} else if(*t > 0.0) {
						print_outwarning_flush("FIRST TIME in pitch data IS NOT ZERO : assuming first pitch runs from time zero\n");
						non_zero_start = 1;
						t++;
						*t = *(t-1);
						*(t-1) = 0.0;
					}
				} else {
					if (*t <= lasttime) {
						sprintf(errstr,"Times do not advance (from %lf to %lf at pair %d) in pitch data\n",
						lasttime,*t,n+1);
						return(DATA_ERROR);
					}
				}
				lasttime = *t++;
			} else {
				if(get_pitch(p,pitchval)<0) {
					strcat(errstr,p);
					sprintf(temp,"'  is a bad pitch value at item %d\n",n+1);
					strcat(errstr,temp);
					return(DATA_ERROR);
				}
				if((n==0) && non_zero_start) {
					pitchval++;
					*pitchval = *(pitchval-1);
					n++;
				}
				pitchval++;
				if(++n >= arraysize) {
					arraysize += BIGARRAY;
					if((*times = (double *)realloc((char *)(*times),arraysize * sizeof(double)))==NULL) {
						sprintf(errstr,"INSUFFICIENT MEMORY to reallocate table of pitch-times.\n");
						return(MEMORY_ERROR);
					}
					if((*pitch = (double *)realloc((char *)(*pitch),arraysize * sizeof(double)))==NULL) {
						sprintf(errstr,"INSUFFICIENT MEMORY to reallocate table of pitches.\n");
						return(MEMORY_ERROR);
					}
					t = *times + n;		
					pitchval = *pitch + n;		
				}
			}
			istime = !istime;
		}
	}
	if(n < 2) {
		sprintf(errstr,"Insufficient data in pitch datafile %s\n",filename);
		return(DATA_ERROR);
	}
	if(!istime) {
		sprintf(errstr,"data in  pitch datafile %s not paired correctly\n",filename);
		return(DATA_ERROR);
	}
	if((*times = (double *)realloc((char *)(*times),n * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to reallocate vowel-time table.\n");
		return(MEMORY_ERROR);
	}
	if((*pitch = (double *)realloc((char *)(*pitch),n * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to reallocate vowels table.\n");
		return(MEMORY_ERROR);
	}
	if(fclose(fp)<0) {
		fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
		fflush(stdout);
	}
	*pcnt = n;
	return(FINISHED);
}

/********************************************************************************************/
/********************************** FORMERLY IN preprocess.c ********************************/
/********************************************************************************************/

/****************************** PARAM_PREPROCESS *********************************/

int param_preprocess(dataptr dz)	
{
	switch(dz->process) {
	case(PITCH):		return specpitch_preprocess(dz);
	case(TRACK):		return track_preprocess(dz);
	case(P_APPROX):		return papprox_preprocess(dz);
	case(P_EXAG):		return pexag_preprocess(dz);
	case(P_INVERT):		return pinvert_preprocess(dz);
	case(P_QUANTISE): 	return pquantise_preprocess(dz);
	case(P_RANDOMISE):	return adjust_params_and_setup_internal_params_for_prandomise(dz);
	case(P_SMOOTH):		return adjust_params_and_setup_internal_params_for_psmooth(dz);
	case(P_TRANSPOSE):	return adjust_params_for_ptranspose(dz);
	case(P_VIBRATO):	return pvibrato_preprocess(dz);
	case(P_FIX):		return pfix_preprocess(dz);
	case(REPITCHB):	    return repitchb_preprocess(dz);
	case(P_CUT):	case(REPITCH):	case(TRNSP):	case(TRNSF):
//TW UPDATES
	case(P_SYNTH):
	case(P_VOWELS):
	case(P_GEN):
	case(P_INSERT):
	case(P_SINSERT):
	case(P_PTOSIL):
	case(P_NTOSIL):
	case(ANALENV):
	case(P_BINTOBRK):
	case(P_INTERP):

		return(FINISHED);
	default:
		sprintf(errstr,"PROGRAMMING PROBLEM: Unknown process in param_preprocess()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);	/* NOTREACHED */
}

/************************** SPECPITCH_PREPROCESS ******************************/

int specpitch_preprocess(dataptr dz)
{
	int exit_status;
	if((exit_status = adjust_parameters_for_specpitch(dz))<0)
		return(exit_status);
	if(dz->mode == PICH_TO_BRK) {
		if((dz->parray[PICH_PBRK] = (double *)malloc(dz->wlength * 2 * sizeof(double)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for pitchbrk array.\n");
			return(MEMORY_ERROR);
		}
	}
	if((exit_status = establish_internal_arrays_for_pitchwork(dz))<0)
		return(exit_status);
	initrand48();		
	dz->param[PICH_PICH] = 0.0;
	return setup_ring(dz);
}

/************ ADJUST_PARAMETERS_FOR_SPECPITCH *************/

int adjust_parameters_for_specpitch(dataptr dz)
{
	/* convert semitone to ratio */
   	dz->param[PICH_RNGE] = pow(SEMITONE_INTERVAL,fabs(dz->param[PICH_RNGE])); 
	/* convert dB to gain */		
	dz->param[PICH_SRATIO] /= 20.0;
	dz->param[PICH_SRATIO] = pow(10.0,dz->param[PICH_SRATIO]);
	dz->param[PICH_SRATIO] = 1.0/dz->param[PICH_SRATIO];
	return(FINISHED);
}

/************ ESTABLISH_INTERNAL_ARRAYS_FOR_PITCHWORK *************/

int establish_internal_arrays_for_pitchwork(dataptr dz)
{
	int exit_status;
	if((exit_status = establish_arrays_for_pitchwork(dz))<0)
		return(exit_status);
	if((exit_status = establish_bottom_frqs_of_channels(dz))<0)
		return(exit_status);
	return establish_testtone_amps(dz);
}

/***************************** ESTABLISH_ARRAYS_FOR_PITCHWORK *************************/

int establish_arrays_for_pitchwork(dataptr dz)
{
	int i;
	if((dz->parray[PICH_PRETOTAMP] = (double *)malloc(dz->wlength * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for amp totalling array.\n");
		return(MEMORY_ERROR);
	}
//RWD NOV97	 quick solution to unassigned value somewhere in spec pitch!
	for(i = 0; i < dz->wlength;i++)
		dz->parray[PICH_PRETOTAMP][i] = 0.0;

	if((dz->pitches = (float *)malloc(dz->wlength * sizeof(float)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for pitches array.\n");
		return(MEMORY_ERROR);
	}
	return(FINISHED);
}

/************************** TRACK_PREPROCESS ******************************/

int track_preprocess(dataptr dz)
{
	int exit_status;
	if((exit_status = convert_track_params(dz))<0)
		return(exit_status);
	if((exit_status = setup_internal_params_and_arrays_for_track(dz))<0)
		return(exit_status);
	initrand48();		
	return establish_internal_arrays_for_pitchwork(dz);
}

/************ CONVERT_TRACK_PARAMS *************/

int convert_track_params(dataptr dz)
{	/* convert semitone to ratio */
   	dz->param[TRAK_RNGE] = pow(SEMITONE_INTERVAL,fabs(dz->param[TRAK_RNGE]));
	/* convert dB to gain */		
	dz->param[TRAK_SRATIO] /= 20.0;
	dz->param[TRAK_SRATIO] = pow(10.0,dz->param[TRAK_SRATIO]);
	dz->param[TRAK_SRATIO] = 1.0/dz->param[TRAK_SRATIO];
	return(FINISHED);
}

/************ SETUP_INTERNAL_PARAMS_AND_ARRAYS_FOR_TRACK *************/

int setup_internal_params_and_arrays_for_track(dataptr dz)
{		/* setup internal arrays */
	dz->param[TRAK_LOLM] = MINPITCH;
	if(dz->mode == TRK_TO_BRK)	{
		if((dz->parray[PICH_PBRK] = (double *)malloc(dz->wlength * 2 * sizeof(double)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for pitch brk array.\n");
			return(MEMORY_ERROR);
		}
	}
	return(FINISHED);
}

/************************** PAPPROX_PREPROCESS ******************************/

int papprox_preprocess(dataptr dz)
{
	int exit_status;
	if(dz->mode==TRANSP_OUT)
		dz->is_transpos = TRUE;
	if((exit_status = convert_msecs_to_secs(PA_TRANG,dz))<0)
		return(exit_status);
	return convert_msecs_to_secs(PA_SRANG,dz);
}

/************************** PEXAG_PREPROCESS ******************************/

int pexag_preprocess(dataptr dz)
{
	if(dz->mode == RANGE_ONLY_TO_T 
	|| dz->mode == CONTOUR_ONLY_TO_T 
	|| dz->mode == R_AND_C_TO_T)
		dz->is_transpos = TRUE;
	return(FINISHED);
}

/************************** PINVERT_PREPROCESS ******************************/

int pinvert_preprocess(dataptr dz)
{
	if(dz->mode==TRANSP_OUT)
		dz->is_transpos = TRUE;
	return(FINISHED);
}

/************************** PQUANTISE_PREPROCESS ******************************/

int pquantise_preprocess(dataptr dz)
{
	if(dz->mode==TRANSP_OUT)
		dz->is_transpos = TRUE;
	if(dz->vflag[PQ_OCTDUPL])
		generate_octave_duplicates(dz);
	eliminate_duplicates_in_quantising_set(dz);
	return sort_quantising_set(dz);
}

/************ ADJUST_PARAMS_AND_SETUP_INTERNAL_PARAMS_FOR_PRANDOMISE *************/

int adjust_params_and_setup_internal_params_for_prandomise(dataptr dz)
{
	int exit_status;
	if((exit_status = convert_msecs_to_secs(PR_TSTEP,dz))<0)
		return(exit_status);
	if(dz->mode==TRANSP_OUT)
		dz->is_transpos = TRUE;
	if(dz->vflag[PR_IS_SLEW]) {
		if(dz->param[PR_SLEW] < 0.0)
			dz->iparam[PR_NEGATIV_SLEW] = TRUE;
		dz->param[PR_SLEW] = -1.0/dz->param[PR_SLEW];
	}
	return(FINISHED);
}

/************ ADJUST_PARAMS_AND_SETUP_INTERNAL_PARAMS_FOR_PSMOOTH *************/

int adjust_params_and_setup_internal_params_for_psmooth(dataptr dz)
{
	int exit_status;
	if((exit_status = convert_msecs_to_secs(PS_TFRAME,dz))<0)
		return(exit_status);
	if(dz->mode==TRANSP_OUT)
		dz->is_transpos = TRUE;
	return(FINISHED);
}

/************ ADJUST_PARAMS_FOR_PTRANSPOSE *************/

int adjust_params_for_ptranspose(dataptr dz)
{
 	dz->param[PT_TVAL] /= 12.0;
	dz->param[PT_TVAL] = pow(2.0,dz->param[PT_TVAL]);
	return(FINISHED);
}

/************************** PVIBRATO_PREPROCESS ******************************/

int pvibrato_preprocess(dataptr dz)
{
	int n;
	double scaling = (PI * 2.0)/P_TABSIZE;
	if(dz->mode==TRANSP_OUT)
		dz->is_transpos = TRUE;
	if((dz->parray[PV_SIN] = (double *)malloc((P_TABSIZE + 1) * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for vibrato sine table.\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<P_TABSIZE;n++)
		dz->parray[PV_SIN][n] = sin((double)n * scaling);
	dz->parray[PV_SIN][n] = 0.0;
	return(FINISHED);
}

/************************** PFIX_PREPROCESS ******************************/

int pfix_preprocess(dataptr dz)
{
	dz->iparam[PF_ISFILTER] = 0;	/* establish pfix filtertype */		
	if(dz->vflag[PF_HIPASS])
		dz->iparam[PF_ISFILTER] |= IS_HIPASS;
	if(dz->vflag[PF_LOPASS])
		dz->iparam[PF_ISFILTER] |= IS_LOPASS;

	if(dz->vflag[PF_STARTCUT]) { 	/* establish_pfix_cutpoints */
	  if(!dz->vflag[PF_ENDCUT])
			dz->iparam[PF_ECUTW] = dz->wlength;
		else
			dz->iparam[PF_ECUTW] = round(dz->param[PF_ECUT]/dz->frametime);
	}
	if(dz->vflag[PF_ENDCUT]) {
		if(!dz->vflag[PF_STARTCUT])
			dz->iparam[PF_SCUTW] = 0;
		else
			dz->iparam[PF_SCUTW] = round(dz->param[PF_SCUT]/dz->frametime);
	}
	if(dz->vflag[PF_ENDCUT] || dz->vflag[PF_STARTCUT]) {
		dz->iparam[PF_ISCUT] = 1;
		if(dz->iparam[PF_SCUTW] >= dz->iparam[PF_ECUTW]) {
			sprintf(errstr,"Remove-pitch times incompatible.\n");
			return(USER_ERROR);
		}
	}
	return(FINISHED);
}

/************************** REPITCHB_PREPROCESS ******************************/

int repitchb_preprocess(dataptr dz)
{
	dz->mode += 3;	/* converts to higher (brkpnt) mode inside the option 'repitch' */

	dz->param[RP_DRED] /= 12.0;
	dz->param[RP_DRED]  = pow(2.0,dz->param[RP_DRED]);
	dz->is_sharp = dz->param[RP_DRED];
	if(dz->is_sharp==0.0) {
		sprintf(errstr,"parameter not set: repitchb_preprocess()\n");
		return(PROGRAM_ERROR);
	}
	dz->is_flat  = 1.0/dz->is_sharp;
	return(FINISHED);
}

/********************************************************************************************/
/********************************** FORMERLY IN procspec.c **********************************/
/********************************************************************************************/

/**************************** SPEC_PROCESS_FILE ****************************/

int spec_process_file(dataptr dz)
{	
	dz->total_windows = 0;

	display_virtual_time(0L,dz);

	switch(dz->process) {
	case(TRNSP):		return outer_loop(dz);
	case(TRNSF):		return outer_loop(dz);
	case(PITCH):		return outer_pitch_loop(dz);
	case(TRACK):		return outer_pitch_loop(dz);
	case(P_APPROX):		return outer_pichpich_loop(dz);
	case(P_EXAG):		return outer_pichpich_loop(dz);
	case(P_INVERT):		return outer_pichpich_loop(dz);
	case(P_QUANTISE): 	return outer_pichpich_loop(dz);
	case(P_RANDOMISE):	return outer_pichpich_loop(dz);
	case(P_SMOOTH):	  	return outer_pichpich_loop(dz);
	case(P_TRANSPOSE):	return outer_pichpich_loop(dz);
	case(P_VIBRATO):  	return outer_pichpich_loop(dz);
	case(P_CUT):		return outer_pichpich_loop(dz);
//TW UPDATES
	case(P_SYNTH):		return outer_pichpich_loop(dz);
	case(P_VOWELS):		return outer_pichpich_loop(dz);
	case(P_INSERT):		return outer_pichpich_loop(dz);
	case(P_SINSERT):	return outer_pichpich_loop(dz);
	case(P_PTOSIL):		return outer_pichpich_loop(dz);
	case(P_NTOSIL):		return outer_pichpich_loop(dz);
	case(P_INTERP):		return outer_pichpich_loop(dz);
	case(ANALENV):		return get_anal_envelope(dz);
	case(P_BINTOBRK):	return convert_pitch_from_binary_to_text(dz);
	case(P_GEN):		return generate_pitch(dz);

	case(P_FIX):		return specpfix(dz);
	case(REPITCH):		return specrepitch(dz);
	case(REPITCHB):		return specrepitch(dz);
	default:
		sprintf(errstr,"Unknown process in procspec()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);	/* NOTREACHED */
}

/**************************** INNER_LOOP ****************************/

int inner_loop
(int *peakscore,int *descnt,int *in_start_portion,int *least,int *pitchcnt,int windows_in_buf,dataptr dz)
{
	int exit_status;
	int local_zero_set = FALSE;
	int wc;
 
   	for(wc=0; wc<windows_in_buf; wc++) {
		if(dz->total_windows==0) {
			if((exit_status = skip_or_special_operation_on_window_zero(dz))<0)
				return(exit_status);
			if(exit_status==TRUE) {
				dz->flbufptr[0] += dz->wanted;
				dz->total_windows++;
				dz->time = (float)(dz->time + dz->frametime);
				continue;
			}
		}
		if((exit_status = read_values_from_all_existing_brktables((double)dz->time,dz))<0)
			return(exit_status);

		switch(dz->process) {
		case(TRNSF):	exit_status = spectrnsf(dz);  						break;
		case(TRNSP):	exit_status = spectrnsp(dz);  						break;
		default:
			sprintf(errstr,"unknown process in inner_loop()\n");
			return(PROGRAM_ERROR);
		}
		if(exit_status<0)
			return(exit_status);
		dz->flbufptr[0] += dz->wanted;
		dz->total_windows++;
		dz->time = (float)(dz->time + dz->frametime);
	}
	if(!dz->zeroset && local_zero_set==TRUE) {
		fprintf(stdout,"WARNING: Zero-amp spectral window(s) encountered: orig window(s) substituted.\n"); 
		fflush(stdout);
		dz->zeroset = TRUE;
	}
	return(FINISHED);
}

/***************** SKIP_OR_SPECIAL_OPERATION_ON_WINDOW_ZERO ************/

int skip_or_special_operation_on_window_zero(dataptr dz)
{
	int vc;
	switch(dz->process) {
	case(TRNSF):
	case(TRNSP):
		for(vc = 0; vc < dz->wanted; vc += 2)
			dz->windowbuf[0][FREQ] = dz->flbufptr[0][FREQ];
		break;
	}
	return(TRUE);
}

/********************************************************************************************/
/********************************** FORMERLY IN pconsistency.c ******************************/
/********************************************************************************************/

/****************************** CHECK_PARAM_VALIDITY_AND_CONSISTENCY *********************************/

int check_param_validity_and_consistency(dataptr dz)
{
	int exit_status;
	int chans;
	int srate;
	int win_overlap;
	int chancnt;
	handle_pitch_zeros(dz);
	switch(dz->process) {
	case(PITCH):	   return check_consistency_of_pitch_params(dz);
	case(TRACK):	   return check_consistency_of_track_params(dz);
	case(P_INVERT):	   return check_compatibility_pinvert_params(dz);
	case(P_RANDOMISE): return check_validity_of_prandomise_params(dz);
	case(P_CUT):	   return check_consistency_of_pcut_params(dz);
	case(P_FIX):	   return check_compatibility_of_pfix_smoothing_flags(dz);
	case(TRNSF):	   return check_for_valid_transposition_ratios(MINPITCH/dz->nyquist,dz->nyquist/MINPITCH,dz);
	case(TRNSP):	   return check_for_valid_transposition_ratios(MINPITCH/dz->nyquist,dz->nyquist/MINPITCH,dz);

	case(P_SYNTH):
	case(P_VOWELS):
		chans = dz->infile->channels;
		dz->infile->channels = dz->infile->origchans;
		if((exit_status = create_sized_outfile(dz->wordstor[0],dz))<0)
			return(exit_status);
		dz->infile->channels = chans;
		break;
	case(ANALENV):
		chans = dz->infile->channels;
		srate = dz->infile->srate;
// JULY 2005 OLD 
		dz->infile->srate    = round(1.0/dz->frametime);
// NEW 
		dz->infile->srate    = (int)(1.0/dz->frametime);
		dz->infile->channels = 1;
		if((exit_status = create_sized_outfile(dz->wordstor[0],dz))<0)
			return(exit_status);
		dz->infile->channels = chans;
		dz->infile->srate    = srate;
		break;
	case(P_GEN):
		if(dz->floatsam_output)
			dz->infile->origstype = SAMP_FLOAT;
		else
			dz->infile->origstype = SAMP_SHORT;
		dz->infile->origrate  = dz->iparam[PGEN_SRATE];
		dz->infile->stype     = SAMP_FLOAT;
		win_overlap = dz->iparam[PGEN_WINOVLP_INPUT]-1;
		chancnt = dz->iparam[PGEN_CHANS_INPUT];
		chancnt = chancnt + (chancnt%2);
		switch(win_overlap) {
			case 0:	dz->infile->Mlen = 4*chancnt;	break;
			case 1:	dz->infile->Mlen = 2*chancnt;	break;
			case 2: dz->infile->Mlen = chancnt;		break;
			case 3: dz->infile->Mlen = chancnt / 2;	break;
			default:
				sprintf(errstr,"pvoc: Invalid window overlap factor.\n");
				return(PROGRAM_ERROR);
		}
		if((dz->infile->Dfac = (int)(dz->infile->Mlen/PVOC_CONSTANT_A)) == 0){
			fprintf(stdout,"WARNING: Decimation too low: adjusted.\n");
			fflush(stdout);
			dz->infile->Dfac = 1;
		}
		dz->infile->origchans = dz->iparam[PGEN_CHANS_INPUT] + 2;
		dz->wanted = dz->infile->origchans;
		dz->clength = dz->wanted/2;
		dz->nyquist = dz->infile->origrate / 2;		
		dz->infile->arate = (float) dz->infile->origrate / (float)dz->infile->Dfac;
		dz->infile->srate = (int) dz->infile->arate;
		dz->frametime = (float)(1.0/dz->infile->arate);
		dz->wlength = (int)round(dz->param[SS_DUR] * dz->infile->arate);
		dz->chwidth = dz->nyquist/(double)dz->clength;
		dz->infile->channels = 1;
		if((exit_status = create_sized_outfile(dz->outfilename,dz))<0)
			return(exit_status);
		break;
	}
	return(FINISHED);
}

/************ CHECK_CONSISTENCY_OF_PITCH_PARAMS *************/

int check_consistency_of_pitch_params(dataptr dz)
{
	if(dz->param[PICH_HILM] <= dz->param[PICH_LOLM]) {
		sprintf(errstr,"Impossible pitch range specified.\n");
		return(USER_ERROR);
	}
	if(dz->mode==PICH_TO_BRK) {
		if(dz->vflag==NULL) {
			sprintf(errstr,"Problem in setting up redundant flag for PITCH.\n");
			return(PROGRAM_ERROR);
		}
		if((dz->vflag = (char *)realloc((char *)dz->vflag,2 * sizeof(char)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for extra internal flags.\n");
			return(MEMORY_ERROR);
		}
		dz->vflag[KEEP_PITCH_ZEROS] = FALSE;
		establish_datareduction(PICH_DATAREDUCE,dz);
	}
	return(FINISHED);
}

/************ CHECK_CONSISTENCY_OF_TRACK_PARAMS *************/

int check_consistency_of_track_params(dataptr dz)
{
	if(dz->vflag!=NULL) {
		sprintf(errstr,"Problem in setting up redundant flag for TRACK.\n");
		return(PROGRAM_ERROR);
	}
	if((dz->vflag = (char *)malloc(2 * sizeof(char)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for extra internal flags.\n");
		return(MEMORY_ERROR);
	}
	dz->vflag[KEEP_PITCH_ZEROS] = FALSE;
	establish_datareduction(TRAK_DATAREDUCE,dz);
//TW UPDATE
	return(FINISHED);
}

/************************** ESTABLISH_DATAREDUCTION ********************************/

void establish_datareduction(int paramno,dataptr dz)
{
	double datareduction = dz->param[paramno] * OCTAVES_PER_SEMITONE;
	datareduction = pow(2.0,datareduction);
	dz->is_sharp  = datareduction;
	dz->is_flat   = 1.0/dz->is_sharp;
}

/************ CHECK_COMPATIBILITY_PINVERT_PARAMS *************/

int check_compatibility_pinvert_params(dataptr dz)
{
	int exit_status;
	int n;
	double maxmean, minmean;
	if((dz->vflag[PI_IS_TOP] || dz->vflag[PI_IS_BOT]) && dz->vflag[PI_IS_MEAN]) {
		if(dz->brksize[PI_MEAN]) {
			maxmean = MIDIMIN;
			if((exit_status = hztomidi(&minmean,dz->nyquist))<0)
				return(exit_status);
			for(n=0;n<dz->itemcnt*2; n+=2) {
				if(dz->brk[PI_MEAN][n] > maxmean)
					maxmean = dz->brk[PI_MEAN][n];
				if(dz->brk[PI_MEAN][n] < minmean)
					minmean = dz->brk[PI_MEAN][n];
			}	 
		} else
			maxmean = minmean = dz->param[PI_MEAN];	
		if(dz->vflag[PI_IS_TOP] && (dz->param[PI_TOP] < maxmean)) {
			if(dz->brksize[PI_MEAN])
				sprintf(errstr,
				"top midipitch value must be ABOVE all mean values in brkpntfile.\n");
			else
				sprintf(errstr,"top midipitch value must be ABOVE mean value.\n");
			return(USER_ERROR);

		}
		if(dz->vflag[PI_IS_BOT] && (dz->param[PI_BOT] > minmean)) {
			if(dz->brksize[PI_MEAN])
				sprintf(errstr,"bot midipitch value must be BELOW all mean values in brkpntfile.\n");
			else
				sprintf(errstr,"bot midipitch value must be BELOW mean value.\n");
			return(USER_ERROR);
		}
   	}
	return(FINISHED);
}

/************ CHECK_VALIDITY_OF_PRANDOMISE_PARAMS *************/

int check_validity_of_prandomise_params(dataptr dz)
{
	if(dz->vflag[PR_IS_SLEW] && fabs(dz->param[PR_SLEW])<=1.0) {
		sprintf(errstr,"slew out of range: slew must be >1 or <-1\n");
		return(USER_ERROR);
	}
	return(FINISHED);
}

/********************** CHECK_CONSISTENCY_OF_PCUT_PARAMS **********************/

int check_consistency_of_pcut_params(dataptr dz)
{
	if(dz->mode==PCUT_BOTH && (dz->param[PC_STT] >= dz->param[PC_END])) {
		sprintf(errstr,"Start and end cut-times incompatible.\n");
		return(USER_ERROR);
	}
	return(FINISHED);
}

/***************************** CHECK_COMPATIBILITY_OF_PFIX_SMOOTHING_FLAGS ******/

int check_compatibility_of_pfix_smoothing_flags(dataptr dz)
{
	if(dz->vflag[PF_TWOW] && !dz->vflag[PF_IS_SMOOTH]) {
		sprintf(errstr,"-w flag cannot be used without -s flag.\n");
		return(USER_ERROR);
	}
	return(FINISHED);
}

/********************* CHECK_FOR_VALID_TRANSPOSITION_RATIOS **********************/

int check_for_valid_transposition_ratios(double mintrans,double maxtrans,dataptr dz)
{
	int n;
	for(n=0;n<dz->wlength;n++) {
		if(dz->transpos[n] < mintrans || dz->transpos[n] > maxtrans) {
			sprintf(errstr,
				"Invalid transposition ratio (%f) encountered.\n(MIN %lf MAX %lf)\n",
				dz->transpos[n],mintrans,maxtrans);
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
	switch(dz->process) {
	case(TRNSP):	case(TRNSF):
		return allocate_single_buffer(dz);

	case(PITCH):	case(TRACK):
//TW UPDATES
	case(P_SYNTH):
	case(P_VOWELS):

		return allocate_single_buffer_plus_extra_pointer(dz);

	case(P_APPROX):		case(P_EXAG): 	  case(P_INVERT):	  
	case(P_QUANTISE): 	case(P_RANDOMISE):case(P_SMOOTH):	  
	case(P_TRANSPOSE):	case(P_VIBRATO):  case(P_CUT): 	  
	case(P_FIX): 	  	case(REPITCH):	  case(REPITCHB):	  
		return(FINISHED);
//TW UPDATES
	case(P_INSERT):
	case(P_SINSERT):
	case(P_PTOSIL):
	case(P_NTOSIL):
	case(P_GEN):
	case(P_INTERP):
	case(P_BINTOBRK):
		return(FINISHED);
	case(ANALENV):
		return allocate_double_buffer(dz);

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
	if     (!strcmp(prog_identifier_from_cmdline,"getpitch"))     	dz->process = PITCH;
	else if(!strcmp(prog_identifier_from_cmdline,"track"))     		dz->process = TRACK;
	else if(!strcmp(prog_identifier_from_cmdline,"approx"))   		dz->process = P_APPROX;
	else if(!strcmp(prog_identifier_from_cmdline,"exag"))     		dz->process = P_EXAG;
	else if(!strcmp(prog_identifier_from_cmdline,"invert"))   		dz->process = P_INVERT;
	else if(!strcmp(prog_identifier_from_cmdline,"quantise")) 		dz->process = P_QUANTISE;
	else if(!strcmp(prog_identifier_from_cmdline,"randomise")) 		dz->process = P_RANDOMISE;
	else if(!strcmp(prog_identifier_from_cmdline,"smooth"))   		dz->process = P_SMOOTH;
	else if(!strcmp(prog_identifier_from_cmdline,"pchshift")) 		dz->process = P_TRANSPOSE;
	else if(!strcmp(prog_identifier_from_cmdline,"vibrato"))  		dz->process = P_VIBRATO;
	else if(!strcmp(prog_identifier_from_cmdline,"cut"))  	   		dz->process = P_CUT;
	else if(!strcmp(prog_identifier_from_cmdline,"fix"))  	   		dz->process = P_FIX;
	else if(!strcmp(prog_identifier_from_cmdline,"combine"))   		dz->process = REPITCH;
	else if(!strcmp(prog_identifier_from_cmdline,"combineb"))  		dz->process = REPITCHB;
	else if(!strcmp(prog_identifier_from_cmdline,"transpose"))	   	dz->process = TRNSP;
	else if(!strcmp(prog_identifier_from_cmdline,"transposef"))	   	dz->process = TRNSF;
//TW UPDATES (or definite instatements)
	else if(!strcmp(prog_identifier_from_cmdline,"synth"))  	   	dz->process = P_SYNTH;
	else if(!strcmp(prog_identifier_from_cmdline,"vowels"))  	   	dz->process = P_VOWELS;
	else if(!strcmp(prog_identifier_from_cmdline,"insertzeros"))  	dz->process = P_INSERT;
	else if(!strcmp(prog_identifier_from_cmdline,"insertsil"))  	dz->process = P_SINSERT;
	else if(!strcmp(prog_identifier_from_cmdline,"pitchtosil"))  	dz->process = P_PTOSIL;
	else if(!strcmp(prog_identifier_from_cmdline,"noisetosil"))  	dz->process = P_NTOSIL;
	else if(!strcmp(prog_identifier_from_cmdline,"analenv"))  		dz->process = ANALENV;
	else if(!strcmp(prog_identifier_from_cmdline,"generate"))  	   	dz->process = P_GEN;
	else if(!strcmp(prog_identifier_from_cmdline,"interp"))  	   	dz->process = P_INTERP;
	else if(!strcmp(prog_identifier_from_cmdline,"pchtotext"))  	dz->process = P_BINTOBRK;
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
	fprintf(stdout,
	"\nREPITCHING OPERATIONS ON SPECTRAL AND PITCH FILES\n\n"
	"USAGE: repitch NAME (mode) infile (infile2) outfile parameters: \n"
	"\n"
	"where NAME can be any one of\n"
	"\n"
	"getpitch\n"
	"approx      exag     invert    quantise    randomise    smooth\n"
	"vibrato     cut      fix       combine     combineb\n"
//TW UPDATES
	"synth    vowels   insertzeros   insertsil   pitchtosil    noisetosil   analenv\n"
	"generate interp   pchtotext\n"
	"pchshift   transpose   transposef\n\n"
	"Type 'repitch getpitch' for more info on repitch getpitch..ETC.\n");
	return(USAGE_ONLY);
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
	if(!strcmp(str,"getpitch")) {
		fprintf(stdout,
		"repitch getpitch 1 infile outfile pfil \n\t [-tR] [-gM] [-sS] [-nH] [-lL] [-hT] [-a] [-z]\n"
		"repitch getpitch 2 infile outfile bfil \n\t [-tR] [-gM] [-sS] [-nH] [-lL] [-hT] [-di] [-a]\n\n" 
		"ATTEMPT TO EXTRACT PITCH FROM SPECTRAL DATA.\n\n" 
		"outfile  If resynthesized, produces tone at detected (possibly varying) pitch.\n\n"
		"pfil     (MODE 1) Binary output file containing pitch information.\n"
		"bfil     (MODE 2) breakpoint (text) outfile of pitch info (as time/Hz pairs).\n"
		"         Either of these may be reused in other pitch-manipulating options.\n\n"
		"-t R     R = Tuning range(semitones) within which harmonics accepted as in tune\n"
		"         (Default 1)\n"
		"-g M     M = minimum number of adjacent windows that must be pitched,\n"
		"         for a pitch-value to be registered (Default %d).\n"
		"-s S     S = signal to noise ratio, in decibels. (Default %.0lfdB)\n"
		"         Windows which are more than SdB below maximum level in sound, are\n"
		"         assumed to be noise, & any detected pitchvalue is assumed spurious.\n"
		"-n H     H = how many of the %d loudest peaks in spectrum must be harmonics\n"
		"         to confirm sound is pitched: Default %d.\n"
		"-l L     L = frequency of LOWEST acceptable pitch (Default min: %.0lfHz).\n"
		"-h T     T = frequency of TOPMOST acceptable pitch (Default max: nyquist/%d).\n"
		"-d i     i = acceptable pitch-ratio error in data reduction. (semitones)\n"
		"         Default 1/8 tone = %lf\n"
		"-a       Alternative pitch-finding algorithm (avoid N<2).\n"
		"-z       Retain unpitched windows (set them to -1)\n"
		"         Default: Set pitch by interpolation between adjacent pitched windows.\n",
		BLIPLEN,SILENCE_RATIO,MAXIMI,ACCEPTABLE_MATCH,MINPITCH,MAXIMI,LOG2(EIGHTH_TONE)*SEMITONES_PER_OCTAVE);
	} else if(!strcmp(str,"approx")) {
		fprintf(stdout,
    	"repitch approx mode pitchfile outfile [-pprange] [-ttrange] [-ssrange]\n\n"
		"MAKE AN APPROXIMATE COPY OF A PITCHFILE.\n\n"
		"PRANGE  Interval (semitones) over which pitch +- randomly varies from orig.\n"
		"        Range > 0.0\n"
		"TRANGE  Time-interval (msecs) by which pitchval can stray from orig time.\n"
		"        Applied to turning-points in pitch-contour.\n"
		"        Range: from duration of 1 analysis window to duration of entire file.\n"
		"SRANGE  Time-interval (msecs) over which pitch contour scanned.\n"
		"        Pitchshift by a semitone within srange, indicates pitch rise or fall.\n"
		"        Range: from duration of %d analwindows to (approx) dur of infile.\n\n"
    	"MODES....\n"
    	"(1) Gives a pitchfile as ouput.\n"
    	"(2) Gives a transposition file as output.\n\n"
    	"Prange, trange and srange may vary over time.\n",2*BLOKCNT);
	} else if(!strcmp(str,"exag")) {
		fprintf(stdout,
    	"repitch exag 1-2 pitchfile outfile meanpch range\n"
    	"repitch exag 3-4 pitchfile outfile meanpch       contour\n"
    	"repitch exag 5-6 pitchfile outfile meanpch range contour\n\n"
		"EXAGGERATE PITCH CONTOUR.\n\n"
		"MEANPITCH pitch (MIDI) around which intervals stretched.\n"
		"RANGE     exagg pitch-range (multiply semitone-intvl). >0.\n"
		"CONTOUR   exagg pitch-contour. Values (0-1).\n\n"
    	"MODES....\n"
    	"(1,3,5) Give a pitchfile as ouput.\n"
    	"(2,4,6) Give a transposition file as output.\n\n"
    	"Meanpitch, range and contour may all vary over time.\n");
	} else if(!strcmp(str,"invert")) {
		fprintf(stdout,
    	"repitch invert mode pitchfile outfile map [-mmeanpch] [-bbot] [-ttop]\n\n"
		"INVERT PITCH CONTOUR OF A PITCH DATA FILE.\n\n"
		"MAP	   Set map to ZERO if no mapping is required..OTHERWISE\n"
		"          map is a file of paired values showing how intervals\n"
		"          (in, possibly fractional, SEMITONES)\n"
		"          are to be mapped onto their inversions.\n"
		"          Range -%.0lf to %.0lf.\n"
		"MEANPCH   pitch (MIDI) around which pitchline inverted.\n"
		"          Range: MIDI equivalents of %.0lfHz to nyquist.\n"
		"BOT       bottom pitch (MIDI) permissible (default 0).\n"
		"          Range: MIDI equivalents of %.0lfHz to nyquist.\n"
 		"TOP       top pitch (MIDI) permissible (default 127).\n"
		"          Range: MIDI equivalents of %.0lfHz to nyquist.\n"
   		"MODES.\n"
    	"(1) Gives a pitchfile as ouput.\n"
    	"(2) Gives a transposition file as output.\n\n"
    	"Meanpitch may vary over time.\n",MAXINTRANGE,MAXINTRANGE,MINPITCH,MINPITCH,MINPITCH);		   
	} else if(!strcmp(str,"quantise")) {
		fprintf(stdout,
    	"repitch quantise mode pitchfile outfile q-set [-o]\n\n"
		"QUANTISE PITCHES IN A PITCH DATA FILE.\n\n"
		"q_set   a file of (possibly fractional) MIDI pitchvals\n"
		"        over which pitch to be quantised.\n"
		"-o		 duplicates q_set in all octave transpositions\n\n"
    	"MODES....\n"
    	"(1) Gives a pitchfile as ouput.\n"
    	"(2) Gives a transposition file as output.\n\n");
	} else if(!strcmp(str,"randomise")) {
		fprintf(stdout,
		"repitch randomise mode pitchfile outfile maxinterval timestep [-sslew]\n\n"
		"RANDOMISE PITCH LINE.\n\n"
		"MAXINTERVAL (semitones) over which pitches can +- randomvary.\n"
		"            Range 0 to %.0lf: Value or brkpnt file.\n"
		"TIMESTEP    maximum timestep between random pitch fluctuations (milliseconds).\n"
		"            Timesteps are random values less than this.\n"
		"            Range: duration of 1 anal window to dur of entire file.\n" 
		"SLEW	    e.g. 2:  upward variation range twice that of downward.\n"
		"    	    e.g -3: downward variation range 3 times that of upward.\n"
		"    	    Range >1 || <-1.\n\n"
    	"MODES....\n"
    	"(1) Gives a pitchfile as ouput.\n"
    	"(2) Gives a transposition file as output.\n\n"
    	"Maxinterval and timestep may vary over time.\n",MAXINTRANGE);
	} else if(!strcmp(str,"smooth")) {
		fprintf(stdout,
    	"repitch smooth mode pitchfile outfile timeframe [-pmeanpch] [-h]\n\n"
		"SMOOTH PITCH CONTOUR IN A PITCH DATA FILE.\n\n"
		"TIMEFRAME (millisecs) over which to interpolate pitch values.\n"
		"          Range: duration of 1 anal window to dur of entire file.\n" 
		"-p        interp between PEAK value in each timeframe block of pitch values.\n"
		"          MEANPITCH is pitch from which peaks measured\n"
		"          (and must be within pitch range, in every timeframe block).\n"
		"          Peak is maximum displacement from mean (up or down).\n"
		"-h        At end of file, hold last interpolated pitchvalue calculated.\n"
		"          Default: interpolate thence to last actual value in input.\n"
    	"MODES....\n"
    	"(1) Gives a pitchfile as ouput.\n"
    	"(2) Gives a transposition file as output.\n\n"
    	"Timeframe and meanpitch may vary over time.\n");
	} else if(!strcmp(str,"pchshift")) {
		fprintf(stdout,
    	"repitch pchshift pitchfile outpitchfile transposition\n\n"
		"SHIFTS PITCHES IN PITCHDATA FILE BY CONSTANT NO OF SEMITONES.\n\n"
		"(To transpose pitch of spectrum in time-varying way,'repitch transpose[f]')\n");
	} else if(!strcmp(str,"vibrato")) {
		fprintf(stdout,
		"repitch vibrato mode pitchfile outfile vibfreq vibrange\n\n"
		"ADD VIBRATO TO PITCH IN A PITCH DATA FILE.\n\n"
		"VIBFREQ   frequency of vibrato itself (Hz). Values >0.\n"
		"VIBRANGE  max interval vibrato moves away from central pitch (semitones).\n"
		"          Values >0.\n\n"
    	"MODES....\n"
    	"(1) Gives a pitchfile as ouput.\n"
    	"(2) Gives a transposition file as output.\n\n"
    	"Vibfreq and vibrange may vary over time.\n");
	} else if(!strcmp(str,"cut")) {
		fprintf(stdout,
		"repitch cut 1 pitchfile outpitchfile starttime\n"
		"repitch cut 2 pitchfile outpitchfile endtime\n"
		"repitch cut 3 pitchfile outpitchfile starttime endtime\n\n"
		"CUT OUT AND KEEP A SEGMENT OF A BINARY PITCH-DATAFILE.\n");
	} else if(!strcmp(str,"fix")) {
		fprintf(stdout,
		"repitch fix pitchfile outpitchfile\n"
		"       [-rt1] [-xt2] [-lbf] [-htf] [-sN] [-bf1] [-ef2] [-w] [-i]\n\n"
		"MASSAGE PITCH DATA IN A BINARY PITCHFILE.\n"
		"Pitchdata may be viewed by running pitchinfo see, and viewing output in viewsf.\n\n"
		"PITCHFILE & OUTPITCHFILE:binary pitchdata files generated by 'spec pitch' etc.\n\n"
		"-r t1  Starttime:  remove pitch from time 't1' (default 0.0).\n"
		"-x t2  Endtime:    end pitch removal at time 't2' (default, end of file).\n"
		"-l bf  Bottom_frq: remove pitch below frequency 'bf'.\n"
		"-h tf  Top_frq:    remove pitch above frequency 'tf'.\n"
		"-s N   Smoothcnt:  smooth onset errors & glitches in pitchdata, N times.\n"
		"-b f1  Startfrq:   force start frequency to be 'f1'.\n"
		"-e f2  Endfrq:     force end frequency to be 'f2'.\n"
		"-w     removes 2-window glitches (Default: 1-window) (Use ONLY with -s)\n"
		"-i     interpolate through ALL non-pitch windows in pitch_data,\n"
		"       producing pitch everywhere.\n"
		"RULES.........\n"
		"(1) AT LEAST ONE flag must be used.\n"
		"(2) When pitches are removed they are replaced by a 'no-pitch' indicator.\n"
    	"    These (and any already existing unpitched windows in the file) can be\n"
		"    converted to pitch-data (by interpolation between adjacent pitches)\n"
		"    using the -i flag.\n"
		"(3) With multiple flags, ORDER of operations inside program is....\n"
    	"    REMOVE-TIMEWISE(rx),REMOVE-FRQWISE(lh),SMOOTH(sw),SET-ENDVALS(be),INTERP(i)\n");
	} else if(!strcmp(str,"combine")) {
		fprintf(stdout,
		"repitch combine 1 pitchfile    pitchfile2    outtransposfile\n" 
		"repitch combine 2 pitchfile    transposfile  outpitchfile\n" 
		"repitch combine 3 transposfile transposfile2 outtransposfile\n\n" 
		"       GENERATE TRANSPOSITION DATA FROM 2 SETS OF PITCH DATA,\n" 
		"          OR TRANSPOSE PITCH DATA WITH TRANSPOSITION DATA,\n" 
		"OR COMBINE 2 SETS OF TRANSPOSITION DATA TO FORM NEW TRANSPOSITION DATA,\n"
		"                 PRODUCING BINARY DATAFILE OUTPUT.\n\n"
		"PITCHFILE       binary pitchdatafile OR time/pitch(frq) brkpnt file\n"
		"TRANSPOSFILE    binary transposition file OR time/transpos(ratio) brkpnt file\n"
		"OUTPITCHFILE    binary pitchdatafile\n"
		"OUTTRANSPOSFILE binary transpositionfile\n\n"
		"NB: It's IMPOSSIBLE to generate binary outfile from exclusively brkpnt infiles.\n");
	} else if(!strcmp(str,"combineb")) {
		fprintf(stdout,
		"repitch combineb 1 pitchfile    pitchfile2    outtbrkfile [-dI]\n" 
		"repitch combineb 2 pitchfile    transposfile  outpbrkfile [-dI]\n" 
		"repitch combineb 3 transposfile transposfile2 outtbrkfile [-dI]\n\n" 
		"       GENERATE TRANSPOSITION DATA FROM 2 SETS OF PITCH DATA,\n" 
		"          OR TRANSPOSE PITCH DATA WITH TRANSPOSITION DATA,\n" 
		"OR COMBINE 2 SETS OF TRANSPOSITION DATA TO FORM NEW TRANSPOSITION DATA,\n"
		"              PRODUCING A TIME/VAL BRKPNT FILE OUTPUT.\n\n"
		"PITCHFILE    binary pitchdatafile OR time/pitch(frq) brkpnt file\n"
		"TRANSPOSFILE binary transposition file OR time/transpos(ratio) brkpnt file\n"
		"OUTPBRKFILE  time/pitch(frq) brkpnt file\n"
		"OUTTBRKFILE  time/transposition(ratio) brkpnt file\n\n"
		"-d  I = acceptable pitch error in brkpntfile data-reduction.\n"
   		"    Range > 1.0 : Default: eighth_tone = %lf\n",LOG2(EIGHTH_TONE)*SEMITONES_PER_OCTAVE);
	} else if(!strcmp(str,"transposef")) {
		fprintf(stdout,
		"repitch transposef 1-3 infile outfile -fN|-pN [-i] transpos [-lminf][-hmaxf][-x]\n"
		"repitch transposef 4   infile transpos outfile -fN|-pN [-i] [-lminf][-hmaxf][-x]\n"
		"\n"
		"TRANSPOSE SPECTRUM : BUT RETAIN ORIGINAL SPECTRAL ENVELOPE\n"
		"\n"
		"MODES\n"
		"1    transposition as a frq ratio.\n"
		"2    transposition in (fractions of) octaves.\n"
		"3    transposition in (fractions of) semitones.\n"
		"4    transposition as a binary data file.\n"
		"\n"
		"-f   extract formant envelope linear frqwise,\n"
		"     using 1 point for every N equally-spaced frequency-channels.\n"
		"-p   extract formant envelope linear pitchwise,\n"
		"     using N equally-spaced pitch-bands per octave.\n"
		"-i   quicksearch for formants (less accurate).\n"
		"\n"
		"-l     MINF = minimum frq, below which data is filtered out.\n"
		"-h     MAXF = maximum frq, above which data is filtered out.\n"
		"-x     Fuller spectrum.\n"
		"\n"
		"frq-ratio, octave or semitone transpositions may vary over time.\n"
		"maxfrq and minfrq may vary over time.\n");
	} else if(!strcmp(str,"transpose")) {
		fprintf(stdout,
		"repitch transpose 1-3 infile          outfile transpos [-lminfrq][-hmaxfrq][-x]\n"
		"repitch transpose 4   infile transpos outfile          [-lminfrq][-hmaxfrq][-x]\n"
		"\n"
		"TRANSPOSE SPECTRUM (SPECTRAL ENVELOPE ALSO MOVES)\n"
		"\n"
		"MODES\n"
		"1    transposition as a frq ratio.\n"
		"2    transposition in (fractions of) octaves.\n"
		"3    transposition in (fractions of) semitones.\n"
		"4    transposition as a binary data file.\n"
		"\n"
		"-l     MINFRQ = minimum frq, below which data is filtered out.\n"
		"-h     MAXFRQ = maximum frq, above which data is filtered out.\n"
		"-x     Fuller spectrum.\n"
		"\n"
		"frq-ratio, octave or semitone transpositions may vary over time.\n");
//TW UPDATES
	} else if(!strcmp(str,"synth")) {
		fprintf(stdout,
		"repitch synth binarypitchfile outanalfile harmonics-data\n"
		"\n"
		"CREATE SPECTRUM OF A SOUND FOLLOWING THE PITCH CONTOUR IN THE PITCH FILE\n"
		"\n"
		"HARMONICS-DATA is a list of the amplitude of each harmonic in sequence, from 1 upwards\n"
		"Amplitudes must lie in the range 0-1\n");
	} else if(!strcmp(str,"generate")) {
		fprintf(stdout,
		"repitch generate outpitchdatafile midipitch-data srate  [-cpoints] [-ooverlap]\n"
		"\n"
		"CREATE BINARY PITCHDATA FILE FROM A TEXTFILE OF TIME-MIDI VALUE PAIRS\n"
		"\n"
		"MIDIPITCH-DATA is a list of paired time and midi-note-values\n"
		"     where times must start at zero and increase,\n"
		"     and note values may be numeric MIDI values (possibly fractional)\n"
		"     OR note names (A,B,C,D,E,F or G)\n"
		"     possibly followed by '#'(sharp) or 'b'(flat)\n"
		"     followed by an octave number between -5 and 5\n"
		"     where 0 corresponds to the octave starting at middle C.\n\n"
		"SRATE is the sample rate of the soundfile that might later be generated\n"
		"     from the binary pitch data.\n"
		"POINTS   No of analysis points (2-32768 (power of 2)): default 1024\n"
		"         More points give better freq resolution but worse time-resolution.\n"
		"OVERLAP  Filter overlap factor (1-4): default 3\n");
	} else if(!strcmp(str,"vowels")) {
		fprintf(stdout,
		"repitch vowels infile outfile vowel-data halfwidth curve pk_range fweight foffset\n"
		"\n"
		"CREATE SPECTRUM OF VOWEL SOUND(S) FOLLOWING PITCH CONTOUR IN PITCH FILE\n"
		"\n"
		"VOWEL-DATA is a vowel, OR a file of paired time/vowel values\n"
		"where vowel is one of the following strings\n"
		"     ee : as in 'heat'\n"        
		"     i  : as in 'hit'\n"
		"     ai : as in 'hate'\n"
		"     aii: as in scottish 'e'\n"
		"     e  : as in 'pet'\n"
		"     a  : as in 'pat'\n"
		"     ar : as in 'heart'\n"
		"     o  : as in 'hot'\n"
		"     or : as in 'taught'\n"
		"     u  : as in 'hood'\n"
		"     uu : as in scottish 'you'\n"
		"     ui : as in scottish 'could'\n"
		"     x  : neutral vowel as in 'herb', or 'a'\n"
		"     xx : as in Southern English 'hub'\n"
		"     n  : as in mean, can, done\n"
		"\n"
		"In brkpnt files, times must start at zero, and increase.\n"
		"\n"
		"HALFWIDTH is half-width of formant, in Hz, as fraction of formant centre-frq (range .01 - 10)\n"
		"CURVE     is the steepness of the formant peak (range 0.1 to 10.0)\n"
		"PK_RANGE  ratio of (max) amplitude range of formant peaks to (max) total range (range 0 to 1)\n"
		"FWEIGHT   is amplitude weighting of fundamental (range 0 to 1)\n"
		"FOFFSET   is amount of scattering of frqs of harmonics from their true value (range 0 to 1)\n"
		"\n");
	} else if(!strcmp(str,"insertzeros")) {
		fprintf(stdout,
		"repitch insertzeros mode infile outfile zeros-data\n"
		"\n"
		"MARK AREAS AS UNPITCHED IN A PITCHDATA FILE\n"
		"\n"
		"MODES\n"
		"1  data as times.\n"
		"2  data as (grouped) sample count: Count samples in mono, pairs in stereo etc\n"
		"\n"
		"ZEROS-DATA\n"
		"is a list of pairs of times between which unpitched data to be indicated.\n");
	} else if(!strcmp(str,"insertsil")) {
		fprintf(stdout,
		"repitch insertsil mode infile outfile silence-data\n"
		"\n"
		"MARK AREAS AS SILENT IN A PITCHDATA FILE\n"
		"\n"
		"MODES\n"
		"1  data as times.\n"
		"2  data as (grouped) sample count: Count samples in mono, pairs in stereo etc\n"
		"\n"
		"SILENCE-DATA\n"
		"is a a list of pairs of times between which silence is to be indicated.\n");
	} else if(!strcmp(str,"pitchtosil")) {
		fprintf(stdout,
		"repitch pitchtosil infile outfile\n"
		"\n"
		"REPLACE PITCHED WINDOWS BY SILENCE\n");
	} else if(!strcmp(str,"noisetosil")) {
		fprintf(stdout,
		"repitch noisetosil infile outfile\n"
		"\n"
		"REPLACE UNPITCHED WINDOWS BY SILENCE\n");
	} else if(!strcmp(str,"analenv")) {
		fprintf(stdout,
		"repitch analenv inanalfile outenvfile\n"
		"\n"
		"EXTRACT THE WINDOW-LOUDNESS ENVELOPE OF AN ANALYSIS FILE\n"
		"\n"
		"Extracts loudness of each window of analysis file.\n"
		"Output data synchronous with pitch or formant data from same analfile\n");
	} else if(!strcmp(str,"interp")) {
		fprintf(stdout,
		"repitch interp 1-2 infile outfile\n"
		"\n"
		"REPLACE NOISE OR SILENCE BY PITCH INTERPOLATED FROM EXISTING PITCHES\n"
		"\n"
		"Mode 1 glides from previous valid pitch to next vaild pitch.\n"
		"Mode 2 sustains previous valid pitch until next vaild pitch appears.\n");
	} else if(!strcmp(str,"pchtotext")) {
		fprintf(stdout,
		"repitch pchtotext infile outfile\n"
		"\n"
		"CONVERT BINARY PITCH DATA TO TEXTFILE\n");

	} else
		fprintf(stdout,"Unknown option '%s'\n",str);
	return(USAGE_ONLY);
}

/******************************** USAGE3 ********************************/

int usage3(char *str1,char *str2)
{
	sprintf(errstr,"Insufficient parameters on command line.\n");
	return(USAGE_ONLY);
}

//TW UPDATE : NEW FUNCTION
/******************************** GET_PITCH ********************************/

int get_pitch(char *str,double *midi)
{
	char *p = str;
	int oct, intmidi = 0, is_noteval = 0;
	switch(*p) {
	case('c'):	case('C'):	intmidi = 0; 	is_noteval = 1; break;
	case('d'):	case('D'):	intmidi = 2; 	is_noteval = 1; break;	
	case('e'):	case('E'):	intmidi = 4; 	is_noteval = 1; break;
	case('f'):	case('F'):	intmidi = 5; 	is_noteval = 1; break;	
	case('g'):	case('G'):	intmidi = 7;	is_noteval = 1; break;
	case('a'):	case('A'):	intmidi = 9;	is_noteval = 1; break;
	case('b'):	case('B'):	intmidi = 11;	is_noteval = 1; break;
	}
	if(is_noteval) {
		p++;	
		switch(*p) {
		case('#'):	intmidi++;	p++; break;
		case('b'):	intmidi--;	p++; break;
		}
		if(sscanf(p,"%d",&oct)!=1) {
			sprintf(errstr,"No (valid) octave value given : '");
			return(-1);
		}
		if(oct < -5 || oct > 5) {
			sprintf(errstr,"Octave value (%d) out of range : '",oct);
			return(-1);
		}
		oct += 5;
		oct *= 12;
		intmidi += oct;
		*midi = (double)intmidi;
	} else if(sscanf(str,"%lf",midi)!=1) {
		sprintf(errstr,"'");
		return(-1);
	}
	if(*midi < 0.0 || *midi >= 132.0) {
		sprintf(errstr,"Midi value (%lf) out of range : '",*midi);
		return(-1);
	}
	return FINISHED;
}

