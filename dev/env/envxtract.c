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

static void getenv_of_buffer
			(int samps_to_process,int envwindow_sampsize,double convertor,float **envptr,float *buffer);
static double  getmaxsamp(int startsamp, int cnt,float *buffer);

/******************************** EXTRACT_ENV_FROM_SNDFILE [GETENV] *****************************
 *
 * NB THIS ALWAYS GETS ENVELOPE FROM sampbuf[0] !!!
 */

int extract_env_from_sndfile(int *bufcnt,int *envcnt,float **env,float **envend,int fileno,dataptr dz)
{
	int n;
	double convertor = 1.0/F_ABSMAXSAMP;
	float *envptr;
	int envwindow_sampsize = dz->iparam[ENV_SAMP_WSIZE];
	float *buffer = dz->sampbuf[0];
	*bufcnt = buffers_in_sndfile(dz->buflen,fileno,dz);
	*envcnt = windows_in_sndfile(fileno,dz);
	if((*env=(float *)malloc((*envcnt+20) * sizeof(float)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for envelope array.\n");
		return(MEMORY_ERROR);
	}
	envptr = *env;
	for(n = 0; n < *bufcnt; n++)	{
		if((dz->ssampsread = fgetfbufEx(dz->sampbuf[0], dz->buflen,dz->ifd[fileno],0)) < 0) {
			sprintf(errstr,"Can't read samples from soundfile: extract_env_from_sndfile()\n");
			return(SYSTEM_ERROR);
		}
		if(sloom)
			display_virtual_time(dz->total_samps_read,dz);		
		getenv_of_buffer(dz->ssampsread,envwindow_sampsize,convertor,&envptr,buffer);
	}
	*envend = envptr;
	return(FINISHED);
}

/************************* GETENV_OF_BUFFER [READENV] *******************************/

void getenv_of_buffer(int samps_to_process,int envwindow_sampsize,double convertor,float **envptr,float *buffer)
{
	int  start_samp = 0;
	float *env = *envptr;
	while(samps_to_process >= envwindow_sampsize) {
		*env++       = (float) (getmaxsamp(start_samp,envwindow_sampsize,buffer) * convertor);
		start_samp  += envwindow_sampsize;
		samps_to_process -= envwindow_sampsize;
	}
	if(samps_to_process)	/* Handle any final short buffer */
		*env++ = (float)(getmaxsamp(start_samp,samps_to_process,buffer) * convertor);
	*envptr = env;
}

/****************************** BUFFERS_IN_SNDFILE [BUFFER_CNT] ******************************/

int buffers_in_sndfile(int buffer_size,int fileno,dataptr dz)
{
	int bufcnt;
	if(((bufcnt = dz->insams[fileno]/buffer_size)*buffer_size)!=dz->insams[0])
		bufcnt++;
	return(bufcnt);
}

/****************************** WINDOWS_IN_SNDFILE [GET_ENVSIZE] ******************************
 *
 * Find samp-length of soundfiles, number of whole buffers this corresponds
 * to (bufcnt) and number of whole sectors reamining (nsec).
 */

int windows_in_sndfile(int fileno,dataptr dz)
{
	int envsize, winsize = dz->iparam[ENV_SAMP_WSIZE];
	if(((envsize = dz->insams[fileno]/winsize) * winsize)!=dz->insams[fileno])
		envsize++;
	return(envsize);
}

/*************************** GETMAXSAMP ******************************
 *
 * REVERTED TO FINDING largest sample in window, rather than POWER.
 */

/*int*/double getmaxsamp(int startsamp, int sampcnt,float *buffer)
{
	int  i, endsamp = startsamp + sampcnt;
	double thisval, thismaxsamp = 0.0;
	for(i = startsamp; i<endsamp; i++) {
		if((thisval =  fabs(buffer[i]))>thismaxsamp)		   
			thismaxsamp = thisval;
	}
	return thismaxsamp;
}
