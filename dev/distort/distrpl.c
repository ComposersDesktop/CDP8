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

static int 	do_cycle_loudrep(int n,int current_buf,int current_pos_in_buf,int *obufpos,dataptr dz);
static int 	do_cycle_loudrep_crosbuf
			(int n,int cycleno_in_group_at_bufcross,int *current_buf,int current_pos_in_buf,int *obufpos,dataptr dz);
static int get_ffcycle(int,dataptr);
static int	write_cycle(int start, int end, int current_buf, int *obufpos,dataptr dz);

/******************************* DISTORT_RPL *****************************/

int distort_rpl(int *current_buf,int initial_phase,int *obufpos,int *current_pos_in_buf,int *cnt,dataptr dz)
{
	int exit_status;
	int n = 0;
	register int i = *current_pos_in_buf;
	int	cycleno_in_group_at_bufcross = -1;
	float *b  = dz->sampbuf[*current_buf];
	switch(initial_phase) {
	case(1):
		for(n=0;n<dz->iparam[DISTRPL_CYCLECNT];n++) {
			dz->lfarray[DISTRPL_CYCLEVAL][n] = 0;
			dz->lparray[DISTRPL_STARTCYC][n] = i;
			while(b[i]>=0.0) {		/* 8 */
				dz->lfarray[DISTRPL_CYCLEVAL][n] += (float) fabs(b[i]);
				if(++i >= dz->ssampsread) {	/* 10 */
					if((exit_status = change_buff(&b,&cycleno_in_group_at_bufcross,current_buf,dz))!=CONTINUE)
						return(exit_status);
					cycleno_in_group_at_bufcross = n;
					i = 0;
				}
			}
			while(b[i]<=0.0) {		/* 18a */
				dz->lfarray[DISTRPL_CYCLEVAL][n] += (float) fabs(b[i]);
				if(++i >= dz->ssampsread) {
					if((exit_status = change_buff(&b,&cycleno_in_group_at_bufcross,current_buf,dz))!=CONTINUE)
						return(exit_status);
					cycleno_in_group_at_bufcross = n;
					i = 0;
				}
			}
		}
		break;
	case(-1):						/* 18b */
		for(n=0;n<dz->iparam[DISTRPL_CYCLECNT];n++) {
			dz->lfarray[DISTRPL_CYCLEVAL][n] = 0.0;
			dz->lparray[DISTRPL_STARTCYC][n] = i;
			while(b[i]<=0.0) {
				dz->lfarray[DISTRPL_CYCLEVAL][n] += (float) fabs(b[i]);
				if(++i >= dz->ssampsread) {
					if((exit_status = change_buff(&b,&cycleno_in_group_at_bufcross,current_buf,dz))!=CONTINUE)
						return(exit_status);
					cycleno_in_group_at_bufcross = n;
					i = 0;
				}
			}
			while(b[i]>=0.0) {
				dz->lfarray[DISTRPL_CYCLEVAL][n] += (float) fabs(b[i]);
				if(++i >= dz->ssampsread) {
					if((exit_status = change_buff(&b,&cycleno_in_group_at_bufcross,current_buf,dz))!=CONTINUE)
						return(exit_status);
					cycleno_in_group_at_bufcross = n;
					i = 0;
				}
			}
		}
		break;
	}
	if(n) {
		if(cycleno_in_group_at_bufcross >=0)
			do_cycle_loudrep_crosbuf(n,cycleno_in_group_at_bufcross,current_buf,i,obufpos,dz);
    	else
			do_cycle_loudrep(n,*current_buf,i,obufpos,dz);
	}
	*current_pos_in_buf = i;
	(*cnt)++;
	return(CONTINUE);
}

/************************** DO_CYCLE_LOUDREP ************************/

int do_cycle_loudrep(int n,int current_buf,int current_pos_in_buf,int *obufpos,dataptr dz)
{
	int exit_status;
   int loudest, k;
    dz->lparray[DISTRPL_STARTCYC][n] = current_pos_in_buf;
    loudest = get_ffcycle(n,dz);
    for(k=0;k<dz->iparam[DISTRPL_CYCLECNT];k++) {
    	if((exit_status = 
    	write_cycle(dz->lparray[DISTRPL_STARTCYC][loudest],dz->lparray[DISTRPL_STARTCYC][loudest+1],current_buf,obufpos,dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}

/************************** DO_CYCLE_LOUDREP_CROSBUF ************************/

int do_cycle_loudrep_crosbuf
(int n,int cycleno_in_group_at_bufcross,int *current_buf,int current_pos_in_buf,int *obufpos,dataptr dz)
{
#define LOUDEST_CYCLE_AFTER_BUFCROSS	(0)
#define LOUDEST_CYCLE_AT_BUFCROSS		(1)
#define LOUDEST_CYCLE_BEFORE_BUFCROSS	(2)
	int  exit_status;
	int loudest, k;
	int  is_cross;
	dz->lparray[DISTRPL_STARTCYC][n] = current_pos_in_buf;
	loudest = get_ffcycle(n,dz);
	if(loudest > cycleno_in_group_at_bufcross)
		is_cross = LOUDEST_CYCLE_AFTER_BUFCROSS;
	else if(cycleno_in_group_at_bufcross == loudest)
		is_cross = LOUDEST_CYCLE_AT_BUFCROSS;
	else
		is_cross = LOUDEST_CYCLE_BEFORE_BUFCROSS;

	for(k=0;k<dz->iparam[DISTRPL_CYCLECNT];k++) {
		switch(is_cross) {
		case(LOUDEST_CYCLE_AFTER_BUFCROSS):
			if((exit_status = 
			write_cycle(dz->lparray[DISTRPL_STARTCYC][loudest],dz->lparray[DISTRPL_STARTCYC][loudest+1],*current_buf,obufpos,dz))<0)
				return(exit_status);
			break;
		case(LOUDEST_CYCLE_AT_BUFCROSS):
			(*current_buf) = !(*current_buf);	/* LOOK IN OTHER BUFFER */
			if((exit_status = 
			write_cycle(dz->lparray[DISTRPL_STARTCYC][loudest],dz->buflen,*current_buf,obufpos,dz))<0)
				return(exit_status);
			(*current_buf) = !(*current_buf);	/* THEN IN CURRENT BUFFER */
			if((exit_status = 
				write_cycle(0L,dz->lparray[DISTRPL_STARTCYC][loudest+1],*current_buf,obufpos,dz))<0)
			return(exit_status);
			break;	
		case(LOUDEST_CYCLE_BEFORE_BUFCROSS):
			(*current_buf) = !(*current_buf);	/* LOOK IN OTHER BUFFER */
			if((exit_status = 
			write_cycle(dz->lparray[DISTRPL_STARTCYC][loudest],dz->lparray[DISTRPL_STARTCYC][loudest+1],*current_buf,obufpos,dz))<0)
				return(exit_status);
			(*current_buf) = !(*current_buf);	/* RETURN TO CURRENT BUFFER */
			break;	
		default:
			sprintf(errstr,"Unknown case in do_cycle_loudrep_crosbuf()\n");
			return(PROGRAM_ERROR);
		}
	}
	return(FINISHED);
}

/************************ GET_FFCYCLE *******************************
 *
 * Find loudest cycle.
 */

int get_ffcycle(int k,dataptr dz)
{
	int /*ffcycle = 0,*/ position = 0;
	float ffcycle = 0;
	int n;
	for(n=0;n<k;n++) {
		if(dz->lfarray[DISTRPL_CYCLEVAL][n] > ffcycle) {
			ffcycle = dz->lfarray[DISTRPL_CYCLEVAL][n];
			position = n;
		}
	}
	return(position);
}

/***************************** WRITE_CYCLE **********************************/

int write_cycle(int start, int end,int current_buf, int *obufpos,dataptr dz)
{
	int exit_status;
	float *b = dz->sampbuf[current_buf];
	int k;
	int partoutbuf  = dz->buflen - *obufpos;				/* FIND HOW MUCH ROOM LEFT IN OUTBUF */
	int insamps_to_write = end - start;   				/* FIND HOW MANY SAMPLES TO WRITE */
	int outoverflow;		     						/* IF SAMPS-TO-WRITE WON'T FIT IN OUTBUF */
	if((outoverflow = insamps_to_write - partoutbuf) > 0) {
		while(*obufpos < dz->buflen)						/* WRITE UP TO END OF OUTBUF */
			dz->sampbuf[2][(*obufpos)++] = b[start++];
		if((exit_status = write_samps(dz->sampbuf[2],dz->buflen,dz))<0)
			return(exit_status);
		/* WRITE OUTBUF TO FILE */
		*obufpos = 0;							 		/* RESET OUTBUF BUFFER INDEX TO START OF BUF */
		insamps_to_write = outoverflow; 				/* RESET INSAMPS-TO-WRITE TO OVERFLOW */
	}
	for(k = 0;k < insamps_to_write; k++)
		dz->sampbuf[2][(*obufpos)++] = b[start++]; 	/*WRITE (REMAINING) INSMPS TO OUTBUF */
	return(FINISHED);
}
