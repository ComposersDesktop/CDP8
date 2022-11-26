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
#include <envel.h>
#include <cdpmain.h>

#include <sfsys.h>

static void apply_envel_to_buffer(int samps_to_process,int envwindow_sampsize,int envcnt,
								  int *cnt,float **envptr,double *step,float *thisval,
								  float *nextval,int incr_cnt,int chans,dataptr dz);
static void apply_env_to_window(int incr_cnt,double step, float startval,
			int startsamp,int chans,int is_log,dataptr dz);
static void step_gen(float *thisval,float *nextval,double *step,float **envptr,int *is_log,int incr_cnt);
static int  reset_file_pos_and_counters(int fileno,dataptr dz);
static void test_apply_env_to_window(int incr_cnt,double step,float startval,
			int startsamp,int chans,int is_log,double *maxval,dataptr dz);
static void test_apply_envel_to_buffer(int samps_to_process,int envwindow_sampsize,int envcnt,int *cnt,
			float **envptr,double *step,float *thisval,float *nextval,int incr_cnt,int chans,
			double *maxval,dataptr dz);
static int  test_impose_envel_on_sndfile(float *env,int envcnt,int bufcnt,int fileno,double *maxval,dataptr dz);
static void adjust_envelope(double adjuster,int envcnt,dataptr dz);

/*************************** IMPOSE_ENVEL [APPLY_THE_ENV] *************************
 *
 * WE Assume that buffersizes have been recalcd etc by this stage,
 * and that they were last-calculated for the CURRENT file!!
 *
 * envcnt MUST BE the TRUE length of the NEW envelope!!!
 */

int impose_envel_on_sndfile(float *env,int envcnt,int bufcnt,int fileno,dataptr dz)
{
	int	   exit_status;
	int   n, cnt;
	double maxval = 0.0;
	int   envwindow_sampsize = dz->iparam[ENV_SAMP_WSIZE];
	int    chans = dz->infile->channels;
	int   incr_cnt = envwindow_sampsize/chans;
	float  *envptr = env;
	double step;
	float  thisval = *envptr;	
	float  nextval = *envptr;	
	if((exit_status = reset_file_pos_and_counters(fileno,dz))<0)
		return(exit_status);

	if(sloom) {
		fprintf(stdout,"INFO: Testing levels of new envelope.\n");
		fflush(stdout);
	}

	if((exit_status = test_impose_envel_on_sndfile(env,envcnt,bufcnt,fileno,&maxval,dz))<0)
		return(exit_status);
	if(maxval > F_MAXSAMP)
		adjust_envelope(F_MAXSAMP/maxval,envcnt,dz);
	envptr  = env;
	thisval = *envptr;
	nextval = *envptr;
	cnt = 0;
	if((exit_status = reset_file_pos_and_counters(fileno,dz))<0)
		return(exit_status);

	if(sloom) {
		fprintf(stdout,"INFO: Imposing new envelope on sound.\n");
		fflush(stdout);
	}
	for(n = 0; n < bufcnt; n++)	{
		if((exit_status = read_samps(dz->sampbuf[0],dz))<0)
			return(exit_status);
		apply_envel_to_buffer
		(dz->ssampsread,envwindow_sampsize,envcnt,&cnt,&envptr,&step,&thisval,&nextval,incr_cnt,chans,dz);
		if(dz->ssampsread > 0) {
			if((exit_status = write_samps(dz->sampbuf[0],dz->ssampsread,dz))<0)
				return(exit_status);
		}
	}
	return(FINISHED);
}

/******************************* APPLY_ENVEL_TO_BUFFER [ENVAPPLY] ******************************
 *
 * Apply envelope to input soundfile, to produce output sound file.
 *
 * If envelope table runs out before sound, last value in table is held.
 */

void apply_envel_to_buffer(int samps_to_process,int envwindow_sampsize,int envcnt,
int *cnt,float **envptr,double *step,float *thisval,float *nextval,int incr_cnt,
int chans,dataptr dz)
{
	int is_log = FALSE;
	int  startsamp = 0;
	while(samps_to_process >= envwindow_sampsize) {
		if(++(*cnt) < envcnt)
			step_gen(thisval,nextval,step,envptr,&is_log,incr_cnt);
		else {
			*thisval = *nextval;
			*step = 0.0;   /* retain last envelope value if we run out of values */
			is_log = FALSE;
		}
		apply_env_to_window(incr_cnt,*step,*thisval,startsamp,chans,is_log,dz);
		startsamp 		 += envwindow_sampsize;
		samps_to_process -= envwindow_sampsize;
	}	
	if(samps_to_process) {					/* Handle any final short buffer */
		if(++(*cnt) < envcnt)
			step_gen(thisval,nextval,step,envptr,&is_log,incr_cnt);
		else {
			*thisval = *nextval;
			*step = 0.0;   /* retain last envelope value if we run out of values */
			is_log = FALSE;
		}
		incr_cnt = samps_to_process/chans;
		apply_env_to_window(incr_cnt,*step,*thisval,startsamp,chans,is_log,dz);
	}
	return;
}

/************************ APPLY_ENV_TO_WINDOW [DO_THE_ENV] ***********************/

void apply_env_to_window
(int incr_cnt,double step, float startval, int startsamp,int chans,int is_log,dataptr dz)
{
	int i, offset, bufpos;
	int chanpos;
	double newval, multiplier, thisstep;
	float *buf = dz->sampbuf[0];
	if(is_log) {
		for(i=0, offset = 0; i<incr_cnt; i++, offset += chans) {
			thisstep    = ((double)i/(double)incr_cnt) * step;
			thisstep    = pow(10,thisstep);
			multiplier  = startval * thisstep;
			for(chanpos=0; chanpos < chans; chanpos++) {
				bufpos  	= startsamp + offset + chanpos;
				newval    	= (double)(buf[bufpos] * multiplier);
				newval      = min(newval,(double)F_MAXSAMP);
				newval      = max(newval,(double)F_MINSAMP);	
				buf[bufpos] = (float) /*round*/(newval);
			}
		}
	} else {
		for(i=0, offset = 0; i<incr_cnt; i++, offset += chans) {
			for(chanpos=0; chanpos < chans; chanpos++) {
				bufpos  	= startsamp + offset + chanpos;
				newval    	= (double)buf[bufpos] * (startval + (step * (double)i));
				newval      = min(newval,(double)F_MAXSAMP);
				newval      = max(newval,(double)F_MINSAMP);	
				buf[bufpos] = (float) /*round*/(newval);
			}
		}
	}
}

/****************************** STEP_GEN ******************************
 *
 * Generates [stereo]-sample-wise step, for each window.
 */

#define STEP_LINEAR_LIMIT	(1.0)

void step_gen(float *thisval,float *nextval,double *step,float **envptr,int *is_log,int incr_cnt)
{   
	double valstep, absstep;
	*thisval = *nextval;
	(*envptr)++;
	*nextval = **envptr;
	valstep = *nextval - *thisval; 
	*step   = valstep/(double)incr_cnt;
/* LOG INTERP FOR EXTREME CASES ONLY --> */
	if((absstep = fabs(*step)) > STEP_LINEAR_LIMIT) {
		*step   = log10((*nextval)/(*thisval));
		*is_log = TRUE;
	}
/* <-- LOG INTERP FOR EXTREME CASES ONLY */
	return;
}
	
/**************************** RESET_FILE_POS_AND_COUNTERS *************************/

int reset_file_pos_and_counters(int fileno,dataptr dz)
{
	if((sndseekEx(dz->ifd[fileno],0,0))<0) {					/* reset all file pointers and counters */
		sprintf(errstr,"seek error: reset_file_pos_and_counters()\n");
		return(SYSTEM_ERROR);
	}
	dz->total_samps_read = 0;
	dz->samps_left = dz->insams[fileno];
	return(FINISHED);
}

/*************************** TEST_IMPOSE_ENVEL *************************
 *
 * WE Assume that buffersizes have been recalcd etc by this stage,
 * and that they were last-calculated for the CURRENT file!!
 *
 * envcnt MUST BE the TRUE length of the NEW envelope!!!
 */

int test_impose_envel_on_sndfile(float *env,int envcnt,int bufcnt,int fileno,double *maxval,dataptr dz)
{
	int	   exit_status;
	int   n, cnt = 0;
	int   envwindow_sampsize = dz->iparam[ENV_SAMP_WSIZE] ;
	int    chans = dz->infile->channels;
	int   incr_cnt = envwindow_sampsize/chans;
	float  *envptr = env;
	double step    = 0.0;
	float  thisval = *envptr;	
	float  nextval = *envptr;	

	if((exit_status = reset_file_pos_and_counters(fileno,dz))<0)
			return(exit_status);
	for(n = 0; n < bufcnt; n++)	{
		if((exit_status = read_samps(dz->sampbuf[0],dz))<0)
			return(exit_status);
		if(sloom)
			display_virtual_time(dz->total_samps_read,dz);
		test_apply_envel_to_buffer
		(dz->ssampsread,envwindow_sampsize,envcnt,&cnt,&envptr,&step,&thisval,&nextval,incr_cnt,chans,maxval,dz);
	}
	return(FINISHED);
}

/******************************* TEST_APPLY_ENVEL_TO_BUFFER [ENVAPPLY] ******************************/

void test_apply_envel_to_buffer(int samps_to_process,int envwindow_sampsize,int envcnt,int *cnt,float **envptr,
double *step,float *thisval,float *nextval,int incr_cnt,int chans,
double *maxval,dataptr dz)
{
	int is_log = FALSE;
	int  startsamp = 0;
	while(samps_to_process >= envwindow_sampsize) {
		if(++(*cnt) < envcnt)
			step_gen(thisval,nextval,step,envptr,&is_log,incr_cnt);
		else {
			*thisval = *nextval;
			*step = 0.0;   /* retain last envelope value if we run out of values */
			is_log = FALSE;
		}
		test_apply_env_to_window(incr_cnt,*step,*thisval,startsamp,chans,is_log,maxval,dz);
		startsamp 		 += envwindow_sampsize;
		samps_to_process -= envwindow_sampsize;
	}	
	if(samps_to_process) {					/* Handle any final short buffer */
		if(++(*cnt) < envcnt)
			step_gen(thisval,nextval,step,envptr,&is_log,incr_cnt);
		else {
			*thisval = *nextval;
			*step = 0.0;   /* retain last envelope value if we run out of values */
			is_log = FALSE;
		}
		incr_cnt = samps_to_process/chans;
		test_apply_env_to_window(incr_cnt,*step,*thisval,startsamp,chans,is_log,maxval,dz);
	}
	return;
}

/************************ TEST_APPLY_ENV_TO_WINDOW [DO_THE_ENV] ***********************/

void test_apply_env_to_window
(int incr_cnt,double step, float startval,
int startsamp, int chans,int is_log,double *maxval,dataptr dz)
{
	int i, offset, bufpos;
	int chanpos;
	double newval, multiplier, thisstep;
	float *buf = dz->sampbuf[0];
	if(is_log) {
		for(i=0, offset = 0; i<incr_cnt; i++, offset += chans) {
			
			thisstep    = ((double)i/(double)incr_cnt) * step;
			thisstep    = pow(10,thisstep);
			multiplier  = startval * thisstep;
			for(chanpos=0; chanpos < chans; chanpos++) {
				bufpos  	= startsamp + offset + chanpos;
				newval    	= (double)(buf[bufpos] * multiplier);
				*maxval = max(*maxval,fabs(newval));
			}
		}
	} else {
		for(i=0, offset = 0; i<incr_cnt; i++, offset += chans) {
			for(chanpos=0; chanpos < chans; chanpos++) {
				bufpos  = startsamp + offset + chanpos;
				newval  = (double)buf[bufpos] * (startval + (step * (double)i));
				*maxval = max(*maxval,fabs(newval));
			}
		}
	}
}

/************************ ADJUST_ENVELOPE ***********************/

void adjust_envelope(double adjuster,int envcnt,dataptr dz)
{
	float *envptr = dz->env;
	float *envend = envptr + envcnt;
	while(envptr < envend) {
		*envptr = (float)(*envptr * adjuster);
		envptr++;
	}
}					   
