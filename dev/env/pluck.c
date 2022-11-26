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
#include <filetype.h>
#include <envel.h>
#include <cdpmain.h>

#include <sfsys.h>
#include <osbind.h>


static int  do_wrap(int samps_to_wrap,dataptr dz);
static int  read_rest_of_file(int initial_bufsize,dataptr dz);
static int  pluck(dataptr dz);
static int envsyn_env(int *phase,int cyclestart,int *cycle_end,double one_less_trof,dataptr dz);

static int  	envsyn_troffed(int cyclestart,int *cyclend,double one_less_trof,int *phase,dataptr dz);
static int 		envsyn_userdef(int cyclestart,int *cyclend,int *phase,dataptr dz);
static int  	envsyn_rising( int cyclestart,int *cyclend,double one_less_trof,int *phase,dataptr dz);
static int  	envsyn_falling(int cyclestart,int *cyclend,double one_less_trof,int *phase,dataptr dz);

static double	do_firsthalf_trof(int k,int cyclehlf,double one_less_trof,dataptr dz);
static double	do_lasthalf_trof(int k,int cyclehlf,double one_less_trof,dataptr dz);
static double	do_rising(int k,int cyclelen,double one_less_trof,dataptr dz);
static double	do_falling(int k,int cyclelen,double one_less_trof,dataptr dz);

static int		read_from_user_envelope(double *env_val,double table_index,int init,dataptr dz);

/****************************** ENVELOPE_PLUCK *************************/

int envelope_pluck(dataptr dz)
{
	
	int exit_status;
	int initial_bufsize;
	int first_read_offset = dz->sampbuf[PLK_INITBUF] - dz->bigbuf;
	dz->buflen -= first_read_offset;			/* size of buffer from where first read takes place */
	if((exit_status = read_samps(dz->sampbuf[PLK_INITBUF],dz))<0)
		return(exit_status);		
	initial_bufsize = dz->buflen;
    if((exit_status = pluck(dz))<0)
		return(exit_status);		
	dz->buflen = dz->iparam[ENV_PLK_OBUFLEN];		/* size of buffer for subsequent reads */
    return read_rest_of_file(initial_bufsize,dz);
}

/**************************** PLUCK ******************************
 *
 * (1)  goalbufend is the last sample of the cycle in the input file buffer from which we will
 *      derive the pluck attack. We read it backwards, from this LAST sample.
 * (2)  The outplukptr points to the position in the output buffer where we
 *      are WRITING pluck-form wavecycles. We write these BKWDS
 *		from here to start of dz->sampbuf[PLK_OUTBUF].
 * (3)	Set goalbufptr to start(i.e. end!!) of goalbuf.
 * (4)	Inverse_index is a BACKWARDS count of the samples in the pluck.
 * (5)	For each wavecycle of Pluck attack..
 * (6) 	For each sample in wavecycle....
 * (7) 	Noiselevel is directly proportional to distance (bkwds) into pluck.
 * (8)	Then reduced via exponential curve.
 * (9)	Becoming this fraction of the maximum amplitude.
 * (10)	Noise input itself is a random value in range +- this noiselevel value.
 * (11) Reinitialise goalbufptr for eack wavecycle.
 * (13a)By the time we reach the start of the pluck, signal will cause#
 *		spurious foldover when added to orig signal: but this will simply make
 *		it more like noise!!
 * (12)	Calculate sample length of initial risetime.
 * (13)	Finally, envelope start of pluck!!
 */

int pluck(dataptr dz)
{
	int   n, m, inverse_index;
	float  *goalbufend, *goalbufptr, *outplukptr;
	double noise_level;
	/*int*/double   thisnoise;
    int   startup;
	int   pluklen = dz->iparam[ENV_PLK_CYCLEN] * dz->iparam[ENV_PLK_WAVELEN];
	if((goalbufend = dz->sampbuf[PLK_PLUKEND] + dz->iparam[ENV_PLK_WAVELEN] - 1) >=dz->sampbuf[PLK_BUFEND]) {
		sprintf(errstr,"End of goal pluck_cycle falls outside buffer: Insufficient memory to proceed.\n");  
		return(GOAL_FAILED);												/* 1 */
	}	
	if((outplukptr = dz->sampbuf[PLK_PLUKEND]  - 1) < dz->bigbuf) {			/* 2 */
		sprintf(errstr,"Buffer accounting problem.\n");  
		return(PROGRAM_ERROR);
	}	
	goalbufptr = goalbufend;												/* 3 */
	inverse_index = 0;														/* 4 */
	for(n=0;n<dz->iparam[ENV_PLK_CYCLEN];n++) {								/* 5 */
		for(m=0;m<dz->iparam[ENV_PLK_WAVELEN];m++) {						/* 6 */
			noise_level = (double)inverse_index/(double)pluklen; 			/* 7 */
			noise_level = pow(noise_level,dz->param[ENV_PLK_DECAY]);		/* 8 */
			noise_level = noise_level * (double)F_MAXSAMP;					/* 9 */
			thisnoise   = /*round*/ (((drand48() * 2.0) - 1.0) * noise_level);	/* 10 */
			*outplukptr = (float)((*goalbufptr) + thisnoise);
			outplukptr--;
			goalbufptr--;
			inverse_index++;
		}
		goalbufptr = goalbufend;											/* 11 */
	}
    startup = round(ENV_PLK_ONSET_TIME * MS_TO_SECS * (double)dz->infile->srate); /* 12 */
	startup = min(startup,dz->iparam[ENV_PLK_CYCLEN] * dz->iparam[ENV_PLK_WAVELEN]);
	for(n=0;n<startup;n++) {
		outplukptr++;														/* 13 */
		*outplukptr = (float) /*round */(*outplukptr * (double)n/(double)startup);
	}	
	return(FINISHED);
}

/*********************** READ_REST_OF_FILE ************************
 *
 * (1)	If the input buffer was NOT filled, then we must be at end of file: so write data and return.
 * (2)	Samples to write is those read,
 *		MINUS the length of initial replaced segment (dz->iparam[ENV_PLK-STSAMP])
 *		PLUS  the added initial segment (dz->iparam[ENV_PLK_OWRAPLEN]).
 *	 ELSE
 * (3)	Write a standard buffer_full, from start of obuf_preflow (where pluck has been constructed).
 * (4)  This leaves, [ENV_PLK_OWRAPLEN] samples in end of dz->sampbuf[PLK_OUTBUF],
 *      SO   samps_to_wrap = [ENV_PLK_OWRAPLEN];
 * (6)  Do the wrap-around.
 * (8)  Whilst ever there is something in wrap-around part of buffer...
 * (10)	Try to read more samples (may get full-buff, part-buf 
 *		or zero if file already exhausted).
 * (11)	Total samps to write is the wrapped samps plus those just read.
 * (12) If this leaves no samps to wrap (i.e. buffer is NOT full, therefore FILE must be exhausted), 
 *		write all these samps to out and quit.
 * (13)	ELSE, write a bufferfull and do another wrap-around.
 */

int read_rest_of_file(int initial_bufsize,dataptr dz)
{
	int  exit_status;
	int samps_to_write, samps_to_wrap;
	int pluklen = dz->iparam[ENV_PLK_CYCLEN] * dz->iparam[ENV_PLK_WAVELEN];

	if(dz->ssampsread < (int)(initial_bufsize)) {										/* 1 */
		if((samps_to_write = dz->ssampsread - dz->iparam[ENV_PLK_ENDSAMP] + pluklen)>0) { /* 2 */
			if((exit_status = write_samps(dz->sampbuf[PLK_OUTBUF],samps_to_write,dz))<0)
				return(exit_status);
		}
	} else {
		if((exit_status = write_samps(dz->sampbuf[PLK_OUTBUF],dz->buflen,dz))<0)			/* 3 */
			return(exit_status);
		samps_to_wrap = dz->iparam[ENV_PLK_OWRAPLEN];													/* 4 */
		if((exit_status = do_wrap(samps_to_wrap,dz))<0)													/* 6 */
			return(exit_status);
		while(samps_to_wrap > 0) {																		/* 8 */
			if((exit_status = read_samps(dz->sampbuf[PLK_INBUF],dz))<0)							/* 10 */
				return(exit_status);
			samps_to_write = samps_to_wrap + dz->ssampsread;        									/* 11 */
			if((samps_to_wrap = samps_to_write - dz->buflen) <= 0) {									/* 12 */
				if(samps_to_write > 0) {
					if((exit_status = write_samps(dz->sampbuf[PLK_OUTBUF],samps_to_write,dz))<0)
						return(exit_status);
				}
			} else {
				if((exit_status = write_samps(dz->sampbuf[PLK_OUTBUF],dz->buflen,dz))<0)	/* 13 */
					return(exit_status);
				if((exit_status = do_wrap(samps_to_wrap,dz))<0)														
					return(exit_status);
			}
		}
	}						
	return(FINISHED);
}

/*********************** DO_WRAP ************************/

int do_wrap(int samps_to_wrap,dataptr dz) 
{
	memmove((char *)dz->sampbuf[PLK_OUTBUF],(char *)dz->sampbuf[PLK_OBUFWRAP],(samps_to_wrap)*sizeof(float));
	return(FINISHED);
}

/************************** ENVSYN **************************/

int envsyn(dataptr dz)
{
	int exit_status;
	int current_pos = 0, cyclelen, cycle_end, done = 0;
	int phase, initial_phase;
	double thistime = 0.0, one_less_trof;
	int samps_needed;

	dz->param[ENVSYN_WSIZE] *= MS_TO_SECS;
	samps_needed = (int)floor((dz->param[ENVSYN_DUR]/ dz->param[ENVSYN_WSIZE]) + 1);
	dz->tempsize = samps_needed  * sizeof(float);
	if((exit_status = read_values_from_all_existing_brktables(thistime,dz)) < 0)
		return(exit_status);
	one_less_trof = 1.0 - dz->param[ENVSYN_TROF];
	cyclelen = (int)round(dz->param[ENVSYN_CYCLEN]/dz->param[ENVSYN_WSIZE]);
	cycle_end = cyclelen;
	phase  = (int)round((double)cyclelen * dz->param[ENVSYN_STARTPHASE]);
	initial_phase = phase;
	for (;;) {
		if(cyclelen < 2) {
			sprintf(errstr,"ENVELOPE DURATION TOO SHORT FOR WINDOWSIZE at time %lf\n",
			(dz->total_samps_written + current_pos) * dz->param[ENV_WSIZE]);
			return(DATA_ERROR);
		}
		if((exit_status = envsyn_env(&phase,current_pos,&cycle_end,one_less_trof,dz))< 0)
			return(exit_status);
		if((done += cyclelen) >= samps_needed + initial_phase)
			break;
		thistime += dz->param[ENVSYN_CYCLEN];
		if((exit_status = read_values_from_all_existing_brktables(thistime,dz)) < 0)
			return(exit_status);
		one_less_trof = 1.0 - dz->param[ENVSYN_TROF];
		cyclelen = (int)round(dz->param[ENVSYN_CYCLEN]/dz->param[ENVSYN_WSIZE]);
		current_pos = cycle_end;
		cycle_end = current_pos + cyclelen;
	}
	if(cycle_end > 0) {
		if((exit_status = write_samps(dz->flbufptr[0],cycle_end,dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}

/************************** ENVSYN_ENV **************************/

int envsyn_env(int *phase,int cyclestart,int *cycle_end,double one_less_trof,dataptr dz)
{
	switch(dz->mode) {
	case(ENVSYN_TROFFED):	return envsyn_troffed(cyclestart,cycle_end,one_less_trof,phase,dz);
	case(ENVSYN_USERDEF):	return envsyn_userdef(cyclestart,cycle_end,phase,dz);
	case(ENVSYN_RISING):   	return envsyn_rising(cyclestart,cycle_end,one_less_trof,phase,dz);
	case(ENVSYN_FALLING):  	return envsyn_falling(cyclestart,cycle_end,one_less_trof,phase,dz);
	default:
		sprintf(errstr,"Unknown case: envsyn_env()\n");
		return(PROGRAM_ERROR);
	}
	return(CONTINUE);
}

/************************** ENVSYN_TROFFED *******************************/

int envsyn_troffed(int cyclestart,int *cyclend,double one_less_trof,int *phase,dataptr dz)
{
	int exit_status;
	register int k = 0, j = cyclestart;
	int cyclelen = *cyclend - cyclestart;
	int cyclehlf = cyclelen/2;
	int cyclemid = cyclestart + cyclehlf;
	float *b = dz->flbufptr[0];
	if(*phase > 0) {
		if(*phase >= cyclehlf) {
			cyclemid = -1;
			k = (*phase - cyclehlf);
		} else {
			cyclemid -= *phase;
			k = *phase;
		}
		(*cyclend) -= *phase;
		*phase = 0;
	}
	while(j<cyclemid) {
		b[j] = (float)do_firsthalf_trof(k,cyclehlf,one_less_trof,dz);
		k++;
		j++;
		if(j >= dz->buflen) {
			if((exit_status = write_samps(b,dz->buflen,dz))<0)
				return(exit_status);
			j = 0;
			*cyclend -= dz->buflen;
		}
	}
	if(cyclemid >= 0)
		k = 0;
	cyclehlf = cyclelen - cyclehlf;
	while(j < *cyclend) {
		b[j] = (float)do_lasthalf_trof(k,cyclehlf,one_less_trof,dz);
		k++;
		j++;
		if(j >= dz->buflen) {
			if((exit_status = write_samps(b,dz->buflen,dz))<0)
				return(exit_status);
			j = 0;
			*cyclend -= dz->buflen;
		}
	}
	return(FINISHED);
}

/*************************** ENVSYN_USERDEF ***************************/

int envsyn_userdef(int cyclestart,int *cyclend,int *phase,dataptr dz)
{
	int exit_status;
	register int k = 0, j = cyclestart;
	int cyclelen = *cyclend - cyclestart;
	double index;
	double z;
	int init = 1;
	float *b = dz->flbufptr[0];
	if(*phase > 0) {
		k = *phase;
		(*cyclend) -= *phase;
		*phase = 0;
	}
	while(j<*cyclend) {
		index = (double)k/(double)cyclelen;
		if((exit_status = read_from_user_envelope(&z,index,init,dz))<0)
			return(exit_status);
		b[j]  = (float)z;
		k++;
		j++;
		if(j >= dz->buflen) {
			if((exit_status = write_samps(b,dz->buflen,dz))<0)
				return(exit_status);
			j = 0;
			*cyclend -= dz->buflen;
		}
		init = 0;
	}
	return(FINISHED);
}

/*************************** ENVSYN_RISING ***********************/

int envsyn_rising(int cyclestart,int *cyclend,double one_less_trof,int *phase,dataptr dz)
{
	int exit_status;
	register int k = 0, j = cyclestart;
	int cyclelen = *cyclend - cyclestart;
	float *b = dz->flbufptr[0];
	
	if(*phase > 0) {
		k = *phase;
		(*cyclend) -= *phase;
		*phase = 0;
	}
	while(j<*cyclend) {
		b[j] = (float)do_rising(k,cyclelen - 1,one_less_trof,dz);
		k++;								 
		j++;
		if(j >= dz->buflen) {
			if((exit_status = write_samps(b,dz->buflen,dz))<0)
				return(exit_status);
			j = 0;
			*cyclend -= dz->buflen;
		}
	}
	return(FINISHED);
}

/********************** ENVSYN_FALLING *************************/

int envsyn_falling(int cyclestart,int *cyclend,double one_less_trof,int *phase,dataptr dz)
{
	int exit_status;
	register int k = 0, j = cyclestart;
	int cyclelen = *cyclend - cyclestart;
	float *b = dz->flbufptr[0];
	if(*phase > 0) {
		k = *phase;
		(*cyclend) -= *phase;
		*phase = 0;
	}
	while(j<*cyclend) {
		b[j] = (float)do_falling(k,cyclelen - 1,one_less_trof,dz);
		k++;
		j++;
		if(j >= dz->buflen) {
			if((exit_status = write_samps(b,dz->buflen,dz))<0)
				return(exit_status);
			j = 0;
			*cyclend -= dz->buflen;
		}
	}
	return(FINISHED);
}

/******************* DO_FIRSTHALF_TROF **************************/

double do_firsthalf_trof(int k,int cyclehlf,double one_less_trof,dataptr dz)
{
	double z = 1.0 - ((double)k/(double)cyclehlf);
	z = (pow(z,dz->param[ENVSYN_EXPON]) * one_less_trof) + dz->param[ENVSYN_TROF];
	return (z);
}

/******************* DO_LASTHALF_TROF **************************/

double do_lasthalf_trof(int k,int cyclehlf,double one_less_trof,dataptr dz)
{
	double z = (double)k/(double)cyclehlf;
	z    = (pow(z,dz->param[ENVSYN_EXPON]) * one_less_trof) + dz->param[ENVSYN_TROF];
	return(z);
}

/************************ DO_RISING_TROFFED *******************************/

double do_rising(int k,int cyclelen,double one_less_trof,dataptr dz)
{
	double z = (double)k/(double)cyclelen;
	z  = pow(z,dz->param[ENVSYN_EXPON]);
	z *= one_less_trof;
	z += dz->param[ENVSYN_TROF];
	return(z);
}

/************************ DO_FALLING_TROFFED *******************************/

double do_falling(int k,int cyclelen,double one_less_trof,dataptr dz)
{
	double z;
	z = 1.0 - ((double)k/(double)cyclelen);
	z = (pow(z,dz->param[ENVSYN_EXPON]) * one_less_trof) + dz->param[ENVSYN_TROF];
	return z;
}

/************************ READ_FROM_USER_ENVELOPE *******************************/

int read_from_user_envelope(double *env_val,double table_index,int init,dataptr dz)
{
	double *p;
	double hi_env, lo_env, hi_index, lo_index;
	double env;
	if(init)
		dz->ptr[ENVSYN_ENV] = dz->parray[ENVSYN_ENV];
	p = dz->ptr[ENVSYN_ENV];
	while(table_index > *p) {
		if((p += 2) >= dz->ptr[ENVSYN_ENVEND]) {
			sprintf(errstr,"Problem reading user envelope data: read_from_user_envelope()\n");
			return(PROGRAM_ERROR);
		}
	}
	if(p != dz->parray[ENVSYN_ENV]) {
		hi_env   = *(p+1);
		hi_index = *p;
		lo_env   = *(p-1);
		lo_index = *(p-2);
		env      = (double)((table_index - lo_index)/(hi_index - lo_index));
		env     *= (double)(hi_env - lo_env);
		env     += (double)lo_env;
	} else
		env = *(p+1);
	dz->ptr[ENVSYN_ENV] = p;
	*env_val = env;
	return(FINISHED);
}
