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
#include <cdpmain.h>
#include <tkglobals.h>
#include <pnames.h>
#include <distort.h>
#include <processno.h>
#include <modeno.h>
#include <globcon.h>
#include <logic.h>
#include <filetype.h>
#include <mixxcon.h>
#include <distort1.h>
#include <flags.h>
#include <speccon.h>
#include <arrays.h>
#include <special.h>
#include <formants.h>
#include <sfsys.h>
#include <osbind.h>
#include <string.h>
#include <arrays.h>
#include <math.h>
#include <ctype.h>

/*

PRESCALE PULSE-ENVELOPE FILE BETWEEN 0 and 1 in time and copy into parray[PULSENV]
PRESCALE PULSE-TRANSPOS FILE BETWEEN 0 and 1 in time and copy into parray[PULSTRN]

NB randomisation pulse-transposition only works if it's a brktable

HAS TO BE MONO SNDFILE;

*/

static int	write_pulse_to_output(int *pbufpos,int *obufpos,dataptr dz);
static void	randomise_pulse_transposition(dataptr dz);
static int	randomise_pulse_envelope(int obufpos,dataptr dz);
static double	gen_sndtabindex(int obufpos,double *sndtabindex,double *pulsenvindex,double *envstep,
					int insertsmps,double tablen,int randomise_transpos,dataptr dz);
static int	extract_waveform(int *waveset_start,dataptr dz);
static int	get_wavesetstart(int ibufpos,dataptr dz);
/* in ap_distort */
static int	prescale_pulse_transpos(dataptr dz);
static int	read_impulse_amp_val(double thistime,double *amp,dataptr dz);

/************************************* PREPROCESS_PULSE *************************************/

int preprocess_pulse(dataptr dz)
{
	double k;
	int exit_status;
	if(dz->iparam[PULSE_ENVSIZE] > 0) {
		if((dz->parray[PULSENV] = (double *)malloc((dz->iparam[PULSE_ENVSIZE] * 2) * sizeof(double)))==NULL) {
			sprintf(errstr,"Out of memory for storing backup of original pulse envelope.\n");
			return(MEMORY_ERROR);
		}
		memcpy((char *)dz->parray[PULSENV],(char *)dz->parray[ORIG_PULSENV],(dz->iparam[PULSE_ENVSIZE] * 2) * sizeof(double));
								/* copy original envelope into parray */
		if((dz->iparray[0] = (int *)malloc(dz->iparam[PULSE_ENVSIZE] * sizeof(int)))==NULL) {
			sprintf(errstr,"Out of memory for storing markers for envelope warping.\n");
			return(MEMORY_ERROR);
		}
	}
	if(dz->brksize[PULSE_TRANSPOS] > 0) {
		if((exit_status = prescale_pulse_transpos(dz))<0)
			return(exit_status);
	} else {
		k = dz->param[PULSE_TRANSPOS] / 12.0;
		dz->param[PULSE_TRANSPOS] = pow(2.0,k);	/* convert semitones to frq-ratio */
	}
	return(FINISHED);
}

/*********************************** PRESCALE_PULSE_TRANSPOS *************************************/

int prescale_pulse_transpos(dataptr dz)
{
	int brklen = dz->brksize[PULSE_TRANSPOS] * 2;
	int maxtimindx = brklen  - 2;
	int n,m;
	double maxtime = dz->brk[PULSE_TRANSPOS][maxtimindx], scaler;
	scaler = 1.0/maxtime;
	for(n=0,m=1;n<brklen;n+=2,m+=2)
		dz->brk[PULSE_TRANSPOS][n] *= scaler;			/* scale to 0 to 1 time range */
	dz->brk[PULSE_TRANSPOS][maxtimindx] = 1.0;
	if((dz->parray[PULSTRN] = (double *)malloc(brklen * sizeof(double)))==NULL) {
		sprintf(errstr,"Out of memory for storing backup of original pulse transposition envelope\n");
		return(MEMORY_ERROR);
	}
	memcpy((char *)dz->parray[PULSTRN],(char *)dz->brk[PULSE_TRANSPOS],brklen * sizeof(double));
	return(FINISHED);
}

/************************************* DO_PULSETRAIN *************************************/

int do_pulsetrain(dataptr dz)
{
	int exit_status;
	float *ibuf = dz->sampbuf[0];	/* stores input  */
	float *obuf = dz->sampbuf[1];	/* stores output */
	float *sbuf = dz->sampbuf[2];	/* stores waveset cycoes, for SYN option  */
	float *pbuf = dz->sampbuf[3];	/* stores pulse, before writing to output */
	int srate = dz->infile->srate;
	float *spliceptr, *obufend = obuf + dz->buflen;
	int startsamp = round(dz->param[PULSE_STARTTIME] * srate);
	int insertdur = round(dz->param[PULSE_DUR] * srate);
	int last_total_ssampsread, total_ssampsread = 0, ssampsread = 0, insertsmps = 0;
	int  smpsecsize = F_SECSIZE, secsleft, edit_todo = (int)round(ENDBIT_SPLICE);
	int ibufpos = 0, ibufleft, obufpos = 0, pbufpos = 0, obufleft;
	double amp, frq, sndtabindex, pulsenvindex;
	int waveset_len = 0;
	double envstep;
	int randomise_transpos = 0, firsttime = 1;
	int smps_pretime, waveset_start;
	double interp, valdiff, k;
	float val, upval;
	int here, n, m;

	if(dz->mode == PULSE_SYNI)
		startsamp = dz->iparam[PULSE_STARTTIME];
	else
		startsamp = round(dz->param[PULSE_STARTTIME] * srate);

	/* PROCESS FILE PRIOR TO PULSE-TRAIN'S START, IF NESS */ 

	if(dz->mode == PULSE_IMP) {
		k = dz->duration;
		if(!dz->vflag[PULSE_KEEPSTART]) {
			k -= dz->param[PULSE_STARTTIME];
			if(!dz->vflag[PULSE_KEEPEND])
				k = min(dz->param[PULSE_DUR],k);
		} else if(!dz->vflag[PULSE_KEEPEND]) {
			k = dz->param[PULSE_STARTTIME] + dz->param[PULSE_DUR];
			k = min(dz->duration,k);
		}
	} else {
		k = dz->param[PULSE_DUR];
		if(dz->vflag[PULSE_KEEPEND]) {
			k += dz->duration;
			if(!dz->vflag[PULSE_KEEPSTART])
				k -= dz->param[PULSE_STARTTIME];
		} else if(dz->vflag[PULSE_KEEPSTART])
			k += dz->param[PULSE_STARTTIME];
	}
	dz->tempsize = (int)round(k * (dz->infile->srate));

	while(total_ssampsread + dz->buflen < startsamp) {
		if(( exit_status = read_samps(ibuf,dz)) < 0) {
			sprintf(errstr, "Can't read samps from input soundfile\n");
			return(SYSTEM_ERROR);
		}
		ssampsread = dz->ssampsread;
		last_total_ssampsread = total_ssampsread;
		total_ssampsread += ssampsread;
		if(dz->vflag[PULSE_KEEPSTART]) {
			if(ssampsread > 0) {
				if((exit_status = write_samps(ibuf,ssampsread,dz))<0)
					return(exit_status);
			}
		}
	}
	if((secsleft = (startsamp - total_ssampsread)/smpsecsize)> 0) {
		if(( ssampsread = fgetfbufEx(ibuf,secsleft * F_SECSIZE, dz->ifd[0],0)) < 0) {
			sprintf(errstr, "Can't read samps from input soundfile\n");
			return(SYSTEM_ERROR);
		}
		last_total_ssampsread = total_ssampsread;
		total_ssampsread += ssampsread;
		if(dz->vflag[PULSE_KEEPSTART]) {
			if(ssampsread > 0) {
				if((exit_status = write_samps(ibuf,ssampsread,dz))<0)
					return(exit_status);
			}
		}
	}
	smps_pretime = startsamp - total_ssampsread;	/* no of samples before param-specified time */

	if(( ssampsread = fgetfbufEx(ibuf, dz->buflen + F_SECSIZE, dz->ifd[0],0)) < 0) {
		sprintf(errstr, "Can't read samples from input soundfile\n");	/* To get wraparound points */
		return(SYSTEM_ERROR);
	}
	last_total_ssampsread = total_ssampsread;
	if(ssampsread > dz->buflen) {	
		sndseekEx(dz->ifd[0],last_total_ssampsread + dz->buflen,0);
		total_ssampsread = last_total_ssampsread + dz->buflen;
	} else {	/* We'RE AT END oF FILE... */	
		total_ssampsread = last_total_ssampsread + ssampsread;
		ibuf[ssampsread] = 0.0;	/* CREATE zero-val wraparound points */
		
	}

	/* FIND START OF WAVESETS */ 

	if((waveset_start = get_wavesetstart(smps_pretime,dz)) < 0) {
		sprintf(errstr,"Can't find the required waveset start within a single buffer.\n");
		return(GOAL_FAILED);
	}
	if((waveset_start > 0) && dz->vflag[PULSE_KEEPSTART]) {
		memcpy((char *)obuf,(char *)ibuf,waveset_start * sizeof(float));
		obufpos = waveset_start;	/* copy any inbuf remnant prior to pulsing, to outbuf */
	}
	ibufpos = waveset_start;

	if(dz->mode!=PULSE_IMP)	{
		if((waveset_len = extract_waveform(&waveset_start,dz))<0) {	/* NB copy rounding sample[s] at end */
			sprintf(errstr,"Can't find the required wavesets within a single buffer.\n");
			return(GOAL_FAILED);
		}
		sndtabindex = 0.0;							/* position in storage buffer */
	} else
		sndtabindex = (double)waveset_start;		/* position in input buffer */

	/* INITIALISE SHAPE AND LENGTH OF FIRST IMPULSE */ 

	if(dz->iparam[PULSE_ENVSIZE] > 0)
		randomise_pulse_envelope(obufpos,dz);	/* randomise shape of pulse, and copy reshaped pulse to read-env-table */
										/* +insert edit shoulders at start and end */
	if(!flteq(dz->param[PULSE_PITCHRAND],0.0)) {
		randomise_transpos = 1;
		if(dz->brksize[PULSE_TRANSPOS] <= 0) {
			fprintf(stdout,"WARNING: Randomisation of transposition only applies to a transposition TABLE\n");
			fflush(stdout);
			randomise_transpos = 0;
		} else
			randomise_pulse_transposition(dz);
	} else if (dz->brksize[PULSE_TRANSPOS] > 0) {		/* if NOT randomised, convert to frqratio NOW */
		for(n=0,m=1;n<dz->brksize[PULSE_TRANSPOS];n++,m+=2) {
			k = dz->parray[PULSTRN][m]/SEMITONES_PER_OCTAVE;
			dz->brk[PULSE_TRANSPOS][m] = pow(2.0,k);
		}
	}
	pulsenvindex = 0.0;
	if(dz->iparam[PULSE_ENVSIZE] <= 0)
		amp = 1.0;
	else
		amp = dz->parray[PULSENV][0];
	if(dz->brksize[PULSE_FRQ] > 0) {
		if((exit_status = read_value_from_brktable(0.0,PULSE_FRQ,dz))<0)
			return exit_status;
	}
	frq = dz->param[PULSE_FRQ];
	if(dz->param[PULSE_FRQRAND] > 0.0) {
		k = ((drand48() * 2.0) - 1.0) * dz->param[PULSE_FRQRAND];
		k /= SEMITONES_PER_OCTAVE;
		k = pow(2.0,k);
		frq *= k;
	}
	envstep = frq/srate;

	/* DO IMPULSE TRAIN */ 

	while(insertsmps < insertdur) {
		if(!firsttime && flteq(pulsenvindex,0.0)) {	/* if at start of new pulse */
			if((exit_status = write_pulse_to_output(&pbufpos,&obufpos,dz))<0)
				return(exit_status);
		}
		firsttime = 0;
		if(dz->mode!=PULSE_IMP)	{
			interp = fmod(sndtabindex,1.0);
			here = (int)floor(sndtabindex);
			val    = sbuf[here++];
			upval  = sbuf[here];
			valdiff = (upval - val) * interp;
			pbuf[pbufpos++] = (float)((val + valdiff) * amp);
			if(pbufpos >= dz->buflen) {
				sprintf(errstr,"Pulse length exceeds length of internal buffer : cannot proceed.\n");
				return(GOAL_FAILED);
			}
		} else {
			interp = fmod(sndtabindex,1.0);
			ibufpos = (int)floor(sndtabindex);
			while(ibufpos  >= ssampsread) {
				/* read extra sector, for wraparound vals for interpolation */
				if(( ssampsread = fgetfbufEx(ibuf, dz->buflen + F_SECSIZE, dz->ifd[0],0)) < 0) {
					sprintf(errstr, "Can't read samps from input soundfile\n");	/* read failed */
					return(SYSTEM_ERROR);
				}
				if(ssampsread <= 0) {	/* read zero bytes = EOF */
					if(obufpos > 0) {
						if((exit_status = write_samps(obuf,obufpos,dz))<0)
							return(exit_status);
					}
					return(FINISHED);
				}
				if(ssampsread <= dz->buflen) {	/* generate zero-val wraparound points if buffer+wrap not full */
					ibuf[ssampsread] = 0.0;
					total_ssampsread += ssampsread;
				} else {
					last_total_ssampsread = total_ssampsread;
					ssampsread = dz->buflen;	/* effective read is a buffer-full */
					total_ssampsread += ssampsread;
					sndseekEx(dz->ifd[0],last_total_ssampsread + dz->buflen,0);
				}
				ibufpos -= dz->buflen; /* as current read succeeded, last input buffer was full: so this val is >= 0 */
				sndtabindex -= (double)dz->buflen;
			}
			val    = ibuf[ibufpos++];
			upval  = ibuf[ibufpos];
			valdiff = (upval - val) * interp;
			pbuf[pbufpos++] = (float)((val + valdiff) * amp);
			if(pbufpos >= dz->buflen) {
				sprintf(errstr,"Pulse length exceeds length of internal buffer : cannot proceed\n");
				return(GOAL_FAILED);
			}
		}
		amp = gen_sndtabindex(obufpos,&sndtabindex,&pulsenvindex,&envstep,insertsmps,(double)waveset_len,randomise_transpos,dz);
		insertsmps++;
	}
	if(pbufpos > 0) {
		if((exit_status = write_pulse_to_output(&pbufpos,&obufpos,dz))<0)
			return(exit_status);
	}

	/* DO REST OF INPUT FILE AFTER PULSETRAIN, IF NESS */ 

	if(dz->vflag[PULSE_KEEPEND]) {
		for(;;) {
			ibufleft = ssampsread - ibufpos;
			obufleft = dz->buflen - obufpos;
			spliceptr = obuf+obufpos;
			if(ibufleft >= obufleft) {
				memcpy((char *)(obuf+obufpos),(char *)(ibuf+ibufpos),obufleft * sizeof(float));
				while(edit_todo > 0) {
					k = *spliceptr * ((ENDBIT_SPLICE - (double)edit_todo)/ENDBIT_SPLICE);
					*spliceptr++ = (float)k;
					edit_todo--;
					if(spliceptr >= obufend)
						break;
				}						
				ibufpos += obufleft;
				if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
					return(exit_status);
				obufpos = 0;
			} else {
				memcpy((char *)(obuf+obufpos),(char *)(ibuf+ibufpos),ibufleft * sizeof(float));
				while(edit_todo > 0) {
					k = *spliceptr * ((ENDBIT_SPLICE - (double)edit_todo)/ENDBIT_SPLICE);
					*spliceptr++ = (float)k;
					edit_todo--;
					if(spliceptr >= obufend)
						break;
				}						
				obufpos += ibufleft;
				if(( exit_status = read_samps(ibuf, dz)) < 0) {
					sprintf(errstr, "Can't read samps from input soundfile\n");
					return(SYSTEM_ERROR);
				}
				ssampsread = dz->ssampsread;
				if(ssampsread <= 0)
					break;
				ibufpos = 0;
			}
		}
	}
	if(obufpos > 0) {
		if((exit_status = write_samps(obuf,obufpos,dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}
	
/************************************* GET_WAVESETSTART *************************************/

int get_wavesetstart(int ibufpos,dataptr dz)
{
	float *ibuf = dz->sampbuf[0];
	
	if(ibuf[ibufpos] > 0.0) {
		while(ibuf[ibufpos] > 0.0) {
			if(++ibufpos >= dz->buflen)
				return(-1);
		}
	} else if(ibuf[ibufpos] < 0.0) { 
		while(ibuf[ibufpos] < 0.0) {
			if(++ibufpos >= dz->buflen)
				return(-1);
		}
	} else {
		while(smpflteq(ibuf[ibufpos],0.0)) {
			if(++ibufpos >= dz->buflen)
				return(-1);
		}
	}
	return(ibufpos);
}

/************************************* EXTRACT_WAVEFORM *************************************/

// NOV 2002 MAXSAMP -> F_MAXSAMP as well as short -> float
int extract_waveform(int *waveset_start,dataptr dz)
{
	int ibufpos = *waveset_start, waveform_len = 0;
	float *ibuf = dz->sampbuf[0];
	float *sbuf = dz->sampbuf[2];
	double frqlimit = dz->infile->srate/PULSE_FRQLIM;
	double amplimit = dbtogain((double)PULSE_DBLIM) * F_MAXSAMP;
	int capture_samps = 0, capture_wavesets = 0;
	int last_waveform_len = 0;
	double maxval, last_maxval = 0.0, wcntr = 0;

	if(dz->mode == PULSE_SYN)
		capture_samps = (int)round(dz->param[PULSE_WAVETIME] * dz->infile->srate);
	else
		capture_wavesets = dz->iparam[PULSE_WAVETIME];
	while(smpflteq(ibuf[ibufpos],0.0)) {
		if(++ibufpos >= dz->buflen)
			return(-1);
	}
	*waveset_start = ibufpos;
	maxval = fabs(ibuf[ibufpos]);

	switch(dz->mode) {
	case(PULSE_SYN):
		for(;;) {
			if(ibuf[ibufpos] > 0.0) {
				while(ibuf[ibufpos] > 0.0) {
					if(++ibufpos >= dz->buflen)
						return(-1);
					maxval = max(maxval,fabs(ibuf[ibufpos]));
				}
				while(ibuf[ibufpos] <= 0.0) {
					if(++ibufpos >= dz->buflen)
						return(-1);
					maxval = max(maxval,fabs(ibuf[ibufpos]));
				}
			} else if(ibuf[ibufpos] < 0.0) { 
				while(ibuf[ibufpos] < 0.0) {
					if(++ibufpos >= dz->buflen)
						return(-1);
					maxval = max(maxval,fabs(ibuf[ibufpos]));
				}
				while(ibuf[ibufpos] >= 0.0) {
					if(++ibufpos >= dz->buflen)
						return(-1);
					maxval = max(maxval,fabs(ibuf[ibufpos]));
				}
			}
			if((waveform_len = ibufpos - *waveset_start) >= capture_samps)
				break;
			else {
				last_waveform_len = waveform_len;
				last_maxval = maxval;
			}
		}
		if(capture_samps - last_waveform_len < waveform_len - capture_samps) {
			waveform_len = last_waveform_len;
			maxval = last_maxval;
		}
		break;
	case(PULSE_SYNI):
		while(wcntr < capture_wavesets) {
			if(ibuf[ibufpos] > 0.0) {
				while(ibuf[ibufpos] > 0.0) {
					if(++ibufpos >= dz->buflen)
						return(-1);
					maxval = max(maxval,fabs(ibuf[ibufpos]));
				}
				while(ibuf[ibufpos] <= 0.0) {
					if(++ibufpos >= dz->buflen)
						return(-1);
					maxval = max(maxval,fabs(ibuf[ibufpos]));
				}
			} else if(ibuf[ibufpos] < 0.0) {
				while(ibuf[ibufpos] < 0.0) {
					if(++ibufpos >= dz->buflen)
						return(-1);
					maxval = max(maxval,fabs(ibuf[ibufpos]));
				}
				while(ibuf[ibufpos] >= 0.0) {
					if(++ibufpos >= dz->buflen)
						return(-1);
					maxval = max(maxval,fabs(ibuf[ibufpos]));
				}
			}
			wcntr++;
		}
		waveform_len = ibufpos - *waveset_start;
		break;
	}
	if(waveform_len <= 0) {
		sprintf(errstr,"Failed to establish a viable sound to loop\n");
		return(GOAL_FAILED);
	} else if((double)waveform_len <= frqlimit) {
		frqlimit = (double)dz->infile->srate/(double)waveform_len;
		fprintf(stdout,"WARNING: Waveset length is only %d samples, corresponding to frq >= %.1lf hz\n",waveform_len,frqlimit);
		fflush(stdout);
	}
	if((double)maxval <= amplimit) {
		fprintf(stdout,"WARNING: Waveset amplitude is <= %lf dB\n",(double)PULSE_DBLIM);
		fflush(stdout);
	}
	memcpy((char *)sbuf,(char *)(ibuf + *waveset_start),waveform_len * sizeof(float));
	return(waveform_len);
}

/************************************* GEN_TABINDEX *************************************/

double gen_sndtabindex(int obufpos,double *sndtabindex,double *pulsenvindex,double *envstep,
					int insertsmps,double tablen,int randomise_transpos,dataptr dz)
{
	int exit_status;
	double thistime, amp, frq, k;
	*pulsenvindex += *envstep;
	if(*pulsenvindex >= 1.0) {
		if(dz->iparam[PULSE_ENVSIZE] <= 0) {			/* no impulse: must continue to end of sndtable (zero-cross) to avoid glitch */
			*sndtabindex += dz->param[PULSE_TRANSPOS];	/* transpos sticks at previous value as table end has been reached */
			if(*sndtabindex >= tablen) {
				*pulsenvindex = 0.0;				/* reset to start of envelope */
				*sndtabindex = 0.0;
				thistime = (double)insertsmps/(double)dz->infile->srate;
				if(dz->brksize[PULSE_FRQ] > 0) {
					if((exit_status = read_value_from_brktable(thistime,PULSE_FRQ,dz))<0)
						return exit_status;
				}
				frq = dz->param[PULSE_FRQ];
				if(dz->param[PULSE_FRQRAND] > 0.0) {
					k = ((drand48() * 2.0) - 1.0) * dz->param[PULSE_FRQRAND];
					k /= SEMITONES_PER_OCTAVE;
					k = pow(2.0,k);
					frq *= k;
				}
				*envstep = frq/dz->infile->srate;
				if(randomise_transpos && (dz->param[PULSE_PITCHRAND] > 0.0))
					randomise_pulse_transposition(dz);
			}
			return(1.0);							/* no impulse: amplitude is always 1.0 */
		}
	/* GET NEW VALUE OF PULSE FRQ, AND HENCE OF STEP IN ENVEL TABLE */
		thistime = (double)insertsmps/(double)dz->infile->srate;
		if(dz->brksize[PULSE_FRQ] > 0) {
			if((exit_status = read_value_from_brktable(thistime,PULSE_FRQ,dz))<0)
				return exit_status;
		}
		frq = dz->param[PULSE_FRQ];
		if(dz->param[PULSE_FRQRAND] > 0.0) {
			k = ((drand48() * 2.0) - 1.0) * dz->param[PULSE_FRQRAND];
			k /= SEMITONES_PER_OCTAVE;
			k = pow(2.0,k);
			frq *= k;
		}
		*envstep = frq/dz->infile->srate;
		if(dz->iparam[PULSE_ENVSIZE] > 0)
			randomise_pulse_envelope(obufpos,dz);	/* randomise shape of pulse, and copy reshaped pulse to read-env-table */
										/* insert edit shoulders at start and end */
										/* orig envelope has been copied to parray[PULSENV], variant copied back to brk[] */
		
		if(randomise_transpos && (dz->param[PULSE_PITCHRAND] > 0.0))
			randomise_pulse_transposition(dz);
		
		*pulsenvindex = 0.0;				/* reset to start of envelope */
		if((exit_status = read_impulse_amp_val(*pulsenvindex,&amp,dz))<0)
			return exit_status;
		if(dz->mode!=PULSE_IMP)
			*sndtabindex = 0.0;
		else
			*sndtabindex = floor(*sndtabindex + 1.0);	/* sync table pointer with actual samples in input stream */
	} else {
		if(dz->iparam[PULSE_ENVSIZE] <= 0) {			/* no impulse */
			if(dz->brksize[PULSE_TRANSPOS]) {
				if((exit_status = read_value_from_brktable(*pulsenvindex,PULSE_TRANSPOS,dz))<0)
					return exit_status;
			}
			*sndtabindex += dz->param[PULSE_TRANSPOS];
			*sndtabindex = fmod(*sndtabindex,tablen);	/* cycle around soundtable */
			return(1.0);								/* no impulse: amplitude is always 1.0 */
		}
		if((exit_status = read_impulse_amp_val(*pulsenvindex,&amp,dz))<0)
			return exit_status;
		if(dz->brksize[PULSE_TRANSPOS]) {
			if((exit_status = read_value_from_brktable(*pulsenvindex,PULSE_TRANSPOS,dz))<0)
				return exit_status;
		}
		*sndtabindex += dz->param[PULSE_TRANSPOS];
		if(dz->mode!=PULSE_IMP)
			*sndtabindex = fmod(*sndtabindex,tablen);
	}
	return amp;
}

/************************************* RANDOMISE_PULSE_ENVELOPE *************************************/

int randomise_pulse_envelope(int obufpos,dataptr dz)
{
	int n, m, kk, maxpos, maxmark;
	int *mark = dz->iparray[0];
	double ratio, above, below, randmove, k, maxi, timearound;
	double convertor = (dz->param[PULSE_FRQ] * ENDBIT_SPLICE)/(double)dz->infile->srate;
	double abovestep, belowstep, minbelowtime, minabovetime;
	int not_above, not_below;

	dz->parray[PULSENV][0] = dz->parray[ORIG_PULSENV][0];
	for(n=3;n<dz->iparam[PULSE_ENVSIZE]*2;n+=2) {
		if(dz->parray[ORIG_PULSENV][n] < dz->parray[ORIG_PULSENV][n-2]) {
			ratio = dz->parray[PULSENV][n-2]/dz->parray[ORIG_PULSENV][n-2];	/* ratio of previous val's new height to it's old height */
			above = (dz->parray[ORIG_PULSENV][n-2] - dz->parray[ORIG_PULSENV][n]) * ratio; /* vert distance twixt preceding point & this, scaled */
			below = (dz->parray[ORIG_PULSENV][n]) * ratio;							/* vert distance twixt 0 & this, scaled */
			k = ((drand48() * 2.0) - 1.0) * dz->param[PULSE_SHAPERAND];
			if(k >= 0.0)
				randmove = above * k;
			else
				randmove = below * k;
			dz->parray[PULSENV][n] = below + randmove;
		} else if(dz->parray[ORIG_PULSENV][n] > dz->parray[ORIG_PULSENV][n-2]) {
			ratio = (1.0 - dz->parray[PULSENV][n-2])/(1.0 - dz->parray[ORIG_PULSENV][n-2]); /* simil ratio of dist from top [1.0] */
			above = (1.0 - dz->parray[ORIG_PULSENV][n]) * ratio; /* vert distance twixt top [1.0] & this point, scaled */
			below = (dz->parray[ORIG_PULSENV][n] - dz->parray[ORIG_PULSENV][n-2]) * ratio; /* vert distance preceeding pnt & this, scaled */
			k = ((drand48() * 2.0) - 1.0) * dz->param[PULSE_SHAPERAND];
			if(k >= 0.0)
				randmove = above * k;
			else
				randmove = below * k;
			dz->parray[PULSENV][n] = dz->parray[PULSENV][n-2] + below + randmove;
		} else {
			dz->parray[PULSENV][n] = dz->parray[PULSENV][n-2];
		}
	}
	/* randomise pulse-env timings */
	for(m=1;m < dz->iparam[PULSE_ENVSIZE]-1; m++)
		mark[m] = 0;
	kk = dz->iparam[PULSE_ENVSIZE] - 2;
	while(kk>0) {
		maxi = 0.0;
		maxpos = 0;
		maxmark = 0;
		not_above = 0;
		not_below = 0;
		for(m=1,n=2;n < (dz->iparam[PULSE_ENVSIZE]-1) * 2; n+=2,m++) {
			if(mark[m]==0) {
				timearound = dz->parray[PULSENV][n+2] - dz->parray[PULSENV][n-2];
				if(timearound > maxi) {
					maxi = timearound;
					maxpos = n;
					maxmark = m;
				}
			}
		}
		abovestep = dz->parray[PULSENV][maxpos+3] - dz->parray[PULSENV][maxpos+1]; 
		belowstep = dz->parray[PULSENV][maxpos+1] - dz->parray[PULSENV][maxpos-1];
		minbelowtime = fabs(convertor * belowstep);
		minabovetime = fabs(convertor * abovestep);
		if((above = dz->parray[PULSENV][maxpos+2] - dz->parray[PULSENV][maxpos]) < minabovetime)
			not_above = 1;
		if((below = dz->parray[PULSENV][maxpos] - dz->parray[PULSENV][maxpos-2]) < minbelowtime)
			not_below = 1;
		if(not_above) {
			if(not_below) {
				sprintf(errstr,"WARNING: possible glitch, due to sudden envelope change, after %lf secs\n",
				(dz->total_samps_written + obufpos)/(double)dz->infile->srate);
				fflush(stdout);
				k = 0.0;
			} else
				k = drand48() * -1.0 * dz->param[PULSE_TIMERAND];
		} else if(not_below)
			k = drand48() * dz->param[PULSE_TIMERAND];
		else
			k = ((drand48() * 2.0) - 1.0) * dz->param[PULSE_TIMERAND];
/* KLUDGE --> */
		k *= 0.5;
/* <-- KLUDGE */
		if(k >= 0.0) 
			randmove = above * k;
		else
			randmove = below * k;
		dz->parray[PULSENV][maxpos] += randmove;
		mark[maxmark] = 1;
		kk--;
	}
	return(FINISHED);
}

/************************************* RANDOMISE_PULSE_TRANSPOSITION *************************************/

void randomise_pulse_transposition(dataptr dz)
{
	int n, m;
	double k;
	for(n=0,m=1;n<dz->brksize[PULSE_TRANSPOS];n++,m+=2) {
		if(flteq(dz->brk[PULSE_TRANSPOS][m],0.0))
			dz->brk[PULSE_TRANSPOS][m] = 1.0;
		else {
			k = ((drand48() * 2.0) - 1.0) * dz->param[PULSE_PITCHRAND];
			k *= dz->parray[PULSTRN][m]/SEMITONES_PER_OCTAVE;
			dz->brk[PULSE_TRANSPOS][m] = pow(2.0,k);
		}
	}
}

/************************************* WRITE_PULSE_TO_OUTPUT *************************************/

int write_pulse_to_output(int *pbufpos,int *obufpos,dataptr dz)
{
	int exit_status;
	float *obuf = dz->sampbuf[1];
	float *pbuf = dz->sampbuf[3];
	int n, m;
	if(dz->iparam[PULSE_ENVSIZE] > 0) {
		for(n=0;n<round(ENDBIT_SPLICE);n++)		/* WRITE PULSE-BUFFER TO OUTPUT, with relevant splices if there's an impulse envelope */
			pbuf[n] = (float)(pbuf[n] * (double)n/ENDBIT_SPLICE);
		for(m = *pbufpos-1,n=0;n<round(ENDBIT_SPLICE);n++,m--)
			pbuf[m] = (float)(pbuf[m] * (double)n/ENDBIT_SPLICE);
	}
	for(n=0;n<*pbufpos;n++) {
		obuf[(*obufpos)++] = pbuf[n];
		if(*obufpos >= dz->buflen) {
			if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
				return(exit_status);
			*obufpos = 0;
		}
	}
	*pbufpos = 0;
	return(FINISHED);
}

/**************************** READ_IMPULSE_AMP_VAL *****************************/

int read_impulse_amp_val(double thistime,double *amp,dataptr dz)
{
    double *p, lotime, loval, hitime, hival, timediff, step, ratio;
	p = dz->parray[PULSENV];
	if(flteq(thistime,0.0)) {
		*amp = dz->parray[PULSENV][1];
		return(FINISHED);
	}
	while(*p < thistime)
		p += 2;
	p -= 2;
	lotime = *p++;
	loval  = *p++;
	hitime = *p++;
	hival  = *p++;
	timediff = thistime - lotime;
	step = hival - loval;
	ratio = timediff/(hitime-lotime);
	*amp = loval + (step * ratio);
    return(FINISHED);
}


