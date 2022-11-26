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
#include <memory.h>
#include <structures.h>
#include <tkglobals.h>
#include <processno.h>
#include <modeno.h>
#include <arrays.h>
#include <globcon.h>
#include <filters.h>
#include <math.h>
#include <osbind.h>

static int  setup_lphp_filter(dataptr dz);
static int  calc_frqs(dataptr dz);
static void randomise_frqs(dataptr dz);
static int  allocate_internal_params_lphp(int chans,dataptr dz);
static void calculate_filter_poles_lphp(double signd,int filter_order,dataptr dz);
static void initialise_filter_coeffs_lphp(int chans,dataptr dz);
static int  allocate_filter_internalparam_arrays(int fltcnt,dataptr dz);
static int  initialise_filter_params(dataptr dz);
static int  establish_order_of_filter(dataptr dz);
static int  initialise_fltbankv2(dataptr dz);

/************************** FILTER_PREPROCESS **************************/

int filter_preprocess(dataptr dz)
{
	int exit_status;
	int n;
	if(dz->process == FLTBANKV2) {
		if((exit_status = initialise_fltbankv2(dz))<0)
			return exit_status;
	}
	switch(dz->process) {
	case(EQ):
	case(FSTATVAR):
	case(FLTSWEEP):
	case(ALLPASS):
		break;
	case(LPHP):
		if((exit_status = setup_lphp_filter(dz))<0)
			return(exit_status);
		break;
	case(FLTBANKC):			
		return calc_frqs(dz);
	case(FLTBANKN):
		if((exit_status = calc_frqs(dz))<0)
			return(exit_status);
		for(n=0;n<dz->iparam[FLT_CNT];n++)
			dz->parray[FLT_AMP][n] = 1.0;
		/* fall thro */
	case(FLTBANKU):
	case(FLTBANKV2):
	case(FLTBANKV):
	case(FLTITER):
		if((exit_status = allocate_filter_internalparam_arrays(dz->iparam[FLT_CNT],dz))<0)
			return(exit_status);
		if((exit_status = initialise_filter_params(dz))<0)
			return(exit_status);
		break;
	default:
		sprintf(errstr,"Unknown case in filter_preprocess()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/********************************* SETUP_LPHP_FILTER *****************************/

int setup_lphp_filter(dataptr dz)
{
	int exit_status;
	int filter_order;
	double signd = 1.0;
	if (dz->param[FLT_PASSFRQ] < dz->param[FLT_STOPFRQ])	/* low pass */
		signd = -1.0;
	filter_order = establish_order_of_filter(dz);

	if((exit_status = allocate_internal_params_lphp(dz->infile->channels,dz))<0)
		return(exit_status);
	calculate_filter_poles_lphp(signd,filter_order,dz);
	initialise_filter_coeffs_lphp(dz->infile->channels,dz);
	return(FINISHED);
}

/***************************** CALC_FRQS **********************************/

int calc_frqs(dataptr dz)
{
	double interval, one_over_total_steps;
	int n;
	switch(dz->mode) {
	case(FLT_EQUALSPAN):
		one_over_total_steps = 1.0/(double)dz->iparam[FLT_CNT];
		for(n=0;n<dz->iparam[FLT_CNT];n++) {
			interval = dz->param[FLT_HIFRQ]/dz->param[FLT_LOFRQ];
			dz->parray[FLT_FRQ][n] = dz->param[FLT_LOFRQ] * pow(interval,(double)n * one_over_total_steps);
		}
		break;
    case(FLT_EQUALINT):
		dz->parray[FLT_FRQ][0] = dz->param[FLT_LOFRQ];
		for(n=1;n<dz->iparam[FLT_CNT];n++)   
			dz->parray[FLT_FRQ][n] =  dz->parray[FLT_FRQ][n-1] * dz->param[FLT_INTSIZE];
		break;
    case(FLT_HARMONIC):	/* offset = 0.0 */
    case(FLT_LINOFFSET):
		dz->parray[FLT_FRQ][0] = dz->param[FLT_LOFRQ];
		for(n=1;n<dz->iparam[FLT_CNT];n++)   
			dz->parray[FLT_FRQ][n] = (dz->param[FLT_LOFRQ] * (double)(n+1))+dz->param[FLT_OFFSET];
		break;
    case(FLT_ALTERNATE):
		dz->parray[FLT_FRQ][0] = dz->param[FLT_LOFRQ];
		for(n=1;n<dz->iparam[FLT_CNT];n++)   
			dz->parray[FLT_FRQ][n] =  dz->param[FLT_LOFRQ] * (double)((n*2)+1);
		break;
    case(FLT_SUBHARM):	/* dz->param[FLT_LOFRQ] already reset at top */
		dz->parray[FLT_FRQ][0] = dz->param[FLT_HIFRQ];
		for(n=1;n<dz->iparam[FLT_CNT];n++)   
			dz->parray[FLT_FRQ][n] =  dz->param[FLT_HIFRQ]/(double)(n+1);
		break;
	default:
		sprintf(errstr,"Unknown mode in	calc_frqs()\n");
		return(PROGRAM_ERROR);
    }
	if(!flteq(dz->param[FLT_RANDFACT],0.0))
		randomise_frqs(dz);
	return(FINISHED);
}

/************************ RANDOMISE_FRQS *******************************/

void randomise_frqs(dataptr dz)
{
	int n;
	double thisrand, frqratio, interval;
	for(n=1;n<dz->iparam[FLT_CNT]-1;n++) {
		thisrand = drand48();						/*  RANGE 0 - 1      */
		if(thisrand >= 0.5) {						/* IF IN TOP 1/2     */
			thisrand -= 0.5;						/* REDUCE BY 1/2     */
			thisrand *= dz->param[FLT_RANDFACT];	/* SCALE BY randfact */
			frqratio = dz->parray[FLT_FRQ][n+1]/dz->parray[FLT_FRQ][n];
			interval = log(frqratio) * thisrand;
			frqratio = exp(interval);
			dz->parray[FLT_FRQ][n] *= frqratio;
						/* SCATTER FRQ IN 1/2 INTERVAL ABOVE CURRENT VAL */
		} else {
			thisrand *= dz->param[FLT_RANDFACT];	/* SCALE BY randfact */
			frqratio = dz->parray[FLT_FRQ][n]/dz->parray[FLT_FRQ][n-1];
			interval = log(frqratio) * (1.0 - thisrand);
			frqratio = exp(interval);
			dz->parray[FLT_FRQ][n] = dz->parray[FLT_FRQ][n-1] * frqratio;
						/* SCATTER FRQ IN 1/2 INTERVAL BELOW CURRENT VAL */
		}
	}
}

/********************************* ALLOCATE_INTERNAL_PARAMS_LPHP *****************************/

//int allocate_internal_params_lphp(int chans,dataptr dz)
//{
//	if((dz->parray[FLT_DEN1] = (double *)malloc(dz->iparam[FLT_CNT] * sizeof(double)))==NULL
//	|| (dz->parray[FLT_DEN2] = (double *)malloc(dz->iparam[FLT_CNT] * sizeof(double)))==NULL
//	|| (dz->parray[FLT_CN]   = (double *)malloc(dz->iparam[FLT_CNT] * sizeof(double)))==NULL
//	|| (dz->parray[FLT_S1]   = (double *)malloc(dz->iparam[FLT_CNT] * sizeof(double)))==NULL
//	|| (dz->parray[FLT_E1]   = (double *)malloc(dz->iparam[FLT_CNT] * sizeof(double)))==NULL
//	|| (dz->parray[FLT_S2]   = (double *)malloc(dz->iparam[FLT_CNT] * sizeof(double)))==NULL
//	|| (dz->parray[FLT_E2]   = (double *)malloc(dz->iparam[FLT_CNT] * sizeof(double)))==NULL) {
//		sprintf(errstr,"INSUFFICIENT MEMORY for arrays of filter parameters.\n");
//		return(MEMORY_ERROR);
//	}
//	if(chans==2) {
//		if((dz->parray[FLT_S1S]   = (double *)malloc(dz->iparam[FLT_CNT] * sizeof(double)))==NULL
//		|| (dz->parray[FLT_E1S]   = (double *)malloc(dz->iparam[FLT_CNT] * sizeof(double)))==NULL
//		|| (dz->parray[FLT_S2S]   = (double *)malloc(dz->iparam[FLT_CNT] * sizeof(double)))==NULL
//		|| (dz->parray[FLT_E2S]   = (double *)malloc(dz->iparam[FLT_CNT] * sizeof(double)))==NULL) {
//			sprintf(errstr,"INSUFFICIENT MEMORY for arrays of filter stereo parameters.\n");
//			return(MEMORY_ERROR);
//		}
//	}
//	return(FINISHED);
//}
//

// TW MULTICHAN 2010
int allocate_internal_params_lphp(int chans,dataptr dz)
{
	int i;
	if((dz->parray[FLT_DEN1] = (double *)calloc(dz->iparam[FLT_CNT] * sizeof(double),sizeof(char)))==NULL
	|| (dz->parray[FLT_DEN2] = (double *)calloc(dz->iparam[FLT_CNT] * sizeof(double),sizeof(char)))==NULL
	|| (dz->parray[FLT_CN]   = (double *)calloc(dz->iparam[FLT_CNT] * sizeof(double),sizeof(char)))==NULL
	|| (dz->parray[FLT_S1_BASE]   = (double *)calloc(dz->iparam[FLT_CNT] * sizeof(double),sizeof(char)))==NULL
	|| (dz->parray[FLT_E1_BASE]   = (double *)calloc(dz->iparam[FLT_CNT] * sizeof(double),sizeof(char)))==NULL
	|| (dz->parray[FLT_S2_BASE]   = (double *)calloc(dz->iparam[FLT_CNT] * sizeof(double),sizeof(char)))==NULL
	|| (dz->parray[FLT_E2_BASE]   = (double *)calloc(dz->iparam[FLT_CNT] * sizeof(double),sizeof(char)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for arrays of filter parameters.\n");
		return(MEMORY_ERROR);
	}
	for(i = 1; i < chans;i++) {
		int index = i*FLT_LPHP_ARRAYS_PER_FILTER;
		if((dz->parray[FLT_S1_BASE + index]   = (double *)calloc(dz->iparam[FLT_CNT] * sizeof(double),sizeof(char)))==NULL
		|| (dz->parray[FLT_E1_BASE + index]   = (double *)calloc(dz->iparam[FLT_CNT] * sizeof(double),sizeof(char)))==NULL
		|| (dz->parray[FLT_S2_BASE + index]   = (double *)calloc(dz->iparam[FLT_CNT] * sizeof(double),sizeof(char)))==NULL
		|| (dz->parray[FLT_E2_BASE + index]   = (double *)calloc(dz->iparam[FLT_CNT] * sizeof(double),sizeof(char)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for arrays of %d-channel filter parameters.\n",chans);
			return(MEMORY_ERROR);
		}
	}
	return(FINISHED);
}

/********************************* CALCULATE_FILTER_POLES_LPHP *****************************/

void calculate_filter_poles_lphp(double  signd,int filter_order,dataptr dz)
{
	double ss, xx, aa, tppwr, x1, x2, cc;
	double pii = 4.0 * atan(1.0);
	double tp = tan(dz->param[FLT_PASSFRQ]);
	int    k;
	ss = pii / (double)(2 * filter_order);
	for (k = 0; k < dz->iparam[FLT_CNT]; k++ ) {
//TW : RWD CORRECTION TALLIES WITH MY UPDATES
		xx = (double) ((2.0 * (k+1)) - 1.0);
		aa = -sin(xx * ss);
		tppwr = pow(tp,2.0);
		cc = 1.0 - (2.0 * aa * tp) + tppwr;
		x1 = 2.0 * (tppwr - 1.0)/cc ;
		x2 = (1.0 + (2.0 * aa * tp) + tppwr)/cc ;
		dz->parray[FLT_DEN1][k] = signd * x1;
		dz->parray[FLT_DEN2][k] = -x2 ;
		dz->parray[FLT_CN][k]   = pow(tp,2.0)/cc ;
	}
}

/********************************* INITIALISE_FILTER_COEFFS_LPHP *****************************/

//void initialise_filter_coeffs_lphp(int chans,dataptr dz)
//{
//	int k;
//	for (k = 0 ; k < dz->iparam[FLT_CNT]; k++) {
//		dz->parray[FLT_S1][k] = 0.0;
//		dz->parray[FLT_S2][k] = 0.0;
//		dz->parray[FLT_E1][k] = 0.0;
//		dz->parray[FLT_E2][k] = 0.0;
//	}
//	if(chans==STEREO)	{
//		for (k = 0 ; k < dz->iparam[FLT_CNT]; k++) {
//			dz->parray[FLT_S1S][k] = 0.0;
//			dz->parray[FLT_S2S][k] = 0.0;
//			dz->parray[FLT_E1S][k] = 0.0;
//			dz->parray[FLT_E2S][k] = 0.0;
//		}
//	}
//}
//

// TW MULTICHAN 2010
void initialise_filter_coeffs_lphp(int chans,dataptr dz)
{
	int k;
	int i,index;
	for (k = 0 ; k < dz->iparam[FLT_CNT]; k++) {
		dz->parray[FLT_S1_BASE][k] = 0.0;
		dz->parray[FLT_S2_BASE][k] = 0.0;
		dz->parray[FLT_E1_BASE][k] = 0.0;
		dz->parray[FLT_E2_BASE][k] = 0.0;
	}
	for(i=1;i < chans; i++){
		index = i *	FLT_LPHP_ARRAYS_PER_FILTER;
		for (k = 0 ; k < dz->iparam[FLT_CNT]; k++) {
			dz->parray[FLT_S1_BASE + i][k] = 0.0;
			dz->parray[FLT_S2_BASE + i][k] = 0.0;
			dz->parray[FLT_E1_BASE + i][k] = 0.0;
			dz->parray[FLT_E2_BASE + i][k] = 0.0;
		}
	}
}

/**************************** ALLOCATE_FILTER_INTERNALPARAM_ARRAYS *******************************/

int allocate_filter_internalparam_arrays(int fltcnt,dataptr dz)
{
	int chans = dz->infile->channels;
	if((dz->parray[FLT_AMPL] = (double *)calloc(fltcnt * sizeof(double),sizeof(char)))==NULL
	|| (dz->parray[FLT_A]    = (double *)calloc(fltcnt * sizeof(double),sizeof(char)))==NULL
	|| (dz->parray[FLT_B]    = (double *)calloc(fltcnt * sizeof(double),sizeof(char)))==NULL
	|| (dz->parray[FLT_WW]   = (double *)calloc(fltcnt * sizeof(double),sizeof(char)))==NULL
	|| (dz->parray[FLT_COSW] = (double *)calloc(fltcnt * sizeof(double),sizeof(char)))==NULL
	|| (dz->parray[FLT_SINW] = (double *)calloc(fltcnt * sizeof(double),sizeof(char)))==NULL
	|| (dz->parray[FLT_Y]    = (double *)calloc(fltcnt * chans * sizeof(double),sizeof(char)))==NULL
	|| (dz->parray[FLT_Z]    = (double *)calloc(fltcnt * chans * sizeof(double),sizeof(char)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for arrays of filter parameters.\n");
		return(MEMORY_ERROR);
	}
    if(dz->vflag[FLT_DBLFILT]) {
		if((dz->parray[FLT_D]    = (double *)calloc(fltcnt * chans * sizeof(double),sizeof(char)))==NULL
		|| (dz->parray[FLT_E]    = (double *)calloc(fltcnt * chans * sizeof(double),sizeof(char)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for double filtering parameters.\n");
			return(MEMORY_ERROR);
		}
	}
	return(FINISHED);
}

/************************ INITIALISE_FILTER_PARAMS ***************************/

int initialise_filter_params(dataptr dz)
{
	int n, chno, k;
	int chans = dz->infile->channels;
	for(n=0;n<dz->iparam[FLT_CNT];n++) {
		get_coeffs1(n,dz);
		get_coeffs2(n,dz);
		for(chno=0;chno<chans;chno++) {
			k = (n*chans)+chno;
			if(dz->vflag[FLT_DBLFILT]) {
				dz->parray[FLT_D][k] = 0.0;
				dz->parray[FLT_E][k] = 0.0;
			}
			dz->parray[FLT_Y][k]  = 0.0;
			dz->parray[FLT_Z][k]  = 0.0;
		}	
	}
	return(FINISHED);
}

/********************************* ESTABLISH_ORDER_OF_FILTER *****************************/

int establish_order_of_filter(dataptr dz)
{
	int filter_order;
	double tc, tp, tt, pii, xx, yy;
	double sr = (double)dz->infile->srate;
	if (dz->param[FLT_PASSFRQ] < dz->param[FLT_STOPFRQ])		/* low pass */
		dz->param[FLT_MUL] = 2.0;
	else {
		dz->param[FLT_MUL] = -2.0;
		dz->param[FLT_PASSFRQ] = dz->nyquist - dz->param[FLT_PASSFRQ];
		dz->param[FLT_STOPFRQ] = dz->nyquist - dz->param[FLT_STOPFRQ];
	}
	pii = 4.0 * atan(1.0);
	dz->param[FLT_PASSFRQ] = pii * dz->param[FLT_PASSFRQ]/sr;
	tp = tan(dz->param[FLT_PASSFRQ]);
	dz->param[FLT_STOPFRQ] = pii * dz->param[FLT_STOPFRQ]/sr;
	tc = tan(dz->param[FLT_STOPFRQ]);
	tt = tc / tp ;
	tt = (tt * tt);
	dz->param[FLT_GAIN] = fabs(dz->param[FLT_GAIN]);
	dz->param[FLT_GAIN] = dz->param[FLT_GAIN] * log(10.0)/10.0 ;
	dz->param[FLT_GAIN] = exp(dz->param[FLT_GAIN]) - 1.0 ;
	xx = log(dz->param[FLT_GAIN])/log(tt) ;
	yy = floor(xx);
	if ((xx - yy) == 0.0 )
		yy = yy - 1.0 ;
	filter_order = ((int)yy) + 1;
	if (filter_order <= 1) 
		filter_order = 2;
	dz->iparam[FLT_CNT] = filter_order/2 ;
	filter_order = 2 * dz->iparam[FLT_CNT] ;
	fprintf(stdout,"INFO: Order of filter is %d\n", filter_order);
	fflush(stdout);
	dz->iparam[FLT_CNT] = min(dz->iparam[FLT_CNT],FLT_LBF);
	filter_order = 2 * dz->iparam[FLT_CNT];
	return(filter_order);
}

/**************************** INITIALISE_FLTBANKV2 *******************************/

int initialise_fltbankv2(dataptr dz)
{
	int n, k, z;

	memset((char *)dz->parray[FLT_INFRQ],0,dz->iparam[FLT_CNT] * sizeof(double));
	memset((char *)dz->parray[FLT_INAMP],0,dz->iparam[FLT_CNT] * sizeof(double));
	for(n = 1,k = 0; n < dz->iparam[FLT_ENTRYCNT];n+=2,k += dz->iparam[FLT_HARMCNT]) {
		dz->parray[FLT_INFRQ][k] = dz->parray[FLT_FBRK][n];		/* Get frqs of fundamentals into array, leaving space for harmonics */
		dz->parray[FLT_INAMP][k] = dz->parray[FLT_FBRK][n+1];	/* Get amps of fundamentals into array, leaving space for harmonics */
	}
	for(n = 1,k=0; n < dz->iparam[HRM_ENTRYCNT];n+=2,k++) {
		dz->parray[HARM_FRQ_CALC][k] = dz->parray[FLT_HBRK][n];
		dz->parray[HARM_AMP_CALC][k] = dz->parray[FLT_HBRK][n+1];
	}
	for(n=0;n<dz->iparam[FLT_CNT];n+=dz->iparam[FLT_HARMCNT]) {
		z = 0;
		for(k=0; k < dz->iparam[FLT_HARMCNT];k++) {	/* calc vals for partials from basefrq vals */
			dz->parray[FLT_INFRQ][n+k] = dz->parray[FLT_INFRQ][n] * dz->parray[HARM_FRQ_CALC][k];
			dz->parray[FLT_INAMP][n+k] = dz->parray[FLT_INAMP][n] * dz->parray[HARM_AMP_CALC][k];
		}
	}
	return(FINISHED);
}

