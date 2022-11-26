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
#include <flags.h>
#include <arrays.h>
#include <focus.h>
#include <cdpmain.h>
#include <formants.h>
#include <speccon.h>
#include <sfsys.h>
#include <float.h>
#include <string.h>
#include <focus.h>

static int  rwd_accumulate(int index,float *flbufptr, float *windowbuf);
static int  do_fold(int vc,double lofrq_limit,double hifrq_limit,dataptr dz);
static int  fold_in(int vc,double thisfrq,dataptr dz);
static int  do_stable_peaks_filter(int *peakscore,int *descnt,double lofrq_limit,double hifrq_limit,dataptr dz);
static int  extract_formant_peaks(int *pkcnt_here,double lofrq_limit,double hifrq_limit,int *least,dataptr dz);
static int  establish_bottom_and_top_of_filter_bands(int pkcnt_here,dataptr dz);
static int  do_focu_filter(float *thisbuf,dataptr dz);
static int  design_filter(int *peakscore,int *descnt,dataptr dz);
static int  increment_pointer_in_output_buffer(dataptr dz);
static int  insert_new_peak_in_peaklist(int position_in_formant,int *least,dataptr dz);
static int  sort_focu_peaks_by_frq(int pkcnt_here,dataptr dz);
static int  write_last_focu_buffers(dataptr dz);
static int  read_first_buffer(int *windows_in_buf,dataptr dz);
static int  sfwind(int *windows_in_buf,int *wndws_read_pre_thisbuf,dataptr dz);
static int  get_buffer_containing_frzpos(int frztime,int *frzpos,int *windows_in_buf,
				int *wndws_read_pre_thisbuf,dataptr dz);
static int  get_freeze_template(dataptr dz);
static int  get_startwindow_position(int *startwpos,int this_segtime,int *wndws_read_pre_thisbuf,
				int *windows_in_buf,int buffer_saved,dataptr dz);
static int  freeze_data(dataptr dz);
static int	save_buffer(dataptr dz);
static int  sfrewind(int *windows_in_buf,int window_sought,int *wndws_read_pre_thisbuf,dataptr dz);
static int	restore_buffer(dataptr dz);
static int  do_specstep(int wc,dataptr dz);

/* NEW CODE */
static int  read_buffer(int *windows_in_buf,dataptr dz);
/* NEW CODE */

/********************************** SPECACCU **********************************/

int specaccu(dataptr dz)
{
	int exit_status;
	int vc, cc;
	if(dz->param[ACCU_GLIS] > 0.0) {
		if(dz->param[ACCU_DECAY] < 1.0) {
			for(cc = 0, vc = 0; cc < dz->clength; cc++,  vc += 2) {
				dz->windowbuf[0][AMPP] = (float)(dz->windowbuf[0][AMPP] * dz->param[ACCU_DINDEX]);
				dz->windowbuf[0][FREQ] = (float)(dz->windowbuf[0][FREQ] * dz->param[ACCU_GINDEX]);
				if((exit_status = rwd_accumulate(vc,dz->flbufptr[0],dz->windowbuf[0]))<0)
					return(exit_status);
			}
		} else {
			for(cc = 0, vc = 0; cc < dz->clength; cc++,  vc += 2) {
				dz->windowbuf[0][FREQ] = (float)(dz->windowbuf[0][FREQ] * dz->param[ACCU_GINDEX]);
				if((exit_status = rwd_accumulate(vc,dz->flbufptr[0],dz->windowbuf[0]))<0)
					return(exit_status);
			}
		}
	} else if(dz->param[ACCU_DECAY] < 1.0) {
		for(cc = 0, vc = 0; cc < dz->clength; cc++,  vc += 2) {
			dz->windowbuf[0][AMPP] = (float)(dz->windowbuf[0][AMPP] * dz->param[ACCU_DINDEX]);
			if((exit_status = rwd_accumulate(vc,dz->flbufptr[0],dz->windowbuf[0]))<0)
				return(exit_status);
		}
	} else {	 
		for(cc = 0, vc = 0; cc < dz->clength; cc++,  vc += 2) {
			if((exit_status = rwd_accumulate(vc,dz->flbufptr[0],dz->windowbuf[0]))<0)
				return(exit_status);
		}
	}
	return(FINISHED);
}

/********************************** RWD_ACCUMULATE **********************************/

int rwd_accumulate(int index,float *flbufptr, float *windowbuf)
{
	int frq = index+1;
	if(flbufptr[index] > windowbuf[index])  {	 /* if current amp > amp in accumulator */
		windowbuf[index] = flbufptr[index]; 
		windowbuf[frq] = flbufptr[frq];	 /* replace amp in accumulator with current amp */
	} else {
		flbufptr[index] = windowbuf[index]; 	 /* else replace current amp with amp in accumulator */
		flbufptr[frq] = windowbuf[frq];								
	}
	return(FINISHED);
}

/**************************** SPECEXAG ****************************/

int specexag(int *zero_set,dataptr dz)
{
	int cc, vc;
	double post_totalamp = 0.0, pre_totalamp = 0.0;
	double maxamp = 0.0, normaliser;
	for( cc = 0 ,vc = 0; cc < dz->clength; cc++, vc += 2)  {
		pre_totalamp += dz->flbufptr[0][vc];
		if(dz->flbufptr[0][vc] > maxamp)
			maxamp = dz->flbufptr[0][vc];
	}
	if(maxamp<=0.0)	{
		*zero_set = TRUE;
		return(FINISHED);
	}
	normaliser = 1.0/maxamp;
	for(cc = 0 ,vc = 0; cc < dz->clength; cc++, vc += 2)  {
		dz->flbufptr[0][vc]  = (float)(dz->flbufptr[0][vc] * normaliser);
		dz->flbufptr[0][vc]  = (float)(pow(dz->flbufptr[0][vc],dz->param[EXAG_EXAG]));
		post_totalamp += dz->flbufptr[0][vc];
	}
	return normalise(pre_totalamp,post_totalamp,dz);
}

/**************************** SPECFOLD ***************************/

int specfold(dataptr dz)
{
	int exit_status;
	int vc;
	double hifrq_limit = dz->param[FOLD_HIFRQ];
	double lofrq_limit = dz->param[FOLD_LOFRQ];
	if(hifrq_limit < lofrq_limit)
		swap(&lofrq_limit,&hifrq_limit);
	if(dz->brksize[FOLD_HIFRQ]) {
		dz->iparam[FOLD_HICHAN] = (int)((hifrq_limit + dz->halfchwidth)/dz->chwidth); /* TRUNCATE */
		dz->param[FOLD_HICHANTOP] = ((double)dz->iparam[FOLD_HICHAN] * dz->chwidth) + dz->halfchwidth;
		if(dz->iparam[FOLD_HICHAN] != 0)
			dz->iparam[FOLD_HICHAN] <<= 1;
	}
	if(dz->brksize[FOLD_LOFRQ]) {
		dz->iparam[FOLD_LOCHAN] = (int)((lofrq_limit + dz->halfchwidth)/dz->chwidth); /* TRUNCATE */
		if(dz->iparam[FOLD_LOCHAN] > 0)
			dz->param[FOLD_LOCHANBOT] = ((double)(dz->iparam[FOLD_LOCHAN]-1) * dz->chwidth) + dz->halfchwidth;
		else
			dz->param[FOLD_LOCHANBOT] = 0.0;
		if(dz->iparam[FOLD_LOCHAN] != 0)
			dz->iparam[FOLD_LOCHAN]<<= 1;
	}
	for(vc = 0; vc < dz->wanted; vc += 2) {
		if((exit_status = do_fold(vc,lofrq_limit,hifrq_limit,dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}

/******************************* DO_FOLD **************************/

int do_fold(int vc,double lofrq_limit,double hifrq_limit,dataptr dz)
{
	int exit_status;
	double thisfrq = fabs((double)dz->flbufptr[0][FREQ]);
	if(thisfrq < 1.0) {
		dz->flbufptr[0][AMPP] = 0.0f;
		return(FINISHED);
	}
	if(thisfrq < lofrq_limit) {
		while(thisfrq < lofrq_limit)
			thisfrq *= 2.0;  
		if(thisfrq < hifrq_limit) {
			if((exit_status = fold_in(vc,thisfrq,dz))<0)
				return(exit_status);
		}
		dz->flbufptr[0][AMPP] = 0.0f;
	} else {
		if(thisfrq > hifrq_limit) {
			while(thisfrq > hifrq_limit)
				thisfrq /= 2.0;  
			if(thisfrq > lofrq_limit) {
				if((exit_status = fold_in(vc,thisfrq,dz))<0)
					return(exit_status);
			}
			dz->flbufptr[0][AMPP] = 0.0f;
		}
	}
	return(FINISHED);
}

/******************************** FOLD_IN ************************/

int fold_in(int vc,double thisfrq,dataptr dz)
{

#define THISCHANSCAN (4)

	int truecc, truevc;
	int minscan, maxscan, n;
	truecc = (int)((thisfrq + dz->halfchwidth)/dz->chwidth); /* TRUNCATE */
	truevc = truecc * 2;
	switch(dz->vflag[FOLD_WITH_BODY]) {
	case(FALSE):
		if(dz->flbufptr[0][AMPP] > dz->flbufptr[0][truevc]) {
			dz->flbufptr[0][truevc++] = dz->flbufptr[0][AMPP];
			dz->flbufptr[0][truevc]   = dz->flbufptr[0][FREQ];
		}
		break;
	case(TRUE):
		if(dz->flbufptr[0][AMPP] > dz->flbufptr[0][truevc]) {
			dz->flbufptr[0][truevc++] = dz->flbufptr[0][AMPP];
			dz->flbufptr[0][truevc]   = dz->flbufptr[0][FREQ];
			return(FINISHED);
		}
		minscan = max(dz->iparam[FOLD_LOCHAN]-2,truevc - (THISCHANSCAN * 2));
		maxscan = min(dz->iparam[FOLD_HICHAN]+2,truevc + (THISCHANSCAN * 2));
		for(n=truevc-2;n>minscan;n-=2) {		
			if(dz->flbufptr[0][AMPP] > dz->flbufptr[0][n]) {
				dz->flbufptr[0][n++] = dz->flbufptr[0][AMPP];
				dz->flbufptr[0][n]   = dz->flbufptr[0][FREQ];
				return(FINISHED);
			}
		}
		for(n=truevc+2;n<maxscan;n+=2) {
			if(dz->flbufptr[0][AMPP] > dz->flbufptr[0][n]) {
				dz->flbufptr[0][n++] = dz->flbufptr[0][AMPP];
				dz->flbufptr[0][n]   = dz->flbufptr[0][FREQ];
				return(FINISHED);
			}
		}
		break;
	default:
		sprintf(errstr,"Unknown case in fold_in()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/**************************** OUTER_FOCU_LOOP ****************************/

int outer_focu_loop(dataptr dz)
{
	int exit_status;
	int samps_read, got, windows_in_buf;
	int peakscore = 0, pitchcnt = 0;
	int in_start_portion = TRUE, least = 0, descnt = 0;
	if(dz->vflag[FOCUS_STABLE])
		dz->flbufptr[1] = dz->flbufptr[2];
	while((samps_read = fgetfbufEx(dz->bigfbuf, dz->buflen,dz->ifd[0],0)) > 0) {
    	got = samps_read;
    	dz->flbufptr[0] = dz->bigfbuf;
    	windows_in_buf = got/dz->wanted;
    	if((exit_status = inner_loop(&peakscore,&descnt,&in_start_portion,&least,&pitchcnt,windows_in_buf,dz))<0)
			return(exit_status);
		if(!dz->vflag[FOCUS_STABLE] && (samps_read > 0)) {
			if((exit_status = write_exact_samps(dz->bigfbuf,samps_read,dz))<0)
				return(exit_status);
		}
	}
	if(samps_read < 0) {
		sprintf(errstr,"Sound read error.\n");
		return(SYSTEM_ERROR);
	}  
	if(dz->vflag[FOCUS_STABLE])
		return write_last_focu_buffers(dz);
	return(FINISHED);
}

/**************************** SPECFOCUS ***************************/

int specfocus(int *peakscore,int *descnt,int *least,dataptr dz)
{
	int exit_status;
	double lofrq_limit, hifrq_limit, d;
	int pkcnt_here;
	lofrq_limit = dz->param[FOCU_LOFRQ];
	hifrq_limit = dz->param[FOCU_HIFRQ];
	if(dz->brksize[FOCU_LOFRQ] || dz->brksize[FOCU_HIFRQ]) {
		if(flteq(lofrq_limit,hifrq_limit)) {
			sprintf(errstr,"Frequency limits define a zero-width band at time %lf.\n",dz->time);
			return(USER_ERROR);
		}
		if(lofrq_limit > hifrq_limit)
			swap(&lofrq_limit,&hifrq_limit);
	}
	if(dz->brksize[FOCU_BW]) {
		d = dz->param[FOCU_BW]/2.0;				/* d is halfbandwidth.						    */
		dz->scalefact = PI/d;		 			/* Convert an 8va offset from filter centre-frq */
								    			/* to a fraction of the bandwidth * PI 		    */
		dz->param[FOCU_BRATIO_UP] = pow(2.0,d);	/* upward transp ratio to top of filtband  		*/
		dz->param[FOCU_BRATIO_DN] = 1.0/dz->param[FOCU_BRATIO_UP];
	}											 /* Downward transp ratio to bot of fband 		*/
	rectify_window(dz->flbufptr[0],dz);
	if((exit_status = extract_specenv(0,0,dz))<0)
		return(exit_status);
	if(dz->vflag[FOCUS_STABLE])		
		return do_stable_peaks_filter(peakscore,descnt,lofrq_limit,hifrq_limit,dz);
	if((exit_status = extract_formant_peaks(&pkcnt_here,lofrq_limit,hifrq_limit,least,dz))<0)
		return(exit_status);
	if((exit_status = establish_bottom_and_top_of_filter_bands(pkcnt_here,dz))<0)
		return(exit_status);
	if((exit_status = construct_filter_envelope(pkcnt_here,dz->flbufptr[0],dz))<0)
		return(exit_status);
	return do_focu_filter(dz->flbufptr[0],dz);
}

/****************************** DO_STABLE_PEAKS_FILTER ***************************/

int do_stable_peaks_filter(int *peakscore,int *descnt,double lofrq_limit,double hifrq_limit,dataptr dz)
{
	int exit_status, thispkcnt;
//	int *fppk = dz->stable->fpk[dz->iparam[FOCU_SL1]], n;
	int n;
																/* shufl bak stord specenvs: */
																/* put latest specenvarray here */
	memmove((char *)dz->stable->spec[0],
//TW CODE ERROR
//	(char *)dz->stable->spec[1],dz->iparam[FOCU_SL1] * dz->infile->specenvcnt * sizeof(int));
	(char *)dz->stable->spec[1],dz->iparam[FOCU_SL1] * dz->infile->specenvcnt * sizeof(float));
	memmove((char *)dz->stable->spec[dz->iparam[FOCU_SL1]],
//TW CODE ERROR
//	(char *)dz->specenvamp,dz->infile->specenvcnt * sizeof(int));						  
	(char *)dz->specenvamp,dz->infile->specenvcnt * sizeof(float));						  
	memmove((char *)dz->stable->fpk[0], (char *)dz->stable->fpk[1],
	dz->iparam[FOCU_SL1] * dz->infile->specenvcnt * sizeof(int));	 
	memmove((char *)dz->stable->total_pkcnt,(char *)(dz->stable->total_pkcnt+1),dz->iparam[FOCU_SL1] * sizeof(int)); 	 
																/* shufl bak stord fpks & shufl bak stored fpk_totals */
	if((exit_status = extract_formant_peaks2(FOCU_SL1,&thispkcnt,lofrq_limit,hifrq_limit,dz))<0)
		return(exit_status);									/* put into into last fpk[z] store */
	dz->stable->total_pkcnt[dz->iparam[FOCU_SL1]] = thispkcnt;	/* put into into last fpk[z] store */
	if(dz->total_windows >= dz->iparam[FOCU_SL1]) {				/* before this we can't design a filter */
		if((exit_status = design_filter(peakscore,descnt,dz))<0)
			return(exit_status);
		if(dz->total_windows==dz->iparam[FOCU_SL1]) {			/* If 1st time to do filtering */
			memmove((char *)(dz->flbufptr[1]),(char *)dz->stable->sbuf[0],(size_t)dz->wanted * sizeof(float));	/* copy zero-amp-1st-buff */
			if((exit_status = increment_pointer_in_output_buffer(dz))<0)
				return(exit_status);
			for(n = 1; n <= dz->stable->offset; n++) { 			/* filter all windows <= offset, using 1st filter */
				if((exit_status = do_focu_filter(dz->stable->sbuf[n],dz))<0)
					return(exit_status);						/* and output them */
		 		memmove((char *)(dz->flbufptr[1]),(char *)dz->stable->sbuf[n],(size_t)dz->wanted * sizeof(float));		   
				if((exit_status = increment_pointer_in_output_buffer(dz))<0)
					return(exit_status);
			}
		} else { 					 							/* all later times to do filtering */
																/* filter buffer at offset and output it */	
			if((exit_status = do_focu_filter(dz->stable->sbuf[dz->stable->offset],dz))<0)
				return(exit_status);
		 	memmove((char *)dz->flbufptr[1],(char *)dz->stable->sbuf[dz->stable->offset],(size_t)dz->wanted * sizeof(float));		   
			if((exit_status = increment_pointer_in_output_buffer(dz))<0)
				return(exit_status);
		}
	}
	memmove((char *)dz->stable->sbuf[0],(char *)dz->stable->sbuf[1],
	(size_t)(dz->wanted * sizeof(float) * dz->iparam[FOCU_SL1]));	 		/* shuffle back stored bufs */
	memmove((char *)dz->stable->sbuf[dz->iparam[FOCU_SL1]],(char *)dz->flbufptr[0],(size_t)dz->wanted * sizeof(float));		 
	return(FINISHED);											/* copy new buf to store */
}

/****************************** EXTRACT_FORMANT_PEAKS ***************************/

int extract_formant_peaks(int *pkcnt_here,double lofrq_limit,double hifrq_limit,int *least,dataptr dz)
{
	int exit_status;
	int n = 0;
	int falling = 0, last_channel_was_in_range = 1;
	*pkcnt_here = 0;
	for(n=0;n<dz->itemcnt;n++)	  
		dz->filtpeak[n] = 0.0;
	n = 1;
	while(n<dz->infile->specenvcnt) {
		if(dz->specenvfrq[n] < lofrq_limit) {
			last_channel_was_in_range = 0;
			n++;
			continue;
		}
		if(dz->specenvamp[n] > dz->specenvamp[n-1])
			falling = 0;
		else {
			if(!falling) {
				if(last_channel_was_in_range) {
					if((exit_status = insert_new_peak_in_peaklist(n-1,least,dz))<0)
						return(exit_status);
				}
				(*pkcnt_here)++;
				falling = 1;
			}
		}
		last_channel_was_in_range = 1;
		if(dz->specenvfrq[n] > hifrq_limit)
			break;
		n++;
	}
	*pkcnt_here = min(*pkcnt_here,dz->itemcnt);
	return sort_focu_peaks_by_frq(*pkcnt_here,dz);
}

/******************************** ESTABLISH_BOTTOM_AND_TOP_OF_FILTER_BANDS ************************/

int establish_bottom_and_top_of_filter_bands(int pkcnt_here,dataptr dz)
{
	int n; 

 	for(n=0;n<pkcnt_here;n++) {		 
		dz->fbandbot[n] = dz->specenvfrq[dz->peakno[n]] * dz->param[FOCU_BRATIO_DN];
		dz->fbandtop[n] = min(dz->specenvfrq[dz->peakno[n]] * dz->param[FOCU_BRATIO_UP],dz->nyquist);
	}
	return(FINISHED);
}

/****************************** DO_FOCU_FILTER ***************************/

int do_focu_filter(float *thisbuf,dataptr dz)
{
	int cc, vc;

	for(cc=0, vc=0; cc < dz->clength; cc++, vc += 2)
		thisbuf[AMPP] = (float)(thisbuf[AMPP] * dz->fsampbuf[cc]);
	return(FINISHED);
}													 

/****************************** DESIGN_FILTER ******************************/ 

int design_filter(int *peakscore,int *descnt,dataptr dz) 
{
	int exit_status;
	int this_pkcnt;
	int n = 0;
	int pkcnt_here;
	if(dz->total_windows<=dz->iparam[FOCU_SL1])
		memset((char *)dz->stable->design_score,0,dz->infile->specenvcnt * sizeof(int));
	if((exit_status = score_peaks(peakscore,FOCU_SL1,FOCU_STABL,dz))<0)
		return(exit_status);
	if(*peakscore==0) {
		memset(dz->fsampbuf,0,(size_t)dz->wanted * sizeof(float));	/* zero filter */
		return(FINISHED);
	}
 	if((exit_status = collect_scores(&this_pkcnt,descnt,dz))<0)
		return(exit_status);	 					 /* Find how many times each peak occurs across the stabilise buffers */
 	if((exit_status = sort_design(this_pkcnt,dz))<0) /* Sort these on the basis of the most commonly occuring */
		return(exit_status);		
	if(this_pkcnt<=dz->itemcnt || (dz->stable->des[dz->itemcnt-1]->score > dz->stable->des[dz->itemcnt]->score)) {
		pkcnt_here = min(this_pkcnt,dz->itemcnt);
		for(n=0;n<pkcnt_here;n++)					 /* If most common pkcnt (or less) peaks more common than ALL others */
			dz->peakno[n] = dz->stable->des[n]->chan;/* these are the peaks to keep */
	} else {										 /* Else choose amongst equivalent peaks, in terms of amplitude */
		if((exit_status = sort_equivalent_scores(this_pkcnt,dz))<0)
			return(exit_status);
		pkcnt_here = dz->itemcnt;
	}
	if((exit_status = sort_focu_peaks_by_frq(pkcnt_here,dz))<0)
		return(exit_status);
	if((exit_status = establish_bottom_and_top_of_filter_bands(pkcnt_here,dz))<0)
		return(exit_status);
	if((exit_status = construct_filter_envelope(pkcnt_here,dz->stable->sbuf[dz->stable->offset],dz))<0)
		return(exit_status);
	if(dz->total_windows>dz->iparam[FOCU_SL1]) {	 /* Logically this should be '>=dz->iparam[FOCU_SL1]' : but */
		if((exit_status = unscore_peaks(peakscore,dz))<0)
			return(exit_status);					 /* don't have to subtract window 0, as there are NO PEAKS there */
	}
	return(FINISHED);
}

/****************************** INCREMENT_POINTER_IN_OUTPUT_BUFFER ***************************/

int increment_pointer_in_output_buffer(dataptr dz)
{
	int exit_status;
	if((dz->flbufptr[1] += dz->wanted) >= dz->flbufptr[3])	{
		if((exit_status = write_exact_samps(dz->flbufptr[2],dz->buflen,dz))<0)
			return(exit_status);
		dz->flbufptr[1] = dz->flbufptr[2];
	}
	return(FINISHED);
}

/****************************** INSERT_NEW_PEAK_IN_PEAKLIST ***************************/

int insert_new_peak_in_peaklist(int position_in_formant,int *least,dataptr dz)
{
	int n;
	double smallest = DBL_MAX;
	if(dz->specenvamp[position_in_formant] > dz->filtpeak[*least])  {
		dz->filtpeak[*least] = dz->specenvamp[position_in_formant];
		dz->peakno[*least] = position_in_formant;
	}
	for(n=0;n<dz->itemcnt;n++) {
		if(dz->filtpeak[n] < smallest) {
			*least = n;
			smallest = dz->filtpeak[n];
		}
	}
	return(FINISHED);
}

/****************************** SORT_FOCU_PEAKS_BY_FRQ ***************************/

int sort_focu_peaks_by_frq(int pkcnt_here,dataptr dz)
{	
	int n, m, k;
	for(n=0;n<pkcnt_here-1;n++) {
		for(m=n+1;m<pkcnt_here;m++) {
			if(dz->specenvfrq[dz->peakno[n]] > dz->specenvfrq[dz->peakno[m]]) {
				k        = dz->peakno[m];
				dz->peakno[m] = dz->peakno[n];
				dz->peakno[n] = k;
			}
		}
	}
	return(FINISHED);
}

/****************************** WRITE_LAST_FOCU_BUFFERS ******************************/ 

int write_last_focu_buffers(dataptr dz)
 {
	int exit_status;
 	int n;
	for(n = dz->stable->offset+1; n < dz->iparam[FOCU_STABL]; n++) {
		if((exit_status = do_focu_filter(dz->stable->sbuf[n],dz))<0)
			return(exit_status);
		memmove((char *)dz->flbufptr[1],(char *)dz->stable->sbuf[n],(size_t)dz->wanted * sizeof(float));
		if((exit_status = increment_pointer_in_output_buffer(dz))<0)
			return(exit_status);
	}
	if(dz->flbufptr[1]!=dz->flbufptr[2]) {
		if((exit_status = write_exact_samps(dz->flbufptr[2],(dz->flbufptr[1] - dz->flbufptr[2]),dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}

/****************** SPECFREEZE  ********************
 *
 * (1)	if no freezing needed.
 * (2)	write any complete buffers before next timepoint.
 * (3)	advance to buffer containing freeze-window if ness.
 * (4)	advance within current buffer, to freeze-window.
 * (5)	get number of windows to process.
 * (6)	set endwindow relative to current buffer.
 * (7)	advance within current buffer, to start-window.
 * (8)	process windows from here to end-window.
 * (9)	if current buffer overrun: proceed to next buffer.
 * (10)	readjust window counters to be relative to nextbuffer start.
 * (11)	write data and go to next buffer.
 * (12)	flush buffer.
 */

int specfreeze(dataptr dz)
{
 	int exit_status;
    int n = 0, buffer_saved;
	int wc, wndws_read_pre_thisbuf = 0, frzpos;
    int startwpos = 0, endwpos = 0, windiff;
	int windows_in_buf;
	if((exit_status = read_first_buffer(&windows_in_buf,dz))<0)
		return(exit_status);
	while(n<dz->itemcnt-1) {
		if(dz->lparray[FRZ_FRZTIME][n] < 0L) {	 									/* 1 */
			while(dz->lparray[FRZ_SEGTIME][n+1] > wndws_read_pre_thisbuf + windows_in_buf) {
				if((exit_status = sfwind(&windows_in_buf,&wndws_read_pre_thisbuf,dz))<0)
					return(exit_status);											/* 2 */
				if(exit_status == FINISHED) {
					sprintf(errstr,"Window search miscalculation 1, specfreeze()\n");
					return(PROGRAM_ERROR);
				}					
			}
			endwpos = dz->lparray[FRZ_SEGTIME][n+1] - wndws_read_pre_thisbuf;
		} else {
			buffer_saved = 0;
			frzpos = dz->lparray[FRZ_FRZTIME][n] - wndws_read_pre_thisbuf;
			if(frzpos > windows_in_buf) {											/* 3 */
				if((exit_status = get_buffer_containing_frzpos
				(dz->lparray[FRZ_FRZTIME][n],&frzpos,&windows_in_buf,&wndws_read_pre_thisbuf,dz))<0)
					return(exit_status);
				buffer_saved = 1;
			}
			dz->flbufptr[0] = dz->bigfbuf + (frzpos * dz->wanted);					/* 4 */
			if((exit_status = get_freeze_template(dz))<0)
				return(exit_status);
			windiff   = dz->lparray[FRZ_SEGTIME][n+1] - dz->lparray[FRZ_SEGTIME][n];/* 5 */
			if((exit_status = get_startwindow_position
			(&startwpos,dz->lparray[FRZ_SEGTIME][n],&wndws_read_pre_thisbuf,&windows_in_buf,buffer_saved,dz))<0)
				return(exit_status);
			endwpos   = startwpos + windiff;		   								/* 6 */
			wc        = startwpos;													/* 7 */
			dz->flbufptr[0] = dz->bigfbuf + (startwpos * dz->wanted);				/* 8 */
			exit_status = CONTINUE;
			while(wc<endwpos) {
				if(exit_status == FINISHED) {
					sprintf(errstr,"Window search miscalculation 2, specfreeze()\n");
					return(PROGRAM_ERROR);
				}					
				if((exit_status = freeze_data(dz))<0)
					return(exit_status);
				dz->flbufptr[0] += dz->wanted;
				if(++wc >= windows_in_buf) {	 									/* 9 */
					wc       = 0;    												/* 10 */
					endwpos -= windows_in_buf;    									/* 11 */
					if((exit_status = sfwind(&windows_in_buf,&wndws_read_pre_thisbuf,dz))<0)
						return(exit_status);
				}
			}
		}		
		n++;
	}
	if((endwpos > 0) && (windows_in_buf > 0)) {										/* 12 */
    	if((exit_status = write_exact_samps(dz->bigfbuf,windows_in_buf * dz->wanted,dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}

/*************************** READ_FIRST_BUFFER ******************************/

int read_first_buffer(int *windows_in_buf,dataptr dz)
{
	int samps_read, got;

	if((samps_read = fgetfbufEx(dz->bigfbuf, dz->buflen,dz->ifd[0],0)) < 0) {
		sprintf(errstr,"Cannot read data from input file: read_first_buffer()\n");
		return(SYSTEM_ERROR);
	}
	got = samps_read;
	*windows_in_buf = got/dz->wanted;    
	return(FINISHED);
}

/*************************** SFWIND *******************************/

int sfwind(int *windows_in_buf,int *wndws_read_pre_thisbuf,dataptr dz)
{
	int exit_status;
	int got, samps_read;
	*wndws_read_pre_thisbuf += *windows_in_buf;
	if(*windows_in_buf > 0) {
		if((exit_status = write_exact_samps(dz->bigfbuf,*windows_in_buf * dz->wanted,dz))<0)
			return(exit_status);
	}
	if((samps_read = fgetfbufEx(dz->bigfbuf, dz->buflen,dz->ifd[0],0)) < 0) {
		sprintf(errstr,"Failed to read sound data: sfwind()\n");
		return(SYSTEM_ERROR);
	}
	got            = samps_read;
	*windows_in_buf = got/dz->wanted;    
	dz->flbufptr[0] = dz->bigfbuf;
	if(samps_read==0)
		return(FINISHED);
	return(CONTINUE);
}

/*************************** GET_BUFFER_CONTAINING_FRZPOS *******************************/

int get_buffer_containing_frzpos(int frztime,int *frzpos,int *windows_in_buf,int *wndws_read_pre_thisbuf,dataptr dz)
{
	int exit_status;
	int samps_read, got;
	int buffer_saved = 0;
	while(*frzpos > *windows_in_buf) {
		if(!buffer_saved) { 
			if((exit_status = save_buffer(dz))<0)
				return(exit_status);
		}
		buffer_saved = 1;
		*wndws_read_pre_thisbuf += *windows_in_buf;    
		if((samps_read = fgetfbufEx(dz->bigfbuf, dz->buflen,dz->ifd[0],0)) <= 0) {
			if(samps_read<0) {
				sprintf(errstr,"Sound read error.\n");
				return(SYSTEM_ERROR);
			} else {				
				sprintf(errstr,"Buffer accnting probl: Can't read input data: get_buffer_containing_frzpos()\n");
				return(PROGRAM_ERROR);
			}
		}
		got = samps_read;
		*windows_in_buf = got/dz->wanted;    
		*frzpos = frztime - *wndws_read_pre_thisbuf;
	}
	return(FINISHED);
}

/*************************** GET_FREEZE_TEMPLATE *******************************/

int get_freeze_template(dataptr dz)
{
	int cc, vc;
	switch(dz->mode) {						
	case(FRZ_AMP):
		for(cc = 0, vc = 0; cc < dz->clength; cc++, vc += 2)
			dz->windowbuf[0][AMPP] = dz->flbufptr[0][AMPP];
		break;
	case(FRZ_FRQ):
		for(cc = 0, vc = 0; cc < dz->clength; cc++, vc += 2)
			dz->windowbuf[0][FREQ] = dz->flbufptr[0][FREQ];
		break;
	case(FRZ_AMP_AND_FRQ):										
		for(vc = 0; vc < dz->wanted; vc++)
			dz->windowbuf[0][vc] = dz->flbufptr[0][vc];
		break;
	default:
		sprintf(errstr,"unknown mode in get_freeze_template()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/*************************** GET_STARTWINDOW_POSITION ******************************
 *
 * If startwindow is not in this buffer: rewind.
 */

int get_startwindow_position
(int *startwpos,int this_segtime,int *wndws_read_pre_thisbuf,int *windows_in_buf,int buffer_saved,dataptr dz)
{
	int exit_status;
	if((*startwpos = this_segtime - *wndws_read_pre_thisbuf)<0) {
		if((exit_status = sfrewind(windows_in_buf,this_segtime,wndws_read_pre_thisbuf,dz))<0)
			return(exit_status);
		if(buffer_saved) {
			if((exit_status = restore_buffer(dz))<0)
				return(exit_status);
		}
		*startwpos = this_segtime - *wndws_read_pre_thisbuf;
	}
	return(FINISHED);
}

/*************************** FREEZE_DATA ******************************/

int freeze_data(dataptr dz)
{
	int cc, vc;			
	switch(dz->mode) {
	case(FRZ_AMP):
		for( cc = 0 ,vc = 0; cc < dz->clength; cc++, vc += 2) 
			dz->flbufptr[0][AMPP] = dz->windowbuf[0][AMPP];
		break;
	case(FRZ_FRQ):
		for( cc = 0 ,vc = 0; cc < dz->clength; cc++, vc += 2) 
			dz->flbufptr[0][FREQ] = dz->windowbuf[0][FREQ];
		break;
	case(FRZ_AMP_AND_FRQ):
		for(vc = 0; vc < dz->wanted; vc++) 
			dz->flbufptr[0][vc] = dz->windowbuf[0][vc];
		break;
	default:
		sprintf(errstr,"unknown mode in freeze_data()\n");
		return(PROGRAM_ERROR);
	}
	return(CONTINUE);
}

/*************************** SAVE_BUFFER *******************************/

int	save_buffer(dataptr dz)
{
	memmove((char *)(dz->flbufptr[2]),(char *)dz->bigfbuf,(size_t)dz->buflen * sizeof(float));
	return(FINISHED);
}

/***************************** SFREWIND ********************************/

int sfrewind(int *windows_in_buf,int window_sought,int *wndws_read_pre_thisbuf,dataptr dz)
{
	int samps_read, got;
    int big_wsize   = dz->buflen/dz->wanted;
	int zzz = (window_sought/big_wsize) * big_wsize;	/* TRUNCATE to a multiple of bigbufsize (in windows) */

	if((sndseekEx(dz->ifd[0],zzz * dz->wanted, 0))<0) {
		sprintf(errstr,"Window seek failed. sfrewind()\n");
		return(SYSTEM_ERROR);
	}
	*wndws_read_pre_thisbuf = zzz;

	if((samps_read = fgetfbufEx(dz->bigfbuf, dz->buflen,dz->ifd[0],0)) <= 0) {
		if(samps_read<0) {
			sprintf(errstr,"Sound read error.\n");
			return(SYSTEM_ERROR);
		} else {				
			sprintf(errstr,"Window reverse search miscalculation.sfrewind()\n");
			return(PROGRAM_ERROR);
		}
	}
	got 	       = samps_read;
	*windows_in_buf = got/dz->wanted;
	return(FINISHED);
}

/*************************** RESTORE_BUFFER *******************************/

int	restore_buffer(dataptr dz)
{
	memmove((char *)dz->bigfbuf,(char *)(dz->flbufptr[2]),(size_t)dz->buflen * sizeof(float));
	return(FINISHED);
}

/**************************** SPECSTEP ****************************/

int specstep(dataptr dz)
{
	int exit_status;
	int samps_read, got, windows_in_buf, wc;

	while((samps_read = fgetfbufEx(dz->bigfbuf, dz->buflen,dz->ifd[0],0)) > 0) {
		got = samps_read;
    	dz->flbufptr[0] = dz->bigfbuf;
    	windows_in_buf = got/dz->wanted;
 		for(wc=0; wc<windows_in_buf; wc++) {
			if((exit_status = do_specstep(wc,dz))<0)
				return(exit_status);
			dz->total_windows++;
			dz->flbufptr[0] += dz->wanted;
 		}
		if(dz->total_windows % dz->iparam[STEP_STEP]!=0) 	/* at bigbufend, if not at start of step */
			memmove((char *)(dz->windowbuf[0]),(char *)(dz->flbufptr[1]),(size_t)dz->wanted * sizeof(float));
															/* copy the unchanging window into saving buffer */
		if((exit_status = write_exact_samps(dz->bigfbuf,samps_read,dz))<0)
			return(exit_status);
	}  
	return(FINISHED);
}

/**************************** DO_SPECSTEP ***************************/

int do_specstep(int wc,dataptr dz)
{
	if(wc==0) {											/*  if at start of a bigfbuf   */
		if(dz->total_windows % dz->iparam[STEP_STEP]!=0) 	/* and not at start of a step */
			memmove((char *)(dz->flbufptr[0]),(char *)(dz->windowbuf[0]),(size_t)dz->wanted * sizeof(float));
							/* get the saved unchanging window into 1st buf of bigfbuf */
		dz->flbufptr[1] = dz->flbufptr[0];/* point to this window, as unchanging window */
	} else if(dz->total_windows % dz->iparam[STEP_STEP]==0)	 	 /* if at start of a step */
		dz->flbufptr[1] = dz->flbufptr[0];/* point to this window, as unchanging window */
	else												 /* if not at start of a step */
		memmove((char *)(dz->flbufptr[0]),(char *)(dz->flbufptr[1]),(size_t)dz->wanted * sizeof(float));
									/* copy the unchanging window into current window */
	return(FINISHED);
}

/* NEW CODE */
/****************** SPECFREEZE2  ********************
 *
 * (1)	if no freezing needed.
 * (2)	write any complete buffers before next timepoint.
 * (3)	advance to buffer containing freeze-window if ness.
 * (4)	advance within current buffer, to freeze-window.
 * (5)	get number of windows to process.
 * (6)	set endwindow relative to current buffer.
 * (7)	advance within current buffer, to start-window.
 * (8)	process windows from here to end-window.
 * (9)	if current buffer overrun: proceed to next buffer.
 * (10)	readjust window counters to be relative to nextbuffer start.
 * (11)	write data and go to next buffer.
 * (12)	flush buffer.
 */

int specfreeze2(dataptr dz)
{
 	int exit_status;
    int n = 0, m;
	int windows_in_buf, inwindowcnt = 0, outbufwincnt = 0, inbufwincnt = 0;
	int bufwinlen = dz->buflen/dz->wanted; 
	if((exit_status = read_first_buffer(&windows_in_buf,dz))<0)
		return(exit_status);
	dz->flbufptr[0] = dz->bigfbuf;
	dz->flbufptr[1] = dz->flbufptr[2];
	while(n<dz->itemcnt) {
		while(inwindowcnt < dz->lparray[FRZ_FRZTIME][n]) {	 									/* 1 */
			memcpy((char *)dz->flbufptr[1],(char *)dz->flbufptr[0],dz->wanted * sizeof(float));
			if(++inwindowcnt >= dz->wlength) {
				dz->finished = TRUE;
				break;
			} else if(++inbufwincnt >= windows_in_buf) {
				if((exit_status = read_buffer(&windows_in_buf,dz))<0)
					return(exit_status);
				dz->flbufptr[0] = dz->bigfbuf;
				inbufwincnt = 0;
			} else
				dz->flbufptr[0] += dz->wanted;

			if(++outbufwincnt >= bufwinlen) {
				if((exit_status = write_samps(dz->flbufptr[2],dz->buflen,dz))<0)
					return(exit_status);
				dz->flbufptr[1] = dz->flbufptr[2];
				outbufwincnt = 0;
			} else
				dz->flbufptr[1] += dz->wanted;
		}
		if(dz->finished)
			break;
		for(m=0;m<dz->lparray[FRZ_SEGTIME][n];m++) {
			memcpy((char *)dz->flbufptr[1],(char *)dz->flbufptr[0],dz->wanted * sizeof(float));
			if(++outbufwincnt >= bufwinlen) {
				if((exit_status = write_samps(dz->flbufptr[2],dz->buflen,dz))<0)
					return(exit_status);
				dz->flbufptr[1] = dz->flbufptr[2];
				outbufwincnt = 0;
			} else
				dz->flbufptr[1] += dz->wanted;
		}
		n++;
	}
	while(inwindowcnt < dz->wlength) {
		memcpy((char *)dz->flbufptr[1],(char *)dz->flbufptr[0],dz->wanted * sizeof(float));
		if(++outbufwincnt >= bufwinlen) {
			if((exit_status = write_samps(dz->flbufptr[2],dz->buflen,dz))<0)
				return(exit_status);
			dz->flbufptr[1] = dz->flbufptr[2];
			outbufwincnt = 0;
		} else
			dz->flbufptr[1] += dz->wanted;
		inwindowcnt++;
		if(++inbufwincnt >= windows_in_buf) {
			if((exit_status = read_buffer(&windows_in_buf,dz))<0)
				return(exit_status);
			dz->flbufptr[0] = dz->bigfbuf;
			inbufwincnt = 0;
		} else
			dz->flbufptr[0] += dz->wanted;
	}
	if(dz->flbufptr[1] != dz->flbufptr[2]) {
		if((exit_status = write_samps(dz->flbufptr[2],(dz->flbufptr[1] - dz->flbufptr[2]),dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}

/*************************** READ_BUFFER ******************************/

int read_buffer(int *windows_in_buf,dataptr dz)
{
	int samps_read, got;
	if((samps_read = fgetfbufEx(dz->bigfbuf, dz->buflen,dz->ifd[0],0)) < 0) {
		sprintf(errstr,"Can't read samples from input soundfile.\n");
		return(SYSTEM_ERROR);
	}
	got = samps_read;
	*windows_in_buf = got/dz->wanted;    
	return(FINISHED);
}

/* END OF NEW CODE */

