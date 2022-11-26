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



/*floatsam version*/
/*NB CUTGATE_SHRTBLOK RENAMED CUTGATE_SAMPBLOK*/
#include <stdio.h>
#include <stdlib.h>
#include <structures.h>
#include <tkglobals.h>
#include <globcon.h>
#include <cdpmain.h>
#include <house.h>
#include <modeno.h>
#include <pnames.h>
#include <logic.h>
#include <flags.h>
#include <arrays.h>
/*RWD April 2004 TW forgot this...*/
#include <processno.h>
#include <sfsys.h>
#include <string.h>
#include <srates.h>

#ifdef unix
#define round(x) lround((x))
#endif

/* RWD keep this? */
#define SAMPLE_T float

#define	IBUF	(0)
#define	OBUF	(1)
#define	BUFEND	(2)
#define	WRAP	(1)

#define	ENV		(0)
#define	ENVEND	(1)
/* RWD ????? */
//TW REDUNDANT
//#define SR		(22050)
//#define SRDAT	(24000)
//#define SRDATLO	(16000)

#define MAXCUTS 	(999)

#define TOPN_DEFAULT_SPLEN 	(15.0)	/* MS */

static int  top_and_tail(dataptr dz);
static int  get_startsamp(float *ibuf,int chans,int nbuff,int *startsamp,double gate,double ngate,dataptr dz);
static int  get_endsamp(float *ibuf,int chans,int nbuff,int *endsamp,double gate,double ngate,dataptr dz);
//TW REVISED
//static int  do_startsplice(float **ibuf,float **obuf,int splicecnt,
//			int startseek,int *sampseek,int *firsttime,int offset,int offset_samps,int wrap_samps,int input_report,dataptr dz);
static int do_startsplice(int splicecnt,int startsamp,int *sampseek,int input_report,dataptr dz);
//TW REVISED
//static int  advance_to_endsamp(float **ibuf,float **obuf,int endsamp,int *sampseek,
//			int *firsttime,int wrap_samps,int offset_samps,int input_report,dataptr dz);
static int  advance_to_endsamp(int endsamp,int *sampseek,int input_report,dataptr dz);
//TW REVISED
//static int  do_end_splice(float **ibuf,float **obuf,int *k,int splicecnt,int firsttime,
//			int wrap_samps,int offset_samps,int input_report,dataptr dz);
static int do_end_splice(int *k,int splicecnt,int input_report,dataptr dz);

static int  do_cutgate(dataptr dz);
//TW SUGGESTS, REDUNDANT
//static int  get_bufmult_factor(int srate,int channels,dataptr dz);
static int  ggetenv(dataptr dz);
static void searchpars(dataptr dz);
static void readenv(int samps_to_process,SAMPLE_T **env,dataptr dz);
static void get_maxsamp(SAMPLE_T *env,int startsamp,dataptr dz);

static int  findcuts(int *ccnt,dataptr dz);
static int  findcuts2(int *ccnt,dataptr dz);
static int  create_outfile_name(int ccnt,char *thisfilename,dataptr dz);
static int  output_cut_sndfile(int ccnt,char *filename,dataptr dz);
static int  store_cut_positions_in_samples(int ccnt,int startse,int endsec,dataptr dz) ;
static int  setup_naming(char **filename,dataptr dz);

static int  do_rectify(dataptr dz);

static int  create_cutgate_buffer(dataptr dz);
static int  create_topntail_buffer(dataptr dz);
//TW FUNCTION NEW REDUNDANT
//static int	do_by_hand(dataptr dz);
//TW UPDATES
static int find_onset_cuts(int *ccnt,dataptr dz);
static int store_startcut_positions_in_samples(int ccnt,int startsec,dataptr dz);


/******************************* PCONSISTENCY_CLEAN *******************************/

int	pconsistency_clean(dataptr dz)
{
//	int exit_status;
	double sr = (double)dz->infile->srate;
	int chans = dz->infile->channels;

	if(dz->process == TOPNTAIL_CLICKS) {
		if(!dz->vflag[STT_TRIM] && !dz->vflag[END_TRIM]) {
			sprintf(errstr,"Input file will be UNCHANGED.\n");
			return(GOAL_FAILED);
		}
		return FINISHED;
	}
	switch(dz->mode) {
	case(HOUSE_CUTGATE):
		dz->iparam[CUTGATE_SPLCNT] = round(dz->param[CUTGATE_SPLEN] * MS_TO_SECS * sr);
		dz->iparam[CUTGATE_SPLEN]  = dz->iparam[CUTGATE_SPLCNT] * chans;
		/* fall thro */
	case(HOUSE_ONSETS):
		if(dz->param[CUTGATE_INITLEVEL]>0 && dz->iparam[CUTGATE_BAKTRAK]==0) {
			fprintf(stdout,"WARNING: INITIAL LEVEL set without Baktraking: no effect.\n");
			fflush(stdout);
		}
		if(flteq(dz->param[CUTGATE_ENDGATE],0.0))
			dz->param[CUTGATE_ENDGATE] = dz->param[CUTGATE_GATE];
		dz->iparam[CUTGATE_MINLEN] = round(dz->param[CUTGATE_MINLEN] * sr) * chans;

		/* fall thro */
	case(HOUSE_CUTGATE_PREVIEW):
//TW SUGGESTS this will work fine, & still be consistent with old-buffering-protocol... 
		dz->iparam[CUTGATE_SAMPBLOK] = F_SECSIZE * dz->infile->channels;
		break;
//TW !!!!!
	case(HOUSE_TOPNTAIL):
		if(dz->vflag[NO_STT_TRIM] && dz->vflag[NO_END_TRIM]) {
			sprintf(errstr,"Input file will be UNCHANGED.\n");
			return(GOAL_FAILED);
		}
//TW UPDATE (redundant)
//		dz->param[TOPN_NGATE] = -dz->param[TOPN_GATE];
		break;
	}
	return(FINISHED);
}

/******************************* CREATE_CLEAN_BUFFERS *******************************/

int	create_clean_buffers(dataptr dz)
{
	if(dz->process == TOPNTAIL_CLICKS)
		return create_topntail_buffer(dz);

	switch(dz->mode) {
	case(HOUSE_CUTGATE_PREVIEW):
//TW NEW CASE
	case(HOUSE_ONSETS):
	case(HOUSE_CUTGATE):	return create_cutgate_buffer(dz);
	case(HOUSE_TOPNTAIL):	return create_topntail_buffer(dz);
	}
	sprintf(errstr,"Unknown mode: create_clean_buffers()\n");
	return(PROGRAM_ERROR);
}

/******************************* CREATE_CUTGATE_BUFFER ****************************/

int create_cutgate_buffer(dataptr dz)
{   
	int k, envelope_space,total_bufsize;
	int numsecs;
	int samp_blocksize = dz->iparam[CUTGATE_SAMPBLOK] * sizeof(float);
	size_t bigbufsize;

	bigbufsize = (size_t)Malloc(-1);
	if((bigbufsize = (bigbufsize/samp_blocksize) * samp_blocksize)<=0)
		bigbufsize = samp_blocksize;
	dz->buflen = (int)(bigbufsize/sizeof(float));
	/* dz->buflen needed for searchpars() */
    searchpars(dz);
	if((k = 
	dz->iparam[CUTGATE_NUMSECS]/dz->iparam[CUTGATE_SAMPBLOK])* dz->iparam[CUTGATE_SAMPBLOK] < dz->iparam[CUTGATE_NUMSECS])
		k++;
	numsecs = k * dz->iparam[CUTGATE_SAMPBLOK];
	envelope_space = (numsecs + dz->iparam[CUTGATE_WINDOWS]) * sizeof(float);
 	total_bufsize = bigbufsize + envelope_space + (F_SECSIZE * sizeof(float));	/* overflow sector in sndbuf */
	if((dz->bigbuf = (float *)malloc((size_t)total_bufsize)) == NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for sound and envelope.\n");
		return(MEMORY_ERROR);
	}
	dz->sampbuf[ENV] = dz->bigbuf + dz->buflen + F_SECSIZE;
	return(FINISHED);
}

/******************************* PROCESS_CLEAN *******************************/

int	process_clean(dataptr dz)
{
	if(dz->process == TOPNTAIL_CLICKS)
		return top_and_tail(dz);

	switch(dz->mode) {
	case(HOUSE_CUTGATE_PREVIEW):
	case(HOUSE_ONSETS):
	case(HOUSE_CUTGATE):	return do_cutgate(dz);

	case(HOUSE_TOPNTAIL):	return top_and_tail(dz);
	case(HOUSE_RECTIFY):	return do_rectify(dz);
	default:	   /*RWD*/
		break;
	}
	sprintf(errstr,"Unknown mode: process_clean()\n");
	return(PROGRAM_ERROR);
}

/*************************** CREATE_TOPNTAIL_BUFFER ************************/
//TW comment old-buffer-protocol not required

int create_topntail_buffer(dataptr dz)
{
	size_t bigbufsize;
	bigbufsize = (size_t) Malloc(-1);
	dz->buflen = (int)(bigbufsize / sizeof(float));
	dz->buflen = (dz->buflen / dz->infile->channels) * dz->infile->channels;
	if((dz->bigbuf = (float*) Malloc(dz->buflen * sizeof(float)))== NULL){
		sprintf(errstr, "Can't allocate memory for sound.\n");
		return(MEMORY_ERROR);
	}
	return(FINISHED);
}


/******************************* TOP_AND_TAIL *******************************/

int top_and_tail(dataptr dz)
{
	int  exit_status;
	float *ibuf = dz->bigbuf;	
	float *obuf = ibuf;
	int	 nbuff;
//	int  gotend   = FALSE;
//	int  gotstart = FALSE;
	int  chans = dz->infile->channels;
//	int  firsttime = TRUE;
	int startsamp = 0;
	int endsamp = dz->insams[0];
	int splicecnt   = round(dz->param[TOPN_SPLEN] * MS_TO_SECS * (double)dz->infile->srate);
	int splicesamps = splicecnt * chans;
	int sampseek = 0;
	int startsplice, endsplice = 0, samps_to_write;
//	int startseek;
	double gate, ngate;

	dz->ssampsread = 0;

	nbuff = dz->insams[0]/dz->buflen;
    if(nbuff * dz->buflen < dz->insams[0]) /* did have ; at end */
		nbuff++;		/* number of buffers contaning entire file */
	gate  = dz->param[TOPN_GATE] * (double)F_MAXSAMP;
	ngate = -gate;
	switch(dz->process) {
	case(TOPNTAIL_CLICKS):
		if(dz->vflag[STT_TRIM]) {
			if((exit_status = get_startsamp(ibuf,chans,nbuff,&startsplice,gate,ngate,dz))<0)
				return(exit_status);
			if((startsamp = startsplice + splicesamps) > dz->insams[0]) {
				sprintf(errstr,"At this gate level and splice length, entire file will be removed.\n");
				return(GOAL_FAILED);
			}
		} else
			startsplice = 0;	/* Needed for settting wrap, offset, seek */
		if(dz->vflag[END_TRIM]) {
			if((exit_status = get_endsamp(ibuf,chans,nbuff,&endsplice,gate,ngate,dz))<0)
				return(exit_status);
			if((endsamp = (endsplice - splicesamps)) < 0) {
				sprintf(errstr,"At this gate level and splice length, entire file will be removed.\n");
				return(GOAL_FAILED);
			}
		}
		if((dz->tempsize = endsplice - startsplice) <= 2 * splicesamps) {
			sprintf(errstr,"At this gate level and splice length, entire file will be removed.\n");
			return(GOAL_FAILED);
		}
		break;
	default:
		if(!dz->vflag[NO_STT_TRIM]) {
			if((exit_status = get_startsamp(ibuf,chans,nbuff,&startsamp,gate,ngate,dz))<0)
				return(exit_status);
		}
		if(!dz->vflag[NO_END_TRIM]) {
			if((exit_status = get_endsamp(ibuf,chans,nbuff,&endsamp,gate,ngate,dz))<0)
				return(exit_status);
		}
		if(endsamp == startsamp) {
			sprintf(errstr,"At this gate level, entire file will be removed.\n");
			return(GOAL_FAILED);
		}
		if((startsplice = startsamp - splicesamps) < 0)
			dz->vflag[NO_STT_TRIM] = TRUE;
		endsplice = endsamp;
		if((endsamp -= splicesamps) <= 0) {
			sprintf(errstr,"At this gate level, entire file will be removed.\n");
			return(GOAL_FAILED);
		}
		if(dz->vflag[NO_STT_TRIM])
			startsplice = 0;
		break;
	}
	
//	startseek = startsamp * sizeof(float);
// DEC 2004
	dz->ssampsread = 0;

	if(((dz->process == TOPNTAIL_CLICKS) && dz->vflag[STT_TRIM])
	|| ((dz->process != TOPNTAIL_CLICKS) && !dz->vflag[NO_STT_TRIM])) {
		if((exit_status = do_startsplice(splicecnt,startsamp,&sampseek,0,dz))<0)
			return(exit_status);
	} else { 
		if(sndseekEx(dz->ifd[0],startsamp,0)<0) {
			sprintf(errstr,"sndseekEx() failed: A\n");
			return(SYSTEM_ERROR);
		}
		sampseek = startsamp;
	}
	if(((dz->process == TOPNTAIL_CLICKS) && !dz->vflag[END_TRIM])
	|| ((dz->process != TOPNTAIL_CLICKS) && dz->vflag[NO_END_TRIM]))
		endsamp = endsplice = dz->insams[0];

	if((exit_status = advance_to_endsamp(endsamp,&sampseek,0,dz))<0)
		return(exit_status);

	samps_to_write = endsamp - sampseek;

	if(((dz->process == TOPNTAIL_CLICKS) && dz->vflag[END_TRIM]) 
	|| ((dz->process != TOPNTAIL_CLICKS) && !dz->vflag[NO_END_TRIM])) {
		if((exit_status = do_end_splice(&samps_to_write,splicecnt,0,dz))<0)
			return(exit_status);
	}
	if(samps_to_write > 0) {
		if((exit_status = write_samps(obuf,samps_to_write,dz))<0)
			return(exit_status);
	}
	dz->total_samps_written = endsplice - startsplice;
	return(FINISHED);
}

/******************************* GET_STARTSAMP *******************************/

int get_startsamp(float *ibuf,int chans,int nbuff,int *startsamp,double gate,double ngate,dataptr dz)
{
	int exit_status;
	int gotstart = FALSE;
	int thisbuff = 0;
	int n;
	while(!gotstart && thisbuff < nbuff) {
		if((exit_status = read_samps(ibuf,dz))<0)
			return(exit_status);
		for(n = 0;n < dz->ssampsread - 1;n++) {
			if(ibuf[n] > gate || ibuf[n] < ngate) {
				gotstart = TRUE;
				break;
			}
		}
		if(gotstart) {
			*startsamp  = n + (thisbuff * dz->buflen);
			*startsamp = (*startsamp/chans) * chans;	/* align to channel group boundary */
			break;
		}
		thisbuff++;
	}
	if(!gotstart) {
		sprintf(errstr,"Entire file is below gate level\n");
		return(GOAL_FAILED);
	}
	return(FINISHED);
}

/******************************* GET_ENDSAMP *******************************/
//RWD based on SECSIZE-aligned
//TW: No, just searching for last sample above gate
int get_endsamp(float *ibuf,int chans,int nbuff,int *endsamp,double gate,double ngate,dataptr dz)
{
	int exit_status;
	int n;
	int gotend = FALSE;
	int	thisbuff = nbuff - 1;	/* buffer that contains last sample */
	while(!gotend && thisbuff >= 0) {
		if(sndseekEx(dz->ifd[0],thisbuff * dz->buflen,0)<0) {
			sprintf(errstr,"sndseek() failed: 1\n");
			return(SYSTEM_ERROR);
		}
		if((exit_status = read_samps(ibuf,dz))<0)
			return(exit_status);
		for(n = dz->ssampsread - 1;n>=0;n--) {
			if(ibuf[n] > gate || ibuf[n] < ngate) {
				gotend = TRUE;
				break;
			}
		}
		if(gotend) {
			*endsamp = n + (thisbuff * dz->buflen);
			*endsamp = (*endsamp/chans) * chans;	/* align to channel group boundary */
			break;
		}
		thisbuff--;
	}
	if(!gotend) {
		sprintf(errstr,"Entire file is below gate level.\n");
		return(GOAL_FAILED);
	}
	return(FINISHED);
}

/******************************* DO_STARTSPLICE *******************************/

int do_startsplice(int splicecnt,int startsamp,int *sampseek,int input_report,dataptr dz)
{
	int exit_status;
	int chans = dz->infile->channels;
	int k, samps_written;
	double aincr, a1;
	int n;
	int m;

	if(sndseekEx(dz->ifd[0],startsamp,0)<0) {
		sprintf(errstr,"sndseek() failed: 3\n");
		return(SYSTEM_ERROR);
	}
// NEW TW June 2004
	*sampseek = startsamp;
	if((exit_status = read_samps(dz->bigbuf,dz))<0)	/* read buffer with additional sector */
		return(exit_status);
	k = 0;
	aincr = 1.0/(double)splicecnt;
	a1 = 0.0;
	for(n = 0;n < splicecnt;n++) {
		for(m = 0; m < chans; m++) {
			dz->bigbuf[k] = (float) ((double)dz->bigbuf[k] * a1);
			k++;
		}
		if(k >= dz->ssampsread) {
			if(input_report) {
				if((exit_status = write_samps_no_report(dz->bigbuf,dz->buflen,&samps_written,dz))<0)
					return(exit_status);
			} else {
				if((exit_status = write_samps(dz->bigbuf,dz->buflen,dz))<0)
					return(exit_status);
			}
			*sampseek += dz->ssampsread;
			if((exit_status = read_samps(dz->bigbuf,dz))<0)		/*RWD added * */
				return(exit_status);
			if(dz->ssampsread <=0)
				return(FINISHED);
			k = 0;
		}
		a1 += aincr;
	}
	return(FINISHED);
}

/******************************* ADVANCE_TO_ENDSAMP *******************************/

int advance_to_endsamp(int endsamp,int *sampseek,int input_report,dataptr dz)
{
	int exit_status;
	int samps_written;
	while(endsamp > *sampseek + dz->ssampsread) {
		if(dz->ssampsread > 0) {
			if(input_report) {
				if((exit_status = write_samps_no_report(dz->bigbuf,dz->ssampsread,&samps_written,dz))<0)
					return(exit_status);
			} else {
				if((exit_status = write_samps(dz->bigbuf,dz->ssampsread,dz))<0)
					return(exit_status);
			}
		}
		*sampseek += dz->ssampsread;
		if((exit_status = read_samps(dz->bigbuf,dz))<0)
			return(exit_status);
		if(dz->ssampsread ==0) {
			return FINISHED;
		}
	}
	return(FINISHED);
}

/******************************* DO_END_SPLICE *******************************/
/*k = nSAMPS*/
int do_end_splice(int *k,int splicecnt,int input_report,dataptr dz)
{
	int exit_status;
	int chans = dz->infile->channels;
	int n, samps_written;
	int m;
	double aincr = -(1.0/splicecnt);
	double a1 = 1.0 + aincr;
	for(n = 0;n < splicecnt;n++) {
		for(m = 0; m < chans; m++) {
			dz->bigbuf[*k] = (float) ((double)dz->bigbuf[*k] * a1);
			(*k)++;
		}
		if(*k >= dz->ssampsread) {
			if(dz->ssampsread > 0) {
				if(input_report) {
					if((exit_status = write_samps_no_report(dz->bigbuf,dz->ssampsread,&samps_written,dz))<0)
						return(exit_status);
				} else {
					if((exit_status = write_samps(dz->bigbuf,dz->ssampsread,dz))<0)
						return(exit_status);
				}
			}
			if((exit_status = read_samps(dz->bigbuf,dz))<0)
				return(exit_status);
			if(dz->ssampsread <=0) {
				return(FINISHED);
			}
			*k = 0;
		}
		a1 += aincr;
	}
	return(FINISHED);
}

/******************************* DO_CUTGATE ****************************/

int do_cutgate(dataptr dz)
{
	int exit_status;
	int n, ccnt = 0;
	int outsize, samps_written;
	char *thisfilename;
	double val;

	dz->sbufptr[ENV] = dz->sampbuf[ENV];

	fprintf(stdout,"INFO: Extracting envelope.\n");
	fflush(stdout);
	if((exit_status = ggetenv(dz))<0)
		return(exit_status);
	switch(dz->mode) {
	case(HOUSE_CUTGATE_PREVIEW):
		outsize = dz->iparam[CUTGATE_NUMSECS];
		if(outsize > 0) {
			if((exit_status = write_samps_no_report(dz->sampbuf[ENV],outsize,&samps_written,dz))<0)
				return(exit_status);
		}
		dz->infile->channels = 1;
		dz->infile->srate = SR_DEFAULT;
		break;
	case(HOUSE_CUTGATE):
		display_virtual_time(0,dz);
		dz->sbufptr[ENV] = dz->sampbuf[ENV];
		fprintf(stdout,"INFO: Finding cutpoints.\n");
		fflush(stdout);
		if(dz->iparam[CUTGATE_SUSTAIN]>0) {
			if((exit_status = findcuts2(&ccnt,dz))<0)
				return(exit_status);
		} else if((exit_status = findcuts(&ccnt,dz))<0)
			return(exit_status);
   		if(ccnt==0) {
			sprintf(errstr,"No segments to extract at this gatelevel.\n");
			return(GOAL_FAILED);
		}
		if((exit_status = setup_naming(&thisfilename,dz))<0)
			return(exit_status);
		fprintf(stdout,"INFO: Cutting file.\n");
		fflush(stdout);
// MULTICHAN -->
		dz->outchans = dz->infile->channels;
// <-- MULTICHAN
		if((exit_status = reset_peak_finder(dz))<0)
			return(exit_status);
		for(n=0;n<ccnt;n++) {
			if (ccnt > 9999) {
				fprintf(stdout,"WARNING: More than 9999 segments: Finishing here\n");
				fflush(stdout);
				free(thisfilename);
				return(FINISHED);
			}
			reset_filedata_counters(dz);
			if(sndseekEx(dz->ifd[0],0,0)<0) {
				sprintf(errstr,"sndseek() failed.\n");
				return(SYSTEM_ERROR);
			}
			if((exit_status = create_outfile_name(n,thisfilename,dz))<0)
				return(exit_status);
			if((exit_status = output_cut_sndfile(n,thisfilename,dz))<0)
				return(exit_status);
		}
		free(thisfilename);
		fprintf(stdout,"INFO: %d segments extracted.\n",ccnt);
		fflush(stdout);
		break;
//TW UPDATE : NEW CASE
	case(HOUSE_ONSETS):
		display_virtual_time(0,dz);
		dz->sbufptr[ENV] = dz->sampbuf[ENV];
		fprintf(stdout,"INFO: Finding cutpoints.\n");
		fflush(stdout);
		if((exit_status = find_onset_cuts(&ccnt,dz))<0)
			return(exit_status);
   		if(ccnt==0) {
			sprintf(errstr,"No segments to extract at this gatelevel.\n");
			return(GOAL_FAILED);
		}
		for(n=0;n<ccnt;n++) {
			val = (double)(dz->lparray[CUTGATE_STIME][n]/dz->infile->channels)/(double)dz->infile->srate;
			fprintf(dz->fp,"%.6lf\n",val);
		}
		fprintf(stdout,"INFO: %d segments extracted.\n",ccnt);
		fflush(stdout);
		break;
	}
	return(FINISHED);
}

/******************************** GGETENV ******************************/

int ggetenv(dataptr dz)
{   
	int exit_status;
	int n;
	int sampstoget;
	float *realend;
	display_virtual_time(0,dz);
	realend = dz->sampbuf[ENV] + dz->iparam[CUTGATE_NUMSECS];
		/* 1ST PASS : WHOLE BUFFERS */
	for(n = 0; n < dz->iparam[CUTGATE_NBUFS]; n++)	{
		if((exit_status = read_samps(dz->bigbuf,dz))<0)
			return(exit_status);
		readenv(dz->buflen,&(dz->sbufptr[ENV]),dz);
		display_virtual_time(dz->total_samps_read,dz);
	}
		/* 2ND PASS : WHOLE SHRTBLOKS */
	sampstoget = dz->iparam[CUTGATE_NSEC] * dz->iparam[CUTGATE_SAMPBLOK];
    if((dz->ssampsread = fgetfbufEx(dz->bigbuf, sampstoget,dz->ifd[0],0))!=sampstoget) {
		sprintf(errstr,"Sound read anomaly.\n");
		if(dz->ssampsread < 0)
			return(SYSTEM_ERROR);
		return(PROGRAM_ERROR);
	}
    readenv(sampstoget,&(dz->sbufptr[ENV]),dz);
    dz->sbufptr[ENVEND] = dz->sbufptr[ENV];			/* end of data */
    while(dz->sbufptr[ENV] < realend + dz->iparam[CUTGATE_WINDOWS]) {	/* pad end of buffer with zeroes */
		*dz->sbufptr[ENV] = 0;
		dz->sbufptr[ENV]++;
	}   
	dz->sbufptr[ENV] = dz->sampbuf[ENV];
	display_virtual_time(dz->total_samps_read,dz);
    return(FINISHED);
}

/****************************** SEARCHPARS ******************************/

void searchpars(dataptr dz)
{   
	int searchsize;

    dz->iparam[CUTGATE_NUMSECS] = dz->insams[0]/dz->iparam[CUTGATE_SAMPBLOK];
    searchsize = dz->iparam[CUTGATE_NUMSECS] * dz->iparam[CUTGATE_SAMPBLOK];
    dz->iparam[CUTGATE_NBUFS] = searchsize/dz->buflen;								/* no. of whole buffs to search */
    dz->iparam[CUTGATE_NSEC]  = (searchsize%dz->buflen)/dz->iparam[CUTGATE_SAMPBLOK]; /* no. further secs to search */
    return;	
}

/************************* READENV *******************************/

void readenv(int samps_to_process,float **env,dataptr dz)
{   int  d;
    for(d=0; d<samps_to_process; d+=dz->iparam[CUTGATE_SAMPBLOK]) {
    	get_maxsamp(*env,d,dz);
		(*env)++;
	}
}

/*************************** GET_MAXSAMP ******************************/

void get_maxsamp(float *env,int startsamp,dataptr dz)
{
	int  i, endsamp = startsamp + dz->iparam[CUTGATE_SAMPBLOK];
	double thisenv = 0;
	for(i = startsamp; i<endsamp; i++)
		thisenv =  max(thisenv,fabs(dz->bigbuf[i]));
	*env  = (float)min(thisenv,F_MAXSAMP);	/* avoids overflowing */
}

/**************************** FINDCUTS ********************************
 *
 * (1)	
 * (2)	If we are NOT inside a sound-to-keep.
 * (3)	  If the level is below the gate level..
 * (4)	    Ignore and continue..
 * (5)	  If the sound is above gate level..
 *	    mark this as (potential) start of a sound-to-keep
 * (6)	    flag sound-to-keep
 * (7)	    set the maximum level in segment to a minimum (0)
 * (8)	If we are INSIDE a sound-to-keep
 * (9)	  If the sound level drops below gate
 * (10)	    Check that 'dz->iparam[CUTGATE_WINDOWS]' succeeding segments are below gate level..
 * (11)	    If this is true,
 * (12)	      if the maximum level in sound-to-keep exceeded dz->iparam[CUTGATE_THRESH]..
 * 	        save the sound start and end times..
 * (13)       In ANY case, mark that we are NO LONGER in a sound-to-keep.
 *	    If it is NOT true, we are still INSIDE a sound-to-keep..
 * (14)	  If however, the sound is above the gate
 * (15)	    Check level of sound, and readjust maxlevel, if necessary..
 */

int findcuts(int *ccnt,dataptr dz)			  
{   
	int exit_status, tooshort_segs = 0, tooquiet_segs = 0;
	int startsec = 0, endsec;
	int	 inside_sound = FALSE, OK, n;
//	float maxlevel = 0, keepit = FALSE;
	float maxlevel = 0;
	float *bak, *endbak;
	int minlen = max(dz->iparam[CUTGATE_SPLEN] * 2,dz->iparam[CUTGATE_MINLEN]);
	double gate 	 = dz->param[CUTGATE_GATE] * (double)F_MAXSAMP;
	double endgate 	 = dz->param[CUTGATE_ENDGATE] * (double)F_MAXSAMP;
	double initlevel = dz->param[CUTGATE_INITLEVEL] * (double)F_MAXSAMP;
	double threshold = dz->param[CUTGATE_THRESH] * (double)F_MAXSAMP;
	
	while(dz->sbufptr[ENV]<dz->sbufptr[ENVEND]) {
		switch(inside_sound) {											/* 1 */
		case(FALSE):													/* 2 */
			if(*(dz->sbufptr[ENV]) <= gate)								/* 3 */
				break;													/* 4 */
	    	if(dz->iparam[CUTGATE_BAKTRAK]) {
				bak = dz->sbufptr[ENV];
				if((endbak = dz->sbufptr[ENV] - dz->iparam[CUTGATE_BAKTRAK]) < dz->sampbuf[ENV])
					endbak = dz->sampbuf[ENV];
				while(bak>endbak) {
					if(*(bak-1) >= initlevel)
						bak--;
					else
						break;
				}
				startsec = bak - dz->sampbuf[ENV];
			} else
		        startsec = dz->sbufptr[ENV] - dz->sampbuf[ENV];			/* 5 */
	    	inside_sound  = /*1*/TRUE;											/* 6 */	 /*RWD was 1 */
	    	maxlevel = *(dz->sbufptr[ENV]);								/* 7 */
	    	break;
		case(TRUE):														/* 8 */
			if(*(dz->sbufptr[ENV]) <= endgate) {						/* 9 */
				OK = TRUE;
				if(dz->iparam[CUTGATE_WINDOWS]>0) {
					for(n=1;n<dz->iparam[CUTGATE_WINDOWS];n++) {		/* 10 */
						if(*(dz->sbufptr[ENV]+n)>endgate) {
							OK = FALSE;
							break;
						}
					}
				}
				if(OK) {												/* 11 */
					endsec = dz->sbufptr[ENV] - dz->sampbuf[ENV];
					if((endsec - startsec) * dz->iparam[CUTGATE_SAMPBLOK] > minlen) {
						if(maxlevel >= threshold) {	/* 12 */
							if((exit_status = store_cut_positions_in_samples(*ccnt,startsec,endsec,dz))<0)
								return(exit_status);
							(*ccnt)++;
						} else {
							tooquiet_segs++;
						}
					} else {
						tooshort_segs++;
					}
					inside_sound = FALSE;								/* 13 */
				}
			} else {													/* 14 */
				if(*(dz->sbufptr[ENV]) > maxlevel)						/* 15 */
					maxlevel = *(dz->sbufptr[ENV]);
			}
			break;
		}
		(dz->sbufptr[ENV])++;
	}
	if(inside_sound) {				/* Ensure any significant block at very end is grabbed */
		endsec = dz->sbufptr[ENV] - dz->sampbuf[ENV];
		if((endsec - startsec) * dz->iparam[CUTGATE_SAMPBLOK] > minlen) {
			if(maxlevel >= threshold) {	/* 12 */
				if((exit_status = store_cut_positions_in_samples(*ccnt,startsec,endsec,dz))<0)
					return(exit_status);
				(*ccnt)++;
			} else {
				tooquiet_segs++;
			}
		} else {
			tooshort_segs++;
		}
	}
	if(*ccnt <= 0) {
		if(tooshort_segs || tooquiet_segs) {
			if(tooshort_segs && tooquiet_segs)
				sprintf(errstr,"%d segments shorter than min duration set (or twice splicelen), and %d too quiet.\n",tooshort_segs,tooquiet_segs);
			else if(tooquiet_segs)
				sprintf(errstr,"%d segments to extract are too quiet (below threshold you set).\n",tooquiet_segs);
			else
				sprintf(errstr,"%d segments are shorter than minimum duration you set, or than twice splicelen.\n",tooshort_segs);
		} else {
			sprintf(errstr,"Whole file is below the gate level.\n");
		}
		return(GOAL_FAILED);
	}
	return(FINISHED);
}

/******************************* FINDCUTS2 ******************************/

int findcuts2(int *ccnt,dataptr dz)
{   
	int exit_status, tooshort_segs = 0;
	int oldstartsec = 0, startsec, endsec;
	int	 inside_sound = FALSE, OK, n;
	/*short maxlevel = 0, keepit = FALSE, *bak, *endbak;*/
//TW UPDATE : prevents WARNING
	int keepit = FALSE;
	float maxlevel = 0.0f, *bak, *endbak;
	int minlen = max(dz->iparam[CUTGATE_SPLEN] * 2,dz->iparam[CUTGATE_MINLEN]);
	double gate 	 = dz->param[CUTGATE_GATE] * (double)F_MAXSAMP;
	double endgate 	 = dz->param[CUTGATE_ENDGATE] * (double)F_MAXSAMP;
	double threshold = dz->param[CUTGATE_THRESH] * (double)F_MAXSAMP;
	double initlevel = dz->param[CUTGATE_INITLEVEL] * (double)F_MAXSAMP;

	while(dz->sbufptr[ENV]<dz->sbufptr[ENVEND]) {
		switch(inside_sound) {					/* 1 */
		case(FALSE):						/* 2 */
			if(*(dz->sbufptr[ENV]) <= gate)				/* 3 */
				break;					/* 4 */
			if(dz->iparam[CUTGATE_BAKTRAK]) {
				bak = dz->sbufptr[ENV];
				if((endbak = dz->sbufptr[ENV] - dz->iparam[CUTGATE_BAKTRAK]) < dz->sampbuf[ENV])
					endbak = dz->sampbuf[ENV];
				while(bak>endbak) {
					if(*(bak-1) >= initlevel)
						bak--;
					else
						break;
				}
    	        startsec = bak - dz->sampbuf[ENV];
			} else
				startsec = dz->sbufptr[ENV] - dz->sampbuf[ENV];			/* 5 */
			if(keepit && ((endsec = startsec - dz->iparam[CUTGATE_SUSTAIN]) > oldstartsec)) {
				if((endsec - oldstartsec) * dz->iparam[CUTGATE_SAMPBLOK] > minlen) {
					if((exit_status = 
					store_cut_positions_in_samples(*ccnt,oldstartsec,startsec - dz->iparam[CUTGATE_SUSTAIN],dz))<0)
						return(exit_status);
					(*ccnt)++;
				} else {
					tooshort_segs = 1;
				}
			}
			keepit = FALSE;
			oldstartsec = startsec;
			inside_sound  = /*1*/TRUE;											/* 6 */
			maxlevel = *(dz->sbufptr[ENV]);								/* 7 */
			break;
		case(TRUE):														/* 8 */
			if(*(dz->sbufptr[ENV]) <= endgate) {						/* 9 */
				OK = TRUE;
				if(dz->iparam[CUTGATE_WINDOWS]>0) {
					for(n=1;n<dz->iparam[CUTGATE_WINDOWS];n++) {		/* 10 */
						if(*(dz->sbufptr[ENV]+n)>gate) {
							OK = FALSE;
							break;
						}
					}
				}
				if(OK) {												/* 11 */
					if(maxlevel > threshold)							/* 12 */
						keepit = TRUE;
					else 
						keepit = FALSE;
					inside_sound = FALSE;								/* 13 */
				}
			} else {													/* 14 */
				if(*(dz->sbufptr[ENV]) > maxlevel)						/* 15 */
					maxlevel = *(dz->sbufptr[ENV]);
			}
			break;
		}
		(dz->sbufptr[ENV])++;
    }
	endsec = dz->sbufptr[ENVEND] - dz->sampbuf[ENV];
    if(keepit && ((endsec - oldstartsec) * dz->iparam[CUTGATE_SAMPBLOK]) > minlen) {
		if(maxlevel >= threshold) {	/* 12 */
			if((exit_status = store_cut_positions_in_samples(*ccnt,oldstartsec,endsec,dz))<0)
				return(exit_status);
		}
		(*ccnt)++;
	}
	if((*ccnt <= 0) && tooshort_segs) {
		sprintf(errstr,"Segments to extract are shorter than minimum duration you set, or than twice splicelen.\n");
		return(GOAL_FAILED);
	}
	return(FINISHED);
}

/******************************* SETUP_NAMING ******************************/

int setup_naming(char **thisfilename,dataptr dz)
{
	int innamelen = strlen(dz->wordstor[0]);
	int numlen = 5;
	if((*thisfilename = (char *)malloc((innamelen + numlen + 1) * sizeof(char)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for outfilename.\n");
		return(MEMORY_ERROR);
	}
	return(FINISHED);
}

/********************************** CREATE_OUTFILE_NAME ********************************/

int create_outfile_name(int ccnt,char *thisfilename,dataptr dz)
{
//TW REVISED Dec 2002
	strcpy(thisfilename,dz->wordstor[0]);
	insert_new_number_at_filename_end(thisfilename,ccnt,1);
	return(FINISHED);
}

/********************************** OUTPUT_CUT_SNDFILE ********************************/

int output_cut_sndfile(int ccnt,char *filename,dataptr dz)
{
	int exit_status;
//	int  firsttime = TRUE;
	int sampseek, startsamp, endsamp, k, samps_to_write, samps_written;

	dz->process_type = UNEQUAL_SNDFILE;	/* allow sndfile to be created */
	if((exit_status = create_sized_outfile(filename,dz))<0) {
		fprintf(stdout, "WARNING: Soundfile %s already exists.\n", filename);
		fflush(stdout);
		dz->process_type = OTHER_PROCESS;
		dz->ofd = -1;
		if(dz->vflag[STOP_ON_SAMENAME])
			return(GOAL_FAILED);
	 	return(FINISHED);
	}							
	startsamp     = dz->lparray[CUTGATE_STIME][ccnt];
	sampseek      = startsamp;

	if((exit_status= do_startsplice(dz->iparam[CUTGATE_SPLCNT],startsamp,&sampseek,1,dz))<0)
		return(exit_status);
	endsamp = dz->lparray[CUTGATE_ETIME][ccnt] - dz->iparam[CUTGATE_SPLEN];
	if((exit_status = advance_to_endsamp(endsamp,&sampseek,1,dz))<0)
		return(exit_status);
	k = endsamp - sampseek;
	if((exit_status = do_end_splice(&k,dz->iparam[CUTGATE_SPLCNT],1,dz))<0)
		return(exit_status);
	if((samps_to_write = k)>0) {
		if((exit_status = write_samps_no_report(dz->bigbuf,samps_to_write,&samps_written,dz))<0)
			return(exit_status);
	}
	display_virtual_time(startsamp + dz->total_samps_read,dz);
	dz->outfiletype  = SNDFILE_OUT;			/* allows header to be written  */

	if((exit_status = headwrite(dz->ofd,dz))<0)
		return(exit_status);
	dz->process_type = OTHER_PROCESS;		/* restore true status */
	dz->outfiletype  = NO_OUTPUTFILE;		/* restore true status */
	if((exit_status = reset_peak_finder(dz))<0)
		return(exit_status);
	if(sndcloseEx(dz->ofd) < 0) {
		fprintf(stdout,"WARNING: Can't close output soundfile %s\n",filename);
		fflush(stdout);
	}
	dz->ofd = -1;
	return(FINISHED);
}

/********************************** STORE_CUT_POSITIONS_IN_SAMPLES ********************************/

int store_cut_positions_in_samples(int ccnt,int startsec,int endsec,dataptr dz) 
{
	if(ccnt==0) {
		if((dz->lparray[CUTGATE_STIME] = (int *)malloc(sizeof(int)))==NULL
		|| (dz->lparray[CUTGATE_ETIME] = (int *)malloc(sizeof(int)))==NULL) {
			sprintf(errstr,"INSUFFICENT MEMORY to store cut times.\n");
			return(MEMORY_ERROR);
		}
	} else {
		if((dz->lparray[CUTGATE_STIME] = 
		(int *)realloc(dz->lparray[CUTGATE_STIME],(ccnt+1) * sizeof(int)))==NULL
		|| (dz->lparray[CUTGATE_ETIME] = 
		(int *)realloc(dz->lparray[CUTGATE_ETIME],(ccnt+1) * sizeof(int)))==NULL) {
			sprintf(errstr,"INSUFFICENT MEMORY to store more cut times.\n");
			return(MEMORY_ERROR);
		}
	}
	dz->lparray[CUTGATE_STIME][ccnt] = startsec * dz->iparam[CUTGATE_SAMPBLOK];
	dz->lparray[CUTGATE_ETIME][ccnt] = endsec   * dz->iparam[CUTGATE_SAMPBLOK];
	return(FINISHED);
}

/********************************** DO_RECTIFY ********************************/

int do_rectify(dataptr dz)
{
	int exit_status;
	int n;
	float *buf = dz->sampbuf[0], maxsamp, minsamp;
	double rectify_shift = dz->param[RECTIFY_SHIFT] * (double)F_MAXSAMP;
	if(flteq(rectify_shift,0.0)) {
		sprintf(errstr,"NO CHANGE to original sound file.\n");
		return(GOAL_FAILED);
	}
	if(rectify_shift > 0.0) {
		fprintf(stdout,"INFO: Finding maximum sample in file.\n");
		fflush(stdout);
		maxsamp = F_MINSAMP;
		while(dz->samps_left > 0) {
			if((exit_status = read_samps(buf,dz))<0)
				return(exit_status);
			for(n= 0;n<dz->ssampsread;n++)
				maxsamp = max(maxsamp,buf[n]);
		}
		if(maxsamp + rectify_shift > (double)F_MAXSAMP) {
			sprintf(errstr,"This rectification will distort the sound.\n");
			return(GOAL_FAILED);
		}
	} else {
		fprintf(stdout,"INFO: Finding minimum sample in file.\n");
		fflush(stdout);
//		minsamp = MAXSAMP;
		minsamp = F_MAXSAMP;
		while(dz->samps_left > 0) {
			if((exit_status = read_samps(buf,dz))<0)
				return(exit_status);
			for(n= 0;n<dz->ssampsread;n++)
				minsamp = min(minsamp,buf[n]);
		}
		if(minsamp + rectify_shift < (double)F_MINSAMP) {
			sprintf(errstr,"This rectification will distort the sound.\n");
			return(GOAL_FAILED);
		}
	}
	reset_filedata_counters(dz);
	if(sndseekEx(dz->ifd[0],0,0)<0) {
		sprintf(errstr,"sndseekEx failed.\n");
		return(SYSTEM_ERROR);
	}
	fprintf(stdout,"INFO: Rectifying.\n");
	fflush(stdout);
	while(dz->samps_left > 0) {
		if((exit_status = read_samps(buf,dz))<0)
			return(exit_status);
		for(n= 0;n<dz->ssampsread;n++)
			buf[n] = (float)(buf[n] + rectify_shift);
		if(dz->ssampsread > 0) {
			if((exit_status = write_exact_samps(buf,dz->ssampsread,dz))<0)
				return(exit_status);
		}
	}
	return(FINISHED);
}

//TW FUNCTION NOW REDUNDANT
/********************************** DO_BY_HAND ********************************/
//
//int do_by_hand(dataptr dz)
//{
//	int n = 0, all_fixes_done = 0, exit_status;
//	int k, last_total_ssampsread = 0;
//	double *la = dz->parray[0];
//	float *buf = dz->sampbuf[0];
//	
//	while(dz->samps_left > 0) {
//		if((exit_status = read_samps(buf,dz))<0)
//			return(exit_status);
//		if(!all_fixes_done) {
//			while(dz->total_samps_read > *la) {
//				k = round(*la++) - last_total_ssampsread;
/* NOT SHORT, IF BUFS NOT SHORT : 2000 */
//				buf[k] = (float)(*la++);
//				if(la >= dz->parray[1]) {
//					all_fixes_done = 1;
//					break;
//				}
//			}
//			last_total_ssampsread = dz->total_samps_read; 
//		}
//		if((exit_status = write_samps(buf,dz->ssampsread,dz))<0)
//			return(exit_status);
//	}
//	return(FINISHED);
//}	
//

//TW UPDATE: NEW FUNCTIONS BELOW (updated for flotsams)

/**************************** FIND_ONSET_CUTS *********************************/

int find_onset_cuts(int *ccnt,dataptr dz)			  
{   
	int exit_status, tooshort_segs = 0, tooquiet_segs = 0;
	int startsec = 0, endsec;
	int	 inside_sound = FALSE, OK, n = 0;
//	short keepit = FALSE;
	float maxlevel = 0;
	float *bak, *endbak, *minpos, *here, minval;
	int minlen 	 = dz->iparam[CUTGATE_MINLEN];
	double gate 	 = dz->param[CUTGATE_GATE] * (double)F_MAXSAMP;
	double endgate 	 = dz->param[CUTGATE_ENDGATE] * (double)F_MAXSAMP;
	double initlevel = dz->param[CUTGATE_INITLEVEL] * (double)F_MAXSAMP;
	double threshold = dz->param[CUTGATE_THRESH] * (double)F_MAXSAMP;
	
	int envlen = dz->sbufptr[ENVEND] - dz->sbufptr[ENV];
	int ratio  = dz->insams[0]/envlen;
	int envstep = envlen/10, envmark = envstep;

	while(dz->sbufptr[ENV]<dz->sbufptr[ENVEND]) {
		switch(inside_sound) {											/* 1 */
		case(FALSE):													/* 2 */
			if(*(dz->sbufptr[ENV]) <= gate)								/* 3 */
				break;													/* 4 */
	    	if(dz->iparam[CUTGATE_BAKTRAK]) {
				bak = dz->sbufptr[ENV];
				if((endbak = dz->sbufptr[ENV] - dz->iparam[CUTGATE_BAKTRAK]) < dz->sampbuf[ENV])
					endbak = dz->sampbuf[ENV];
				while(bak>endbak) {
					if(*(bak-1) > endgate) {
						minval = *bak;
						minpos = bak;
						for(here = bak+1;here <= dz->sbufptr[ENV];here++) {
							if(*here < minval) {
								minval = *here;
								minpos = here;
							}
						}
						bak = minpos;
						break;
					} else if(*(bak-1) >= initlevel)
						bak--;
					else
						break;
				}
				startsec = bak - dz->sampbuf[ENV];
			} else
		        startsec = dz->sbufptr[ENV] - dz->sampbuf[ENV];			/* 5 */
	    	inside_sound  = 1;											/* 6 */
	    	maxlevel = *(dz->sbufptr[ENV]);								/* 7 */
	    	break;
		case(TRUE):														/* 8 */
			if(*(dz->sbufptr[ENV]) <= endgate) {						/* 9 */
				OK = TRUE;
				if(dz->iparam[CUTGATE_WINDOWS]>0) {
					for(n=1;n<dz->iparam[CUTGATE_WINDOWS];n++) {		/* 10 */
						if(*(dz->sbufptr[ENV]+n)>endgate) {
							OK = FALSE;
							break;
						}
					}
				}
				if(OK) {												/* 11 */
					endsec = dz->sbufptr[ENV] - dz->sampbuf[ENV];
					if((endsec - startsec) * dz->iparam[CUTGATE_SAMPBLOK] > minlen) {
						if(maxlevel >= threshold) {							/* 12 */
							if((exit_status = store_startcut_positions_in_samples(*ccnt,startsec,dz))<0)
								return(exit_status);
							(*ccnt)++;
						} else {
							tooquiet_segs = 1;
						}
					} else {
						tooshort_segs = 1;
					}
					inside_sound = FALSE;									/* 13 */
				}
			} else {													/* 14 */
				if(*(dz->sbufptr[ENV]) > maxlevel)						/* 15 */
					maxlevel = *(dz->sbufptr[ENV]);
			}
			break;
		}
		(dz->sbufptr[ENV])++;
		if(++n > envmark) {
			if(sloom) {
				envmark += envstep;
				display_virtual_time(n * ratio,dz);			 
			}
		}
	}
	if(inside_sound) {				/* Ensure any significant block at very end is grabbed */
		endsec = dz->sbufptr[ENV] - dz->sampbuf[ENV];
		if((endsec - startsec) * dz->iparam[CUTGATE_SAMPBLOK] > minlen) {
			if(maxlevel >= threshold) {	/* 12 */
				if((exit_status = store_cut_positions_in_samples(*ccnt,startsec,endsec,dz))<0)
					return(exit_status);
				(*ccnt)++;
			} else {
				tooquiet_segs++;
			}
		} else {
			tooshort_segs++;
		}
	}
	if(*ccnt <= 0) {
		if(tooshort_segs || tooquiet_segs) {
			if(tooshort_segs && tooquiet_segs)
				sprintf(errstr,"%d segments shorter than min duration set (or twice splicelen), and %d too quiet.\n",tooshort_segs,tooquiet_segs);
			else if(tooquiet_segs)
				sprintf(errstr,"%d segments to extract are too quiet (below threshold you set).\n",tooquiet_segs);
			else
				sprintf(errstr,"%d segments are shorter than minimum duration you set, or than twice splicelen.\n",tooshort_segs);
		} else {
			sprintf(errstr,"Whole file is below the gate level.\n");
		}
		return(GOAL_FAILED);
	}
	return(FINISHED);
}

/********************************** STORE_STARTCUT_POSITIONS_IN_SAMPLES ********************************/

int store_startcut_positions_in_samples(int ccnt,int startsec,dataptr dz) 
{
	if(ccnt==0) {
		if((dz->lparray[CUTGATE_STIME] = (int *)malloc(sizeof(int)))==NULL) {
			sprintf(errstr,"INSUFFICENT MEMORY to store cut times.\n");
			return(MEMORY_ERROR);
		}
	} else {
		if((dz->lparray[CUTGATE_STIME] = 
		(int *)realloc(dz->lparray[CUTGATE_STIME],(ccnt+1) * sizeof(int)))==NULL) {
			sprintf(errstr,"INSUFFICENT MEMORY to store more cut times.\n");
			return(MEMORY_ERROR);
		}
	}
	dz->lparray[CUTGATE_STIME][ccnt] = startsec * dz->iparam[CUTGATE_SAMPBLOK];
	return(FINISHED);
}

/********************************** HOUSE_GATE ********************************/

int house_gate(dataptr dz) 
{
	int exit_status;
	int gotfirst = 0;
//	int zerosgot = 0, finished = 0, cutcount = 0, chans = dz->infile->channels;
	int zerosgot = 0, finished = 0, chans = dz->infile->channels;
	int startsamp = 0, arraysize = BIGARRAY, n, i, j;
//	int samps_to_get = dz->buflen, samps_to_write;
	int samps_to_write;
	int *cutpoint;
//	int total_samps_read = 0, cutcnt = 0, secseek;
	int total_samps_read = 0, cutcnt = 0;
	char *thisfilename;
	int endsamp;
	if((cutpoint = (int *)malloc(arraysize * sizeof(int)))==NULL) {
		sprintf(errstr,"Insufficient memory to store cutting points.\n");
		return(MEMORY_ERROR);
	}
	display_virtual_time(0,dz);
	fprintf(stdout,"INFO: Finding cutpoints.\n");
	fflush(stdout);

	if((exit_status = read_samps(dz->sampbuf[1],dz)) < 0) {
		sprintf(errstr,"Sound read anomaly.\n");
		return(SYSTEM_ERROR);
	}
	total_samps_read += dz->ssampsread;
	display_virtual_time(total_samps_read,dz);
	n = dz->buflen;
	do {
		memcpy((char *)dz->sampbuf[0],(char *)dz->sampbuf[1],dz->buflen * sizeof(float));
		if(dz->ssampsread != dz->buflen)
			finished = 1;
		if(!finished) {
			if((exit_status = read_samps(dz->sampbuf[1],dz)) < 0) {
				sprintf(errstr,"Sound read anomaly.\n");
				return(SYSTEM_ERROR);
			}
			if(dz->ssampsread != dz->buflen) {
				total_samps_read += dz->ssampsread;
				display_virtual_time(total_samps_read,dz);
			}
		}
		n -= dz->buflen;
		if(!finished) {
			endsamp = max(dz->ssampsread,(dz->iparam[GATE_ZEROS] * chans));
			endsamp += dz->buflen;
		} else
			endsamp = dz->ssampsread;
		
		while(n<endsamp) {
			if(smpflteq(dz->sampbuf[0][n],0.0)) {
				if(!gotfirst) {
					n += chans;
					continue;
				}
				zerosgot++;
				if(zerosgot > dz->iparam[GATE_ZEROS]) {
					n += chans;
					continue;
				}
				else if(zerosgot == dz->iparam[GATE_ZEROS]) {
					cutpoint[cutcnt++] = (startsamp + n  - ((dz->iparam[GATE_ZEROS]-1) * chans));
				 	if(cutcnt >= arraysize) {
						arraysize += BIGARRAY;
						if((cutpoint = (int *)realloc((char *)cutpoint,arraysize * sizeof(int)))==NULL) {
							sprintf(errstr,"Insufficient memory to store cutting points.\n");
							return(MEMORY_ERROR);
						}
					}
				}
			} else {
				if(!gotfirst) {
					cutpoint[cutcnt++] = (startsamp + n);
					gotfirst = 1;
					n += chans;
					continue;
				} else {
					if(zerosgot >= dz->iparam[GATE_ZEROS]) {
						cutpoint[cutcnt++] = (startsamp + n);
					 	if(cutcnt >= arraysize) {
							arraysize += BIGARRAY;
							if((cutpoint = (int *)realloc((char *)cutpoint,arraysize * sizeof(int)))==NULL) {
								sprintf(errstr,"Insufficient memory to store cutting points.\n");
								return(MEMORY_ERROR);
							}
						}
					}
					zerosgot = 0;
				}
			}
			n += chans;
		}
		startsamp += dz->buflen;
	} while(!finished);
	if(gotfirst) {
		if(zerosgot < dz->iparam[GATE_ZEROS])
			cutpoint[cutcnt++] = dz->insams[0];
	} else {
		sprintf(errstr,"No silent gaps found: no cuts made.\n");
		return(GOAL_FAILED);
	}
	if(!EVEN(cutcnt)) {
		sprintf(errstr,"cut points not paired correctly.\n");
		return(PROGRAM_ERROR);
	}
	cutcnt /= 2;
	if((exit_status = setup_naming(&thisfilename,dz))<0)
		return(exit_status);
	fprintf(stdout,"INFO: Cutting file.\n");
	fflush(stdout);
// MULTICHAN -->
	dz->outchans = dz->infile->channels;
// <--  MULTICHAN
	if((exit_status = reset_peak_finder(dz))<0)
		return(exit_status);
	display_virtual_time(0,dz);
	for(n = 0,i= 0,j=1; n < cutcnt; n++,i+=2,j+=2) {
		if (n > 1999) {
			fprintf(stdout,"WARNING: More than 9999 segments: Finishing here\n");
			fflush(stdout);
			free(thisfilename);
			return(FINISHED);
		}
		reset_filedata_counters(dz);
		if((exit_status = create_outfile_name(n,thisfilename,dz))<0)
			return(exit_status);
		dz->process_type = UNEQUAL_SNDFILE;	/* allow sndfile to be created */
		if((exit_status = create_sized_outfile(thisfilename,dz))<0) {
			fprintf(stdout, "WARNING: Soundfile %s already exists.\n", thisfilename);
			fflush(stdout);
			dz->process_type = OTHER_PROCESS;
			dz->ofd = -1;
		 	return(FINISHED);
		}							
		sndseekEx(dz->ifd[0],cutpoint[i],0);
		display_virtual_time(cutpoint[i],dz);
		if((exit_status = read_samps(dz->sampbuf[0],dz)) < 0) {
			sprintf(errstr,"Sound read anomaly.\n");
			return(SYSTEM_ERROR);
		}
		samps_to_write = cutpoint[j] - cutpoint[i];
		
		while(samps_to_write > dz->buflen) {
			if((exit_status = write_samps(dz->sampbuf[0],dz->buflen,dz))<0)
				return(exit_status);
			samps_to_write -= dz->buflen;
			if((exit_status = read_samps(dz->sampbuf[0],dz)) < 0) {
				sprintf(errstr,"Sound read anomaly.\n");
				return(SYSTEM_ERROR);
			}
		}
		if(samps_to_write) {
			if((exit_status = write_samps(dz->sampbuf[0],samps_to_write,dz))<0)
				return(exit_status);
		}
		dz->outfiletype  = SNDFILE_OUT;			/* allows header to be written  */
		if((exit_status = headwrite(dz->ofd,dz))<0)
			return(exit_status);
		dz->process_type = OTHER_PROCESS;		/* restore true status */
		dz->outfiletype  = NO_OUTPUTFILE;		/* restore true status */
		if((exit_status = reset_peak_finder(dz))<0)
			return(exit_status);
		if(sndcloseEx(dz->ofd) < 0) {
			fprintf(stdout,"WARNING: Can't close output soundfile %s\n",thisfilename);
			fflush(stdout);
		}
		dz->ofd = -1;
	}
	display_virtual_time(dz->insams[0],dz);
	free(thisfilename);
	fprintf(stdout,"INFO: %d segments extracted.\n",cutcnt);
	fflush(stdout);
	return(FINISHED);
}

/********************************** HOUSE_GATE2 ********************************/

#define NOTHING_FOUND			0
#define INTERGLITCH_FOUND		1
#define MEASURING_GLITCH		2
#define MEASURING_INTERGLITCH	3

int house_gate2(dataptr dz) 
{
	int  exit_status;
	float *ibuf = dz->sampbuf[0], *filtbuf, *maxsamp = NULL;
	int n, k, f, cnt, z = 0, possible_start = 0;
	int splicelen  = dz->iparam[GATE2_SPLEN];
	double spliceratio, spliceincr = 1.0/(double)splicelen, sum, sum2, thissamp;
	int m, j, gotzero, search_state = 0, chans = dz->infile->channels;
	int nonzerosgot = 0, zerosgot = 0,*pos; 
	int endcut = 0, startcut = 0, cutcnt, last_total_samps_read, endsplice, startsplice;
	int filtlen = dz->iparam[GATE2_FILT] * chans, filt_index;

	k = (int)ceil((double)dz->insams[0]/(double)(dz->iparam[GATE2_ZEROS] * chans));
	k += 2;
	k *= 2;
	if(k < 0 || ((dz->lparray[0] = (int *)malloc(k * sizeof(int)))==NULL)) {
		sprintf(errstr,"Insufficient memory to store glitch positions.\n");
		return(MEMORY_ERROR);
	}
	if((filtbuf = (float *)malloc(filtlen * sizeof(float)))==NULL) {
		sprintf(errstr,"Insufficient memory to create filter buffer.\n");
		return(MEMORY_ERROR);
	}
	if(dz->vflag[0]) {
		if((maxsamp = (float *)malloc((k/2) * sizeof(float)))==NULL) {
			sprintf(errstr,"No memory to store glitch maxima.\n");
			return(MEMORY_ERROR);
		}
		memset((char *)maxsamp,0,(k/2) * sizeof(float));
	}
	memset((char *)filtbuf,0,filtlen * sizeof(float));
	pos = dz->lparray[0];
	k = 0;
	fprintf(stdout,"INFO: Searching for glitches.\n");
	fflush(stdout);
	if((exit_status = read_samps(ibuf,dz))<0)
		return(exit_status);
	last_total_samps_read = 0;
	sum = 0;
	filt_index = 0;
	cnt = 0;
	while(dz->ssampsread > 0) {
		n = 0;
		while(n<dz->ssampsread) {
			cnt += chans;
			gotzero = 0;
			thissamp = 0.0;
			for(m=0;m < chans ;m++) {				/* add samples cyclically to a buffer */
				filtbuf[filt_index++] = ibuf[n+m];
				thissamp += fabs(ibuf[n+m]);
			}
			filt_index %= filtlen;					/* cycle around buffer */

			thissamp /= (double)chans;				/* average abs size of samples in all channels at this point */
			cnt = min(cnt,filtlen);
			sum = 0.0;								/* sum over filt buffer len, or total no of samps read, whichever is smaller */
			for(f = 0;f < cnt;f++)
				sum += fabs(filtbuf[f]);
			sum /= (double)cnt;						/* find average abs samp val over last 'filtlen' samps */

			if(sum <= dz->param[GATE2_LEVEL])
				gotzero = 1;
			if(thissamp > dz->param[GATE2_LEVEL]) {	/* note abs level of this sample */
				possible_start = n + last_total_samps_read;
				gotzero = 0;
			}
			if(gotzero) {
				nonzerosgot = 0;
				zerosgot++;
				switch(search_state) {
				case(NOTHING_FOUND):			/* not yet enough pre-glitch sub-threshold signal */
					if(zerosgot >= dz->iparam[GATE2_ZEROS])
						search_state = INTERGLITCH_FOUND;
					break;
				case(INTERGLITCH_FOUND):		/* already got enough pre-glitch sub-threshold signal */
					break;
				case(MEASURING_GLITCH):			/* found glitch: mark end, prepare to measure post-glitch */
					endcut = last_total_samps_read + n;
					search_state = MEASURING_INTERGLITCH;
					break;
				case(MEASURING_INTERGLITCH):	/* waiting for enough post-glitch, sub-threshold signal */
					if(zerosgot >= dz->iparam[GATE2_ZEROS]) {
						pos[k++] = startcut;
						pos[k++] = endcut;
						z++;
						search_state = INTERGLITCH_FOUND;
					}							/* if enough found, also enough preglitch for next glitch */
					break;
				}
			} else {
				nonzerosgot++;
				zerosgot = 0;
				switch(search_state) {
				case(NOTHING_FOUND):			/* not yet enough pre-glitch sub-threshold signal */
					break;
				case(INTERGLITCH_FOUND):		/* enough signal before threshold, mark start of excision */
					startcut = possible_start;
					if(dz->vflag[0]) {
						f = filt_index - chans;
						maxsamp[z] = 0.0f;
						for(j = n + last_total_samps_read; j >= startcut; j-=chans, f -= chans) {
							if(f < 0)			/* use filtbuf to find maxsamp between here & possible_start */
								f += filtlen;
							thissamp = 0.0;
							for(m=0;m < chans ;m++)
								thissamp += fabs(filtbuf[f+m]);
							thissamp /= (double)chans;
							maxsamp[z] = (float)max(thissamp,(double)maxsamp[z]);
						}
					}
					nonzerosgot += (n + last_total_samps_read - possible_start);
					search_state = MEASURING_GLITCH;
					break;
				case(MEASURING_GLITCH):			/* measuring possible glitch: if too long, abandon */
					if(dz->vflag[0])
						maxsamp[z] = (float)max((double)maxsamp[z],thissamp);
					if(nonzerosgot > dz->iparam[GATE2_DUR])
						search_state = NOTHING_FOUND;
					break;
				case(MEASURING_INTERGLITCH):	/* was waiting for enough postglitch 'ZEROS' but not enough */
					search_state = NOTHING_FOUND;
					break;
				}
			}
			n += chans;
		}
		last_total_samps_read += dz->ssampsread;
		if((exit_status = read_samps(ibuf,dz))<0)
			return(exit_status);
	}
	if(search_state == 2) {
		pos[k++] = startcut;
		pos[k++] = dz->insams[0];
	}
	cutcnt = k;
	fprintf(stdout,"INFO: glitches found = %d\n",k/2);
	fflush(stdout);
	if(dz->vflag[0]) {
		for(n=0,z=0;n<cutcnt;n+=2,z++) {
			k = (pos[n+1] - pos[n])/chans;
			sum  = (double)(k * SECS_TO_MS)/(double)dz->infile->srate;
			k = pos[n]/chans;
			sum2 = (double)k/(double)dz->infile->srate;
			fprintf(stdout,"INFO: GLITCH length %.2lf ms : val %f : at (grouped)sample %d : time %lf secs\n",sum,maxsamp[z],k,sum2);
		}
		fflush(stdout);
	}
	if(cutcnt == 0) {
		sprintf(errstr,"None of the sound will be gated.\n");
		return(GOAL_FAILED);
	}
	sndseekEx(dz->ifd[0],0,0);
	reset_filedata_counters(dz);
	fprintf(stdout,"INFO: Gating the file.\n");
	fflush(stdout);
	last_total_samps_read = 0;
	if((exit_status = read_samps(ibuf,dz))<0)
		return(exit_status);
	for(k=0;k < cutcnt;k+=2) {
		startcut = pos[k] - last_total_samps_read;
		endsplice = startcut;
		startcut -= (splicelen * chans);
		if(startcut < 0) {
			sprintf(errstr,"Bad initial cut: program error.\n");
			return(PROGRAM_ERROR);
		}
		endcut = pos[k+1] - last_total_samps_read;
		startsplice = endcut;
		endcut += (splicelen * chans);
		if(endcut > dz->insams[0]) {
			endcut = dz->insams[0];
			n = endcut - startsplice;
			if(n!=0)
				spliceincr = 1.0/(double)(n/chans);
		}
		spliceratio = 1.0;
		n = startcut;
		while(n >= dz->ssampsread) {
			if((exit_status = write_samps(ibuf,dz->ssampsread,dz))<0)
				return(exit_status);
			last_total_samps_read = dz->total_samps_read;
			if((exit_status = read_samps(ibuf,dz))<0)
				return(exit_status);
			n			-= dz->buflen;
			endsplice	-= dz->buflen;
			startsplice -= dz->buflen;
			endcut		-= dz->buflen;
		}
		while(n<endsplice) {
			spliceratio -= spliceincr; 
			for(m=0;m<chans;m++) 
				ibuf[n+m]  = (float)(ibuf[n+m] * spliceratio);
			n += chans;
			if(n >= dz->ssampsread) {
				if((exit_status = write_samps(ibuf,dz->ssampsread,dz))<0)
					return(exit_status);
				last_total_samps_read = dz->total_samps_read;
				if((exit_status = read_samps(ibuf,dz))<0)
					return(exit_status);
				n			-= dz->buflen;
				endsplice	-= dz->buflen;
				startsplice -= dz->buflen;
				endcut		-= dz->buflen;
			}
		}
		while(n < startsplice) {
			ibuf[n++] = 0.0f;
			if(n >= dz->ssampsread) {
				if((exit_status = write_samps(ibuf,dz->ssampsread,dz))<0)
					return(exit_status);
				last_total_samps_read = dz->total_samps_read;
				if((exit_status = read_samps(ibuf,dz))<0)
					return(exit_status);
				n			-= dz->buflen;
				startsplice -= dz->buflen;
				endcut		-= dz->buflen;
			}
		}
		spliceratio = 0.0;
		while(n < endcut) {
			spliceratio += spliceincr; 
			for(m=0;m<chans;m++) 
				ibuf[n+m]  = (float)(ibuf[n+m] * spliceratio);
			n += chans;
			if(n >= dz->ssampsread) {
				if((exit_status = write_samps(ibuf,dz->ssampsread,dz))<0)
					return(exit_status);
				last_total_samps_read = dz->total_samps_read;
				if((exit_status = read_samps(ibuf,dz))<0)
					return(exit_status);
				n		-= dz->buflen;
				endcut	-= dz->buflen;
			}
		}
	}
	while(dz->ssampsread > 0) {
		if((exit_status = write_samps(ibuf,dz->ssampsread,dz))<0)
			return(exit_status);
		if((exit_status = read_samps(ibuf,dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}

