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
#include <string.h>

#include <structures.h>
#include <tkglobals.h>
#include <globcon.h>
#include <processno.h>
#include <modeno.h>
#include <filetype.h>
#include <arrays.h>
#include <mix.h>
#include <cdpmain.h>

#include <sfsys.h>

#include <mix.h>
#include <limits.h>

#ifdef unix
#define round(x) lround((x))
#endif

static int    do_mix2(float *inbuf1,float *inbuf2,double *dblbuf,float *obuf,dataptr dz);
static int    do_mix2_stagger(float *inbuf1,float *obuf,dataptr dz);
static int    do_mix2_skip(dataptr dz);

static int 	  cross_stagger(int *total_samps_read,int *end_of_samps_to_process,float *obuf,dataptr dz);
static int 	  pre_crossmix(int *samps_read2,int *cross_end,int *total_samps_read,
						int *end_of_samps_to_process,float *inbuf1,float *inbuf2,float *obuf,dataptr dz);
static int 	  crossmix(int *samps_read2,int *cross_end,int *total_samps_read,int *end_of_samps_to_process,
						float *inbuf1,float *inbuf2,float *obuf,dataptr dz);
static int 	  cross(int startcross,int endcross,float *inbuf1,float *inbuf2,float *obuf,dataptr dz);
static double read_cos(double d,dataptr dz);
static int 	  advance_in_files(int *samps_read2,int *total_samps_read,int *end_of_samps_to_process,
						int *cross_end,float *inbuf1,float *inbuf2,dataptr dz);
static void   copy_to_channel(float *inbuf,float *obuf,int sampcnt,int outchans);

static int renew_bufs(int typ,float *buf0,float *buf1,dataptr dz);
static int do_stitch(int typ,int *mbf,int *sbf,float *buf0,float *buf1,double atten,int bflen,dataptr dz);
static int flush_bufs(int typ,int *mbf,int *sbf,float *buf0,float *buf1,dataptr dz);

//TW JAN 2010: FOR MULTICHANNEL
//static int read_automix_sound(int *activebuf,int *chans,int chantype,dataptr dz);
static int read_automix_sound(int *activebuf,dataptr dz);

/************************************ MIXTWO ************************************/

int mixtwo(dataptr dz)
{
	int exit_status;
	char tempfnam[200];
	double *dblbuf = (double *) dz->sampbuf[3];		/* RWD I suppose we can keep this...*/
	float *inbuf1  = dz->sampbuf[0];
	float *inbuf2  = dz->sampbuf[1];
	float *obuf    = dz->sampbuf[2];
	int extra_skip;

	if(dz->iparam[MIX_STTA] > 0) {
		if((sndseekEx(dz->ifd[0],dz->iparam[MIX_STTA],0))<0) {
			sprintf(errstr,"sndseek() failed in first file.\n");
			return(SYSTEM_ERROR);
		}
	}
	if((extra_skip = dz->iparam[MIX_STTA] - dz->iparam[MIX_STAGGER]) > 0) {
		if((dz->iparam[MIX_SKIP] += extra_skip) > dz->insams[1]) {
			sprintf(errstr,"Mix skips the 2nd file entirely.\n");
			return(DATA_ERROR);
		}
	}
	if(dz->iparam[MIX_SKIP] > 0) {
		if((exit_status = do_mix2_skip(dz))<0)
			return(exit_status);
	}
	if((dz->iparam[MIX_STAGGER] -= dz->iparam[MIX_STTA]) > 0) {
		if((exit_status = do_mix2_stagger(inbuf1,obuf,dz))<0)
			return(exit_status);
	} else {
		dz->iparam[MIX_STAGGER] = 0;
	}
	if((exit_status = do_mix2(inbuf1,inbuf2,dblbuf,obuf,dz))<0)
		return(exit_status);
	if(is_converted_to_stereo >= 0) {
		strcpy(tempfnam,snd_getfilename(dz->ifd[is_converted_to_stereo]));
		sndcloseEx(dz->ifd[is_converted_to_stereo]);
		if(remove(tempfnam)<0)
			fprintf(stdout, "ERROR: %s: Can't remove temporary stereo soundfile %s.\n",sferrstr(),tempfnam);
		dz->ifd[is_converted_to_stereo] = -1;
	}
	return(FINISHED);
}

/************************************ DO_MIX2_SKIP ************************************/

int do_mix2_skip(dataptr dz)
{
	if((sndseekEx(dz->ifd[1],dz->iparam[MIX_SKIP],0))<0) {
		sprintf(errstr,"sndseek() failed: do_mix2_skip()\n");
		return(SYSTEM_ERROR);
	}
	return(FINISHED);
}

/************************************ DO_MIX2_STAGGER ************************************/

int do_mix2_stagger(float *inbuf1,float *obuf,dataptr dz)
{
	int exit_status;
	int stagbufs,stagsamps, n;
//TW
	int  shsecsize = F_SECSIZE;	   /* RWD I suppose we can keep this...*/
	int samps_read;


	stagbufs  = dz->iparam[MIX_STAGGER]/dz->buflen;
	stagsamps = dz->iparam[MIX_STAGGER] % dz->buflen;
	/*RWD do we still need this test? Is it still kosher ? */
	if(((stagsamps/shsecsize)*shsecsize) != stagsamps) {
		sprintf(errstr,"Error in sector arithmetic: do_mix2_stagger()\n");
		return(PROGRAM_ERROR);
	}
	while(stagbufs > 0) {
		if((samps_read = fgetfbufEx(inbuf1,dz->buflen,dz->ifd[0],0)) < dz->buflen) {
			if(samps_read < 0) {
				sprintf(errstr,"Sound read error.\n");
				return(SYSTEM_ERROR);
			} else {			
				sprintf(errstr,"Problem 1 reading from file1 during stagger\n");
				return(PROGRAM_ERROR);
			}
		}
		for(n=0;n<dz->buflen;n++)
			obuf[n]  = (float)(inbuf1[n] * dz->param[MIX2_GAIN1]);
		if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
			return(exit_status);
		stagbufs--;
	}			
	if(stagsamps) {
		if((samps_read = fgetfbufEx(inbuf1,stagsamps,dz->ifd[0],0)) < stagsamps) {
			if(samps_read < 0) {
				sprintf(errstr,"Sound read error.\n");
				return(SYSTEM_ERROR);
			} else {			
				sprintf(errstr,"Problem 2 reading from file1 during stagger\n");
				return(PROGRAM_ERROR);
			}
		}
		for(n=0;n<stagsamps;n++)
			obuf[n] = (float)(inbuf1[n]  * dz->param[MIX2_GAIN1]);
		if((exit_status = write_samps(obuf,stagsamps,dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}

/************************************ DO_MIX2 ************************************/

int do_mix2(float *inbuf1,float *inbuf2,double *dblbuf,float *obuf,dataptr dz)
{
	int exit_status;
	int gross_samps_read = 1, gross_total_samps_read = 0, cutoff_in_buf;
	int samps_read1, samps_read2, ssampsread, ssampsread1, ssampsread2, n;
	unsigned int cutoff_samps;

	cutoff_samps  = (dz->iparam[MIX_DURA] - dz->iparam[MIX_STTA]) - dz->iparam[MIX_STAGGER];
	if((cutoff_samps) <= 0)
		cutoff_samps = UINT_MAX;

	
	while(gross_samps_read > 0) {
		memset((char *)inbuf1,0,(size_t)(dz->buflen * sizeof(float)));
		memset((char *)inbuf2,0,(size_t)(dz->buflen * sizeof(float)));
		memset((char *)dblbuf,0,(size_t)(dz->buflen * sizeof(double)));
		if((samps_read1 = fgetfbufEx(inbuf1,dz->buflen,dz->ifd[0],0))<0) {
			sprintf(errstr,"Problem reading from file1 during mix.\n");
			return(SYSTEM_ERROR);
		}
		if((samps_read2 = fgetfbufEx(inbuf2,dz->buflen,dz->ifd[1],0))<0) {
			sprintf(errstr,"Problem reading from file1 during mix.\n");
			return(SYSTEM_ERROR);
		}
		if((gross_samps_read = max(samps_read1,samps_read2))<=0)
			break;
		gross_total_samps_read += gross_samps_read;

		if(cutoff_samps > (unsigned int) gross_total_samps_read)
			cutoff_in_buf = gross_samps_read;
		else 
			cutoff_in_buf = (cutoff_samps % dz->buflen);

		ssampsread1 = samps_read1;
		ssampsread1 = min(ssampsread1,cutoff_in_buf);

		if(ssampsread1) {
			for(n=0;n<ssampsread1;n++)
				dblbuf[n] = inbuf1[n] * dz->param[MIX2_GAIN1];
			if((ssampsread2 = samps_read2)>0) {
				ssampsread2 = min(ssampsread2,cutoff_in_buf);
				for(n=0;n<ssampsread2;n++)
		 			dblbuf[n] += inbuf2[n] * dz->param[MIX2_GAIN2];
			}
		} else {
			ssampsread2 = samps_read2;
			ssampsread2 = min(ssampsread2,cutoff_in_buf);
			for(n=0;n<ssampsread2;n++)
	 			dblbuf[n] = inbuf2[n] * dz->param[MIX2_GAIN2];
		}
		ssampsread = gross_samps_read;
		ssampsread = min(ssampsread,cutoff_in_buf);
		for(n=0;n<ssampsread;n++)
			obuf[n] = (float)dblbuf[n];
		if(ssampsread > 0) {
			if((exit_status = write_samps(obuf,ssampsread,dz))<0)
				return(exit_status);
		}
		if((unsigned int)gross_total_samps_read >= cutoff_samps)
			break;
	}
	return(FINISHED);
}


/******************************** MIX_CROSS **********************************/

int mix_cross(dataptr dz)
{
	int exit_status;
	float *inbuf1 = dz->sampbuf[0];
	float *inbuf2 = dz->sampbuf[1];
	float *obuf   = dz->sampbuf[0];
	int samps_read2 = 0;
	int end_of_samps_to_process = dz->iparam[MCR_END];
	int cross_end = 0;
	int total_samps_read = 0;
	int remain;
	if(end_of_samps_to_process <= 0) {
		sprintf(errstr,"Accounting problem: mix_cross()\n");
		return(PROGRAM_ERROR);
	}
	display_virtual_time(0,dz);
	if(dz->iparam[MCR_STAGGER]>0 && (exit_status = cross_stagger(&total_samps_read,&end_of_samps_to_process,obuf,dz))<0)
		return(exit_status);
	if(end_of_samps_to_process <=0) {
		sprintf(errstr,"Accounting problem 1: mix_cross()\n");
		return(PROGRAM_ERROR);
	}
	while(total_samps_read <= dz->iparam[MCR_BEGIN]) {
		if((exit_status = pre_crossmix
		(&samps_read2,&cross_end,&total_samps_read,&end_of_samps_to_process,inbuf1,inbuf2,obuf,dz))<0)
			return(exit_status);
		if(exit_status == FINISHED)
			break;
	}
	if(end_of_samps_to_process <=0) {
		sprintf(errstr,"Accounting problem 2: mix_cross()\n");
		return(PROGRAM_ERROR);
	}
	remain = samps_read2 - end_of_samps_to_process;
	if(remain < 0) {
		do {
			if((exit_status = crossmix
			(&samps_read2,&cross_end,&total_samps_read,&end_of_samps_to_process,inbuf1,inbuf2,obuf,dz))<0)
				return(exit_status);
		} while((remain = samps_read2 - end_of_samps_to_process) < 0);
	}
	if(remain)
		memmove((char *)(obuf + cross_end),(char *)(inbuf2 + cross_end),(size_t)(remain * sizeof(float)));
	if(samps_read2 > 0) {
		if((exit_status = write_samps(obuf,samps_read2,dz))<0)
			return(exit_status);
	}
	while((samps_read2 = fgetfbufEx(obuf, dz->buflen,dz->ifd[1],0))>0) {
		if((exit_status = write_samps(obuf,samps_read2,dz))<0)
			return(exit_status);
	}
	if(samps_read2<0) {
		sprintf(errstr,"Sound read error.\n");
		return(SYSTEM_ERROR);
	}
	return(FINISHED);
}

/******************************** CROSS **********************************/

int cross(int startcross,int endcross,float *inbuf1,float *inbuf2,float *obuf,dataptr dz)
{
	double xxx, yyy;
	register int i;
	int crosindex  = dz->iparam[MCR_INDEX];
	double crosfact = dz->param[MCR_CROSFACT];
	double powfac;
	switch(dz->iparam[MCR_CONTOUR]) {
	case(MCR_LINEAR):	  		/* LINEAR */
		for(i=startcross;i<endcross;i++) {
			xxx = (double)crosindex * crosfact;
			yyy = 1.0 - xxx;
			obuf[i] = (float)((inbuf1[i] * yyy) + (inbuf2[i] * xxx));
			crosindex++;
		}
		break;
	case(MCR_COSIN):			   /* COSIN */
		for(i=startcross;i<endcross;i++) {
			xxx = (double)crosindex * crosfact;
			xxx = read_cos(xxx,dz);
			yyy = 1.0 - xxx;
			obuf[i] = (float)((inbuf1[i] * yyy) + (inbuf2[i] * xxx));
			crosindex++;
		}
		break;
	case(MCR_SKEWED):		  	/* SKEWED COSIN */
		powfac   = dz->param[MCR_POWFAC];
		for(i=startcross;i<endcross;i++) {
			xxx = (double)crosindex * crosfact;
			xxx = pow(xxx,powfac);
			xxx = read_cos(xxx,dz);
			yyy = 1.0 - xxx;
			obuf[i] = (float)((inbuf1[i] * yyy) + (inbuf2[i] * xxx));
			crosindex++;
		}
		break;
	}
	dz->iparam[MCR_INDEX] = crosindex;
	return(FINISHED);
}

/************************** READ_COS **********************/

double read_cos(double d,dataptr dz)
{
	int j, k;
	double frac, diff;
	double *costable = dz->parray[MCR_COSTABLE];
	d *= (double)MCR_TABLEN;
	j = (int)d;	/* TRUNCATE */
	if((k=j+1)>MCR_TABLEN)
		return(1.0);
	frac = d - (double)j;
	diff = costable[k] - costable[j];
	frac *= diff;
	return(costable[j] + frac);
}

/************************** CROSSMIX **********************/

int crossmix(int *samps_read2,int *cross_end,int *total_samps_read,int *end_of_samps_to_process,
float *inbuf1,float *inbuf2,float *obuf,dataptr dz)
{
	int   exit_status;
	if((exit_status = advance_in_files
	(samps_read2,total_samps_read,end_of_samps_to_process,cross_end,inbuf1,inbuf2,dz))<0)
		return(exit_status);
	*cross_end = min(*cross_end,dz->buflen);
	if((exit_status = cross(0L,*cross_end,inbuf1,inbuf2,obuf,dz))<0)
		return(exit_status);
	if(*cross_end==dz->buflen) {
		if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}

/************************** PRE_CROSSMIX **********************/

int pre_crossmix(int *samps_read2,int *cross_end,int *total_samps_read,
int *end_of_samps_to_process,float *inbuf1,float *inbuf2,float *obuf,dataptr dz)
{
	int   exit_status;
	int  ssampstart, cross_start;
	int  last_total_samps_read = *total_samps_read;
	if((exit_status = advance_in_files
	(samps_read2,total_samps_read,end_of_samps_to_process,cross_end,inbuf1,inbuf2,dz))<0)
		return(exit_status);
	if(*total_samps_read > dz->iparam[MCR_BEGIN]) {
		if((cross_start = dz->iparam[MCR_BEGIN] - last_total_samps_read)>=dz->buflen) {
			sprintf(errstr,"Accounting problem: pre_crossmix()\n");
			return(PROGRAM_ERROR);
		}
		ssampstart = cross_start;
		*cross_end = min(*cross_end,dz->buflen);
		if((exit_status = cross(ssampstart,*cross_end,inbuf1,inbuf2,obuf,dz))<0)
			return(exit_status);
		if(*cross_end < dz->buflen)
			return(FINISHED);
	}
	if((exit_status= write_samps(obuf,dz->buflen,dz))<0)
		return(exit_status);
	return(CONTINUE);
}

/************************** CROSS_STAGGER **********************/

int cross_stagger(int *total_samps_read,int *end_of_samps_to_process,float *obuf,dataptr dz)
{
	int exit_status;
	int  samps_read1, samps_written;
	int stagger = dz->iparam[MCR_STAGGER];
	int stagbufs  = stagger/dz->buflen;
	int stagsamps = stagger%dz->buflen;
//TW SAFE??
//	if(((stagsamps/F_SECSIZE)*F_SECSIZE)!=stagsamps) {
//		sprintf(errstr,"Sector accounting problem: cross_stagger()\n");
//		return(PROGRAM_ERROR);
//	}
	while(stagbufs>0) {
		if((samps_read1 = fgetfbufEx(obuf, dz->buflen,dz->ifd[0],0))!=dz->buflen) {
			if(samps_read1<0) {
				sprintf(errstr,"Sound read error.\n");
				return(SYSTEM_ERROR);
			} else {
				sprintf(errstr,"First file read failed:1: cross_stagger()\n");
				return(PROGRAM_ERROR);
			}
		}
//		if((samps_written = fputfbufEx(obuf,dz->buflen,dz->ofd))!=dz->buflen) {
		if((exit_status = write_samps_no_report(obuf,dz->buflen,&samps_written,dz))<0)
			return(exit_status);
		if(samps_written!=dz->buflen) { 
			sprintf(errstr,"Problem writing samps:1: cross_stagger()\n");
			return(PROGRAM_ERROR);
		}
		dz->total_samps_written  += samps_written;
		*total_samps_read        += dz->buflen;
 		*end_of_samps_to_process -= dz->buflen;
		stagbufs--;
	}
	if(stagsamps > 0) {
		if((samps_read1 = fgetfbufEx(obuf, stagsamps,dz->ifd[0],0))!=stagsamps) {
			if(samps_read1<0) {
				sprintf(errstr,"Sound read error.\n");
				return(SYSTEM_ERROR);
			} else {
				sprintf(errstr,"First file read failed:2: cross_stagger()\n");
				return(PROGRAM_ERROR);
			}
		}
//		if((samps_written = fputfbufEx(obuf,stagsamps,dz->ofd))!=stagsamps) {
		if((exit_status = write_samps_no_report(obuf,stagsamps,&samps_written,dz) )< 0)
			return(exit_status);
		if(samps_written != stagsamps) {			
			sprintf(errstr,"Problem writing samps:2: cross_stagger()\n");
			return(PROGRAM_ERROR);
		}
		dz->total_samps_written  += stagsamps;
		*total_samps_read        += stagsamps;
 		*end_of_samps_to_process -= stagsamps;
	}
	return(FINISHED);
}

/************************** ADVANCE_IN_FILES **********************/

int advance_in_files(int *samps_read2,int *total_samps_read,int *end_of_samps_to_process,
int *cross_end,float *inbuf1,float *inbuf2,dataptr dz)
{
	int samps_read1;	

	*end_of_samps_to_process = dz->iparam[MCR_END] - *total_samps_read;
	if((samps_read1 = fgetfbufEx(inbuf1, dz->buflen,dz->ifd[0],0))<0) {
		sprintf(errstr,"First file read failed: advance_in_files()\n");
		return(SYSTEM_ERROR);
	}
	if((*samps_read2 = fgetfbufEx(inbuf2, dz->buflen,dz->ifd[1],0))<0) {
		sprintf(errstr,"Second file read failed: advance_in_files()\n");
		return(SYSTEM_ERROR);
	}
	*total_samps_read += *samps_read2;	/* BECAUSE FADE IS TOWARDS FILE 2: and fade must END before then */
	*cross_end = *end_of_samps_to_process;
	return(FINISHED);
}

/************************************ MIX_INTERL ************************************/

int mix_interl(dataptr dz)
{
	int exit_status;
	float *inbuf  = dz->sampbuf[0];
	float *outbuf = dz->sampbuf[1];	/* NB this buffer is large enough to take output */
	
	int samps_read, samps_left[MAX_MI_OUTCHANS];
	int n, gross_samps_read, gross_samps_left = 0, last_total_samps_written, samps_to_write;

	for(n=0;n<dz->infilecnt;n++)  {
		samps_left[n] = dz->insams[n];
		gross_samps_left = max(gross_samps_left,samps_left[n]);
	}

	while(gross_samps_left) {
		memset((char *)outbuf,0,(size_t)(dz->buflen * dz->infilecnt * sizeof(float)));
		gross_samps_read = 0;
		gross_samps_left = 0;
		for(n=0;n<dz->infilecnt;n++) {
			if(samps_left[n]>0) {
	 			if((samps_read = fgetfbufEx(inbuf,dz->buflen,dz->ifd[n],0))<0) {
					sprintf(errstr,"Failed to read data from file %d: mix_interl()\n",n+1);
					return(SYSTEM_ERROR);
				}
				samps_left[n] -= samps_read;
				copy_to_channel(inbuf,outbuf+n,samps_read,dz->infilecnt);
				gross_samps_read = max(gross_samps_read,samps_read);
				gross_samps_left = max(gross_samps_left,samps_left[n]);
			}
		}
		samps_to_write =  gross_samps_read*dz->infilecnt;
		last_total_samps_written = dz->total_samps_written;
		if(samps_to_write) {
			if((exit_status = write_samps(outbuf,samps_to_write,dz))<0)
				return(exit_status);
		}
		if(dz->total_samps_written - last_total_samps_written != samps_to_write) {
			sprintf(errstr,"Problem writing data: mix_interl()\n");
			return(PROGRAM_ERROR);
		}
	}
// TW
//	/*RWD too late here! */
//	dz->infile->channels = dz->infilecnt;	/* Force output to final channel-count */

	return(FINISHED);
}

/************************************ COPY_TO_CHANNEL ************************************/

void copy_to_channel(float *inbuf,float *outbuf,int sampcnt,int outchans)
{
	int n, m;
	for(n=0,m=0;n<sampcnt;n++,m+=outchans)
		outbuf[m] = inbuf[n];	
}

/******************************** CROSS_STITCH *****************************/

int cross_stitch(dataptr dz) {
	double srate = (double)dz->infile->srate;
	int n = 0, mbf, sbf, procend, input_procend;
	int exit_status, t = 0,v=0,m=0,done = 0;
	double *brrk = NULL, thisval, nextval = 0.0, valincr, atten = 0.0, val_here;
	double time_here, lastrtime,nextrtime, lastval, tratio;

	int nexttime = 0, thistime;
	float *buf0 = dz->sampbuf[0];
	float *buf1 = dz->sampbuf[1];
	int typ;
	int input_procskip, samps_to_skip = 0;

	if(dz->param[MIX_SKEW] <= dz->param[MIX_SKIP]) {
		sprintf(errstr,"Start and end times of mix are reversed, or equal.\n");
		return(DATA_ERROR);
	}
	/* RWD make this samps */
	dz->tempsize = 
	(int)round((dz->param[MIX_SKEW] - dz->param[MIX_SKIP]) * (double)(srate * 
	max(dz->infile->channels,dz->otherfile->channels)));
	display_virtual_time(0,dz);
	if(dz->infile->channels > 2) {
		typ = 4;	/* multichan multichan */
	} else if(dz->infile->channels==1) {
		if(dz->otherfile->channels==1) {
			typ = 0; 	/* mono mono */
		} else {	
			typ = 1;	/* mono stereo */
		}
	} else {
		if(dz->otherfile->channels==1) {
			typ = 2; 	/* stereo mono */
		} else {	
			typ = 3;	/* stereo stereo */
		}
	}
	procend = dz->insams[0]/dz->infile->channels;
	input_procend = (int)round(dz->param[MIX_SKEW] * srate);
	procend = min(procend,input_procend);

	if((input_procskip = (int)round(dz->param[MIX_SKIP] * srate)) > 0) {
		samps_to_skip = input_procskip * dz->infile->channels;
//TW SAFE??
//		samps_to_skip /= (F_SECSIZE * dz->infile->channels);
//		samps_to_skip *= (F_SECSIZE * dz->infile->channels);	/* skip to nearest sector */

		if((sndseekEx(dz->ifd[0],samps_to_skip,0))<0) {
			sprintf(errstr,"seek() failed searching for startpoint of mix in first file.\n");
			return(SYSTEM_ERROR);
		}
		samps_to_skip = input_procskip * dz->otherfile->channels;
//TW SAFE??
//		samps_to_skip /= (F_SECSIZE * dz->otherfile->channels);
//		samps_to_skip *= (F_SECSIZE * dz->otherfile->channels);	/* skip to nearest sector */

		if((sndseekEx(dz->ifd[1],samps_to_skip,0))<0) {
			sprintf(errstr,"sndseek() failed searching for startpoint of mix in second file.\n");
			return(SYSTEM_ERROR);
		}
	}
	if(dz->brksize[MIX_STAGGER]) {
		brrk = dz->brk[MIX_STAGGER];
		if(input_procskip > 0) {
			thistime = samps_to_skip/dz->otherfile->channels;
			time_here = (double)thistime/srate;
			m = 0;
			while(brrk[m] <= time_here) {
				m += 2;
				if(m >= dz->brksize[MIX_STAGGER]) {			/* IF beyond end of brk */
					dz->param[MIX_STAGGER] = brrk[m-1];		/* set atten to last brkval */
					dz->brksize[MIX_STAGGER] = 0;			/* set brkfile as not existing */
					break;
				}
			}
			if(dz->brksize[MIX_STAGGER]) {					/* If still in brkfile */
				lastrtime = brrk[m-2];						/* Note previous brkpnt-time+val */
				lastval   = brrk[m-1];
				n = thistime;								/* set the internal sample counter to here */
				t = m;										/* set pointer for (following) brkpnt times */
				v = m+1;									/* set pointer for (following) brkpnt vals */
				m /= 2;										/* set position in brk-pairs */
				nextrtime = brrk[t];
				nexttime  = round(nextrtime * srate);
				nextval   = brrk[v];						
															/* calc value at start-time */
				tratio = (double)(time_here - lastrtime)/(double)(nextrtime - lastrtime);
				val_here = ((nextval - lastval) * tratio) + lastval;
				nexttime = thistime;						/* set up starting time & val */
				nextval  = val_here;
			}								
		} else {
			nexttime = round(brrk[0] * srate);
			nextval  = brrk[1];	/* set up starting time & val */
			n = 0;			   	/* set the internal sample counter to start */
			t = 2;			   	/* set pointer for (following) brkpnt times */
			v = 3;			   	/* set pointer for (following) brkpnt vals */
			m = 1;			   	/* set position in brk-pairs */
		}
	} else {
		n = input_procskip;
	}
	memset((char *)buf0,0,dz->buflen * dz->infile->channels* sizeof(float));
	memset((char *)buf1,0,dz->buflen * dz->otherfile->channels * sizeof(float));
	if((fgetfbufEx(buf0,dz->buflen * dz->infile->channels,dz->ifd[0],0)) < 0) {
		sprintf(errstr,"Can't read samps from 1st soundfile.\n");
		return(SYSTEM_ERROR);
	}
	if((fgetfbufEx(buf1,dz->buflen * dz->otherfile->channels,dz->ifd[1],0)) < 0) {
		sprintf(errstr,"Can't read samps from 2nd soundfile.\n");
		return(SYSTEM_ERROR);
	}
	mbf = 0;
	sbf = 0;
	if(dz->brksize[MIX_STAGGER] == 0)
		atten = dz->param[MIX_STAGGER];
	else {
		for(; m < dz->brksize[MIX_STAGGER]; m++,t+=2,v+=2) {
			thistime = nexttime;
			thisval  = nextval;
			nexttime = round(brrk[t] * srate);
			nextval  = brrk[v];
			valincr = (nextval - thisval)/(double)(nexttime - thistime);
			if(nexttime >= procend) {		/* IF brktable extends beyond end of sound */
				nexttime = procend;
				done = 1;
			}
			atten = thisval;
			while(n < nexttime) {
  				if(do_stitch(typ,&mbf,&sbf,buf0,buf1,atten,dz->buflen,dz)) {
					if((exit_status = renew_bufs(typ,buf0,buf1,dz))<0)
						return(exit_status);
					mbf = 0;
					sbf = 0;
				}
				atten += valincr;
				n++;
			}
			if(done)				   	/* IF brktable extends beyond end of sound: break */
				break;
		}
	}
	while(n < procend) {				/* IF brktable stops short of end of sound, or no brktable */
		if(do_stitch(typ,&mbf,&sbf,buf0,buf1,atten,dz->buflen,dz)) {
			if((exit_status = renew_bufs(typ,buf0,buf1,dz))<0)
				return(exit_status);
			mbf = 0;
			sbf = 0;
		}
		n++;
	}
	return flush_bufs(typ,&mbf,&sbf,buf0,buf1,dz);
}

/******************************** DO_STITCH *****************************/

int do_stitch(int typ,int *mbf,int *sbf,float *buf0,float *buf1,double atten,int bflen,dataptr dz)
{
	int bufdone = 0, k;
	switch(typ) {
	case(0):
		buf0[*mbf] = (float)((buf0[*mbf] * atten) + (buf1[*mbf] * (1.0 - atten)));
		if(++(*mbf) >=  bflen)
			bufdone = 1;
		break;
	case(1):
		buf1[*sbf] = (float)((buf0[*mbf] * atten) + (buf1[*sbf] * (1.0 - atten)));
		(*sbf)++;
		buf1[*sbf] = (float)((buf0[*mbf] * atten) + (buf1[*sbf] * (1.0 - atten)));
		(*sbf)++;
		if(++(*mbf) >=  bflen)
			bufdone = 1;
		break;
	case(2):
		buf0[*sbf] = (float)((buf0[*sbf] * atten) + (buf1[*mbf] * (1.0 - atten)));
		(*sbf)++;
		buf0[*sbf] = (float)((buf0[*sbf] * atten) + (buf1[*mbf] * (1.0 - atten)));
		(*sbf)++;
		if(++(*mbf) >=  bflen)
			bufdone = 1;
		break;
	case(3):
		buf0[*sbf] = (float)((buf0[*sbf] * atten) + (buf1[*sbf] * (1.0 - atten)));
		(*sbf)++;
		buf0[*sbf] = (float)((buf0[*sbf] * atten) + (buf1[*sbf] * (1.0 - atten)));
		if(++(*sbf) >=  bflen * 2)
			bufdone = 1;
		break;
	case(4):
		for(k=0; k < dz->infile->channels; k++) {
			buf0[*sbf] = (float)((buf0[*sbf] * atten) + (buf1[*sbf] * (1.0 - atten)));
			(*sbf)++;
		}
		if(*sbf >=  bflen * dz->infile->channels)
			bufdone = 1;
		break;
	}	
	return(bufdone);
}

/******************************** RENEW_BUFS *****************************/

int renew_bufs(int typ,float *buf0,float *buf1,dataptr dz)
{
	int exit_status;
	switch(typ) {
	case(0):
		if((exit_status = write_samps(buf0,dz->buflen,dz))<0)
			return(exit_status);
		memset((char *)buf0,0,dz->buflen * sizeof(float));
		memset((char *)buf1,0,dz->buflen * sizeof(float));
		break;
	case(1):
		if((exit_status = write_samps(buf1,dz->buflen * 2,dz))<0)
			return(exit_status);
		memset((char *)buf0,0,dz->buflen * sizeof(float));
		memset((char *)buf1,0,dz->buflen * 2 * sizeof(float));
		break;
	case(2):
		if((exit_status = write_samps(buf0,dz->buflen * 2,dz))<0)
			return(exit_status);
		memset((char *)buf0,0,dz->buflen * 2 * sizeof(float));
		memset((char *)buf1,0,dz->buflen * sizeof(float));
		break;
	case(3):
		if((exit_status = write_samps(buf0,dz->buflen * 2,dz))<0)
			return(exit_status);
		memset((char *)buf0,0,dz->buflen * 2 * sizeof(float));
		memset((char *)buf1,0,dz->buflen * 2 * sizeof(float));
		break;
	case(4):
		if((exit_status = write_samps(buf0,dz->buflen * dz->infile->channels,dz))<0)
			return(exit_status);
		memset((char *)buf0,0,dz->buflen * dz->infile->channels * sizeof(float));
		memset((char *)buf1,0,dz->buflen * dz->otherfile->channels * sizeof(float));
		break;
	}
	if((fgetfbufEx(buf0,dz->buflen * dz->infile->channels,dz->ifd[0],0)) < 0) {
		sprintf(errstr,"Can't read samps from 1st soundfile.\n");
		return(SYSTEM_ERROR);
	}
	if((fgetfbufEx(buf1,dz->buflen * dz->otherfile->channels,dz->ifd[1],0)) < 0) {
		sprintf(errstr,"Can't read samps from 2nd soundfile.\n");
		return(SYSTEM_ERROR);
	}
	return(FINISHED);
}

/******************************** FLUSH_BUFS *****************************/

int flush_bufs(int typ,int *mbf,int *sbf,float *buf0,float *buf1,dataptr dz)
{
	int exit_status;
	switch(typ) {
	case(0):						/* Output file takes its channel cnt from 1st input file : HENCE....  */
		if(*mbf > 0) {
			if((exit_status = write_samps(buf0,*mbf,dz))<0)
				return(exit_status);
		}							/* 1st input file MONO : output file MONO */
		break;
	case(1):
		if(*sbf > 0) {
			if((exit_status = write_samps(buf1,*sbf,dz))<0)
				return(exit_status);
		}							/* 1st input file MONO : output file STEREO */
		dz->infile->channels = 2;	/* Force output file to have stereo header */
		break;
	case(2):						/* 1st input file STEREO : output file STEREO */
	case(3):
	case(4):
		if(*sbf > 0) {
			if((exit_status = write_samps(buf0,*sbf,dz))<0)
				return(exit_status);
		}
		break;
	}
	return(FINISHED);
}

//TW JANUARY 2010: MODIFIED FOR MULTICHANNEL

/*************************** DO_AUTOMIX *******************************/

int do_automix(dataptr dz)
{
	double srate = (double)dz->infile->srate;
	unsigned int nexttime, thistime, outcnt = 0, bufcnt = 0;
	int outchans = 1, exit_status;
	double *p = dz->parray[0], *arrayend = dz->parray[0] + dz->itemcnt;
	double *thislevel, *nextlevel, *incr, sum, convertor_to_time, convertor_to_smps;
	int *activebuf, overflows = 0;
	unsigned int max_insams = 0;
	int *chans, n;

	if((chans = (int *)malloc(dz->infilecnt * sizeof(int)))==NULL) {
		sprintf(errstr,"Insufficient memory for level stores\n");
		return(MEMORY_ERROR);
	}
	if((thislevel = (double *)malloc(dz->infilecnt * sizeof(double)))==NULL) {
		sprintf(errstr,"Insufficient memory for level stores\n");
		return(MEMORY_ERROR);
	}
	if((nextlevel = (double *)malloc(dz->infilecnt * sizeof(double)))==NULL) {
		sprintf(errstr,"Insufficient memory for level stores\n");
		return(MEMORY_ERROR);
	}
	if((incr = (double *)malloc(dz->infilecnt * sizeof(double)))==NULL) {
		sprintf(errstr,"Insufficient memory for level stores\n");
		return(MEMORY_ERROR);
	}
	if((activebuf = (int *)malloc(dz->infilecnt * sizeof(int)))==NULL) {
		sprintf(errstr,"Insufficient memory for level stores\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<dz->infilecnt;n++) {
		activebuf[n] = 1;
		if(sndgetprop(dz->ifd[n],"channels", (char *)&(chans[n]), sizeof(int)) < 0) {
			sprintf(errstr,"Failure to read channel data, for input file %d\n",n+1);
			return(DATA_ERROR);
		}
		if(n>0 && chans[n] != chans[0]) {
			sprintf(errstr,"Process only works with files having the same number of channels: file %d will not work\n",n+1);
			return(DATA_ERROR);
		}
	}
	outchans = chans[0];
	convertor_to_smps = srate * outchans;
	convertor_to_time = 1.0/convertor_to_smps;
	for(n=0;n<dz->infilecnt;n++)
		max_insams = max(max_insams,(unsigned int)dz->insams[n]);
	dz->tempsize = max_insams;	/* for scrollbar display */
	if((exit_status = create_sized_outfile(dz->outfilename,dz))<0)
		return(exit_status);
	if((exit_status = read_automix_sound(activebuf,dz))<0)
		return(exit_status);
	nexttime = (unsigned int)round(*p++ * convertor_to_smps);
	for(n = 0;n<dz->infilecnt; n++)
		nextlevel[n] = *p++;
	while(outcnt < max_insams) {
		thistime = nexttime;	  				/* establish amplitude ramps */
		for(n = 0; n<dz->infilecnt; n++)
			thislevel[n] = nextlevel[n];
		if(p < arrayend) {
			nexttime = (unsigned int)round(*p++ * convertor_to_smps);
			for(n = 0;n<dz->infilecnt; n++) {
				nextlevel[n] = *p++;
				incr[n] = (nextlevel[n] - thislevel[n])/(double)(nexttime - thistime);
			}
		} else {								/* if amp-data runs out */
			for(n = 0;n<dz->infilecnt; n++)		/* set amp increments to zero */
				incr[n] = 0.0;
			nexttime = max_insams;				/* and proceed to end of file */
		}
		nexttime = min(nexttime, max_insams);	/* if snd-data runs out, curtail loop to snd-data end */
		while(outcnt < nexttime) {
			sum = 0.0;
			for(n=0;n<dz->infilecnt;n++) {
				if(activebuf[n]) {
					sum += dz->sampbuf[n][bufcnt] * thislevel[n];
					thislevel[n] += incr[n];
					if(outcnt >= (unsigned int)dz->insams[n])
						activebuf[n] = 0;
				}
			}
			if(fabs(sum) > F_MAXSAMP)
				overflows++;
			dz->sampbuf[dz->infilecnt][bufcnt] = (float)(sum * dz->param[0]);
			if(++bufcnt >= (unsigned int)dz->buflen) {
				if((exit_status = write_samps(dz->sampbuf[dz->infilecnt],dz->buflen,dz))<0)
					return(exit_status);
				if((exit_status = read_automix_sound(activebuf,dz))<0)
					return(exit_status);
				bufcnt = 0;
			}
			outcnt++;
			if(dz->brksize[0]) {
				if((exit_status = read_value_from_brktable((double)outcnt * convertor_to_time,0,dz))<0)
					return(exit_status);
			}
		}
	}
	if(bufcnt) {
		if((exit_status = write_samps(dz->sampbuf[dz->infilecnt],bufcnt,dz))<0)
			return(exit_status);
	}
	if(overflows) {
		fprintf(stdout,"WARNING: %d samples were clipped.\n",overflows);
		fflush(stdout);
	}
	return(FINISHED);
}

/*************************** READ_AUTOMIX_SOUND *******************************/

int read_automix_sound(int *activebuf,dataptr dz)
{
	int n;
	for(n=0;n<dz->infilecnt;n++) {
		if(!activebuf[n])		  	/* only read from files with data remaining */
			continue;
		if((dz->ssampsread = fgetfbufEx(dz->sampbuf[n],dz->buflen,dz->ifd[n],0)) < 0) {
			sprintf(errstr,"Can't read samps from input soundfile %d.\n",n+1);
			return(SYSTEM_ERROR);
		}
	}
	return(FINISHED);
}


/************************************ MIXMANY ************************************/

int mixmany(dataptr dz)
{
	int exit_status;
	int gross_samps_read, n, m;
	int ssampsread;
	double *dblbuf = (double *)dz->sampbuf[2], maxdsamp = 0.0;
	float *ibuf    = dz->sampbuf[0];
	float *obuf    = dz->sampbuf[1];

	dz->tempsize = 0;
	for(n=0;n<=dz->infilecnt;n++)
		dz->tempsize = max(dz->tempsize,dz->insams[n]);
	display_virtual_time(0L,dz);
	fprintf(stdout,"INFO: Finding maximum level\n");
	fflush(stdout);
	do {
		memset((char *)ibuf,0,(size_t)dz->buflen * sizeof(float));
		memset((char *)dblbuf,0,(size_t)dz->buflen * sizeof(double));
		gross_samps_read = 0;
		for(n=0;n<dz->infilecnt;n++) {
			if((ssampsread = fgetfbufEx(ibuf,dz->buflen,dz->ifd[n],0))<0) {
				sprintf(errstr,"Problem reading from file %d\n",n+1);
				return(SYSTEM_ERROR);
			}
			gross_samps_read = max(gross_samps_read,ssampsread);

			if(ssampsread > 0) {
				for(m=0;m<ssampsread;m++)
					dblbuf[m] += (double)ibuf[m];
			}
		}
		if(gross_samps_read<=0)
			break;
		for(m=0;m<gross_samps_read;m++)
			maxdsamp = max(maxdsamp,fabs(dblbuf[m]));

	} while(gross_samps_read > 0);
	maxdsamp = F_MAXSAMP/maxdsamp;
	for(n=0;n<dz->infilecnt;n++) {
		if(sndseekEx(dz->ifd[n],0,0)<0) {
			sprintf(errstr,"seek failed for file %d\n",n+1);
			return(SYSTEM_ERROR);
		}
	}
	dz->total_samps_written = 0;
	display_virtual_time(0,dz);
	fprintf(stdout,"INFO: Doing the mix.\n");
	fflush(stdout);
	do {
		memset((char *)ibuf,0,(size_t)dz->buflen * sizeof(float));
		memset((char *)dblbuf,0,(size_t)dz->buflen * sizeof(double));
		gross_samps_read = 0;
		for(n=0;n<dz->infilecnt;n++) {
			if((ssampsread = fgetfbufEx(ibuf,dz->buflen,dz->ifd[n],0))<0) {
				sprintf(errstr,"Problem reading from file %d during mix.\n",n+1);
				return(SYSTEM_ERROR);
			}
			gross_samps_read = max(gross_samps_read,ssampsread);
			if(ssampsread  > 0) {
				for(m=0;m<ssampsread;m++)
					dblbuf[m] += (double)ibuf[m];
			}
		}
		if(gross_samps_read<=0)
			break;
		for(m=0;m<gross_samps_read;m++)
			obuf[m] = (float)(dblbuf[m] * maxdsamp);
		if((exit_status = write_samps(obuf,gross_samps_read,dz))<0)
			return(exit_status);
	} while(gross_samps_read > 0);
	return(FINISHED);
}


