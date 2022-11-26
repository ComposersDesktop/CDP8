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



/* floatsams version*/
#include <stdio.h>
#include <stdlib.h>
#include <structures.h>
#include <tkglobals.h>
#include <globcon.h>
#include <processno.h>
#include <modeno.h>
#include <arrays.h>
#include <filters.h>
#include <cdpmain.h>
#include <sfsys.h>
/*RWD*/
#include <string.h>

#ifndef cdp_round
extern int cdp_round(double a);
#endif

#if defined unix || defined __GNUC__
#define round(x) lround((x))
#endif

static void 	filtering(int n,int chans,float *buf,
					double *a,double *b,double *y,double *z,double *d,double *e,double *ampl,dataptr dz);
static double	check_float_limits(double sum, dataptr dz);
static int 		newvalue(int valno,int valincrno, int sampcntno, dataptr dz);
static int 		newfval(int *fsams,dataptr dz);
static int 		do_filters(dataptr dz);
static int 		do_qvary_filters(dataptr dz);
static int 		do_fvary_filters(dataptr dz);
static void		print_filter_frqs(dataptr dz);
static int 		do_varifilter(dataptr dz);
static int 		do_sweep_filter(dataptr dz);
static double 	getfrq(double lfrq,double hfrq,double sfrq,dataptr dz);
static int 		do_allpass_filter(dataptr dz);
static int 		allpass(float *buf,int chans,double prescale,dataptr dz);
static int 		varidelay_allpass(float *buf,int chans,double prescale,dataptr dz);
static int 		do_eq_filter(dataptr dz);
static float    multifilter(double *dll,double *dbb,double *dnn,double *dhh,double *dpp,
					double qfac,double coeff,float input,dataptr dz);
static int 		do_lphp_filter(dataptr dz);
static int 	my_modulus(int x,int y);
static void lphp_filt_chan(double *e1,double *e2,double *s1,double *s2,
					double *den1,double *den2,double *cn,dataptr dz,int chan);

static int		do_fvary2_filters(dataptr dz);

/****************************** FILTER_PROCESS *************************/

int filter_process(dataptr dz)
{
	int exit_status = FINISHED;
	int filter_tail = 0, bufspace;
	if(dz->process==FLTBANKC) {
		print_filter_frqs(dz);
		return(FINISHED);
	}
	display_virtual_time(0,dz);


/* NEW CODE */
	if(dz->process==FLTBANKV) {
		if((exit_status = newfval(&(dz->iparam[FLT_FSAMS]),dz))<0)
			return(exit_status);
	} else 	if(dz->process==FLTBANKV2) {
		dz->iparam[FLT_FSAMS] = dz->iparam[FLT_BLOKSIZE];
		dz->param[FLT_TIMESTEP] = (double)dz->iparam[FLT_BLOKSIZE]/(double)dz->infile->srate;
		dz->param[FLT_TOTALTIME] = 0.0;
		if((exit_status = newfval2(dz->parray[FLT_FBRK],dz->parray[FLT_HBRK],dz))<0)
			return(exit_status);
	}
/* NEW CODE */

	while(dz->samps_left > 0 || filter_tail > 0) {
		memset((char *)dz->sampbuf[0],0,(size_t) (dz->buflen * sizeof(float)));
		if(filter_tail) {
			dz->ssampsread = 0;			
		} else {
			if((exit_status = read_samps(dz->sampbuf[0],dz))<0)
				return(exit_status);
			if(dz->samps_left <= 0) {
				if(dz->process==FLTBANKV || dz->process==FLTBANKV2)
					filter_tail = round(dz->param[FILT_TAILV] * (double)dz->infile->srate) * dz->infile->channels;
				else 
					filter_tail = (int)round((dz->param[FILT_TAIL] * (double)dz->infile->srate) * dz->infile->channels);
            }
        }
		if(filter_tail) {
			bufspace = dz->buflen - dz->ssampsread;
			if(bufspace >= filter_tail) {
				dz->ssampsread += filter_tail;				
				filter_tail = 0;
			} else {
				dz->ssampsread = dz->buflen;				
				filter_tail -= bufspace;
			}
		}
		switch(dz->process) {
		case(FLTBANKN):
		case(FLTBANKU):
			if(dz->brksize[FLT_Q])	
				exit_status = do_qvary_filters(dz);
			else					
				exit_status = do_filters(dz);
			break;
		case(FLTBANKV):	exit_status = do_fvary_filters(dz);		break;
		case(FLTBANKV2):exit_status = do_fvary2_filters(dz);	break;
		case(FLTSWEEP):	exit_status = do_sweep_filter(dz);		break;
		case(FSTATVAR):	exit_status = do_varifilter(dz);		break;
		case(ALLPASS):	exit_status = do_allpass_filter(dz);	break;
		case(EQ):	    exit_status = do_eq_filter(dz);			break;
		case(LPHP):	    exit_status = do_lphp_filter(dz);		break;
		}
		if(exit_status <0)
			return(exit_status);
		if(dz->ssampsread > 0) {
			if((exit_status = write_samps(dz->sampbuf[0],dz->ssampsread,dz))<0)
				return(exit_status);
		}
	}				
    if(dz->iparam[FLT_OVFLW] > 0)  {
		if(!sloom && !sloombatch)
			fprintf(stdout,"Number of overflows: %d\n",dz->iparam[FLT_OVFLW]);
		else
			fprintf(stdout,"INFO: Number of overflows: %d\n",dz->iparam[FLT_OVFLW]);
		fflush(stdout);
	}
	return(FINISHED);
}

/*************************** DO_FVARY_FILTERS *****************************/

int do_fvary_filters(dataptr dz)
{
	int exit_status;
	int    n, m, fno, chans = dz->infile->channels;
	float *buf = dz->sampbuf[0];

	double *fincr = dz->parray[FLT_FINCR];
	double *aincr = dz->parray[FLT_AINCR];
 
 	double *ampl = dz->parray[FLT_AMPL];
	double *a = dz->parray[FLT_A];
	double *b = dz->parray[FLT_B];
	double *y = dz->parray[FLT_Y];
	double *z = dz->parray[FLT_Z];
	double *d = dz->parray[FLT_D];
	double *e = dz->parray[FLT_E];
 	int fsams = dz->iparam[FLT_FSAMS];
	if (dz->vflag[DROP_OUT_AT_OVFLOW]) {
		for (n = 0; n < dz->ssampsread; n += chans) { 
			if(fsams <= 0) {
				if((exit_status = newfval(&fsams,dz))<0)
					return(exit_status);
			}
			if(dz->brksize[FLT_Q]) {
				if((dz->iparam[FLT_SAMS] -= chans) <= 0) {
					if(!newvalue(FLT_Q,FLT_Q_INCR,FLT_SAMS,dz)) {
						sprintf(errstr,"Ran out of Q values: do_fvary_filters()\n");
						return(PROGRAM_ERROR);
					}
					dz->iparam[FLT_SAMS] *= chans;
				}
			}
			if((dz->iparam[FLT_BLOKCNT] -= chans) <= 0) {
				for (fno = 0; fno < dz->iparam[FLT_CNT]; fno++) {
					get_coeffs1(fno,dz);
					get_coeffs2(fno,dz);
				}
				if(dz->brksize[FLT_Q])
					dz->param[FLT_Q] *= dz->param[FLT_Q_INCR];
				
				for(m=0;m<dz->iparam[FLT_CNT];m++) {
					dz->parray[FLT_FRQ][m] *= fincr[m];
					dz->parray[FLT_AMP][m] *= aincr[m];
				}
				dz->iparam[FLT_BLOKCNT] = dz->iparam[FLT_BLOKSIZE] * chans;
			}
			filtering(n,chans,buf,a,b,y,z,d,e,ampl,dz);
			if(dz->iparam[FLT_OVFLW] > 0) {
				sprintf(errstr,"Filter overflowed\n");
				return(GOAL_FAILED);
			}
			fsams--;
		}
	} else {
		for (n = 0; n < dz->ssampsread; n += chans) { 
			if(fsams <= 0) {
				if((exit_status = newfval(&fsams,dz))<0)
					return(exit_status);
			}
			if(dz->brksize[FLT_Q]) {
				if((dz->iparam[FLT_SAMS] -= chans) <= 0) {
					if(!newvalue(FLT_Q,FLT_Q_INCR,FLT_SAMS,dz)) {
			 			sprintf(errstr,"Ran out of Q values: do_fvary_filters()\n");
						return(PROGRAM_ERROR);
					}
					dz->iparam[FLT_SAMS] *= chans;
				}
			}
			if((dz->iparam[FLT_BLOKCNT] -= chans) <= 0) {
				for (fno = 0; fno < dz->iparam[FLT_CNT]; fno++) {
					get_coeffs1(fno,dz);
					get_coeffs2(fno,dz);
				}
				if(dz->brksize[FLT_Q])
					dz->param[FLT_Q] *= dz->param[FLT_Q_INCR];

				for(m=0;m<dz->iparam[FLT_CNT];m++) {
					dz->parray[FLT_FRQ][m] *= fincr[m];
					dz->parray[FLT_AMP][m] *= aincr[m];
				}
				dz->iparam[FLT_BLOKCNT] = dz->iparam[FLT_BLOKSIZE] * chans;
			}
			filtering(n,chans,buf,a,b,y,z,d,e,ampl,dz);
			fsams--;
		}
	}
 	dz->iparam[FLT_FSAMS] = fsams;
	return(CONTINUE);
}

/*************************** DO_FILTERS *******************************/

int do_filters(dataptr dz)
{
	int n;
	int  chans = dz->infile->channels;
	float *buf = dz->sampbuf[0];
	double *ampl = dz->parray[FLT_AMPL];
	double *a = dz->parray[FLT_A];
	double *b = dz->parray[FLT_B];
	double *y = dz->parray[FLT_Y];
	double *z = dz->parray[FLT_Z];
	double *d = dz->parray[FLT_D];
	double *e = dz->parray[FLT_E];
	for (n = 0; n < dz->ssampsread; n += chans)
		filtering(n,chans,buf,a,b,y,z,d,e,ampl,dz);
	return(CONTINUE);
}

/*************************** DO_QVARY_FILTERS *****************************/

int do_qvary_filters(dataptr dz)
{
	int   n;
	int    fno, chans = dz->infile->channels;
	float *buf = dz->sampbuf[0];
	double *ampl = dz->parray[FLT_AMPL];
	double *a = dz->parray[FLT_A];
	double *b = dz->parray[FLT_B];
	double *y = dz->parray[FLT_Y];
	double *z = dz->parray[FLT_Z];
	double *d = dz->parray[FLT_D];
	double *e = dz->parray[FLT_E];

	for (n = 0; n < dz->ssampsread; n += chans) { 
		if((dz->iparam[FLT_SAMS] -= chans) <= 0) {
			if(!newvalue(FLT_Q,FLT_Q_INCR,FLT_SAMS,dz)) {
		 		sprintf(errstr,"Ran out of Q values: do_qvary_filters()\n");
				return(PROGRAM_ERROR);
			}
			dz->iparam[FLT_SAMS] *= chans;
		}
		if((dz->iparam[FLT_BLOKCNT] -= chans) <= 0) {
			for (fno = 0; fno < dz->iparam[FLT_CNT]; fno++)
				get_coeffs2(fno,dz);
			dz->param[FLT_Q] *= dz->param[FLT_Q_INCR];
			dz->iparam[FLT_BLOKCNT] = dz->iparam[FLT_BLOKSIZE] * chans;
		}
		filtering(n,chans,buf,a,b,y,z,d,e,ampl,dz);
	}
	return(CONTINUE);
}

/******************************* NEWVALUE ***************************
 *
 * VAL     is the base value from which we calculate.
 * VALINCR is the value increment per block of samples.
 * SAMPCNT is the number of samples from 1 brkpnt val to next.
 */					   

int newvalue(int valno,int valincrno, int sampcntno, dataptr dz)
{
	double *p;				   
	double ratio, one_over_steps;
	double thistime;
	double thisval;
	int    linear_flag = FALSE;
	if(dz->process==ALLPASS && valno==FLT_DELAY)	
		linear_flag = dz->vflag[FLT_LINDELAY];
	p = dz->brkptr[valno];
	if(p - dz->brk[valno] >= dz->brksize[valno] * 2)
		return(FALSE);
	thistime = (double)round((*p++) * dz->infile->srate);
	thisval  = *p++;
	dz->iparam[sampcntno]    = round(thistime - dz->lastind[valno]);
	/* steps = no_of_samples/sampsize_of_blok: therefore.. */
	one_over_steps = (double)dz->iparam[FLT_BLOKSIZE]/(double)dz->iparam[sampcntno];	
	if(linear_flag)
		dz->param[valincrno] = (thisval - dz->lastval[valno]) * one_over_steps;
	else {
		ratio                = (thisval/dz->lastval[valno]);
		dz->param[valincrno] = pow(ratio,(one_over_steps));
	}					 
	dz->param[valno]     = dz->lastval[valno];
	dz->lastval[valno]   = thisval;
	dz->lastind[valno]   = thistime;
	dz->brkptr[valno]    = p;
	return(TRUE);
}

/******************************* NEWFVAL ***************************
 *
 * VAL is the base value from which we calculate.
 * VALINCR is the value increment per block of samples.
 * FSAMS is the number of samples (per channel) from 1 brkpnt val to next.
 * brk is the particular table we're accessing.
 */

int newfval(int *fsams,dataptr dz)
{
	int   thistime, lasttime;
	double rratio, one_over_steps;
	int   n,m,k;
	double thisval;
	double *lastfval = dz->parray[FLT_LASTFVAL];
	double *lastaval = dz->parray[FLT_LASTAVAL];
	double *aincr    = dz->parray[FLT_AINCR];
	double *fincr    = dz->parray[FLT_FINCR];
	int total_frqcnt = dz->iparam[FLT_CNT] * dz->iparam[FLT_TIMESLOTS];
	
	if(dz->iparam[FLT_TIMES_CNT]>dz->iparam[FLT_TIMESLOTS]) {
		sprintf(errstr,"Ran off end of filter data: newfval()\n"); 
		return(PROGRAM_ERROR);
	}
	k = dz->iparam[FLT_TIMES_CNT];
	lasttime  = dz->lparray[FLT_SAMPTIME][k-1];
	thistime  = dz->lparray[FLT_SAMPTIME][k];
	*fsams = thistime - lasttime;
	/* steps  = fsams/FLT_BLOKSIZE: therefore ... */	
	one_over_steps  = (double)dz->iparam[FLT_BLOKSIZE]/(double)(*fsams);	
	if(dz->iparam[FLT_FRQ_INDEX] >= total_frqcnt)
		return(FINISHED);
	for(n=0, m= dz->iparam[FLT_FRQ_INDEX];n<dz->iparam[FLT_CNT];n++,m++) {
	/* FREQUENCY */
		thisval = dz->parray[FLT_INFRQ][m];
		if(flteq(lastfval[n],thisval))
			fincr[n] = 1.0;
		else {
			rratio = (thisval/lastfval[n]);
			fincr[n] = pow(rratio,one_over_steps);
		}
		dz->parray[FLT_FRQ][n] = lastfval[n];
		lastfval[n] = thisval;
	/* AMPLITUDE */
		thisval = dz->parray[FLT_INAMP][m];
		if(flteq(thisval,lastaval[n]))
			aincr[n] = 1.0;
		else {
			rratio = (thisval/lastaval[n]);
			aincr[n] = pow(rratio,one_over_steps);
		}
		dz->parray[FLT_AMP][n] = lastaval[n];
		lastaval[n] = thisval;
	}
	dz->iparam[FLT_FRQ_INDEX] += dz->iparam[FLT_CNT];
	dz->iparam[FLT_TIMES_CNT]++;
	return(FINISHED);
}

/************************** FILTERING ****************************/

void filtering(int n,int chans,float *buf,double *a,double *b,double *y,double *z,
				double *d,double *e,double *ampl,dataptr dz)
{
	double input, sum, xx;
	int chno, this_samp, fno, i;

	for(chno = 0; chno < chans; chno++) {
		this_samp = n + chno;
		input = (double)buf[this_samp];
		sum   = 0.0;
		for (fno = 0; fno < dz->iparam[FLT_CNT]; fno++) {
			i    = (fno * chans) + chno;
			xx   = input + (a[fno] * y[i]) + (b[fno] * z[i]);
			z[i] = y[i];
			y[i] = xx;
			if(dz->vflag[FLT_DBLFILT]) {
				xx   += (a[fno] * d[i]) + (b[fno] * e[i]);
				e[i] = d[i];
				d[i] = xx;
			}
			sum += (xx * ampl[fno]);
		}
		sum *= dz->param[FLT_GAIN];
		sum = check_float_limits(sum,dz);
		buf[this_samp] = (float) sum;
	}
}

/************************** IO_FILTERING ****************************/

void io_filtering
(float *buf1,float *buf2,int chans,int n,
double *a,double *b,double *y,double *z,double *d,double *z1,double *ampl,dataptr dz)
{
	double input, sum, xx;
	int    chno, this_samp, fno, i;
	for(chno = 0; chno < chans; chno++) {
		this_samp = n + chno;
		input = (double)buf1[this_samp];
		sum   = 0.0;
		for (fno = 0; fno < dz->iparam[FLT_CNT]; fno++) {
			i    = (fno * chans) + chno;
			xx   = input + (a[fno] * y[i]) + (b[fno] * z[i]);
			z[i] = y[i];
			y[i] = xx;
			if(dz->vflag[FLT_DBLFILT]) {
				xx   += (a[fno] * d[i]) + (b[fno] * z1[i]);
				z1[i] = d[i];
				d[i] = xx;
			}
			sum += (xx * ampl[fno]);
		}
		sum *= dz->param[FLT_GAIN];
		sum = check_float_limits(sum,dz);
		buf2[this_samp] = (float) sum;
	}
}

/************************ CHECK_FLOAT_LIMITS **************************/

//TODO: if shorts o/p - do clipping; if floatsams, report but don't change!
double check_float_limits(double sum,dataptr dz)
{
	double peak = fabs(sum);
#ifdef NOTDEF
	//do this when 'modify loudness' can handle floatsams!
	if(dz->true_outfile_stype== SAMP_FLOAT){
		
		if(peak > 1.0){
			dz->iparam[FLT_OVFLW]++;
			dz->peak_fval = max(dz->peak_fval,peak);
		}		
	}
	else {
#endif
		if (sum > 1.0) {
//TW SUGGEST KEEP THIS; prevents FILTER BLOWING UP: see notes
			dz->param[FLT_GAIN] *= 0.9999;
			dz->iparam[FLT_OVFLW]++;
			dz->peak_fval = max(dz->peak_fval,peak);
			//return(1.0);
			if(dz->clip_floatsams)
				sum = 1.0;
		}
		if (sum < -1.0) {
//TW SUGGEST KEEP THIS; prevents FILTER BLOWING UP: see notes

			dz->param[FLT_GAIN] *= 0.9999;
			dz->iparam[FLT_OVFLW]++;
			dz->peak_fval = max(dz->peak_fval,peak);
			//return(-1.0);
			if(dz->clip_floatsams)
				sum = -1.0;
		}
#ifdef NOTDEF
	}
#endif
	return sum;
}
/************************ PRINT_FILTER_FRQS *******************************/

void print_filter_frqs(dataptr dz)
{
	int n;
	double *frq = dz->parray[FLT_FRQ];
	for(n=0;n<dz->iparam[FLT_CNT];n++)
		fprintf(dz->fp,"%lf\n",frq[n]);
}
	
/******************************* DO_VARIFILTER ******************************/

int do_varifilter(dataptr dz)
{
	double *dls = dz->parray[FLT_DLS];
 	double *dbs = dz->parray[FLT_DBS];
	double *dhs = dz->parray[FLT_DHS];
	double *dns = dz->parray[FLT_DNS];
	double *dops[2];
	double coeff = 0.0;
	float  *buf  = dz->sampbuf[0];
	int   n;
	int    k, chans = dz->infile->channels;
	int    is_fbrk = FALSE, is_qbrk = FALSE;
	if(dz->brksize[FLT_ONEFRQ])	is_fbrk = TRUE;
	if(dz->brksize[FLT_Q])	    is_qbrk = TRUE;
	for(n=0;n<chans;n++) {
		switch(dz->mode){
		case(FSW_HIGH):		dops[n] = &(dz->parray[FLT_DHS][n]);	break;
		case(FSW_LOW):		dops[n] = &(dz->parray[FLT_DLS][n]);	break;
		case(FSW_BAND):		dops[n] = &(dz->parray[FLT_DBS][n]);	break;
		case(FSW_NOTCH):	dops[n] = &(dz->parray[FLT_DNS][n]);	break;
		}
	}
	for (n = 0 ; n < dz->ssampsread; n += chans) {
		if(is_fbrk && (--dz->iparam[FLT_FSAMS] <= 0)) {
			if(!newvalue(FLT_ONEFRQ,FLT_F_INCR,FLT_FSAMS,dz)) {
		 		sprintf(errstr,"Ran out of sweepfrq values: do_varifilter()\n");
				return(PROGRAM_ERROR);
			}
		}
		if(is_qbrk && (--dz->iparam[FLT_SAMS] <= 0)) {
			if(!newvalue(FLT_Q,FLT_Q_INCR,FLT_SAMS,dz)) { 
		 		sprintf(errstr,"Ran out of Q values: do_sweep_filter()\n");
				return(PROGRAM_ERROR);
			}
		}
		if(--dz->iparam[FLT_BLOKCNT] <= 0){
			coeff = (2.0 * PI * dz->param[FLT_ONEFRQ]) * dz->param[FLT_INV_SR];
			if(is_fbrk)    dz->param[FLT_ONEFRQ] *= dz->param[FLT_F_INCR];
			if(is_qbrk) {
				dz->param[FLT_QFAC] = 1.0/(1.0 + dz->param[FLT_Q]);
				dz->param[FLT_Q]   *= dz->param[FLT_Q_INCR];
			}
			dz->iparam[FLT_BLOKCNT] = dz->iparam[FLT_BLOKSIZE];
		}
		for(k=0;k<chans;k++)
			buf[n+k] = multifilter(&(dbs[k]),&(dls[k]),&(dhs[k]),&(dns[k]),dops[k],dz->param[FLT_Q],coeff,buf[n+k],dz);
	}
	return(CONTINUE);
}

/******************************* DO_SWEEP_FILTER ******************************/

int do_sweep_filter(dataptr dz)
{
	double *dls = dz->parray[FLT_DLS];
 	double *dbs = dz->parray[FLT_DBS];
	double *dhs = dz->parray[FLT_DHS];
	double *dns = dz->parray[FLT_DNS];
	double *dops[2];
	double coeff = 0.0, frq;
	float  *buf  = dz->sampbuf[0];
	int   n;
	int    k, chans = dz->infile->channels;
	int    is_lbrk = FALSE, is_hbrk = FALSE, is_sbrk = FALSE, is_qbrk = FALSE;
	if(dz->brksize[FLT_LOFRQ])	is_lbrk = TRUE;
	if(dz->brksize[FLT_HIFRQ])	is_hbrk = TRUE;
	if(dz->brksize[FLT_SWPFRQ])	is_sbrk = TRUE;
	if(dz->brksize[FLT_Q])	    is_qbrk = TRUE;
	for(n=0;n<chans;n++) {
		switch(dz->mode){
		case(FSW_HIGH):		dops[n] = &(dz->parray[FLT_DHS][n]);	break;
		case(FSW_LOW):		dops[n] = &(dz->parray[FLT_DLS][n]);	break;
		case(FSW_BAND):		dops[n] = &(dz->parray[FLT_DBS][n]);	break;
		case(FSW_NOTCH):	dops[n] = &(dz->parray[FLT_DNS][n]);	break;
		}
	}
	for (n = 0 ; n < dz->ssampsread; n += chans) {
		if(is_lbrk && (--dz->iparam[FLT_LOSAMS] <= 0)) {
			if(!newvalue(FLT_LOFRQ,FLT_LOINCR,FLT_LOSAMS,dz)) { 
		 		sprintf(errstr,"Ran out of lofrq values: do_sweep_filter()\n");
				return(PROGRAM_ERROR);
			}
		}
		if(is_hbrk && (--dz->iparam[FLT_HISAMS] <= 0)) {
			if(!newvalue(FLT_HIFRQ,FLT_HIINCR,FLT_HISAMS,dz)) { 
		 		sprintf(errstr,"Ran out of hifrq values: do_sweep_filter()\n");
				return(PROGRAM_ERROR);
			}
		}
		if(is_sbrk && (--dz->iparam[FLT_SWSAMS] <= 0)) {
			if(!newvalue(FLT_SWPFRQ,FLT_SWINCR,FLT_SWSAMS,dz)) { 
		 		sprintf(errstr,"Ran out of sweepfrq values: do_sweep_filter()\n");
				return(PROGRAM_ERROR);
			}
		}
		if(is_qbrk && (--dz->iparam[FLT_SAMS] <= 0)) {
			if(!newvalue(FLT_Q,FLT_Q_INCR,FLT_SAMS,dz)) { 
		 		sprintf(errstr,"Ran out of Q values: do_sweep_filter()\n");
				return(PROGRAM_ERROR);
			}
		}
		if(--dz->iparam[FLT_BLOKCNT] <= 0){
			frq   = getfrq(dz->param[FLT_LOFRQ],dz->param[FLT_HIFRQ],dz->param[FLT_SWPFRQ],dz);
			coeff = (2.0 * PI * frq) * dz->param[FLT_INV_SR];
			if(is_lbrk)    dz->param[FLT_LOFRQ]  *= dz->param[FLT_LOINCR];
			if(is_hbrk)    dz->param[FLT_HIFRQ]  *= dz->param[FLT_HIINCR];
			if(is_sbrk)    dz->param[FLT_SWPFRQ] *= dz->param[FLT_SWINCR];
			if(is_qbrk) {
				dz->param[FLT_QFAC] = 1.0/(1.0 + dz->param[FLT_Q]);
				dz->param[FLT_Q]   *= dz->param[FLT_Q_INCR];
			}
			dz->iparam[FLT_BLOKCNT] = dz->iparam[FLT_BLOKSIZE];
		}
		for(k=0;k<chans;k++)
			buf[n+k] = multifilter(&(dbs[k]),&(dls[k]),&(dhs[k]),&(dns[k]),dops[k],dz->param[FLT_Q],coeff,buf[n+k],dz);
	}
	return(CONTINUE);
}

/*************************** GETFRQ ****************************
 *
 * NB sfrq is already adjusted to advance over a whole flt_blok.
 */

double getfrq(double lfrq,double hfrq,double sfrq,dataptr dz)
{
	double thispos;
	double range = hfrq - lfrq;
	thispos  = (cos(dz->param[FLT_CYCPOS]) + 1.0) * 0.5;
	dz->param[FLT_CYCPOS]  =  fmod(dz->param[FLT_CYCPOS] + sfrq,TWOPI);
	return((thispos * range) + lfrq);
}

/******************************* DO_ALLPASS_FILTER ******************************/

int do_allpass_filter(dataptr dz)
{
	int exit_status;
	int    chans    = dz->infile->channels;
	float  *buf     = dz->sampbuf[0];
	double prescale = dz->param[FLT_PRESCALE];

	if(dz->brksize[FLT_DELAY])   
		exit_status = varidelay_allpass(buf,chans,prescale,dz);
	else
		exit_status = allpass(buf,chans,prescale,dz);

	return(exit_status);
}

/******************************* VARIDELAY_ALLPASS ******************************/

int varidelay_allpass(float *buf,int chans,double prescale,dataptr dz)
{
	int   n, thisdelbfpos1, thisdelbfpos2, sampno;
	int    delay, channo;
	double frac, ip, op, delval1,delval2, delval;
	double *delbuf1 = dz->parray[FLT_DELBUF1];
	double *delbuf2 = dz->parray[FLT_DELBUF2];
	int    maxdelsamps = dz->iparam[FLT_MAXDELSAMPS] * chans;
	int	   delbufpos   = dz->iparam[FLT_DELBUFPOS];
	switch(dz->mode) {
	case(FLT_PHASESHIFT):
		for(n=0;n<dz->ssampsread;n+= chans){
			if(--dz->iparam[FLT_SAMS] <= 0) {
				if(!newvalue(FLT_DELAY,FLT_D_INCR,FLT_SAMS,dz)) {
		 			sprintf(errstr,"Ran out of Delay vals: varidelay_allpass()\n");
					return(PROGRAM_ERROR);
				}
			}
			delay = (int)dz->param[FLT_DELAY];
			frac  = dz->param[FLT_DELAY] - (double)delay;
			for(channo = 0;channo < chans; channo++) {
				sampno = n + channo;
				thisdelbfpos1 = my_modulus((delbufpos - (delay*chans)),maxdelsamps);
				thisdelbfpos2 = my_modulus((thisdelbfpos1 + chans)    ,maxdelsamps);
				ip = (double)buf[sampno]	* prescale;
				op = (-dz->param[FLT_GAIN]) * ip;
				delval1     = delbuf1[thisdelbfpos1];
				delval2     = delbuf1[thisdelbfpos2];
				delval      = delval1 + ((delval2 - delval1) * frac);
				op += delval;
				delval1     = delbuf2[thisdelbfpos1];
				delval2     = delbuf2[thisdelbfpos2];
				delval      = delval1 + ((delval2 - delval1) * frac);
				op += dz->param[FLT_GAIN] * delval;
				delbuf1[delbufpos] = ip;
				delbuf2[delbufpos] = op;
				if(++delbufpos >= maxdelsamps)
//					delbufpos = 0;
					delbufpos -= maxdelsamps;		/*RWD 9:2001 */
//TW ?????? maxdelsamps is a mutiple of channels, delbufpos is incremented by 1 each time
//   so (delbufpos -= maxdelsamps) = 0
				buf[sampno] = (float) (op);
			}
			if(dz->vflag[FLT_LINDELAY])
				dz->param[FLT_DELAY] += dz->param[FLT_D_INCR];
			else
				dz->param[FLT_DELAY] *= dz->param[FLT_D_INCR];
		}
		break;
	case(FLT_PHASER):
		for(n=0;n<dz->ssampsread;n+= chans){
			if(--dz->iparam[FLT_SAMS] <= 0) {
				if(!newvalue(FLT_DELAY,FLT_D_INCR,FLT_SAMS,dz)) {
	 				sprintf(errstr,"Ran out of Delay vals: varidelay_allpass()\n");
					return(PROGRAM_ERROR);
				}
			}
			delay = (int)dz->param[FLT_DELAY];
			frac  = dz->param[FLT_DELAY] - (double)delay;
			for(channo = 0;channo < chans; channo++) {
				sampno = n + channo;
				thisdelbfpos1 = my_modulus((delbufpos - (delay*chans)),maxdelsamps);
				thisdelbfpos2 = my_modulus((thisdelbfpos1 + chans)    ,maxdelsamps);
				ip = (double)buf[sampno]	* prescale;
				op = (-dz->param[FLT_GAIN]) * ip;
				delval1     = delbuf1[thisdelbfpos1];
				delval2     = delbuf1[thisdelbfpos2];
				delval      = delval1 + ((delval2 - delval1) * frac);
				op += delval;
				delval1     = delbuf2[thisdelbfpos1];
				delval2     = delbuf2[thisdelbfpos2];
				delval      = delval1 + ((delval2 - delval1) * frac);
				op += dz->param[FLT_GAIN] * delval;
				delbuf1[delbufpos] = ip;
				delbuf2[delbufpos] = op;
				if(++delbufpos >= maxdelsamps)
					//delbufpos = 0;
					delbufpos -= maxdelsamps;	/*RWD 9:2001 (see e_tmp/cdpfloat ) */
//TW ?????? maxdelsamps is a mutiple of channels, delbufpos is incremented by 1 each time
//   so (delbufpos -= maxdelsamps) = 0
				buf[sampno] = (float) ((op + (double)buf[sampno]) * 0.5);
			}
			if(dz->vflag[FLT_LINDELAY])
				dz->param[FLT_DELAY] += dz->param[FLT_D_INCR];
			else
				dz->param[FLT_DELAY] *= dz->param[FLT_D_INCR];
		}
		break;
	default:
		sprintf(errstr,"Unknown case:varidelay_allpass()\n");
		return(PROGRAM_ERROR); 
  	}
	dz->iparam[FLT_DELBUFPOS] = delbufpos;
	return(FINISHED);
}

/******************************* ALLPASS ******************************/

int allpass(float *buf,int chans,double prescale,dataptr dz)
{
	int   n, sampno;
	int    channo;
	double *delbuf1 = dz->parray[FLT_DELBUF1];
	double *delbuf2 = dz->parray[FLT_DELBUF2];
	int    maxdelsamps = dz->iparam[FLT_MAXDELSAMPS] * chans;
	int    delbufpos   = dz->iparam[FLT_DELBUFPOS];
	double ip, op; 
	switch(dz->mode) {
	case(FLT_PHASESHIFT):
		for(n=0;n<dz->ssampsread;n+=chans){
			for(channo = 0;channo < chans; channo++) {
				sampno = n + channo;
				ip = (double)buf[sampno] * prescale;
				op = (-dz->param[FLT_GAIN]) * ip;
				op += delbuf1[delbufpos];
				op += dz->param[FLT_GAIN] * delbuf2[delbufpos];
				delbuf1[delbufpos] = ip;
				delbuf2[delbufpos] = op;
				if(++delbufpos >= maxdelsamps)
					/*delbufpos = 0;*/
					delbufpos -= maxdelsamps;			/*RWD */
//TW ?????? maxdelsamps is a mutiple of channels, delbufpos is incremented by 1 each time
//   so (delbufpos -= maxdelsamps) = 0
				buf[sampno] = (float)(op);
			}
		}
		break;
	case(FLT_PHASER):
		for(n=0;n<dz->ssampsread;n+=chans){
			for(channo = 0;channo < chans; channo++) {
				sampno = n + channo;
				ip = (double)buf[sampno] * prescale;
				op = (-dz->param[FLT_GAIN]) * ip;
				op += delbuf1[delbufpos];
				op += dz->param[FLT_GAIN] * delbuf2[delbufpos];
				delbuf1[delbufpos] = ip;
				delbuf2[delbufpos] = op;
				if(++delbufpos >= maxdelsamps)
					/*delbufpos = 0;*/
					delbufpos -= maxdelsamps;			/*RWD */
//TW ?????? maxdelsamps is a mutiple of channels, delbufpos is incremented by 1 each time
//   so (delbufpos -= maxdelsamps) = 0
				buf[sampno] = (float)((op + (double)buf[sampno]) * 0.5);
			}
		}
		break;
	default:
		sprintf(errstr,"Unknown case: allpass()\n");
		return(PROGRAM_ERROR);
  	}
	dz->iparam[FLT_DELBUFPOS] = delbufpos;
	return(FINISHED);
}

/******************************* DO_EQ_FILTER ******************************/

int do_eq_filter(dataptr dz)
{
	double ip, op;
	int   n, sampno;
	float  *buf = dz->sampbuf[0];
	int   chans = dz->infile->channels, chno;
	double *xx1	= dz->parray[FLT_XX1];
	double *xx2 = dz->parray[FLT_XX2];
	double *yy1 = dz->parray[FLT_YY1];
	double *yy2 = dz->parray[FLT_YY2];
	double a0   = dz->param[FLT_A0];
	double a1   = dz->param[FLT_A1];
	double a2   = dz->param[FLT_A2];
	double b1   = dz->param[FLT_B1];
	double b2   = dz->param[FLT_B2];
	double prescale = dz->param[FLT_PRESCALE];
	//double inv_of_maxsamp = 1.0/(double)F_MAXSAMP;
	for(n = 0;n < dz->ssampsread; n+= chans){
		for(chno = 0;chno < chans; chno++) {
			sampno = n+chno;
			ip  = (double)buf[sampno] /* * inv_of_maxsamp*/;
			ip *= prescale;
			op  = (a0 * ip) + (a1 * xx1[chno]) + (a2 * xx2[chno]) - (b1 * yy1[chno]) - (b2 * yy2[chno]);
			xx2[chno] = xx1[chno];
			xx1[chno] = ip;
			yy2[chno] = yy1[chno];
			yy1[chno] = op;
			buf[sampno] = (float)(op);
		}
	}
	return(FINISHED);
}

/************************** MULTIFILTER **************************/

float multifilter(double *dll,double *dbb,double *dnn,double *dhh,double *dpp,
					double qfac,double coeff,float input,dataptr dz)
{
	double dya, dsa;
	dya   = (double)input;
	*dhh  = (qfac * *dbb) - *dll - dya;
	*dbb  = *dbb - (coeff * *dhh);
	*dll  = *dll - (coeff * *dbb);
	*dnn  = *dll + *dhh;
	dsa   = (*dpp) * dz->param[FLT_GAIN]; /* dpp points to dhh,dbb,dll,dnn */
#ifdef NOTDEF
	if (fabs(dsa) > F_MAXSAMP) {
		dz->iparam[FLT_OVFLW]++;
		dz->param[FLT_GAIN] *= .9999;
		if (dsa  > 0.0) 
			dsa = (double)F_MAXSAMP;
		else 
			dsa = (double)F_MINSAMP;
	}
	return((float)dsa);	 /* TRUNCATE */
#else
	return (float) check_float_limits(dsa,dz);
#endif
}

/***************************** FILTER *************************************/
/* RWD TODO: add fix for nchans */

//int do_lphp_filter(dataptr dz)
//{
//	double *e1s,*e2s,*s1s,*s2s;
//	double *e1	 = dz->parray[FLT_E1];
//	double *e2	 = dz->parray[FLT_E2];
//	double *s1	 = dz->parray[FLT_S1];
//	double *s2	 = dz->parray[FLT_S2];
//	double *den1 = dz->parray[FLT_DEN1];
//	double *den2 = dz->parray[FLT_DEN2];
//	double *cn	 = dz->parray[FLT_CN];
//	switch(dz->infile->channels) {
//	case(STEREO):
//		e1s	 = dz->parray[FLT_E1S];
//		e2s	 = dz->parray[FLT_E2S];
//		s1s	 = dz->parray[FLT_S1S];
//		s2s	 = dz->parray[FLT_S2S];
//		lphp_filt_stereo(e1,e2,s1,s2,e1s,e2s,s1s,s2s,den1,den2,cn,dz);
//		break;
//	case(MONO):
//		lphp_filt(e1,e2,s1,s2,den1,den2,cn,dz);
//		break;
//	}
//	return(FINISHED);
//}
//		
// TW MULTICHAN 2010
int do_lphp_filter(dataptr dz)
{
	int i;
	int index;
	
	double *e1,*e2,*s1,*s2;
	
	double *den1 = dz->parray[FLT_DEN1];
	double *den2 = dz->parray[FLT_DEN2];
	double *cn	 = dz->parray[FLT_CN];	

	for(i=0; i < dz->infile->channels; i++) {
		index = i * FLT_LPHP_ARRAYS_PER_FILTER;
	
		e1	 = dz->parray[FLT_E1_BASE + index];
		e2	 = dz->parray[FLT_E2_BASE + index];
		s1	 = dz->parray[FLT_S1_BASE + index];
		s2	 = dz->parray[FLT_S2_BASE + index];
		
		lphp_filt_chan(e1,e2,s1,s2,den1,den2,cn,dz,i);

	}
	return(FINISHED);
}

/***************************** LPHP_FILT *************************************/

void lphp_filt(double *e1,double *e2,double *s1,double *s2,
					double *den1,double *den2,double *cn,dataptr dz)
{
	int i;
	int  k;
	float *buf = dz->sampbuf[0];
	double ip, op = 0.0, b1;
 	for (i = 0 ; i < dz->ssampsread; i++) {
		ip = (double) buf[i];
		for (k = 0 ; k < dz->iparam[FLT_CNT]; k++) {
			b1    = dz->param[FLT_MUL] * cn[k];
			op    = (cn[k] * ip) + (den1[k] * s1[k]) + (den2[k] * s2[k]) + (b1 * e1[k]) + (cn[k] * e2[k]);
			s2[k] = s1[k];
			s1[k] = op;
			e2[k] = e1[k];
			e1[k] = ip;
		}
		op *= dz->param[FLT_PRESCALE];
				//RWD NB gain modification...
#ifdef NOTDEF
		if (fabs(op) > 1.0) {
			dz->iparam[FLT_OVFLW]++;
//TW SUGGEST KEEP THIS: PREVENTS FILTER BLOWING UP
			dz->param[FLT_PRESCALE] *= .9999;
			if (op  > 0.0)
				op = 1.0;
			else 
				op = -1.0;
		}
		buf[i] = (float)op;
#else
		buf[i] = (float) check_float_limits(op,dz);
#endif
	}
}

/*RWD 4:2000 */
void lphp_filt_chan(double *e1,double *e2,double *s1,double *s2,
					double *den1,double *den2,double *cn,dataptr dz,int chan)
{
	int i;
	int  k;
	float *buf = dz->sampbuf[0];
	double ip, op = 0.0, b1;
 	for (i = chan ; i < dz->ssampsread; i+= dz->infile->channels) {
		ip = (double) buf[i];
		for (k = 0 ; k < dz->iparam[FLT_CNT]; k++) {
			b1    = dz->param[FLT_MUL] * cn[k];
			op    = (cn[k] * ip) + (den1[k] * s1[k]) + (den2[k] * s2[k]) + (b1 * e1[k]) + (cn[k] * e2[k]);
			s2[k] = s1[k];
			s1[k] = op;
			e2[k] = e1[k];
			e1[k] = ip;
		}
		op *= dz->param[FLT_PRESCALE];
		//RWD NB gain modification...
#ifdef NOTDEF
		if (fabs(op) > 1.0) {
			dz->iparam[FLT_OVFLW]++;
//TW SUGGEST KEEP THIS: PREVENTS FILTER BLOWING UP
			dz->param[FLT_PRESCALE] *= .9999;
			if (op  > 0.0)
				op = 1.0;
			else 
				op = -1.0;
		}
		buf[i] = (float)op;
#else
		buf[i] = (float) check_float_limits(op,dz);
#endif
	}
}

/***************************** LPHP_FILT_STEREO *************************************/

void lphp_filt_stereo(double *e1,double *e2,double *s1,double *s2,double *e1s,double *e2s,double *s1s,double *s2s,
					double *den1,double *den2,double *cn,dataptr dz)
{
	int i, j;
	int  k;
	float *buf = dz->sampbuf[0];
	double ip, op = 0.0, b1;
 	for (i = 0 ; i < dz->ssampsread; i+=2) {
		ip = (double)buf[i];
		for (k = 0 ; k < dz->iparam[FLT_CNT]; k++) {
			b1    = dz->param[FLT_MUL] * cn[k];
			op    = (cn[k] * ip) + (den1[k] * s1[k]) + (den2[k] * s2[k]) + (b1 * e1[k]) + (cn[k] * e2[k]);
			s2[k] = s1[k];
			s1[k] = op;
			e2[k] = e1[k];
			e1[k] = ip;
		}
		op *= dz->param[FLT_PRESCALE];
#ifdef NOTDEF
		if (fabs(op) > F_MAXSAMP) {
			dz->iparam[FLT_OVFLW]++;
			dz->param[FLT_PRESCALE] *= .9999;
			if (op  > 0.0)
				op = (double)F_MAXSAMP;
			else 
				op = (double)F_MINSAMP;
		}
		buf[i] = (float)op;
#else
		buf[i] = (float) check_float_limits(op,dz);
#endif
		j = i+1;
		ip = (double)buf[j];
		for (k = 0 ; k < dz->iparam[FLT_CNT]; k++) {
			b1     = dz->param[FLT_MUL] * cn[k];
			op     = (den1[k] * s1s[k]) + (den2[k] * s2s[k]) + (cn[k] * ip) + (b1 * e1s[k]) + (cn[k] * e2s[k]);
			s2s[k] = s1s[k];
			s1s[k] = op;
			e2s[k] = e1s[k];
			e1s[k] = ip;
		}
		op *= dz->param[FLT_PRESCALE];
#ifdef NOTDEF
		if (fabs(op) > F_MAXSAMP) {
			dz->iparam[FLT_OVFLW]++;
			dz->param[FLT_PRESCALE] *= .9999;
			if (op  > 0.0)
				op = (double)F_MAXSAMP;
			else 
				op = (double)F_MINSAMP;
		}
		buf[j] = (float)op;
#else
		buf[j] = (float) check_float_limits(op,dz);
#endif
	}
}

/***************************** MY_MODULUS *************************************/

int my_modulus(int x,int y)
{
	if(x>=0)
		return(x%y);
	while(x < 0)
		x += y;
	return(x);
}

/**************************** NEWFVAL2 *******************************/

int newfval2(double *fbrk,double *hbrk,dataptr dz)
{
	int n, m, k,/* z,*/ timepoint, nextrow;
	int entrycnt = dz->iparam[FLT_ENTRYCNT];
	int wordcnt  = dz->iparam[FLT_WORDCNT];
	int hentrycnt = dz->iparam[HRM_ENTRYCNT];
	int hwordcnt  = dz->iparam[HRM_WORDCNT];
	//int srate = dz->infile->srate;
	double lotime, hitime, timefrac, valdiff;
	double thistime = dz->param[FLT_TOTALTIME];

	memset((char *)dz->parray[FLT_INFRQ],0,dz->iparam[FLT_CNT] * sizeof(double));
	memset((char *)dz->parray[FLT_INAMP],0,dz->iparam[FLT_CNT] * sizeof(double));
	timepoint = 0;
	while(thistime >= fbrk[timepoint]) {	/* search times in frq array */
		timepoint += entrycnt;
		if(timepoint >= wordcnt)
			break;
	}
	timepoint -= entrycnt;
	lotime = fbrk[timepoint];
	nextrow = timepoint + entrycnt;
	for(n = timepoint+1,k = 0; n < nextrow;n+=2,k += dz->iparam[FLT_HARMCNT]) {
		dz->parray[FLT_INFRQ][k] = fbrk[n];		/* Get frqs of fundamentals into array, leaving space for harmonics */
		dz->parray[FLT_INAMP][k] = fbrk[n+1];	/* Get amps of fundamentals into array, leaving space for harmonics */
	}
	if(nextrow != wordcnt) {					/* if not at end of table, do interpolation */
		nextrow += entrycnt;
		timepoint += entrycnt;
		hitime = fbrk[timepoint];
		timefrac = (thistime - lotime)/(hitime - lotime);
		k = 0;
		for(n = timepoint+1,k=0; n < nextrow;n+=2,k += dz->iparam[FLT_HARMCNT]) {
			/* FRQ */
			valdiff = fbrk[n] - dz->parray[FLT_INFRQ][k];
			valdiff *= timefrac;
			dz->parray[FLT_INFRQ][k] = dz->parray[FLT_INFRQ][k] + valdiff;
			/* AMP */
			valdiff = fbrk[n+1] - dz->parray[FLT_INAMP][k];
			valdiff *= timefrac;
			dz->parray[FLT_INAMP][k] = dz->parray[FLT_INAMP][k] + valdiff;
		}
	}
	timepoint = 0;
	while(thistime >= hbrk[timepoint]) {	/* search times in frq array */
		timepoint += hentrycnt;
		if(timepoint >= hwordcnt) {
			break;
		}
	}
	timepoint -= hentrycnt;
	lotime = hbrk[timepoint];
	nextrow = timepoint + hentrycnt;
	k = 0;
	for(n = timepoint+1,k=0; n < nextrow;n+=2,k++) {
		dz->parray[HARM_FRQ_CALC][k] = hbrk[n];
		dz->parray[HARM_AMP_CALC][k] = hbrk[n+1];
	}
	if(nextrow != hwordcnt) {					/* if not at end of table, do interpolation */
		nextrow += hentrycnt;
		timepoint += hentrycnt;
		hitime = hbrk[timepoint];
		timefrac = (thistime - lotime)/(hitime - lotime);
		k = 0;
		for(n = timepoint+1,k=0; n < nextrow;n+=2,k++) {
			/* PARTIAL MULTIPLIER */
			valdiff = hbrk[n] - dz->parray[HARM_FRQ_CALC][k];
			valdiff *= timefrac;
			dz->parray[HARM_FRQ_CALC][k] = dz->parray[HARM_FRQ_CALC][k] + valdiff;
			/* PARTIAL AMP */
			valdiff = hbrk[n+1] - dz->parray[HARM_AMP_CALC][k];
			valdiff *= timefrac;
			dz->parray[HARM_AMP_CALC][k] = dz->parray[HARM_AMP_CALC][k] + valdiff;
		}
	}
	for(k=0;k<dz->iparam[FLT_CNT];k+=dz->iparam[FLT_HARMCNT]) {
//		z = 0;
		for(m=0; m < dz->iparam[FLT_HARMCNT];m++) {	/* calc vals for partials from basefrq vals */
			dz->parray[FLT_INFRQ][k+m] = dz->parray[FLT_INFRQ][k] * dz->parray[HARM_FRQ_CALC][m];
			dz->parray[FLT_INAMP][k+m] = dz->parray[FLT_INAMP][k] * dz->parray[HARM_AMP_CALC][m];
			dz->parray[FLT_FRQ][k+m] = dz->parray[FLT_INFRQ][k+m];
			dz->parray[FLT_AMP][k+m] = dz->parray[FLT_INAMP][k+m];
		}
	}
	dz->param[FLT_TOTALTIME] += dz->param[FLT_TIMESTEP];
	return(FINISHED);
}

/*************************** DO_FVARY2_FILTERS *****************************/

int do_fvary2_filters(dataptr dz)
{
	int exit_status;
	int    n, fno, chans = dz->infile->channels;
	float *buf = dz->sampbuf[0];

	//double *fincr = dz->parray[FLT_FINCR];
	//double *aincr = dz->parray[FLT_AINCR];
 
 	double *ampl = dz->parray[FLT_AMPL];
	double *a = dz->parray[FLT_A];
	double *b = dz->parray[FLT_B];
	double *y = dz->parray[FLT_Y];
	double *z = dz->parray[FLT_Z];
	double *d = dz->parray[FLT_D];
	double *e = dz->parray[FLT_E];
 
 	int fsams = dz->iparam[FLT_FSAMS];
	for (n = 0; n < dz->ssampsread; n += chans) { 
		if(dz->brksize[FLT_Q]) {
			if((dz->iparam[FLT_SAMS] -= chans) <= 0) {
				if(!newvalue(FLT_Q,FLT_Q_INCR,FLT_SAMS,dz)) {
			 		sprintf(errstr,"Ran out of Q values: do_fvary2_filters()\n");
					return(PROGRAM_ERROR);
				}
				dz->iparam[FLT_SAMS] *= chans;
			}
		}
		if(fsams <= 0) {
			if((exit_status = newfval2(dz->parray[FLT_FBRK],dz->parray[FLT_HBRK],dz))<0)
				return(exit_status);
			fsams = dz->iparam[FLT_FSAMS];
			for (fno = 0; fno < dz->iparam[FLT_CNT]; fno++) {
				get_coeffs1(fno,dz);
				get_coeffs2(fno,dz);
			}
			if(dz->brksize[FLT_Q])
				dz->param[FLT_Q] *= dz->param[FLT_Q_INCR];
		}

		filtering(n,chans,buf,a,b,y,z,d,e,ampl,dz);
		fsams--;
	}
	return(CONTINUE);
}

