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
#include <tkglobals.h>
#include <globcon.h>
#include <processno.h>
#include <modeno.h>
#include <arrays.h>
#include <filters.h>
#include <sfsys.h>
#include <osbind.h>
/*RWD*/
#include <math.h>
#include <string.h>


static int  force_start_and_end_val(int paramno,dataptr dz);
static void initialise_filter_table_read(int param,dataptr dz);
static int  setup_internal_eq_params(dataptr dz);
static int  setup_internal_fstatvar_params(dataptr dz);
static int  setup_internal_fltsweep_params(dataptr dz);
static int  setup_internal_allpass_params_and_delaybufs(dataptr dz);
static int  setup_internal_fltbankn_params(dataptr dz);
static void setup_internal_iterfilt_params(dataptr dz);
static int  allocate_tvarying_filter_arrays(int timeslot_cnt,int harmonics_cnt,dataptr dz);
static int  put_tvarying_filter_data_in_arrays(double *fbrk,dataptr dz);
static int  initialise_fltbankv_internal_params(dataptr dz);
static int  error_name(int paramno,char *paramname,dataptr dz);
static int  setup_stereo_variables(int a,int b,int c,int d,int chans,dataptr dz);
static int  setup_eqfilter_coeffs(dataptr dz);
static int  convert_delay_times_to_samples_and_get_maxdelay(dataptr dz);
static int  count_filters(dataptr dz);
static void convert_phase_from_input_range_to_actual_range(dataptr dz);
static void convert_sfrq(dataptr dz);
static void set_doflag(dataptr dz);
/* ANDY MOORER ROUTINES:  for 2nd-order presence and shelving filters */
/* rationalised slightly  */
#define PII 	  	(3.141592653589793238462643)
#define ROOT2OF2 	(0.7071067811965475244)		/* = sqrt(2)/2 */
#define SPN 1.65436e-24							/* Smallest possible number on DEC VAX-780: but it will work here */

static void   presence(double cf,double boost,double bw,double *a0,double *a1,double *a2,double *b1,double *b2);
static void   shelve(double cf,double boost,double *a0,double *a1,double *a2,double *b1,double *b2,int mode);
static double bw2angle(double a,double bw);
static void   normalise_b0_to_one(double b0,double *a0,double *a1,double *a2,double *b1,double *b2);
static void   bilinear_transform(double a,double t0,double t1,double t2,double asq,double *x0,double *x1,double *x2);
static void   set_the_tvars(double gamma,double *t0,double *t1,double *t2);
static void   establish_a_asq_A_F_F2_tmp(double cf,double boost,
					double *a,double *asq,double *A,double *F,double *f2,double *temp);
static int test_pitch_and_spectrum_varying_filterdata(double *fbrk,double *hbrk,dataptr dz);
static int allocate_tvarying2_filter_arrays(dataptr dz);

/************************************* FILTER_PCONSISTENCY *********************************/

int filter_pconsistency(dataptr dz)
{
	int exit_status;
	initrand48();
	switch(dz->process) {			/* preset internal counters, or defaulted variables */
	case(FLTBANKN):
	case(FSTATVAR):
	case(FLTBANKU):
	case(FLTBANKV):
	case(FLTBANKV2):
	case(FLTSWEEP):
		dz->iparam[FLT_SAMS]     = 0;
		dz->iparam[FLT_OVFLW]    = 0;
		dz->iparam[FLT_BLOKSIZE] = BSIZE;
		dz->iparam[FLT_BLOKCNT]  = 0;
		dz->param[FLT_INV_SR]    = 1.0/(double)dz->infile->srate;
		if(dz->brksize[FLT_Q]) {
			if((exit_status = force_start_and_end_val(FLT_Q,dz))<0)
				return(exit_status);
			initialise_filter_table_read(FLT_Q,dz);
		}
		break;
	case(FLTBANKC):
		dz->param[FLT_RANDFACT]	 = 0.0;
		dz->param[FLT_INV_SR]    = 1.0/(double)dz->infile->srate;
		break;
	case(EQ):
		dz->iparam[FLT_OVFLW]    = 0;
		dz->param[FLT_INV_SR]    = 1.0/(double)dz->infile->srate;
		break;
	case(LPHP):
		if(dz->mode==FLT_MIDI) {
			miditohz(dz->param[FLT_PASSFRQ]);
			miditohz(dz->param[FLT_STOPFRQ]);
		}
		dz->iparam[FLT_OVFLW]    = 0;
		break;
	case(FLTITER):
		dz->iparam[FLT_OVFLW]    = 0;
		dz->param[FLT_INV_SR]    = 1.0/(double)dz->infile->srate;
		dz->iparam[FLT_SAMS]     = 0;
		break;
	case(ALLPASS):
		dz->iparam[FLT_BLOKSIZE] = 1; /* delay (& strength) incremented on (stereo-pair)sample-by-sample basis */
		dz->iparam[FLT_OVFLW]    = 0;
		dz->iparam[FLT_SAMS]     = 0;
		break;
	}
	switch(dz->process) {
	case(EQ):  		return setup_internal_eq_params(dz);
	case(FSTATVAR):	return setup_internal_fstatvar_params(dz);
	case(FLTSWEEP):	return setup_internal_fltsweep_params(dz);
	case(ALLPASS):	return setup_internal_allpass_params_and_delaybufs(dz);
	case(FLTBANKN):
	case(FLTBANKC):	return setup_internal_fltbankn_params(dz);
	case(FLTITER):
		setup_internal_iterfilt_params(dz);
		break;
	case(FLTBANKV):
		dz->param[FLT_ROLLOFF] = dbtogain(dz->param[FLT_ROLLOFF]);
		if((exit_status = allocate_tvarying_filter_arrays(dz->iparam[FLT_TIMESLOTS],dz->iparam[FLT_HARMCNT],dz))<0)
			return(exit_status);
		if((exit_status = put_tvarying_filter_data_in_arrays(dz->parray[FLT_FBRK],dz))<0)
			return(exit_status);
		if((exit_status = initialise_fltbankv_internal_params(dz))<0)
			return(exit_status);
		break;
	case(FLTBANKV2):
		if((exit_status = allocate_tvarying2_filter_arrays(dz))<0)
			return(exit_status);
		if((exit_status = test_pitch_and_spectrum_varying_filterdata(dz->parray[FLT_FBRK],dz->parray[FLT_HBRK],dz))<0)
			return(exit_status);
		if((exit_status = allocate_filter_frq_amp_arrays(dz->iparam[FLT_CNT],dz))<0)
			return(exit_status);
		if((exit_status = newfval2(dz->parray[FLT_FBRK],dz->parray[FLT_HBRK],dz))<0)
			return(exit_status);
		break;
	}
	return(FINISHED);
}

/*************************************** FORCE_START_AND_END_VAL **************************************/

int force_start_and_end_val(int paramno,dataptr dz)
{
	int    exit_status;
 	double lasttime, filedur, firsttime, *p;
	int   k, n;
	char paramname[10];
	if((exit_status = error_name(paramno,paramname,dz))<0)
		return(exit_status);

	firsttime = *(dz->brk[paramno]);
	if(firsttime < 0.0) {
		sprintf(errstr,"First time in %s file is -ve: Can't proceed\n",paramname);
		return(DATA_ERROR);
	}
	if(flteq(firsttime,0.0))
		*(dz->brk[paramno]) = 0.0;
	else {							/* FORCE VALUE AT TIME 0 */
	 	dz->brksize[paramno]++;
		if((dz->brk[paramno] = (double *)realloc(dz->brk[paramno],dz->brksize[paramno] * 2 * sizeof(double)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to reallocate filter array.\n");
			return(MEMORY_ERROR);
		}
		k = dz->brksize[paramno] * 2;
		for(n=k-1;n>=2;n--)
			dz->brk[paramno][n] = dz->brk[paramno][n-2];
		dz->brk[paramno][0] = 0.0;
		dz->brk[paramno][1] = dz->brk[paramno][3];
	}

	lasttime = *(dz->brk[paramno] + ((dz->brksize[paramno]-1) * 2));
	filedur  = (double)(dz->insams[0]/dz->infile->channels)/(double)dz->infile->srate;
	if(lasttime >= filedur + FLT_TAIL)
		return(FINISHED);			/* FORCE Q VALUE AT (BEYOND) END OF FILE */
	dz->brksize[paramno]++;
	if((dz->brk[paramno] = (double *)realloc(dz->brk[paramno],dz->brksize[paramno] * 2 * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to reallocate filter array.\n");
		return(MEMORY_ERROR);
	}
	p = (dz->brk[paramno] + ((dz->brksize[paramno]-1) * 2));
	*p++ = filedur + FLT_TAIL + 1.0;
	*p = *(p-2);
	return(FINISHED);
}

/************************************* INITIALISE_FILTER_TABLE_READ *********************************/

void initialise_filter_table_read(int param,dataptr dz)
{
	dz->lastind[param] = (double)round((*dz->brk[param]) * dz->infile->srate);
	dz->lastval[param] = *(dz->brk[param]+1);
	dz->brkptr[param]  = dz->brk[param] + 2;
}

/**************************** SETUP_INTERNAL_EQ_PARAMS **********************/

int setup_internal_eq_params(dataptr dz)
{
	int exit_status;
	dz->param[FLT_ONEFRQ] *= dz->param[FLT_INV_SR];
	dz->param[FLT_BW]     *= dz->param[FLT_INV_SR];
	if((exit_status = setup_stereo_variables(FLT_XX1,FLT_XX2,FLT_YY1,FLT_YY2,dz->infile->channels,dz))<0)
		return(exit_status);
	return setup_eqfilter_coeffs(dz);
}

/************************************* SETUP_INTERNAL_FSTATVAR_PARAMS *********************************/

int setup_internal_fstatvar_params(dataptr dz)
{
	int exit_status;

	if(dz->brksize[FLT_Q]==0)
    	dz->param[FLT_QFAC] = 1.0/(1.0 + dz->param[FLT_Q]);

	if(dz->brksize[FLT_Q]) {
		if((exit_status = force_start_and_end_val(FLT_Q,dz))<0)
			return(exit_status);
		initialise_filter_table_read(FLT_Q,dz);
	}
	if(dz->brksize[FLT_ONEFRQ]) {
		if((exit_status = force_start_and_end_val(FLT_ONEFRQ,dz))<0)
			return(exit_status);
		initialise_filter_table_read(FLT_ONEFRQ,dz);
	}
	return setup_stereo_variables(FLT_DLS,FLT_DBS,FLT_DHS,FLT_DNS,dz->infile->channels,dz);
}

/************************************* SETUP_INTERNAL_FLTSWEEP_PARAMS *********************************/

int setup_internal_fltsweep_params(dataptr dz)
{
	int exit_status;
	convert_phase_from_input_range_to_actual_range(dz);
	dz->iparam[FLT_LOSAMS]   = 0;
	dz->iparam[FLT_HISAMS]   = 0;
	dz->iparam[FLT_SWSAMS]   = 0;
	dz->param[FLT_CYCPOS] 	 = dz->param[FLT_SWPPHASE];
	if(dz->brksize[FLT_Q]==0)
    	dz->param[FLT_QFAC] = 1.0/(1.0 + dz->param[FLT_Q]);

    convert_sfrq(dz);

	if(dz->brksize[FLT_LOFRQ]) {
		if((exit_status = force_start_and_end_val(FLT_LOFRQ,dz))<0)
			return(exit_status);
		initialise_filter_table_read(FLT_LOFRQ,dz);
	}
	if(dz->brksize[FLT_HIFRQ]) {
		if((exit_status = force_start_and_end_val(FLT_HIFRQ,dz))<0)
			return(exit_status);
		initialise_filter_table_read(FLT_HIFRQ,dz);
	}
	if(dz->brksize[FLT_SWPFRQ]) {
		if((exit_status = force_start_and_end_val(FLT_SWPFRQ,dz))<0)
			return(exit_status);
		initialise_filter_table_read(FLT_SWPFRQ,dz);
	}
	return setup_stereo_variables(FLT_DLS,FLT_DBS,FLT_DHS,FLT_DNS,dz->infile->channels,dz);
}

/******************** CONVERT_PHASE_FROM_INPUT_RANGE_TO_ACTUAL_RANGE *******************/

void convert_phase_from_input_range_to_actual_range(dataptr dz)
{
	dz->param[FLT_SWPPHASE] += .5;
	if(dz->param[FLT_SWPPHASE] >= 1.0)
		dz->param[FLT_SWPPHASE] -= 1.0;
	dz->param[FLT_SWPPHASE] *=  TWOPI;
}

/**************************** CONVERT_SFRQ ***************************
 *
 * (1)	frq is in cycles/sec.
 * (2)	Divide by SR gives what fraction of a cycle we get through in one sample.
 * (3)	Multiply by FLT_BLOKSIZE, gives what fraction of a cycle we get through in one sample-blok.
 * (4)  Multiply by TWOPI gives what part of a TWOPI-cycle do we get through in one sample-blok.
 */

void convert_sfrq(dataptr dz)
{
	double *p, *end;
	if(dz->brksize[FLT_SWPFRQ]) {
		p = dz->brk[FLT_SWPFRQ] + 1;
		end = dz->brk[FLT_SWPFRQ] + (dz->brksize[FLT_SWPFRQ]*2);
		while(p<end) {
			if(flteq(*p,0.0))
				*p = FLT_MINSWEEP;						/* prevent divides by zero */
			*p *= dz->param[FLT_INV_SR] * dz->iparam[FLT_BLOKSIZE] * TWOPI;	 /* 1,2,3,4 */
			p += 2;
		}
	} else {
		if(flteq(dz->param[FLT_SWPFRQ],0.0))
				dz->param[FLT_SWPFRQ] = FLT_MINSWEEP;	/* prevent divides by zero */
		dz->param[FLT_SWPFRQ] *= dz->param[FLT_INV_SR] * dz->iparam[FLT_BLOKSIZE] * TWOPI;
	}
}

/**************************** SETUP_INTERNAL_ALLPASS_PARAMS_AND_DELAYBUFS **********************/

int setup_internal_allpass_params_and_delaybufs(dataptr dz)
{

#define DELAY_SAFETY (64)

	int exit_status;
	int delbufsize;
	dz->iparam[FLT_MAXDELSAMPS]  = convert_delay_times_to_samples_and_get_maxdelay(dz);
	if(dz->brksize[FLT_DELAY])
		dz->iparam[FLT_MAXDELSAMPS] += DELAY_SAFETY;
	delbufsize = dz->iparam[FLT_MAXDELSAMPS] * dz->infile->channels * sizeof(double);
	/*RWD 9:2001 need calloc */
	if((dz->parray[FLT_DELBUF1] = (double *)calloc((size_t)delbufsize,sizeof(char)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for 1st delay buffer.\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[FLT_DELBUF2] = (double *)calloc((size_t)delbufsize,sizeof(char)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for 2nd delay buffer.\n");
		return(MEMORY_ERROR);
	}
	dz->iparam[FLT_DELBUFPOS] = 0;

	if(dz->brksize[FLT_DELAY]) {
		if((exit_status = force_start_and_end_val(FLT_DELAY,dz))<0)
			return(exit_status);
		initialise_filter_table_read(FLT_DELAY,dz);
	}
	return(FINISHED);
}

/************************************* SETUP_INTERNAL_FLTBANKN_PARAMS *********************************/

int setup_internal_fltbankn_params(dataptr dz)
{
	int exit_status;
	double  temp;
	switch(dz->mode) {
	case(FLT_EQUALINT):  dz->param[FLT_INTSIZE] = pow(SEMITONE_INTERVAL,dz->param[FLT_INTSIZE]); 	break;
	case(FLT_HARMONIC):	 dz->param[FLT_OFFSET]  = 0.0;  											break;
	}
	if(dz->param[FLT_LOFRQ] > dz->param[FLT_HIFRQ]) {	 /* correct inverted frq range */
		temp  				 = dz->param[FLT_LOFRQ];
		dz->param[FLT_LOFRQ] = dz->param[FLT_HIFRQ];
		dz->param[FLT_HIFRQ] = temp;
	}
	if((exit_status = count_filters(dz))<0)
		return(exit_status);
	return allocate_filter_frq_amp_arrays(dz->iparam[FLT_CNT],dz);
}	

/************************************* SETUP_INTERNAL_ITERFILT_PARAMS *********************************/

void setup_internal_iterfilt_params(dataptr dz)
{
	int srate = dz->infile->srate;
	int  chans = dz->infile->channels;
	dz->iparam[FLT_OUTDUR]  	= max(round(dz->param[FLT_OUTDUR] * (double)(srate * chans)),dz->insams[0]);
	dz->iparam[FLT_MSAMPDEL] 	= round(dz->param[FLT_DELAY] * (double)srate);
	dz->iparam[FLT_SMPDEL]   	= dz->iparam[FLT_MSAMPDEL] * chans;
	dz->iparam[FLT_INMSAMPSIZE] = dz->insams[0]/chans;
	if(flteq(dz->param[FLT_PRESCALE],0.0)) {								/* set default prescaling of input */
		dz->iparam[FLT_MAXOVERLAY] = round(((double)dz->iparam[FLT_INMSAMPSIZE]/(double)dz->iparam[FLT_MSAMPDEL])+1.0);
		if(dz->param[FLT_RANDDEL] > 0.0)
			dz->iparam[FLT_MAXOVERLAY]++;
		dz->param[FLT_PRESCALE] = 1.0/(double)dz->iparam[FLT_MAXOVERLAY];	
	}
	dz->param[FLT_PSHIFT] /= SEMITONES_PER_OCTAVE;							/* convert semitones to fractions of 8vas */
	set_doflag(dz);
}

/************************** SET_DOFLAG ********************************/

void set_doflag(dataptr dz)
{   
	if(dz->param[FLT_PSHIFT]>0.0) {
		if(dz->vflag[FLT_PINTERP_OFF]) {
			if(dz->infile->channels>1)
				dz->iparam[FLT_DOFLAG] = ST_SHIFT;
			else
				dz->iparam[FLT_DOFLAG] = MN_SHIFT;
		} else {
			if(dz->infile->channels>1)
				dz->iparam[FLT_DOFLAG] = ST_FLT_INTP_SHIFT;
			else
				dz->iparam[FLT_DOFLAG] = MN_FLT_INTP_SHIFT;
		}
	} else {
		if(dz->infile->channels>1)
			dz->iparam[FLT_DOFLAG] = ITER_STEREO;
		else
			dz->iparam[FLT_DOFLAG] = ITER_MONO;
	}
	if(flteq(dz->param[FLT_AMPSHIFT],0.0))
	dz->iparam[FLT_DOFLAG] += FIXED_AMP;
/* ??? */
	if(dz->iparam[FLT_MSAMPDEL] >= dz->iparam[FLT_INMSAMPSIZE])
		dz->vflag[FLT_EXPDEC] = 0;
/* ??? */
}

/**************************** ALLOCATE_TVARYING_FILTER_ARRAYS *******************************/

int allocate_tvarying_filter_arrays(int timeslot_cnt,int harmonics_cnt,dataptr dz)
{
	dz->iparam[FLT_CNT] *= harmonics_cnt;

	if((dz->lparray[FLT_SAMPTIME] = (int *)calloc(timeslot_cnt * sizeof(int),sizeof(char)))==NULL
	|| (dz->parray[FLT_INFRQ]     = (double *)calloc(dz->iparam[FLT_CNT] * timeslot_cnt * sizeof(double),sizeof(char)))==NULL
	|| (dz->parray[FLT_INAMP]     = (double *)calloc(dz->iparam[FLT_CNT] * timeslot_cnt * sizeof(double),sizeof(char)))==NULL
	|| (dz->parray[FLT_FINCR]     = (double *)calloc(dz->iparam[FLT_CNT] * sizeof(double),sizeof(char)))==NULL
	|| (dz->parray[FLT_AINCR]     = (double *)calloc(dz->iparam[FLT_CNT] * sizeof(double),sizeof(char)))==NULL
	|| (dz->parray[FLT_LASTFVAL]  = (double *)calloc(dz->iparam[FLT_CNT] * sizeof(double),sizeof(char)))==NULL
	|| (dz->parray[FLT_LASTAVAL]  = (double *)calloc(dz->iparam[FLT_CNT] * sizeof(double),sizeof(char)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for filter coefficients.\n");
		return(MEMORY_ERROR);
	}
	return(FINISHED);
}

/**************************** ALLOCATE_TVARYING2_FILTER_ARRAYS *******************************/

int allocate_tvarying2_filter_arrays(dataptr dz)
{
	if((dz->parray[FLT_INFRQ]     = (double *)malloc(dz->iparam[FLT_CNT] * sizeof(double)))==NULL
	|| (dz->parray[FLT_INAMP]     = (double *)malloc(dz->iparam[FLT_CNT] * sizeof(double)))==NULL
	|| (dz->parray[FLT_FINCR]     = (double *)malloc(dz->iparam[FLT_CNT] * sizeof(double)))==NULL
	|| (dz->parray[FLT_AINCR]     = (double *)malloc(dz->iparam[FLT_CNT] * sizeof(double)))==NULL
	|| (dz->parray[FLT_LASTFVAL]  = (double *)malloc(dz->iparam[FLT_CNT] * sizeof(double)))==NULL
	|| (dz->parray[FLT_LASTAVAL]  = (double *)malloc(dz->iparam[FLT_CNT] * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for filter coefficients.\n");
		return(MEMORY_ERROR);
	}
	return(FINISHED);
}

/**************************** PUT_TVARYING_FILTER_DATA_IN_ARRAYS *******************************/

int put_tvarying_filter_data_in_arrays(double *fbrk,dataptr dz)
{
	int timescnt = 0, freqcnt = 0, ampcnt = 0, n, m;
	double atten;
	int total_frq_cnt = dz->iparam[FLT_CNT] * dz->iparam[FLT_TIMESLOTS];
	int entrycnt = dz->iparam[FLT_ENTRYCNT];
	int wordcnt  = dz->iparam[FLT_WORDCNT];
	int j;
	int srate = dz->infile->srate;
	if(dz->parray[FLT_INFRQ]==NULL) {
		sprintf(errstr,"FLT_INFRQ array not established: put_tvarying_filter_data_in_arrays()\n");
		return(PROGRAM_ERROR);
	} 
	if(dz->parray[FLT_INAMP]==NULL) {
		sprintf(errstr,"FLT_INAMP array not established: put_tvarying_filter_data_in_arrays()\n");
		return(PROGRAM_ERROR);
	} 
	if(dz->lparray[FLT_SAMPTIME]==NULL) {
		sprintf(errstr,"FLT_SAMPTIME array not established: put_tvarying_filter_data_in_arrays()\n");
		return(PROGRAM_ERROR);
	} 
	for(n=0;n<wordcnt;n++) {
		m = n%entrycnt;
		if(m==0) {
			if(timescnt >= dz->iparam[FLT_TIMESLOTS]) {
				sprintf(errstr,"Error 0 in filter counting: put_tvarying_filter_data_in_arrays()\n");
				return(PROGRAM_ERROR);
			} 
			dz->lparray[FLT_SAMPTIME][timescnt++] = round(fbrk[n] * dz->infile->srate);

		} else if(ODD(m)) {
			for(j=1;j<=dz->iparam[FLT_HARMCNT];j++) {
				if(freqcnt >= total_frq_cnt) {
					sprintf(errstr,"Error 1 in filter counting: put_tvarying_filter_data_in_arrays()\n");
					return(PROGRAM_ERROR);
				} 
				if((dz->parray[FLT_INFRQ][freqcnt] = fbrk[n] * (double)j) > FLT_MAXFRQ) {
					sprintf(errstr,"Filter Harmonic %d of %.1lfHz = %.1lfHz beyond filter limit %.1lf.\n",
					j,fbrk[n],dz->parray[FLT_INFRQ][freqcnt],FLT_MAXFRQ);
					return(DATA_ERROR);
				}
				freqcnt++;
			}
		} else {
			atten = 1.0;
			for(j=1;j<=dz->iparam[FLT_HARMCNT];j++) {
				if(ampcnt >= total_frq_cnt) {
					sprintf(errstr,"Error 2 in filter counting: put_tvarying_filter_data_in_arrays()\n");
					return(PROGRAM_ERROR);
				} 
				dz->parray[FLT_INAMP][ampcnt] = fbrk[n] * atten;
				ampcnt++;
				atten *= dz->param[FLT_ROLLOFF];
			}
		}
	}
	if(freqcnt != total_frq_cnt || ampcnt != freqcnt || timescnt != dz->iparam[FLT_TIMESLOTS]) {
		sprintf(errstr,"Filter data accounting problem: read_time_varying_filter_data()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/**************************** INITALISE_FLTBANKV_INTERNAL_PARAMS **********************/

int initialise_fltbankv_internal_params(dataptr dz)
{
	int exit_status;
	int n;
	if((exit_status = allocate_filter_frq_amp_arrays(dz->iparam[FLT_CNT],dz))<0)
		return(exit_status);
	for(n = 0;n<dz->iparam[FLT_CNT];n++) {
		dz->parray[FLT_FRQ][n]      = dz->parray[FLT_INFRQ][n];
		dz->parray[FLT_AMP][n]      = dz->parray[FLT_INAMP][n];
		dz->parray[FLT_LASTFVAL][n] = dz->parray[FLT_FRQ][n];
		dz->parray[FLT_LASTAVAL][n] = dz->parray[FLT_AMP][n];
	}
	dz->iparam[FLT_FRQ_INDEX] = dz->iparam[FLT_CNT];
	dz->iparam[FLT_TIMES_CNT] = 1;
	return(FINISHED);
}

/*************************************** ERROR_NAME **************************************/

int error_name(int paramno,char *paramname,dataptr dz)
{	
	switch(dz->process) {
	case(FLTSWEEP):
		switch(paramno) {
		case(FLT_LOFRQ):	strcpy(paramname,"lofrq");		break;
		case(FLT_HIFRQ):	strcpy(paramname,"hifrq");		break;
		case(FLT_SWPFRQ):	strcpy(paramname,"sweepfrq");	break;
		case(FLT_Q):		strcpy(paramname,"Q");			break;
		default:
			sprintf(errstr,"Invalid case in error_name()\n");
			return(PROGRAM_ERROR);
		}
		break;
	case(FLTBANKN):
	case(FLTBANKU):
	case(FLTBANKV):
	case(FLTBANKV2):
		switch(paramno) {
		case(FLT_Q):		strcpy(paramname,"Q");			break;
		default:
			sprintf(errstr,"Invalid case in error_name()\n");
			return(PROGRAM_ERROR);
		}
		break;
	case(FLTITER):
	case(ALLPASS):
		switch(paramno) {
		case(FLT_DELAY):	strcpy(paramname,"delay");		break;
		default:
			sprintf(errstr,"Invalid case in error_name()\n");
			return(PROGRAM_ERROR);
		}
		break;
	case(FSTATVAR):
		switch(paramno) {
		case(FLT_Q):		strcpy(paramname,"Q");			break;
		case(FLT_ONEFRQ):	strcpy(paramname,"frq");		break;
		default:
			sprintf(errstr,"Invalid case in error_name()\n");
			return(PROGRAM_ERROR);
		}
		break;
	default:
		sprintf(errstr,"Invalid case in error_name()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/*************************** SETUP_STEREO_VARIABLES ************************/

int setup_stereo_variables(int a,int b,int c,int d,int chans,dataptr dz)
{
	int n;
	if((dz->parray[a] = (double *)calloc(chans * sizeof(double),sizeof(char)))==NULL
	|| (dz->parray[b] = (double *)calloc(chans * sizeof(double),sizeof(char)))==NULL
	|| (dz->parray[c] = (double *)calloc(chans * sizeof(double),sizeof(char)))==NULL
	|| (dz->parray[d] = (double *)calloc(chans * sizeof(double),sizeof(char)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for stereo coefficients.\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<chans;n++) {
		dz->parray[a][n] = 0.0;
		dz->parray[b][n] = 0.0;
		dz->parray[c][n] = 0.0;
		dz->parray[d][n] = 0.0;
	}
	return(FINISHED);
}

/******************************* SETUP_EQFILTER_COEFFS ******************************/

int setup_eqfilter_coeffs(dataptr dz)
{
	switch(dz->mode) {
	case(FLT_LOSHELF):
	case(FLT_HISHELF):
		shelve(dz->param[FLT_ONEFRQ],dz->param[FLT_BOOST],
		&(dz->param[FLT_A0]),&(dz->param[FLT_A1]),&(dz->param[FLT_A2]),
		&(dz->param[FLT_B1]),&(dz->param[FLT_B2]),dz->mode);
		break;
	case(FLT_PEAKING):
		presence(dz->param[FLT_ONEFRQ],dz->param[FLT_BOOST],dz->param[FLT_BW],
		&(dz->param[FLT_A0]),&(dz->param[FLT_A1]),&(dz->param[FLT_A2]),
		&(dz->param[FLT_B1]),&(dz->param[FLT_B2]));
		break;
	default:
		sprintf(errstr,"Unknown mode: setup_eqfilter_coeffs()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/* ANDY MOORER routines, rationalised slightly */

/* Design programs for 2nd-order presence and shelving filters */

#define PII 	  	(3.141592653589793238462643)
#define ROOT2OF2 	(0.7071067811965475244)		/* = sqrt(2)/2 */
#define SPN 1.65436e-24							/* Smallest possible number on DEC VAX-780: but it will work here */

/* ----------------------------------------------------------------------------
  PRESENCE - Design straightforward 2nd-order presence filter.
Must be given (normalised) centre frequency, bandwidth and boost/cut in dB.

Returns filter of form:

	         -1      -2
	A0 + A1 Z  + A2 Z					   
T(z) = ---------------------
	         -1      -2
	1 +  B1 Z  + B2 Z
--------------------------------------------------------------------------- */

void presence(double cf,double boost,double bw,double *a0,double *a1,double *a2,double *b1,double *b2)
{
	double a, A, F, xfmbw, C, tmp, alphan, alphad, b0, recipb0, asq, F2, a2plus1, ma2plus1;

 	establish_a_asq_A_F_F2_tmp(cf,boost,&a,&asq,&A,&F,&F2,&tmp);

	xfmbw = bw2angle(a, bw);
	C = 1.0 / tan(2 * PII * xfmbw);	/* Co-tangent of angle */

	if(fabs(tmp) <= SPN)
		alphad = C;
	else { 
		alphad = (C * C * (F2 - 1))/tmp;
		alphad = sqrt(alphad);
	
	}
	alphan = A * alphad;
	
	a2plus1  = 1 + asq;
	ma2plus1 = 1 - asq;
	*a0 = a2plus1 + alphan * ma2plus1;
	*a1 = 4 * a;
	*a2 = a2plus1 - alphan * ma2plus1;

	b0  = a2plus1 + alphad * ma2plus1;
	*b2 = a2plus1 - alphad * ma2plus1;

	recipb0 = 1.0/b0;
	*a0 *= recipb0;
	*a1 *= recipb0;
	*a2 *= recipb0;
	*b1 = *a1;		 /* is this correct ?? */
	*b2 *= recipb0;
}

/* -------------------------------------------------------------------------
SHELVE - Design straightforward 2nd-order shelving filter.
Must be given (normalised) centre frequency, bandwidth and boost/cut in dB.
This is a LOW shelving filter, in  that the response at z = -1 will be 1.0.

Returns filter of form:

	         -1      -2
	A0 + A1 Z  + A2 Z
T(z) = ---------------------			   ANDY MOORER
	         -1      -2
	1 +  B1 Z  + B2 Z
--------------------------------------------------------------------------- */

void shelve(double cf,double boost,double *a0,double *a1,double *a2,double *b1,double *b2,int mode)
{
	double a, A, F, tmp, b0, asq, F2, gamman, gammad, ta0, ta1, ta2, tb0, tb1, tb2;

 	establish_a_asq_A_F_F2_tmp(cf,boost,&a,&asq,&A,&F,&F2,&tmp);

	if(fabs(tmp) <= SPN) 
		gammad = 1.0;
/* NB: CHANGED FROM THE ORIGINAL *
	else
		gammad = pow((F2 - 1.0)/tmp,0.25);
AS THIS GAVE A -VE RESULT: SOME KIND OF AIRTHMETIC ERROR INSIDE pow */
	else {
		gammad = (F2 - 1.0)/tmp;
		gammad = pow(gammad,0.25);		   	/* Fourth root */
	}
	gamman = sqrt(A) * gammad;

   /* Once for the numerator */

	set_the_tvars(gamman,&ta0,&ta1,&ta2);

	if(mode==FLT_HISHELF)
		ta1 = -ta1;
	
   /* And again for the denominator */

	set_the_tvars(gammad,&tb0,&tb1,&tb2);

	if(mode==FLT_HISHELF)
		tb1 = -tb1;

   /* Now bilinear transform to proper centre frequency */

	bilinear_transform(a,ta0,ta1,ta2,asq,a0,a1,a2);
	bilinear_transform(a,tb0,tb1,tb2,asq,&b0,b1,b2);
	
   /* Normalise b0 to 1.0 for realisability */

	normalise_b0_to_one(b0,a0,a1,a2,b1,b2);
}

/* -------------------------------------------------------------------------
   BW2ANGLE - Given bilinear transform parameter and
desired bandwidth (as normalised frequency), computes bandedge, e, of filter
as if it were centred at the frequency .25 (or pi/2, or srate/4). The 
bandwidth would then 2*(.25-e). e is guaranteed to be between 0 and .25.

To state it differently, given a filter centered on .25 with low bandedge
e and high bandedge .5-e, the bilinear transform by a 
will produce a new filter with bandwidth (i.e., difference between the
high bandedge frequency and low bandedge frequency) of bw	 ANDY MOORER
------------------------------------------------------------------------- */

double bw2angle(double a,double bw)
{	
	double T, d, sn, cs, mag, delta, theta, tmp, a2, a4, asnd;

	T     = tan(2*PII*bw);
	a2    = a * a;
	a4    = a2 * a2;
	d     = 2 * a2 * T;
	sn    = (1 + a4) * T;
	cs    = (1 - a4);
	mag   = sqrt((sn * sn) + (cs * cs));
	d    /= mag;
	delta = atan2(sn, cs);
	asnd  = asin(d);
	theta = 0.5 * (PII - asnd - delta);	/* Bandedge for prototype */
	tmp   = 0.5 * (asnd - delta);		/* Take principal branch */
	if(tmp > 0 && tmp < theta) 
		theta = tmp;
	return(theta /(2 * PII));			/* Return normalised frequency */
}

/********************************* ESTABLISH_A_ASQ_A_F *********************************/

void establish_a_asq_A_F_F2_tmp(double cf,double boost,double *a,double *asq,double *A,double *F,double *F2,double *tmp)
{
	double sqrt2 = sqrt(2.0);
	double cf_less_quarter = cf - 0.25;
	double b_over20 = boost/20.0;
	*a   = tan(PII * cf_less_quarter);			/* Warp factor */ 
	*asq = (*a) * (*a);
	*A   = pow(10.0, b_over20);					/* Cvt dB to factor */
	if(boost < 6.0 && boost > -6.0)
		*F = sqrt(*A);
	else if(*A > 1.0) 
		*F = *A/sqrt2;
	else 
		*F = (*A) * sqrt2;
	  /* If |boost/cut| < 6dB, then doesn't make sense to use 3dB point.
	     Use of root makes bandedge at half the boost/cut amount 
	  */
	*F2 = (*F) * (*F);
	*tmp = ((*A) * (*A)) - (*F2);
}

/********************************* SET_THE_TVARS *********************************/

void set_the_tvars(double gamma,double *t0,double *t1,double *t2)
{
	double gamma2, gam2p1, siggam2;
	gamma2  = gamma * gamma;
	gam2p1  = 1.0 + gamma2;
	siggam2 = 2.0 * ROOT2OF2 * gamma;
	
	*t0 = gam2p1 + siggam2;
	*t1 = -2.0 * (1.0 - gamma2);
	*t2 = gam2p1 - siggam2;
}

/********************************* BILINEAR_TRANSFORM *********************************/

void bilinear_transform(double a,double t0,double t1,double t2,double asq,double *x0,double *x1,double *x2)
{
	double aa1;
	aa1 = a * t1;
	*x0  = t0 + aa1 + (asq * t2);
	*x1 = (2 * a * (t0 + t2)) + ((1 + asq) * t1);
	*x2 = (asq * t0) + aa1 + t2;
}

/********************************* NORMALISE_B0_TO_ONE *********************************/

void normalise_b0_to_one(double b0,double *a0,double *a1,double *a2,double *b1,double *b2)
{
	double recipb0 = 1.0/b0;
	*a0 *= recipb0;
	*a1 *= recipb0;
	*a2 *= recipb0;
	*b1 *= recipb0;
	*b2 *= recipb0;
}
							   
/******************* CONVERT_DELAY_TIMES_TO_SAMPLES_AND_GET_MAXDELAY *******************/

int convert_delay_times_to_samples_and_get_maxdelay(dataptr dz)
{
	double *p, *end;
	double maxdelay = 0.0;
	if(dz->brksize[FLT_DELAY]) {
		p = dz->brk[FLT_DELAY] + 1;
		end = dz->brk[FLT_DELAY] + (dz->brksize[FLT_DELAY] * 2);
		while(p < end) {
			if((*p = (double)round(*p * MS_TO_SECS * (double)dz->infile->srate))<1)
				*p = 1.0;
			maxdelay = max(*p,maxdelay);
			p+= 2;
		}
		return(round(maxdelay));
	}
	if((dz->param[FLT_DELAY] = (double)round(dz->param[FLT_DELAY] * MS_TO_SECS * (double)dz->infile->srate))<1)
		dz->param[FLT_DELAY] = 1.0;
	return(round(dz->param[FLT_DELAY]));
}

/************************** COUNT_FILTERS ****************************/

int count_filters(dataptr dz)
{   
	int f_cnt = 0;
	double thisfrq;
	switch(dz->mode) {
	case(FLT_EQUALSPAN): 
		f_cnt = dz->iparam[FLT_INCOUNT];
		break;
	case(FLT_EQUALINT):
		thisfrq = dz->param[FLT_LOFRQ]; 
		while(thisfrq < dz->param[FLT_HIFRQ]) {
			f_cnt++;
			thisfrq *= dz->param[FLT_INTSIZE];
		}
		break;
	case(FLT_HARMONIC):	/* offset = 0.0 */
	case(FLT_LINOFFSET):
		thisfrq = dz->param[FLT_LOFRQ];
		while((thisfrq+dz->param[FLT_OFFSET]) < dz->param[FLT_HIFRQ]) {
			f_cnt++;
			thisfrq = dz->param[FLT_LOFRQ] * (double)(f_cnt+1);
		}
		break;
	case(FLT_ALTERNATE):
		thisfrq = dz->param[FLT_LOFRQ];
		while(thisfrq < dz->param[FLT_HIFRQ]) {
			f_cnt++;
			thisfrq = dz->param[FLT_LOFRQ] * (double)((f_cnt*2)+1);
		}
		break;
	case(FLT_SUBHARM):
		thisfrq = dz->param[FLT_HIFRQ];
		while(thisfrq > dz->param[FLT_LOFRQ]) {
			f_cnt++;	
			thisfrq = dz->param[FLT_HIFRQ]/(double)(f_cnt+1);
		}
		break;
	}
	if(f_cnt > FLT_MAX_FILTERS || f_cnt < 0) {
		sprintf(errstr,"Too many filters [%d]: max %d\n",f_cnt,FLT_MAX_FILTERS);
		return(DATA_ERROR);
	}
	if(f_cnt < 1) {
		sprintf(errstr,"Too few filters in range specified.\n");
		return(DATA_ERROR);
	}
	dz->iparam[FLT_CNT] = (int)f_cnt;
	return(f_cnt);
}

/**************************** TEST_PITCH_AND_SPECTRUM_VARYING_FILTERDATA *******************************/

int test_pitch_and_spectrum_varying_filterdata(double *fbrk,double *hbrk,dataptr dz)
{
	int n, m, k, timepoint, htimepoint, nextrow;
	int entrycnt = dz->iparam[FLT_ENTRYCNT];
	int wordcnt  = dz->iparam[FLT_WORDCNT];
	int hentrycnt = dz->iparam[HRM_ENTRYCNT];
	int hwordcnt  = dz->iparam[HRM_WORDCNT];
	int srate = dz->infile->srate;
	double thistime, lotime, hitime, timefrac, valdiff;
	double timestep = (double)dz->iparam[FLT_BLOKSIZE]/(double)dz->infile->srate;
	double lasttime = fbrk[wordcnt - entrycnt];
	lasttime = min(lasttime,hbrk[hwordcnt - hentrycnt]);

	dz->iparam[FLT_BLOKCNT] = dz->iparam[FLT_BLOKSIZE] * dz->infile->channels;

	timepoint = 0;
	htimepoint = 0;
	fprintf(stdout,"INFO: TESTING PITCH AND HARMONICS DATA.\n");
	fflush(stdout);
	if((dz->parray[HARM_FRQ_CALC] = (double *)malloc(dz->iparam[FLT_HARMCNT] * sizeof(double)))==NULL) {
		sprintf(errstr,"OUT OF MEMORY TO STORE HARMONIC SHIFT DATA\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[HARM_AMP_CALC] = (double *)malloc(dz->iparam[FLT_HARMCNT] * sizeof(double)))==NULL) {
		sprintf(errstr,"OUT OF MEMORY TO STORE HARMONICS AMPLITUDE ADJUST DATA\n");
		return(MEMORY_ERROR);
	}
	memset((char *)dz->parray[FLT_INFRQ],0,dz->iparam[FLT_CNT] * sizeof(double));

	
	for(thistime = 0.0; thistime < lasttime;thistime += timestep) {
		timepoint = 0;
		while(thistime >= fbrk[timepoint]) {	/* search times in frq array */
			if((timepoint += entrycnt)  >= wordcnt)
				break;
		}
		timepoint -= entrycnt;
		lotime = fbrk[timepoint];
		nextrow = timepoint + entrycnt;
		for(n = timepoint+1,k = 0; n < nextrow;n+=2,k += dz->iparam[FLT_HARMCNT])
			dz->parray[FLT_INFRQ][k] = fbrk[n];		/* Get frqs of fundamentals into array, leaving space for harmonics */
		if(nextrow != wordcnt) {					/* if not at end of table, do interpolation */
			nextrow += entrycnt;
			timepoint += entrycnt;
			hitime = fbrk[timepoint];
			timefrac = (thistime - lotime)/(hitime - lotime);
			k = 0;
			for(n = timepoint+1,k=0; n < nextrow;n+=2,k += dz->iparam[FLT_HARMCNT]) {
				valdiff = fbrk[n] - dz->parray[FLT_INFRQ][k];
				valdiff *= timefrac;
				dz->parray[FLT_INFRQ][k] = dz->parray[FLT_INFRQ][k] + valdiff;
			}
		}
		htimepoint = 0;
		while(thistime >= hbrk[htimepoint]) {	/* search times in frq array */
			if((htimepoint += hentrycnt) >= hwordcnt)
				break;
		}
		htimepoint -= hentrycnt;
		lotime = hbrk[htimepoint];
		nextrow = htimepoint + hentrycnt;
		k = 0;
		for(n = htimepoint+1,k=0; n < nextrow;n+=2,k++)
			dz->parray[HARM_FRQ_CALC][k] = hbrk[n];
		if(nextrow != hwordcnt) {					/* if not at end of table, do interpolation */
			nextrow += hentrycnt;
			htimepoint += hentrycnt;
			hitime = hbrk[htimepoint];
			timefrac = (thistime - lotime)/(hitime - lotime);
			k = 0;
			for(n = htimepoint+1,k=0; n < nextrow;n+=2,k++) {
				/* PARTIAL MULTIPLIER */
				valdiff = hbrk[n] - dz->parray[HARM_FRQ_CALC][k];
				valdiff *= timefrac;
				dz->parray[HARM_FRQ_CALC][k] = dz->parray[HARM_FRQ_CALC][k] + valdiff;
				/* PARTIAL AMP */
			}
		}
		for(k=0;k<dz->iparam[FLT_CNT];k+=dz->iparam[FLT_HARMCNT]) {
			for(m=0; m < dz->iparam[FLT_HARMCNT];m++) {	/* calc vals for partials from basefrq vals */
				if((dz->parray[FLT_INFRQ][k+m] = dz->parray[FLT_INFRQ][k] * dz->parray[HARM_FRQ_CALC][m]) > FLT_MAXFRQ) {
					sprintf(errstr,"PARTIAL NO %lf TOO HIGH (Frq %lf Root %lf) AT TIME %lf\n",
					dz->parray[HARM_FRQ_CALC][m],dz->parray[FLT_INFRQ][k+m],dz->parray[FLT_INFRQ][k]/dz->parray[HARM_FRQ_CALC][0],thistime);
					return(DATA_ERROR);
				}
			}
		}
	}
	fprintf(stdout,"INFO: FINISHED TESTING PITCH AND HARMONICS DATA.\n");
	fflush(stdout);
	return(FINISHED);
}

