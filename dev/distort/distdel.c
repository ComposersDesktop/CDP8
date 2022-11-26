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


static int	do_up_cycle(int keep_or_loose,float **buf,int *current_buf,int *current_pos_in_buf,
			int *lastzero,int *bufcrosing,int *obufpos,dataptr dz);
static int	do_dn_cycle(int keep_or_loose,float **buf,int *current_buf,int *current_pos_in_buf,
			int *lastzero,int *bufcrosing,int *obufpos,dataptr dz);
static int	do_cycle_loud(float *buf,int cyclecnt,int current_pos_in_buf,int *obufpos,dataptr dz);
static int	do_cycle_quiet(float *buf,int cyclecnt,int current_pos_in_buf,int *obufpos,dataptr dz);
static int	do_cycle_loud_crosbuf(int cyclecnt,int current_buf,int current_pos_in_buf,
			int cycleno_in_group_at_bufcros,int *obufpos,dataptr dz);
static int	do_cycle_quiet_crosbuf(int cyclecnt,int current_buf,int current_pos_in_buf,
			int cycleno_in_group_at_bufcros,int *obufpos,dataptr dz);
static int	get_loudest_cycle(int cyclecnt,dataptr dz);
static int	get_quietest_cycle(int cyclecnt,dataptr dz);
static int	write_cycle(int start, int end,float *buf,int *obufpos,dataptr dz);

/********************************** DISTORT_DEL ******************************/

int distort_del
(int *current_buf,int *current_pos_in_buf,int phase,int *obufpos,int *cnt,dataptr dz)
{
	int exit_status;
	int n;
	float *b = dz->sampbuf[*current_buf];
	int bufcrosing = FALSE;
	int lastzero = *current_pos_in_buf;
	switch(phase) {
	case(1):
		if((exit_status = do_up_cycle(KEEP,&b,current_buf,current_pos_in_buf,&lastzero,&bufcrosing,obufpos,dz))!=CONTINUE)
			return(exit_status);
		if((exit_status = do_dn_cycle(KEEP,&b,current_buf,current_pos_in_buf,&lastzero,&bufcrosing,obufpos,dz))!=CONTINUE)
			return(exit_status);
		break;
	case(-1):
		if((exit_status = do_dn_cycle(KEEP,&b,current_buf,current_pos_in_buf,&lastzero,&bufcrosing,obufpos,dz))!=CONTINUE)
			return(exit_status);
		if((exit_status = do_up_cycle(KEEP,&b,current_buf,current_pos_in_buf,&lastzero,&bufcrosing,obufpos,dz))!=CONTINUE)
			return(exit_status);
		break;
	}
	if((*current_pos_in_buf - lastzero)!=0) {
		if((exit_status = write_cycle(lastzero,*current_pos_in_buf,b,obufpos,dz))<0)
			return(exit_status);
	}
	lastzero = *current_pos_in_buf;
	for(n=0;n<dz->iparam[DISTDEL_CYCLECNT];n++) {		  /* FOR NUMBER OF UNWANTED CYCLES */
		switch(phase) {
		case(1):
			if((exit_status = do_up_cycle(LOSE,&b,current_buf,current_pos_in_buf,&lastzero,&bufcrosing,obufpos,dz))!=CONTINUE)
				return(exit_status);
			if((exit_status = do_dn_cycle(LOSE,&b,current_buf,current_pos_in_buf,&lastzero,&bufcrosing,obufpos,dz))!=CONTINUE)
				return(exit_status);
			break;
		case(-1):
			if((exit_status = do_dn_cycle(LOSE,&b,current_buf,current_pos_in_buf,&lastzero,&bufcrosing,obufpos,dz))!=CONTINUE)
				return(exit_status);
			if((exit_status = do_up_cycle(LOSE,&b,current_buf,current_pos_in_buf,&lastzero,&bufcrosing,obufpos,dz))!=CONTINUE)
				return(exit_status);
			break;
		}
	}
	(*cnt)++;
	return(CONTINUE);
}

/******************************* DISTORT_DEL_WITH_LOUDNESS ******************************/

int distort_del_with_loudness(int *current_buf,int phase,int *obufpos,int *current_pos_in_buf,int *cnt,dataptr dz)
{
	int exit_status = CONTINUE;
	int cyclecnt = 0;				/* RWD added init */
//TW CONFIRM DELETE	*int bufcross = 0;
	register int i = *current_pos_in_buf;
//TW CONFIRM DELETE	int cyclestart = i;
	float *b  = dz->sampbuf[*current_buf];
	int cycleno_in_group_at_bufcros = -1;
	switch(phase) {				
	case(1):
		for(cyclecnt=0;cyclecnt<dz->iparam[DISTDEL_CYCLECNT];cyclecnt++) {
			/* RWD 4:2002 use lfarray as this now contains amplitudes */
			dz->lfarray[DISTDEL_CYCLEVAL][cyclecnt] = 0.0;			
			dz->lparray[DISTDEL_STARTCYC][cyclecnt] = i;	
			while(b[i]>=0) {		
				dz->lfarray[DISTDEL_CYCLEVAL][cyclecnt] += b[i];
				if(++i >= dz->ssampsread) {
					if((exit_status = change_buff(&b,&cycleno_in_group_at_bufcros,current_buf,dz))<0)
						return(exit_status);
					cycleno_in_group_at_bufcros = cyclecnt;
					if(exit_status !=CONTINUE)
						break;
					i = 0;
				}
			}
			if(exit_status !=CONTINUE)
				break;
			while(b[i]<=0.0) {	
				dz->lfarray[DISTDEL_CYCLEVAL][cyclecnt] -= b[i];
				if(++i >= dz->ssampsread) {
					if((exit_status = change_buff(&b,&cycleno_in_group_at_bufcros,current_buf,dz))<0)
						return(exit_status);
					cycleno_in_group_at_bufcros = cyclecnt;
					if(exit_status !=CONTINUE)
						break;
					i = 0;
				}
			}
			if(exit_status !=CONTINUE)
				break;
		}
		break;
	case(-1):						
		for(cyclecnt=0;cyclecnt<dz->iparam[DISTDEL_CYCLECNT];cyclecnt++) {
			dz->lfarray[DISTDEL_CYCLEVAL][cyclecnt] = 0.0;
			dz->lparray[DISTDEL_STARTCYC][cyclecnt] = i;
			while(b[i]<=0.0) {
				dz->lfarray[DISTDEL_CYCLEVAL][cyclecnt] -= b[i];
				if(++i >= dz->ssampsread) {
					if((exit_status = change_buff(&b,&cycleno_in_group_at_bufcros,current_buf,dz))<0)
						return(exit_status);
					cycleno_in_group_at_bufcros = cyclecnt;
					if(exit_status !=CONTINUE)
						break;
					i = 0;
				}
			}
			if(exit_status !=CONTINUE)
				break;
			while(b[i]>=0.0) {
				dz->lfarray[DISTDEL_CYCLEVAL][cyclecnt] += b[i];
				if(++i >= dz->ssampsread) {
					if((exit_status = change_buff(&b,&cycleno_in_group_at_bufcros,current_buf,dz))<0)
						return(exit_status);
					cycleno_in_group_at_bufcros = cyclecnt;
					if(exit_status !=CONTINUE)
						break;
					i = 0;
				}
			}
			if(exit_status !=CONTINUE)
				break;
		}
		break;
	}
	if(cyclecnt) {
		if(cycleno_in_group_at_bufcros >= 0) {	/* cycle set crosses between buffers */
			switch(dz->mode) {
			case(KEEP_STRONGEST): 
				if((exit_status = do_cycle_loud_crosbuf(cyclecnt,*current_buf,i,cycleno_in_group_at_bufcros,obufpos,dz))<0)
					return(exit_status);
				break;
			case(DELETE_WEAKEST): 
				if((exit_status = do_cycle_quiet_crosbuf(cyclecnt,*current_buf,i,cycleno_in_group_at_bufcros,obufpos,dz))<0)
					return(exit_status);
				break;
			}
		} else {
			switch(dz->mode) {
			case(KEEP_STRONGEST):	
				if((exit_status = do_cycle_loud(b,cyclecnt,i,obufpos,dz))<0)	
					return(exit_status);
				break;
			case(DELETE_WEAKEST):	
				if((exit_status = do_cycle_quiet(b,cyclecnt,i,obufpos,dz))<0)
					return(exit_status);
				break;
			}
		}
	}
	*current_pos_in_buf = i;
	(*cnt)++;
	return(exit_status);
}

/*************************** DO_UP_CYCLE ******************************/

int do_up_cycle(int keep_or_loose,float **buf,int *current_buf,int *current_pos_in_buf,
				int *lastzero,int *bufcrosing,int *obufpos,dataptr dz)
{
	int exit_status;
	while((*buf)[*current_pos_in_buf]>=0.0) {
		if(++(*current_pos_in_buf) >= dz->ssampsread) {
			if(keep_or_loose ==KEEP) {
				if((exit_status= write_cycle(*lastzero,*current_pos_in_buf,*buf,obufpos,dz))< 0)
					return(exit_status);
			}
			if((exit_status = change_buf(current_buf,bufcrosing,buf,dz))!=CONTINUE)
				return(exit_status);
			*lastzero = *current_pos_in_buf = 0;
		}
	}						
	return(CONTINUE);
}

/************************* DO_DN_CYCLE ******************************/

int do_dn_cycle(int keep_or_loose,float **buf,int *current_buf,int *current_pos_in_buf,
				int *lastzero,int *bufcrosing,int *obufpos,dataptr dz)
{
	int exit_status;
	while((*buf)[*current_pos_in_buf]<=0.0) {
		if(++(*current_pos_in_buf) >= dz->ssampsread) {
			if(keep_or_loose ==KEEP) {
				if((exit_status= write_cycle(*lastzero,*current_pos_in_buf,*buf,obufpos,dz))< 0)
					return(exit_status);
			}
			if((exit_status = change_buf(current_buf,bufcrosing,buf,dz))!=CONTINUE)
				return(exit_status);
			*lastzero = *current_pos_in_buf = 0;
		}
	}						
	return(CONTINUE);
}

/************************ DO_CYCLE_LOUD **************************/

int do_cycle_loud(float *buf,int cyclecnt,int current_pos_in_buf,int *obufpos,dataptr dz)
{
	int loudest = get_loudest_cycle(cyclecnt,dz);
	dz->lparray[DISTDEL_STARTCYC][cyclecnt] = current_pos_in_buf;
	return write_cycle(dz->lparray[DISTDEL_STARTCYC][loudest],dz->lparray[DISTDEL_STARTCYC][loudest+1],buf,obufpos,dz);
}


/************************ DO_CYCLE_LOUD_CROSBUF **************************/

int do_cycle_loud_crosbuf
(int cyclecnt,int current_buf,int current_pos_in_buf,int cycleno_in_group_at_bufcros,int *obufpos,dataptr dz)
{
	int exit_status;
	int loudest;
	float *buf;
	loudest = get_loudest_cycle(cyclecnt,dz);
	dz->lparray[DISTDEL_STARTCYC][cyclecnt] = current_pos_in_buf;
	if(cycleno_in_group_at_bufcros == loudest) {
		buf = dz->sampbuf[!current_buf]; /* GO TO PREVIOUS BUFFER */
		if((exit_status = write_cycle(dz->lparray[DISTDEL_STARTCYC][loudest],dz->buflen,buf,obufpos,dz))<0)
			return(exit_status);
		buf = dz->sampbuf[current_buf];
		exit_status = write_cycle(0L,dz->lparray[DISTDEL_STARTCYC][loudest+1],buf,obufpos,dz);
	} else if (cycleno_in_group_at_bufcros > loudest) {
		buf = dz->sampbuf[!current_buf]; /* GO TO PREVIOUS BUFFER */
		exit_status = write_cycle(dz->lparray[DISTDEL_STARTCYC][loudest],dz->lparray[DISTDEL_STARTCYC][loudest+1],buf,obufpos,dz);
	} else {
		buf = dz->sampbuf[current_buf]; /* GO TO PREVIOUS BUFFER */
		exit_status = write_cycle(dz->lparray[DISTDEL_STARTCYC][loudest],dz->lparray[DISTDEL_STARTCYC][loudest+1],buf,obufpos,dz);
	}
	return(exit_status);
}

/************************ GET_LOUDEST_CYCLE *******************************
 *
 * Find loudest cycle.
 */

int get_loudest_cycle(int cyclecnt,dataptr dz)
{
	int /*ffcycle = 0,*/ position = 0;
	float ffcycle = 0.0;
	int n;
	for(n=0;n<cyclecnt;n++) {
		if(dz->lfarray[DISTDEL_CYCLEVAL][n] > ffcycle) {
			ffcycle = dz->lfarray[DISTDEL_CYCLEVAL][n];
			position = n;
		}
	}
	return(position);
}

/***************************** DO_CYCLE_QUIET *************************/

int do_cycle_quiet(float *buf,int cyclecnt,int current_pos_in_buf,int *obufpos,dataptr dz)
{
	int exit_status;
	int quietest = get_quietest_cycle(cyclecnt,dz);
	dz->lparray[DISTDEL_STARTCYC][cyclecnt] = current_pos_in_buf;	
	if((exit_status = write_cycle(dz->lparray[DISTDEL_STARTCYC][0],dz->lparray[DISTDEL_STARTCYC][quietest],buf,obufpos,dz))<0)
		return(exit_status);
	return write_cycle(dz->lparray[DISTDEL_STARTCYC][quietest+1],dz->lparray[DISTDEL_STARTCYC][cyclecnt],buf,obufpos,dz);
}

/************************** DO_CYCLE_QUIET_CROSBUF *************************/

int do_cycle_quiet_crosbuf
(int cyclecnt,int current_buf,int current_pos_in_buf,int cycleno_in_group_at_bufcros,int *obufpos,dataptr dz)
{
	int exit_status;
	int quietest = get_quietest_cycle(cyclecnt,dz);
	float *buf;
	dz->lparray[DISTDEL_STARTCYC][cyclecnt] = current_pos_in_buf;
	if(cycleno_in_group_at_bufcros == quietest) {
		buf = dz->sampbuf[!current_buf];
		if((exit_status = write_cycle(dz->lparray[DISTDEL_STARTCYC][0],dz->lparray[DISTDEL_STARTCYC][quietest],buf,obufpos,dz))<0)
			return(exit_status);
		buf = dz->sampbuf[current_buf];
		exit_status = write_cycle(dz->lparray[DISTDEL_STARTCYC][quietest+1],dz->lparray[DISTDEL_STARTCYC][cyclecnt],buf,obufpos,dz);
	} else if(cycleno_in_group_at_bufcros > quietest) {
		buf = dz->sampbuf[!current_buf];
		if((exit_status = write_cycle(dz->lparray[DISTDEL_STARTCYC][0],dz->lparray[DISTDEL_STARTCYC][quietest],buf,obufpos,dz))<0)
			return(exit_status);
		if((exit_status = write_cycle(dz->lparray[DISTDEL_STARTCYC][quietest+1],dz->buflen,buf,obufpos,dz))<0)
			return(exit_status);
		buf = dz->sampbuf[current_buf];
		exit_status = write_cycle(0L,dz->lparray[DISTDEL_STARTCYC][cyclecnt],buf,obufpos,dz);
	} else {
		buf = dz->sampbuf[!current_buf];
		if((exit_status = write_cycle(dz->lparray[DISTDEL_STARTCYC][0],dz->buflen,buf,obufpos,dz))<0)
			return(exit_status);
		buf = dz->sampbuf[current_buf];
		if((exit_status = write_cycle(0L,dz->lparray[DISTDEL_STARTCYC][quietest],buf,obufpos,dz))<0)
			return(exit_status);
		exit_status = write_cycle(dz->lparray[DISTDEL_STARTCYC][quietest+1],dz->lparray[DISTDEL_STARTCYC][cyclecnt],buf,obufpos,dz);
	}
	return(exit_status);
}

/************************ GET_QUIETEST_CYCLE *******************************
 *
 * Find quietest cycle.
 */

int get_quietest_cycle(int cyclecnt,dataptr dz)
{
//TW FIXED
//	int quiestest_cycle = dz->lparray[DISTDEL_CYCLEVAL][0], position = 0;
	float quiestest_cycle = dz->lfarray[DISTDEL_CYCLEVAL][0];
	int position = 0;
	
	int n;
	for(n=1;n<cyclecnt;n++) {
		if(dz->lfarray[DISTDEL_CYCLEVAL][n] < quiestest_cycle) {
			quiestest_cycle = dz->lfarray[DISTDEL_CYCLEVAL][n];
			position = n;
		}
	}
	return(position);
}

/***************************** WRITE_CYCLE **********************************/

int write_cycle(int start,int end,float *buf,int *obufpos,dataptr dz)
{
	int exit_status;
	float *obuf = dz->sampbuf[2] + *obufpos;
	int partoutbuf  = dz->buflen - *obufpos; 	/* FIND HOW MUCH ROOM LEFT IN OUTBUF */
	int insamps_to_write = end - start;   		/* FIND HOW MANY SAMPLES TO WRITE */
	int outoverflow;		     				/* IF SAMPS-TO-WRITE WON'T FIT IN OUTBUF */
	buf  += start;
	while((outoverflow = insamps_to_write - partoutbuf) > 0) {
		memmove((char *)obuf,(char *)buf,partoutbuf * sizeof(float));
		obuf = dz->sampbuf[2];
		buf += partoutbuf;
		if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
			return(exit_status);
		*obufpos = 0;
		insamps_to_write = outoverflow;
		partoutbuf = dz->buflen;
	}
	memmove((char *)obuf,(char *)buf,insamps_to_write * sizeof(float));
	*obufpos += insamps_to_write; 
	return(CONTINUE);
}
