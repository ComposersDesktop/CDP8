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



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <structures.h>
#include <tkglobals.h>
#include <pnames.h>
#include <globcon.h>
#include <cdpmain.h>
#include <house.h>
#include <modeno.h>
#include <sfsys.h>
#include <filetype.h>
#include <srates.h>
#include <flags.h>

#ifdef unix
#define round(x) lround((x))
#endif

static int 	  create_convert_buffer(dataptr dz);
static int	  create_reprop_buffer(dataptr dz);
static int 	  do_convert_process(dataptr dz);
static int	  reprop_process(dataptr dz);
static void   strip_dir(char *str);
static int	  create_cubic_spline_bufs(dataptr dz);
static int	  cubic_spline(dataptr dz);

/****************************** CREATE_RESPEC_BUFFERS ****************************/

int create_respec_buffers(dataptr dz)
{
	switch(dz->mode) {
	case(HOUSE_RESAMPLE):	return create_cubic_spline_bufs(dz);
	case(HOUSE_CONVERT):	return create_convert_buffer(dz);
	case(HOUSE_REPROP):		return create_reprop_buffer(dz);
	}
	return(FINISHED);
}

/***************************** DO_RESPECIFICATION ******************************/

int do_respecification(dataptr dz)
{
	switch(dz->mode) {
	case(HOUSE_RESAMPLE):	return cubic_spline(dz);
	case(HOUSE_CONVERT):	return do_convert_process(dz);
	case(HOUSE_REPROP):		return reprop_process(dz);
	}
	return(FINISHED);
}

/***************************** CREATE_CONVERT_BUFFER ******************************/

int create_convert_buffer(dataptr dz)
{
	int bigbufsize;
	int factor = (sizeof(short) + sizeof(float))/sizeof(short);

	bigbufsize = (int) (long)Malloc(-1);
	dz->buflen = bigbufsize/sizeof(float) / factor;
	if((dz->bigbuf = (float *)malloc((size_t)
		(dz->buflen * factor * sizeof(float))))==NULL) {
		sprintf(errstr,"Insufficient memory for sound.\n");
		return(MEMORY_ERROR);
	}
	dz->sampbuf[0]  = dz->bigbuf;
   	dz->flbufptr[0] = (float *)(dz->sampbuf[0] + dz->buflen); 
	return(FINISHED);
}

/***************************** CREATE_REPROP_BUFFER ******************************/

int create_reprop_buffer(dataptr dz)
{
	size_t bigbufsize;
	bigbufsize = (size_t)Malloc(-1);
	dz->buflen = (int)(bigbufsize / sizeof(float));
	
	if((dz->bigbuf = (float *)malloc(dz->buflen* sizeof(float)))==NULL) {
		sprintf(errstr,"Insufficient memory for sound.\n");
		return(MEMORY_ERROR);
	}
	dz->sampbuf[0]  = dz->bigbuf;
	return(FINISHED);
}

/************************** REPROP_PROCESS **************************/
//RWD.1.99  modified to check that the iparams are active
//RWD.1.99  modified to check that the iparams are active
/*RWD Oct 2001: but does not support 24bit files, etc.... yet; need new cmdline flags for that */
int reprop_process(dataptr dz)
{
	int intype;
	SFPROPS props = {0};
	int exit_status;
//TW modified: as infile->stype is now merely a flag for SOUND v NOT-SOUND DATA
	if(dz->infile->filetype != SNDFILE){
		sprintf(errstr,"This process only works with soundfiles.\n");
		return(DATA_ERROR);
	}
	if(!snd_headread(dz->ifd[0],&props)) {
		fprintf(stdout,"Failure to read sample type\n");
		fflush(stdout);
		return(DATA_ERROR);
	}
	switch(props.samptype) {
	case (2):  /* FLOAT32 */	intype = SAMP_SHORT;	break;
	case (1):  /* SHORT16 */	intype = SAMP_FLOAT;	break;
	default:
		sprintf(errstr,"This process only works with 16-bit or floating-point soundfiles.\n");
		return(DATA_ERROR);
	}
	if(dz->iparam[HSPEC_SRATE]==dz->infile->srate
	&& dz->iparam[HSPEC_CHANS]==dz->infile->channels
	&& dz->iparam[HSPEC_TYPE] == intype) {
		sprintf(errstr,"NO CHANGE to input file.\n");
		return(GOAL_FAILED);
	}
	/*RWD oct 2001: NB refuses 9-ch B-format files! */
	if((dz->iparam[HSPEC_CHANS] > 0) && (
		dz->iparam[HSPEC_CHANS] != 1 
		&& dz->iparam[HSPEC_CHANS] != 2
		&& dz->iparam[HSPEC_CHANS] != 4
		&& dz->iparam[HSPEC_CHANS] != 6
		&& dz->iparam[HSPEC_CHANS] != 8
		&& dz->iparam[HSPEC_CHANS] != 16
		)) {
    	sprintf(errstr,"\nInvalid channel count [%d]",dz->iparam[HSPEC_CHANS]);
		return(DATA_ERROR);
	}
	if((dz->iparam[HSPEC_SRATE] > 0) && (!IS_LOSR(dz->iparam[HSPEC_SRATE]) && !IS_HISR(dz->iparam[HSPEC_SRATE]))) {
    	sprintf(errstr,"\nInvalid sample rate [%d]",dz->iparam[HSPEC_SRATE]);
		return(DATA_ERROR);
	}

	if(dz->iparam[HSPEC_TYPE] >= SAMP_SHORT &&   (dz->iparam[HSPEC_TYPE]!=SAMP_SHORT
	&& dz->iparam[HSPEC_TYPE]!=SAMP_FLOAT)) {
    	sprintf(errstr,"\nInvalid sample type [%d]",dz->iparam[HSPEC_TYPE]);
		return(DATA_ERROR);
	}
	//RWD only update if the value is legal!
	if(dz->iparam[HSPEC_TYPE] >= SAMP_SHORT)
		dz->infile->stype    = dz->iparam[HSPEC_TYPE];
	if(dz->iparam[HSPEC_SRATE] > 0)
		dz->infile->srate    = dz->iparam[HSPEC_SRATE];
	if(dz->iparam[HSPEC_CHANS] > 0)
		dz->infile->channels = dz->iparam[HSPEC_CHANS];

	
	dz->ofd = sndcreat_formatted(dz->outfilename,-1,dz->infile->stype,dz->infile->channels,
									dz->infile->srate,CDP_CREATE_NORMAL);
//TW ADDED : for peak chunk
	dz->outchans = dz->infile->channels;
	establish_peak_status(dz);
	//test!
	dz->outfilesize = 0;
	if(dz->ofd < 0)	{
		sprintf(errstr,"Unable to open outfile %s\n",dz->outfilename);
		return SYSTEM_ERROR;
	}

	while(dz->samps_left) {		
		if((exit_status = read_samps(dz->sampbuf[0],dz))<0)
			return(exit_status);	
		if(dz->ssampsread > 0) {
			if((exit_status = write_exact_samps(dz->sampbuf[0],dz->ssampsread,dz))<0)
				return(exit_status);
		}
	}
	return(FINISHED);
}

/************************** DO_CONVERT_PROCESS **************************/

int do_convert_process(dataptr dz)
{
	int exit_status;
	float *fbuf = dz->flbufptr[0];
	SFPROPS props = {0};
	int outtype;		/*RWD*/

//TW dz->infile->stype now merely flags whether we have a soundfile
	if(dz->infile->filetype != SNDFILE){
		sprintf(errstr,"This process only works with soundfiles.\n");
		return(DATA_ERROR);
	}
	if(!snd_headread(dz->ifd[0],&props)) {
		fprintf(stdout,"Failure to read sample type\n");
		fflush(stdout);
		return(DATA_ERROR);
	}
	switch(props.samptype) {
	case (2):  /* FLOAT32 */	outtype = SAMP_SHORT;	break;
	case (1):  /* SHORT16 */	outtype = SAMP_FLOAT;	break;
	default:
		sprintf(errstr,"This process only works with 16-bit or floating-point soundfiles.\n");
		return(DATA_ERROR);
	}

	dz->ofd = sndcreat_formatted(dz->outfilename,-1,
		outtype,dz->infile->channels,
									dz->infile->srate,CDP_CREATE_NORMAL);	
	if(dz->ofd < 0)	{
		sprintf(errstr,"Unable to open outfile %s\n",dz->outfilename);
		return SYSTEM_ERROR;
	}
	dz->outchans = dz->infile->channels;
	establish_peak_status(dz);
	dz->tempsize = dz->insams[0];
	while(dz->samps_left){
		if((exit_status = read_samps(fbuf,dz))<0)
			return(exit_status);
		if(dz->ssampsread) {
			if((exit_status = write_samps(fbuf,dz->ssampsread,dz)) < 0)
				return exit_status;
		}
	}
	return(FINISHED);
}

/********************** BATCH_EXPAND ****************************/
 
int batch_expand(dataptr dz)
{
	int	   total_wordcnt = 0;
	int    n, m, k, linelen = dz->wordcnt[0];
	char   temp[200], temp2[200], temp3[10];
	char   **fname_wordstor = dz->wordstor + (dz->all_words - (dz->itemcnt * 2));
	char   **param_wordstor = dz->wordstor + (dz->all_words - dz->itemcnt);
	int	    fq;

	if(dz->infilecnt - 1 <= 0) {
		sprintf(errstr,"No sound files entered.\n");
		return(DATA_ERROR);
	}
	switch(dz->mode) {
	case(ONE_PARAM):
		for(k = 0; k < dz->itemcnt; k++) {
			total_wordcnt = 0;
			for(n=0;n< dz->linecnt;n++) {
				for(m=0;m<linelen;m++) {
					if(m==0) {
						strcpy(temp,dz->wordstor[total_wordcnt]);
					} else if(total_wordcnt % linelen == dz->iparam[BE_INFILE]) {
						strcat(temp," ");
						strcat(temp,fname_wordstor[k]);
					} else if(total_wordcnt % linelen == dz->iparam[BE_OUTFILE]) {
					strcpy(temp3,"_b");
					sprintf(temp2,"%d",n);
					strcat(temp3,temp2);
					strcpy(temp2,fname_wordstor[k]);
					strip_dir(temp2);
					insert_new_chars_at_filename_end(temp2,temp3);
					strcat(temp," ");
					strcat(temp,temp2);
						if((fq = sndopenEx(temp,0,CDP_OPEN_RDONLY)) >= 0) {
							sndcloseEx(fq);
							fq = -1;
							sprintf(errstr,"File %s already exists: cannot use as outname in batchfile\n",temp);
							return(DATA_ERROR);
						}
					} else if(total_wordcnt % linelen == dz->iparam[BE_PARAM]) {
						strcat(temp," ");
						strcat(temp,param_wordstor[k]);
					} else {
						strcat(temp," ");
						strcat(temp,dz->wordstor[total_wordcnt]);
					}
					total_wordcnt++;
				}
				strcat(temp,"\n");
				if(fputs(temp,dz->fp)<0) {
					sprintf(errstr,"Failed to copy of original batchfile line %d to output file.\n",n+1);
					return(SYSTEM_ERROR);
				}
			}
		}
		break;		
	case(MANY_PARAMS):
		total_wordcnt = 0;
		for(n=0,k=0;n< dz->linecnt;n++,k++) {
			if (k >= dz->itemcnt)
				break;	
			for(m=0;m<linelen;m++) {
				if(m==0) {
					/*sprintf*/strcpy(temp,dz->wordstor[total_wordcnt]); // RWD 07 2010 see above
				} else if(total_wordcnt % linelen == dz->iparam[BE_INFILE]) {
					strcat(temp," ");
					strcat(temp,fname_wordstor[k]);
				} else if(total_wordcnt % linelen == dz->iparam[BE_OUTFILE]) {
					strcpy(temp2,fname_wordstor[k]);
					insert_new_chars_at_filename_end(temp2,"_b");
					strcat(temp," ");
					strcat(temp,temp2);
					if((fq = sndopenEx(temp,0,CDP_OPEN_RDONLY)) >= 0) {
						sndcloseEx(fq);
						fq = -1;
						sprintf(errstr,"File %s already exists: cannot use as outname in batchfile\n",temp);
						return(DATA_ERROR);
					}
				} else if(total_wordcnt % linelen == dz->iparam[BE_PARAM]) {
					strcat(temp," ");
					strcat(temp,param_wordstor[k]);
				} else {
					strcat(temp," ");
					strcat(temp,dz->wordstor[total_wordcnt]);
				}
				total_wordcnt++;
			}
			strcat(temp,"\n");
			if(fputs(temp,dz->fp)<0) {
				sprintf(errstr,"Failed to print new batchfile line %d to output file.\n",n+1);
				return(SYSTEM_ERROR);
			}
		}
		break;
	}
	return FINISHED;
}

/********************** STRIP_DIR ****************************/
 
void strip_dir(char *str)
{
	char *p, *q, *r = str;
	q = str + strlen(str);
	p = q - 1;
	while(p > str) {
		if(*p == '/') {
			p++;
			while(p <= q)
				*r++ = *p++;
			return;
		}
		p--;
	}
}

/********************** CREATE_CUBIC_SPLINE_BUFS ****************************/
 
int create_cubic_spline_bufs(dataptr dz)
{
	int dblratio = sizeof(double)/sizeof(float);
	int dbl_buflen;
	int k, chans = dz->infile->channels;
	int dbl_chans = chans * dblratio;
	size_t bigbuf_bigsize, bigbufsize;
	int fsecsize = SECSIZE/sizeof(float);
	int framesize = fsecsize * dz->infile->channels;

	bigbuf_bigsize = ((size_t) Malloc(-1))/sizeof(float);
	bigbufsize = (bigbuf_bigsize/framesize) * framesize;
	if(bigbufsize <= 0)
		bigbufsize = framesize;
	k = (3 * dblratio) + 2;
	bigbuf_bigsize = bigbufsize * k;
	bigbuf_bigsize  += fsecsize;
	bigbuf_bigsize  += (chans * dblratio * 3);
	if((dz->bigbuf = (float *)malloc((size_t)(bigbuf_bigsize * sizeof(float)))) == NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
		return(MEMORY_ERROR);
	}
	memset((char *)dz->bigbuf,0,bigbuf_bigsize * sizeof(float));
	/* NB: wraparound points for both samples and derivatives are thus zeroed */
	dz->buflen = (int) bigbufsize;
	dbl_buflen = dz->buflen * dblratio;
	/* sampbuf[0] has chans samps store at start to read wraparound points */
	dz->sampbuf[0] = dz->bigbuf + fsecsize;
	dz->sampbuf[1] = dz->sampbuf[0] + dz->buflen;
	/* sampbufs[2][3][4] have additional chans samps store at start,and are doubles buffers */
	dz->sampbuf[2] = dz->sampbuf[1] + dz->buflen + dbl_chans;
	dz->sampbuf[3] = dz->sampbuf[2] + dbl_buflen + dbl_chans;
	dz->sampbuf[4] = dz->sampbuf[3] + dbl_buflen + dbl_chans;
	return(FINISHED);
}

/********************** CUBIC_SPLINE ****************************/
 
int cubic_spline(dataptr dz)
{
	int exit_status;
	int chans = dz->infile->channels;	
	int fsecsize = SECSIZE/sizeof(float);
		/* Input and Output and Derivatives buffers */
	float *ibuf = dz->sampbuf[0], *obuf = dz->sampbuf[1];
	double *y2 = (double *)dz->sampbuf[2];
	double *u  = (double *)dz->sampbuf[3];
	double *y3 = (double *)dz->sampbuf[4];
		/* constants used in calculation */
	double incr = (double)dz->infile->srate/(double)dz->iparam[HSPEC_SRATE];
	double sig = 0.5;
	double sig_less_one = sig - 1.0;
		/* flags used in calculation */
	int all_read = 0, start = 1;
		/* other variables */
	double d = 0.0, p;
	int n,m,k,obufcnt = 0, samps_to_process;
	double kk, hi, lo, hidif, lodif, x1, x2, x3, x4;
	int lochan, hichan;
	double samp;
	int inlo, inhi; 
	dz->tempsize = round((double)dz->insams[0]/incr);
		/* setup wraparound pointers */
	//create outfile here, now we have required format info	
	if((dz->ofd = sndcreat_formatted(dz->outfilename,-1,dz->infile->stype,dz->infile->channels,
									/*dz->infile->srate*/
									dz->iparam[HSPEC_SRATE],
									CDP_CREATE_NORMAL)) < 0) {
		sprintf(errstr,"Unable to open outfile %s\n",dz->outfilename);
		return SYSTEM_ERROR;
	}
	dz->outchans = dz->infile->channels;
	establish_peak_status(dz);

	k = dz->buflen - fsecsize - chans;
	dz->sbufptr[0] = dz->sampbuf[0] - chans;
	dz->sbufptr[1] = dz->sampbuf[0] + k;
	dz->ptr[0] = y2 - chans;
	dz->ptr[1] = y2 + k;
	dz->ptr[2] = u  - chans;
	dz->ptr[3] = u  + k;
	dz->ptr[4] = y3 - chans;
	dz->ptr[5] = y3 + k;

	while(dz->samps_left > 0) {
		/* Read samps, leaving overlapping samps at end of buffer to do wraparound calculations */		
		
		if((exit_status = read_samps(ibuf,dz))<0)
			return(exit_status);
		if(dz->total_samps_read < dz->insams[0]) {
// APRIL 2005
//			sfseek(dz->ifd[0],(dz->total_samps_read - fsecsize) * sizeof(float),0);
			sndseekEx(dz->ifd[0],dz->total_samps_read - fsecsize,0);

			samps_to_process = dz->ssampsread - fsecsize;
			dz->total_samps_read -= fsecsize;
			dz->samps_left += fsecsize;
		} else {
			all_read = 1;
			k = dz->ssampsread;
				/* if at end of data, add buffering zeros after end */
			for(n=0;n<chans;n++) {
				dz->sampbuf[0][k] = 0.0f;
				k++;
			}
			samps_to_process = dz->ssampsread; 
		}
			/* CALCULATE 2nd DERIVATIVE : STAGE 1 */
		for(n=0;n<samps_to_process;n+= chans) {
			for(m = 0;m <chans; m++) {
				k = n + m;
				p = sig * y2[k-chans] + 2.0;
				y2[k] = sig_less_one/p;
//				u[k]  = (ibuf[k+chans] - ibuf[k]) - (ibuf[k] - ibuf[k-chans]);
				u[k]  = (round(32767 * ibuf[k+chans]) - round(32767 * ibuf[k])) - (round(32767 * ibuf[k]) - round(32767 * ibuf[k-chans]));
				x1 = (6.0 * u[k])/2.0;
				x2 = sig * u[k-chans];
				u[k]  = (x1 - x2)/p;
			}
		}
		if(all_read) {	/* add "natural spline" end vals of 2nd derivative */
			for(m = 0;m <chans; m++)
				y2[n+m] = 0.0;
		}
			/* CALCULATE 2nd DERIVATIVE : STAGE 2 */
		for(n = samps_to_process-chans;n >= 0;n -= chans) {
			for(m = 0;m <chans; m++) {
				k = n + m;
				y3[k] = (y2[k] * y2[k+chans]) + u[k];
			}
		}
			/* DO SPLINE */
		k = samps_to_process/chans;
		kk = (double)k;
		if(start) {
			for(m=0;m<chans;m++)
				obuf[m] = ibuf[m];
			obufcnt = chans;
			start = 0;
		}
		for(;;) {
			d += incr;			/* Advance re-read pointer in buffer */
			if(d >= kk) {
				d -= incr;
				d -= kk;		/* if at end of input buffer, reset pointer, and break */
				break;
			}
			lo = floor(d);		/* find buffer indices of samples above and below pointer */
			hi = lo + 1.0;
			hidif = hi - d;
			lodif = d - lo;
			lochan = (int)lo * chans;
			hichan = (int)hi * chans;
			for(m=0;m<chans;m++) {
				n = obufcnt + m;
				inlo = lochan + m; 
				inhi = hichan + m; 
								/* DO SPLINE */
//				x1 = (hidif * ibuf[inlo]) + (lodif * ibuf[inhi]);	
				x1 = (hidif * round(32767 * ibuf[inlo])) + (lodif * round(32767 * ibuf[inhi]));	
				x2 = ((hidif * hidif * hidif) - hidif) * y3[inlo];
				x3 = ((lodif * lodif * lodif) - lodif) * y3[inhi];
				x4 = (x2 + x3)/6.0;
//				samp = x1 + x4;
				samp = ((int)round(x1 + x4)/32767.0);
				if(samp > 1.0)
					samp = 1.0;
				if(samp < -1.0)
					samp = -1.0;
				obuf[n] = (float)samp;
			}
			obufcnt += chans;
			if(obufcnt >= dz->buflen) {
				if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
					return(exit_status);
				obufcnt = 0;
			}
		}
		if(!all_read) {    /* do wrap-around points for input & derivatives */
			memcpy((char *)dz->sbufptr[0],(char *)dz->sbufptr[1],chans * sizeof(float));
			memcpy((char *)dz->ptr[0],(char *)dz->ptr[1],chans * sizeof(double));
			memcpy((char *)dz->ptr[2],(char *)dz->ptr[3],chans * sizeof(double));
			memcpy((char *)dz->ptr[4],(char *)dz->ptr[5],chans * sizeof(double));
		}	
	}
	if(obufcnt > 0) {
		if((exit_status = write_samps(obuf,obufcnt,dz))<0)
			return(exit_status);
	}
	return FINISHED;
}