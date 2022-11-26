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



/* floatsam version TW */
#include <stdio.h>
#include <stdlib.h>
#include <structures.h>
#include <tkglobals.h>
#include <pnames.h>
#include <globcon.h>
#include <processno.h>
#include <modeno.h>
#include <arrays.h>
#include <flags.h>
#include <highlight.h>
#include <cdpmain.h>
#include <formants.h>
#include <speccon.h>
#include <sfsys.h>
#include <osbind.h>
#include <highlight.h>
//TW UPDATE
#include <vowels.h>

#if defined unix || defined __GNUC__
#define round(x) lround((x))
#endif

static int  do_filtering(double frq_limit,double hifrq_limit,dataptr dz);
static int  below_limitfrq_and_zeroed(int vc,double loskirt,dataptr dz);
static int  below_limitfrq_and_filtered(int vc,double limitfrq,double loskirt,double loskirt_bwidth,dataptr dz);
static int  above_limitfrq_and_zeroed(int vc,double hiskirt,dataptr dz);
static int  filter_above_limitfrq(int vc,double limitfrq,double hiskirt,double hiskirt_bwidth,dataptr dz);
static int  filtered_in_lower_skirt(int vc,double skirttop,double loskirt,double loskirt_bwidth,dataptr dz);
static int  filtered_in_upper_skirt(int vc,double skirt_bottom,double hiskirt,double hiskirt_bwidth,dataptr dz);
static int  do_greq_filter(dataptr dz);
static int  invert_greq_filter_envelope(dataptr dz);
static int  process_specsplit_channel(int cc,bandptr bb,dataptr dz);
static int  do_on(double bandhilimit,double bandlolimit,dataptr dz);
static int  do_below(double bandmid,dataptr dz);
static int  do_above(double bandmid,dataptr dz);
static int  do_boost(double bandhilimit,double bandlolimit,dataptr dz);
static int  do_above_boost(double bandhilimit,double bandlolimit,int *in_start_portion, dataptr dz);
static int  do_below_boost(double bandhilimit,double bandlolimit,int *in_start_portion,dataptr dz);
static int  do_once_below(double bandlolimit,int *in_start_portion,dataptr dz);
static int  do_once_above(double bandhilimit,int *in_start_portion,dataptr dz);
static int  initiate_new_arpeg_note(int cc,int vc,dataptr dz);
static int  initiate_new_arpeg_note_with_fixed_pitch(int cc,int vc,dataptr dz);
static int  temporarily_sustain_arpeg_note(int cc,double thisamp,dataptr dz);
static int  temporarily_sustain_arpeg_note_with_fixed_pitch(int cc,int vc,double thisamp,dataptr dz);
static int  get_non_linear_decimation(double *nonlin_dec,int cc,dataptr dz);
static int  get_decimation(double *dec,int cc,dataptr dz);
static int  sustain_arpeg_note(int cc,int vc,dataptr dz);
static int  sustain_arpeg_note_with_fixed_pitch(int cc,int vc,dataptr dz);
static int  nonlin_sustain_arpeg_note(int cc,int vc, dataptr dz);
static int  nonlin_sustain_arpeg_note_with_fixed_pitch(int cc, int vc, dataptr dz);
static int  set_bit_to_zero(int bflagno,int mask,dataptr dz);
static int  set_bit_to_one(int bflagno,int mask,dataptr dz);
static int  outside_skirts(int vc,double loskirt,double hiskirt,dataptr dz);
static int  reset_timechanging_arpe_variables(double *frqrange,double *lofrq, double *hifrq,dataptr dz);
static int  locate_current_wavetable_position(dataptr dz);
//TW UPDATE
static int    do_vowel_filter(double *vamp,double formant1,double formant2,double formant3,double f2atten,double f3atten,
							double *sensitivity,int senslen,dataptr dz);
static int get_formant_frqs
(int vowel,double *formant1, double *formant2, double *formant3, double *f2atten, double *f3atten);
static int define_sensitivity_curve(double **sensitivity,int *senslen);
static int adjust_for_sensitivity(double *amp,double frq,double *sensitivity,int senslen);



/**************************** SPECFILT ***************************/

int specfilt(dataptr dz)
{
	int exit_status;
	int vc;
	double frq_limit, hifrq_limit = 0.0, pre_amptotal, post_amptotal;
	if(dz->brksize[FILT_QQ])
		dz->param[FILT_QQ] 	 = exp(dz->param[FILT_QQ]);
	if(dz->brksize[FILT_FRQ1])
		frq_limit = exp(dz->param[FILT_FRQ1]);
	else
		frq_limit = dz->param[FILT_FRQ1];
	switch(dz->mode) {
	case(F_BND):
	case(F_BND_NORM):
	case(F_NOTCH):
	case(F_NOTCH_NORM):
	case(F_BAND_GAIN):
	case(F_NOTCH_GAIN):
		if(dz->brksize[FILT_FRQ2])
			hifrq_limit = exp(dz->param[FILT_FRQ2]);
		else
			hifrq_limit = dz->param[FILT_FRQ2];
		if(frq_limit > hifrq_limit)
			swap(&frq_limit,&hifrq_limit);
		break;
	}
	if((exit_status = get_totalamp(&pre_amptotal,dz->flbufptr[0],dz->wanted))<0)
			return(exit_status);
	if((exit_status = do_filtering(frq_limit,hifrq_limit,dz))<0)
		return(exit_status);
	if((exit_status = get_totalamp(&post_amptotal,dz->flbufptr[0],dz->wanted))<0)
			return(exit_status);
	switch(dz->mode) {
	case(F_HI_NORM): 
	case(F_LO_NORM): 
	case(F_BND_NORM): 
	case(F_NOTCH_NORM):   /* normalised output */
		if((exit_status = normalise(pre_amptotal,post_amptotal,dz))<0)
			return(exit_status);
		break;
	case(F_HI_GAIN): 
	case(F_LO_GAIN): 
	case(F_BAND_GAIN): 
	case(F_NOTCH_GAIN):	/*  post-gain */
		for(vc = 0; vc < dz->wanted; vc += 2)
			dz->flbufptr[0][AMPP] = (float)(dz->flbufptr[0][AMPP] * dz->param[FILT_PG]);
		break;
	}
	return(FINISHED);
}

/**************************** DO_FILTERING ***************************/

int do_filtering(double frq_limit,double hifrq_limit,dataptr dz)
{
	int exit_status;
	int vc;											   
	double loskirt, loskirt_bwidth, hiskirt, hiskirt_bwidth;
	switch(dz->mode) {
	case(F_HI): 
	case(F_HI_NORM): 
	case(F_HI_GAIN):
		loskirt        = max(0.0,(frq_limit - dz->param[FILT_QQ]));
		loskirt_bwidth = frq_limit - loskirt;
		for(vc = 0; vc < dz->wanted; vc += 2) {
			if((exit_status = below_limitfrq_and_zeroed(vc,loskirt,dz))<0)
				return(exit_status);
			if(exit_status==TRUE)
				continue;
			if((exit_status = below_limitfrq_and_filtered(vc,frq_limit,loskirt,loskirt_bwidth,dz))<0)
				return(exit_status);
		}		    
		break;
	case(F_LO): 
	case(F_LO_NORM): 
	case(F_LO_GAIN):
		hiskirt        = min(dz->nyquist,(frq_limit + dz->param[FILT_QQ]));
		hiskirt_bwidth = hiskirt - frq_limit;
		for(vc = 0; vc < dz->wanted; vc += 2) {
			if((exit_status = above_limitfrq_and_zeroed(vc,hiskirt,dz))<0)
				return(exit_status);
			if(exit_status==TRUE)
				continue;
			if((exit_status = filter_above_limitfrq(vc,frq_limit,hiskirt,hiskirt_bwidth,dz))<0)
				return(exit_status);
		}		    
		break;
	case(F_BND): 
	case(F_BND_NORM): 
	case(F_BAND_GAIN):
		loskirt        = max(0.0,(frq_limit - dz->param[FILT_QQ]));
		loskirt_bwidth = frq_limit - loskirt;
		hiskirt        = min(dz->nyquist,(hifrq_limit + dz->param[FILT_QQ]));
		hiskirt_bwidth = hiskirt - hifrq_limit;
		for(vc = 0; vc < dz->wanted; vc += 2) {
			if((exit_status = below_limitfrq_and_zeroed(vc,loskirt,dz))<0)
				return(exit_status);
			if(exit_status==TRUE)
				continue;
			if((exit_status = below_limitfrq_and_filtered(vc,frq_limit,loskirt,loskirt_bwidth,dz))<0)
				return(exit_status);
			if(exit_status == TRUE) 	
				continue;
			if((exit_status = above_limitfrq_and_zeroed(vc,hiskirt,dz))<0)
				return(exit_status);
			if(exit_status == TRUE) 	
				continue;
			if((exit_status = filter_above_limitfrq(vc,hifrq_limit,hiskirt,hiskirt_bwidth,dz))<0)
				return(exit_status);
		}		    
		break;
	case(F_NOTCH): 
	case(F_NOTCH_NORM): 
	case(F_NOTCH_GAIN):
		loskirt        = max(0.0,(frq_limit - dz->param[FILT_QQ]));
		loskirt_bwidth = frq_limit - loskirt;
		hiskirt        = min(dz->nyquist,(hifrq_limit + dz->param[FILT_QQ]));
		hiskirt_bwidth = hiskirt - hifrq_limit;
		for(vc = 0; vc < dz->wanted; vc += 2) {
			if(outside_skirts(vc,loskirt,hiskirt,dz))
				continue;
			if((exit_status = filtered_in_lower_skirt(vc,frq_limit,loskirt,loskirt_bwidth,dz))<0)
				return(exit_status);
			if(exit_status==TRUE)	  
				continue;
			if((exit_status = filtered_in_upper_skirt(vc,hifrq_limit,hiskirt,hiskirt_bwidth,dz))<0)
				return(exit_status);
			if(exit_status==TRUE)	  
				continue;
			dz->flbufptr[0][AMPP] = 0.0F;
		}		    
		break;
	default:
		sprintf(errstr,"unknown mode in do_filtering()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/**************************** BELOW_LIMITFRQ_AND_ZEROED ***************************/

int below_limitfrq_and_zeroed(int vc,double loskirt,dataptr dz)
{
	if(dz->flbufptr[0][FREQ] < loskirt) {
		dz->flbufptr[0][AMPP] = 0.0F;
		return(TRUE);
	}
	return(FALSE);
}

/**************************** BELOW_LIMITFRQ_AND_FILTERED ***************************/

int below_limitfrq_and_filtered(int vc,double limitfrq,double loskirt,double loskirt_bwidth,dataptr dz)
{
	if(dz->flbufptr[0][FREQ] < limitfrq) {
		dz->flbufptr[0][AMPP] = (float)(dz->flbufptr[0][AMPP] * ((dz->flbufptr[0][FREQ] - loskirt)/loskirt_bwidth));
		return(TRUE);
	}
	return(FALSE);
}

/**************************** FILTER_ABOVE_LIMITFRQ ***************************/

int filter_above_limitfrq(int vc,double limitfrq,double hiskirt,double hiskirt_bwidth,dataptr dz)
{
	if(dz->flbufptr[0][FREQ] > limitfrq) {
		dz->flbufptr[0][AMPP] = (float)(dz->flbufptr[0][AMPP] * ((hiskirt - dz->flbufptr[0][FREQ])/hiskirt_bwidth));
		return(TRUE);
	}
	return(FALSE);
}

/**************************** ABOVE_LIMITFRQ_AND_ZEROED ***************************/

int above_limitfrq_and_zeroed(int vc,double hiskirt,dataptr dz)
{
	if(dz->flbufptr[0][FREQ] > hiskirt) {
		dz->flbufptr[0][AMPP] = 0.0F;
		return(TRUE);
	}
	return(FALSE);
}

/**************************** FILTER_IN_LOWER_SKIRT ***************************/

int filtered_in_lower_skirt(int vc,double skirttop,double loskirt,double loskirt_bwidth,dataptr dz)
{
	if(dz->flbufptr[0][FREQ] < skirttop) {
		dz->flbufptr[0][AMPP] = (float)(dz->flbufptr[0][AMPP] * (1.0 - ((dz->flbufptr[0][FREQ] - loskirt)/loskirt_bwidth)));
		return(TRUE);
	}
	return(FALSE);
}

/**************************** FILTER_IN_UPPER_SKIRT ***************************/

int filtered_in_upper_skirt(int vc,double skirt_bottom,double hiskirt,double hiskirt_bwidth,dataptr dz)
{
	if(dz->flbufptr[0][FREQ] > skirt_bottom) {
		dz->flbufptr[0][AMPP] = (float)(dz->flbufptr[0][AMPP] * (1.0 - ((hiskirt - dz->flbufptr[0][FREQ])/hiskirt_bwidth)));
		return(TRUE);
	}
	return(FALSE);
}

/***********************  SPECGREQ **********************/

int specgreq(dataptr dz)
{
	int exit_status;
	if((exit_status = construct_filter_envelope((int)dz->itemcnt,dz->flbufptr[0],dz))<0)
		return(exit_status);
	if(dz->vflag[GREQ_NOTCH]) {
		if((exit_status = invert_greq_filter_envelope(dz))<0)
			return(exit_status);
	}
	if((exit_status = do_greq_filter(dz))<0)
		return(exit_status);
	return(FINISHED);
}

/****************************** DO_GREQ_FILTER ***************************/

int do_greq_filter(dataptr dz)
{
	int cc, vc;
	for(cc=0, vc=0; cc < dz->clength; cc++, vc += 2)
		dz->flbufptr[0][AMPP] = (float)(dz->flbufptr[0][AMPP] * dz->fsampbuf[cc]);
	return(FINISHED);
}													 

/****************************** INVERT_GREQ_FILTER_ENVELOPE ***************************/

int invert_greq_filter_envelope(dataptr dz)
{
	int cc;
	for(cc=0; cc<dz->clength; cc++)
		dz->fsampbuf[cc] = (float)(1.0 - dz->fsampbuf[cc]);
	return(FINISHED);
}	

/******************************** SPECSPLIT ******************************/

int specsplit(dataptr dz) 
{
	int exit_status;
	int cc, bno = 0, bottom, top, done = 0, k;
	bandptr bb;
	double bandbot, bandtop;

	rectify_window(dz->flbufptr[0],dz);
	if((exit_status = get_amp_and_frq(dz->flbufptr[0],dz))<0)
		return(exit_status);		
	cc   = 0;			/* channel counter */
	while(bno<dz->itemcnt) {		/* For all bands */
		bb      = dz->band[bno];
		bandbot = bb->bfrqlo;	
		bandtop = bb->bfrqhi;
		while(dz->freq[cc]<bandbot) {	/* while chan-frq<bandbot: do nothing */
			if(++cc>=dz->clength) {
				done = 1;		
				break;   /* if all channels<bandbot, we.ve finished pass */
			}
		}
		if(done)
			break;
		bottom = cc;		/* set bottom channel for inner loop */
		while(dz->freq[cc]<bandtop) {
			if(++cc>=dz->clength) {
				done = 1;	/* find channels up to top of band */
				break;
			}
		}
		top = min(cc,(dz->clength)-1); 	/* set top channel for inner loop */
		if((bb->bdoflag & DO_TRANSPOSITION) && bb->btrans>1.0) {
			for( k = top; k>= bottom; k--) {
				if((exit_status = process_specsplit_channel(k,bb,dz))<0)
					return(exit_status);
			}
		} else {
			for( k = bottom; k <= top; k++) {
				if((exit_status = process_specsplit_channel(k,bb,dz))<0)
					return(exit_status);
			}
		}
		if(done)
			break;
		bno++;
	}
	if((exit_status = put_amp_and_frq(dz->flbufptr[0],dz))<0)
		return(exit_status);
	return(FINISHED);
}

/************************ PROCESS_SPECSPLIT_CHANNEL *****************************/

int process_specsplit_channel(int cc,bandptr bb,dataptr dz)
{
	int exit_status;
	double thismult;
	double newamp = dz->amp[cc];	/* DEFAULT */
	double newfrq = dz->freq[cc];	/* DEFAULT */
	int newchan;

	if(bb->bdoflag & DO_AMPLITUDE_CHANGE) {
		if(bb->bdoflag & DO_RAMPED_AMPLITUDE) {
			thismult  = (dz->freq[cc] - bb->bfrqlo)/(bb->bfrqdif);
			thismult *= bb->bampdif;
			thismult += bb->bamp;
			newamp    = dz->amp[cc] * thismult;
		} else
			newamp   = dz->amp[cc] * bb->bamp;
	}
	if(bb->bdoflag & DO_TRANSPOSITION)  {
		if(bb->badditive) {
			if((newfrq  = dz->freq[cc] + bb->btrans)<dz->nyquist) {
				if((exit_status = get_channel_corresponding_to_frq(&newchan,newfrq,dz))<0)
					return(exit_status);
				if(newchan!=cc) {
					dz->freq[newchan] = (float)newfrq;
					dz->amp[newchan]  = (float)newamp;
				}  	
			}
		} else {
			if((newfrq  = dz->freq[cc] * bb->btrans)<dz->nyquist) {
				if((exit_status = get_channel_corresponding_to_frq(&newchan,newfrq,dz))<0)
					return(exit_status);
				if(newchan!=cc) {
					dz->freq[newchan] = (float)newfrq;
					dz->amp[newchan]  = (float)newamp;
				}
			}	
		}
		if(!(bb->bdoflag & DO_ADD_TO_SPECTRUM)) {
			newamp =   	/* set interpolated value on old amp */
			(dz->amp[max(cc-1,0)] + dz->amp[min(cc+1,(dz->clength-1))])/2.0;
		}
	}
	dz->freq[cc] = (float)newfrq;
	dz->amp[cc]  = (float)newamp;
	return(FINISHED);
}

/********************************** SPECARPE **********************************/

int specarpe(int *in_start_portion,dataptr dz)
{
	int exit_status;
	double	lofrq, hifrq, frqrang;
	double	bandmid, bandhilimit, bandlolimit;
	int 	tabindex;
	if((exit_status = reset_timechanging_arpe_variables(&frqrang,&lofrq,&hifrq,dz))<0)
		return(exit_status);		
	if((exit_status = locate_current_wavetable_position(dz))<0)
		return(exit_status);		
	tabindex    = round(dz->param[ARPE_WAVETABPOS]);
											    	/* LOCATE CURRENT FREQUENCY BAND */
	bandmid     = (dz->parray[ARPE_TAB][tabindex] * frqrang) + lofrq; 
	bandhilimit = min(bandmid + dz->param[ARPE_HBAND],dz->nyquist);
	bandlolimit = max(bandmid - dz->param[ARPE_HBAND],0.0);
	  
	if((exit_status = get_amp_and_frq(dz->flbufptr[0],dz))<0)		/* SEPARATE THE AMP AND FREQ DATA */
		return(exit_status);		

	switch(dz->mode) {		 /* ELIMINATE OR MODIFY FREQUENCIES, AS NECESSARY */
	case(ON):  			exit_status = do_on(bandhilimit,bandlolimit,dz);							break;
	case(BELOW):  		exit_status = do_below(bandmid,dz);											break;
	case(ABOVE):  		exit_status = do_above(bandmid,dz);											break;
	case(BOOST):  		exit_status = do_boost(bandhilimit,bandlolimit,dz); 		   				break;
	case(ABOVE_BOOST):  exit_status = do_above_boost(bandhilimit,bandlolimit,in_start_portion,dz);	break;
	case(BELOW_BOOST):  exit_status = do_below_boost(bandhilimit,bandlolimit,in_start_portion,dz);	break;
	case(ONCE_ABOVE):  	exit_status = do_once_above(bandhilimit,in_start_portion,dz);				break;
	case(ONCE_BELOW):  	exit_status = do_once_below(bandlolimit,in_start_portion,dz);				break;
	default:
		sprintf(errstr,"unknown mode in specarpe()\n");
		return(PROGRAM_ERROR);
	}
	if(exit_status<0)
		return(exit_status);		

	if((exit_status = put_amp_and_frq(dz->flbufptr[0],dz))<0) /*  WRITE MODIFIED AMP & FREQ BUFF TO SAMP BUFF */
		return(exit_status);		
	return(FINISHED);
}

/*************************** DO_ON ****************************/

int do_on(double bandhilimit,double bandlolimit,dataptr dz)
{
	int exit_status;
	int cc, vc = 0;
	double dec, nonlin_dec = 0.0, thisamp;
	switch(dz->iparam[ARPE_SUSFLAG]) {
	case(AP_NORMAL):
		for(cc=0,vc=0;cc<dz->clength;cc++,vc+=2) {
			if(dz->iparray[ARPE_KEEP][cc]) {
				if((exit_status = sustain_arpeg_note(cc,vc,dz))<0)
					return(exit_status);
			} else {
				if(dz->freq[cc]>=bandlolimit && dz->freq[cc]<=bandhilimit) {
					if((exit_status = initiate_new_arpeg_note(cc,vc,dz))<0)
						return(exit_status);
				} else
					dz->amp[cc]  = 0.0f;
			}
	    }
		break;
	case(AP_SUSTAIN_PITCH):
		for(cc=0,vc=0;cc<dz->clength;cc++,vc+=2) {
			if(dz->iparray[ARPE_KEEP][cc]) {
				if((exit_status = sustain_arpeg_note_with_fixed_pitch(cc,vc,dz))<0)
					return(exit_status);
			} else {
				if(dz->freq[cc]>=bandlolimit && dz->freq[cc]<=bandhilimit) {
					if((exit_status = initiate_new_arpeg_note_with_fixed_pitch(cc,vc,dz))<0)
						return(exit_status);
				} else
					dz->amp[cc]  = 0.0f;
			}
	    }
		break;
	case(AP_LIMIT_SUSTAIN):
		for(cc=0,vc=0;cc<dz->clength;cc++,vc+=2) {
			if(dz->freq[cc]>=bandlolimit && dz->freq[cc]<=bandhilimit) {
				if((exit_status = get_decimation(&dec,cc,dz))<0)
						return(exit_status);
				if(dz->iparray[ARPE_KEEP][cc] && ((thisamp = dz->windowbuf[0][AMPP] * dec) > dz->amp[cc])) {
					if((exit_status = temporarily_sustain_arpeg_note(cc,thisamp,dz))<0)
						return(exit_status);
				} else {
					if((exit_status = initiate_new_arpeg_note(cc,vc,dz))<0)
						return(exit_status);
				}
			} else {
				if(dz->iparray[ARPE_KEEP][cc]) {
					if((exit_status = sustain_arpeg_note(cc,vc,dz))<0)
						return(exit_status);
				} else
					dz->amp[cc] = 0.0f;
			}
	    }
		break;
	case(AP_SUSTAIN_PITCH_AND_LIMIT_SUSTAIN):
		for(cc=0,vc=0;cc<dz->clength;cc++,vc+=2) {
			if(dz->freq[cc]>=bandlolimit && dz->freq[cc]<=bandhilimit) {
				if((exit_status = get_decimation(&dec,cc,dz))<0)
					return(exit_status);
				if(dz->iparray[ARPE_KEEP][cc] && ((thisamp = dz->windowbuf[0][AMPP] * dec) > dz->amp[cc])) {
					if((exit_status = temporarily_sustain_arpeg_note_with_fixed_pitch(cc,vc,thisamp,dz))<0)
						return(exit_status);
				} else {
					if((exit_status = initiate_new_arpeg_note_with_fixed_pitch(cc,vc,dz))<0)
						return(exit_status);
				}
			} else {
				if(dz->iparray[ARPE_KEEP][cc]) {
					if((exit_status = sustain_arpeg_note_with_fixed_pitch(cc,vc,dz))<0)
						return(exit_status);
				} else
					dz->amp[cc] = 0.0f;
			}
	    }										
		break;
	case(AP_NONLIN):
		for(cc=0,vc=0;cc<dz->clength;cc++,vc+=2) {
			if(dz->iparray[ARPE_KEEP][cc]) {
				if((exit_status = nonlin_sustain_arpeg_note(cc,vc,dz))<0)
					return(exit_status);
			} else {
				if(dz->freq[cc]>=bandlolimit && dz->freq[cc]<=bandhilimit) {
					if((exit_status = initiate_new_arpeg_note(cc,vc,dz))<0)
						return(exit_status);
				} else
					dz->amp[cc]  = 0.0f;
			}
	    }
		break;
	case(AP_NONLIN_AND_SUSTAIN_PITCH):
		for(cc=0,vc=0;cc<dz->clength;cc++,vc+=2) {
			if(dz->iparray[ARPE_KEEP][cc]) {
				if((exit_status = nonlin_sustain_arpeg_note_with_fixed_pitch(cc,vc,dz))<0)
					return(exit_status);
			} else {
				if(dz->freq[cc]>=bandlolimit && dz->freq[cc]<=bandhilimit) {
					if((exit_status = initiate_new_arpeg_note_with_fixed_pitch(cc,vc,dz))<0)
						return(exit_status);
				} else
					dz->amp[cc]  = 0.0f;
			}
	    }
		break;
	case(AP_NONLIN_AND_LIMIT_SUSTAIN):
		for(cc=0,vc=0;cc<dz->clength;cc++,vc+=2) {
			if(dz->freq[cc]>=bandlolimit && dz->freq[cc]<=bandhilimit) {
				if(dz->iparray[ARPE_KEEP][cc]) {
					if((exit_status = get_non_linear_decimation(&nonlin_dec,cc,dz))<0)
						return(exit_status);
				}
				if(dz->iparray[ARPE_KEEP][cc] && ((thisamp = dz->windowbuf[0][AMPP] * nonlin_dec) > dz->amp[cc])) {
					if((exit_status = temporarily_sustain_arpeg_note(cc,thisamp,dz))<0)
						return(exit_status);
				} else {
					if((exit_status = initiate_new_arpeg_note(cc,vc,dz))<0)
						return(exit_status);
				}
			} else {
				if(dz->iparray[ARPE_KEEP][cc]) {
					if((exit_status = nonlin_sustain_arpeg_note(cc,vc,dz))<0)
						return(exit_status);
				} else
					dz->amp[cc] = 0.0f;
			}
	    }
		break;
	case(AP_NONLIN_AND_SUSTAIN_PITCH_AND_LIMIT_SUSTAIN):
		for(cc=0,vc=0;cc<dz->clength;cc++,vc+=2) {
			if(dz->freq[cc]>=bandlolimit && dz->freq[cc]<=bandhilimit) {
				if(dz->iparray[ARPE_KEEP][cc]) {
					if((exit_status = get_non_linear_decimation(&nonlin_dec,cc,dz))<0)
						return(exit_status);
				}
				if(dz->iparray[ARPE_KEEP][cc] && ((thisamp = dz->windowbuf[0][AMPP] * nonlin_dec) > dz->amp[cc])) {
					if((exit_status = temporarily_sustain_arpeg_note_with_fixed_pitch(cc,vc,thisamp,dz))<0)
						return(exit_status);
				} else {
					if((exit_status = initiate_new_arpeg_note_with_fixed_pitch(cc,vc,dz))<0)
						return(exit_status);
				}
			} else {
				if(dz->iparray[ARPE_KEEP][cc]) {
					if((exit_status = nonlin_sustain_arpeg_note_with_fixed_pitch(cc,vc,dz))<0)
						return(exit_status);
				} else
					dz->amp[cc] = 0.0f;
			}
	    }
		break;
	}
	return(FINISHED);
}


/******************************* DO_BELOW **************************/

int do_below(double bandmid,dataptr dz)
{	
	int cc;
	for(cc=0;cc<dz->clength;cc++) {
		if(dz->freq[cc]>bandmid)
			dz->amp[cc] = 0.0f;
//TW JULY 2006
		else
			dz->amp[cc] = (float)(dz->amp[cc] * dz->param[ARPE_AMPL]);
	}
	return(FINISHED);
}

/******************************** DO_ABOVE ***********************/

int do_above(double bandmid,dataptr dz)
{
	int cc;
	for(cc=0;cc<dz->clength;cc++) {
		if(dz->freq[cc]<bandmid)
			dz->amp[cc] = 0.0f;
//TW JULY 2006
		else
			dz->amp[cc] = (float)(dz->amp[cc] * dz->param[ARPE_AMPL]);
	}
	return(FINISHED);
}

/**************************** DO_BOOST ****************************/

int do_boost(double bandhilimit,double bandlolimit,dataptr dz)
{
	int exit_status;
	int cc, vc;
	double dec, nonlin_dec = 0.0,thisamp;
	switch(dz->iparam[ARPE_SUSFLAG]) {
	case(AP_NORMAL):
		for(cc=0,vc=0;cc<dz->clength;cc++,vc+=2) {
			if(dz->iparray[ARPE_KEEP][cc]) {
				if((exit_status = sustain_arpeg_note(cc,vc,dz))<0)
					return(exit_status);
			} else if(dz->freq[cc]>=bandlolimit && dz->freq[cc]<=bandhilimit) {
				if((exit_status = initiate_new_arpeg_note(cc,vc,dz))<0)
					return(exit_status);
			}
	    }
		break;
	case(AP_SUSTAIN_PITCH):
		for(cc=0,vc=0;cc<dz->clength;cc++,vc+=2) {
			if(dz->iparray[ARPE_KEEP][cc]) {
				if((exit_status = sustain_arpeg_note_with_fixed_pitch(cc,vc,dz))<0)
					return(exit_status);
			} else if(dz->freq[cc]>=bandlolimit && dz->freq[cc]<=bandhilimit) {
				if((exit_status = initiate_new_arpeg_note_with_fixed_pitch(cc,vc,dz))<0)
					return(exit_status);
			}
	    }
		break;
	case(AP_LIMIT_SUSTAIN):
		for(cc=0,vc=0;cc<dz->clength;cc++,vc+=2) {
			if(dz->freq[cc]>=bandlolimit && dz->freq[cc]<=bandhilimit) {
				if((exit_status = get_decimation(&dec,cc,dz))<0)
					return(exit_status);
				if(dz->iparray[ARPE_KEEP][cc] && ((thisamp = dz->windowbuf[0][AMPP] * dec) > dz->amp[cc])) {
					if((exit_status = temporarily_sustain_arpeg_note(cc,thisamp,dz))<0)
						return(exit_status);
				} else {
					if((exit_status = initiate_new_arpeg_note(cc,vc,dz))<0)
						return(exit_status);
				}
			} else {
				if(dz->iparray[ARPE_KEEP][cc]) {
					 if((exit_status = sustain_arpeg_note(cc,vc,dz))<0)
						return(exit_status);
				}
			}
	    }
		break;
	case(AP_SUSTAIN_PITCH_AND_LIMIT_SUSTAIN):
		for(cc=0,vc=0;cc<dz->clength;cc++,vc+=2) {
			if(dz->freq[cc]>=bandlolimit && dz->freq[cc]<=bandhilimit) {
				if((exit_status = get_decimation(&dec,cc,dz))<0)
					return(exit_status);
				if(dz->iparray[ARPE_KEEP][cc] && ((thisamp = dz->windowbuf[0][AMPP] * dec) > dz->amp[cc])) {
					if((exit_status = temporarily_sustain_arpeg_note_with_fixed_pitch(cc,vc,thisamp,dz))<0)
						return(exit_status);
				} else {
					if((exit_status = initiate_new_arpeg_note_with_fixed_pitch(cc,vc,dz))<0)
						return(exit_status);
				}
			} else {
				if(dz->iparray[ARPE_KEEP][cc]) {
					if((exit_status = sustain_arpeg_note_with_fixed_pitch(cc,vc,dz))<0)
						return(exit_status);
				}
			}
	    }
		break;
	case(AP_NONLIN):
		for(cc=0,vc=0;cc<dz->clength;cc++,vc+=2) {
			if((exit_status = get_non_linear_decimation(&nonlin_dec,cc,dz))<0)
				return(exit_status);
			if(dz->iparray[ARPE_KEEP][cc]) {
				dz->amp[cc] = (float)(dz->windowbuf[0][AMPP] * nonlin_dec);
				dz->iparray[ARPE_KEEP][cc]--;
			} else if(dz->freq[cc]>=bandlolimit && dz->freq[cc]<=bandhilimit) {
				if((exit_status = initiate_new_arpeg_note(cc,vc,dz))<0)
					return(exit_status);
			}
	    }
		break;
	case(AP_NONLIN_AND_SUSTAIN_PITCH):
		for(cc=0,vc=0;cc<dz->clength;cc++,vc+=2) {
			if(dz->iparray[ARPE_KEEP][cc]) {
				if((exit_status = nonlin_sustain_arpeg_note_with_fixed_pitch(cc,vc,dz))<0)
					return(exit_status);
			} else if(dz->freq[cc]>=bandlolimit && dz->freq[cc]<=bandhilimit) {
				if((exit_status = initiate_new_arpeg_note_with_fixed_pitch(cc,vc,dz))<0)
					return(exit_status);
			}
	    }
		break;
	case(AP_NONLIN_AND_LIMIT_SUSTAIN):
		for(cc=0,vc=0;cc<dz->clength;cc++,vc+=2) {
			if(dz->freq[cc]>=bandlolimit && dz->freq[cc]<=bandhilimit) {
				if(dz->iparray[ARPE_KEEP][cc]) {
					if((exit_status = get_non_linear_decimation(&nonlin_dec,cc,dz))<0)
						return(exit_status);
				}
				if(dz->iparray[ARPE_KEEP][cc] && ((thisamp = dz->windowbuf[0][AMPP] * nonlin_dec) > dz->amp[cc])) {
					if((exit_status = temporarily_sustain_arpeg_note(cc,thisamp,dz))<0)
						return(exit_status);
				} else {
					if((exit_status = initiate_new_arpeg_note(cc,vc,dz))<0)
						return(exit_status);
				}
			} else {
				if(dz->iparray[ARPE_KEEP][cc]) {
					 if((exit_status = nonlin_sustain_arpeg_note(cc,vc,dz))<0)
					 	return(exit_status);
				}
			}
	    }
		break;
	case(AP_NONLIN_AND_SUSTAIN_PITCH_AND_LIMIT_SUSTAIN):
		for(cc=0,vc=0;cc<dz->clength;cc++,vc+=2) {
			if(dz->freq[cc]>=bandlolimit && dz->freq[cc]<=bandhilimit) {
				if(dz->iparray[ARPE_KEEP][cc]) {
					if((exit_status = get_non_linear_decimation(&nonlin_dec,cc,dz))<0)
						return(exit_status);
				}
				if(dz->iparray[ARPE_KEEP][cc] && ((thisamp = dz->windowbuf[0][AMPP] * nonlin_dec) > dz->amp[cc])) {
					if((exit_status = temporarily_sustain_arpeg_note_with_fixed_pitch(cc,vc,thisamp,dz))<0)
						return(exit_status);
				} else {
					if((exit_status = initiate_new_arpeg_note_with_fixed_pitch(cc,vc,dz))<0)
						return(exit_status);
				}
			} else {
				if(dz->iparray[ARPE_KEEP][cc]) {
					if((exit_status = nonlin_sustain_arpeg_note_with_fixed_pitch(cc,vc,dz))<0)
						return(exit_status);
				}
			}
	    }
		break;
	}
	return(FINISHED);
}

/************************* DO_ABOVE_BOOST ****************************/

int do_above_boost(double bandhilimit,double bandlolimit,int *in_start_portion, dataptr dz)
{
	int cc;   
	if(*in_start_portion) {
		if((dz->iparam[ARPE_WTYPE]==SIN || dz->iparam[ARPE_WTYPE]==SAW)) {
			if(dz->param[ARPE_WAVETABPOS]>=ARPE_TABSIZE/2)
				*in_start_portion = FALSE;
		} else {
			if(dz->param[ARPE_WAVETABPOS] < dz->param[ARPE_LAST_TABPOS])
			*in_start_portion = FALSE;
		}
	}
	if(*in_start_portion) {
		for(cc=0;cc<dz->clength;cc++) {
			if(dz->freq[cc]>bandlolimit)
				dz->amp[cc] = 0.0f;
//TW JULY 2006
			else
				dz->amp[cc] = (float)(dz->amp[cc] * dz->param[ARPE_AMPL] * dz->iparam[ARPE_SUST]);
		}
	} else
		return do_boost(bandhilimit,bandlolimit,dz);
	return(FINISHED);
}

/**************************** DO_BELOW_BOOST ************************/

int do_below_boost(double bandhilimit,double bandlolimit,int *in_start_portion,dataptr dz)
{
	int cc;
	if(*in_start_portion) {
		if(dz->param[ARPE_WAVETABPOS] < dz->param[ARPE_LAST_TABPOS])
			*in_start_portion = FALSE;
	}
	if(*in_start_portion) {
		for(cc=0;cc<dz->clength;cc++) {
			if(dz->freq[cc]<bandhilimit)
				dz->amp[cc] = 0.0f;
//TW JULY 2006
			else
				dz->amp[cc] = (float)(dz->amp[cc] * dz->param[ARPE_AMPL] * dz->iparam[ARPE_SUST]);
		}
	} else
		return do_boost(bandhilimit,bandlolimit,dz);
	return(FINISHED);
}

/************************** DO_ONCE_BELOW ***************************/

int do_once_below(double bandlolimit,int *in_start_portion,dataptr dz)
{
	int cc;
	if(*in_start_portion) {
		if((dz->iparam[ARPE_WTYPE]==SIN || dz->iparam[ARPE_WTYPE]==SAW)) {
			if(dz->param[ARPE_WAVETABPOS] >= ARPE_TABSIZE/2)
				*in_start_portion = FALSE;
		} else {
			if(dz->param[ARPE_WAVETABPOS] < dz->param[ARPE_LAST_TABPOS])
			*in_start_portion = FALSE;
		}
	}
	if(*in_start_portion) {
		for(cc=0;cc<dz->clength;cc++) {
			if(dz->freq[cc]>bandlolimit)
				dz->amp[cc] = 0.0f;
//TW JULY 2006
			else
				dz->amp[cc] = (float)(dz->amp[cc] * dz->param[ARPE_AMPL]);
		}
	}
	return(FINISHED);
}

/***************************** DO_ONCE_ABOVE **************************/

int do_once_above(double bandhilimit,int *in_start_portion,dataptr dz)
{
	int cc;
	if(*in_start_portion) {
		if(dz->param[ARPE_WAVETABPOS] < dz->param[ARPE_LAST_TABPOS])
			*in_start_portion = FALSE;
	}
	if(*in_start_portion) {
		for(cc=0;cc<dz->clength;cc++) {
			if(dz->freq[cc]<bandhilimit)
				dz->amp[cc] = 0.0f;
//TW JULY 2006
			else
				dz->amp[cc] = (float)(dz->amp[cc] * dz->param[ARPE_AMPL]);
		}
	}
	return(FINISHED);
}

/******************** INITIATE_NEW_ARPEG_NOTE ********************/

int initiate_new_arpeg_note(int cc,int vc,dataptr dz)
{
	dz->iparray[ARPE_KEEP][cc] = dz->iparam[ARPE_SUST];
	dz->amp[cc] *= (float)(dz->param[ARPE_AMPL] * dz->iparray[ARPE_KEEP][cc]);
	dz->windowbuf[0][AMPP] = dz->amp[cc];
	(dz->iparray[ARPE_KEEP][cc])--;
	return(FINISHED);
}

/******************** INITIATE_NEW_ARPEG_NOTE_WITH_FIXED_PITCH ********************/

int initiate_new_arpeg_note_with_fixed_pitch(int cc,int vc,dataptr dz)
{
	dz->iparray[ARPE_KEEP][cc] = dz->iparam[ARPE_SUST];
	dz->amp[cc] *= (float)(dz->param[ARPE_AMPL] * dz->iparray[ARPE_KEEP][cc]);
	dz->windowbuf[0][AMPP] = dz->amp[cc];
	dz->windowbuf[0][FREQ] = dz->freq[cc];
	dz->iparray[ARPE_KEEP][cc]--;
	return(FINISHED);
}

/******************** TEMPORARILY_SUSTAIN_ARPEG_NOTE ********************/

int temporarily_sustain_arpeg_note(int cc,double thisamp,dataptr dz)
{
	dz->amp[cc] = (float)thisamp;
	dz->iparray[ARPE_KEEP][cc]--;
	return(FINISHED);
}


/******************** TEMPORARILY_SUSTAIN_ARPEG_NOTE_WITH_FIXED_PITCH ********************/

int temporarily_sustain_arpeg_note_with_fixed_pitch(int cc,int vc,double thisamp,dataptr dz)
{
	dz->amp[cc]  = (float)thisamp;
	dz->freq[cc] = dz->windowbuf[0][FREQ];
	dz->iparray[ARPE_KEEP][cc]--;
	return(FINISHED);
}

/******************** GET_NON_LINEAR_DECIMATION ********************/

int get_non_linear_decimation(double *nonlin_dec,int cc,dataptr dz)
{
	*nonlin_dec = (double)dz->iparray[ARPE_KEEP][cc]/(double)dz->iparam[ARPE_SUST];
	*nonlin_dec = pow(*nonlin_dec,dz->param[ARPE_NONLIN]);
	return(FINISHED);
}

/******************** GET_DECIMATION ********************/

int get_decimation(double *dec,int cc,dataptr dz)
{
	*dec = (double)dz->iparray[ARPE_KEEP][cc]/(double)dz->iparam[ARPE_SUST];
	return(FINISHED);
}

/******************** SUSTAIN_ARPEG_NOTE ********************/

int sustain_arpeg_note(int cc,int vc,dataptr dz)
{
	double z = (double)dz->iparray[ARPE_KEEP][cc]/(double)dz->iparam[ARPE_SUST];
	dz->amp[cc] = (float)(dz->windowbuf[0][AMPP] * z);
	dz->iparray[ARPE_KEEP][cc]--;
	return(FINISHED);
}

/******************** SUSTAIN_ARPEG_NOTE_WITH_FIXED_PITCH ********************/

int sustain_arpeg_note_with_fixed_pitch(int cc,int vc,dataptr dz)
{
	double z = (double)dz->iparray[ARPE_KEEP][cc]/(double)dz->iparam[ARPE_SUST];
	dz->amp[cc] = (float)(dz->windowbuf[0][AMPP] * z);
	dz->freq[cc] = dz->windowbuf[0][FREQ];
	dz->iparray[ARPE_KEEP][cc]--;
	return(FINISHED);
}

/******************** NONLIN_SUSTAIN_ARPEG_NOTE ********************/

int nonlin_sustain_arpeg_note(int cc,int vc, dataptr dz)
{
	double z = (double)dz->iparray[ARPE_KEEP][cc]/(double)dz->iparam[ARPE_SUST];
	z = pow(z,dz->param[ARPE_NONLIN]);
	dz->amp[cc] = (float)(dz->windowbuf[0][AMPP] * z);
	dz->iparray[ARPE_KEEP][cc]--;
	return(FINISHED);
}

/******************** NONLIN_SUSTAIN_ARPEG_NOTE_WITH_FIXED_PITCH ********************/

int nonlin_sustain_arpeg_note_with_fixed_pitch(int cc, int vc, dataptr dz)
{
	double z = (double)dz->iparray[ARPE_KEEP][cc]/(double)dz->iparam[ARPE_SUST];
	z = pow(z,dz->param[ARPE_NONLIN]);
	dz->amp[cc]  = (float)(dz->windowbuf[0][AMPP] * z);
	dz->freq[cc] = dz->windowbuf[0][FREQ];
	dz->iparray[ARPE_KEEP][cc]--;
	return(FINISHED);
}

/****************************** SPECPLUCK ****************************/

int specpluck(dataptr dz)
{
	int exit_status;
	int mask = 1;
	int cc, vc, bflagno, is_set;
	if(dz->total_windows==1) {		/* Set up BITFLAGS for current state of chans */
		for(cc = 0, vc = 0; cc < dz->clength; cc++, vc += 2){
			if((exit_status = choose_bflagno_and_reset_mask_if_ness
			(&bflagno,cc,&mask,dz->iparam[PLUK_LONGPOW2],dz->iparam[PLUK_DIVMASK]))<0)
				return(exit_status);
			if(!flteq((double)dz->flbufptr[0][AMPP],0.0))
				dz->lparray[PLUK_BFLG][bflagno] |= mask;
			mask <<= 1;		    
		}
	} else {		 			/* Check change of state of channels */
		for(cc = 0, vc = 0; cc < dz->clength; cc++, vc += 2){
			if((exit_status = choose_bflagno_and_reset_mask_if_ness
			(&bflagno,cc,&mask,dz->iparam[PLUK_LONGPOW2],dz->iparam[PLUK_DIVMASK]))<0)
				return(exit_status);
			is_set = dz->lparray[PLUK_BFLG][bflagno] & mask;
			if(!flteq((double)dz->flbufptr[0][vc],0.0)) { 	/* If chan amp NOT zero */
				if(!is_set) { 						/* if bit previously 0 (for zero amp) */
					dz->flbufptr[0][vc] = (float)(dz->flbufptr[0][vc] * dz->param[PLUK_GAIN]);
					exit_status = set_bit_to_one(bflagno,mask,dz);	   /* Give boost to chan amp */
				}
			} else {	 		     				/* channel amp IS zero */
				if(is_set)							/* if bit previously 1 for nonzero amp */
					exit_status = set_bit_to_zero(bflagno,mask,dz);
			}
			mask <<= 1;			    				/* move bitmask upwards */
		}
	}		
	return(FINISHED);
}

/****************************** SET_BIT_TO_ZERO ****************************/

int set_bit_to_zero(int bflagno,int mask,dataptr dz)
{
	mask = ~mask;		 					  /* bit-invert mask */
	dz->lparray[PLUK_BFLG][bflagno] &= mask;  /* Set bit to 0 */
	return(FINISHED);
}


/****************************** SET_BIT_TO_ONE ****************************/

int set_bit_to_one(int bflagno,int mask,dataptr dz)
{
	dz->lparray[PLUK_BFLG][bflagno] |= mask;  
	return(FINISHED);
}

/********************************** SPECTRACE **********************************/

int spectrace(dataptr dz)
{
	int exit_status;
	int invtrindex, cc, vc;
	int    chans_outside_fltband_to_keep = 0, chans_in_filtband;
	double	hifrq_limit = 0.0, lofrq_limit = 0.0;
	chvptr quietest, loudest;
	if((dz->mode == TRC_ALL || dz->vflag[TRACE_RETAIN]) && dz->iparam[TRAC_INDX] >= dz->clength)
		return(FINISHED);
	if(dz->mode != TRC_ALL) {
		hifrq_limit = dz->param[TRAC_HIFRQ];
		lofrq_limit = dz->param[TRAC_LOFRQ];
		if(hifrq_limit < lofrq_limit)
			swap(&hifrq_limit,&lofrq_limit);
		if(dz->vflag[TRACE_RETAIN] ) {
			chans_in_filtband = dz->clength;
			for(cc=0,vc=0;cc<dz->clength;cc++,vc+=2) {
				if(dz->flbufptr[0][FREQ] < lofrq_limit || dz->flbufptr[0][FREQ] > hifrq_limit)
					chans_in_filtband--;
			}
			if((chans_outside_fltband_to_keep = dz->iparam[TRAC_INDX] - chans_in_filtband) <= 0) {
				chans_outside_fltband_to_keep = 0;
				for(cc=0,vc=0;cc<dz->clength;cc++,vc+=2) {
					if(dz->flbufptr[0][FREQ] < lofrq_limit || dz->flbufptr[0][FREQ] > hifrq_limit)
						dz->flbufptr[0][AMPP] = 0.0f;
				}
			}
		} else {
			for(cc=0,vc=0;cc<dz->clength;cc++,vc+=2) {
				if(dz->flbufptr[0][FREQ] < lofrq_limit || dz->flbufptr[0][FREQ] > hifrq_limit)
					dz->flbufptr[0][AMPP] = 0.0f;
			}
		}
	}
	if(chans_outside_fltband_to_keep>0) {
		if((exit_status = initialise_ring_vals(chans_outside_fltband_to_keep,-BIGAMP,dz))<0)
			return(exit_status);
		for(vc = 0; vc < dz->wanted; vc += 2) {
			if(dz->flbufptr[0][FREQ] < lofrq_limit || dz->flbufptr[0][FREQ] > hifrq_limit) {
				if((exit_status = if_one_of_loudest_chans_store_in_ring(vc,dz))<0)
					return(exit_status);
				dz->flbufptr[0][AMPP]  = 0.0F;
			}
		}
     /* RESTORE TRUE AMPLITUDE (STORED IN RING) IN LOUDEST OUTSIDER CHANNELS ONLY */
		loudest = dz->ringhead;		
		do {
			dz->flbufptr[0][loudest->loc] = loudest->val;
		} while((loudest = loudest->next)!=dz->ringhead);
	} else {
		if(dz->iparam[TRAC_INDX]>(dz->clength/2)) { 						/* IF MORE CHANS TO KEEP THAN TO REJECT */
			invtrindex = dz->clength - dz->iparam[TRAC_INDX];   			/* COUNT CHANS TO REJECT */
			if((exit_status = initialise_ring_vals(invtrindex,BIGAMP,dz))<0)/* MAXIMISE ALL QUIETEST STORES IN RING */
				return(exit_status);
			for(vc = 0; vc < dz->wanted; vc += 2) {							/* STORE INDICES OF QUIETEST CHANS */
				if((exit_status = if_one_of_quietest_chans_store_in_ring(vc,dz))<0)
					return(exit_status);
			}
			quietest = dz->ringhead;		
			do {								  							/* ZERO ALL QUIETEST CHANS */
				dz->flbufptr[0][quietest->loc] = 0.0F;
			} while((quietest = quietest->next)!=dz->ringhead);
		} else {															/* IF MORE CHANS TO REJECT THAN TO KEEP */
			if((exit_status = initialise_ring_vals(dz->iparam[TRAC_INDX],-BIGAMP,dz))<0)
				return(exit_status);										/* MINIMISE ALL LOUDEST STORES IN RING */
			for(vc = 0; vc < dz->wanted; vc += 2) {
				if((exit_status = if_one_of_loudest_chans_store_in_ring(vc,dz))<0)
					return(exit_status);									/* STORE INDICES OF LOUDEST CHANS */
				dz->flbufptr[0][AMPP]  = 0.0F;  							/* INITIALISE EVERY CHANNEL TO ZERO AMP	 */
			}
			loudest = dz->ringhead;	 			
			do {															/* RESTORE AMPLITUDE IN LOUDEST CHANS ONLY */
				dz->flbufptr[0][loudest->loc] = loudest->val;
			} while((loudest = loudest->next)!=dz->ringhead);
		}
	}
	return(FINISHED);
}

/**************************** OUTSIDE_SKIRTS ***************************/

int outside_skirts(int vc,double loskirt,double hiskirt,dataptr dz)
{
	if(dz->flbufptr[0][FREQ] < loskirt || dz->flbufptr[0][FREQ] > hiskirt)
		return(TRUE);
	return(FALSE);
}

/***************************** RESET_TIMECHANGING_ARPE_VARIABLES **************************/

int reset_timechanging_arpe_variables(double *frqrange,double *lofrq, double *hifrq,dataptr dz)
{
	double this_arpfrq;

	if(dz->brksize[ARPE_ARPFRQ]) {				  /* RESET ARPFREQ, if ness */
		this_arpfrq = (dz->param[ARPE_LASTARPFRQ] + dz->param[ARPE_ARPFRQ])/2.0;
		dz->param[ARPE_LASTARPFRQ] = dz->param[ARPE_ARPFRQ];
		dz->param[ARPE_ARPFRQ] = this_arpfrq;
	}
												  /* RESET arp amplification, if ness */
//TW JULY 2006
	if(dz->mode < BELOW) {
		if(dz->brksize[ARPE_AMPL] || dz->brksize[ARPE_SUST])
			dz->param[ARPE_AMPL]  /= (double)dz->iparam[ARPE_SUST];
	}

	*lofrq = dz->param[ARPE_LOFRQ];				  /* RESET arp hi and lo limits */
	*hifrq = dz->param[ARPE_HIFRQ];
	if((dz->brksize[ARPE_LOFRQ] || dz->brksize[ARPE_HIFRQ])
	&& (*hifrq < *lofrq))
		swap(hifrq,lofrq);
	*frqrange = *hifrq - *lofrq;

	return(FINISHED);
}											    	

/******************** LOCATE_CURRENT_WAVETABLE_POSITION ********************/

int locate_current_wavetable_position(dataptr dz)
{											    	
	double tabincr;
	tabincr 					= dz->frametime * dz->param[ARPE_ARPFRQ] * ARPE_TABSIZE;
	dz->param[ARPE_LAST_TABPOS] = dz->param[ARPE_WAVETABPOS];
	dz->param[ARPE_WAVETABPOS]  = fmod(dz->param[ARPE_WAVETABPOS] + tabincr,(double)ARPE_TABSIZE);
	return(FINISHED);
}

//TW UPDATE: NEW FUNCTIONS (adjusted for float)
/************************************ VOWEL_FILTER ************************************/

#define NOISEBASE	(0.2)

int vowel_filter(dataptr dz)
{
	int *vowels = dz->iparray[0];
	double *times = dz->parray[0];
	double startformant1, startformant2, startformant3, endformant1, endformant2, endformant3;
	double formant1, formant2, formant3;
	double form1step, form2step, form3step;
	double starttime, endtime, time, timefrac, timestep, *sensitivity;
	double f3startatten, f3endatten, f3attenstep, f3atten;
	double f2startatten, f2endatten, f2attenstep, f2atten;
	int exit_status, senslen;
	int n = 0, t = 0;
	double *amp;
	int total_windows_got, windows_in_buf;

	if((amp = (double *)malloc(dz->clength * sizeof(double)))==NULL) {
		sprintf(errstr,"Insufficient memory to store vowel envelope\n");
		return(MEMORY_ERROR);
	}
	if((exit_status = define_sensitivity_curve(&sensitivity,&senslen))<0)
		return(exit_status);

	dz->flbufptr[0] = dz->bigfbuf;
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

	total_windows_got = 0;

	while(n < dz->wlength) {
		if(n >= total_windows_got) {
			if((exit_status = read_samps(dz->bigfbuf, dz)) < 0) {
				sprintf(errstr,"Problem reading source analysis data.\n");
				return(SYSTEM_ERROR);
			}
			dz->flbufptr[0] = dz->bigfbuf;
			windows_in_buf = dz->ssampsread/dz->wanted;
			total_windows_got += windows_in_buf;
		}
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
		if((exit_status = do_vowel_filter
		(amp,formant1,formant2,formant3,f2atten,f3atten,sensitivity,senslen,dz)) <0)
			return(exit_status);

		if((dz->flbufptr[0] += dz->wanted) >= dz->flbufptr[1]) {
			if((exit_status = write_samps(dz->bigfbuf,dz->buflen,dz))<0)
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
		
/************************************ DO_VOWEL_FILTER ************************************
 *
 * "sensitivity" compensates for frq sensitivity of ear at low end, and attenuates
 * formant bands above c3500.
 */

int do_vowel_filter(double *vamp,double formant1,double formant2,double formant3,double f2atten,double f3atten,
							double *sensitivity,int senslen,dataptr dz)

{
	double hfwidth1, hfwidth2, hfwidth3 = 0.0, lolim1, lolim2, lolim3 = 0.0, hilim1, hilim2, hilim3 = 0.0;
//	double thisfrq, amp, amp2, amp3 = 0.0, totamp = 0.0;
	double thisfrq, amp, amp2 = 0.0, amp3 = 0.0;
	int exit_status, cc, vc;
	int overlapped_formants12 = 0, overlapped_formants23 = 0, overlapped_formants13 = 0;
	int is_overlap12, is_overlap23, is_overlap13;
	double toplim, pre_totamp, post_totamp, maxamp, maxvamp, ampscale;
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
	maxamp = 0.0;
	pre_totamp = 0.0;
	for(cc = 0, vc = 0; cc < dz->clength; cc++, vc += 2) {
		pre_totamp += dz->flbufptr[0][AMPP];
		maxamp = max(dz->flbufptr[0][AMPP],maxamp);		
	}
	maxvamp = 0.0;
	/* find the formant envelope amplitude at frequency of every window */
	if(!is_third_formant)
		hilim3 = hilim2;
	for(cc = 0, vc = 0;cc <dz->clength; cc++, vc += 2) {
		thisfrq = dz->flbufptr[0][FREQ];
		amp = 0.0;
		is_overlap12 = 0;
		is_overlap23 = 0;
		is_overlap13 = 0;
		if(thisfrq < lolim1 || thisfrq > hilim3)
			;
		else if((thisfrq > lolim1) && (thisfrq < hilim1)) {
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
		vamp[cc] = amp;
		maxvamp = max(maxvamp,vamp[cc]);
	}
	/* set scaling factor to allow for max level of input window */
	if(flteq(maxvamp,0.0))
		ampscale = 0.0;
	else
		ampscale = (maxamp/maxvamp);
	post_totamp = 0.0;
	for(cc = 0,vc = 0;cc <dz->clength; cc++,vc += 2) {
		vamp[cc] *= ampscale;
/* force window to formant level ONLY if existing val > a given proportion of formant level */
		if((double)dz->flbufptr[0][AMPP] > vamp[cc] * dz->param[VF_THRESH])
			dz->flbufptr[0][AMPP] =  (float)vamp[cc];
		post_totamp += dz->flbufptr[0][AMPP];
	}
/* normalise to overall level of original window */
	if((exit_status = normalise(pre_totamp,post_totamp,dz))<0)
		return(exit_status);
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
	/* hood, mud (North of England)  */		*f2atten = U_F2ATTEN;	*f3atten = U_F3ATTEN;
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
	/* 'THe'  */							*f2atten = TH_F2ATTEN;	*f3atten = TH_F3ATTEN;
					break;
	default:
		sprintf(errstr,"Unknown vowel\n");
		return(PROGRAM_ERROR);
	}
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
		*amp *= sensitivity[1];
		return(FINISHED);
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
	*amp *= multiplier;
	return(FINISHED);
}

