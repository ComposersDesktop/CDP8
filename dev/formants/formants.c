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
#include <fmnts.h>
#include <cdpmain.h>
#include <formants.h>
#include <speccon.h>
#include <sfsys.h>
#include <string.h>
#include <fmnts.h>

static int  inner_form_loop(int windows_in_buf,dataptr dz);
static int  do_specform(dataptr dz);
static int  form_filter(dataptr dz);
static int  form_gain(dataptr dz);
static int  vocode_channel(int vc,dataptr dz);
static int	eliminate_out_of_range_data(dataptr dz);
static int	vocogain(dataptr dz);
static int  reset_file_to_start_and_pointers_to_after_descriptor_blocks(int *windows_in_buf,dataptr dz);
static int  renormalise_formsee(dataptr dz);
//static int  setformval(int *thissamp,float inval,double normaliser,double adjuster);
static int  setformval(float *thissamp,float inval,double normaliser,double adjuster);
static int  do_specformants(dataptr dz);
static int  extract_formant_and_place_in_output_buffer(int formantlen,dataptr dz);

static int  get_max_formantval(dataptr dz);
static int  initialise_the_formant_data(int *windows_in_buf,dataptr dz);

/****************************** SPECFORMANTS ***************************/

int specformants(dataptr dz)
{
	int exit_status;
	int wc, windows_in_buf;
	int OK  = 1;
	if(sloom)
		dz->total_samps_read = 0L;
	dz->time = 0.0f;
	while(OK) {
		if((exit_status = read_samps(dz->bigfbuf,dz)) < 0)
			return exit_status;
		if(dz->ssampsread <= 0)
			break;
		dz->flbufptr[0] = dz->bigfbuf;
		windows_in_buf = dz->ssampsread/dz->wanted;    
		for(wc=0; wc<windows_in_buf; wc++) {
			if(dz->total_windows==0) {
				if((exit_status = skip_or_special_operation_on_window_zero(dz))<0)
				 	return(exit_status);
				if(exit_status==TRUE) {
					dz->flbufptr[0] += dz->wanted;
					dz->total_windows++;
					dz->time = (float)(dz->time + dz->frametime);
					continue;
				}
			}
			if((exit_status = do_specformants(dz))<0)
				return(exit_status);
			dz->flbufptr[0]  += dz->wanted;			/* move along bigfbuf */
			dz->total_windows++;
			dz->time = (float)(dz->time + dz->frametime);
		}
	}
	if(dz->iparam[FMNT_SAMPS_TO_WRITE] > 0) {
		if((exit_status = write_samps(dz->flbufptr[2],dz->iparam[FMNT_SAMPS_TO_WRITE],dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}

/**************************** DO_SPECFORMANTS ***************************
 *
 * Extract formants from an analfile.
 */

int do_specformants(dataptr dz)
{
	int exit_status;
	rectify_window(dz->flbufptr[0],dz);
	if((exit_status = extract_specenv(0,0,dz))<0)
		return(exit_status);
	memmove((char *)dz->flbufptr[1],(char *)dz->specenvamp,(dz->infile->specenvcnt * sizeof(float)));
	if((dz->iparam[FMNT_SAMPS_TO_WRITE] += dz->infile->specenvcnt) >= dz->buflen2) {
		if((exit_status = write_exact_samps(dz->flbufptr[2],dz->buflen2,dz))<0)
			return(exit_status);
		dz->iparam[FMNT_SAMPS_TO_WRITE] = 0L;
		dz->flbufptr[1] = dz->flbufptr[2];
	} else
		dz->flbufptr[1] += dz->infile->specenvcnt;	/* move along dz->flbufptr[2] */
	return(FINISHED);
}

/****************************** SPECFORM ***************************/

int specform(dataptr dz)
{
	int exit_status, current_status;
	int windows_in_buf;
 
	if(sloom)
		dz->total_samps_read = 0L;

	dz->time = 0.0f;
	if((exit_status = setup_formant_params(dz->ifd[1],dz))<0)
		return(exit_status);

//	if((exit_status = read_samps(dz->bigfbuf,dz)) < 0)	ALREADY READ IN setup_formant_params()
//		return(exit_status);
	while(dz->ssampsread > 0) {
    	dz->flbufptr[0] = dz->bigfbuf;
    	windows_in_buf = dz->ssampsread/dz->wanted;
	   	if((exit_status = inner_form_loop(windows_in_buf,dz))<0) {
			dz->specenvamp2 = (float *)0;
			return(exit_status);
		}
		current_status = exit_status;
		if(dz->flbufptr[0] - dz->bigfbuf > 0) {
			if((exit_status = write_samps(dz->bigfbuf,(dz->flbufptr[0] - dz->bigfbuf),dz))<0) {
				dz->specenvamp2 = NULL;
				return(exit_status);
			}
		}
		if(current_status!=CONTINUE)
			break;
		if((exit_status = read_samps(dz->bigfbuf,dz)) < 0)
			return(exit_status);
	}
 	dz->specenvamp2 = NULL;
	return(FINISHED);
}

/****************************** INNER_FORM_LOOP ***************************/

int inner_form_loop(int windows_in_buf,dataptr dz)
{
	int exit_status;
	int wc;
	double pre_amptotal, post_amptotal;
 	for(wc=0; wc<windows_in_buf; wc++) {
		if(dz->total_windows==0) {
			if((exit_status = move_along_formant_buffer(dz))!=CONTINUE)
				return(exit_status);
			dz->flbufptr[0] += dz->wanted;
			dz->total_windows++;
			dz->time = (float)(dz->time + dz->frametime);
			continue;			/* SKIP FIRST (ZERO-AMP) WINDOW */
		}
		if(dz->mode==FORM_REPLACE) {
			rectify_window(dz->flbufptr[0],dz);
			if((exit_status = extract_specenv(0,0,dz))<0)
				return(exit_status);
		}
		dz->specenvamp2  = dz->flbufptr[1];
		pre_amptotal = post_amptotal = 0.0;
		if((exit_status = get_totalamp(&pre_amptotal,dz->flbufptr[0],dz->wanted))<0)
			return(exit_status);
		if((exit_status = do_specform(dz))<0)
			return(exit_status);
		if((exit_status = form_filter(dz))<0)
				return(exit_status);
		if((exit_status = get_totalamp(&post_amptotal,dz->flbufptr[0],dz->wanted))<0)
			return(exit_status);
		if((exit_status = normalise(pre_amptotal,post_amptotal,dz))<0)
			return(exit_status);
		if(!flteq(dz->param[FORM_GAIN],1.0)) {
			if((exit_status = form_gain(dz))<0)
				return(exit_status);
  				/* NB This check must be BEFORE move_along_formant_buffer() */
		}
		dz->flbufptr[0] += dz->wanted;
  		if(++dz->total_windows >= dz->wlength)
			return(FINISHED);
		if((exit_status = move_along_formant_buffer(dz))!=CONTINUE)
			return(exit_status);
		dz->time = (float)(dz->time + dz->frametime);
	}
	return(CONTINUE);
}

/**************************** DO_SPECFORM ***************************
 *
 * Impose formants onto an analfile.
 */

int do_specform(dataptr dz)
{
	int exit_status;
	int vc;
	double thisspecamp0, thisspecamp1, thisvalue;
	rectify_window(dz->flbufptr[0],dz);
	switch(dz->mode) {
	case(FORM_REPLACE):
		for(vc = 0; vc < dz->wanted ; vc += 2) {
			if((exit_status = getspecenvamp(&thisspecamp0,(double)dz->flbufptr[0][FREQ],0,dz))<0)
				return(exit_status);
			if((exit_status = getspecenvamp(&thisspecamp1,(double)dz->flbufptr[0][FREQ],1,dz))<0)
				return(exit_status);
			if(thisspecamp0 < VERY_TINY_VAL)
				dz->flbufptr[0][AMPP] = 0.0f;
			else {
				if((thisvalue = dz->flbufptr[0][AMPP] * thisspecamp1/thisspecamp0)< VERY_TINY_VAL)
					dz->flbufptr[0][AMPP] = 0.0f;
				else
					dz->flbufptr[0][AMPP] = (float)thisvalue;
			}
		}
		break;
	case(FORM_IMPOSE):
		for(vc = 0; vc < dz->wanted ; vc += 2) {
			if((exit_status = getspecenvamp(&thisspecamp1,(double)dz->flbufptr[0][FREQ],1,dz))<0)
				return(exit_status);
			if((thisvalue = dz->flbufptr[0][AMPP] * thisspecamp1)< VERY_TINY_VAL)
				dz->flbufptr[0][AMPP] = 0.0f;
			else
				dz->flbufptr[0][AMPP] = (float)thisvalue;
		}
		break;
	default:
		sprintf(errstr,"Unknown case in do_specform()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/****************************** FORM_FILTER ***************************/

int form_filter(dataptr dz)
{
	int vc;
	for(vc = 0; vc < dz->wanted ; vc += 2) {
		if(dz->flbufptr[0][FREQ] < dz->param[FORM_FBOT]
			|| dz->flbufptr[0][FREQ] > dz->param[FORM_FTOP])
			dz->flbufptr[0][AMPP] = 0.0f;
	}
	return(FINISHED);
}

/****************************** FORM_GAIN ***************************/

int form_gain(dataptr dz)
{
	int vc;
	for(vc=0;vc<dz->wanted;vc+=2)
		dz->flbufptr[0][AMPP] = (float)(dz->flbufptr[0][AMPP] * dz->param[FORM_GAIN]);
	return(FINISHED);
}

/************************** SPECVOCODE ***********************/

int specvocode(dataptr dz)
{
	int exit_status;
	int vc;
	double pre_amptotal, post_amptotal;
	rectify_window(dz->flbufptr[0],dz);
	if((exit_status = extract_specenv(0,0,dz))<0)
		return(exit_status);
	rectify_window(dz->flbufptr[1],dz);
	if((exit_status = extract_specenv(1,1,dz))<0)
		return(exit_status);
	if((exit_status = get_totalamp(&pre_amptotal,dz->flbufptr[0],dz->wanted))<0)
		return(exit_status);
	for(vc = 0; vc < dz->wanted; vc += 2) {
		if((exit_status = vocode_channel(vc,dz))<0)
			return(exit_status);
	}
	if((exit_status = eliminate_out_of_range_data(dz))<0)
		return(exit_status);
	if((exit_status = get_totalamp(&post_amptotal,dz->flbufptr[0],dz->wanted))<0)
		return(exit_status);
	if((exit_status = normalise(pre_amptotal,post_amptotal,dz))<0)
		return(exit_status);
	return vocogain(dz);
}

/************************** VOCODE_CHANNEL ***********************/

int vocode_channel(int vc,dataptr dz)
{
	int exit_status;
	double thisvalue;
	double thisspecamp1, thisspecamp2;
	if((exit_status = getspecenvamp(&thisspecamp1,(double)dz->flbufptr[0][FREQ],0,dz))<0)
		return(exit_status);
	if((exit_status = getspecenvamp(&thisspecamp2,(double)dz->flbufptr[1][FREQ],1,dz))<0)
		return(exit_status);
	if(thisspecamp1 <= 0.0)
		dz->flbufptr[0][AMPP] = 0.0f;
	else {
		if((thisvalue = dz->flbufptr[0][AMPP] * thisspecamp2/thisspecamp1)  < VERY_TINY_VAL)
			dz->flbufptr[0][AMPP] = 0.0f;
		else
			dz->flbufptr[0][AMPP] = (float)thisvalue;
	}
	return(FINISHED);
}

/************************** ELIMINATE_OUT_OF_RANGE_DATA ***********************/

int	eliminate_out_of_range_data(dataptr dz)
{
	int vc;
	for(vc = 0;vc < dz->wanted; vc+=2) {
		if(dz->flbufptr[0][FREQ] < dz->param[VOCO_LOF] || dz->flbufptr[0][FREQ] > dz->param[VOCO_HIF])
			dz->flbufptr[0][AMPP] = 0.0f;
	}
	return(FINISHED);
}

/************************** VOCOGAIN ***********************/

int	vocogain(dataptr dz)
{
	int vc;
	if(!flteq(dz->param[VOCO_GAIN],1.0))	{
		for(vc=0;vc<dz->wanted;vc+=2)
			dz->flbufptr[0][AMPP] = (float)(dz->flbufptr[0][AMPP] * dz->param[VOCO_GAIN]);
	}
	return(FINISHED);
}

/**************************** SPECFMNTSEE ***************************/

int specfmntsee(dataptr dz)
{
	int exit_status;
	double thisval;
	int   windows_in_buf, wc;
	int    cc;
	double normaliser;
//	double adjuster   = (double)MAXSAMP/log(10000.0);
	double adjuster   = F_MAXSAMP/log(10000.0);
	int outcnt = 0, outbufcnt = 0;
	if(sloom)
		dz->total_samps_read = 0L;
	if((exit_status = initialise_the_formant_data(&windows_in_buf,dz))<0)
		return(exit_status);
	normaliser = 9999.0/dz->param[FMNTSEE_MAX];
	dz->time = 0.0f;
	do {
		for(wc=0; wc<windows_in_buf; wc++) {
			if(dz->total_windows==0) {
				if((exit_status = skip_or_special_operation_on_window_zero(dz))<0)
					return(exit_status);
				if(exit_status==TRUE) {
					dz->flbufptr[0] += dz->wanted;
					dz->total_windows++;
					dz->time = (float)(dz->time + dz->frametime);
					continue;
				}
			}
			for(cc = 0; cc < dz->wanted; cc++) {
				thisval  = log((dz->flbufptr[0][cc] * normaliser) + 1.0);
				thisval *= adjuster;

//				dz->sndbuf[outcnt] = (short)round(thisval);
				dz->sndbuf[outcnt] = (float)thisval;

				if(++outcnt >= (dz->infile->specenvcnt + SPACER) * FMNT_BUFMULT) {
					sprintf(errstr,"buffer accounting error in specfmntsee()\n");
					return(PROGRAM_ERROR);
				}
			}
			for(cc = 0; cc < SPACER; cc++)
//				dz->sndbuf[outcnt++] = (short)0; 
				dz->sndbuf[outcnt++] = 0.0f; 
			if(++outbufcnt >= FMNT_BUFMULT) {
				if(outcnt != (dz->infile->specenvcnt + SPACER) * FMNT_BUFMULT) {
					sprintf(errstr,"bufcount error in specfmntsee()\n");
					return(PROGRAM_ERROR);
				}
				if(outcnt > 0) {
					if((exit_status = write_samps(dz->sndbuf,outcnt,dz))<0)
						return(exit_status);
				}
				outcnt    = 0;
				outbufcnt = 0;
			}
			dz->flbufptr[0]  += dz->wanted;		/* move along bigfbuf */
			dz->time = (float)(dz->time + dz->frametime);
		}
		if((exit_status = read_samps(dz->bigfbuf,dz)) < 0) {
			sprintf(errstr,"Sound read error.\n");
			return(SYSTEM_ERROR);
		}
		dz->flbufptr[0] = dz->bigfbuf;
		windows_in_buf = dz->ssampsread/dz->wanted;    
	} while(windows_in_buf > 0);
	if(outcnt) {
		if((exit_status = write_samps(dz->sndbuf,outcnt,dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}

/*********************** RESET_FILE_TO_START_AND_POINTERS_TO_AFTER_DESCRIPTOR_BLOCKS **********************/

int reset_file_to_start_and_pointers_to_after_descriptor_blocks(int *windows_in_buf,dataptr dz)
{
	int exit_status;
	if(sndseekEx(dz->ifd[0],0L,0)<0) { /* RESET FILE POINTER TO START OF DATA */
		sprintf(errstr,"seek failed in reset_file_to_start_and_pointers_to_after_descriptor_blocks()\n");
		return(SYSTEM_ERROR);
	}
	if((exit_status = read_samps(dz->bigfbuf,dz)) < 0) {
		sprintf(errstr,"Read problem after rewind in reset_file_to_start_and_pointers_to_after_descriptor_blocks()\n");
		return(SYSTEM_ERROR);
	}
	*windows_in_buf = dz->ssampsread/dz->wanted;    
	if((*windows_in_buf -= DESCRIPTOR_DATA_BLOKS) < 0) {
		sprintf(errstr,"Buffers too SMALL in reset_file_to_start_and_pointers_to_after_descriptor_blocks()\n");
		return(PROGRAM_ERROR);
	}
	dz->flbufptr[0] = dz->bigfbuf + (DESCRIPTOR_DATA_BLOKS * dz->wanted);
	return(FINISHED);
}

/**************************** OUTER_FORMSEE_LOOP ****************************/

int outer_formsee_loop(dataptr dz)
{
	int exit_status;
	int windows_in_buf, peakscore = 0, pitchcnt = 0;
	int in_start_portion = TRUE, least = 0, descnt = 0;

	if(sloom)
		dz->total_samps_read = 0L;
   if((exit_status = read_samps(dz->bigfbuf,dz)) < 0)
	   return(exit_status);

   while(dz->ssampsread > 0) {
    	dz->flbufptr[0] = dz->bigfbuf;
    	windows_in_buf = dz->ssampsread/dz->wanted;
		if((exit_status = inner_loop(&peakscore,&descnt,&in_start_portion,&least,&pitchcnt,windows_in_buf,dz))<0)
			return(exit_status);
		if((exit_status = write_samps(dz->bigfbuf,dz->ssampsread,dz))<0)
			return(exit_status);
	   if((exit_status = read_samps(dz->bigfbuf,dz)) < 0)
		   return(exit_status);
	}
	if(dz->ssampsread < 0) {
		sprintf(errstr,"Sound read error.\n");
		return(SYSTEM_ERROR);
	}  
    return renormalise_formsee(dz);
}


/******************************* RENORMALISE_FORMSEE ****************************
 *
 * Multiply by 9999/formantmax ->  range 0 TO 9999.
 * Add 1                        ->  range 1 TO 10000.
 * Take log :			->  range 0 TO log(10000).(log display)
 * Times MAXSAMPP/log(10000) :	->  range 0 TO MAXSAMPP	(log log display)
 */

int renormalise_formsee(dataptr dz)
{
	int exit_status;
	int	 cc, vc;
	int wc, outcnt, outbufcnt, total_samps_read;
	int ssampsread, total_samps_todo = dz->total_samps_written;
	int windows_in_buf;
	int windows_remaining = dz->wlength;
	float thissamp;
	double normaliser;
	double adjuster   = F_MAXSAMP/log(10000.0);
	int chans, stype;
	int srate;
	if(dz->param[FSEE_FMAX] < VERY_TINY_VAL) {
		sprintf(errstr,"No siginificant data found in srcfile\n");
		return(DATA_ERROR);
	}
//TW NEW MECHANISM writes renormalised vals from original file created (tempfile) into true outfile
// true outfile gets the name the user originally input
	if(sndcloseEx(dz->ifd[0]) < 0) {
		sprintf(errstr, "WARNING: Can't close input soundfile\n");
		return(SYSTEM_ERROR);
	}
	if(sndcloseEx(dz->ofd) < 0) {
		sprintf(errstr, "WARNING: Can't close output soundfile\n");
		return(SYSTEM_ERROR);
	}
	if((dz->ifd[0] = sndopenEx(dz->outfilename,0,CDP_OPEN_RDWR)) < 0) {   /* RWD Nov 2003 was RDONLY */
		sprintf(errstr,"Failure to reopen file %s for renormalisation.\n",dz->outfilename);
		return(SYSTEM_ERROR);
	}
    /* RWD Jumy 2010 no lopmger need this - now built into sndopenEx in sfsys */
	//sndseekEx(dz->ifd[0],0,0);
	chans = dz->infile->channels;
	srate = dz->infile->srate;
	stype = dz->infile->stype;
	dz->infile->channels = 1;
	dz->infile->srate    = dz->infile->origrate;
	dz->infile->stype	 = SAMP_SHORT;
	if((exit_status = create_sized_outfile(dz->wordstor[0],dz))<0) {
		sprintf(errstr,"Failure to create file %s for renormalisation.\n",dz->wordstor[0]);
		return(exit_status);
	}
	dz->infile->channels = chans;
	dz->infile->srate    = srate;
	dz->infile->stype = stype;
	if((exit_status = reset_peak_finder(dz))<0)
		return(exit_status);

	normaliser = 9999.0/dz->param[FSEE_FMAX];
	total_samps_todo = dz->total_samps_written;
	outbufcnt				 = 0L;
	outcnt                   = 0L;
	total_samps_read         = 0L;
	dz->total_samps_written  = 0L;
	dz->total_windows		 = 0L;
	dz->total_samps_read     = 0L;
	while(total_samps_todo > 0) {
		if((exit_status = read_samps(dz->bigfbuf,dz)) < 0) {
			sprintf(errstr,"Sound read error.\n");
			close_and_delete_tempfile(dz->outfilename,dz);
			return(SYSTEM_ERROR);
		}  
		dz->flbufptr[0] = dz->bigfbuf;
		total_samps_read += dz->ssampsread;
		ssampsread  = dz->ssampsread;
		if(total_samps_todo - ssampsread<0) {
			ssampsread = total_samps_todo;
			total_samps_todo = 0;
		} else
			total_samps_todo -= ssampsread;
		windows_in_buf = ssampsread/dz->wanted;
		if(windows_remaining - windows_in_buf < 0) {
			windows_in_buf = windows_remaining;
			windows_remaining = 0;
		} else
			windows_remaining -= windows_in_buf;
		for(wc=0;wc<windows_in_buf;wc++) {
			if(dz->total_windows==0) {
				for(cc=0;cc<dz->clength;cc++)
					dz->sndbuf[outcnt++] = 0.0f;
			} else {
				for(cc=0,vc=0;cc<dz->iparam[FSEE_FCNT];cc++,vc += 2)  {		/* 1 */
					if((exit_status = setformval(&thissamp,dz->flbufptr[0][AMPP],normaliser,adjuster))<0)
						return(exit_status);
					dz->sndbuf[outcnt++] = thissamp;
				}
				for(;cc<dz->clength;cc++)
					dz->sndbuf[outcnt++] = 0.0f;
			}
			if(++outbufcnt >= FMNT_BUFMULT) {
				if(outcnt!=dz->clength * FMNT_BUFMULT) {
					sprintf(errstr,"Buffercnt error in renormalise_formsee()\n");
					close_and_delete_tempfile(dz->outfilename,dz);
					return(PROGRAM_ERROR);
				}
				if(outcnt > 0) {
					if((exit_status = write_samps(dz->sndbuf,outcnt,dz))<0)  {
						close_and_delete_tempfile(dz->outfilename,dz);
						return(exit_status);
					}
				}
				outcnt    = 0;
				outbufcnt = 0;
			}
			dz->flbufptr[0] += dz->wanted;
			dz->total_windows++;
		}
		if(windows_remaining <=0)
			break;
	}
	if(outcnt) {
		if((exit_status = write_samps(dz->sndbuf,outcnt,dz))<0) {
			close_and_delete_tempfile(dz->outfilename,dz);
			return(exit_status);
		}
	}
	close_and_delete_tempfile(dz->outfilename,dz);
	return(FINISHED);
}

/****************************** SETFORMVAL ***************************/

//int setformval(int *thissamp,float inval,double normaliser,double adjuster)
int setformval(float *thissamp,float inval,double normaliser,double adjuster)
{
	double thisval = log((inval * normaliser) + 1.0);
//	thisval  *= adjuster;
//	*thissamp  = round(thisval);
	*thissamp  = (float)(thisval * adjuster);
	return(FINISHED);
}

/**************************** SPECFORMSEE ***************************/

int specformsee(dataptr dz)
{
	int exit_status;
	rectify_window(dz->flbufptr[0],dz);
	if((exit_status = extract_specenv(0,0,dz))<0)
		return(exit_status);
	return extract_formant_and_place_in_output_buffer(dz->iparam[FSEE_FCNT],dz);
}
	
/*********************** EXTRACT_FORMANT_AND_PLACE_IN_OUTPUT_BUFFER **********************/

int extract_formant_and_place_in_output_buffer(int formantlen,dataptr dz)
{
	int exit_status;
	int cc, vc;
	double thisamp;
	for(cc=0,vc=0;cc<formantlen;cc++,vc+=2) {
		if((exit_status = getspecenvamp(&thisamp,dz->windowbuf[0][cc],0,dz))<0)
			return(exit_status);
		dz->flbufptr[0][AMPP] = (float)thisamp;
		if(dz->flbufptr[0][AMPP] > dz->param[FSEE_FMAX])
			dz->param[FSEE_FMAX] = dz->flbufptr[0][AMPP];
	}
	for(;cc<dz->clength;cc++,vc+=2)
		dz->flbufptr[0][AMPP] = 0.0f;
	return(FINISHED);
}

/***************************** GET_MAX_FORMANTVAL *************************/

int get_max_formantval(dataptr dz)
{
	int exit_status;
	int   total_floats_in_file = (dz->wlength + DESCRIPTOR_DATA_BLOKS) * dz->infile->specenvcnt;
	int   floats_read = min(total_floats_in_file,dz->big_fsize);
	float *sbufend = dz->bigfbuf + floats_read;
	float *sbuf    = dz->flbufptr[0]; 
			/* which has already skipped the float descriptor blocks */
	dz->param[FMNTSEE_MAX] = 0.0;
	do {
		while(sbuf < sbufend) {
			if(*sbuf > dz->param[FMNTSEE_MAX])
				dz->param[FMNTSEE_MAX] = *sbuf;
			sbuf++;
		}
		if((exit_status = read_samps(dz->bigfbuf,dz)) < 0) {
			sprintf(errstr,"Sound read error.\n");
			return(SYSTEM_ERROR);
		}  
		sbuf      = dz->bigfbuf;
		sbufend   = dz->bigfbuf + dz->ssampsread;
	} while(dz->ssampsread > 0);
	return(FINISHED);
}

/***************************** INITIALISE_THE_FORMANT_DATA *************************/

int initialise_the_formant_data(int *windows_in_buf,dataptr dz)
{
	int exit_status;
	int fsamps_to_read = dz->descriptor_samps/DESCRIPTOR_DATA_BLOKS;
	if((exit_status = read_samps(dz->bigfbuf,dz)) < 0)
		return(exit_status);
	if(dz->ssampsread < dz->descriptor_samps) {
		sprintf(errstr,"No significant data in formantfile.\n");
		return(DATA_ERROR);
	}
	dz->flbufptr[0] = dz->bigfbuf;
	memmove((char *)dz->specenvpch,(char *)dz->flbufptr[0],(size_t)(fsamps_to_read * sizeof(float)));
	dz->flbufptr[0] += dz->infile->specenvcnt;
	memmove((char *)dz->specenvtop,(char *)dz->flbufptr[0],(size_t)(fsamps_to_read * sizeof(float)));
	dz->flbufptr[0] += dz->infile->specenvcnt;
	dz->flbufptr[0] = dz->bigfbuf + (DESCRIPTOR_DATA_BLOKS * dz->wanted);
	*windows_in_buf = dz->ssampsread/dz->wanted;    
	if((*windows_in_buf -= DESCRIPTOR_DATA_BLOKS) <= 0) {
		sprintf(errstr,"Buffers too SMALL for formant data.\n");
		return(PROGRAM_ERROR);
	}
	if(dz->vflag[FMNT_VIEW])
		print_formant_params_to_screen(dz);
	if((exit_status = get_max_formantval(dz))<0)
		return(exit_status);
	return reset_file_to_start_and_pointers_to_after_descriptor_blocks(windows_in_buf,dz);
}
