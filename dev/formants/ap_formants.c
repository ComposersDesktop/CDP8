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
#include <fmnts.h>
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
/********************************** FORMERLY IN pconsistency.c ******************************/
/********************************************************************************************/

static int 	check_compatibility_of_params_for_form(dataptr dz);

/********************************************************************************************/
/********************************** FORMERLY IN procspec.c **********************************/
/********************************************************************************************/

static int  zero_formant(dataptr dz);

/********************************************************************************************/
/********************************** FORMERLY IN preprocess.c ********************************/
/********************************************************************************************/

#define MSECOND  (1.059463)
#define SECOND   (1.122462)
#define MTHIRD   (1.1892)
#define THIRD    (1.259921)
#define FOURTH   (1.33484)
#define TRITONE  (1.414214)
#define FIFTH    (1.4983)
#define MSIXTH   (1.5874)
#define SIXTH    (1.6818)
#define MSEVENTH (1.781797)
#define SEVENTH  (1.88775)
static int  formants_preprocess(dataptr dz);
static int  setup_internal_arrays_and_params_for_fmntsee(dataptr dz);
//TW REMOVED: redundant
//static int  get_max_formantval(dataptr dz);
static int  setup_internal_params_and_arrays_for_formsee(dataptr dz);
static int  get_formant_sample_points(int *fcnt,dataptr dz);

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
	case(FORMANTS):   		dz->extra_bufcnt =  0; dz->bptrcnt = 4; 	is_spec = TRUE;		break;
	case(FORM):       		dz->extra_bufcnt =  0; dz->bptrcnt = 4; 	is_spec = TRUE;		break;
	case(VOCODE):     		dz->extra_bufcnt =  0; dz->bptrcnt = 4; 	is_spec = TRUE;		break;
	case(FMNTSEE):    		dz->extra_bufcnt =  0; dz->bptrcnt = 1; 	is_spec = TRUE;		break;
	case(FORMSEE):    		dz->extra_bufcnt =  1; dz->bptrcnt = 1; 	is_spec = TRUE;		break;
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
	dz->ptr_cnt    = -1;		//base constructor...process
	dz->array_cnt  = -1;
	dz->iarray_cnt = -1;
	dz->larray_cnt = -1;
	switch(dz->process) {
	case(FORMANTS):	dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(FORM):	  	dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(VOCODE): 	dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(FMNTSEE):	dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(FORMSEE):	dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
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
	case(FORMANTS):		setup_process_logic(ANALFILE_ONLY,		  	ANAL_TO_FORMANTS,	FORMANTS_OUT,	dz);	break;
	case(FORM):		  	setup_process_logic(ANAL_AND_FORMANTS,	  	BIG_ANALFILE,		ANALFILE_OUT,	dz);	break;
	case(VOCODE):	  	setup_process_logic(TWO_ANALFILES,		  	MIN_ANALFILE,		ANALFILE_OUT,	dz);	break;
	case(FMNTSEE):	  	setup_process_logic(FORMANTFILE_ONLY,	  	PSEUDOSNDFILE,		SNDFILE_OUT,	dz);	break;
	case(FORMSEE):	  	setup_process_logic(ANALFILE_ONLY,	  		PSEUDOSNDFILE,		SNDFILE_OUT,	dz);	break;
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
	mode = 0;
	switch(process) {
	case(FORMANTS):	  	exit_status = set_internalparam_data("i",ap);					break;
	case(FORM):		  	return(FINISHED);
	case(VOCODE):	  	return(FINISHED);									
	case(FMNTSEE):	  	exit_status = set_internalparam_data("d",ap);					break;
	case(FORMSEE):	  	exit_status = set_internalparam_data("di",ap);					break;
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
	return(FINISHED);
}

/********************************************************************************************/
/********************************** FORMERLY IN preprocess.c ********************************/
/********************************************************************************************/


/****************************** PARAM_PREPROCESS *********************************/

int param_preprocess(dataptr dz)	
{
	switch(dz->process) {
	case(FORMANTS):		return formants_preprocess(dz);
	case(VOCODE):		return initialise_specenv2(dz);
	case(FMNTSEE):		return setup_internal_arrays_and_params_for_fmntsee(dz);
	case(FORMSEE):		return setup_internal_params_and_arrays_for_formsee(dz);
	case(FORM):			
		break;
	default:
		sprintf(errstr,"PROGRAMMING PROBLEM: Unknown process in param_preprocess()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/************************** FORMANTS_PREPROCESS ******************************/

int formants_preprocess(dataptr dz)
{
	int exit_status;
	if((exit_status = write_formant_descriptor(&dz->flbufptr[1],dz->flbufptr[2],dz))<0)
		return(exit_status);
	dz->iparam[FMNT_SAMPS_TO_WRITE] = dz->descriptor_samps;
	return(FINISHED);
}

/******************** PRINT_FORMANT_PARAMS_TO_SCREEN **********************/

void print_formant_params_to_screen(dataptr dz)
{
/* CORRECTIONS: June 1999 */
	int n;
	float *bf = dz->bigfbuf;
	char temp[200];
	
	fprintf(stdout,"INFO: Number of formant bands %d\n",dz->infile->specenvcnt);
	fprintf(stdout,"INFO: Formant envelope frequency band centres (hz)\n");
	sprintf(errstr,"INFO: ");
	for(n=0;n<dz->infile->specenvcnt;n++) {
		sprintf(temp," frq[%d] = %.1f",n,exp(*bf++));
		strcat(errstr,temp);
		if((n % 3)==2) {
			fprintf(stdout,"%s\n",errstr);
			sprintf(errstr,"INFO: ");
		} else {
			strcat(errstr,"\t");
		}
	}
	if((n % 3) > 0)
		fprintf(stdout,"%s\n",errstr);
	fprintf(stdout,"INFO: Formant band tops (HZ)\n");
	sprintf(errstr,"INFO: ");
	for(n=0;n<dz->infile->specenvcnt;n++) {
		sprintf(temp," top[%d] = %.1f",n,*bf++);
		strcat(errstr,temp);
		if((n % 3)==2) {
			fprintf(stdout,"%s\n",errstr);
			sprintf(errstr,"INFO: ");
		} else {
			strcat(errstr,"\t");
		}
	}
	if((n % 3) > 0)
		fprintf(stdout,"%s\n",errstr);
	fflush(stdout);
}

/************ SETUP_INTERNAL_ARRAYS_AND_PARAMS_FOR_FMNTSEE *************/

int setup_internal_arrays_and_params_for_fmntsee(dataptr dz)
{
    dz->wlength = dz->insams[0]/dz->wanted;	/* sample frames to process */
	if((dz->sndbuf  = (float *)malloc((size_t)(((dz->infile->specenvcnt + SPACER)*sizeof(float)) * FMNT_BUFMULT)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for pseudosound buffer.\n");
		return(MEMORY_ERROR);
	}
	return(FINISHED);
}

/***************************** GET_MAX_FORMANTVAL *************************/

//TW UPDATE: get_max_formantval REMOVED , REDUNDANT

/************ SETUP_INTERNAL_PARAMS_AND_ARRAYS_FOR_FORMSEE *************/

int setup_internal_params_and_arrays_for_formsee(dataptr dz)
{
	int exit_status;
    dz->wlength = dz->insams[0]/dz->wanted;	/* sample frames to process */
	if((exit_status = get_formant_sample_points(&(dz->iparam[FSEE_FCNT]),dz))<0)
		return(exit_status);
	dz->param[FSEE_FMAX]  = 0.0;
	if((dz->sndbuf = (float *)malloc(dz->clength * FMNT_BUFMULT * sizeof(float)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for formant pseudo-sound array.\n");
		return(MEMORY_ERROR);
	}
	return(FINISHED);
}

/*********************** GET_FORMANT_SAMPLE_POINTS **********************/

int get_formant_sample_points(int *fcnt,dataptr dz)
{
	double thisfrq = dz->chwidth, frqhere, triple_band = 0.0;
	int n = 0;
	switch(dz->vflag[FSEE_DISPLAY]) {
	case(PICHWISE_FORMANTS):
		while(thisfrq < dz->nyquist) {
			n++;
			if((frqhere=thisfrq*MSECOND)< dz->nyquist) n++;	    else    break;
			if((frqhere=thisfrq*SECOND) < dz->nyquist) n++;	    else    break;
			if((frqhere=thisfrq*MTHIRD) < dz->nyquist) n++;	    else    break;
			if((frqhere=thisfrq*THIRD)  < dz->nyquist) n++;	    else    break;
			if((frqhere=thisfrq*FOURTH) < dz->nyquist) n++;	    else    break;
			if((frqhere=thisfrq*TRITONE)< dz->nyquist) n++;	    else    break;
			if((frqhere=thisfrq*FIFTH)  < dz->nyquist) n++;	    else    break;
			if((frqhere=thisfrq*MSIXTH) < dz->nyquist) n++;	    else    break;
			if((frqhere=thisfrq*SIXTH)  < dz->nyquist) n++;	    else    break;
			if((frqhere=thisfrq*MSEVENTH)<dz->nyquist) n++;	    else    break;
			if((frqhere=thisfrq*SEVENTH)< dz->nyquist) n++;	    else    break;
			thisfrq *= 2.0;
		}
		*fcnt = n;
		break;
	case(FREQWISE_FORMANTS):
		triple_band = dz->chwidth * 3.0;
		while(thisfrq < dz->nyquist) {
			n++;
			thisfrq += triple_band;
		}
		*fcnt = n;
		break;
	}
	if((dz->windowbuf[0] = (float *)realloc((char *)dz->windowbuf[0],(*fcnt) * sizeof(float)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for formant sample points buffer.\n");
		return(MEMORY_ERROR);
	}
	n = 0;
	thisfrq = dz->chwidth;
	switch(dz->vflag[FSEE_DISPLAY]) {
	case(PICHWISE_FORMANTS):
		for(;;) {
			dz->windowbuf[0][n++]=(float)thisfrq;	       	   if(n >= *fcnt) break;
			dz->windowbuf[0][n++]=(float)(thisfrq * MSECOND);  if(n >= *fcnt) break;
			dz->windowbuf[0][n++]=(float)(thisfrq * SECOND);   if(n >= *fcnt) break;
			dz->windowbuf[0][n++]=(float)(thisfrq * MTHIRD);   if(n >= *fcnt) break;
			dz->windowbuf[0][n++]=(float)(thisfrq * THIRD);    if(n >= *fcnt) break;
			dz->windowbuf[0][n++]=(float)(thisfrq * FOURTH);   if(n >= *fcnt) break;
			dz->windowbuf[0][n++]=(float)(thisfrq * TRITONE);  if(n >= *fcnt) break;
			dz->windowbuf[0][n++]=(float)(thisfrq * FIFTH);    if(n >= *fcnt) break;
			dz->windowbuf[0][n++]=(float)(thisfrq * MSIXTH);   if(n >= *fcnt) break;
			dz->windowbuf[0][n++]=(float)(thisfrq * SIXTH);    if(n >= *fcnt) break;
			dz->windowbuf[0][n++]=(float)(thisfrq * MSEVENTH); if(n >= *fcnt) break;
			dz->windowbuf[0][n++]=(float)(thisfrq * SEVENTH);  if(n >= *fcnt) break;
			thisfrq *= 2.0;
		}
		break;
	case(FREQWISE_FORMANTS):
		while(thisfrq < dz->nyquist) {
			dz->windowbuf[0][n++] = (float)thisfrq;
			thisfrq += triple_band;
		}
		break;
	}
	if(*fcnt > dz->clength || *fcnt==0) {
		sprintf(errstr,"Invalid formant_cnt: get_formant_sample_points()\n");
		return(PROGRAM_ERROR);
	}
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
	case(FORMANTS):	return specformants(dz);
	case(FORM):		return specform(dz);
	case(VOCODE):	return outer_twofileinput_loop(dz);
	case(FMNTSEE):	return specfmntsee(dz);	
	case(FORMSEE):	return outer_formsee_loop(dz);
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
		case(FORMSEE):	exit_status = specformsee(dz);						break;
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
//TW UPDATE
//	if(!dz->zeroset && local_zero_set==TRUE) {
	if(!dz->zeroset && (local_zero_set==TRUE)) {
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
	case(FORMANTS): case(FORMSEE): case(FMNTSEE):	case(VOCODE): 
		switch(dz->process) {
		case(FORMSEE):
		case(FMNTSEE):
			memset((char *)dz->flbufptr[0],0,(size_t)dz->wanted * sizeof(float));
			break;
		case(FORMANTS):
			if((exit_status = zero_formant(dz))<0)
				return(exit_status);
			if((dz->iparam[FMNT_SAMPS_TO_WRITE] += dz->infile->specenvcnt) >= dz->buflen2) {
				if((exit_status = write_exact_samps(dz->flbufptr[2],dz->buflen2,dz))<0)
					return(exit_status);
				dz->iparam[FMNT_SAMPS_TO_WRITE] = 0;
				dz->flbufptr[1] = dz->flbufptr[2];
			} else
				dz->flbufptr[1] += dz->infile->specenvcnt;
			break;
		}
		return(TRUE);
	}
	return(FALSE);
}

/**************************** OUTER_TWOFILEINPUT_LOOP ***************************/

int outer_twofileinput_loop(dataptr dz)
{
	int exit_status;
	int windows_to_process, windows_in_buf, samps_read, samps_to_write, wc, got;
	int file_to_keep = 0, finished = 0, stop_at_end_of_shortest_file = 0;
	int stop_at_end_of_process = 0;
	dz->time = 0.0f;
	switch(dz->process) {
	case(VOCODE):
		windows_to_process = min(dz->insams[0],dz->insams[1])/dz->wanted;
		stop_at_end_of_shortest_file = 1;
		break;
	default:
		sprintf(errstr,"unknown case in outer_twofileinput_loop()\n");
		return(PROGRAM_ERROR);
	}
   	while(!finished) {

		if((exit_status = read_both_files(&windows_in_buf,&got,dz))<0)
			return(exit_status);
		samps_to_write = got;
 		for(wc=0; wc<windows_in_buf; wc++) {
			if(dz->total_windows==0) {
				if((exit_status = skip_or_special_operation_on_window_zero(dz))<0)
				 	return(exit_status);
				if(exit_status==TRUE) {
					if((exit_status = advance_one_2fileinput_window(dz))<0)
						return(exit_status);
					continue;
				}
			}
			if((exit_status = read_values_from_all_existing_brktables((double)dz->time,dz))<0)
				return(exit_status);
			switch(dz->process) {
			case(VOCODE): 
				if((exit_status = specvocode(dz))<0)
					return(exit_status);
				break;
			default:
				sprintf(errstr,"unknown process in outer_twofileinput_loop()\n");
				return(PROGRAM_ERROR);
			}
			if((exit_status = advance_one_2fileinput_window(dz))<0)
				return(exit_status);
			if(dz->total_windows >= windows_to_process) {
				if((exit_status = keep_excess_samps_from_correct_file(&samps_to_write,file_to_keep,got,wc,dz))<0)
					return(exit_status);
				finished = 1;
				break;
			}
		}
		if(samps_to_write > 0) {
			if((exit_status = write_samps(dz->bigfbuf,samps_to_write,dz))<0)
				return(exit_status);
		}
	}
	if(!stop_at_end_of_process && !stop_at_end_of_shortest_file) {
		if((exit_status = read_either_file(&samps_read,file_to_keep,dz))<0)
			return(exit_status);
		while(samps_read > 0) {
			if((exit_status = write_samps(dz->bigfbuf,samps_read,dz))<0)
				return(exit_status);
			if((exit_status = read_either_file(&samps_read,file_to_keep,dz))<0)
				return(exit_status);
		}
	}
	return(FINISHED);
}

/****************************** ZERO_FORMANT ***************************/

int zero_formant(dataptr dz)
{
	int n;
	for(n=0;n<dz->infile->specenvcnt;n++)
		dz->flbufptr[1][n] = 0.0f;
	return(FINISHED);
}

/********************************************************************************************/
/********************************** FORMERLY IN pconsistency.c ******************************/
/********************************************************************************************/

/****************************** CHECK_PARAM_VALIDITY_AND_CONSISTENCY *********************************/

int check_param_validity_and_consistency(dataptr dz)
{
	int exit_status, chans, stype;
	int srate;
	char filename[200];
	handle_pitch_zeros(dz);
	switch(dz->process) {
	case(FORMANTS):
		chans = dz->infile->channels;
//TW 2003: specenvcnt is number of pieces of data in a single formant
//TW 2004: it would be more logical for channels to be equal to specenvcnt
//TW However, this breaks some other things ...notably 
//TW the gobo, which will only accept Formant,Pitch (and Envelope) files, for recombination
//TW if they share the same number of channels (i.e. 1)
//TW Rewriting the gobo program is one's worst nightmare!!!
		dz->infile->channels = 1;
		if((exit_status = create_sized_outfile(dz->wordstor[0],dz))<0)
			return(exit_status);
		dz->infile->channels = chans;
		break;
	case(FORM):		   
		return check_compatibility_of_params_for_form(dz);
	case(FMNTSEE):
		chans = dz->infile->channels;
		srate = dz->infile->srate;
		stype = dz->infile->stype;
		dz->infile->channels = 1;
		dz->infile->srate    = dz->infile->origrate;
		dz->infile->stype	 = SAMP_SHORT;
		if((exit_status = create_sized_outfile(dz->wordstor[0],dz))<0)
			return(exit_status);
		dz->infile->channels = chans;
		dz->infile->srate    = srate;
		dz->infile->stype = stype;
		break;
	case(FORMSEE):
		chans = dz->infile->channels;
		srate = dz->infile->srate;
		stype = dz->infile->stype;
		dz->infile->channels = 1;
		dz->infile->srate    = dz->infile->origrate;
		dz->infile->stype	 = SAMP_SHORT;
//TW  process uses tempfile before normalising output
		strcpy(filename,dz->outfilename);
		get_other_filename(filename,'1');
		if((exit_status = create_sized_outfile(filename,dz))<0)
			return(exit_status);
		dz->infile->channels = chans;
		dz->infile->srate    = srate;
		dz->infile->stype = stype;
		break;
	}
	return(FINISHED);
}

/************ CHECK_COMPATIBILITY_OF_PARAMS_FOR_FORM *************/

int check_compatibility_of_params_for_form(dataptr dz)
{
	if(dz->param[FORM_FTOP] <= dz->param[FORM_FBOT]) {
		sprintf(errstr,"Frequency limits incompatible\n");
		return(USER_ERROR);
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
	case(FMNTSEE):	case(FORMSEE):	
		return allocate_single_buffer(dz);

	case(VOCODE):	
		return allocate_double_buffer(dz);

	case(FORMANTS):	case(FORM):		
		return allocate_analdata_plus_formantdata_buffer(dz);
	default:
		sprintf(errstr,"Unknown program no. in allocate_large_buffers()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/********************************************************************************************/
/********************************** FORMERLY IN cmdline.c ***********************************/
/********************************************************************************************/

int get_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	if     (!strcmp(prog_identifier_from_cmdline,"get"))  			dz->process = FORMANTS;
	else if(!strcmp(prog_identifier_from_cmdline,"put"))  	   		dz->process = FORM;
	else if(!strcmp(prog_identifier_from_cmdline,"vocode"))    		dz->process = VOCODE;
	else if(!strcmp(prog_identifier_from_cmdline,"see"))	   		dz->process = FMNTSEE;
	else if(!strcmp(prog_identifier_from_cmdline,"getsee"))   		dz->process = FORMSEE;
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
	"\nEXTRACTION AND USE OF FORMANTS FROM SPECTRAL FILES\n\n"
	"USAGE: formants NAME (mode) infile (infile2) outfile parameters: \n"
	"\n"
	"where NAME can be any one of\n"
	"\n"
	"get     put     vocode     see      getsee\n\n"
	"Type 'formants get' for more info on formants get..ETC.\n");
	return(USAGE_ONLY);
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
	if(!strcmp(str,"get")) {
		fprintf(stdout,
		"formants get infile outfile -fN|-pN\n\n"
		"EXTRACT EVOLVING FORMANT ENVELOPE FROM AN ANALYSIS FILE.\n\n"
		"OUTFILE is a formant file, which can be used in other formant applications.\n\n"
		"-f     extract formant envelope linear frqwise,\n"
		"       using 1 point for every N equally-spaced frequency-channels.\n"
		"-p     extract formant envelope linear pitchwise,\n"
		"       using N equally-spaced pitch-bands per octave.\n");
	} else if(!strcmp(str,"put")) {
		fprintf(stdout,
		"formants put 1 infile fmntfile outfile [-i] [-llof] [-hhif] [-ggain]\n"
		"formants put 2 infile fmntfile outfile      [-llof] [-hhif] [-ggain]\n\n"
		"IMPOSE SPECTRAL ENVELOPE IN A FORMANTFILE ON SPECTRUM IN A PVOC ANALYSIS FILE.\n\n"
		"MODES.\n"
		"(1) New formant envelope REPLACES sound's own formant envelope.\n"
		"(2) New formant envelope IMPOSED ON TOP OF sound's own formant envelope.\n\n"
		"INFILE and OUTFILE are PVOC analysis files. FMNTFILE is a formant data file.\n"
		"-l      LOF  = low frq,  below which spectrum is set to zero.\n"
		"-h      HIF  = high frq, above which spectrum is set to zero.\n"
		"-g      GAIN = adjustment to spectrum loudness (normally < 1.0).\n"
		"-i      quicksearch for formants (less accurate).\n");
	} else if(!strcmp(str,"vocode")) {
		fprintf(stdout,
		"formants vocode infile infile2 outfile -fN|-pN [-llof] [-hhif] [-ggain]\n\n"
		"IMPOSE SPECTRAL ENVELOPE OF 2nd SOUND, ON 1st SOUND.\n\n"
		"-f     extract formant envelope linear frqwise,\n"
		"       using 1 point for every N equally-spaced frequency-channels.\n"
		"-p     extract formant envelope linear pitchwise,\n"
		"       using N equally-spaced pitch-bands per octave.\n"
		"-l     LOF = low frq,  below which data is filtered out.\n"
		"-h     HIF = high frq, above which data is filtered out.\n"
		"-g     GAIN adjustment to signal (normally < 1.0).\n");
	} else if(!strcmp(str,"see")) {
		fprintf(stdout,
		"formants see infile outsndfile [-v]\n\n"
		"CONVERT FORMANT DATA IN A BINARY FORMANTDATA FILE TO 'SNDFILE' FOR VIEWING.\n\n"
		"-v     display data about formant-band parameters.\n\n"
		"The resulting logarithmically scaled display indicates formant shapes,\n"
		"but NOT the absolute amplitude values.\n");
	} else if(!strcmp(str,"getsee")) {
		fprintf(stdout,
		"formants getsee infile outsndfile -fN|-pN [-s]\n\n"
		"EXTRACT FORMANTS FROM ANALFILE AND WRITE AS 'SNDFILE' FOR VIEWING.\n\n"
		"-f     extract formant envelope linear frqwise,\n"
		"       using 1 point for every N equally-spaced frequency-channels.\n"
		"-p     extract formant envelope linear pitchwise,\n"
		"       using N equally-spaced pitch-bands per octave.\n"
		"-s     semitone bands for display (Default: equal hz bands).\n\n"
		"The resulting logarithmically scaled display indicates formant shapes,\n"
		"but NOT the absolute amplitude values.\n"
		"\n"
//TW temporary cmdline restriction
		"(Do not use filenames which end in '1').\n");
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

