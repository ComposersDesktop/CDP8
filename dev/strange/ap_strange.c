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
#include <strange.h>
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

/********************************************************************************************/
/********************************** FORMERLY IN buffers.c ***********************************/
/********************************************************************************************/

static int 	allocate_warp_buffer(dataptr dz);

/********************************************************************************************/
/********************************** FORMERLY IN preprocess.c ********************************/
/********************************************************************************************/

#define	ROLL_OFF	(500.0)
static int  setup_internal_arrays_for_specshift(dataptr dz);
static int  establish_internal_params_for_glis(dataptr dz);
static int  setup_internal_arrays_for_waver(dataptr dz);
static int  setup_array_of_botfrq_of_each_channel(int *botchan,dataptr dz);
static int  warp_preprocess(dataptr dz);
static int  convert_params_for_warp(dataptr dz);
static int  setup_internal_arrays_and_params_for_specwarp(dataptr dz);
static int  convert_pitchdata_to_midi_for_warp(dataptr dz);
static int  invert_preprocess(dataptr dz);
static int  setup_internal_arrays_for_invert(dataptr dz);
static int  extract_normalising_criteria_by_reading_infile(dataptr dz);

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
	case(SHIFT):      		dz->extra_bufcnt =  0; dz->bptrcnt = 1; 	is_spec = TRUE;		break;
	case(GLIS):       		dz->extra_bufcnt =  0; dz->bptrcnt = 1; 	is_spec = TRUE;		break;
	case(WAVER):      		dz->extra_bufcnt =  0; dz->bptrcnt = 1; 	is_spec = TRUE;		break;
	case(WARP):	      		dz->extra_bufcnt =  1; dz->bptrcnt = 7; 	is_spec = TRUE;		break;
	case(INVERT):     		dz->extra_bufcnt =  0; dz->bptrcnt = 1; 	is_spec = TRUE;		break;	
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
	case(SHIFT):  	dz->array_cnt =2;  dz->iarray_cnt =2;  dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(GLIS):   	dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(WAVER):  	dz->array_cnt =1;  dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(WARP):   	dz->array_cnt =2;  dz->iarray_cnt =1;  dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break; 	
	case(INVERT): 	dz->array_cnt =1;  dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
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
	case(SHIFT):		setup_process_logic(ANALFILE_ONLY,		  	EQUAL_ANALFILE,		ANALFILE_OUT,	dz);	break;
	case(GLIS):			setup_process_logic(ANALFILE_ONLY,		  	EQUAL_ANALFILE,		ANALFILE_OUT,	dz);	break;
	case(WAVER):		setup_process_logic(ANALFILE_ONLY,		  	EQUAL_ANALFILE,		ANALFILE_OUT,	dz);	break;
	case(WARP):		  	setup_process_logic(ANAL_WITH_PITCHDATA,  	BIG_ANALFILE,		ANALFILE_OUT,	dz);	break;
	case(INVERT):		setup_process_logic(ANALFILE_ONLY,		  	EQUAL_ANALFILE,		ANALFILE_OUT,	dz);	break;
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
	case(SHIFT):	  	return(FINISHED);
	case(GLIS):		  	exit_status = set_internalparam_data("ddddddd",ap);				break;
	case(WAVER):      	exit_status = set_internalparam_data("ii",ap);					break;
	case(WARP):		  	exit_status = set_internalparam_data("iii",ap);					break;
	case(INVERT):	  	return(FINISHED);
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
	switch(dz->process) {
	case(SHIFT):		return setup_internal_arrays_for_specshift(dz);
	case(GLIS):			return establish_internal_params_for_glis(dz);
	case(WAVER):		return setup_internal_arrays_for_waver(dz);
	case(WARP):			return warp_preprocess(dz);	
	case(INVERT):		return invert_preprocess(dz);
	default:
		sprintf(errstr,"PROGRAMMING PROBLEM: Unknown process in param_preprocess()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);	/* NOTREACHED */
}

/************ SETUP_INTERNAL_ARRAYS_FOR_SPECSHIFT *************/

int setup_internal_arrays_for_specshift(dataptr dz)
{
	int n;
	if((dz->iparray[SHIFT_OVER]  = (int *)malloc(dz->clength * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for shift over array.\n");
		return(MEMORY_ERROR);
	}
	if((dz->iparray[SHIFT_DONE]  = (int *)malloc(dz->clength * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for shift done array.\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[SHIFT_CHTOP]  
		= (double *)malloc(dz->clength * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for shift channel top array.\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[SHIFT_CHMID]  
		= (double *)malloc(dz->clength * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for shift channel mid array.\n");
		return(MEMORY_ERROR);
	}
	dz->parray[SHIFT_CHTOP][0] = dz->halfchwidth;
	dz->parray[SHIFT_CHMID][0] = 0.0;
	for(n = 1;n < dz->clength; n++) {
		dz->parray[SHIFT_CHTOP][n] = dz->parray[SHIFT_CHTOP][n-1] + dz->chwidth;
		dz->parray[SHIFT_CHMID][n] = dz->parray[SHIFT_CHMID][n-1] + dz->chwidth;
	}
	dz->parray[SHIFT_CHMID][0] = 1.0;	 /* arbitrary, but NOT zero!! */
	return(FINISHED);
}

/********************** ESTABLISH_INTERNAL_PARAMS_FOR_GLIS **********************/

int establish_internal_params_for_glis(dataptr dz)
{
	dz->param[GLIS_CONVERTOR] = dz->frametime/SEMITONES_PER_OCTAVE;
	if(dz->brksize[GLIS_RATE]==0)
		dz->param[GLIS_RATE] *= dz->param[GLIS_CONVERTOR];
	dz->param[GLIS_HALF_SHIFT]   = dz->param[GLIS_SHIFT]/2.0;
	dz->param[GLIS_REFPITCH]      = log(GLIS_REFERENCE_FRQ);
	dz->param[GLIS_HALF_REFPITCH] = log(GLIS_REFERENCE_FRQ/2.0);
	if(dz->vflag[GLIS_FTOP]) {
		dz->param[GLIS_FRQTOP_TOP] = min(dz->nyquist,dz->param[GLIS_HIFRQ] + ROLL_OFF);
		dz->param[GLIS_ROLL_OFF]   = 
			min(dz->param[GLIS_FRQTOP_TOP] - dz->param[GLIS_HIFRQ], ROLL_OFF);
	}
	dz->param[GLIS_BASEFRQ] = GLIS_REFERENCE_FRQ;
	return(FINISHED);
}

/************ SETUP_INTERNAL_ARRAYS_FOR_WAVER *************/

int setup_internal_arrays_for_waver(dataptr dz)
{
	int exit_status;
   	if((dz->parray[WAVER_CHFBOT] = (double *)malloc((dz->clength + 1) * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for waver channel bottom array.\n");
		return(MEMORY_ERROR);
	}
	if((exit_status = setup_array_of_botfrq_of_each_channel(&(dz->iparam[WAVER_BOTCHAN]),dz))<0)
		return(exit_status);
   	dz->iparam[WAVER_STRCHANS] = dz->clength - dz->iparam[WAVER_BOTCHAN];
	/* No of chans to be stretched */

	return(FINISHED);
}

/*********************** SETUP_ARRAY_OF_BOTFRQ_OF_EACH_CHANNEL *********************/

int setup_array_of_botfrq_of_each_channel(int *botchan,dataptr dz)
{
	int n = 1;
	double boundary; 
	*botchan = 0;
	dz->parray[WAVER_CHFBOT][0] = 0.0;
	boundary  = dz->chwidth/2.0;
	while(boundary < dz->nyquist && n < dz->clength) {
		dz->parray[WAVER_CHFBOT][n] = boundary;
		if(*botchan==0 && (dz->param[WAVER_LOFRQ] < boundary)) 	/* IF botchan not set */
			*botchan = n;		/* AND if botfrq BELOW this chan */
					     		/* set botchan to THIS chan */
		boundary += dz->chwidth;
		n++;
	}
	(*botchan)--;			   /* RESET botchan to its true location */
	if(n!=dz->clength) {
		sprintf(errstr,"arithmetic error in setup_array_of_botfrq_of_each_channel()\n");
		return(PROGRAM_ERROR);
	}
	dz->parray[WAVER_CHFBOT][dz->clength] = dz->nyquist; 
	/* upper boundary channel clength-1 */
	return(FINISHED);
}

/************************** WARP_PREPROCESS ******************************/

int warp_preprocess(dataptr dz)
{
	int exit_status;
	if((exit_status = convert_params_for_warp(dz))<0)
		return(exit_status);
	if((exit_status = setup_internal_arrays_and_params_for_specwarp(dz))<0)
		return(exit_status);
	return convert_pitchdata_to_midi_for_warp(dz);
}

/************ CONVERT_PARAMS_FOR_WARP *************/

int convert_params_for_warp(dataptr dz)
{
	int exit_status;
	if((exit_status = convert_msecs_to_secs(WARP_TRNG,dz))<0)
		return(exit_status);
	return convert_msecs_to_secs(WARP_SRNG,dz);
}

/************ SETUP_INTERNAL_ARRAYS_AND_PARAMS_FOR_SPECWARP *************/

int setup_internal_arrays_and_params_for_specwarp(dataptr dz)
{
	double d;
	int cc;
	if((dz->parray[WARP_P2]	     
		= (double *)malloc(dz->wlength * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for warp array.\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[WARP_AVP]     = (double *)malloc(((dz->wlength/BLOKCNT) + 2) * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for warp average array.\n");
		return(MEMORY_ERROR);
	}
	if((dz->iparray[WARP_CHANGE] = (int    *)malloc(((dz->wlength/BLOKCNT) + 2) * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for warp change array.\n");
		return(MEMORY_ERROR);
	}
	dz->flbufptr[WCHTOP] = dz->windowbuf[0];	   /* use existing windowbuf for chtop and chbot store */
	dz->flbufptr[WCHBOT] = dz->windowbuf[0] + dz->clength;
	d = 0.0;
	for(cc=0; cc < dz->clength;cc++)	{
		dz->flbufptr[WCHTOP][cc] = (float)min(dz->nyquist,d + (dz->chwidth * (double)CHANSPAN));
		dz->flbufptr[WCHBOT][cc] = (float)max(0.0,d - (dz->chwidth * (double)CHANSPAN));
 		d += dz->chwidth;
	}
	dz->iparam[WARP_PART_INBUF] = 0;
	dz->iparam[WARP_ATEND] = 0;
	return(FINISHED);
}

/************ CONVERT_PITCHDATA_TO_MIDI_FOR_WARP *************/

int convert_pitchdata_to_midi_for_warp(dataptr dz)
{
	int exit_status;
	int n;
	double val;
	for(n=0;n<dz->wlength;n++)	{
		if(dz->pitches[n]<MINPITCH)
			dz->pitches[n] = -1.0f;
		else {
			if((exit_status = hztomidi(&val,dz->pitches[n]))<0)
				return(exit_status);
			dz->pitches[n] = (float)val;
		}
	}
	return(FINISHED);
}

/************************** INVERT_PREPROCESS ******************************/

int invert_preprocess(dataptr dz)
{
	int exit_status;
	if((exit_status = setup_internal_arrays_for_invert(dz))<0)
		return(exit_status);
	return extract_normalising_criteria_by_reading_infile(dz);
}

/************ SETUP_INTERNAL_ARRAYS_FOR_INVERT *************/

int setup_internal_arrays_for_invert(dataptr dz)
{
	int cc, vc; 
   	if((dz->parray[INV_AMPRATIO] = (double *)malloc(dz->clength * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for invert amplitude ratio array.\n");
		return(MEMORY_ERROR);
	}
	for(cc = 0, vc = 0; cc < dz->clength; cc++, vc += 2) {
		dz->amp[cc] 		 = (float)(-BIGAMP);
		dz->parray[INV_AMPRATIO][cc] = -BIGAMP;
	}
	return(FINISHED);
}

/*************** EXTRACT_NORMALISING_CRITERIA_BY_READING_INFILE ********************/

int extract_normalising_criteria_by_reading_infile(dataptr dz)
{
	int exit_status;
	int samps_read, w_to_buf, wc;
	double pre_totalamp;
	int cc, vc;
	fprintf(stdout,"INFO: Extracting normalisation criteria.\n");
	fflush(stdout);
	while((samps_read = fgetfbufEx(dz->bigfbuf,dz->big_fsize,dz->ifd[0],0)) > 0) {
		dz->flbufptr[0] = dz->bigfbuf;     
		w_to_buf    = samps_read/dz->wanted;
		for(wc=0; wc<w_to_buf; wc++) {
			switch(dz->mode) {
			case(INV_NORMAL):
				for(cc = 0, vc = 0; cc < dz->clength; cc++, vc += 2)
					dz->amp[cc] = max(dz->amp[cc],dz->flbufptr[0][vc]);
				break;
			case(INV_KEEPAMP):
				if((exit_status = get_totalamp(&pre_totalamp,dz->flbufptr[0],dz->wanted))<0)
					return(exit_status);
				if(pre_totalamp > MINAMP) {
					for(cc = 0, vc = 0; cc < dz->clength; cc++, vc += 2)
						dz->parray[INV_AMPRATIO][cc] = 
						max(dz->parray[INV_AMPRATIO][cc],dz->flbufptr[0][vc]/pre_totalamp);
				}
				break;
			default:
				sprintf(errstr,"Unknown progmode in extract_normalising_criteria_by_reading_infile()\n"); 
				return(PROGRAM_ERROR);
			}
			dz->flbufptr[0] += dz->wanted; 
		}
	}
	if(samps_read < 0) {
		sprintf(errstr,"Sound read error.\n");
		return(SYSTEM_ERROR);
	}
	if(sndseekEx(dz->ifd[0],0,0)<0) {
		sprintf(errstr,"seek failed in extract_normalising_criteria_by_reading_infile()\n");
		return(SYSTEM_ERROR);
	}
	fprintf(stdout,"INFO: Processing the file.\n");
	fflush(stdout);
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
	case(SHIFT):	return specshift(dz);
	case(GLIS):		return outer_loop(dz);
	case(INVERT):	return outer_loop(dz);

	case(WAVER): 	return specwaver(dz);
	case(WARP): 	return specwarp(dz);
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
		case(GLIS):		exit_status = specglis(dz);	  						break;
		case(INVERT):	exit_status = specinvert(dz); 						break;
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
	int exit_status = FINISHED;
	switch(dz->process) {
	case(INVERT): 	case(GLIS): 	
		switch(dz->process) {
		case(GLIS):
			if((exit_status = initialise_window_frqs(dz))<0)
				return(exit_status);
			break;
		}
		return(TRUE);
	}
	return(FALSE);
}

/********************************************************************************************/
/********************************** FORMERLY IN pconsistency.c ******************************/
/********************************************************************************************/

/****************************** CHECK_PARAM_VALIDITY_AND_CONSISTENCY *********************************/

int check_param_validity_and_consistency(dataptr dz)
{
	handle_pitch_zeros(dz);
	return(FINISHED);
}

/********************************************************************************************/
/********************************** FORMERLY IN buffers.c ***********************************/
/********************************************************************************************/

/**************************** ALLOCATE_LARGE_BUFFERS ******************************/

int allocate_large_buffers(dataptr dz)
{
	switch(dz->process) {
	case(SHIFT):	case(GLIS):		case(WAVER):	
	case(INVERT):	
		return allocate_single_buffer(dz);

	case(WARP):		
		return allocate_warp_buffer(dz);
	default:
		sprintf(errstr,"Unknown program no. in allocate_large_buffers()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);	/* NOTREACHED */
}

/*************************** ALLOCATE_WARP_BUFFER ****************************
 *
 *	2 equal large buffs and an overflow of dz->wanted floats.
 */

int allocate_warp_buffer(dataptr dz)
{
	unsigned int buffersize;
	if(dz->bptrcnt < 7) {
		sprintf(errstr,"Insufficient bufptrs established in allocate_warp_buffer()\n");
		return(PROGRAM_ERROR);
	}
//TW REVISED, secsize mult of bufs not needed
	buffersize = dz->wanted * BUF_MULTIPLIER;
	dz->buflen = buffersize;
	buffersize *= 2;
	buffersize += dz->wanted;
	if((dz->bigfbuf	= (float*)malloc((size_t)buffersize * sizeof(float)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for sound buffers.\n");
		return(MEMORY_ERROR);
	}

	dz->big_fsize   = dz->buflen;
	dz->flbufptr[0] = dz->bigfbuf;
	dz->flbufptr[2] = dz->flbufptr[0] + dz->big_fsize;
	dz->flbufptr[1] = dz->flbufptr[2];
	dz->flbufptr[3] = dz->flbufptr[2] + dz->big_fsize;
	return(FINISHED);
}

/********************************************************************************************/
/********************************** FORMERLY IN cmdline.c ***********************************/
/********************************************************************************************/

int get_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	if     (!strcmp(prog_identifier_from_cmdline,"shift"))	   		dz->process = SHIFT;
	else if(!strcmp(prog_identifier_from_cmdline,"glis"))	   		dz->process = GLIS;
	else if(!strcmp(prog_identifier_from_cmdline,"waver"))	   		dz->process = WAVER;
	else if(!strcmp(prog_identifier_from_cmdline,"warp"))  	   		dz->process = WARP;
	else if(!strcmp(prog_identifier_from_cmdline,"invert"))    		dz->process = INVERT;
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
	"\nSTRANGE OPERATIONS ON A SPECTRAL FILE\n\n"
	"USAGE: strange NAME (mode) infile outfile parameters: \n"
	"\n"
	"where NAME can be any one of\n"
	"\n"
	"shift    glis    waver    invert\n\n"
	"Type 'strange shift' for more info on strange shift..ETC.\n");
	return(USAGE_ONLY);
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
	if(!strcmp(str,"shift")) {
		fprintf(stdout,
		"strange shift 1   infile outfile frqshift               [-l]\n"
		"strange shift 2-3 infile outfile frqshift frq_divide    [-l]\n"
		"strange shift 4-5 infile outfile frqshift frqlo   frqhi [-l]\n"
		"\n"
		"LINEAR FREQUENCY SHIFT OF (PART OF) THE SPECTRUM\n"
		"\n"
		"MODES :-\n"
		"1   Shift the whole spectrum.\n"
		"2   Shift the spectrum above frq_divide.\n"
		"3   Shift the spectrum below frq_divide.\n"
		"4   Shift the spectrum only in the range frqlo and frqhi.\n"
		"5   Shift the spectrum outside the range  frqlo to frqhi.\n"
		"\n"
		"frqshift      linear shift of spectral frequencies (same for all).\n"
		"frq_divide    frq at which shifting starts or stops.\n"
		"frqlo & frqhi define a range inside or outside of which shifting takes place.\n"
		"-l            log interpolation between varying frq vals (Default: linear).\n"
		"              (Useful only if any of above input parameters are time-varying,\n"
	   	"               AND frqshift vals must be all +ve, or all -ve for this to work).\n"
		"\n"
		"frqshift,frq_divide,frqlo & frqhi may vary over time.\n");
	} else if(!strcmp(str,"glis")) {
		fprintf(stdout,
		"strange glis 1 infile outfile -fN|-pN [-i] glisrate        [-ttopfrq] \n"
		"strange glis 2 infile outfile -fN|-pN [-i] glisrate hzstep [-ttopfrq] \n"
		"strange glis 3 infile outfile -fN|-pN [-i] glisrate        [-ttopfrq] \n"
		"\n"
		"CREATE GLISSANDI INSIDE THE (CHANGING) SPECTRAL ENVELOPE OF ORIGINAL SOUND\n"
		"\n"
		"MODES :-\n"
		"1   shepard tones.\n"
		"2   inharmonic glide.\n"
		"3   self-glissando.\n"
		"\n"
		"-f     extract formant envelope linear frqwise,\n"
		"       using 1 point for every N equally-spaced frequency-channels.\n"
		"-p     extract formant envelope linear pitchwise,\n"
		"       using N equally-spaced pitch-bands per octave.\n"
		"-i     quicksearch for formants (less accurate).\n"
		"\n"
		"glisrate rate of glissing in semitones per second (-ve val for downward gliss).\n"
		"hzstep   partials-spacing in inharmonic glide.\n"
	    "         Range: FROM channel-frq-width TO nyquist/2.\n"
		"topfrq   top of spectrum: must be > 2*chanwidth of analysis (default:nyquist)\n"
		"\n"
		"glisrate may vary over time.\n");
	} else if(!strcmp(str,"waver")) {
		fprintf(stdout,
		"strange waver 1 infile outfile vibfrq stretch botfrq\n"
		"strange waver 2 infile outfile vibfrq stretch botfrq expon\n"
		"\n"
		"OSCILLATE BETWEEN HARMONIC AND INHARMONIC STATE\n"
		"\n"
		"MODES :-\n"
		"1   Standard spectral stretching for inharmonic state.\n"
		"2   Specify spectral stretching for inharmonic state.\n"
		"\n"
		"vibfrq    is frq of oscillation. \n"
		"stretch   is maximum spectral stretch in inharmonic state. \n"
		"botfrq    is frq above which spectral stretching happens.\n"
		"expon     defines type of stretch (must be > 0.0).\n"
		"\n"
		"vibfrq and stretch may vary over time.\n");
/***
	} else if(!strcmp(str,"warp")) {
		fprintf(stdout,
		"strange warp infile pitchfile outfile [-pprange] [-ttrange] [-ssrange]\n\n"
		"PRODUCE AN APPROXIMATE COPY OF SPECTRUM.\n\n"
		"PITCHFILE must be derived from infile.\n"
		"PRANGE    Interval (semitones) over which pitch varies\n"
		"          +- randomly from orig. Range > 0.0.\n"
		"TRANGE    Time-interval (msecs) by which pitch can stray from orig time.\n"
		"          Range: duration of 1 analysis window to dur of entire file.\n"
		"SRANGE    Time-interval (msecs) over which pitch contour scanned.\n"
		"          Pitchshift by %.2lf semitones within scanrange, indicates\n"
		"          pitch rise or fall.\n"
		"          Range: from dur of %d anal windows to (approx) dur of entire file.\n\n"
		"prange, trange and srange may vary over time.\n",WARP_MININT,2*BLOKCNT);
***/
	} else if(!strcmp(str,"invert")) {
		fprintf(stdout,
		"strange invert mode infile outfile\n"
		"\n"
		"INVERT THE SPECTRUM\n"
		"\n"
		"MODES :-\n"
		"1	Normal inversion.\n"
		"2	Output sound retains amplitude envelope of source sound.\n");
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

