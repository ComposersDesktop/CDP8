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
#include <processno.h>
#include <modeno.h>
#include <arrays.h>
#include <flags.h>
#include <specinfo.h>
#include <cdpmain.h>
#include <formants.h>
#include <speccon.h>
#include <sfsys.h>
#include <osbind.h>
#include <string.h>
#include <specinfo.h>

static int  inner_level_loop(int windows_in_buf,dataptr dz);
static int  do_speclevel(dataptr dz);
static int  normalise_as_transfering_to_sndbuf_and_write_to_file(dataptr dz);
static int  read_another_buffer(int *totalwindows_in_buf,dataptr dz);
static int  zero_energy_bands(dataptr dz);
static int  print_energy_bands(dataptr dz);
static int  inner_textout_only_loop(int *peakscore,int *descnt,int windows_in_buf,dataptr dz);
static int  output_peak_data(dataptr dz);
static int  specpeak(dataptr dz);
static int  specprint(dataptr dz);
static int  specreport(int *peakscore,int *descnt,dataptr dz);
static int  find_maximum(int *maxloc,dataptr dz);
static int  print_window(int windowcnt,dataptr dz);
static int  report_stable_peaks(int *peakscore,int *descnt,dataptr dz);
static int  sort_peaks(int *peakscore,int *descnt,dataptr dz) ;
static int  sort_report_peaks_by_amplitude(int pkcnt_here,dataptr dz);
static int  sort_report_peaks_by_frq(int pkcnt_here,dataptr dz);
static int	show_new_peaks(int pkcnt_here,dataptr dz);
static int  print_peaks(int pkcnt_here,dataptr dz);

/****************************** SPECWCNT ***************************/

int specwcnt(dataptr dz)
{
	sprintf(errstr,"File contains %d windows\n",dz->wlength);
	return(FINISHED);
}

/************************* SPECCHAN ***************************/

int specchan(dataptr dz)
{
	int thischan = (int)((dz->param[CHAN_FRQ]+(dz->halfchwidth))/dz->chwidth);	/* TRUNCATE */
	sprintf(errstr,"%.2lf Hz is in channel %d\n",dz->param[CHAN_FRQ],thischan);
	return(FINISHED);
}

/************************* SPECFRQ ***************************/

int specfrq(dataptr dz)
{
 	double frq = (double)dz->iparam[FRQ_CHAN] * dz->chwidth;
	sprintf(errstr,"%.2lf Hz is centre frq of channel %d\n",frq,dz->iparam[FRQ_CHAN]);
	return(FINISHED);
}

/**************************** SPECLEVEL ****************************/

int speclevel(dataptr dz)
{
	int exit_status;
	int samps_read, windows_in_buf;

	if(sloom)
		dz->total_samps_read = 0L;

	while((samps_read = fgetfbufEx(dz->bigfbuf,dz->big_fsize,dz->ifd[0],0)) > 0) {

		if(sloom)
			dz->total_samps_read += samps_read;

    	dz->flbufptr[0] = dz->bigfbuf;
    	windows_in_buf = samps_read/dz->wanted;
    	if((exit_status = inner_level_loop(windows_in_buf,dz))<0)
			return(exit_status);
		if(dz->finished)
		   break;
	}  
	if(samps_read<0) {
		sprintf(errstr,"Sound read error.\n");
		return(SYSTEM_ERROR);
	}
	if((exit_status = normalise_as_transfering_to_sndbuf_and_write_to_file(dz))<0)
		return(exit_status);
	return(FINISHED);
}

/**************************** INNER_LEVEL_LOOP ****************************/

int inner_level_loop(int windows_in_buf,dataptr dz)
{
	int exit_status;
	int wc;
   	for(wc=0; wc<windows_in_buf; wc++) {
		if((exit_status = do_speclevel(dz))<0)
			return(exit_status);
		dz->flbufptr[0] += dz->wanted;
		dz->total_windows++;
	}
	return(FINISHED);
}

/**************************** DO_SPECLEVEL ****************************/

int do_speclevel(dataptr dz)
{

#define VERY_BIG  (10000000.0)

	int vc;
	dz->parray[0][dz->itemcnt] = 0.0;
	for(vc = 0; vc < dz->wanted; vc += 2) 
		dz->parray[0][dz->itemcnt] += dz->flbufptr[0][vc];
	if(dz->parray[0][dz->itemcnt] > VERY_BIG) {
		sprintf(errstr,"Apparent(?) huge signal level in analysis window %d. do_speclevel()\n",dz->itemcnt + 1);
		return(PROGRAM_ERROR);
	}
	if(++dz->itemcnt > dz->wlength) {
		sprintf(errstr,"Storage arithmetic problem in do_speclevel().\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/********** NORMALISE_AS_TRANSFERING_TO_SNDBUF_AND_WRITE_TO_FILE *********/

int normalise_as_transfering_to_sndbuf_and_write_to_file(dataptr dz)
{

	double maxwindow = 0.0, normaliser;
	int n;
	for(n=0;n<dz->wlength;n++) {
		if(dz->parray[0][n] > maxwindow)
		maxwindow = dz->parray[0][n];
	}
	if(flteq(maxwindow,0.0)) {
		sprintf(errstr,"No signal level in analysis file.\n");
		return(DATA_ERROR);
	}
	normaliser = F_MAXSAMP/maxwindow;
	for(n=0;n<dz->wlength;n++)
		dz->sndbuf[n] = (float)(dz->parray[0][n] * normaliser);
	return write_samps(dz->sndbuf,dz->wlength,dz);
}

/**************************** SPECOCTVU ****************************/

int specoctvu(dataptr dz)
{
	int exit_status;
	int totalwindows_in_buf, windownumber_in_buf;
	int x, q, n;
	double topfrq;
	if((exit_status = read_another_buffer(&totalwindows_in_buf,dz))!=CONTINUE) {
		if(exit_status==FINISHED)
			fprintf(stdout,"WARNING: No data in input file.\n");
		return(exit_status);
	}
	windownumber_in_buf = 0;
	while(dz->total_windows < dz->wlength && totalwindows_in_buf > 0) {
		if((exit_status = zero_energy_bands(dz))<0)
			return(exit_status);
		for(x = 0; x<dz->iparam[OCTVU_TBLOK];x++) {
			if(windownumber_in_buf >= totalwindows_in_buf) {
				if((exit_status = read_another_buffer(&totalwindows_in_buf,dz))!=CONTINUE)
					break;
				windownumber_in_buf = 0;
			}
			q = n = 0;
			topfrq = dz->param[OCTVU_BBBTOP];
			for(q=0;q<dz->itemcnt;q++) {
				for(n = dz->iparray[OCTVU_CHBBOT][q]; n < dz->iparray[OCTVU_CHBTOP][q]; n += 2) {
					if(fabs(dz->flbufptr[0][n+1]) < topfrq)
						dz->parray[OCTVU_ENERGY][q] += dz->flbufptr[0][n];
				}
				topfrq *= 2.0;
			}
			dz->flbufptr[0] += dz->wanted;
			windownumber_in_buf++;
			if(++dz->total_windows >= dz->wlength) {
				break;
			}
		}
		if(exit_status<0)
			return(exit_status);
		if(x>0 && (exit_status = print_energy_bands(dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}

/**************************** READ_ANOTHER_BUFFER ****************************/

int read_another_buffer(int *totalwindows_in_buf,dataptr dz)
{
	int samps_read;
	if((samps_read = fgetfbufEx(dz->bigfbuf, dz->big_fsize,dz->ifd[0],0)) <= 0) {
		if(samps_read<0) {
			sprintf(errstr,"Sound read error.\n");
			return(SYSTEM_ERROR);
		}
		return(FINISHED);
	}
	dz->flbufptr[0] 		 = dz->bigfbuf;
	*totalwindows_in_buf = samps_read/dz->wanted;
	return(CONTINUE);
}

/**************************** ZERO_ENERGY_BANDS ****************************/

int zero_energy_bands(dataptr dz)
{
	int n;
	for(n=0;n<dz->itemcnt;n++)
		dz->parray[OCTVU_ENERGY][n] = 0.0;
	return(FINISHED);
}

/******************** PRINT_ENERGY_BANDS ***********************/

int print_energy_bands(dataptr dz)
{

#define SCALEFACT (5000.0)

	int n;
	char temp[64];
	double output;
	errstr[0] = ENDOFSTR;
	for(n=0;n<dz->itemcnt;n++) {
		output = SCALEFACT * dz->parray[OCTVU_ENERGY][n];
		if(output>0)	{							
			output = log(output);
			if(sloom) {
				sprintf(temp,"%.3lf",output);
				strcat(errstr,temp);
			}
			fprintf(dz->fp,"%.3lf",output);
		} else {
			if(sloom) {
				sprintf(temp,"-----"/*,output*/);
				strcat(errstr,temp);
			}
			fprintf(dz->fp,"-----");
		}
		if(n < dz->itemcnt-1) {
			if(sloom)
				strcat(errstr,"\t");
			fprintf(dz->fp,"\t");
		} else {
			if(sloom) {
				fprintf(stdout,"INFO: %s\n",errstr);
				fflush(stdout);
				errstr[0] = ENDOFSTR;
			}
			fprintf(dz->fp,"\n");
		}
	}
	return(FINISHED);
}

/**************************** OUTER_TEXTOUT_ONLY_LOOP ****************************/

int outer_textout_only_loop(dataptr dz)
{
	int exit_status;
	int samps_read, windows_in_buf;
	int peakscore = 0;
	int descnt = 0;
	dz->time = 0.0f;
	while((samps_read = fgetfbufEx(dz->bigfbuf, dz->big_fsize,dz->ifd[0],0)) > 0) {
    	dz->flbufptr[0] = dz->bigfbuf;
    	windows_in_buf = samps_read/dz->wanted;
    	if((exit_status = inner_textout_only_loop(&peakscore,&descnt,windows_in_buf,dz))<0)
			return(exit_status);
		if(dz->finished)
		   break;
	}
	if(samps_read<0) {
		sprintf(errstr,"Sound read error.\n");
		return(SYSTEM_ERROR);
	}
	if(dz->process == PEAK && dz->itemcnt) {
 		if((exit_status = output_peak_data(dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}

/**************************** INNER_TEXTOUT_ONLY_LOOP ****************************/

int inner_textout_only_loop(int *peakscore,int *descnt,int windows_in_buf,dataptr dz)
{
	int exit_status;
	int wc;
   	for(wc=0; wc<windows_in_buf; wc++) {
		if(dz->total_windows==0) {
			if((exit_status = skip_or_special_operation_on_window_zero(dz))<0)
				return(exit_status);
			if(exit_status==TRUE) {
				dz->flbufptr[0] += dz->wanted;
				dz->total_windows++;
				continue;
			}
		}
		if((exit_status = read_values_from_all_existing_brktables((double)dz->time,dz))<0)
			return(exit_status);
		switch(dz->process) {
		case(PEAK):		exit_status = specpeak(dz);	  break;
		case(PRINT):	exit_status = specprint(dz);  break;
		case(REPORT):	exit_status = specreport(peakscore,descnt,dz); break;
		default:
			sprintf(errstr,"unknown process in inner_textout_only_loop()\n");
			return(PROGRAM_ERROR);
		}
		if(exit_status<0)
			return(exit_status);
		dz->flbufptr[0] += dz->wanted;
		dz->total_windows++;
		dz->time = (float)(dz->time + dz->frametime);
	}
	return(FINISHED);
}

/******************************** OUTPUT_PEAK_DATA *************************/

int output_peak_data(dataptr dz)
{
	int exit_status;
	int here;
	int k;
	double frqtop, frqbot, starttime;
	starttime = dz->param[PEAK_ENDTIME];
	dz->param[PEAK_ENDTIME]  += (dz->iparam[PEAK_TGROUP] * dz->frametime);
	if((exit_status = find_maximum(&here,dz))<0)
		return(exit_status);
	if(here==0)
		frqbot = dz->param[PEAK_CUTOFF];
	else
		frqbot = dz->parray[PEAK_BAND][here-1];
	frqtop = dz->parray[PEAK_BAND][here];

	if(sloom) {
		fprintf(stdout,"INFO: WINDOW\t%.4lf\t%.4lf\t:PEB \t%.4lf\tTO\t%.4lf\n",starttime, dz->param[PEAK_ENDTIME], frqbot, frqtop);
		fflush(stdout);
	}
	fprintf(dz->fp,"WINDOW\t%.4lf\t%.4lf\t:PEB \t%.4lf\tTO\t%.4lf\n",starttime, dz->param[PEAK_ENDTIME], frqbot, frqtop);
	for(k=0;k<dz->iparam[PEAK_BANDCNT];k++)
		dz->parray[PEAK_MAXI][k] = 0.0;
	return(FINISHED);
}

/****************************** SPECPEAK ****************************/

int specpeak(dataptr dz)
{
	int exit_status;
	double thisamp, sensitivity_of_ear;
	int cc, vc, n;
	for( cc = 0 ,vc = 0; cc < dz->clength; cc++, vc += 2) {
		if(dz->flbufptr[0][FREQ]<dz->param[PEAK_CUTOFF] || dz->flbufptr[0][FREQ]>dz->nyquist)
			continue;
		n = 0;
		while(dz->flbufptr[0][FREQ] > dz->parray[PEAK_BAND][n])
			n++;
		thisamp = dz->flbufptr[0][AMPP];
		if(dz->vflag[PEAK_HEAR]) {
			if((exit_status = getspecenvamp(&sensitivity_of_ear,dz->flbufptr[0][FREQ],0,dz))<0)
				return(exit_status);
			thisamp *= sensitivity_of_ear;
		}
		dz->parray[PEAK_MAXI][n] += thisamp;
	}
	if(++dz->itemcnt >= dz->iparam[PEAK_TGROUP]) {
		if((exit_status = output_peak_data(dz))<0)
			return(exit_status);
		dz->itemcnt = 0;
	}
	return(FINISHED);
}

/****************************** SPECPRINT ****************************/

int specprint(dataptr dz)
{
	int exit_status;
	if(dz->total_windows >= dz->iparam[PRNT_ENDW]) {
		dz->finished = 1;
		return(FINISHED);
	}
	if(dz->total_windows >= dz->iparam[PRNT_STARTW]) {
		if((exit_status = print_window(dz->total_windows,dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}

/**************************** SPECREPORT ***************************/

int specreport(int *peakscore,int *descnt,dataptr dz)
{
	int exit_status;
	double lofrq, hifrq;
	lofrq = dz->param[REPORT_LOFRQ];
	hifrq = dz->param[REPORT_HIFRQ];
	if(dz->brksize[REPORT_LOFRQ] || dz->brksize[REPORT_HIFRQ]) {
		if(flteq(lofrq,hifrq)) {
			sprintf(errstr,"Frequency limits define a zero-width band at time %.2lf.\n",dz->time);
			return(USER_ERROR);
		}
		if(lofrq > hifrq)
			swap(&lofrq,&hifrq);
	}
	rectify_window(dz->flbufptr[0],dz);
	if((exit_status = extract_specenv(0,0,dz))<0)
		return(exit_status);
	if((exit_status = report_stable_peaks(peakscore,descnt,dz))<0)
		return(exit_status);
	return(FINISHED);
}

/**************************** FIND_MAXIMUM **************************/

int find_maximum(int *maxloc,dataptr dz)
{
	double thismax = -1.0;
	int k;
	*maxloc = -1;
	for(k=0;k<dz->iparam[PEAK_BANDCNT];k++) {
		if(dz->parray[PEAK_MAXI][k] > thismax) {
			thismax = dz->parray[PEAK_MAXI][k];
			*maxloc = k;
		}
	}
/* what if maxloc is NOT SET */
	if(*maxloc < 0) {
		sprintf(errstr,"Failed to find any peak in data\n");
		return(GOAL_FAILED);
	}
	return(FINISHED);
}

/******************************** PRINT_WINDOW *************************/

int print_window(int windowcnt,dataptr dz)
{
	int cc, vc;
	fprintf(dz->fp,"************* WINDOW %d *****************\n",windowcnt);
	for( cc = 0 ,vc = 0; cc < dz->clength; cc++, vc += 2) {
		fprintf(dz->fp,"amp[%d] = %.12f\t",cc,dz->flbufptr[0][AMPP]);
		if(dz->flbufptr[0][FREQ] < .01f)
			fprintf(dz->fp,"frq[%d] = %lf\n",cc,dz->flbufptr[0][FREQ]);
		else
			fprintf(dz->fp,"frq[%d] = %.2lf\n",cc,dz->flbufptr[0][FREQ]);
    }
	return(FINISHED);
}

/****************************** REPORT_STABLE_PEAKS ***************************/

int report_stable_peaks(int *peakscore,int *descnt,dataptr dz)
{
	int exit_status;
	int thispkcnt;
	/* shuffle back stored specenvs */
	memmove((char *)dz->stable->spec[0],
	(char *)dz->stable->spec[1],dz->iparam[REPORT_SL1] * (dz->infile->specenvcnt * sizeof(int)));
	/* put latest specenv array here */
	memmove((char *)dz->stable->spec[dz->iparam[REPORT_SL1]],
	(char *)dz->specenvamp,(dz->infile->specenvcnt * sizeof(int)));
	/* shuffle back stored fpks */
	memmove((char *)dz->stable->fpk[0],
	(char *)dz->stable->fpk[1],dz->iparam[REPORT_SL1] * (dz->infile->specenvcnt * sizeof(int)));
	/* shuffle back stored fpk_totals */
	memmove((char *)dz->stable->total_pkcnt,
	(char *)(dz->stable->total_pkcnt+1),dz->iparam[REPORT_SL1] * sizeof(int));
	/* put into last fpk[z] store */
	if((exit_status = extract_formant_peaks2(REPORT_SL1,&thispkcnt,dz->param[REPORT_LOFRQ],dz->param[REPORT_HIFRQ],dz))<0)
		return(exit_status);
	dz->stable->total_pkcnt[dz->iparam[REPORT_SL1]] = thispkcnt;
	if(dz->total_windows >= dz->iparam[REPORT_SL1]) {
		if((exit_status = sort_peaks(peakscore,descnt,dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}


/****************************** SORT_PEAKS ****************6**************/ 

int sort_peaks(int *peakscore,int *descnt,dataptr dz) 
{
	int exit_status;
	int this_pkcnt;
	int n = 0, pkcnt_here;
	if(dz->total_windows<=dz->iparam[REPORT_SL1])
		memset((char *)dz->stable->design_score,0,dz->infile->specenvcnt * sizeof(int));
	if((exit_status = score_peaks(peakscore,REPORT_SL1,REPORT_STABL,dz))<0)
		return(exit_status);
	if(*peakscore==0)
		return(FINISHED);
		 		 	/* Find how many times each peak occurs across the stabilise buffers */
 	if((exit_status = collect_scores(&this_pkcnt,descnt,dz))<0)
		return(exit_status);
		/* Sort these on the basis of the most commonly occuring */
 	if((exit_status = sort_design(this_pkcnt,dz))<0)
		return(exit_status);
	if(this_pkcnt<=dz->itemcnt || (dz->stable->des[dz->itemcnt-1]->score > dz->stable->des[dz->itemcnt]->score)) {
		pkcnt_here = min(this_pkcnt,dz->itemcnt);
		for(n=0;n<pkcnt_here;n++)	/* If most common pkcnt (or less) peaks more common than ALL others */
			dz->peakno[n] = dz->stable->des[n]->chan;/* these are the peaks to keep */
	} else {						/* Else choose amongst equivalent peaks, in terms of amplitude */
		if((exit_status = sort_equivalent_scores(this_pkcnt,dz))<0)
			return(exit_status);
		pkcnt_here = dz->itemcnt;
	}
	switch(dz->mode) {
	case(AMP_ORDERED_TIMED): 
	case(AMP_ORDERED_UNTIMED):
		if((exit_status = sort_report_peaks_by_amplitude(pkcnt_here,dz))<0)
			return(exit_status);
		break;
	case(FRQ_ORDERED_TIMED): 
	case(FRQ_ORDERED_UNTIMED):	
		if((exit_status = sort_report_peaks_by_frq(pkcnt_here,dz))<0)
			return(exit_status);
		break;
	default:
		sprintf(errstr,"unknown case in sort_peaks()\n");
		return(PROGRAM_ERROR);
	}
	if((exit_status = show_new_peaks(pkcnt_here,dz))<0)
		return(exit_status);
	if(dz->total_windows>dz->iparam[REPORT_SL1]) {	/* Logically this should be '>=sl1' : but */
		if((exit_status = unscore_peaks(peakscore,dz))<0) /* don't have to subtract window 0, as NO PEAKS there */
			return(exit_status);
	}
	return(FINISHED);
}

/****************************** SORT_REPORT_PEAKS_BY_AMPLITUDE ******************************/ 

int sort_report_peaks_by_amplitude(int pkcnt_here,dataptr dz)
{
	int n, m, thischan;
	double thisamp;
	for(n=0;n<pkcnt_here;n++) {	 		/* For every channel in the set of possible channels */
		dz->stable->des[n]->amp = 0.0; 				/* and store in des-array assocd with channel */
		thischan = dz->stable->des[n]->chan;
		for(m=0;m<dz->iparam[REPORT_STABL];m++)  		/* sum amplitudes in that chan across all specenv buffers */
			dz->stable->des[n]->amp += dz->stable->spec[m][thischan]; 	/* and store in des-array assocd with channel */
	}
	for(n=1;n<pkcnt_here;n++) {
		thisamp  = dz->stable->des[n]->amp;
		m = n-1;
		while(m >= 0 && dz->stable->des[m]->amp > thisamp) {
			dz->stable->des[m+1]->amp  = dz->stable->des[m]->amp;
			dz->stable->des[m+1]->chan = dz->stable->des[m]->chan;
			m--;
		}
		dz->stable->des[m+1]->amp  = dz->stable->des[n]->amp;
		dz->stable->des[m+1]->chan = dz->stable->des[n]->chan;
	}
	return(FINISHED);
}
 
/****************************** SORT_REPORT_PEAKS_BY_FRQ ***************************/

int sort_report_peaks_by_frq(int pkcnt_here,dataptr dz)
{	
	int n, m, k;
	for(n=0;n<pkcnt_here-1;n++) {
		for(m=n+1;m<pkcnt_here;m++) {
			if(dz->specenvfrq[dz->peakno[n]] > dz->specenvfrq[dz->peakno[m]]) {
				k             = dz->peakno[m];
				dz->peakno[m] = dz->peakno[n];
				dz->peakno[n] = k;
			}
		}
	}
	return(FINISHED);
}

/****************************** SHOW_NEW_PEAKS ***************************/

int	show_new_peaks(int pkcnt_here,dataptr dz)
{
	int exit_status;
	int n, m, newpeak = 0;					 /*RWD added init */
	if(pkcnt_here!=dz->iparam[REPORT_LAST_PKCNT_HERE]) {
		if((exit_status = print_peaks(pkcnt_here,dz))<0)
			return(exit_status);
	} else {
		for(n=0;n<pkcnt_here;n++) {
			newpeak = 1; 
			for(m=0;m<dz->iparam[REPORT_LAST_PKCNT_HERE];m++) {
				if(dz->peakno[n] == dz->lastpeakno[m]) {
				   	newpeak = 0;
					break;
				}
			}
			if(newpeak)
				break;
		}
		if(newpeak) {
			if((exit_status = print_peaks(pkcnt_here,dz))<0)
				return(exit_status);
		}
	}
	dz->iparam[REPORT_LAST_PKCNT_HERE] = pkcnt_here;
	for(n = 0;n < pkcnt_here; n++)
		dz->lastpeakno[n] = dz->peakno[n];
	return(FINISHED);
}

/****************************** PRINT_PEAKS ***************************/

int print_peaks(int pkcnt_here,dataptr dz)
{
	int n;
	char temp[200];
	switch(dz->mode) {
	case(FRQ_ORDERED_TIMED): 
	case(AMP_ORDERED_TIMED):
		if(sloom)
			sprintf(errstr,"%.3lf secs : ",dz->time - (dz->stable->offset * dz->frametime));
		fprintf(dz->fp,"%.3lf secs : ",dz->time - (dz->stable->offset * dz->frametime));
	case(FRQ_ORDERED_UNTIMED): 
	case(AMP_ORDERED_UNTIMED):
		for(n=0;n<pkcnt_here;n++) {
			if(sloom) {
				sprintf(temp,"\t%.2f",dz->specenvfrq[dz->peakno[n]]);
				strcat(errstr,temp);
			}
			fprintf(dz->fp,"\t%.2f",dz->specenvfrq[dz->peakno[n]]);
			if(n && n%8==0) {
				if(sloom) {
					fprintf(stdout,"INFO: %s\n",errstr);
					fflush(stdout);
					sprintf(errstr,"\t\t");
				}
				fprintf(dz->fp,"\n\t\t");
			}
		}
		break;
	default:
		sprintf(errstr,"Unknown case in print_peaks()\n");
		return(PROGRAM_ERROR);
	}
	if(sloom) {
		fprintf(stdout,"INFO: %s\n",errstr);
		fflush(stdout);
	}
	fprintf(dz->fp,"\n");
	return(FINISHED);
}
