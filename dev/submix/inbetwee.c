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
#include <filetype.h>
#include <modeno.h>
#include <logic.h>
#include <arrays.h>
#include <mix.h>
#include <cdpmain.h>

#include <sfsys.h>

#ifdef unix
#define round(x) lround((x))
#endif

static int  do_inbetweening(int n,dataptr dz);
static int	cyclesync_inbetweening(int n,dataptr dz);
static int  do_between_mix(int insamps1,int insamps2,int n,float *buf1,float *buf2,float *obuf,dataptr dz);
static int  make_betweenfile_name(int n,char *generic_name,char **filename);
static int  make_tk_betweenfile_name(int n,char *generic_name,char **filename);

/***************************** MIX_INBETWEEN ****************************/

int mix_inbetween(dataptr dz)
{
	int exit_status;
	int n;
	char *filename;

	for(n=0;n<dz->iparam[INBETW];n++) {
		if(!sloom) {
			if((exit_status = make_betweenfile_name(n,dz->wordstor[dz->extra_word],&filename))<0)
				return(exit_status);
		} else {
			if((exit_status = make_tk_betweenfile_name(n,dz->wordstor[dz->extra_word],&filename))<0)
				return(exit_status);
			fprintf(stdout,"INFO: making inbetween sound '%s'\n",filename);
			fflush(stdout);
			dz->total_samps_read = 0L;
		}
		dz->process_type = UNEQUAL_SNDFILE;	/* allow sndfile to be created: suitable for truncation */
		if((exit_status = create_sized_outfile(filename,dz))<0) {
			sprintf(errstr,"Failed to open inbetween file %s\n",filename);
			free(filename);
			return(exit_status);
		}							
		switch(dz->process) {
		case(MIXINBETWEEN):
			exit_status = do_inbetweening(n,dz);
			break;
		case(CYCINBETWEEN):
			exit_status = cyclesync_inbetweening(n,dz);
			break;
		}
		if(exit_status < 0) {
			free(filename);
			return(exit_status);
		}
#ifdef NOTDEF
		if((exit_status = truncate_outfile(dz))<0) {
			free(filename);
			return(exit_status);
		}
#endif
		dz->process_type = EQUAL_SNDFILE;	/* prevents further attempts to truncate on quitting here */
		dz->outfiletype  = SNDFILE_OUT;		/* allows header to be written  */
		if((exit_status = headwrite(dz->ofd,dz))<0) {
			free(filename);
			return(exit_status);
		}
		dz->outfiletype = NO_OUTPUTFILE;
		if((exit_status = reset_peak_finder(dz))<0)
			return(exit_status);
		if(sndcloseEx(dz->ofd) < 0) {
			sprintf(errstr, "WARNING: Can't close output soundfile %s\n",filename);
			free(filename);
			return(SYSTEM_ERROR);
		}
		free(filename);
		dz->ofd = -1;
		dz->process_type = OTHER_PROCESS;	/* prevents program from trying to operate further on an outfile */
	}
	return(FINISHED);
}

/****************************** DO_INBETWEENING **************************/

int do_inbetweening(int n,dataptr dz)
{
	int  exit_status;
	float *buf1 = dz->sampbuf[0];
	float *buf2 = dz->sampbuf[1];
	float *obuf = dz->sampbuf[2];
	int  samps_left1, samps_left2, gross_samps_left;
	int  samps_read1 = 0, samps_read2 = 0, gross_samps_read, last_total_samps_written;

	dz->total_samps_written = 0;

	if(sndseekEx(dz->ifd[0],0L,0)<0	|| sndseekEx(dz->ifd[1],0L,0)<0) {
		sprintf(errstr,"ERROR: Seek failure in input files: do_inbetweening()\n");
		return(SYSTEM_ERROR);
	}
	samps_left1 = dz->insams[0];
	samps_left2 = dz->insams[1];
	gross_samps_left = max(samps_left1,samps_left2);

	while(gross_samps_left > 0) {
		memset((char *)obuf,0,(size_t)(dz->buflen * sizeof(float)));
//TW UDPATES
		memset((char *)buf1,0,(size_t)(dz->buflen * sizeof(float)));
		memset((char *)buf2,0,(size_t)(dz->buflen * sizeof(float)));

		if(samps_left1>0) {		
			if((samps_read1 = fgetfbufEx(buf1, dz->buflen,dz->ifd[0],0))<0) {
				sprintf(errstr,"Failed to read data: Infile 1: do_inbetweening()\n");
				return(SYSTEM_ERROR);
			}
			samps_left1 -= samps_read1;
		}
		if(samps_left2>0) {		
			if((samps_read2 = fgetfbufEx(buf2, dz->buflen,dz->ifd[1],0))<0) {
				sprintf(errstr,"Failed to read data: Infile 2: do_inbetweening()\n");
				return(SYSTEM_ERROR);
			}
			samps_left2 -= samps_read2;
		}
		gross_samps_read = max(samps_read1,samps_read2);
		gross_samps_left = max(samps_left1,samps_left2);

		if((exit_status = do_between_mix(samps_read1,samps_read2,n,buf1,buf2,obuf,dz))<0)
			return(exit_status);

		last_total_samps_written = dz->total_samps_written;
		if(gross_samps_read > 0) {
			if((exit_status = write_samps(obuf, gross_samps_read,dz))<0)
				return(exit_status);
		}
		if(dz->total_samps_written - last_total_samps_written != gross_samps_read) {
			sprintf(errstr,"Anomaly writing data to file: do_inbetweening()\n");
			return(PROGRAM_ERROR);
		}
	}
	return(FINISHED);
}

/***************************** DO_BETWEEN_MIX ***************************/

int do_between_mix(int insamps1,int insamps2,int n,float *buf1,float *buf2,float *obuf,dataptr dz)
{
	register int i;
	int minspan = min(insamps1,insamps2);
	int maxspan = max(insamps1,insamps2);	
	double ratio = dz->brk[INBETW][n], invratio = 1.0 - ratio;
	int  overflow_samps;
	if     (insamps1 > insamps2)	overflow_samps = 1;
	else if(insamps2 > insamps1)	overflow_samps = 2;
	else							overflow_samps = 0;
	for(i = 0; i < minspan; i++)
		obuf[i] = (float)((buf1[i] * invratio) + (buf2[i] * ratio));
	switch(overflow_samps) {
	case(1):	
		while(i < maxspan) {
			obuf[i] = (float)(buf1[i] * invratio);
			i++;
		}
		break;
	case(2):	
		while(i < maxspan) {
			obuf[i] = (float)(buf2[i] * ratio);	
			i++;
		}
		break;
	}
	return(FINISHED);
}

/**************************** MAKE_BETWEENFILE_NAME *******************************/

int make_betweenfile_name(int n,char *generic_name,char **filename)
{
	int hundreds, tens, units;
//TW REVISED Dec 2002
//	char *p;
	if(n >= MAXBETWEEN) {
		sprintf(errstr,"Counting error: make_betweenfile_name()\n");
		return(PROGRAM_ERROR);
	}
	n++;
	hundreds = n/100;
	n       -= hundreds * 100;
	tens     =  n/10;
	n       -= tens * 10;
	units    =  n;
//TW REVISED Dec 2002
//	sprintf(errstr,"%s",generic_name);
//	p = errstr + strlen(errstr);
//	*p++ = hundreds + INT_TO_ASCII;
//	*p++ = tens     + INT_TO_ASCII;
//	*p++ = units    + INT_TO_ASCII;
//	*p = ENDOFSTR;
	
	strcpy(errstr,generic_name);
	insert_new_number_at_filename_end(errstr,hundreds,0);
	insert_new_number_at_filename_end(errstr,tens,0);
	insert_new_number_at_filename_end(errstr,units,0);
	
	if((*filename = (char *)malloc((strlen(errstr) + 6) * sizeof(char)))==NULL) {
		sprintf(errstr,	"INSUFFICIENT MEMORY for filename.\n");
		return(MEMORY_ERROR);
	}
	strcpy(*filename,errstr);
	return(FINISHED);
}

/**************************** MAKE_TK_BETWEENFILE_NAME *******************************/

int make_tk_betweenfile_name(int n,char *generic_name,char **filename)
{
//TW REVISED Dec 2002
//	char *p;
	if(n >= MAXBETWEEN) {
		sprintf(errstr,"Counting error: make_tk_betweenfile_name()\n");
		return(PROGRAM_ERROR);
	}
	strcpy(errstr,generic_name);
//TW REVISED Dec 2002
	insert_new_number_at_filename_end(errstr,n,1);
//	p = errstr + strlen(errstr);
//	p--;
//	sprintf(p,"%d",n);
	if((*filename = (char *)malloc((strlen(errstr) + 6) * sizeof(char)))==NULL) {
		sprintf(errstr,	"INSUFFICIENT MEMORY for filename.\n");
		return(MEMORY_ERROR);
	}
	strcpy(*filename,errstr);
	return(FINISHED);
}

/**************************** CYCLESYNC_INBETWEENING *******************************/

int cyclesync_inbetweening(int m,dataptr dz)
{
	int exit_status, isneg;
	int n0, n1, n, cyclecnt;
	int outcnt = dz->iparam[INBETW] + 1;
	float *ibuf0 = dz->sampbuf[0], *ibuf1 = dz->sampbuf[1], *obuf = dz->sampbuf[2];
	int *cycs0, *cycs1, *lens0, *lens1;
	int thislen, j, k;
	double lendiff, ratio, invratio, incr0, incr1, indx0, indx1;
	double endval, startval, diff, frac, val0, val1;
	int aa, bb;

	cycs0 = dz->lparray[0];
	cycs1 = dz->lparray[1];
	lens0 = dz->lparray[2];
	lens1 = dz->lparray[3];
	cyclecnt = dz->itemcnt - 1;		/* there is one less length then there are zero-crossings */
	j = 0;
	m++;
	for(n = 0;n < cyclecnt; n++) {
		n0 = cycs0[n];
		n1 = cycs1[n];
		lendiff = (double)(lens1[n] - lens0[n]);
		isneg = 0;
		if(lendiff < 0.0) {
			isneg = 1;
			lendiff = -lendiff;
		}
		ratio = (double)m/(double)outcnt;
		invratio = 1.0 - ratio;

		/* FIND LENGTH OF CYCLE BY INTERPOLATING BETWEEN LENGTHS OF CYCLES IN TWO FILES */
		
		if(isneg)
			thislen = lens0[n] - (int)round(lendiff * ratio);
		else
			thislen = lens0[n] + (int)round(lendiff * ratio);

		/* FIND INCREMENT OF FILE-READING INDEX FOR EACH FILE */

		incr0 = (double)lens0[n]/(double)thislen;
		incr1 = (double)lens1[n]/(double)thislen;

		/* SET INITIAL VALUE OF THIS CYCLE BY INTERP ON 1st VAL IN CYCLE IN 2 FILES */

		obuf[j] = (float)((ibuf0[n0] * invratio) + (ibuf1[n1] * ratio));
		if(++j >= dz->buflen) {
			if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
				return(exit_status);
			j = 0;
		}

		/* READ REST OF CYCLES */

		indx0 = 0.0;
		indx1 = 0.0;
		for(k = 1; k < thislen; k ++) {
			indx0 += incr0;
			indx1 += incr1;

			/* GET A VALUE FROM EACH FILE BY INTERPOLATED READING */
			
			aa = (int)round(n0 + floor(indx0));
			startval = (double)ibuf0[aa];
			if((bb = aa + 1) >= cycs0[n+1])	/* If interp goes beyond cycle-end, treat endval as 0 */
				endval = 0.0;
			else
				endval = (double)ibuf0[bb];
			diff = endval - startval;
			frac = fmod(indx0,1.0);
			val0 = startval + (diff * frac);

			aa = (int)round(n1 + floor(indx1));
			startval = (double)ibuf1[aa];
			if((bb = aa + 1) >= cycs1[n+1])	/* If interp goes beyond cycle-end, treat endval as 0 */
				endval = 0.0;
			else
				endval = (double)ibuf1[bb];
			diff = endval - startval;
			frac = fmod(indx1,1.0);
			val1 = startval + (diff * frac);

		/* SET OUTVAL BY INTERPOLATING BETWEEN VALS GOT FROM BOTH FILES */

			obuf[j] = (float)((val0 * invratio) + (val1 * ratio));
			if(++j >= dz->buflen) {
				if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
					return(exit_status);
				j = 0;
			}
		}
	}
	if(j > 0) {
		if((exit_status = write_samps(obuf,j,dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}
