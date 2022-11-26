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

static int 	is_filtered_out(int cyclecnt,dataptr dz);
static int 	copy_cycle_to_output(float *b,int endpos,int startpos,int *obufpos,dataptr dz);
static int 	copy_crosbuf_cycle_to_output(int endpos,int startpos,int current_buf,int *obufpos,dataptr dz);

/************************** DISTORT_FLT **************************/

int distort_flt(int *current_buf,int initial_phase,int *obufpos,int *current_pos_in_buf,dataptr dz)
{
	int exit_status;
	register int i = *current_pos_in_buf;
	float *b = dz->sampbuf[*current_buf];
	int cyclelen;
	int cyclestart = *current_pos_in_buf;
	int buffer_overrun = FALSE;
	switch(initial_phase) {
	case(1):
		while(b[i]>=0) {
			if(++i >= dz->ssampsread) {
				if((exit_status = change_buf(current_buf,&buffer_overrun,&b,dz))!=CONTINUE) {
					*current_pos_in_buf = i;
					return(exit_status);
				}
				i = 0;
			}
		}
		while(b[i]<=0) {
			if(++i >= dz->ssampsread) {
				if((exit_status = change_buf(current_buf,&buffer_overrun,&b,dz))!=CONTINUE) {
					*current_pos_in_buf = i;
					return(exit_status);
				}
				i = 0;
			}
		}
		break;
	case(-1):
		while(b[i]<=0) {
			if(++i >= dz->ssampsread) {
				if((exit_status = change_buf(current_buf,&buffer_overrun,&b,dz))!=CONTINUE) {
					*current_pos_in_buf = i;
					return(exit_status);
				}
				i = 0;
			}
		}
		while(b[i]>=0) {
			if(++i >= dz->ssampsread) {
				if((exit_status = change_buf(current_buf,&buffer_overrun,&b,dz))!=CONTINUE) {
					*current_pos_in_buf = i;
					return(exit_status);
				}
				i = 0;
			}
		}
		break;
	}
	cyclelen = i - cyclestart;
	if(buffer_overrun==TRUE)
		cyclelen += dz->buflen;
	exit_status = is_filtered_out(cyclelen,dz);
	switch(exit_status) {
	case(TRUE):	 /* ignore this cycle */ 
		break;	
	case(FALSE): /*  keep this cycle  */
		if(buffer_overrun==TRUE)
			exit_status = copy_crosbuf_cycle_to_output(i,cyclestart,*current_buf,obufpos,dz);
		else
			exit_status = copy_cycle_to_output(b,i,cyclestart,obufpos,dz);	
		break;
	}
	if(exit_status<0)
		return(exit_status);
	*current_pos_in_buf = i;
	return(CONTINUE);
}

/****************************** IS_FILTERED_OUT ****************************/

int is_filtered_out(int cyclelen,dataptr dz)
{
	switch(dz->mode) {
	case(DISTFLT_HIPASS):
		if(cyclelen>dz->iparam[DISTFLT_LOFRQ_CYCLELEN])
			return(TRUE);
		break;
	case(DISTFLT_LOPASS):
		if(cyclelen<dz->iparam[DISTFLT_HIFRQ_CYCLELEN])
			return(TRUE);
		break;
	case(DISTFLT_BANDPASS):
		if(cyclelen<dz->iparam[DISTFLT_HIFRQ_CYCLELEN] || cyclelen>dz->iparam[DISTFLT_LOFRQ_CYCLELEN])
			return(TRUE);
		break;
	default: 
		sprintf(errstr,"Unknown mode in is_filtered_out()\n");
		return(PROGRAM_ERROR);
	}
	return(FALSE);
}

/**************************** COPY_CYCLE_TO_OUTPUT *****************************/

int copy_cycle_to_output(float *b,int endpos,int startpos,int *obufpos,dataptr dz)
{
	int exit_status;
	while(startpos < endpos) {
		if((exit_status = output_val(b[startpos++],obufpos,dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}

/************************* COPY_CROSBUF_CYCLE_TO_OUTPUT ****************************/

int copy_crosbuf_cycle_to_output(int endpos,int startpos,int current_buf,int *obufpos,dataptr dz)
{
	int exit_status;
	float *b = dz->sampbuf[!current_buf];
	while(startpos < dz->buflen) {
		if((exit_status = output_val(b[startpos++],obufpos,dz))<0)
			return(exit_status);
	}
	b = dz->sampbuf[current_buf];
	startpos = 0;
	while(startpos < endpos) {
		if((exit_status = output_val(b[startpos++],obufpos,dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}
