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

#define	FIRST_FILE	(0)
#define	SECOND_FILE	(1)

static int get_initial_phases_for_resize(float *samplebuf0,int sampcnt0,float *samplebuf1,int sampcnt1,int *initial_phase);
static int read_samps_cc(int k,int current_buf,int *bsamps_left,int *samps_read,int *in_samps,dataptr dz);
static int get_cycles_and_resize(int *,int *,int *,int *,int *,int *,int *,int *,dataptr);
static int switchbuf(float **b,int *current_buf,int *i,int *samps_left,int *samps_read,int *in_samps,dataptr dz);
static int read_cycle_from_file0(int *,int *,int *,int *,int *,int *,int,dataptr);
static int read_cycle_from_file1(int *,int *,int *,int *,int *,int *,int,dataptr);
static int write_scaled_cycle_to_outbuf(int lastzero,int cyclelen0,int cyclelen1,int current_buf,int *obufpos,dataptr dz);

/***************************** TWO_INFILES_RESIZE_PROCESS *****************************/

int two_infiles_resize_process(dataptr dz)
{
	int exit_status;
	int bsamps_left[2],samps_read[2],in_samps;
	int current_buf = 0, initial_phase[2];
	int bufpos_file0 = 0, bufpos_file1 = 0, obufpos = 0;
	bsamps_left[0] = dz->insams[0];
	bsamps_left[1] = dz->insams[1];
	if((exit_status = read_samps_cc(FIRST_FILE,current_buf,bsamps_left,samps_read,&in_samps,dz))<0
	|| (exit_status = read_samps_cc(SECOND_FILE,current_buf,bsamps_left,samps_read,&in_samps,dz))<0)
		return(exit_status);
	if((exit_status = get_initial_phases_for_resize(dz->sampbuf[3],in_samps,dz->sampbuf[0],samps_read[0],initial_phase))<0)
		return(exit_status);
	do {
		exit_status = get_cycles_and_resize
		(&bufpos_file0,&bufpos_file1,&obufpos,initial_phase,&current_buf,bsamps_left,samps_read,&in_samps,dz);
	} while(exit_status==CONTINUE);
	if(obufpos > 0)
		return write_samps(dz->sampbuf[2],obufpos,dz);
	return FINISHED;
}

/**************************** READ_SAMPS_CC ****************************/

int read_samps_cc(int k,int current_buf,int *bsamps_left,int *samps_read,int *in_samps,dataptr dz)
{
	int sampsread;
	switch(k) {
	case(0):
		if((sampsread = fgetfbufEx(dz->sampbuf[3], dz->buflen,dz->ifd[0],0)) < 0) {
			sprintf(errstr, "Can't read from 1st input soundfile\n");
			return(SYSTEM_ERROR);
		}
		bsamps_left[0] -= sampsread;
		*in_samps = sampsread;
		break;
	case(1):
		if((sampsread = fgetfbufEx(dz->sampbuf[current_buf], dz->buflen,dz->ifd[1],0)) < 0) {
			sprintf(errstr, "Can't read from 2nd input soundfile\n");
			return(SYSTEM_ERROR);
		}
		bsamps_left[1] -= sampsread;
		samps_read[current_buf] = sampsread;
		break;
	default:
		sprintf(errstr,"Unknown case in read_samps_cc()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/*************************** GET_INITIAL_PHASES_FOR_RESIZE **********************/

int get_initial_phases_for_resize(float *samplebuf0,int sampcnt0,float *samplebuf1,int sampcnt1,int *initial_phase)
{
	int bufpos = 0;
	while(samplebuf0[bufpos]==0.0 && bufpos < sampcnt0)   
		bufpos++;
	if(bufpos >= sampcnt0) {
		sprintf(errstr,"No siginificant data in first buffer of file 1\n");
		return(DATA_ERROR);
	}
	if(samplebuf0[bufpos] > 0.0)   
		initial_phase[0] = 1; 
	else 
		initial_phase[0] = -1;
	bufpos = 0;
//TW
//	while(samplebuf1[bufpos]==0 && bufpos < sampcnt1)   
	while(smpflteq(samplebuf1[bufpos],0.0) && bufpos < sampcnt1)   
		bufpos++;
	if(bufpos >= sampcnt1) {
		sprintf(errstr,"No siginificant data in first buffer of file 2\n");
		return(DATA_ERROR);
	}
	if(samplebuf1[bufpos] > 0.0)   
		initial_phase[1] = 1; 
	else 
		initial_phase[1] = -1;
	return(FINISHED);
}

/********************************** GET_CYCLES_AND_RESIZE *******************************/

int get_cycles_and_resize(int *bufpos_file0,int *bufpos_file1,int *obufpos,
int *initial_phase,int *current_buf,int *bsamps_left,int *samps_read,int *in_samps,dataptr dz)
{
	int exit_status;
	int  cyclelen[2] = {0,0};
	int  lastzero = *bufpos_file1;

	if((exit_status = read_cycle_from_file0
	(bufpos_file0,&(cyclelen[0]),in_samps,bsamps_left,samps_read,current_buf,initial_phase[0],dz))!=CONTINUE)
		return(exit_status);

	if((exit_status = read_cycle_from_file1
	(bufpos_file1,&(cyclelen[1]),in_samps,bsamps_left,samps_read,current_buf,initial_phase[1],dz))!=CONTINUE)
		return(exit_status);

	return write_scaled_cycle_to_outbuf(lastzero,cyclelen[0],cyclelen[1],*current_buf,obufpos,dz);
}

/**************************** SWITCHBUF *****************************
 *
 * Switch to other input buffer for 2nd infile.
 */

int switchbuf(float **b,int *current_buf,int *i,int *bsamps_left,int *samps_read,int *in_samps,dataptr dz)
{
	int exit_status;
	if(bsamps_left[1]<=0)
		return(FINISHED);
	*current_buf = !(*current_buf);
	*b = dz->sampbuf[*current_buf];
	if((exit_status = read_samps_cc(SECOND_FILE,*current_buf,bsamps_left,samps_read,in_samps,dz))<0)
		return(exit_status);
	*i = 0;
	return(CONTINUE);
}

/**************************** READ_CYCLE_FROM_FILE0 *****************************/

int read_cycle_from_file0
(int *bufpos_file0,int *cyclelen0,int *in_samps,int *bsamps_left,int *samps_read,
int *current_buf,int initial_phase, dataptr dz)
{
	int exit_status;
	int i 		= *bufpos_file0;
	int cyclen = *cyclelen0;
	float *b = dz->sampbuf[3];
	switch(initial_phase) {
	case(1):
		while(b[i]>=0.0) {
			cyclen++;
			if(++i >= *in_samps) {
				if(bsamps_left[0]<=0)
					return(FINISHED);
				if((exit_status = read_samps_cc(FIRST_FILE,*current_buf,bsamps_left,samps_read,in_samps,dz))<0)
					return(exit_status);
				i = 0;
			}
		}
		while(b[i]<=0.0) {
			cyclen++;
			if(++i >= *in_samps) {
				if(bsamps_left[0]<=0)
					return(FINISHED);
				if((exit_status = read_samps_cc(FIRST_FILE,*current_buf,bsamps_left,samps_read,in_samps,dz))<0)
					return(exit_status);
				i = 0;
			}
		}
		break;
	case(-1):
		while(b[i]<=0.0) {
			cyclen++;
			if(++i >= *in_samps) {
				if(bsamps_left[0]<=0)
					return(FINISHED);
				if((exit_status = read_samps_cc(FIRST_FILE,*current_buf,bsamps_left,samps_read,in_samps,dz))<0)
					return(exit_status);
				i = 0;
			}
		}
		while(b[i]>=0.0) {
			cyclen++;
			if(++i >= *in_samps) {
				if(bsamps_left[0]<=0)
					return(FINISHED);
				if((exit_status = read_samps_cc(FIRST_FILE,*current_buf,bsamps_left,samps_read,in_samps,dz))<0)
					return(exit_status);
				i = 0;
			}
		}
		break;
	}
	*bufpos_file0 = i;
	*cyclelen0 	  = cyclen;
	return(CONTINUE);
}

/**************************** READ_CYCLE_FROM_FILE1 *****************************/

int read_cycle_from_file1
(int *bufpos_file1,int *cyclelen1,int *in_samps,int *bsamps_left,int *samps_read,
int *current_buf,int initial_phase,dataptr dz)
{
	int exit_status;
	int i = *bufpos_file1;
	float *b = dz->sampbuf[*current_buf];
	int cyclen = *cyclelen1;
	switch(initial_phase) {
	case(1):
		while(b[i]>=0.0) {
			cyclen++;
			if(++i >= samps_read[*current_buf]) {
				if((exit_status = switchbuf(&b,current_buf,&i,bsamps_left,samps_read,in_samps,dz))!=CONTINUE)
					return(exit_status);
			}
		}
		while(b[i]<=0.0) {
			cyclen++;
			if(++i >= samps_read[*current_buf]) {
				if((exit_status = switchbuf(&b,current_buf,&i,bsamps_left,samps_read,in_samps,dz))!=CONTINUE)
					return(exit_status);
			}
		}
		break;
	case(-1):
		while(b[i]<=0.0) {
			cyclen++;
			if(++i >= samps_read[*current_buf]) {
				if((exit_status = switchbuf(&b,current_buf,&i,bsamps_left,samps_read,in_samps,dz))!=CONTINUE)
					return(exit_status);
			}
		}
		while(b[i]>=0.0) {
			cyclen++;
			if(++i >= samps_read[*current_buf]) {
				if((exit_status = switchbuf(&b,current_buf,&i,bsamps_left,samps_read,in_samps,dz))!=CONTINUE)
					return(exit_status);
			}
		}
		break;
	}
	*bufpos_file1 = i;
	*cyclelen1 = cyclen;
	return(CONTINUE);
}

/**************************** WRITE_SCALED_CYCLE_TO_OUTBUF *****************************/

int write_scaled_cycle_to_outbuf(int lastzero,int cyclelen0,int cyclelen1,int current_buf,int *obufpos,dataptr dz)
{
	int exit_status;
	float *b;
	int j = *obufpos;
	int in_previous_buf = FALSE;
	int sampcnt, index;
	double scaler = (double)cyclelen1/(double)cyclelen0;
	if(lastzero + cyclelen1 >= dz->buflen) {
		b = dz->sampbuf[!current_buf];
		in_previous_buf = TRUE;
		for(sampcnt=0;sampcnt<cyclelen0;sampcnt++) {
			index  = round((double)sampcnt * scaler);
			index += lastzero;						
			if(in_previous_buf) {					
				if(index >= dz->buflen) {			
					index -= dz->buflen;
					b     = dz->sampbuf[current_buf];
					in_previous_buf = FALSE;
				}
			} else									
				index -= dz->buflen;
			dz->sampbuf[2][j] = b[index];		
			if(++j >= dz->buflen) {				
				if((exit_status = write_samps(dz->sampbuf[2],dz->buflen,dz))<0) {
					sprintf(errstr,"write_samps failed in write_scaled_cycle_to_outbuf()\n");
					return(exit_status);
				}
				j = 0;
			}
		}
	} else {
		b = dz->sampbuf[current_buf];
		for(sampcnt=0;sampcnt<cyclelen0;sampcnt++) {
			index  = round((double)sampcnt * scaler);
			dz->sampbuf[2][j] = b[index + lastzero];
			if(++j >= dz->buflen) {				
				if((exit_status = write_samps(dz->sampbuf[2],dz->buflen,dz))<0) {
					sprintf(errstr,"write_samps failed in write_scaled_cycle_to_outbuf()\n");
					return(exit_status);
				}
				j = 0;
			}
		}
	}
	*obufpos = j;
	return(CONTINUE);
}
