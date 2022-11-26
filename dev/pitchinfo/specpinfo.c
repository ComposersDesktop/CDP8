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
#include <memory.h>
#include <structures.h>
#include <tkglobals.h>
#include <pnames.h>
#include <globcon.h>
#include <processno.h>
#include <modeno.h>
#include <arrays.h>
#include <specpinfo.h>
#include <cdpmain.h>
#include <formants.h>
#include <speccon.h>
#include <sfsys.h>
#include <osbind.h>
#include <float.h>
#include <specpinfo.h>

static int  do_specpinfo(dataptr dz);
static int  specpzeros(dataptr dz);
static int  prescale_transposition_data(dataptr dz);
static int  generate_noise_in_buffer(double noischan_amp,dataptr dz);
static int  generate_tone_in_buffer(double pre_totalamp,double thisfrq,dataptr dz);
static int  preset_chanfrqs_to_chancentres(dataptr dz);
static int  construct_screen_message(dataptr dz);

/**************************** SPECPINFO *********************/

int specpinfo(dataptr dz)
{
	int exit_status, valid_pitch_data = FALSE;
	int n;
	if(dz->is_transpos) {
		if((dz->transpos = (float *)malloc(dz->wlength * sizeof(float)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for transposition array.\n");
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
	case(P_INFO): 	  	
		if((exit_status = do_specpinfo(dz))<0)
			return(exit_status);
		break;
	case(P_ZEROS):	  	
		if((exit_status = specpzeros(dz))<0)
			return(exit_status);
		break;
	default:
		sprintf(errstr,"Unknown process in specpinfo()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/************************************* DO_SPECPINFO ******************************/

int do_specpinfo(dataptr dz)
{
	int exit_status;
	int n, minpos = -1, maxpos = -1;
	for(n=0;n<dz->wlength;n++) {
		if(dz->pitches[n]<MINPITCH)
			continue;
		if(dz->pitches[n] > dz->param[PINF_MAX]) {
			dz->param[PINF_MAX] = dz->pitches[n];
			maxpos = n;
		}
		if(dz->pitches[n] < dz->param[PINF_MIN]) {
			dz->param[PINF_MIN] = dz->pitches[n];
			minpos = n;
		}
		dz->param[PINF_MEAN] += dz->pitches[n];
	}
	dz->param[PINF_MEAN] /= (double)dz->wlength;
	dz->param[PINF_MAXPOS] = (double)maxpos * dz->frametime;
	dz->param[PINF_MINPOS] = (double)minpos * dz->frametime;
	if(maxpos < 0) {
		sprintf(errstr,"No valid pitch information found.\n");
		return(GOAL_FAILED);
	}
	if(minpos < 0) {
		sprintf(errstr,"No valid pitch below the nyquist frq found.\n");
		return(GOAL_FAILED);
	}
	if((exit_status = construct_screen_message(dz))<0)
		return(exit_status);
	return(FINISHED);
}
			
/***************************** SPECPZEROS *****************/

int specpzeros(dataptr dz)
{
	int n;
	dz->zeroset = FALSE;
	for(n=0;n<dz->wlength;n++) {
		if(dz->pitches[n]<MINPITCH) {
			dz->zeroset = TRUE;
			break;
		}
	}
	if(dz->zeroset)
		sprintf(errstr,"File contains unpitched windows.\n");
	else
		sprintf(errstr,"File does NOT contain unpitched windows.\n");
	return(FINISHED);
}
	
/*************************************** SPECPSEE *************************/

int specpsee(dataptr dz)
{
	int exit_status;
	int wc; 
	double val;
 	if(dz->is_transpos) {
		if((exit_status = prescale_transposition_data(dz))<0)
			return(exit_status);
    	for(wc=0;wc<dz->wlength;wc++) {
			*(dz->sbufptr[0]++) = dz->transpos[wc];
			if(dz->sbufptr[0] >= dz->sampbuf[1]) {
				if((exit_status = write_exact_samps(dz->bigbuf,dz->buflen,dz))<0)
					return(exit_status);
				dz->sbufptr[0] = dz->bigbuf;
			} 
		}
	} else {
		for(wc=0;wc<dz->wlength;wc++) {
			if((val = dz->pitches[wc] * dz->param[PSEE_SCF]) > F_MAXSAMP) {
				if(!dz->zeroset) {
					fprintf(stdout,"WARNING: output value(s) too big: truncated.\n");
					dz->zeroset = TRUE;
				}
				val = F_MAXSAMP;
			}
			if(val < F_MINSAMP) {
				if(!dz->zeroset) {
					fprintf(stdout,"WARNING: output value(s) too small: truncated.\n");
					dz->zeroset = TRUE;
				}
				val = F_MINSAMP;
			}
			*(dz->sbufptr[0]++) = (float)val;
			if(dz->sbufptr[0] >= dz->sampbuf[1]) {
				if((exit_status = write_exact_samps(dz->bigbuf,dz->buflen,dz))<0)
					return(exit_status);
				dz->sbufptr[0] = dz->bigbuf;
			} 
		}
	}
	if(dz->sbufptr[0] != dz->bigbuf) {
		if((exit_status = write_samps(dz->bigbuf,dz->sbufptr[0] - dz->bigbuf,dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}

/*************************** PRESCALE_TRANSPOSITION_DATA ********************/

int prescale_transposition_data(dataptr dz)
{
	int n;
	double scalefact;
	double mintransp = DBL_MAX;
	double maxtransp = -DBL_MAX;
	for(n=0;n<dz->wlength;n++) {
		if(dz->transpos[n] <= 0.0) {
			sprintf(errstr,"Anomaly in transposition data: prescale_transposition_data()\n");
			return(PROGRAM_ERROR);
		} 
		dz->transpos[n] = (float)log10(dz->transpos[n]);
		if(dz->transpos[n] < mintransp)
			mintransp = dz->transpos[n];
		if(dz->transpos[n] > maxtransp)
			maxtransp = dz->transpos[n];
	}
	maxtransp = max(fabs(maxtransp),fabs(mintransp));
	scalefact = F_MAXSAMP/2.0/maxtransp;
	for(n=0;n<dz->wlength;n++)
		dz->transpos[n] = (float)(dz->transpos[n] * scalefact);
	return(FINISHED);
}

/************************* SPECPHEAR *************************/

int specphear(dataptr dz)
{
#define WINDOW_AMP 		(0.50)	/* AVOID OVERLOAD */

	int exit_status;
	double pre_totalamp, noischan_amp;
	double thisfrq;
	int n = 0;
	int vc;
	pre_totalamp = WINDOW_AMP * dz->param[PH_GAIN];

	noischan_amp = pre_totalamp/(double)dz->clength;
	memset((char *)dz->bigfbuf,0,(size_t)(dz->big_fsize * sizeof(float)));
	dz->flbufptr[0] = dz->bigfbuf;
	dz->infile->channels = dz->infile->origchans;	/* setup channel count for outfile */
	dz->wanted  = dz->infile->channels;

	while(n < dz->wlength) {
		thisfrq = (double)dz->pitches[n];
		if(thisfrq < 0.0) {		/* NO PITCH FOUND : GENERATE NOISE */
			if((exit_status = generate_noise_in_buffer(noischan_amp,dz))<0)
				return(exit_status);
		} else {				/* GENERATE TESTTONE AT FOUND PITCH */
			if((exit_status = preset_chanfrqs_to_chancentres(dz))<0)
				return(exit_status);
			if((exit_status = generate_tone_in_buffer(pre_totalamp,thisfrq,dz))<0)
				return(exit_status);
		}
		if(n==0) {
			for(vc = 0; vc < dz->wanted ; vc += 2)
			dz->flbufptr[0][AMPP] = 0.0f;
		}			
		if((dz->flbufptr[0] += dz->wanted) >= dz->flbufptr[1]) {
			if((exit_status = write_exact_samps(dz->bigfbuf,dz->big_fsize,dz))<0)
				return(exit_status);
			memset((char *)dz->bigfbuf,0,(size_t)(dz->big_fsize * sizeof(float)));
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

/*************************** GENERATE_NOISE_IN_BUFFER *********************/

int generate_noise_in_buffer(double noischan_amp,dataptr dz)
{
	int cc, vc;	
	dz->flbufptr[0][0] = (float)noischan_amp;
	dz->flbufptr[0][1] = (float)(drand48() * dz->halfchwidth);
	for(cc = 1, vc = 2; cc < dz->clength-1; cc++, vc += 2) {
		dz->flbufptr[0][FREQ] = (float)((drand48() * dz->chwidth) + dz->windowbuf[CHBOT][cc]);
		dz->flbufptr[0][AMPP] = (float)noischan_amp;
	}
	dz->flbufptr[0][FREQ] = (float)((drand48() * dz->halfchwidth) + dz->windowbuf[CHBOT][cc]);
	dz->flbufptr[0][AMPP] = (float)noischan_amp;
	return(FINISHED);
}

/********************** GENERATE_TONE_IN_BUFFER ***************************/

int generate_tone_in_buffer(double pre_totalamp,double thisfrq,dataptr dz)
{
	int exit_status;
	int cc, vc, m, ampadjusted = 0;
	int lastvc = -1;
	double post_totalamp;
	for(m=0;m<PARTIALS_IN_TEST_TONE;m++) {
		cc = (int)((thisfrq + dz->halfchwidth)/dz->chwidth); 	/* TRUNCATE */
		vc = cc * 2;
		if(vc != lastvc) { /* AVOID OVERWRITING ALREADY GENERATED LOWER PARTIALS */
			dz->flbufptr[0][AMPP] = dz->windowbuf[TESTPAMP][m];
			dz->flbufptr[0][FREQ] = (float)thisfrq;
		}
		lastvc = vc;
		if((thisfrq = thisfrq * 2.0) > dz->nyquist) {
			post_totalamp = (double)dz->windowbuf[TOTPAMP][m];

					  /* THIS NORMALISATION DOESN'T QUITE WORK */
			if((exit_status = normalise(pre_totalamp,post_totalamp,dz))<0)
				return(exit_status);
			ampadjusted = 1; /* IF PITCH IS LESS THAN CHAN SPACING */
			break;	   /* BUT THAT'S MINOR AESTHETIC QUIBBLE ONLY */
		}
	}
	if(!ampadjusted) {
		post_totalamp = (double)dz->windowbuf[TOTPAMP][PARTIALS_IN_TEST_TONE-1];
		if((exit_status = normalise(pre_totalamp,post_totalamp,dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}

/**************************** PRESET_CHANFRQS_TO_CHANCENTRES **********************
 *
 * Preset all frqs to centre of channels.
 */

int preset_chanfrqs_to_chancentres(dataptr dz)
{
	int cc, vc;	
	dz->flbufptr[0][1] = (float)(dz->halfchwidth/2.0);
	for(cc = 1, vc = 2; cc < dz->clength-1; cc++, vc += 2)
		dz->flbufptr[0][FREQ] = (float)(dz->windowbuf[CHBOT][cc] + dz->halfchwidth);
	dz->flbufptr[0][FREQ]=(float)(dz->windowbuf[CHBOT][dz->clength-1]+(dz->halfchwidth/2.0));
	return(FINISHED);
}

/****************************** SPECPWRITE *********************************/

int specpwrite(dataptr dz)
{
	int exit_status;
	int brkcnt;
	if((dz->parray[PW_BRK] = (double *)malloc(dz->wlength * 2 * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for pitch brkpnt array.\n");
		return(MEMORY_ERROR);
	}
	if(dz->is_transpos)	{
		if((exit_status = convert_pch_or_transpos_data_to_brkpnttable(&brkcnt,dz->transpos,dz->frametime,PW_BRK,dz))<0)
			return(exit_status);
	} else {
		if((exit_status = convert_pch_or_transpos_data_to_brkpnttable(&brkcnt,dz->pitches,dz->frametime,PW_BRK,dz))<0)
			return(exit_status);
	}
	return write_brkfile(dz->fp,brkcnt,PW_BRK,dz);
}
    
/****************************** CONSTRUCT_SCREEN_MESSAGE *********************************/

int construct_screen_message(dataptr dz)
{
	int exit_status;
	int oct = 0;
	double z1, z2, z3, z4, z5;
	if((exit_status = hztomidi(&z1,dz->param[PINF_MAX]))<0)
		return(exit_status);
	if((exit_status = hztomidi(&z2,dz->param[PINF_MIN]))<0)
		return(exit_status);
	if((exit_status = hztomidi(&z3,dz->param[PINF_MEAN]))<0)
		return(exit_status);
	if((exit_status = hztomidi(&z4,dz->param[PINF_MAX]))<0)
		return(exit_status);
	if((exit_status = hztomidi(&z5,dz->param[PINF_MIN]))<0)
		return(exit_status);
	z4 -= z5;
	if(z4 > 12.0)  {
		oct = (int)(z4/12.0); /* TRUNCATE */
		z4  -= (double)(oct * 12);
	}
	if(oct > 0)	{
		sprintf(errstr,"MAX PITCH : %.2lfHZ\tMIDI : %.2lf\tTIME %.2lf\n"			
				"MIN PITCH : %.2lfHZ\tMIDI : %.2lf\tTIME %.2lf\n"			
				"MEAN PITCH: %.2lfHZ\tMIDI : %.2lf\n\n"			
				"TOTAL RANGE: %d OCTAVES and %.2lf SEMITONES\n",
				dz->param[PINF_MAX],z1,dz->param[PINF_MAXPOS],dz->param[PINF_MIN],z2,dz->param[PINF_MINPOS],
				dz->param[PINF_MEAN],z3, oct, z4);			
	} else {
		sprintf(errstr,"MAX PITCH : %.2lfHZ\tMIDI : %.2lf\tTIME %.2lf\n"			
				"MIN PITCH : %.2lfHZ\tMIDI : %.2lf\tTIME %.2lf\n"			
				"MEAN PITCH: %.2lfHZ\tMIDI : %.2lf\n\n"			
				"TOTAL RANGE: %.2lf SEMITONES\n",
				dz->param[PINF_MAX],z1,dz->param[PINF_MAXPOS],dz->param[PINF_MIN],z2,dz->param[PINF_MINPOS],
				dz->param[PINF_MEAN],z3,z4);			
	}
	return(FINISHED);
}
