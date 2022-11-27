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
#include <processno.h>
#include <modeno.h>
#include <arrays.h>
#include <filetype.h>
#include <mix.h>
#include <cdpmain.h>

#include <sfsys.h>
#include <string.h>

#define	START	(0)
#define	LEFT	(0)
#define	RIGHT	(1)

#define MAX_CLIP_DISPLAY (16)

static int  adjust_buffer_status(int n,int *thispos,int *active_bufcnt,dataptr dz);
static int  read_samps_to_an_inbuf(int n,dataptr dz);
static void make_stereo(float *inbuf,dataptr dz);
static int  make_stereo_left_only(float *inbuf,dataptr dz);
static int  make_stereo_right_only(float *inbuf,dataptr dz);
static int  adjust_activebufs_list(int active_bufcnt,dataptr dz);
static int  do_mix(int samps_to_mix,int *position_in_outbuf,int active_bufcnt,int *outbuf_space,dataptr dz);
static void do_level_check(int samps_to_mix,double *d_maxsamp,int *maxloc,int active_bufcnt,
			int *outbuf_space,int *clipsize,int *total_samps_used,int *position_in_outbuf,dataptr dz);
static int  do_silence(int samps_to_mix,int *position_in_outbuf,int *outbuf_space,dataptr dz);
static void do_zero_level(int samps_to_mix,int *position_in_outbuf,
			int *outbuf_space,int *total_samps_used,dataptr dz);
static int  do_mix2(int samps_to_mix,int *position_in_outbuf,int active_bufcnt,int *outbuf_space,dataptr dz);
static void do_the_level(float *olbuf,int *opos,int cnt,double *d_maxsamp,int *maxloc,int *clipsize,
			int total_samps_used,dataptr dz);
static double gain_to_db(double val);
static int  mix_read_samps(float *inbuf,int samps_to_read,int n,dataptr dz);
static int  scale_smps(int n,dataptr dz);

static int clipcnt = 0;

/*************************** MMIX *******************************/

int mmix(dataptr dz)
{
	int exit_status;
	int n;
	int thispos, nextpos, samps_to_mix;
	int position_in_outbuf = 0;
	int active_bufcnt = 0;
	int outbuf_space = dz->buflen;
	display_virtual_time(0L,dz);
	if(sloom) {
		if(dz->iparam[MIX_STRTPOS_IN_ACTION] > 0) {
			fprintf(stdout,"INFO: Skipping initial part of mix.\n");
			fflush(stdout);
		}
	}
	for(n=0;n<dz->iparam[MIX_TOTAL_ACTCNT]-1;n++) {	/* Switch bufs ON or OFF as indicated in act[], & get position */
		if((exit_status = adjust_buffer_status(n,&thispos,&active_bufcnt,dz))<0)
			return(exit_status);					/* and while doing so, count active buffers. */

		nextpos = (dz->act[n+1])->position;				/* Get next position, (from which to calc dur of this mix-chunk) */
		if((exit_status = adjust_activebufs_list(active_bufcnt,dz))<0)
			return(exit_status);					/* Ensure only pointers in ACTIVE bufs are in buf-pointer list */
		if(dz->iparam[MIX_STRTPOS] > 0) {			/* If mix does not start at zero */ 
			if(nextpos <= dz->iparam[MIX_STRTPOS]){	/* update MIX_STRTPOS_IN_ACTION */
				dz->iparam[MIX_STRTPOS_IN_ACTION] = (int)(dz->iparam[MIX_STRTPOS_IN_ACTION] - (nextpos - thispos));	
				continue;							/* and skip action until we reach a valid mix-action */
			}
		}											/* If we're in a valid mix action */
 													/* i.e. there is >zero time between this action and next */
													/* AND time is later than time where mix-action starts */
		if((samps_to_mix = nextpos - thispos - dz->iparam[MIX_STRTPOS_IN_ACTION])>0) {
			if(active_bufcnt==0) {					/* If no buffers are active, fill time with silence. */
				if((exit_status = do_silence(samps_to_mix,&position_in_outbuf,&outbuf_space,dz))<0)
					return(exit_status);
			} else {		   						/* Else, do mix */
				switch(dz->vflag[ALTERNATIVE_MIX]) {
				case(FALSE):   
					if((exit_status = do_mix(samps_to_mix,&position_in_outbuf,active_bufcnt,&outbuf_space,dz))<0)
						return(exit_status);
				  	break;
				case(TRUE):   
					if((exit_status = do_mix2(samps_to_mix,&position_in_outbuf,active_bufcnt,&outbuf_space,dz))<0)
						return(exit_status);
					break;								 
				default:
					sprintf(errstr,"Unknown mixtype. mmix()\n");
					return(PROGRAM_ERROR);					
				}									/* Having got to start of actual mixing, */
			}										/* MIX_STRTPOS_IN_ACTION is set to zero  */
 													/* and NO LONGER  affects calculations.  */
			dz->iparam[MIX_STRTPOS_IN_ACTION] = dz->iparam[MIX_STRTPOS]  = 0;
		}
	}												/* Write any data remaining in output buffer. */
	if(position_in_outbuf > 0)
		return write_samps(dz->sampbuf[OBUFMIX],position_in_outbuf,dz);	
	return FINISHED;
}

/*************************** MIX_LEVEL_CHECK *****************************/

int mix_level_check(double *normalisation, dataptr dz)
{
	int exit_status;
	int n;
	int thispos, nextpos, samps_to_mix;
/* JUNE 2000 --> */
	double dbnormalisation;
/* <-- JUNE 2000 */
	int position_in_outbuf = 0;
//TW REMOVED 'maxsamp' and corrected printing of long (now double) items
	int maxloc  = 0;
	int active_bufcnt = 0;
	int outbuf_space = dz->buflen;
	int clipsize = 0;
	int total_samps_used = 0;
	/* RWD */
	double d_maxsamp = 0.0;

	switch(dz->mode) {
	case(MIX_LEVEL_ONLY):
		break;
	case(MIX_LEVEL_AND_CLIPS):	
	case(MIX_CLIPS_ONLY):	
		if(dz->fp == NULL) {
			sprintf(errstr,"Output textfile not established: mix_level_check()\n");
			return(PROGRAM_ERROR);
		}
		break;
	}
	display_virtual_time(0L,dz);
	for(n=0;n<dz->iparam[MIX_TOTAL_ACTCNT]-1;n++) {
		if((exit_status = adjust_buffer_status(n,&thispos,&active_bufcnt,dz))<0)
			return(exit_status);				
		nextpos = (dz->act[n+1])->position;
		if((exit_status = adjust_activebufs_list(active_bufcnt,dz))<0)
			return(exit_status);				
		if(dz->iparam[MIX_STRTPOS] > 0) {
			if(nextpos <= dz->iparam[MIX_STRTPOS])  {
				dz->iparam[MIX_STRTPOS_IN_ACTION] = (int)(dz->iparam[MIX_STRTPOS_IN_ACTION] - (nextpos - thispos));
				continue;
			}
		}		
		if((samps_to_mix = nextpos - thispos - dz->iparam[MIX_STRTPOS_IN_ACTION])>0) {
			if(active_bufcnt==0)
				do_zero_level(samps_to_mix,&position_in_outbuf,&outbuf_space,&total_samps_used,dz);				
			else
				do_level_check(samps_to_mix,&d_maxsamp,&maxloc,active_bufcnt,
				&outbuf_space,&clipsize,&total_samps_used,&position_in_outbuf,dz);				
			dz->iparam[MIX_STRTPOS_IN_ACTION] = dz->iparam[MIX_STRTPOS]  = 0;
		}
	}
//TW makes code function clear
	*normalisation = F_MAXSAMP / d_maxsamp;
/* JUNE 2000 --> */
	dbnormalisation = gain_to_db(*normalisation);
/* <-- JUNE 2000 */
	switch(dz->mode) {
	case(MIX_LEVEL_ONLY):
		sprintf(errstr,"MAX SAMPLE ENCOUNTERED : %lf at %lf secs\nNORMALISATION REQUIRED : %lf   OR  %.4lfdB\n",
		d_maxsamp,(double)maxloc/(double)dz->out_chans/(double)dz->infile->srate,*normalisation,dbnormalisation);
		break;
	case(MIX_LEVEL_AND_CLIPS):	
		fprintf(dz->fp,"\nMAX SAMPLE ENCOUNTERED : %lf at %lf secs\n",
		d_maxsamp,(double)maxloc/(double)dz->out_chans/(double)dz->infile->srate);
		fprintf(dz->fp,"NORMALISATION REQUIRED : %lf   OR  %.4lfdB\n",
		*normalisation,dbnormalisation);
		if(sloom) {
			fprintf(stdout,"INFO: MAX SAMPLE ENCOUNTERED : %lf at %lf secs\n",
			d_maxsamp,(double)maxloc/(double)dz->out_chans/(double)dz->infile->srate);
			fprintf(stdout,"INFO: NORMALISATION REQUIRED : %lf   OR  %.4lfdB\n",*normalisation,dbnormalisation);
			fflush(stdout);
		}
		break;
	case(MIX_CLIPS_ONLY):	
		if(sloom) {
			if(dz->mode == MIX_CLIPS_ONLY && clipcnt == 0) {
				fprintf(dz->fp,"No clipping.\n");
				fprintf(stdout,"INFO: No clipping.\n");
				fflush(stdout);
			}
		}
		break;
	default:
		sprintf(errstr,"Unknown case in mix_level_check()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/************************** ADJUST_BUFFER_STATUS **************************
 *
 * 	And keep track of number of active buffers (active_bufcnt).
 */

int adjust_buffer_status(int n,int *thispos,int *active_bufcnt,dataptr dz)
{
	int exit_status;
	switch(dz->act[n]->role) {
	case(MIX_ACTION_ON):									/* buffer is being used for 1st time in this mix-action */
		(*active_bufcnt)++;									/* Increment the count of ACTIVE buffers */
		dz->buflist[dz->act[n]->val->bufno]->status = MIX_ACTION_ON;	/* Change buffer status to ON */
		/* fall thro */
	case(MIX_ACTION_REFILL):								/* buffer is being reused OR used for 1st time */
		if((exit_status = read_samps_to_an_inbuf(n,dz))<0)	/* Read_samples into buffer */
			return(exit_status);
		if((exit_status = scale_smps(n,dz))<0)				/* Scale the samples */
			return(exit_status);
		dz->buflist[dz->act[n]->val->bufno]->here = dz->buflist[dz->act[n]->val->bufno]->buf;
		break;												/* Reset location_in_buffer_pointer (here) to buffer start */

	case(MIX_ACTION_OFF):									/* buffer is finished with */
		dz->buflist[dz->act[n]->val->bufno]->status = MIX_ACTION_OFF;	/* Change buffer status to OFF */
		(*active_bufcnt)--;									/* Decrement the number of ACTIVE buffers */
		break;
	default:
		sprintf(errstr,"Unknown case in adjust_buffer_status()\n");
		return(PROGRAM_ERROR);
	}
	*thispos = dz->act[n]->position; 							/* Return position (starttime-in-samps) of the action */
	return(FINISHED);
}

/************************* READ_SAMPS_TO_AN_INBUF ***************************/

int read_samps_to_an_inbuf(int n,dataptr dz)
{
	int exit_status;
	int  samps_to_read = dz->buflen;
	float *inbuf = dz->buflist[dz->act[n]->val->bufno]->buf;
	switch(dz->act[n]->val->stereo) {
	case(MONO):					
	case(STEREO):				
		if((exit_status = mix_read_samps(inbuf,samps_to_read,n,dz))<0)
			return(exit_status);
		break;
	case(MONO_TO_STEREO):			
		samps_to_read /= 2;
		if((exit_status = mix_read_samps(dz->sampbuf[STEREOBUF],samps_to_read,n,dz))<0)
			return(exit_status);
		make_stereo(inbuf,dz);
		dz->ssampsread *= 2;
		break;
	case(MONO_TO_CENTRE):					 
		samps_to_read /= 2;
		if((exit_status = mix_read_samps(dz->sampbuf[STEREOBUF],samps_to_read,n,dz))<0)
			return(exit_status);
		break;
	case(MONO_TO_LEFT):				
		samps_to_read /= 2;
		if((exit_status = mix_read_samps(dz->sampbuf[STEREOBUF],samps_to_read,n,dz))<0)
			return(exit_status);
		if((exit_status = make_stereo_left_only(inbuf,dz))<0)
			return(exit_status);
		dz->ssampsread *= 2;
		break;
	case(MONO_TO_RIGHT):			
		samps_to_read /= 2;
		if((exit_status = mix_read_samps(dz->sampbuf[STEREOBUF],samps_to_read,n,dz))<0)
			return(exit_status);
		if((exit_status = make_stereo_right_only(inbuf,dz))<0)
			return(exit_status);
		dz->ssampsread *= 2;
		break;
	case(STEREO_MIRROR):
	case(STEREO_TO_CENTRE):
	case(STEREO_TO_LEFT):
	case(STEREO_TO_RIGHT):
	case(STEREOLEFT_TO_LEFT):
	case(STEREOLEFT_TO_RIGHT):
	case(STEREOLEFT_PANNED):
	case(STEREORIGHT_TO_LEFT):
	case(STEREORIGHT_TO_RIGHT):
	case(STEREORIGHT_PANNED):
	case(STEREO_PANNED):
		if((exit_status = mix_read_samps(dz->sampbuf[STEREOBUF],samps_to_read,n,dz))<0)
			return(exit_status);
		break;
	default:
		sprintf(errstr,"Unknown case in read_samps_to_an_inbuf()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/****************************** MAKE_STEREO ***************************#
 *
 * Duplicate mono samples to make stereo buffer.
 */

void make_stereo(float *inbuf,dataptr dz)
{
	int m, k;
	for(k=0;k<dz->ssampsread;k++) {
		m = k * 2;
		inbuf[m++] = dz->sampbuf[STEREOBUF][k];
		inbuf[m]   = dz->sampbuf[STEREOBUF][k];
	}
}

/******************** MAKE_STEREO_LEFT_ONLY ***************************#
 *
 * Put mono samples in left ONLY of stereo buffer.
 */

int make_stereo_left_only(float *inbuf,dataptr dz)
{
	int m, k;
	memset((char *)inbuf,0,dz->buflen * sizeof(float));
	for(k=0,m=0;k<dz->ssampsread;k++,m+=2)
		inbuf[m] = dz->sampbuf[STEREOBUF][k];
	return(FINISHED);
}

/******************** MAKE_STEREO_RIGHT_ONLY ***************************#
 *
 * Put mono samples in left ONLY of stereo buffer.
 */

int make_stereo_right_only(float *inbuf,dataptr dz)
{
	int m, k;
	memset((char *)inbuf,0,dz->buflen * sizeof(float));
	for(k=0,m=1;k<dz->ssampsread;k++,m+=2)
		inbuf[m] = dz->sampbuf[STEREOBUF][k];
	return(FINISHED);
}

/*********************** ADJUST_ACTIVEBUFS_LIST **************************
 *
 * Set the activebuf pointers to point only to the active buffers!!
 *
 * NB active_bufcnt has been reset by adjust_buffer_status()
 */

int adjust_activebufs_list(int active_bufcnt,dataptr dz)
{   
	int n, k = 0;
	for(n=0;n<dz->bufcnt;n++) {
		if(dz->buflist[n]->status == MIX_ACTION_ON) {
			dz->activebuf[k] = n;
			dz->activebuf_ptr[k++] = dz->buflist[n]->here;
		}
	}
	if(k>active_bufcnt) {
		sprintf(errstr,"Accounting error: adjust_activebufs_list()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/************************* DO_MIX **********************************
 *
 * This algorithm puts all the samples from buffer 1 into output, then
 * sums in all the samples form buffer 2, and so on. This is fastest
 * routine (slightly) but risks a sample overflowing on the Nth buffer
 * and hence giving CLIPPING, whereas, in do_mix2,  the sample sums are
 * accumulated in a long & hence later sample additions might negate
 * the overflow, negating the clipping in the final output.
 */

int do_mix(int samps_to_mix,int *position_in_outbuf,int active_bufcnt,int *outbuf_space,dataptr dz)
{
	int exit_status;
	int  opos = *position_in_outbuf;
	int   m, start = opos;
	int  n, overflow;
	float *obuf = dz->sampbuf[OBUFMIX];
	float *outptr = obuf+opos;
	if(dz->iparam[MIX_STRTPOS] > 0) {			/* If mix doesn't start at mixlist start, */						
		for(n=0;n<active_bufcnt;n++)			/* and hence most likely not at buffer start, */
			dz->activebuf_ptr[n] += dz->iparam[MIX_STRTPOS_IN_ACTION];
	} 											/* increment pointers in all active_bufs to actual mix start */

	if((overflow = samps_to_mix - (*outbuf_space)) <= 0) {	/* If samps_to_write DOESN'T overflow output buffer */
	
		memmove((char *)outptr,(char *)(dz->activebuf_ptr[0]),samps_to_mix  *sizeof(float));
		dz->activebuf_ptr[0] += samps_to_mix;				/* Increment pointer in this active buf */
		dz->buflist[dz->activebuf[0]]->here += samps_to_mix;/* Increment current_position_in_buffer_pointer also */
		opos += samps_to_mix;								/* Increment position_in_outbuf */
								 							
		for(m=1;m<active_bufcnt;m++) {						/* For each of remaining ACTIVE buffers */
			opos = start;									/* Reset output-buf pointer to start-of-write point */
			for(n=0;n<samps_to_mix;n++) 					/* Add in samples from the other buffers */
				obuf[opos++] += *(dz->activebuf_ptr[m]++);
			dz->buflist[dz->activebuf[m]]->here += samps_to_mix;		
															/* And update current_position_in_buffer_pointers */
		}
		if((*outbuf_space = dz->buflen - opos)<=0) {		/* if output buffer is full */
			if((exit_status = write_samps(obuf,dz->buflen ,dz))<0)
				return(exit_status);						/* write a full buffer */
			*outbuf_space = dz->buflen;						/* and reset available space and buffer position */
			opos = 0;
		}
		*position_in_outbuf = opos;
		return(FINISHED);
															/* IF samps_to_write DOES overflow output buffer */
	} else {												/* which can only happen ONCE, */
															/* as in- & out- bufs are same size */
		if(*outbuf_space>0) {
			memmove((char *)(outptr),(char *)(dz->activebuf_ptr[0]),(*outbuf_space) * sizeof(float));
			dz->activebuf_ptr[0] += *outbuf_space;
			dz->buflist[dz->activebuf[0]]->here += *outbuf_space;	
			opos += *outbuf_space;
			for(m=1;m<active_bufcnt;m++) {
				opos = start;
				for(n=0;n<*outbuf_space;n++)
					obuf[opos++] += *(dz->activebuf_ptr[m]++);
				dz->buflist[dz->activebuf[m]]->here += *outbuf_space;
			}
		}
		if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
			return(exit_status);
		if(overflow) {
			start = opos = 0;
			memmove((char *)obuf,(char *)(dz->activebuf_ptr[0]),overflow*sizeof(float));
			dz->activebuf_ptr[0] += overflow;
			dz->buflist[dz->activebuf[0]]->here += overflow;
			opos = overflow;
			for(m=1;m<active_bufcnt;m++) {
				opos = start;
				for(n=0;n<overflow;n++)
					obuf[opos++] += *(dz->activebuf_ptr[m]++);
				dz->buflist[dz->activebuf[m]]->here += overflow;
			}
		}
	}
	*outbuf_space = dz->buflen - opos;						/* Reset the space-left-in-outbuf */
	*position_in_outbuf = opos;
	return(FINISHED);
}

/************************* DO_LEVEL_CHECK **********************************
 *
 * As do_mix, but finds max sample.
 */

void do_level_check(int samps_to_mix,double *d_maxsamp,int *maxloc,int active_bufcnt,
int *outbuf_space,int *clipsize,int *total_samps_used,int *position_in_outbuf,dataptr dz)
{
	int  opos = *position_in_outbuf;
	int m, start = opos;
	float *active_start = dz->activebuf_ptr[0];
	int n, overflow, k;
	float *olbuf = dz->sampbuf[OBUFMIX];
	
	if(sloom) {
		if(dz->iparam[MIX_STRTPOS_IN_ACTION] > 0) {
			fprintf(stdout,"INFO: Skipping initial part of mix.\n");
			fflush(stdout);
		}
	}
	if(dz->iparam[MIX_STRTPOS] > 0) {
		for(n=0;n<active_bufcnt;n++)
			dz->activebuf_ptr[n] += dz->iparam[MIX_STRTPOS_IN_ACTION];
	} 
	if((overflow = samps_to_mix - (*outbuf_space)) <= 0) {
		for(k=0;k<samps_to_mix;k++)
			olbuf[opos++] = *active_start++;
		dz->buflist[dz->activebuf[0]]->here += samps_to_mix;		
		opos += samps_to_mix;						
		for(m=1;m<active_bufcnt;m++) {					
			opos = start;						
			for(n=0;n<samps_to_mix;n++)					
				olbuf[opos++] += *(dz->activebuf_ptr[m]++);
			dz->buflist[dz->activebuf[m]]->here += samps_to_mix;			
		}
		opos = start;
		do_the_level(olbuf,&opos,samps_to_mix,d_maxsamp,maxloc,clipsize,*total_samps_used,dz);
		*total_samps_used += samps_to_mix;
	} else {								
		if(*outbuf_space > 0) {	
			for(k=0;k<*outbuf_space;k++)
				olbuf[opos++] = *active_start++;
			dz->activebuf_ptr[0] += *outbuf_space;	
			dz->buflist[dz->activebuf[0]]->here += *outbuf_space;		
			for(m=1;m<active_bufcnt;m++) {
				opos = start;
				for(n=0;n<*outbuf_space;n++)
					olbuf[opos++] += *(dz->activebuf_ptr[m]++);
				dz->buflist[dz->activebuf[m]]->here += *outbuf_space;	
			}
			opos = start;
			do_the_level(olbuf,&opos,*outbuf_space,d_maxsamp,maxloc,clipsize,*total_samps_used,dz);
			*total_samps_used += *outbuf_space;
		}
		if(!sloom)
			display_virtual_time(*total_samps_used,dz);
		if(overflow) {
			start = opos = 0;					
			active_start = dz->activebuf_ptr[0];
			for(k=0;k<overflow;k++)
				olbuf[opos++] = *active_start++;
			dz->buflist[dz->activebuf[0]]->here += overflow;		
			for(m=1;m<active_bufcnt;m++) {				
				opos = start;
				for(n=0;n<overflow;n++)
					olbuf[opos++] += *(dz->activebuf_ptr[m]++);
				dz->buflist[dz->activebuf[m]]->here += overflow;		
			}
			opos = start;
			do_the_level(olbuf,&opos,overflow,d_maxsamp,maxloc,clipsize,*total_samps_used,dz);
			*total_samps_used += overflow;
		}
	}
	*outbuf_space = dz->buflen - opos;					
	*position_in_outbuf = opos;
	return;
}

/************************* DO_SILENCE ***********************************/

int do_silence(int samps_to_mix,int *position_in_outbuf,int *outbuf_space,dataptr dz)
{
	int exit_status;
	int opos = *position_in_outbuf;
	int overflow;
	float *obuf = dz->sampbuf[OBUFMIX];
	while((overflow = samps_to_mix - *outbuf_space) > 0) {
		memset((char *)(obuf+opos),0,(*outbuf_space) * sizeof(float));
		if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
			return(exit_status);
		opos = 0;
		*outbuf_space = dz->buflen;
		samps_to_mix = overflow;
	}
	if(samps_to_mix) {
		memset((char *)(obuf+opos),0,samps_to_mix*sizeof(float));
		opos += samps_to_mix;
		*outbuf_space = dz->buflen - opos;
	}
	*position_in_outbuf = opos;
	return(FINISHED);
}

/************************* DO_ZERO_LEVEL ***********************************/

void do_zero_level(int samps_to_mix,int *position_in_outbuf,int *outbuf_space,int *total_samps_used,dataptr dz)
{
	int overflow;
	while((overflow = samps_to_mix - *outbuf_space) > 0) {
		*total_samps_used += dz->buflen;
		display_virtual_time(*total_samps_used,dz);
		*position_in_outbuf = 0;
		*outbuf_space = dz->buflen;
		samps_to_mix = overflow;
	}
	if(samps_to_mix) {
		*total_samps_used += samps_to_mix;
		*position_in_outbuf += samps_to_mix;
		*outbuf_space = dz->buflen - *position_in_outbuf;
	}
}

/******************************** DO_MIX2 ***************************
 *
 * Alternative mix algorithm that SUMS all the buffers for a single sample
 * then puts into output, before proceeding to next sample.
 * In this routine some +ve overflow may be cancelled by a subsequent -ve
 * sample (and vice versa), preventing final signal overflow.
 * However this is slightly slower.
 */

int do_mix2(int samps_to_mix,int *position_in_outbuf,int active_bufcnt,int *outbuf_space,dataptr dz)
{
	int exit_status;
	int m /*, samp_sum*/;
	double d_samp_sum;			/*RWD*/
	int n, overflow;
	float *obuf = dz->sampbuf[OBUFMIX];
	int opos = *position_in_outbuf;
	if(dz->iparam[MIX_STRTPOS] > 0) {
		for(n=0;n<active_bufcnt;n++)
			dz->activebuf_ptr[n] += dz->iparam[MIX_STRTPOS_IN_ACTION];
	}
	if((overflow = samps_to_mix - *outbuf_space) <= 0) {
		for(n=0;n<samps_to_mix;n++) {
			d_samp_sum = *(dz->activebuf_ptr[0]++);
			for(m=1;m<active_bufcnt;m++)
				d_samp_sum += *(dz->activebuf_ptr[m]++);
			obuf[opos++] = (float)d_samp_sum;
		}
		for(m=0;m<active_bufcnt;m++)
			dz->buflist[dz->activebuf[m]]->here += samps_to_mix;
	} else {
		if(*outbuf_space > 0) {	/* APRIL 1995 */
			for(n=0;n<*outbuf_space;n++) {
				d_samp_sum = *(dz->activebuf_ptr[0]++);
				for(m=1;m<active_bufcnt;m++)
					d_samp_sum += *(dz->activebuf_ptr[m]++);
				obuf[opos++] = (float)d_samp_sum;
			}
			for(m=0;m<active_bufcnt;m++)
				dz->buflist[dz->activebuf[m]]->here += *outbuf_space;
		}
		if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
			return(exit_status);
		if(overflow) {
			opos = 0;
			for(n=0;n<overflow;n++) {
				d_samp_sum = *(dz->activebuf_ptr[0]++);
				for(m=1;m<active_bufcnt;m++)
					d_samp_sum += *(dz->activebuf_ptr[m]++);
				obuf[opos++] = (float)d_samp_sum;
			}
			for(m=0;m<active_bufcnt;m++)
				dz->buflist[dz->activebuf[m]]->here += overflow;
		}
	}
	*outbuf_space = dz->buflen - opos;
	*position_in_outbuf = opos;
	return(FINISHED);
}

/***************************** DO_THE_LEVEL ********************************/

void do_the_level
(float *olbuf,int *opos,int cnt,double *d_maxsamp,int *maxloc,int *clipsize,int total_samps_used,dataptr dz)
{
	int clipstart = 0, z /*, thisval*/;
	double d_thisval;  /*RWD */
	int this_opos = *opos;
	switch(dz->mode) {
	case(MIX_LEVEL_ONLY):
		for(z=0;z<cnt;z++) {
			if((d_thisval = fabs(olbuf[this_opos])) > *d_maxsamp) {
				*d_maxsamp = d_thisval;
				*maxloc  = total_samps_used + this_opos;
			}
			this_opos++;
		}
		break;
	case(MIX_CLIPS_ONLY):	
		for(z=0;z<cnt;z++) {
			if(fabs(olbuf[this_opos]) > F_MAXSAMP) {
				if(*clipsize==0)
					clipstart = total_samps_used + this_opos;
				(*clipsize)++;
			} else {
				if(*clipsize > 0) {
					if(sloom) {
						if(clipcnt <= MAX_CLIP_DISPLAY) {
							if(clipcnt == MAX_CLIP_DISPLAY) {
								fprintf(stdout,"INFO: See output data file for more information on clipping.\n");
							} else {
								fprintf(stdout,"INFO: Clip at time %lf secs : sample %d : For %d samples\n",
								(double)clipstart/(double)dz->out_chans/(double)dz->infile->srate,clipstart,*clipsize);
							}
							fflush(stdout);
						}
					}
					fprintf(dz->fp,"Clip at time %lf secs : sample %d : For %d samples\n",
					(double)clipstart/(double)dz->out_chans/(double)dz->infile->srate,clipstart,*clipsize);
					*clipsize = 0;
					clipcnt++;
				}
			}
			this_opos++;
		}
		break;
	case(MIX_LEVEL_AND_CLIPS):	
		for(z=0;z<cnt;z++) {
			if((d_thisval = fabs(olbuf[this_opos])) > *d_maxsamp) {
				*d_maxsamp = d_thisval;
				*maxloc  =  total_samps_used + this_opos;
			}
			if(d_thisval > F_MAXSAMP) {
				if(*clipsize==0)
					clipstart = total_samps_used + this_opos;
				(*clipsize)++;
			} else {
				if(*clipsize>0) {
					if(sloom) {
						if(clipcnt <= MAX_CLIP_DISPLAY) {
							if(clipcnt == MAX_CLIP_DISPLAY) {
								fprintf(stdout,"INFO: See output data file for more information on clipping.\n");
							} else {
								fprintf(stdout,"INFO: Clip at time %lf secs : sample %d : For %d samples\n",
								(double)clipstart/(double)dz->out_chans/(double)dz->infile->srate,clipstart,*clipsize);
							}
							fflush(stdout);
						}
					}
					fprintf(dz->fp,"Clip at time %lf secs : sample %d : For %d samples\n",
					(double)clipstart/(double)dz->out_chans/(double)dz->infile->srate,clipstart,*clipsize);
					*clipsize = 0;
					clipcnt++;
				}
			}
			this_opos++;
		}
		break;
	}
	*opos = this_opos;
	if(sloom)
		display_virtual_time((total_samps_used + this_opos),dz);
}

/************************ GAIN_TO_DB ************************/

double gain_to_db(double val)
{
	val = log10(val);
	val *= 20.0;
	return(val);
}

/*************************** MIX_READ_SAMPS **************************/

int mix_read_samps(float *inbuf,int samps_to_read,int n,dataptr dz)
{
	int thisfile = dz->act[n]->val->ifd;
	if((dz->ssampsread = fgetfbufEx(inbuf, samps_to_read,thisfile,0)) < 0) {
		sprintf(errstr, "Can't read samps from input soundfile %d\n",n+1);
		return(SYSTEM_ERROR);
	}
	return(FINISHED);
}

/************************* SCALE_SMPS **************************/

int scale_smps(int n,dataptr dz)
{
	int m, k;
	double thisscale;
	float *inbuf = dz->buflist[dz->act[n]->val->bufno]->buf;
	float *stereobuf = dz->sampbuf[STEREOBUF];
	int stereo_state = dz->act[n]->val->stereo;
	switch(stereo_state) {
	case(MONO): 					/* Scale the mono source. */
		m = 0;
		thisscale = dz->act[n]->val->llscale;
		if(thisscale==0.0) {
		
			memset((char *)inbuf,0,dz->buflen * sizeof(float));
		}
		else if(thisscale != 1.0){
			while(m<dz->ssampsread) {
				inbuf[m] = (float)(inbuf[m] * thisscale);
				m++;
			}
			break;
		}
		break;
	case(MONO_TO_CENTRE):				/*  work on the 1/2 buflen still in the stereobuf. */ 
		m = 0;							/* Scale it and copy to both L and R in inbuf.     */
		k = 0;
		thisscale = dz->act[n]->val->llscale;
		if(thisscale==0.0) {
		
			memset((char *)inbuf,0,dz->buflen * sizeof(float));
		}
		else if(thisscale == 1.0){
			while(m<dz->ssampsread) {
				inbuf[k]   = stereobuf[m];
				inbuf[k+1] = inbuf[k];
				k+=2;
				m++;
			}
		}
		else{
			while(m<dz->ssampsread) {
				inbuf[k]   = (float)(stereobuf[m] * thisscale);
				inbuf[k+1] = inbuf[k];
				k+=2;
				m++;
			}
			break;
		}
       	dz->ssampsread *= 2;
		break;
	case(STEREO_TO_CENTRE):				/* sum the stereo pair in stereobuf */
 		m = 0;							/* Scale the result (already adjusted for centring) */
		k = 0;							/*and copy to both L and R in inbuf */
		thisscale = dz->act[n]->val->llscale;
		if(thisscale==0.0) {
			memset((char *)inbuf,0,dz->buflen * sizeof(float));
		}
		else if(thisscale==1.0){
			while(m<dz->ssampsread) {
				inbuf[k] = (float)(stereobuf[m]+stereobuf[m+1]);
				inbuf[k+1] = inbuf[k];
				k+=2;
				m+=2;
			}
		}
		else{
			while(m<dz->ssampsread) {
				inbuf[k] = (float)((stereobuf[m]+stereobuf[m+1]) * thisscale);
				inbuf[k+1] = inbuf[k];
				k+=2;
				m+=2;
			}
			break;
		}
		break;
	case(MONO_TO_LEFT):				/*  scale left of stereo source only: (right is preset to zero!). */
		thisscale = dz->act[n]->val->llscale;
		if(thisscale==0.0) {
			memset((char *)inbuf,0,dz->buflen * sizeof(float));
		}
		else if(thisscale!=1.0){
			m = LEFT;
			while(m<dz->ssampsread) {
				inbuf[m] = (float)(inbuf[m] * thisscale);
				m += 2;
			}			
		}
		break;
	case(MONO_TO_RIGHT):				/* scale right of stereo source only: (left is preset to zero!).*/
		thisscale = dz->act[n]->val->rrscale;
		if(thisscale==0.0) {
			memset((char *)inbuf,0,dz->buflen * sizeof(float));
		}
		else if(thisscale != 1.0){
			m = RIGHT;
			while(m<dz->ssampsread) {
				inbuf[m] = (float)(inbuf[m] * thisscale);
				m += 2;
			}			
		}
		break;
	case(MONO_TO_STEREO):			/* scale each channel of stereo_ified source.*/
	case(STEREO): 					/* scale each channel of stereo source.*/
		m = LEFT;
		thisscale = dz->act[n]->val->llscale;
		if(thisscale==0.0) {
			while(m<dz->ssampsread) {
				inbuf[m] = 0.0f;
				m += 2;
			}
		}
		else if(thisscale != 1.0){
			while(m<dz->ssampsread) {
				inbuf[m] = (float)(inbuf[m] * thisscale);
				m += 2;
			}			
		}
		m = RIGHT;
		thisscale = dz->act[n]->val->rrscale;
		if(thisscale==0.0) {
			while(m<dz->ssampsread) {
				inbuf[m] = 0.0f;
				m += 2;
			}
		}
		else if(thisscale != 1.0){
			while(m<dz->ssampsread) {
				inbuf[m] = (float)(inbuf[m] * thisscale);
				m += 2;
			}		
		}
		break;
	case(STEREO_MIRROR):				/* Scale L channel (in stereobuf) & copy to R.	   */
 										/* Then scale R channel (in stereobuf) & copy to L */
		m = RIGHT;	/* put to right */
		k = LEFT;	/* the scaled left channel */
		thisscale = dz->act[n]->val->llscale;
		if(thisscale==0.0) {
			while(m<dz->ssampsread) {
				inbuf[m] = 0.0f;
				m += 2;
			}
		}
		else if(thisscale==1.0){
			while(m<dz->ssampsread) {
				inbuf[m] = stereobuf[k];
				m += 2;
				k += 2;
			}			
		}
		else{
			while(m<dz->ssampsread) {
				inbuf[m] = (float)(stereobuf[k] * thisscale);
				m += 2;
				k += 2;
			}			
		}
		m = LEFT;	/* put to left */
		k = RIGHT;	/* the scaled right channel */	
		thisscale = dz->act[n]->val->rrscale;
		if(thisscale==0.0) {
			while(m<dz->ssampsread) {
				inbuf[m] = 0.0f;
				m += 2;
			}
		}
		else if(thisscale==1.0){
			while(m<dz->ssampsread) {
				inbuf[m] = stereobuf[k];
				m += 2;
				k += 2;
			}
		}
		else {
			while(k<dz->ssampsread) {
				inbuf[m] = (float)(stereobuf[k] * thisscale);
				m += 2;
				k += 2;
			}			
		}
		break;
	case(STEREO_PANNED): 				/* each stereo INPUT channel contributes to each stereo OUTPUT channel. */
										/* So copy scaled version of first to each outchan, */
										/* then do same to 2nd inchans, ADDING it into the outbuf */
		m = 0;
		thisscale = dz->act[n]->val->llscale;
		while(m<dz->ssampsread) {
			inbuf[m] =(float)(stereobuf[m] * thisscale);
			m += 2;
		}
		m = 0;
		thisscale = dz->act[n]->val->lrscale;
		while(m<dz->ssampsread) {
			inbuf[m+1] = (float)(stereobuf[m] * thisscale);
			m += 2;
		}
		m = 1;
		thisscale = dz->act[n]->val->rlscale;
		while(m<dz->ssampsread) {
			inbuf[m-1] = (float)(inbuf[m-1] + (stereobuf[m] * thisscale));
			m += 2;
		}
		m = 1;
		thisscale = dz->act[n]->val->rrscale;
		while(m<dz->ssampsread) {
			inbuf[m] = (float)(inbuf[m]  + (stereobuf[m] * thisscale));
			m += 2;
		}
		break; 
	case(STEREO_TO_LEFT):				/* both channels are at same level, but both panned to left. */
 	case(STEREO_TO_RIGHT):				/* Preset outbuf to zero, then sum each pair of stereo channels, */
										/* scale result and write to left. */
										/* For STEREO_TO_RIGHT Ditto , but write to right. */
	
		if(stereo_state==STEREO_TO_LEFT) {
			m = START;	/* read from start */
			k = LEFT;	/* write to left */
			thisscale = dz->act[n]->val->llscale;
		} else {
			m = START;	/* read from start */
			k = RIGHT;	/* write to right */
			thisscale = dz->act[n]->val->rrscale;
		}
		memset((char *)inbuf,0,dz->buflen * sizeof(float));
		
		if(thisscale==0.0) {
			;
		}
		else if(thisscale==1.0){
			while(m<dz->ssampsread) {
				inbuf[k] = (float)(stereobuf[m]+stereobuf[m+1]);
				k+=2;
				m+=2;
			}
		}
		else{
			while(m<dz->ssampsread) {
				inbuf[k] = (float)((stereobuf[m]+stereobuf[m+1]) * thisscale);
				k+=2;
				m+=2;
			}			
		}
		break;
	case(STEREOLEFT_TO_LEFT):				/* for STEREOLEFT_TO_LEFT there is no right channel output or input. */
 	case(STEREORIGHT_TO_RIGHT):				/* zero outbuf, & copy scaled left channel to left outchan */
											/* For STEREORIGHT_TO_RIGHT there is no left channel output or input. */
 											/* zero outbuf, & copy scaled right channel to right output */
 	
		if(stereo_state==STEREOLEFT_TO_LEFT) {
			m = LEFT;	/* write from left to left */
			thisscale = dz->act[n]->val->llscale;
		} else {
			m = RIGHT;	/* write from right to right */
			thisscale = dz->act[n]->val->rrscale;
		}
		memset((char *)inbuf,0,dz->buflen * sizeof(float));
		if(thisscale==0.0) {
			;
		}
		else if(thisscale==1.0){
			while(m<dz->ssampsread) {
				inbuf[m] = stereobuf[m];
				m+=2;
			}
		}
		else {
			while(m<dz->ssampsread) {
				inbuf[m] = (float)(stereobuf[m] * thisscale);
				m+=2;
			}		
		}
		break;
	case(STEREOLEFT_TO_RIGHT):				/* For STEREOLEFT_TO_RIGHT there is no L chan output or R chan input. */
 	case(STEREORIGHT_TO_LEFT):				/* Zero buffer, & copy scaled left chan to right */
 											/* For STEREORIGHT_TO_LEFT there is no R chan output or L chan input. */
 											/* Zero buffer, & copy scaled right chan to left */
	
		if(stereo_state==STEREOLEFT_TO_RIGHT) {
			m = LEFT;	/* write from left to right */
			k = RIGHT;
			thisscale = dz->act[n]->val->rrscale;
		} else {
			m = RIGHT;	/* write from right to left */
			k = LEFT;
			thisscale = dz->act[n]->val->llscale;
		}
		memset((char *)inbuf,0,dz->buflen * sizeof(float));
		if(thisscale==0.0) {
			;
		}
		else if(thisscale==1.0){
			while(m<dz->ssampsread) {
				inbuf[k] = stereobuf[m];
				m+=2;
				k+=2;
			}
		}
		else{
			while(m<dz->ssampsread) {
				inbuf[k] = (float)(stereobuf[m] * thisscale);
				m+=2;
				k+=2;
			}		
		}
		break;
	case(STEREOLEFT_PANNED):				/* For STEREOLEFT_PANNED there is no R chan input. */
 	case(STEREORIGHT_PANNED):				/* Copy left input, left_scaled, to left. */
 											/* Then copy left input right_scaled to right. */
 											/* For STEREORIGHT_PANNED there is no L chan input. */
 											/* Copy right input, right_scaled, to right. */
 											/* Then copy right input left_scaled to left.*/
	
		if(stereo_state==STEREOLEFT_PANNED) {
			m = LEFT;	/* left to left */
			thisscale = dz->act[n]->val->llscale;
		} else {
			m = RIGHT;	/* right to right */
			thisscale = dz->act[n]->val->rrscale;
		}
		if(thisscale==0.0) {
			while(m<dz->ssampsread) {
				inbuf[m] = 0.0f;
				m+=2;
			}
		}
		else if(thisscale==1.0){
			while(m<dz->ssampsread) {
				inbuf[m] = stereobuf[m];
				m+=2;
			}
		}
		else{
			while(m<dz->ssampsread) {
				inbuf[m] = (float)(stereobuf[m] * thisscale);
				m+=2;
			}			
		}
		if(stereo_state==STEREOLEFT_PANNED) {
			m = LEFT;	/* left to right */
			k = RIGHT;
			thisscale = dz->act[n]->val->rrscale;
		} else {
			m = RIGHT;	/* right to left */
			k = LEFT;
			thisscale = dz->act[n]->val->llscale;
		}
		if(thisscale==0.0) {
			while(m<dz->ssampsread) {
				inbuf[m] = 0.0f;
				m+=2;
			}
		}
		else if(thisscale==1.0){
			while(m<dz->ssampsread) {
				inbuf[k] = stereobuf[m];
				m+=2;
				k+=2;
			}
		}
		else{
			while(m<dz->ssampsread) {
				inbuf[k] = (float)(stereobuf[m] * thisscale);
				m+=2;
				k+=2;
			}		
		}
		break;
	default:
		sprintf(errstr,"Unknown case in scale_smps()\n");
		return (PROGRAM_ERROR);
	}
	return(FINISHED);
}
