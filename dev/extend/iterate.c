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
#include <extend.h>
#include <cdpmain.h>
#include <sfsys.h>
#include <osbind.h>
#include <limits.h>



static int  read_the_input_snd(dataptr dz);
static int  new_read_the_input_snd(int samplen,int wrap,dataptr dz);
static void scale_input(dataptr dz);
static int get_next_writestart(int write_start,dataptr dz);

//TW UPDATED ALL THESE FUNCTIONS
//HAVE DONE FLOAT CONVERSIONS HERE
static int iterate(int cnt,int pass,double *gain,double *pshift,
				int write_end,int local_write_start,int inmsampsize,double level,double *maxsamp,int pstep,int iterating,dataptr dz);
static int iter(int cnt,int passno, double *gain,int local_write_start,int inmsampsize,double level,double *maxsamp,int iterating,dataptr dz);
static int iter_stereo(int cnt,int passno, double *gain,int local_write_start,int inmsampsize,double level,double *maxsamp,int iterating,dataptr dz);
static int iter_shift_interp(int cnt,int passno, double *gain,double *pshift,int local_write_start,int inmsampsize,double level,double *maxsamp,int pstep,int iterating,dataptr dz);
static int iter_shift_interp_stereo(int cnt,int passno, double *gain,double *pshift,int local_write_start,int inmsampsize,double level,double *maxsamp,int pstep,int iterating,dataptr dz);
static int fixa_iter(int cnt,int passno,double *gain,int local_write_start,int inmsampsize,double level,double *maxsamp,int iterating,dataptr dz);
static int fixa_iter_stereo(int cnt,int passno,double *gain,int local_write_start,int inmsampsize,double level,double *maxsamp,int iterating,dataptr dz);
static int fixa_iter_shift_interp(int cnt,int passno,double *gain,double *pshift,int local_write_start,int inmsampsize,double level,double *maxsamp,int pstep,int iterating,dataptr dz);
static int fixa_iter_shift_interp_stereo(int cnt,int passno,double *gain,double *pshift,int local_write_start,int inmsampsize,double level,double *maxsamp,int pstep,int iterating,dataptr dz);

static int new_iterate(int cnt,int pass,double *gain,double *pshift,
				int write_end,int local_write_start,int inmsampsize,double level,int pstep,int iterating,dataptr dz);
static int new_iter(int cnt,int passno, double *gain,int local_write_start,int inmsampsize,double level,int iterating,dataptr dz);
static int new_iter_stereo(int cnt,int passno, double *gain,int local_write_start,int inmsampsize,double level,int iterating,dataptr dz);
static int new_iter_shift_interp(int cnt,int passno, double *gain,double *pshift,int local_write_start,int inmsampsize,double level,int pstep,int iterating,dataptr dz);
static int new_iter_shift_interp_stereo(int cnt,int passno, double *gain,double *pshift,int local_write_start,int inmsampsize,double level,int pstep,int iterating,dataptr dz);
static int new_fixa_iter(int cnt,int passno,double *gain,int local_write_start,int inmsampsize,double level,int iterating,dataptr dz);
static int new_fixa_iter_stereo(int cnt,int passno,double *gain,int local_write_start,int inmsampsize,double level,int iterating,dataptr dz);
static int new_fixa_iter_shift_interp(int cnt,int passno,double *gain,double *pshift,int local_write_start,int inmsampsize,double level,int pstep,int iterating,dataptr dz);
static int new_fixa_iter_shift_interp_stereo(int cnt,int passno,double *gain,double *pshift,int local_write_start,int inmsampsize,double level,int pstep,int iterating,dataptr dz);

static double get_gain(dataptr dz);
static double get_pshift(dataptr dz);
static void do_endsplice(float *buf,int dur,int splicelen,double spliceincr,int chans);
static int do_iteration_extend(dataptr dz);

/****************************** DO_ITERATION *************************
 *
 * (1) First event is always copy of original.
 */

//TW COMPLETELY UPDATED FUNCTION : (flt-converted)
#define ACCEPTABLE_LEVEL 0.75

int do_iteration(dataptr dz)
{
	int    exit_status, iterating;
	int   write_end, tail, cnt, arraysize = BIGARRAY;
	float *tailend;
	int    bufs_written, finished;
	double level, thistime;
	int   out_sampdur = 0, inmsampsize;
	int   write_start, local_write_start;
	double one_over_sr = 1.0/(double)dz->infile->srate, maxsamp = 0.0;
	int    passno, is_penult = 0, pstep;
	int   k;
	double *gain, *pshift, gaingain = -1.0;
	int   *wstart /*,  chunkmsamps = 0, chunksampsize = 0 */ ;
	float	*orig_inbuf = dz->sampbuf[0];

	if(dz->process == ITERATE_EXTEND)
		return do_iteration_extend(dz);
	pstep = ITER_STEP;
	iterating = 1;
	if ((gain = (double *)malloc(arraysize * sizeof(double)))==NULL) {
		sprintf(errstr,"Insufficient memory to store gain values\n");
		return(MEMORY_ERROR);
	}
	if ((pshift = (double *)malloc(arraysize * sizeof(double)))==NULL) {
		sprintf(errstr,"Insufficient memory to store pitch shift values\n");
		return(MEMORY_ERROR);
	}
	if ((wstart = (int *)malloc(arraysize * sizeof(int)))==NULL) {
		sprintf(errstr,"Insufficient memory to store pitch shift values\n");
		return(MEMORY_ERROR);
	}
/*
	if(dz->process == ITERATE_EXTEND) {
		chunksampsize = dz->iparam[CHUNKEND] - dz->iparam[CHUNKSTART]; 
		chunkmsamps = chunksampsize/dz->infile->channels; 
	}
*/
	if(dz->mode==ITERATE_DUR)
		out_sampdur  = round(dz->param[ITER_DUR] * (double)dz->infile->srate) * dz->infile->channels;
	if(sloom) {
		switch(dz->mode) {
		case(ITERATE_DUR):		
			dz->tempsize = out_sampdur;		
			break;
		case(ITERATE_REPEATS):	
			dz->tempsize = dz->insams[0] * (dz->iparam[ITER_REPEATS]+1);	/* approx */
			break; 
		}
	}
	for(passno=0;passno<2;passno++) {
		is_penult = 0;
		cnt = 0;
		bufs_written = 0;
		write_start = 0;
		maxsamp = 0.0;
		memset((char *)dz->sampbuf[1],0,dz->buflen * sizeof(float));
		level = dz->param[ITER_FADE];
		sndseekEx(dz->ifd[0],0L,0);
		display_virtual_time(0L,dz);
		fflush(stdout);
		dz->sampbuf[0] = orig_inbuf;
		if(passno > 0) {
			print_outmessage_flush("Second pass, for greater level\n");
			dz->tempsize = dz->total_samps_written;
			dz->total_samps_written = 0;
			memset((char *)dz->sampbuf[0],0,(dz->sampbuf[3] - dz->sampbuf[0]) * sizeof(float));
		}
		if((exit_status = read_the_input_snd(dz))<0)
			return(exit_status);
		if(dz->iparam[ITER_DO_SCALE])								
			scale_input(dz);
		inmsampsize = dz->insams[0]/dz->infile->channels;	/* no. of 'stereo'-samps to process */
		/* 1 */
		local_write_start = 0;
		switch(dz->iparam[ITER_PROCESS]) {
		case(MONO):		      		
			iter(0,passno,gain,local_write_start,inmsampsize,level,&maxsamp,iterating,dz);		
			break;
		case(STEREO):	      		
			iter_stereo(0,passno,gain,local_write_start,inmsampsize,level,&maxsamp,iterating,dz);			 		
			break;
		case(MN_INTP_SHIFT):      	
			iter_shift_interp(0,passno,gain,pshift,local_write_start,inmsampsize,level,&maxsamp,pstep,iterating,dz);	 		
			break;
		case(ST_INTP_SHIFT):      	
			iter_shift_interp_stereo(0,passno,gain,pshift,local_write_start,inmsampsize,level,&maxsamp,pstep,iterating,dz);		
			break;
		case(FIXA_MONO):	      	
			fixa_iter(0,passno,gain,local_write_start,inmsampsize,level,&maxsamp,iterating,dz);			 		
			break;
		case(FIXA_STEREO):	      	
			fixa_iter_stereo(0,passno,gain,local_write_start,inmsampsize,level,&maxsamp,iterating,dz);		 		
			break;
		case(FIXA_MN_INTP_SHIFT): 	
			fixa_iter_shift_interp(0,passno,gain,pshift,local_write_start,inmsampsize,level,&maxsamp,pstep,iterating,dz); 		
			break;
		case(FIXA_ST_INTP_SHIFT): 	
			fixa_iter_shift_interp_stereo(0,passno,gain,pshift,local_write_start,inmsampsize,level,&maxsamp,pstep,iterating,dz); 
			break;
		}
		write_end   = dz->insams[0];
		thistime = 0.0;
		if((exit_status = read_values_from_all_existing_brktables(thistime,dz))<0)
			return(exit_status);
		if(dz->brksize[ITER_DELAY])
			dz->iparam[ITER_MSAMPDEL] = round(dz->param[ITER_DELAY] * (double)dz->infile->srate);
		if(passno==0)
			wstart[cnt] = get_next_writestart(write_start,dz);
		write_start = wstart[cnt];
		local_write_start = write_start;
		finished = FALSE;
		for(;;) {
			switch(dz->mode) {
			case(ITERATE_DUR):
				if(write_start >= out_sampdur)
					finished = TRUE;
				break;
			case(ITERATE_REPEATS):
				if(cnt >= dz->iparam[ITER_REPEATS])
					finished = TRUE;
				break;
			}
			if(finished)
				break;
			while(local_write_start >= dz->buflen) {
				if(passno > 0) {
					if((exit_status = write_samps(dz->sampbuf[1],dz->buflen,dz))<0)
						return(exit_status);
				}
				bufs_written++;
				tail = write_end - dz->buflen;
				memset((char *)dz->sampbuf[1],0,dz->buflen * sizeof(float));
				if(tail > 0) {
					memmove((char *)dz->sampbuf[1],(char *)dz->sampbuf[2],tail * sizeof(float));
					tailend = dz->sampbuf[1] + tail;
				} else
					tailend = dz->sampbuf[2];
				memset((char *)tailend,0,(dz->sampbuf[3] - tailend) * sizeof(float));
				local_write_start -= dz->buflen;
				write_end         -= dz->buflen;
			}
			cnt++;
			if((passno == 0) && (cnt >= arraysize)) {
				arraysize += BIGARRAY;
				if ((gain = (double *)realloc((char *)gain,arraysize * sizeof(double)))==NULL) {
					sprintf(errstr,"Insufficient memory to store gain values (2)\n");
					return(MEMORY_ERROR);
				}
				if ((pshift = (double *)realloc((char *)pshift,arraysize * sizeof(double)))==NULL) {
					sprintf(errstr,"Insufficient memory to store gain values (2)\n");
					return(MEMORY_ERROR);
				}
				if ((wstart = (int *)realloc((char *)wstart,arraysize * sizeof(int)))==NULL) {
					sprintf(errstr,"Insufficient memory to store gain values (2)\n");
					return(MEMORY_ERROR);
				}
			}
			thistime = ((dz->buflen * bufs_written) + local_write_start) * one_over_sr;
			
			if((exit_status = read_values_from_all_existing_brktables(thistime,dz))<0)
				return(exit_status);
			if(is_penult) {
				dz->param[ITER_PSCAT] = 0.0;
				dz->param[ITER_ASCAT] = 0.0;
			}
			if(dz->brksize[ITER_DELAY])
				dz->iparam[ITER_MSAMPDEL] = round(dz->param[ITER_DELAY] * (double)dz->infile->srate);
			write_end = iterate(cnt,passno,gain,pshift,write_end,local_write_start,inmsampsize,level,&maxsamp,pstep,iterating,dz);
			level *= dz->param[ITER_FADE];
			if(passno==0)
				wstart[cnt] = get_next_writestart(write_start,dz);
			write_start = wstart[cnt];
			local_write_start = write_start - (bufs_written * dz->buflen);
		}
		if(passno > 0) {
			if(write_end > 0) {
				if((exit_status = write_samps(dz->sampbuf[1],write_end,dz))<0)
					return(exit_status);
			}
		} else {
			if(maxsamp <= 0.0) {
				sprintf(errstr,"No significant signal level found");
				return(DATA_ERROR);
			}
			if(maxsamp < ACCEPTABLE_LEVEL || maxsamp > 0.99)
				gaingain = ACCEPTABLE_LEVEL/maxsamp;
			else
				gaingain = 1.0;
			switch(dz->iparam[ITER_PROCESS]) {
			case(MONO):		      		
			case(STEREO):	      		
			case(MN_INTP_SHIFT):      	
			case(ST_INTP_SHIFT):      	
				for(k=0;k<=cnt;k++)
					gain[k] *= gaingain;
				break;
			case(FIXA_MONO):	      	
			case(FIXA_STEREO):	      	
			case(FIXA_MN_INTP_SHIFT): 	
			case(FIXA_ST_INTP_SHIFT): 	
				for(k=0;k<=cnt;k++)
					gain[k] = gaingain;
				break;
			}
		}
	}
	return FINISHED;
}

/****************************** DO_ITERATION_EXTEND **************************/

//2012 TW COMPLETELY UPDATED FUNCTION : (Better iterate-extend)

int do_iteration_extend(dataptr dz)
{
	int    exit_status, iterating;
	int   write_end, tail, cnt, arraysize = BIGARRAY, remnant = 0;
	float *tailend;
	int    bufs_written, finished, units = 0;
	int   twoseccnt = dz->infile->srate * 2;
	double level, thistime, localmax=0.0;
	int   out_sampdur = 0, inmsampsize;
	int   write_start, local_write_start;
	double srate = (double)dz->infile->srate, one_over_sr = 1.0/srate, maxsamp = 0.0;
	int    passno, is_penult = 0, pstep;
	int   k, orig_sampdel = 0, ii, jj, kk, n;
	double *gain, *pshift, gaingain = -1.0;
	int   *wstart, /* chunkmsamps = 0, */ chunksampsize = 0;
	int splicelen = ITX_SPLICELEN;
	double spliceincr = 1.0/(double)ITX_SPLICELEN, splicer;
	float *ibuf = dz->sampbuf[0], *obuf = dz->sampbuf[1], *ovflw = dz->sampbuf[2], *chunkbuf = dz->sampbuf[3];
	int chans = dz->infile->channels;

	pstep = ITER_SSTEP;
	iterating = 0;
	if ((gain = (double *)malloc(arraysize * sizeof(double)))==NULL) {
		sprintf(errstr,"Insufficient memory to store gain values\n");
		return(MEMORY_ERROR);
	}
	if ((pshift = (double *)malloc(arraysize * sizeof(double)))==NULL) {
		sprintf(errstr,"Insufficient memory to store pitch shift values\n");
		return(MEMORY_ERROR);
	}
	if ((wstart = (int *)malloc(arraysize * sizeof(int)))==NULL) {
		sprintf(errstr,"Insufficient memory to store pitch shift values\n");
		return(MEMORY_ERROR);
	}
	chunksampsize = dz->iparam[CHUNKEND] - dz->iparam[CHUNKSTART];
	/* chunkmsamps = chunksampsize/chans; */
	if(dz->mode==ITERATE_DUR)
		out_sampdur  = round(dz->param[ITER_DUR] * srate) * chans;
	if(sloom) {
		switch(dz->mode) {
		case(ITERATE_DUR):		
			dz->tempsize = out_sampdur;		
			break;
		case(ITERATE_REPEATS):	
			dz->tempsize = dz->insams[0] + (chunksampsize * dz->iparam[ITER_REPEATS]);	/* approx */
			break; 
		}
	}
	if(dz->mode == ITERATE_DUR)
		out_sampdur -= dz->insams[0] - dz->iparam[CHUNKEND];
	for(passno=0;passno<2;passno++) {
		is_penult = 0;
		cnt = 0;
		bufs_written = 0;
		write_start = 0;
		maxsamp = 0.0;
		memset((char *)obuf,0,dz->buflen * sizeof(float));
		level = 1.0;
		localmax = 0.0;
		if(passno > 0) {
			print_outmessage_flush("Second pass, for correct level\n");
			memset((char *)ibuf,0,(chunkbuf - ibuf) * sizeof(float));
		}
		sndseekEx(dz->ifd[0],0L,0);
		reset_filedata_counters(dz);
		display_virtual_time(0L,dz);
		fflush(stdout);

		if(passno == 0) {
			sndseekEx(dz->ifd[0],dz->iparam[CHUNKSTART],0);
			dz->total_samps_read = dz->iparam[CHUNKSTART];
			dz->samps_left = dz->insams[0] - dz->total_samps_read;
			dz->total_samps_written = dz->iparam[CHUNKSTART]; //For accountancy for local_write_start
			/* Make bakup copy of chunk to iterate, with splice at start */
			if((exit_status = new_read_the_input_snd(dz->buflen,0,dz))<0)
				return(exit_status);
			for(jj = 0; jj < chunksampsize;jj++)
				localmax = max(localmax,fabs(ibuf[jj]));
			if(localmax <= 0.0) {
				sprintf(errstr,"No significant signal found in frozen segment");
				return(DATA_ERROR);
			}
			memcpy((char *)chunkbuf,(char *)ibuf,(chunksampsize * sizeof(float)));
			jj = 0;
			splicer = 0.0;
			for(kk = 0; kk < splicelen; kk++) {
				for(ii=0; ii <chans;ii++) {
					chunkbuf[jj] = (float)(chunkbuf[jj] * splicer);
					jj++;
				}
				splicer += spliceincr;
			}
		} else {
			if(dz->iparam[CHUNKSTART] == 0) {
				if((exit_status = new_read_the_input_snd(chunksampsize,1,dz))<0)
					return(exit_status);
			} else {
				while(dz->total_samps_read < dz->iparam[CHUNKSTART]) {
					if((exit_status = new_read_the_input_snd(dz->buflen,0,dz))<0)
						return(exit_status);
					if(dz->total_samps_read < dz->iparam[CHUNKSTART]) {
						for(ii =0; ii < dz->buflen;ii++)
							ibuf[ii] = (float)(ibuf[ii] * gaingain);
						if((exit_status = write_samps(ibuf,dz->buflen,dz))<0)
							return(exit_status);
						write_end = dz->total_samps_written;
					} else {
						remnant = dz->iparam[CHUNKSTART] - (dz->total_samps_read - dz->ssampsread);
						if(remnant > 0) {
							for(ii =0; ii < remnant;ii++)
								ibuf[ii] = (float)(ibuf[ii] * gaingain);
							if((exit_status = write_samps(ibuf,remnant,dz))<0)
								return(exit_status);
						}
						memset((char *)ibuf,0,dz->buflen * sizeof(float));
						write_end = dz->total_samps_written;
						sndseekEx(dz->ifd[0],dz->iparam[CHUNKSTART],0);
						dz->total_samps_read = dz->iparam[CHUNKSTART];
						dz->samps_left = dz->insams[0] - dz->total_samps_read;
						if((exit_status = new_read_the_input_snd(chunksampsize,1,dz))<0)
							return(exit_status);
					}
				}
			}
		}
		/*Put splice on END of iterated chunk in TRUE buffer */
		jj = chunksampsize + chans - 1;
		for(ii=0; ii <chans;ii++) {
			ibuf[jj] = (float)0;		/* These are the wrap-around points */
			jj--;
		}
		splicer = 0.0;
		for(kk = 0; kk < splicelen; kk++) {
			for(ii=0; ii <chans;ii++) {
				ibuf[jj] = (float)(ibuf[jj] * splicer);
				jj--;
			}
			splicer += spliceincr;
		}
		inmsampsize = chunksampsize/chans;
		// copy this at its original level to output buffer (because it must match level of immediately previous sound)
		jj = 0;
		for(ii=0; ii < inmsampsize; ii++) {
			kk = ii*chans;
			for(n=0;n<chans;n++) {
				if(passno == 0)
					obuf[jj] = ibuf[kk];	//	Copy at original level, as this is start of iterated chunk, to be level-assessed
				else
					obuf[jj] = (float)(ibuf[kk] * gaingain);
				jj++;
				kk++;
			}
		}
		iterating = 1;
		
		write_end  = dz->total_samps_written + chunksampsize;
		thistime   = (double)((dz->total_samps_read - chunksampsize)/chans)/srate;
		jj = 0;		/* Put splice on START of iterated chunk */
		splicer = 0.0;
		for(kk = 0; kk < splicelen; kk++) {
			for(ii=0; ii <chans;ii++) {
				ibuf[jj] = (float)(ibuf[jj] * splicer);
				jj++;
			}
			splicer += spliceincr;
		}
		if((exit_status = read_values_from_all_existing_brktables(thistime,dz))<0)
			return(exit_status);
		if(dz->process == ITERATE_EXTEND) {
			orig_sampdel = round(dz->param[ITER_DELAY] * srate);
			dz->iparam[ITER_MSAMPDEL] = dz->iparam[CHUNKSTART]/chans + orig_sampdel;
			dz->iparam[ITER_MSAMPDEL] -= ITX_SPLICELEN * 2;
		}
		if(passno==0)
			wstart[cnt] = dz->iparam[ITER_MSAMPDEL] * chans;

		write_start = wstart[cnt];
		local_write_start = write_start - dz->iparam[CHUNKSTART];
		finished = FALSE;
		for(;;) {
			if(is_penult) {			/* If previously got to end, break */
				/* If last repeat, get orig material, with splice at start */
				memcpy((char *)ibuf,(char *)chunkbuf,chunksampsize * sizeof(float));
				for(jj = 0, k = local_write_start; jj < chunksampsize; jj++,k++) {			//	Write the final iteration
					if(k >= dz->buflen) {
						if(passno == 0) {
							for(ii =0; ii < dz->buflen;ii++)
								maxsamp = max(maxsamp,fabs(obuf[ii]));
						} else {
							for(ii =0; ii < dz->buflen;ii++)
								obuf[ii] = (float)(obuf[ii] * gaingain);
							if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
								return(exit_status);
						}
						bufs_written++;
						tail = write_end - dz->buflen;
						memset((char *)obuf,0,dz->buflen * sizeof(float));
						if(tail > 0) {
							memmove((char *)obuf,(char *)ovflw,tail * sizeof(float));
							tailend = obuf + tail;
						} else
							tailend = ovflw;
						memset((char *)tailend,0,(chunkbuf - tailend) * sizeof(float));
						local_write_start -= dz->buflen;
						write_end         -= dz->buflen;
						k = 0;
					}
					obuf[k] = (float)(obuf[k] + ibuf[jj]);
				}
				while(dz->samps_left > 0) {
					if((exit_status = new_read_the_input_snd(dz->buflen,0,dz))<0)
						return(exit_status);
					for(jj = 0; jj < dz->ssampsread; jj++,k++) {
						if(k >= dz->buflen) {
							if(passno == 0) {
								if(write_end > 0) {
									for(ii =0; ii < min(dz->buflen,write_end);ii++)
										maxsamp = max(maxsamp,fabs(obuf[ii]));
								}
							} else {
								for(ii =0; ii < dz->buflen;ii++)
									obuf[ii] = (float)(obuf[ii] * gaingain);
								if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
									return(exit_status);
							}
							bufs_written++;
							tail = write_end - dz->buflen;
							memset((char *)obuf,0,dz->buflen * sizeof(float));
							if(tail > 0) {
								memmove((char *)obuf,(char *)ovflw,tail * sizeof(float));
								tailend = obuf + tail;
							} else
								tailend = ovflw;
							memset((char *)tailend,0,(chunkbuf - tailend) * sizeof(float));
							local_write_start -= dz->buflen;
							write_end         -= dz->buflen;
							k = 0;
						}
						obuf[k] = ibuf[jj];
					}
				}
				write_end = max(k,write_end);
				finished = 1;
				break;
			}
			if(cnt==0) {
				if(!dz->brksize[ITER_DELAY])
					dz->iparam[ITER_MSAMPDEL]= orig_sampdel;
			}
			/* Zet to normal iteration delay  for 2nd Iteration: */
			iterating = 1;
			
			switch(dz->mode) {
			case(ITERATE_DUR):
				if(write_start >= out_sampdur)
					is_penult = 1;
				break;
			case(ITERATE_REPEATS):
				if(cnt >= dz->iparam[ITER_REPEATS] - 1)
					is_penult = 1;
				break;
			}
			if(finished)
				break;
			while(local_write_start >= dz->buflen) {
				if(passno > 0) {
					for(ii = 0;ii < dz->buflen;ii++)
						obuf[ii] = (float)(obuf[ii] * gaingain);
					if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
						return(exit_status);
				} else {
					for(ii=0;ii<dz->buflen;ii++)
						maxsamp = max(maxsamp,fabs(obuf[ii]));
					dz->total_samps_written += dz->buflen; // Accountancy for local_write_start
					if(sloom) {
						if(dz->total_samps_written > twoseccnt * units) {
							fprintf(stdout,"Processed %d secs\n",units * 10);
							fflush(stdout);
							units++;
						}
					}
				}
				bufs_written++;
				tail = write_end - dz->buflen;
				memset((char *)obuf,0,dz->buflen * sizeof(float));
				if(tail > 0) {
					memmove((char *)obuf,(char *)ovflw,tail * sizeof(float));
					tailend = obuf + tail;
				} else
					tailend = ovflw;
				memset((char *)tailend,0,(chunkbuf - tailend) * sizeof(float));
				local_write_start -= dz->buflen;
				write_end         -= dz->buflen;
			}
			cnt++;
			if((passno == 0) && (cnt >= arraysize)) {
				arraysize += BIGARRAY;
				if ((gain = (double *)realloc((char *)gain,arraysize * sizeof(double)))==NULL) {
					sprintf(errstr,"Insufficient memory to store gain values (2)\n");
					return(MEMORY_ERROR);
				}
				if ((pshift = (double *)realloc((char *)pshift,arraysize * sizeof(double)))==NULL) {
					sprintf(errstr,"Insufficient memory to store gain values (2)\n");
					return(MEMORY_ERROR);
				}
				if ((wstart = (int *)realloc((char *)wstart,arraysize * sizeof(int)))==NULL) {
					sprintf(errstr,"Insufficient memory to store gain values (2)\n");
					return(MEMORY_ERROR);
				}
			}
			thistime = ((dz->buflen * bufs_written) + local_write_start) * one_over_sr;
			
			if((exit_status = read_values_from_all_existing_brktables(thistime,dz))<0)
				return(exit_status);
			if(is_penult) {
				dz->param[ITER_PSCAT] = 0.0;
				dz->param[ITER_ASCAT] = 0.0;
				continue;				//	We will write the final segment at original level, as part of tail of sound: no iteration pass here
			}
			if(dz->brksize[ITER_DELAY])
				dz->iparam[ITER_MSAMPDEL] = round(dz->param[ITER_DELAY] * srate);
			write_end = new_iterate(cnt,passno,gain,pshift,write_end,local_write_start,inmsampsize,level,pstep,iterating,dz);
			if(passno==0)
				wstart[cnt] = get_next_writestart(write_start,dz);
			write_start = wstart[cnt];
			local_write_start = write_start - dz->total_samps_written;
		}
		if(passno > 0) {
			if(write_end > 0) {
				for(ii =0; ii < write_end;ii++)
					obuf[ii] = (float)(obuf[ii] * gaingain);
				if((exit_status = write_samps(obuf,write_end,dz))<0)
					return(exit_status);
			}
		} else {
			if(write_end > 0) {
				for(ii =0; ii < write_end;ii++)
					maxsamp = max(maxsamp,fabs(obuf[ii]));
			}
			gaingain = (localmax/maxsamp) * dz->param[ITER_LGAIN];
		}
	}
	return FINISHED;
}

/*************************** READ_THE_INPUT_SND **************************/

int read_the_input_snd(dataptr dz)
{
	int samps, k, samps_read;
	int n;
	if((samps_read = fgetfbufEx(dz->sampbuf[0], dz->insams[0]/* + SECSIZE*/,dz->ifd[0],0)) <= 0) {
		sprintf(errstr,"Can't read bytes from input soundfile\n");
		if(samps_read<0)
			return(SYSTEM_ERROR);
		return(DATA_ERROR);
	}
	if(samps_read!=dz->insams[0]) {
		sprintf(errstr, "Failed to read all of source file. read_the_input_snd()\n");
		return(PROGRAM_ERROR);
	}
	samps = samps_read / dz->infile->channels;

	if(dz->vflag[IS_ITER_PSCAT]) {
		k = samps * dz->infile->channels;
		for(n=0;n<dz->infile->channels;n++) {
			dz->sampbuf[0][k] = (float)0;
			k++;		/* GUARD POINTS FOR INTERPOLATION */
		}
	}
	return(FINISHED);
}

/*************************** NEW_READ_THE_INPUT_SND **************************/

int new_read_the_input_snd(int samplen,int wrap,dataptr dz)
{
	int samps, k, samps_read;
	int n;
	if((samps_read = fgetfbufEx(dz->sampbuf[0], samplen/* + SECSIZE*/,dz->ifd[0],0)) <= 0) {
		sprintf(errstr,"Can't read bytes from input soundfile\n");
		if(samps_read<0)
			return(SYSTEM_ERROR);
		return(DATA_ERROR);
	}
	dz->total_samps_read += samps_read;
	dz->samps_left -= samps_read;
	dz->ssampsread = samps_read;
	if(wrap) {
		if(samps_read!=samplen) {
			sprintf(errstr, "Failed to all of the input sound. new_read_the_input_snd()\n");
			return(PROGRAM_ERROR);
		}
		samps = samps_read / dz->infile->channels;
		if(dz->param[ITER_PSCAT] > 0.0) {
			k = samps * dz->infile->channels;
			for(n=0;n<dz->infile->channels;n++) {
				dz->sampbuf[0][k] = (float)0;
				k++;		/* GUARD POINTS FOR INTERPOLATION */
			}
		}
	}
	return(FINISHED);
}

/******************************* SCALE_INPUT ****************************/

void scale_input(dataptr dz)
{
	int n;
	int end = dz->insams[0];
	if(dz->iparam[ITER_PROCESS]!=FIXA_MONO && dz->iparam[ITER_PROCESS]!=FIXA_STEREO)
		end = dz->insams[0] + dz->infile->channels;	/* ALLOW FOR GUARD POINTS */
	for(n=0; n < end; n++)
		dz->sampbuf[0][n] = (float)(dz->sampbuf[0][n] * dz->param[ITER_GAIN]);
}

/*************************** GET_NEXT_WRITESTART ****************************/

int get_next_writestart(int write_start,dataptr dz)
{
	int this_step;
	double d;  
	int mwrite_start = write_start/dz->infile->channels;
	if((dz->process == ITERATE_EXTEND && dz->param[ITER_RANDOM] > 0.0) 
	|| (dz->process != ITERATE_EXTEND && dz->vflag[IS_ITER_RAND])) {
		d = ((drand48() * 2.0) - 1.0) * dz->param[ITER_RANDOM];
		d += 1.0;
		this_step = (int)round((double)dz->iparam[ITER_MSAMPDEL] * d);
		mwrite_start += this_step;
	} else
		mwrite_start += dz->iparam[ITER_MSAMPDEL];
	write_start = mwrite_start * dz->infile->channels;
	return(write_start);
}    

/******************************* ITERATE *****************************/

//TW COMPLETELY UPDATED FUNCTION : (flt-converted)
int new_iterate
(int cnt,int pass,double *gain,double *pshift,int write_end,int local_write_start,
	int inmsampsize,double level,int pstep,int iterating,dataptr dz)
{
	int wr_end = 0;
	switch(dz->iparam[ITER_PROCESS]) {
	case(MONO):		      		
		wr_end = new_iter(cnt,pass,gain,local_write_start,inmsampsize,level,iterating,dz);							
		break;
	case(STEREO):	      		
		wr_end = new_iter_stereo(cnt,pass,gain,local_write_start,inmsampsize,level,iterating,dz);			 		
		break;
	case(MN_INTP_SHIFT):      	
		wr_end = new_iter_shift_interp(cnt,pass,gain,pshift,local_write_start,inmsampsize,level,pstep,iterating,dz);				
		break;
	case(ST_INTP_SHIFT):      	
		wr_end = new_iter_shift_interp_stereo(cnt,pass,gain,pshift,local_write_start,inmsampsize,level,pstep,iterating,dz);		
		break;
	case(FIXA_MONO):	      	
		wr_end = new_fixa_iter(cnt,pass,gain,local_write_start,inmsampsize,level,iterating,dz);			 			
		break;
	case(FIXA_STEREO):	      	
		wr_end = new_fixa_iter_stereo(cnt,pass,gain,local_write_start,inmsampsize,level,iterating,dz);		 		
		break;
	case(FIXA_MN_INTP_SHIFT): 	
		wr_end = new_fixa_iter_shift_interp(cnt,pass,gain,pshift,local_write_start,inmsampsize,level,pstep,iterating,dz); 		
		break;
	case(FIXA_ST_INTP_SHIFT): 	
		wr_end = new_fixa_iter_shift_interp_stereo(cnt,pass,gain,pshift,local_write_start,inmsampsize,level,pstep,iterating,dz); 
		break;
	}
	return max(wr_end,write_end);
}

/**************************** ITER ***************************/

//TW COMPLETELY UPDATED FUNCTION : (flt-converted)
int new_iter(int cnt,int passno, double *gain,int local_write_start,int inmsampsize,double level,int iterating,dataptr dz)
{
	register int i, j = local_write_start;
	float *outbuf = dz->sampbuf[1];
	float *inbuf  = dz->sampbuf[0];
	double z;
	double thisgain;
	if(passno == 0) {
		gain[cnt] = get_gain(dz);
		thisgain = gain[cnt];

		if(dz->process != ITERATE_EXTEND && dz->vflag[IS_ITER_FADE]) {
			for(i=0; i < inmsampsize; i++) {
				z = outbuf[j] + (inbuf[i] * thisgain * level);
				outbuf[j++] = (float)z;
			}
		} else {
			for(i=0; i < inmsampsize; i++) {
				if(iterating)
					z = outbuf[j] + (inbuf[i] * thisgain);
				else
					z = outbuf[j] + inbuf[i];
				outbuf[j++] = (float)z;
			}
		}
	} else {
		thisgain = gain[cnt];

		if(dz->process != ITERATE_EXTEND && dz->vflag[IS_ITER_FADE]) {
			for(i=0; i < inmsampsize; i++) {
				z = outbuf[j] + (inbuf[i] * thisgain * level);
				outbuf[j++] = (float)z;
			}
		} else {
			for(i=0; i < inmsampsize; i++) {
			if(iterating)
				z = outbuf[j] + (inbuf[i] * thisgain);
			else
				z = outbuf[j] + inbuf[i];
				outbuf[j++] = (float)z;
			}
		}
	}
	return(j);
}

/**************************** ITER_STEREO ***************************/

//TW COMPLETELY UPDATED FUNCTION : (flt-converted)
int new_iter_stereo(int cnt,int passno, double *gain,int local_write_start,int inmsampsize,double level,int iterating,dataptr dz)
{
	register int i, j = local_write_start, k;
	int n;
	float *outbuf = dz->sampbuf[1];
	float *inbuf  = dz->sampbuf[0];
	double z;
	double thisgain;
	if(passno == 0) {
		gain[cnt] = get_gain(dz);
		thisgain = gain[cnt];

		if(dz->process != ITERATE_EXTEND && dz->vflag[IS_ITER_FADE]) {
			for(i=0; i < inmsampsize; i++) {
				k = i*dz->infile->channels;
				for(n=0;n<dz->infile->channels;n++) {
					z = outbuf[j] + (inbuf[k++] * thisgain * level);
					outbuf[j++]  = (float)z;
				}
			}
		} else {
			for(i=0; i < inmsampsize; i++) {
				k = i*dz->infile->channels;
				for(n=0;n<dz->infile->channels;n++) {
					if(iterating)
						z = outbuf[j] + (inbuf[k++] * thisgain);
					else 
						z = outbuf[j] + inbuf[k++];
					outbuf[j++]  = (float)z;
				}
			}
		}
	} else {
		thisgain = gain[cnt];

		if(dz->process != ITERATE_EXTEND && dz->vflag[IS_ITER_FADE]) {
			for(i=0; i < inmsampsize; i++) {
				k = i*dz->infile->channels;
				for(n=0;n<dz->infile->channels;n++) {
					z = outbuf[j] + (inbuf[k++] * thisgain * level);
					outbuf[j++]  = (float)z;
				}
			}
		} else {
			for(i=0; i < inmsampsize; i++) {
				k = i*dz->infile->channels;
				for(n=0;n<dz->infile->channels;n++) {
					if(iterating)
						z = outbuf[j] + (inbuf[k++] * thisgain);
					else 
						z = outbuf[j] + inbuf[k++];
					outbuf[j++]  = (float)z;
				}
			}
		}
	}
	return(j);
}

/**************************** ITER_SHIFT_INTERP ***************************/

//TW COMPLETELY UPDATED FUNCTION : (flt-converted)
int new_iter_shift_interp(int cnt,int passno, double *gain,double *pshift,int local_write_start,
		int inmsampsize,double level,int pstep,int iterating,dataptr dz)
{
	register int i = 0, j = local_write_start;
	double d = 0.0, part = 0.0;
	float val, nextval, diff;
	float *outbuf = dz->sampbuf[1];
	float *inbuf  = dz->sampbuf[0];
	double z;
	double thisgain;
	if(passno == 0) {
		gain[cnt] = get_gain(dz);
		thisgain = gain[cnt];
		if(dz->process != ITERATE_EXTEND && dz->vflag[IS_ITER_FADE]) {
			while(i < inmsampsize) {
				val     = inbuf[i++];
				nextval = inbuf[i];
				diff    = nextval - val;
				z = val + ((double)diff * part);
				z = (z * thisgain * level);
				z += outbuf[j];
				outbuf[j++] = (float)z;
				d      += dz->param[pstep];
				i      = (int)d; 			/* TRUNCATE */
				part   = d - (double)i; 
			}
		} else {
			while(i < inmsampsize) {
				val     = inbuf[i++];
				nextval = inbuf[i];
				diff    = nextval - val;
				z = val + ((double)diff * part);
				if(iterating)
					z = (z * thisgain);
				z += outbuf[j];
				outbuf[j++] = (float)z;
				d      += dz->param[pstep];
				i      = (int)d; 			/* TRUNCATE */
				part   = d - (double)i; 
			}
		}
		pshift[cnt] = get_pshift(dz);
		dz->param[pstep] = pshift[cnt];
	} else {
		thisgain = gain[cnt];
		if(dz->process != ITERATE_EXTEND && dz->vflag[IS_ITER_FADE]) {
			while(i < inmsampsize) {
				val     = inbuf[i++];
				nextval = inbuf[i];
				diff    = nextval - val;
				z = val + ((double)diff * part);
				z = (z * thisgain * level);
				z += outbuf[j];
				outbuf[j++] = (float)z;
				d      += dz->param[pstep];
				i      = (int)d; 			/* TRUNCATE */
				part   = d - (double)i; 
			}
		} else {
			while(i < inmsampsize) {
				val     = inbuf[i++];
				nextval = inbuf[i];
				diff    = nextval - val;
				z = val + ((double)diff * part);
				if(iterating)
					z = (z * thisgain);
				z += outbuf[j];
				outbuf[j++] = (float)z;
				d      += dz->param[pstep];
				i      = (int)d; 			/* TRUNCATE */
				part   = d - (double)i; 
			}
		}
		dz->param[pstep] = pshift[cnt];
	}
	return(j);
}

/*********************** ITER_SHIFT_INTERP_STEREO *************************/

//TW COMPLETELY UPDATED FUNCTION : (flt-converted)
int new_iter_shift_interp_stereo(int cnt,int passno, double *gain,double *pshift,int local_write_start,
		int inmsampsize,double level,int pstep,int iterating,dataptr dz)
{
	register int i = 0, j = local_write_start, k;
	int n;
	double d = 0.0, part = 0.0;
	float val, nextval, diff;
	float *outbuf = dz->sampbuf[1];
	float *inbuf  = dz->sampbuf[0];
	double z;
	double thisgain;
	if(passno == 0) {
		gain[cnt] = get_gain(dz);
		thisgain = gain[cnt];

		if(dz->process != ITERATE_EXTEND && dz->vflag[IS_ITER_FADE]) {
			while(i < inmsampsize) {
				k = i*dz->infile->channels;
				for(n=0;n<dz->infile->channels;n++) {
					val     = inbuf[k];
					nextval = inbuf[k+dz->infile->channels];
					diff    = nextval - val;
					z = val + ((double)diff * part);
					z = (z * thisgain * level);
					z += outbuf[j];
					outbuf[j++] = (float)z;
					k++;
				}
				d   += dz->param[pstep];
				i    = (int)d; 			/* TRUNCATE */
				part = d - (double)i; 
			}
		} else {
			while(i < inmsampsize) {
				k = i*dz->infile->channels;
				for(n=0;n<dz->infile->channels;n++) {
					val     = inbuf[k];
					nextval = inbuf[k+dz->infile->channels];
					diff    = nextval - val;
					z = val + ((double)diff * part);
					if(iterating)
						z = (z * thisgain);
					z += outbuf[j];
					outbuf[j++] = (float)z;
					k++;
				}
				d   += dz->param[pstep];
				i    = (int)d; 			/* TRUNCATE */
				part = d - (double)i; 
			}
		}
		pshift[cnt] = get_pshift(dz);
		dz->param[pstep] = pshift[cnt];
	} else {
		thisgain = gain[cnt];

		if(dz->process != ITERATE_EXTEND && dz->vflag[IS_ITER_FADE]) {
			while(i < inmsampsize) {
				k = i*dz->infile->channels;
				for(n=0;n<dz->infile->channels;n++) {
					val     = inbuf[k];
					nextval = inbuf[k+dz->infile->channels];
					diff    = nextval - val;
					z = val + ((double)diff * part);
					z = (z * thisgain * level);
					z += outbuf[j];
					outbuf[j++] = (float)z;
					k++;
				}
				d   += dz->param[pstep];
				i    = (int)d; 			/* TRUNCATE */
				part = d - (double)i; 
			}
		} else {
			while(i < inmsampsize) {
				k = i*dz->infile->channels;
				for(n=0;n<dz->infile->channels;n++) {
					val     = inbuf[k];
					nextval = inbuf[k+dz->infile->channels];
					diff    = nextval - val;
					z = val + ((double)diff * part);
					if(iterating)
						z = (z * thisgain);
					z += outbuf[j];
					outbuf[j++] = (float)z;
					k++;
				}
				d   += dz->param[pstep];
				i    = (int)d; 			/* TRUNCATE */
				part = d - (double)i; 
			}
		}
		dz->param[pstep] = pshift[cnt];
	}
	return(j);
}

/**************************** FIXA_ITER ***************************/

//TW COMPLETELY UPDATED FUNCTION : (flt-converted)
int new_fixa_iter(int cnt,int passno,double *gain,int local_write_start,int inmsampsize,double level,int iterating,dataptr dz)
{
	register int i, j = local_write_start;
	float *outbuf = dz->sampbuf[1];
	float *inbuf  = dz->sampbuf[0];
	double z;

	if(passno ==0) {
		if(dz->process != ITERATE_EXTEND && dz->vflag[IS_ITER_FADE]) {
			for(i=0; i < inmsampsize; i++) {
				z = outbuf[j] + (inbuf[i] * level);
				outbuf[j]  = (float)z;
				j++;
			}
		} else {
			for(i=0; i < inmsampsize; i++) {
				z = outbuf[j] + inbuf[i];
				outbuf[j]  = (float)z;
				j++;
			}
		}
	} else {
		if(dz->process != ITERATE_EXTEND && dz->vflag[IS_ITER_FADE]) {
			for(i=0; i < inmsampsize; i++) {
				z = outbuf[j] + (inbuf[i] * level * gain[cnt]);
				outbuf[j]  = (float)z;
				j++;
			}
		} else if(dz->process == ITERATE_EXTEND) {
			for(i=0; i < inmsampsize; i++) {
				z = outbuf[j] + inbuf[i];
				outbuf[j]  = (float)z;
				j++;
			}
		} else {
			for(i=0; i < inmsampsize; i++) {
				if(iterating)
					z = outbuf[j] + (inbuf[i] * gain[cnt]);
				else
					z = outbuf[j] + inbuf[i];
				outbuf[j]  = (float)z;
				j++;
			}
		}
	}
	return(j);
}

/**************************** FIXA_ITER_STEREO ***************************/

//TW COMPLETELY UPDATED FUNCTION : (flt-converted)
int new_fixa_iter_stereo(int cnt,int passno,double *gain,int local_write_start,int inmsampsize,double level,int iterating,dataptr dz)
{
	register int i, j = local_write_start, k;
	int n;
	float *outbuf = dz->sampbuf[1];
	float *inbuf  = dz->sampbuf[0];
	double z;

	if(passno == 0) {
		if(dz->process != ITERATE_EXTEND && dz->vflag[IS_ITER_FADE]) {
			for(i=0; i < inmsampsize; i++) {
				k = i*dz->infile->channels;
				for(n=0;n<dz->infile->channels;n++) {
					z = outbuf[j] + (inbuf[k++] * level);
					outbuf[j]  = (float)z;
					j++;
				}
			}
		} else {
			for(i=0; i < inmsampsize; i++) {
				k = i*dz->infile->channels;
				for(n=0;n<dz->infile->channels;n++) {
					z = outbuf[j] + inbuf[k++];
					outbuf[j]  = (float)z;
					j++;
				}
			}
		}
 	} else {
		if(dz->process != ITERATE_EXTEND && dz->vflag[IS_ITER_FADE]) {
			for(i=0; i < inmsampsize; i++) {
				k = i*dz->infile->channels;
				for(n=0;n<dz->infile->channels;n++) {
					z = outbuf[j] + (inbuf[k++] * level * gain[cnt]);
					outbuf[j]  = (float)z;
					j++;
				}
			}
		} else if(dz->process == ITERATE_EXTEND) {
			for(i=0; i < inmsampsize; i++) {
				k = i*dz->infile->channels;
				for(n=0;n<dz->infile->channels;n++) {
					z = outbuf[j] + inbuf[k++];
					outbuf[j]  = (float)z;
					j++;
				}
			}
		} else {
			for(i=0; i < inmsampsize; i++) {
				k = i*dz->infile->channels;
				for(n=0;n<dz->infile->channels;n++) {
					if(iterating)
						z = outbuf[j] + (inbuf[k++] * gain[cnt]);
					else
						z = outbuf[j] + inbuf[k++];
					outbuf[j]  = (float)z;
					j++;
				}
			}
		}
	}
 	return(j);
}

/**************************** FIXA_ITER_SHIFT_INTERP ***************************/

//TW COMPLETELY UPDATED FUNCTION : (flt-converted)
int new_fixa_iter_shift_interp(int cnt,int passno,double *gain,double *pshift,int local_write_start,
		int inmsampsize,double level,int pstep,int iterating,dataptr dz)
{
	register int i = 0, j = local_write_start;
 	double d = 0.0, part = 0.0;
 	float val, nextval, diff;
 	float *outbuf = dz->sampbuf[1];
	float *inbuf  = dz->sampbuf[0];
	double z;

	if(passno == 0) {
		if(dz->process != ITERATE_EXTEND && dz->vflag[IS_ITER_FADE]) {
		 	while(i < inmsampsize) {
		 		val     = inbuf[i++];
		 		nextval = inbuf[i];
		 		diff    = nextval - val;
				z = val + ((double)diff * part);
				z = (z * level);
				z += outbuf[j];
				outbuf[j++] = (float)z;
		 		d      += dz->param[pstep];
		 		i      = (int)d; 			/* TRUNCATE */
		 		part   = d - (double)i; 
		 	}
		} else {
		 	while(i < inmsampsize) {
		 		val     = inbuf[i++];
		 		nextval = inbuf[i];
		 		diff    = nextval - val;
				z = val + ((double)diff * part);
				z += outbuf[j];
		 		outbuf[j++] = (float)z;
		 		d      += dz->param[pstep];
		 		i      = (int)d; 			/* TRUNCATE */
		 		part   = d - (double)i; 
		 	}
		}
		pshift[cnt] = get_pshift(dz);
	} else {
		if(dz->process != ITERATE_EXTEND && dz->vflag[IS_ITER_FADE]) {
		 	while(i < inmsampsize) {
		 		val     = inbuf[i++];
		 		nextval = inbuf[i];
		 		diff    = nextval - val;
				z = val + ((double)diff * part);
				z = (z * level * gain[cnt]);
				z += outbuf[j];
		 		outbuf[j++] = (float)z;
		 		d      += dz->param[pstep];
		 		i      = (int)d; 			/* TRUNCATE */
		 		part   = d - (double)i; 
		 	}
		} else if(dz->process == ITERATE_EXTEND) {
		 	while(i < inmsampsize) {
		 		val     = inbuf[i++];
		 		nextval = inbuf[i];
		 		diff    = nextval - val;
				z = val + ((double)diff * part);
				z += outbuf[j];
		 		outbuf[j++] = (float)z;
		 		d      += dz->param[pstep];
		 		i      = (int)d; 			/* TRUNCATE */
		 		part   = d - (double)i; 
		 	}
		} else {
		 	while(i < inmsampsize) {
		 		val     = inbuf[i++];
		 		nextval = inbuf[i];
		 		diff    = nextval - val;
				z = val + ((double)diff * part);
				if(iterating)
					z = (z * gain[cnt]);
				z += outbuf[j];
		 		outbuf[j++] = (float)z;
		 		d      += dz->param[pstep];
		 		i      = (int)d; 			/* TRUNCATE */
		 		part   = d - (double)i; 
		 	}
		}
	}
	dz->param[pstep] = pshift[cnt];
 	return(j);
}

/*********************** FIXA_ITER_SHIFT_INTERP_STEREO *************************/

//TW COMPLETELY UPDATED FUNCTION : (flt-converted)
int new_fixa_iter_shift_interp_stereo(int cnt,int passno,double *gain,double *pshift,int local_write_start,
		int inmsampsize,double level,int pstep,int iterating,dataptr dz)
{
	register int i = 0, j = local_write_start, k;
	int n;
	double d = 0.0, part = 0.0;
	float val, nextval, diff;
	float *outbuf = dz->sampbuf[1];
	float *inbuf  = dz->sampbuf[0];
	double z;

	if(passno == 0) {
		if(dz->process != ITERATE_EXTEND && dz->vflag[IS_ITER_FADE]) {
			while(i < inmsampsize) {
				k = i*dz->infile->channels;
				for(n=0;n<dz->infile->channels;n++) {
					val     = inbuf[k];
					nextval = inbuf[k+dz->infile->channels];
					diff    = nextval - val;
					z = val + ((double)diff * part);
					z = (z * level);
					z += outbuf[j];
					outbuf[j++] = (float)z;
					k++;
				}
				d   += dz->param[pstep];
				i    = (int)d; 			/* TRUNCATE */
				part = d - (double)i; 
			}
		} else {
			while(i < inmsampsize) {
				k = i*dz->infile->channels;
				for(n=0;n<dz->infile->channels;n++) {
					val     = inbuf[k];
					nextval = inbuf[k+dz->infile->channels];
					diff    = nextval - val;
					z = val + ((double)diff * part);
					z += outbuf[j];
					outbuf[j++] = (float)z;
					k++;
				}
				d   += dz->param[pstep];
				i    = (int)d; 			/* TRUNCATE */
				part = d - (double)i; 
			}
		}
		pshift[cnt] = get_pshift(dz);
	} else {
		if(dz->process != ITERATE_EXTEND && dz->vflag[IS_ITER_FADE]) {
			while(i < inmsampsize) {
				k = i*dz->infile->channels;
				for(n=0;n<dz->infile->channels;n++) {
					val     = inbuf[k];
					nextval = inbuf[k+dz->infile->channels];
					diff    = nextval - val;
					z = val + ((double)diff * part);
					z = (z * level * gain[cnt]);
					z += outbuf[j];
					outbuf[j++] = (float)z;
					k++;
				}
				d   += dz->param[pstep];
				i    = (int)d; 			/* TRUNCATE */
				part = d - (double)i; 
			}
		} else if(dz->process == ITERATE_EXTEND) {
			while(i < inmsampsize) {
				k = i*dz->infile->channels;
				for(n=0;n<dz->infile->channels;n++) {
					val     = inbuf[k];
					nextval = inbuf[k+dz->infile->channels];
					diff    = nextval - val;
					z = val + ((double)diff * part);
					z += outbuf[j];
					outbuf[j++] = (float)z;
					k++;
				}
				d   += dz->param[pstep];
				i    = (int)d; 			/* TRUNCATE */
				part = d - (double)i; 
			}
		} else {
			while(i < inmsampsize) {
				k = i*dz->infile->channels;
				for(n=0;n<dz->infile->channels;n++) {
					val     = inbuf[k];
					nextval = inbuf[k+dz->infile->channels];
					diff    = nextval - val;
					z = val + ((double)diff * part);
					if(iterating)
						z = (z * gain[cnt]);
					z += outbuf[j];
					outbuf[j++] = (float)z;
					k++;
				}
				d   += dz->param[pstep];
				i    = (int)d; 			/* TRUNCATE */
				part = d - (double)i; 
			}
		}
	}
	dz->param[pstep] = pshift[cnt];
	return(j);
}

/******************************** GET_GAIN *****************************/

double get_gain(dataptr dz)
{
	double scatter;
	double newlgain = 0.0;
	switch(dz->process) {
	case(ITERATE_EXTEND):
		newlgain = 1.0;
		if (dz->param[ITER_ASCAT] > 0.0) {
			scatter  = drand48() * dz->param[ITER_ASCAT];
			scatter  = 1.0 - scatter;
			newlgain = scatter;
		}
		break;
	case(ITERATE):
		newlgain = dz->param[ITER_GAIN];
		if(dz->vflag[IS_ITER_ASCAT]) {
			scatter  = drand48() * dz->param[ITER_ASCAT];
			scatter  = 1.0 - scatter;
			newlgain = scatter * (double)dz->param[ITER_GAIN];
		}
	}
	return(newlgain);
}

/******************************** GET_PSHIFT *****************************/

double get_pshift(dataptr dz)
{
	double scatter;
	scatter = (drand48() * 2.0) - 1.0;
	scatter *= dz->param[ITER_PSCAT];
	return(pow(2.0,scatter * OCTAVES_PER_SEMITONE));
}

/******************************** ACCENT_STREAM *****************************/

int accent_stream(dataptr dz) {
	int n = 0, thispos = 0, last_write_end = 0, oflo = 0, seccnt, s_read;
	float *bptr, zz;
	double maxzz = 0.0, minzz = 0.0;
	int overlap ,i, j, exit_status, t,v,m,done = 0;
	double *brrk, convertor, thisval, nextval, valincr, atten = 1.0;
	int nexttime, thistime;
	float *sbrk = dz->extrabuf[0];
	int *la  = dz->lparray[0];
	if(((seccnt = dz->insams[0]/F_SECSIZE) * F_SECSIZE) < dz->insams[0])
		seccnt++;
	if((dz->ssampsread = fgetfbufEx(sbrk,seccnt * F_SECSIZE,dz->ifd[0],0)) < 0) {
		sprintf(errstr,"Can't read samps from input soundfile.\n");
		return(SYSTEM_ERROR);
	}
	s_read = dz->ssampsread;
	for(n = 1;n < dz->itemcnt; n++) {
		if(la[n] - la[n-1] <= 0) {
			sprintf(errstr,"Entry times not in ascending order. Cannot proceed.\n");
			return(DATA_ERROR);
		}
	}
	if(dz->brksize[ACC_ATTEN]) {
		if(dz->brksize[ACC_ATTEN]<2) {
			sprintf(errstr,"Breaktable has only one value: Cannot proceed.\n");
			return(DATA_ERROR);
		}
		convertor = (double)(dz->infile->srate * dz->infile->channels);
		brrk = dz->brk[ACC_ATTEN];
		n = 0;
		nexttime = round(brrk[0] * convertor);
		nextval  = brrk[1];
		for(m = 1,t = 2,v = 3; m < dz->brksize[ACC_ATTEN]; m++,t+=2,v+=2) {
			thistime = nexttime;
			thisval  = nextval;
			nexttime = round(brrk[t] * convertor);
			nextval  = brrk[v];
			valincr = (nextval - thisval)/(double)(nexttime - thistime);
			if(nexttime >= s_read) {		/* IF brktable extends beyond end of sound */
				nexttime = s_read;
				done = 1;
			}
			atten = thisval;
			while(n < nexttime) {
				sbrk[n] = (float) /*round*/((double)sbrk[n] * atten);
				atten += valincr;
				n++;
			}
			if(done)				   	/* IF brktable extends beyond end of sound: break */
				break;
		}
		while(n < s_read) {				/* IF brktable stops short of end of sound */
			sbrk[n] = (float) /*round*/((double)sbrk[n] * atten);
			n++;
		}

	} else if(dz->param[ACC_ATTEN] < 1.0) {
		for(n=0;n<s_read;n++)
			sbrk[n] = (float)/*round*/((double)sbrk[n] * dz->param[ACC_ATTEN]);
	}
	dz->tempsize = la[dz->itemcnt-1];
	n = 0;
	memset((char *)dz->sampbuf[0],0,dz->buflen * sizeof(float));
	while(n < dz->itemcnt) {
		thispos = la[n] - dz->total_samps_written;
		if(thispos >= dz->buflen) {
			if((exit_status = write_samps(dz->sampbuf[0],dz->buflen,dz))<0)
				return(exit_status);
			memset((char *)dz->sampbuf[0],0,dz->buflen * sizeof(float));
			if(last_write_end > dz->buflen) {
				last_write_end -= dz->buflen;
				memcpy((char *)dz->sampbuf[0],(char *)(dz->sampbuf[0] + dz->buflen),last_write_end * sizeof(float));
			} else  {
				last_write_end = 0;
			}
		} else {
			bptr = dz->sampbuf[0] + thispos;
			if((overlap = last_write_end - thispos) <= 0) {
				memcpy((char *)bptr,(char *)sbrk,dz->insams[0]* sizeof(float));
				last_write_end = thispos + dz->insams[0];
			} else {
				for(i=0,j = thispos;i<overlap; i++,j++) {
					zz    = *bptr + sbrk[i];
					maxzz = max(zz,maxzz);
					minzz = min(zz,minzz);
					if(maxzz > F_MAXSAMP || minzz < F_MINSAMP) {
						if(!oflo) {
							fprintf(stdout,"WARNING: First overflow at event %d\n",n+1);
							fflush(stdout);
						}
						oflo++;				
					}
					*bptr = (float)zz;
					bptr++;
	   			}
				memcpy((char *)bptr,(char *)(sbrk + overlap),(dz->insams[0] - overlap) * sizeof(float));
				last_write_end = thispos + dz->insams[0]; 
			}
			n++;
		}
	}
	if(last_write_end > 0) {
		if((exit_status = write_samps(dz->sampbuf[0],last_write_end,dz))<0)
			return(exit_status);
	}
	if(oflo) {
		if(oflo > 1) {
			fprintf(stdout,"WARNING: there were %d overflows\n",oflo);
			fflush(stdout);
		} else {
			fprintf(stdout,"WARNING: there was %d overflow\n",oflo);
			fflush(stdout);
		}
	}
	return(FINISHED);
}

//TW NEW FUNCTION (converted for floats)
/******************************** DO_SEQUENCE *****************************/

int do_sequence(dataptr dz)
{
	int exit_status = FINISHED, warned = 0;
	double *d, transpos, time, level, kd, frac, sum;
	float *obufptr, *ibufptr, *tbufptr;
	int k, n, outlen, thissamp, samps_read, last_total_samps_written = 0, max_write = 0;
	int m, c;
	double thisval, nextval, diff, fracval, oval;
	int chans = dz->infile->channels, srate = dz->infile->srate;

	dz->total_samps_written = 0; 

	if((samps_read = fgetfbufEx(dz->sampbuf[0], dz->insams[0],dz->ifd[0],0)) <= 0) {
		sprintf(errstr,"Can't read bytes from input soundfile.\n");
		return(SYSTEM_ERROR);
	}
	for(d = dz->parray[0]; d < dz->parray[1]; d+=3) {
		tbufptr = dz->sampbuf[3], ibufptr = dz->sampbuf[0];
		time = *d;
		transpos = *(d+1);
		level    = *(d+2) * dz->param[SEQ_ATTEN];
		for(c = 0;c <chans;c++)
			*tbufptr++ = *ibufptr++;
		kd = 0.0;
		do {
			kd += transpos;
			k = (int)floor(kd);
			frac = kd - (double)k;
			k *= chans;
			if(k >= dz->insams[0] + chans)
				break;
			for(m=0;m<chans;m++) {
				thisval = ibufptr[k];
				nextval = ibufptr[k+chans];
				diff    = nextval - thisval;
				fracval = diff * frac;
				oval    = thisval + fracval;
				*tbufptr++ = (float)oval;
				k++;
			}
		} while(k < dz->insams[0] + chans);	/* allows for wraparound point(s) at end */
		outlen = tbufptr - dz->sampbuf[3];
		thissamp = ((int)round(time * srate) * chans) - last_total_samps_written;
		while(thissamp >= dz->buflen) {
			if((exit_status = write_samps(dz->sampbuf[1],dz->buflen,dz))<0)
				return(exit_status);
			last_total_samps_written = dz->total_samps_written;
			memcpy((char *)dz->sampbuf[1],(char *)dz->sampbuf[2],dz->buflen * sizeof(float));
			memset((char *)dz->sampbuf[2],0,dz->buflen * sizeof(float));
			max_write = 0;
			thissamp -= dz->buflen;
		}
		obufptr = dz->sampbuf[1] + thissamp;
		tbufptr = dz->sampbuf[3];
		for(n=0;n<outlen;n++) {
			sum = *obufptr + (*tbufptr * level);
			if(!warned && (sum >= F_MAXSAMP)) {
				print_outwarning_flush("OVERLOAD!!\n");
				warned = 1;
			}
			*obufptr = (float)sum;
			obufptr++;
			tbufptr++;
		}
		if((obufptr - dz->sampbuf[1]) > max_write)
			max_write = obufptr - dz->sampbuf[1];
	}
	if(max_write > 0)
		exit_status = write_samps(dz->sampbuf[1],max_write,dz);
	return(exit_status);
}

/******************************** DO_DOUBLING ********************************/
/*RWD*/
#define SAMPLE_T float

int do_doubling(dataptr dz)
{
	int exit_status, k, m;
	double now = 0.0, spliceincr, spliceratio, seg_advance = 0;
	int splicelen_samps, seg_samps = 0, samps_left, samps_remain, n;
	int chans = dz->infile->channels, finished = 0;
	SAMPLE_T *obuf = dz->sampbuf[1], *ibuf = dz->sampbuf[0], *sbuf = dz->sampbuf[2], *sbufend;
	SAMPLE_T *obufend = dz->sampbuf[1] + dz->buflen;
	SAMPLE_T *ibufend = dz->sampbuf[0] + dz->buflen, *dnsplic, *upsplic, val;
	SAMPLE_T *splicend, *splicstt;

	dz->tempsize = dz->insams[0];
	if(!dz->vflag[NO_TIME_EXPAND]) {
		for(k = 1; k < dz->iparam[SEG_REPETS]; k++) {
			if((dz->tempsize += dz->insams[0]) < 0) {
				dz->tempsize = INT_MAX;
				break;
			}
		}
	}
	splicelen_samps = round(SPLICEDUR * dz->infile->srate);
	if((splicend = (SAMPLE_T *)malloc(splicelen_samps * chans * sizeof(SAMPLE_T)))==NULL) {
		sprintf(errstr,"Insufficient Memory for end-splice buffer\n");
		return(MEMORY_ERROR);
	}
	if((splicstt = (SAMPLE_T *)malloc(splicelen_samps * chans * sizeof(SAMPLE_T)))==NULL) {
		sprintf(errstr,"Insufficient Memory for start-splice buffer\n");
		return(MEMORY_ERROR);
	}
	memset((char *)splicend,0,splicelen_samps * chans * sizeof(SAMPLE_T));
	memset((char *)splicstt,0,splicelen_samps * chans * sizeof(SAMPLE_T));
	spliceincr = 1.0/(double)splicelen_samps;
	if(dz->brksize[SEG_DUR] == 0) {
		dz->iparam[SEG_DUR] = round(dz->param[SEG_DUR] * dz->infile->srate);
		seg_advance = dz->param[SEG_DUR]  - SPLICEDUR;
		seg_samps   = dz->iparam[SEG_DUR] - splicelen_samps;
	}
	if((exit_status = read_samps(dz->sampbuf[0],dz))< 0) {
		sprintf(errstr,"Failure to read from input file\n");
		return(SYSTEM_ERROR);
	}
	ibufend = dz->sampbuf[0] + dz->ssampsread;
	samps_left = dz->ssampsread;
	while(samps_left > 0) {
		if(dz->brksize[SEG_DUR] > 0) {
			read_value_from_brktable(now,SEG_DUR, dz);
			dz->iparam[SEG_DUR] = round(dz->param[SEG_DUR] * dz->infile->srate);
			seg_advance = dz->param[SEG_DUR]  - SPLICEDUR;
			seg_samps   = dz->iparam[SEG_DUR] - splicelen_samps;
		}
		spliceratio = 0.0;
		sbuf = dz->sampbuf[2];
		/* ZERO THE SEGMENT BUFFER */
		memset((char *)sbuf,0,dz->iparam[SEGLEN] * sizeof(SAMPLE_T));
		dnsplic = splicend;
		/* Copy downsplice from previous segment into the segment buffer */
		for(n = 0; n < splicelen_samps; n++) {
			for(m = 0;m < chans;  m++)
				*sbuf++ = *dnsplic++;
		}
		sbuf = dz->sampbuf[2];
		/* get upsplice of new segment */
		upsplic = splicstt;
		for(n = 0; n < splicelen_samps; n++) {
			for(m = 0;m < chans;  m++) {
				val = (SAMPLE_T)(*ibuf * spliceratio);
		/* store it */
				*upsplic++ = val;
		/* and also add it to new-segment buffer */
				*sbuf = (SAMPLE_T)(val + *sbuf);
				sbuf++;
				ibuf++;
			}
			spliceratio += spliceincr;
			samps_left -= chans;
			if(samps_left <= 0) {
				if((exit_status = read_samps(dz->sampbuf[0],dz)) < 0) {
					sprintf(errstr,"Failure to read from input file\n");
					return(SYSTEM_ERROR);
				}
				if((samps_left = dz->ssampsread) == 0) {
					finished = 1;
					break;
				}
				ibuf = dz->sampbuf[0];
				ibufend = ibuf + dz->ssampsread;
			}
		}
		if(finished)
			break;
		/* Copy body of new sement to seg buffer */
		while(n < seg_samps) {
			for(m = 0;m < chans;  m++)
				*sbuf++ = *ibuf++;
			if((samps_left -= chans) <= 0) {
				if((exit_status = read_samps(dz->sampbuf[0],dz)) < 0) {
					sprintf(errstr,"Failure to read from input file\n");
					return(SYSTEM_ERROR);
				}
				if((samps_left = dz->ssampsread) == 0) {
					finished = 1;
					break;
				}
				ibuf = dz->sampbuf[0];
				ibufend = ibuf + dz->ssampsread;
			}
			n++;
		}
		if(finished)
			break;
		/* this is the COPYING-END of sbuf, prior to last splice */
		sbufend = sbuf;
		/* Do downsplice of new segment */
		spliceratio = 1.0;
		dnsplic = splicend;
		for(n = 0; n < splicelen_samps; n++) {
			spliceratio -= spliceincr;
			for(m = 0;m < chans;  m++) {
				val = (SAMPLE_T)(*ibuf * spliceratio);
		/* and store in new-seg buffer */
				*sbuf++    = val;
		/* and in downsplice buffer */
				*dnsplic++ = val;
				ibuf++;

			}
			if((samps_left -= chans) <= 0) {
				if((exit_status = read_samps(dz->sampbuf[0],dz)) < 0) {
					sprintf(errstr,"Failure to read from input file\n");
					return(SYSTEM_ERROR);
				}
				if((samps_left = dz->ssampsread) == 0) {
					finished = 1;
					break;
				}
				ibuf = dz->sampbuf[0];
				ibufend = ibuf + dz->ssampsread;
			}
		}
		if(finished)
			break;
		sbuf    = sbufend;
		upsplic = splicstt;
		/* Combine upslice with downsplice at end of seg buffer */
		for(n = 0; n < splicelen_samps; n++) {
			for(m = 0;m < chans;  m++) {
				*sbuf = (SAMPLE_T)(*sbuf + *upsplic++);
				sbuf++;
			}
		}
		for(k=0;k<dz->iparam[SEG_REPETS];k++) {
		/* at 1st repeat, replace initial start-splice with splice from end of seg to start of seg */
			if(k == 1) {
				sbuf = dz->sampbuf[2];
				dnsplic = splicend;
				upsplic = splicstt;
				for(n = 0; n < splicelen_samps; n++) {
					for(m = 0;m < chans;  m++)
						*sbuf++ = (SAMPLE_T)(*dnsplic++ + *upsplic++);
				}
			}
			sbuf = dz->sampbuf[2];
			while(sbuf < sbufend) {
				*obuf++ = *sbuf++;
				if(obuf >= obufend) {
					if((exit_status = write_samps(dz->sampbuf[1],dz->buflen,dz))<0)
						return(exit_status);
					obuf = dz->sampbuf[1];
				}
			}
		}
		if(dz->vflag[NO_TIME_EXPAND]) {
			now  += seg_advance * dz->iparam[SEG_REPETS];
			ibuf += seg_samps * chans * (dz->iparam[SEG_REPETS] - 1);
			ibuf -= splicelen_samps * chans;
			while(ibuf > ibufend) {
				if((exit_status = read_samps(dz->sampbuf[0],dz)) < 0) {
					sprintf(errstr,"Failure to read from input file\n");
					return(SYSTEM_ERROR);
				}
				if((samps_left = dz->ssampsread) == 0) {
					finished = 1;
					break;
				}
				ibuf -= dz->buflen;
				ibufend = dz->sampbuf[0] + dz->ssampsread;
			}
		} else {
			now += seg_advance;
		}
		if(finished)
			break;
	}
	if((samps_remain = obuf - dz->sampbuf[1]) > 0) {
		if((exit_status = write_samps(dz->sampbuf[1],samps_remain,dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}

/******************************** DO_SEQUENCE2 *****************************/

int do_sequence2(dataptr dz)
{
	int exit_status = FINISHED, warned = 0;
	double *d, transpos, time, level, kd, frac;
	float *obufptr, *ibufptr, *tbufptr;
	float *tbuffer  = dz->sampbuf[dz->infilecnt+2];
	float *obuf     = dz->sampbuf[dz->infilecnt];
	int obuflen = dz->lparray[0][dz->infilecnt];
	float *ovflwbuf = dz->sampbuf[dz->infilecnt+1];
	int k, n, outlen, thissamp, last_total_samps_written = 0, max_write = 0, dur;
	int m, c, insno;
	float thisval, nextval;
	double diff, fracval, sum;
	int chans = dz->infile->channels, srate = dz->infile->srate;
	int splicelen = (int)round(dz->param[SEQ_SPLIC] * MS_TO_SECS * dz->infile->srate), this_splicelen, samps_read;
	double spliceincr = 1.0/(double)splicelen, this_spliceincr;
	splicelen *= dz->infile->channels;
	dz->total_samps_written = 0; 
	for(n=0;n<dz->infilecnt;n++) {
		if((samps_read = fgetfbufEx(dz->sampbuf[n], dz->insams[n],dz->ifd[n],0)) < 0) {
			sprintf(errstr,"Can't read samples from input soundfile\n");
			return(SYSTEM_ERROR);
		}
	}
	for(d = dz->parray[0]; d < dz->parray[1]; d+=5) {
		tbufptr = tbuffer;
		memset((char *)tbuffer,0,obuflen * sizeof(float));
		insno    = (int)*d;
		time     = *(d+1);
		transpos = *(d+2);
		level    = *(d+3) * dz->param[SEQ_ATTEN];
		dur      = (int)round(*(d+4) * dz->infile->srate) * dz->infile->channels;
		ibufptr = dz->sampbuf[insno];
		if(flteq(transpos,1.0)) {
			memcpy((char *)tbuffer,(char *)ibufptr,dz->insams[insno] * sizeof(float));
			tbufptr += dz->insams[insno];
		} else {
			for(c = 0;c <chans;c++)
				*tbufptr++ = *ibufptr++;
			kd = 0.0;
			do {
				kd += transpos;
				k = (int)floor(kd);
				frac = kd - (double)k;
				k *= chans;
				if(k >= dz->insams[insno] + chans)
					break;
				for(m=0;m<chans;m++) {
					thisval = ibufptr[k];
					nextval = ibufptr[k+chans];
					diff    = nextval - thisval;
					fracval = diff * frac;
					*tbufptr++ = (float)(thisval + fracval);
					k++;
				}
			} while(k < dz->insams[insno] + chans);	/* allows for wraparound point(s) at end */
		}
		if((outlen = tbufptr - tbuffer) > dur) {
			this_splicelen = splicelen;
			if(this_splicelen >= dur) {
				this_splicelen = (dur/chans) - 1;
				this_spliceincr = 1.0/(double)(this_splicelen);
				this_splicelen *= chans;
			} else 
				this_spliceincr = spliceincr;
			do_endsplice(tbuffer,dur,this_splicelen,this_spliceincr,dz->infile->channels);
			outlen = dur;
		}
		thissamp = ((int)round(time * srate) * chans) - last_total_samps_written;
		while(thissamp >= obuflen) {
			if((exit_status = write_samps(obuf,obuflen,dz))<0)
				return(exit_status);
			last_total_samps_written = dz->total_samps_written;
			memcpy((char *)obuf,(char *)ovflwbuf,obuflen * sizeof(float));
			memset((char *)ovflwbuf,0,obuflen * sizeof(float));
			max_write = 0;
			thissamp -= obuflen;
		}
		obufptr = obuf + thissamp;
		tbufptr = tbuffer;
		for(n=0;n<outlen;n++) {
			sum = *obufptr + (*tbufptr * level);
			if(!warned && (fabs(sum) > F_MAXSAMP)) {
				print_outwarning_flush("OVERLOAD!!\n");
				warned = 1;
			}
			*obufptr = (float)sum;
			obufptr++;
			tbufptr++;
		}
		if((obufptr - obuf) > max_write)
			max_write = obufptr - obuf;
	}
	if(max_write > 0)
		exit_status = write_samps(obuf,max_write,dz);
	return(exit_status);
}

/******************************** DO_ENDSPLICE *****************************/

void do_endsplice(float *buf,int dur,int splicelen,double spliceincr,int chans)
{
	int n, startsplice = dur - splicelen;
	int m;
	double thisincr = 1.0 - spliceincr;
	for(n = startsplice; n < dur; n+=chans) {
		for(m=0;m<chans;m++) {
			buf[n+m] = (float)(buf[n+m] * thisincr);
			thisincr -= spliceincr;
		}
	}
}

/******************************* ITERATE *****************************/

//TW COMPLETELY UPDATED FUNCTION : (flt-converted)
int iterate
(int cnt,int pass,double *gain,double *pshift,int write_end,int local_write_start,
	int inmsampsize,double level,double *maxsamp,int pstep,int iterating,dataptr dz)
{
	int wr_end = 0;
	switch(dz->iparam[ITER_PROCESS]) {
	case(MONO):		      		
		wr_end = iter(cnt,pass,gain,local_write_start,inmsampsize,level,maxsamp,iterating,dz);							
		break;
	case(STEREO):	      		
		wr_end = iter_stereo(cnt,pass,gain,local_write_start,inmsampsize,level,maxsamp,iterating,dz);			 		
		break;
	case(MN_INTP_SHIFT):      	
		wr_end = iter_shift_interp(cnt,pass,gain,pshift,local_write_start,inmsampsize,level,maxsamp,pstep,iterating,dz);				
		break;
	case(ST_INTP_SHIFT):      	
		wr_end = iter_shift_interp_stereo(cnt,pass,gain,pshift,local_write_start,inmsampsize,level,maxsamp,pstep,iterating,dz);		
		break;
	case(FIXA_MONO):	      	
		wr_end = fixa_iter(cnt,pass,gain,local_write_start,inmsampsize,level,maxsamp,iterating,dz);			 			
		break;
	case(FIXA_STEREO):	      	
		wr_end = fixa_iter_stereo(cnt,pass,gain,local_write_start,inmsampsize,level,maxsamp,iterating,dz);		 		
		break;
	case(FIXA_MN_INTP_SHIFT): 	
		wr_end = fixa_iter_shift_interp(cnt,pass,gain,pshift,local_write_start,inmsampsize,level,maxsamp,pstep,iterating,dz); 		
		break;
	case(FIXA_ST_INTP_SHIFT): 	
		wr_end = fixa_iter_shift_interp_stereo(cnt,pass,gain,pshift,local_write_start,inmsampsize,level,maxsamp,pstep,iterating,dz); 
		break;
	}
	return max(wr_end,write_end);
}

/**************************** ITER ***************************/

//TW COMPLETELY UPDATED FUNCTION : (flt-converted)
int iter(int cnt,int passno, double *gain,int local_write_start,int inmsampsize,double level,double *maxsamp,int iterating,dataptr dz)
{
	register int i, j = local_write_start;
	float *outbuf = dz->sampbuf[1];
	float *inbuf  = dz->sampbuf[0];
	double z;
	double thisgain;
	if(passno == 0) {
		gain[cnt] = get_gain(dz);
		thisgain = gain[cnt];

		if(dz->process != ITERATE_EXTEND && dz->vflag[IS_ITER_FADE]) {
			for(i=0; i < inmsampsize; i++) {
				z = outbuf[j] + (inbuf[i] * thisgain * level);
				*maxsamp = max(*maxsamp,fabs(z));
				outbuf[j++] = (float)z;
			}
		} else {
			for(i=0; i < inmsampsize; i++) {
				if(iterating)
					z = outbuf[j] + (inbuf[i] * thisgain);
				else
					z = outbuf[j] + inbuf[i];
				*maxsamp = max(*maxsamp,fabs(z));
				outbuf[j++] = (float)z;
			}
		}
	} else {
		thisgain = gain[cnt];

		if(dz->process != ITERATE_EXTEND && dz->vflag[IS_ITER_FADE]) {
			for(i=0; i < inmsampsize; i++) {
				z = outbuf[j] + (inbuf[i] * thisgain * level);
				outbuf[j++] = (float)z;
			}
		} else {
			for(i=0; i < inmsampsize; i++) {
			if(iterating)
				z = outbuf[j] + (inbuf[i] * thisgain);
			else
				z = outbuf[j] + inbuf[i];
				outbuf[j++] = (float)z;
			}
		}
	}
	return(j);
}

/**************************** ITER_STEREO ***************************/

//TW COMPLETELY UPDATED FUNCTION : (flt-converted)
int iter_stereo(int cnt,int passno, double *gain,int local_write_start,int inmsampsize,double level,double *maxsamp,int iterating,dataptr dz)
{
	register int i, j = local_write_start, k;
	int n;
	float *outbuf = dz->sampbuf[1];
	float *inbuf  = dz->sampbuf[0];
	double z;
	double thisgain;
	if(passno == 0) {
		gain[cnt] = get_gain(dz);
		thisgain = gain[cnt];

		if(dz->process != ITERATE_EXTEND && dz->vflag[IS_ITER_FADE]) {
			for(i=0; i < inmsampsize; i++) {
				k = i*dz->infile->channels;
				for(n=0;n<dz->infile->channels;n++) {
					z = outbuf[j] + (inbuf[k++] * thisgain * level);
					*maxsamp = max(*maxsamp,fabs(z));
					outbuf[j++]  = (float)z;
				}
			}
		} else {
			for(i=0; i < inmsampsize; i++) {
				k = i*dz->infile->channels;
				for(n=0;n<dz->infile->channels;n++) {
					if(iterating)
						z = outbuf[j] + (inbuf[k++] * thisgain);
					else 
						z = outbuf[j] + inbuf[k++];
					*maxsamp = max(*maxsamp,fabs(z));
					outbuf[j++]  = (float)z;
				}
			}
		}
	} else {
		thisgain = gain[cnt];

		if(dz->process != ITERATE_EXTEND && dz->vflag[IS_ITER_FADE]) {
			for(i=0; i < inmsampsize; i++) {
				k = i*dz->infile->channels;
				for(n=0;n<dz->infile->channels;n++) {
					z = outbuf[j] + (inbuf[k++] * thisgain * level);
					outbuf[j++]  = (float)z;
				}
			}
		} else {
			for(i=0; i < inmsampsize; i++) {
				k = i*dz->infile->channels;
				for(n=0;n<dz->infile->channels;n++) {
					if(iterating)
						z = outbuf[j] + (inbuf[k++] * thisgain);
					else 
						z = outbuf[j] + inbuf[k++];
					outbuf[j++]  = (float)z;
				}
			}
		}
	}
	return(j);
}

/**************************** ITER_SHIFT_INTERP ***************************/

//TW COMPLETELY UPDATED FUNCTION : (flt-converted)
int iter_shift_interp(int cnt,int passno, double *gain,double *pshift,int local_write_start,
		int inmsampsize,double level,double *maxsamp,int pstep,int iterating,dataptr dz)
{
	register int i = 0, j = local_write_start;
	double d = 0.0, part = 0.0;
	float val, nextval, diff;
	float *outbuf = dz->sampbuf[1];
	float *inbuf  = dz->sampbuf[0];
	double z;
	double thisgain;
	if(passno == 0) {
		gain[cnt] = get_gain(dz);
		thisgain = gain[cnt];
		if(dz->process != ITERATE_EXTEND && dz->vflag[IS_ITER_FADE]) {
			while(i < inmsampsize) {
				val     = inbuf[i++];
				nextval = inbuf[i];
				diff    = nextval - val;
				z = val + ((double)diff * part);
				z = (z * thisgain * level);
				z += outbuf[j];
//				*maxsamp = max(*maxsamp,abs(z));
				*maxsamp = max(*maxsamp,fabs(z));
				outbuf[j++] = (float)z;
				d      += dz->param[pstep];
				i      = (int)d; 			/* TRUNCATE */
				part   = d - (double)i; 
			}
		} else {
			while(i < inmsampsize) {
				val     = inbuf[i++];
				nextval = inbuf[i];
				diff    = nextval - val;
				z = val + ((double)diff * part);
				if(iterating)
					z = (z * thisgain);
				z += outbuf[j];
//				*maxsamp = max(*maxsamp,abs(z));
				*maxsamp = max(*maxsamp,fabs(z));
				outbuf[j++] = (float)z;
				d      += dz->param[pstep];
				i      = (int)d; 			/* TRUNCATE */
				part   = d - (double)i; 
			}
		}
		pshift[cnt] = get_pshift(dz);
		dz->param[pstep] = pshift[cnt];
	} else {
		thisgain = gain[cnt];
		if(dz->process != ITERATE_EXTEND && dz->vflag[IS_ITER_FADE]) {
			while(i < inmsampsize) {
				val     = inbuf[i++];
				nextval = inbuf[i];
				diff    = nextval - val;
				z = val + ((double)diff * part);
				z = (z * thisgain * level);
				z += outbuf[j];
				outbuf[j++] = (float)z;
				d      += dz->param[pstep];
				i      = (int)d; 			/* TRUNCATE */
				part   = d - (double)i; 
			}
		} else {
			while(i < inmsampsize) {
				val     = inbuf[i++];
				nextval = inbuf[i];
				diff    = nextval - val;
				z = val + ((double)diff * part);
				if(iterating)
					z = (z * thisgain);
				z += outbuf[j];
				outbuf[j++] = (float)z;
				d      += dz->param[pstep];
				i      = (int)d; 			/* TRUNCATE */
				part   = d - (double)i; 
			}
		}
		dz->param[pstep] = pshift[cnt];
	}
	return(j);
}

/*********************** ITER_SHIFT_INTERP_STEREO *************************/

//TW COMPLETELY UPDATED FUNCTION : (flt-converted)
int iter_shift_interp_stereo(int cnt,int passno, double *gain,double *pshift,int local_write_start,
		int inmsampsize,double level,double *maxsamp,int pstep,int iterating,dataptr dz)
{
	register int i = 0, j = local_write_start, k;
	int n;
	double d = 0.0, part = 0.0;
	float val, nextval, diff;
	float *outbuf = dz->sampbuf[1];
	float *inbuf  = dz->sampbuf[0];
	double z;
	double thisgain;
	if(passno == 0) {
		gain[cnt] = get_gain(dz);
		thisgain = gain[cnt];

		if(dz->process != ITERATE_EXTEND && dz->vflag[IS_ITER_FADE]) {
			while(i < inmsampsize) {
				k = i*dz->infile->channels;
				for(n=0;n<dz->infile->channels;n++) {
					val     = inbuf[k];
					nextval = inbuf[k+dz->infile->channels];
					diff    = nextval - val;
					z = val + ((double)diff * part);
					z = (z * thisgain * level);
					z += outbuf[j];
//					*maxsamp = max(*maxsamp,abs(z));
					*maxsamp = max(*maxsamp,fabs(z));
					outbuf[j++] = (float)z;
					k++;
				}
				d   += dz->param[pstep];
				i    = (int)d; 			/* TRUNCATE */
				part = d - (double)i; 
			}
		} else {
			while(i < inmsampsize) {
				k = i*dz->infile->channels;
				for(n=0;n<dz->infile->channels;n++) {
					val     = inbuf[k];
					nextval = inbuf[k+dz->infile->channels];
					diff    = nextval - val;
					z = val + ((double)diff * part);
					if(iterating)
						z = (z * thisgain);
					z += outbuf[j];
//					*maxsamp = max(*maxsamp,abs(z));
					*maxsamp = max(*maxsamp,fabs(z));
					outbuf[j++] = (float)z;
					k++;
				}
				d   += dz->param[pstep];
				i    = (int)d; 			/* TRUNCATE */
				part = d - (double)i; 
			}
		}
		pshift[cnt] = get_pshift(dz);
		dz->param[pstep] = pshift[cnt];
	} else {
		thisgain = gain[cnt];

		if(dz->process != ITERATE_EXTEND && dz->vflag[IS_ITER_FADE]) {
			while(i < inmsampsize) {
				k = i*dz->infile->channels;
				for(n=0;n<dz->infile->channels;n++) {
					val     = inbuf[k];
					nextval = inbuf[k+dz->infile->channels];
					diff    = nextval - val;
					z = val + ((double)diff * part);
					z = (z * thisgain * level);
					z += outbuf[j];
					outbuf[j++] = (float)z;
					k++;
				}
				d   += dz->param[pstep];
				i    = (int)d; 			/* TRUNCATE */
				part = d - (double)i; 
			}
		} else {
			while(i < inmsampsize) {
				k = i*dz->infile->channels;
				for(n=0;n<dz->infile->channels;n++) {
					val     = inbuf[k];
					nextval = inbuf[k+dz->infile->channels];
					diff    = nextval - val;
					z = val + ((double)diff * part);
					if(iterating)
						z = (z * thisgain);
					z += outbuf[j];
					outbuf[j++] = (float)z;
					k++;
				}
				d   += dz->param[pstep];
				i    = (int)d; 			/* TRUNCATE */
				part = d - (double)i; 
			}
		}
		dz->param[pstep] = pshift[cnt];
	}
	return(j);
}

/**************************** FIXA_ITER ***************************/

//TW COMPLETELY UPDATED FUNCTION : (flt-converted)
int fixa_iter(int cnt,int passno,double *gain,int local_write_start,int inmsampsize,double level,double *maxsamp,int iterating,dataptr dz)
{
	register int i, j = local_write_start;
	float *outbuf = dz->sampbuf[1];
	float *inbuf  = dz->sampbuf[0];
	double z;

	if(passno ==0) {
		if(dz->process != ITERATE_EXTEND && dz->vflag[IS_ITER_FADE]) {
			for(i=0; i < inmsampsize; i++) {
				z = outbuf[j] + (inbuf[i] * level);
				*maxsamp = max(*maxsamp,fabs(z));
				outbuf[j]  = (float)z;
				j++;
			}
		} else {
			for(i=0; i < inmsampsize; i++) {
				z = outbuf[j] + inbuf[i];
				*maxsamp = max(*maxsamp,fabs(z));
				outbuf[j]  = (float)z;
				j++;
			}
		}
	} else {
		if(dz->process != ITERATE_EXTEND && dz->vflag[IS_ITER_FADE]) {
			for(i=0; i < inmsampsize; i++) {
				z = outbuf[j] + (inbuf[i] * level * gain[cnt]);
				outbuf[j]  = (float)z;
				j++;
			}
		} else {
			for(i=0; i < inmsampsize; i++) {
				if(iterating)
					z = outbuf[j] + (inbuf[i] * gain[cnt]);
				else
				z = outbuf[j] + inbuf[i];
				outbuf[j]  = (float)z;
				j++;
			}
		}
	}
	return(j);
}

/**************************** FIXA_ITER_STEREO ***************************/

//TW COMPLETELY UPDATED FUNCTION : (flt-converted)
int fixa_iter_stereo(int cnt,int passno,double *gain,int local_write_start,int inmsampsize,double level,double *maxsamp,int iterating,dataptr dz)
{
	register int i, j = local_write_start, k;
	int n;
	float *outbuf = dz->sampbuf[1];
	float *inbuf  = dz->sampbuf[0];
	double z;

	if(passno == 0) {
		if(dz->process != ITERATE_EXTEND && dz->vflag[IS_ITER_FADE]) {
			for(i=0; i < inmsampsize; i++) {
				k = i*dz->infile->channels;
				for(n=0;n<dz->infile->channels;n++) {
					z = outbuf[j] + (inbuf[k++] * level);
//					*maxsamp = max(*maxsamp,abs(z));
					*maxsamp = max(*maxsamp,fabs(z));
					outbuf[j]  = (float)z;
					j++;
				}
			}
		} else {
			for(i=0; i < inmsampsize; i++) {
				k = i*dz->infile->channels;
				for(n=0;n<dz->infile->channels;n++) {
					z = outbuf[j] + inbuf[k++];
//					*maxsamp = max(*maxsamp,abs(z));
					*maxsamp = max(*maxsamp,fabs(z));
					outbuf[j]  = (float)z;
					j++;
				}
			}
		}
 	} else {
		if(dz->process != ITERATE_EXTEND && dz->vflag[IS_ITER_FADE]) {
			for(i=0; i < inmsampsize; i++) {
				k = i*dz->infile->channels;
				for(n=0;n<dz->infile->channels;n++) {
					z = outbuf[j] + (inbuf[k++] * level * gain[cnt]);
					outbuf[j]  = (float)z;
					j++;
				}
			}
		} else {
			for(i=0; i < inmsampsize; i++) {
				k = i*dz->infile->channels;
				for(n=0;n<dz->infile->channels;n++) {
					if(iterating)
						z = outbuf[j] + (inbuf[k++] * gain[cnt]);
					else
						z = outbuf[j] + inbuf[k++];
					outbuf[j]  = (float)z;
					j++;
				}
			}
		}
	}
 	return(j);
}

/**************************** FIXA_ITER_SHIFT_INTERP ***************************/

//TW COMPLETELY UPDATED FUNCTION : (flt-converted)
int fixa_iter_shift_interp(int cnt,int passno,double *gain,double *pshift,int local_write_start,
		int inmsampsize,double level,double *maxsamp,int pstep,int iterating,dataptr dz)
{
	register int i = 0, j = local_write_start;
 	double d = 0.0, part = 0.0;
 	float val, nextval, diff;
 	float *outbuf = dz->sampbuf[1];
	float *inbuf  = dz->sampbuf[0];
	double z;

	if(passno == 0) {
		if(dz->process != ITERATE_EXTEND && dz->vflag[IS_ITER_FADE]) {
		 	while(i < inmsampsize) {
		 		val     = inbuf[i++];
		 		nextval = inbuf[i];
		 		diff    = nextval - val;
				z = val + ((double)diff * part);
				z = (z * level);
				z += outbuf[j];
				*maxsamp = max(*maxsamp,fabs(z));
				outbuf[j++] = (float)z;
		 		d      += dz->param[pstep];
		 		i      = (int)d; 			/* TRUNCATE */
		 		part   = d - (double)i; 
		 	}
		} else {
		 	while(i < inmsampsize) {
		 		val     = inbuf[i++];
		 		nextval = inbuf[i];
		 		diff    = nextval - val;
				z = val + ((double)diff * part);
				z += outbuf[j];
				*maxsamp = max(*maxsamp,fabs(z));
		 		outbuf[j++] = (float)z;
		 		d      += dz->param[pstep];
		 		i      = (int)d; 			/* TRUNCATE */
		 		part   = d - (double)i; 
		 	}
		}
		pshift[cnt] = get_pshift(dz);
	} else {
		if(dz->process != ITERATE_EXTEND && dz->vflag[IS_ITER_FADE]) {
		 	while(i < inmsampsize) {
		 		val     = inbuf[i++];
		 		nextval = inbuf[i];
		 		diff    = nextval - val;
				z = val + ((double)diff * part);
				z = (z * level * gain[cnt]);
				z += outbuf[j];
		 		outbuf[j++] = (float)z;
		 		d      += dz->param[pstep];
		 		i      = (int)d; 			/* TRUNCATE */
		 		part   = d - (double)i; 
		 	}
		} else {
		 	while(i < inmsampsize) {
		 		val     = inbuf[i++];
		 		nextval = inbuf[i];
		 		diff    = nextval - val;
				z = val + ((double)diff * part);
				if(iterating)
					z = (z * gain[cnt]);
				z += outbuf[j];
		 		outbuf[j++] = (float)z;
		 		d      += dz->param[pstep];
		 		i      = (int)d; 			/* TRUNCATE */
		 		part   = d - (double)i; 
		 	}
		}
	}
	dz->param[pstep] = pshift[cnt];
 	return(j);
}

/*********************** FIXA_ITER_SHIFT_INTERP_STEREO *************************/

//TW COMPLETELY UPDATED FUNCTION : (flt-converted)
int fixa_iter_shift_interp_stereo(int cnt,int passno,double *gain,double *pshift,int local_write_start,
		int inmsampsize,double level,double *maxsamp,int pstep,int iterating,dataptr dz)
{
	register int i = 0, j = local_write_start, k;
	int n;
	double d = 0.0, part = 0.0;
	float val, nextval, diff;
	float *outbuf = dz->sampbuf[1];
	float *inbuf  = dz->sampbuf[0];
	double z;

	if(passno == 0) {
		if(dz->process != ITERATE_EXTEND && dz->vflag[IS_ITER_FADE]) {
			while(i < inmsampsize) {
				k = i*dz->infile->channels;
				for(n=0;n<dz->infile->channels;n++) {
					val     = inbuf[k];
					nextval = inbuf[k+dz->infile->channels];
					diff    = nextval - val;
					z = val + ((double)diff * part);
					z = (z * level);
					z += outbuf[j];
					*maxsamp = max(*maxsamp,fabs(z));
					outbuf[j++] = (float)z;
					k++;
				}
				d   += dz->param[pstep];
				i    = (int)d; 			/* TRUNCATE */
				part = d - (double)i; 
			}
		} else {
			while(i < inmsampsize) {
				k = i*dz->infile->channels;
				for(n=0;n<dz->infile->channels;n++) {
					val     = inbuf[k];
					nextval = inbuf[k+dz->infile->channels];
					diff    = nextval - val;
					z = val + ((double)diff * part);
					z += outbuf[j];
					*maxsamp = max(*maxsamp,fabs(z));
					outbuf[j++] = (float)z;
					k++;
				}
				d   += dz->param[pstep];
				i    = (int)d; 			/* TRUNCATE */
				part = d - (double)i; 
			}
		}
		pshift[cnt] = get_pshift(dz);
	} else {
		if(dz->process != ITERATE_EXTEND && dz->vflag[IS_ITER_FADE]) {
			while(i < inmsampsize) {
				k = i*dz->infile->channels;
				for(n=0;n<dz->infile->channels;n++) {
					val     = inbuf[k];
					nextval = inbuf[k+dz->infile->channels];
					diff    = nextval - val;
					z = val + ((double)diff * part);
					z = (z * level * gain[cnt]);
					z += outbuf[j];
					outbuf[j++] = (float)z;
					k++;
				}
				d   += dz->param[pstep];
				i    = (int)d; 			/* TRUNCATE */
				part = d - (double)i; 
			}
		} else {
			while(i < inmsampsize) {
				k = i*dz->infile->channels;
				for(n=0;n<dz->infile->channels;n++) {
					val     = inbuf[k];
					nextval = inbuf[k+dz->infile->channels];
					diff    = nextval - val;
					z = val + ((double)diff * part);
					if(iterating)
						z = (z * gain[cnt]);
					z += outbuf[j];
					outbuf[j++] = (float)z;
					k++;
				}
				d   += dz->param[pstep];
				i    = (int)d; 			/* TRUNCATE */
				part = d - (double)i; 
			}
		}
	}
	dz->param[pstep] = pshift[cnt];
	return(j);
}

