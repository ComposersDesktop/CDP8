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
#include <structures.h>
#include <tkglobals.h>
#include <globcon.h>
#include <modeno.h>
#include <arrays.h>
#include <distort.h>
#include <cdpmain.h>

#include <sfsys.h>
#include <osbind.h>

#define	FCYCBUF	(2)

static void do_fractal(int grouplen,int current_buf,dataptr dz);
static void do_fractal_crosbuf(int grouplen,int bufcros_cycle,int current_buf,dataptr dz);
static int 	get_scaled_halfcycle(int n,int *cnt,float **buf,int *bufpos,int *grouplen,int *current_buf,
			int *is_bufcros,int *bufcros_cycle,int prescale_param,int *last_total_samps,dataptr dz);
static int 	change_buf_marking_crossing_cycle_no(int n,float **buf,int *is_bufcross,int *bufcros_cycle,int *current_buf,dataptr dz);

/**************************** DISTORTF ******************************
 *
 * Distort file by adding smaller scale perturbations modelled on large.
 */

int distortf(int *bufpos,int phase,int *last_total_samps,int *current_buf,dataptr dz)
{
	int exit_status;
	int grouplen = 0;
	int cnt = 0, n;
	int firstime = 1;   
	float *buf  = dz->sampbuf[*current_buf];
	int bufcros_cycle = 0;
	int is_bufcros = 0;
	double thistime = (double)(dz->total_samps_written + *bufpos)/(double)(dz->infile->srate);
	if((exit_status = read_values_from_all_existing_brktables(thistime,dz))<0)
		return(exit_status);
	switch(phase) {
	case(1):
		for(n=0;n<dz->iparam[DISTORTF_SCALE];n++) {
			dz->lparray[DISTORTF_STARTCYC][n] = *bufpos;
			while(buf[*bufpos]>=0.0) {
				if((exit_status = get_scaled_halfcycle(n,&cnt,&buf,bufpos,&grouplen,current_buf,&is_bufcros,&bufcros_cycle,DISTORTF_PRESCALE,last_total_samps,dz))!=CONTINUE)
					return(exit_status);
			}
			while(buf[*bufpos]<=0.0) {
				if((exit_status = get_scaled_halfcycle(n,&cnt,&buf,bufpos,&grouplen,current_buf,&is_bufcros,&bufcros_cycle,DISTORTF_PRESCALE,last_total_samps,dz))!=CONTINUE)
					return(exit_status);
			}
			if(is_bufcros && firstime) {
				dz->lparray[DISTORTF_CYCLEN][n] = dz->buflen - dz->lparray[DISTORTF_STARTCYC][n] + *bufpos;
				firstime = 0;
			} else
				dz->lparray[DISTORTF_CYCLEN][n] = *bufpos - dz->lparray[DISTORTF_STARTCYC][n];
		}
		break;
	case(-1):
		for(n=0;n<dz->iparam[DISTORTF_SCALE];n++) {
			dz->lparray[DISTORTF_STARTCYC][n] = *bufpos;
			while(buf[*bufpos]<=0.0) {
				if((exit_status = get_scaled_halfcycle(n,&cnt,&buf,bufpos,&grouplen,current_buf,&is_bufcros,&bufcros_cycle,DISTORTF_PRESCALE,last_total_samps,dz))!=CONTINUE)
					return(exit_status);
			}
			while(buf[*bufpos]>=0.0) {
				if((exit_status = get_scaled_halfcycle(n,&cnt,&buf,bufpos,&grouplen,current_buf,&is_bufcros,&bufcros_cycle,DISTORTF_PRESCALE,last_total_samps,dz))!=CONTINUE)
					return(exit_status);
			}
			if(is_bufcros && firstime) {
				dz->lparray[DISTORTF_CYCLEN][n] = dz->buflen - dz->lparray[DISTORTF_STARTCYC][n] + *bufpos;
				firstime = 0;
			} else
				dz->lparray[DISTORTF_CYCLEN][n] = *bufpos - dz->lparray[DISTORTF_STARTCYC][n];
		}
		break;
	}
	dz->lparray[DISTORTF_STARTCYC][dz->iparam[DISTORTF_SCALE]] = *bufpos;
	if(is_bufcros) {
		do_fractal_crosbuf(grouplen,bufcros_cycle,*current_buf,dz);
		if((exit_status = write_samps(dz->sampbuf[!(*current_buf)],dz->buflen,dz))<0)
			return(exit_status);
		is_bufcros = 0;
	} else
		do_fractal(grouplen,*current_buf,dz);
	return(CONTINUE);
}

/***************************** DO_FRACTAL *******************************/

void do_fractal(int grouplen,int current_buf,dataptr dz)
{
	int n, k, i, index;
	double ratio;
	float *buf = dz->sampbuf[current_buf];
	for(n=0;n<dz->iparam[DISTORTF_SCALE];n++) {
		ratio = (double)grouplen/(double)dz->lparray[DISTORTF_CYCLEN][n];
		k = 0;
		i = dz->lparray[DISTORTF_STARTCYC][n];
		while(i < dz->lparray[DISTORTF_STARTCYC][n+1]) {
			index    = round(ratio * (double)k++);
			buf[i++] += (float)(dz->param[DISTORTF_AMPFACT] * dz->sampbuf[FCYCBUF][index]);
		}
	}
}

/*************************** DO_FRACTAL_CROSBUF ***************************/

void do_fractal_crosbuf(int grouplen,int bufcros_cycle,int current_buf,dataptr dz)
{
	float *buf = dz->sampbuf[!current_buf];
	int n, k, i, index;
	double ratio;
	for(n=0;n<bufcros_cycle;n++) {
		ratio = (double)grouplen/(double)dz->lparray[DISTORTF_CYCLEN][n];
		k = 0;
		i = dz->lparray[DISTORTF_STARTCYC][n];
		while(i < dz->lparray[DISTORTF_STARTCYC][n+1]) {
			index    = round(ratio * (double)k++);
			buf[i++] += (float)(dz->param[DISTORTF_AMPFACT] * dz->sampbuf[FCYCBUF][index]);
		}
	}
	ratio = (double)grouplen/(double)dz->lparray[DISTORTF_CYCLEN][n];
	k = 0;
	i = dz->lparray[DISTORTF_STARTCYC][n];
	while(i < dz->buflen) {
		index    = round(ratio * (double)k++);
		buf[i++] += (float)(dz->param[DISTORTF_AMPFACT] * dz->sampbuf[FCYCBUF][index]);
	}
	buf = dz->sampbuf[current_buf];
	i = 0;
	while(i < dz->lparray[DISTORTF_STARTCYC][n+1]) {
		index    = round(ratio * (double)k++);
		buf[i++] += (float)(dz->param[DISTORTF_AMPFACT] * dz->sampbuf[FCYCBUF][index]);
	}
	for(n=bufcros_cycle+1;n<dz->iparam[DISTORTF_SCALE];n++) {
		ratio = (double)grouplen/(double)dz->lparray[DISTORTF_CYCLEN][n];
		k = 0;
		i = dz->lparray[DISTORTF_STARTCYC][n];
		while(i < dz->lparray[DISTORTF_STARTCYC][n+1]) {
			index    = round(ratio * (double)k++);
			buf[i++] += (float)(dz->param[DISTORTF_AMPFACT] * dz->sampbuf[FCYCBUF][index]);
		}
	}
}

/**************************** GET_SCALED_HALFCYCLE **************************/

int get_scaled_halfcycle(int n,int *cnt,float **buf,int *bufpos,int *grouplen,int *current_buf,int *is_bufcros,int *bufcros_cycle,
				  int prescale_param,int *last_total_samps,dataptr dz)
{
	int exit_status;
	if(++(*cnt) >= dz->iparam[DISTORTF_SCALE]) {

		dz->sampbuf[FCYCBUF][*grouplen] = (*buf)[*bufpos];
		if(++(*grouplen) >dz->iparam[DISTORTF_CYCBUFLEN]) {
			cop_out(*bufpos,*grouplen,*last_total_samps,dz);
			return FINISHED;
		}
		*cnt = 0;
	}
	if(++(*bufpos) >= dz->ssampsread) {
		*last_total_samps = dz->total_samps_read;
		if((exit_status = change_buf_marking_crossing_cycle_no(n,buf,is_bufcros,bufcros_cycle,current_buf,dz))!=CONTINUE)
			return(exit_status);
		if(dz->vflag[IS_PRESCALED])
			prescale(*current_buf,prescale_param,dz);
		*bufpos = 0;
	}
	return(CONTINUE);
}

/****************************** CHANGE_BUF_MARKING_CROSSING_CYCLE_NO ****************************
 *
 * (1) 'bufcros_cycle' marks the cycle-no which crosses between buffers.
 */

int change_buf_marking_crossing_cycle_no(int n,float **buf,int *is_bufcros,int *bufcros_cycle,int *current_buf,dataptr dz)
{
	int exit_status;
	if(dz->samps_left <= 0)
		return(FINISHED);
	if(*is_bufcros==TRUE) {
		sprintf(errstr,"cycle_search exceeds buffer size.\n");
		return(GOAL_FAILED);
	}
	*current_buf = !(*current_buf);
	*buf = dz->sampbuf[*current_buf];
	*bufcros_cycle = n;
	*is_bufcros = 1;
	if((exit_status = read_samps(*buf,dz))<0)
		return(exit_status);		
	return(CONTINUE);
}
