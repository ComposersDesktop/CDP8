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

static void reverse_cycles(int startpos,int current_pos,int current_buf,dataptr dz);
//TW UPDATE
//static void reverse_cycles_crosbuf(int cyclestart,int current_pos,int current_buf,dataptr dz);
static int  reverse_cycles_crosbuf(int cyclestart,int current_pos,int current_buf,dataptr dz);

/************************** DISTORT_REV **************************/

int distort_rev(int *current_buf,int initial_phase,int *current_pos,int *buffer_overrun,int *cnt,dataptr dz)
{
	int exit_status;
	int cyclestart = *current_pos;
	float *b = dz->sampbuf[*current_buf];
	if((exit_status = get_full_cycle(b,buffer_overrun,current_buf,initial_phase,current_pos,DISTORTR_CYCLECNT,dz))!=CONTINUE)
		return(exit_status);
//TW UPDATE
//	if(*buffer_overrun)
//		reverse_cycles_crosbuf(cyclestart,*current_pos,*current_buf,dz);
//	else
	if(*buffer_overrun) {
		if((exit_status = reverse_cycles_crosbuf(cyclestart,*current_pos,*current_buf,dz))<0)
			return(exit_status);
	} else
		reverse_cycles(cyclestart,*current_pos,*current_buf,dz);	
	(*cnt)++;
	return(CONTINUE);
}

/**************************** REVERSE_CYCLES ***************************/

void reverse_cycles(int startpos,int current_pos,int current_buf,dataptr dz)
{
	register int j, endpos = current_pos-1;
	int midpoint = (current_pos - startpos)/2;	
	float *buf = dz->sampbuf[current_buf];
	float dummy;
	for(j=0;j<midpoint;j++) {
		dummy           = buf[startpos];
		buf[startpos++] = buf[endpos];
		buf[endpos--]   = dummy;
	}
}

/******************** REVERSE_CYCLES_CROSBUF ****************/

//TW UPDATE
//void reverse_cycles_crosbuf(int cyclestart,int current_pos,int current_buf,dataptr dz)
int reverse_cycles_crosbuf(int cyclestart,int current_pos,int current_buf,dataptr dz)
{
//TW UPDATE
//	int exit_status;
	float *this_buf      = dz->sampbuf[current_buf];
	float *previous_buf  = dz->sampbuf[!current_buf];
	int samps_in_thisbuf = current_pos;
	int samps_in_previous_buf = dz->buflen - cyclestart;
	int midpoint  = (samps_in_thisbuf + samps_in_previous_buf)/2;
	int min_samps_in_one_of_bufs  = min(samps_in_previous_buf,samps_in_thisbuf);
	register int j, startpos  = cyclestart, endpos  = current_pos - 1;
	float dummy;
	for(j=0;j<min_samps_in_one_of_bufs;j++) {
		dummy                    = previous_buf[startpos];
		previous_buf[startpos++] = this_buf[endpos];
		this_buf[endpos--]       = dummy;
	}
	if(samps_in_previous_buf>samps_in_thisbuf) {
		endpos = dz->buflen - 1;
		for(j=min_samps_in_one_of_bufs;j<midpoint;j++) {
			dummy                    = previous_buf[startpos];
			previous_buf[startpos++] = previous_buf[endpos];
			previous_buf[endpos--]   = dummy;
		}
	} else {
		startpos = 0;
		for(j=min_samps_in_one_of_bufs;j<midpoint;j++) {
			dummy                = this_buf[startpos];
			this_buf[startpos++] = this_buf[endpos];
			this_buf[endpos--]   = dummy;
		}
	}

	return write_samps(dz->sampbuf[!current_buf],dz->buflen,dz);
}
