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



/* floatsam version */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <structures.h>
#include <tkglobals.h>
#include <globcon.h>
#include <processno.h>
#include <modeno.h>
#include <arrays.h>
#include <distort.h>
#include <cdpmain.h>

#include <sfsys.h>
#include <osbind.h>


static int 		do_multiply(int endend,int phaseswitch,int lastzero,int *output_phase,int current_buf,
				    int *endsample,dataptr dz);
static int 		do_cros_multiply(int endend,int phaseswitch,int lastzero,int *output_phase,int current_buf,
						 int *endsample,dataptr dz);
static int		do_the_division(int i,int *output_phase,int *startindex,int *startmarker,int *endindex,int lastzero,
						int lastmarker,int *no_of_half_cycles,int current_buf,int *endsample,dataptr dz);
static int		do_divide(float *ob1,float *ob2,int input_phase,int endend,double ratio,int *output_phase,
					int startindex,int endindex,int current_buf,int *endsample,dataptr dz);
static int		do_idivide(float *ob1,float *ob2,int input_phase,int endend,double ratio,int *output_phase,
			   int startindex,int endindex,int current_buf,int *endsample,dataptr dz);
static int		too_big(int startindex,int endindex,dataptr dz);
static int		get_iphase(int startmarker,int startindex,int current_buf,dataptr dz);
static float	interpval2(int m,int current_buf,double ratio,int startindex,dataptr dz);
static float	interpval(int m,int current_buf,double ratio,int startindex,dataptr dz);
static float	getval2(int m,int current_buf,double ratio,int startindex,dataptr dz);
static float	getval(int m,int current_buf,double ratio,int startindex,dataptr dz);
static double 	set_ratio(int endend,int startindex,int endindex,dataptr dz);
static int 		do_md(int crosbuf,int current_buf,int i,int lastzero,int lastmarker,int *output_phase,
				int *no_of_half_cycles,int *endsample,int *startindex,int *startmarker,
				int *endindex,dataptr dz);

/************************* MDISTORT ***********************************
 *
 * Works on SINGLE HALF wavecycles.
 *
 * This looks for single half-wavecycles, and does not change there
 * number (or their phase) in the output. It therefore resets the
 * input & output phase on each entry.
 */

int mdistort(int is_last,int *lastzero,int *endsample,int *output_phase,int current_buf,
float *cyclemax,int *no_of_half_cycles,int *startindex,int *startmarker,int *endindex,dataptr dz)
{
	int exit_status;
	int crosbuf = FALSE;
	register int i = *lastzero;
	int samples, lastmarker, oldbufcnt;
	int phase;
	float *b = dz->sampbuf[current_buf];
	if(is_last)
		samples  = dz->ssampsread;
	else
		samples = dz->buflen;
//TW CHANGED
	while(smpflteq(b[i],0.0) && i<samples)
		i++;	
	lastmarker = i;
	if(b[i]<0)
		phase  = -1;
	else
		phase  = 1;
	while(i < samples)  {
		switch(phase) {
		case(1):	    
			if(b[i] > 0) {
				if(b[i]>*cyclemax)
					*cyclemax=b[i];
				i++;
			} else {
				if((exit_status = do_md(crosbuf,current_buf,i,*lastzero,lastmarker,output_phase,
								  		no_of_half_cycles,endsample,startindex,startmarker,endindex,dz))<0)
					return(exit_status);
			 	i = lastmarker = reset(i,samples,b,lastzero,cyclemax);
				if(b[i]<0)
					phase = -1;
			}
			break;
		case(-1):
			if(b[i] < 0.0) {
				if(b[i]<*cyclemax)
					*cyclemax=b[i];
				i++;
			} else {
				if((exit_status = do_md(crosbuf,current_buf,i,*lastzero,lastmarker,output_phase,
										no_of_half_cycles,endsample,startindex,startmarker,endindex,dz))<0)
					return(exit_status);
				i = lastmarker = reset(i,samples,b,lastzero,cyclemax);
				if(b[i]>0.0)
					phase = 1;
			}
			break;
		}
	}
	if(!is_last) {
		crosbuf = TRUE;
		oldbufcnt = dz->buflen - *lastzero;
		b = dz->sampbuf[!current_buf];
		samples = dz->ssampsread;
		i = 0;
		exit_status = CONTINUE;	

		while(exit_status == CONTINUE && i < samples)  {
			switch(phase) {
			case(1):	    
				if(b[i] > 0.0) {
					if(b[i] > *cyclemax)
						*cyclemax=b[i];
					i++;
				} else {
					if((exit_status = do_md(crosbuf,current_buf,i,*lastzero,lastmarker,output_phase,
										no_of_half_cycles,endsample,startindex,startmarker,endindex,dz))<0)
						return(exit_status);
			 		i = lastmarker = reset(i,samples,b,lastzero,cyclemax);
					if(b[i]<0)
						phase = -1;
				}
				break;
			case(-1):
				if(b[i] < 0.0) {
					if(b[i]<*cyclemax)
						*cyclemax=b[i];
					i++;
				} else {
					if((exit_status = do_md(crosbuf,current_buf,i,*lastzero,lastmarker,output_phase,
										no_of_half_cycles,endsample,startindex,startmarker,endindex,dz))<0)
						return(exit_status);
					i = lastmarker = reset(i,samples,b,lastzero,cyclemax);
					if(b[i]>0.0)
						phase = 1;
				}
				break;
			}
		}
#ifdef NOTDEF
		/* removed TW 5:2002 */
		if(i >= samples) {
			sprintf(errstr,"wavecycle-group too large for buffer.\n");
			return(GOAL_FAILED);
		}
#endif
	}
	return(FINISHED);
}

/*************************** DO_MULTIPLY ******************************
 *
 * (1)	Set up pointers to input and output buffers.
 * (4)	Length of the complete half_cycle established.
 * (5)  "mid_cyclelen" is length of first half-cycle in transformed sound.
 * (6)  "remnant" is the number of samples left over when the cycling factor
 *	is divided into the true src half_cyclelen.
 * (7)  "mid_cycle" is buffer index of first half-cycle in transformed sound.
 * (8)	With no phase-inversion, 1st OUTPUT half_cycle established by indexing
 *	complete input half_cycle.
 * (9)	With phase-inversion, ditto + phase-inversion.
 * (10)	If there are remnant samples, interpolating sample pads are added
 *	between the output half-cycles, until no remant samples remain.
 * (1))	For all the remaining output halfcycles, establish start at end of
 *	previous halfcycle, and end at mid_cycle_len beyond this.
 * (12)	If it's on odd half-cycle, copy first half-cycle here, with
 *	phase-inversion.
 * (13)	But if it's an even half-cycle, copy ditto WITHOUT phase inversion.
 * (14)	If factor is odd, startphase of transformed segements switch from
 *	positive to negative and vice versa.
 */

int do_multiply(int endend,int phaseswitch,int lastzero,int *output_phase,int current_buf,int *endsample,dataptr dz)
{
	int  index;
	register int n, m, k;
	float *b1  = dz->sampbuf[current_buf];					/* 1 */
	float *ob1 = dz->sampbuf[current_buf+2];
	int  start, half_cyclelen, mid_cyclelen, remnant, mid_cycle;
	half_cyclelen = endend -  lastzero;				/* 4 */
	mid_cyclelen  = half_cyclelen/dz->iparam[DISTORTM_FACTOR];			/* 5 */
	if(!mid_cyclelen) {
		switch(phaseswitch) {
		case(1):
			for(n = lastzero; n < endend; n++)	/* i.e. just copy */
				ob1[n] = b1[n];
			break;
		case(-1):
			for(n = lastzero; n < endend; n++)
				ob1[n] = -b1[n];
			break;
		}
		return(FINISHED);
	}
	remnant	  = half_cyclelen - (mid_cyclelen * dz->iparam[DISTORTM_FACTOR]);	/* 6 */
	mid_cycle     = lastzero + mid_cyclelen;			/* 7 */
	switch(phaseswitch) {
	case(1):
		for(n = lastzero,m=0; n < mid_cycle; n++,m++) {
			index = round((double)m * (double)dz->iparam[DISTORTM_FACTOR]);		/* 8 */
			index += lastzero;
			ob1[n] = b1[index];
		}
		break;
	case(-1):
		for(n = lastzero,m=0; n < mid_cycle; n++,m++) {
			index = round((double)m * (double)dz->iparam[DISTORTM_FACTOR]);		/* 9 */
			index += lastzero;
			ob1[n] = -b1[index];
		}
		break;
	}
	if(remnant) {
		ob1[mid_cycle] = ob1[mid_cycle-1]/2;				/* 10 */
		mid_cycle++;
		remnant--;
	}
	for(k=1; k<dz->iparam[DISTORTM_FACTOR]; k++) {
		start      = mid_cycle;					/* 11 */
		mid_cycle += mid_cyclelen;
		if(ODD(k)) {						/* 12 */
			for(n = start, m = lastzero; n < mid_cycle; n++, m++)
				ob1[n] = -ob1[m];
		} else {						/* 13 */
			for(n = start, m = lastzero; n < mid_cycle; n++, m++)
				ob1[n] = ob1[m];
		}
		if(remnant) {
			ob1[mid_cycle] = ob1[n-1] /*/2 */ * 0.5f;
			mid_cycle++;					/* 10 */
			remnant--;
		}
	}
	if(dz->vflag[DISTORTM_SMOOTH]) {		/* TW MAR:1995 Smoothing at end of cycle */
		while(mid_cycle < endend) {
			ob1[mid_cycle] = ob1[mid_cycle-1] /*/2 */ * 0.5f;
			mid_cycle++;		
		}
	}
	if(ODD(dz->iparam[DISTORTM_FACTOR]))						/* 14 */
		*output_phase = -(*output_phase);
	*endsample = endend;
	return(FINISHED);
}

/*************************** DO_CROS_MULTIPLY ******************************/

int do_cros_multiply
(int endend,int phaseswitch,int lastzero,int *output_phase,int current_buf,int *endsample,dataptr dz)
{
	/*short*/int  index;			  /* RWD 4:2002 why a short ? */
	register int m, n, k;
	float *b1  = dz->sampbuf[current_buf];
	float *bb, *q;
	float *ob1 = dz->sampbuf[current_buf + 2];
	float *b2  = dz->sampbuf[!current_buf];
	float *ob2 = dz->sampbuf[(!current_buf) + 2];
	int  start, half_cyclelen, mid_cyclelen, remnant;
	int  mid_cycle, oldbufcnt;
	int   is_2nd_buf = 0;
	half_cyclelen = endend + dz->buflen -  lastzero;
	mid_cyclelen  = half_cyclelen/dz->iparam[DISTORTM_FACTOR];
	remnant	  	  = half_cyclelen - (mid_cyclelen * dz->iparam[DISTORTM_FACTOR]);
	mid_cycle     = lastzero + mid_cyclelen;
	if(!mid_cyclelen) {		/* i.e. IF WERE TRYING TO CREATE MORE CYCLES THAN THERE ARE SAMPLES */
		switch(phaseswitch) {
		case(1):
			for(n = lastzero; n < dz->buflen; n++)  	/* i.e. just copy */
				ob1[n] = b1[n];
			for(n = 0; n < endend; n++)
				ob2[n] = b2[n];
			break;
		case(-1):
			for(n = lastzero; n < dz->buflen; n++)  	/* i.e. just copy */
				ob1[n] = -b1[n];
			for(n = 0; n < endend; n++)
				ob2[n] = -b2[n];
			break;
		}
		return(FINISHED);
	}
/* OTHERWISE: USE dz->sampbuf[4] and [5] AS TEMPORARY STORAGE LOCATIONS */
	oldbufcnt = dz->buflen - lastzero;
	bb 		  = dz->sampbuf[4]; 	/* copy start of complete halfcycle to dz->sampbuf[4] */
	q  		  = b1 + lastzero;
	memmove(bb,q,oldbufcnt * sizeof(float));
	bb += oldbufcnt;	/* copy rest of complete halfcycle to end of that */
	q  = b2;
	memmove(bb,q,endend * sizeof(float));
	for(n=0; n < mid_cyclelen; n++) {
		index = round((double)n * (double)dz->iparam[DISTORTM_FACTOR]);
		dz->sampbuf[5][n] = dz->sampbuf[4][index];/* convert halfcycle to compressed form */
	}
	start = lastzero;
	if(mid_cycle > dz->buflen)
		is_2nd_buf = 1;
	if(!is_2nd_buf) {    	/* GOAL HALFCYCLE LIES IN FIRST BUFFER */
		switch(phaseswitch) {
		case(1):
			for(n=start,m=0; n < mid_cycle; n++,m++)
				ob1[n] = dz->sampbuf[5][m];
			break;
		case(-1):
			for(n=start,m=0; n < mid_cycle; n++,m++)
				ob1[n] = -dz->sampbuf[5][m];
			break;
		}
		if(remnant) {
			if(mid_cycle < dz->buflen) {
				ob1[mid_cycle] = ob1[n-1]/2;
				if(++mid_cycle >= dz->buflen) {
					mid_cycle = 0;
					is_2nd_buf = 1;
				}
			} else {
				ob2[0] = ob1[dz->buflen-1]/2;
				mid_cycle = 1;
				is_2nd_buf = 1;
			}
			remnant--;
		}
	} else {		/* GOAL HALFCYCLE STRADDLES THE 2 BUFFERS */
		switch(phaseswitch) {
		case(1):
			for(n=start,m=0; n < dz->buflen; n++,m++)
				ob1[n] = dz->sampbuf[5][m];
			mid_cycle -= dz->buflen;
			for(n=0; n < mid_cycle; n++, m++)
				ob2[n] = dz->sampbuf[5][m];
			break;
		case(-1):
			for(n=start,m=0; n < dz->buflen; n++,m++)
				ob1[n] = -dz->sampbuf[5][m];
			mid_cycle -= dz->buflen;
			for(n=0; n < mid_cycle; n++, m++)
				ob2[n] = -dz->sampbuf[5][m];
			break;
		}
	}
	phaseswitch = -phaseswitch;
	for(k=1; k<dz->iparam[DISTORTM_FACTOR]; k++) {	/* FOR ALL THE OTHER HALF-CYCLES */
		if(!is_2nd_buf) {
			start      = mid_cycle;
			mid_cycle += mid_cyclelen;
			if(mid_cycle < dz->buflen) {
				if(phaseswitch==1) {	 /* GOAL HALFCYCLE LIES IN 1ST BUFFER */
					for(n = start, m = 0; n < mid_cycle; n++, m++)
						ob1[n] = dz->sampbuf[5][m];
				} else {
					for(n = start, m = 0; n < mid_cycle; n++, m++)
						ob1[n] = -dz->sampbuf[5][m];
				}
				if(remnant) {
					ob1[mid_cycle] = ob1[n-1] /*/2*/ * 0.5f;
					remnant--;
					if(++mid_cycle>=dz->buflen) {
						mid_cycle = 0;
						is_2nd_buf = 1;
					}
				}
			} else {	/* GOAL HALFCYCLE STRADDLES 2 BUFFERS */
				mid_cycle -= dz->buflen;
				if(phaseswitch==1) {
					for(n = start, m = 0; n < dz->buflen; n++, m++)
						ob1[n] = dz->sampbuf[5][m];
					for(n = 0; n < mid_cycle; n++, m++)
						ob2[n] = dz->sampbuf[5][m];
				} else {
					for(n = start, m = 0; n < dz->buflen; n++, m++)
						ob1[n] = -dz->sampbuf[5][m];
					for(n = 0; n < mid_cycle; n++, m++)
						ob2[n] = -dz->sampbuf[5][m];
				}
				if(remnant) {
					if(n>0)
						ob2[mid_cycle] = ob2[n-1] /*/2 */ * 0.5f;
					else
						ob2[mid_cycle] = ob1[dz->buflen-1] /* /2*/ * 0.5f;
					remnant--;
					mid_cycle++;
				}
				is_2nd_buf = 1;
			}
		} else {	/* GOAL HALFCYCLE IS ENTIRELY IN 2nd BUFFER */
			start  = mid_cycle;
			mid_cycle += mid_cyclelen;
			if(phaseswitch==1) {
				for(n = start, m = 0; n < mid_cycle; n++, m++)
					ob2[n] = dz->sampbuf[5][m];
			} else {
				for(n = start, m = 0; n < mid_cycle; n++, m++)
					ob2[n] = -dz->sampbuf[5][m];
			}
			if(remnant) {
				ob2[mid_cycle] = ob2[n-1] /* /2 */ * 0.5f;
				remnant--;
				mid_cycle++;
			}
		}
		phaseswitch = -phaseswitch;
	}
	if(dz->vflag[DISTORTM_SMOOTH]) {	/* TW MAR:1995 Smoothing at end of cycle */
		if(mid_cycle!=endend) {	
			if(mid_cycle > endend) {
				while(mid_cycle < dz->buflen) {
					ob1[mid_cycle] = ob1[mid_cycle-1]/* /2 */ * 0.5f;
					mid_cycle++;		
				}
				mid_cycle = 0;
				if(endend>0)
					ob2[mid_cycle++] =  ob1[dz->buflen-1]/* /2 */ * 0.5f;
			} else {
				if(mid_cycle==0)
					ob2[mid_cycle++] = ob1[dz->buflen-1]/* /2 */ * 0.5f;
			}
			while(mid_cycle < endend) {
				ob2[mid_cycle] = ob2[mid_cycle-1]/* /2 */ * 0.5f;
				mid_cycle++;
			}
		}
	}
	if(ODD(dz->iparam[DISTORTM_FACTOR]))		/* CHANGE OUTPUT PHASE FOR NEXT PASS, IF FACTOR IS ODD */
		*output_phase = -(*output_phase);
	*endsample = endend;
	return(FINISHED);
}

/************************* SET_PHASESWITCH ****************************
 *
 * (1)	lastmarker is AFTER lastzero, So if it's bufptr val is LESS
 *	it must be in the other buffer.
 * (2)	If the factor is ODD, the start-phase of the waveform switches
 *	every cycle. In this case, if input phase and output start-phase are
 *	same, no phase-inversion necessary (phaseswitch = 1). Otherwise
 *	phase of input signal must be inverted (phaseswitch = -1).
 *	The logic of the code is equivalent to this (though it takes
 *	a moment to deduce this!!
 * (3)  If the factor is even, output start-phase is always positive.
 *	Hence input phase must be inverted (phaseswitch = -1) only if it is
 *	-ve.
 */

int set_phaseswitch(int output_phase,int lastzero,int lastmarker,int current_buf,dataptr dz)
{
	float *bbuf = dz->sampbuf[current_buf];
	if(lastmarker < lastzero)				/* 1 */
		bbuf = dz->sampbuf[!current_buf];
	if(ODD(dz->iparam[DISTORTM_FACTOR])) {	/* 2 */
		if(bbuf[lastmarker] > 0.0)
			return(output_phase);
		else    
			return(-output_phase);
	} else {								/* 3 */
		if(bbuf[lastmarker] > 0.0)
			return(1);
	}	/* ELSE */
	return(-1);
}

/************************* DO_THE_DIVISION ****************************/

int do_the_division(int i,int *output_phase,int *startindex,int *startmarker,int *endindex,int lastzero,
					int lastmarker,int *no_of_half_cycles,int current_buf,int *endsample,dataptr dz)
{
	int exit_status;
	double ratio;	
	float  *ob1,*ob2;
	int    input_phase;
	(*no_of_half_cycles)++;
	if(*no_of_half_cycles == 1) {						 
		*startindex  = lastzero;
		*startmarker = lastmarker;
		*endindex = i;
	} else {
		if(*no_of_half_cycles >= dz->iparam[DISTORTM_FACTOR]) {
			if(too_big(*startindex,*endindex,dz)) {
				sprintf(errstr,"Wavecycle length exceeds buffer length at %lf\n",
				(double)(dz->total_samps_written + lastzero)/(double)dz->infile->srate);
				return(GOAL_FAILED);
			}
			ratio = set_ratio(i,*startindex,*endindex,dz);
			ob1 = dz->sampbuf[current_buf + 2];
			ob2 = dz->sampbuf[(!current_buf) + 2];
			input_phase = get_iphase(*startmarker,*startindex,current_buf,dz) * (*output_phase);
			if(dz->vflag[DISTORTD_INTERP])
				exit_status = do_idivide(ob1,ob2,input_phase,i,ratio,output_phase,*startindex,*endindex,current_buf,endsample,dz);
			else
				exit_status = do_divide(ob1,ob2,input_phase,i,ratio,output_phase,*startindex,*endindex,current_buf,endsample,dz);
			if(exit_status < 0)
				return(exit_status);
			*no_of_half_cycles = 0;
			return(FINISHED);
		}
	}
	return(CONTINUE);
}

/************************** SET_RATIO ***************************/

double set_ratio(int endend,int startindex,int endindex,dataptr dz)
{
	double ratio;
	if(startindex < endindex) { 
		if(endindex < endend)    /* SRC CYC & ALL GOAL CYCS IN SAME BUFFER  */
			ratio =(double)(endindex - startindex)/(double)(endend - startindex);
		else   		 	/* SRC CYC IN 1 BUF, GOAL CYC CROSSES BUFS */
			ratio = (double)(endindex - startindex)/(double)(endend + dz->buflen - startindex);
	} else {		 	 /* SRC CYC & ALL GOAL CYCS, CROSS BUFFERS  */
		ratio = (double)(endindex + dz->buflen - startindex)/(double)(endend + dz->buflen - startindex);
	}
	return(ratio);
}

/*************************** DO_DIVIDE ******************************/

int do_divide(float *ob1,float *ob2,int input_phase,int endend,double ratio,int *output_phase,
			   int startindex,int endindex,int current_buf,int *endsample,dataptr dz)
{
	register int n, m;
	switch(input_phase) {
	case(1):			/* SRC CYCLE AND ENTIRE SET OF GOAL CYCLES IN SAME BUFFER */
		if(startindex < endindex) {
			if(endindex < endend) {
				for(n=startindex,m=0; n <endend; n++,m++)
					ob1[n] = getval(m,current_buf,ratio,startindex,dz);
			} else {	/* SRC CYCLE IN ONE BUFFER, GOAL CYCLE CROSSES BUFFER BOUNDARY */
				for(n=startindex,m=0; n <dz->buflen; n++,m++)
					ob1[n] = getval(m,current_buf,ratio,startindex,dz);
				for(n=0; n <endend; n++,m++)
					ob2[n] = getval(m,current_buf,ratio,startindex,dz);
			}
		} else {		/* SRC CYCLE AND SET OF GOAL CYCLES, BOTH CROSS BUFFER BOUNDARY */
			for(n=startindex,m=0; n <dz->buflen; n++,m++)
				ob1[n] = getval(m,current_buf,ratio,startindex,dz);
			for(n=0; n <endend; n++,m++) 
				ob2[n] = getval2(m,current_buf,ratio,startindex,dz);
		}
		break;
	case(-1):
		if(startindex < endindex) {
			if(endindex < endend) { 	/* SRC CYC & ALL GOAL CYCS IN 1 BUFF */
				for(n=startindex,m=0; n <endend; n++,m++)
					ob1[n] = -getval(m,current_buf,ratio,startindex,dz);
			} else {		/* SRC CYC IN 1 BUF, GOAL SET CROSSES BUFFS */
				for(n=startindex,m=0; n <dz->buflen; n++,m++)
					ob1[n] = -getval(m,current_buf,ratio,startindex,dz);
				for(n=0; n <endend; n++,m++)
					ob2[n] = -getval(m,current_buf,ratio,startindex,dz);
			}
		} else {	   		/* SRC CYC & GOAL SET, CROSS BUFFS */
			for(n=startindex,m=0; n <dz->buflen; n++,m++)
				ob1[n] = -getval(m,current_buf,ratio,startindex,dz);
			for(n=0; n <endend; n++,m++)
				ob2[n] = -getval2(m,current_buf,ratio,startindex,dz);
		}
		break;
	}
	*output_phase = -(*output_phase);
	*endsample = endend;
	return(FINISHED);
}

/*************************** DO_IDIVIDE ******************************/

int do_idivide(float *ob1,float *ob2,int input_phase,int endend,double ratio,int *output_phase,
			   int startindex,int endindex,int current_buf,int *endsample,dataptr dz)
{
	register int n = 0, m;
	switch(input_phase) {
	case(1):
		if(startindex < endindex) {
			if(endindex < endend) { /* SRC CYC & ALL GOAL CYCS IN SAME BUFF */
				for(n=startindex,m=0; n <endend; n++,m++)
					ob1[n] = interpval(m,current_buf,ratio,startindex,dz);
			} else {		    /* SRC CYC IN 1 BUF, GOAL CYC CROSSES BUF */
				for(n=startindex,m=0; n <dz->buflen; n++,m++)
					ob1[n] = interpval(m,current_buf,ratio,startindex,dz);
				for(n=0; n <endend; n++,m++)
					ob2[n] = interpval(m,current_buf,ratio,startindex,dz);
			}
		} else {		    /* SRC CYC & GOAL SET, CROSS BUFFERS  */
			for(n=startindex,m=0; n <dz->buflen; n++,m++)
				ob1[n] = interpval(m,current_buf,ratio,startindex,dz);
			for(n=0; n <endend; n++,m++)
				ob2[n] = interpval2(m,current_buf,ratio,startindex,dz);
		}
		break;
	case(-1):
		if(startindex < endindex) {
			if(endindex < endend) {  /* SRC CYC & ALL GOAL CYCS IN SAME BUFF */
				for(n=startindex,m=0; n <endend; n++,m++)
					ob1[n] = -interpval(m,current_buf,ratio,startindex,dz);
			} else {		   		 /* SRC CYC IN 1 BUF, GOAL SET CROSSES BUF */
				for(n=startindex,m=0; n <dz->buflen; n++,m++)
					ob1[n] = -interpval(m,current_buf,ratio,startindex,dz);
				for(n=0; n <endend; n++,m++)
					ob2[n] = -interpval(m,current_buf,ratio,startindex,dz);
			}
		} else {		    		/* SRC CYC & GOAL SET, CROSS BUFFS  */
			for(n=startindex,m=0; n <dz->buflen; n++,m++)
				ob1[n] = -interpval(m,current_buf,ratio,startindex,dz);
			for(n=0; n <endend; n++,m++)
				ob2[n] = -interpval2(m,current_buf,ratio,startindex,dz);
		}
		break;
	}
	*output_phase = -(*output_phase);
	*endsample = n;
	return(FINISHED);
}

/************************** GETVAL ******************************/

float getval(int m,int current_buf,double ratio,int startindex,dataptr dz)
{
	float *bbuf = dz->sampbuf[current_buf];
	int index = (int)round((double)m * ratio);
	if((index += startindex)>=dz->buflen)
		index = dz->buflen-1;
	return(bbuf[index]);
}

/************************ GETVAL2 ****************************/

float getval2(int m,int current_buf,double ratio,int startindex,dataptr dz)
{
	float *bbuf;
	int index = (int)round((double)m * ratio);
	index += startindex;
	if(index >= dz->buflen) { 						/* If beyond end of thisbuf, */
		if((index -= dz->buflen)>=dz->buflen)	/* put index within range    */
			index = dz->buflen-1;			
		bbuf = dz->sampbuf[!current_buf];   	/* BUT read from other buf.  */
	} else
		bbuf   = dz->sampbuf[current_buf];
	return(bbuf[index]);
}

/***************************** INTERPVAL **********************************/

float interpval(int m,int current_buf,double ratio,int startindex,dataptr dz)
{
	float *bbuf = dz->sampbuf[current_buf];
	double findex = (double)m * ratio;
	int   index  = (int)findex; /* truncate */
	double frac   = findex - (double)index;
	float diff,bb,ba;
	index += startindex;
	ba     = bbuf[index];
	if(++index>=dz->buflen) {
		bbuf = dz->sampbuf[!current_buf];
		index = 0;
	}
	bb = bbuf[index];
	diff = (float) /*round*/((double)(bb - ba) * frac);
	return(ba + diff);
}

/****************************** INTERPVAL2 ***************************/

float interpval2(int m,int current_buf,double ratio,int startindex,dataptr dz)
{
	float *bbuf;
	float diff, ba, bb;
	double findex = (double)m * ratio;
	int   index  = (int)findex; /* truncate */
	double frac   = findex - (double)index;
	index += startindex;
	if(index >= dz->buflen) { 						/* If beyond end thisbuf   */
		if((index -= dz->buflen)>=dz->buflen)		/* put index within range  */
			index = dz->buflen-1;			
		bbuf   = dz->sampbuf[!current_buf];        /* BUT read from other buf. */
		ba     = bbuf[index];
		if(index < dz->buflen-1)
			index++;
	} else {
		bbuf = dz->sampbuf[current_buf];
		ba  = bbuf[index];
		if(++index>=dz->buflen) {	/* IF at very end of buffer */
			bbuf  = dz->sampbuf[!current_buf];
			index = 0;				/* goto firstsamp in nextbuf */
		}
	}
	bb = bbuf[index];
	diff = (float)/*round*/((double)(bb - ba) * frac);
	return(ba + diff);
}

/***************************** GET_IPHASE ************************/

int get_iphase(int startmarker,int startindex,int current_buf,dataptr dz)
{
	float *bbuf;
	if(startmarker < startindex)
		bbuf = dz->sampbuf[!current_buf];
	else
		bbuf = dz->sampbuf[current_buf];
	if(bbuf[startmarker] > 0)
		return(1);
	return(-1);
}

/*************************** TOO_BIG *******************************/

int too_big(int startindex,int endindex,dataptr dz)
{
	int wavelen;
	if((wavelen = endindex - startindex)<0)
		wavelen += dz->buflen;
	wavelen *= dz->iparam[DISTORTM_FACTOR];
	if(wavelen >= dz->buflen)
		return(1);
	return(0);
}

/*************************** DO_MD *******************************/

int do_md(int crosbuf,int current_buf,int i,int lastzero,int lastmarker,int *output_phase,
	int *no_of_half_cycles,int *endsample,int *startindex,int *startmarker,int *endindex,dataptr dz)
{
	int exit_status;
	double thistime;
	int phaseswitch;
	if(*no_of_half_cycles == 0) {
		thistime = (double)(dz->total_samps_read - dz->ssampsread + lastmarker)/(double)dz->infile->srate;
		if((exit_status = read_values_from_all_existing_brktables(thistime,dz))<0)
			return(exit_status);
	}
	switch(dz->process) {
	case(DISTORT_MLT):
		phaseswitch = set_phaseswitch(*output_phase,lastzero,lastmarker,current_buf,dz);
		if(crosbuf==TRUE)
			return do_cros_multiply(i,phaseswitch,lastzero,output_phase,current_buf,endsample,dz);
		else
			return do_multiply(i,phaseswitch,lastzero,output_phase,current_buf,endsample,dz);
	case(DISTORT_DIV):
		return do_the_division(i,output_phase,startindex,startmarker,endindex,lastzero,lastmarker,
						no_of_half_cycles,current_buf,endsample,dz);
	}
	sprintf(errstr,"Unknown case in do_md()\n");
	return(PROGRAM_ERROR);
}
