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
#include <distort.h>
#include <cdpmain.h>

#include <sfsys.h>
#include <osbind.h>


static int 	do_repeat(int current_buf,int incycles_start,int incycles_end,int *obufpos,dataptr dz);
static int 	do_repeat_bufcros(int current_buf,int incycles_start,int incycles_end,int *obufpos,dataptr dz);
static int	do_interp(int current_buf,int incycle_start,int incycle_end,int *lastcycle_len,
				int lastcycle_start,int *previous_cycle_crossed_bufs,int *obufpos,dataptr dz);
static int	do_interp_bufcros(int current_buf,int incycle_end,int incycle_start,int *lastcycle_len,
				int lastcycle_start,int *previous_cycle_crossed_bufs,int *obufpos,dataptr dz);
static int	write_to_outbuf(int *obufpos,int collectbufpos,dataptr dz);
static int	no_fault_change_buff(float **b,int *current_buf,dataptr dz);

/********************** SKIP_INITIAL_CYCLES ******************************/

int skip_initial_cycles(int *current_pos,int *current_buf,int phase,int skip_paramno,dataptr dz)
{
	int exit_status;
	register int i = 0;
	int n;
	float *b = dz->sampbuf[*current_buf];
	switch(phase) {
	case(1):
		for(n=0;n<dz->iparam[skip_paramno];n++) {
			while(b[i]>=0.0) {
				if(++i >= dz->ssampsread) {
					if((exit_status = no_fault_change_buff(&b,current_buf,dz))<0)
						return(exit_status);
				}
			}
			while(b[i]<=0.0) {
				if(++i >= dz->ssampsread) {
					if((exit_status = no_fault_change_buff(&b,current_buf,dz))<0)
						return(exit_status);
				}
			}
		}
		break;
	case(-1):
		for(n=0;n<dz->iparam[skip_paramno];n++) {
			while(b[i]<=0.0) {
				if(++i >= dz->ssampsread) {
					if((exit_status = no_fault_change_buff(&b,current_buf,dz))<0)
						return(exit_status);
				}
			}
			while(b[i]>=0.0) {
				if(++i >= dz->ssampsread) {
					if((exit_status = no_fault_change_buff(&b,current_buf,dz))<0)
						return(exit_status);
				}
			}
		}
		break;
	}
	*current_pos = i;
	return(FINISHED);
}

/**************************** DISTORT_RPT ******************************
 *
 * Distort file by repeating GROUPS of cycles.
 */

int distort_rpt(int *current_buf,int initial_phase,int *obufpos,int *current_pos_in_buf,int *cnt,
	int cyclecnt,int *lastcycle_len,int *lastcycle_start,int *previous_cycle_crossed_bufs,dataptr dz)
{
	int exit_status;
	register int i = *current_pos_in_buf;
	register int n;
	int cycleno_in_group_at_bufcros = -1;
	float *inbuf  = dz->sampbuf[*current_buf];
	int incycles_end, incycles_start = i, jump_cyclecnt;
	for(n=0;n<cyclecnt;n++) {
		switch(initial_phase) {
		case(1):
			while(inbuf[i]>=0.0) {
				if(++i >= dz->ssampsread) {
					if((exit_status = change_buff(&inbuf,&cycleno_in_group_at_bufcros,current_buf,dz))!=CONTINUE)
						return(exit_status);
					cycleno_in_group_at_bufcros = n;
					i = 0;
				}
			}
			while(inbuf[i]<=0.0) {
				if(++i >= dz->ssampsread) {
					if((exit_status = change_buff(&inbuf,&cycleno_in_group_at_bufcros,current_buf,dz))!=CONTINUE)
						return(exit_status);
					cycleno_in_group_at_bufcros = n;
					i = 0;
				}
			}
			break;
		case(-1):
			while(inbuf[i]<=0.0) {
				if(++i >= dz->ssampsread) {
					if((exit_status = change_buff(&inbuf,&cycleno_in_group_at_bufcros,current_buf,dz))!=CONTINUE)
						return(exit_status);
					cycleno_in_group_at_bufcros = n;
					i = 0;
				}
			}
			while(inbuf[i]>=0.0) {
				if(++i >= dz->ssampsread) {
					if((exit_status = change_buff(&inbuf,&cycleno_in_group_at_bufcros,current_buf,dz))!=CONTINUE)
						return(exit_status);
					cycleno_in_group_at_bufcros = n;
					i = 0;
				}
			}
			break;
		}
	}
	incycles_end = i;
	switch(dz->process) {
	case(DISTORT_INTP):
		if(cycleno_in_group_at_bufcros >= 0)
			exit_status = do_interp_bufcros(*current_buf,incycles_start,incycles_end,lastcycle_len,
										*lastcycle_start,previous_cycle_crossed_bufs,obufpos,dz);
		else
			exit_status = do_interp(*current_buf,incycles_start,incycles_end,lastcycle_len,
										*lastcycle_start,previous_cycle_crossed_bufs,obufpos,dz);
		*lastcycle_start = *current_pos_in_buf;
		break;
	case(DISTORT_RPT):
	case(DISTORT_RPT2):
		if(cycleno_in_group_at_bufcros >= 0)
			exit_status = do_repeat_bufcros(*current_buf,incycles_start,incycles_end,obufpos,dz);
		else
			exit_status = do_repeat(*current_buf,incycles_start,incycles_end,obufpos,dz);
		break;
	default:
		sprintf(errstr,"Unknown case in distort_rpt()\n");
		return(PROGRAM_ERROR);
	}
	if(dz->process == DISTORT_RPT2) {
		jump_cyclecnt = (dz->iparam[DISTRPT_MULTIPLY] - 1) * cyclecnt;
		for(n=0;n<jump_cyclecnt;n++) {
			switch(initial_phase) {
			case(1):
				while(inbuf[i]>=0) {
					if(++i >= dz->ssampsread) {
						if((exit_status = change_buff(&inbuf,&cycleno_in_group_at_bufcros,current_buf,dz))!=CONTINUE)
							return(exit_status);
						cycleno_in_group_at_bufcros = n;
						i = 0;
					}
				}
				while(inbuf[i]<=0) {
					if(++i >= dz->ssampsread) {
						if((exit_status = change_buff(&inbuf,&cycleno_in_group_at_bufcros,current_buf,dz))!=CONTINUE)
							return(exit_status);
						cycleno_in_group_at_bufcros = n;
						i = 0;
					}
				}
				break;
			case(-1):
				while(inbuf[i]<=0) {
					if(++i >= dz->ssampsread) {
						if((exit_status = change_buff(&inbuf,&cycleno_in_group_at_bufcros,current_buf,dz))!=CONTINUE)
							return(exit_status);
						cycleno_in_group_at_bufcros = n;
						i = 0;
					}
				}
				while(inbuf[i]>=0) {
					if(++i >= dz->ssampsread) {
						if((exit_status = change_buff(&inbuf,&cycleno_in_group_at_bufcros,current_buf,dz))!=CONTINUE)
							return(exit_status);
						cycleno_in_group_at_bufcros = n;
						i = 0;
					}
				}
				break;
			}
		}
	}
	if(exit_status<0)
		return(exit_status);
	*current_pos_in_buf = i;
	(*cnt)++;
	return(CONTINUE);
}

/*********************** DO_REPEAT *************************/

int do_repeat(int current_buf,int incycles_start,int incycles_end,int *obufpos,dataptr dz)
{
	int exit_status;
	float *obuf, *ibuf = dz->sampbuf[current_buf] + incycles_start;
	int outcnt = 0;
	int incycle_len = incycles_end - incycles_start;
	int incycle_samps = incycle_len;
	int first_part_outbuf, second_part_outbuf;
	int next_obufpos;
	int finished = FALSE;
	do {
		while((next_obufpos = *obufpos + incycle_len) <= dz->buflen) { 
			obuf = dz->sampbuf[2] + *obufpos;
			memmove((char *)obuf,(char *)ibuf,incycle_samps * sizeof(float));
			*obufpos = next_obufpos;
			if(++outcnt >= dz->iparam[DISTRPT_MULTIPLY]) {
				finished = TRUE;
				break;
			}
		}
		if(!finished) {
			first_part_outbuf  = dz->buflen - *obufpos;
			second_part_outbuf = incycle_len - first_part_outbuf;
			obuf = dz->sampbuf[2] + *obufpos;
			memmove((char *)obuf,(char *)ibuf,first_part_outbuf * sizeof(float));
			obuf = dz->sampbuf[2];
			if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
				return(exit_status);
			memmove((char *)obuf,(char *)(ibuf + first_part_outbuf),second_part_outbuf * sizeof(float));
			*obufpos = second_part_outbuf;
			if(++outcnt >= dz->iparam[DISTRPT_MULTIPLY])
				finished = TRUE;
		}
	} while(!finished);
	return(FINISHED);
}

/*********************** DO_REPEAT_BUFCROS *************************/

int do_repeat_bufcros(int current_buf,int incycles_start,int incycles_end,int *obufpos,dataptr dz)
{
	int exit_status;
	float *obuf, *ibuf, *tempbuf;
	int outcnt = 0;
	int incycles_len = dz->buflen - incycles_start + incycles_end;
	int incycle_samps = incycles_len;
	int first_part_inbuf, second_part_inbuf;
	int first_part_outbuf, second_part_outbuf;
	int next_obufpos;
	int finished = FALSE;
	if(incycles_len >= dz->buflen) {
		sprintf(errstr,"cycleslen exceeds bufferlen %lf secs\n",
		(double)(dz->total_samps_read - dz->ssampsread + incycles_start)/(double)dz->infile->srate);
		return(GOAL_FAILED);
	}
	first_part_inbuf  = dz->buflen - incycles_start;
	second_part_inbuf = incycles_len - first_part_inbuf;
	ibuf = dz->sampbuf[!current_buf] + incycles_start;
	tempbuf = dz->sampbuf[3];
	memmove((char *)tempbuf,(char *)ibuf,first_part_inbuf * sizeof(float));
	ibuf = dz->sampbuf[current_buf]; 
	tempbuf += first_part_inbuf;
	memmove((char *)tempbuf,(char *)ibuf,second_part_inbuf * sizeof(float));
	ibuf = dz->sampbuf[3];
	do {
		while((next_obufpos = *obufpos + incycles_len) <= dz->buflen) { 
			obuf = dz->sampbuf[2] + *obufpos;
			memmove((char *)obuf,(char *)ibuf,incycle_samps * sizeof(float));
			*obufpos = next_obufpos;
			if(++outcnt >= dz->iparam[DISTRPT_MULTIPLY]) {
				finished = TRUE;
				break;
			}
		}
		if(!finished) {
			first_part_outbuf  = dz->buflen - *obufpos;
			second_part_outbuf = incycles_len - first_part_outbuf;
			obuf = dz->sampbuf[2] + *obufpos;
			memmove((char *)obuf,(char *)ibuf,first_part_outbuf * sizeof(float));
			obuf = dz->sampbuf[2];
			if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
				return(exit_status);
			memmove((char *)obuf,(char *)(ibuf + first_part_outbuf),second_part_outbuf * sizeof(float));
			*obufpos = second_part_outbuf;
			if(++outcnt >= dz->iparam[DISTRPT_MULTIPLY])
				finished = TRUE;
		}
	} while(!finished);
	return(FINISHED);
}

/*************************** DO_INTERP ****************************/

int do_interp(int current_buf,int incycle_start,int incycle_end,int *lastcycle_len,
			int lastcycle_start,int *previous_cycle_crossed_bufs,int *obufpos,dataptr dz)
{
	register int k;
	int 	this_cyclelen, n;
	double 	part[2], d, ratio;
	int 	cycpoint[2];
	int 	incycle_len = incycle_end - incycle_start;
	int 	cycdiff = incycle_len - *lastcycle_len; /* difference in length of cycles */
	float 	*tempbuf = dz->sampbuf[3];
	float 	*inbuf   = dz->sampbuf[current_buf];
	float 	*collectbuf = dz->sampbuf[4];
	int 	collectbufpos = 0;
	int 	maxcyclelen = max(incycle_len,*lastcycle_len);
	if(maxcyclelen * dz->iparam[DISTINTP_MULTIPLY] > dz->buflen) {
		sprintf(errstr,"Cycles too long for buffer\n");
		return(GOAL_FAILED);
	}
	if(*lastcycle_len > 0) {		/* very first cycle (lastcycle_len=0)is merely copied: otherwise, interp */
		for(n=1;n<dz->iparam[DISTINTP_MULTIPLY];n++) {
			d  = round((double)cycdiff * (double)n/(double)dz->iparam[DISTINTP_MULTIPLY]);
			d += (double)(*lastcycle_len);
			this_cyclelen = (int)d; 
			if(*previous_cycle_crossed_bufs) {
				for(k=0;k<this_cyclelen;k++) {	   
					ratio = (double)k/(double)this_cyclelen;
					cycpoint[0]  = round(ratio * (double)(*lastcycle_len));
					cycpoint[1]  = round(ratio * (double)incycle_len);
					cycpoint[1] += incycle_start; 
					part[0] = (double)tempbuf[cycpoint[0]] * (double)(dz->iparam[DISTINTP_MULTIPLY] - n);
					part[1] = (double)inbuf[cycpoint[1]] * (double)n;
					collectbuf[collectbufpos++] = (float)/*round*/((part[0] + part[1])/dz->iparam[DISTINTP_MULTIPLY]);
				}
				*previous_cycle_crossed_bufs = FALSE;
			} else {
				for(k=0;k<this_cyclelen;k++) {	   
					ratio = (double)k/(double)this_cyclelen;
					cycpoint[0]  = round(ratio * (double)(*lastcycle_len));
					cycpoint[0] += lastcycle_start; 
					cycpoint[1]  = round(ratio * (double)incycle_len);
					cycpoint[1] += incycle_start; 
					part[0] = (double)inbuf[cycpoint[0]] * (double)(dz->iparam[DISTINTP_MULTIPLY] - n);
					part[1] = (double)inbuf[cycpoint[1]] * (double)n;
					collectbuf[collectbufpos++] = (float)/*round*/((part[0] + part[1])/dz->iparam[DISTINTP_MULTIPLY]);
				}
			}
		}
	}
	/******* COPY GOAL CYCLE ******/
	memmove((char *)(&(collectbuf[collectbufpos])),(char *)(&(inbuf[incycle_start])),incycle_len * sizeof(float));
	collectbufpos += incycle_len;
	*lastcycle_len   = incycle_len;
	return write_to_outbuf(obufpos,collectbufpos,dz);
}

/*************************** DO_INTERP_CROSBUF ****************************/

int do_interp_bufcros(int current_buf,int incycle_start,int incycle_end,int *lastcycle_len,int lastcycle_start,
					   int *previous_cycle_crossed_bufs,int *obufpos,dataptr dz)
{
	register int k;
	int 	this_cyclelen, n;
	double 	part[2], d, ratio;
	int 	cycpoint[2];
	int 	first_incycle_part  = dz->buflen - incycle_start;
	int 	second_incycle_part = incycle_end;
	int 	incycle_len = first_incycle_part + second_incycle_part;
	int 	cycdiff = incycle_len - *lastcycle_len; /* difference in length of cycles */
	float 	*inbuf;
	float 	*tempbuf    = dz->sampbuf[3];
	float 	*collectbuf = dz->sampbuf[4];
	int 	collectbufpos = 0;
	int 	maxcyclelen = max(incycle_len,*lastcycle_len);
	if(maxcyclelen * dz->iparam[DISTINTP_MULTIPLY] > dz->buflen) {
		sprintf(errstr,"Cycles too long for buffer\n");
		return(GOAL_FAILED);
	}
	if(*previous_cycle_crossed_bufs) { 			/* IF THE CURRENT CYCLE CROSSES A BUFFER, AND THE LAST CYCLE CROSSES */
		sprintf(errstr,"Buffer space overrun\n"); /* A BUFFER, 2 BUFFER ENDS ARE CROSSED AND WE CAN'T DO THE READING */
		return(GOAL_FAILED);
	}
	*previous_cycle_crossed_bufs = TRUE;

	if(*lastcycle_len > 0) {		/* very first cycle (lastcycle_len=0)is merely copied: otherwise, interp */

		/* PUT BUFFER_CROSSING CYCLE INTO A TEMPORARY STORE */
		inbuf = dz->sampbuf[!current_buf] + incycle_start;
		tempbuf = dz->sampbuf[3];
		memmove((char *)tempbuf,(char *)inbuf,first_incycle_part * sizeof(float));
		inbuf = dz->sampbuf[current_buf];
		tempbuf += first_incycle_part;
		memmove((char *)tempbuf,(char *)inbuf,second_incycle_part * sizeof(float));
		tempbuf = dz->sampbuf[3];

		for(n=1;n<dz->iparam[DISTINTP_MULTIPLY];n++) {
			d  = round((double)cycdiff * (double)n/(double)dz->iparam[DISTINTP_MULTIPLY]);
			d += (double)(*lastcycle_len);
			this_cyclelen = (int)d; 
			for(k=0;k<this_cyclelen;k++) {	   
				ratio = (double)k/(double)this_cyclelen;
				cycpoint[0]  = round(ratio * (double)(*lastcycle_len));
				cycpoint[0] += lastcycle_start; 
				inbuf = dz->sampbuf[!current_buf];	/* PREVIOUS CYC MUST BE COMPLETELY IN PREVIOUS BUF */  
				part[0] = (double)inbuf[cycpoint[0]] * (double)(dz->iparam[DISTINTP_MULTIPLY]-n);
				cycpoint[1]  = round(ratio * (double)this_cyclelen);
				part[1] = (double)tempbuf[cycpoint[1]] * (double)(dz->iparam[DISTINTP_MULTIPLY]-n);
				collectbuf[collectbufpos++] = (float)/*round*/((part[0] + part[1])/dz->iparam[DISTINTP_MULTIPLY]);
			}
		}
	}
	/******* COPY GOAL CYCLE ******/
	memmove((char *)(&(collectbuf[collectbufpos])),(char *)tempbuf,incycle_len * sizeof(float));
	collectbufpos += incycle_len;
	*lastcycle_len   = incycle_len;
	return write_to_outbuf(obufpos,collectbufpos,dz);
}


/****************************** NO_FAULT_CHANGE_BUFF *****************************/

int no_fault_change_buff(float **b,int *current_buf,dataptr dz)
{
	if(dz->samps_left <= 0) {
		sprintf(errstr,"Not enough cycles to perform this process.\n");
		return(GOAL_FAILED);
	}
	*current_buf = !(*current_buf);
	*b = dz->sampbuf[*current_buf];
	return read_samps(*b,dz);
}

/****************************** WRITE_TO_OUTBUF *****************************/

int write_to_outbuf(int *obufpos,int collectbufpos,dataptr dz)
{
	int exit_status;	
	int obuf_free = dz->buflen - *obufpos;
	int collectbufcnt = collectbufpos;
	float *collectbuf = dz->sampbuf[4];
	dz->sbufptr[2] = dz->sampbuf[2] + *obufpos;
	if(collectbufcnt <= obuf_free) {
		memmove((char *)dz->sbufptr[2],(char *)collectbuf,collectbufcnt * sizeof(float));
		*obufpos += collectbufcnt;
	} else {
		memmove((char *)dz->sbufptr[2],(char *)collectbuf,obuf_free * sizeof(float));
		if((exit_status = write_samps(dz->sampbuf[2],dz->buflen,dz))<0)
			return(exit_status);
		dz->sbufptr[2] = dz->sampbuf[2];
		collectbuf    += obuf_free;
		collectbufcnt -= obuf_free;
		memmove((char *)dz->sbufptr[2],(char *)collectbuf, collectbufcnt * sizeof(float));
		*obufpos = collectbufcnt;
	}
	return(FINISHED);
}

/**************************** DISTORT_RPT_FRQLIM ******************************
 *
 * Distort file by repeating GROUPS of cycles, but don't count too-short cycles.
 */

int distort_rpt_frqlim(int *current_buf,int initial_phase,int *obufpos,int *current_pos_in_buf,int *cnt,
	int cyclecnt,dataptr dz)
{
	int exit_status;
	register int i = *current_pos_in_buf;
	register int n;
	int mincyclen = (int)round(dz->infile->srate/dz->param[DISTRPT_CYCLIM]), thiscyclen;
	int cycleno_in_group_at_bufcros = -1;
	float *inbuf  = dz->sampbuf[*current_buf];
	int incycles_end, incycles_start = i;
	for(n=0;n<cyclecnt;n++) {
		switch(initial_phase) {
		case(1):
			thiscyclen = 0;
			for(;;) {
				while(inbuf[i]>=0) {
					if(++i >= dz->ssampsread) {
						if((exit_status = change_buff(&inbuf,&cycleno_in_group_at_bufcros,current_buf,dz))!=CONTINUE)
							return(exit_status);
						cycleno_in_group_at_bufcros = n;
						i = 0;
					}
					thiscyclen++;
				}
				while(inbuf[i]<=0) {
					if(++i >= dz->ssampsread) {
						if((exit_status = change_buff(&inbuf,&cycleno_in_group_at_bufcros,current_buf,dz))!=CONTINUE)
							return(exit_status);
						cycleno_in_group_at_bufcros = n;
						i = 0;
					}
					thiscyclen++;
				}
				if(thiscyclen >= mincyclen)
					break;
			}
			break;
		case(-1):
			thiscyclen = 0;
			for(;;) {
				while(inbuf[i]<=0) {
					if(++i >= dz->ssampsread) {
						if((exit_status = change_buff(&inbuf,&cycleno_in_group_at_bufcros,current_buf,dz))!=CONTINUE)
							return(exit_status);
						cycleno_in_group_at_bufcros = n;
						i = 0;
					}
					thiscyclen++;
				}
				while(inbuf[i]>=0) {
					if(++i >= dz->ssampsread) {
						if((exit_status =  change_buff(&inbuf,&cycleno_in_group_at_bufcros,current_buf,dz))!=CONTINUE)
							return(exit_status);
						cycleno_in_group_at_bufcros = n;
						i = 0;
					}
					thiscyclen++;
				}
				if(thiscyclen >= mincyclen)
					break;
			}
			break;
		}
	}
	incycles_end = i;
	if(cycleno_in_group_at_bufcros >= 0)
		exit_status = do_repeat_bufcros(*current_buf,incycles_start,incycles_end,obufpos,dz);
	else
		exit_status = do_repeat(*current_buf,incycles_start,incycles_end,obufpos,dz);
	if(exit_status<0)
		return(exit_status);
	*current_pos_in_buf = i;
	(*cnt)++;
	return(CONTINUE);
}
