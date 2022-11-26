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
#include <modeno.h>
#include <arrays.h>
#include <distort.h>
#include <cdpmain.h>

#include <sfsys.h>
#include <osbind.h>


static int read_samps_bb(int k,int *bsamps_left,int *sampsread,dataptr dz);
static int get_initial_phases(float **sampbuf,int *samps_read,int *initial_phase,int *rephase);
static int get_cycle(int *p,int *q,int n,int *initial_phase,int rephase,int *bsamps_left,int *samps_read,dataptr dz);
static int write_inverted_block(float *b,int obuf_endwrite,int *obufpos,int *last_read_marker,dataptr dz);
static int write_this_block(float *b,int samps_to_copy,int obuf_endwrite,int *obufpos,int *last_read_marker,dataptr dz);

/***************************** TWO_INFILES_PROCESS *****************************/

int two_infiles_interleave_process(dataptr dz)
{
	int exit_status;
 	int bsamps_left[2], samps_read[2];
 	int initial_phase[2];
	int rephase = FALSE;
	int i1 = 0, i2 = 0, j = 0;
	bsamps_left[0] = dz->insams[0];
	bsamps_left[1] = dz->insams[1];
	if((exit_status = read_samps_bb(0,bsamps_left,samps_read,dz))<0
	|| (exit_status = read_samps_bb(1,bsamps_left,samps_read,dz))<0)
		return(exit_status);

	if((exit_status = get_initial_phases(dz->sampbuf,samps_read,initial_phase,&rephase))<0)
		return(exit_status);
	switch(dz->mode) {
	case(DISTINT_INTRLV):
		do {
			if((exit_status = get_cycle(&i1,&j,0,initial_phase,rephase,bsamps_left,samps_read,dz))!=CONTINUE)
				break;
			exit_status = get_cycle(&i2,&j,1,initial_phase,rephase,bsamps_left,samps_read,dz);
		} while(exit_status==CONTINUE);
		break;
	default:
		sprintf(errstr,"Unknown case in two_infiles_interleave_process()\n");
		return(PROGRAM_ERROR);
	}
	if(exit_status<0)
		return(exit_status);
	if(j > 0)
		return write_samps(dz->sampbuf[2],j,dz);
	return FINISHED;
}

/**************************** READ_SAMPS_BB ****************************/

int read_samps_bb(int k,int *bsamps_left,int *samps_read,dataptr dz)
{
	int /*bytes_read*/sampsread;
	if(( sampsread = fgetfbufEx(dz->sampbuf[k], dz->buflen,dz->ifd[k],0)) < 0) {
		sprintf(errstr, "Can't read from input soundfile %d\n",k+1);
		return(SYSTEM_ERROR);
	}
	bsamps_left[k] -= sampsread;
	if(sloom)
		dz->total_samps_read = (bsamps_left[0] + bsamps_left[1]) >> 1;
	samps_read[k]   = sampsread;
	return(FINISHED);
}

/********************************** GET_CYCLE ******************************/

int get_cycle(int *p,int *q,int n,int *initial_phase,int rephase,int *bsamps_left,int *samps_read,dataptr dz)
{
	int exit_status;
	int   startphase  = initial_phase[n];
	float *b          = dz->sampbuf[n];
	int  samples     = samps_read[n];
	int  samps_to_copy, obuf_endwrite;
	register int ibufpos = *p;				
	int obufpos = *q;
	int  last_read_marker = ibufpos;				
	int   in_1st_half_of_cycle = TRUE, OK = TRUE;
	switch(startphase) {					
	case(1):
		while(b[ibufpos]>=0) {
			if(++ibufpos >= samples) {
				OK = FALSE;
				break;
			}
		}
		if(!OK)
			break;
		in_1st_half_of_cycle = FALSE;
		while(b[ibufpos]<=0) {
			if(++ibufpos >= samples)
				break;
		}
		break;
	case(-1):
		while(b[ibufpos]<=0)	{
			if(++ibufpos >= samples) {
				OK = FALSE;
				break;
			}
		}
		if(!OK)
			break;
		in_1st_half_of_cycle = FALSE;
		while(b[ibufpos]>=0) {
			if(++ibufpos >= samples)
				break;
		}
		break;
	}
	samps_to_copy = ibufpos - last_read_marker;	  /* may be only PART of cycle, if end of input buf was encountered */
	obuf_endwrite = obufpos + samps_to_copy;
	if(n==1 && rephase)		/* if this is 2nd snd, and phase is different from 1st snd, invert all vals */
		exit_status = write_inverted_block(b,obuf_endwrite,&obufpos,&last_read_marker,dz);
	else 							
		exit_status = write_this_block(b,samps_to_copy,obuf_endwrite,&obufpos,&last_read_marker,dz);

	if(exit_status < 0)
		return(exit_status);
		
	if(ibufpos>=samples) {	 		/* if we reached input buffer end */
		if(bsamps_left[n] <= 0) {	/* if we reached file end */
			*q = obufpos;
			return(FINISHED);
		}							/* else, deal with part-cycle in next buffer */
		read_samps_bb(n,bsamps_left,samps_read,dz);
		last_read_marker = 0;
		ibufpos  		 = 0;
		switch(startphase) {
		case(1):
			if(in_1st_half_of_cycle)  {
				while(b[ibufpos]>=0) {
					if(++ibufpos >= dz->buflen) {
						sprintf(errstr,"Cyclelen exceeeds buffer_size: cannot proceed\n");
						return(GOAL_FAILED);
					}
				}
			}
			while(b[ibufpos]<=0) {
				if(++ibufpos >= dz->buflen) {
					sprintf(errstr,"Cyclelen exceeeds buffer_size: cannot proceed\n");
					return(GOAL_FAILED);
				}
			}
			break;
		case(-1):
			if(in_1st_half_of_cycle) {
				while(b[ibufpos]<=0) {
					if(++ibufpos >= dz->buflen) {
						sprintf(errstr,"Cyclelen exceeeds buffer_size: cannot proceed\n");
						return(GOAL_FAILED);
					}
				}
			}
			while(b[ibufpos]>=0) {
				if(++ibufpos >= dz->buflen) {
					sprintf(errstr,"Cyclelen exceeeds buffer_size: cannot proceed\n");
					return(GOAL_FAILED);
				}
			}
			break;
		}
		samps_to_copy = ibufpos;
		obuf_endwrite = obufpos + samps_to_copy;   
		if(n==1 && rephase)
			exit_status = write_inverted_block(b,obuf_endwrite,&obufpos,&last_read_marker,dz);
		else 							
			exit_status = write_this_block(b,samps_to_copy,obuf_endwrite,&obufpos,&last_read_marker,dz);
	
		if(exit_status < 0)
			return(exit_status);
	}
	*p = ibufpos;
	*q = obufpos;
	return(CONTINUE);
}

/*************************** GET_INITIAL_PHASES **********************/

int get_initial_phases(float **sampbuf,int *samps_read,int *initial_phase,int *rephase)
{
	int bufpos = 0;
//TW CHANGED
	while(smpflteq(sampbuf[0][bufpos],0.0) && bufpos < samps_read[0])   
		bufpos++;
	if(bufpos >= samps_read[0]) {
		sprintf(errstr,"No siginificant data in first buffer of file 1\n");
		return(DATA_ERROR);
	}
	if(sampbuf[0][bufpos] > 0)   
		initial_phase[0] = 1; 
	else 
		initial_phase[0] = -1;
	bufpos = 0;
//TW CHANGED
	while(smpflteq(sampbuf[1][bufpos],0.0) && bufpos < samps_read[1])   
		bufpos++;
	if(bufpos >= samps_read[1]) {
		sprintf(errstr,"No siginificant data in first buffer of file 2\n");
		return(DATA_ERROR);
	}
	if(sampbuf[1][bufpos] > 0)   
		initial_phase[1] = 1; 
	else 
		initial_phase[1] = -1;
	if(initial_phase[0] != initial_phase[1])
		*rephase = TRUE;
	return(FINISHED);
}

/*************************** WRITE_INVERTED_CYCLE **********************/

int write_inverted_block(float *b,int obuf_endwrite,int *obufpos,int *last_read_marker,dataptr dz)
{
	int exit_status;
	int outremain = obuf_endwrite - dz->buflen;
	register int i = *obufpos;
	register int j = *last_read_marker;
	if(outremain>0) {
		while(i<dz->buflen) 
			dz->sampbuf[2][i++] = (float) -b[j++];	   /*RWD  added cast, also below */
		if((exit_status = write_samps(dz->sampbuf[2],dz->buflen,dz))<0) {
			sprintf(errstr,"Failed to write samps in write_inverted_block()\n");
			return(SYSTEM_ERROR);
		}
		i = 0;
		while(i<outremain) 	
			dz->sampbuf[2][i++] = (float)  -b[j++];
	} else {
		while(i<obuf_endwrite) 	
			dz->sampbuf[2][i++] = (float) -b[j++];
	}
	*obufpos = i;
	*last_read_marker = j;
	return(FINISHED);
}

/*************************** WRITE_THIS_CYCLE **********************/

int write_this_block(float *b,int samps_to_copy,int obuf_endwrite,int *obufpos,int *last_read_marker,dataptr dz)
{
	int exit_status;
	int outremain = obuf_endwrite - dz->buflen;
	int part_writelen;
	if(outremain > 0) {
		part_writelen = dz->buflen - *obufpos;
		memmove((char *)(dz->sampbuf[2] + *obufpos),(char *)(b + *last_read_marker),part_writelen * sizeof(float));
		if((exit_status = write_samps(dz->sampbuf[2],dz->buflen ,dz))<0) {
			sprintf(errstr,"Failed to write samps in write_this_block()\n");
			return(SYSTEM_ERROR);
		}
		*last_read_marker += part_writelen;
		samps_to_copy     -= part_writelen;
		memmove((char *)(dz->sampbuf[2]),(char *)(b + *last_read_marker),samps_to_copy * sizeof(float));
		*obufpos = outremain;
		*last_read_marker += samps_to_copy;
	} else {
		memmove((char *)(dz->sampbuf[2] + *obufpos),(char *)(b + *last_read_marker),samps_to_copy * sizeof(float));
		*obufpos = obuf_endwrite;
		*last_read_marker += samps_to_copy;
	}
	return FINISHED;		/*RWD*/
}
