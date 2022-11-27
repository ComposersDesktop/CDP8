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
#include <specinfo.h>
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

static int	check_validity_of_octvu_params(dataptr dz);
static int 	check_consistency_of_report_params(dataptr dz);

/********************************************************************************************/
/********************************** FORMERLY IN preprocess.c ********************************/
/********************************************************************************************/

#define MARGIN_OF_ERROR (16) /* 8 channels * 2 for both amp AND frq */

static int  setup_internal_arrays_and_params_for_level(dataptr dz);
static int  calculate_octvu_energy_band_parameters(dataptr dz);
static int  specpeak_preprocess(dataptr dz);
static int  create_hearing_response_adjustment_curve(dataptr dz);
static int  get_frq_bands(int *cnt,dataptr dz);
static int	 setup_internal_params_and_arrays_for_specreport(dataptr dz);
static int  setup_internal_print_params(dataptr dz);

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
	case(WINDOWCNT):  		dz->extra_bufcnt =  0; dz->bptrcnt = 0; 	is_spec = TRUE;		break;
	case(CHANNEL):    		dz->extra_bufcnt =  0; dz->bptrcnt = 0; 	is_spec = TRUE;		break;
	case(FREQUENCY):  		dz->extra_bufcnt =  0; dz->bptrcnt = 0; 	is_spec = TRUE;		break;
	case(LEVEL):      		dz->extra_bufcnt =  0; dz->bptrcnt = 1; 	is_spec = TRUE;		break;
	case(OCTVU):      		dz->extra_bufcnt =  0; dz->bptrcnt = 1; 	is_spec = TRUE;		break;
	case(PEAK):	      		dz->extra_bufcnt =  0; dz->bptrcnt = 1; 	is_spec = TRUE;		break;
	case(REPORT):     		dz->extra_bufcnt =  0; dz->bptrcnt = 4; 	is_spec = TRUE;		break;
	case(PRINT):      		dz->extra_bufcnt =  0; dz->bptrcnt = 1; 	is_spec = TRUE;		break;
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
	case(WINDOWCNT):dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(CHANNEL):	dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(FREQUENCY):dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(LEVEL):  	dz->array_cnt =1;  dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(OCTVU):  	dz->array_cnt =1;  dz->iarray_cnt =2;  dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(PEAK):   	dz->array_cnt =2;  dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(REPORT): 	dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(PRINT):	dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
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
	case(WINDOWCNT):	setup_process_logic(ANALFILE_ONLY,		  	SCREEN_MESSAGE,		NO_OUTPUTFILE,	dz);	break;
	case(CHANNEL):		setup_process_logic(ANALFILE_ONLY,		  	SCREEN_MESSAGE,		NO_OUTPUTFILE,	dz);	break;
	case(FREQUENCY):	setup_process_logic(ANALFILE_ONLY,		  	SCREEN_MESSAGE,		NO_OUTPUTFILE,	dz);	break;
	case(LEVEL):		setup_process_logic(ANALFILE_ONLY,		  	PSEUDOSNDFILE,		SNDFILE_OUT,	dz);	break;
	case(OCTVU):		setup_process_logic(ANALFILE_ONLY,		  	TO_TEXTFILE,		TEXTFILE_OUT,	dz);	break;
	case(PEAK):			setup_process_logic(ANALFILE_ONLY,		  	TO_TEXTFILE,		TEXTFILE_OUT,	dz);	break;
	case(REPORT):		setup_process_logic(ANALFILE_ONLY,		  	TO_TEXTFILE,		TEXTFILE_OUT,	dz);	break;
	case(PRINT):		setup_process_logic(ANALFILE_ONLY,		  	TO_TEXTFILE,		TEXTFILE_OUT,	dz);	break;
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
	case(WINDOWCNT):  	return(FINISHED);
	case(CHANNEL):	  	return(FINISHED);
	case(FREQUENCY):  	return(FINISHED);
	case(LEVEL):	  	return(FINISHED);
	case(OCTVU):	  	exit_status = set_internalparam_data("di",ap);					break;
	case(PEAK):		  	exit_status = set_internalparam_data("dii",ap);					break;
	case(REPORT):	  	exit_status = set_internalparam_data("ii",ap);					break;
	case(PRINT):	  	exit_status = set_internalparam_data("ii",ap);					break;
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
	case(LEVEL):		return setup_internal_arrays_and_params_for_level(dz);
	case(OCTVU):		return calculate_octvu_energy_band_parameters(dz);
	case(PEAK):			return specpeak_preprocess(dz);
	case(REPORT):		return setup_internal_params_and_arrays_for_specreport(dz);
	case(PRINT):		return setup_internal_print_params(dz);

	case(WINDOWCNT):	case(CHANNEL):		case(FREQUENCY):	
		return(FINISHED);
	default:
		sprintf(errstr,"PROGRAMMING PROBLEM: Unknown process in param_preprocess()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);	/* NOTREACHED */
}

/************ SETUP_INTERNAL_ARRAYS_AND_PARAMS_FOR_LEVEL *************/

int setup_internal_arrays_and_params_for_level(dataptr dz)
{
	dz->wlength   = dz->insams[0]/dz->wanted;
    if((dz->parray[0] = (double *)malloc(dz->wlength * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for level info array.\n");
		return(MEMORY_ERROR);
	}
    if((dz->sndbuf    = (float  *)malloc(dz->wlength * sizeof(float))) ==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for level sound buffer.\n");
		return(MEMORY_ERROR);
	}
	return(FINISHED);
}

/******************** CALCULATE_OCTVU_ENERGY_BAND_PARAMETERS ******************/

int calculate_octvu_energy_band_parameters(dataptr dz)
{
	int n, last_chan_bandtop;
	double *bandtop;
	double last_bandtop, bandmid;

	if(dz->vflag[IS_OCTVU_FUND]) {
		last_bandtop = dz->param[OCTVU_FUND] * ROOT_2;
		dz->itemcnt = 1;
		while(last_bandtop < dz->nyquist) {
			  last_bandtop *= 2;
			  dz->itemcnt++;
		}
	} else
		dz->itemcnt  = DEFAULT_OCTBANDS;

	if((dz->parray[OCTVU_ENERGY]  = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL
	|| (bandtop   				  = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL
	|| (dz->iparray[OCTVU_CHBTOP] = (int    *)malloc(dz->itemcnt * sizeof(int   )))==NULL
	|| (dz->iparray[OCTVU_CHBBOT] = (int    *)malloc(dz->itemcnt * sizeof(int   )))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for level for octave band arrays.\n");
		return(MEMORY_ERROR);
	}
	if(dz->vflag[IS_OCTVU_FUND]) {
		last_bandtop = dz->param[OCTVU_FUND] * ROOT_2;
		for(n = 0; n < dz->itemcnt;n++) {
			bandtop[n] = last_bandtop;
			last_bandtop *= 2.0;
		}
		n--;
		bandtop[n] = min(bandtop[n],dz->nyquist);
	} else {
		last_bandtop = dz->nyquist;
		for(n = dz->itemcnt-1;n >= 0 ; n--) {
			bandtop[n] = last_bandtop;
			last_bandtop /= 2.0;
		}
	}
	last_chan_bandtop = 0;
	dz->param[OCTVU_BBBTOP] = bandtop[0];
	last_chan_bandtop = 0;
	for(n = 0;n < dz->itemcnt;n++) {
		dz->iparray[OCTVU_CHBBOT][n]  = max(0,last_chan_bandtop - (2 * MARGIN_OF_ERROR));
		dz->iparray[OCTVU_CHBTOP][n]  = (int)((bandtop[n] + dz->halfchwidth)/dz->chwidth);/* TRUNCATE */
		dz->iparray[OCTVU_CHBTOP][n]++;
		dz->iparray[OCTVU_CHBTOP][n] *= 2; /* allow for amp AND frq */
		dz->iparray[OCTVU_CHBTOP][n]  = min(dz->wanted, dz->iparray[OCTVU_CHBTOP][n] + MARGIN_OF_ERROR);
		last_chan_bandtop = dz->iparray[OCTVU_CHBTOP][n];
	}
	bandmid = bandtop[0]/ROOT_2;
	fprintf(dz->fp,"\t\t\t\tBAND FREQUENCIES\n");
	for(n = 0;n < dz->itemcnt;n++) {
		fprintf(dz->fp,"%.2lf",bandmid);
		bandmid *= 2.0;
		if(n<dz->itemcnt-1)
			fprintf(dz->fp,"\t");
		else
			fprintf(dz->fp,"\n\n");
	}
	return(FINISHED);
}

/************************** SPECPEAK_PREPROCESS ******************************/

int specpeak_preprocess(dataptr dz)
{
	int exit_status;
	int k;
	dz->param[PEAK_FWINDOW] *= OCTAVES_PER_SEMITONE;
	if(dz->vflag[PEAK_HEAR] && (exit_status = create_hearing_response_adjustment_curve(dz))<0)
		return(exit_status);
	if((dz->iparam[PEAK_TGROUP] = round(dz->param[PEAK_TWINDOW]/dz->frametime))<=0)
		dz->iparam[PEAK_TGROUP] = 1;
	 if((exit_status = get_frq_bands(&(dz->iparam[PEAK_BANDCNT]),dz))<0)
	 	return(exit_status);
	for(k=0;k<dz->iparam[PEAK_BANDCNT];k++)
		dz->parray[PEAK_MAXI][k] = 0.0;
	if(exit_status<0)
		return(exit_status);
	dz->param[PEAK_ENDTIME] = 0.0;
	return(FINISHED);
}

/***************** CREATE_HEARING_RESPONSE_ADJUSTMENT_CURVE ********************/

int create_hearing_response_adjustment_curve(dataptr dz)
{   float *sf, *sp, *sa;
    dz->infile->specenvcnt = 17;
    if((dz->specenvfrq = (float *)malloc(dz->infile->specenvcnt * sizeof(float)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for spectral envelope frq array.\n");
		return(MEMORY_ERROR);
	}
    if((dz->specenvpch = (float *)malloc(dz->infile->specenvcnt * sizeof(float)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for spectral envelope pitch array.\n");
		return(MEMORY_ERROR);
	}
    if((dz->specenvamp = (float *)malloc(dz->infile->specenvcnt * sizeof(float)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for spectral envelope amplitude array.\n");
		return(MEMORY_ERROR);
	}
    sf = dz->specenvfrq;
    sp = dz->specenvpch;
    sa = dz->specenvamp;
    sf[0]  =    1.0f;   sa[0]  = 0.0f;       sp[0]  = 0.0f;       
    sf[1]  =   20.0f;   sa[1]  = 0.01f;      sp[1]  = (float)log(sf[1]);   
    sf[2]  =   50.0f;   sa[2]  = 0.141254f;  sp[2]  = (float)log(sf[2]);   
    sf[3]  =  100.0f;   sa[3]  = 0.446684f;  sp[3]  = (float)log(sf[3]);   
    sf[4]  =  200.0f;   sa[4]  = 1.0f;       sp[4]  = (float)log(sf[4]);   
    sf[5]  =  400.0f;   sa[5]  = 1.995262f;  sp[5]  = (float)log(sf[5]);   
    sf[6]  =  500.0f;   sa[6]  = 1.995262f;  sp[6]  = (float)log(sf[6]);   
    sf[7]  = 1000.0f;   sa[7]  = 0.794328f;  sp[7]  = (float)log(sf[7]);   
    sf[8]  = 1600.0f;   sa[8]  = 0.630957f;  sp[8]  = (float)log(sf[8]);   
    sf[9]  = 2000.0f;   sa[9]  = 1.258925f;  sp[9]  = (float)log(sf[9]);   
    sf[10] = 3500.0f;   sa[10] = 2.371374f;  sp[10] = (float)log(sf[10]);  
    sf[11] = 5000.0f;   sa[11] = 1.995262f;  sp[11] = (float)log(sf[11]);  
    sf[12] = 6000.0f;   sa[12] = 1.0f;       sp[12] = (float)log(sf[12]);  
    sf[13] = 8000.0f;   sa[13] = 0.501187f;  sp[13] = (float)log(sf[13]);  
    sf[14] = 10000.0f;  sa[14] = 0.031622f;  sp[14] = (float)log(sf[14]);  
    sf[15] = 14000.0f;  sa[15] = 0.0f;       sp[15] = (float)log(sf[15]);  
    sf[16] = 30000.0f;  sa[16] = 0.0f;       sp[16] = (float)log(sf[16]);  
	return(FINISHED);
}

/******************************* GET_FRQ_BANDS ***********************/

int get_frq_bands(int *cnt,dataptr dz)
{
	double bandratio = pow(2.0,dz->param[PEAK_FWINDOW]);
	double frqtop = dz->param[PEAK_CUTOFF] * bandratio; 
	int k = 0;
	while(frqtop < dz->nyquist) {
		k++;
		frqtop *= bandratio;
	}
	k++;
	if((dz->parray[PEAK_BAND] = (double *)malloc(k * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for peak bands array.\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[PEAK_MAXI] = (double *)malloc(k * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for peak maximi array.\n");
		return(MEMORY_ERROR);
	}
	frqtop = dz->param[PEAK_CUTOFF] * bandratio; 
	k = 0;
	while(frqtop < dz->nyquist) {
		dz->parray[PEAK_BAND][k++] = (float)frqtop;
		frqtop *= bandratio;
	}
	dz->parray[PEAK_BAND][k++] = (float)dz->nyquist;
	*cnt = k;
	return(FINISHED);
}

/************ SETUP_INTERNAL_PARAMS_AND_ARRAYS_FOR_SPECREPORT *************/

int	setup_internal_params_and_arrays_for_specreport(dataptr dz)
{
	int exit_status;
	dz->itemcnt = dz->iparam[REPORT_PKCNT];		/* specenv chans where peaks are */
	if((dz->peakno     = (int *)malloc((dz->itemcnt+1) * sizeof(int)))==NULL)	 {
		sprintf(errstr,"INSUFFICIENT MEMORY for peak reporting count array.\n");
		return(MEMORY_ERROR);
	}											/* specenv chans where peaks are */
	if((dz->lastpeakno = (int *)malloc((dz->itemcnt+1) * sizeof(int)))==NULL)	 {
		sprintf(errstr,"INSUFFICIENT MEMORY for peak reporting last peak array.\n");
		return(MEMORY_ERROR);
	}
	dz->peakno[dz->itemcnt] = 0;
	if((exit_status = setup_stability_arrays_and_constants(REPORT_STABL,REPORT_SL1,dz))<0)
		return(exit_status);
	dz->iparam[REPORT_LAST_PKCNT_HERE] = 0;
	return(FINISHED);
}

/************ SETUP_INTERNAL_PRINT_PARAMS *************/

int setup_internal_print_params(dataptr dz)
{
   	dz->iparam[PRNT_STARTW] = round(dz->param[PRNT_STIME] / dz->frametime);
   	dz->iparam[PRNT_ENDW]   = dz->iparam[PRNT_STARTW] + dz->iparam[PRNT_WCNT];
	if(dz->iparam[PRNT_ENDW] > dz->wlength)
		dz->iparam[PRNT_ENDW] = dz->wlength;
	return(FINISHED);
}


/********************************************************************************************/
/********************************** FORMERLY IN procspec.c **********************************/
/********************************************************************************************/

/**************************** SPEC_PROCESS_FILE ****************************/

int spec_process_file(dataptr dz)
{	
	dz->total_windows = 0;

	switch(dz->process) {
	case(WINDOWCNT):return specwcnt(dz);
	case(CHANNEL):	return specchan(dz);
	case(FREQUENCY):return specfrq(dz);
	case(LEVEL):	return speclevel(dz);							
	case(OCTVU):	return specoctvu(dz);
	case(PEAK):		return outer_textout_only_loop(dz);							
	case(REPORT): 	return outer_textout_only_loop(dz);
	case(PRINT):	return outer_textout_only_loop(dz);							
	default:
		sprintf(errstr,"Unknown process in procspec()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);	/* NOTREACHED */
}

/***************** SKIP_OR_SPECIAL_OPERATION_ON_WINDOW_ZERO ************/

int skip_or_special_operation_on_window_zero(dataptr dz)
{
	int exit_status = FINISHED;
	switch(dz->process) {
	case(REPORT):
		rectify_window(dz->flbufptr[0],dz);
		if((exit_status = extract_specenv(0,0,dz))<0)
			return(exit_status);
 /* put 1st specenv in end of stability store && copy 1st buf to end of bufstore */
		memmove((char *)dz->stable->spec[dz->iparam[REPORT_SL1]],
			(char *)dz->specenvamp,dz->infile->specenvcnt * sizeof(int));
		memmove((char *)dz->stable->sbuf[dz->iparam[REPORT_SL1]],
			(char *)dz->flbufptr[0],(size_t)dz->wanted * sizeof(float));		 
		dz->stable->total_pkcnt[dz->iparam[REPORT_SL1]] = 0; /* 1st window has 0 amp, hence 0 peaks */
		return(TRUE);
	}
	return(FALSE);
}

/***************************** INNER_LOOP (redundant) **************************/

int inner_loop
(int *peakscore,int *descnt,int *in_start_portion,int *least,int *pitchcnt,int windows_in_buf,dataptr dz)
{
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
	handle_pitch_zeros(dz);
	switch(dz->process) {
	case(OCTVU):	   return check_validity_of_octvu_params(dz);
	case(REPORT):	   return check_consistency_of_report_params(dz);
	case(LEVEL):		
		chans = dz->infile->channels;
		srate = dz->infile->origrate;
		stype = dz->infile->stype;
		dz->infile->channels = 1;
		dz->infile->srate    = dz->infile->origrate;
		dz->infile->stype    = SAMP_SHORT;
		if((exit_status = create_sized_outfile(dz->wordstor[0],dz))<0)
			return(exit_status);
		dz->infile->channels = chans;
		dz->infile->origrate = srate;
		dz->infile->stype	 = stype;
		break;
	}
	return(FINISHED);
}

/********************** CHECK_VALIDITY_OF_OCTVU_PARAMS **********************/

int check_validity_of_octvu_params(dataptr dz)
{
	if((dz->iparam[OCTVU_TBLOK] = round((dz->param[OCTVU_TSTEP] * MS_TO_SECS)
		/dz->frametime))>dz->wlength) {
		sprintf(errstr,"Time_step too long for input file.\n");
		return(DATA_ERROR);
	}
	if(dz->iparam[OCTVU_TBLOK]<=0) {
		sprintf(errstr,
			"Time_step too short for input file (are you using secs instead of mS?).\n");
		return(DATA_ERROR);
	}
	return(FINISHED);
}

/************ CHECK_CONSISTENCY_OF_REPORT_PARAMS *************/

int check_consistency_of_report_params(dataptr dz)
{
	if(dz->brksize[REPORT_LOFRQ]==0 && dz->brksize[REPORT_HIFRQ]==0) {
		if(flteq(dz->param[REPORT_LOFRQ],dz->param[REPORT_HIFRQ])) {
			sprintf(errstr,"Frequency limits define a zero-width frq band.\n");
			return(USER_ERROR);
		}
		if(dz->param[REPORT_LOFRQ] > dz->param[REPORT_HIFRQ])
			swap(&dz->param[REPORT_LOFRQ],&dz->param[REPORT_HIFRQ]);
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
	case(LEVEL): 	case(OCTVU):	case(PEAK):
	case(PRINT):	
		return allocate_single_buffer(dz);

	case(REPORT):	
		return allocate_double_buffer(dz);

	case(WINDOWCNT): case(CHANNEL):	 case(FREQUENCY):
		return(FINISHED);
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
	if     (!strcmp(prog_identifier_from_cmdline,"windowcnt")) 		dz->process = WINDOWCNT;
	else if(!strcmp(prog_identifier_from_cmdline,"channel"))   		dz->process = CHANNEL;
	else if(!strcmp(prog_identifier_from_cmdline,"frequency")) 		dz->process = FREQUENCY;
	else if(!strcmp(prog_identifier_from_cmdline,"level"))     		dz->process = LEVEL;
	else if(!strcmp(prog_identifier_from_cmdline,"octvu"))     		dz->process = OCTVU;
	else if(!strcmp(prog_identifier_from_cmdline,"peak"))  	   		dz->process = PEAK;
	else if(!strcmp(prog_identifier_from_cmdline,"report"))    		dz->process = REPORT;
	else if(!strcmp(prog_identifier_from_cmdline,"print"))     		dz->process = PRINT;
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
	"\nINFORMATION ON A SPECTRAL FILE\n\n"
	"USAGE: specinfo NAME (mode) infile outfile parameters: \n"
	"\n"
	"where NAME can be any one of\n"
	"\n"
	"windowcnt  channel  frequency  level   octvu   peak   report   print\n\n"
	"Type 'specinfo windowcnt' for more info on specinfo windowcnt..ETC.\n");
	return(USAGE_ONLY);
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
	if(!strcmp(str,"windowcnt")) {
		sprintf(errstr,
		"specinfo windowcnt infile\n\n"
		"RETURNS NUMBER OF ANALYSIS WINDOWS IN INFILE.\n");
	} else if(!strcmp(str,"channel")) {
		sprintf(errstr,
		"specinfo channel infile frq\n\n"
		"RETURNS PVOC CHANNEL NUMBER CORRESPONDING TO FREQUENCY GIVEN.\n");
	} else if(!strcmp(str,"frequency")) {
		sprintf(errstr,
		"specinfo frequency infile analysis_channel_number\n\n"
		"RETURNS CENTRE FRQ OF PVOC CHANNEL SPECIFIED.\n");
	} else if(!strcmp(str,"level")) {
		sprintf(errstr,
		"specinfo level infile outsndfile\n\n"
		"CONVERT (VARYING) LEVEL OF ANALFILE TO PSEUDO-SNDFILE (1 WINDOW -> 1 SAMPLE)\n\n"
		"View with a sndfile display program.\n\n"
		"DO NOT ATTEMPT TO PLAY FILE!!.\n");
	} else if(!strcmp(str,"octvu")) {
		sprintf(errstr,
		"specinfo octvu infile outtextfile time_step [-ffundamental]\n\n"
		"DISPLAY TIMEVARYING AMPLITUDE OF SPECTRUM, WITHIN OCTAVE BANDS.\n\n"
		"timestep is in MILLIsecs.\n"
		"Band-energy is totalled over each timestep duration.\n"
		"-f Octaves are centered on octave transpositions of fundamental.\n\n"
		"The reported values are RELATIVE levels only\n"
		"and the lowest band includes all energy down to '0Hz'.\n");
	} else if(!strcmp(str,"peak")) {
		sprintf(errstr,
		"specinfo peak infile outtextfile [-ccutoff_frq] [-ttimewindow] [-ffrqwindow] [-h]\n\n"
		"LOCATE TIME-VARYING ENERGY CENTRE OF SPECTRUM.\n\n"
		"CUTOFF_FRQ  above which spectral search begins\n"
		"TIMEWINDOW  for energy averaging: in SECONDS\n"
		"FRQWINDOW   for energy averaging: in SEMITONES\n"
		"-h          adjust result for sensitivity of ear.\n\n"
		"OUTFILE is a text file of lines of data in form...\n\n"
		"	WINDOW starttime endtime : PEB lofrq TO hifrq'\n\n"
		"(where 'PEB' means PEAK ENERGY BAND)\n");
	} else if(!strcmp(str,"report")) {
		fprintf(stdout,
		"specinfo report mode infile outfile -fN|-pN [-i] pks [-bbt] [-ttp] [-sval]\n\n"
		"REPORT ON LOCATION OF FREQUENCY PEAKS IN EVOLVING SPECTRUM,\n\n"
		"OUTFILE is a textfile\n\n"
		"MODES\n"
		"(1) Report on spectral peaks.\n"
		"(2) Report spectral peaks in loudness order.\n"
		"(3) Report spectral peaks as frq only (no time data).\n"
		"(4) Report spectral peaks in loudness order, as frq only (no time data).\n\n"
		"-f   extract formant envelope linear frqwise,\n"
		"     using 1 point for every N equally-spaced frequency-channels.\n"
		"-p   extract formant envelope linear pitchwise,\n"
		"     using N equally-spaced pitch-bands per octave.\n"
		"-i   quicksearch for formants (less accurate).\n"
		"PKS  (max) number of peaks to find : Range 1 - %d\n"
		"-b   BT is bottom frequency to start peak search.\n"
		"-t   TP is top frequency to stop peak search.\n"
		"-s   VAL is number of windows over which peaks averaged.\n"
		"     Range 2 - %d : default %d : WARNING: high values slow program!!\n\n"
		"bottom frequency & top frequency may vary over time.\n",MAXPKCNT,MAXSTABIL,DEFAULT_STABILITY);
	} else if(!strcmp(str,"print")) {
		sprintf(errstr,
		"specinfo print infile outtextfile time [-wwindowcnt]\n\n"
		"PRINT DATA IN ANALYSIS FILE AS TEXT TO FILE.\n\n"
		"TIME      in file at which printout begins.\n"
		"WINDOWCNT number of windows to print.\n"
		"OUTFILE   textfile to accept data.\n\n"
   		"WARNING:  using more than a few windows will generate a huge textfile.\n");
	} else
		sprintf(errstr,"Unknown option '%s'\n",str);
	return(USAGE_ONLY);
}

/******************************** USAGE3 ********************************/

int usage3(char *str1,char *str2)
{
	if(!strcmp(str1,"windowcnt"))	
		return(CONTINUE);
	else
		sprintf(errstr,"Insufficient parameters on command line.\n");
	return(USAGE_ONLY);
}


