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
#include <simple.h>
#include <cdpmain.h>
#include <formants.h>
#include <speccon.h>
#include <sfsys.h>
#include <string.h>
#include <simple.h>

#define VERYBIG  		(10000.0)
#define SRCFILE			(0)
#define NOISEFILE		(1)
#define COMPAREFILE		(2)

static int  copy_cut_samps(int startwindow,int *endwindow,int windows_per_buf,int endchunksize,dataptr dz);
static int  outer_clean_loop(int whichfile,int no_of_marked_channels,dataptr dz);
static int  adjust_noise_level(dataptr dz);
static int  setup_cleaning_filter(dataptr dz);
static int  reorganise_marks(int *no_of_marked_channels,dataptr dz);
static int  read_noise_data(int windows_in_buffer,dataptr dz);
static int  read_comparison_data(int windows_in_buffer,dataptr dz);
static int  do_cleaning(int windows_in_buffer,int no_of_marked_channels,dataptr dz);
static int  skip_skipwindows(int *wc,int windows_in_buffer,dataptr dz);
static int  generate_magnified_outfile(dataptr dz);
static int  read_cut_samps(dataptr dz);

/*********************** SPECGAIN ***************************/

int specgain(dataptr dz)
{
	int vc;
	for( vc = 0; vc < dz->wanted; vc += 2)
		dz->flbufptr[0][AMPP] = (float)(dz->flbufptr[0][AMPP] * dz->param[GAIN_GAIN]);
	return(FINISHED);
}

/*********************** SPECLIMIT ***************************/

int speclimit(dataptr dz)
{
	int vc;
	for( vc = 0; vc < dz->wanted; vc += 2) {
		if(dz->flbufptr[0][AMPP] < dz->param[LIMIT_THRESH])
			dz->flbufptr[0][AMPP] = 0.0f;
	}
	return(FINISHED);
}

/**************************** SPECCLEAN ***************************/

int specclean(dataptr dz)
{
	int exit_status;
	int no_of_marked_channels = 0;
	if((exit_status = outer_clean_loop(NOISEFILE,no_of_marked_channels,dz))<0)
		return(exit_status);
	if((exit_status = adjust_noise_level(dz))<0)
		return(exit_status);
	switch(dz->mode) {
	case(FILTERING):
		if((exit_status = setup_cleaning_filter(dz))<0)
			return(exit_status);
		break;
	case(COMPARING):
		if((exit_status = outer_clean_loop(COMPAREFILE,no_of_marked_channels,dz))<0)
			return(exit_status);
		if((exit_status = reorganise_marks(&no_of_marked_channels,dz))<0)
			return(exit_status);
		break;
	}
	return outer_clean_loop(SRCFILE,no_of_marked_channels,dz);
}

/**************************** OUTER_CLEAN_LOOP ****************************/

int outer_clean_loop(int whichfile,int no_of_marked_channels,dataptr dz)
{
	int exit_status;
	int samps_read, windows_in_buffer;
    while((samps_read = fgetfbufEx(dz->bigfbuf, dz->big_fsize,dz->ifd[whichfile],0)) > 0) {
    	dz->flbufptr[0]    = dz->bigfbuf;
	   	windows_in_buffer = samps_read/dz->wanted;
		switch(whichfile) {
		case(NOISEFILE):	
			if((exit_status = read_noise_data(windows_in_buffer,dz))<0)
				return(exit_status);
			break;
		case(COMPAREFILE):	
			if((exit_status = read_comparison_data(windows_in_buffer,dz))<0)
				return(exit_status);
			break;
		case(SRCFILE):		
			if((exit_status = do_cleaning(windows_in_buffer,no_of_marked_channels,dz))<0)
				return(exit_status);
			break;
    	default:
    		sprintf(errstr,"Unknown option in outer_clean_loop()\n");
			return(PROGRAM_ERROR);
		}
		if(whichfile==SRCFILE && (samps_read > 0)) {
			if((exit_status = write_exact_samps(dz->bigfbuf,samps_read,dz))<0)
				return(exit_status);
		}
	}  
	if(samps_read<0) {
		sprintf(errstr,"Sound read error.\n");
		return(SYSTEM_ERROR);
	}
	if(whichfile!=SRCFILE)
		dz->time = 0.0f;
	return(FINISHED);
}

/************************ ADJUST_NOISE_LEVEL **********************/

int adjust_noise_level(dataptr dz)
{
	int cc;
	for( cc = 0; cc < dz->clength; cc++)
		dz->amp[cc] = (float)(dz->amp[cc] * dz->param[CL_GAIN]);
	return(FINISHED);
}

/************************* SETUP_CLEANING_FILTER ***************************/

int setup_cleaning_filter(dataptr dz)
{
	int cc, vc;
	double chtop   = dz->chwidth/2.0;
	if(chtop < dz->param[CL_FRQ])
		dz->amp[0] = 0.0f;
	for( cc = 1, vc = 2; cc < dz->clength; cc++, vc +=2) {
		chtop += dz->chwidth;
		if(chtop < dz->param[CL_FRQ])
			dz->amp[cc] = 0.0f;
	}
	return(FINISHED);
}

/************************ REORGANISE_MARKS **********************
 *
 * Instead of keeping 1024 marks, keep the locations of all SET marks!!
 * Convert locations fron channel no (cc) to buffer location (vc)
 */

int reorganise_marks(int *no_of_marked_channels,dataptr dz)
{
	int n;
	*no_of_marked_channels = 0;
	for(n=0;n < dz->clength;n++) {
		if(dz->iparray[CL_MARK][n]) {
			dz->iparray[CL_MARK][*no_of_marked_channels] = n;
			(*no_of_marked_channels)++;
		}
	}
	if(*no_of_marked_channels==0) {
		sprintf(errstr,"Comparison file is already clean, relative to noisefile.\n");
		return(GOAL_FAILED);
	}
	for(n=0;n < *no_of_marked_channels; n++)		/* change marks from cc to vc */
		dz->iparray[CL_MARK][n] *= 2;
	return(FINISHED);
}

/************************* READ_NOISE_DATA ***************************/

int read_noise_data(int windows_in_buffer,dataptr dz)
{
	int wc;
	int cc, vc;
	for(wc=0; wc<windows_in_buffer; wc++) {
		for( cc = 0 ,vc = 0; cc < dz->clength; cc++, vc += 2) {
			if(dz->flbufptr[0][AMPP] > dz->amp[cc]) 
				dz->amp[cc] = dz->flbufptr[0][AMPP];
		}
		dz->flbufptr[0] += dz->wanted;
	}
	return(FINISHED);
}

/************************* READ_COMPARISON_DATA ***************************/

int read_comparison_data(int windows_in_buffer,dataptr dz)
{
	int wc;
	int cc, vc;
	for(wc=0; wc<windows_in_buffer; wc++) {
		for( cc = 0 ,vc = 0; cc < dz->clength; cc++, vc += 2) {
			if(dz->flbufptr[0][AMPP] > dz->amp[cc])
				dz->iparray[CL_MARK][cc] = 0;
		}
		dz->flbufptr[0] += dz->wanted;
	}
	return(FINISHED);
}
	
/************************* DO_CLEANING ***************************/

int do_cleaning(int windows_in_buffer,int no_of_marked_channels,dataptr dz)
{
	int exit_status;
	int wc = 0;
	int n, cc, vc;
	switch(dz->mode) {
	case(COMPARING):
		for(wc=0; wc<windows_in_buffer; wc++) {
			for(n=0;n<no_of_marked_channels;n++)
				dz->flbufptr[0][dz->iparray[CL_MARK][n]] = 0.0F;
			dz->flbufptr[0] += dz->wanted;
		}
		break;
	case(FROMTIME):
	case(ANYWHERE):
		wc = 0;
		if(!dz->finished) {
			if((exit_status = skip_skipwindows(&wc,windows_in_buffer,dz))<0)
				return(exit_status);
			if(exit_status==FINISHED)
				break;
		}
		/* fall thro */
	case(FILTERING):
		for(; wc<windows_in_buffer; wc++) {
			for(cc=0, vc = 0;cc < dz->clength; cc++, vc += 2) {
				if(dz->flbufptr[0][AMPP] < dz->amp[cc]) {
					dz->flbufptr[0][AMPP] = 0.0F;
					if(dz->mode==FROMTIME)
						dz->amp[cc] = (float)VERYBIG;
				}
			}
			dz->flbufptr[0] += dz->wanted;
		}
		break;
	default:
		sprintf(errstr,"unknown case in do_cleaning()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/************************ SKIP_SKIPWINDOWS **********************/

int skip_skipwindows(int *wc,int windows_in_buffer,dataptr dz)
{
	int step;
	if((step = dz->iparam[CL_SKIPW] - dz->total_windows) >= windows_in_buffer) {
		dz->total_windows += windows_in_buffer;
		dz->time += dz->frametime * windows_in_buffer;
		return(FINISHED);
	} else {
		dz->total_windows += step;
		*wc = step;
		dz->time += dz->frametime * step;
		dz->flbufptr[0] += step * dz->wanted;
		dz->finished = TRUE;		
	}
	return(CONTINUE);
}

/************************* SPECCUT ***************************/

int speccut(dataptr dz)
{
	int exit_status;
	int samps_remain, bufs_to_skip, endchunksize;
	int startwindow = (int)(dz->param[CUT_STIME]/dz->frametime);
	int endwindow   = (int)(dz->param[CUT_ETIME]/dz->frametime);
	int windows_per_buf = dz->big_fsize/dz->wanted;
	int w_to_skip = 0;
	endwindow   = min(endwindow,dz->wlength);
	if((bufs_to_skip = startwindow/windows_per_buf) > 0) { /* TRUNCATE */
		w_to_skip = bufs_to_skip * windows_per_buf;
		if((sndseekEx(dz->ifd[0],w_to_skip * dz->wanted,0))<0) {
			sprintf(errstr,"Window seek failed: speccut().\n");
			return(SYSTEM_ERROR);
		}
	}
	startwindow -= w_to_skip;
	endwindow   -= w_to_skip;
	if((exit_status = read_cut_samps(dz))<0)
		return(exit_status);
	dz->flbufptr[0] = dz->bigfbuf + (startwindow * dz->wanted);
	endchunksize  = windows_per_buf - startwindow;
	dz->flbufptr[1] = dz->flbufptr[2];
	if(endwindow > 0) {
		do {
			if((exit_status = copy_cut_samps(startwindow,&endwindow,windows_per_buf,endchunksize,dz))<0)
				return(exit_status);
		} while(exit_status==CONTINUE);
	}
	if((samps_remain = dz->flbufptr[1] - dz->flbufptr[2]) > 0) {
		if((exit_status = write_samps(dz->flbufptr[2],samps_remain,dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}


/************************* COPY_CUT_SAMPS ***************************/

int copy_cut_samps(int startwindow,int *endwindow,int windows_per_buf,int endchunksize,dataptr dz)
{
	int exit_status;
	if(*endwindow >= windows_per_buf) {
		memmove((char *)dz->flbufptr[1],(char *)dz->flbufptr[0],(size_t)(endchunksize * dz->wanted * sizeof(float)));
		dz->flbufptr[1] += endchunksize * dz->wanted;
		dz->flbufptr[0]  = dz->bigfbuf;
		if((exit_status = read_cut_samps(dz))<0)
			return(exit_status);
		*endwindow -= windows_per_buf;
	} else {
		memmove((char *)dz->flbufptr[1],(char *)dz->flbufptr[0],(size_t)((*endwindow - startwindow) * dz->wanted * sizeof(float)));
		dz->flbufptr[1] += (*endwindow - startwindow) * dz->wanted;
		return(FINISHED);
	}
	if(*endwindow >= startwindow) {
		if(startwindow > 0)
			memmove((char *)dz->flbufptr[1],(char *)dz->flbufptr[0],(size_t)(startwindow * dz->wanted * sizeof(float)));
		if((exit_status = write_exact_samps(dz->flbufptr[2],dz->big_fsize,dz))<0)
			return(exit_status);
		dz->flbufptr[1]  = dz->flbufptr[2];
		dz->flbufptr[0] += startwindow * dz->wanted;
	} else {
		memmove((char *)dz->flbufptr[1],(char *)dz->flbufptr[0],(size_t)(*endwindow * dz->wanted * sizeof(float)));
		dz->flbufptr[1] += *endwindow * dz->wanted;
		return(FINISHED);
	}
	return(CONTINUE);
}

/****************************** SPECGRAB_OR_MAGNIFY ***************************/

int specgrab_or_magnify(int frztime_paramno,dataptr dz)
{
	int exit_status;
	int frzwindow, bufs_to_seek, samps_read;
	int big_wsize = dz->big_fsize/dz->wanted;
	frzwindow = (int)(dz->param[frztime_paramno]/dz->frametime); 		/* truncate */
	frzwindow = min(frzwindow,dz->wlength-1); /* if time > filelen, get last window */
	bufs_to_seek = (frzwindow * dz->wanted)/dz->big_fsize;				/* truncate */
	if(sndseekEx(dz->ifd[0],bufs_to_seek * dz->big_fsize,0)<0) {
		sprintf(errstr,"seek failed in specgrab_or_magnify().\n");
		return(SYSTEM_ERROR);
	}
	if((samps_read = fgetfbufEx(dz->bigfbuf,dz->big_fsize,dz->ifd[0],0)) < dz->wanted) {
		if(samps_read<0) {
			sprintf(errstr,"Sound read error.\n");
			return(SYSTEM_ERROR);
		} else {
			sprintf(errstr,"Problem reading infile data in specgrab_or_magnify().\n");
			return(PROGRAM_ERROR);
		}
	}
	frzwindow -= bufs_to_seek * big_wsize;
	dz->flbufptr[0] = dz->bigfbuf + (frzwindow * dz->wanted);
	switch(dz->process) {
	case(GRAB):
		if((exit_status = write_samps(dz->flbufptr[0],dz->wanted,dz))<0)
			return(exit_status);
		break;
	case(MAGNIFY):
    	dz->total_windows = round(dz->param[MAG_DUR]/dz->frametime);
		return generate_magnified_outfile(dz);
		break;
	default:
		sprintf(errstr,"unknown application in specgrab_or_magnify()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/******************************** GENERATE_MAGNIFIED_OUTFILE **********************************/


int generate_magnified_outfile(dataptr dz)
{
    int exit_status;
    int wc, windows_left;
    int vc, cc, written_whole_buffer = 0;
    int w_to_buf = dz->big_fsize/dz->wanted;
    float *bigbufend = dz->bigfbuf + dz->big_fsize;

	memmove((char *)dz->windowbuf[0],(char *)dz->flbufptr[0],(size_t)(dz->wanted * sizeof(float)));
    dz->flbufptr[0]  = dz->bigfbuf;
    for(wc=0; wc<dz->total_windows; wc++) {
		memmove((char *)dz->flbufptr[0],(char *)dz->windowbuf[0],(size_t)(dz->wanted * sizeof(float)));
        if((dz->flbufptr[0] += dz->wanted) >= bigbufend) {
            if(!written_whole_buffer) {
                for( cc = 0 ,vc = 0; cc < dz->clength; cc++, vc += 2)
                    dz->bigfbuf[vc] = 0.0F;                                 /* ZERO AMPS IN WINDOW ONE */
            }
			if((exit_status = write_exact_samps(dz->bigfbuf,dz->big_fsize,dz))<0)
                return(exit_status);
            if(!written_whole_buffer)                                       /* RESTORE DATA IN WINDOW ONE */
				memmove((char *)dz->bigfbuf,(char *)dz->windowbuf[0],(size_t)(dz->wanted * sizeof(float)));
            written_whole_buffer = 1;
            dz->flbufptr[0] = dz->bigfbuf;
		}
    }
    if(!written_whole_buffer) {
		for( cc = 0 ,vc = 0; cc < dz->clength; cc++, vc += 2)
            dz->bigfbuf[vc] = 0.0F;                         /* ZERO AMPS IN WINDOW ONE */
		if(dz->flbufptr[0] -  dz->bigfbuf > 0) {
			if((exit_status = write_samps(dz->bigfbuf,dz->flbufptr[0] -  dz->bigfbuf,dz))<0)
				return(exit_status);
		}
    } else {
        if((windows_left  = dz->total_windows % w_to_buf)> 0) {
            if((exit_status = write_samps(dz->bigfbuf,windows_left * dz->wanted,dz))<0)
                return(exit_status);
        }
    }
    return(FINISHED);
}

/************************* READ_CUT_SAMPS ***************************/

int read_cut_samps(dataptr dz)
{
	if(fgetfbufEx(dz->bigfbuf,dz->big_fsize,dz->ifd[0],0) < 0) {
		sprintf(errstr,"Window read failed: read_cut_samps().\n");
		return(SYSTEM_ERROR);
	}
	return(FINISHED);
}
