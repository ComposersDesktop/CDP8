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
#include <tkglobals.h>
#include <pnames.h>
#include <globcon.h>
#include <modeno.h>
#include <arrays.h>
#include <flags.h>
#include <strange.h>
#include <cdpmain.h>
#include <formants.h>
#include <speccon.h>
#include <sfsys.h>
#include <osbind.h>
#include <string.h>
#include <strange.h>

#ifdef unix
#define round(x) lround((x))
#endif

#define	RATIO_LIMIT 			 (20.0)	   /* Max ratio actual chanamp allowed to exceed
											  formant derived amp for frq */
#define SMOOTHER				  (0.5)
#define	VERY_SMALL (0.0000000000000001)

static int  zero_sampbuf(dataptr dz);
static int  establish_frq_params(double *scalefact,dataptr dz);
static int  write_partials_moving_down(double scalefact,dataptr dz);
static int  write_partials_moving_up(double scalefact,dataptr dz);
static int  cut_or_rolloff_high_frq(dataptr dz);
static int  selfglis_down_within_spectral_envelope(int vc,double transpos,dataptr dz);
static int  glis_down_within_spectral_envelope(double fundamental,double *thisfrq,double scalefact,int *n,dataptr dz);
static int  selfglis_up_within_spectral_envelope(int vc,double transpos,dataptr dz);
static int  glis_up_within_spectral_envelope(double fundamental,double *thisfrq,double scalefact,int *n,dataptr dz);
static int  do_waver(int *wc,int *w_to_buf,int *samps_read,int wcnt,int upwcnt,int dnwcnt,dataptr dz);
static int  spectrally_stretch_window(double thisstr,dataptr dz);
static int  advance_along_windows(int *wc,int *w_to_buf,int *samps_read,dataptr dz);
static int  warp_pitch(int *avcnt,dataptr dz);
static int  warp_time(int avcnt,dataptr dz);
static int  get_pitch_averages(int *avcnt,dataptr dz);
static int  setup_and_do_pitchwarp(int start,int avg_no,int *lastpos,double *lastmidi,dataptr dz);
static int  do_pitch_wiggle(double *thismidi,double ttime, dataptr dz);
static int  warp_pitches(int thispos,int diff,int *lastpos,double thismidi,double *lastmidi,dataptr dz);
static int  setup_and_do_timewarp(float **thisbuf,float **nextbuf,int n,int *lastoutpos,
				int *lastinpos,int *last_total_wndws,dataptr dz);
static int  adjust_input_buffers(float **thisbuf,float **nextbuf,int thisindex,
				int *last_total_wndws,int do_next,dataptr dz);
static int  transpos_initial_windows(float **thisbuf,float **nextbuf,int cnt,int *last_total_wndws,dataptr dz);
static int	do_time_wiggle(double *ttime,dataptr dz);
static int  do_timewarp(float **thisbuf,float **nextbuf,int thisinpos,int outdiff,
				int *lastinpos,int *last_total_wndws,dataptr dz);
static int  find_currentbuf_index(int *currentbuf_index,int *last_total_wndws,float **thisbuf,dataptr dz);
static int  find_currentbuf_nextindex(int *currentbuf_nextindex,int last_total_wndws,dataptr dz);
static int  setup_thisbuf_and_nextbuf(int last_total_wndws,float **thisbuf,float **nextbuf,dataptr dz);
static int  interp_between_vals_in_two_bufs(float *thisbuf,float *nextbuf,double interpval,dataptr dz);
static int  interp_between_pitch_vals(double *thismidi,float *pitch,int thisindex,double interpval,dataptr dz);
static int  interp_between_pitch_vals2(double *thismidi,double *pitch,int thisindex,double interpval,dataptr dz);
static int  transpose_outbuf(double transp,dataptr dz);
static int  put_transposed_data_in_appropriate_chans(double transp,dataptr dz);
static int  put_thisdata_in_appropriate_chan(int cc,int vc, dataptr dz);
static int	do_specshift(int windows_in_buf,dataptr dz);
static int  do_the_specshift(dataptr dz);
static int  logread_shift(double timenow,int btktable_no,dataptr dz);
static int  read_value_from_brktable_using_log_interpolation(double thistime,int paramno,dataptr dz);
static int  reassign_frqs_to_appropriate_chans(dataptr dz);
static int  do_chan_shifting(int cc,int vc,dataptr dz);
static int  assign_new_frqs_to_appropriate_channels(dataptr dz);

/**************************** SPECSHIFT ****************************/

int specshift(dataptr dz)
{
	int exit_status;
	int samps_read, windows_in_buf;
	dz->time = 0.0f;
	if(dz->bptrcnt <= 0) {
		sprintf(errstr,"flbufptr[0] not established by outer_shift_loop()\n");
		return(PROGRAM_ERROR);
	}
	while((samps_read = fgetfbufEx(dz->bigfbuf,dz->big_fsize,dz->ifd[0],0)) > 0) {
    	dz->flbufptr[0] = dz->bigfbuf;
	 	windows_in_buf = samps_read/dz->wanted;
		if((exit_status = do_specshift(windows_in_buf,dz))<0)
			return(exit_status);
		if((exit_status = write_exact_samps(dz->bigfbuf,samps_read,dz))<0)
			return(exit_status);
	}  
	if(samps_read<0) {
		sprintf(errstr,"Sound read error.\n");
		return(SYSTEM_ERROR);
	}
	return(FINISHED);
}

/**************************** DO_SPECSHIFT ****************************/

int	do_specshift(int windows_in_buf,dataptr dz)
{
	int exit_status;
	int wc;
   	for(wc=0; wc<windows_in_buf; wc++) {
		if(dz->vflag[SHIFT_LOG]) {
			if(dz->brksize[SHIFT_SHIF]) {
				if((exit_status = logread_shift(dz->time,SHIFT_SHIF,dz))<0)
					return(exit_status);
			}
			if(dz->brksize[SHIFT_FRQ1]) {
				if((exit_status = 
					read_value_from_brktable_using_log_interpolation((double)dz->time,SHIFT_FRQ1,dz))<0)
					return(exit_status);
			}
			if(dz->brksize[SHIFT_FRQ2]) {
				if((exit_status = 
					read_value_from_brktable_using_log_interpolation((double)dz->time,SHIFT_FRQ2,dz))<0)
					return(exit_status);
			}
		} else {
			if((exit_status = read_values_from_all_existing_brktables((double)dz->time,dz))<0)
				return(exit_status);
		}
		if((exit_status = do_the_specshift(dz))<0)
			return(exit_status);
		dz->flbufptr[0] += dz->wanted;
		dz->time = (float)(dz->time + dz->frametime);
	}
	return(FINISHED);
}

/**************************** DO_THE_SPECSHIFT ***************************
 *
 * linear frequency shift of spectrum.
 */

int do_the_specshift(dataptr dz)
{
	int cc, vc;
	switch(dz->mode) {
	case(SHIFT_ALL):
		for(cc=0,vc=0;cc< dz->clength; cc++, vc+=2)
			dz->flbufptr[0][FREQ] = (float)(dz->flbufptr[0][FREQ] + dz->param[SHIFT_SHIF]);
		break;
	case(SHIFT_ABOVE):
		for(cc=0,vc=0;cc< dz->clength; cc++, vc+=2) {
			if(dz->flbufptr[0][FREQ] > dz->param[SHIFT_FRQ1])
				dz->flbufptr[0][FREQ] = (float)(dz->flbufptr[0][FREQ] + dz->param[SHIFT_SHIF]);
		}
		break;
	case(SHIFT_BELOW):
		for(cc=0,vc=0;cc< dz->clength; cc++, vc+=2) {
			if(dz->flbufptr[0][FREQ] < dz->param[SHIFT_FRQ1])
				dz->flbufptr[0][FREQ] = (float)(dz->flbufptr[0][FREQ] + dz->param[SHIFT_SHIF]);
		}
		break;
	case(SHIFT_BETWEEN):
		for(cc=0,vc=0;cc< dz->clength; cc++, vc+=2) {
			if(dz->flbufptr[0][FREQ] > dz->param[SHIFT_FRQ1] && dz->flbufptr[0][FREQ] < dz->param[SHIFT_FRQ2])
				dz->flbufptr[0][FREQ] = (float)(dz->flbufptr[0][FREQ] + dz->param[SHIFT_SHIF]);
		}
		break;
	case(SHIFT_OUTSIDE):
		for(cc=0,vc=0;cc< dz->clength; cc++, vc+=2) {
			if(dz->flbufptr[0][FREQ] < dz->param[SHIFT_FRQ1] || dz->flbufptr[0][FREQ] > dz->param[SHIFT_FRQ2])
				dz->flbufptr[0][FREQ] = (float)(dz->flbufptr[0][FREQ] + dz->param[SHIFT_SHIF]);
		}
		break;
	default:
		sprintf(errstr,"Unkonwn mode in do_specshift()\n");
		return(PROGRAM_ERROR);
	}
	return reassign_frqs_to_appropriate_chans(dz);
 }

/*************************** REASSIGN_FRQS_TO_APPROPRIATE_CHANS ***************************/

int reassign_frqs_to_appropriate_chans(dataptr dz)
{
	int exit_status;
	int cc, vc;
 	for(cc=0,vc=0;cc< dz->clength; cc++, vc+=2) {
		dz->iparray[SHIFT_OVER][cc] = 0;
		dz->iparray[SHIFT_DONE][cc] = 0;
	}
	if(dz->param[SHIFT_SHIF] > 0.0) {
		for(cc=0,vc=0;cc< dz->clength; cc++, vc+=2) {
			if(dz->flbufptr[0][FREQ] > (float)dz->parray[SHIFT_CHTOP][cc]) 
				dz->iparray[SHIFT_OVER][cc] = 1;
		}					    /* Work down from top, shifting up */
		for(cc=dz->clength-1,vc=(dz->clength-1)*2;cc>=0; cc--, vc-=2) {
			if(dz->iparray[SHIFT_OVER][cc]) {
				if((exit_status = do_chan_shifting(cc,vc,dz))<0)
					return(exit_status);
			}
		}
	}					      	/* Work up from bottom shifting down */
	if(dz->param[SHIFT_SHIF] < 0.0) {
		for(cc=1,vc=2;cc< dz->clength; cc++, vc+=2) {		
			if(dz->flbufptr[0][FREQ] <= (float)dz->parray[SHIFT_CHTOP][cc-1]) {
				if((exit_status = do_chan_shifting(cc,vc,dz))<0)
					return(exit_status);
			}
		}
	}				
	return(FINISHED);
}		

/*************************** DO_CHAN_SHIFTING ***************************/

int do_chan_shifting(int cc,int vc,dataptr dz)
{
	int k = (int)((dz->flbufptr[0][FREQ] + dz->halfchwidth)/dz->chwidth);  /* TRUNCATE */
	int newvc = k * 2;
	if(k >=0 && k < dz->clength) {
		if(!dz->iparray[SHIFT_DONE][k]
		|| (dz->iparray[SHIFT_DONE][k] && (dz->flbufptr[0][AMPP] > dz->flbufptr[0][newvc]))) {
			dz->flbufptr[0][newvc++]    = dz->flbufptr[0][AMPP];
			dz->flbufptr[0][newvc]      = dz->flbufptr[0][FREQ];
			dz->iparray[SHIFT_DONE][k] = 1;
		}
	}
	dz->flbufptr[0][AMPP] = 0.0f;
	dz->flbufptr[0][FREQ] = (float)dz->parray[SHIFT_CHMID][cc];
	return(FINISHED);
}

/*********************** SPECGLIS ***************************/

int specglis(dataptr dz)
{
	int exit_status;
	double pre_totalamp, post_totalamp;
	double scalefact;
	if(dz->brksize[GLIS_RATE])
		dz->param[GLIS_RATE] *= dz->param[GLIS_CONVERTOR];
	rectify_window(dz->flbufptr[0],dz);
	if((exit_status = extract_specenv(0,0,dz))<0)
		return(exit_status);
	if((exit_status = get_totalamp(&pre_totalamp,dz->flbufptr[0],dz->wanted))<0)
		return(exit_status);
	if(dz->mode != SELFGLIS) {
		if((exit_status = zero_sampbuf(dz))<0)
			return(exit_status);
	}
	if((exit_status = establish_frq_params(&scalefact,dz))<0)
		return(exit_status);
	if(dz->param[GLIS_RATE] <= 0.0) {
		if((exit_status = write_partials_moving_down(scalefact,dz))<0)
			return(exit_status);
	} else {
		if((exit_status = write_partials_moving_up(scalefact,dz))<0)
			return(exit_status);
	}
	if(dz->vflag[GLIS_FTOP]) {
		if((exit_status = cut_or_rolloff_high_frq(dz))<0)
			return(exit_status);
	}
	if((exit_status = get_totalamp(&post_totalamp,dz->flbufptr[0],dz->wanted))<0)
		return(exit_status);
	return normalise(pre_totalamp,post_totalamp,dz);
}


/*********************** ZERO_SAMPBUF **********************/

int zero_sampbuf(dataptr dz)
{
	int vc;
	for(vc = 0; vc < dz->wanted; vc += 2) 
		dz->flbufptr[0][AMPP] = 0.0f;
	return(FINISHED);
}

/*********************** ESTABLISH_FRQ_PARAMS **********************/

int establish_frq_params(double *scalefact,dataptr dz)
{
	double thispitch;
	dz->param[GLIS_BASEFRQ] *= pow(2.0,dz->param[GLIS_RATE]);
	if(dz->param[GLIS_RATE] >= 0.0) {		 /* GOING UP */
		while(dz->param[GLIS_BASEFRQ] > GLIS_REFERENCE_FRQ * 2.0)
			dz->param[GLIS_BASEFRQ] /= 2.0;
		thispitch  = log(dz->param[GLIS_BASEFRQ]);
		*scalefact = thispitch - dz->param[GLIS_REFPITCH];
	} else {								/* GOING DOWN */
		while(dz->param[GLIS_BASEFRQ] < GLIS_REFERENCE_FRQ/2.0)
			dz->param[GLIS_BASEFRQ] *= 2.0;
		thispitch  = log(dz->param[GLIS_BASEFRQ]);
		*scalefact = thispitch - dz->param[GLIS_HALF_REFPITCH];
	}
 	return(FINISHED);
}

/********************* WRITE_PARTIALS_MOVING_DOWN *********************
 *
 * (0)	Assume we start at fundamental.
 * (1)	Calculate partials from where we are.
 * (2)  Fade out odd partials.
 * (3)	Move up by equal frq steps, or by multiples of current fundamental.
 */

int write_partials_moving_down(double scalefact,dataptr dz)
{
	int exit_status;
	int n, cc, vc;									/* 0 */
	double fundamental, thisfrq, transpos;
	switch(dz->mode) {
	case(SELFGLIS):
		rectify_window(dz->flbufptr[0],dz);
		transpos = dz->param[GLIS_BASEFRQ]/GLIS_REFERENCE_FRQ;
		for(cc=0,vc=0;cc < dz->clength;cc++,vc+=2) {
			dz->amp[cc]  = 0.0f;
			dz->freq[cc] = dz->flbufptr[0][FREQ];
		}
		for(vc=0;vc < dz->wanted; vc+=2) {
			if((exit_status = selfglis_down_within_spectral_envelope(vc,transpos,dz))<0)
				return(exit_status);
		}
		if((exit_status = put_amp_and_frq(dz->flbufptr[0],dz))<0)
			return(exit_status);
		break;
	case(INHARMONIC):
	case(SHEPARD):
		fundamental = dz->param[GLIS_BASEFRQ];				/* 1 */
		thisfrq 	= dz->param[GLIS_BASEFRQ];
		n = 1;
		while(thisfrq < dz->nyquist) {
			if((exit_status = glis_down_within_spectral_envelope(fundamental,&thisfrq,scalefact,&n,dz))<0)
				return(exit_status);
		}
		break;
	default:
		sprintf(errstr,"Unknown mode in write_partials_moving_down()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/************************ WRITE_PARTIALS_MOVING_UP ***********************
 *
 * (0)	Assume we start at 2nd harmonic.
 * (1)	Calculate partials from a fundamental an 8va below ref pitch.
 * (2)  Fade in lowest component, and odd partials.
 * (3)	Move up by equal frq steps, or by multiples of current fundamental.
 */

int write_partials_moving_up(double scalefact,dataptr dz)
{
	int exit_status;
	int n = 2, cc, vc;						/* 0 */
	double fundamental = dz->param[GLIS_BASEFRQ]/2.0;			/* 1 */
	double thisfrq 	   = dz->param[GLIS_BASEFRQ];
	double transpos;
	switch(dz->mode) {
	case(SELFGLIS) :
		transpos = dz->param[GLIS_BASEFRQ]/ GLIS_REFERENCE_FRQ;
		for(cc=0,vc=0;cc < dz->clength;cc++,vc+=2) {
			dz->amp[cc]  = 0.0f;
			dz->freq[cc] = dz->flbufptr[0][FREQ];
		}
		for(vc=0;vc < dz->wanted;vc+=2) {
			if((exit_status = selfglis_up_within_spectral_envelope(vc,transpos,dz))<0)
				return(exit_status);		
		}
		if((exit_status = put_amp_and_frq(dz->flbufptr[0],dz))<0)
			return(exit_status);		
		break;
    case(SHEPARD):
    case(INHARMONIC):
		while(thisfrq < dz->nyquist) {
			if((exit_status = glis_up_within_spectral_envelope(fundamental,&thisfrq,scalefact,&n,dz))<0)
				return(exit_status);
		}
		break;
	default:
		sprintf(errstr,"Unknown mode in write_partials_moving_up()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/****************************** CUT_OR_ROLLOFF_HIGH_FRQ *****************************/

int cut_or_rolloff_high_frq(dataptr dz)
{
	int vc;
	for(vc = 0; vc < dz->wanted;vc += 2) {
		if(dz->flbufptr[0][FREQ] > dz->param[GLIS_HIFRQ]) {
			if(dz->flbufptr[0][FREQ] >= dz->param[GLIS_FRQTOP_TOP])
				dz->flbufptr[0][AMPP] = 0.0f;
			else
				dz->flbufptr[0][AMPP] =
			(float)(dz->flbufptr[0][AMPP] * (dz->param[GLIS_FRQTOP_TOP] - dz->flbufptr[0][FREQ])/dz->param[GLIS_ROLL_OFF]);
		}
	}
	return(FINISHED);
}

/******************* SELFGLIS_DOWN_WITHIN_SPECTRAL_ENVELOPE ********************/

int selfglis_down_within_spectral_envelope(int vc,double transpos,dataptr dz)
{
	int exit_status;
	double ampratio, amphere, thisfrq, thisamp, frqhi, amphi;
	int n;
	if((exit_status = getspecenvamp(&amphere,dz->flbufptr[0][FREQ],0,dz))<0)
		return(exit_status);
	if(amphere <= VERY_SMALL)
		ampratio = 0.0;
	else {
		if((ampratio = fabs(dz->flbufptr[0][AMPP]/amphere)) > RATIO_LIMIT)
			ampratio = RATIO_LIMIT;
	}
	if((thisfrq  = fabs(dz->flbufptr[0][FREQ]) * transpos) < dz->nyquist) {
		if((exit_status = getspecenvamp(&thisamp,thisfrq,0,dz))<0)
			return(exit_status);
		thisamp *= ampratio;
		thisamp *= pow(max(0.0,(transpos * 2.0) - 1.0),SMOOTHER);
		if((exit_status = get_channel_corresponding_to_frq(&n,thisfrq,dz))<0)
			return(exit_status);
		if(n >=0 && n < dz->clength && thisamp > dz->amp[n]) {
			dz->amp[n++] = (float)thisamp;
			dz->freq[n]  = (float)thisfrq;
		}
	}	
	if((frqhi = thisfrq * 2.0) < dz->nyquist) {
		if((exit_status = getspecenvamp(&amphi,frqhi,0,dz))<0)
			return(exit_status);
		amphi   *= ampratio;
		amphi   *= pow(max(0.0,2.0 - (transpos * 2.0)),SMOOTHER);
		if((exit_status = get_channel_corresponding_to_frq(&n,thisfrq,dz))<0)
			return(exit_status);
		if(n >=0 && n < dz->clength && amphi > dz->amp[n]) {
			dz->amp[n++] = (float)amphi;
			dz->freq[n]  = (float)frqhi;
		}
	}
	return(FINISHED);
}

/******************* GLIS_DOWN_WITHIN_SPECTRAL_ENVELOPE ********************/

int glis_down_within_spectral_envelope(double fundamental,double *thisfrq,double scalefact,int *n,dataptr dz)
{
	int exit_status;
	int cc, vc;
	double thisamp;
	if((exit_status = get_channel_corresponding_to_frq(&cc,*thisfrq,dz))<0)
		return(exit_status);
	vc = cc * 2;
	dz->flbufptr[0][FREQ] = (float)(*thisfrq);
	if((exit_status = getspecenvamp(&thisamp,*thisfrq,0,dz))<0)
		return(exit_status);
	dz->flbufptr[0][AMPP] = (float)thisamp;
	if(ODD(*n))
		dz->flbufptr[0][AMPP] = (float)(dz->flbufptr[0][AMPP] * scalefact);	/* 2 */
	(*n)++;
	switch(dz->mode) {
	case(INHARMONIC): *thisfrq += dz->param[GLIS_SHIFT];		break;
	case(SHEPARD):    *thisfrq  = fundamental * (double)(*n);	break;	
	}							/* 3 */
	return(FINISHED);
}

/******************* SELFGLIS_UP_WITHIN_SPECTRAL_ENVELOPE ********************/

int selfglis_up_within_spectral_envelope(int vc,double transpos,dataptr dz)
{
	int exit_status;
	double amphere, ampratio;
	double thisfrq, thisamp, frqlo, amplo;
	int n;
	if((exit_status = getspecenvamp(&amphere,dz->flbufptr[0][FREQ],0,dz))<0)
		return(exit_status);
	if(amphere <= VERY_SMALL)
		ampratio = 0.0;
	else {
		if((ampratio = fabs(dz->flbufptr[0][AMPP]/amphere)) > RATIO_LIMIT)
			ampratio = RATIO_LIMIT;
	}
	if((thisfrq  = fabs(dz->flbufptr[0][FREQ]) * transpos) < dz->nyquist) {
		if((exit_status = getspecenvamp(&thisamp,thisfrq,0,dz))<0)
			return(exit_status);
		thisamp *= ampratio;
		thisamp *= pow(2.0 - transpos,SMOOTHER);
		if((exit_status = get_channel_corresponding_to_frq(&n,thisfrq,dz))<0)
			return(exit_status);
		if(n >=0 && n < dz->clength && thisamp > dz->amp[n]) {
			dz->amp[n]  = (float)thisamp;
			dz->freq[n] = (float)thisfrq;
		}
	}
	if((frqlo = thisfrq/2.0) < dz->nyquist) {
		if((exit_status = getspecenvamp(&amplo,frqlo,0,dz))<0)
			return(exit_status);
		amplo   *= ampratio;
		amplo   *= pow(transpos - 1.0,SMOOTHER);
		exit_status = get_channel_corresponding_to_frq(&n,thisfrq,dz);
		if(n < dz->clength && amplo > dz->amp[n]) {
			dz->amp[n]  = (float)amplo;
			dz->freq[n] = (float)frqlo;
		}
	}
	return(FINISHED);
}

/******************* GLIS_UP_WITHIN_SPECTRAL_ENVELOPE ********************/

int glis_up_within_spectral_envelope(double fundamental,double *thisfrq,double scalefact,int *n,dataptr dz)
{
	int exit_status;
	int cc, vc;
	double thisamp;
	if((exit_status = get_channel_corresponding_to_frq(&cc,*thisfrq,dz))<0)
		return(exit_status);
	vc = cc * 2;
	dz->flbufptr[0][FREQ] = (float)(*thisfrq);
	if((exit_status = getspecenvamp(&thisamp,*thisfrq,0,dz))<0)
		return(exit_status);
	dz->flbufptr[0][AMPP] = (float)thisamp;
	if(ODD(*n) || *n==2)
		dz->flbufptr[0][AMPP] = (float)(dz->flbufptr[0][AMPP] * scalefact);	/* 2 */
	(*n)++;
	switch(dz->mode) {
	case(INHARMONIC):*thisfrq += dz->param[GLIS_HALF_SHIFT];	break;
	case(SHEPARD):   *thisfrq  = fundamental * (double)(*n); 	break;
	}						/* 3 */
	return(FINISHED);
}

/*************************** SPECINVERT *********************/

int specinvert(dataptr dz)
{
	int exit_status;
	double zort, pre_totalamp;
	int cc, vc;
	switch(dz->mode) {
	case(INV_NORMAL):
		for(cc = 0, vc = 0; cc < dz->clength; cc++, vc += 2)
			dz->flbufptr[0][AMPP] = dz->amp[cc] - dz->flbufptr[0][AMPP];
		break;
	case(INV_KEEPAMP):
		if((exit_status = get_totalamp(&pre_totalamp,dz->flbufptr[0],dz->wanted))<0)
			return(exit_status);
		if(pre_totalamp > MINAMP) {
			for(cc = 0, vc = 0; cc < dz->clength; cc++, vc += 2) {
				zort = dz->parray[INV_AMPRATIO][cc] - (dz->flbufptr[0][vc]/pre_totalamp);
				dz->flbufptr[0][vc] = (float)(zort * pre_totalamp);
			}
		}
		break;
	}
	return(FINISHED);
}

/******************************* SPECWAVER ********************************/

int specwaver(dataptr dz)
{
	int exit_status = FINISHED;
	double duration = dz->wlength * dz->frametime;
	int   wcnt, upwcnt, dnwcnt, wc, samps_read, w_to_buf;
	if(duration <= 0.0) {	
		sprintf(errstr,"Zero length analysis file: cannot proceed.\n");
		return(DATA_ERROR);
	}
	if((samps_read = fgetfbufEx(dz->bigfbuf, dz->big_fsize,dz->ifd[0],0)) > 0) {
		dz->flbufptr[0] = dz->bigfbuf;
		w_to_buf       = samps_read/dz->wanted;    
		wc             = 0;
	}
	else {
		if(samps_read<0) {
			sprintf(errstr,"Sound read error.\n");
			return(SYSTEM_ERROR);
		}
		sprintf(errstr,"No windows found. specwaver()\n");
		return(PROGRAM_ERROR);
    }
	while(dz->time < duration) {
		if((exit_status = read_values_from_all_existing_brktables((double)dz->time,dz))<0)
			return(exit_status);
		wcnt = round(1.0/(dz->param[WAVER_VIB] * dz->frametime));
		dnwcnt = upwcnt = wcnt/2;
		if(ODD(wcnt))
			upwcnt++;
		if((exit_status = do_waver(&wc,&w_to_buf,&samps_read,wcnt,upwcnt,dnwcnt,dz))!=CONTINUE)
			break;
	}
	return(exit_status);
}

/******************************** DO_WAVER *******************************/

int do_waver(int *wc,int *w_to_buf,int *samps_read,int wcnt,int upwcnt,int dnwcnt,dataptr dz)
{
	int exit_status;
	int n;
	double strstep = dz->param[WAVER_STR]/(double)upwcnt;
	double thisstr = dz->param[WAVER_STR];
	for(n=0;n<upwcnt;n++) {
		if((exit_status = spectrally_stretch_window(thisstr,dz))<0)
			return(exit_status);
		if((exit_status = advance_along_windows(wc,w_to_buf,samps_read,dz))!=CONTINUE)
			return(exit_status);
		thisstr += strstep;
	}
	if(ODD(wcnt))
		thisstr -= strstep;
	for(n=0;n<dnwcnt;n++) {
		if((exit_status = spectrally_stretch_window(thisstr,dz))<0)
			return(exit_status);
		if((exit_status = advance_along_windows(wc,w_to_buf,samps_read,dz))!=CONTINUE)
			return(exit_status);
		thisstr -= strstep;
	}
	return(CONTINUE);
}
    
/*********************** SPECTRALLY_STRETCH_WINDOW *****************************/

int spectrally_stretch_window(double thisstr,dataptr dz)
{
	int dd;
	int cc, vc;
	double herestr, thislocation, strdiff = thisstr - 1.0;
	for(dd=0,cc=dz->iparam[WAVER_BOTCHAN],vc=dz->iparam[WAVER_BOTCHAN]*2; cc < dz->clength; dd++,cc++,vc+=2) {
		thislocation = (double)dd/(double)dz->iparam[WAVER_STRCHANS];
		if(dz->mode==WAVER_SPECIFIED)
			thislocation = pow(thislocation,dz->param[WAVER_EXP]);
		herestr  = (thislocation * strdiff) + 1.0;
		dz->amp[cc]  = dz->flbufptr[0][AMPP];
		dz->freq[cc] = (float)fabs(dz->flbufptr[0][FREQ] * herestr);
	}
	return assign_new_frqs_to_appropriate_channels(dz);
}

/***************************** ADVANCE_ALONG_WINDOWS *************************/

int advance_along_windows(int *wc,int *w_to_buf,int *samps_read,dataptr dz)
{
	int exit_status;
	dz->flbufptr[0] += dz->wanted;
	if(++(*wc) >= *w_to_buf) {
		if(*samps_read > 0) {
			if((exit_status = write_exact_samps(dz->bigfbuf,*samps_read,dz))<0)
				return(exit_status);
		}
		if((*samps_read = fgetfbufEx(dz->bigfbuf, dz->big_fsize,dz->ifd[0],0)) > 0) {
			dz->flbufptr[0] = dz->bigfbuf;
			*w_to_buf      = *samps_read/dz->wanted;    
			*wc            = 0;
		} else {
//TW CORRECTED indirection error : Dec 2002
			if(*samps_read<0) {
				sprintf(errstr,"Sound read error.\n");
				return(SYSTEM_ERROR);
			}
			return(FINISHED);
		}
	}  
	dz->time = (float)(dz->time + dz->frametime);
	return(CONTINUE);
}

/****************************** SPECWARP ***************************/

int	specwarp(dataptr dz)
{
	int exit_status;
	int avcnt;
	if(sloom) {
		fprintf(stdout,"INFO: Warping pitch.\n");
		fflush(stdout);
	}
	if((exit_status = warp_pitch(&avcnt,dz))<0)
		return(exit_status);
	if(sloom) {
		fprintf(stdout,"INFO: Warping time.\n");
		fflush(stdout);
	}
	return warp_time(avcnt,dz);
}

/************************** WARP_PITCH ************************/

int warp_pitch(int *avcnt,dataptr dz)
{
	int exit_status;
	int   n = 0, lastpos = 0, thispos, diff, q, start;
	double lastmidi;
	double ttime, thismidi;
	initrand48();
	if((exit_status = get_pitch_averages(avcnt,dz))<0)
		return(exit_status);
	if((exit_status = get_statechanges(*avcnt,WARP_SRNG,WARP_AVP,WARP_CHANGE,WARP_MININT,-WARP_MININT,IS_PITCH,dz))<0)
		return(exit_status);
	while(dz->parray[WARP_AVP][n] < MIDIMIN) {
	    if(++n >= *avcnt) {
			sprintf(errstr,"No valid pitch-average data found: warp_pitch()\n");
			return(PROGRAM_ERROR);
		}
	}
	dz->iparray[WARP_CHANGE][n] = 1;
	start = n;
	for(;n < *avcnt;n++) {
		if(dz->iparray[WARP_CHANGE][n])	{
			if((exit_status = setup_and_do_pitchwarp(start,n,&lastpos,&lastmidi,dz))<0)
				return(exit_status);
		}
	}
	thispos = (dz->wlength - 1);
	if((diff = thispos - lastpos) > 0) {
		q = (*avcnt) - 1;
		while(dz->parray[WARP_AVP][q]<MIDIMIN) {
			if(--q<=lastpos)
				return(FINISHED);
		}	
		thismidi = dz->parray[WARP_AVP][q];
		ttime = (q * BLOKCNT) * dz->frametime;
		if((exit_status = do_pitch_wiggle(&thismidi,ttime,dz))<0)
			return(exit_status);
		if((exit_status = warp_pitches(thispos,diff,&lastpos,thismidi,&lastmidi,dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}

/************************** WARP_TIME ************************/

int warp_time(int avcnt,dataptr dz)
{
	int exit_status = CONTINUE;
	int   lastoutpos = 0, lastinpos  = 0,  indiff, remain;
	int   n, ip, samps_read;
	int last_total_wndws = 0;
	float *nextbuf = dz->flbufptr[0] + dz->wanted, *thisbuf = dz->flbufptr[0];
	if(sloom)
		dz->total_samps_read = 0;
	if((samps_read = fgetfbufEx(dz->bigfbuf,dz->big_fsize,dz->ifd[0],0)) <= 0) {
		if(samps_read<0) {
			sprintf(errstr,"Sound read error.\n");
			return(SYSTEM_ERROR);
		} else {
			sprintf(errstr,"Cannot read data from input analysis file.\n");
			return(DATA_ERROR);
		}
	}
	if(sloom)
		dz->total_samps_read += samps_read;
	dz->iparam[WARP_WREAD] = samps_read/dz->wanted;
	if((dz->flbufptr[6] = dz->bigfbuf + samps_read)!=dz->flbufptr[2]) {
		dz->iparam[WARP_PART_INBUF] = 1;
		dz->iparam[WARP_ATEND] = 1;
	}
	for(n=0;n<avcnt;n++) {
		if(dz->iparray[WARP_CHANGE][n]) {
			if((exit_status = setup_and_do_timewarp(&thisbuf,&nextbuf,n,&lastoutpos,&lastinpos,&last_total_wndws,dz))<0)
				return(exit_status);
		}
		if(exit_status!=CONTINUE)
			break;
	}
	if((indiff = (dz->wlength - 1) - lastinpos) > 0) {
		ip = lastinpos + 1;
		while(lastinpos < dz->wlength) {
			if((exit_status = adjust_input_buffers(&thisbuf,&nextbuf,ip,&last_total_wndws,0,dz))<0)
				return(exit_status);
			if(dz->flbufptr[1]!=thisbuf)
				memmove((char *)dz->flbufptr[1],(char *)thisbuf,(size_t)(dz->wanted * sizeof(float)));
			if((dz->flbufptr[1] += dz->wanted) >= dz->flbufptr[3]) {
				if((exit_status = write_exact_samps(dz->flbufptr[2],dz->big_fsize,dz))<0)
					return(exit_status);
				dz->flbufptr[1] = dz->flbufptr[2];
			}
 			lastinpos++;	/* MAY 1997 ??? */
			ip = lastinpos;	/* MAY 1997 ??? */
		}
	}
	if((remain = (dz->flbufptr[1]  - dz->flbufptr[2]))>0) {
		if((exit_status = write_samps(dz->flbufptr[2],remain,dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}

/************************** GET_PITCH_AVERAGES ************************/

int get_pitch_averages(int *avcnt,dataptr dz)
{
	int k = 0, OK = 1, n = 0, m, cnt;
	while(OK) {
		dz->parray[WARP_AVP][k] = 0.0;
		m = 0;
		cnt = 0;
		while(m < BLOKCNT) {
			if(dz->pitches[n]<MIDIMIN) {
				m++;
				if(++n >= dz->wlength) {
					OK = 0;
					break;
				}
				continue;
			}
			dz->parray[WARP_AVP][k] += dz->pitches[n];
			cnt++;
			m++;
			if(++n >= dz->wlength) {
				OK = 0;
				break;
			}
		}
		if(cnt==0)
			dz->parray[WARP_AVP][k] = -1.0;
		else
		    dz->parray[WARP_AVP][k] /= (double)cnt;
		k++;
	}
	if((dz->parray[WARP_AVP] = (double *)realloc((char *)dz->parray[WARP_AVP],k * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for warp averages array.\n");
		return(MEMORY_ERROR);
	}
	*avcnt = k;
	return(FINISHED);
}

/************************** SETUP_AND_DO_PITCHWARP ************************/

int setup_and_do_pitchwarp(int start,int avg_no,int *lastpos,double *lastmidi,dataptr dz)
{
	int exit_status;
	int here, q, z, diff;
	double thismidi, ttime;
	here = min((dz->wlength - 1),avg_no * BLOKCNT); /* just being ultra careful !! */
	for(q = here; q < here+BLOKCNT; q++) {
		if(q>=dz->wlength)
			break;
		if(dz->pitches[q] >= MIDIMIN)
			break;
	}
	if(q >= here+BLOKCNT || q>=dz->wlength) {	   /* No valid pitchdata here !! */
		for(z=here; z<q; z++)
			dz->parray[WARP_P2][z] = -1.0;
		dz->iparray[WARP_CHANGE][avg_no] = 0;
		return(FINISHED);
	} else		/* [WARP_CHANGE] remembers position of approp wndw to work on */
		dz->iparray[WARP_CHANGE][avg_no] = q;	
	here     = q;
	ttime    = dz->frametime * (double)here;
	thismidi = dz->pitches[here];
	if((exit_status = do_pitch_wiggle(&thismidi,ttime,dz))<0)
			return(exit_status);
	if(avg_no==start) {
		dz->parray[WARP_P2][q] = thismidi;
		*lastpos  = q;
		*lastmidi = thismidi;
		return(FINISHED);
	}
	diff  = q - *lastpos;
	return warp_pitches(q,diff,lastpos,thismidi,lastmidi,dz);
}

/***************** DO_PITCH_WIGGLE *******************/

int do_pitch_wiggle(double *thismidi,double ttime, dataptr dz)
{
	int exit_status;
	double wiggle, this_prange;
	if(dz->brksize[WARP_PRNG]) {
		if((exit_status  = read_value_from_brktable(ttime,WARP_PRNG,dz))<0)
			return(exit_status);
	}
	wiggle = (drand48() * 2.0) -1.0;	/* random  +-  */
	this_prange = wiggle * dz->param[WARP_PRNG];
	*thismidi += this_prange;
	return(FINISHED);
}

/***************** WARP_PITCHES *******************/

int warp_pitches(int thispos,int diff,int *lastpos,double thismidi,double *lastmidi,dataptr dz)
{
	int   n, m;
	double mididiff = thismidi - *lastmidi;
	double midistep = mididiff/(double)diff;
	for(m=1, n=(*lastpos)+1; m <= diff; m++, n++) {
		if(n < dz->wlength && dz->pitches[n]<MIDIMIN) {
			dz->parray[WARP_P2][n] = -1.0;
			continue;
		}
		thismidi = *lastmidi + (midistep * (double)m);
		dz->parray[WARP_P2][n] = thismidi;
	}
	*lastmidi = thismidi;
	*lastpos  = thispos;
	return(FINISHED);
}

/************************** SETUP_AND_DO_TIMEWARP ************************/

int setup_and_do_timewarp
(float **thisbuf,float **nextbuf,int n,int *lastoutpos,int *lastinpos,int *last_total_wndws,dataptr dz)
{
	int exit_status;
	int thisoutpos, outdiff;
	int thisinpos  = dz->iparray[WARP_CHANGE][n];
	double ttime = dz->frametime * (double)thisinpos;
	if(n==0) {
		if((exit_status = transpos_initial_windows(thisbuf,nextbuf,thisinpos,last_total_wndws,dz))!=CONTINUE)
			return(exit_status);
		*lastinpos   = thisinpos;			 
		*lastoutpos  = thisinpos;
	} else {
		if((exit_status = do_time_wiggle(&ttime,dz))<0)
			return(exit_status);
		thisoutpos = round(ttime/dz->frametime);
		thisoutpos = max(0L,thisoutpos);
		if((outdiff = thisoutpos - *lastoutpos)>0) {/* number of windows to scan in output data */
			if((exit_status = do_timewarp(thisbuf,nextbuf,thisinpos,outdiff,lastinpos,last_total_wndws,dz))!=CONTINUE)
				return(exit_status);
		}
		*lastinpos  = thisinpos; 	/* ??? */
		*lastoutpos = thisoutpos;	/* ??? */
	}
	return(CONTINUE);
}

/***************** ADJUST_INPUT_BUFFERS *******************/

int adjust_input_buffers(float **thisbuf,float **nextbuf,int thisindex,int *last_total_wndws,int do_next,dataptr dz)
{
	int exit_status;
	int currentbuf_index, currentbuf_nextindex;
	currentbuf_index = thisindex - *last_total_wndws;

	if(currentbuf_index >= dz->iparam[WARP_WREAD]) {
		if((exit_status = find_currentbuf_index(&currentbuf_index,last_total_wndws,thisbuf,dz))!=CONTINUE)
			return(exit_status);
	}
	if(do_next) {
		currentbuf_nextindex = currentbuf_index + 1;
		if(currentbuf_nextindex >= dz->iparam[WARP_WREAD]) {
			if(dz->flbufptr[3]!=*thisbuf)
				memmove((char *)dz->flbufptr[3],(char *)(*thisbuf),(size_t)(dz->wanted * sizeof(float)));
			*thisbuf   = dz->flbufptr[3];
			if((exit_status = find_currentbuf_nextindex(&currentbuf_nextindex,*last_total_wndws,dz))<0)
				return(exit_status);
			*nextbuf  = dz->bigfbuf + (currentbuf_nextindex * dz->wanted);
		}
	}
	return(FINISHED);
}

/***************** TRANSPOS_INITIAL_WINDOWS   ******************/

int transpos_initial_windows(float **thisbuf,float **nextbuf,int cnt,int *last_total_wndws,dataptr dz)
{
	int exit_status;
	int cc, vc;
	int n = 0, samps_to_write, samps_read;
	double transp;
	while(n <= cnt) {
		rectify_window(dz->flbufptr[0],dz);
		if(dz->pitches[n] > MIDIMIN) {
			transp = SEMITONES_AS_RATIO(dz->parray[WARP_P2][n] - dz->pitches[n]);
			for(cc=0,vc=0;cc<dz->clength;cc++,vc+=2) {
				dz->flbufptr[1][AMPP] = dz->flbufptr[0][AMPP];	
				dz->flbufptr[1][FREQ] = (float)(dz->flbufptr[0][FREQ] * transp);
			}
			if((exit_status = put_transposed_data_in_appropriate_chans(transp,dz))<0)
				return(exit_status);
		} else {
			for(vc=0;vc<dz->wanted;vc+=2)
				dz->flbufptr[1][vc] = dz->flbufptr[0][vc];	  /* Copy unpitched windows */
		}
		dz->flbufptr[1] += dz->wanted;
		if((dz->flbufptr[0] += dz->wanted) >= dz->flbufptr[6]) {
 			if(dz->iparam[WARP_PART_INBUF]) {
				if((samps_to_write = dz->flbufptr[1] - dz->flbufptr[2])>0)
					return write_samps(dz->flbufptr[2],samps_to_write,dz);
				return FINISHED;
			} else {
				if((exit_status = write_exact_samps(dz->flbufptr[2],dz->big_fsize,dz))<0)
					return(exit_status);
			} 
			if((samps_read = fgetfbufEx(dz->bigfbuf,dz->big_fsize,dz->ifd[0],0)) <= 0) {
				if(samps_read<0) {
					sprintf(errstr,"Sound read error.\n");
					return(SYSTEM_ERROR);
				}
    			return(FINISHED);
			}
			if(sloom)
				dz->total_samps_read += samps_read;
			*last_total_wndws += dz->iparam[WARP_WREAD];
			dz->iparam[WARP_WREAD] = samps_read/dz->wanted;
			if(samps_read < dz->big_fsize) {
				dz->iparam[WARP_PART_INBUF] = 1;
				dz->flbufptr[6] = dz->bigfbuf + samps_read;
			}
			dz->flbufptr[1] = dz->flbufptr[2];
			dz->flbufptr[0] = dz->bigfbuf;
		}
		n++;
	}
	if((exit_status = setup_thisbuf_and_nextbuf(*last_total_wndws,thisbuf,nextbuf,dz))<0)
		return(exit_status);
	return(CONTINUE);
}

/************************** DO_TIME_WIGGLE ************************/

int	do_time_wiggle(double *ttime,dataptr dz)
{	
	int exit_status;
	double this_trange;
	double wiggle = (drand48() * 2.0) -1.0;
	if(dz->brksize[WARP_TRNG])	 {		/* RWD need curly */
		if((exit_status = read_value_from_brktable(*ttime,WARP_TRNG,dz))<0)
			return(exit_status);
	}
	this_trange = dz->param[WARP_TRNG] * wiggle;
	*ttime  += this_trange;
	return(FINISHED);
}

/************************** DO_TIMEWARP ************************/

int do_timewarp
(float **thisbuf,float **nextbuf,int thisinpos,int outdiff,int *lastinpos,int *last_total_wndws,dataptr dz)
{
	int exit_status;
	int   indiff = thisinpos - *lastinpos;						/* number of windows to scan in  input data */
	double step = (double)indiff/(double)outdiff;	       		/* fractional step in input data */
	int   move;
	int   n, ip, lastip = *lastinpos;
	double thisfrac, thisindex = (double)(*lastinpos) + step;  	/* fractional position in input data */
	double oldmidi, newmidi, transp; 

	for(n=1; n <= outdiff; n++, thisindex+=step) {
		ip = (int)thisindex; /* TRUNCATE */
		thisfrac = thisindex - (double)ip;
		move     = ip - lastip;
		lastip   = ip;
		if(move) {
			if((exit_status = adjust_input_buffers(thisbuf,nextbuf,ip,last_total_wndws,1,dz))!=CONTINUE)
				return(exit_status);
		}
		if((exit_status = interp_between_vals_in_two_bufs(*thisbuf,*nextbuf,thisfrac,dz))<0)
			return(exit_status);
		if((exit_status = interp_between_pitch_vals(&oldmidi,dz->pitches,ip,thisfrac,dz))<0)
			return(exit_status);
		if((exit_status = interp_between_pitch_vals2(&newmidi,dz->parray[WARP_P2],ip,thisfrac,dz))<0)
			return(exit_status);
		transp  = SEMITONES_AS_RATIO(newmidi - oldmidi);
		if((exit_status = transpose_outbuf(transp,dz))<0)
			return(exit_status);
		if((dz->flbufptr[1] += dz->wanted) >= dz->flbufptr[3]) {
			if((exit_status = write_exact_samps(dz->flbufptr[2],dz->big_fsize,dz))<0)
				return(exit_status);
			dz->flbufptr[1] = dz->flbufptr[2];
		}
	}
	return(CONTINUE);
}

/***************** FIND_CURRENTBUF_INDEX *******************/	

int find_currentbuf_index(int *currentbuf_index,int *last_total_wndws,float **thisbuf,dataptr dz)
{
	int samps_read = 0;
	int finalbuf_index;
	if(dz->iparam[WARP_ATEND]) {
		*currentbuf_index = max((dz->wlength - 2) - (*last_total_wndws),0);
		return(CONTINUE);
	}
	while(*currentbuf_index >= dz->iparam[WARP_WREAD]) {
		if((samps_read = fgetfbufEx(dz->bigfbuf,dz->big_fsize,dz->ifd[0],0)) <= 0) {
			if(samps_read<0) {
				sprintf(errstr,"Sound read error.\n");
				return(SYSTEM_ERROR);
			}
			return(FINISHED);
		}
		if(sloom)
			dz->total_samps_read += samps_read;
		if(samps_read <= dz->big_fsize)
			dz->iparam[WARP_ATEND] = 1;
		*currentbuf_index -= dz->iparam[WARP_WREAD];
		*last_total_wndws += dz->iparam[WARP_WREAD];
		if(dz->iparam[WARP_ATEND]) {
			finalbuf_index = max((dz->wlength - 2) - (*last_total_wndws),0);
			*currentbuf_index = min(*currentbuf_index,finalbuf_index);
		}
	}
	dz->iparam[WARP_WREAD] = samps_read/dz->wanted;
	if(dz->iparam[WARP_ATEND])
		dz->flbufptr[6] = dz->bigfbuf + samps_read;

	*thisbuf = dz->bigfbuf + (*currentbuf_index * dz->wanted);
	return(CONTINUE);
}

/***************** FIND_CURRENTBUF_NEXTINDEX *******************/

int find_currentbuf_nextindex(int *currentbuf_nextindex,int last_total_wndws,dataptr dz)
{
	int samps_read;
	int lastindex = (dz->wlength - 1) - last_total_wndws;
	*currentbuf_nextindex = lastindex;
	if(dz->iparam[WARP_ATEND])
		return(FINISHED);
	if((samps_read = fgetfbufEx(dz->bigfbuf, dz->big_fsize,dz->ifd[0],0)) <= 0) {
		if(samps_read<0) {
			sprintf(errstr,"Sound read error.\n");
			return(SYSTEM_ERROR);
		}
		dz->iparam[WARP_ATEND] = 1;
		return(FINISHED);
	}
	if(sloom)
		dz->total_samps_read += samps_read;
	dz->iparam[WARP_WREAD] = samps_read/dz->wanted;
	if(samps_read < dz->big_fsize) {
		dz->iparam[WARP_ATEND] = 1;
		dz->flbufptr[6] = dz->bigfbuf + samps_read;
	}
	*currentbuf_nextindex = 0L;
	return(FINISHED);
}

/***************** SETUP_THISBUF_AND_NEXTBUF *******************/

int setup_thisbuf_and_nextbuf(int last_total_wndws,float **thisbuf,float **nextbuf,dataptr dz)
{
	int  exit_status;
	int currentbuf_nextindex;
	if(dz->flbufptr[0] + dz->wanted >= dz->flbufptr[2]) {
		memmove((char *)dz->flbufptr[3],(char *)dz->flbufptr[0],(size_t)dz->wanted * sizeof(float));
		*thisbuf   = dz->flbufptr[3];
		if((exit_status = find_currentbuf_nextindex(&currentbuf_nextindex,last_total_wndws,dz))<0)
			return(exit_status);
		*nextbuf   = dz->bigfbuf + (currentbuf_nextindex * dz->wanted);
	}
	return(FINISHED);
}

/***************** INTERP_BETWEEN_VALS_IN_TWO_BUFS *******************/

int interp_between_vals_in_two_bufs(float *thisbuf,float *nextbuf,double interpval,dataptr dz)
{
	int vc;
	double thismidi, nextmidi, mididiff, ampdiff;
	rectify_window(thisbuf,dz);
	if(interpval<FLTERR) {
		rectify_window(thisbuf,dz);
		if(dz->flbufptr[1]!=thisbuf)
			memmove((char *)dz->flbufptr[1],(char *)thisbuf,(size_t)(dz->wanted * sizeof(float)));
		return(FINISHED);
	}
	if(interpval > (1.0 - FLTERR)) {
		rectify_window(nextbuf,dz);
		if(dz->flbufptr[1]!=nextbuf)
			memmove((char *)dz->flbufptr[1],(char *)nextbuf,(size_t)(dz->wanted * sizeof(float)));
		return(FINISHED);
	}
	rectify_window(thisbuf,dz);
	rectify_window(nextbuf,dz);
	for(vc = 0; vc<dz->wanted; vc+=2) {
		thismidi = log(max(SPEC_MINFRQ,(double)thisbuf[FREQ]));	/* quick calc using logs, without hztimidi() */
		nextmidi = log(max(SPEC_MINFRQ,(double)nextbuf[FREQ]));	/* works because logs to different bases are simply */
		mididiff = nextmidi - thismidi;	 						/* scaled versions of one another. See Feynmann book. */
		ampdiff  = nextbuf[AMPP] - thisbuf[AMPP];
		dz->flbufptr[1][FREQ] = (float)exp(thismidi + (mididiff * interpval));	/* exp() = antilog of log() */
		dz->flbufptr[1][AMPP] = (float)(thisbuf[AMPP] + (ampdiff * interpval));
	}
	return(FINISHED);
}

/***************** INTERP_BETWEEN_PITCH_VALS *******************/

int interp_between_pitch_vals(double *thismidi,float *pitch,int thisindex,double interpval,dataptr dz)
{
	double mididiff;
	if(thisindex<0) {
		sprintf(errstr,"Pitch_interpolation val < 0 in interp_between_pitch_vals()\n");
		return(PROGRAM_ERROR);
	}
	if(thisindex >= (dz->wlength - 2)) {
		*thismidi = (double)pitch[(dz->wlength - 1)];
		return(FINISHED);
	}
	if(interpval < FLTERR) {
		*thismidi = (double)pitch[thisindex];
		return(FINISHED);
	}
	if(interpval > (1.0 - FLTERR)) {
		*thismidi = (double)pitch[thisindex+1];
		return(FINISHED);
	}
	mididiff  = pitch[thisindex+1] - pitch[thisindex];
	*thismidi = pitch[thisindex] + (mididiff * interpval);
	return(FINISHED);
}

/***************** INTERP_BETWEEN_PITCH_VALS2 *******************/

int interp_between_pitch_vals2(double *thismidi,double *pitch,int thisindex,double interpval,dataptr dz)
{
	double mididiff;
	if(thisindex<0) {
		sprintf(errstr,"Interpolation val < 0 in interp_between_pitch_vals2()\n");
		return(PROGRAM_ERROR);
	}
	if(thisindex >= (dz->wlength - 2)) {
		*thismidi = pitch[(dz->wlength - 1)];
		return(FINISHED);
	}
	if(interpval < FLTERR) {
		*thismidi = pitch[thisindex];
		return(FINISHED);
	}
	if(interpval > (1.0 - FLTERR)) {
		*thismidi = pitch[thisindex+1];
		return(FINISHED);
	}
	mididiff = pitch[thisindex+1] - pitch[thisindex];
	*thismidi = pitch[thisindex] + (mididiff * interpval);
	return(FINISHED);
}

/***************** TRANSPOSE_OUTBUF *******************/

int transpose_outbuf(double transp,dataptr dz)
{
	int vc;
	for(vc=0;vc<dz->wanted;vc+=2)
		dz->flbufptr[1][FREQ] = (float)(dz->flbufptr[1][FREQ] * transp);
	return put_transposed_data_in_appropriate_chans(transp,dz);
}

/***************** PUT_TRANSPOSED_DATA_IN_APPROPRIATE_CHANS *******************/	

int put_transposed_data_in_appropriate_chans(double transp,dataptr dz)
{
	int exit_status;
	int cc,vc;
	if((exit_status = rectify_frqs(dz->flbufptr[1],dz))<0)
		return(exit_status);
	if(transp < 1.0) {	  			  /* If transposition is downwards, go up the channels */
		for(cc=0,vc=0; cc < dz->clength; cc++,vc+=2) {
			if((exit_status = put_thisdata_in_appropriate_chan(cc,vc,dz))<0)
				return(exit_status);
		}
	} else {						  /* If transposition is upwards, go down the channels */
		for(cc=dz->clength-1,vc = dz->wanted-2; cc >=0 ;cc--,vc-=2) {
			if((exit_status = put_thisdata_in_appropriate_chan(cc,vc,dz))<0)
				return(exit_status);
		}
	}
	return(FINISHED);
}

/***************** PUT_THISDATA_IN_APPROPRIATE_CHAN *******************/	

int put_thisdata_in_appropriate_chan(int cc,int vc, dataptr dz)
{
	int exit_status;
	int truecc, truevc, trutop;
	if(dz->flbufptr[1][FREQ] > dz->nyquist) {			/* zero out frqs above nyquist */
		dz->flbufptr[1][AMPP] = 0.0f;
		dz->flbufptr[1][FREQ] = (float)dz->nyquist;
		return(FINISHED);
	}									    /* if new frq outside acceptable frq limits for channel */
	if(dz->flbufptr[1][FREQ] < dz->flbufptr[WCHBOT][cc] || dz->flbufptr[1][FREQ] > dz->flbufptr[WCHTOP][cc]) { 
		if((exit_status = get_channel_corresponding_to_frq(&truecc,(double)dz->flbufptr[1][FREQ],dz))<0)
			return(exit_status);
		trutop = min(dz->clength-1,truecc + CHANSPAN);
		truecc = max(truecc - CHANSPAN,0); /* find bottom and top chans that could receive this frq */
		truevc = truecc * 2;				
		while(dz->flbufptr[1][FREQ] > dz->flbufptr[1][truevc+1]) {
			truevc += 2;		/* find position of new freq amongst freq ordering of existing data */
			if(truevc >= trutop)
				break;
		}
		truevc -= 2;
		do {
			if(dz->flbufptr[1][AMPP] > dz->flbufptr[1][truevc]) { /* If transpd data stronger than existing data  */
				dz->flbufptr[1][truevc]   = dz->flbufptr[1][AMPP];/* replace existing data by transposed data     */
				dz->flbufptr[1][truevc+1] = dz->flbufptr[1][FREQ];
				break;
		 	}
		 	truevc += 2;
		}  while(truevc < trutop);
		dz->flbufptr[1][AMPP] = 0.0f;			/* Zero out channel from which data has been moved  */
		if(cc>0 && cc < dz->clength-1)			/* Interp frq of that chan, betwn preexisting chans */
			dz->flbufptr[1][FREQ] = (float)((dz->flbufptr[1][FREQ+2]  + dz->flbufptr[1][FREQ-2])/2.0);
		else {					            	
			if(cc==0) dz->flbufptr[1][FREQ] = 0.0f;
			else	  dz->flbufptr[1][FREQ] = (float)dz->nyquist;
		}										/* This chan may later be filld by other moved data */
	}
	return(FINISHED);
}

/**************************** LOGREAD_SHIFT *****************************/

int logread_shift(double timenow,int btktable_no,dataptr dz)
{
	double *endpair, *p;
	double hival, loval, hiind, loind, lopshift, interval, newpshift;
    if(!dz->brkinit[btktable_no]) {
		dz->firstval[btktable_no] = *(dz->brk[btktable_no]+1);
		endpair                   = dz->brk[btktable_no] 
									+ ((dz->brksize[btktable_no]-1)*2);
		dz->lastind[btktable_no]  = *endpair;
		dz->lastval[btktable_no]  = *(endpair+1);
 		dz->brkinit[btktable_no]  = 1;
    }
	p = dz->brkptr[btktable_no];
	if(timenow <= *(dz->brk[btktable_no])) {
		dz->param[btktable_no] = dz->firstval[btktable_no];
		return(FINISHED);
	} else if(timenow >= dz->lastind[btktable_no]) {
		dz->param[btktable_no] = dz->lastval[btktable_no];
		return(FINISHED);
	} 
	if(timenow > *(p)) {
		while(*(p)<timenow)
			p += 2;
	} else {
		while(*(p)>=timenow)
			p -= 2;
		p += 2;
	}
	hival = *(p+1);
	hiind  = *p;
	loval = *(p-1);
	loind  = *(p-2);
	if(flteq(loval,hival))
		dz->param[btktable_no]  = loval;
	else if(loval < 0.0 && hival < 0.0) {
		lopshift  = log(-loval);
		interval  = log(-hival/-loval);
		interval *= (timenow - loind)/(hiind - loind);
		newpshift = lopshift + interval;
		dz->param[btktable_no] = -exp(newpshift);
	} else if (loval > 0.0 && hival > 0.0){
		lopshift  = log(loval);
		interval  = log(hival/loval);
		interval *= (timenow - loind)/(hiind - loind);
		newpshift = lopshift + interval;
		dz->param[btktable_no] = exp(newpshift);
	} else {
/*  if shift changes from +ve (or 0) to -ve (or 0) or v.v., do linear shift */
		interval  = hival - loval;
		interval *= (timenow - loind)/(hiind - loind);
		dz->param[btktable_no] = loval + interval;
	}	
 	dz->brkptr[btktable_no] = p;
	return(FINISHED);
}

/**************************** READ_VALUE_FROM_BRKTABLE_USING_LOG_INTERPOLATION *****************************
 *
 * Slightly weird function that fudges problem of log(0) and log(-ve).
 */

int read_value_from_brktable_using_log_interpolation(double thistime,int paramno,dataptr dz)
{
	double *endpair, *p;
	double hival, loval, hiind, loind, lopitch, interval, newpitch;

    if(!dz->brkinit[paramno]) {
		dz->brkptr[paramno]   = dz->brk[paramno];
		dz->firstval[paramno] = *(dz->brk[paramno]+1);
		endpair             = dz->brk[paramno] + ((dz->brksize[paramno]-1)*2);
		dz->lastind[paramno]  = *endpair;
		dz->lastval[paramno]  = *(endpair+1);
 		dz->brkinit[paramno] = 1;
    }
	p = dz->brkptr[paramno];
	if(thistime <= *(dz->brk[paramno])) {
		dz->param[paramno] = dz->firstval[paramno];
		return(FINISHED);
	} else if(thistime >= dz->lastind[paramno]) {
		dz->param[paramno] = dz->lastval[paramno];
		return(FINISHED);
	} 
	if(thistime > *(p)) {
		while(*(p)<thistime)
			p += 2;
	} else {
		while(*(p)>=thistime)
			p -= 2;
		p += 2;
	}
	hival = *(p+1);
	hiind = *p;
	loval = *(p-1);
	loind = *(p-2);
	if(flteq(loval,hival))
		dz->param[paramno]  = loval;
	else if(loval < 0.0 && hival < 0.0) {
		lopitch   = log(-loval);
		interval  = log(-hival/-loval);
		interval *= (thistime - loind)/(hiind - loind);
		newpitch  = lopitch + interval;
		dz->param[paramno] = -exp(newpitch);
	} else if (loval > 0.0 && hival > 0.0){
		lopitch   = log(loval);
		interval  = log(hival/loval);
		interval *= (thistime - loind)/(hiind - loind);
		newpitch  = lopitch + interval;
		dz->param[paramno] = exp(newpitch);
	} else {
		/*  if val changes from +ve (or 0) to -ve (or 0) or v.v., do linear shift */
		interval  = hival - loval;
		interval *= (thistime - loind)/(hiind - loind);
		dz->param[paramno] = loval + interval;
	}	
 	dz->brkptr[paramno] = p;
	return(FINISHED);
}

/**************************** ASSIGN_NEW_FRQS_TO_APPROPRIATE_CHANNELS ****************************
 *
 * (0)	Find the lowest channel for which we have generated a new freq value.
 * (0a) If no frqs lie below nyquist, zero all chan amps above botchan!!
 * (1)	Set initial amplitude for each transformed channel to zero.
 * (2)	Look through the set of stretched channels for any frequency
 *		values which lie below the top frq boundary of the current channel,
 * (3)	IF they are also above the bottom freq boundary of this channel,
 *		(i.e. they should be in THIS channel), if their amplitude
 *		is greater than any amp already assigned to this channel,
 *		substitute this frq/amp pair as the true value for this channel.
 * (4)	If we run out of values among the transformed freq values,
 *		break, and set OK=0 to break from outer loop too.
 * (5)	If we've broken from inner loop, break from outer loop too.
 * (6)	If we've broken from inner loop, zero amplitude of any channels
 *		which have not been assigned a new frequency.
 */

int assign_new_frqs_to_appropriate_channels(dataptr dz)
{
	int dd = dz->iparam[WAVER_BOTCHAN];								/* 0 */
	int  OK = 1, cc = 0, vc;
	while(dz->freq[dd] >= (float)(dz->parray[WAVER_CHFBOT][cc+1])) {
		if(++cc >= dz->clength) {					/* 0a */
			for(vc = dz->iparam[WAVER_BOTCHAN] * 2; vc < dz->wanted; vc += 2)
				dz->flbufptr[0][AMPP] = 0.0F;
			return(FINISHED);
		}
	}
	for(vc=cc*2; cc<dz->clength; cc++,vc+=2) {
		dz->flbufptr[0][AMPP] = 0.0F;					/* 1 */
		while(dz->freq[dd] < (float)(dz->parray[WAVER_CHFBOT][cc+1])) {	/* 2 */
			if((dz->freq[dd] >= dz->parray[WAVER_CHFBOT][cc]) && (dz->amp[dd] > dz->flbufptr[0][AMPP])) {
				dz->flbufptr[0][AMPP] = dz->amp[dd];		/* 3 */
				dz->flbufptr[0][FREQ] = dz->freq[dd];
			}
			if(++dd>=dz->clength) {					/* 4 */
				OK = 0;
				break;
			}
		}
		if(!OK)										/* 5 */
			break;
	}
	if(!OK) {
		while(vc<dz->wanted) {						/* 6 */
			dz->flbufptr[0][AMPP] = 0.0F;
			vc += 2;
		}
	}
	return(FINISHED);
}
