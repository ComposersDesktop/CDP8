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

static int 	do_cycle_tele(int n,int current_buf,int *obufpos,dataptr dz);
static int 	do_cycle_tele_crosbuf(int n,int cycleno_in_group_at_bufcros,int current_buf,int *obufpos,dataptr dz);
static int get_longcycle(int,dataptr);
static int get_meancycle(int,dataptr);
static float 	indexed_value(float *b,int k,double ratio,dataptr dz);
static float 	indexed_value_at_crosbuf(float *b,int k,double ratio,int current_buf,dataptr dz);

/******************************* DISTORT_TEL ******************************/
						 
int distort_tel(int *current_buf,int initial_phase,int *obufpos,int *current_pos_in_buf,int *cnt,dataptr dz)
{
	int exit_status;
	register int i = *current_pos_in_buf;
	int n = 0;
	int cycleno_in_group_at_bufcros = -1;
	float *b  = dz->sampbuf[*current_buf];
	switch(initial_phase) {				/* 4 */
	case(1):
		for(n=0;n<dz->iparam[DISTTEL_CYCLECNT];n++) {		/* 5 */
			dz->lparray[DISTTEL_CYCLEVAL][n] = 0;			/* 6 */
			dz->lparray[DISTTEL_STARTCYC][n] = i;		/* 7 */
			while(b[i]>=0.0) {		/* 8 */	
				/* RWD NB this is counting samples */
				dz->lparray[DISTTEL_CYCLEVAL][n]++;
				if(++i >= dz->ssampsread) {	/* 10 */
					if((exit_status = change_buff(&b,&cycleno_in_group_at_bufcros,current_buf,dz))!=CONTINUE)
						return(exit_status);
					cycleno_in_group_at_bufcros = n;
					i = 0;
				}
			}
			while(b[i]<=0.0) {		/* 18a */
				dz->lparray[DISTTEL_CYCLEVAL][n]++;
				if(++i >= dz->ssampsread) {
					if((exit_status = change_buff(&b,&cycleno_in_group_at_bufcros,current_buf,dz))!=CONTINUE)
						return(exit_status);
					cycleno_in_group_at_bufcros = n;
					i = 0;
				}
			}
		}
		break;
	case(-1):						/* 18b */
		for(n=0;n<dz->iparam[DISTTEL_CYCLECNT];n++) {
			dz->lparray[DISTTEL_CYCLEVAL][n] = 0;
			dz->lparray[DISTTEL_STARTCYC][n] = i;
			while(b[i]<=0.0) {
				dz->lparray[DISTTEL_CYCLEVAL][n]++;
				if(++i >= dz->ssampsread) {
					if((exit_status = change_buff(&b,&cycleno_in_group_at_bufcros,current_buf,dz))!=CONTINUE)
						return(exit_status);
					cycleno_in_group_at_bufcros = n;
					i = 0;
				}
			}
			while(b[i]>=0.0) {
				dz->lparray[DISTTEL_CYCLEVAL][n]++;
				if(++i >= dz->ssampsread) {
					if((exit_status = change_buff(&b,&cycleno_in_group_at_bufcros,current_buf,dz))!=CONTINUE)
						return(exit_status);
					cycleno_in_group_at_bufcros = n;
					i = 0;
				}
			}
		}
		break;
	}
	if(n) {
		if(cycleno_in_group_at_bufcros >= 0) {
			exit_status = do_cycle_tele_crosbuf(n,cycleno_in_group_at_bufcros,*current_buf,obufpos,dz);
		} else {
			exit_status = do_cycle_tele(n,*current_buf,obufpos,dz);
		}
		if(exit_status<0)
			return(exit_status);
	}
	*current_pos_in_buf = i;
	(*cnt)++;
	return(CONTINUE);
}

/*************************** DO_CYCLE_TELE *************************/

int do_cycle_tele(int n,int current_buf,int *obufpos,dataptr dz)
{
	int exit_status;
	float *b = dz->sampbuf[current_buf];
	double ratio;
	int k, j, reflength/*, cyclesum*/;
	float cyclesum;
	/*int*/float average_value;
	if(dz->vflag[IS_DISTTEL_AVG])
		reflength = get_meancycle(n,dz);
	else
		reflength = get_longcycle(n,dz);
	for(j = 0;j < reflength; j++) {
		ratio = (double)j/(double)reflength;
		cyclesum = 0.0;
		k = 0;
		while(k<n)
			cyclesum += indexed_value(b,k++,ratio,dz);
		average_value = (float) /*round*/ ((double)cyclesum/(double)n);
		if((exit_status = output_val(average_value,obufpos,dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}

/*************************** DO_CYCLE_TELE_CROSBUF *************************/

int do_cycle_tele_crosbuf(int n,int cycleno_in_group_at_bufcros,int current_buf,int *obufpos,dataptr dz)
{							 
	int exit_status;
	float *b;
	double ratio;
	int k, j, reflength /*, cyclesum*/;
	float cyclesum;
	/*int*/float average_value;
	if(dz->vflag[IS_DISTTEL_AVG])
		reflength = get_meancycle(n,dz);
	else
		reflength = get_longcycle(n,dz);
	for(j = 0;j < reflength; j++) {
		ratio = (double)j/(double)reflength;
		cyclesum = 0.0;
		k = 0;
		b = dz->sampbuf[!current_buf];
		while(k < cycleno_in_group_at_bufcros)
			cyclesum += indexed_value(b,k++,ratio,dz);
		cyclesum += indexed_value_at_crosbuf(b,k++,ratio,current_buf,dz);
		b = dz->sampbuf[current_buf];
		while(k<n)
			cyclesum += indexed_value(b,k++,ratio,dz);
		average_value = (float) /*round*/((double)cyclesum/(double)n);
		if((exit_status = output_val(average_value,obufpos,dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}


/***************************** INDEXED_VALUE ***********************/

float indexed_value(float *b,int k,double ratio,dataptr dz)
{
	int index = round((double)(dz->lparray[DISTTEL_CYCLEVAL][k])*ratio);
	index += dz->lparray[DISTTEL_STARTCYC][k];
	return b[index];
}

/*********************** INDEXED_VALUE_AT_CROSBUF ***********************/

float indexed_value_at_crosbuf(float *b,int k,double ratio,int current_buf,dataptr dz)
{
	int index = round((double)(dz->lparray[DISTTEL_CYCLEVAL][k])*ratio);
	index += dz->lparray[DISTTEL_STARTCYC][k];
	if(index >= dz->buflen) {
		index -= dz->buflen;
		b = dz->sampbuf[current_buf];
	}
	return b[index];
}

/************************ GET_LONGCYCLE *******************************
 *
 * Find longest cycle.
 */

int get_longcycle(int k,dataptr dz)
{
	int longest = 0;
	int n;
	for(n=0;n<k;n++) {
		if(dz->lparray[DISTTEL_CYCLEVAL][n] > longest)
			longest = dz->lparray[DISTTEL_CYCLEVAL][n];
	}
	return(longest);
}

/************************ GET_MEANCYCLE *******************************
 *
 * Find average cycle length.
 */

int get_meancycle(int k,dataptr dz)
{
	int sum = 0, mean;
	int n;
	for(n=0;n<k;n++)
		sum += dz->lparray[DISTTEL_CYCLEVAL][n];
	mean = round((double)sum/(double)k);
	return(mean);
}

/************************* OUTPUT_VAL **********************/

int output_val(float value,int *obufpos,dataptr dz)
{
	int exit_status;
	dz->sampbuf[2][(*obufpos)++] = value;
	if(*obufpos>=dz->buflen) {
		if((exit_status = write_samps(dz->sampbuf[2],dz->buflen,dz))<0)
			return(exit_status);
		*obufpos = 0;
	}
	return(FINISHED);
}
