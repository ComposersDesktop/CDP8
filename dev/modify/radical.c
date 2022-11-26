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
#include <memory.h>
#include <string.h>
#include <structures.h>
#include <tkglobals.h>
#include <globcon.h>
#include <arrays.h>
#include <cdpmain.h>
#include <modify.h>
#include <modeno.h>

//TW UPDATES
#include <arrays.h>
#include <flags.h>
#include <sfsys.h>
#include <osbind.h>

#if defined unix || defined __GNUC__
#define round(x) lround((x))
#endif

#ifndef HUGE
#define HUGE 3.40282347e+38F
#endif

//extern int snd_makepath(char path[],const char *sfname);
extern char *get_other_filename_x(char *filename,char c);

#define SCRUB_FORWARDS  (TRUE)
#define SCRUB_BACKWARDS (FALSE)
#define SCRUB_SAFE (.66)

static void reverse_the_buffer(float *ibuf,int chans,long ssampsread);
static int  do_lowbit(float *maxsamp,float *postmaxsamp,dataptr dz);
static int do_lowbit2(dataptr dz);

static int  shred(int current_buf,int work_len,dataptr dz);
static int  get_basis_lengths(int *worklen,dataptr dz);
static void normal_scat(dataptr dz);
static void heavy_scat(dataptr dz);
static void permute_chunks(dataptr dz);
static void insert(int n,int t,dataptr dz);
static void prefix(int n,dataptr dz);
static void shuflup(int k,dataptr dz);
static void do_startsplice(float *i,float *j,dataptr dz);
static void do_endsplice(float *i,float *j,dataptr dz);
static void do_bufend_splices(int current_buf,dataptr dz);
static void ptr_sort(int end,dataptr dz);
static int  check_for_too_small_buf(dataptr dz);

static int 	  create_sintable(dataptr dz);
static double get_skew(void);
static void   get_speed(double *lospeed,double *speedrang,dataptr dz);
static void   get_end(int *direction,int *here,dataptr dz);
static int	  gen_readtab(double skew,double lospeed,double speedrange,int scrublen_in_src,dataptr dz);
static int 	  scrub(int *hhere,int *tthere,int direction,int effective_tablen,dataptr dz);
static int 	  read_samps_for_scrub(int bufno,dataptr dz);

static double read_rmod_tab(double dtabindex,dataptr dz);

//TW UPDATES (converted to flotsams)
static int get_obuf_sampoffset(int n,dataptr dz);
static int transpos_and_add_to_obuf(int n,int *obufpos,double *inbufpos,float *lastival,int chans,dataptr dz);

/******************************* CREATE_REVERSING_BUFFERS *******************************/

int create_reversing_buffers(dataptr dz)
{
//TW BIG SIMPLIFICATION as buffer sector-alignment abolished
	size_t bigbufsize;
	bigbufsize = (size_t) Malloc(-1);
	dz->buflen = (int)(bigbufsize / sizeof(float));
	dz->buflen = (dz->buflen / dz->infile->channels) * dz->infile->channels;

	dz->iparam[REV_BUFCNT] = dz->insams[0]/dz->buflen;
	if(dz->iparam[REV_BUFCNT] > 0)
		dz->iparam[REV_RSAMPS] 	  = dz->insams[0] - (dz->iparam[REV_BUFCNT] * dz->buflen);
	if((dz->bigbuf = (float *)malloc(dz->buflen * sizeof(float)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for sound buffers.\n");
		return(MEMORY_ERROR);
	}
	dz->sampbuf[0] = dz->bigbuf; 
	return FINISHED;
}

/******************************* DO_REVERSING *******************************/

int do_reversing(dataptr dz)
{
//TW BIG SIMPLIFICATION as buffer sector-alignment abolished
	int exit_status;
	float *buf = dz->sampbuf[0];
	int chans  = dz->infile->channels;
	int nbuffs = dz->iparam[REV_BUFCNT];
	int thisbufsamps;

	if(nbuffs <=0) {
		if((exit_status = read_samps(buf,dz)) < 0) {
			sprintf(errstr,"Failed (1) to read input file.\n");
			return(PROGRAM_ERROR);
		}
		if(dz->ssampsread != dz->insams[0]) {
			sprintf(errstr,"Bad accounting.\n");
			return(PROGRAM_ERROR);
		}
		reverse_the_buffer(buf,chans,dz->ssampsread);
		if(dz->ssampsread > 0) {
			if((exit_status = write_samps(buf,dz->ssampsread,dz)) < 0) {
				sprintf(errstr,"Failed (1) to write to output file.");
				return(PROGRAM_ERROR);
			}
		}
	} else {
		thisbufsamps = dz->insams[0] - dz->iparam[REV_RSAMPS];
		if((sndseekEx(dz->ifd[0], thisbufsamps, 0)) < 0) {
			sprintf(errstr,"Reverse: failed to seek to point in file\n");
			return(SYSTEM_ERROR);
		}
		if((thisbufsamps -= dz->buflen)<0) {	/* ready for next iteration */
			sprintf(errstr,"Bad accounting 1.\n");
			return(PROGRAM_ERROR);
		}

		if((exit_status = read_samps(buf,dz)) < 0) {
			sprintf(errstr,"Failed (1a) to read input file\n");
			return(PROGRAM_ERROR);
		}
		if(dz->ssampsread != dz->iparam[REV_RSAMPS]) {
			sprintf(errstr,"Bad accounting 2.\n");
			return(PROGRAM_ERROR);
		}

		reverse_the_buffer(buf,chans,dz->ssampsread);

		if(dz->ssampsread > 0) {
			if((write_samps(buf, dz->ssampsread,dz)) < 0) {	/* writes (truncated number of) whole sector */
				sprintf(errstr,"Failed (1a) to write to output file");
				return(SYSTEM_ERROR);
			}
		}
		while(nbuffs>0) {
			if((sndseekEx(dz->ifd[0], thisbufsamps, 0)) < 0) {
				sprintf(errstr,"Reverse: failed to seek to point in file.\n");
				return(SYSTEM_ERROR);
			}
			if((exit_status = read_samps(buf,dz)) < 0) {
				sprintf(errstr,"Failed (1b) to read input file\n");
				return(PROGRAM_ERROR);
			}
			if(dz->ssampsread != dz->buflen) {
				sprintf(errstr,"Bad accounting 3.\n");
				return(PROGRAM_ERROR);
			}
			nbuffs--;
			if(nbuffs>0 && (thisbufsamps -= dz->buflen)<0) {	/* ready for next iteration */
				sprintf(errstr,"Bad accounting 4.\n");
				return(PROGRAM_ERROR);
			}
			reverse_the_buffer(buf,chans,dz->ssampsread);

			if(dz->ssampsread > 0) {
				if((exit_status = write_exact_samps(buf,dz->ssampsread,dz)) < 0) {	
					sprintf(errstr,"Failed (1b) to write to output file");
					return(PROGRAM_ERROR);
				}
			}
		}
	}
	return(FINISHED);
}
			
/******************************* REVERSE_THE_BUFFER *******************************/

void reverse_the_buffer(float *ibuf,int chans,long ssampsread)
{	
	int half_ssampsread = ((ssampsread/chans)/2) * chans;
	int n, m, k;
	float temp;
	for(n = 0,m = ssampsread-chans;n< half_ssampsread;n+=chans,m-=chans) {
		for(k=0;k<chans;k++) {
			temp      = ibuf[n+k];
			ibuf[n+k] = ibuf[m+k];
			ibuf[m+k] = temp;
		}
	}
}

/***************************** LOBIT_PCONSISTENCY *************************/

int lobit_pconsistency(dataptr dz)
{
	int garf = 1;
	int m;
	SFPROPS props = {0};
	if(!snd_headread(dz->ifd[0],&props)) {
		fprintf(stdout,"Failure to read sample size\n");
		fflush(stdout);
		return(DATA_ERROR);
	}
	
//TW ADDED
	dz->iparam[LOBIT_BRES_SHIFT] = 0;

	if(dz->iparam[LOBIT_BRES]!=MAX_BIT_DIV) {
    	dz->iparam[LOBIT_BRES] = MAX_BIT_DIV - dz->iparam[LOBIT_BRES];
//TW DISCOVERED ERROR
//	    garf = 1;
	    garf = 2;
	    for(m=1;m<dz->iparam[LOBIT_BRES];m++)
			garf *= 2;
    	dz->iparam[LOBIT_BRES] = garf;
//TW SIMPLIFIED, removed shift arithmetic
	}
//TW ADDED non-16-bit cases

	switch(props.samptype) {
	case(0):	/* SHORT8 */			
	case(7):	/* INT_MASKED */
		sprintf(errstr,"Process does not work with 8-bit files.\n");
		return(DATA_ERROR);
	case(1):	/* SHORT16 */	break;
	case(2):	/* FLOAT32 */
	case(3):	/* INT_32 */	dz->iparam[LOBIT_BRES] *= (256 * 256);	break;
	case(4):	/* INT2424 */	
	case(5):	/* INT2432 */	dz->iparam[LOBIT_BRES] *= 256;			break;
	case(6):	/* INT2024 */	dz->iparam[LOBIT_BRES] *= 16;			break;
	default:
		sprintf(errstr,"Unknown sample type.\n");
		return(DATA_ERROR);
	}
//TW SIMPLIFIED, removed shift arithmetic for the time averaging
	return(FINISHED);
}

/****************************** LOBIT_PROCESS *************************/

int lobit_process(dataptr dz)
{   
	int exit_status;
	float *buffer = dz->sampbuf[0];
	float maxsamp = 0.0, postmaxsamp = 0.0;
	display_virtual_time(0L,dz);
	if(dz->mode == MOD_LOBIT)
		dz->buflen = (dz->buflen / (dz->iparam[LOBIT_TSCAN] * dz->infile->channels)) * dz->iparam[LOBIT_TSCAN] * dz->infile->channels;
	while(dz->samps_left != 0) {
		if((exit_status = read_samps(buffer,dz))<0)
			return(exit_status);
		switch(dz->mode) {
		case(MOD_LOBIT):
			if((exit_status = do_lowbit(&maxsamp,&postmaxsamp,dz))<0)
				return(exit_status);
			break;
		case(MOD_LOBIT2):
			if((exit_status = do_lowbit2(dz))<0)
				return(exit_status);
			break;
		}
		if(dz->ssampsread > 0) {
			if((exit_status = write_exact_samps(buffer,dz->ssampsread,dz))<0)
				return(exit_status);
		}
    }
	return(FINISHED);
}

/**************************** DO_LOWBIT ***************************/

int do_lowbit(float *maxsamp,float *postmaxsamp,dataptr dz)
{   
	register int i, j;
	double x, maxsampval;
	int k, todo_samples;
	int n;
//TW MODIFIED FOR floats and non-16-bit sndfile formats
	float *buffer = dz->sampbuf[0];
	int tscan     = dz->iparam[LOBIT_TSCAN];
	int bres      = dz->iparam[LOBIT_BRES];
	int chans	  = dz->infile->channels;
	SFPROPS props = {0};
	
	if(!snd_headread(dz->ifd[0],&props)) {
		fprintf(stdout,"Failure to read sample size\n");
		fflush(stdout);
		return(DATA_ERROR);
	}
	switch(props.samptype) {
	case(1):	/* SHORT16 */	maxsampval = MAXSAMP;		break;
	case(2):	/* FLOAT32 */
	case(3):	/* INT_32 */	maxsampval = 2147483647.0;	break;
	case(4):	/* INT2424 */	
	case(5):	/* INT2432 */	maxsampval = 8388607.0;		break;
	case(6):	/* INT2024 */	maxsampval = 524287.0;		break;
	/*RWD April 2004: need a default case; may add new formats one day! */
		/* OR: make one of the above cases the default... */
	default:
		fprintf(stdout,"Unsupported sample type\n");
		fflush(stdout);
		return(DATA_ERROR);
	/* other cases rejected earlier */
	}
	if(tscan!=1) {
        if((todo_samples = (dz->ssampsread/(tscan * chans)) * tscan * chans) != dz->ssampsread) {
		    for(i = todo_samples;i<dz->ssampsread;i++)
		        buffer[i] = 0.0f;
	    	dz->ssampsread = todo_samples;
		}
    }
//TW MODIFIED to include non-16-bit cases, & files that are not mono
	for(i = 0; i < dz->ssampsread; i += (tscan * chans))  {
		for(n=0;n < chans; n++) {
			x = 0.0;
			k = i + n + (tscan * chans);
			for(j = i+n;j < k; j+= chans) {
				*maxsamp = (float) max(*maxsamp,fabs(buffer[j]));
				x += buffer[j];
			}
			x /= tscan;					/* Take a time average */
			x *= maxsampval;			/* convert to 'integer' range */
			x /= bres;
			x  = (double)round(x);		/* do a rounding mod bres */
			x *= bres;
			x /= maxsampval;
			x = min(x,1.0);
			x = max(x,-1.0);
			k = min(k,dz->ssampsread);
			for(j = i+n;j < k; j+= chans) {
				buffer[j] = (float)x;	/* reconvert to float range */
				*postmaxsamp = (float) max(*postmaxsamp,fabs(buffer[j]));
			}
		}
	}
	return(FINISHED);
}

/**************************** DO_LOWBIT2 ***************************/

int do_lowbit2(dataptr dz)
{   
	int n;
//TW MODIFIED FOR floats and non-16-bit sndfile formats
	float *buffer = dz->sampbuf[0];
	double samp;
	int srcbits = 16;	// We assume src is quasi-16bit
	int nbits   = dz->iparam[LOBIT_BRES];
	int rescalebits = srcbits - nbits;
	double rescalefac = pow(2.0,rescalebits);
	for(n = 0; n < dz->ssampsread; n++) {
		samp = buffer[n] * 32767.0;		//	prescale to 16-bit range
		if(samp <= -32767.0)	
			samp = -32767.0;	//	keep within range
		// do mid-rise quantisation
		samp /= rescalefac;
		samp = round(samp+0.5) - 0.5;	//	round to a multiple of 0.5
		samp *= rescalefac;
		// rescale back to float 0-1 representation 
		buffer[n] = (float)(samp/32768.0);
	}
	return FINISHED;
}

/***************************** SHRED_PCONSISTENCY *************************/

int shred_pconsistency(dataptr dz)
{
	int chans = dz->infile->channels;
	double duration = (double)(dz->insams[0]/chans)/(double)dz->infile->srate;
    initrand48();
	dz->iparam[SHR_CHCNT] = round(duration/dz->param[SHRED_CHLEN]);
	if(dz->param[SHRED_SCAT] > (double)dz->iparam[SHR_CHCNT]) {
		sprintf(errstr,"Scatter value cannot be greater than infileduration/chunklength.\n");
		return(DATA_ERROR);
	}    
	if(dz->param[SHRED_SCAT] > 1.0)
		dz->iparam[SHRED_SCAT] = round(dz->param[SHRED_SCAT]);
	else
		dz->iparam[SHRED_SCAT] = 0;		
	/* setup splice params */
	dz->iparam[SHRED_SPLEN]     = SHRED_SPLICELEN  * chans;
    if((dz->lparray[SHR_CHUNKPTR] = (int *)malloc(dz->iparam[SHR_CHCNT] * sizeof(long)))==NULL
    || (dz->lparray[SHR_CHUNKLEN] = (int *)malloc(dz->iparam[SHR_CHCNT] * sizeof(long)))==NULL
    || (dz->iparray[SHR_PERM]     = (int  *)malloc(dz->iparam[SHR_CHCNT] * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for shredding arrays.\n");
		return(MEMORY_ERROR);
	}
    dz->lparray[SHR_CHUNKPTR][0] = 0;	/* first chunk is at start of buffer */
	return(FINISHED);
}

/***************************** SHRED_PREPROCESS *************************/

int shred_preprocess(dataptr dz)
{
	int sectorcnt;
	sectorcnt = dz->insams[0]/dz->buflen;
	if(sectorcnt * dz->buflen < dz->insams[0])
		sectorcnt++;
	if(sectorcnt > 1)
		fprintf(stdout,"INFO: File will be shredded in %d distinct sectors.\n",sectorcnt);
	fflush(stdout);
	return(FINISHED);
}

/***************************CREATE_SHRED_BUFFERS **************************/
//TW replaced by global
//#define SHRED_SECSIZE (256)
/* RWD sigh..... */
int create_shred_buffers(dataptr dz)
{
	int exit_status;
	int bigfilesize, file_size_in_frames;
	double bufs_per_file, lchunkcnt; 
	size_t basic_bufchunk, bufchunk = (size_t)Malloc(-1);
	int framesize = F_SECSIZE * dz->infile->channels;
	bigfilesize = dz->insams[0];
	if((dz->bigbuf = (float *)malloc((bigfilesize * dz->bufcnt) * sizeof(float)))==NULL) {
		bufchunk /= sizeof(float);
		basic_bufchunk = bufchunk;
		for(;;) {
			if((dz->buflen = (int) bufchunk) < 0) {
				sprintf(errstr,"INSUFFICIENT MEMORY to allocate sound buffer.\n");
				return(MEMORY_ERROR);
			}
			dz->buflen  = (dz->buflen/(framesize * dz->bufcnt)) * framesize * dz->bufcnt;
			dz->buflen /= dz->bufcnt;
		/* NEW --> */
			if(dz->buflen <= 0)
				dz->buflen  = framesize;
		/* <-- NEW */
			file_size_in_frames = dz->insams[0]/framesize;
			if(framesize * file_size_in_frames < dz->insams[0])
				file_size_in_frames++;
			bigfilesize = file_size_in_frames * framesize;
			if(bigfilesize <= dz->buflen)
				dz->buflen  = bigfilesize;
			if((dz->bigbuf = (float *)malloc((dz->buflen * dz->bufcnt) * sizeof(float)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY to allocate sound buffer.\n");
				return(MEMORY_ERROR);
			}
			if((exit_status = check_for_too_small_buf(dz))>=0)
				break;
			bufchunk += basic_bufchunk;
		}
		dz->sampbuf[0] = dz->bigbuf;
		dz->sampbuf[1] = dz->sampbuf[0] + dz->buflen;
		if(bigfilesize == dz->buflen) {
			dz->iparam[SHR_LAST_BUFLEN] = dz->insams[0];	/* i.e. buflen = true filelen */
			dz->iparam[SHR_LAST_CHCNT]  = dz->iparam[SHR_CHCNT];
			dz->iparam[SHR_LAST_SCAT]   = dz->iparam[SHRED_SCAT];
		} else {
			bufs_per_file = (double)dz->insams[0]/(double)dz->buflen;
			lchunkcnt     = (double)dz->iparam[SHR_CHCNT]/bufs_per_file;
			dz->iparam[SHR_CHCNT]       = round(lchunkcnt);
			dz->iparam[SHRED_SCAT]      = min(dz->iparam[SHRED_SCAT],dz->iparam[SHR_CHCNT]);
			dz->iparam[SHR_LAST_BUFLEN] = (dz->insams[0]%dz->buflen);
			dz->iparam[SHR_LAST_CHCNT]  = round(lchunkcnt*((double)dz->iparam[SHR_LAST_BUFLEN]/(double)dz->buflen));
			dz->iparam[SHR_LAST_SCAT]   = min(dz->iparam[SHRED_SCAT],dz->iparam[SHR_LAST_CHCNT]);
		}
		if(dz->iparam[SHR_LAST_CHCNT] < 2) {
			fprintf(stdout, "WARNING: FINAL BUFFER WON'T BE SHREDDED (Too short for chunklen set).\n");
			fprintf(stdout, "WARNING: It will shred if you\n");
			fprintf(stdout, "WARNING: a) shorten infile by (>) chunklen, OR\n");
			fprintf(stdout, "WARNING: b) alter chunklen until last buffer has >1 chunk in it.\n");
			fflush(stdout);
		}
	} else {
		dz->buflen = bigfilesize;
		dz->sampbuf[0] = dz->bigbuf;
		dz->sampbuf[1] = dz->sampbuf[0] + dz->buflen;
		dz->iparam[SHR_LAST_BUFLEN] = dz->insams[0];	/* i.e. buflen = true filelen */
		dz->iparam[SHR_LAST_CHCNT]  = dz->iparam[SHR_CHCNT];
		dz->iparam[SHR_LAST_SCAT]   = dz->iparam[SHRED_SCAT];
		if(dz->iparam[SHR_LAST_CHCNT] < 2) {
			fprintf(stdout, "WARNING: FINAL BUFFER WON'T BE SHREDDED (Too short for chunklen set).\n");
			fprintf(stdout, "WARNING: It will shred if you\n");
			fprintf(stdout, "WARNING: a) shorten infile by (>) chunklen, OR\n");
			fprintf(stdout, "WARNING: b) alter chunklen until last buffer has >1 chunk in it.\n");
			fflush(stdout);
		}
	}
	memset((char *) (dz->sampbuf[0]),0,(dz->buflen * dz->bufcnt) * sizeof(float));
	return(FINISHED);
}

/************************* SHRED_PROCESS ***************************/

int shred_process(dataptr dz)
{
	int exit_status;
	int n, cnt = 0, checker = 0, work_len;
	int current_buf;
	if(sloom && dz->iparam[SHRED_SCAT]) {
		fprintf(stdout,"WARNING: There is a finite possibility program will not terminate.\n");
		fprintf(stdout,"WARNING: If in doubt, press STOP\n");
		fflush(stdout);
	}
	do {
		if(!sloom && !sloombatch)
			fprintf(stdout,"\nBUFFER %d ",cnt++);
		else {
			fprintf(stdout,"INFO: BUFFER %d\n",cnt++);
			fflush(stdout);
		}
		if((exit_status = read_samps(dz->sampbuf[0],dz))<0)
			return(exit_status);
		current_buf = 0;
		if(dz->samps_left <= 0 ) {
			dz->buflen = dz->iparam[SHR_LAST_BUFLEN];
			dz->iparam[SHRED_SCAT] = dz->iparam[SHR_LAST_SCAT];
			dz->iparam[SHR_CHCNT]  = dz->iparam[SHR_LAST_CHCNT];
		}
		if((dz->iparam[SHR_CHCNT_LESS_ONE] = dz->iparam[SHR_CHCNT] - 1)<=0) {
			if(cnt==1) {
				sprintf(errstr,"SOUNDFILE TOO SMALL to shred.\n");
				return(DATA_ERROR);
			} else {
				n = dz->buflen;
				dz->buflen = dz->ssampsread;
				do_bufend_splices(current_buf,dz);
				if((exit_status = write_exact_samps(dz->sampbuf[current_buf],dz->buflen,dz))<0)
					return(exit_status);
				dz->buflen = n;
			}
		} else {
			if((exit_status = get_basis_lengths(&work_len,dz))<0)
				return(exit_status);
			if(!sloom && !sloombatch)
				checker = 0;
			else
				display_virtual_time(0L,dz);
			for(n=0;n<dz->iparam[SHRED_CNT];n++) {
				if((exit_status = shred(current_buf,work_len,dz))<0)
					return(exit_status);
				current_buf = !current_buf;	

				if(!sloom && !sloombatch) {
					if(n==checker) {
						printf("*");
						checker += 16;
					}
				} else
					display_virtual_time((long)n,dz);
			}
			do_bufend_splices(current_buf,dz);
			if((exit_status = write_exact_samps(dz->sampbuf[current_buf],dz->buflen,dz))<0)
				return(exit_status);
		}
	} while(dz->samps_left > 0);
	return(FINISHED);
}

/******************************* SHRED **************************
 *
 * (1)	If the random-scattering of chunk boundaries is <= 1.0, scatter
 *	each chunk boundary separately (normal_scat).
 *	Else, scatter chunk boundaries over groups of chunklens (heavy_scat).
 * (2)	Calculate the lengths of the chunks, from difference of their positions.
 * (2A)	NB the length of the is the difference between its start and
 *	THE END OF THE work_len (NOT  the ned of the buffer). Chunklens are
 *	measured from start of startsplice to START of endsplice (see
 *	diagram).
 * (3)	Generate a permutation (permm[]) of the chunk numbers.
 *
 * (4)	The chunk that is to be FIRST in the permuted set does not need
 *	to be spliced at its beginning (as there's nothing prior to it,
 *	to splice into) but only at its end.
 * (a)	The address from which we copy is position in current buffer
 *	where chunk associated with 0 (chunk[permm[0]) is.
 * (b)	The address to copy TO is start of the OTHER  buffer (buf[!this]).
 * (c) 	The unit_len of the chunk we're copying is chunklen[permm[0]].
 * (d)	Copy all of this.
 * (f)	Copy SPLICELEN extra (full_len is SPLICELEN bigger that unit_len..
 *	.. see diagram), making an endsplice on it as we do.
 *
 * (5)	For the rest of the chunks APART FROM THE LAST....
 * (a)	as 4(a): n associated with perm[n]..
 * (b)  advance in new buffer by chunklen of PREVIOUSLY written chunk..
 *	This 'length' is length to START of its endsplice, which is where
 *	new chunk is spliced in.
 * (c)  as 4(c)
 * (d)  Do a startsplice as we copy from old_address to new_address.
 * (e)	Copy from END of this startsplice, a segment of length chnklen
 *	(length of the total copied chunk) MINUS length of that startsplice
 *	(SPLICELEN).
 * (f)	as 4(f).
 *
 * (6)	For the last chunk, we don't need an endsplice (nothing to splice to)
 * (a-d)as 5(a-d).
 * (e)	Copy from end of STARTSPLICE to end of ENDSPLICE ... i.e.
 *	a whole chunklen, because full_len - SPLICELEN = chunklen.
 *
 *         ___full_len___
 *        |              |
 *         _chunklen__   |
 *        | S         |
 *        | T         |E
 *        | R ________|N
 *        | T/|       \D
 *        | / |        \
 *	  |/  |         \|
 *            |          |
 *            |_chunklen_|
 *        |
 *        |___full_len___|
 *
 * (7)	Set memory in old buffer to 0 (because we ADD into it, with splices).
 */

int shred(int current_buf,int work_len,dataptr dz)
{   
	int n;
	float *old_addr, *new_addr;
	int chnk_len;
	char *destination = NULL;
	int *chunkptr = dz->lparray[SHR_CHUNKPTR];
	int *chunklen = dz->lparray[SHR_CHUNKLEN];
	int  *permm    = dz->iparray[SHR_PERM];

	if(!dz->iparam[SHRED_SCAT])	
		normal_scat(dz);											/* 1 */
	else	
		heavy_scat(dz);
	for(n=0;n<dz->iparam[SHR_CHCNT_LESS_ONE];n++)					/* 2 */
		chunklen[n] = chunkptr[n+1] - chunkptr[n];
	chunklen[n] = work_len - chunkptr[n];							/* 2A */
	permute_chunks(dz);												/* 3 */
		/* NEW FIRST-SEGMENT IN BUFFER */							/* 4 */
	old_addr = dz->sampbuf[current_buf]  + chunkptr[permm[0]];		/* a */
	new_addr = dz->sampbuf[!current_buf];							/* b */
	chnk_len = chunklen[permm[0]];									/* c */
	if(dz->vflag[0]) {
		do_startsplice(old_addr,new_addr,dz);
		destination = (char *)(new_addr+dz->iparam[SHRED_SPLEN]);
		memcpy(destination,
		(char *)(old_addr+dz->iparam[SHRED_SPLEN]),(chnk_len-dz->iparam[SHRED_SPLEN]) * sizeof(float));
	} else {
		destination = (char *)new_addr;
		memcpy(destination,(char *)old_addr,chnk_len*sizeof(float));
	}
	do_endsplice(old_addr+chnk_len, new_addr+chnk_len,dz);			/* f */

		 /* MIDDLE-SEGMENTS IN BUFFER */							/* 5 */
	for(n=1;n<dz->iparam[SHR_CHCNT_LESS_ONE];n++) {
		old_addr  = dz->sampbuf[current_buf]  + chunkptr[permm[n]];	/* a */
		new_addr += chnk_len;										/* b */
		chnk_len  = chunklen[permm[n]];								/* c */
		do_startsplice(old_addr,new_addr,dz);						/* d */
		destination = (char *)(new_addr+dz->iparam[SHRED_SPLEN]);
		memcpy(destination,
		(char *)(old_addr+dz->iparam[SHRED_SPLEN]),(chnk_len-dz->iparam[SHRED_SPLEN]) * sizeof(float));
		do_endsplice(old_addr+chnk_len,new_addr+chnk_len,dz);
	}							/* f */
		/* NEW END-SEGMENT IN BUFFER */								/* 6 */
	old_addr  = dz->sampbuf[current_buf] + chunkptr[permm[n]];		/* a */
	new_addr += chnk_len;											/* b */
	chnk_len  = chunklen[permm[n]];									/* c */
	do_startsplice(old_addr,new_addr,dz);							/* d */
	if(dz->vflag[0])
		do_endsplice(old_addr+chnk_len,new_addr+chnk_len,dz);
	destination = (char *)(new_addr+dz->iparam[SHRED_SPLEN]);
	memcpy(destination,(char *)(old_addr+dz->iparam[SHRED_SPLEN]),chnk_len*sizeof(float));

		     /* RESET BUFFERS */
	memset((char *)dz->sampbuf[current_buf],0,dz->buflen * sizeof(float)); /* 7 */
	return(FINISHED);
}

/*********************** GET_BASIS_LENGTHS **********************
 *
 *
 *    ______________buflen_______________
 *   |............worklen.............   |  buflen - SPLICELEN  = worklen.
 *   | unitlen unitlen		      	  |  |  unit_len * dz->iparam[SHR_CHCNT] = worklen.
 *   |	      |	      | 	          |  |  
 *   |_full_len_      |       _full_len__|  full_len = worklen + SPLICELEN.
 *   |	      |	|     |      |	      |  |
 *   |   _____| |_____|  ____|   _____|  |
 *   |  /|    \ /     \ /     \ /     \  |
 *   | / |    |X|      X       X       \ |
 *   |/  |    / \     / \     / \       \|
 *   |	 |    | 
 *   |	 |    | 
 *       rawlen
 *
 *
 */

int get_basis_lengths(int *work_len,dataptr dz)
{   
	int excess, full_len, endunit_len, endfull_len;
	*work_len = dz->buflen - dz->iparam[SHRED_SPLEN];
	dz->iparam[SHR_UNITLEN] = (int)round((*work_len)/dz->iparam[SHR_CHCNT]);
	excess = dz->iparam[SHR_UNITLEN] % dz->infile->channels;
	dz->iparam[SHR_UNITLEN] -= excess;
	excess   = (*work_len) - (dz->iparam[SHR_UNITLEN] * dz->iparam[SHR_CHCNT]);
	if(excess % dz->infile->channels) {
		sprintf(errstr,"Problem in buffer accounting.\n");
		return(PROGRAM_ERROR);
	}
	dz->iparam[SHR_RAWLEN]  = dz->iparam[SHR_UNITLEN] - dz->iparam[SHRED_SPLEN];
	full_len = dz->iparam[SHR_UNITLEN] + dz->iparam[SHRED_SPLEN];
	endunit_len = dz->iparam[SHR_UNITLEN] + excess;
	endfull_len = full_len + excess;
	dz->iparam[SHR_ENDRAWLEN]  = dz->iparam[SHR_RAWLEN]  + excess;
	if(full_len < (dz->iparam[SHRED_SPLEN] * 2) || endfull_len < (dz->iparam[SHRED_SPLEN] * 2)) {
		sprintf(errstr,"Chunksize too small for splices.\n");
		return(DATA_ERROR);
	}
	if(dz->iparam[SHRED_SCAT]) {
		dz->iparam[SHR_SCATGRPCNT] = (int)(dz->iparam[SHR_CHCNT]/dz->iparam[SHRED_SCAT]);
		dz->iparam[SHR_ENDSCAT]    = (int)(dz->iparam[SHR_CHCNT] - (dz->iparam[SHR_SCATGRPCNT] * dz->iparam[SHRED_SCAT]));
		dz->iparam[SHR_RANGE]      = dz->iparam[SHR_UNITLEN] * dz->iparam[SHRED_SCAT];
		dz->iparam[SHR_ENDRANGE]   = ((dz->iparam[SHR_ENDSCAT]-1) * dz->iparam[SHR_UNITLEN]) + endunit_len;
	}
	return(FINISHED);
}

/************************** NORMAL_SCAT ******************************
 *
 * (1)	TOTLA_LEN generates the unscattered positions of the chunks.
 *	Each is UNIT_LEN long, so they are equally spaced at UNIT_LEN
 *	intervals.
	We can't scatter the FIRST chunk as it MUST start at ZERO!!
 * (2)	For all chunks except the first and last...
 * (3)	    Scatter position of chunk over +- 1/2 of value of scatter,
 *	    times the RAW-distance (not including splices) between chunks.
 * (4)      Add (could be negative) this scattering to orig position.
 * (5)	For the last chunk, do the same, scattering over RAW-len of previous
 *	chunk, if scatter -ve, and over endraw_len of final chunk, if +ve.
 */

void normal_scat(dataptr dz)
{   
	double this_scatter;
	int n, k;
	int chunkscat, total_len = dz->iparam[SHR_UNITLEN];	/* 1 */
	int *chunkptr = dz->lparray[SHR_CHUNKPTR];
	for(n=1;n<dz->iparam[SHR_CHCNT_LESS_ONE];n++) {			/* 2 */
		this_scatter  = (drand48() - 0.5) * dz->param[SHRED_SCAT];
		chunkscat = (int)(this_scatter * (double)dz->iparam[SHR_RAWLEN]);
		k = chunkscat % dz->infile->channels;
		chunkscat -= k;
		chunkptr[n] = total_len + chunkscat;
		total_len  += dz->iparam[SHR_UNITLEN];				/* 4 */
	}
	this_scatter  = (drand48() - 0.5) * dz->param[SHRED_SCAT];
	if(this_scatter<0.0) {					/* 5 */
		chunkscat   = (int)(this_scatter * (double)dz->iparam[SHR_RAWLEN]);
		k = chunkscat % dz->infile->channels;
		chunkscat -= k;
		chunkptr[n] = total_len - chunkscat;
	} else {
		chunkscat   = (int)(this_scatter * (double)dz->iparam[SHR_ENDRAWLEN]);
		k = chunkscat % dz->infile->channels;
		chunkscat -= k;
		chunkptr[n] = total_len + chunkscat;
	}
}

/*********************** HEAVY_SCAT ***************************
 *
 * (1)	Start at the chunk (this=1) AFTER the first (which can't be moved).
 * (2) 	STARTPTR marks the start of the chunk GROUP (and will be advanced
 *	by RANGE, which is length of chunk-group).
 * (3)	The loop will generate a set of positions for the chunks in
 *	a chunk-group. In the first chunkgroup the position of the
 *	first chunk (start of file) can't be moved, so loop starts at
 *	(first=) 1. Subsequemt loop passes start at 0.
 * (4)	For eveery chunk-group.
 * (5)	  Set the index of the first chunk in this group (start) to the
 *	  current index (this).
 * (6)	  For every member of this chunk-group.
 * (7)	    Generate a random-position within the chunk-grp's range
 *	    and check it is not too close ( < SPLICELEN) to the others.
 *	      Set a checking flag (OK).
 * (8)	      Generate a position within the range, and after the startptr.
 * (9)		Compare it with all previously generated positions in this
 *		chunk-grp AND with last position of previous chunk-group!!
 * 		If it's closer than SPLICELEN, set OK = 0, drop out of
		checking loop and generate another position instead.
 * (10)	      If the position is OK, drop out of position generating loop..
 * (11)	    Advance to next chunk in this group.
 * (12)	  Once all this group is done, advance the group startpoint by RANGE.
 * (13)	  After FIRST grp, all positions can by varied, so set the initial
 *	  loop counter to (first=)0.
 * (14)	If there are chunks left over (endscat!=0)..
 *	  Follow the same procedure for chunks in end group, using the
 *	  alternative variables, endscat and endrange.
 */

void heavy_scat(dataptr dz)
{   
	int	thiss = 1, that, start, n, m, OK;				/* 1 */
	int k;
	int startptr = 0;							/* 2 */
	int endptr = 0;
	int first = 1;								/* 3 */
	int *chunkptr = dz->lparray[SHR_CHUNKPTR];
	for(n=0;n<dz->iparam[SHR_SCATGRPCNT];n++) {				/* 4 */
		start = thiss;							/* 5 */
		endptr += dz->iparam[SHR_RANGE];
		for(m=first;m<dz->iparam[SHRED_SCAT];m++) {			/* 6 */
			do{							/* 7 */
				OK = 1;					
				chunkptr[thiss] = (long)(drand48()*dz->iparam[SHR_RANGE]); 	/* TRUNCATE (?)*/
				chunkptr[thiss] += startptr; 					/* 8 */
				k = chunkptr[thiss] % dz->infile->channels;
				chunkptr[thiss] -= k;
				for(that=start-1; that<thiss; that++) {
					if(abs(chunkptr[thiss] - chunkptr[that])<dz->iparam[SHRED_SPLEN]) {
						OK = 0;					/* 9 */
						break;
					}
					if(abs(endptr - chunkptr[thiss])<dz->iparam[SHRED_SPLEN]) {
						OK = 0;
						break;
					}
				}
			} while(!OK);							/* 10 */
			thiss++;							/* 11 */
		}
		startptr += dz->iparam[SHR_RANGE];					/* 12 */
		first = 0;								/* 13 */
	}

	endptr += dz->iparam[SHR_ENDRANGE];

	if(dz->iparam[SHR_ENDSCAT]) {							/* 14 */
		start = thiss;
		for(m=0;m<dz->iparam[SHR_ENDSCAT];m++) {
			do {
				OK = 1;
				chunkptr[thiss] = (long)(drand48() * dz->iparam[SHR_ENDRANGE]);	  /* TRUNCATE (?) */
				chunkptr[thiss] += startptr;
				k = chunkptr[thiss] % dz->infile->channels;
				chunkptr[thiss] -= k;
				for(that=start-1; that<thiss; that++) {
					if(abs(chunkptr[thiss] - chunkptr[that])<dz->iparam[SHRED_SPLEN]) {
						OK = 0;
						break;
					}
					if(abs(endptr - chunkptr[thiss])<dz->iparam[SHRED_SPLEN]) {
						OK = 0;
						break;
					}
				}
			} while(!OK);
			thiss++;
		}
	}
	ptr_sort(thiss,dz);
}

/*************************** PERMUTE_CHUNKS ***************************/

void permute_chunks(dataptr dz)
{   
	int n, t;
	for(n=0;n<dz->iparam[SHR_CHCNT];n++) {
		t = (int)(drand48() * (double)(n+1));	 /* TRUNCATE */
		if(t==n)
			prefix(n,dz);
		else
			insert(n,t,dz);

	}
}

/****************************** INSERT ****************************/

void insert(int n,int t,dataptr dz)
{   
	shuflup(t+1,dz);
	dz->iparray[SHR_PERM][t+1] = n;
}

/****************************** PREFIX ****************************/

void prefix(int n,dataptr dz)
{   
	shuflup(0,dz);
	dz->iparray[SHR_PERM][0] = n;
}

/****************************** SHUFLUP ****************************/

void shuflup(int k,dataptr dz)
{   
	int n;
	for(n = dz->iparam[SHR_CHCNT_LESS_ONE]; n > k; n--)
		dz->iparray[SHR_PERM][n] = dz->iparray[SHR_PERM][n-1];
}

/************************* DO_STARTSPLICE *************************
 *
 * (0)	For each position in splice.
 * (1)	Get the value from the source file & multiply it by position in splice.
 *	We 'should' multiply it by position/SPLICELEN,
 *	so new value is SPLICELEN too large.
 * (2)	Add a rounding factor (if value were correct, this would be 1/2)
 *	But everything is SPLICELEN times too big, so add SHRED_HSPLICELEN.
 * (3)	Divide by SPLICELEN.
 * (4)	ADD this value to existing value (this is a splice TO another chunk!!).
 */

void do_startsplice(float *i,float *j,dataptr dz)
{   
	double z;
	int n;
	for(n = 0; n <dz->iparam[SHRED_SPLEN]; n ++) {			/* 0 */
		z     = (*i) * (double)n/dz->iparam[SHRED_SPLEN];	/* 1 */
		i++;
		*j = (float) /*round*/((*j) + z);						/* 4 */
		j++;											
	}
}

/************************* DO_ENDSPLICE *************************/

void do_endsplice(float *i,float *j,dataptr dz)
{   
	double z;
	int n;
	for(n = dz->iparam[SHRED_SPLEN]-1; n>=0; n--) {
		z     = *i++ * (double)n/dz->iparam[SHRED_SPLEN];
		*j++ += (float) /*round*/ (z);
	}
}

/********************* DO_BUFEND_SPLICES *************************/

void do_bufend_splices(int current_buf,dataptr dz)
{
	double z;
	int  n;
	float *b = dz->sampbuf[current_buf];
	for(n = 0; n <dz->iparam[SHRED_SPLEN]; n ++) {
		z     = (*b) * (double)n/dz->iparam[SHRED_SPLEN];
		*b++  = (float)z;
	}
	b = dz->sampbuf[current_buf] + dz->buflen - dz->iparam[SHRED_SPLEN];
	for(n = dz->iparam[SHRED_SPLEN]-1; n>=0; n--) {
		z     = (*b) * (double)n/dz->iparam[SHRED_SPLEN];
		*b++  = (float) /*round*/(z);
	}
}

/**************************** CHECK_FOR_TOO_SMALL_BUF *********************/

int check_for_too_small_buf(dataptr dz)
{
	int chunksamps = round(dz->param[SHRED_CHLEN] * dz->infile->srate) * dz->infile->channels; 
	if(dz->iparam[SHRED_SCAT]) {
		if(dz->buflen <= chunksamps * dz->iparam[SHRED_SCAT]) {
			sprintf(errstr,"Internal sound buffer too short (%lf secs) for chunksize and scatter.\n",
	       (double)(dz->buflen/dz->infile->channels)/(double)dz->infile->srate);
			return(GOAL_FAILED);
		}
	} else {
		if(dz->buflen <= chunksamps * 2) {
			sprintf(errstr,"Sound buffer too short for chunksize.\n");
			return(GOAL_FAILED);
		}
	}
	return(FINISHED);
}

/************************** PTR_SORT ***************************/

void ptr_sort(int end,dataptr dz)
{   
	int i,j;
	int a;
	int *chunkptr = dz->lparray[SHR_CHUNKPTR];
	for(j=1;j<end;j++) {
		a = chunkptr[j];
		i = j-1;
		while(i >= 0 && chunkptr[i] > a) {
			chunkptr[i+1]=chunkptr[i];
			i--;
		}
		chunkptr[i+1] = a;
	}
}    

/*************************** SCRUB_PCONSISTENCY **************************/

int scrub_pconsistency(dataptr dz)
{
	if(dz->param[SCRUB_MINSPEED] > dz->param[SCRUB_MAXSPEED])
		swap(&(dz->param[SCRUB_MINSPEED]),&dz->param[SCRUB_MAXSPEED]);
	dz->iparam[SCRUB_TOTALDUR] = round(dz->param[SCRUB_TOTALDUR] * (double)(dz->infile->srate * dz->infile->channels));
	return(FINISHED);
}

/*************************** SCRUB_PREPROCESS **************************/

int scrub_preprocess(dataptr dz)
{
	int exit_status;
	double sr = (double)dz->infile->srate;
	int chans = dz->infile->channels;
   initrand48();
    if((exit_status = create_sintable(dz))<0)
		return(exit_status);
	dz->iparam[SCRUB_STARTRANGE] = round(dz->param[SCRUB_STARTRANGE] * sr);
	dz->iparam[SCRUB_ESTART]     = round(dz->param[SCRUB_ESTART]     * sr);
	dz->iparam[SCRUB_ENDRANGE]   = (dz->insams[0]/chans) - dz->iparam[SCRUB_ESTART];
    dz->param[SCRUB_SPEEDRANGE]  = dz->param[SCRUB_MAXSPEED] - dz->param[SCRUB_MINSPEED];
	return(FINISHED);
}

/*************************** CREATE_SINTABLE **************************/

int create_sintable(dataptr dz)
{   
	int n;
	/* GENERATE A WARPED SINUS TABLE, Range 0 - 1 */
	if((dz->parray[SCRUB_SIN] = (double *)malloc((SCRUB_SINTABSIZE+1) * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create sintable for scrubbing.\n");
		return(MEMORY_ERROR);
	}	
	for(n=0;n<SCRUB_SINTABSIZE;n++) {
		dz->parray[SCRUB_SIN][n] = sin(PI * (double)n/(double)SCRUB_SINTABSIZE);
		dz->parray[SCRUB_SIN][n] = pow(dz->parray[SCRUB_SIN][n],SCRUB_SINWARP);
	}
	dz->parray[SCRUB_SIN][n] = 0.0;
	if((dz->parray[SCRUB_READTAB] = (double *)malloc((SCRUB_SINTABSIZE+1) * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create skewtable for scrubbing.\n");
		return(MEMORY_ERROR);
	}	
	return(FINISHED);
}

/***************************CREATE_BUFFERS **************************/

int create_scrub_buffers(dataptr dz)
{
	size_t bigbufsize;
	long shsecsize = F_SECSIZE, sum = 0L;
	long framesize = F_SECSIZE * dz->infile->channels;
    bigbufsize = (size_t)Malloc(-1);
	bigbufsize /= sizeof(float);
    bigbufsize = (bigbufsize/(framesize*dz->bufcnt)) * framesize * dz->bufcnt;
    bigbufsize = bigbufsize/dz->bufcnt;
	if(bigbufsize <= 0)
		bigbufsize  = framesize;
    dz->buflen = (int) bigbufsize;
    if((dz->bigbuf = (float *)malloc(  
		   ((dz->buflen * dz->bufcnt) + (F_SECSIZE*2)) * sizeof(float))) == NULL) {
		sprintf(errstr, "INSUFFICIENT MEMORY for sound buffers.\n");
		return(MEMORY_ERROR);
	}
    dz->sampbuf[0] = dz->bigbuf;
    dz->sampbuf[1] = dz->sampbuf[0] + dz->buflen + (shsecsize*2);	  /* inbuf has wraparound sectors */
	dz->iparam[SCRUB_DROPOUT] = FALSE;
    if(dz->iparam[SCRUB_TOTALDUR] <= dz->buflen + shsecsize) {
		dz->buflen = dz->iparam[SCRUB_TOTALDUR];
		dz->iparam[SCRUB_DROPOUT] = TRUE;
    }
	dz->iparam[SCRUB_BUFCNT] = 0;
	while(sum < dz->insams[0]) {
		dz->iparam[SCRUB_BUFCNT]++;
		sum += dz->buflen;
	}
	return(FINISHED);
}

/****************************** DO_SCRUBBING *************************/

int do_scrubbing(dataptr dz)
{   
	int exit_status;
/*	float *ibuf = dz->sampbuf[0];*/
	float *obuf = dz->sampbuf[1];
	int chans = dz->infile->channels;
	int scrublen_in_src, effective_tablen;
	double skew, lospeed,speedrange;
	int direction = SCRUB_BACKWARDS, here, lasthere = 0, there = 0;
	display_virtual_time(0,dz);	/* fraction of total_dur!! */
	get_end(&direction,&lasthere,dz);	/* setup start position */		
	do {
		here = lasthere;
		get_end(&direction,&here,dz);		
		skew = get_skew();
		get_speed(&lospeed,&speedrange,dz);
		scrublen_in_src = abs(here - lasthere);
		effective_tablen = gen_readtab(skew,lospeed,speedrange,scrublen_in_src,dz);
		if((exit_status = scrub(&lasthere,&there,!direction,effective_tablen,dz))<0)
			return(exit_status);
		if(exit_status == FINISHED)
			break;
		if(dz->vflag[0])		//	FORWARD SCRUB ONLY
			break;
	} while((there * chans) + dz->total_samps_written < dz->iparam[SCRUB_TOTALDUR]);
	if(there > 0)
		return write_samps(obuf,there * chans,dz);
	return(FINISHED);
}

/******************************* GET_SKEW ******************************/

double get_skew(void)
{	double skew;
	skew = (drand48() * 2.0) - 1.0;  	/* -1 to +1 */
    skew *= SCRUB_SKEWRANGE;
    skew += 1.0;						/* 1 +- SKEWRANGE */ 
	return(skew);
}

/******************************* GET_SPEED ******************************
 *
 * Generate a speed distribution which is pitch (not frq) distributed.
 */

void get_speed(double *lospeed,double *speedrang,dataptr dz)
{
	double hispeed;				
	hispeed  = drand48() * dz->param[SCRUB_SPEEDRANGE] * SCRUB_RANGE_REDUCE_FACTOR;
	hispeed  = dz->param[SCRUB_MAXSPEED] - hispeed;
	*lospeed  = drand48() * dz->param[SCRUB_SPEEDRANGE] * SCRUB_RANGE_REDUCE_FACTOR;				
	*lospeed += dz->param[SCRUB_MINSPEED];
	*speedrang = hispeed - *lospeed;
}

/******************************* GET_END ******************************/

void get_end(int *direction,int *here,dataptr dz)
{   
	int lasthere = *here;
	switch(*direction) {
	case(SCRUB_FORWARDS):
		*here = round(drand48() * dz->iparam[SCRUB_ENDRANGE]);
		*here += dz->iparam[SCRUB_ESTART];
		*direction = SCRUB_BACKWARDS;
		break;
	case(SCRUB_BACKWARDS):
		*here = round(drand48() * dz->iparam[SCRUB_STARTRANGE]);
		*direction = SCRUB_FORWARDS;
		break;
	}
//TW COMMENT: this statement has no effect
	dz->param[SCRUB_LENGTH] = abs(lasthere - *here);
}

/***************************** GEN_READTAB *****************************/

int gen_readtab(double skew,double lospeed,double speedrange,int scrublen_in_src,dataptr dz)
{   
	int n, here, next, tablen = -1;
	double step, sin_index, frac, dindex;
	double *sintab  = dz->parray[SCRUB_SIN];
	double *readtab = dz->parray[SCRUB_READTAB];
	double readtab_integral = 0.0, last_readtab_integral, searchstep = 1.01;
	/* GENERATE A SKEWED WARPED-SINUS-TABLE, Range 0 - 1, from orig WARPED-SINUS-TABLE */ 

	if(flteq(skew,1.0)) {	
		memcpy((char *)readtab,(char *)sintab,(SCRUB_SINTABSIZE + 1) * sizeof(double));
		tablen = SCRUB_SINTABSIZE;
	} else {
		sin_index = 0.0;
		step =  (double)SCRUB_SINTABSIZE * (1.0-skew);		/* initial step found using */
		step /= (1.0 - pow(skew,(double)SCRUB_SINTABSIZE));	/* geometric series sum     */
		readtab[0] = 0.0;
		for(n=1;n<SCRUB_SINTABSIZE;n++) {
			sin_index += step;
			if((here = (int)sin_index) >= SCRUB_SINTABSIZE) {	   /* TRUNCATE */
				tablen = n;
				sprintf(errstr,"scrubbing table incomplete by %d items\n",SCRUB_SINTABSIZE - n);
				print_outwarning_flush(errstr);
				break;
			} else {
				frac = sin_index - (double)here;
				next = here + 1;
				readtab[n]  = (sintab[next] - sintab[here]) * frac;
				readtab[n] += sintab[here];
				step *= skew;
			}
		}
		if(tablen < 0)
			tablen = SCRUB_SINTABSIZE;
		readtab[tablen] = readtab[tablen-1];
	}	
	for(n=0;n<=tablen;n++) {
							/* range is initially 0 - 1 */
		readtab[n] *= speedrange;		/* change scale to range of speedrange */
		readtab[n] += lospeed;			/* shift values to be within range lospeed to hispeed */
		readtab[n] *= OCTAVES_PER_SEMITONE;	/* convert values to octaves */
		readtab[n]  = pow(2.0,readtab[n]);	/* convert vals to transposition ratios = increments in infile */
		readtab_integral += readtab[n];  	/* integrate func to get total length */
	}
	/* If the speedvary process causes sample-read to fall off end of file */
	/* move more quickly through the sample-read-incrs file */
	while(readtab_integral >= (double)scrublen_in_src) {
		last_readtab_integral = readtab_integral;
		dz->param[SCRUB_TABINC] = readtab_integral/(double)scrublen_in_src;
		dindex = 0.0;		
		readtab_integral = 0.0;
		while(dindex <= (double)tablen) {
			here = (int)floor(dindex);
			next = here+1;
			frac = dindex - (double)here;
			readtab_integral += (readtab[n] * (1.0 - frac)) + (readtab[next] * frac);
			dindex += dz->param[SCRUB_TABINC];
		}
		if(last_readtab_integral == readtab_integral) {
			/* if the search process stalls, kick restart it */
			readtab_integral *= searchstep;
			searchstep *= searchstep;
		}
	}
	/* in effect: change area under function by expanding or contracting it along x-axis */
	dz->param[SCRUB_TABINC] = readtab_integral/(double)scrublen_in_src;
	return tablen;
}

/**************************** SCRUB *******************************/

#define SCRUB_SPLICE (0.1)

int scrub(int *hhere,int *tthere,int direction,int effective_tablen,dataptr dz)
{   
	int exit_status, scrub_exit_status;
	int   index;
	double lfrac, inval;
	int    n, m;
	int    chans = dz->infile->channels;
	int   effective_buflen = dz->buflen/chans;
	int    abshere = *hhere, there = *tthere;
	int    here  = abshere % effective_buflen;
	int    bufno = abshere/effective_buflen;  /* TRUNCATE */
	int	   truehere, truethere;
	int   cnt = 0, splicend, scrubend = 0;
	double splincr, splval;
	double dindex, dhere = here, frac, val;
	float  *ibuf = dz->sampbuf[0];
	float  *obuf = dz->sampbuf[1];
	double *readtab = dz->parray[SCRUB_READTAB];

	if((exit_status = read_samps_for_scrub(bufno,dz))<0)  /* read wraparound buffer */
		return(exit_status);		
   	if(there >= effective_buflen) {
		if(dz->iparam[SCRUB_DROPOUT])
			return(FINISHED);
		if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
			return(exit_status);
		there = 0;
		truethere = there *chans;
	}
	for(m=0;m<chans;m++)
		obuf[(there*chans)+m] = 0.0f;

	splicend = (int)round((double)dz->infile->srate * SCRUB_SPLICE);
	dindex = dz->param[SCRUB_TABINC];
	while (dindex < (double)effective_tablen) {
		dindex += dz->param[SCRUB_TABINC];
		scrubend++;
	}
	scrubend -= splicend;
	if(scrubend < splicend) {
		scrubend += splicend;
		splicend = scrubend/2;
		scrubend -= splicend;
	}
	splincr = 1.0/(double)splicend;
	splval = splincr;

	dindex = dz->param[SCRUB_TABINC];
	there++;
	truethere = there *chans;
	do {
    	if(there >= effective_buflen) {
			if(dz->iparam[SCRUB_DROPOUT])
				return(FINISHED);
			if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
				return(exit_status);
			there = 0;
			truethere = there *chans;
		}
		index = (long)dindex;		/* TRUNCATE */ /* interpolate vals in readtable */
		frac  = dindex - (double)index;
		val   = (readtab[index+1] - readtab[index]) * frac;
		val  += readtab[index];
 		scrub_exit_status = CONTINUE;
		if(direction==SCRUB_FORWARDS) {
		    dhere += val;  /* use readtab val to interpolate in sndfile */
			here = (int)dhere;						/* TRUNCATE */
			truehere  = here * chans;
			if(truehere >= dz->ssampsread) {
				if(++bufno >= dz->iparam[SCRUB_BUFCNT]) {
					fprintf(stdout,"WARNING: overreached infile.\n");
					fflush(stdout);
		 			scrub_exit_status = FINISHED;
					break;
				}
				if((exit_status = read_samps_for_scrub(bufno,dz))<0)  /* read wraparound buffer */
					return(exit_status);		
				truehere -= dz->buflen;
				here  -= effective_buflen;
				dhere -= (double)effective_buflen;
			}
		} else {
		    dhere -= val;  
			here = (int)dhere;						/* TRUNCATE */
			truehere  = here * chans;
 			if(here < 0) {
				if(--bufno < 0) {
					fprintf(stdout,"WARNING: fell off start of infile.\n");
					fflush(stdout);
					scrub_exit_status = FINISHED;
					break;
				}
				if((exit_status = read_samps_for_scrub(bufno,dz))<0)  /* read wraparound buffer */
					return(exit_status);		
				truehere += dz->buflen;
				here  += effective_buflen;
				dhere += (double)effective_buflen;
			}
		}
		lfrac = dhere - (double)here;
		if (cnt > scrubend) {
			for(m=0;m<chans;m++) {
				inval =  ibuf[truehere+m] * (1.0 - lfrac);
				inval += ibuf[truehere+m+chans] * lfrac;
				obuf[truethere+m] = (float)(inval * splval);
				splval -= splincr;
			}
		} else if(cnt < splicend) {
			for(m=0;m<chans;m++) {
				inval =  ibuf[truehere+m] * (1.0 - lfrac);
				inval += ibuf[truehere+m+chans] * lfrac;
				obuf[truethere+m] = (float)(inval * splval);
				splval += splincr;
			}
		} else {
			for(m=0;m<chans;m++) {
				inval =  ibuf[truehere+m] * (1.0 - lfrac);
				inval += ibuf[truehere+m+chans] * lfrac;
				obuf[truethere+m] = (float) /*round*/(inval);	
			}
		}
		there++;
		truethere = there *chans;
		cnt++;
	} while ((dindex += dz->param[SCRUB_TABINC]) < SCRUB_SINTABSIZE);

	for(n=0;n<SCRUB_SINTABSIZE;n++) {		/* create silent gap to avoid clicks */
		if(there >= effective_buflen) {
			if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
				return(exit_status);
			there = 0;
			truethere = 0;
		}
		for(m=0;m<chans;m++)
			obuf[truethere+m] = 0.0f;
		truethere += chans;
		there++;
    }
    *hhere  = (bufno * effective_buflen) + here;
	*tthere = there;
	return(scrub_exit_status);
}

/**************************** READ_SAMPS_FOR_SCRUB *******************************/

int read_samps_for_scrub(int bufno,dataptr dz)
{
	int exit_status;
	float *buf = dz->sampbuf[0];
	if(sndseekEx(dz->ifd[0],bufno * dz->buflen,0)<0) {  
		sprintf(errstr,"sndseek() 1 failed in scrub.\n");		  /* seek to start of buffer from which to read */
		return(SYSTEM_ERROR);
	}
	dz->buflen += F_SECSIZE;	  /* samps to read, with F_SECSIZE  wraparound for interpd reading last sample in buf */
	/* zeroes buffer in case wraparound on last-samp-in-file !! */
	memset((char *)buf,0,(dz->buflen + F_SECSIZE) * sizeof(float));	
	/* zeros FURTHER F_SECSIZE samps in case file exactly fills the F_SECSIZE-extended buffer */
																   /* & there's  wraparound on last-samp-in-file !! */
	if((exit_status = read_samps(buf,dz))<0)  									  /* read wraparound buffer */
		return(exit_status);		
	dz->buflen -= F_SECSIZE;	 												  /* reset to unwrapped buffer size */
	if(dz->ssampsread > dz->buflen)		/* if read beyond effective bufsize, there are samps in file beyond this */
		dz->ssampsread = dz->buflen;	/* reset samps_read to non-wrapped samps read */
	return(FINISHED);
}

/*************************** CROSSMOD_PCONSISTENCY **************************/

int crossmod_pconsistency(dataptr dz)
{
	if(dz->infile->srate != dz->otherfile->srate) {
		sprintf(errstr,"Cross modulation only works with files of same srate.\n");
		return(DATA_ERROR);
	}
	if((dz->otherfile->channels > 2 && dz->infile->channels != 1) || (dz->infile->channels > 2 && dz->otherfile->channels != 1)) {
		if(dz->infile->channels != dz->otherfile->channels) {
			sprintf(errstr,"With multichannel files, Cross modulation ONLY works if files have same number of channels OR if one file is mono.\n");
			return(DATA_ERROR);
		}
	}
	return(FINISHED);
}

/*************************** CREATE_CROSSMOD_BUFFERS **************************/
//TW replaced by global
//#define FSECSIZE (256)

int create_crossmod_buffers(dataptr dz)
{
	size_t bigbufsize;
	int framesize;
	int bufchunks;

	bufchunks  = dz->infile->channels;
	bufchunks += dz->otherfile->channels;
	bufchunks += max(dz->otherfile->channels,dz->infile->channels);
	bigbufsize = (size_t)Malloc(-1);
	dz->buflen = (int)(bigbufsize / sizeof(float));

//TW MODIFIED ....
	framesize = F_SECSIZE * bufchunks;
	if((dz->buflen  = (dz->buflen/framesize) * framesize)<=0)
		dz->buflen  = framesize;

	if((dz->bigbuf = (float *)malloc(dz->buflen * sizeof(float))) == NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
		return(PROGRAM_ERROR);
	}
	dz->buflen /= bufchunks;
	dz->sampbuf[0] = dz->bigbuf;
	dz->sampbuf[1] = dz->sampbuf[0] + (dz->buflen * dz->infile->channels);
	dz->sampbuf[2] = dz->sampbuf[1] + (dz->buflen * dz->otherfile->channels);
	return(FINISHED);
}

/**************************** RING_MODULATE *************************/

int ring_modulate(dataptr dz)
{
	int exit_status;
	double sr = (double)dz->infile->srate;
	int chans = dz->infile->channels;
	double thistime = 0.0;
	double this_pitch = 0.0, next_pitch, pitchstep = 0.0, thisfrq, tabstep, tabindex = 0.0, sinval;
	double tabconvertor = (double)RM_SINTABSIZE/sr;
	float  *ibuf = dz->sampbuf[0];
	int blokcnt =  0;
	long m,k,j,r;
	long total_stereosamp_cnt = 0;
	if(dz->brksize[RM_FRQ]) {
		if((exit_status = read_value_from_brktable(thistime,RM_FRQ,dz))<0)
			return(exit_status);
	}
	next_pitch = log10(dz->param[RM_FRQ]);
	while(dz->samps_left > 0)	{
		if((exit_status = read_samps(ibuf,dz))<0)
			return(exit_status);
		dz->ssampsread /= chans;
		for(m=0,k=0;m<dz->ssampsread;m++,k+=chans) {
			if(blokcnt<=0) {
				total_stereosamp_cnt += RING_MOD_BLOK;
				thistime = (double)total_stereosamp_cnt/sr;	/* reading 1 block ahead */
				if(dz->brksize[RM_FRQ]) {
					if((exit_status = read_value_from_brktable(thistime,RM_FRQ,dz))<0)
						return(exit_status);
				}
				this_pitch = next_pitch;
				next_pitch = log10(dz->param[RM_FRQ]);		
				pitchstep  = (next_pitch - this_pitch)/(double)RING_MOD_BLOK;
				blokcnt = RING_MOD_BLOK;
			}
			blokcnt--;
			thisfrq = pow(10.0,this_pitch);
			tabstep = thisfrq * tabconvertor;
			sinval = read_rmod_tab(tabindex,dz);
			for(j=0;j<chans;j++) {			
				r = k+j;
				ibuf[r] = (float)/*round*/((double)ibuf[r] * sinval);
			}
			tabindex += tabstep;	
			while(tabindex >= (double)RM_SINTABSIZE)
				tabindex  -= (double)RM_SINTABSIZE;
			this_pitch += pitchstep;
		}
		if(dz->ssampsread > 0) {
			if((exit_status = write_exact_samps(ibuf,dz->ssampsread * chans,dz))<0)
				return(exit_status);
		}
	}
	return(FINISHED);
}

/**************************** CROSS_MODULATE *************************/

int cross_modulate(dataptr dz)
{
	int exit_status;
	/*double sr = (double)dz->infile->srate;*/
	float  *ibuf1 = dz->sampbuf[0];
	float  *ibuf2 = dz->sampbuf[1];
	float  *obuf  = dz->sampbuf[2];
	int    chans1 = dz->infile->channels;
	int    chans2 = dz->otherfile->channels;
	int    maxchans = max(chans1,chans2);
	int   samps_read, samps_read1, samps_read2, samps_to_write;
	int   m,k,j,r;
	dz->tempsize = max(dz->insams[0],dz->insams[1]);
	display_virtual_time(0L,dz);
	for(;;)	{
		if((samps_read1 = fgetfbufEx(ibuf1,dz->buflen * chans1,dz->ifd[0],0))<0) {
			sprintf(errstr,"Failed to read sound from 1st file.\n");
			return(SYSTEM_ERROR);
		}
		if((samps_read2 = fgetfbufEx(ibuf2,dz->buflen * chans2,dz->ifd[1],0))<0) {
			sprintf(errstr,"Failed to read sound from 2nd file.\n");
			return(SYSTEM_ERROR);
		}
		if((samps_read = min(samps_read1/chans1,samps_read2/chans2))<=0)
			break;
		samps_to_write = samps_read * maxchans;
		dz->ssampsread = samps_read;
		
		for(m=0;m<dz->ssampsread;m++) {
			switch(chans1) {
			case(STEREO):						//	STEREO-STEREO
				switch(chans2) {
				case(STEREO):
					k = m * 2;
					for(j=0;j<STEREO;j++) {			
						r = k+j;
						obuf[r] = (float)(ibuf1[r] * ibuf2[r]);
					}
					break;										
				case(MONO):						//	STEREO-MONO
					k = m * 2;
					for(j=0;j<STEREO;j++) {			
						r = k+j;
						obuf[r] = (float)(ibuf1[r] * ibuf2[m]);
					}
					break;										
				}
				break;
			case(MONO):							// MONO-MULTICHANNEL
				k = m * chans2;
				for(j=0;j<chans2;j++) {			
					r = k+j;
					obuf[r] = (float)(ibuf1[m] * ibuf2[r]);
				}
				break;
			default:
				switch(chans2) {				// MULTICHANNEL-MONO
				case(MONO):
					k = m * chans1;
					for(j=0;j<chans1;j++) {			
						r = k+j;
						obuf[r] = (float)(ibuf1[r] * ibuf2[m]);
					}
					break;										
				default:						// MULTICHANNEL-MULTICHANNEL
					obuf[m] = (float)(ibuf1[m] * ibuf2[m]);
					break;
				}			//	OTHER POSSIBLE CHANNEL COMBOS ARE TRAPPED EARLIER
			}
		}
		if(samps_to_write==dz->buflen * maxchans) {
			if((exit_status = write_exact_samps(obuf,samps_to_write,dz))<0)
				return(exit_status);
		} else if(samps_to_write > 0) {
			if((exit_status = write_samps(obuf,samps_to_write,dz))<0)
				return(exit_status);
		}
	}
	dz->infile->channels = maxchans;
	return(FINISHED);
}

/**************************** READ_RMOD_TAB *************************/

double read_rmod_tab(double dtabindex,dataptr dz)
{
	double *sintab = dz->parray[RM_SINTAB];
	int    tabindex = (int)dtabindex;	/* TRUNCATE */
	double frac  = dtabindex - (double)tabindex;
	double loval =  sintab[tabindex];
	double hival = 	sintab[tabindex+1];
	double incr  =  (hival - loval) * frac;
	return(loval + incr);
}

/********************** CREATE_RM_SINTAB *************************/

int create_rm_sintab(dataptr dz)
{
	int n;
	double *sintab, sinsize = (double) RM_SINTABSIZE;
#ifdef NOTDEF
	double scaling = (double)MAXSHORT/(double)(MAXSHORT+2);	/* avoid clipping inverted max -ve sample */
#endif
	if((dz->parray[RM_SINTAB] = (double *)malloc((RM_SINTABSIZE+1) * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for sine rtable.\n");
		return(MEMORY_ERROR);
	}
	sintab = dz->parray[RM_SINTAB];
	for(n=0;n<RM_SINTABSIZE;n++)
		sintab[n] = sin(TWOPI * (double)n/sinsize) /* * scaling*/;
	sintab[n] = 0.0;
	return(FINISHED);
}

//TW UPDATE: NEW FUNCTIONS BELOW (converted to flotsams)
/********************** STACK_PREPROCESS *************************/

int stack_preprocess(dataptr dz)
{
	int n, k;
	double  step, sum, thisstep, mult;

	if(dz->itemcnt == 0) {		   /* value NOT in file */
		if((dz->parray[STACK_TRANS] = 
		(double *)realloc((char *)(dz->parray[STACK_TRANS]),dz->iparam[STACK_CNT] * sizeof(double)))==NULL) {
			sprintf(errstr,"Insufficient memory for storing stack transpositions.\n");
			return(MEMORY_ERROR);
		}
		thisstep = 1.0;
		mult = dz->parray[STACK_TRANS][0];
		if(mult > 1.0) {
			for(n=0;n<dz->iparam[STACK_CNT];n++) {
				dz->parray[STACK_TRANS][n] = thisstep;
				thisstep *= mult;
			}
		} else {
			for(n=dz->iparam[STACK_CNT] - 1;n>=0;n--) {	/* pitch highest track always at top */
				dz->parray[STACK_TRANS][n] = thisstep;
				thisstep *= mult;
			}
		}
		dz->itemcnt  = dz->iparam[STACK_CNT];
	} else if(dz->itemcnt != dz->iparam[STACK_CNT]) {
		fprintf(stdout,"WARNING: Count of transposition values in file (%d)\n",dz->itemcnt);
		fprintf(stdout,"WARNING: does not correspond to number of stack components entered as parameter (%d),\n",dz->iparam[STACK_CNT]);
		fprintf(stdout,"WARNING: Using %d as number of transpositions\n",dz->itemcnt);
		fflush(stdout);
		dz->iparam[STACK_CNT] = dz->itemcnt;
	}
	if((dz->parray[STACK_AMP] = (double *)malloc(dz->iparam[STACK_CNT] * sizeof(double)))==NULL) {
		sprintf(errstr,"Insufficient memory for storing stack amplitudes.\n");
		return(MEMORY_ERROR);
	}
	dz->param[STACK_MINTRANS] = HUGE;
	for(n=0;n<dz->iparam[STACK_CNT];n++)
		dz->param[STACK_MINTRANS] = min(dz->param[STACK_MINTRANS],dz->parray[STACK_TRANS][n]);
	dz->param[STACK_DUR]  = (double)round(dz->param[STACK_DUR] * 10)/10.0;	
	if(flteq(dz->param[STACK_DUR],1.0))
		dz->iparam[STACK_DUR] = 0;
	else if(flteq(dz->param[STACK_DUR],0.0)) {
		sprintf(errstr,"You have asked to make NONE of the output!!");
		return(USER_ERROR);
	} else {
		dz->param[STACK_DUR] /= dz->param[STACK_MINTRANS];
		dz->iparam[STACK_DUR] = round(dz->param[STACK_DUR] * (dz->insams[0]/dz->infile->channels)) * dz->infile->channels;
	}
	/* attack distance from start, in input, in grouped-samples */
	dz->iparam[STACK_OFFSET] = (long)round(dz->param[STACK_OFFSET] * dz->infile->srate);	
/* attack distance from start, in output of slowest transposition, in grouped-samples */
	if(dz->param[STACK_MINTRANS] < 1.0)
		dz->iparam[STACK_OFFSET] = (long)round(dz->iparam[STACK_OFFSET] * (1.0/dz->param[STACK_MINTRANS]));		

	k = dz->iparam[STACK_CNT] - 1;
	if(flteq(dz->param[STACK_LEAN],1.0)) {
		for(n=0;n<dz->iparam[STACK_CNT];n++)
			dz->parray[STACK_AMP][n] = dz->param[STACK_GAIN]/dz->iparam[STACK_CNT];
	} else { 
		dz->parray[STACK_AMP][0] = dz->param[STACK_LEAN];
		dz->parray[STACK_AMP][k] = 1.0;
		step = dz->parray[STACK_AMP][k] - dz->parray[STACK_AMP][0];
		step /= (double)k;
		for(n = 1; n < k; n++)
			dz->parray[STACK_AMP][n] = dz->parray[STACK_AMP][n-1] + step;
		sum = 0.0;
		for(n = 0; n < dz->iparam[STACK_CNT]; n++)
			sum += dz->parray[STACK_AMP][n];
		for(n = 0; n < dz->iparam[STACK_CNT]; n++) {
			dz->parray[STACK_AMP][n] /= sum;
			dz->parray[STACK_AMP][n] *= dz->param[STACK_GAIN];
			if(dz->vflag[STACK_SEE]) {
				fprintf(stdout,"INFO: stackitem[%d] level %lf\n",n,dz->parray[STACK_AMP][n]);
			}
		}
		if(dz->vflag[STACK_SEE])
			fflush(stdout);
	}
	return(FINISHED);
}

/********************** DO_STACK *************************/

int do_stack(dataptr dz)
{
	int exit_status;
//TW ADDED FOR MULTICHANNEL WORK
	int qq;
	float *ibuf = dz->sampbuf[0], *obuf = dz->sampbuf[1];
	int chans = dz->infile->channels;
	int bufsamplen = dz->buflen/chans;	/* buffer length in grouped-samples */
	int all_transpositions_active, n, nn, lastpos, *obufpos, max_obufpos;
	double *inbufpos, maxoutsamp;
	int *active_transpos;
	float *lastibufval;
	int *obufskip, *seeklen, *origseeklen, *offset, *eof_in_ibuf;

	if(dz->iparam[STACK_DUR] <= 0)
		dz->tempsize = (int)round(dz->insams[0] * (1.0/dz->param[STACK_MINTRANS]));
	else 
		dz->tempsize = dz->iparam[STACK_DUR];

	if((inbufpos = (double *)malloc(dz->iparam[STACK_CNT] * sizeof(double)))==NULL) {
		sprintf(errstr,"Insufficient memory for storing input buffer pointers.\n");
		return(MEMORY_ERROR);
	}
	if((obufpos = (int *)malloc(dz->iparam[STACK_CNT] * sizeof(int)))==NULL) {
		sprintf(errstr,"Insufficient memory for storing output buffer pointers.\n");
		return(MEMORY_ERROR);
	}
	if((active_transpos = (int *)malloc(dz->iparam[STACK_CNT] * sizeof(int)))==NULL) {
		sprintf(errstr,"Insufficient memory for storing transposition flags.\n");
		return(MEMORY_ERROR);
	}
	if((obufskip = (int *)malloc(dz->iparam[STACK_CNT] * sizeof(int)))==NULL) {
		sprintf(errstr,"Insufficient memory for storing seek values.\n");
		return(MEMORY_ERROR);
	}
	if((lastibufval = (float *)malloc(dz->iparam[STACK_CNT] * sizeof(float) * dz->infile->channels))==NULL) {
		sprintf(errstr,"Insufficient memory for storing last values in input bufs values.\n");
		return(MEMORY_ERROR);
	}
	if((seeklen = (int *)malloc(dz->iparam[STACK_CNT] * sizeof(int)))==NULL) {
		sprintf(errstr,"Insufficient memory for storing seek values.\n");
		return(MEMORY_ERROR);
	}
	if((origseeklen = (int *)malloc(dz->iparam[STACK_CNT] * sizeof(int)))==NULL) {
		sprintf(errstr,"Insufficient memory for storing original seek values.\n");
		return(MEMORY_ERROR);
	}
	if((offset = (int *)malloc(dz->iparam[STACK_CNT] * sizeof(int)))==NULL) {
		sprintf(errstr,"Insufficient memory for storing offset values.\n");
		return(MEMORY_ERROR);
	}
	if((eof_in_ibuf = (int *)malloc(dz->iparam[STACK_CNT] * sizeof(int)))==NULL) {
		sprintf(errstr,"Insufficient memory for storing end-of-file markers.\n");
		return(MEMORY_ERROR);
	}

	all_transpositions_active = 0;
	for(n=0;n<dz->iparam[STACK_CNT];n++) {
		all_transpositions_active |= (int) pow(2.0,n);	/* set all bitflags for active buffers */
	 	seeklen[n] = origseeklen[n] = 0;
		inbufpos[n] = 0.0;
	 	obufskip[n] = get_obuf_sampoffset(n,dz);
		eof_in_ibuf[n]  = dz->insams[0];
		lastibufval[n]  = 0.0f;
		active_transpos[n] = 1;
	}
	if(chans > 1) {
		while(n < dz->iparam[STACK_CNT] * chans)
			lastibufval[n++]  = 0.0f;
	}
	if(dz->vflag[1]) {

		/* GET MAX OUTPUT LEVEL, THEN ADJUST LATER */
		if(sloom) {
			fprintf(stdout,"INFO: Assessing Output Level.\n");
			fflush(stdout);
		}
		maxoutsamp = 0.0;
		do {
			memset((char *)obuf,0,dz->buflen * sizeof(float));	/* clear the output buffer */
			max_obufpos = 0;					  	/* clear count of outsamps written */

			for(n=0;n<dz->iparam[STACK_CNT];n++) {	/* For each transposition */

				if(!active_transpos[n])				/* if this transposition inactive, go to next transpos */
					continue;
				if(obufskip[n] >= dz->buflen) {		/* if more samps to skip than space in obuf */
					obufskip[n] -= dz->buflen;		/* decrement skip-value, ready for next outbuf, & go to next transpos */
					continue;
				}									/* get the current seek position for this transposition */
				sndseekEx(dz->ifd[0],seeklen[n],0);	/* go to current buffer in infile, for this transposition */
				obufpos[n] = obufskip[n];			/* set pointer to start in outputbuf for this write */

					/* THIS LOOP READS INPUT AND WRITES OUTPUT UNTIL outbuf FULL OR inbuf OR infile EXHAUSTED */
				do {
					memset((char *)ibuf,0,dz->buflen * sizeof(float));	/* clear the input buffer */
					if((exit_status = read_samps(ibuf,dz)) < 0) {
						sprintf(errstr,"Failed to read from input file.\n");
						return(PROGRAM_ERROR);
					}

						/* THIS LOOP WRITES OUTPUT UNTIL outbuf FULL OR inbuf OR infile EXHAUSTED */
					do {
						if((exit_status = transpos_and_add_to_obuf(n,&(obufpos[n]),&(inbufpos[n]),lastibufval,chans,dz)) < 0)
							return(exit_status);
						if(inbufpos[n] > (double)eof_in_ibuf[n]) {
							active_transpos[n] = 0;						/* break at end of input FILE */
							break;
						} 															
			 			if (inbufpos[n] > bufsamplen - 1) {				/* break at end of input BUFFER */

							inbufpos[n] -= (double)bufsamplen;			/* move ibufpointer to buf start */
							lastpos = dz->buflen - chans;				/* get last position in current buffer */
	// MODIFY FOR MULTICHANNEL WORK
							for(qq=0;qq<chans;qq++)
								lastibufval[n+(dz->iparam[STACK_CNT] * qq)] = ibuf[lastpos++];
							seeklen[n] += dz->buflen;					/* set seek position for next read, in case */					
																		/* we (later) break out during a WRITE-end */
																		
							if((eof_in_ibuf[n] -= dz->buflen) <= dz->buflen)	/* if EOF is in the next buffer */
								eof_in_ibuf[n] /= chans;				/* convert to a grouped-sample count */
							break;										/* break from here and READ MORE INFILE */
						}
					} while(obufpos[n] < dz->buflen);					/* break at end of OUTPUT BUFFER */

				} while(active_transpos[n] && (obufpos[n] < dz->buflen));	/* if inFILE or outBUF finished, break from loop */
				max_obufpos = max(max_obufpos,obufpos[n]);
				obufskip[n] = 0;									/* written outbuf we skipped into, so NO skip next time */
				if(!active_transpos[n])								/* if FILE finished, reset flag for outer loop */
					all_transpositions_active &= ~((int) pow(2.0,n));
			}
			if(max_obufpos > 0) {
				if(dz->iparam[STACK_DUR] > 0) {
					if(dz->total_samps_written + max_obufpos >= dz->iparam[STACK_DUR]) {
						max_obufpos = dz->iparam[STACK_DUR] - dz->total_samps_written;
						all_transpositions_active = 0;
					}
				}
				for(nn = 0; nn < max_obufpos; nn++) {
					if(fabs(obuf[nn]) > maxoutsamp)
						maxoutsamp = fabs(obuf[nn]);
				}
			}					
		} while(all_transpositions_active);
		
		
		if(flteq(maxoutsamp,0.0)) {
			sprintf(errstr,"NO SIGNIFICANT SIGNAL LEVEL GENERATED\n");
			return(DATA_ERROR);
		}
		for(n = 0; n < dz->iparam[STACK_CNT]; n++)
			dz->parray[STACK_AMP][n] *= (0.95/maxoutsamp);

		all_transpositions_active = 0;
		for(n=0;n<dz->iparam[STACK_CNT];n++) {
			all_transpositions_active |= (int)pow(2.0,n);	/* set all bitflags for active buffers */
	 		seeklen[n] = origseeklen[n] = 0;
			inbufpos[n] = 0.0;
	 		obufskip[n] = get_obuf_sampoffset(n,dz);
			eof_in_ibuf[n]  = dz->insams[0];
			lastibufval[n]  = 0.0f;
			active_transpos[n] = 1;
		}
		if(chans > 1) {
			while(n < dz->iparam[STACK_CNT] * chans)
				lastibufval[n++]  = 0.0f;
		}
		if(sloom) {
			fprintf(stdout,"INFO: Generating Output.\n");
			fflush(stdout);
		}
	}
	
		/* THIS LOOP REPEATS UNTIL ALL INPUT TRANSPOSITION PROCESSES ARE COMPLETED */

	do {
		memset((char *)obuf,0,dz->buflen * sizeof(float));	/* clear the output buffer */
		max_obufpos = 0;					  	/* clear count of outsamps written */

		for(n=0;n<dz->iparam[STACK_CNT];n++) {	/* For each transposition */

			if(!active_transpos[n])				/* if this transposition inactive, go to next transpos */
				continue;
			if(obufskip[n] >= dz->buflen) {		/* if more samps to skip than space in obuf */
				obufskip[n] -= dz->buflen;		/* decrement skip-value, ready for next outbuf, & go to next transpos */
				continue;
			}									/* get the current seek position for this transposition */
			sndseekEx(dz->ifd[0],seeklen[n],0);	/* go to current buffer in infile, for this transposition */
			obufpos[n] = obufskip[n];			/* set pointer to start in outputbuf for this write */

				/* THIS LOOP READS INPUT AND WRITES OUTPUT UNTIL outbuf FULL OR inbuf OR infile EXHAUSTED */
			do {
				memset((char *)ibuf,0,dz->buflen * sizeof(float));	/* clear the input buffer */
				if((exit_status = read_samps(ibuf,dz)) < 0) {
					sprintf(errstr,"Failed to read from input file.\n");
					return(PROGRAM_ERROR);
				}

					/* THIS LOOP WRITES OUTPUT UNTIL outbuf FULL OR inbuf OR infile EXHAUSTED */
				do {
					if((exit_status = transpos_and_add_to_obuf(n,&(obufpos[n]),&(inbufpos[n]),lastibufval,chans,dz)) < 0)
						return(exit_status);
					if(inbufpos[n] > (double)eof_in_ibuf[n]) {
						active_transpos[n] = 0;						/* break at end of input FILE */
						break;
					} 															
			 		if (inbufpos[n] > bufsamplen - 1) {				/* break at end of input BUFFER */

						inbufpos[n] -= (double)bufsamplen;			/* move ibufpointer to buf start */
						lastpos = dz->buflen - chans;				/* get last position in current buffer */
// MODIFY FOR MULTICHANNEL WORK
						for(qq=0;qq<chans;qq++)
							lastibufval[n+(dz->iparam[STACK_CNT] * qq)] = ibuf[lastpos++];
						seeklen[n] += dz->buflen;					/* set seek position for next read, in case */					
																	/* we (later) break out during a WRITE-end */
																	
						if((eof_in_ibuf[n] -= dz->buflen) <= dz->buflen)	/* if EOF is in the next buffer */
							eof_in_ibuf[n] /= chans;				/* convert to a grouped-sample count */
						break;										/* break from here and READ MORE INFILE */
					}
				} while(obufpos[n] < dz->buflen);					/* break at end of OUTPUT BUFFER */

			} while(active_transpos[n] && (obufpos[n] < dz->buflen));	/* if inFILE or outBUF finished, break from loop */
			max_obufpos = max(max_obufpos,obufpos[n]);
			obufskip[n] = 0;									/* written outbuf we skipped into, so NO skip next time */
			if(!active_transpos[n])								/* if FILE finished, reset flag for outer loop */
				all_transpositions_active &= ~((int)pow(2.0,n));
		}
		if(max_obufpos > 0) {
			if(dz->iparam[STACK_DUR] > 0) {
				if(dz->total_samps_written + max_obufpos >= dz->iparam[STACK_DUR]) {
					max_obufpos = dz->iparam[STACK_DUR] - dz->total_samps_written;
					all_transpositions_active = 0;
				}
			}
			if((exit_status = write_samps(obuf,max_obufpos,dz))<0)
				return(exit_status);
		}					
	} while(all_transpositions_active);
	return(FINISHED);
}

/********************** GET_OBUF_SAMPOFFSET *************************/

int get_obuf_sampoffset(int n,dataptr dz)
{
	double speed_ratio_vs_slowest, offset_frac;
	int skip;

	speed_ratio_vs_slowest = dz->parray[STACK_TRANS][n]/dz->param[STACK_MINTRANS];
	if(flteq(speed_ratio_vs_slowest,1.0))
		return 0;
	offset_frac = 1.0 - (1.0/speed_ratio_vs_slowest);
	skip = (int)round(dz->iparam[STACK_OFFSET] * offset_frac);
	return (skip * dz->infile->channels);

}

/********************** TRANSPOS_AND_ADD_TO_OBUF *************************/
//TW new version for multichannel work !!
int transpos_and_add_to_obuf(int n,int *obufpos,double *inbufpos,float *lastival,int chans,dataptr dz)
{
	float lastval, nextval, *ibuf = dz->sampbuf[0], *obuf = dz->sampbuf[1];
	int i_bufpos, o_bufpos = *obufpos, i_bufpos_base = 0;
	int k;
	double step, val;

	if(*inbufpos < -1.0) {
		sprintf(errstr,"Impossible input buffer position encountered\n");
		return(PROGRAM_ERROR);
	}
	if(*inbufpos < 0.0)	  				/* ibuf pointer points between lastval in last buffer and 1st in this buffer */
		step = 1.0 + (*inbufpos);		/* interp position between these */
	else {
		step = fmod(*inbufpos,1.0);
		i_bufpos_base = (int)floor(*inbufpos) * chans;
	}
	for(k=0;k<chans;k++) {
		if(*inbufpos < 0.0) {	  			/* ibuf pointer points between lastval in last buffer and 1st in this buffer */
			i_bufpos = k;					/* point to channel at start of this buffer */
			lastval = lastival[n + (k * dz->iparam[STACK_CNT])];	/* get channel-val from end of last bufer */
			nextval = ibuf[i_bufpos];	
		} else {
			i_bufpos = i_bufpos_base + k;
			lastval = ibuf[i_bufpos];
			nextval = ibuf[i_bufpos+chans];	
		}												/* add interpd val to val already in outbuf */
		val = lastval + ((nextval - lastval) * step);
		val *= dz->parray[STACK_AMP][n];			/* scaling by amplitude scalers */
		obuf[o_bufpos] = (float)(obuf[o_bufpos] + val);
		o_bufpos++;
	}
	*obufpos = o_bufpos;							/* advance normally in outbuf */
	*inbufpos += dz->parray[STACK_TRANS][n];		/* advance ibuf-pointer by transpos value */
	return(FINISHED);
}
