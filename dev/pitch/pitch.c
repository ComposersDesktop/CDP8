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
#include <pnames.h>
#include <structures.h>
#include <tkglobals.h>
#include <globcon.h>
#include <modeno.h>
#include <arrays.h>
#include <flags.h>
#include <pitch.h>
#include <cdpmain.h>
#include <formants.h>
#include <speccon.h>
#include <sfsys.h>
#include <pitch.h>
#include <memory.h>

static int  do_spectral_shiftp(dataptr dz);
static int  replace_partial_by_tuned_partial(int cc,int vc,dataptr dz);
static int  focus_partial_towards_tuned_value(int vc, int cc,dataptr dz);
static int  gotnewfrq(int *newvc,int vc,double *thisamp,double *thisfrq, double thisspecamp,int n, dataptr dz);
static int  gotnewfrq2(int *newvc,int vc,double *thisamp,double *thisfrq,int n, dataptr dz);
static int  remove_unwanted_frq_areas(double lofrq_limit,double hifrq_limit,dataptr dz);

/********************************** SPECALT **********************************/
 
int specalt(int *pitchcnt,dataptr dz)
{
	int n = 1, cc, vc;
	int exit_status;				/* RWD: for specbare() retval ! */
	double pitch;
	if((exit_status = specbare(pitchcnt,dz))<0)		 
		return exit_status;
	(*pitchcnt)--;
	pitch = dz->pitches[(*pitchcnt)++];
	if(pitch < 0.0)				/*   UNPITCHED WINDOW : IGNORE */
		return(FINISHED);
	switch(dz->mode) {
	case(DELETE_ODD):
		for( cc = 0 ,vc = 0; cc < dz->clength; cc++, vc += 2) {
			if(dz->flbufptr[0][vc]>0.0) {
				if(ODD(n))
					dz->flbufptr[0][vc] = 0.0F;
				n++;
			}
		}
		break;
	case(DELETE_EVEN):
		for( cc = 0 ,vc = 0; cc < dz->clength; cc++, vc += 2)  {
			if(dz->flbufptr[0][vc]>0.0) {
				if(EVEN(n))
					dz->flbufptr[0][vc] = 0.0F;
				n++;
			}
		}
		break;
	default:
		sprintf(errstr,"Invalid option in specalt()\n");
		return(PROGRAM_ERROR);
	break;
	}
	return(FINISHED);
}


/**************************** SPECOCT ****************************
 *
 * (1)	TRANSPOSE UPWARDS.
 *		(a)	Delete all channels that are NOT harmonics.
 *		(b)	Delete all harmonics that are not EVEN (or *3 or *4 etc) harmonics.
 * (2)	TRANSPOSE DOWNWARDS.
 *		Insert harmonics below and between the original fundamental and harmonics.
 */
 	
int specoct(dataptr dz)
{
	int exit_status;
	double pre_totalamp, post_totalamp, thisfrq, basefrq, baseamp, thisamp;
	int n, specenvcnt, vc, truecc, truevc;
	if((thisfrq = dz->pitches[dz->total_windows]) <= 0.0)
		return(FINISHED);
	if((exit_status = get_totalamp(&pre_totalamp,dz->flbufptr[0],dz->wanted))<0)
		return(exit_status);
	switch(dz->mode) {
	case(OCT_UP):		/* UPWARDS: DELETING ALTERNATE HARMONICS */
		thisfrq *= (double)dz->iparam[OCT_HMOVE];
		for(vc = 0; vc < dz->wanted; vc += 2) {
			if((exit_status = is_harmonic(&n,(double)dz->flbufptr[0][FREQ],thisfrq))<0)
				return(exit_status);
			if(exit_status==FALSE)
				dz->flbufptr[0][AMPP] = 0.0f;
		}
		break;
	case(OCT_DN):		/* DNWARDS: INSERT INTERVENING HARMONICS */
	case(OCT_DN_BASS):	/* DITTO WITH BASS REINFORCE */
		if((exit_status = extract_specenv_over_partials(&specenvcnt,thisfrq,0,dz))<0)
			return(exit_status);
		basefrq = thisfrq/(double)dz->iparam[OCT_HMOVE];
		if((exit_status = getspecenvamp(&baseamp,basefrq,0,dz))<0)
			return(exit_status);
		thisfrq = basefrq;
		n = 1;
		while(thisfrq < dz->nyquist) {
			if((exit_status = get_channel_corresponding_to_frq(&truecc,thisfrq,dz))<0)
				return(exit_status);
			truevc = truecc * 2;
			if(dz->mode==OCT_DN_BASS && (thisfrq < basefrq))
				dz->flbufptr[0][truevc] = 
				(float)(dz->param[OCT_BREI] - ((dz->param[OCT_BREI] - 1.0)*((double)n/(double)dz->iparam[OCT_HMOVE])));
			else {
				if((exit_status = getspecenvamp(&thisamp,thisfrq,0,dz))<0)
					return(exit_status);
				dz->flbufptr[0][truevc] = (float)thisamp;
			}
			truevc++;
			dz->flbufptr[0][truevc]   = (float)thisfrq;
			do {
				n++;
			} while(n%dz->iparam[OCT_HMOVE]==0);
			thisfrq = basefrq * (double)n;
		}
		break;
	default:
		sprintf(errstr,"Unknown mode in specoct()\n");
		return(PROGRAM_ERROR);
	}					/* NORMALISE */
	if((exit_status = get_totalamp(&post_totalamp,dz->flbufptr[0],dz->wanted))<0)
		return(exit_status);
	return normalise(pre_totalamp,post_totalamp,dz);
}														 

/****************************** SPECSHIFTP ****************************
 *
 * transpose (part of) spectrum.
 */

int specshiftp(dataptr dz)
{
	int exit_status;
	if(dz->vflag[SHP_IS_DEPTH] && flteq(dz->param[SHIFTP_DEPTH],0.0))
		return(FINISHED);
	if(dz->brksize[SHIFTP_FFRQ] || dz->brksize[SHIFTP_SHF1] || dz->brksize[SHIFTP_SHF2]) {
		convert_shiftp_vals(dz);
		if((exit_status = adjust_params_and_setup_internal_params_for_shiftp(dz))<0)
			return(exit_status);
	}
	if((exit_status = get_amp_and_frq(dz->flbufptr[0],dz))<0)
		return(exit_status);		
	if(dz->vflag[SHP_IS_DEPTH])  {
		if((exit_status = reset_shiftps_according_to_depth_value(dz))<0)
			return(exit_status);
	}
	if((exit_status = do_spectral_shiftp(dz))<0)
		return(exit_status);
	if((exit_status = put_amp_and_frq(dz->flbufptr[0],dz))<0)
		return(exit_status);
	return(FINISHED);
}

/************************ DO_SPECTRAL_SHIFTP **********************/

int do_spectral_shiftp(dataptr dz)
{
	double shft = dz->param[SHIFTP_NS1];
	int   j, k, kk;
	switch(dz->mode)  {
	case(P_OCT_UP):	 /* MOVE SPECTRUM, above fdcno, UP AN OCTAVE */
	case(P_OCT_UP_AND_DN):
		k = dz->iparam[SHIFTP_FDCNO]*2;
		for(j = dz->clength-1; j>= k; j += -2) {
			dz->amp[j]    = dz->amp[j/2];
			dz->freq[j]   = (float)(2.0 * dz->freq[j/2]);
			dz->amp[j-1]  = 0.0f;
			dz->freq[j-1] = 0.0f;
		}
		for(j = dz->iparam[SHIFTP_FDCNO]; j<k; j++) {
			dz->amp[j]    = 0.0f;
			dz->freq[j]   = 0.0f;
		}
		if(dz->mode==P_OCT_UP)	
	    		break;
  	case(P_OCT_DN):	 /* MOVE SPECTRUM, below fdcno, DOWN AN OCTAVE */
		kk = dz->iparam[SHIFTP_FDCNO]-1;
		j = round((double)kk/2.0);
		if( kk != (2*j))
			kk = kk-1;
		for(j = 0; j <= kk; j += 2) {
			k = j/2;	
			dz->amp[k]    = dz->amp[j];
			dz->freq[k]   = (float)(dz->freq[j]/2.0);
			dz->amp[k+1]  = 0.0f;
			dz->freq[k+1] = 0.0f;
		}
		kk = kk/2;
		kk = kk+2;
		for(j = kk; j<dz->iparam[SHIFTP_FDCNO]; j++) {
			dz->amp[j]  = 0.0f;
			dz->freq[j] = 0.0f;
		}
		break;
	case(P_SHFT_UP):	 /* MOVE SPECTRUM, above fdcno, UP BY SHIFT */
	case(P_SHFT_UP_AND_DN):
		if( shft > 1.0) {
			j = dz->clength-1;
			k  = round((double)j/shft);
			while( k >= dz->iparam[SHIFTP_FDCNO]) {
				dz->amp[j]  = dz->amp[k];
				dz->freq[j] = (float)(shft * dz->freq[k]);
				j-- ;
				k  = round((double)j/shft);
			}
			for( k=j; k>dz->iparam[SHIFTP_FDCNO];k-- ) {
				dz->amp[k]  = 0.0f;
				dz->freq[k] = 0.0f;
			}				
		} else {
			j = dz->iparam[SHIFTP_FDCNO];
			k  = round((double)j/shft);
			while( k <= (dz->clength-1)) {
				dz->amp[j]  = dz->amp[k];
				dz->freq[j] = (float)(shft * dz->freq[k]);
				j++ ;
				k  = round((double)j/shft);
			}
			for( k=j; k<dz->clength; k++ ) {
				dz->amp[k]  = 0.0f;
				dz->freq[k] = 0.0f;
			}				
		}
		if(dz->mode==P_SHFT_UP)
			break;
		else
			shft = dz->param[SHIFTP_NS2];
	case(P_SHFT_DN):   /* MOVE SPECTRUM, below fdcno, DOWN BY SHIFT */
		if( shft > 1.0) {
			j = dz->iparam[SHIFTP_FDCNO]-1; 
			k = round((double)j/shft);
			while( k > 0) {
				dz->amp[j]  = dz->amp[k];
				dz->freq[j] = (float)(shft * dz->freq[k]);
				j-- ;
				k = round((double)j/shft);
			}
			for( k=j; k>0 ;k-- ) {
				dz->amp[k]  = 0.0f;	  /* AMP AND FREQ AT 0 Hz ARE RETAINED */
				dz->freq[k] = 0.0f;
			}				
		} else {
			j = 1;
			k = round((double)j/shft);
			while( k <= dz->iparam[SHIFTP_FDCNO]-1) {
				dz->amp[j]  = dz->amp[k];
				dz->freq[j] = (float)(shft * dz->freq[k]);
				j++ ;
				k = round((double)j/shft);
			}
			for( k=j; k<dz->iparam[SHIFTP_FDCNO]; k++ ) {
				dz->amp[k]  = 0.0f;
				dz->freq[k] = 0.0f;
			}				
		}
		break;
	default:
		sprintf(errstr,"unknown mode in do_spectral_shiftp()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/**********************************  SPECTUNE ********************************/

int spectune(dataptr dz)
{
	int exit_status;
	chvptr loudest;
	int cc, vc;
	if(dz->vflag[TUNE_FBOT]) {
		for(cc = 0, vc = 0; vc < dz->wanted; cc++, vc += 2) {
			if(dz->flbufptr[0][FREQ] < dz->param[TUNE_BFRQ])
				dz->flbufptr[0][AMPP] = 0.0f;
		}
	}
	if(dz->vflag[TUNE_TRACE]) {
		if((exit_status = initialise_ring_vals(dz->iparam[TUNE_INDX],-1.0,dz))<0)
			return(exit_status);
		for(cc = 0, vc = 0; vc < dz->wanted; cc++, vc += 2) {
			if((exit_status = if_one_of_loudest_chans_store_in_ring(vc,dz))<0)
				return(exit_status);
		}

		memset((char *)(dz->iparray[TUNE_LOUD]),0,dz->clength * sizeof(int));
		loudest = dz->ringhead;
		do {
			dz->iparray[TUNE_LOUD][loudest->loc] = 1;
		}  while((loudest = loudest->next)!=dz->ringhead);
		for(cc=0,vc=0;cc<dz->clength;cc++,vc+=2) {
			if(dz->iparray[TUNE_LOUD][cc]) {
				if((exit_status = replace_partial_by_tuned_partial(cc,vc,dz))<0)
					return(exit_status);
			} else
				dz->flbufptr[0][vc] = (float)(dz->flbufptr[0][vc] * dz->param[TUNE_CLAR]);
		}
	} else {
		for(cc = 0, vc = 0; vc < dz->wanted; cc++, vc += 2) {
			if(dz->frq_template[cc]>0.0) {
				if((exit_status = focus_partial_towards_tuned_value(FREQ,cc,dz))<0)
					return(exit_status);
			} else
				dz->flbufptr[0][AMPP] = (float)(dz->flbufptr[0][AMPP] * dz->param[TUNE_CLAR]);
		}
	}
	return(FINISHED);
}

/************************** REPLACE_PARTIAL_BY_TUNED_PARTIAL *******************************/

int replace_partial_by_tuned_partial(int cc,int vc,dataptr dz)
{
	int exit_status;
	int aa = cc, bb = cc, here, there;
	double upratio, dnratio;
	if(dz->frq_template[cc]>0.0) 						/* if loudchan is a validfrq chan */
		return focus_partial_towards_tuned_value(FREQ,cc,dz);	
														/* when loudchan NOT validfrq chan */
	while(aa>=0 && dz->frq_template[aa]<=0.0)			/* search downwards */
		aa--;				
	while(bb<dz->clength && dz->frq_template[bb]<=0.0) 	/* and upwards */
		bb++;								 			/* for closest validfrq chans */
	if(aa<0 && bb<dz->clength) {	
		here = bb*2;					  				/* if none below, use one above */
		aa   = bb;
	} else if(bb>=dz->clength && aa>=0) {				/* if none above, use one below */
		here  = aa*2;
	} else {			
		here  = aa*2;	  								/* otherwise, choose nearest validfrq */
		there = bb*2;
		upratio = dz->flbufptr[0][there+1]/dz->flbufptr[0][FREQ];
		dnratio = dz->flbufptr[0][FREQ]/dz->flbufptr[0][here+1];
		if(upratio < dnratio) {
			here = there;
			aa   = bb;
		}
	}				
	if((exit_status = focus_partial_towards_tuned_value(here+1,aa,dz))<0)
		return(exit_status);
	dz->flbufptr[0][here] = dz->flbufptr[0][AMPP];  		/* and gets ampl of nearest loudchan,    */
	dz->flbufptr[0][AMPP] = 0.0F;	    				/* while actual loudchan is zeroed out.  */
	return(FINISHED);
}

/********************************* FOCUS_PARTIAL_TOWARDS_TUNED_VALUE *******************************/

int focus_partial_towards_tuned_value(int vc, int cc,dataptr dz)
{
	double frqshift;
	frqshift  = dz->frq_template[cc] - dz->flbufptr[0][vc];
	frqshift *= dz->param[TUNE_FOC]; 
	dz->flbufptr[0][vc] = (float)(dz->flbufptr[0][vc] + frqshift);
	return(FINISHED);
}

/****************************** SPECPICK ****************************/

int specpick(dataptr dz)
{
	int exit_status;
	int mask = 1;
	int cc, vc, bflagno;
	for( cc = 0 ,vc = 0; cc < dz->clength; cc++, vc += 2)  {
		if((exit_status = choose_bflagno_and_reset_mask_if_ness
		(&bflagno,cc,&mask,dz->iparam[PICK_LONGPOW2],dz->iparam[PICK_DIVMASK]))<0)
			return(exit_status);
		if(!(dz->lparray[PICK_BFLG][bflagno] & mask)) 		/* if bit NOT set, modify amplitude */
			dz->flbufptr[0][AMPP] = (float)(dz->flbufptr[0][AMPP] * dz->param[PICK_CLAR]);
		mask <<= 1;										/* move bitmask upwards */
	}		
	return(FINISHED);
}

/***************************** SPECCHORD2 *********************************/

int specchord2(dataptr dz)
{
	int exit_status;
	int newvc, cc, vc, n;
	double pre_amptotal, post_amptotal;
	double thisamp, thisfrq;
	double hifrq = dz->param[CHORD_HIFRQ];
	double lofrq = dz->param[CHORD_LOFRQ];
	if(dz->brksize[CHORD_HIFRQ] || dz->brksize[CHORD_LOFRQ]) {
		if(hifrq < lofrq)
			swap(&hifrq,&lofrq);
	}
	if((exit_status = get_totalamp(&pre_amptotal,dz->flbufptr[0],dz->wanted))<0)
		return(exit_status);
	for(vc = 0; vc < dz->wanted; vc += 2)
		dz->windowbuf[0][AMPP] = 0.0F;
	for( cc = 0 ,vc = 0; cc < dz->clength; cc++, vc += 2) {
		for(n=0;n<dz->itemcnt;n++) {	  /* for each note of chord */
			if((exit_status = gotnewfrq2(&newvc,vc,&thisamp,&thisfrq,n,dz))<0)
				return(exit_status);
			if(exit_status==FALSE)
				continue;
			switch(dz->vflag[CHORD_BODY]) {
			case(CHD_LESSFUL):
				if((exit_status = move_data_into_appropriate_channel(vc,newvc,(float)thisamp,(float)thisfrq,dz))<0)
					return(exit_status);
				break;
			case(CHD_NORMAL):  
				if((exit_status = move_data_into_some_appropriate_channel(newvc,(float)thisamp,(float)thisfrq,dz))<0)
					return(exit_status);
				break;
			default:
				sprintf(errstr,"Unknown case in dz->vflag[CHROD_BODY]: specchord()\n");
				return(PROGRAM_ERROR); 
			}
		}
	}
	for(vc = 0; vc < dz->wanted; vc+=2)
		dz->flbufptr[0][AMPP] = dz->windowbuf[0][AMPP];
	if(dz->vflag[CHORD_FBOT] || dz->vflag[CHORD_FTOP]) {
		if((exit_status = remove_unwanted_frq_areas(lofrq,hifrq,dz))<0)
			return(exit_status);
	}
	if((exit_status = get_totalamp(&post_amptotal,dz->flbufptr[0],dz->wanted))<0)
		return(exit_status);
	return normalise(pre_amptotal,post_amptotal,dz);
}

/*************************** GOTNEWFRQ2 *************************/

int gotnewfrq2(int *newvc,int vc,double *thisamp,double *thisfrq,int n, dataptr dz)
{
	int newcc;
	if((*thisfrq = fabs(dz->flbufptr[0][FREQ] * dz->transpos[n])) > dz->nyquist)
		return(FALSE);
	*thisamp = dz->flbufptr[0][AMPP];
	if((newcc  = (int)((*thisfrq + dz->halfchwidth)/dz->chwidth)) >= dz->clength) /* TRUNCATE */
		return(FALSE);
	*newvc = newcc * 2; 
	return(TRUE);
}

/************************* REMOVE_UNWANTED_FRQ_AREAS ***************************/

int remove_unwanted_frq_areas(double lofrq_limit,double hifrq_limit,dataptr dz)
{
	int cc, vc;
	if(dz->vflag[CHORD_FBOT]) {
		if(dz->vflag[CHORD_FTOP]) {
			for(cc = 0,vc = 0; cc < dz->clength; cc++, vc += 2) {
				if(dz->flbufptr[0][FREQ] < lofrq_limit || dz->flbufptr[0][FREQ] > hifrq_limit)
					dz->flbufptr[0][AMPP] = 0.0f;
			}
		} else {
			for(cc = 0,vc = 0; cc < dz->clength; cc++, vc += 2) {
				if(dz->flbufptr[0][FREQ] < lofrq_limit)
					dz->flbufptr[0][AMPP] = 0.0f;
			}
		}
	} else if(dz->vflag[CHORD_FTOP]) {
		for(cc = 0,vc = 0; cc < dz->clength; cc++, vc += 2) {
			if(dz->flbufptr[0][FREQ] > hifrq_limit)
				dz->flbufptr[0][AMPP] = 0.0f;
		}
	}
	return(FINISHED);
}

/***************************** SPECCHORD *********************************/

int specchord(dataptr dz)
{
	int exit_status;
	int newvc, cc, vc, n;
	double pre_amptotal, post_amptotal;
	double thisamp, thisfrq, thisspecamp;
	double hifrq = dz->param[CHORD_HIFRQ];
	double lofrq = dz->param[CHORD_LOFRQ];
	if(dz->brksize[CHORD_HIFRQ] || dz->brksize[CHORD_LOFRQ]) {
		if(hifrq < lofrq)
			swap(&hifrq,&lofrq);
	}
	if((exit_status = extract_specenv(0,0,dz))<0)
		return(exit_status);
	if((exit_status = get_totalamp(&pre_amptotal,dz->flbufptr[0],dz->wanted))<0)
		return(exit_status);
	for(vc = 0; vc < dz->wanted; vc += 2)
		dz->windowbuf[0][AMPP] = 0.0F;
	for( cc = 0 ,vc = 0; cc < dz->clength; cc++, vc += 2) {
		if((exit_status = getspecenvamp(&thisspecamp,(double)dz->flbufptr[0][FREQ],0,dz))<0)
			return(exit_status);
		if(thisspecamp < VERY_TINY_VAL)
			continue;
		for(n=0;n<dz->itemcnt;n++) {	  /* for each note of chord */
			if((exit_status = gotnewfrq(&newvc,vc,&thisamp,&thisfrq,thisspecamp,n,dz))<0)
				return(exit_status);
			if(exit_status==FALSE)
				continue;
			switch(dz->vflag[CHORD_BODY]) {
			case(CHD_LESSFUL):
				if((exit_status = move_data_into_appropriate_channel(vc,newvc,(float)thisamp,(float)thisfrq,dz))<0)
					return(exit_status);
				break;
			case(CHD_NORMAL):  
				if((exit_status = move_data_into_some_appropriate_channel(newvc,(float)thisamp,(float)thisfrq,dz))<0)
					return(exit_status);
				break;
			default:
				sprintf(errstr,"Unknown case in dz->vflag[CHROD_BODY]: specchord()\n");
				return(PROGRAM_ERROR); 
			}
		}
	}
	for(vc = 0; vc < dz->wanted; vc+=2)
		dz->flbufptr[0][AMPP] = dz->windowbuf[0][AMPP];
	if(dz->vflag[CHORD_FBOT] || dz->vflag[CHORD_FTOP]) {
		if((exit_status = remove_unwanted_frq_areas(lofrq,hifrq,dz))<0)
			return(exit_status);
	}
	if((exit_status = get_totalamp(&post_amptotal,dz->flbufptr[0],dz->wanted))<0)
		return(exit_status);
	return normalise(pre_amptotal,post_amptotal,dz);
}

/*************************** GOTNEWFRQ *************************/

int gotnewfrq(int *newvc,int vc,double *thisamp,double *thisfrq, double thisspecamp,int n, dataptr dz)
{
	int exit_status;
	int newcc;
	double newspecamp;
	if((*thisfrq = fabs(dz->flbufptr[0][FREQ] * dz->transpos[n])) > dz->nyquist)
		return(FALSE);
	if((exit_status = getspecenvamp(&newspecamp,(double)(*thisfrq),0,dz))<0)
		return(exit_status);
	if(newspecamp < VERY_TINY_VAL)
		return(FALSE);
	if((*thisamp = dz->flbufptr[0][AMPP]*newspecamp/thisspecamp)<VERY_TINY_VAL)
		return(FALSE);
	if((newcc  = (int)((*thisfrq + dz->halfchwidth)/dz->chwidth)) >= dz->clength) /* TRUNCATE */
		return(FALSE);
	*newvc = newcc * 2; 
	return(TRUE);
}
