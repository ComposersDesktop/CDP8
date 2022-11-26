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
#include <structures.h>
#include <globcon.h>
#include <tkglobals.h>
#include <pnames.h>
#include <processno.h>
#include <modeno.h>
#include <logic.h>
#include <cdpmain.h>
#include <pvoc.h>
#include <string.h>
#include <sfsys.h>
#include <pvoc.h>

// NEW 2014 -->
static int  outfloats(float *nextOut, float *maxsampl,float *minsample,float *local_maxsample,int *num_overflows,int todo, int finished, dataptr dz);
// <-- NEW 2014
static int  pvoc_float_array(int nnn,float **ptr);
static int  sndwrite_header(int N2,int *Nchans,float *arate,float R,int D,int origsize,int *isr,int M,dataptr dz);
static void hamming(float *win,int winLen,int even);
static int  pvoc_time_display(int nI,int srate,int *samptime,dataptr dz);
static int write_samps_pvoc(float *bbuf,int samps_to_write,dataptr dz);

/****************************** HAMMING ******************************/

void hamming(float *win,int winLen,int even)
{
	float Pi,ftmp;
	int i;

/***********************************************************
					Pi = (float)((double)4.*atan((double)1.));
***********************************************************/
	Pi = (float)PI;
	ftmp = Pi/winLen;

	if (even) {
		for (i=0; i<winLen; i++)
		*(win+i) = (float)((double).54 + (double).46*cos((double)(ftmp*((float)i+.5))));
		*(win+winLen) = 0.0f;}
	else{	*(win) = 1.0f;
		for (i=1; i<=winLen; i++)
		*(win+i) =(float)((double).54 + (double).46*cos((double)(ftmp*(float)i)));
	}
	return;
}

/****************************** FLOAT_ARRAY ******************************/

int pvoc_float_array(int nnn,float **ptr)
{	/* set up a floating point array length nnn. */
	*ptr = (float *) calloc(nnn,sizeof(float));
	if(*ptr==NULL){
		sprintf(errstr,"pvoc: insufficient memory\n");
		return(MEMORY_ERROR);
	}
	return(FINISHED);
}

/****************************** PVOC_PROCESS ******************************/

int pvoc_process(dataptr dz)
{
	int exit_status;
	int finished = 0;
	int num_overflows = 0;
	int samptime = SAMP_TIME_STEP;
	double	rratio;
	float	*input,			/* pointer to start of input buffer */
			*output,		/* pointer to start of output buffer */
			*anal,			/* pointer to start of analysis buffer */
			*syn,			/* pointer to start of synthesis buffer */
			*banal,			/* pointer to anal[1] (for FFT calls) */
			*bsyn,			/* pointer to syn[1]  (for FFT calls) */
			*nextIn,		/* pointer to next empty word in input */
			*nextOut,		/* pointer to next empty word in output */
			*analWindow,	/* pointer to center of analysis window */
			*synWindow,		/* pointer to center of synthesis window */
			*maxAmp,		/* pointer to start of max amp buffer */
			*avgAmp,		/* pointer to start of avg amp buffer */
			*avgFrq,		/* pointer to start of avg frq buffer */
			*env,			/* pointer to start of spectral envelope */
			*i0,			/* pointer to amplitude channels */
			*i1,			/* pointer to frequency channels */
			*oi,			/* pointer to old phase channels */
			*oldInPhase,	/* pointer to start of input phase buffer */
			*oldOutPhase,	/* pointer to start of output phase buffer */
			maxsample = 0.0, minsample = 0.0, biggest;

	int		M = 0,			/* length of analWindow impulse response */
			D = 0,			/* decimatin factor */
			I = 0,			/* interpolation factor (default will be I=D)*/
			/* RWD */
			/*F = 0,*/			/* fundamental frequency (determines dz->iparam[PVOC_CHANS]) */
			analWinLen,		/* half-length of analysis window */
			synWinLen;		/* half-length of synthesis window */

	int	sampsize,		/* sample size for output file */
			outCount,		/* number of samples written to output */
			ibuflen,		/* length of input buffer */
			obuflen,		/* length of output buffer */
			nI = 0,			/* current input (analysis) sample */
			nO,				/* current output (synthesis) sample */
			nMaxOut;		/* last output (synthesis) sample */
	int	origsize = 0,	/* sample type of file analysed */
			isr,			/* sampling rate */
			Nchans,			/* no of chans */
			endsamp = VERY_BIG_INT;

	float	real,			/* real part of analysis data */
			imag,			/* imaginary part of analysis data */
			mag,			/* magnitude of analysis data */
			phase,			/* phase of analysis data */
			angleDif,		/* angle difference */
			RoverTwoPi,		/* R/D divided by 2*Pi */
			TwoPioverR,		/* 2*Pi divided by R/I */
			sum,			/* scale factor for renormalizing windows */
//			ftot = 0.0f,	/* scale factor for calculating statistics */
			rIn,			/* decimated sampling rate */
			rOut,			/* pre-interpolated sampling rate */
			R,				/* input sampling rate */
// NEW 2014 -->
			local_maxsample = 0.0;
// <-- NEW 2014

	int		i,j,k,		/* index variables */
			Dd,			/* number of new inputs to read (Dd <= D) */
			Ii,			/* number of new outputs to write (Ii <= I) */
			N2,			/* dz->iparam[PVOC_CHANS]/2 */
			NO,			/* synthesis NO = dz->iparam[PVOC_CHANS] / P */
			NO2,		/* NO/2 */
			IO,			/* synthesis IO = I / P */
			IOi,		/* synthesis IOi = Ii / P */
			Mf = 0,		/* flag for even M */
#ifdef SINGLE_SAMP
			rv,			/* return value from fgetfloat */
#endif
			flag = 0;	/* end-of-input flag */
	float	arate = 1.0 /* TW: arbitrary initialisation */;		/* sample rate for header on stdout if -A */
	/*RWD */
	float F = 0.0f;

	if(sndgetprop(dz->ifd[0],"sample type", (char *) &sampsize, sizeof(int)) < 0){
		sprintf(errstr,"pvoc: failure to get sample type\n");
		return(MEMORY_ERROR);
	}

	if (dz->process==PVOC_SYNTH) {
		isr      = dz->infile->origrate;
		sampsize = dz->infile->origstype;							   		
		arate    = dz->infile->arate;
		Nchans   = dz->infile->channels;
		dz->iparam[PVOC_CHANS] = Nchans - 2;
		M		 = dz->infile->Mlen;
		D        = dz->infile->Dfac;
		R    	 = ((float) D * arate);
	} else {
		isr 	 = dz->infile->srate;
		R        = (float)isr;
		Nchans   = dz->infile->channels;
		/*RWD OCT 05 need this to preserved hires infile formats */
		origsize = dz->infile->stype;	
	}
	if(flteq(R,0.0)) {
		sprintf(errstr,"Problem: zero sampling rate\n");
		return(DATA_ERROR);
	}
	
	sampsize = ((dz->iparam[PVOC_ANAL_ONLY]) ? SAMP_FLOAT:dz->infile->stype);
	N2 = dz->iparam[PVOC_CHANS] / 2;

	F = /*(int)*/(float)(R /(float)dz->iparam[PVOC_CHANS]);	  /*RWD*/

	if(dz->process!=PVOC_SYNTH) {
		switch(dz->iparam[PVOC_WIN_OVERLAP]){
			case 0:	M = 4*dz->iparam[PVOC_CHANS];	break;
			case 1:	M = 2*dz->iparam[PVOC_CHANS];	break;
			case 2: M = dz->iparam[PVOC_CHANS];		break;
			case 3: M = N2;							break;
			default:
				sprintf(errstr,"pvoc: Invalid window overlap factor.\n");
				return(PROGRAM_ERROR);
		}
	}

	if(!sloom) {
		if(dz->iparam[PVOC_ANAL_ONLY] && (dz->process==PVOC_SYNTH)) {
			sprintf(errstr,"both -A and -S specified: Impossible!\n");
			return(PROGRAM_ERROR);
		}
	}

	Mf = 1 - M%2;

	if (M < 7) {
		fprintf(stdout,"WARNING: analWindow impulse response is too small\n");
 		fflush(stdout);
	}
	ibuflen = 4 * M;
	obuflen = 4 * M;

	if(dz->process!=PVOC_SYNTH) {
		if((D = (int)(M/PVOC_CONSTANT_A)) == 0){
			fprintf(stdout,"WARNING: Decimation too low: adjusted.\n");
			fflush(stdout);
			D = 1;
		}
	}

	switch(dz->process) {
	case(PVOC_ANAL):
	case(PVOC_EXTRACT):
		arate = (float)(R/D);	/* Needed to write to outheader */
		dz->wanted = dz->iparam[PVOC_CHANS] + 2;	

		/* fall thro */
	case(PVOC_SYNTH):
		dz->frametime = (float)(1.0/arate);
		break;
	}

	if(sloom) {
		if(dz->process==PVOC_SYNTH) {
			dz->tempsize = (dz->insams[0]/dz->infile->channels) * dz->infile->Dfac;	
				/*length of output in samps: needed for time-display in TK mode */
		} else {
			dz->tempsize = dz->insams[0];
		}
	}

	I   = D;
	NO  = dz->iparam[PVOC_CHANS];	/* synthesis transform will be NO points */
	NO2 = NO/2;
	IO  = I;

	if((exit_status = sndwrite_header(N2,&Nchans,&arate,R,D,origsize,&isr,M,dz))<0)
		return(exit_status);

//TW 
//	if(!sloom) {
	if(!sloom && !sloombatch) {
		fprintf(stdout,"analysis/synthesis beginning\n");	
		fflush(stdout);
	}
	/* set up analysis window: The window is assumed to be symmetric
		with M total points.  After the initial memory allocation,
		analWindow always points to the midpoint of the window
		(or one half sample to the right, if M is even); analWinLen
		is half the true window length (rounded down). Any low pass
		window will work; a Hamming window is generally fine,
		but a Kaiser is also available.  If the window duration is
		longer than the transform (M > N), then the window is
		multiplied by a sin(x)/x function to meet the condition:
		analWindow[Ni] = 0 for i != 0.  In either case, the
		window is renormalized so that the phase vocoder amplitude
		estimates are properly scaled.  The maximum allowable
		window duration is ibuflen/2. */

	if((exit_status = pvoc_float_array(M+Mf,&analWindow))<0)
		return(exit_status);
	analWindow += (analWinLen = M/2);

	hamming(analWindow,analWinLen,Mf);

	for (i = 1; i <= analWinLen; i++)
		*(analWindow - i) = *(analWindow + i - Mf);

	if (M > dz->iparam[PVOC_CHANS]) {
		if (Mf)
		*analWindow *=(float)
		((double)dz->iparam[PVOC_CHANS]*sin((double)PI*.5/dz->iparam[PVOC_CHANS])/(double)(PI*.5));
		for (i = 1; i <= analWinLen; i++) 
			*(analWindow + i) *=(float)
			((double)dz->iparam[PVOC_CHANS] * sin((double) (PI*(i+.5*Mf)/dz->iparam[PVOC_CHANS])) / (PI*(i+.5*Mf))); /* D.Timis */
		for (i = 1; i <= analWinLen; i++)
			*(analWindow - i) = *(analWindow + i - Mf);
	}

	sum = 0.0f;
	for (i = -analWinLen; i <= analWinLen; i++)
		sum += *(analWindow + i);

	sum = (float)(2.0/sum);		/*factor of 2 comes in later in trig identity*/

	for (i = -analWinLen; i <= analWinLen; i++)
		*(analWindow + i) *= sum;

	/* set up synthesis window:  For the minimal mean-square-error
		formulation (valid for N >= M), the synthesis window
		is identical to the analysis window (except for a
		scale factor), and both are even in length.  If N < M,
		then an interpolating synthesis window is used. */

	if((exit_status = pvoc_float_array(M+Mf,&synWindow))<0)
		return(exit_status);
	synWindow += (synWinLen = M/2);

	if (M <= dz->iparam[PVOC_CHANS]){
		hamming(synWindow,synWinLen,Mf);
		for (i = 1; i <= synWinLen; i++)
			*(synWindow - i) = *(synWindow + i - Mf);

		for (i = -synWinLen; i <= synWinLen; i++)
			*(synWindow + i) *= sum;

		sum = 0.0f;
		for (i = -synWinLen; i <= synWinLen; i+=I)
			sum += *(synWindow + i) * *(synWindow + i);

		sum = (float)(1.0/ sum);

		for (i = -synWinLen; i <= synWinLen; i++)
			*(synWindow + i) *= sum;
	} else {	
		hamming(synWindow,synWinLen,Mf);
		for (i = 1; i <= synWinLen; i++)
			*(synWindow - i) = *(synWindow + i - Mf);

		if (Mf)
			*synWindow *= (float)((double)IO * sin((double) (PI*.5/IO)) / (double)(PI*.5));
		for (i = 1; i <= synWinLen; i++) 
			*(synWindow + i) *=(float)
			((double)IO * sin((double) (PI*(i+.5*Mf)/IO)) /(double) (PI*(i+.5*Mf)));
		for (i = 1; i <= synWinLen; i++)
			*(synWindow - i) = *(synWindow + i - Mf);

		sum = (float)(1.0/sum);

		for (i = -synWinLen; i <= synWinLen; i++)
			*(synWindow + i) *= sum;
	}
      
	/* set up input buffer:  nextIn always points to the next empty
		word in the input buffer (i.e., the sample following
		sample number (n + analWinLen)).  If the buffer is full,
		then nextIn jumps back to the beginning, and the old
		values are written over. */

	if((exit_status = pvoc_float_array(ibuflen,&input))<0)
		return(exit_status);

	nextIn = input;

	/* set up output buffer:  nextOut always points to the next word
		to be shifted out.  The shift is simulated by writing the
		value to the standard output and then setting that word
		of the buffer to zero.  When nextOut reaches the end of
		the buffer, it jumps back to the beginning.  */

	if((exit_status = pvoc_float_array(obuflen,&output))<0)
		return(exit_status);

	nextOut = output;
	/* set up analysis buffer for (N/2 + 1) channels: The input is real,
		so the other channels are redundant. oldInPhase is used
		in the conversion to remember the previous phase when
		calculating phase difference between successive samples. */

	if((exit_status = pvoc_float_array(dz->iparam[PVOC_CHANS]+2,&anal))<0)
		return(exit_status);
	banal = anal + 1;

	if((exit_status = pvoc_float_array(N2+1,&oldInPhase))<0)
		return(exit_status);
	if((exit_status = pvoc_float_array(N2+1,&maxAmp))<0)
		return(exit_status);
	if((exit_status = pvoc_float_array(N2+1,&avgAmp))<0)
		return(exit_status);
	if((exit_status = pvoc_float_array(N2+1,&avgFrq))<0)
		return(exit_status);
	if((exit_status = pvoc_float_array(N2+1,&env))<0)
		return(exit_status);

	/* set up synthesis buffer for (dz->iparam[PVOC_CHANS]/2 + 1) channels: (This is included
		only for clarity.)  oldOutPhase is used in the re-
		conversion to accumulate angle differences (actually angle
		difference per second). */

	if((exit_status = pvoc_float_array(NO+2,&syn))<0)
		return(exit_status);
	bsyn = syn + 1;

	if((exit_status = pvoc_float_array(NO2+1,&oldOutPhase))<0)
		return(exit_status);

	/* initialization: input time starts negative so that the rightmost
		edge of the analysis filter just catches the first non-zero
		input samples; output time is always T times input time. */

	outCount = 0;
	rIn  = (float)(R/(float)D);
	rOut = (float)(R/(float)I);
	RoverTwoPi = (float)(rIn/TWOPI);
	TwoPioverR = (float)(TWOPI/rOut);
	nI = -(analWinLen / D) * D;	/* input time (in samples) */
	nO = nI;					/* output time (in samples) */
	Dd = analWinLen + nI + 1;	/* number of new inputs to read */
	Ii = 0;				/* number of new outputs to write */
	IOi = 0;
	flag = 1;

	/* main loop:  If endsamp is not specified it is assumed to be very large
		and then readjusted when fgetfloat detects the end of input. */

	display_virtual_time(0L,dz);	

	while(nI < (endsamp + analWinLen)){
		if (dz->process==PVOC_SYNTH){
#ifdef SINGLE_SAMP
			for (i = 0; i < dz->iparam[PVOC_CHANS]+2; i++){		/* synthesis only */
				if ((rv = fgetfloat((anal+i),dz->ifd[0])) <= 0){
					goto epilog;
				}
			}
#else
// NEW 2014 -->
			memset((char *)anal,0,(dz->iparam[PVOC_CHANS]+2) * sizeof(float));	/* Ensure buffer is wiped, esp for empty buffers at process end */
// <-- NEW 2014
			if((i = fgetfbufEx(anal, dz->iparam[PVOC_CHANS]+2, dz->ifd[0], 0)) < 0) {
				sfperror("pvoc: read error: ");
				return(SYSTEM_ERROR);
			}
// NEW 2014 -->
			if(i < dz->iparam[PVOC_CHANS]+2) {
				finished = 1;				//	Finished reading input
				if(local_maxsample == 0.0)	//	Finished writing output
					goto epilog;
			}
// <-- NEW 2014
#endif
		} else {		/* prepare for analysis: read next Dd input values */
#ifdef SINGLE_SAMP
			for (i = 0; i < Dd; i++){
				if (fgetfloat(nextIn++,dz->ifd[0]) <= 0)
					Dd = i;			/* EOF ? */
				if (nextIn >= (input + ibuflen))
					nextIn -= ibuflen;
			}
#else
			{
				static float *sbuf = 0;
				static int sblen = 0;
				int got, tocp;
				float *sp;

				if(sblen < Dd) {
					if(sbuf != 0)
						free(sbuf);

					if((sbuf = (float *)malloc(Dd*sizeof(float))) == 0) {
						sprintf(errstr, "pvoc: can't allocate short buffer\n");
						return(MEMORY_ERROR);
					}
					sblen = Dd;
				}
				if((got = fgetfbufEx(sbuf, Dd, dz->ifd[0],0)) < 0) {
					sfperror("pvoc: read error");
					return(SYSTEM_ERROR);
				}
				if(got < Dd)
					Dd = got;
				sp = sbuf;

				tocp = min(got, input+ibuflen-nextIn);
				got -= tocp;
				while(tocp-- > 0)
					*nextIn++ = *sp++;

				if(got > 0) {
					nextIn -= ibuflen;
					while(got-- > 0)
						*nextIn++ = *sp++;
				}
				if (nextIn >= (input + ibuflen))
					nextIn -= ibuflen;
			}
#endif
			if (nI > 0)
				for (i = Dd; i < D; i++){	/* zero fill at EOF */
					*(nextIn++) = 0.0f;
					if (nextIn >= (input + ibuflen))
						nextIn -= ibuflen;
				}
	/* analysis: The analysis subroutine computes the complex output at
		time n of (dz->iparam[PVOC_CHANS]/2 + 1) of the phase vocoder channels.  It operates
		on input samples (n - analWinLen) thru (n + analWinLen) and
		expects to find these in input[(n +- analWinLen) mod ibuflen].
		It expects analWindow to point to the center of a
		symmetric window of length (2 * analWinLen +1).  It is the
		responsibility of the main program to ensure that these values
		are correct!  The results are returned in anal as succesive
		pairs of real and imaginary values for the lowest (dz->iparam[PVOC_CHANS]/2 + 1)
		channels.   The subroutines fft and reals together implement
		one efficient FFT call for a real input sequence.  */


			for (i = 0; i < dz->iparam[PVOC_CHANS]+2; i++) 
				*(anal + i) = 0.0f;	/*initialize*/

			j = (nI - analWinLen - 1 + ibuflen) % ibuflen;	/*input pntr*/

			k = nI - analWinLen - 1;			/*time shift*/
			while (k < 0)
				k += dz->iparam[PVOC_CHANS];
			k = k % dz->iparam[PVOC_CHANS];
			for (i = -analWinLen; i <= analWinLen; i++) {
				if (++j >= ibuflen)
					j -= ibuflen;
				if (++k >= dz->iparam[PVOC_CHANS])
					k -= dz->iparam[PVOC_CHANS];
				*(anal + k) += *(analWindow + i) * *(input + j);
			}
			if((exit_status = fft_(anal,banal,1,N2,1,-2))<0)
				return(exit_status);
			if((exit_status = reals_(anal,banal,N2,-2))<0)
				return(exit_status);

	/* conversion: The real and imaginary values in anal are converted to
		magnitude and angle-difference-per-second (assuming an 
		intermediate sampling rate of rIn) and are returned in
		anal. */

			for(i=0,i0=anal,i1=anal+1,oi=oldInPhase; i <= N2; i++,i0+=2,i1+=2,oi++){
				real = *i0;
				imag = *i1;
				*i0 =(float) sqrt((double)(real * real + imag * imag));
							/* phase unwrapping */
				if (*i0 == 0.)
					angleDif = 0.0f;
	
				else {
					rratio = atan((double)imag/(double)real);
					if(real<0.0) {
						if(imag<0.0)
							rratio -= PI;
						else
							rratio += PI;
					}
					angleDif  = (phase = (float)rratio) - *oi;
					*oi = phase;
				}

				if (angleDif > PI)
					angleDif = (float)(angleDif - TWOPI);
				if (angleDif < -PI)
					angleDif = (float)(angleDif + TWOPI);

						/* add in filter center freq.*/

				*i1 = angleDif * RoverTwoPi + ((float) i * F);
			}
		}

	/* if analysis only, write out interleaved instantaneous amplitudes
		and frequencies; otherwise perform resynthesis */

    	if (dz->iparam[PVOC_ENVOUT_ONLY]) {
#ifdef SINGLE_SAMP
			for (i=0; i <= N2; i++)
				fputfloat((env+i),dz->ofd);
#else
			write_samps_pvoc(env, N2+1, dz);
#endif
    	} else if (dz->iparam[PVOC_MAGOUT_ONLY]) {
#ifdef SINGLE_SAMP
			for (i=0; i <= N2; i++)
				fputfloat((anal + 2*i),dz->ofd);
#else
			float *fp = anal;
			for (i=0; i <= N2; i++) {
				fputfloat(fp,dz->ofd);
				fp += 2;
			}
#endif
		} else if (dz->iparam[PVOC_ANAL_ONLY]) {
#ifdef SINGLE_SAMP
			for (i=0; i < dz->iparam[PVOC_CHANS]+2; i++)
				fputfloat((anal+i),dz->ofd);
#else
			write_samps_pvoc(anal, dz->iparam[PVOC_CHANS]+2, dz);
#endif
		} else { /* synthesis */
				

			if (dz->iparam[PVOC_PARTIAL_RESYNTH]){		 /* zero out non-selected channels */
				for (i = 0; i < dz->iparam[PVOC_AF_PAIR_LO]; i++)
					*(anal+2*i) = 0.0f;
				for (i = dz->iparam[PVOC_AF_PAIR_HI]+1; i <= N2; i++)
					*(anal+2*i) = 0.0f;
				if (dz->iparam[PVOC_SELECTED_CHAN] == 1) {
					for (i = dz->iparam[PVOC_AF_PAIR_LO]; i <= dz->iparam[PVOC_AF_PAIR_HI]; i++) {
						if (i%2 == 0)
							*(anal+2*i) = 0.0f;
					}
				}
				if (dz->iparam[PVOC_SELECTED_CHAN] == 2) {
					for (i = dz->iparam[PVOC_AF_PAIR_LO]; i <= dz->iparam[PVOC_AF_PAIR_HI]; i++) {
						if (i%2 != 0)
							*(anal+2*i) = 0.0f;
					}
				}
			}

	/* reconversion: The magnitude and angle-difference-per-second in syn
		(assuming an intermediate sampling rate of rOut) are
		converted to real and imaginary values and are returned in syn.
		This automatically incorporates the proper phase scaling for
		time modifications. */

			if (NO <= dz->iparam[PVOC_CHANS]){
				for (i = 0; i < NO+2; i++)
					*(syn+i) = *(anal+i);
			}
			else {
				for (i = 0; i <= dz->iparam[PVOC_CHANS]+1; i++) 
					*(syn+i) = *(anal+i);
				for (i = dz->iparam[PVOC_CHANS]+2; i < NO+2; i++) 
					*(syn+i) = 0.0f;
			}

			for(i=0, i0=syn, i1=syn+1; i<= NO2; i++,i0+=2,i1+=2){
				mag = *i0;
				*(oldOutPhase + i) += *i1 - ((float) i * F);
				phase = *(oldOutPhase + i) * TwoPioverR;
				*i0 = (float)((double)mag * cos((double)phase));
				*i1 = (float)((double)mag * sin((double)phase));
			}

	/* synthesis: The synthesis subroutine uses the Weighted Overlap-Add
		technique to reconstruct the time-domain signal.  The (dz->iparam[PVOC_CHANS]/2 + 1)
		phase vocoder channel outputs at time n are inverse Fourier
		transformed, windowed, and added into the output array.  The
		subroutine thinks of output as a shift register in which 
		locations are referenced modulo obuflen.  Therefore, the main
		program must take care to zero each location which it "shifts"
		out (to standard output). The subroutines reals and fft
		together perform an efficient inverse FFT.  */

			if((exit_status = reals_(syn,bsyn,NO2,2))<0)
				return(exit_status);
			if((exit_status = fft_(syn,bsyn,1,NO2,1,2))<0)
				return(exit_status);

			j = nO - synWinLen - 1;
			while (j < 0)
				j += obuflen;
			j = j % obuflen;

			k = nO - synWinLen - 1;
			while (k < 0)
				k += NO;
			k = k % NO;

			for (i = -synWinLen; i <= synWinLen; i++) {	/*overlap-add*/
				if (++j >= obuflen)
					j -= obuflen;
				if (++k >= NO)
					k -= NO;
				*(output + j) += *(syn + k) * *(synWindow + i);
			}

#ifdef SINGLE_SAMP
			for (i = 0; i < IOi; i++){	/* shift out next IOi values */
				fputfloat(nextOut,dz->ofd);
				*(nextOut++) = 0.;
				if (nextOut >= (output + obuflen))
					nextOut -= obuflen;
				outCount++;
			}
#else
			for (i = 0; i < IOi;){	/* shift out next IOi values */
				int j;
				int todo = min(IOi-i, output+obuflen-nextOut);

// NEW 2014 -->
				if((exit_status = outfloats(nextOut,&maxsample,&minsample,&local_maxsample,&num_overflows,todo,finished,dz))<0)
					return(exit_status);
// <-- NEW 2014
				i += todo;
				outCount += todo;
				for(j = 0; j < todo; j++)
					*nextOut++ = 0.0f;
				if (nextOut >= (output + obuflen))
					nextOut -= obuflen;

			}
#endif
		}
					
		if(flag 							/* flag means do this operation only once */
		&& (nI > 0) && (Dd < D)){			/* EOF detected */
			flag = 0;
			endsamp = nI + analWinLen - (D - Dd);
		}

		nI += D;				/* increment time */
		nO += IO;
								/* Dd = D except when the end of the sample stream intervenes */
		Dd = min(D, max(0, D+endsamp-nI-analWinLen));

		if (nO > (synWinLen + I))
			Ii = I;
		else if (nO > synWinLen)
			Ii = nO - synWinLen;
		else {
			Ii = 0;
			for (i=nO+synWinLen; i<obuflen; i++) {
				if (i > 0)
					*(output+i) = 0.0f;
			}
		}
		IOi = Ii;


		if(nI > samptime && (exit_status = pvoc_time_display(nI,isr,&samptime,dz))<0)
			return(exit_status);
	}	/* End of main while loop */

	if(!dz->iparam[PVOC_ANAL_ONLY]) {
		nMaxOut = endsamp;
		while (outCount <= nMaxOut){
#ifdef SINGLE_SAMP
			outCount++;
			fputfloat(nextOut++,dz->ofd);
			if (nextOut >= (output + obuflen))
				nextOut -= obuflen;
#else
			int todo = min(nMaxOut-outCount, output+obuflen-nextOut);
			if(todo == 0)
				break;
// NEW 2014 -->
			if((exit_status = outfloats(nextOut,&maxsample,&minsample,&local_maxsample,&num_overflows,todo,finished,dz))<0)
				return(exit_status);
// <-- NEW 2014
			outCount += todo;
			nextOut += todo;
			if (nextOut >= (output + obuflen))
				nextOut -= obuflen;

#endif
		}
		if((exit_status = pvoc_time_display((int)endsamp,isr,&samptime,dz))<0)
			return(exit_status);
	}

epilog:

#ifndef NOOVERCHK
	if(num_overflows > 0) {
		biggest =  maxsample;
		if(-minsample > maxsample)
			biggest = -minsample;
		fprintf(stdout, "WARNING: %d samples overflowed, and were clipped\n",num_overflows);
		fprintf(stdout, "WARNING: maximum sample was %f  :  minimum sample was %f\n",maxsample,minsample);
		fprintf(stdout, "WARNING: You should reduce source level to avoid clipping: use gain of <= %lf\n",1.0/biggest);
		fflush(stdout);
	}

#endif
	return(FINISHED);
}

/*MCA
 *	Convert floats -> shorts explicitly, since we are compiled with
 *	hardware FP(probably), and the sound filing system is not!
 *	(even without this, it should be more efficient!)
 */

// NEW 2014 -->
int outfloats(float *nextOut,float *maxsample,float *minsample,float *local_maxsample,int *num_overflows,int todo,int finished,dataptr dz)
{
	static float *sbuf = 0;
	static int sblen = 0;
	float *sp;
	int cnt;
	float val;
	float local_minsample = 0.0;
	*local_maxsample = 0.0;
	if(sblen < todo) {
		if(sbuf != 0)
			free(sbuf); 
		if((sbuf = (float *)malloc(todo*sizeof(float))) == 0) {
			sprintf(errstr, "pvoc: can't allocate output buffer\n");
			return(MEMORY_ERROR);
		}
		sblen = todo;
	}

	sp = sbuf;
#ifdef NOOVERCHK
	for(cnt = 0; cnt < todo; cnt++)
		*sp++ = *nextOut++;	
#else
	for(cnt = 0; cnt < todo; cnt++) {
		val = *nextOut++;
		if(val >= 1.0 || val <= -1.0) {
			(*num_overflows)++;
			if(val > 0.0f) {
				if(val > *maxsample)
					*maxsample = val;
				val = 1.0f;
			}
			if(val < 0.0f) {
				if(val < *minsample)
					*minsample = val;
				val = -1.0f;
			}
		}
		*sp = val;
		if(*sp > *local_maxsample)
			*local_maxsample = val;
		if(*sp < local_minsample)
			local_minsample = val;
		sp++;
	}
#endif
	*local_maxsample = max(-local_minsample,*local_maxsample);
	if(finished && (*local_maxsample == 0.0))	//	Input read finished && Output buffer empty
		todo = 0;
	if(todo > 0) {
		if(write_samps_pvoc(sbuf, todo, dz) < 0) {
			sfperror("pvoc: write error");
			return(SYSTEM_ERROR);
		}
	}
	return(FINISHED);
// <-- NEW 2014
}

/************************************ SNDWRITE_HEADER ************************************/

int sndwrite_header(int N2,int *Nchans,float *arate,float R,int D,int origsize,int *isr,int M,dataptr dz)
{
	if (dz->iparam[PVOC_ANAL_ONLY]){

		if (dz->iparam[PVOC_ENVOUT_ONLY] || dz->iparam[PVOC_MAGOUT_ONLY]){
			*Nchans = N2 + 1;
			if(sndputprop(dz->ofd,"blocksize",(char *)Nchans,sizeof(int)) < 0 )
				fprintf(stdout,"WARNING: pvoc failed to write bloksize data\n");
			*arate = (float)((float)(*Nchans)*R/(float)D);
		} else {
			*Nchans = dz->iparam[PVOC_CHANS] + 2;
			dz->outfile->channels = *Nchans;
			*arate = (float)(R/(float)D);
		}
		dz->outfile->srate = (int)(*arate);
		dz->outfile->arate = *arate;
		dz->outfile->origstype = origsize;
		dz->outfile->origrate = *isr;
		dz->outfile->Mlen = M;
		dz->outfile->Dfac = D;

	} else {	/* Synthesis */
	    *isr = (int)R;
		dz->outfile->srate = *isr;
	    *Nchans = 1;
		dz->outfile->channels = *Nchans;
	}
	fflush(stdout);
	return(FINISHED);
}

/****************************** PVOC_PREPROCESS ******************************/

int pvoc_preprocess(dataptr dz)
{
	int ampfrqpair_cnt;
	dz->iparam[PVOC_ENVOUT_ONLY] = FALSE;
	dz->iparam[PVOC_MAGOUT_ONLY] = FALSE;
	dz->iparam[PVOC_ANAL_ONLY]   = FALSE;
	switch(dz->process) {
	case(PVOC_ANAL):
		dz->iparam[PVOC_CHANS]       = dz->iparam[PVOC_CHANS_INPUT];
		dz->iparam[PVOC_WIN_OVERLAP] = dz->iparam[PVOC_WINOVLP_INPUT]-1;
		switch(dz->mode) {
		case(STANDARD_ANAL):	dz->iparam[PVOC_ANAL_ONLY] = TRUE;										break;
		case(ENVEL_ONLY):		dz->iparam[PVOC_ANAL_ONLY] = TRUE; dz->iparam[PVOC_ENVOUT_ONLY] = TRUE; break;
		case(MAG_ONLY):			dz->iparam[PVOC_ANAL_ONLY] = TRUE; dz->iparam[PVOC_MAGOUT_ONLY] = TRUE; break;
		default:
			sprintf(errstr,"Unknown mode in pvoc_preprocess()\n");
			return(PROGRAM_ERROR);
		}
		break;
	case(PVOC_SYNTH):		
		dz->iparam[PVOC_CHANS] = dz->infile->channels - 2;
		/*RWD need this! */
		dz->iparam[PVOC_PARTIAL_RESYNTH] = FALSE;
		break;
	case(PVOC_EXTRACT):		
		dz->iparam[PVOC_CHANS] = dz->iparam[PVOC_CHANS_INPUT];
  		ampfrqpair_cnt = dz->iparam[PVOC_CHANS]/2;
		dz->iparam[PVOC_WIN_OVERLAP] = dz->iparam[PVOC_WINOVLP_INPUT]-1;
		if(dz->iparam[PVOC_CHANSLCT_INPUT]) {
			dz->iparam[PVOC_SELECTED_CHAN]  = dz->iparam[PVOC_CHANSLCT_INPUT];
			dz->iparam[PVOC_PARTIAL_RESYNTH] = TRUE;
		}
		dz->iparam[PVOC_AF_PAIR_LO] = 0;
		if(dz->iparam[PVOC_LOCHAN_INPUT] > 0) {
			dz->iparam[PVOC_AF_PAIR_LO] = min(dz->iparam[PVOC_LOCHAN_INPUT],ampfrqpair_cnt);
			dz->iparam[PVOC_PARTIAL_RESYNTH] = TRUE;
		}
		dz->iparam[PVOC_AF_PAIR_HI] = ampfrqpair_cnt;
		if(dz->iparam[PVOC_HICHAN_INPUT] > 0) {
			dz->iparam[PVOC_AF_PAIR_HI] = min(dz->iparam[PVOC_HICHAN_INPUT],ampfrqpair_cnt);
			dz->iparam[PVOC_PARTIAL_RESYNTH] = TRUE;
		}
		if((dz->iparam[PVOC_CHANSLCT_INPUT] == 0) && (dz->iparam[PVOC_LOCHAN_INPUT] == 0)
		&& (dz->iparam[PVOC_HICHAN_INPUT] == 0)) {
			sprintf(errstr,"As no channels have been selected, output sound would be the same as the input.\n");
			return(DATA_ERROR);
		}
		break;
	default:
		sprintf(errstr,"Unknown process in pvoc_preprocess()\n");
		return(PROGRAM_ERROR);
	}
	/* Force PVOC_CHANS to be even */
	dz->iparam[PVOC_CHANS] = dz->iparam[PVOC_CHANS] + (dz->iparam[PVOC_CHANS]%2);
	return(FINISHED);
}

/*********************************** PVOC_TIME_DISPLAY ***********************************/

int pvoc_time_display(int nI,int srate,int *samptime,dataptr dz)
{
	int true_chans, true_srate;
	int true_outfiletype;
	switch(dz->process) {
	case(PVOC_ANAL):
		true_outfiletype = dz->outfiletype;
		dz->outfiletype  = SNDFILE_OUT;	
	  	display_virtual_time(nI,dz);	
		dz->outfiletype	 = true_outfiletype;
	  	break;	 
	case(PVOC_SYNTH): 	
		true_chans = dz->infile->channels;
		true_srate = dz->infile->srate;
		dz->infile->channels = 1;
		dz->infile->srate    = srate;	
	  	display_virtual_time(nI,dz);	
		dz->infile->channels = true_chans;
		dz->infile->srate    = true_srate;
		break;	 
	case(PVOC_EXTRACT): 	
		display_virtual_time(nI,dz);	
		break;	 
	default:
		sprintf(errstr,"Unknown process in pvoc_time_display()\n");
		return(PROGRAM_ERROR);
	}
	*samptime += SAMP_TIME_STEP;
	return(FINISHED);
}

/******************************* WRITE_SAMPS_PVOC ********************************/

int write_samps_pvoc(float *bbuf,int samps_to_write,dataptr dz)
{
	
	int samps_written;
	int i,j;
   	int granularity = 22100;
   	int this_granularity = 0;
	float val;

	if(dz->needpeaks){
		for(i=0;i < samps_to_write; i += dz->outchans){
			for(j = 0;j < dz->outchans;j++){
				val = (float)fabs(bbuf[i+j]);
				/* this way, posiiton of first peak value is stored */
				if(val > dz->outpeaks[j].value){
					dz->outpeaks[j].value = val;
					dz->outpeaks[j].position = dz->outpeakpos[j];
				}
			}
			/* count framepos */
			for(j=0;j < dz->outchans;j++)
				dz->outpeakpos[j]++;
		}
	}
	if((samps_written = fputfbufEx(bbuf,samps_to_write,dz->ofd))<=0) {
		sprintf(errstr,"Can't write to output soundfile: %s\n",sferrstr());
		return(SYSTEM_ERROR);
	}
	dz->total_samps_written += samps_written;	
    this_granularity += samps_to_write;
    if(this_granularity > granularity){
        display_virtual_time(dz->total_samps_written,dz);
        this_granularity -= granularity;
    }
	return(FINISHED);
}
