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

static int	write_cycles(int *obufpos,int cyc_average,dataptr dz);
static int distorta_func_crosbuf(int,int,int *,int,dataptr);
static int distorta_func(int current_buf,int cyc_average,int *obufpos,dataptr dz);
static int get_cyc_average(int *cyc_average,dataptr dz);

/**************************** DISTORT_AVG ******************************
 *
 * This works on groups of N input COMPLETE CYCLES, where the output
 * group is not necessarily the same length as the input group.
 *
 * It requires a global current_pos_in_buf & dz->iparam[DISTORTA_OUTCNT].
 *
 * change_buff now uses cycleno_in_group_at_bufcros to MARK the cycle number at which
 * the buff-boundary is crossed.
 */

int distort_avg(int *current_buf,int initial_phase,int *obufpos,int *current_pos_in_buf,int *cnt,dataptr dz)
{
	int exit_status;
	register int i = *current_pos_in_buf;
	register int n;
	int cyc_average;
	float *b  = dz->sampbuf[*current_buf];
	int cycleno_in_group_at_bufcros = -1;
	for(n=0;n<dz->iparam[DISTORTA_CYCLECNT];n++) {
		dz->lparray[DISTORTA_STARTCYC][n] = i;
		dz->lparray[DISTORTA_CYCLEN][n]   = 0;
		switch(initial_phase) {
		case(1):
			while(b[i]>=0) {
				dz->lparray[DISTORTA_CYCLEN][n]++;
				if(++i >= dz->ssampsread) {
					if((exit_status = change_buff(&b,&cycleno_in_group_at_bufcros,current_buf,dz))!=CONTINUE)
						return(exit_status);
					cycleno_in_group_at_bufcros = n;
					i = 0;
				}
			}
			while(b[i]<=0) {
				dz->lparray[DISTORTA_CYCLEN][n]++;
				if(++i >= dz->ssampsread) {
					if((exit_status = change_buff(&b,&cycleno_in_group_at_bufcros,current_buf,dz))!=CONTINUE)
						return(exit_status);
					cycleno_in_group_at_bufcros = n;
					i = 0;
				}
			}
			break;
		case(-1):
			while(b[i]<=0) {
				dz->lparray[DISTORTA_CYCLEN][n]++;
				if(++i >= dz->ssampsread) {
					if((exit_status = change_buff(&b,&cycleno_in_group_at_bufcros,current_buf,dz))!=CONTINUE)
						return(exit_status);
					cycleno_in_group_at_bufcros = n;
					i = 0;
				}
			}
			while(b[i]>=0) {
				dz->lparray[DISTORTA_CYCLEN][n]++;
				if(++i >= dz->ssampsread) {
					if((exit_status = change_buff(&b,&cycleno_in_group_at_bufcros,current_buf,dz))!=CONTINUE)
						return(exit_status);
					cycleno_in_group_at_bufcros = n;
					i = 0;
				}
			}
			break;
		}
	}
	if((exit_status = get_cyc_average(&cyc_average,dz))<0)
		return(exit_status);
	if(cycleno_in_group_at_bufcros < 0) 	/* cycle set does not cross between buffers */
		exit_status = distorta_func(*current_buf,cyc_average,obufpos,dz);
	else
		exit_status = distorta_func_crosbuf(cyc_average,*current_buf,obufpos,cycleno_in_group_at_bufcros,dz);
	if(exit_status < 0)
		return(exit_status);
	*current_pos_in_buf = i;
	(*cnt)++;
	return(CONTINUE);
}

/****************************** CHANGE_BUFF *****************************/

int change_buff(float **b,int *cycleno_in_group_at_bufcros,int *current_buf,dataptr dz)
{
	int exit_status;
	if(dz->samps_left <= 0)
		return(FINISHED);
	if(*cycleno_in_group_at_bufcros >= 0) {
		sprintf(errstr,"cycle_search exceeds buffer size\n");
		return(GOAL_FAILED);
	}
	*current_buf = !(*current_buf);
	*b = dz->sampbuf[*current_buf];
	if((exit_status = read_samps(*b,dz))<0)
		return(exit_status);
	return(CONTINUE);
}

/*************************** GET_CYC_AVERAGE ***************************/

int get_cyc_average(int *cyc_average,dataptr dz)
{
	register int n;
	int k = 0;
	for(n=0;n<dz->iparam[DISTORTA_CYCLECNT];n++)
		k += dz->lparray[DISTORTA_CYCLEN][n];
	k = round((double)k/(double)dz->iparam[DISTORTA_CYCLECNT]);
	if(k > dz->iparam[DISTORTA_CYCBUFLEN]) {
		sprintf(errstr,"wavelength exceeds maximum size\n");
		return(GOAL_FAILED);
	}
	*cyc_average = k;
	return(FINISHED);
}

/************************* DISTORT_FUNC ***************************/

int distorta_func(int current_buf,int cyc_average,int *obufpos,dataptr dz)
{
	register int n, m;
	int cycindex;
	double sum, zorg;
	int j = 0;
	float *b = dz->sampbuf[current_buf];
	for(m=0;m<cyc_average;m++) {
		sum = 0.0;
		zorg = (double)m/(double)cyc_average;
		for(n=0;n<dz->iparam[DISTORTA_CYCLECNT];n++) {
			cycindex = round(zorg * (double)dz->lparray[DISTORTA_CYCLEN][n]);
			sum += b[dz->lparray[DISTORTA_STARTCYC][n] + cycindex]; 
		}
		/*RWD added cast */
		dz->sampbuf[3][j++] = (float) /*round*/(sum/(double)dz->iparam[DISTORTA_CYCLECNT]);
	}
	return write_cycles(obufpos,cyc_average,dz);
}

/*************************** DISTORT_FUNC_CROSBUF ***********************/

int distorta_func_crosbuf(int cyc_average,int current_buf,int *obufpos,int cycleno_in_group_at_bufcros,dataptr dz)
{
	register int n, m, k;
	int cycindex;
	double sum, zorg;
	int j = 0;
	float *b;
	for(m=0;m<cyc_average;m++) {
		sum = 0.0;
		b = dz->sampbuf[!current_buf];		/* CYCLES BEFORE CROSSOVER CYCLE */
		zorg = (double)m/(double)cyc_average;
		for(n=0;n<cycleno_in_group_at_bufcros;n++) {
			cycindex = round(zorg * (double)dz->lparray[DISTORTA_CYCLEN][n]);
			k = dz->lparray[DISTORTA_STARTCYC][n] + cycindex; 
			sum += b[k]; 
		}				/* CROSSOVER CYCLE */
		cycindex = round(zorg * (double)dz->lparray[DISTORTA_CYCLEN][n]);
		k = dz->lparray[DISTORTA_STARTCYC][n] + cycindex; 
		if(k >= dz->buflen) {
			b  = dz->sampbuf[current_buf];
			k -= dz->buflen;
		}
		sum += b[k]; 
		b = dz->sampbuf[current_buf];		/* CYCLES AFTER CROSSOVER CYCLE */
		for(n=cycleno_in_group_at_bufcros+1;n<dz->iparam[DISTORTA_CYCLECNT];n++) {
			cycindex = round(zorg * (double)dz->lparray[DISTORTA_CYCLEN][n]);
			k = dz->lparray[DISTORTA_STARTCYC][n] + cycindex; 
			sum += b[k]; 
		}
		/*RWD added cast */
		dz->sampbuf[3][j++] = (float) /*round*/(sum/(double)dz->iparam[DISTORTA_CYCLECNT]);
	}
	return write_cycles(obufpos,cyc_average,dz);
}

/************************* WRITE_CYCLES **************************/

int write_cycles(int *obufpos,int cyc_average,dataptr dz)
{
	int exit_status;
	register int k = *obufpos, n, m, j;  
	for(m=0;m<dz->iparam[DISTORTA_CYCLECNT];m++) {
		j = 0;
		for(n=0;n<cyc_average;n++) {
			dz->sampbuf[2][k++] = dz->sampbuf[3][j++];
			if(k >= dz->buflen) {
				if((exit_status = write_samps(dz->sampbuf[2],dz->buflen,dz))<0)
					return(exit_status);
				k = 0;
			}
		}
	}
	*obufpos = k;
	return(FINISHED);
}
