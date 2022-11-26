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
#include <envel.h>
#include <cdpmain.h>
//TW UPDATE
#include <logic.h>
#include <limits.h>
#include <osbind.h>

#include <sfsys.h>


#define INBUF_START		(0)
#define OUTBUF_START	(1)
#define OUTBUF_END		(2)

static int  init_outbuf
	(int startsamp,int *is_simple,int *write_offset,int *samp_offset,int *samps_left_to_process,dataptr dz);
static int  init_brkpnts
	(int *sampno,double *starttime,double *gain,double **endbrk,double **nextbrk,
	 double *nextgain,double *gain_step,double *gain_incr,int *nextbrk_sampno,dataptr dz);
static int  advance_brkpnts
	(double starttime,double *gain,double *endbrk,double **nextbrk,double *nextgain,
	 double *gain_step,double *gain_incr,int *nextbrk_sampno,dataptr dz);
static int  calc_samps_to_process
	(int *startsamp,int *samps_left_to_process,dataptr dz);
static int  simple_envel_processing
	(int *samps_left_to_process,double starttime,double *endbrk,double **nextbrk,double *nextgain,
	 int *sampno,double *gain,double *gain_step,double *gain_incr,int *nextbrk_sampno,dataptr dz);
static int  buf_staggered_envel_processing
	(int *samps_left_to_process,int samp_offset,int write_offset,double starttime,double *endbrk,double **nextbrk,
	 double *nextgain,int *sampno,double *gain,double *gain_step,double *gain_incr,int *nextbrk_sampno,dataptr dz);
static int  read_partbuf_samps
	(int samps_to_read,int *samps_left_to_process, dataptr dz);
static int  do_envelope
	(int samp_cnt,double starttime,double *endbrk,double **nextbrk,double *nextgain,
	 int *sampno,double *gain,double *gain_step,double *gain_incr,int *nextbrk_sampno,dataptr dz);
static int  read_ssamps(int samps_to_read,int *samps_left_to_process,dataptr dz);
static int  do_simple_read(int *samps_left_to_process,int bufsize,dataptr dz);
static int  do_offset_read(int *samps_left_to_process,int samp_offset,int bufsize,dataptr dz);
static int  skip_into_file(int startsamp,int extended_buf_sampsize,int *write_offset,int *samp_offset,dataptr dz);

/******************************* APPLY_BRKPNT_ENVELOPE *******************************/

int apply_brkpnt_envelope(dataptr dz)
{
	int exit_status;
	int startsamp, samps_left_to_process;
	int is_simple;
	int write_offset, samp_offset, sampno, nextbrk_sampno;
	double starttime, gain, nextgain, gain_step, gain_incr;
	double *endbrk, *nextbrk; 
	if((exit_status = calc_samps_to_process(&startsamp,&samps_left_to_process,dz))<0)
		return(exit_status);
	if((exit_status = init_outbuf(startsamp,&is_simple,&write_offset,&samp_offset,&samps_left_to_process,dz))<0)
		return(exit_status);
	if((exit_status = init_brkpnts
	(&sampno,&starttime,&gain,&endbrk,&nextbrk,&nextgain,&gain_step,&gain_incr,&nextbrk_sampno,dz))<0)
		return(exit_status);
	exit_status = FINISHED;
	if(is_simple)
		exit_status = simple_envel_processing
		(&samps_left_to_process,starttime,endbrk,&nextbrk,&nextgain,
		&sampno,&gain,&gain_step,&gain_incr,&nextbrk_sampno,dz);
	else
		exit_status = buf_staggered_envel_processing
		(&samps_left_to_process,samp_offset,write_offset,starttime,endbrk,&nextbrk,
		&nextgain,&sampno,&gain,&gain_step,&gain_incr,&nextbrk_sampno,dz);

	return(exit_status);
}

/**************************** INIT_OUTBUF *******************************
 *
 *	|-------------extended-buffer-size-----------|
 *       X    
 *	     |-------------buffer-size---------------|
 * 	     		    INBUF		    
 *	|----|----|----|----|----|----|----|----|----|
 *	|    |					     				 |
 *	   | |					     			   | |
 	  write					     			(=write
 	  offset				     			 offset)
 *	   | |					     			   | |
 *	|  |					   				   |
 *  samp					   				   |
 * offset					   				   |
 *	|  |					   				   |
 *	   |----|----|----|----|----|----|----|----| 
 *			    OUTBUF
 *	   |-------------buffer-size---------------|
 *
 * IF the brkpoint table starts at 0,
 * we can use THE SAME buffer for input and output.
 * 		OTHERWISE.........
 * (8)	First extimate of the number of samples in the input file which
 *		we can skip, i.e. the number of complete extended-buffer-fulls
 *		(see diagram) we can read before reaching first significant sample.
 * (9)	After counting out these, we can assess how many COMPLETED sectors
 *		are still to be read and ignored.
 * (10)	We can now add the number of samples in these sectors to our first
 *		estimate of samples to be skipped.
 * (11)	The extent to which the OUTPUT buffer will be offset from the
 *		INPUT buffer, (samp_offset) is given by the remaining part-sector of samps.
 *		(see diagram).
 * (12)	After the initial write, we will WRITE to a block 1 SECTOR later,
 *		(X in diagram), so the WRITE-OFFSET is as shown.
 * (13)	We now SEEK an exact number of sectors in the input file (thus
 *		discarding these samps from our processing).
 * (14)	If the samp_offset is ZERO, then we input and output buffers are
 *		the same as at 4,5,6.
 * (16)	Otherwise, fill the EXTENDED buffer (see diagram).
 * (17)	Move the address of future read-ins (inbuf) 1 sector forward (to X in diagram).
 * (18)	Set the address of the read-out buffer at the correct offset.
 * (19)	Set the flag for the more difficult case.
 * (20)	Decrement number of samps left to process.
 *		If this takes samps_left below zero, this means we have
 *		read a whole sector in place of a part-sector, so we must
 *		readjust the value of samps_read!!
 */

int init_outbuf(int startsamp,int *is_simple,int *write_offset,
				int *samp_offset,int *samps_left_to_process,dataptr dz)
{
	int exit_status;
	int extended_buf_sampsize = dz->buflen;
	int shsecsize = F_SECSIZE;
	/*RWD: set the  vars*/
	*samp_offset = *write_offset = 0;

	if(startsamp==0) {											/* If we start at beginning */
		dz->sampbuf[OUTBUF_START] = dz->sampbuf[INBUF_START];	/* inbuf and outbuf coincide */
		if((exit_status = do_simple_read(samps_left_to_process,dz->buflen,dz))<0)
			return(exit_status);								/* setup simple buffering option */
		*is_simple = TRUE;											
	} else {													
 		if((exit_status = skip_into_file(startsamp,extended_buf_sampsize,write_offset,samp_offset,dz))<0)
			return(exit_status);	/* calculate sectors to skip into file, and any part-sector offset, and skip */
		if(*samp_offset==0) {									 /* If no offset */
			dz->sampbuf[OUTBUF_START] = dz->sampbuf[INBUF_START];/* inbuf and outbuf coincide */				
			if((exit_status = do_simple_read(samps_left_to_process,dz->buflen,dz))<0)
				return(exit_status);
			*is_simple = TRUE;			
		} else {												 /*  Otherwise there is an offset */
			if((exit_status = do_offset_read(samps_left_to_process,*samp_offset,
				     /*dz->bigbufsize + SECSIZE*/dz->buflen,dz))<0)
				return(exit_status);							 /* read initial LARGE buffer */							
			dz->sampbuf[OUTBUF_START] = dz->sampbuf[INBUF_START] + *samp_offset;
																 /* Set outbuf (write) position */
			dz->sampbuf[OUTBUF_END]  = dz->sampbuf[OUTBUF_START] + dz->buflen;	
																 /* Set endpointer of outbuf */
			dz->sampbuf[INBUF_START] += shsecsize;				 /* Set next read position for inbuf */
																 /* to permit correct wrap_around of part_sector */
			*is_simple = FALSE;									 /* Flag staggerd-buffers option */
		}
	}
	return(FINISHED);
}

/**************************** INIT_BRKPNTS *******************************
 * NB Here samples are taken to be a short for mono, 2 shorts for stereo, etc.
 * and times are measured in such 'samples'.
 */

int init_brkpnts(int *sampno,double *starttime,double *gain,double **endbrk,double **nextbrk,
double *nextgain,double *gain_step,double *gain_incr,int *nextbrk_sampno,dataptr dz)
{
	double nexttime, time_from_start;
	int paramno = dz->extrabrkno;
	*endbrk = dz->brk[paramno] + ((dz->brksize[paramno]-1) * 2);
	*sampno    = 0;
	*starttime = dz->brk[paramno][0];
	*gain      = dz->brk[paramno][1];
	*nextbrk   = dz->brk[paramno] + 2;
	nexttime   = **nextbrk;
	*nextgain  = *((*nextbrk) + 1);
	*gain_step = *nextgain - *gain;
	time_from_start = nexttime - *starttime;
/* OCT 1996 */
	if((*nextbrk_sampno  = (int)round(time_from_start * (double)dz->infile->srate))<0) {
		sprintf(errstr,"Impossible brkpoint time: (%.2lf secs)\n",time_from_start);
		return(PROGRAM_ERROR);
	}
	*gain_incr = (*gain_step)/(double)(*nextbrk_sampno);
	return(FINISHED);
}

/**************************** ADVANCE_BRKPNTS *****************************
 * NB Here samples are taken to be a short for mono, 2 shorts for stereo, etc.
 * and times are measured in such 'samples'.
 */

int advance_brkpnts(double starttime,double *gain,double *endbrk,double **nextbrk,
double *nextgain,double *gain_step,double *gain_incr,int *nextbrk_sampno,dataptr dz)
{
	double  nexttime, time_from_start;
	int   lastbrk_sampno, sampdist;
	if(*nextbrk!=endbrk) {
		*nextbrk   += 2;
		*gain      = *nextgain;
		nexttime   = **nextbrk;
		*nextgain  = *((*nextbrk) + 1);
		*gain_step = *nextgain - *gain;
		lastbrk_sampno = *nextbrk_sampno;
		time_from_start = nexttime - starttime;
/* OCT 1996 */
		if((*nextbrk_sampno  = (int)round(time_from_start * dz->infile->srate))<0) {
			sprintf(errstr,"Impossible brkpoint time: (%.2lf secs)\n",time_from_start);
			return(PROGRAM_ERROR);
		}
		sampdist   = *nextbrk_sampno - lastbrk_sampno;
		*gain_incr = (*gain_step)/(double)sampdist;
	}
	return(FINISHED);
}

/************************ CALC_SAMPS_TO_PROCESS ****************************
 *
 * (1)	Find time of final breakpoint value.
 * (a)	Find the sample position of the brkpoint-table's starting time.
 * (b)	Find its samp position.
 * (2)	Find this time in samples (here samples mean short samples).
 * (3)  The last samp to be processed is either the samp corresponding
 *	to the last sample-time in brkpnt file OR the end of the input file.
 * (4)	Number of samps-to-process = endsamp-time - startsamp-time.
 * (5)	This also gives the required size of output file.
 */

int calc_samps_to_process(int *startsamp,int *samps_left_to_process,dataptr dz)
{
	int paramno = dz->extrabrkno;
	int endbrk_samptime, endsamp, start_samp;
	double lastbrktime, firstbrktime;
	double infiledur = (double)(dz->insams[0]/dz->infile->channels)/(double)dz->infile->srate;

	lastbrktime = *(dz->brk[paramno] + ((dz->brksize[paramno] - 1) * 2));
	if(flteq(lastbrktime,infiledur)) {
		endsamp = dz->insams[0];
	} else {
		endbrk_samptime  = round(lastbrktime * (double)dz->infile->srate);
		endbrk_samptime *= dz->infile->channels;
		endsamp          = min((int)endbrk_samptime,dz->insams[0]);
	}
	firstbrktime = *(dz->brk[paramno]);
	*startsamp   = round(firstbrktime * (double)dz->infile->srate);
	*startsamp  *= dz->infile->channels;
	if((start_samp = *startsamp ) >= dz->insams[0]) {
		sprintf(errstr,"envelope begins after end of file: can't proceed.\n");
		return(DATA_ERROR);
	}	
	*samps_left_to_process = endsamp - start_samp;
	return(FINISHED);
}

/*********************** SIMPLE_ENVEL_PROCESSING ********************************
 *
 * Simple case, input and output buffers coincide.
 *
 * (1)	If we read beyond number of samps we anticipated, this means
 * 	we have read to the end of a sector, where we need a part-sector,
 * 	so we must adjust value of actual samps read!!!
 */

int simple_envel_processing
(int *samps_left_to_process,double starttime,double *endbrk,double **nextbrk,double *nextgain,
int *sampno,double *gain,double *gain_step,double *gain_incr,int *nextbrk_sampno,dataptr dz)
{
	int exit_status;
	while(*samps_left_to_process > 0) {
		if((exit_status = do_envelope
		(dz->buflen,starttime,endbrk,nextbrk,nextgain,sampno,gain,gain_step,gain_incr,nextbrk_sampno,dz))<0)
			return(exit_status);
		if((exit_status = write_samps(dz->sampbuf[OUTBUF_START],dz->buflen,dz))<0)
			return(exit_status);
		if((exit_status = do_simple_read(samps_left_to_process,dz->buflen,dz))<0)
			return(exit_status);
	}
	if((exit_status = do_envelope
	(dz->ssampsread,starttime,endbrk,nextbrk,nextgain,sampno,gain,gain_step,gain_incr,nextbrk_sampno,dz))<0)
		return(exit_status);
	if(dz->ssampsread > 0)
		return write_samps(dz->sampbuf[OUTBUF_START],dz->ssampsread,dz);
	return(FINISHED);
}

/*********************** BUF_STAGGERED_ENVEL_PROCESSING ********************************
 *
 * Non-simple case, input  and output buffers do not coincide.
 *
 *	     ___________tail is copied thus_________
 *	    |					    				|
 *	    |					    				|
 * 	    v		    INBUF		    			^
 *	|----|----|----|----|----|----|----|----|----|
 *	     |					     				 |
 *	   | |					   				   | |
 *     write_offset				   				tail
 *	   | |					   				   | |
 *	   |					   				   |
 *	   |----|----|----|----|----|----|----|----| 
 *			    OUTBUF
 *
 * (1)	If we are inside the loop, there are still samps left to read
 *		from input file, so this input buffer must be FULL.
 *		This means we can WRITE a full output-buffer.
 * (2)	Copy the tail-portion of the input buffer (that falls outside the
 *		end of the output buffer (see diagram)), to the start of the output
		buffer (see diagram).
 * (3)	Read samps to inbuf location, decrementing no of samps left to read.
 * (4)	If first time, (No buffers written: samps were read into VERY START of buf) 
 *		no. of samps to write equals samps_read 
 *		MINUS those not needed, in samp_offset area.....
 * (5)	After first time, (Buffers laready writ: samps now read into the displaced inbuf)
 *		number of samps to write equals those read
 *		PLUS those already in start of outbuffer, haing been copied back into the write_offset segment.
 * (6)	Do envelope on the incomplete (or exactly full) buffer.
 * (7)	Write this.
 */

int buf_staggered_envel_processing
(int *samps_left_to_process,int samp_offset,int write_offset,double starttime,double *endbrk,double **nextbrk,
double *nextgain,int *sampno,double *gain,double *gain_step,double *gain_incr,int *nextbrk_sampno,dataptr dz)
{
	int exit_status;
	int samps_to_write_here;
	int  firstime = TRUE;
	while(*samps_left_to_process > 0) {
		firstime = FALSE;
		if((exit_status = do_envelope
		(dz->buflen,starttime,endbrk,nextbrk,nextgain,sampno,gain,gain_step,gain_incr,nextbrk_sampno,dz))<0)
			return(exit_status);

		if((exit_status = write_samps(dz->sampbuf[OUTBUF_START],dz->buflen,dz))<0)
			return(exit_status);										/* 1 */
		memmove((char *)dz->sampbuf[OUTBUF_START],(char *)dz->sampbuf[OUTBUF_END],write_offset * sizeof(float));
		if((exit_status = do_simple_read(samps_left_to_process,dz->buflen,dz))<0)		
			return(exit_status);										/* 3 */
	}
	if(firstime)
		samps_to_write_here = dz->ssampsread - samp_offset;				/* 4 */
	else 
		samps_to_write_here = dz->ssampsread + write_offset;			/* 5 */
	if((exit_status = do_envelope										/* 6 */
	(samps_to_write_here,starttime,endbrk,nextbrk,nextgain,sampno,gain,gain_step,gain_incr,nextbrk_sampno,dz))<0)
		return(exit_status);
																	 	/* 7 */
	if(samps_to_write_here > 0)
		return write_samps(dz->sampbuf[OUTBUF_START],samps_to_write_here,dz);
	return(FINISHED);
}

/**************************** READ_PARTBUF_SAMPS *****************************/

int read_partbuf_samps(int samps_to_read,int *samps_left_to_process, dataptr dz)
{
	if((dz->ssampsread = fgetfbufEx(dz->sampbuf[INBUF_START], samps_to_read,dz->ifd[0],0)) < 0)  {
		sprintf(errstr,"Can't read samples from input soundfile: read_partbuf_samps()\n");
		return(SYSTEM_ERROR);
	}
	if(dz->ssampsread < samps_to_read) {
		sprintf(errstr,"Error in buffering arithmetic: read_partbuf_samps()\n");
		return(PROGRAM_ERROR);
	}
	*samps_left_to_process -= dz->ssampsread;
	return(FINISHED);
}
							  
/************************* DO_ENVELOPE ********************************
 *
 * (1)	endsampno (absolute numbering) is the final sample to be treated in this call.
 *
 *	  While ever the quit-flag is not set...
 * (2)	if the sample number of the next break-pnt falls before or at the end
 *		of this function call...
 *		Set the 'change' flag, which will cause envelope breakpoints to be
 *		advanced.
 * (3)	set end of the inner-loop pass to this next brkpnt.
 * (4)	If the brkpnt falls exactly at end of pass, set the 'quit' flag.
 *		Otherwise quit remains FALSE, and the outer loop will be recalled (after
 *		breakpoints have been advanced).
 * (5)	Otherwise, the next breakpnt falls beyond the end of this function call.
 *		So don't advance the breakpoints (change = FALSE), sample at end of inner
 *		loop (this_endsamp) is the sample at end of this call-to-function
 *		(endsampno), AND  set flag to quit at end of inner loop.
 * (6)	Counter for inner loop is this-endsamp minus current position (sampno).
 * (7)	Inner loop, get gain for each sample, and increment samples accordingly.
 * (8)	Increment the absolute count of processed samples (sampno).
 * (9)	If change flag is set, reset gain and advance breakpoints.
 * (10)	Retain the enveloping gain current at end of buffer.
 */

int do_envelope
(int samp_cnt,double starttime,double *endbrk,double **nextbrk,double *nextgain,
int *sampno,double *gain,double *gain_step,double *gain_incr,int *nextbrk_sampno,dataptr dz)
{
	int    exit_status;
	int   n, m, sampcnt;
	int   endsampno, this_endsamp, this_sampcnt;
	float  *obuf = dz->sampbuf[OUTBUF_START];
	float  *obufend = obuf + dz->buflen;
	int    quit = FALSE, change;
	double   val;
	double this_gain      = *gain;
	double this_gain_incr = *gain_incr;
	double this_gain_step = *gain_step;
	sampcnt = samp_cnt / dz->infile->channels;	/* 'stereo'-samples to process */
	endsampno = *sampno + sampcnt;				/* 1 */
	if(sampcnt * dz->infile->channels > dz->buflen) {
		sprintf(errstr,"Buffering anomaly: do_envelope()\n");
		return(PROGRAM_ERROR);
	}
	while(quit==FALSE) {						
		if(*nextbrk_sampno <= endsampno) {		/* 2 */
			change = TRUE;
			this_endsamp = *nextbrk_sampno;		/* 3 */
			if(*nextbrk_sampno==endsampno)		
				quit = TRUE;					/* 4 */
		} else {
			change = FALSE;						/* 5 */
			this_endsamp = endsampno;
			quit = TRUE;
		}
		this_sampcnt = this_endsamp - *sampno;	/* 6 */
		if(obuf + (this_sampcnt * dz->infile->channels) > obufend) {
			sprintf(errstr,"array overrun: do_envelope()\n");
			return(PROGRAM_ERROR);
		}
		if(flteq(this_gain,1.0) && flteq(this_gain_step,0.0))
			obuf += this_sampcnt * dz->infile->channels;				/* no modification of sound */
		else {
			for(n=0;n<this_sampcnt;n++) {		/* 7 */
				this_gain += this_gain_incr;
				for(m=0;m<dz->infile->channels;m++) {				
					val = /*round*/((double)(*obuf) * this_gain);
					*obuf = (float)val;
					obuf++;
				}
			}
		}
		*sampno += this_sampcnt;				/* 8 */
		if(change) {							/* 9 */
			if((exit_status = advance_brkpnts
			(starttime,gain,endbrk,nextbrk,nextgain,gain_step,gain_incr,nextbrk_sampno,dz))<0)
				return(exit_status);
			this_gain = *gain;
			this_gain_incr = *gain_incr;
			this_gain_step = *gain_step;
		}
	}
	*gain = this_gain;							/* 10 */
	return(FINISHED);
}

/************************* READ_SSAMPS ******************************/

int read_ssamps(int samps_to_read,int *samps_left_to_process,dataptr dz)
{
	if((dz->ssampsread = fgetfbufEx(dz->sampbuf[INBUF_START], samps_to_read,dz->ifd[0],0)) < 0) {
		sprintf(errstr,"Can't read samples from input soundfile: read_ssamps()\n");
		return(SYSTEM_ERROR);
	}
	if(dz->ssampsread!=samps_to_read) {
		sprintf(errstr,"Error in buffering arithmetic: read_ssamps()\n");
		return(PROGRAM_ERROR);
	}
	*samps_left_to_process -= dz->ssampsread;
	return(FINISHED);
}

/******************************** DO_SIMPLE_READ **********************************/

int do_simple_read(int *samps_left_to_process,int bufsize,dataptr dz)
{
	int exit_status;
	if(*samps_left_to_process < bufsize)
		exit_status = read_partbuf_samps(*samps_left_to_process,samps_left_to_process,dz);
	else
		exit_status = read_ssamps(bufsize,samps_left_to_process,dz);
	return(exit_status);
}

/******************************** DO_OFFSET_READ **********************************/

int do_offset_read(int *samps_left_to_process,int samp_offset,int bufsize,dataptr dz)
{
	int exit_status;
	if(*samps_left_to_process < bufsize) {
		exit_status = read_partbuf_samps(*samps_left_to_process + samp_offset,samps_left_to_process,dz);
		*samps_left_to_process = 0;
	} else {
		exit_status = read_ssamps(bufsize,samps_left_to_process,dz);
		*samps_left_to_process += samp_offset;	/* some of samps-read are not part of samps-to-process */
	}
	return(exit_status);
}

/******************************** SKIP_INTO_FILE **********************************/
//RWD ugh! unless this is simply a sndseek to startsamp....
//TW unfortunately, it's intrinsically tied up with old buffering scheme
//#ifdef NOTDEF
int skip_into_file(int startsamp,int extended_buf_sampsize,int *write_offset,int *samp_offset,dataptr dz)
{
	int skipsamps=(startsamp/extended_buf_sampsize) * extended_buf_sampsize;	/* 8 */
	int sampsecsize = ENV_FSECSIZE;
	int sectors_remaining = (startsamp - skipsamps)/sampsecsize;				/* 9 */
	skipsamps   += sectors_remaining * sampsecsize;								/* 10 */
	*samp_offset  = startsamp - skipsamps;										/* 11 */
	*write_offset = ENV_FSECSIZE - *samp_offset;								/* 12 */
	if(sndseekEx(dz->ifd[0],skipsamps,0)<0) {									/* 13 */
		sprintf(errstr,": sndseekEx failed: skip_into_file()\n");
		return(SYSTEM_ERROR);
	}
	return(FINISHED);
}
//#endif

//TW UPDATE: NEW CODE

/*** NEW april 2002 ****/

static int fill_grid_array(double starttime,int *firstpass, double *next_gridstart, double **d, int *arraysize, dataptr dz);
static int check_available_space(dataptr dz);
static int create_grid_envelope(int n, double *next_gridstart,dataptr dz);
static int do_grid_envelope(dataptr dz);
static int do_grid_biz(int sib,int eib,int style,int *splicepos,dataptr dz);
static void get_nextposition_pair(double *ds,int *startsamp,int *endsamp,int *style,dataptr dz);

#define GRID_ZERO	(0)
#define GRID_COPY	(1)
#define GRID_DOWN	(2)
#define GRID_UP		(3)

/************************************ DO_GRIDS ********************************/

int do_grids(dataptr dz)
{
	int exit_status;
	int n;
	int namelen, numlen;
	char *outfilename;
//TW REVISION DEc 2002
//	char *outfilenumber;
	char *p, *q, *r;
	double next_gridstart;

	numlen  = 4;

	if(!sloom) {
		namelen = strlen(dz->wordstor[0]);
		q = dz->wordstor[0];
		r = dz->wordstor[0] + namelen;
		p = r - 1;
		while((*p != '\\') && (*p != '/') && (*p != ':')) {
			p--	;
			if(p < dz->wordstor[0])
				break;
		}
		if(p > dz->wordstor[0]) {
			p++;
			while(p <= r)
				*q++ = *p++;
		}
	}
	namelen = strlen(dz->wordstor[0]);		
//	if(sndunlink(dz->ofd) < 0) {
//		fprintf(stdout,"WARNING: Can't set initial dummy output soundfile for deletion.\n");
//		fflush(stdout);
//	}
//	if(sndcloseEx(dz->ofd) < 0) {
//		fprintf(stdout,"ERROR: Can't close initial dummy output soundfile\n");
//		return(SYSTEM_ERROR);
//	}
	dz->ofd = -1;
	if((exit_status = check_available_space(dz))<0)
		return(exit_status);
	if((dz->tempsize = dz->insams[0] * dz->iparam[GRID_COUNT])<0)
		dz->tempsize = INT_MAX;
	if(sloom)
		namelen--;									/* Drop the 0 at end of name */
	next_gridstart = 0.0;
	if((exit_status = reset_peak_finder(dz))<0)
		return(exit_status);
	for(n=0;n<dz->iparam[GRID_COUNT];n++) {
		if(sndseekEx(dz->ifd[0],0,0)<0) {
			sprintf(errstr,"sndseekEx() failed.\n");
			return(SYSTEM_ERROR);
		}
		reset_filedata_counters(dz);
// FEB 2010 TW
		if((outfilename = (char *)malloc((namelen + numlen + 10) * sizeof(char)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for outfilename.\n");
			return(MEMORY_ERROR);
		}				
	    strcpy(outfilename,dz->wordstor[0]);
//TW REvision Dec 2002
		if(sloom)
			insert_new_number_at_filename_end(outfilename,n,1);
// FEB 2010 TW
		else {
			insert_separator_on_sndfile_name(outfilename,1);
			insert_new_number_at_filename_end(outfilename,n,0);
		}
		dz->process_type = EQUAL_SNDFILE;	/* allow sndfile to be created */
		if((exit_status = create_sized_outfile(outfilename,dz))<0) {
			if(!sloom) {
				sprintf(errstr, "Soundfile %s already exists: Made %d grids only.\n",outfilename,n-1);
				free(outfilename);
				dz->process_type = OTHER_PROCESS;
				dz->ofd = -1;
				return(GOAL_FAILED);
			} else {
				dz->process_type = OTHER_PROCESS;
				dz->ofd = -1;
				sprintf(errstr, "Soundfile %s already exists: Made %d grids only.\n",outfilename,n-1);
				return(SYSTEM_ERROR);
			}
		}							
		dz->process_type = OTHER_PROCESS;
		if((exit_status = create_grid_envelope(n,&next_gridstart,dz))<0)
			return(exit_status);
		if((exit_status = do_grid_envelope(dz))<0)
			return(exit_status);
		dz->process_type = EQUAL_SNDFILE;		/* allows header to be written  */
		dz->outfiletype  = SNDFILE_OUT;			/* allows header to be written  */
		if((exit_status = headwrite(dz->ofd,dz))<0) {
			free(outfilename);
			return(exit_status);
		}
		dz->process_type = OTHER_PROCESS;		/* restore true status */
		dz->outfiletype  = NO_OUTPUTFILE;		/* restore true status */
		if((exit_status = reset_peak_finder(dz))<0)
			return(exit_status);
		if(sndcloseEx(dz->ofd) < 0) {
			fprintf(stdout,"WARNING: Can't close output soundfile %s\n",outfilename);
			fflush(stdout);
		}
		free(outfilename);
		dz->ofd = -1;
	}
	return(FINISHED);
}

/************************************ CHECK_AVAILABLE_SPACE ********************************/
#define LEAVESPACE	(10*1024)

int check_available_space(dataptr dz)
{
	unsigned int slots;
	unsigned int freespace = getdrivefreespace("temp") - LEAVESPACE;
	freespace /= sizeof(float);
	slots = freespace/dz->insams[0];
	if(slots < (unsigned int)dz->iparam[COPY_CNT]) {
		sprintf(errstr,"Insufficient space on disk to create %d copies.\n"
		               "You have space for %d copies.\n",dz->iparam[COPY_CNT],slots);
		return(GOAL_FAILED);
	}
	return(FINISHED);
}

/************************************ CREATE_GRID_ENVELOPE ********************************/

int create_grid_envelope(int n, double *next_gridstart,dataptr dz)
{
	double *d, time, thistime, grid_step, half_gridsplen;
	int arraysize = 8, old_arraysize;
	double starttime;
	int firstpass, exit_status;

	if(dz->brksize[GRID_WIDTH] == 0) {
		grid_step = dz->param[GRID_WIDTH] * dz->iparam[GRID_COUNT];
		switch(n) {
		case(0):
			if((dz->parray[0] = (double *)malloc(arraysize * sizeof(double)))==NULL) {
				sprintf(errstr,"Insufficient memory for grid envelope,\n");
				return(MEMORY_ERROR);
			}
			d = dz->parray[0];
			for(time = 0.0; time <= dz->duration; time += grid_step) {
				thistime = time;	
				if(flteq(time,0.0))
					*d++ = thistime;
				else {
					if(dz->brksize[GRID_SPLEN] > 0)
						read_value_from_brktable(thistime,GRID_SPLEN,dz);
					half_gridsplen = dz->param[GRID_SPLEN]/2.0;
					*d++ = thistime + half_gridsplen;
				}
				*d++ = 1.0;
				thistime += dz->param[GRID_WIDTH];
				if(dz->brksize[GRID_SPLEN] > 0)
					read_value_from_brktable(thistime,GRID_SPLEN,dz);
				half_gridsplen = dz->param[GRID_SPLEN]/2.0;
				*d++ = thistime - half_gridsplen;
				*d++ = 1.0;
				*d++ = thistime + half_gridsplen;
				*d++ = 0.0;
				thistime = time + grid_step;
				if(dz->brksize[GRID_SPLEN] > 0) {
					read_value_from_brktable(thistime,GRID_SPLEN,dz);
				}
				half_gridsplen = dz->param[GRID_SPLEN]/2.0;
				*d++ = thistime - half_gridsplen;
				*d++ = 0.0;
				old_arraysize = arraysize;
				arraysize += 8;
				if((dz->parray[0] = (double *)realloc((char *)dz->parray[0],arraysize * sizeof(double)))==NULL) {
					sprintf(errstr,"Insufficient memory for grid envelope,\n");
					return(MEMORY_ERROR);
				}
				d = dz->parray[0] + old_arraysize;
				dz->parray[1] = d;		/* mark end of parray */
			}
			break;
		case(1):
			for(d = dz->parray[1]-2; d >= dz->parray[0];d -= 2) {
				*(d+4) = (*d) + dz->param[GRID_WIDTH];		/* add time to values, and shift them up two brkpnt places */
				*(d+5) = *(d+1);
			}
			*(dz->parray[0])   = 0.0;						/* Insert a zero time zero val */			
			*(dz->parray[0]+1) = 0.0;						
			if(dz->brksize[GRID_SPLEN] > 0) {					/* And a splice-start zero */
				read_value_from_brktable(*(dz->parray[0]+4),GRID_SPLEN,dz);
			}
			half_gridsplen = dz->param[GRID_SPLEN]/2.0;
			*(dz->parray[0]+2) = *(dz->parray[0]+4) - half_gridsplen;
			*(dz->parray[0]+3) = 0.0;
			*(dz->parray[0]+4) += half_gridsplen;
			dz->parray[1] += 4;								/* mark end of parray */
			break;
		default:											/* move grid by grid-width, except zerotime point */
			for(d = dz->parray[1]-2; d >= dz->parray[0]+2;d -= 2)
				*d += dz->param[GRID_WIDTH];
			break;
		}
	} else {
		switch(n) {
		case(0):
			starttime = 0.0;
			firstpass = 1;
			if((dz->parray[0] = (double *)malloc(arraysize * sizeof(double)))==NULL) {
				sprintf(errstr,"Insufficient memory for grid envelope,\n");
				return(MEMORY_ERROR);
			}
			d = dz->parray[0];
			if((exit_status = fill_grid_array(starttime,&firstpass,next_gridstart,&d,&arraysize,dz))<0)
				return(exit_status);
			arraysize += 64;	/* safety */
			if((dz->parray[0] = (double *)realloc((char *)dz->parray[0],arraysize * sizeof(double)))==NULL) {
				sprintf(errstr,"Insufficient memory for grid envelope,\n");
				return(MEMORY_ERROR);
			}
			break;
		default:											/* move grid by grid-width, except zerotime point */
			arraysize = 12;
			starttime = *next_gridstart;
			firstpass = 1;
			d = dz->parray[0] + 4;
			if((exit_status = fill_grid_array(starttime,&firstpass,next_gridstart,&d,&arraysize,dz))<0)
				return(exit_status);
			*(dz->parray[0])   = 0.0;						/* Insert a zero time zero val */			
			*(dz->parray[0]+1) = 0.0;						
			if(dz->brksize[GRID_SPLEN] > 0)					/* And a splice-start zero */
				read_value_from_brktable(*(dz->parray[0]+4),GRID_SPLEN,dz);
			half_gridsplen = dz->param[GRID_SPLEN]/2.0;
			*(dz->parray[0]+2) = *(dz->parray[0]+4) - half_gridsplen;
			*(dz->parray[0]+3) = 0.0;
			*(dz->parray[0]+4) += half_gridsplen;
			break;
		}
	}
	return(FINISHED);
}

/************************************ FILL_GRID_ARRAY ********************************/

int fill_grid_array(double starttime,int *firstpass, double *next_gridstart, double **d, int *arraysize, dataptr dz)
{
	double grid_step = 0.0, time, thistime, start_gridwidth, this_gridwidth, half_gridsplen = 0.0;
	int z;
	int old_arraysize, k;
	for(time = starttime; time <=dz->duration; time += grid_step) {
		grid_step = 0.0;
		thistime = time;
		read_value_from_brktable(thistime,GRID_WIDTH,dz);
		start_gridwidth = dz->param[GRID_WIDTH];
		if(*firstpass)							/* save initial distance to next grid set */
			*next_gridstart += start_gridwidth;
		this_gridwidth = start_gridwidth;
		grid_step += this_gridwidth;
		thistime += this_gridwidth;
		for(z=1;z<dz->iparam[GRID_COUNT];z++) {				  /*sum steps of all grids to get step from */
			read_value_from_brktable(thistime,GRID_WIDTH,dz); /* current window of this grid, to next window of this grid */
			this_gridwidth = dz->param[GRID_WIDTH];
			grid_step += this_gridwidth;
			thistime += this_gridwidth;
		}
		thistime = time;	
		if(*firstpass)
			half_gridsplen = 0.0;	/* Envelope starts without splice ... splice added in calling routine */
		else if(dz->brksize[GRID_SPLEN] > 0) {
			read_value_from_brktable(thistime,GRID_SPLEN,dz);
			half_gridsplen = dz->param[GRID_SPLEN]/2.0;
		}
		**d = thistime + half_gridsplen;
		(*d)++;
		**d = 1.0;
		(*d)++;
		thistime += start_gridwidth;
		if(dz->brksize[GRID_SPLEN] > 0) {
			read_value_from_brktable(thistime,GRID_SPLEN,dz);
			half_gridsplen = dz->param[GRID_SPLEN]/2.0;
		} else if(*firstpass)
			half_gridsplen = dz->param[GRID_SPLEN]/2.0;	/* if no brkpnt table, only need to calc on first pass */
		*firstpass = 0;
		**d = thistime - half_gridsplen;
		(*d)++;
		**d = 1.0;
		(*d)++;
		**d = thistime + half_gridsplen;
		(*d)++;
		**d = 0.0;
		(*d)++;
		thistime = time + grid_step;
		if(dz->brksize[GRID_SPLEN] > 0) {
			read_value_from_brktable(thistime,GRID_SPLEN,dz);
			half_gridsplen = dz->param[GRID_SPLEN]/2.0;
		}
		**d = thistime - half_gridsplen;
		(*d)++;
		**d = 0.0;
		(*d)++;
		old_arraysize = *arraysize;
		*arraysize += 8;
		k = *d - dz->parray[0];
		if((dz->parray[0] = (double *)realloc((char *)dz->parray[0],(*arraysize) * sizeof(double)))==NULL) {
			sprintf(errstr,"Insufficient memory for grid envelope,\n");
			return(MEMORY_ERROR);
		}
		*d = dz->parray[0] + k;
		dz->parray[1] = *d;	/* mark end of parray */
	}
 	return(FINISHED);
}

/************************************ DO_GRID_ENVELOPE ********************************/

int do_grid_envelope(dataptr dz)
{
	double *ds = dz->parray[0];
	int sib, eib; /* startpos-ion-buf, edipos-in-buf */
	int style, exit_status, finished= 0;
	int startsamp, endsamp, splicepos = 0,last_total_samps_read = 0;
	sndseekEx(dz->ifd[0],0,0);
	get_nextposition_pair(ds,&startsamp,&endsamp,&style,dz);
	sib = startsamp - last_total_samps_read;
	eib = endsamp   - last_total_samps_read;
	if((exit_status = read_samps(dz->sampbuf[0],dz)) < 0)
		return(exit_status);
	if(dz->ssampsread == 0) {
		sprintf(errstr,"No data found in input soundfile\n");
		return(GOAL_FAILED);
	}
	while(ds < dz->parray[1] - 2) {
		do {
			while(eib < dz->ssampsread) {
				do_grid_biz(sib,eib,style,&splicepos,dz);
				if((ds += 2) >= dz->parray[1] - 2) {
					finished = 1;
					break;
				}
				get_nextposition_pair(ds,&startsamp,&endsamp,&style,dz);
				sib = startsamp - last_total_samps_read;
				eib = endsamp   - last_total_samps_read;
			}
			if(finished)
				break;
			do_grid_biz(sib,dz->ssampsread,style,&splicepos,dz);
			if(dz->ssampsread > 0) {
				if((exit_status = write_samps(dz->sampbuf[0],dz->ssampsread,dz))<0)
					return(exit_status);
			}
			last_total_samps_read = dz->total_samps_read;
			if((exit_status = read_samps(dz->sampbuf[0],dz)) < 0)
				return(exit_status);
			if(dz->ssampsread == 0) {
				finished = 1;
				break;
			}
			sib = 0;
			eib = endsamp - last_total_samps_read;
			if(eib == 0) {
				if((ds += 2) >= dz->parray[1] - 2) {
					finished = 1;
					break;
				}
				get_nextposition_pair(ds,&startsamp,&endsamp,&style,dz);
				sib = startsamp - last_total_samps_read;
				eib = endsamp   - last_total_samps_read;
			}
		} while(!finished);
		if(finished)
			break;
	}
	if(dz->ssampsread > 0) {
		if((exit_status = write_samps(dz->sampbuf[0],dz->ssampsread,dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}

/************************************ DO_GRID_BIZ ********************************/

int do_grid_biz(int sib,int eib,int style,int *splicepos,dataptr dz)
{
	double splice_incr, splice_val;
	int n;
	switch(style) {
	case(GRID_ZERO):	memset((char *)(dz->sampbuf[0] + sib),0,(eib - sib) * sizeof(float));	break;
	case(GRID_COPY):	break;
	case(GRID_UP):
		dz->iparam[GRID_SPLEN] = (int)round(dz->param[GRID_SPLEN] * dz->infile->srate) * dz->infile->channels;
		splice_incr = 1.0 / (double)dz->iparam[GRID_SPLEN];
		splice_val = *splicepos * splice_incr;
		for(n = sib; n < eib; n++) {
			dz->sampbuf[0][n] = (float)(dz->sampbuf[0][n] * splice_val);
			splice_val += splice_incr;
			(*splicepos)++;
		}
		if(*splicepos >= dz->iparam[GRID_SPLEN])
			*splicepos = 0;
		break;
	case(GRID_DOWN):
		dz->iparam[GRID_SPLEN] = (int)round(dz->param[GRID_SPLEN] * dz->infile->srate) * dz->infile->channels;
		splice_incr = 1.0 / (double)dz->iparam[GRID_SPLEN];
		splice_val = 1.0 - (*splicepos * splice_incr);
		for(n = sib; n < eib; n++) {
			dz->sampbuf[0][n] = (float)(dz->sampbuf[0][n] * splice_val);
			splice_val -= splice_incr;
			(*splicepos)++;
		}
		if(*splicepos >= dz->iparam[GRID_SPLEN])
			*splicepos = 0;
		break;
	}
	return(FINISHED);
}

/************************************ GET_NEXTPOSITION_PAIR ********************************/

void get_nextposition_pair(double *ds,int *startsamp,int *endsamp,int *style,dataptr dz)
{
	double *de = ds+2, startval, endval;
	*startsamp = round(*ds * dz->infile->srate) * dz->infile->channels;
	*endsamp   = round(*de * dz->infile->srate) * dz->infile->channels;
	startval = *(ds+1);		
	endval   = *(de+1);
	if(flteq(startval,1.0)) {
		if(flteq(endval,1.0))
			*style = GRID_COPY;
		else
			*style = GRID_DOWN;
	} else {
		if(flteq(endval,1.0))
			*style = GRID_UP;
		else
			*style = GRID_ZERO;
	}
}
