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



/* flotsam version */
#include <stdio.h>
#include <stdlib.h>
#include <structures.h>
#include <tkglobals.h>
#include <pnames.h>
#include <globcon.h>
#include <processno.h>
#include <modeno.h>
#include <special.h>
#include <logic.h>
#include <arrays.h>
#include <flags.h>
#include <repitch.h>
#include <cdpmain.h>
#include <formants.h>
#include <speccon.h>
#include <sfsys.h>
#include <osbind.h>
#include <repitch.h>
#include <pvoc.h>

#include <vowels.h>
#include <vowels2.h>


#if defined unix || defined __GNUC__
#define round(x) lround((x))
#endif

#define	GLARG		 (0.1)	/* 2nd-derivative maximum, for smoothing */
#define SLOPE_FUDGE  (0.2)


static int  specpitch(dataptr dz);
static int  spectrack(dataptr dz);
static int  tranpose_within_formant_envelope(int vc,dataptr dz);
static int  reposition_partials_in_appropriate_channels(int with_body,dataptr dz);
static int  zero_outofrange_channels(double *totalamp,double lofrq_limit,double hifrq_limit,dataptr dz);
static int  close_to_frq_already_in_ring(chvptr *there,double frq1,dataptr dz);
static int  substitute_in_ring(int vc,chvptr here,chvptr there,dataptr dz);
static int  insert_in_ring(int vc, chvptr here, dataptr dz);
static int  put_ring_frqs_in_ascending_order(chvptr **partials,float *minamp,dataptr dz);
static int  found_pitch(chvptr *partials,double lo_loud_partial,double hi_loud_partial,float minamp,dataptr dz);
static int  found_pitch_1(chvptr *partials,double lo_loud_partial,double hi_loud_partial,float minamp,dataptr dz);
static int  found_pitch_2(chvptr *partials,dataptr dz);
static int  smooth_spurious_octave_leaps(int pitchno,float minamp,dataptr dz);
static int  equivalent_pitches(double frq1, double frq2, dataptr dz);
static int  is_peak_at(double frq,int window_offset,float minamp,dataptr dz);
static int  enough_partials_are_harmonics(chvptr *partials,dataptr dz);
static int  is_a_harmonic(double frq1,double frq2,dataptr dz);
static int	do_pitch_cut(dataptr dz);
static int	do_pitch_filter(dataptr dz);
static int	do_pitch_smoothing(dataptr dz);
static int	skip_to_first_pitch(int *first_pitch,dataptr dz);
static int	calc_slopechanges(int m, double *slopechange,dataptr dz);
static int	is_start_of_glitch(int n,double *slopechange,dataptr dz);
static int	do_onset_smooth(int n, double *slopechange,dataptr dz);
static int	do_double_onset_smooth(int n, double *slopechange,dataptr dz);
static int	do_smooth(int n, double *slopechange,dataptr dz);
static int	get_max_and_min_pitches(double *maxpitch,double *minpitch,dataptr dz);
static int	write_remaining_pitch_or_transpos_data(int final_length_in_windows,dataptr dz);
static int	specpapprox(int *,double *,dataptr dz);
static int	specpcut(int *,dataptr dz);
static int	specpexag(dataptr dz);
static int 	specpinvert(dataptr dz);
static int 	specpquantise(dataptr dz);
static int	specprand(dataptr dz);
static int	specpsmooth(dataptr dz);
static int 	specptranspose(dataptr dz);
static int	specpvib(dataptr dz);
static int	get_midimean(double *midimean,dataptr dz);
static int 	set_pval(double midivalue,int n,dataptr dz);
static int	do_tail(int n, double lastmidi,dataptr dz);
static int	get_pitchapprox_averages(int *avcnt,dataptr dz);
static int	get_rand_interval(double *thisintv,dataptr dz);
static int	approx_func1(int *,int *,double *,int *,int n,dataptr dz);
static int	approx_func2(int *newlength_of_data,double lastmidi,int lastpos,int avcnt,dataptr dz);
static int	interval_mapping(double *thisint,double thismidi,dataptr dz);
static int	peak_interp(int pitchno,int last_validpitch_no,int *lastmaxpos,double meanpich,
		double minint,double maxint,double *lastmidi,dataptr dz);

static int  tidy_up_pitch_data(dataptr dz);
static int  generate_tone(dataptr dz);
static int  anti_noise_smoothing(int wlength,float *pitches,float frametime);
static int  is_smooth_from_both_sides(int n,double max_pglide,float *pitches);
static int  is_initialpitch_smooth(char *smooth,double max_pglide,float *pitches);
static int  is_finalpitch_smooth(char *smooth,double max_pglide,int wlength,float *pitches);
static int  is_smooth_from_before(int n,char *smooth,double max_pglide,float *pitches);
static int  is_smooth_from_after(int n,char *smooth,double max_pglide,float *pitches);
static int  test_glitch_sets(char *smooth,double max_pglide,int wlength,float *pitches);
static void remove_unsmooth_pitches(char *smooth,int wlength,float *pitches);
static int  test_glitch_forwards(int gltchstart,int gltchend,char *smooth,double max_pglide,float *pitches);
static int  test_glitch_backwards(int gltchstart,int gltchend,char *smooth,
				double max_pglide,int wlength,float *pitches);

/* RWD NB: changes outfile header properties - rejigging required! */


static int  write_pitch_outheader_from_analysis_inheader_to_second_outfile(int ofd,dataptr dz);

static int  local_peak(int thiscc,double frq, float *thisbuf, dataptr dz);
static int  interpolate_pitch(float *floatbuf,int skip_silence,dataptr dz);
static double hz_to_pitchheight(double frqq);
static double pitchheight_to_hz(double pitch_height);
static int    eliminate_blips_in_pitch_data(dataptr dz);
static int    mark_zeros_in_pitchdata(dataptr dz);
static int    pitch_found(dataptr dz);
static int    do_interpolating(int *pitchno,float *floatbuf,int skip_silence,dataptr dz);
static void   check_transpos(float *t,dataptr dz);
static void   check_pitch(float *t,dataptr dz);
static int 	  trap_junk(int final_length_in_windows,dataptr dz);
static int 	  write_pitch_or_transpos_data(int final_length_in_windows,dataptr dz);
static int	  pitch_insert(int is_sil,dataptr dz);
static int	  pitch_to_silence(dataptr dz);
static int	  unpitch_to_silence(dataptr dz);
static int 	  generate_vowels(dataptr dz);
static int 	  generate_vowel_spectrum(double frq,double formant1,double formant2,double formant3,
				double f2atten,double f3atten,double *sensitivity,int senslen,int is_offset,dataptr dz);
static int 	  get_formant_frqs(int vowel,double *formant1,double *formant2,double *formant3,
				double *f2atten,double *f3atten);
static int 	  define_sensitivity_curve(double **sensitivity,int *senslen);
static int 	  adjust_for_sensitivity(double *amp,double frq,double *sensitivity,int senslen);
static int 	  remove_pitch_zeros(dataptr dz);

static int convert_single_window_pch_or_transpos_data_to_brkpnttable
			(int *brksize,float *floatbuf,float frametime,int array_no,dataptr dz);

int are_pitch_zeros = 0;

/********************************** SPECTRNSF **********************************
 *
 * transpose spectrum, but retain original spectral envelope.
 */

int spectrnsf(dataptr dz)
{
	int exit_status;
	double pre_totalamp, post_totalamp;
	double lofrq_limit, hifrq_limit;
	int cc, vc;
	rectify_window(dz->flbufptr[0],dz);
	if((exit_status = extract_specenv(0,0,dz))<0)
		return(exit_status);
	if((exit_status = get_totalamp(&pre_totalamp,dz->flbufptr[0],dz->wanted))<0)
		return(exit_status);
	for(cc = 0, vc = 0; cc < dz->clength; cc++, vc += 2) {
		if((exit_status = tranpose_within_formant_envelope(vc,dz))<0)
			return(exit_status);
	}
	if((exit_status = reposition_partials_in_appropriate_channels(TRNSF_BODY,dz))<0)
		return(exit_status);
	if(dz->vflag[TRNSF_FBOT] || dz->vflag[TRNSF_FTOP]) {
		lofrq_limit = dz->param[TRNSF_LOFRQ];
		hifrq_limit = dz->param[TRNSF_HIFRQ];
		if(hifrq_limit <  lofrq_limit)
			swap(&hifrq_limit,&lofrq_limit);
		if((exit_status = zero_outofrange_channels(&post_totalamp,lofrq_limit,hifrq_limit,dz))<0)
			return(exit_status);
	} else {
		if((exit_status = get_totalamp(&post_totalamp,dz->flbufptr[0],dz->wanted))<0)
			return(exit_status);
	}
	return normalise(pre_totalamp,post_totalamp,dz);
}


/********************************** SPECTRNSP **********************************
 *
 * transpose spectrum, (spectral envelope also moves).
 */
int spectrnsp(dataptr dz)
{
	int exit_status;
	double pre_totalamp, post_totalamp;
	double lofrq_limit, hifrq_limit;
	int cc, vc;
	rectify_window(dz->flbufptr[0],dz);
	if((exit_status = get_totalamp(&pre_totalamp,dz->flbufptr[0],dz->wanted))<0)
		return(exit_status);
	for(cc = 0, vc = 0; cc < dz->clength; cc++, vc += 2)
		dz->flbufptr[0][FREQ] = (float)(dz->flbufptr[0][FREQ]*dz->transpos[dz->total_windows]);
	if((exit_status = reposition_partials_in_appropriate_channels(TRNSP_BODY,dz))<0)
		return(exit_status);
	if(dz->vflag[TRNSP_FBOT] || dz->vflag[TRNSP_FTOP]) {
		lofrq_limit = dz->param[TRNSP_LOFRQ];
		hifrq_limit = dz->param[TRNSP_HIFRQ];
		if(hifrq_limit <  lofrq_limit)
			swap(&hifrq_limit,&lofrq_limit);
		if((exit_status = zero_outofrange_channels(&post_totalamp,lofrq_limit,hifrq_limit,dz))<0)
			return(exit_status);
	} else {
		if((exit_status = get_totalamp(&post_totalamp,dz->flbufptr[0],dz->wanted))<0)
			return(exit_status);
	}
	return normalise(pre_totalamp,post_totalamp,dz);
}

/************************** TRANPOSE_WITHIN_FORMANT_ENVELOPE *****************************/

int tranpose_within_formant_envelope(int vc,dataptr dz)
{
	int exit_status;
	double thisspecamp, newspecamp, thisamp, formantamp_ratio; 
	if((exit_status = getspecenvamp(&thisspecamp,(double)dz->flbufptr[0][FREQ],0,dz))<0)
		return(exit_status);
	dz->flbufptr[0][FREQ] = (float)(fabs(dz->flbufptr[0][FREQ])*dz->transpos[dz->total_windows]);
	if(dz->flbufptr[0][FREQ] < dz->nyquist) {
		if(thisspecamp < VERY_TINY_VAL)
			dz->flbufptr[0][AMPP] = 0.0f;
		else {
			if((exit_status = getspecenvamp(&newspecamp,(double)dz->flbufptr[0][FREQ],0,dz))<0)
				return(exit_status);
			if(newspecamp < VERY_TINY_VAL)
				dz->flbufptr[0][AMPP] = 0.0f;
			else {
				formantamp_ratio = newspecamp/thisspecamp;
				if((thisamp = dz->flbufptr[0][AMPP] * formantamp_ratio) < VERY_TINY_VAL)
					dz->flbufptr[0][AMPP] = 0.0f;
				else
					dz->flbufptr[0][AMPP] = (float)thisamp;
			}
		}
	}
	return(FINISHED);
}

/************************ REPOSITION_PARTIALS_IN_APPROPRIATE_CHANNELS *************************
 *
 * (1)	At each pass, preset store-buffer channel amps to zero.
 * (2)	Move frq data into appropriate channels, carrying the
 *		amplitude information along with them.
 *		Work down spectrum 	for upward transposition, and
 * (3)	up spectrum for downward transposition,
 *		so that we do not overwrite transposed data before we move it.
 * (4)	Put new frqs back into src buff.
 */

int reposition_partials_in_appropriate_channels(int with_body,dataptr dz)
{
	int exit_status;
	int truecc,truevc;
	int cc, vc;
	for(vc = 0; vc < dz->wanted; vc+=2)						/* 1 */
		dz->windowbuf[0][vc] = 0.0f;
	if(dz->transpos[dz->total_windows] > 1.0f) {			/* 2 */
		for(cc=dz->clength-1,vc = dz->wanted-2; cc>=0; cc--, vc-=2) {
			if(dz->flbufptr[0][FREQ] < dz->nyquist && dz->flbufptr[0][AMPP] > 0.0f) {
				if((exit_status = get_channel_corresponding_to_frq(&truecc,(double)dz->flbufptr[0][FREQ],dz))<0)
					return(exit_status);
				truevc = truecc * 2;
				switch(dz->vflag[with_body]) {			
				case(FALSE): 
					if((exit_status = move_data_into_appropriate_channel(vc,truevc,dz->flbufptr[0][AMPP],dz->flbufptr[0][FREQ],dz))<0)
						return(exit_status);
					break;
				case(TRUE):  
					if((exit_status = move_data_into_some_appropriate_channel(truevc,dz->flbufptr[0][AMPP],dz->flbufptr[0][FREQ],dz))<0)
						return(exit_status);
					break;
				default:
					sprintf(errstr,"Unknown case for vflag[with_body]: reposition_partials_in_appropriate_channels()\n");
					return(PROGRAM_ERROR);
				}								/* upward transpos, chandata tends to thin */
			}									/* case(TRUE) tries for fuller spectrum    */
		}
		for(vc = 0; vc < dz->wanted; vc++)
			dz->flbufptr[0][vc] = dz->windowbuf[0][vc];	
	} else if(dz->transpos[dz->total_windows] < 1.0f){		/* 3 */
		for(cc=0,vc = 0; cc < dz->clength; cc++, vc+=2) {
			if(dz->flbufptr[0][FREQ] < dz->nyquist && dz->flbufptr[0][FREQ]>0.0) {
				if((exit_status = get_channel_corresponding_to_frq(&truecc,(double)dz->flbufptr[0][FREQ],dz))<0)
					return(exit_status);
				truevc = truecc * 2;
				if((exit_status = move_data_into_appropriate_channel(vc,truevc,dz->flbufptr[0][AMPP],dz->flbufptr[0][FREQ],dz))<0)
					return(exit_status);
			}
		}
		for(vc = 0; vc < dz->wanted; vc++)
			dz->flbufptr[0][vc] = dz->windowbuf[0][vc];		/* 4 */
	}
	return(FINISHED);
}

/******************* ZERO_OUTOFRANGE_CHANNELS *****************/

int zero_outofrange_channels(double *totalamp,double lofrq_limit,double hifrq_limit,dataptr dz)
{
	int cc, vc;
	*totalamp = 0.0;
	for(cc = 0,vc = 0; cc < dz->clength; cc++, vc += 2) {
		if(dz->flbufptr[0][FREQ] < lofrq_limit || dz->flbufptr[0][FREQ] > hifrq_limit)
			dz->flbufptr[0][AMPP] = 0.0f;
		else
			*totalamp += dz->flbufptr[0][AMPP];
	}
	return(FINISHED);
}

/***************************** OUTER_PITCH_LOOP ***********************/

int outer_pitch_loop(dataptr dz)
{
	int exit_status;
	int samps_read, wc, windows_in_buf, brklen;
	double totalamp;
	int thismode = 0;
	while((samps_read = fgetfbufEx(dz->bigfbuf,dz->big_fsize,dz->ifd[0],0)) > 0) {
		dz->flbufptr[0] = dz->bigfbuf;
		windows_in_buf = samps_read/dz->wanted;    
		for(wc=0; wc<windows_in_buf; wc++, dz->total_windows++) {
			if(dz->total_windows==0 && dz->wlength > 1) {
				dz->pitches[0] = (float)NOT_PITCH;
				dz->flbufptr[0] += dz->wanted;
				continue;
			}
			if((exit_status = get_totalamp(&totalamp,dz->flbufptr[0],dz->wanted))<0)
				return(exit_status);
			dz->parray[PICH_PRETOTAMP][dz->total_windows] = totalamp;
			switch(dz->process) {
			case(PITCH):
				if((exit_status = specpitch(dz))<0)
					return(exit_status);
				break;
			case(TRACK):	
				if((exit_status = spectrack(dz))<0)
					return(exit_status);
				break;
			default:
				sprintf(errstr,"Unknown case in outer_pitch_loop()\n");
				return(PROGRAM_ERROR);
			}
			dz->flbufptr[0] += dz->wanted;
		}
	}  
	if(samps_read<0) {
		sprintf(errstr,"Sound read error.\n");
		return(SYSTEM_ERROR);
	}
	if((exit_status = tidy_up_pitch_data(dz))<0)
		return(exit_status);
	if((exit_status = generate_tone(dz))<0)
		return(exit_status);

	dz->total_samps_written = 0;

	switch(dz->process) {
	case(PITCH):	if(dz->mode == PICH_TO_BRK)	thismode = 1;	break;	
	case(TRACK):	if(dz->mode == TRK_TO_BRK)	thismode = 1;	break;	
	default:
		sprintf(errstr,"Unknown mode in outer_pitch_loop().\n");
		return(PROGRAM_ERROR);
	}

	switch(thismode) {
	case(0):

		if((exit_status = write_samps_to_elsewhere(dz->other_file,dz->pitches,dz->wlength,dz))<0)
			return(exit_status);
		if((exit_status = write_pitch_outheader_from_analysis_inheader_to_second_outfile
		(dz->other_file,dz))<0)
			return(exit_status);
		break;
	case(1):
/* MAY 2001 BRKPNT OUTPUT ELIMINATES PITCH ZEROS */
		if((exit_status = interpolate_pitch(dz->pitches,0,dz))<0)
			return(exit_status);
		if(dz->wlength == 1) {
			if((exit_status = convert_single_window_pch_or_transpos_data_to_brkpnttable
			(&brklen,dz->pitches,dz->frametime,PICH_PBRK,dz))<0)
				return(exit_status);
		} else {
			if((exit_status = convert_pch_or_transpos_data_to_brkpnttable
			(&brklen,dz->pitches,dz->frametime,PICH_PBRK,dz))<0)
				return(exit_status);
		}
		if((exit_status = write_brkfile(dz->fp,brklen,PICH_PBRK,dz))<0)
			return(exit_status);
		if(fclose(dz->fp)<0) {
			fprintf(stdout, "WARNING: Failed to close output brkpntfile.\n");
			fflush(stdout);
		}
		break;
	default:
		sprintf(errstr,"unknown output case in outer_pitch_loop()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/****************************** SPECPITCH *******************************
 *
 * (1)	Ignore partials below low limit of pitch.
 * (2)  If this channel data is louder than any existing piece of data in ring.
 *		(Ring data is ordered loudness-wise)...
 * (3)	If this freq is too close to an existing frequency..
 * (4)	and if it is louder than that existing frequency data..
 * (5)	Substitute in in the ring.
 * (6)	Otherwise, (its a new frq) insert it into the ring.
 */
 
int specpitch(dataptr dz)
{
	int exit_status;
	int vc;
	chvptr here, there, *partials;
	float minamp;
	double loudest_partial_frq, nextloudest_partial_frq, lo_loud_partial, hi_loud_partial;	
	if((partials = (chvptr *)malloc(MAXIMI * sizeof(chvptr)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for partials array.\n");
		return(MEMORY_ERROR);
	}
	if((exit_status = initialise_ring_vals(MAXIMI,-1.0,dz))<0)
		return(exit_status);
	if((exit_status = rectify_frqs(dz->flbufptr[0],dz))<0)
		return(exit_status);
	for(vc=0;vc<dz->wanted;vc+=2) {
		here = dz->ringhead;
		if(dz->flbufptr[0][FREQ] > dz->param[PICH_LOLM]) {  			/* 1 */
			do {								
				if(dz->flbufptr[0][AMPP] > here->val) {			   	/* 2 */
					if((exit_status = close_to_frq_already_in_ring(&there,(double)dz->flbufptr[0][FREQ],dz))<0)
						return(exit_status);
					if(exit_status==TRUE) {
						if(dz->flbufptr[0][AMPP] > there->val) {		/* 4 */
							if((exit_status = substitute_in_ring(vc,here,there,dz))<0) /* 5 */
								return(exit_status);
						}
					} else	{										/* 6 */
						if((exit_status = insert_in_ring(vc,here,dz))<0)
							return(exit_status);
					}				
					break;
				}
			} while((here = here->next)!=dz->ringhead);
		}
	}
	loudest_partial_frq     = dz->flbufptr[0][dz->ringhead->loc + 1];
	nextloudest_partial_frq = dz->flbufptr[0][dz->ringhead->next->loc + 1];
	if(loudest_partial_frq < nextloudest_partial_frq) {
		lo_loud_partial = loudest_partial_frq;
		hi_loud_partial = nextloudest_partial_frq;
	} else {
		lo_loud_partial = nextloudest_partial_frq;
		hi_loud_partial = loudest_partial_frq;
	}
	if((exit_status = put_ring_frqs_in_ascending_order(&partials,&minamp,dz))<0)
		return(exit_status);
	if((exit_status = found_pitch(partials,lo_loud_partial,hi_loud_partial,minamp,dz))<0)
		return(exit_status);
	if(exit_status==TRUE && dz->param[PICH_PICH]>=MINPITCH)
		dz->pitches[dz->total_windows] = (float)dz->param[PICH_PICH];
	else
		dz->pitches[dz->total_windows] = (float)NOT_PITCH;
	return smooth_spurious_octave_leaps(dz->total_windows,minamp,dz);
}

/**************************** CLOSE_TO_FRQ_ALREADY_IN_RING *******************************/

int close_to_frq_already_in_ring(chvptr *there,double frq1,dataptr dz)
{
#define EIGHT_OVER_SEVEN	(1.142857143)

	double frq2, frqratio;
	*there = dz->ringhead;
	do {
		if((*there)->val > 0.0) {
			frq2 = dz->flbufptr[0][(*there)->loc + 1];
			if(frq1 > frq2)
				frqratio = frq1/frq2;
			else
			    frqratio = frq2/frq1;
			if(frqratio < EIGHT_OVER_SEVEN)
				return(TRUE);
		}
	} while((*there = (*there)->next) != dz->ringhead);
	return(FALSE);
}

/******************************* SUBSITUTE_IN_RING **********************/

int substitute_in_ring(int vc,chvptr here,chvptr there,dataptr dz)
{
	chvptr spare, previous;
	if(here!=there) {
		if(there==dz->ringhead) {
			sprintf(errstr,"IMPOSSIBLE! in substitute_in_ring()\n");
			return(PROGRAM_ERROR);
		}
		spare = there;
		there->next->last = there->last; /* SPLICE REDUNDANT STRUCT FROM RING */
		there->last->next = there->next;
		previous = here->last;
		previous->next = spare;			/* SPLICE ITS ADDRESS-SPACE BACK INTO RING */
		spare->last = previous;			/* IMMEDIATELY BEFORE HERE */
		here->last = spare;
		spare->next = here;
		if(here==dz->ringhead)			/* IF HERE IS RINGHEAD, MOVE RINGHEAD */
			dz->ringhead = spare;
		here = spare;					/* POINT TO INSERT LOCATION */
	}
	here->val = dz->flbufptr[0][AMPP]; 	/* IF here==there */
	here->loc = vc;	    				/* THIS WRITES OVER VAL IN EXISTING RING LOCATION */
	return(FINISHED);
}

/*************************** INSERT_IN_RING ***************************/

int insert_in_ring(int vc, chvptr here, dataptr dz)
{
	chvptr previous, newend, spare;
	if(here==dz->ringhead) {
		dz->ringhead = dz->ringhead->last;
		spare = dz->ringhead;
	} else {
		if(here==dz->ringhead->last)
			spare = here;
		else {
			spare  = dz->ringhead->last;
			newend = dz->ringhead->last->last; 	/* cut ENDADR (spare) out of ring */
			dz->ringhead->last = newend;
			newend->next = dz->ringhead;
			previous = here->last;
			here->last = spare;					/* reuse spare address at new loc by */ 
			spare->next = here;  				/* inserting it back into ring before HERE */
			previous->next = spare;
			spare->last = previous;
		}
	}
	spare->val = dz->flbufptr[0][vc];			/* Store new val in spare ring location */
	spare->loc = vc;
	return(FINISHED);
}

/************************** PUT_RING_FRQS_IN_ASCENDING_ORDER **********************/

int put_ring_frqs_in_ascending_order(chvptr **partials,float *minamp,dataptr dz)
{
	int k;
	chvptr start, ggot, here = dz->ringhead;
	float minpitch;
	*minamp = (float)MAXFLOAT;
	for(k=0;k<MAXIMI;k++) {
		if((*minamp = min(dz->flbufptr[0][here->loc],*minamp))>=(float)MAXFLOAT) {
			sprintf(errstr,"Problem with amplitude out of range: put_ring_frqs_in_ascending_order()\n");
			return(PROGRAM_ERROR);
		}
		(here->loc)++;		/* CHANGE RING TO POINT TO FRQS, not AMPS */
		here->val = dz->flbufptr[0][here->loc];
		here = here->next;
	}
	here = dz->ringhead;
	minpitch = dz->flbufptr[0][here->loc];
	for(k=1;k<MAXIMI;k++) {
		start = ggot = here;
		while((here = here->next)!=start) {		/* Find lowest frq */
			if(dz->flbufptr[0][here->loc] < minpitch) {	
				minpitch = dz->flbufptr[0][here->loc];
				ggot = here;
			}
		}
		(*partials)[k-1] = ggot;				/* Save its address */
		here = ggot->next;						/* Move to next ring site */
		minpitch = dz->flbufptr[0][here->loc];	/* Preset minfrq to val there */
		ggot->last->next = here;				/* Unlink ringsite ggot */
		here->last = ggot->last;
	}
	(*partials)[k-1] = here;	     			/* Remaining ringsite is maximum */

	here = dz->ringhead = (*partials)[0];   	/* Reconstruct ring */
	for(k=1;k<MAXIMI;k++) {
		here->next = (*partials)[k];
		(*partials)[k]->last = here;
		here = here->next;
	}
	here->next = dz->ringhead;					/* Close up ring */
	dz->ringhead->last = here;
	return(FINISHED);
}

/******************************	 FIND_PITCH **************************/

int found_pitch(chvptr *partials,double lo_loud_partial,double hi_loud_partial,float minamp,dataptr dz)
{
	switch(dz->vflag[PICH_ALTERNATIVE_METHOD]) {
	case(FALSE): return(found_pitch_1(partials,lo_loud_partial,hi_loud_partial,minamp,dz));
	case(TRUE):	 return(found_pitch_2(partials,dz));
	default:
		sprintf(errstr,"Unknown case in found_pitch()\n");
		return(PROGRAM_ERROR);
	}
}

/******************************	 FIND_PITCH_1 **************************/

#define MAXIMUM_PARTIAL (64)

int found_pitch_1(chvptr *partials,double lo_loud_partial,double hi_loud_partial,float minamp,dataptr dz)
{
	int n, m, k, maximi_less_one = MAXIMUM_PARTIAL - 1, endd = 0;
	double whole_number_ratio, comparison_frq;
	for(n=1;n<maximi_less_one;n++) {
		for(m=n+1;m<MAXIMUM_PARTIAL;m++) {	/* NOV 7 */
			whole_number_ratio = (double)m/(double)n;
			comparison_frq     = lo_loud_partial * whole_number_ratio;
			if(equivalent_pitches(comparison_frq,hi_loud_partial,dz))
				endd = (MAXIMUM_PARTIAL/m) * n;		/* explanation at foot of file */
			else if(comparison_frq > hi_loud_partial)
				break;


			for(k=n;k<=endd;k+=n) {
				dz->param[PICH_PICH] = lo_loud_partial/(double)k;
				if(dz->param[PICH_PICH]>dz->param[PICH_HILM])
					continue;
				if(dz->param[PICH_PICH]<dz->param[PICH_LOLM])
					break;
				if(is_peak_at(dz->param[PICH_PICH],0,minamp,dz)){
					if(dz->iparam[PICH_MATCH] <= 2)
						return TRUE;
					else if(enough_partials_are_harmonics(partials,dz))
						return TRUE;
				}
			}
		}
	}
	return(FALSE);
}


/********************** FIND_PITCH_2 *****************************/

int found_pitch_2(chvptr *partials,dataptr dz)
{
	int m, n, k, good_match;
	int top_of_test = MAXIMI - dz->iparam[PICH_MATCH] + 1;
	double resolved_pitch, prevpitch,pitchdiff = 0.0;
	int diffcount = 0;
	for(n=0;n<top_of_test;n++) {
		for(m=1;m<=MAXHARM;m++) {
			dz->param[PICH_PICH] = (partials[n]->val)/(double)m;
			if(dz->param[PICH_PICH] > dz->param[PICH_HILM])
				continue;	
			if(dz->param[PICH_PICH] < dz->param[PICH_LOLM])
				break;
			good_match = 1;
			prevpitch = dz->param[PICH_PICH];

			if(dz->iparam[PICH_MATCH] > 1) {
				for(k=n+1;k<MAXIMI;k++) {
					if(is_a_harmonic((double)(partials[k]->val),dz->param[PICH_PICH],dz)) {
						pitchdiff += (double)(partials[k]->val - prevpitch);
						prevpitch = (double)(partials[k]->val);
						diffcount++;
						if(++good_match >= dz->iparam[PICH_MATCH]) {
							resolved_pitch = pitchdiff / (double)diffcount;
							if(equivalent_pitches(resolved_pitch,dz->param[PICH_PICH],dz)){
								partials[0]->val = (float) resolved_pitch;							
								dz->param[PICH_PICH] = resolved_pitch;
							}
							return TRUE;
						}
					}
				}
			}
		}
	}
	return(FALSE);
}

/************************ SMOOTH_SPURIOUS_OCTAVE_LEAPS ***************************/

int smooth_spurious_octave_leaps(int pitchno,float minamp,dataptr dz)
{
#define ALMOST_TWO	        (1.75)

	double thispitch = dz->pitches[pitchno];
	double startpitch, lastpitch;
	int k = 0;
	if(pitchno<=0)
		return(FINISHED);
	lastpitch = dz->pitches[pitchno-1];
	if(lastpitch > dz->param[PICH_LOLM] && thispitch > dz->param[PICH_LOLM]) {	/* OCTAVE ADJ HERE */
		if(thispitch > lastpitch) {		/* OCTAVE ADJ FORWARDS */
			startpitch = thispitch;
			while(thispitch/lastpitch > ALMOST_TWO)
				thispitch /= 2.0;
			if(thispitch!=startpitch) {
				if(thispitch < dz->param[PICH_LOLM])
					return(FINISHED);
				if(is_peak_at(thispitch,0L,minamp,dz))				
					dz->pitches[pitchno] = (float)thispitch;
				else 
					dz->pitches[pitchno] = (float)startpitch;
			}
			return(FINISHED);
		} else {
			while(pitchno>=1) {			/* OCTAVE ADJ BCKWARDS */
				k++;
				if((thispitch = dz->pitches[pitchno--])<dz->param[PICH_LOLM])
					return(FINISHED);
			
				if((lastpitch = dz->pitches[pitchno])<dz->param[PICH_LOLM])
					return(FINISHED);
			
				startpitch = lastpitch;
				while(lastpitch/thispitch > ALMOST_TWO)
					lastpitch /= 2.0;
			
				if(lastpitch!=startpitch) {
					if(lastpitch < dz->param[PICH_LOLM])
						return(FINISHED);
					if(is_peak_at(lastpitch,k,minamp,dz))					
						dz->pitches[pitchno] = (float)lastpitch;
					else 
						dz->pitches[pitchno] = (float)startpitch;
				}
			}
		}
	}
	return(FINISHED);
}

/**************************** EQUIVALENT_PITCHES *************************/

int equivalent_pitches(double frq1, double frq2, dataptr dz)
{
	double ratio;
	int   iratio;
	double intvl;

	ratio = frq1/frq2;
	iratio = round(ratio);

	if(iratio!=1)
		return(FALSE);

	if(ratio > iratio)
		intvl = ratio/(double)iratio;
	else
		intvl = (double)iratio/ratio;
	if(intvl > dz->param[PICH_RNGE])	
		return FALSE;
	return TRUE;
}

/*************************** IS_PEAK_AT ***************************/

#define PEAK_LIMIT (.05)

int is_peak_at(double frq,int window_offset,float minamp,dataptr dz)
{
	float *thisbuf;
	int cc, vc, searchtop, searchbot;
	if(window_offset) {								/* BAKTRAK ALONG BIGBUF, IF NESS */
		thisbuf = dz->flbufptr[0] - (window_offset * dz->wanted);
		if((int)thisbuf < 0 || thisbuf < dz->bigfbuf || thisbuf >= dz->flbufptr[1])
			return(FALSE);
	} else
		thisbuf = dz->flbufptr[0];
	cc = (int)((frq + dz->halfchwidth)/dz->chwidth);		 /* TRUNCATE */
	searchtop = min(dz->clength,cc + CHANSCAN + 1);
	searchbot = max(0,cc - CHANSCAN);
	for(cc = searchbot ,vc = searchbot*2; cc < searchtop; cc++, vc += 2) {
		if(!equivalent_pitches((double)thisbuf[vc+1],frq,dz)) {
			continue;
		}
		if(thisbuf[vc] < minamp * PEAK_LIMIT)
			continue;
		if(local_peak(cc,frq,thisbuf,dz))		
			return TRUE;
	}
	return FALSE;
}

/**************************** ENOUGH_PARTIALS_ARE_HARMONICS *************************/

int enough_partials_are_harmonics(chvptr *partials,dataptr dz)
{
	int n, good_match = 0;
	double thisfrq;
	for(n=0;n<MAXIMI;n++) {
		if((thisfrq = dz->flbufptr[0][partials[n]->loc]) < dz->param[PICH_PICH])
			continue;
		if(is_a_harmonic(thisfrq,dz->param[PICH_PICH],dz)){		
			if(++good_match >= dz->iparam[PICH_MATCH])
				return TRUE;
		}
	}
	return FALSE;
}

/**************************** IS_A_HARMONIC *************************/

int is_a_harmonic(double frq1,double frq2,dataptr dz)
{
	double ratio = frq1/frq2;
	int   iratio = round(ratio);
	double intvl;

	ratio = frq1/frq2;
	iratio = round(ratio);

	if(ratio > iratio)
		intvl = ratio/(double)iratio;
	else
		intvl = (double)iratio/ratio;
	if(intvl > dz->param[PICH_RNGE])
		return(FALSE);
	return(TRUE);
}

/***************************** SPECTRACK *********************************/

int spectrack(dataptr dz)
{
	int exit_status;
	double thisfrq =  dz->param[TRAK_PICH], frqtop, frqbot, maxamp;
	double outfrq  = 0.0;
	int n = 0, vc = 0, passed_limit = 0, maxloc;
	while((thisfrq =  dz->param[TRAK_PICH] * (double)++n) < dz->param[TRAK_HILM]) {
		frqtop   = thisfrq * dz->param[TRAK_RNGE];
		frqbot   = thisfrq / dz->param[TRAK_RNGE];
		if((exit_status = rectify_frqs(dz->flbufptr[0],dz))<0)
			return(exit_status);
		while(dz->flbufptr[0][FREQ] < frqbot) {	/* JUMP OVER CHANNELS  */
			if((vc+=2) >=dz->wanted) {
				outfrq = -1.0;
				passed_limit = 1;
				break;
			}
		}
		if(passed_limit)						/* IF GONE PAST LIMIT  */
			break;								/* BREAK OUT OF LOOP   */
		if(dz->flbufptr[0][FREQ]>frqtop) {		/* IF OUTSIDE RANGE    */
			outfrq = -1.0;						/* BREAK OUT OF LOOP   */
			break;
		}
		maxamp   = dz->flbufptr[0][AMPP];		/* SET MAXAMP TO 1ST IN-RANGE CH */
		maxloc   = vc;							/* NOTE NO. OF CH THUS MARKED    */
		while((vc+=2) < dz->wanted) {
			if(dz->flbufptr[0][FREQ]>frqtop)   	/* IF BEYOND CURRENT RANGE, STOP */
				break;
			if(dz->flbufptr[0][AMPP]>maxamp) {	/* IF LOUDER THAN MAX, RESET MAX */
				maxamp = dz->flbufptr[0][AMPP];	/* AND RESET MAXAMP CHANNEL NO.  */
				maxloc = vc;
			}
		}
		outfrq = dz->flbufptr[0][maxloc+1];
	}
	if(outfrq>MINPITCH)					/* If pitch has been found  */
		dz->param[TRAK_PICH] = outfrq;	/* reset goal pitch to this */
	dz->pitches[dz->total_windows] = (float)outfrq;
	return(FINISHED);
}

/**************************** OUTER_PICHPICH_LOOP *********************/

int outer_pichpich_loop(dataptr dz)
{
	int exit_status, valid_pitch_data = FALSE;
	int final_length_in_windows = dz->wlength, n;
	double lastmidi;
	if(dz->is_transpos) {
		if((dz->transpos = (float *)malloc(dz->wlength * sizeof(float)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for transpositions array.\n");
			return(MEMORY_ERROR);
		}
		for(n=0;n<dz->wlength;n++)
			dz->transpos[n] = 1.0f;	/* DEFAULT: no transposition */
	}
	for(n=0;n<dz->wlength;n++) {
		if(dz->pitches[n] > FLTERR) {
			valid_pitch_data = 1;
			break;
		}
	}
	if(!valid_pitch_data) {
		sprintf(errstr,"No valid pitches found in input data.\n");
		return(DATA_ERROR);
	}
	switch(dz->process) {
	case(P_APPROX):
		if((exit_status = specpapprox(&final_length_in_windows,&lastmidi,dz))<0)
			return(exit_status);
		if((exit_status = trap_junk(final_length_in_windows,dz))<0)
			return(exit_status);
		return write_remaining_pitch_or_transpos_data(final_length_in_windows,dz);
	case(P_CUT):		
		if((exit_status = specpcut(&final_length_in_windows,dz))<0)
			return(exit_status);
		return write_remaining_pitch_or_transpos_data(final_length_in_windows,dz);
	case(P_EXAG):	
		if((exit_status = specpexag(dz))<0)
			return(exit_status);
		break;
	case(P_INVERT):		
		if((exit_status = specpinvert(dz))<0)
			return(exit_status);
		break;
	case(P_QUANTISE):	
		if((exit_status = specpquantise(dz))<0)
			return(exit_status);
		break;
	case(P_RANDOMISE):	
		if((exit_status = specprand(dz))<0)
			return(exit_status);
		break;
	case(P_SMOOTH):		
		if((exit_status = specpsmooth(dz))<0)
			return(exit_status);
		break;
	case(P_TRANSPOSE):	
		if((exit_status = specptranspose(dz))<0)
			return(exit_status);
		break;
	case(P_VIBRATO):	
		if((exit_status = specpvib(dz))<0)
			return(exit_status);
		break;
	case(P_SYNTH):	
		return generate_tone(dz);
		break;
	case(P_VOWELS):	
		return generate_vowels(dz);
		break;
	case(P_INSERT):	
		if((exit_status = pitch_insert(0,dz)) < 0)
			return(exit_status);	
		return write_pitch_or_transpos_data(final_length_in_windows,dz);
		break;
	case(P_SINSERT):	
		if((exit_status = pitch_insert(1,dz)) < 0)
			return(exit_status);	
		return write_pitch_or_transpos_data(final_length_in_windows,dz);
		break;
	case(P_PTOSIL):	
		if((exit_status = pitch_to_silence(dz)) < 0)
			return(exit_status);	
		return write_pitch_or_transpos_data(final_length_in_windows,dz);
		break;
	case(P_NTOSIL):	
		if((exit_status = unpitch_to_silence(dz)) < 0)
			return(exit_status);	
		return write_pitch_or_transpos_data(final_length_in_windows,dz);
		break;
	case(P_INTERP):	
		if((exit_status = remove_pitch_zeros(dz)) < 0)
			return(exit_status);	
		return write_pitch_or_transpos_data(dz->wlength,dz);
		break;

	default:
		sprintf(errstr,"Unknown process in outer_pichpich_loop()\n");
		return(PROGRAM_ERROR);
	}
	if((exit_status = trap_junk(final_length_in_windows,dz))<0)
		return(exit_status);
	return write_pitch_or_transpos_data(final_length_in_windows,dz);
}

/************************** SPECPAPPROX ************************/

int specpapprox(int *newlength_of_data,double *lastmidi,dataptr dz)
{
	int exit_status;
	int    avcnt;
	int   n = 0, diff;
	int    is_firstime = TRUE;
	int	   lastpos = 0;
	*newlength_of_data = dz->wlength;
	initrand48();
	if((exit_status = get_pitchapprox_averages(&avcnt,dz))<0)
		return(exit_status);
	if((dz->iparray[PA_CHANGE] = (int *)malloc(avcnt * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for pitch approximation array.\n");
		return(MEMORY_ERROR);
	}
	if((exit_status = get_statechanges(avcnt,PA_SRANG,PA_AVPICH,PA_CHANGE,SEMITONE_INTERVAL,SEMITONE_DOWN,IS_FRQ,dz))<0)
		return(exit_status);
	while(dz->parray[PA_AVPICH][n]<0.0) {
	    if(++n>=avcnt) {
			sprintf(errstr,"No valid pitch-average data found.\n");
			return(DATA_ERROR); /* ??? */
		}
	}
	dz->iparray[PA_CHANGE][n] = 1;
	for(;n<avcnt;n++) {
		if(dz->iparray[PA_CHANGE][n]) {
			if((exit_status = approx_func1(newlength_of_data,&is_firstime,lastmidi,&lastpos,n,dz))<0)
				return(exit_status);
			if(exit_status==FINISHED)
				break;
		}
	}
	if((diff = (dz->wlength-1) - lastpos) > 0 
	&& (exit_status = approx_func2(newlength_of_data,*lastmidi,lastpos,avcnt,dz))<0)
		return(exit_status);
	return(FINISHED);
}

/***************************** SPECPCUT ***************************/

int specpcut(int *final_length_in_windows,dataptr dz)
{
	int m, n;
	int startcut = 0;
	int endcut = dz->wlength;
	if(dz->mode != PCUT_START_ONLY)	    /* is_endtime */
		endcut   = round(dz->param[PC_END]/dz->frametime);
	if(dz->mode != PCUT_END_ONLY)	    /* is_starttime */
		startcut = round(dz->param[PC_STT]/dz->frametime);
	for(m = 0, n = startcut; n < endcut; n++, m++)
		dz->pitches[m] = dz->pitches[n];
	*final_length_in_windows = endcut - startcut;
	return(FINISHED);
}

/************************************* SPECPEXAG ******************************/

int specpexag(dataptr dz)
{
	int exit_status;
	int n;
	double thismidi, maxmidi, minmidi, variance, thispich;
	double maxpitch = FLTERR, minpitch = dz->nyquist, meanpich, maxint, minint;
	dz->time = 0.0f;
	if((exit_status = get_max_and_min_pitches(&maxpitch,&minpitch,dz))<0)
		return(exit_status);
	if((exit_status = hztomidi(&maxmidi,maxpitch))<0)
		return(exit_status);
	if((exit_status = hztomidi(&minmidi,minpitch))<0)
		return(exit_status);
	for(n=0;n<dz->wlength;n++) {
		if(dz->pitches[n]<MINPITCH)
			continue;
		if((exit_status = read_values_from_all_existing_brktables((double)dz->time,dz))<0)
			return(exit_status);
		meanpich = miditohz(dz->param[PEX_MEAN]);
		thispich = dz->pitches[n];
				   /* contour variable specified */
		if(dz->mode != RANGE_ONLY_TO_P && dz->mode != RANGE_ONLY_TO_T) {  
			if((exit_status = hztomidi(&thismidi,thispich))<0)
				return(exit_status);
			maxint = maxmidi - dz->param[PEX_MEAN];
			minint = dz->param[PEX_MEAN] - minmidi;
			if(thismidi >= dz->param[PEX_MEAN])
				variance = ((thismidi - dz->param[PEX_MEAN])/12.0)/maxint;
			else
				variance = ((dz->param[PEX_MEAN] - thismidi)/12.0)/minint;
			if(!flteq(variance,0.0))
			variance = pow(variance,dz->param[PEX_CNTR]);
			if(thismidi>=dz->param[PEX_MEAN])
				thispich = (float)(pow(maxpitch,variance) * pow(meanpich,(1.0-variance)));
			else
				thispich = (float)(pow(minpitch,variance) * pow(meanpich,(1.0-variance)));
		}

		if(dz->mode != CONTOUR_ONLY_TO_P && dz->mode != CONTOUR_ONLY_TO_T) {
			/* range variable specified */
			if((exit_status = hztomidi(&thismidi,thispich))<0)
				return(exit_status);
			thismidi = dz->param[PEX_MEAN] + ((thismidi - dz->param[PEX_MEAN]) 
				* dz->param[PEX_RANG]);
			thispich = (float)miditohz(thismidi);
		}
		if(dz->is_transpos) {	/* transpos output */
			dz->transpos[n] = (float)(thispich/dz->pitches[n]);
			check_transpos(&(dz->transpos[n]),dz);
		} else {
			dz->pitches[n] = (float)thispich;
			check_pitch(&(dz->pitches[n]),dz);
		}
		dz->time = (float)(dz->time + dz->frametime);
	}
	return(FINISHED);
}

/****************************** SPECPFIX ********************************/
 
int specpfix(dataptr dz)
{
	int exit_status;
	if(dz->iparam[PF_ISCUT]    && (exit_status = do_pitch_cut(dz))<0)
		return(exit_status);
	if(dz->iparam[PF_ISFILTER] && (exit_status = do_pitch_filter(dz))<0)
		return(exit_status);
	if(dz->vflag[PF_IS_SMOOTH] && (exit_status = do_pitch_smoothing(dz))<0)
		return(exit_status);
	if(dz->vflag[PF_IS_SMARK])
		dz->pitches[0] = (float)dz->param[PF_SMARK];
	if(dz->vflag[PF_IS_EMARK])
		dz->pitches[dz->wlength-1] = (float)dz->param[PF_EMARK];
	if(dz->vflag[PF_INTERP] && (exit_status = interpolate_pitch(dz->pitches,1,dz))<0)
		return(exit_status);
	return write_exact_samps(dz->pitches,dz->wlength,dz);
}

/************************** SPECPINVERT ************************/

int specpinvert(dataptr dz)
{	 
	int exit_status;
	int n;
	double midimean = 0.0, thismidi, thisint;
	dz->time = 0.0f;
	if(!dz->vflag[PI_IS_MEAN]) {
		if((exit_status = get_midimean(&midimean,dz))<0)
			return(exit_status);
	} else
		midimean = dz->param[PI_MEAN];
	for(n=0;n<dz->wlength;n++) {
		if(dz->pitches[n]<MINPITCH)
			continue;		
		if((exit_status = read_values_from_all_existing_brktables((double)dz->time,dz))<0)
			return(exit_status);
		if((exit_status = hztomidi(&thismidi,dz->pitches[n]))<0)
			return(exit_status);
		if(dz->is_mapping) {
			if((exit_status = interval_mapping(&thisint,thismidi - midimean,dz))<0)
				return(exit_status);
			thismidi = max(dz->param[PI_BOT],min(dz->param[PI_TOP],midimean + thisint));
		} else
			thismidi = min(dz->param[PI_TOP],max(dz->param[PI_BOT],((2.0 * midimean) - thismidi)));
		if(dz->is_transpos) {
			dz->transpos[n] = (float)(miditohz(thismidi)/dz->pitches[n]);
			check_transpos(&(dz->transpos[n]),dz);
		} else  {
			dz->pitches[n] = (float)miditohz(thismidi);
			check_pitch(&(dz->pitches[n]),dz);
		}
		dz->time = (float)(dz->time + dz->frametime);
	}
	return(FINISHED);	
}

/************************************** SPECPQUANTISE **************************/

int specpquantise(dataptr dz)
{
	int exit_status;
	int n;
	int got;
	double *p, thismidi;
	double *pend = dz->parray[PQ_QSET] + dz->itemcnt;
	for(n=0;n<dz->wlength;n++) {
		if(dz->pitches[n]<MINPITCH)
			continue;
		got = 0;
		p = dz->parray[PQ_QSET]; 
		if((exit_status = hztomidi(&thismidi,dz->pitches[n]))<0)
			return(exit_status);
		if(*p >= thismidi) {
			if((exit_status = set_pval(*p,n,dz))<0)
				return(exit_status);
			continue;
		}
		while(*p < thismidi) {
			if(++p >= pend) {
				p--;
				if((exit_status = set_pval(*p,n,dz))<0)
					return(exit_status);
				got = 1;
				break;
			}
		}
		if(got)
			continue;
		if((*p - thismidi) > (thismidi - *(p-1))) {
			if((exit_status = set_pval(*(p-1),n,dz))<0)
				return(exit_status);
		} else {
			if((exit_status = set_pval(*p,n,dz))<0)
				return(exit_status);
		}
	}
	return(FINISHED);	
}

/***************************************** SPECPRAND *********************/

int specprand(dataptr dz)
{
	int exit_status;
	int n = 0, lastn, m, thiswstep, diff, z;
	double thisintv, ttime = 0.0, lastmidi, thismidi, mididiff, midistep;
	initrand48();
	while(dz->pitches[n]<MINPITCH) {
		if(++n>=dz->wlength) {
			sprintf(errstr,"No valid pitchdata found in pitch file.\n");
			return(DATA_ERROR);
		}
	}
	if((exit_status = hztomidi(&lastmidi,dz->pitches[n]))<0)
		return(exit_status);
	ttime = dz->frametime * (double)n;
	if((exit_status = read_values_from_all_existing_brktables(ttime,dz))<0)
		return(exit_status);
	if((exit_status = get_rand_interval(&thisintv,dz))<0)
		return(exit_status);
	lastmidi += thisintv;
	if((exit_status = set_pval(lastmidi,n,dz))<0)
		return(exit_status);
	lastn = n;
	do {
		thiswstep = round(max(dz->frametime,drand48() * dz->param[PR_TSTEP])/dz->frametime);
		if((n += thiswstep) >= dz->wlength) {
		 	sprintf(errstr,"Too much unpitched data in file to proceed this time. But try again.\n");
			return(DATA_ERROR);
		}
		ttime = dz->frametime * (double)n;
		if(dz->brksize[PR_TSTEP]) {
			if((exit_status = read_value_from_brktable(ttime,PR_TSTEP,dz))<0)
				return(exit_status);
		}
	} while(dz->pitches[n]<MINPITCH);

	while(n<dz->wlength) {
		if(dz->brksize[PR_MXINT]) {
			if((exit_status = read_value_from_brktable(ttime,PR_MXINT,dz))<0)
				return(exit_status);
		}
		if((exit_status = get_rand_interval(&thisintv,dz))<0)
			return(exit_status);
		if((exit_status = hztomidi(&thismidi,dz->pitches[n]))<0)
			return(exit_status);
		thismidi += thisintv;
		mididiff  = thismidi - lastmidi;
		diff	  = n - lastn;
		midistep  = mididiff/(double)diff; 
		for(m = lastn+1;m<=n;m++) {			/* Interp pitch between randomised points */
			if(dz->pitches[m] < MINPITCH)
				continue;
			lastmidi  += midistep;
			if((exit_status = set_pval(lastmidi,m,dz))<0)
				return(exit_status);
		}
		lastmidi  = thismidi;
		lastn = n;
		do {
			if(dz->brksize[PR_TSTEP]) {
				if((exit_status = read_value_from_brktable(ttime,PR_TSTEP,dz))<0)
					return(exit_status);
			}
			thiswstep = round(max(dz->frametime,drand48() * dz->param[PR_TSTEP])/dz->frametime);
			if((n+=thiswstep) > dz->wlength)
				break;
			ttime = dz->frametime * (double)n;
		} while(dz->pitches[n]<MINPITCH);
	}
	if(lastn < dz->wlength-1) {
		z = dz->wlength - 1;
		while(dz->pitches[z]<MINPITCH) {
			if(--z<=lastn)
				return(FINISHED);
		}
		n = z;
		thisintv   = ((drand48() * 2.0) - 1.0) * dz->param[PR_MXINT];
		if((exit_status = hztomidi(&thismidi,dz->pitches[n]))<0)
			return(exit_status);
		thismidi  += thisintv;
		mididiff   = thismidi - lastmidi;
		if((diff = n - lastn)<2)
			return(FINISHED);
		midistep   = mididiff/(double)diff; 
		for(m = lastn+1;m<=n;m++) {
			if(dz->pitches[n]<MINPITCH)
				continue;
			lastmidi  += midistep;
			if((exit_status = set_pval(lastmidi,m,dz))<0)
				return(exit_status);
		}
	}
	return(FINISHED);	
}

/*************************************** SPECPSMOOTH ***********************************/

int	specpsmooth(dataptr dz)
{
	int exit_status;
	double ttime = 0.0, maxpitch = FLTERR, minpitch = dz->nyquist, 
		maxmidi, minmidi, lastmidi;
	double meanpich, maxint, minint, startmidi, endmidi, thismidi, mdiff, step;
	int lastmaxpos = 0, n=0, m, z, interpfact, lastn;
	if((exit_status = get_max_and_min_pitches(&maxpitch,&minpitch,dz))<0)
		return(exit_status);
	if((exit_status = hztomidi(&maxmidi,maxpitch))<0)
		return(exit_status);
	if((exit_status = hztomidi(&minmidi,minpitch))<0)
		return(exit_status);
	n = 0;
	while(dz->pitches[n]<MINPITCH) {
		if(++n >= dz->wlength) {
			sprintf(errstr,"No valid data in pitchfile\n");
			return(DATA_ERROR);
		}
	}
	if(dz->is_transpos)
		dz->transpos[n] = 1.0f;
	if((exit_status = hztomidi(&lastmidi,dz->pitches[n]))<0)
		return(exit_status);
	ttime    = (double)n * dz->frametime;
	if((exit_status = read_values_from_all_existing_brktables(ttime,dz))<0)
		return(exit_status);
	interpfact = round(dz->param[PS_TFRAME]/dz->frametime);
	lastn = n;
	do {
		if((n += interpfact) >=dz->wlength) {
			sprintf(errstr,"Too much unpitched data in file: cannot proceed. Try different interpfact(s).\n");
			return(DATA_ERROR);
		}
		ttime = (double)n * dz->frametime;
		if(dz->brksize[PS_TFRAME]) {
			if((exit_status = read_value_from_brktable(ttime,PS_TFRAME,dz))<0)
				return(exit_status);
			interpfact = round(dz->param[PS_TFRAME]/dz->frametime);
		}
	} while(dz->pitches[n]<MINPITCH);
	if(dz->vflag[PS_MEANP]) {
		meanpich = miditohz(dz->param[PS_MEAN]);
		maxint   = maxmidi - dz->param[PS_MEAN];
		minint   = dz->param[PS_MEAN] - minmidi;
		while(n<dz->wlength) {
			if((exit_status = peak_interp(n,lastn,&lastmaxpos,meanpich,minint,maxint,&lastmidi,dz))<0)
				return(exit_status);
			if(dz->brksize[PS_TFRAME]) {
				if((exit_status = read_value_from_brktable(ttime,PS_TFRAME,dz))<0)
					return(exit_status);
				interpfact = round(dz->param[PS_TFRAME]/dz->frametime);
			}
			if(dz->brksize[PS_MEAN]) {
				if((exit_status = read_value_from_brktable(ttime,PS_MEAN,dz))<0)
					return(exit_status);
				meanpich = miditohz(dz->param[PS_MEAN]);
				maxint = maxmidi - dz->param[PS_MEAN];
				minint = dz->param[PS_MEAN] - minmidi;
			}
			lastn = n;
			do{
				if((n += interpfact) >= dz->wlength)
					break;
				ttime = (double)n * dz->frametime;
				if(dz->brksize[PS_TFRAME]) {
					if((exit_status = read_value_from_brktable(ttime,PS_TFRAME,dz))<0)
						return(exit_status);
					interpfact = round(dz->param[PS_TFRAME]/dz->frametime);
				}
			} while(dz->pitches[n]<MINPITCH);
		}
		n = lastmaxpos;
	} else {
		while(n<dz->wlength) {
			z = n - lastn;
			if((exit_status = hztomidi(&startmidi,dz->pitches[lastn]))<0)
				return(exit_status);
			if((exit_status = hztomidi(&endmidi,dz->pitches[n]))<0)
				return(exit_status);
			mdiff = endmidi - startmidi;
			step = mdiff/(double)z;
			for(m=1;m<=z;m++) {
				if(dz->pitches[m+lastn]<MINPITCH)
					continue;
				thismidi = startmidi + (step * (double)m);
				if((exit_status = set_pval(thismidi,m+lastn,dz))<0)
					return(exit_status);
			}
			lastn = n;
			do{
				if((n += interpfact) >=dz->wlength)
					break;
				ttime = (float)((double)n * dz->frametime);
				if(dz->brksize[PS_TFRAME]) {
					if((exit_status = read_value_from_brktable(ttime,PS_TFRAME,dz))<0)
						return(exit_status);
					interpfact = round(dz->param[PS_TFRAME]/dz->frametime);
				}
			} while(dz->pitches[n]<MINPITCH);
		}
		n = lastn;
		if((exit_status = hztomidi(&lastmidi,dz->pitches[n]))<0)
			return(exit_status);
	}
	if(n < dz->wlength-1)
		if((exit_status = do_tail(n,lastmidi,dz))<0)
			return(exit_status);
	return(FINISHED);
}

/***************************** SPECPTRANSPOSE *****************/

int	specptranspose(dataptr dz)
{	
	int n;
	for(n=0;n<dz->wlength;n++) {
		if(dz->pitches[n]<MINPITCH)
			continue;
		dz->pitches[n] = (float)(dz->pitches[n] * dz->param[PT_TVAL]);
		check_pitch(&(dz->pitches[n]),dz);
	}
	return(FINISHED);
}

/***************************** SPECPVIB *****************/

int specpvib(dataptr dz)
{
	int exit_status;
	double indexf = 0.0, intvl, indexstep;
	int index = 0;
	int n;
	dz->time = 0.0f;
	if((exit_status = read_values_from_all_existing_brktables((double)dz->time,dz))<0)
		return(exit_status);
	indexstep = dz->param[PV_FRQ] * dz->frametime * (double)P_TABSIZE;
	for(n=0;n<dz->wlength;n++) {
		if(dz->pitches[n]<MINPITCH)
			continue;
		intvl = dz->parray[PV_SIN][index] * dz->param[PV_RANG];
		intvl = pow(SEMITONE_INTERVAL,intvl);
		if(dz->is_transpos) {
			dz->transpos[n] = (float)intvl;
			check_transpos(&(dz->transpos[n]),dz);
		} else {
			dz->pitches[n] = (float)(dz->pitches[n] * intvl);
			check_pitch(&(dz->pitches[n]),dz);
		}
		indexf += indexstep;
		index   = round(indexf) % P_TABSIZE;
		dz->time = (float)(dz->time + dz->frametime);
		if(dz->brksize[PV_FRQ]) {
			if((exit_status = read_value_from_brktable((double)dz->time,PV_FRQ,dz))<0)
				return(exit_status);
			indexstep = dz->param[PV_FRQ] * dz->frametime * (double)P_TABSIZE;
		}
		if(dz->brksize[PV_RANG]) {
			if((exit_status = read_value_from_brktable((double)dz->time,PV_RANG,dz))<0)
				return(exit_status);
		}
	}
	return(FINISHED);
}

/*************************** SPECREPITCH ********************/

int specrepitch(dataptr dz)
{
	int exit_status;
	int n, brklen;
	float frametime;
	if(dz->wlength == 0) {	/* brkpnt only data entered */
		dz->wlength = round(dz->duration/DEFAULT_FRAMETIME);
		frametime   = DEFAULT_FRAMETIME;
	} else
		frametime   = dz->frametime;
	switch(dz->mode) {
	case(PPT): case(PPT_TO_BRK):	/* PPT */	
		if((dz->transpos = (float *)malloc(dz->wlength * sizeof(float)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for transpositions array.\n");
			return(MEMORY_ERROR);
		}
		for(n=0;n<dz->wlength;n++) {
			if(dz->pitches[n]<MINPITCH || dz->pitches2[n]<MINPITCH)
				dz->transpos[n] = 1.0f;
			else
				dz->transpos[n] = (float)(dz->pitches2[n]/dz->pitches[n]);
			check_transpos(&(dz->transpos[n]),dz);
		}
		break;
	case(PTP): case(PTP_TO_BRK):	/* PTP */
		for(n=0;n<dz->wlength;n++) {
			if(dz->pitches[n] < MINPITCH)
				continue;
			dz->pitches[n]  = (float)(dz->pitches[n] * dz->transpos[n]);	
			check_pitch(&(dz->pitches[n]),dz);
		}
		break;
	case(TTT): case(TTT_TO_BRK):	/* TTT */
		for(n=0;n<dz->wlength;n++) {
			dz->transpos[n] = (float)(dz->transpos[n] * dz->transpos2[n]);	
			check_transpos(&(dz->transpos[n]),dz);
		}
		break;
	default:
		sprintf(errstr,"unknown case in specrepitch()\n");
		return(PROGRAM_ERROR);
	}
	switch(dz->mode) {
	case(PTP):		  /* out P-bin */
		if((exit_status = write_samps(dz->pitches,dz->wlength,dz))<0)
			return(exit_status);
		break;
	case(PPT): 
	case(TTT):		  /* out T-bin */
		dz->outfiletype = TRANSPOS_OUT; 
		if((exit_status = write_samps(dz->transpos,dz->wlength,dz))<0)
			return(exit_status);
		dz->is_transpos = TRUE;
		break;
	case(PTP_TO_BRK): /* P-brk out */ 
		if((dz->parray[RP_TBRK] 
			= (double *)malloc(dz->wlength * sizeof(double) * 2))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for pitch brkpnt array.\n");
			return(MEMORY_ERROR);
		}
		if((exit_status = interpolate_pitch(dz->pitches,0,dz))<0)
			return(exit_status);
		if((exit_status = convert_pch_or_transpos_data_to_brkpnttable(
			&brklen,dz->pitches,frametime,RP_TBRK,dz))<0)
			return(exit_status);
		if((exit_status = write_brkfile(dz->fp,brklen,RP_TBRK,dz))<0)
			return(exit_status);
		break;
	case(PPT_TO_BRK): 
	case(TTT_TO_BRK): /* T_brk out */
		if((dz->parray[RP_TBRK] 
			= (double *)malloc(dz->wlength * sizeof(double) * 2))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for transposition brkpnt array.\n");
			return(MEMORY_ERROR);
		}
		if((exit_status = convert_pch_or_transpos_data_to_brkpnttable(&brklen,dz->transpos,frametime,RP_TBRK,dz))<0)
			return(exit_status);
		if((exit_status = write_brkfile(dz->fp,brklen,RP_TBRK,dz))<0)
			return(exit_status);
		break;
	}
	return(FINISHED);
}

/******************************* DO_PITCH_CUT ******************************/

int do_pitch_cut(dataptr dz)
{   int n; 
    for(n=dz->iparam[PF_SCUTW];n<dz->iparam[PF_ECUTW];n++)
        dz->pitches[n] = (float)NOT_PITCH;
	return(FINISHED);
}

/************************* DO_PITCH_SMOOTHING ***************************/

int do_pitch_smoothing(dataptr dz)
{
	int exit_status;
	int z;
	int first_valid_pitch, n, wlength_less_2 = dz->wlength - 2;
	double *slopechange;
	if((slopechange = (double *)malloc(dz->wlength * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for slopechange array.\n");
		return(MEMORY_ERROR);
	}
	for(z=0;z<dz->iparam[PF_SMOOTH];z++) {
		if((exit_status = skip_to_first_pitch(&first_valid_pitch,dz))<0)
			return(exit_status);
		if((exit_status = calc_slopechanges(first_valid_pitch,slopechange,dz))<0)
			return(exit_status);
		for(n=1;n<wlength_less_2;n++) {
			if(fabs(slopechange[n]) > GLARG) {
				if(dz->pitches[n+1]<0.0)
					continue;
				if((exit_status = is_start_of_glitch(n,slopechange,dz))<0)
					return(exit_status);
				if(exit_status==TRUE)
					continue;   /* WILL BE SMOOTHED OUT AT NEXTSTEP */
				if(dz->pitches[n-1]<MINPITCH) {		/* ONSET SMOOTH */
					if((exit_status = do_onset_smooth(n,slopechange,dz))<0)
						return(exit_status);
					continue;
				}
				if(n>=2 && dz->pitches[n-2]<MINPITCH) {	/* DOUBLE ONSET SMOOTH */
					if((exit_status = do_double_onset_smooth(n,slopechange,dz))<0)
						return(exit_status);
					continue;
				}					/* NORMAL SMOOTH */
				if((exit_status = do_smooth(n,slopechange,dz))<0)
					return(exit_status);
			}
		}
	}
	free(slopechange);
	return(FINISHED);
}

/****************************** DO_SMOOTH **************************
 *
 * Including checking for end of a pitched segment.
 */

int do_smooth(int pitchno,double *slopechange,dataptr dz)
{
	double val, val0, val1;
	if(slopechange[pitchno+1] > GLARG  && pitchno>=1) {
		if(pitchno+2 < dz->wlength && dz->pitches[pitchno+2] < 0.0) {
			val0 = log10(dz->pitches[pitchno-1]);		/* TAIL */
			val1 = log10(dz->pitches[pitchno]);
			val = val1 - val0;
			dz->pitches[pitchno+1] = (float)pow((double)10,(val1 + val));
			slopechange[pitchno]   = 0.0; /* CORRECT SLOPECHANGE APPROPRIATELY */
			slopechange[pitchno+1] = 0.0; /* AVOID SPURIOUS SMOOTH AT END_OF_TAIL PITCH */
			return(FINISHED);
		} else {
			if(pitchno+3 < dz->wlength && dz->pitches[pitchno+3] < MINPITCH) {
				val0 = log10(dz->pitches[pitchno-1]);	/* DOUBLE TAIL */
				val1 = log10(dz->pitches[pitchno]);
				val  = val1 - val0;
				dz->pitches[pitchno+1] = (float)pow((double)10,(val1 + val));
				val += val;
				dz->pitches[pitchno+2] = (float)pow((double)10,(val1 + val));
				slopechange[pitchno]   = 0.0;	/* CORRECT SLOPECHANGES APPROPRIATELY */
				slopechange[pitchno+1] = 0.0;
				slopechange[pitchno+2] = 0.0;	/* AVOID SPURIOUS SMOOTH AT END_OF_TAIL PITCH */
				return(FINISHED);
			}
		}
	}							/* NORMAL */
	val0 = log10(dz->pitches[pitchno-1]);	
	val1 = log10(dz->pitches[pitchno+1]);
	val  = (val0 + val1)/2.0;
	dz->pitches[pitchno] = (float)pow((double)10,val);
	slopechange[pitchno] = 0.0;		   		/* CORRECT SLOPECHANGE APPROPRIATELY */
	return(FINISHED);
}

/************************ SKIP_TO_FIRST_PITCH ***********************/

int skip_to_first_pitch(int *first_pitch,dataptr dz)
{
	*first_pitch = 0;
	while(dz->pitches[*first_pitch] < MINPITCH) {
		if(++(*first_pitch) >= dz->wlength) {
			sprintf(errstr,"No pitch data found.\n");
			return(DATA_ERROR);
		}
	}
	if(++(*first_pitch) >= dz->wlength) {
		sprintf(errstr,"No pitch data found.\n");
		return(DATA_ERROR);
	}
	return(FINISHED);
}

/************************* CALC_SLOPECHANGES ***********************/

int calc_slopechanges(int first_valid_pitch,double *slopechange,dataptr dz)
{
	int n, k, wlength_less_1 = dz->wlength - 1;
	int OK = 1;
	double preslope = 0.0, postslope;
	for(n=0;n<dz->wlength;n++)
		slopechange[n] = 0.0;	
	for(n = first_valid_pitch;n<wlength_less_1;n++) {
		k = 1;
		while(dz->pitches[n+k] < 0.0) {
			k++;
			if(k+n >= dz->wlength) {
				OK = 0;
				break;
			}
			if(!OK)
				break;
		}
		if(!OK)
			break;
		postslope = (log10(dz->pitches[n+k]) - log10(dz->pitches[n]))/(double)k;
		slopechange[n] = postslope - preslope;
		n += k - 1;
		preslope = postslope;
	}
	return(FINISHED);
}

/********************** IS_START_OF_GLITCH **********************
 *
 * WITHOUT THOROUGH CHECK worried that logs may get 0 or -ve vals..
 */

int is_start_of_glitch(int pitchno,double *slopechange,dataptr dz)
{
	double val, val0, val1;
	if(dz->vflag[PF_TWOW]) {
		if(pitchno+3 < dz->wlength-1
		&& fabs(slopechange[pitchno+1]) > GLARG
		&& fabs(slopechange[pitchno+2]) > GLARG
		&& fabs(slopechange[pitchno+3]) > GLARG
		&& ((slopechange[pitchno] > 0.0 
			&& slopechange[pitchno+1] < 0.0 
			&& slopechange[pitchno+2] < 0.0 
			&& slopechange[pitchno+3] > 0.0)  	/* +--+ */
		||  (slopechange[pitchno] < 0.0 
			&& slopechange[pitchno+1] > 0.0 
			&& slopechange[pitchno+2] < 0.0 
			&& slopechange[pitchno+3] > 0.0)	/* -++- */
		   )
		){
			val0 = log10(dz->pitches[pitchno]);
			val1 = log10(dz->pitches[pitchno+3]);
			val  = (val1 - val0)/3.0;	/* 2 WINDOW GLITCH */
			val0 = log10(dz->pitches[pitchno] + val);
			dz->pitches[pitchno+1] = (float)pow((double)10,val0);
			val += val;
			val1 = log10(dz->pitches[pitchno] + val);
			dz->pitches[pitchno+2] = (float)pow((double)10,val1);
			slopechange[pitchno+1] = 0.0;
			slopechange[pitchno+2] = 0.0;
			return(TRUE);
		}
	}
	if(fabs(slopechange[pitchno+1]) > GLARG 
	&& fabs(slopechange[pitchno+2]) > GLARG
	&& ((slopechange[pitchno] > 0.0 
		&& slopechange[pitchno+1] < 0.0 
		&& slopechange[pitchno+2] > 0.0) 	/* +-+ */
	||  (slopechange[pitchno] < 0.0 
		&& slopechange[pitchno+1] > 0.0 
		&& slopechange[pitchno+2] < 0.0)	/* -+- */
	   )
	)
		return(TRUE);
	return(FALSE);
}

/*********************** DO_ONSET_SMOOTH ********************/

int do_onset_smooth(int pitchno, double *slopechange,dataptr dz)
{
	double val, val0, val1;
	if(dz->pitches[pitchno+2] < MINPITCH)
		return(FINISHED);
	val0 = log10(dz->pitches[pitchno+1]);
	val1 = log10(dz->pitches[pitchno+2]);
	val  = (2.0 * val0) - val1;
	dz->pitches[pitchno] = (float)pow((double)10,val);
	slopechange[pitchno+1] = 0.0; 	/* CHANGE OF SLOPE HENCE BECOMES ZERO AT NEXT PITCH */
	return(FINISHED);
}			      	/* THIS ALSO PREVENTS double_onset BEING CALLED SPURIOUSLY */

/************************ DO_DOUBLE_ONSET_SMOOTH ********************
 *
 *    NORMAL DOUBLE ONSET GLITCH	  SITUATION TO BE AVOIDED
 *						 .
 *						  .
 *					           .
 *	        postslope			    .
 *	......  X---X---X           X       
 *	      /					   /  \postslope
 *	     /					  /    \
 *	X---X            	 X---X      X---X
 *      |				     |
 *      |				     |
 *  0...|				 0...|
 *
 *					     					 X
 *					     					 |\
 *  reverse predict from postslope, OK	     | \reverse predict from postslope
 *	X---X---X---X---X		     			 |	\ gives spurious glitch
 *	|				     					 |	 X
 *	|				     					 |	  \
 *	|				     					 |     \
 * 	|				     					 |	    \
 *	|			             				 |       X       
 *  0...|       			     			 |	      \
 *	     				     				 |	       \
 *	                         	     		 |          X---X
 *       				     				 |
 *       				     				 |
 *  					 				 0...|
 *	                    	           ,X       
 *	       					          /   \
 *	      			interp instead  ,X     \
 *		                   	       /        \
 *		                   	     X           X---X
 *       				     	 |
 *       				      	 |
 *  	 				 	 0...|
 *
 *		and at next pass, there'll probably be a 2nd interp thus..
 *	                    	                           
 *	      					    ,X.
 *		                   	  /    'X.
 *		                   	 X        ' X---X
 *       				     |
 *       				     |
 *  	 				 0...|
 *
 *
 */

int do_double_onset_smooth(int pitchno,double *slopechange,dataptr dz)
{
	double val, val0, val1, preslope, postslope, pn0 ,pn1, pn2;
	if(dz->pitches[pitchno+2] <  MINPITCH)
		return(FINISHED);
	pn0 = log10(dz->pitches[pitchno]);
	pn1 = log10(dz->pitches[pitchno+1]);
	pn2 = log10(dz->pitches[pitchno+2]);
	preslope  = fabs(pn1 - pn0);
	postslope = fabs(pn2 - pn1);
	if(postslope > preslope * SLOPE_FUDGE) {	/* 1 */
		val0 = log10(dz->pitches[pitchno-1]);	/* IF SLOPE AHEAD LOOKS TOO BIG  */
		val1 = pn1; 		   			/* DON'T RISK USING IT TO GET VALS HERE. */
		val  = (val0 + val1)/2.0;		/* INTERP INSTEAD */
		dz->pitches[pitchno] = (float)pow((double)10,val);
		slopechange[pitchno] = 0.0;		/* CORRECT SLOPECHANGE APPROPRIATELY */
	} else {
		val0 = pn1;		   				/* OTHERWISE BASE VALS ON SLOPE AHEAD */
		val1 = pn2;
		val  = (2.0 * val0) - val1;
		dz->pitches[pitchno] = (float)pow((double)10,val);
		val  = (3.0 * val0) - (2.0 * val1);
		dz->pitches[pitchno-1] = (float)pow((double)10,val);
		slopechange[pitchno]   = 0.0;
		slopechange[pitchno+1] = 0.0;		/* CORRECT SLOPECHANGES APPROPRIATELY */
	}
	return(FINISHED);
}

/********************** GET_MAX_AND_MIN_PITCHES **********************/

int get_max_and_min_pitches(double *maxpitch,double *minpitch,dataptr dz)
{
	int n;
	for(n=0;n<dz->wlength;n++) {
		if(dz->pitches[n] > MINPITCH) {
			if(dz->pitches[n] > *maxpitch)
				*maxpitch = dz->pitches[n];
			if(dz->pitches[n] < *minpitch)
				*minpitch = dz->pitches[n];
		}
	}
	return(FINISHED);
}

/***************************** WRITE_REMAINING_PITCH_OR_TRANSPOS_DATA ******************************/

int write_remaining_pitch_or_transpos_data(int final_length_in_windows,dataptr dz)
{
	int exit_status;
	if(final_length_in_windows > 0) {
		if(dz->is_transpos) {
			if((exit_status = write_samps(dz->transpos,final_length_in_windows,dz))<0)
				return(exit_status);
		}else {
			if((exit_status = write_samps(dz->pitches,final_length_in_windows,dz))<0)
				return(exit_status);
		}
	}
	return(FINISHED);
}

/***************************** GET_MIDIMEAN ******************************/

int get_midimean(double *midimean,dataptr dz)
{
	int exit_status;
	int n;
	double  val;
	*midimean = 0.0;
	for(n=0;n<dz->wlength;n++) {
		if(dz->pitches[n]<MINPITCH)
			continue;
		if((exit_status = hztomidi(&val,dz->pitches[n]))<0)
			return(exit_status);
		*midimean += val;
	}
	*midimean /= (double)dz->wlength;	
	return(FINISHED);
}

/***************************** SET_PVAL ******************************/

int set_pval(double midivalue,int n,dataptr dz)
{
	if(dz->is_transpos) {
		dz->transpos[n] = (float)(miditohz(midivalue)/dz->pitches[n]);
		check_transpos(&(dz->transpos[n]),dz);
	} else {
		dz->pitches[n] = (float)miditohz(midivalue);
		check_pitch(&(dz->pitches[n]),dz);
	}
	return(FINISHED);
}

/***************************************** DO_TAIL ************************/

int do_tail(int n,double lastmidi,dataptr dz)
{
	int exit_status;
	double thispich, thismidi, endmidi, mdiff, step;
	int   diff, m, z;
	if(dz->vflag[PS_HOLD]) {
		thispich = dz->pitches[n];
		n++;
		while(n < dz->wlength) {
			if(dz->pitches[n]<MINPITCH) {
				n++;
				continue;
			}
			if(dz->is_transpos) {
				dz->transpos[n] = (float)(thispich/dz->pitches[n]);
				check_transpos(&(dz->transpos[n]),dz);
			} else {
				dz->pitches[n] = (float)thispich;
				check_pitch(&(dz->pitches[n]),dz);
			}
			n++;
		}
	} else {
		z = dz->wlength-1;
		while(dz->pitches[z]<MINPITCH) {
			if(--z==n)
				return(FINISHED);
		}
		if((exit_status = hztomidi(&endmidi,dz->pitches[z]))<0)
			return(exit_status);
		mdiff   = endmidi - lastmidi;
		if((diff = z - n)<2)
			return(FINISHED);
		step = mdiff/(double)diff;
		n++;
		for(m=1;n <= z;n++,m++) {
			if(dz->pitches[n]<MINPITCH)
				continue;
			thismidi = lastmidi + (step * (double)m);
			if((exit_status = set_pval(thismidi,n,dz))<0)
				return(exit_status);
		}
	}
	return(FINISHED);
}

/************************** GET_PITCHAPPROX_AVERAGES ************************/

int get_pitchapprox_averages(int *avcnt,dataptr dz)
{
	int exit_status;
	int OK = 1, n = 0, m, cnt;
	double val;
	*avcnt = 0;
	if((dz->parray[PA_AVPICH] = (double *)malloc(dz->wlength * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for averaging array.\n");
		return(MEMORY_ERROR);
	}
	while(OK) {
		dz->parray[PA_AVPICH][*avcnt] = 0.0;
		m = 0;
		cnt = 0;
		while(m < BLOKCNT) {
			if(dz->pitches[n]<MINPITCH) {
				m++;
				if(++n >= dz->wlength) {
					OK = 0;
					break;
				}
				continue;
			}
			if((exit_status = hztomidi(&val,dz->pitches[n]))<0)
				return(exit_status);
			dz->parray[PA_AVPICH][*avcnt] += val;
			cnt++;
			m++;
			if(++n >= dz->wlength) {
				OK = 0;
				break;
			}
		}
		if(cnt==0)
			dz->parray[PA_AVPICH][*avcnt] = -1.0f;
		else {
		    dz->parray[PA_AVPICH][*avcnt] /= (double)cnt;
		    dz->parray[PA_AVPICH][*avcnt]  = miditohz(dz->parray[PA_AVPICH][*avcnt]);
		}
		(*avcnt)++;
	}
	return(FINISHED);
}

/********************************** APPROX_FUNC1 **********************************/

int approx_func1(int *newlength_of_data,int *is_firsttime,
					double *lastmidi,int *lastpos,int n,dataptr dz)
{
	int q, m, thispos, diff;
	int exit_status = CONTINUE;
	double ttime, thismidi, wiggle, this_prange, this_trange, mididiff, midistep;
	int here = min(dz->wlength-1,n * BLOKCNT); /* just being ultra careful !! */
	for(q=here;q<here+BLOKCNT;q++) {
		if(q>=dz->wlength)
			break;
		if(dz->pitches[q] > MINPITCH)
			break;
	}
	if(q>=here+BLOKCNT || q>=dz->wlength)	   /* No valid pitchdata here !! */
		return(CONTINUE);
	here = q;
	ttime = dz->frametime * (double)here;
	if((exit_status = hztomidi(&thismidi,dz->pitches[here]))<0)
		return(exit_status);
	if(dz->brksize[PA_PRANG])  {	/* RWD need curly */
		if((exit_status = read_value_from_brktable(ttime,PA_PRANG,dz))<0)
			return(exit_status);
	}	
	wiggle = (drand48() * 2.0) -1.0;	/* random  +-  */
	this_prange = wiggle * dz->param[PA_PRANG];
	thismidi += this_prange;
	if(*is_firsttime) {
		if((exit_status = set_pval(thismidi,q,dz))<0)
			return(exit_status);
		*lastpos  = q;
		*lastmidi = thismidi;
		*is_firsttime = FALSE;
	} else {
		wiggle = (drand48() * 2.0) -1.0;
		if(dz->brksize[PA_TRANG])  {	/* RWD need curly */
			if((exit_status = read_value_from_brktable(ttime,PA_TRANG,dz))<0)
				return(exit_status);
		}	
		this_trange = dz->param[PA_TRANG] * wiggle;
		ttime  += this_trange;
		thispos = round(ttime/dz->frametime);
		thispos = max(0L,thispos);
		if((diff = thispos - *lastpos)>0) {
			if(thispos > (*newlength_of_data)-1) {
				if(dz->is_transpos) {
					thispos = (*newlength_of_data)-1;
					exit_status = FINISHED;
				} else {
					*newlength_of_data = thispos + 1;
					if((dz->pitches  = (float *)realloc((char *)dz->pitches,
						(*newlength_of_data) * sizeof(float)))==NULL) {
						sprintf(errstr,"INSUFFICIENT MEMORY to reallocate pitch array.\n");
						return(MEMORY_ERROR);
					}
				}
			}
			mididiff = thismidi - *lastmidi;
			midistep = mididiff/(double)diff;
			for(m=1,q= *lastpos + 1;m<=diff;m++,q++) {
				if(q < dz->wlength && dz->pitches[q]<MINPITCH)
					continue;
				thismidi = *lastmidi + (midistep * (double)m);
				if((exit_status = set_pval(thismidi,q,dz))<0)
					return(exit_status);
			}
			*lastmidi = thismidi;
			*lastpos  = thispos;
		}
	}
 	return(exit_status);
}

/********************************** APPROX_FUNC2 **********************************/

int approx_func2(int *newlength_of_data,double lastmidi,int lastpos,int avcnt,dataptr dz)
{
	int exit_status;
	int q, n, thispos, diff = 0;
	double thismidi,ttime, wiggle, this_prange, this_trange;
	double mididiff, midistep;
	q = avcnt-1;
	while(dz->parray[PA_AVPICH][q]<0.0) {
		if(--q<=lastpos)
			return(FINISHED);
	}	
	if((exit_status = hztomidi(&thismidi,dz->parray[PA_AVPICH][q]))<0)
		return(exit_status);
	ttime = (q * BLOKCNT) * dz->frametime;
	if(dz->brksize[PA_PRANG]) {		//RWD need curly
		if((exit_status = read_value_from_brktable(ttime,PA_PRANG,dz))<0)
			return(exit_status);
	}	
	wiggle = (drand48() * 2.0) -1.0;	/* random  +-  */
	this_prange = wiggle * dz->param[PA_PRANG];
	thismidi += this_prange;
	if(dz->is_transpos)
		thispos = dz->wlength-1;
	else {
		wiggle = (drand48() * 2.0) -1.0;	/* random  +-  */
		if(dz->brksize[PA_TRANG]) {		/* RWD need curly */
			if((exit_status = read_value_from_brktable(ttime,PA_TRANG,dz))<0)
				return(exit_status);
		}	
		this_trange = dz->param[PA_TRANG] * wiggle;
		ttime  += this_trange;
		thispos = round(ttime/dz->frametime);
		thispos = max(0L,thispos);
		if((diff = thispos - lastpos)>0 && thispos > *newlength_of_data-1) {
			*newlength_of_data = thispos + 1;
			if((dz->pitches  = 
			(float *)realloc((char *)dz->pitches,*newlength_of_data * sizeof(float)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY to reallocate pitch array.\n");
				return(MEMORY_ERROR);
			}
		}
	}
	if(diff>0) {
		mididiff = thismidi - lastmidi;
		midistep = mididiff/(double)diff;
		for(n=1,q=lastpos+1;n<=diff;n++,q++) {
			if(q < dz->wlength && dz->pitches[q]<MINPITCH)
				continue;
			thismidi = lastmidi + (midistep * (double)n);
			if((exit_status = set_pval(thismidi,q,dz))<0)
				return(exit_status);
		}
	} else
		*newlength_of_data = lastpos + 1;
	return(FINISHED);
}

/************************** GET_RAND_INTERVAL ************************/

int get_rand_interval(double *thisintv,dataptr dz)
{
	double wiggle = ((drand48() * 2.0) - 1.0);
	if(dz->vflag[PR_IS_SLEW]) {
		if(wiggle < dz->param[PR_SLEW] 
			|| (dz->iparam[PR_NEGATIV_SLEW]==TRUE 
			&& (wiggle > dz->param[PR_SLEW])))
			wiggle = -wiggle;
	}
	*thisintv = dz->param[PR_MXINT] * wiggle;
	return(FINISHED);
}
		
/************************** INTERVAL_MAPPING ************************
 *
 * Approximate input interval to nearest value in LHS column of input map.
 * Find variance from that value.										    
 * Return corresponding value in RHS column of map, with the variance correction added.
 */

int interval_mapping(double *thisint,double thismidi,dataptr dz)
{
	double *p    = dz->parray[PI_INTMAP];
	double *pend = dz->parray[PI_INTMAP] + (dz->itemcnt * 2);
	double variance, v1, v2;
	if(thismidi <= *p) {				 /* intvl is below all entries in mapping table */
		variance = thismidi - *p;
		*thisint = *(p+1) - variance;	 /* return map of bottom val (-variance) */
		return(FINISHED); 				
	}
	while(thismidi > *p) {
		p += 2;
		if(p >= pend) {					 /* intvl is above all entries in mapping table */
			p -= 2;
			variance = thismidi - *p;
			*thisint = *(p+1) - variance;/* return map of top val (-variance) */
			return(FINISHED);
		}
	}
	v1 = *p - thismidi;					 /* intvl is between 2 entries in mapping table */
	v2 = thismidi - *(p-2);
	if(v1 > v2) {						 /* Compare variances, to find which intvl closer */
		variance = v2;
		*thisint = *(p-1) - variance;
	} else {
		variance = v1;
		*thisint = *(p+1) + variance;	 /* return appropriate mapped pitch (+/-variance) */
	}
	return(FINISHED);			 
}

/************************************** PEAK_INTERP ****************/

int peak_interp
(int pitchno,int last_validpitch_no,int *lastmaxpos,double meanpich,
double minint,double maxint,double *lastmidi,dataptr dz)							
{
	int exit_status;
	double thispitch, thismidi, variance, mdiff,step;
	int m, k, maxpos = 0, minpos = 0, diff;
 	double maxvar = 0.0, minvar = 0.0;
	k = pitchno - last_validpitch_no;
	for(m=0;m<k;m++) {
		if((thispitch = dz->pitches[k+m])<MINPITCH)
			continue;
		if(thispitch >= meanpich) {
			if((variance = thispitch/meanpich)>maxvar) {
				maxvar = variance;
				maxpos = k+m;
			}
		} else {
			if((variance = meanpich/thispitch)>minvar) {
				minvar = variance;
				minpos = k+m;
			}
   		}
//TW avoid log(0)
		if(minvar > 0.0)
			minvar = LOG2(minvar) * 12.0;
//TW avoid log(0)
		if(maxvar > 0.0)
			maxvar = LOG2(maxvar) * 12.0;
		minvar /= minint;
		maxvar /= maxint;	 /* NORMALISE */
		if(maxvar < minvar)
			maxpos = minpos;
	}
	diff = maxpos - *lastmaxpos;
	if((exit_status = hztomidi(&thismidi,dz->pitches[maxpos]))<0)
		return(exit_status);
	mdiff =	thismidi - *lastmidi;
	step = mdiff/(double)diff;
	for(m=1;m<=diff;m++) {
		thismidi = *lastmidi + (step * (double)m);
		if(dz->pitches[(*lastmaxpos)+m]<MINPITCH)
			continue;
		if((exit_status = set_pval(thismidi,(*lastmaxpos)+m,dz))<0)
			return(exit_status);
	}
	*lastmidi = thismidi;
	*lastmaxpos = maxpos;
	return(FINISHED);
}

/*************************** DO_PITCH_FILTER *************************/

int do_pitch_filter(dataptr dz)
{
	int n;
	switch(dz->iparam[PF_ISFILTER]) {
	case(IS_HIPASS):
		for(n=0;n<dz->wlength;n++) {	
			if(dz->pitches[n] < dz->param[PF_LOF])
			dz->pitches[n] = (float)NOT_PITCH;
		}
		break;
	case(IS_LOPASS):
		for(n=0;n<dz->wlength;n++) {	
			if(dz->pitches[n] > dz->param[PF_HIF])
			dz->pitches[n] = (float)NOT_PITCH;
		}
		break;
	case(IS_BANDPASS):
		for(n=0;n<dz->wlength;n++) {	
			if(dz->pitches[n] < dz->param[PF_LOF] || dz->pitches[n] > dz->param[PF_HIF])
				dz->pitches[n] = (float)NOT_PITCH;
		}
		break;
	}
	return(FINISHED);
}

/***************************** TIDY_UP_PITCH_DATA ***********************/

int tidy_up_pitch_data(dataptr dz)
{
	int exit_status;
	if((exit_status = anti_noise_smoothing(dz->wlength,dz->pitches,dz->frametime))<0)
		return(exit_status);
	if((exit_status = mark_zeros_in_pitchdata(dz))<0)
		return(exit_status);
	if((exit_status = eliminate_blips_in_pitch_data(dz))<0)
		return(exit_status);
	if(dz->vflag[KEEP_PITCH_ZEROS]==FALSE)
		return interpolate_pitch(dz->pitches,1,dz);
	return pitch_found(dz);
}

/************************* GENERATE_TONE *************************/

int generate_tone(dataptr dz)
{

#define VOLUME_PAD	(0.3)
#define NOISEBASE	(0.2)

	int exit_status;
	int m, done, cc, vc, partials_in_test_tone;
	float thisamp;
	double thisfrq, basefrq;
	int n = 0, last_partial;
	double noisrange = 1.0 - NOISEBASE;
	double level, totamp = 0.0;
	dz->flbufptr[0] = dz->bigfbuf;
	thisamp = (float)(VOLUME_PAD/(double)dz->clength);
	if(dz->process==P_SYNTH) {
		dz->wanted = dz->clength * 2;
		partials_in_test_tone = dz->itemcnt;
	} else
		partials_in_test_tone = PARTIALS_IN_TEST_TONE;

	while(n < dz->wlength) {
		done = FALSE;
		thisfrq = dz->pitches[n];
		if(thisfrq < 0.0){		/* NO PITCH FOUND : GENERATE NOISE */
			if(thisfrq > NOT_SOUND) {
				if(dz->process!=P_SYNTH)
					thisamp = (float)((dz->parray[PICH_PRETOTAMP][n]/(double)dz->clength) * VOLUME_PAD);
				basefrq = 0.0;
				dz->flbufptr[0][1] = (float)(drand48() * dz->halfchwidth);
				basefrq += dz->halfchwidth;
				for(cc = 1, vc = 2; cc < dz->clength - 1; cc++, vc += 2) {
					dz->flbufptr[0][FREQ] = (float)((drand48() * dz->chwidth) + basefrq);
					dz->flbufptr[0][AMPP] = (float)(thisamp * ((drand48() * noisrange) + NOISEBASE));
					basefrq += dz->chwidth;
				}
				dz->flbufptr[0][FREQ] = (float)(dz->nyquist - (drand48() * dz->halfchwidth));
				dz->flbufptr[0][AMPP] = (float)(thisamp * ((drand48() * noisrange) + NOISEBASE));
				done = TRUE;
			} else {
				basefrq = 0.0;
				for(cc = 0, vc = 0; cc < dz->clength-1; cc++, vc += 2) {
					dz->flbufptr[0][FREQ] = (float)basefrq;
					dz->flbufptr[0][AMPP] = 0.0f;
					basefrq += dz->chwidth;
				}
				dz->flbufptr[0][FREQ] = (float)dz->nyquist;
				dz->flbufptr[0][AMPP] = 0.0f;
				done = TRUE;
			}
		} else {			/* GENERATE TESTTONE AT FOUND PITCH */
			if(dz->process==P_SYNTH) {
				for(cc = 0, vc = 0; cc < dz->clength; cc++, vc += 2)
					dz->flbufptr[0][AMPP] = 0.0f;
				basefrq = thisfrq;
				totamp = 0.0;
				for(m=0;m<partials_in_test_tone;m++) {
					cc = (int)((thisfrq + dz->halfchwidth)/dz->chwidth);
					vc = cc * 2;
					if((level = dz->parray[PICH_SPEC][m]) > 0.0) { 
						dz->flbufptr[0][AMPP] = (float)level;
						dz->flbufptr[0][FREQ] = (float)thisfrq;
						totamp += dz->flbufptr[0][AMPP];
					}
					if((thisfrq += basefrq) > dz->nyquist) {
						if((exit_status = normalise(VOLUME_PAD,totamp,dz))<0)
							return(exit_status);
						done = TRUE;
						break;
					}
				}
			} else {
				for(cc = 0, vc = 0; cc < dz->clength; cc++, vc += 2)
					dz->flbufptr[0][AMPP] = 0.0f;
				for(m=0;m<partials_in_test_tone;m++) {
					cc = (int)((thisfrq + dz->halfchwidth)/dz->chwidth); 	/* TRUNCATE */
					vc = cc * 2;
					dz->flbufptr[0][AMPP] = dz->windowbuf[TESTPAMP][m];
					dz->flbufptr[0][FREQ] = (float)thisfrq;
					if((thisfrq = thisfrq * 2.0) > dz->nyquist) {
						if((exit_status = normalise(dz->parray[PICH_PRETOTAMP][n] * VOLUME_PAD,
										dz->windowbuf[TOTPAMP][m],dz))<0)
							return(exit_status);
						done = TRUE;
						break;
					}
				}
			}
		}

		if(!done) {
			last_partial = partials_in_test_tone-1;
			if(dz->process==P_SYNTH) {
				if((level = dz->parray[PICH_SPEC][last_partial]) > 0.0) {
					if((exit_status = normalise(VOLUME_PAD,totamp,dz))<0)
						return(exit_status);
				}
			} else {
				if((exit_status = normalise(dz->parray[PICH_PRETOTAMP][n],
				dz->windowbuf[TOTPAMP][last_partial],dz))<0)
					return(exit_status);
			}
		}
		if((dz->flbufptr[0] += dz->wanted) >= dz->flbufptr[1]) {
			if((exit_status = write_samps(dz->bigfbuf,dz->big_fsize,dz))<0)
				return(exit_status);
			dz->flbufptr[0] = dz->bigfbuf;
		}
		n++;
	}
	if(dz->flbufptr[0] != dz->bigfbuf) {
		if((exit_status = write_samps(dz->bigfbuf,dz->flbufptr[0] - dz->bigfbuf,dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}

/********************** ANTI_NOISE_SMOOTHING *********************/
/*RWD used in conditional test, so better as a real var:*/
static const int MIN_SMOOTH_SET = 3;
/*#define MIN_SMOOTH_SET (3)*/		/* minimum number of adjacent smooth pitches to imply true pitch present */
#define MAX_GLISRATE   (16.0)	/* Assumptions: pitch can't move faster than 16 octaves per sec: MAX_GLISRATE */ 
								/* Possible movement from window-to-window = MAX_GLISRATE * dz->frametime */

int anti_noise_smoothing(int wlength,float *pitches,float frametime)
{
	char *smooth;
	double max_pglide;
	int n;
	if(MIN_SMOOTH_SET<3) {
		sprintf(errstr,"Bad constant: MIN_SMOOTH_SET: anti_noise_smoothing()\n");
		return(PROGRAM_ERROR);
	}
	if(wlength < MIN_SMOOTH_SET + 1)
		return(FINISHED);

	max_pglide = pow(2.0,MAX_GLISRATE * frametime);

	if((smooth = (char *)malloc((size_t)wlength))==NULL) {
		sprintf(errstr,"aINSUFFICIENT MEMORY for smoothing array.\n");
		return(MEMORY_ERROR);
	}
	for(n=1;n<wlength-1;n++)
		smooth[n] = (char)is_smooth_from_both_sides(n,max_pglide,pitches);
	smooth[0]         = (char)is_initialpitch_smooth(smooth,max_pglide,pitches);
	smooth[wlength-1] = (char)is_finalpitch_smooth(smooth,max_pglide,wlength,pitches);
	for(n=MIN_SMOOTH_SET-1;n<wlength;n++) {
		if(!smooth[n])
			smooth[n] = (char)is_smooth_from_before(n,smooth,max_pglide,pitches);
	}
	for(n=0;n<=wlength-MIN_SMOOTH_SET;n++) {
		if(!smooth[n])
			smooth[n] = (char)is_smooth_from_after(n,smooth,max_pglide,pitches);
	}
	test_glitch_sets(smooth,max_pglide,wlength,pitches);
	remove_unsmooth_pitches(smooth,wlength,pitches);
	free(smooth);
	return(FINISHED);
}		

/********************** IS_SMOOTH_FROM_BOTH_SIDES *********************
 *
 * verify a pitch if it has continuity with the pitches on either side.
 */

int is_smooth_from_both_sides(int n,double max_pglide,float *pitches)
{
	float thispitch, pitch_before, pitch_after;
	double pre_interval, post_interval;
	if((thispitch = pitches[n]) < FLTERR)
		return FALSE;
	if((pitch_before = pitches[n-1]) < FLTERR)
		return  FALSE;
	if((pitch_after = pitches[n+1]) < FLTERR)
		return  FALSE;
	pre_interval  = pitch_before/thispitch;
	if(pre_interval < 1.0)
		pre_interval = 1.0/pre_interval;
	post_interval = pitch_after/thispitch;
	if(post_interval < 1.0)
		post_interval = 1.0/post_interval;
	if(pre_interval  > max_pglide
	|| post_interval > max_pglide)
		return  FALSE;
	return  TRUE;
}

/********************** IS_INITIALPITCH_SMOOTH *********************
 *
 * verify first pitch if it has continuity with an ensuing verified pitch.
 */

int is_initialpitch_smooth(char *smooth,double max_pglide,float *pitches)
{
	float thispitch;
	int n;
	double post_interval;
	if((thispitch = pitches[0]) < FLTERR)
		return FALSE;
	for(n=1;n < MIN_SMOOTH_SET;n++) {
		if(smooth[n]) {
			post_interval = pitches[n]/pitches[0];
			if(post_interval < 1.0)
				post_interval = 1.0/post_interval;
			if(post_interval <= pow(max_pglide,(double)n))
				return TRUE;
		}
	}	
	return(FALSE);
}
			
/********************** IS_FINALPITCH_SMOOTH *********************
 *
 * verify final pitch if it has continuity with a preceding verified pitch.
 */

int is_finalpitch_smooth(char *smooth,double max_pglide,int wlength,float *pitches)
{
	float thispitch;
	double pre_interval;
	int n;
	int last = wlength - 1;
	if((thispitch = pitches[last]) < FLTERR)
		return FALSE;
	for(n=1;n < MIN_SMOOTH_SET;n++) {
		if(smooth[last-n]) {
			pre_interval = pitches[last-n]/pitches[last];
			if(pre_interval < 1.0)
				pre_interval = 1.0/pre_interval;
			if(pre_interval <= pow(max_pglide,(double)n))
				return TRUE;
		}
	}
	return(FALSE);
}
			
/********************** IS_SMOOTH_FROM_BEFORE *********************
 *
 * verify a pitch which has continuity with a preceding set of verified pitches.
 */

int is_smooth_from_before(int n,char *smooth,double max_pglide,float *pitches)
{
	float thispitch, pitch_before;
	double pre_interval;
	int m;
	if((thispitch = pitches[n]) < FLTERR)
		return FALSE;
	for(m=1;m<MIN_SMOOTH_SET;m++) {		/* If there are (MIN_SMOOTH_SET-1) smooth pitches before */
		if(!smooth[n-m])
			return(FALSE);
	}
	pitch_before = pitches[n-1];		/* Test the interval with the previous pitch */
	pre_interval  = pitch_before/thispitch;
	if(pre_interval < 1.0)
		pre_interval = 1.0/pre_interval;
	if(pre_interval  > max_pglide)   
		return  FALSE;				   	/* And if it's acceptably smooth */
	return  TRUE;					   	/* mark this pitch as smooth also */
}

/********************** IS_SMOOTH_FROM_AFTER *********************
 *
 * verify a pitch which has continuity with a following set of verified pitches.
 */

int is_smooth_from_after(int n,char *smooth,double max_pglide,float *pitches)
{
	float thispitch, pitch_after;
	double post_interval;
	int m;
	if((thispitch = pitches[n]) < FLTERR)
		return FALSE;
	for(m=1;m<MIN_SMOOTH_SET;m++) {	/* If there are (MIN_SMOOTH_SET-1) smooth pitches after */
		if(!smooth[n+m])
		return(FALSE);
	}
	pitch_after = pitches[n+1];		/* Test the interval with the next pitch */
	post_interval  = pitch_after/thispitch;
	if(post_interval < 1.0)
		post_interval = 1.0/post_interval;
	if(post_interval  > max_pglide)   
		return  FALSE;				/* And if it's acceptably smooth */
	return  TRUE;					/* mark this pitch as smooth also */
}

/********************** TEST_GLITCH_SETS *********************
 *
 * This function looks for any sets of values that appear to be glitches
 * amongst the real pitch data.
 * It is possible some items are REAL pitch data isolated BETWEEN short glitches.
 * This function checks for these cases.
 */

int test_glitch_sets(char *smooth,double max_pglide,int wlength,float *pitches)
{
	int exit_status;
	int gotglitch = FALSE;
	int n, gltchend, gltchstart = 0;
	for(n=0;n<wlength;n++) {
		if(gotglitch) {			/* if inside a glitch */
			if(smooth[n]) {		/* if reached its end, mark the end, then process the glitch */
				gltchend = n;
				if((exit_status = test_glitch_forwards(gltchstart,gltchend,smooth,max_pglide,pitches))<0)
					return(exit_status);
				if((exit_status = test_glitch_backwards(gltchstart,gltchend,smooth,max_pglide,wlength,pitches))<0)
					return(exit_status);
				gotglitch = 0;
			}
		} else {				/* look for a glitch and mark its start */
			if(!smooth[n]) {
				gotglitch = 1;
				gltchstart = n;
			}
		}
	}
	if(gotglitch) {				/* if inside a glitch at end of data, process glitch */
		gltchend = n;
		test_glitch_forwards(gltchstart,gltchend,smooth,max_pglide,pitches);
	}
	return(FINISHED);
}

/********************* REMOVE_UNSMOOTH_PITCHES ***********************
 *
 * delete all pitches which have no verified continuity with surrounding pitches.
 */

void remove_unsmooth_pitches(char *smooth,int wlength,float *pitches)
{
	int n;
	for(n=0;n<wlength;n++) {
		if(!smooth[n])		
			pitches[n] = (float)NOT_PITCH;
	}
}

/********************** TEST_GLITCH_FORWARDS *********************
 *
 * searching from start of glitch, look for isolated true pitches
 * amongst glitch data.
 */

#define LAST_SMOOTH_NOT_SET (-1)

int test_glitch_forwards(int gltchstart,int gltchend,char *smooth,double max_pglide,float *pitches)
{
	int n, glcnt;
	int last_smooth, previous;
	double pre_interval;
	if((previous = gltchstart - 1) < 0)
		return FINISHED;
	if(pitches[previous] < FLTERR) {
		sprintf(errstr,"Error in previous smoothing logic: test_glitch_forwards()\n");
		return(PROGRAM_ERROR);
	}
	last_smooth = previous;
	n = gltchstart+1;								/* setup params for local search of glitch */
	glcnt = 1;
	while(n < gltchend) {							/* look through the glitch */
		if(pitches[n] > FLTERR) {					/* if glitch location holds a true pitch */
			pre_interval = pitches[n]/pitches[previous]; 
			if(pre_interval < 1.0)
				pre_interval = 1.0/pre_interval;	/* compare against previous verified pitch */
			if(pre_interval <= pow(max_pglide,(double)(n-previous))) {
				smooth[n] = TRUE;	   				/* if comparable: mark this pitch as verified */
				last_smooth = n;
			}
		}											
		n++;										/* Once more than a max-glitch-set has been scanned */							  				
													/* or the end of the entire glitch is reached */
		if(++glcnt >= MIN_SMOOTH_SET || n >= gltchend) {
			if(last_smooth == previous)
				break;								/* If no new verifiable pitch found, give up */
			previous = last_smooth;
			n = last_smooth + 1;					/* Otherwise start a new local search from newly verified pitch */
			glcnt = 1;									
		}
	}
	return(FINISHED);
}

/********************** TEST_GLITCH_BACKWARDS *********************
 *
 * searching from end of glitch, look for isolated true pitches
 * amongst glitch data.
 */

int test_glitch_backwards(int gltchstart,int gltchend,char *smooth,double max_pglide,int wlength,float *pitches)
{
	int n, glcnt, next, next_smooth;
	double post_interval;
	if((next = gltchend) >= wlength)
		return FINISHED;
	if(pitches[next] < FLTERR) {
		sprintf(errstr,"Error in previous smoothing logic: test_glitch_backwards()\n");
		return(PROGRAM_ERROR);
	}
	next_smooth = next;
	n = gltchend-2;			 						/* setup params for local search of glitch */
	glcnt = 1;
	while(n >= gltchstart) {						/* look through the glitch */
		if(pitches[n] > FLTERR) {					/* if glitch location holds a true pitch */
			post_interval = pitches[n]/pitches[next]; 
			if(post_interval < 1.0)
				post_interval = 1.0/post_interval;	/* compare against previous verified pitch */
			if(post_interval <= pow(max_pglide,(double)(next - n))) {
				smooth[n] = TRUE;	   				/* if comparable: mark this pitch as verified */
				next_smooth = n;
			}
		}							  				
		n--;						  				/* Once more than a max-glitch-set has been scanned */
													/* or the start of the entire glitch is reached */							  				
		if(++glcnt >= MIN_SMOOTH_SET || n < gltchstart) {	  				
			if(next_smooth == next)
				break;								/* If no new verifiable pitch found, give up */
			next = next_smooth;
			n = next_smooth - 1;
			glcnt = 1;								/* Otherwise start a new local search */
		}
	}
	return(FINISHED);
}

/*********** WRITE_PITCH_OUTHEADER_FROM_ANALYSIS_INHEADER_TO_SECOND_OUTFILE **************
 *
 * Works for specpitch and spectrack: which write to 2nd datafile!!!
 */

int write_pitch_outheader_from_analysis_inheader_to_second_outfile(int ofd,dataptr dz)
{
	int exit_status;
	int orig_process 	 = dz->process_type;
	int orig_outfiletype = dz->outfiletype;
	int orig_chans       = dz->infile->channels;
	int orig_origchans   = dz->infile->origchans;
	dz->process_type = ANAL_TO_PITCH;
	dz->outfiletype  = PITCH_OUT;
	dz->outfile->origchans = dz->infile->channels;
	dz->outfile->channels  = 1;
	if((exit_status = headwrite(ofd,dz))<0)
		return(exit_status);
		/* restore orig values */
	dz->process_type       = orig_process;
	dz->outfiletype        = orig_outfiletype;
	dz->outfile->origchans = orig_origchans;
	dz->outfile->channels  = orig_chans;
	return(FINISHED);
}

/***************************** LOCAL_PEAK **************************/

int local_peak(int thiscc,double frq, float *thisbuf, dataptr dz)
{
	int thisvc = thiscc * 2;
	int cc, vc, searchtop, searchbot;
	double frqtop = frq * SEMITONE_INTERVAL;
	double frqbot = frq / SEMITONE_INTERVAL;
	searchtop = (int)((frqtop + dz->halfchwidth)/dz->chwidth);		/* TRUNCATE */
	searchtop = min(dz->clength,searchtop + PEAKSCAN + 1);
	searchbot = (int)((frqbot + dz->halfchwidth)/dz->chwidth);		/* TRUNCATE */
	searchbot = max(0,searchbot - PEAKSCAN);
	for(cc = searchbot ,vc = searchbot*2; cc < searchtop; cc++, vc += 2) {
		if(thisbuf[thisvc] < thisbuf[vc])
			return(FALSE);
	}
	return(TRUE);
}

/**************************** INTERPOLATE_PITCH ***************************/

int interpolate_pitch(float *floatbuf,int skip_silence,dataptr dz)
{
	int exit_status;
	int pitchno;
	for(pitchno=0;pitchno<dz->wlength;pitchno++) {
		if(floatbuf[pitchno] < MINPITCH) {
			if(skip_silence && flteq((double)floatbuf[pitchno],NOT_SOUND))
				continue;
			if((exit_status = do_interpolating(&pitchno,floatbuf,skip_silence,dz))<0)
				return(exit_status);
		}
	}
	return(FINISHED);
}

/****************************** DO_INTERPOLATING ************************
 *
 * WITHOUT THOROUGH CHECK, worried about logs getting <= 0.0.
 */

int do_interpolating(int *pitchno,float *floatbuf,int skip_silence,dataptr dz)
{
#define MID_PITCH   (0)
#define FIRST_PITCH (1)
#define END_PITCH   (2)
#define NO_PITCH    (3)

	int act_type = MID_PITCH;
	int start = *pitchno, m;
	double startpitch, endpitch, thispitch, lastpitch, pstep;
	if(*pitchno==0L)
		act_type = FIRST_PITCH;
	while(floatbuf[*pitchno] < MINPITCH) {
		if(++(*pitchno)>=dz->wlength) {
			if(act_type == FIRST_PITCH)
				act_type = NO_PITCH;
			else
				act_type = END_PITCH;
			break;
		}
	}
	if(act_type==MID_PITCH) {
		m = start-1;		
		while(floatbuf[m] < MINPITCH) {
			if(--m <= 0) {
				act_type = FIRST_PITCH;
				break;
			}
		}
		start = m;
	}
	switch(act_type) {
	case(MID_PITCH):
		startpitch = hz_to_pitchheight((double)floatbuf[start-1]);
		endpitch   = hz_to_pitchheight((double)floatbuf[*pitchno]);
		pstep = (endpitch - startpitch)/(double)((*pitchno) - (start - 1));
		lastpitch = startpitch;
		for(m=start;m<*pitchno;m++) {   /* INTERP PITCH ACROSS UNPITCHED SEG */
			thispitch  = lastpitch + pstep;
			if(!(skip_silence && flteq(floatbuf[m],NOT_SOUND)))
				floatbuf[m] = (float)pitchheight_to_hz(thispitch);
			lastpitch = thispitch;
		}
		break;
	case(FIRST_PITCH):
		for(m=0;m<*pitchno;m++) {	/* EXTEND FIRST CLEAR PITCH BACK TO START */
			if(!(skip_silence && flteq(floatbuf[m],NOT_SOUND)))
				floatbuf[m] = floatbuf[*pitchno];
		}
		break;
	case(END_PITCH):			/* EXTEND LAST CLEAR PITCH ON TO END */
		for(m=start;m<*pitchno;m++) {
			if(!(skip_silence && flteq(floatbuf[m],NOT_SOUND)))
				floatbuf[m] = floatbuf[start-1];
		}
		break;
	case(NO_PITCH): 
		sprintf(errstr,"No valid pitch found.\n");
		return(GOAL_FAILED);				 
	}
	(*pitchno)--;
	return(FINISHED);
}

/***************************** HZ_TO_PITCHHEIGHT *******************************
 *
 * Real pitch is 12 * log2(frq/basis_frq).
 * 
 * BUT (with a little help from the Feynman lectures!!)
 * (1) 	The basis_frq is arbitrary, and cancels out, so let it be 1.0.
 		i.e. pitch1 = 12 * log2(frq1/basis_frq) = 12 * (log2(frq1) - log2(basis_frq));
 			 pitch2 = 12 * log2(frq2/basis_frq) = 12 * (log2(frq2) - log2(basis_frq));
			 pitch1 - pitch2 = 12 * (log2(frq1) - log2(frq2)) = 12 * log2(frq1/frq2);
 * (2) 	Finding the difference of 2 log2() numbers, interpolating and
 *     	reconverting to pow(2.0,...) is no different to doing same
 *		calculation to base e.
 * (3)	The (12 *) is also a cancellable factor in all this.
 * 		So pitch_height serves the same function as pitch in these calculations!!
 */

double hz_to_pitchheight(double frqq)
{   
	return log(frqq);
}

/***************************** PITCHHEIGHT_TO_HZ *******************************/

double pitchheight_to_hz(double pitch_height)
{
	return exp(pitch_height);
}

/************************** ELIMINATE_BLIPS_IN_PITCH_DATA ****************************
 *
 * (1) 	Eliminate any group of 'dz->param[PICH_VALID]' pitched windows, bracketed by
 *	unpitched windows, as unreliable data.
 */

int eliminate_blips_in_pitch_data(dataptr dz)
{
	int q;
	int n, m, k, wlength_less_bliplen;
	int OK = 1;
	switch(dz->process) {
	case(PITCH): q = PICH_VALID;	break;
	case(TRACK): q = TRAK_VALID;	break;
	default:
		sprintf(errstr,"unknown case in eliminate_blips_in_pitch_data()\n");
		return(PROGRAM_ERROR);
	}
	if(dz->iparam[q]<=0)
		return(FINISHED);
	wlength_less_bliplen = dz->wlength - dz->iparam[q];
	for(n=1;n<wlength_less_bliplen;n++) {
		if(dz->pitches[n] > 0.0) {
			if(dz->pitches[n-1] < 0.0) {
				for(k = 1; k <= dz->iparam[q]; k++) {
					if(dz->pitches[n+k] < 0.0) {
						for(m=0;m<k;m++)
							dz->pitches[n+m] = (float)NOT_PITCH;
						n += k;
						continue;
					}
				}
			}
		}
	}
	n = wlength_less_bliplen;
	if((dz->pitches[n] > 0.0) && (dz->pitches[n-1] < 0.0)) {
/* UNREACHABLE at level4 ??? */
		for(k = 1; k < dz->iparam[q]; k++) {
			if(dz->pitches[n+k] < 0.0)
				OK = 0;
			break;
		}
	}
	if(!OK)  {
		for(n=wlength_less_bliplen;n<dz->wlength;n++)
			dz->pitches[n] = (float)NOT_PITCH;
	}
	return(FINISHED);
}

/********************************** MARK_ZEROS_IN_PITCHDATA ************************
 *
 * Disregard data on windows which are SILENCE_RATIO below maximum level.
 */

int mark_zeros_in_pitchdata(dataptr dz)
{
	int k;
	int n;
	double maxlevel = 0.0, minlevel;
	switch(dz->process) {
	case(PITCH): k = PICH_SRATIO;	break;
	case(TRACK): k = TRAK_SRATIO;	break;
	default:
		sprintf(errstr,"unknown case in mark_zeros_in_pitchdata()\n");
		return(PROGRAM_ERROR);
	}
	for(n=0;n<dz->wlength;n++) {
		if(dz->parray[PICH_PRETOTAMP][n] > maxlevel)
			maxlevel = dz->parray[PICH_PRETOTAMP][n];
	}
	minlevel = maxlevel * dz->param[k];
	for(n=0;n<dz->wlength;n++) {
		if(dz->parray[PICH_PRETOTAMP][n] < minlevel)
			dz->pitches[n] = (float)NOT_SOUND;
	}
	return(FINISHED);
}

/**************************** PITCH_FOUND ****************************/

int pitch_found(dataptr dz)
{
	int n;
    for(n=0;n<dz->wlength;n++) {
		if(dz->pitches[n] > NOT_PITCH)
			return(FINISHED);
	}
	sprintf(errstr,"No valid pitch found.\n");
	return(GOAL_FAILED);
}
 
/**************************** CHECK_TRANSPOS ****************************/

void check_transpos(float *t,dataptr dz)
{
	if(*t <= MIN_TRANSPOS) {
		if(!dz->fzeroset) {
			fprintf(stdout,"WARNING: Transposition(s) by > max permitted: adjusted.\n");
			fflush(stdout);
			dz->fzeroset = TRUE;
		}
		*t = (float)(MIN_TRANSPOS + FLTERR);
	}
	if(*t >= MAX_TRANSPOS) {
		if(!dz->fzeroset) {
			fprintf(stdout,"WARNING: Transposition(s) by > max permitted: adjusted.\n");
			fflush(stdout);
			dz->fzeroset = TRUE;
		}
		*t = (float)(MAX_TRANSPOS - 1.0);
	}
}

/**************************** CHECK_PITCH ****************************/

void check_pitch(float *t,dataptr dz)
{
	if(*t <= SPEC_MINFRQ) {
		if(!dz->fzeroset) {
			fprintf(stdout,"WARNING: Pitch(es) out of permitted range: adjusted.\n");
			fflush(stdout);
			dz->fzeroset = TRUE;
		}
		*t = (float)(SPEC_MINFRQ + FLTERR);
	}
	if(*t >= DEFAULT_NYQUIST) {
		if(!dz->fzeroset) {
			fprintf(stdout,"WARNING: Pitch(es) out of permitted range: adjusted.\n");
			fflush(stdout);
			dz->fzeroset = TRUE;
		}
		*t = (float)(DEFAULT_NYQUIST-1.0);
	}
}

/***************************** WRITE_PITCH_OR_TRANSPOS_DATA ******************************/

int write_pitch_or_transpos_data(int final_length_in_windows,dataptr dz)
{
	int exit_status;
	if(final_length_in_windows > 0) {
		if(dz->is_transpos) {
			if((exit_status = write_exact_samps(dz->transpos,final_length_in_windows,dz))<0)
				return(exit_status);
		} else {
			if((exit_status = write_exact_samps(dz->pitches,final_length_in_windows,dz))<0)
				return(exit_status);
		}
	}
	return(FINISHED);
}

/***************************** TRAP_JUNK ******************************/

int trap_junk(int final_length_in_windows,dataptr dz)
{
	int n;
	int caught_zero = 0, caught_skrch = 0;
	double mintrans, maxtrans;
	if((dz->process==P_EXAG && ODD(dz->mode)) || dz->mode==TRANSP_OUT) {	
		/* Transposition file output */
		maxtrans = dz->nyquist/MINPITCH;
		mintrans = 1.0/maxtrans;
		for(n=0;n<final_length_in_windows;n++) {
			//if(!caught_zero && (dz->pitches[n] < mintrans && !flteq((double)dz->pitches[n],NOT_PITCH)))
			//	caught_zero = 1;
			/*RWD 6:2001 */
			if(!caught_zero && (dz->transpos[n] < mintrans && !
				    (flteq((double)dz->transpos[n],NOT_PITCH)  || flteq((double)dz->transpos[n],NOT_SOUND))))
				caught_zero = 1;
			if(!caught_skrch && (dz->transpos[n] > maxtrans))
				caught_skrch = 1;
			if(caught_zero && caught_skrch)
				break;
		}
		if(caught_zero || caught_skrch) {
			if(caught_zero)
				sprintf(errstr,"You have generated transposition data < the minimum possible.\n");
			if(caught_skrch)
				sprintf(errstr,"You have generated transposition data > the maximum possible.\n");
			return(GOAL_FAILED);
		}
	} else { /* Pitch file output */
		for(n=0;n<final_length_in_windows;n++) {
			//if(!caught_zero && (dz->pitches[n] < MINPITCH && !flteq((double)dz->pitches[n],NOT_PITCH)))
			//	caught_zero = 1;
			/* RWD 6:2001 trap zero windows too? */
			if(!caught_zero && (dz->pitches[n] < MINPITCH && !
				(flteq((double)dz->pitches[n],NOT_PITCH)  || flteq((double)dz->pitches[n],NOT_SOUND))))
				caught_zero = 1;
			if(!caught_skrch && (dz->pitches[n] > dz->nyquist))
				caught_skrch = 1;
			if(caught_zero && caught_skrch)
				break;
		}

		if(caught_zero || caught_skrch) {
			if(caught_zero)
				sprintf(errstr,"You have generated pitch data below %.0lfHz.\n",MINPITCH);
			if(caught_skrch)
				sprintf(errstr,"You have generated pitch data above the nyquist frq.\n");
			return(GOAL_FAILED);
		}

	}
	return(FINISHED);
}

int pitch_insert(int is_sil,dataptr dz)
{
	int n, m, start, end;
	int last_window = dz->wlength - 1;
	int cnt = 0;
	for(n=0;n<dz->itemcnt;n++) {
		start = dz->lparray[0][n];		
		end  =  dz->lparray[1][n];
		if(start > last_window)
			break;
		end = min(end,last_window);
		if(is_sil) {
			for(m=start;m<=end;m++) {
				dz->pitches[m] = (float)NOT_SOUND;		
				cnt++;
			}
		} else {
			for(m=start;m<=end;m++) {
				dz->pitches[m] = (float)NOT_PITCH;		
				cnt++;
			}
		}
	}
	if(cnt==0) {
		sprintf(errstr,"No insertions made.\n");
		return(GOAL_FAILED);
	}
	return(FINISHED);
}

int pitch_to_silence(dataptr dz)
{
	int n, cnt = 0;

	for(n=0;n < dz->wlength;n++) {
		if(!flteq((double)dz->pitches[n],NOT_PITCH)) {
			dz->pitches[n] = (float)NOT_SOUND;			
			cnt++;
		}
	}
	if(cnt==0) {
		sprintf(errstr,"No silence inserted.\n");
		return(GOAL_FAILED);
	}
	return(FINISHED);
}

int unpitch_to_silence(dataptr dz)
{
	int n, cnt = 0;

	for(n=0;n<dz->wlength;n++) {
		if(flteq((double)dz->pitches[n],NOT_PITCH)) {
			dz->pitches[n] = (float)NOT_SOUND;			
			cnt++;
		}
	}
	if(cnt==0) {
		sprintf(errstr,"No silence inserted.\n");
		return(GOAL_FAILED);
	}
	return(FINISHED);
}

/***************************** GET_ANAL_ENVELOPE ***********************/

int get_anal_envelope(dataptr dz)
{
	int exit_status;
	int samps_read, wc, windows_in_buf, samps_left;
	double totalamp;
	dz->flbufptr[1] = dz->flbufptr[2];
	while((samps_read = fgetfbufEx(dz->bigfbuf,dz->big_fsize,dz->ifd[0],0)) > 0) {
		dz->flbufptr[0] = dz->bigfbuf;
		windows_in_buf = samps_read/dz->wanted;
		for(wc=0; wc<windows_in_buf; wc++) {
			if((exit_status = get_totalamp(&totalamp,dz->flbufptr[0],dz->wanted))<0)
				return(exit_status);
			*(dz->flbufptr[1]) = (float)totalamp;
			if(++dz->flbufptr[1] >= dz->flbufptr[3]) {
				dz->flbufptr[1] = dz->flbufptr[2];
				if((exit_status = write_samps(dz->flbufptr[2],dz->big_fsize,dz))<0)
					return(exit_status);
			}
			dz->flbufptr[0] += dz->wanted;
		}
	}  
	if((samps_left = dz->flbufptr[1] - dz->flbufptr[2]) > 0) {
		if((exit_status = write_samps(dz->flbufptr[2],samps_left,dz))<0)
			return(exit_status);
	}
	dz->outfile->window_size = (float)(dz->frametime * SECS_TO_MS);
	return(FINISHED);
}

/************************************ GENERATE_VOWELS ************************************/

int generate_vowels(dataptr dz)
{
	int *vowels = dz->iparray[0];
	double *times = dz->parray[0];
	double startformant1, startformant2, startformant3, endformant1, endformant2, endformant3;
	double formant1, formant2, formant3;
	double form1step, form2step, form3step;
	double starttime, endtime, time, timefrac, timestep, *sensitivity;
	double thisfrq, basefrq;
	double f3startatten, f3endatten, f3attenstep, f3atten;
	double f2startatten, f2endatten, f2attenstep, f2atten;
	float thisamp = (float)(VOLUME_PAD/(double)dz->clength);
	double noisrange = 1.0 - NOISEBASE;
	int cc, vc, exit_status, senslen, is_offset = 0;
	int n = 0, t = 0;

	if((exit_status = define_sensitivity_curve(&sensitivity,&senslen))<0)
		return(exit_status);

	if(dz->param[PV_OFFSET] > 0.0)
		is_offset = 1;
	dz->flbufptr[0] = dz->bigfbuf;
	dz->wanted = dz->infile->origchans;

	if((exit_status = get_formant_frqs
	(vowels[t],&startformant1,&startformant2,&startformant3,&f2startatten,&f3startatten))<0)
		return(exit_status);
	starttime = times[t++];
	if((exit_status = get_formant_frqs(vowels[t],&endformant1,&endformant2,&endformant3,&f2endatten,&f3endatten))<0)
		return(exit_status);
	endtime = times[t++];
	form1step = endformant1 - startformant1;
	form2step = endformant2 - startformant2;
	form3step = endformant3 - startformant3;
	f2attenstep = f2endatten - f2startatten; 
	f3attenstep = f3endatten - f3startatten; 
	timestep  = endtime-starttime;
	formant1 = startformant1;	/* works if only one vowel is entered (for time zero) */
	formant2 = startformant2;
	formant3 = startformant3;
	f2atten  = f2startatten;
	f3atten  = f3startatten;
	while(n < dz->wlength) {
		thisfrq = dz->pitches[n];
		if(thisfrq < 0.0){		/* NO PITCH FOUND : GENERATE NOISE */
			if(thisfrq > NOT_SOUND) {
				basefrq = 0.0;
				dz->flbufptr[0][1] = (float)(drand48() * dz->halfchwidth);
				basefrq += dz->halfchwidth;
				for(cc = 1, vc = 2; cc < dz->clength - 1; cc++, vc += 2) {
					dz->flbufptr[0][FREQ] = (float)((drand48() * dz->chwidth) + basefrq);
					dz->flbufptr[0][AMPP] = (float)(thisamp * ((drand48() * noisrange) + NOISEBASE));
					basefrq += dz->chwidth;
				}
				dz->flbufptr[0][FREQ] = (float)(dz->nyquist - (drand48() * dz->halfchwidth));
				dz->flbufptr[0][AMPP] = (float)(thisamp * ((drand48() * noisrange) + NOISEBASE));
			} else {			/* NO SOUND FOUND, GENERATE SILENCE */
				basefrq = 0.0;
				for(cc = 0, vc = 0; cc < dz->clength-1; cc++, vc += 2) {
					dz->flbufptr[0][FREQ] = (float)basefrq;
					dz->flbufptr[0][AMPP] = 0.0f;
					basefrq += dz->chwidth;
				}
				dz->flbufptr[0][FREQ] = (float)dz->nyquist;
				dz->flbufptr[0][AMPP] = 0.0f;
			}
		} else {				/* GENERATE VOWEL */
			basefrq = 0.0;
			for(cc = 0, vc = 0; cc < dz->clength-1; cc++, vc += 2) {
				dz->flbufptr[0][AMPP] = 0.0f;
				dz->flbufptr[0][FREQ] = (float)basefrq;	 /* default frq, overwritten by vowel partials */
				basefrq += dz->chwidth;
			}
			dz->flbufptr[0][AMPP] = 0.0f;
			dz->flbufptr[0][FREQ] = (float)dz->nyquist;
			if(dz->itemcnt) {
				time = n * dz->frametime;
				while(time >= endtime) {	  	/* advance along vowels */
					startformant1 = endformant1;
					startformant2 = endformant2;
					startformant3 = endformant3;
					f2startatten  = f2endatten;
					f3startatten  = f3endatten;
					starttime = endtime;
					if(t < dz->itemcnt) {
						if((exit_status = get_formant_frqs(vowels[t],&endformant1,&endformant2,&endformant3,&f2endatten,&f3endatten))<0)
							return(exit_status);
						endtime = times[t++];
					} else
						break;
					form1step = endformant1 - startformant1;
					form2step = endformant2 - startformant2;
					form3step = endformant3 - startformant3;
					f2attenstep = f2endatten - f2startatten; 
					f3attenstep = f3endatten - f3startatten; 
					timestep  = endtime-starttime;
				}
				if(!flteq(starttime,endtime)) {			   		/* interpolate between vowels : or retain last vowel */
					timefrac = (time - starttime)/timestep;
					formant1 = startformant1 + (form1step * timefrac);
					formant2 = startformant2 + (form2step * timefrac);
					formant3 = startformant3 + (form3step * timefrac);
					f2atten  = f2startatten  + (f2attenstep * timefrac);
					f3atten  = f3startatten  + (f3attenstep * timefrac);
				}
			}
			if((exit_status = generate_vowel_spectrum
			(thisfrq,formant1,formant2,formant3,f2atten,f3atten,sensitivity,senslen,is_offset,dz)) <0)
				return(exit_status);
		}
		if((dz->flbufptr[0] += dz->wanted) >= dz->flbufptr[1]) {
			if((exit_status = write_samps(dz->bigfbuf,dz->big_fsize,dz))<0)
				return(exit_status);
			dz->flbufptr[0] = dz->bigfbuf;
		}
		n++;
	}
	if(dz->flbufptr[0] != dz->bigfbuf) {
		if((exit_status = write_samps(dz->bigfbuf,dz->flbufptr[0] - dz->bigfbuf,dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}
		
/************************************ DEFINE_SENSITIVITY_CURVE ************************************
 *
 * approximate compensation for aural sensitivity 
 */

#define	LOFRQ_BOOST		(2.511)		/* 8dB  */
#define	HIFRQ_LOSS		(0.4)		/* -8dB */

#define	LOFRQ_FOOT		(250.0)
#define	MIDFRQSHELF_BOT	(2000.0)
#define	MIDFRQSHELF_TOP	(3000.0)
#define	HIFRQ_FOOT		(4000.0)
#define	TOP_OF_SPECTRUM	(96000.0)	/* double maximum nyquist (i.e. >nyquist: for safety margin)  */

int define_sensitivity_curve(double **sensitivity,int *senslen)
{
	int arraysize = BIGARRAY;
	double *p;
	int n = 0;

	if((*sensitivity = (double *)malloc(arraysize * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for time data.\n");
		return(MEMORY_ERROR);
	}
	p = *sensitivity;
	*p++ = 0.0;				*p++ = 1.0;						n+= 2;	/* everything must be in 0-1 range */
	*p++ = LOFRQ_FOOT;		*p++ = 1.0;						n+= 2;	/* for pow() calculations to work, later */
	*p++ = MIDFRQSHELF_BOT;	*p++ = 1.0/LOFRQ_BOOST;			n+= 2;
	*p++ = MIDFRQSHELF_TOP;	*p++ = 1.0/LOFRQ_BOOST;			n+= 2;
	*p++ = HIFRQ_FOOT;		*p++ = HIFRQ_LOSS/LOFRQ_BOOST;	n+= 2;
	*p++ = TOP_OF_SPECTRUM;	*p++ = HIFRQ_LOSS/LOFRQ_BOOST;	n+= 2;

	if((*sensitivity = (double *)realloc((char *)(*sensitivity),n * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for sensitivity curve.\n");
		return(MEMORY_ERROR);
	}
	*senslen = n;
	return(FINISHED);
}

/************************************ GET_FORMANT_FRQS ************************************/

int get_formant_frqs
(int vowel,double *formant1, double *formant2, double *formant3, double *f2atten, double *f3atten)
{
	switch(vowel) {
	case(VOWEL_EE):	*formant1= EE_FORMANT1;	*formant2= EE_FORMANT2;	*formant3= EE_FORMANT3;	
	/* heed  */								*f2atten = EE_F2ATTEN;	*f3atten = EE_F3ATTEN;
					break;
	case(VOWEL_I):	*formant1= I_FORMANT1;	*formant2= I_FORMANT2;	*formant3= I_FORMANT3;
	/* hid  */								*f2atten = I_F2ATTEN;	*f3atten = I_F3ATTEN;
					break;
	case(VOWEL_AI):	*formant1= AI_FORMANT1;	*formant2= AI_FORMANT2;	*formant3= AI_FORMANT3;	
	/* maid  */								*f2atten = AI_F2ATTEN;	*f3atten = AI_F3ATTEN;
					break;
	case(VOWEL_AII): *formant1= AII_FORMANT1;	*formant2= AII_FORMANT2;	*formant3= AII_FORMANT3;	
	/* scottish educAted  */				*f2atten = AII_F2ATTEN;	*f3atten = AII_F3ATTEN;
					break;
	case(VOWEL_E):	*formant1= E_FORMANT1;	*formant2= E_FORMANT2;	*formant3= E_FORMANT3;	
	/* head  */								*f2atten = E_F2ATTEN;	*f3atten = E_F3ATTEN;
					break;
	case(VOWEL_A):	*formant1= A_FORMANT1;	*formant2= A_FORMANT2;	*formant3= A_FORMANT3;	
	/* had  */								*f2atten = A_F2ATTEN;	*f3atten = A_F3ATTEN;
					break;
	case(VOWEL_AR):	*formant1= AR_FORMANT1;	*formant2= AR_FORMANT2;	*formant3= AR_FORMANT3;	
	/* hard  */								*f2atten = AR_F2ATTEN;	*f3atten = AR_F3ATTEN;
					break;
	case(VOWEL_O):	*formant1= O_FORMANT1;	*formant2= O_FORMANT2;	*formant3= O_FORMANT3;	
	/* hod  */								*f2atten = O_F2ATTEN;	*f3atten = O_F3ATTEN;
					break;
	case(VOWEL_OR):	*formant1= OR_FORMANT1;	*formant2= OR_FORMANT2;	*formant3= OR_FORMANT3;	
	/* hoard  */							*f2atten = OR_F2ATTEN;	*f3atten = OR_F3ATTEN;
					break;
	case(VOWEL_OA):	*formant1= OA_FORMANT1;	*formant2= OA_FORMANT2;	*formant3= OA_FORMANT3;	
	/* load (North of England)  */			*f2atten = OA_F2ATTEN;	*f3atten = OA_F3ATTEN;
					break;
	case(VOWEL_U):	*formant1= U_FORMANT1;	*formant2= U_FORMANT2;	*formant3= U_FORMANT3;	
	/* hood, mud (Norht of England)  */		*f2atten = U_F2ATTEN;	*f3atten = U_F3ATTEN;
					break;
	case(VOWEL_UU):	*formant1= UU_FORMANT1;	*formant2= UU_FORMANT2;	*formant3= UU_FORMANT3;	
	/* Scottish edUcated  */				*f2atten = UU_F2ATTEN;	*f3atten = UU_F3ATTEN;
					break;
	case(VOWEL_UI):	*formant1= UI_FORMANT1;	*formant2= UI_FORMANT2;	*formant3= UI_FORMANT3;	
	/* Scottish 'could'  */					*f2atten = UI_F2ATTEN;	*f3atten = UI_F3ATTEN;
					break;
	case(VOWEL_OO):	*formant1= OO_FORMANT1;	*formant2= OO_FORMANT2;	*formant3= OO_FORMANT3;	
	/* mood  */								*f2atten = OO_F2ATTEN;	*f3atten = OO_F3ATTEN;
					break;
	case(VOWEL_XX):	*formant1= XX_FORMANT1;	*formant2= XX_FORMANT2;	*formant3= XX_FORMANT3;	
	/* mud (South of England)  */			*f2atten = XX_F2ATTEN;	*f3atten = XX_F3ATTEN;
					break;
	case(VOWEL_X):	*formant1= X_FORMANT1;	*formant2= X_FORMANT2;	*formant3 = X_FORMANT3;	
	/* the, herd  */						*f2atten = X_F2ATTEN;	*f3atten = X_F3ATTEN;
					break;
	case(VOWEL_N):	*formant1= N_FORMANT1;	*formant2= N_FORMANT2;	*formant3 = N_FORMANT3;	
	/* 'n'  */								*f2atten = N_F2ATTEN;	*f3atten = N_F3ATTEN;
					break;
	case(VOWEL_M):	*formant1= M_FORMANT1;	*formant2= M_FORMANT2;	*formant3 = M_FORMANT3;	
	/* 'm' */								*f2atten = M_F2ATTEN;	*f3atten = M_F3ATTEN;
					break;
	case(VOWEL_R):	*formant1= R_FORMANT1;	*formant2= R_FORMANT2;	*formant3 = R_FORMANT3;	
	/* dRaws  */							*f2atten = R_F2ATTEN;	*f3atten = R_F3ATTEN;
					break;
	case(VOWEL_TH):	*formant1= TH_FORMANT1;	*formant2= TH_FORMANT2;	*formant3 = TH_FORMANT3;	
	/* dRaws  */							*f2atten = TH_F2ATTEN;	*f3atten = TH_F3ATTEN;
					break;
	default:
		sprintf(errstr,"Unknown vowel\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/************************************ GENERATE_VOWEL_SPECTRUM ************************************
 *
 * "sensitivity" compensates for frq sensitivity of ear at low end, and attenuates
 * formant bamds above c3500.
 */

int generate_vowel_spectrum(double frq,double formant1,double formant2,double formant3,double f2atten,double f3atten,
							double *sensitivity,int senslen,int is_offset,dataptr dz)

{	
	double hfwidth1, hfwidth2, hfwidth3 = 0.0, lolim1, lolim2, lolim3 = 0.0, hilim1, hilim2, hilim3 = 0.0;
	double basefrq = frq, harmfrq = basefrq, thisfrq = basefrq, frq_offset;
	double amp, amp2 = 0.0, amp3 = 0.0, totamp = 0.0;
	int exit_status, cc, vc;
	int overlapped_formants12 = 0, overlapped_formants23 = 0, overlapped_formants13 = 0;
	int is_overlap12, is_overlap23, is_overlap13;
	double toplim;
	int is_third_formant = 0;
	double signal_base = 1.0 - dz->param[PV_PKRANG];

	if(formant3 > 0.0)
		is_third_formant = 1;
	hfwidth1 = formant1 * dz->param[PV_HWIDTH];	/* set limits of formant bands */
	lolim1  = formant1 - hfwidth1;
	hilim1  = formant1 + hfwidth1;
	hfwidth2 = formant2 * dz->param[PV_HWIDTH];
	lolim2  = formant2 - hfwidth2;
	hilim2  = formant2 + hfwidth2;
	if(is_third_formant) {
		hfwidth3 = formant3 * dz->param[PV_HWIDTH];
		lolim3  = formant3 - hfwidth3;
		hilim3  = formant3 + hfwidth3;
	}

 	if(hilim1 > lolim2)		/* deal with overlapping formants */
		overlapped_formants12 = 1;
	if(is_third_formant) {	
		if(hilim2 > lolim3)
			overlapped_formants23 = 1;
		if(hilim1 > lolim3)
			overlapped_formants13 = 1;
	}
	if(is_third_formant)
		toplim =  hilim3;
	else
		toplim =  hilim2;
	while(thisfrq < toplim) {
		amp = 0.0;					/* amplitude will get signal_base * sensitivity */
		is_overlap12 = 0;
		is_overlap23 = 0;
		is_overlap13 = 0;
		if(thisfrq < lolim1) {
			if(flteq(thisfrq,basefrq))
				amp = dz->param[PV_FUNBAS];
		} else if((thisfrq > lolim1) && (thisfrq < hilim1)) {
 			if(overlapped_formants12 && (thisfrq > lolim2)) {
				is_overlap12 = 1;
				if(thisfrq >= formant2)
					amp2 = (hilim2 - thisfrq)/hfwidth2;
				else
					amp2 = (thisfrq - lolim2)/hfwidth2;
				amp2 *= f2atten;
			}
			if(is_third_formant) {
				if(overlapped_formants13 && (thisfrq > lolim3)) {
					is_overlap13 = 1;
					if(thisfrq >= formant3)
						amp3 = (hilim3 - thisfrq)/hfwidth3;
					else
						amp3 = (thisfrq - lolim3)/hfwidth3;
					amp3 *= f3atten;
				}
			}
			if(thisfrq >= formant1)
				amp = (hilim1 - thisfrq)/hfwidth1;
			else
				amp = (thisfrq - lolim1)/hfwidth1;
			if(is_overlap12)
				amp = max(amp,amp2);
			if(is_third_formant && is_overlap13)
				amp = max(amp,amp3);
		} else if((thisfrq > lolim2) && (thisfrq < hilim2)) {
			if(is_third_formant && overlapped_formants23 && (thisfrq > lolim3)) {
				is_overlap23 = 1;
				if(thisfrq >= formant3)
					amp3 = (hilim3 - thisfrq)/hfwidth3;
				else
					amp3 = (thisfrq - lolim3)/hfwidth3;
				amp3 *= f3atten;
			}
			if(thisfrq >= formant2)
				amp = (hilim2 - thisfrq)/hfwidth2;
			else
				amp = (thisfrq - lolim2)/hfwidth2;
			amp *= f2atten;

			if(is_third_formant && is_overlap23)
				amp = max(amp,amp3);
		} else if(is_third_formant && (thisfrq > lolim3)) {
			if(thisfrq >= formant3)
				amp = (hilim3 - thisfrq)/hfwidth3;
			else
				amp = (thisfrq - lolim3)/hfwidth3;
			amp *= f3atten;
		}
		amp = pow(amp,dz->param[PV_CURVIT]);
		amp *= dz->param[PV_PKRANG];
		amp += signal_base;
		if((exit_status = adjust_for_sensitivity(&amp,(double)thisfrq,sensitivity,senslen))<0)
			return(exit_status);
		cc = (int)((thisfrq + dz->halfchwidth)/dz->chwidth);
		vc = cc * 2;
		dz->flbufptr[0][AMPP] = (float)amp;
		dz->flbufptr[0][FREQ] = (float)thisfrq;
		totamp += amp;
		if(is_offset) {
			harmfrq += basefrq; 
			frq_offset = (drand48() - .5) * dz->param[PV_OFFSET] * basefrq;
			if(harmfrq + frq_offset > dz->nyquist)
				thisfrq = harmfrq - frq_offset;
			else
				thisfrq = harmfrq + frq_offset;
		} else
			thisfrq += basefrq;
		if(thisfrq > dz->nyquist) {
			sprintf(errstr,"Error in setting formant: overran nyquist\n");
			return(PROGRAM_ERROR);
		}
	}
	if((exit_status = normalise(VOLUME_PAD,totamp,dz))<0)
		return(exit_status);
	return(FINISHED);
}

/************************************ ADJUST_FOR_SENSITIVITY ************************************/

int adjust_for_sensitivity(double *amp,double frq,double *sensitivity,int senslen)
{
	int n = 0;
	double multiplier, losensfrq, hisensfrq, losens, hisens, frqfrac, sensstep;

	while(frq > sensitivity[n]) {
		n += 2;
		if(n > senslen) {
			sprintf(errstr,"Failed to find sensitivity value (1)\n");
			return(PROGRAM_ERROR);
		}
	}
	hisensfrq = sensitivity[n]; 
	n -= 2;
	if(n < 0) {
		sprintf(errstr,"Failed to find sensitivity value (2)\n");
		return(PROGRAM_ERROR);
	}
	losensfrq = sensitivity[n]; 
	frqfrac = (frq - losensfrq)/(hisensfrq - losensfrq);
	n++;
	losens = sensitivity[n];
	n += 2;
	if(n >= senslen) {
		sprintf(errstr,"Failed to find sensitivity value (3)\n");
		return(PROGRAM_ERROR);
	}
	hisens = sensitivity[n];
	sensstep = hisens - losens;

	multiplier = losens + (sensstep * frqfrac);
	* amp *= multiplier;
	return(FINISHED);
}

/************************************ GENERATE_PITCH ************************************/

int generate_pitch(dataptr dz)
{
	double *times = dz->parray[0];
	double *pitch = dz->parray[1];
	double startpitch, endpitch, pitchstep, thispitch;
	double starttime, endtime, time, timefrac, timestep;
	int n = 0, m = 0, samps_written;

	if((dz->pitches = (float *)malloc(dz->wlength * sizeof(float)))==NULL) {
		sprintf(errstr,"Insufficient memory to store pitch data\n");
		return(MEMORY_ERROR);
	}
	starttime  = times[m];
	startpitch = pitch[m];
	m++;
	endtime  = times[m];
	endpitch = pitch[m];
	m++;
	pitchstep = endpitch - startpitch;
	timestep  = endtime - starttime;
	while(n < dz->wlength) {
		time = n * dz->frametime;
		while(time >= endtime) {	  	/* advance along (MIDI) pitches */
			startpitch = endpitch;
			starttime  = endtime;
			if(m < dz->itemcnt) {
				endtime  = times[m];
				endpitch = pitch[m];
				m++;
			} else
				break;
		}
		if(!flteq(starttime,endtime)) {			   		/* interpolate between pitches : or retain last pitches */
			pitchstep = endpitch - startpitch;
			timestep  = endtime - starttime;
			timefrac  = (time - starttime)/timestep;
			thispitch = (pitchstep * timefrac) + startpitch;
		} else
			thispitch = startpitch;
		dz->pitches[n++] = (float)miditohz(thispitch);
	}
	dz->is_transpos = 0;	
	return write_samps_no_report(dz->pitches,dz->wlength,&samps_written,dz);
}

/********************** REMOVE_PITCH_ZEROS *********************
 *
 * This function removes pitch zeroes (and si;ences) by interpolation.
 */

int remove_pitch_zeros(dataptr dz)
{
	int gotglitch = FALSE, gstart = -1;
	double pstep, pstartval = 0.0;
	int n, m;
	for(n=0;n<dz->wlength;n++) {
		if(gotglitch) {
			if(dz->pitches[n] < MINPITCH)
				continue;
			if(gstart<0) {
				for(m=0; m < n; m++)				   /* Interp to start if ness */
					dz->pitches[m] = dz->pitches[n];
			} else {								   /* Interp between good vals */
				switch(dz->mode) {
				case(PI_GLIDE):
					pstep = (dz->pitches[n] - pstartval)/(double)(n - gstart);
					for(m=gstart+1; m < n; m++) {
						pstartval += pstep;
						dz->pitches[m] = (float)pstartval;
					}						
					break;
				case(PI_SUSTAIN):
					for(m=gstart+1; m < n; m++)
						dz->pitches[m] = (float)pstartval;
					break;
				}
			}
			gotglitch = 0;
		} else {
			if(dz->pitches[n] >= MINPITCH)
				continue;
			gstart = n-1;							
			if(gstart >= 0)
				pstartval = (double)dz->pitches[gstart];
			gotglitch = 1;
		}					
	}
	if(gotglitch) {
		if(gstart < 0) {
			sprintf(errstr,"No pitched data found.");
			return(GOAL_FAILED);
		}
		for(m=gstart+1; m < n; m++)					   /* Interp to end if ness */
			dz->pitches[m] = (float)pstartval;
	}
	return(FINISHED);
}

/*************************** CONVERT_PITCH_FROM_BINARY_TO_TEXT **************************/

int convert_pitch_from_binary_to_text(dataptr dz)
{
	int exit_status;
	int brklen, n, m;

	if((dz->parray[0] = malloc(((dz->wlength + 1) * 2) * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY FOR TEXT DATA\n");
		return MEMORY_ERROR;
	}
	if((exit_status = interpolate_pitch(dz->pitches,0,dz))<0)
		return(exit_status);
	if(dz->wlength == 1) {
		if((exit_status = convert_single_window_pch_or_transpos_data_to_brkpnttable(&brklen,dz->pitches,dz->frametime,0,dz))<0)
			return(exit_status);
	} else {
		if((exit_status = convert_pch_or_transpos_data_to_brkpnttable(&brklen,dz->pitches,dz->frametime,0,dz))<0)
			return(exit_status);
	}
	for(n=0,m=0;n<brklen;n++,m+=2)
		fprintf(dz->fp,"%lf\t%lf\n",dz->parray[0][m],dz->parray[0][m+1]);
	return(FINISHED);
}

/***************** CONVERT_SINGLE_WINDOW_PCH_OR_TRANSPOS_DATA_TO_BRKPNTTABLE ***********************/

int convert_single_window_pch_or_transpos_data_to_brkpnttable(int *brksize,float *floatbuf,float frametime,int array_no,dataptr dz)
{
	double *q;
	float *p = floatbuf;
	int bsize;

	q = dz->parray[array_no];
	*q++ = 0.0;
	*q++ = (double)*p++;
	bsize = q - dz->parray[array_no];

	if((dz->parray[array_no] = (double *)realloc((char *)dz->parray[array_no],bsize*sizeof(double)))==NULL) {
		sprintf(errstr,"convert_single_window_pch_or_transpos_data_to_brkpnttable()\n");
		return(MEMORY_ERROR);
	}
	*brksize = bsize/2;
	return(FINISHED);
}
    
