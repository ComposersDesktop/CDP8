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
#include <structures.h>
#include <tkglobals.h>
#include <globcon.h>
#include <modeno.h>
#include <arrays.h>
#include <distort.h>
#include <cdpmain.h>

#include <sfsys.h>
#include <osbind.h>

#define HCYCBUF	(2)

static int 	get_halfcycle(float **buf,int *bufpos,int *cyclelen,int *current_buf,int *is_bufcros,
			int prescale_param,int *last_total_samps,dataptr dz);
static void do_harmonic(float *buf,int endsamp,int cyclestart,int cyclelen,dataptr dz);
static void do_harmonic_crosbuf(int endsamp,int cyclestart,int cyclelen,int current_buf,dataptr dz);

/**************************** DISTORTH ******************************
 *
 * Distort file by adding harmonics over each wavecycle.
 */

int distorth(int *bufpos,int phase,int *last_total_samps,int *current_buf,dataptr dz)
{
	int exit_status;
	int cyclelen = 0;
	float *buf  = dz->sampbuf[*current_buf];
	int is_bufcros = 0;
	int cyclestart = *bufpos;
	switch(phase) {
	case(1):
		while(buf[*bufpos]>=0) {
			if((exit_status = get_halfcycle(&buf,bufpos,&cyclelen,current_buf,&is_bufcros,
				DISTORTH_PRESCALE,last_total_samps,dz))!=CONTINUE)
				return(exit_status);
		}
		while(buf[*bufpos]<=0) {
			if((exit_status = get_halfcycle(&buf,bufpos,&cyclelen,current_buf,&is_bufcros,
				DISTORTH_PRESCALE,last_total_samps,dz))!=CONTINUE)
				return(exit_status);
		}
		break;
	case(-1):
		while(buf[*bufpos]<=0) {
			if((exit_status = get_halfcycle(&buf,bufpos,&cyclelen,current_buf,&is_bufcros,
				DISTORTH_PRESCALE,last_total_samps,dz))!=CONTINUE)
				return(exit_status);
		}
		while(buf[*bufpos]>=0) {
			if((exit_status = get_halfcycle(&buf,bufpos,&cyclelen,current_buf,&is_bufcros,
				DISTORTH_PRESCALE,last_total_samps,dz))!=CONTINUE)
				return(exit_status);
		}
		break;
	}
	if(is_bufcros) {
		do_harmonic_crosbuf(*bufpos,cyclestart,cyclelen,*current_buf,dz);
		if((exit_status = write_samps(dz->sampbuf[!(*current_buf)],dz->buflen,dz))<0)
			return(exit_status);
		is_bufcros = 0;
	} else
		do_harmonic(buf,*bufpos,cyclestart,cyclelen,dz);
	return(CONTINUE);
}

/**************************** GET_HALFCYCLE **************************/

int get_halfcycle(float **buf,int *bufpos,int *cyclelen,int *current_buf,int *is_bufcros,
				  int prescale_param,int *last_total_samps,dataptr dz)
{
	int exit_status;
	dz->sampbuf[HCYCBUF][*cyclelen] = (*buf)[*bufpos];
	if(++(*cyclelen) >dz->iparam[DISTORTH_CYCBUFLEN]) {
		cop_out(*bufpos,*cyclelen,*last_total_samps,dz);
		return FINISHED;
	}
	if(++(*bufpos) >= dz->ssampsread) {
		*last_total_samps = dz->total_samps_read;
		if((exit_status = change_buf(current_buf,is_bufcros,buf,dz))!=CONTINUE)
			return(exit_status);
		if(dz->vflag[IS_PRESCALED])
			prescale(*current_buf,prescale_param,dz);
		*bufpos = 0;
	}
	return(CONTINUE);
}

/*************************** DO_HARMONIC ***************************/

void do_harmonic(float *buf,int endsamp,int cyclestart,int cyclelen,dataptr dz)
{
//TW UPDATE
//	int n, foldover = 0;
	int n;
//TW REVISION (avoid warnings)
	double val;
	register int k, i;
	for(n=0;n<dz->iparam[DISTORTH_HCNT];n++) {
		k = cyclestart;
		i = dz->iparray[DISTORTH_HNO][n];
//TW UPDATE
		if(i >= cyclelen/4)	/* Foldover */
			continue;
		while(k < endsamp) { 
			/*RWD added cast: but always get the warning with +=... */
//			buf[k++] += (float) /*round*/(dz->sampbuf[HCYCBUF][i] * dz->parray[DISTORTH_AMP][n]);
//TW AVOID WARNING
			val =  buf[k] + (dz->sampbuf[HCYCBUF][i] * dz->parray[DISTORTH_AMP][n]);
			buf[k++] = (float)val;
			i += dz->iparray[DISTORTH_HNO][n];
//TW UPDATE
			while(i >= cyclelen) 
				i -= cyclelen;
//TW UPDATE
		}
	}
}

/************************* DO_HARMONIC_CROSBUF *************************/

void do_harmonic_crosbuf(int endsamp,int cyclestart,int cyclelen,int current_buf,dataptr dz)
{
	float *buf;
//TW UPDATE
//	int n, foldover = 0;
	int n;
//TW ADDITION (avoid warnings)
	double val;
	register int k, i;
	for(n=0;n<dz->iparam[DISTORTH_HCNT];n++) {
		k = cyclestart;
		i = dz->iparray[DISTORTH_HNO][n];
//TW UPDATE
		if(i >= cyclelen/4)	/* Foldover */
			continue;
		buf = dz->sampbuf[!current_buf];
		while(k < dz->buflen) {
			/*RWD added cast */
//			buf[k++] += (float) /*round*/(dz->sampbuf[HCYCBUF][i] * dz->parray[DISTORTH_AMP][n]);
//TW REVISION (avoid warnings)
			val = buf[k] + (dz->sampbuf[HCYCBUF][i] * dz->parray[DISTORTH_AMP][n]);
			buf[k++] = (float)val;
			i += dz->iparray[DISTORTH_HNO][n];
//TW UPDATE
//			while(i >= cyclelen) {
			while(i >= cyclelen)
				i -= cyclelen;
//				if(dz->iparam[FOLDOVER_WARNING]==FALSE && foldover==1) {
//					fprintf(stdout,"\nWARNING: FOLDOVER\n");
//					fflush(stdout);
//					dz->iparam[FOLDOVER_WARNING] = TRUE;				
//				}
//				foldover++;
//			}
//			foldover = 0;
		}
		buf = dz->sampbuf[current_buf];
		k = 0;
		while(k < endsamp) {
			/*RWD added cast */
//			buf[k++] += (float)/* round*/(dz->sampbuf[HCYCBUF][i] * dz->parray[DISTORTH_AMP][n]);
//TW REVISION (avoid warnings)
			val = buf[k] + (dz->sampbuf[HCYCBUF][i] * dz->parray[DISTORTH_AMP][n]);
			buf[k++] = (float)val;
			i += dz->iparray[DISTORTH_HNO][n];
//TW UPDATE
//			while(i >= cyclelen) {
			while(i >= cyclelen)
				i -= cyclelen;
//				if(dz->iparam[FOLDOVER_WARNING]==FALSE && foldover==1) {
//					fprintf(stdout,"WARNING : FOLDOVER\n");
//					fflush(stdout);
//					dz->iparam[FOLDOVER_WARNING] = TRUE;				
//				}
//				foldover++;
//			}
//			foldover = 0;
		}
	}
}
