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



/* floatsam vesion */
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

#ifdef unix
#define round(x) lround((x))
#endif

static int get_distort(int oldlen,int *pos_in_cycle_group,double *last_transpos,double *next_transpos,
		double *transpos_step,int *thiscyclelen,int *init,dataptr dz);
static int do_distrt(int oldlen,int *pos_in_cycle_group,double *last_transpos,double *next_transpos,
		double *transpos_step,int *thiscyclelen,int *init,int *obufremain,int current_inbuf_pos, dataptr dz);
static int move_to_outbut(int n,int *obufremain,dataptr dz);

/************************* DISTORT_PITCH ***********************************
 *
 * Works on SINGLE HALF wavecycles.
 *
 * This looks for single half-wavecycles, and does not change there
 * number (or their phase) in the output. It therefore resets the
 * input & output phase on each entry.
 */

int distort_pitch(dataptr dz)
{
	int exit_status;
	int pos_in_cycle_group = 0, thiscyclelen = 0, init = 1;
	double last_transpos = 0.0, next_transpos = 0.0, transpos_step = 0.0;
	int obufremain = dz->buflen;
	int current_buf = 0;
	int current_inbuf_pos = 0;
	register int j = 1; /* for gardpnt */
	int phase, cnt = 0;
	float *inbuf      = dz->sampbuf[0];
	float *cyclestore = dz->sampbuf[2];
	cyclestore[0] = 0; /* gardpnt for wrap_around in interpolating values */
	if((exit_status = read_samps(inbuf,dz))<0)
		return(exit_status);
	if((exit_status = get_initial_phase(&phase,dz))<0)
		return(exit_status);
	if((exit_status = skip_initial_cycles(&current_inbuf_pos,&current_buf,phase,DISTPCH_SKIPCNT,dz))<0)
		return(exit_status);
	do{
		while(current_inbuf_pos < dz->ssampsread)  {	
			switch(phase) {
			case(1):
				if(inbuf[current_inbuf_pos] >= 0) {
					cyclestore[j] = inbuf[current_inbuf_pos];
					if(++j >= dz->buflen) {
						sprintf(errstr,"Cycle too large for buffer at %lf\n",
						(double) dz->total_samps_read/(double)dz->infile->srate);
						return(GOAL_FAILED);
					}
					current_inbuf_pos++;
				} else {
					if(inbuf[current_inbuf_pos]<0) {
						phase=-1;
						cnt++;
					}
				}
				break;
			case(-1):
				if(inbuf[current_inbuf_pos] <= 0) {
					cyclestore[j] = inbuf[current_inbuf_pos];
					if(++j >= dz->buflen) {
						sprintf(errstr,"Cycle too large for buffer at %lf\n",
						(double)dz->total_samps_read/(double)dz->infile->srate);
						return(GOAL_FAILED);
					}
					current_inbuf_pos++;
				} else {
					if(inbuf[current_inbuf_pos]>0) {
						phase=1;
						cnt++;
					}
				}
				break;
			}
			if(cnt>=2) {	/* i.e. once we have a complete wavecycle */
				if((exit_status = do_distrt
				(j-1,&pos_in_cycle_group,&last_transpos,&next_transpos,&transpos_step,&thiscyclelen,&init,&obufremain,current_inbuf_pos,dz))<0)
					return(exit_status);  /* j-1 = length of cycle */
				cyclestore[0] = cyclestore[j-1];/*  wrap_around gardpnt for interpolation of values  */
				j = 1;							/* leave space for grdpnt */
				cnt = 0;
			}
		}
		if((exit_status = read_samps(inbuf,dz))<0)
			return(exit_status);
		current_inbuf_pos = 0;
	} while(dz->ssampsread>0);
	if(dz->sbufptr[1]!=dz->sampbuf[1])
		return write_samps(dz->sampbuf[1],dz->sbufptr[1] - dz->sampbuf[1],dz);
	return(FINISHED);
}

/*********************** GET_DISTORT *************************/

int get_distort
(int oldlen,int *pos_in_cycle_group,double *last_transpos,double *next_transpos,
double *transpos_step,int *thiscyclelen,int *init,dataptr dz)
{
	int newlen;
	double randval;
	if(*init) {
		*thiscyclelen = round(drand48() * (double)dz->iparam[DISTPCH_CYCLECNT]) + 1;
		*next_transpos = ((drand48() * 2.0) - 1.0) * dz->param[DISTPCH_OCTVAR];
		*transpos_step = (*next_transpos - *last_transpos)/(double)(*thiscyclelen);
		*pos_in_cycle_group = 0;
		*init = 0;
	} else {
		if(++(*pos_in_cycle_group) < *thiscyclelen)
			*last_transpos  += *transpos_step;
		else {
			*thiscyclelen  = round(drand48() * (double)dz->iparam[DISTPCH_CYCLECNT]) + 1;
			*last_transpos = *next_transpos;
			*next_transpos = ((drand48() * 2.0) - 1.0) * dz->param[DISTPCH_OCTVAR];
			*transpos_step = (*next_transpos - *last_transpos)/(double)(*thiscyclelen);
			*pos_in_cycle_group = 0;
		}
	}
	randval = pow(2.0,*last_transpos);
	newlen = round((double)oldlen * randval);
	return(newlen);
}
		
/*********************** DO_DISTRT *************************/

int do_distrt(int oldlen,int *pos_in_cycle_group,double *last_transpos,double *next_transpos,
double *transpos_step,int *thiscyclelen,int *init,int *obufremain,int current_inbuf_pos, dataptr dz)
{
	int exit_status;
	float *cyclestore 		 = dz->sampbuf[2];
	float *warpedcycle_store = dz->sampbuf[3];
    int distortbuf_pos = 0, k, newlen;
	double step, here, ratio;
	float thisin, nextin;
	double thistime;
	if(*init) {
		if(dz->brksize[DISTPCH_OCTVAR] > 0) {
			if((exit_status = read_value_from_brktable(0.0,DISTPCH_OCTVAR,dz))<0)
				return exit_status;
		}
		if(dz->brksize[DISTPCH_CYCLECNT] > 0) {
			if((exit_status = read_value_from_brktable(0.0,DISTPCH_CYCLECNT,dz))<0)
				return exit_status;
		}
	}
	newlen = get_distort(oldlen,pos_in_cycle_group,last_transpos,next_transpos,transpos_step,thiscyclelen,init,dz);
	thistime = (double)(dz->total_samps_read - dz->ssampsread + current_inbuf_pos)/(double)dz->infile->srate;
	if(dz->brksize[DISTPCH_OCTVAR] > 0) {
		if((exit_status = read_value_from_brktable(thistime,DISTPCH_OCTVAR,dz))<0)
			return exit_status;
	}
	if(dz->brksize[DISTPCH_CYCLECNT] > 0) {
		if((exit_status = read_value_from_brktable(thistime,DISTPCH_CYCLECNT,dz))<0)
			return exit_status;
	}
	if(newlen <=0)
		newlen = 1;
	if(newlen == oldlen)  {
		memmove((char *)warpedcycle_store,(char *)cyclestore,oldlen * sizeof(float));
		distortbuf_pos = newlen;
	} else {	
		step = (double)oldlen/(double)newlen;
		here = step;			    
		while((k = (int)here)<oldlen) {
			thisin = cyclestore[k];
			nextin = cyclestore[k+1];
			ratio = here - (double)k;
			warpedcycle_store[distortbuf_pos++]  = (float)(/*round*/((double)(nextin - thisin)*ratio) + thisin);
			if(distortbuf_pos >= dz->buflen) {
				if((exit_status = move_to_outbut(dz->buflen,obufremain,dz))<0)
					return(exit_status);
				distortbuf_pos = 0;
			}
			here += step;
		}
	}
	if(distortbuf_pos)
		return move_to_outbut(distortbuf_pos,obufremain,dz);
	return(FINISHED);
}

/*********************** MOVE_TO_OUTBUT *************************/

int move_to_outbut(int n,int *obufremain,dataptr dz)
{
	int exit_status;
	/* dz->sampbuf[1] = outbuf */
	float *warpedcycle_ptr = dz->sampbuf[3];
	while(n > *obufremain) {
		if(*obufremain) {
			memmove((char *)dz->sbufptr[1],(char *)warpedcycle_ptr,*obufremain * sizeof(float));
			warpedcycle_ptr += *obufremain;
			n      -= *obufremain;
		}
		if((exit_status = write_samps(dz->sampbuf[1],dz->buflen,dz))<0)
			return(exit_status);
		dz->sbufptr[1]  = dz->sampbuf[1];
		*obufremain 	= dz->buflen;
	}
	if(n) {
 		memmove((char *)dz->sbufptr[1],(char *)warpedcycle_ptr,n * sizeof(float));
		dz->sbufptr[1]  += n;
		*obufremain 	-= n;
	}
	return(FINISHED);
}
