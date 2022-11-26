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



/* floatsam version*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <structures.h>
#include <tkglobals.h>
#include <globcon.h>
#include <modeno.h>
#include <arrays.h>
#include <distort.h>
#include <cdpmain.h>

#include <sfsys.h>
#include <osbind.h>

#if defined unix || defined __GNUC__
#define round lround
#endif

//TW COMMENT : phase as used here, with vals >1 or <-1 (meaning above zero or below zero) in intrinsically 'int' 
//             phase is also not 'float' elsewhere, 
//			   To avoid the construction "switch((int)phase)", I've made phase 'int' everywhere, casting in situ where ness.

//static int 	distort_func(float *b,int tthis,int i,float phase,int lastzero,float cyclemax,dataptr dz);
static int 	distort_func(float *b,int tthis,int i,int phase,int lastzero,float cyclemax,dataptr dz);
//static int 	distort_func_crosbuf(int tthis,int i,float phase,int lastzero,float cyclemax,int oldbufcnt,dataptr dz);
static int 	distort_func_crosbuf(int tthis,int i,int phase,int lastzero,float cyclemax,int oldbufcnt,dataptr dz);
static int 	sinusoid(int tthis,int lastzero,float cyclemax,int endcycle,dataptr dz);
static int 	end_sinusoid(int tthis,int lastzero,float cyclemax,int newbufcnt,int oldbufcnt,dataptr dz);
static int 	genpulse(int step,float maxamp,float *bufptr,float *bufend);
static int 	triangle(float *b,int lastzero,float cyclemax,int endcycle);
static int 	end_triangle(int tthis,int lastzero,float cyclemax,int newbufcnt,int oldbufcnt,dataptr dz);
//static int 	exaggerate(int lastzero,int thiszero,float phase,float *b,double powfac);
static int 	exaggerate(int lastzero,int thiszero,int phase,float *b,double powfac);
static int 	do_exagg
			(double lastmax,int lastpos,double thismax,int thispos,double powfac,float *b);
//static int 	do_exagg2(float phase,int lastpos,int thispos,double powfac,float *b);
static int 	do_exagg2(int phase,int lastpos,int thispos,double powfac,float *b);
//static int 	end_exagg(int tthis,int lastzero,int newbufcnt,int oldbufcnt,double powfac,float phase,dataptr dz);
static int 	end_exagg(int tthis,int lastzero,int newbufcnt,int oldbufcnt,double powfac,int phase,dataptr dz);
static int 	get_powfac(int lastzero,int thiszero,dataptr dz);

/************************* DO_DISTORT ***********************************
 *
 * Works on SINGLE HALF wavecycles.
 *
 * This looks for single half-wavecycles, and does not change there
 * number (or their phase) in the output. It therefore resets the
 * input & output phase on each entry.
 */								   

int do_distort(int tthis,int is_last,int *lastzero,float *cyclemax,dataptr dz)
{
	int exit_status;
	register int i = *lastzero;
	int	oldbufcnt, lastmarker;
	int 	phase = 1;	   
	float 	*b 	    = dz->sampbuf[tthis];
	int 	samples = dz->buflen;
	if(is_last)
		samples  = dz->ssampsread;
//TW CHANGED
	while(smpflteq(b[i],0.0) && i<samples)
		i++;
	lastmarker = i;
	if(b[i]<0)
		phase  = -1;
	while(i < samples)  {				
		switch(phase) {
		case(1):	    
			if(b[i] > 0) {
				if(b[i]>*cyclemax)
					*cyclemax=b[i];
				i++;
			} else {
				if((exit_status = distort_func(b,tthis,i,phase,*lastzero,*cyclemax,dz))<0)
					return(exit_status);
				i = lastmarker = reset(i,samples,b,lastzero,cyclemax);
				if(b[i]<0)
					phase=-1;
			}
			break;
		case(-1):
			if(b[i] < 0) {
				if(b[i]<*cyclemax)
					*cyclemax=b[i];
				i++;
			} else {
				if((exit_status = distort_func(b,tthis,i,phase,*lastzero,*cyclemax,dz))<0)
					return(exit_status);
				i = lastmarker = reset(i,samples,b,lastzero,cyclemax);
				if(b[i]>0)
					phase=1;
			}
			break;
		}
	}
	if(!is_last) {
		oldbufcnt = dz->buflen - *lastzero;
		b = dz->sampbuf[!tthis];
		samples = dz->ssampsread;
		i = 0;
		switch(phase) {
		case(1):	    
			while(i < samples)  {
				if(b[i]>0) {
					if(b[i]>*cyclemax)
						*cyclemax = b[i];
					i++;
				} else
					break;
			}
			break;
		case(-1):
			while(i < samples)  {
				if(b[i]<0) {
					if(b[i]<*cyclemax)
						*cyclemax = b[i];
					i++;
				} else
					break;
			}
			break;
		}
		if((exit_status = distort_func_crosbuf(tthis,i,phase,*lastzero,*cyclemax,oldbufcnt,dz))<0)
			return(exit_status);
		i = lastmarker = reset(i,samples,dz->sampbuf[!tthis],lastzero,cyclemax);
	}
	return(FINISHED);
}

/*************************** DISTORT_FUNC ******************************/

int distort_func(float *b,int tthis,int i,int phase,int lastzero,float cyclemax,dataptr dz)
{
	int exit_status;
	register int j;
	int halfcycle, step;
	float	 clipmax;
	switch(dz->mode) {
	case(DISTORT_SQUARE_FIXED):
		clipmax = FCLIPMAX * (float)phase;
		for(j=lastzero;j<i;j++)
			b[j]=clipmax;			/* 4 */
		break;
	case(DISTORT_SQUARE):
		for(j=lastzero;j<i;j++)
			b[j]=cyclemax;
		break;
	case(DISTORT_TRIANGLE_FIXED):
		clipmax = FCLIPMAX * (float)phase;
		if(i - lastzero > 3) {
			if((exit_status = triangle(b,lastzero,clipmax,i))<0)
				return(exit_status);
		}
		break;
	case(DISTORT_TRIANGLE):
		if(i - lastzero > 3) {
			if((exit_status = triangle(b,lastzero,cyclemax,i))<0)
				return(exit_status);
		}
		break;
	case(DISTORT_INVERT_HALFCYCLE):
		for(j=lastzero;j<i;j++)
			b[j]=cyclemax - b[j];
		break;
	case(DISTORT_CLICK):
		halfcycle = i - lastzero;
		step = min((int)dz->iparam[DISTORT_PULSE],halfcycle);
		if((exit_status = genpulse(step,cyclemax,b+lastzero,b+i))<0)
			return(exit_status);
		break;
	case(DISTORT_SINE):
		if(i - lastzero > 5) {
			if((exit_status = sinusoid(tthis,lastzero,cyclemax,i,dz))<0)
				return(exit_status);
		}
		break;
	case(DISTORT_EXAGG):
		if(dz->brksize[DISTORT_POWFAC] && (exit_status = get_powfac(lastzero,i,dz))<0)
			return(exit_status);
		if((exit_status = exaggerate(lastzero,i,phase,dz->sampbuf[tthis],dz->param[DISTORT_POWFAC]))<0)
			return(exit_status);
		break;
	default:
		sprintf(errstr,"Unknown case in distort_func()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/************************ DISTORT_FUNC_CROSBUF **********************/

int distort_func_crosbuf(int tthis,int i,int phase,int lastzero,float cyclemax,int oldbufcnt,dataptr dz)
{
	int exit_status;
	register int j;
	int end_bit, halfcycle, step;
	float *buf = dz->sampbuf[tthis];
	/*int*/float clipmax;
	switch(dz->mode) {
	case(DISTORT_SQUARE_FIXED):
		clipmax = (float)(FCLIPMAX * (float)phase);
		for(j=lastzero;j<dz->buflen; j++)
			buf[j] = clipmax;
		buf = dz->sampbuf[!tthis];
		for(j=0;j<i;j++)
			buf[j] = clipmax;
		break;
	case(DISTORT_SQUARE):
		for(j=lastzero;j<dz->buflen; j++)
			buf[j] = cyclemax;
		buf = dz->sampbuf[!tthis];
		for(j=0;j<i;j++)	      
			buf[j] = cyclemax;
		break;
	case(DISTORT_TRIANGLE_FIXED):
		clipmax = (float)(FCLIPMAX * (float)phase);
		if(oldbufcnt + i > 3) {
			if((exit_status = end_triangle(tthis,lastzero,clipmax,i,oldbufcnt,dz))<0)
				return(exit_status);
		}
		break;
	case(DISTORT_TRIANGLE):
		if(oldbufcnt + i > 3) {
			if((exit_status = end_triangle(tthis,lastzero,cyclemax,i,oldbufcnt,dz))<0)
				return(exit_status);
		}
		break;
	case(DISTORT_INVERT_HALFCYCLE):
		for(j=lastzero;j<dz->buflen; j++) 
			buf[j] = cyclemax - buf[j];
		buf = dz->sampbuf[!tthis];
		for(j=0;j<i;j++)	      
			buf[j] = cyclemax - buf[j];
		break;
	case(DISTORT_CLICK):
		end_bit = dz->buflen - lastzero;
		halfcycle = i + end_bit;
		step = min((int)dz->iparam[DISTORT_PULSE],halfcycle);
		if(step > end_bit) {
			if((exit_status = genpulse(end_bit,cyclemax,buf+lastzero,buf+dz->buflen))<0)
				return(exit_status);
			buf = dz->sampbuf[!tthis];
			if((exit_status = genpulse(step - end_bit,cyclemax,buf,buf+i))<0)
				return(exit_status);
		} else {
			if((exit_status = genpulse(end_bit,cyclemax,buf + lastzero,buf + dz->buflen))<0)
				return(exit_status);
			buf = dz->sampbuf[!tthis];
			if((exit_status = genpulse(0,cyclemax,buf,buf+i))<0)
				return(exit_status);
		}
		break;
	case(DISTORT_SINE):
		if(oldbufcnt + i > 5) {
			if((exit_status = end_sinusoid(tthis,lastzero,cyclemax,i,oldbufcnt,dz))<0)
				return(exit_status);
		}
		break;
	case(DISTORT_EXAGG):
		if(dz->brksize[DISTORT_POWFAC] && (exit_status = get_powfac(lastzero,dz->buflen+i,dz))<0)
			return(exit_status);
		if((exit_status = end_exagg(tthis,lastzero,i,oldbufcnt,dz->param[DISTORT_POWFAC],phase,dz))<0)
			return(exit_status);
		break;
	default:
		sprintf(errstr,"Unknown case in distort_func_crosbuf()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/***************************** SINUSOID ******************************
 *
 * Create sinusoidal half-wave, over half-cycle.
 */

int sinusoid(int tthis,int lastzero,float cyclemax,int endcycle,dataptr dz)
{
	float *bb          = dz->sampbuf[tthis]; 
	int i, j, start   = lastzero;					/* 1 */
	int half_cyclecnt = endcycle - lastzero;		/* 2 */
	double index_factor = (double)SINETABLEN/(double)half_cyclecnt;
	for(j=start, i=0; j<endcycle; j++, i++)
		bb[j] = /*round*/(float) (dz->parray[DISTORT_SIN][round(i * index_factor)] * cyclemax);
	return(FINISHED);
}

/*********************** END_SINUSOID ***************************/

int end_sinusoid(int tthis,int lastzero,float cyclemax,int newbufcnt,int oldbufcnt,dataptr dz)
{
	float *bb           = dz->sampbuf[tthis];
	int i, j, start    = lastzero;
	int half_cyclecnt  = oldbufcnt + newbufcnt;
	double index_factor = (double)SINETABLEN/(double)half_cyclecnt;
	for(j=start, i=0; j<dz->buflen; j++,i++)
		bb[j] = /*round*/(float)(dz->parray[DISTORT_SIN][round(i * index_factor)] * cyclemax);
	bb = dz->sampbuf[!tthis];
	for(j=0; j<newbufcnt; j++, i++)
		bb[j] = /*round*/(float)(dz->parray[DISTORT_SIN][round(i * index_factor)] * cyclemax);
	return(FINISHED);
}

/***************************** GENPULSE ****************************/

int genpulse(int step,float maxamp,float *bufptr,float *bufend)
{
	int n, m, z, sum = 0;
	float ampval =  maxamp;
	while(sum < step) {
		n = round(drand48() * MAXWIDTH);	  /* TRUNCATE */
		n++;
		sum += n;
		z = min(n,step-sum);
		for(m=0;m<z;m++)
			*bufptr++ = ampval;
		ampval = -ampval;
	}
	while(bufptr<bufend)
		*bufptr++ = 0;
	return(FINISHED);
}

/***************************** TRIANGLE ******************************
 *
 * Create triangular half-wave, over half-cycle.
 *
 * (0)	Samples values calculated as reals, then approxd to integers.
 * (1)	establish start of loop after initial sample (set to zero).
 * (2)  'half_cyclecnt' is number of samples in the half_cycle.
 * 	'halfcnt' is count to mid-point of triangle form.
 * (3)	establish midpoint of half_cycle.
 * (4)	Set initial sample to zero.
 * (5)	Calculate real increment(decrement) to create triangular form.
 * (6)	Create triangle from start to middle.
 * (7)	For even number of samples in half-cycle, add extra peak sample.
 * (8) 	Create middle sample of triangle.
 * (9)	Create triangle from middle to end.
 */

int triangle(float *b,int lastzero,float cyclemax,int endcycle)
{
	double realval = 0.0, step;						/* 0 */
	int start     = lastzero+1;					/* 1 */
	int half_cyclecnt = endcycle - lastzero;		/* 2 */
	int halfcnt   = (half_cyclecnt+1)/2;
	int middle    = lastzero + halfcnt; 			/* 3 */
	int j;
	b[lastzero] = 0;								/* 4 */
	step = (double)cyclemax/(double)halfcnt;		/* 5 */
	for(j=start; j<middle; j++) {
		realval += step;							/* 6 */
		b[j]    = (float)realval;
	}
	if(EVEN(half_cyclecnt))
		realval += step;							/* 7 */
	b[middle] = (float)/*round*/(realval);				/* 8 */
	for(j=middle+1; j<endcycle; j++) {
		realval -= step;							/* 9 */
		b[j]     = (float)realval;
	}
	return(FINISHED);
}

/*********************** END_TRIANGLE ***************************
 *
 * (0)	Sample values calculated as reals, then approximated to integers.
 * (1)	half_cycle is now that counted in original buffer, plus the part
 *	counted into the next buffer (newbufcnt).
 * (1a)	The count as far as middle of half-cycle.
 * (1b)	middle = actual sample index of midpoint of half-cycle.
 * (1c) Set inital sample of half-cycle to a zero.
 * (1d) calculate the increment (=decrement) to create triangle wave.
 * (2)  If midpoint of triangle is in ORIGINAL buffer...
 * (3)	..create values from start to middle,
 * (A)  ..if half_cycle has even number of samples, get extra peak value.
 * (5)  ..put in middle value.
 * (6)	..create values from middle to end of buf.
 * (7)	..switch to other buf.
 * (8)	..create values from start of new buf, to end of triangle.
 * (9)  Else, midpoint of triangle is in OTHER buffer...
 * (10)	..create values from start to end of buffer.
 * (11)	..decrement location of middle by size of original buffer.
 * (12)	..switch to other buf.
 * (13)	..create values from start of new buf, to middle of triangle.
 * (14) ..if half_cycle has even number of samples, get extra peak value.
 * (15) ..put in middle value.
 * (16)	..create values from middle to end of triangle.
 */

int end_triangle(int tthis,int lastzero,float cyclemax,int newbufcnt,int oldbufcnt,dataptr dz)
{
	double realval = 0.0, step;					/* 0 */
	int start     = lastzero+1;
	int half_cyclecnt  = oldbufcnt + newbufcnt;/* 1 */
	int halfcnt   = (half_cyclecnt+1)/2;		/* 1a */
	int middle    = lastzero + halfcnt;		/* 1b */
	int j;
	float *b = dz->sampbuf[tthis];
	b[lastzero] = 0.0;							/* 1c */
	step = (double)cyclemax/(double)(halfcnt);
	if(middle < dz->buflen) {					/* 2 */
		for(j=start; j<middle; j++) {
			realval += step;					/* 3 */
			b[j]    = (float)realval;
		}
		if(EVEN(half_cyclecnt))					/* A */
			realval += step;
		b[middle] = (float)realval;				/* 5 */
		for(j=middle+1; j<dz->buflen; j++) {
			realval -= step;					/* 6 */
			b[j]    = (float)realval;
		}
		b = dz->sampbuf[!tthis];				/* 7 */
		for(j=0; j<newbufcnt; j++) {
			realval -= step;					/* 8 */
			b[j]    = (float)realval;
		}
	} else {									/* 9 */
		for(j=start; j<dz->buflen; j++) {
			realval += step;					/* 10 */
			b[j]    = (float)realval;
		}
		middle -= dz->buflen;					/* 11 */
		b = dz->sampbuf[!tthis];				/* 12 */
		for(j=0; j<middle; j++) {
			realval += step;					/* 13 */
			b[j]    = (float)realval;
		}
		if(EVEN(half_cyclecnt))					/* 14 */
			realval += step;
		b[middle] = (float)realval;				/* 15 */
		for(j=middle+1; j<newbufcnt; j++) {
			realval -= step;					/* 16 */
			b[j]     = (float)realval;
		}
	}
	return(FINISHED);
}

/************************** RESET ******************************
 *
 * (9)	Reset lastzero, marking state of new (half)wavecycle.
 * (10) Skip any zero samples : drop out if end of samples in buffer reached.
 * (11)	Reset maximum sample in halfcycle to mimimum (zero).
 * (12) Return first NON-ZERO sample.
 */

int reset(int i,int samples,float *bbuf,int *lastzero,float *cyclemax)
{
	*lastzero = i;						/* 9 */
	while(bbuf[i]==0.0 && i < samples)	/* 10 */
		i++;
	*cyclemax=0.0;						/* 11 */
	return(i);							/* 12 */
}

/****************************** EXAGGERATE *******************************/

int exaggerate(int lastzero,int thiszero,int phase,float *b,double powfac)
{
	int exit_status;
	int n = lastzero + 1;
	double lastmax, thismax, thismin;
	int   lastpos, thispos, minpos;
	int OK = 1;
	switch(phase) {
	case(1):
		while(b[n] >= b[n-1])
			n++;
		lastmax = (double)b[n-1];
		lastpos = n-1;
		if(powfac < 0.0)
			exit_status = do_exagg2(phase,lastzero+1,lastpos,powfac,b);
		else
			exit_status = do_exagg2(phase,lastzero,lastpos,powfac,b);
		if(exit_status < 0)
			return(exit_status);
		while(n < thiszero) {
			while(b[n] < b[n-1]) {
				if(++n >= thiszero) {
					OK = 0;
					break;
				}
			}
			if(OK) {
				thismin = (double)b[n-1];
				minpos  = n-1;
				while(b[n] >= b[n-1])
					n++;
				thismax = (double)b[n-1];
				thispos = n-1;
				if((exit_status = do_exagg(lastmax,lastpos,thismax,thispos,powfac,b))<0)
					return(exit_status);
				lastmax = thismax;
				lastpos = thispos;
			} else
				break;
		}
		if(powfac < 0.0)
			exit_status = do_exagg2(phase,lastpos+1,n-1,powfac,b);
		else
			exit_status = do_exagg2(phase,lastpos,n-1,powfac,b);
		if(exit_status < 0)
			return(exit_status);
		break;
	case(-1):
		while(b[n] <= b[n-1])
			n++;
		lastmax = (double)b[n-1];
		lastpos = n-1;
		if(powfac < 0.0)
			exit_status = do_exagg2(phase,lastzero+1,lastpos,powfac,b);
		else
			exit_status = do_exagg2(phase,lastzero,lastpos,powfac,b);
		if(exit_status < 0)
			return(exit_status);
		while(n < thiszero) {
			while(b[n] > b[n-1]) {
				if(++n >= thiszero) {
					OK = 0;
					break;
				}
			}
			if(OK) {
				thismin = (double)b[n-1];
				minpos  = n-1;
				while(b[n] <= b[n-1])
					n++;
				thismax = (double)b[n-1];
				thispos = n-1;
				if((exit_status = do_exagg(lastmax,lastpos,thismax,thispos,powfac,b))<0)
					return(exit_status);
				lastmax = thismax;
				lastpos = thispos;
			} else
				break;
		}
		if(powfac < 0.0)
			exit_status = do_exagg2(phase,lastpos+1,n-1,powfac,b);
		else
			exit_status = do_exagg2(phase,lastpos,n-1,powfac,b);
		if(exit_status < 0)
			return(exit_status);
		break;
	}
	return(FINISHED);
}

/****************************** DO_EXAGG ******************************/

int do_exagg
(double lastmax,int lastpos,double thismax,int thispos,double powfac,float *b)
{
	double maxdif = thismax - lastmax;
	double posdif = thispos - lastpos;
	double local_max, rati_o;
	int n, m;
	for(n = lastpos+1, m = 1; n<thispos; n++, m++) {
		local_max = lastmax + (maxdif * (double)m/posdif);
		rati_o    = min(((double)b[n]/local_max),1.0);
		rati_o    = pow(rati_o,powfac);
		b[n] 	  = (float)/*round*/((double)b[n] * rati_o);
	}
	return(FINISHED);
}

/*************************** DO_EXAGG2 *******************************/

int do_exagg2(int phase,int lastpos,int thispos,double powfac, float *b)
{
	int n;
	double val, valdiff = (double)max(fabs(b[lastpos]),fabs(b[thispos]));
	for(n = lastpos; n<=thispos; n++) {
		val  = (double)b[n]/valdiff;
		val *= (float)phase;
		val  = pow(val,powfac);
		val *= valdiff * (float)phase;
		b[n] = (float)/*round*/(val);
	}
	return(FINISHED);
}

/************************ END_EXAGG *******************************/

int end_exagg(int tthis,int lastzero,int newbufcnt,int oldbufcnt,double powfac,int phase,dataptr dz)
{   
	float *bold  = dz->sampbuf[tthis] + lastzero;
    float *bnew  = dz->sampbuf[!tthis];
    float *btemp = dz->sampbuf[2];
    memmove((char *)btemp,(char *)bold,oldbufcnt * sizeof(float));
    btemp += oldbufcnt;				
    memmove((char *)btemp,(char *)bnew,newbufcnt * sizeof(float));
    exaggerate(0,oldbufcnt+newbufcnt,phase,dz->sampbuf[2],powfac);

    btemp = dz->sampbuf[2];			/* copy the transformed material back to where it came from */
    memmove((char *)bold,(char *)btemp,oldbufcnt * sizeof(float));
    btemp += oldbufcnt;
    memmove((char *)bnew,(char *)btemp,newbufcnt * sizeof(float));
	return(FINISHED);
}

/************************ GET_POWFAC *******************************/

int get_powfac(int lastzero,int thiszero,dataptr dz)
{
	double thistime;
	thistime  = (double)(dz->total_samps_read - dz->ssampsread - dz->buflen); 	/* START OF BUF IN USE */
	thistime += ((double)(thiszero + lastzero)/2.0);  			/* MEAN OFFSET OF CYCLE IN CURRENT BUF */
	thistime /= (double)dz->infile->srate;
	return read_value_from_brktable(thistime,DISTORT_POWFAC,dz);
}

//TW UPDATE : NEW FUNCTION
/************************ DISTORT_OVERLOAD *******************************/

// NOV 2002: NEED TO FIX buffers to float, MAXSAMP to F_MAXSAP, (short) to THINGY!!
int distort_overload(dataptr dz)
{
	int exit_status, n;
//	short *buf;
	float *buf;
	double *tab = NULL;
	double maxgate, mingate, inc_ratio, time = 0.0, timestep = 0.01, tabposd = 0.0, normaliser, k;
	int stepper, tabpos = 0, in_maxgate = 0, in_mingate = 0;

	buf = dz->sampbuf[0];
	if(dz->mode==OVER_SINE)
		tab = dz->parray[0];
	maxgate = dz->param[DISTORTER_MULT];
	mingate = -maxgate;
	inc_ratio = (double)DISTORTER_TABLEN/(double)dz->infile->srate;
	stepper = (int)round((double)dz->infile->srate * timestep);
	normaliser = ((double)F_MAXSAMP/(maxgate + 1.0)) /* TEST * 0.9 */;

//	while(dz->bytes_left > 0) {
	while(dz->samps_left > 0) {
//		if((exit_status = read_bytes((char *)dz->bigbuf,dz))<0)
		if((exit_status = read_samps(dz->bigbuf,dz))<0)
			return(exit_status);
		switch(dz->mode) {
		case(OVER_NOISE):
			for(n=0;n<dz->ssampsread;n++) {
				if(!(n%stepper)) {
					if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
						return(exit_status);
					time += timestep;
				}
				if((double)buf[n] >= maxgate) {
					k = 1.0 - (drand48() * dz->param[DISTORTER_DEPTH]);
//					buf[n] = (short)round(k * maxgate);
					buf[n] = (float)(k * maxgate);
				} else if((double)buf[n] <= mingate) {
					k = 1.0 - (drand48() * dz->param[DISTORTER_DEPTH]);
//					buf[n] = (short)round(k * mingate);
					buf[n] = (float)(k * mingate);
				}
//				buf[n] = (short)round(buf[n] * normaliser);	
				buf[n] = (float)(buf[n] * normaliser);	
			}
			break;
		case(OVER_SINE):
			for(n=0;n<dz->ssampsread;n++) {
				if(!(n%stepper)) {
					if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
						return(exit_status);
					time += timestep;
				}
				if((double)buf[n] >= maxgate) {
					if(!in_maxgate) {
						tabpos = 0;
						tabposd = 0.0;
						in_mingate = 0;
						in_maxgate = 1;
					}
					k = 1.0 + (tab[tabpos] * dz->param[DISTORTER_DEPTH]);
//					buf[n] = (short)round(k * maxgate); 
					buf[n] = (float)(k * maxgate); 
				} else if((double)buf[n] <= mingate) {
					if(!in_mingate)	{
						tabpos = 0;
						tabposd = 0.0;
						in_maxgate = 0;
						in_mingate = 1;
					}
					k = 1.0 + (tab[tabpos] * dz->param[DISTORTER_DEPTH]);
//					buf[n] = (short)round(k * mingate);
					buf[n] = (float)(k * mingate);
				} else {
					in_maxgate = 0;
					in_mingate = 0;
				}
//				buf[n] = (short)round(buf[n] * normaliser);
				buf[n] = (float)(buf[n] * normaliser);
				tabposd = fmod(tabposd + (dz->param[DISTORTER_FRQ] * inc_ratio),(double)DISTORTER_TABLEN);
				tabpos = round(tabposd) % DISTORTER_TABLEN;
			}
			break;
		}
		if(dz->ssampsread > 0) {
			if((exit_status = write_samps(dz->bigbuf,dz->ssampsread,dz))<0)
				return(exit_status);
		}
	}
	return(FINISHED);
}
