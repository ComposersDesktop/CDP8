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
#include <stdlib.h>
#include <sfsys.h>
#include <osbind.h>

static int		distorte_func(int cyclestart,int cyclend,int current_buf,dataptr dz);
static int		distorte_func_crosbuf(int cyclestart,int cyclend,int current_buf,dataptr dz);

static int  	distorte_rising( int cyclestart,int cyclend,int current_buf,dataptr dz);
static int  	distorte_falling(int cyclestart,int cyclend,int current_buf,dataptr dz);
static int  	distorte_troffed(int cyclestart,int cyclend,int current_buf,dataptr dz);
static int  	distorte_linrise(int cyclestart,int cyclend,int current_buf,dataptr dz);
static int  	distorte_linfall(int cyclestart,int cyclend,int current_buf,dataptr dz);
static int  	distorte_lintrof(int cyclestart,int cyclend,int current_buf,dataptr dz);
static int 		distorte_userdef(int cyclestart,int cyclend,int current_buf,dataptr dz);
static int  	distorte_rising_troffed( int cyclestart,int cyclend,int current_buf,dataptr dz);
static int  	distorte_falling_troffed(int cyclestart,int cyclend,int current_buf,dataptr dz);
static int  	distorte_linrise_troffed(int cyclestart,int cyclend,int current_buf,dataptr dz);
static int  	distorte_linfall_troffed(int cyclestart,int cyclend,int current_buf,dataptr dz);

static int 		distorte_rising_bufcross( int cyclestart,int cyclend,int current_buf,dataptr dz);
static int  	distorte_falling_bufcross(int cyclestart,int cyclend,int current_buf,dataptr dz);
static int  	distorte_troffed_bufcross(int cyclestart,int cyclend,int current_buf,dataptr dz);
static int  	distorte_linrise_bufcross(int cyclestart,int cyclend,int current_buf,dataptr dz);
static int  	distorte_linfall_bufcross(int cyclestart,int cyclend,int current_buf,dataptr dz);
static int  	distorte_lintrof_bufcross(int cyclestart,int cyclend,int current_buf,dataptr dz);
static int  	distorte_userdef_bufcross(int cyclestart,int cyclend,int current_buf,dataptr dz);
static int  	distorte_rising_troffed_bufcross( int cyclestart,int cyclend,int current_buf,dataptr dz);
static int  	distorte_falling_troffed_bufcross(int cyclestart,int cyclend,int current_buf,dataptr dz);
static int  	distorte_linrise_troffed_bufcross(int cyclestart,int cyclend,int current_buf,dataptr dz);
static int  	distorte_linfall_troffed_bufcross(int cyclestart,int cyclend,int current_buf,dataptr dz);

static double	do_firsthalf_trof(int k,int cyclehlf,dataptr dz);
static double	do_lasthalf_trof(int k,int cyclehlf,dataptr dz);
static double	do_rising(int k,int cyclelen,dataptr dz);
static double	do_falling(int k,int cyclelen,dataptr dz);
static double	do_lintrof_firsthalf(int k,int cyclehlf,dataptr dz) ;
static double	do_lintrof_lasthalf(int k,int cyclehlf,dataptr dz);
static double	do_rising_troffed(int k,int cyclelen,dataptr dz);
static double	do_falling_troffed(int k,int cyclelen,dataptr dz);
static double	do_linrise_troffed(int k,int cyclelen,dataptr dz);
static double	do_linfall_troffed(int k,int cyclelen,dataptr dz);

static int		read_from_user_envelope(double *env_val,double table_index,int init,dataptr dz);

/************************** DISTORT_ENV **************************/

int distort_env(int *current_buf,int initial_phase,int *current_pos,int *buffer_overrun,int *cnt,dataptr dz)
{
	int exit_status = CONTINUE;
	int cyclestart = *current_pos;
	float *b = dz->sampbuf[*current_buf];
	dz->param[ONE_LESS_TROF] = 1.0 - dz->param[DISTORTE_TROF];
	exit_status = get_full_cycle(b,buffer_overrun,current_buf,initial_phase,current_pos,DISTORTE_CYCLECNT,dz);
	if(*buffer_overrun)
		distorte_func_crosbuf(cyclestart,*current_pos,*current_buf,dz);
	else
		distorte_func(cyclestart,*current_pos,*current_buf,dz);	
	(*cnt)++;
	return(exit_status);
}

/***************************** GET_FULL_CYCLE *******************************/

int get_full_cycle(float *b,int *buffer_overrun,int *current_buf,int initial_phase,int *current_pos,int cyclecnt_param,dataptr dz)
{
	int exit_status;
	register int i = *current_pos;
	register int n;
	switch(initial_phase) {
	case(1):
		for(n=0;n<dz->iparam[cyclecnt_param];n++) {
			while(b[i]>=0.0) {
				if(++i >= dz->ssampsread) {
					if((exit_status = change_buf(current_buf,buffer_overrun,&b,dz))!=CONTINUE) {
						*current_pos = i;
						return(exit_status);
					}
					i = 0;
				}
			}
			while(b[i]<=0.0) {
				if(++i >= dz->ssampsread) {
					if((exit_status = change_buf(current_buf,buffer_overrun,&b,dz))!=CONTINUE) {
						*current_pos = i;
						return(exit_status);
					}

					i = 0;
				}
			}
		}
		break;
	case(-1):
		for(n=0;n<dz->iparam[DISTORTE_CYCLECNT];n++) {
			while(b[i]<=0.0) {
				if(++i >= dz->ssampsread) {
					if((exit_status = change_buf(current_buf,buffer_overrun,&b,dz))!=CONTINUE) {
						*current_pos = i;
						return(exit_status);
					}
					i = 0;
				}
			}
			while(b[i]>=0.0) {
				if(++i >= dz->ssampsread) {
					if((exit_status = change_buf(current_buf,buffer_overrun,&b,dz))!=CONTINUE)	 {
						*current_pos = i;
						return(exit_status);
					}
					i = 0;
				}
			}
		}
		break;
	}
	*current_pos = i;
	return(CONTINUE);
}

/***************************** CHANGE_BUF *******************************/

int change_buf(int *current_buf,int *buffer_overrun,float **buf,dataptr dz)
{
 	int exit_status;
	if(dz->samps_left<=0) 
		return(FINISHED);
	if(*buffer_overrun==TRUE) {
		sprintf(errstr,"Buffer overflow: cycle(set) too long at %lf secs.\n",
		(double)dz->total_samps_read/(double)dz->infile->srate);
		return(GOAL_FAILED);
	}
	*buffer_overrun = TRUE;
	*current_buf = !(*current_buf);
	*buf = dz->sampbuf[*current_buf];
	if((exit_status = read_samps(*buf,dz))<0)
		return(exit_status);		
	return(CONTINUE);
}

/***************************** DISTORTE_FUNC **************************/

int  distorte_func(int cyclestart,int cyclend,int current_buf,dataptr dz)
{   
	switch(dz->mode) {
	case(DISTORTE_RISING):   		return distorte_rising(cyclestart,cyclend,current_buf,dz);
	case(DISTORTE_FALLING):  		return distorte_falling(cyclestart,cyclend,current_buf,dz);
	case(DISTORTE_TROFFED):  		return distorte_troffed(cyclestart,cyclend,current_buf,dz);
	case(DISTORTE_LINRISE):  		return distorte_linrise(cyclestart,cyclend,current_buf,dz);
	case(DISTORTE_LINFALL):  		return distorte_linfall(cyclestart,cyclend,current_buf,dz);
	case(DISTORTE_LINTROF):  		return distorte_lintrof(cyclestart,cyclend,current_buf,dz);
	case(DISTORTE_USERDEF):  		return distorte_userdef(cyclestart,cyclend,current_buf,dz);
	case(DISTORTE_RISING_TR):   	return distorte_rising_troffed(cyclestart,cyclend,current_buf,dz);
	case(DISTORTE_FALLING_TR):  	return distorte_falling_troffed(cyclestart,cyclend,current_buf,dz);
	case(DISTORTE_LINRISE_TR):  	return distorte_linrise_troffed(cyclestart,cyclend,current_buf,dz);
	case(DISTORTE_LINFALL_TR):  	return distorte_linfall_troffed(cyclestart,cyclend,current_buf,dz);
	default:
		sprintf(errstr,"Unknown case: distorte_func()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);	/* NOTREACHED */
}

/**************************** DISTORTE_FUNC_CROSBUF ************************/

int distorte_func_crosbuf(int cyclestart,int cyclend,int current_buf,dataptr dz)
{
	switch(dz->mode) {
	case(DISTORTE_RISING):  return distorte_rising_bufcross( cyclestart,cyclend,current_buf,dz);
	case(DISTORTE_FALLING): return distorte_falling_bufcross(cyclestart,cyclend,current_buf,dz);
	case(DISTORTE_TROFFED): return distorte_troffed_bufcross(cyclestart,cyclend,current_buf,dz);
	case(DISTORTE_LINRISE): return distorte_linrise_bufcross(cyclestart,cyclend,current_buf,dz);
	case(DISTORTE_LINFALL): return distorte_linfall_bufcross(cyclestart,cyclend,current_buf,dz);
	case(DISTORTE_LINTROF): return distorte_lintrof_bufcross(cyclestart,cyclend,current_buf,dz);
	case(DISTORTE_USERDEF): return distorte_userdef_bufcross(cyclestart,cyclend,current_buf,dz);
	case(DISTORTE_RISING_TR):  return distorte_rising_troffed_bufcross( cyclestart,cyclend,current_buf,dz);
	case(DISTORTE_FALLING_TR): return distorte_falling_troffed_bufcross(cyclestart,cyclend,current_buf,dz);
	case(DISTORTE_LINRISE_TR): return distorte_linrise_troffed_bufcross(cyclestart,cyclend,current_buf,dz);
	case(DISTORTE_LINFALL_TR): return distorte_linfall_troffed_bufcross(cyclestart,cyclend,current_buf,dz);
	default:
		sprintf(errstr,"Unknown case: distorte_func_crosbuf()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);	/* NOTREACHED */
}

/************************** DISTORTE_TROFFED *******************************/

int distorte_troffed(int cyclestart,int cyclend,int current_buf,dataptr dz)
{
	register int k = 0, j = cyclestart;
	int cyclelen = cyclend - cyclestart;
	int cyclehlf = cyclelen/2;
	int cyclemid = cyclestart + cyclehlf;
	float *b = dz->sampbuf[current_buf];
	while(j<cyclemid) {
		b[j] = (float)/*ound*/(b[j] * do_firsthalf_trof(k,cyclehlf,dz));
		j++;
		k++;
	}
	k = 0;
	cyclehlf = cyclelen - cyclehlf;
	while(j < cyclend) {
		b[j] = (float)/*round*/(b[j] * do_lasthalf_trof(k,cyclehlf,dz));
		j++;
		k++;
	}
	return(FINISHED);
}

/*************************** DISTORTE_RISING ******************************/

int distorte_rising(int cyclestart,int cyclend,int current_buf,dataptr dz)
{
	register int k = 0, j = cyclestart;
	int cyclelen = cyclend - cyclestart;
	float *b = dz->sampbuf[current_buf];
	while(j<cyclend) {
		b[j] = (float)/*round*/((double)b[j] * do_rising(k,cyclelen,dz));
		j++;
		k++;
	}
	return(FINISHED);
}

/************************** DISTORTE_FALLING ******************************/

int distorte_falling(int cyclestart,int cyclend,int current_buf,dataptr dz)
{
	register int k = 0, j = cyclestart;
	int cyclelen = cyclend - cyclestart;
	float *b = dz->sampbuf[current_buf];
	while(j<cyclend) {
		b[j] = (float)/*round*/((double)b[j] * do_falling(k,cyclelen,dz));
		j++;
		k++;
	}
	return(FINISHED);
}

/************************* DISTORTE_LINRISE **************************/

int distorte_linrise(int cyclestart,int cyclend,int current_buf,dataptr dz)
{
	register int k = 0, j = cyclestart;
	double z;
	int cyclelen = cyclend - cyclestart;
	float *b = dz->sampbuf[current_buf];
	while(j<cyclend) {
		z    = (double)k/(double)cyclelen;
		b[j] = (float)/*round*/((double)b[j] * z);
		j++;
		k++;
	}
	return(FINISHED);
}

/***************************** DISTORTE_LINFALL ***************************/

int distorte_linfall(int cyclestart,int cyclend,int current_buf,dataptr dz)
{
	register int k = 0, j = cyclestart;
	double z;
	int cyclelen = cyclend - cyclestart;
	float *b = dz->sampbuf[current_buf];
	while(j<cyclend) {
		z    = 1.0 - ((double)k/(double)cyclelen);
		b[j] = (float)/*round*/((double)b[j] * z);
		j++;
		k++;
	}
	return(FINISHED);
}

/****************************** DISTORTE_LINTROF ****************************/

int distorte_lintrof(int cyclestart,int cyclend,int current_buf,dataptr dz)
{
	register int k = 0, j = cyclestart;
	int cyclelen = cyclend - cyclestart;
	int cyclehlf = cyclelen/2;
	int cyclemid = cyclestart + cyclehlf;
	float *b = dz->sampbuf[current_buf];
	while(j<cyclemid) {
		b[j] = (float)/*round*/((double)b[j] * do_lintrof_firsthalf(k,cyclehlf,dz));
		j++;
		k++;
	}
	k = 0;
	cyclehlf = cyclelen - cyclehlf;
	while(j < cyclend) {
		b[j] = (float)/*round*/((double)b[j] * do_lintrof_lasthalf(k,cyclehlf,dz));
		j++;
		k++;
	}
	return(FINISHED);
}

/*************************** DISTORTE_USERDEF ***************************/

int distorte_userdef(int cyclestart,int cyclend,int current_buf,dataptr dz)
{
	int exit_status;
	register int k = 0, j = cyclestart;
	int cyclelen = cyclend - cyclestart;
	double index;
	double z;
	int init = 1;
	float *b = dz->sampbuf[current_buf];
	while(j<cyclend) {
		index = (double)k/(double)cyclelen;
		if((exit_status = read_from_user_envelope(&z,index,init,dz))<0)
			return(exit_status);
		b[j]  = (float)/*round*/((double)b[j] * z);
		j++;
		k++;
		init = 0;
	}
	return(FINISHED);
}

/*************************** DISTORTE_RISING_TROFFED ***********************/

int distorte_rising_troffed(int cyclestart,int cyclend,int current_buf,dataptr dz)
{
	register int k = 0, j = cyclestart;
	int cyclelen = cyclend - cyclestart;
	float *b = dz->sampbuf[current_buf];
	while(j<cyclend) {
		b[j] = (float) /*round*/((double)b[j] * do_rising_troffed(k,cyclelen,dz));
		j++;
		k++;								 
	}
	return(FINISHED);
}

/********************** DISTORTE_FALLING_TROFFED *************************/

int distorte_falling_troffed(int cyclestart,int cyclend,int current_buf,dataptr dz)
{
	register int k = 0, j = cyclestart;
	int cyclelen = cyclend - cyclestart;
	float *b = dz->sampbuf[current_buf];
	while(j<cyclend) {
		b[j] = (float)/*round*/((double)b[j] * do_falling_troffed(k,cyclelen,dz));
		j++;
		k++;
	}
	return(FINISHED);
}

/************************* DISTORTE_LINRISE_TROFFED **************************/

int distorte_linrise_troffed(int cyclestart,int cyclend,int current_buf,dataptr dz)
{
	register int k = 0, j = cyclestart;
	int cyclelen = cyclend - cyclestart;
	float *b = dz->sampbuf[current_buf];
	while(j<cyclend) {
		b[j] = (float)/*round*/((double)b[j] * do_linrise_troffed(k,cyclelen,dz));
		j++;
		k++;
	}
	return(FINISHED);
}

/*************************** DISTORTE_LINFALL_TROFFED ***************************/

int distorte_linfall_troffed(int cyclestart,int cyclend,int current_buf,dataptr dz)
{
	register int k = 0, j = cyclestart;
	int cyclelen = cyclend - cyclestart;
	float *b = dz->sampbuf[current_buf];
	while(j<cyclend) {
		b[j] = (float)/*round*/((double)b[j] * do_linfall_troffed(k,cyclelen,dz));
		j++;
		k++;
	}
	return(FINISHED);
}

/********************** DISTORTE_TROFFED_BUFCROSS ***************************/

int distorte_troffed_bufcross(int cyclestart,int cyclend,int current_buf,dataptr dz)
{
	int exit_status;
	register int k = 0, j = cyclestart;
	int   overspill;
	int cyclelen = (cyclend + dz->buflen)- cyclestart;
	int  cyclehlf = cyclelen/2;
	int cyclemid = cyclestart + cyclehlf;
	float *b = dz->sampbuf[!current_buf];
	if((overspill = cyclemid - dz->buflen) < 0) { /* CYCLEMID<BUFLEN */
		while(j<cyclemid) {
			b[j] = (float)/*round*/(b[j] * do_firsthalf_trof(k,cyclehlf,dz));
			j++; k++;
		}
		k = 0;
		cyclehlf = cyclelen - cyclehlf;
		while(j < dz->buflen) {
			b[j] = (float)/*round*/(b[j] * do_lasthalf_trof(k,cyclehlf,dz));
			j++; k++;
		}
		if((exit_status = write_samps(dz->sampbuf[!current_buf],dz->buflen,dz))<0)
			return(exit_status);
		b = dz->sampbuf[current_buf];
		j = 0;
	} else {		/* CYCLEMID >= BUFLEN */
		while(j<dz->buflen) {
			b[j] = (float)/*round*/(b[j] * do_firsthalf_trof(k,cyclehlf,dz));
			j++; k++;
		}
		if((exit_status = write_samps(dz->sampbuf[!current_buf],dz->buflen,dz))<0)
			return(exit_status);
		b = dz->sampbuf[current_buf];
		j = 0;
		while(j<overspill) {
			b[j] = (float)/*round*/(b[j] * do_firsthalf_trof(k,cyclehlf,dz));
			j++; k++;
		}
		k = 0;
		cyclehlf = cyclelen - cyclehlf;
	}
	while(j < cyclend) {
		b[j] = (float)/*round*/(b[j] * do_lasthalf_trof(k,cyclehlf,dz));
		j++;
		k++;
	}
	return(FINISHED);
}

/********************** DISTORTE_RISING_BUFCROSS **************************/

int distorte_rising_bufcross(int cyclestart,int cyclend,int current_buf,dataptr dz)
{
	int exit_status;
	register int k = 0, j = cyclestart;
	int cyclelen = (cyclend + dz->buflen)- cyclestart;
	float *b = dz->sampbuf[!current_buf];
	while(j<dz->buflen) {
		b[j] = (float)/*round*/((double)b[j] * do_rising(k,cyclelen,dz));
		j++;
		k++;
	}
	if((exit_status = write_samps(dz->sampbuf[!current_buf],dz->buflen,dz))<0)
		return(exit_status);
	b = dz->sampbuf[current_buf];
	j = 0;
	while(j<cyclend) {
		b[j] = (float)/*round*/((double)b[j] * do_rising(k,cyclelen,dz));
		j++;
		k++;
	}
	return(FINISHED);
}

/******************** DISTORTE_FALLING_BUFCROSS *************************/

int distorte_falling_bufcross(int cyclestart,int cyclend,int current_buf,dataptr dz)
{
	int exit_status;
	register int k = 0, j = cyclestart;
	int cyclelen = (cyclend + dz->buflen)- cyclestart;
	float *b = dz->sampbuf[!current_buf];
	while(j<dz->buflen) {
		b[j] = (float)/*round*/((double)b[j] * do_falling(k,cyclelen,dz));
		j++;
		k++;
	}
	if((exit_status = write_samps(dz->sampbuf[!current_buf],dz->buflen,dz))<0)
		return(exit_status);
	b = dz->sampbuf[current_buf];
	j = 0;
	while(j<cyclend) {
		b[j] = (float)/*round*/((double)b[j] * do_falling(k,cyclelen,dz));
		j++;
		k++;
	}
	return(FINISHED);
}

/********************* DISTORTE_LINRISE_BUFCROSS *************************/

int distorte_linrise_bufcross(int cyclestart,int cyclend,int current_buf,dataptr dz)
{
	int exit_status;
	register int k = 0, j = cyclestart;
	double z;
	int cyclelen = (cyclend + dz->buflen)- cyclestart;
	float *b = dz->sampbuf[!current_buf];
	while(j<dz->buflen) {
		z    = (double)k/(double)cyclelen;
		b[j] = (float)/*round*/((double)b[j] * z);
		j++;
		k++;
	}
	if((exit_status = write_samps(dz->sampbuf[!current_buf],dz->buflen,dz))<0)
		return(exit_status);
	b = dz->sampbuf[current_buf];
	j = 0;
	while(j<cyclend) {
		z    = (double)k/(double)cyclelen;
		b[j] = (float)/*round*/((double)b[j] * z);
		j++;
		k++;
	}
	return(FINISHED);
}

/********************* DISTORTE_LINFALL_BUFCROSS **************************/

int distorte_linfall_bufcross(int cyclestart,int cyclend,int current_buf,dataptr dz)
{
	int exit_status;
	register int k = 0, j = cyclestart;
	double z;
	int cyclelen = (cyclend + dz->buflen)- cyclestart;
	float *b = dz->sampbuf[!current_buf];
	while(j<dz->buflen) {
		z    = 1.0 - ((double)k/(double)cyclelen);
		b[j] = (float)/*round*/((double)b[j] * z);
		j++;
		k++;
	}
	if((exit_status = write_samps(dz->sampbuf[!current_buf],dz->buflen,dz))<0)
		return(exit_status);
	b = dz->sampbuf[current_buf];
	j = 0;
	while(j<cyclend) {
		z    = 1.0 - ((double)k/(double)cyclelen);
		b[j] = (float)/*round*/((double)b[j] * z);
		j++;
		k++;
	}
	return(FINISHED);
}

/*********************** DISTORTE_LINTROF_BUFCROSS ****************************/

int distorte_lintrof_bufcross(int cyclestart,int cyclend,int current_buf,dataptr dz)
{
	int exit_status;
	register int k = 0, j = cyclestart;
	int   overspill;
	int cyclelen = (cyclend + dz->buflen)- cyclestart;
	int cyclehlf = cyclelen/2;
	int cyclemid = cyclestart + cyclehlf;
	float *b = dz->sampbuf[!current_buf];
	if((overspill = cyclemid - dz->buflen) < 0) { /* CYCLEMID<BUFLEN */
		while(j<cyclemid) {
			b[j] = (float)/*round*/((double)b[j] * do_lintrof_firsthalf(k,cyclehlf,dz));
			j++;
			k++;
		}
		k = 0;
		cyclehlf = cyclelen - cyclehlf;
		while(j < dz->buflen) {
			b[j] = (float)/*round*/((double)b[j] * do_lintrof_lasthalf(k,cyclehlf,dz));
			j++;
			k++;
		}
		if((exit_status = write_samps(dz->sampbuf[!current_buf],dz->buflen,dz))<0)
			return(exit_status);
		b = dz->sampbuf[current_buf];
		j = 0;
	} else {		/* CYCLEMID >= BUFLEN */
		while(j<dz->buflen) {
			b[j] = (float)/*round*/((double)b[j] * do_lintrof_firsthalf(k,cyclehlf,dz));
			j++;
			k++;
		}
		if((exit_status = write_samps(dz->sampbuf[!current_buf],dz->buflen,dz))<0)
			return(exit_status);
		b = dz->sampbuf[current_buf];
		j = 0;
		while(j<overspill) {
			b[j] = (float)/*round*/((double)b[j] * do_lintrof_firsthalf(k,cyclehlf,dz));
			j++;
			k++;
		}
		k = 0;
		cyclehlf = cyclelen - cyclehlf;
	}
	while(j < cyclend) {
		b[j] = (float)/*round*/((double)b[j] * do_lintrof_lasthalf(k,cyclehlf,dz));
		j++;
		k++;
	}
	return(FINISHED);
}

/********************** DISTORTE_USERDEF_BUFCROSS **************************/

int distorte_userdef_bufcross(int cyclestart,int cyclend,int current_buf,dataptr dz)
{
	int exit_status;
	register int k = 1, j = cyclestart;
	int cyclelen = (cyclend + dz->buflen)- cyclestart;
	double index = 0.0;
	double z;
	int init = 1;
	float *b = dz->sampbuf[!current_buf];
	while(j<dz->buflen) {
		index = (double)k/(double)cyclelen;
		if((exit_status = read_from_user_envelope(&z,index,init,dz))<0)
			return(exit_status);
		b[j]  = (float)/*round*/((double)b[j] * z);
		j++;
		k++;
		init = 0;
	}
	if((exit_status = write_samps(dz->sampbuf[!current_buf],dz->buflen,dz))<0)
		return(exit_status);
	b = dz->sampbuf[current_buf];
	j = 0;
	while(j<cyclend) {
		index = (double)k/(double)cyclelen;
		if((exit_status = read_from_user_envelope(&z,index,init,dz))<0)
			return(exit_status);
		b[j]  = (float)/*round*/((double)b[j] * z);
		j++;
		k++;
	}
	return(FINISHED);
}

/********************** DISTORTE_RISING_TROFFED_BUFCROSS **************************/

int distorte_rising_troffed_bufcross(int cyclestart,int cyclend,int current_buf,dataptr dz)
{
	int exit_status;
	register int k = 0, j = cyclestart;
	int cyclelen = (cyclend + dz->buflen)- cyclestart;
	float *b = dz->sampbuf[!current_buf];
	while(j<dz->buflen) {
		b[j] = (float)/*round*/((double)b[j] * do_rising_troffed(k,cyclelen,dz));
		j++;
		k++;								 
	}
	if((exit_status = write_samps(dz->sampbuf[!current_buf],dz->buflen,dz))<0)
		return(exit_status);
	b = dz->sampbuf[current_buf];
	j = 0;
	while(j<cyclend) {
		b[j] = (float)/*round*/((double)b[j] * do_rising_troffed(k,cyclelen,dz));
		j++;
		k++;								 
	}
	return(FINISHED);
}

/******************** DISTORTE_FALLING_TROFFED_BUFCROSS *************************/

int distorte_falling_troffed_bufcross(int cyclestart,int cyclend,int current_buf,dataptr dz)
{
	int exit_status;
	register int k = 0, j = cyclestart;
	int cyclelen = (cyclend + dz->buflen)- cyclestart;
	float *b = dz->sampbuf[!current_buf];
	while(j<dz->buflen) {
		b[j] = (float)/*round*/((double)b[j] * do_falling_troffed(k,cyclelen,dz));
		j++;
		k++;
	}
	if((exit_status = write_samps(dz->sampbuf[!current_buf],dz->buflen,dz))<0)
		return(exit_status);
	b = dz->sampbuf[current_buf];
	j = 0;
	while(j<cyclend) {
		b[j] = (float)/*round*/((double)b[j] * do_falling_troffed(k,cyclelen,dz));
		j++;
		k++;
	}
	return(FINISHED);
}

/********************* DISTORTE_LINRISE_TROFFED_BUFCROSS *************************/

int distorte_linrise_troffed_bufcross(int cyclestart,int cyclend,int current_buf,dataptr dz)
{
	int exit_status;
	register int k = 0, j = cyclestart;
	int cyclelen = (cyclend + dz->buflen)- cyclestart;
	float *b = dz->sampbuf[!current_buf];
	while(j<dz->buflen) {
		b[j] = (float)/*round*/((double)b[j] * do_linrise_troffed(k,cyclelen,dz));
		j++;
		k++;
	}
	if((exit_status = write_samps(dz->sampbuf[!current_buf],dz->buflen,dz))<0)
		return(exit_status);
	b = dz->sampbuf[current_buf];
	j = 0;
	while(j<cyclend) {
		b[j] = (float)/*round*/((double)b[j] * do_linrise_troffed(k,cyclelen,dz));
		j++;
		k++;
	}
	return(FINISHED);
}

/******************* DISTORTE_LINFALL_TROFFED_BUFCROSS **************************/

int distorte_linfall_troffed_bufcross(int cyclestart,int cyclend,int current_buf,dataptr dz)
{
	int exit_status;
	register int k = 0, j = cyclestart;
	int cyclelen = (cyclend + dz->buflen)- cyclestart;
	float *b = dz->sampbuf[!current_buf];
	while(j<dz->buflen) {
		b[j] = (float)/*round*/((double)b[j] * do_linfall_troffed(k,cyclelen,dz));
		j++;
		k++;
	}
	if((exit_status = write_samps(dz->sampbuf[!current_buf],dz->buflen,dz))<0)
		return(exit_status);
	b = dz->sampbuf[current_buf];
	j = 0;
	while(j<cyclend) {
		b[j] = (float)/*round*/((double)b[j] * do_linfall_troffed(k,cyclelen,dz));
		j++;
		k++;
	}
	return(FINISHED);
}

/******************* DO_FIRSTHALF_TROF **************************/

double do_firsthalf_trof(int k,int cyclehlf,dataptr dz)
{
	double z = 1.0 - ((double)k/(double)cyclehlf);
	z = (pow(z,dz->param[DISTORTE_EXPON]) * dz->param[ONE_LESS_TROF]) + dz->param[DISTORTE_TROF];
	return (z);
}

/******************* DO_LASTHALF_TROF **************************/

double do_lasthalf_trof(int k,int cyclehlf,dataptr dz)
{
	double z = (double)k/(double)cyclehlf;
	z    = (pow(z,dz->param[DISTORTE_EXPON]) * dz->param[ONE_LESS_TROF]) + dz->param[DISTORTE_TROF];
	return(z);
}

/******************* DO_RISING **************************/

double do_rising(int k,int cyclelen,dataptr dz)
{
	double z = (double)k/(double)cyclelen;
	z = pow(z,dz->param[DISTORTE_EXPON]);
	return z;
}

/******************* DO_FALLING **************************/

double do_falling(int k,int cyclelen,dataptr dz)
{
	double z = 1.0 - ((double)k/(double)cyclelen);
	z    = pow(z,dz->param[DISTORTE_EXPON]);
	return z;
}

/************************ DO_LINTROF_FIRSTHALF *******************************/

double do_lintrof_firsthalf(int k,int cyclehlf,dataptr dz) 
{
	double z = 1.0 - ((double)k/(double)cyclehlf);
	z = (z * dz->param[ONE_LESS_TROF]) + dz->param[DISTORTE_TROF];
	return z;
}

/************************ DO_LINTROF_LASTHALF *******************************/

double do_lintrof_lasthalf(int k,int cyclehlf,dataptr dz)
{
	double z = (double)k/(double)cyclehlf;
	z = (z * dz->param[ONE_LESS_TROF]) + dz->param[DISTORTE_TROF];
	return z;
}

/************************ DO_RISING_TROFFED *******************************/

double do_rising_troffed(int k,int cyclelen,dataptr dz)
{
	double z = (double)k/(double)cyclelen;
	z  = pow(z,dz->param[DISTORTE_EXPON]);
	z *= dz->param[ONE_LESS_TROF];
	z += dz->param[DISTORTE_TROF];
	return(z);
}

/************************ DO_FALLING_TROFFED *******************************/

double do_falling_troffed(int k,int cyclelen,dataptr dz)
{
	double z = 1.0 - ((double)k/(double)cyclelen);
	z = (pow(z,dz->param[DISTORTE_EXPON]) * dz->param[ONE_LESS_TROF]) + dz->param[DISTORTE_TROF];
	return z;
}

/************************ DO_LINRISE_TROFFED *******************************/

double do_linrise_troffed(int k,int cyclelen,dataptr dz)
{
	double z = (double)k/(double)cyclelen;
	z = (z * dz->param[ONE_LESS_TROF]) + dz->param[DISTORTE_TROF];
	return z;
}

/************************ DO_LINFALL_TROFFED *******************************/

double do_linfall_troffed(int k,int cyclelen,dataptr dz)
{
	double z = 1.0 - ((double)k/(double)cyclelen);
	z = (z * dz->param[ONE_LESS_TROF]) + dz->param[DISTORTE_TROF];
	return z;
}

/************************ READ_FROM_USER_ENVELOPE *******************************/

int read_from_user_envelope(double *env_val,double table_index,int init,dataptr dz)
{
	double *p;
	double hi_env, lo_env, hi_index, lo_index;
	double env;
	if(init)
		dz->ptr[DISTORTE_ENV] = dz->parray[DISTORTE_ENV];
	p = dz->ptr[DISTORTE_ENV];
	while(table_index > *p) {
		if((p += 2) >= dz->ptr[DISTORTE_ENVEND]) {
			sprintf(errstr,"Problem reading user envelope data: read_from_user_envelope()\n");
			return(PROGRAM_ERROR);
		}
	}
	if(p != dz->parray[DISTORTE_ENV]) {
		hi_env   = *(p+1);
		hi_index = *p;
		lo_env   = *(p-1);
		lo_index = *(p-2);
		env      = (double)((table_index - lo_index)/(hi_index - lo_index));
		env     *= (double)(hi_env - lo_env);
		env     += (double)lo_env;
	} else
		env = *(p+1);
	dz->ptr[DISTORTE_ENV] = p;
	*env_val = env;
	return(FINISHED);
}		
