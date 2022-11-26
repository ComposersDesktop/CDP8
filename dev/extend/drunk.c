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



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <structures.h>
#include <tkglobals.h>
#include <globcon.h>
#include <processno.h>
#include <modeno.h>
#include <arrays.h>
#include <extend.h>
#include <cdpmain.h>
#include <sfsys.h>
#include <osbind.h>


int  drunk(int here,int *obufpos,int *thisbuf,int thisdur,double pregain,int *endwrite,int outbufspace,dataptr dz);
int  do_pause(int here,int *obufpos,int *endwrite,int *thisbuf,int thisdur,double pregain,int outbufspace,dataptr dz);
int  get_thisdur(int *thisdur,int *goalpcnt,int *pcnt,dataptr dz);
int  get_pos_and_dur_for_pause(int *here,int *thisdur,int *thisbuf,int *ibufpos,double pregain,dataptr dz);
int  get_pos(int *here,int thisdur,dataptr dz);
void do_pregain(double pregain,dataptr dz);
int  is_paus(int *goalpcnt,int *pcnt,dataptr dz);
int  convert_time_to_samplecnts(int paramno,dataptr dz);
int  convert_sec_steps_to_grain_steps(dataptr dz);
int  adjust_bufs(int here,int *ibufpos,int *thisbuf,double pregain,dataptr dz);
int  do_intermediate_write(int *endwrite,int *obufpos,dataptr dz);
int  make_drnk_splicetab(dataptr dz);
void setup_paus_params(dataptr dz);
int  get_maxvalue(int paramno,double *maxval,dataptr dz);
double get_pregain(dataptr dz);
int  do_start_splice(int chans,float *obuf,int *obufpos,float *ibuf,int *ibufpos,int *thisbuf,double pregain,
		int *endwrite,int outbufspace,dataptr dz);
int  do_end_splice(int chans,float *obuf,int *obufpos,float *ibuf,int *ibufpos,int *thisbuf,double pregain,dataptr dz);
int  bounce_off_file_end_if_necessary(int *here,int thisdur,int chans,int splicelen,dataptr dz);
int  get_pausdur(int *here,int splicelen,int chans,int *thisdur,dataptr dz);
int get_step(int chans,dataptr dz);
int get_new_pos(int here,int ambitus,int locus,int step);
int get_new_locus_pos(int here,int ambitus,int locus,int step);

/*************************** DO_DRUNKEN_WALK *************************/

int do_drunken_walk(dataptr dz)
{
	int exit_status;
	int here = 0, ibufpos, obufpos = 0, samps_processed = 0;
	double outsamptime;
	double  pregain = 1.0;
	int thisdur, endwrite = 0;
	int thisbuf;
	int is_a_pause;
	int goalpcnt = -1;	/* total events needed before pause: initialisation value -1 */
	int pcnt = 0;		/* count of events since last pause: initial val 0 > -1 */
	int chans = dz->infile->channels;
	int finished = FALSE;
	int outbufspace = dz->sampbuf[3] - dz->sampbuf[1];

	display_virtual_time(0,dz);
	if((exit_status = read_samps(dz->sampbuf[0],dz))<0)
		return(exit_status);
	if(dz->vflag[IS_DRNK_OVERLAP]) {
		pregain = get_pregain(dz);
		do_pregain(pregain,dz);
	}
	while(samps_processed < dz->iparam[DRNK_TOTALDUR]) {
		outsamptime  = (double)((obufpos + dz->total_samps_written)/chans);
		if((exit_status = read_values_from_all_existing_brktables(outsamptime,dz))<0)
			return(exit_status);
		while((is_a_pause = get_thisdur(&thisdur,&goalpcnt,&pcnt,dz))==TRUE) {
			setup_paus_params(dz);
			if((exit_status = get_pos_and_dur_for_pause(&here,&thisdur,&thisbuf,&ibufpos,pregain,dz))<0)
				return(exit_status);
			if((exit_status = do_pause(ibufpos,&obufpos,&endwrite,&thisbuf,thisdur,pregain,outbufspace,dz))<0)
				return(exit_status);
			if((samps_processed = dz->total_samps_written + endwrite) >= dz->iparam[DRNK_TOTALDUR]) {
				finished = TRUE;
				break;
			}
			outsamptime  = (double)((obufpos + dz->total_samps_written)/chans);
			if((exit_status = read_values_from_all_existing_brktables(outsamptime,dz))<0)
				return(exit_status);
		}
		if(finished)
			break;
		if((exit_status = get_pos(&here,thisdur,dz))<0)
			return(exit_status);
		if((exit_status = adjust_bufs(here,&ibufpos,&thisbuf,pregain,dz))<0)
			return(exit_status);
		if((exit_status = drunk(ibufpos,&obufpos,&thisbuf,thisdur,pregain,&endwrite,outbufspace,dz))<0)
			return(exit_status);
		if(obufpos >= dz->buflen) {
			if((exit_status = do_intermediate_write(&endwrite,&obufpos,dz))<0)
				return(exit_status);
		}
		samps_processed = dz->total_samps_written + endwrite;
	}
	if(endwrite > 0) {
		if((exit_status = write_samps(dz->sampbuf[1],endwrite,dz))<0)
			return(exit_status);
	}
	display_virtual_time(dz->total_samps_written,dz);
	return(FINISHED);
}

/**************************** DRUNK **************************/

int drunk(int ibufpos,int *obufpos,int *thisbuf,int thisdur,double pregain,int *endwrite,int outbufspace,dataptr dz)
{
	int exit_status;
	int n, overlap, copyblk;
	int  chans = dz->infile->channels;
	int splicelen = dz->iparam[DRNK_SPLICELEN] * chans;
	int outbpos;
	float *ibuf = dz->sampbuf[0], *thisin;
	float *obuf = dz->sampbuf[1];
	int available_insamps;
	if((exit_status = do_start_splice(chans,obuf,obufpos,ibuf,&ibufpos,thisbuf,pregain,endwrite,outbufspace,dz))<0)
		return(exit_status);
	outbpos = *obufpos;
	copyblk = thisdur - splicelen;
	available_insamps  = dz->ssampsread - ibufpos;
	while(available_insamps < copyblk) {
		thisin  = ibuf+ibufpos;
		for(n=0;n<available_insamps;n++) {
			if(outbpos >= outbufspace) {
				*endwrite = outbpos;
				if((exit_status = do_intermediate_write(endwrite,&outbpos,dz))<0)
					return(exit_status);
			}
			obuf[outbpos++] += thisin[n];
		}
		if((exit_status = read_samps(dz->sampbuf[0],dz))<0)
			return(exit_status);
		if(dz->vflag[IS_DRNK_OVERLAP])
			do_pregain(pregain,dz);			 
		(*thisbuf)++;
		ibufpos  = 0;
		copyblk -= available_insamps;
		available_insamps = dz->ssampsread;
	}			
	if(copyblk > 0) {
		thisin  = ibuf+ibufpos;
		for(n=0;n<copyblk;n++) {
			if(outbpos >= outbufspace) {
				*endwrite = outbpos;
				if((exit_status = do_intermediate_write(endwrite,&outbpos,dz))<0)
					return(exit_status);
			}
			obuf[outbpos++] += thisin[n];	
		}
		ibufpos += copyblk;
	}
	if((exit_status = do_end_splice(chans,obuf,&outbpos,ibuf,&ibufpos,thisbuf,pregain,dz))<0)
		return(exit_status);
	*endwrite = outbpos;
	outbpos -= splicelen;
	if(dz->vflag[IS_DRNK_OVERLAP]) {
		overlap = round((double)(thisdur/chans) * dz->param[DRNK_OVERLAP]) * chans; 
		outbpos  -= overlap;
	}
	*obufpos = outbpos;
	return(FINISHED);
} 

/**************************** DO_PAUSE **************************/

int do_pause(int ibufpos,int *obufpos,int *endwrite,int *thisbuf,int thisdur,double pregain,
	int outbufspace,dataptr dz)
{
	int exit_status;
	int n, copyblk, write_block;
	int outbpos;
	int  chans = dz->infile->channels;
	int splicelen = dz->iparam[DRNK_SPLICELEN] * chans;
	float *ibuf = dz->sampbuf[0], *thisin;
	float *obuf = dz->sampbuf[1], *thisout;
	int available_insamps, available_outspace;
/* ?? */
	if((exit_status = do_start_splice(chans,obuf,obufpos,ibuf,&ibufpos,thisbuf,pregain,endwrite,outbufspace,dz))<0)
		return(exit_status);
	outbpos = *obufpos;
	copyblk = thisdur - splicelen;
	write_block = dz->sampbuf[3] - dz->sampbuf[1]; /* writeable block = outbuf + overflow */
	available_outspace = write_block - outbpos;
	available_insamps  = dz->ssampsread - ibufpos;
	while(copyblk >= available_outspace) {
		while(available_insamps <= available_outspace) {
			thisout = obuf+outbpos;
			thisin  = ibuf+ibufpos;
			for(n=0;n<available_insamps;n++)
				thisout[n] += thisin[n];	
			if((exit_status = read_samps(dz->sampbuf[0],dz))<0)
				return(exit_status);
			if(dz->vflag[IS_DRNK_OVERLAP])
				do_pregain(pregain,dz);
			(*thisbuf)++;
			ibufpos     		= 0;
			outbpos   		   += available_insamps;
			copyblk 		   -= available_insamps;
			available_outspace -= available_insamps;
			available_insamps   = dz->ssampsread;			
		}
		if(available_outspace > 0) {
			thisout = obuf+outbpos;
			thisin  = ibuf+ibufpos;
			for(n=0;n<available_outspace;n++)
				thisout[n] += thisin[n];	
			ibufpos  		  += available_outspace;
			outbpos 		  += available_outspace;
			copyblk 		  -= available_outspace;
			available_insamps  = dz->ssampsread - ibufpos;			
		}
		if(write_block > 0) {
			if((exit_status = write_samps(dz->sampbuf[1],write_block,dz))<0)
				return(exit_status);
		}
		display_virtual_time(dz->total_samps_written,dz);
		memset((char *)dz->sampbuf[1],0,write_block * sizeof(float));
		write_block = dz->sampbuf[2] - dz->sampbuf[1];	/* change writeable block to outbuf only */
		outbpos = 0;
		available_outspace = dz->buflen;
	}
	while(available_insamps < copyblk) {
		thisout = obuf+outbpos;
		thisin  = ibuf+ibufpos;
		for(n=0;n<available_insamps;n++)
			thisout[n] += thisin[n];	
		if((exit_status = read_samps(dz->sampbuf[0],dz))<0)
			return(exit_status);
		if(dz->vflag[IS_DRNK_OVERLAP])
			do_pregain(pregain,dz);
		(*thisbuf)++;
		ibufpos  = 0;
		outbpos += available_insamps;
		copyblk -= available_insamps;
		available_insamps = dz->ssampsread;			
	}
	if(copyblk > 0) {
		thisout = obuf+outbpos;
		thisin  = ibuf+ibufpos;
		for(n=0;n<copyblk;n++)
			thisout[n] += thisin[n];	
		ibufpos += copyblk;
		outbpos += copyblk;
	}
	if((exit_status = do_end_splice(chans,obuf,&outbpos,ibuf,&ibufpos,thisbuf,pregain,dz))<0)
		return(exit_status);
	*endwrite = outbpos;
	outbpos -= splicelen;
	if(outbpos >= dz->buflen) {	/* if we're now beyond outbuf end: do bufwrite & copyback */
		if((exit_status = do_intermediate_write(endwrite,&outbpos,dz))<0)
			return(exit_status);
	}
	*obufpos = outbpos;
	return FINISHED;
} 

/***************************** GET_THISDUR *******************************/

int get_thisdur(int *thisdur,int *goalpcnt,int *pcnt,dataptr dz)
{
	double randv;
	if(dz->mode==HAS_SOBER_MOMENTS && is_paus(goalpcnt,pcnt,dz))
		return TRUE;
	randv  = drand48() * dz->param[DRNK_CLOKRND];
	if((*thisdur = round((double)dz->iparam[DRNK_CLOKTIK] * (1.0 + randv)))<dz->iparam[DRNK_SPLICELEN])
		*thisdur = round((double)dz->iparam[DRNK_CLOKTIK] * (1.0 - randv));
	*thisdur *= dz->infile->channels;
	return FALSE;
}

/***************************** GET_POS_AND_DUR_FOR_PAUSE *******************************/

int get_pos_and_dur_for_pause(int *here,int *thisdur,int *thisbuf,int *ibufpos,double pregain,dataptr dz)
{										   
	int exit_status;
	int step;
	int chans = dz->infile->channels;
	int ambitus   = dz->iparam[DRNK_AMBITUS]   * chans;
	int locus     = dz->iparam[DRNK_LOCUS] 	* chans;
	int splicelen = dz->iparam[DRNK_SPLICELEN] * chans;

	if(ambitus>0 && (step = get_step(chans,dz))!=0)
		*here = get_new_pos(*here,ambitus,locus,step);

	if((exit_status = get_pausdur(here,splicelen,chans,thisdur,dz))<0)
		return(exit_status);
	
	return adjust_bufs(*here,ibufpos,thisbuf,pregain,dz);
}

/***************************** GET_POS *******************************/

int get_pos(int *here,int thisdur,dataptr dz)
{										   
	int step = 0;
	int  chans = dz->infile->channels;
	int ambitus   = dz->iparam[DRNK_AMBITUS]   * chans;
	int locus 	   = dz->iparam[DRNK_LOCUS] 	* chans;
	int splicelen = dz->iparam[DRNK_SPLICELEN] * chans;

	if(ambitus > 0 && (step = get_step(chans,dz))!=0)
		*here = get_new_pos(*here,ambitus,locus,step);
	else if(dz->iparam[DRNK_LAST_LOCUS] != dz->iparam[DRNK_LOCUS])
		*here = get_new_locus_pos(*here,ambitus,locus,step);
	dz->iparam[DRNK_LAST_LOCUS] = dz->iparam[DRNK_LOCUS];
	return bounce_off_file_end_if_necessary(here,thisdur,chans,splicelen,dz);
}

/*************************** PREGAIN ****************************
 *
 * Allows for effect of overlapping segments.
 */

void do_pregain(double pregain,dataptr dz)
{
	int i;
	double j;
	float *iptr = dz->sampbuf[0];
	for(i=0;i<dz->ssampsread;i++) {
		j = (*iptr) * pregain;
		*iptr++ = (float) /*round*/j; 
	}
}

/******************************* IS_PAUS *******************************/

int is_paus(int *goalpcnt,int *pcnt,dataptr dz)
{
	if(*goalpcnt < 0) {								/* if 1st time */
		setup_paus_params(dz);
		*goalpcnt  = round(drand48() * (double)dz->iparam[DRNK_DRNKTIK_RANG]);
		*goalpcnt += dz->iparam[DRNK_MIN_DRNKTIK];
	} else if((*pcnt)++ >= *goalpcnt) {				/* if counted enough events to insert pause  */
		setup_paus_params(dz);
		*goalpcnt  = round(drand48() * (double)dz->iparam[DRNK_DRNKTIK_RANG]);
		*goalpcnt += dz->iparam[DRNK_MIN_DRNKTIK];
		*pcnt = 0;									/* Reset event counter to zero    */
		return TRUE;								/* Flag that pz has been returned */
	}
	return FALSE;									/* ELSE Flag no pz on this occasion */
}

/************************* ADJUST_BUFS *********************/

int adjust_bufs(int here,int *ibufpos,int *thisbuf,double pregain,dataptr dz)
{
	int exit_status;
	int nextbuf = here/dz->buflen;
	if(nextbuf != *thisbuf) {
		if((sndseekEx(dz->ifd[0],nextbuf * dz->buflen,0))<0) {
			sprintf(errstr,"seek error in adjust_bufs()\n");
			return(SYSTEM_ERROR);
		}
		if((exit_status = read_samps(dz->sampbuf[0],dz))<0)
			return(exit_status);
		if(dz->vflag[IS_DRNK_OVERLAP])
			do_pregain(pregain,dz);
	}			
	*ibufpos = here % dz->buflen;
	if(*ibufpos >= dz->ssampsread) {
		sprintf(errstr,"Buffer accounting problem: adjust_bufs()\n"); 
		return(PROGRAM_ERROR);
	}
	*thisbuf = nextbuf;
	return FINISHED;
}

/************************* DO_INTERMEDIATE_WRITE *********************/

int do_intermediate_write(int *endwrite,int *obufpos,dataptr dz)
{
	int exit_status;
	int samps_to_move   = *endwrite - dz->buflen;
	int samps_available = dz->sampbuf[3] - dz->sampbuf[1];
	if((exit_status = write_samps(dz->sampbuf[1],dz->buflen,dz))<0)
		return(exit_status);
	display_virtual_time(dz->total_samps_written,dz);
	memmove((char *)dz->sampbuf[1],(char *)dz->sampbuf[2],(size_t)(samps_to_move* sizeof(float)));
	memset((char *)(dz->sampbuf[1] + samps_to_move),0,(size_t)((samps_available - samps_to_move)*sizeof(float)));
	*endwrite -= dz->buflen;
	*obufpos -= dz->buflen;
	return(FINISHED);
}

/************************* SETUP_PAUS_PARAMS *********************/

void setup_paus_params(dataptr dz)
{
	int tempval;

    if(dz->iparam[DRNK_MIN_DRNKTIK] > dz->iparam[DRNK_MAX_DRNKTIK]) {
		tempval      				 = dz->iparam[DRNK_MIN_DRNKTIK];
		dz->iparam[DRNK_MIN_DRNKTIK] = dz->iparam[DRNK_MAX_DRNKTIK];
		dz->iparam[DRNK_MAX_DRNKTIK] = tempval;
    }
    dz->iparam[DRNK_DRNKTIK_RANG] = (int)(dz->iparam[DRNK_MAX_DRNKTIK] - dz->iparam[DRNK_MIN_DRNKTIK]);

	if(dz->param[DRNK_MIN_PAUS] >= dz->insams[0])
		dz->iparam[DRNK_MAXHOLD] = TRUE;
	else {
		dz->iparam[DRNK_MAXHOLD] = FALSE;
		if(dz->iparam[DRNK_MIN_PAUS] > dz->iparam[DRNK_MAX_PAUS]) {
			tempval					  = dz->iparam[DRNK_MIN_PAUS];
			dz->iparam[DRNK_MIN_PAUS] = dz->iparam[DRNK_MAX_PAUS];
			dz->iparam[DRNK_MAX_PAUS] = tempval;
		}
		dz->iparam[DRNK_PAUS_RANG] = (int)(dz->iparam[DRNK_MAX_PAUS] - dz->iparam[DRNK_MIN_PAUS]);
	}
}

/************************* GET_PREGAIN *********************/
			 
double get_pregain(dataptr dz)
{
	double val;
	val = 1.0 - dz->param[DRNK_MAX_OVLAP];		/* e.g. 3/4+ -> 1/4 || .76 -> .24 */
	val = 1.0/val;								/* i.e  1/4  -> 4   || .24 -> 4+  */
	val += 1.0;									/* i.e. 4    -> 5   || 4+  -> 5+  */	
//TW UPDATE
//	return val;
	return 1.0/val;
}

/*************************** DO_START_SPLICE **************************/

int do_start_splice(int chans,float *obuf,int *obufpos,float *ibuf,int *ibufpos,int *thisbuf,
	double pregain,int *endwrite,int outbufspace,dataptr dz) 
{
	int exit_status;
	int n;
	int m;
	int outbpos = *obufpos;
	int inbpos  = *ibufpos;
	for(n=0;n<dz->iparam[DRNK_SPLICELEN];n++) {
		for(m=0;m<chans;m++) {			
			if(outbpos >= outbufspace) {
				*endwrite = outbpos;
				if((exit_status = do_intermediate_write(endwrite,&outbpos,dz))<0)
					return(exit_status);
			}
			obuf[outbpos++] += (float)/*round*/((double)ibuf[inbpos++] * dz->parray[DRNK_SPLICETAB][n]); 
		}
		if(inbpos >= dz->ssampsread) {
			if((exit_status = read_samps(dz->sampbuf[0],dz))<0)
				return(exit_status);
			if(dz->vflag[IS_DRNK_OVERLAP])
				do_pregain(pregain,dz);
			inbpos = 0;
			(*thisbuf)++;
		}
	}
	*obufpos = outbpos;
	*ibufpos = inbpos;
	return(FINISHED);
}


/*************************** DO_END_SPLICE **************************/

int do_end_splice(int chans,float *obuf,int *obufpos,float *ibuf,int *ibufpos,int *thisbuf,double pregain,dataptr dz) 
{
	int exit_status;
	int n;
	int m;
	int outbpos = *obufpos;
	int inbpos  = *ibufpos;
	for(n=dz->iparam[DRNK_SPLICELEN]-1;n>=0;n--) {
		for(m=0;m<chans;m++)			
			obuf[outbpos++] += (float)/*round*/((double)ibuf[inbpos++] * dz->parray[DRNK_SPLICETAB][n]); 
		if(inbpos >= dz->ssampsread) {
			if((exit_status = read_samps(dz->sampbuf[0],dz))<0)
				return(exit_status);
			if(dz->vflag[IS_DRNK_OVERLAP])
				do_pregain(pregain,dz);
			inbpos = 0;
			(*thisbuf)++;			
		}
	}
	*obufpos = outbpos;
	*ibufpos = inbpos;
	return(FINISHED);
}

/*************************** GET_NEW_POS **************************/

int get_new_pos(int here,int ambitus,int locus,int step)
{
	int current_stray = here - locus; 					/* distance frm start available seg (locus) to current pos*/
	int new_stray = current_stray + step;			 	/* new_stray = distance from locus */
	int otherstray, newstep;
	if(new_stray > ambitus || new_stray < -ambitus ) {	/* if new_stray greater than ambitus */
		otherstray = current_stray - step;
		if(otherstray >= 0 && otherstray <= ambitus)
			here = locus + otherstray;					/* try reversing step */
		else if(otherstray <0 && otherstray >= -ambitus)
			here = locus + otherstray;						
		else {												
			newstep = abs(new_stray) % ambitus;			/* otherwise take modulus */
			if(step >= 0)
				here = locus + newstep;
			else
				here = locus - newstep;
		}
	} else
		here += step;
	if(here<0) 											/* Bounce off start of buffer if ness */
		here = -here;
	return(here);
}

/*************************** GET_NEW_LOCUS_POS **************************/

int get_new_locus_pos(int here,int ambitus,int locus,int step)
{
	int current_stray = here - locus; 					/* distance frm start available seg (locus) to current pos*/
	int new_stray = current_stray + step;			 	/* new_stray = distance from locus */
	if(new_stray > ambitus || new_stray < -ambitus)		/* if new_stray greater than ambitus */
		here = locus + step;
	else
		here += step;
	if(here<0) 											/* Bounce off start of buffer if ness */
		here = -here;
	return(here);
}

/*************************** GET_STEP **************************/

int get_step(int chans,dataptr dz)
{
	int step;
	double dstep  = (drand48() * 2.0) - 1.0;	/* range +1 to -1 */
	dstep *= dz->param[DRNK_GSTEP];		/* range +gstep to -gstep */
	step   = round(dstep);				/* Current stepsize in mintime-grains */	 			
	step  *= dz->iparam[DRNK_LGRAIN];	/* Current stepsize in samples 		  */
	step  *= chans;
	return step;
}

/*************************** GET_PAUSDUR **************************/

int get_pausdur(int *here,int splicelen,int chans,int *thisdur,dataptr dz)
{
	int this_pause;
	int max_pause = dz->insams[0] - *here - splicelen;
	if(EVEN(chans) && ODD(max_pause)) {
		sprintf(errstr,"Stereo anomaly at file end: get_pausdur()\n");
		return(PROGRAM_ERROR);
	}
	if(max_pause < 0) {
		sprintf(errstr,"Negative pause duration: get_pausdur()\n");
		return(PROGRAM_ERROR);
	}
	if(dz->iparam[DRNK_MAXHOLD])
		*thisdur = max_pause;
	else {
		this_pause  = round(drand48() * dz->iparam[DRNK_PAUS_RANG]); /* gen pause dur between */
		this_pause += dz->iparam[DRNK_MIN_PAUS]; 			    	 /* given limits */
		this_pause *= chans;
		*thisdur  = min(this_pause,max_pause);    	    			 /* but not >endoffile */
	}										   				  
	return(FINISHED);
}

/*************************** BOUNCE_OFF_FILE_END_IF_NECESSARY **************************/
//TW MY CURRENT CODE AGREES WITH THIS, so removed #ifdef
int bounce_off_file_end_if_necessary(int *here,int thisdur,int chans,int splicelen,dataptr dz)
{
	int diff, initial_diff, needed;
	initial_diff = abs(dz->insams[0] - *here);	/* Ensure enough room to read final segment and splice */
	if(EVEN(chans) && ODD(initial_diff)) {
		sprintf(errstr,"Stereo anomaly at file end: bounce_off_file_end_if_necessary()\n");
		return(PROGRAM_ERROR);
	}
	if((needed = thisdur + splicelen) > dz->insams[0]) {
		sprintf(errstr,"Segment duration exceeds size of sound\n");
		return(DATA_ERROR);
	}
	while((diff = dz->insams[0] - *here) < needed) {
		*here -= initial_diff; 		   			/* Bounce off end of buffer if ness */
		if(*here < 0 || initial_diff == 0) {	/* But, if it bounces off BOTH ends !! */
			*here = (dz->insams[0] - needed)/dz->infile->channels;
			*here = (int)round(*here * drand48()) * dz->infile->channels;
		}
	}
	return(FINISHED);
}

