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
#include <pnames.h>
#include <tkglobals.h>
#include <globcon.h>
#include <modeno.h>
#include <arrays.h>
#include <morph.h>
#include <cdpmain.h>
#include <formants.h>
#include <speccon.h>
#include <sfsys.h>
#include <string.h>
#include <morph.h>

#define BEFORE 	(0)
#define	IN		(1)
#define	AFTER	(2)

#define NO_NORMALISE    (0)
#define NORMALISE_FILE1 (1)
#define NORMALISE_FILE2 (2)

static int  read_two_individual_input_windows(dataptr dz);
static int  establish_glide_ratios(dataptr dz);
static int  establish_temporary_file_for_offsetting(dataptr dz);
static int determine_process_startwindow(dataptr dz);
static int determine_process_endwindow(dataptr dz);
static int  read_bothfiles_for_bridge(int *windows_in_buf,dataptr dz);
static int  do_specbridge(dataptr dz);
static int  create_temporary_file_frontpadded_with_silence(dataptr dz);
static int  do_interpolation(int where,dataptr dz);
static int  set_tempfile_properties(dataptr dz);
static int  set_normalisation_type(int *normalisation_type,int where,double *normaliser, dataptr dz);
static int  read_specific_file_to_specific_buf(int fileno,int *windows_to_buf,
				float **fbufptr,float *floatbuf,dataptr dz);
static int  do_specmorph(double alen, double flen, dataptr dz);
static void remove_temp_file(dataptr dz);
static int  warped_cosin(double *interpratio,double exponent,dataptr dz);
static int  time_warp(double *interpratio,double exponent);
static int  cosin_lookup(double *interpratio,dataptr dz);

/*********************** SPECGLIDE ***************************/

int specglide(dataptr dz)
{
	int exit_status;
	int wc;
	int cc, vc;
	double interp_fact;
	if((exit_status = read_two_individual_input_windows(dz))<0)
		return(exit_status);
	if((exit_status = establish_glide_ratios(dz))<0)
		return(exit_status);
	dz->flbufptr[0] = dz->bigfbuf;
	dz->flbufptr[1] = dz->bigfbuf + dz->big_fsize; /* bufend */
	for(wc = 0; wc < dz->wlength; wc++) {
		interp_fact = (double)wc/(double)(dz->wlength-1);
		for(cc = 0, vc = 0; cc < dz->clength; cc++, vc += 2) {
			if(dz->iparray[GLIDE_ZERO][cc]) {
				dz->flbufptr[0][AMPP] = dz->windowbuf[1][AMPP];
				dz->flbufptr[0][FREQ] = dz->windowbuf[1][FREQ];
			} else {
				dz->flbufptr[0][AMPP] = (float)(dz->windowbuf[0][AMPP] + (interp_fact * dz->windowbuf[1][AMPP]));
				dz->flbufptr[0][FREQ] = (float)(dz->windowbuf[0][FREQ] * pow(2.0,(dz->parray[GLIDE_INF][cc] * interp_fact)));
			}
		}
		if((dz->flbufptr[0] += dz->wanted) >= dz->flbufptr[1])	 {	/* move along bigfbuf */
			if((exit_status = write_exact_samps(dz->bigfbuf,dz->big_fsize,dz))<0)
				return(exit_status);
			dz->flbufptr[0] = dz->bigfbuf;
		}
	}
	if(dz->flbufptr[0] != dz->bigfbuf) {
		if((exit_status = write_samps(dz->bigfbuf,dz->flbufptr[0] - dz->bigfbuf,dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}

/****************************** READ_TWO_INDIVIDUAL_INPUT_WINDOWS ***************************/

int read_two_individual_input_windows(dataptr dz)
{
	int samps_read;
	if((samps_read = fgetfbufEx(dz->bigfbuf,dz->wanted,dz->ifd[0],0)) < dz->wanted) {
		if(samps_read < 0) {
			sprintf(errstr,"Sound read error.\n");
			return(SYSTEM_ERROR);
		} else {
			sprintf(errstr,"Failed to read a window from 1st input file.\n");
			return(DATA_ERROR);
		}
	}
	memmove((char *)dz->windowbuf[0],(char *)dz->bigfbuf,(size_t)(dz->wanted * sizeof(float)));

	if((samps_read = fgetfbufEx(dz->bigfbuf,dz->wanted,dz->ifd[1],0)) < dz->wanted) {
		if(samps_read < 0) {
			sprintf(errstr,"Sound read error.\n");
			return(SYSTEM_ERROR);
		} else {
			sprintf(errstr,"Failed to read a window from 2nd input file.\n");
			return(DATA_ERROR);
		}
	}
	memmove((char *)dz->windowbuf[1],(char *)dz->bigfbuf,(size_t)(dz->wanted * sizeof(float)));
	return(FINISHED);
}

/****************************** ESTABLISH_GLIDE_RATIOS ***************************/

int establish_glide_ratios(dataptr dz)
{
 	int cc, vc;
	for(cc = 0, vc = 0; cc < dz->clength; cc++, vc += 2) {
		dz->iparray[GLIDE_ZERO][cc] = 0;
		if(flteq((double)dz->windowbuf[0][FREQ],0.0) || flteq((double)dz->windowbuf[1][FREQ],0.0)) {
			dz->iparray[GLIDE_ZERO][cc] = 1;	/* mark any zeros in frq */
			dz->windowbuf[1][AMPP] = 0.0f;
			dz->windowbuf[1][FREQ] = (float)((double)cc * dz->chwidth);
		} else {
			dz->windowbuf[1][AMPP] = (float)(dz->windowbuf[1][AMPP] - dz->windowbuf[0][AMPP]);
			if(dz->windowbuf[0][FREQ] < 0.0)
				dz->windowbuf[0][FREQ] = -dz->windowbuf[0][FREQ]; /*invert any -ve frqs */
			if(dz->windowbuf[1][FREQ]<0.0)
				dz->windowbuf[1][FREQ] = -dz->windowbuf[1][FREQ];
			dz->parray[GLIDE_INF][cc] = log(dz->windowbuf[1][FREQ]/dz->windowbuf[0][FREQ])/LOG10_OF_2;
		}
	}
	return(FINISHED);
}

/**************************** SPECBRIDGE ***************************
 *
 *	COMMENT FOR RWD: I know this is irrational!!
 *
 *  MOVING POINTERS
 *			flbufptr[0]						flbufptr[1]						flbufptr[5]
 *				|								|								|
 *	|			V				   |			V				  |				V
 *	|______________________________|______________________________|______________________________
 *	|	 input buffer for file0	   |	 input buffer for file1   |	       output buffer		 |
 *	|_____________ifd[0]___________|_(ifd[1] becoming) otherfile__|______________________________|
 *	
 *	/\	  						   /\							  /\							 /\
 *	bigfbuf					   flbufptr[2]					   flbufptr[3]  					 flbufptr[4]
 * FIXED POINTERS
 *
 *		I've tidied it up in 'distort' package, so fixed pointers are called windowbuf[]
 *											  and moving pointers are called flbufptr[]
 */

int specbridge(dataptr dz)
{
	int exit_status;
	int windows_in_buf, startwindow, endwindow, remain, wc;
	dz->flbufptr[0] = dz->bigfbuf;   	 /* moving pointer */
	dz->flbufptr[1] = dz->flbufptr[2]; /* moving pointer */
	dz->flbufptr[5] = dz->flbufptr[3]; /* moving pointer */
//TW we only needed dz->outfilesize for truncating the final outfile: which no longer happens
	if((exit_status = establish_temporary_file_for_offsetting(dz))<0)
		return(exit_status);
	startwindow = determine_process_startwindow(dz);
	endwindow   = determine_process_endwindow(dz);
	dz->total_windows = 0;

	if(startwindow >= endwindow) {
		sprintf(errstr,"error in window arithmetic:	specbridge()\n");
		return(PROGRAM_ERROR);
	}
  	while(dz->total_windows < endwindow) {
		if((exit_status = read_bothfiles_for_bridge(&windows_in_buf,dz))<0)
			return(exit_status);
		if(windows_in_buf <=0)
			break;
 		for(wc=0; wc<windows_in_buf; wc++) {
			if(dz->total_windows >= startwindow) {
				if((exit_status = do_specbridge(dz))<0)
					return(exit_status);
			}
			if((exit_status = advance_one_2fileinput_window(dz))<0)
				return(exit_status);
			if(dz->total_windows >= endwindow)
				break;
		}
	}
	if((remain = dz->flbufptr[5]  - dz->flbufptr[3]) > 0) {
		if((exit_status = write_samps(dz->flbufptr[3],remain,dz))<0)
			return(exit_status);
	}

	remove_temp_file(dz);
	return(FINISHED);
}

/*************************** ESTABLISH_TEMPORARY_FILE_FOR_OFFSETTING ***********************/

int establish_temporary_file_for_offsetting(dataptr dz)
{
	int exit_status;
	dz->flbufptr[5] = dz->flbufptr[3];
 	if((dz->insams = (int  *)realloc((char *)(dz->insams),(dz->infilecnt+1) * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to enlarge insams array.\n");
		return(MEMORY_ERROR);
	}
 	if(dz->param[BRG_OFFSET] > 0.0) {
 		if((exit_status = create_temporary_file_frontpadded_with_silence(dz))<0)
			return(exit_status);
  	} else {	/* point to original file */
 		dz->other_file  =  dz->ifd[1];
		dz->insams[2]	=  dz->insams[1];
	}	
 	return(FINISHED);
}

/*************************** DETERMINE_PROCESS_STARTWINDOW ***********************/

int determine_process_startwindow(dataptr dz)
{
	int startwindow;
	switch(dz->mode) {
	case(BRG_NO_NORMALISE):
	case(BRG_NORM_TO_FILE1):
	case(BRG_NORM_FROM_1_TO_2):
		if(dz->param[BRG_SF2] > 0.0 || dz->param[BRG_SA2] > 0.0)
 			startwindow = round(dz->param[BRG_OFFSET]/dz->frametime);	/* start_of_sound from infile2 */
		else
 			startwindow = 0;
		break;
	case(BRG_NORM_TO_MIN):  
	case(BRG_NORM_FROM_2_TO_1):
	case(BRG_NORM_TO_FILE2):
		startwindow = round(dz->param[BRG_OFFSET]/dz->frametime);	/* start_of_sound from infile2 */
		break;
	default:
		sprintf(errstr,"Unknown case in determine_process_startwindow()\n");
		return(PROGRAM_ERROR);
	}
	return(startwindow);
}

/*************************** DETERMINE_PROCESS_ENDWINDOW ***********************/

int determine_process_endwindow(dataptr dz)
{
#define	STOP_AT_END_OF_INFILE1  		(0)
#define	STOP_AT_END_OF_INFILE2			(1)
#define	STOP_AT_END_OF_SHORTEST_FILE	(2)
	int process_end;
	int endwindow = 0;
	switch(dz->mode) {
	case(BRG_NO_NORMALISE):
		if(dz->param[BRG_EF2] <= 0.0 && dz->param[BRG_EA2] <= 0.0)
 			process_end = STOP_AT_END_OF_INFILE1;
		else if(dz->param[BRG_EF2] >= 1.0 && dz->param[BRG_EA2] >= 1.0)
 			process_end = STOP_AT_END_OF_INFILE2;
		else
			process_end = STOP_AT_END_OF_SHORTEST_FILE;
		break;
	case(BRG_NORM_TO_MIN):  
		process_end = STOP_AT_END_OF_SHORTEST_FILE;
		break;
	case(BRG_NORM_TO_FILE1):
	case(BRG_NORM_FROM_2_TO_1):
		if(dz->param[BRG_EF2] > 0.0 || dz->param[BRG_EA2] > 0.0)
			process_end = STOP_AT_END_OF_SHORTEST_FILE;
		else
			process_end = STOP_AT_END_OF_INFILE1;
		break;
	case(BRG_NORM_TO_FILE2):
	case(BRG_NORM_FROM_1_TO_2):
		if(dz->param[BRG_EF2] < 1.0 || dz->param[BRG_EA2] < 1.0)
			process_end = STOP_AT_END_OF_SHORTEST_FILE;
		else
			process_end = STOP_AT_END_OF_INFILE2;
		break;
	default:
		sprintf(errstr,"Unknown case in determine_process_endwindow()\n");
		return(PROGRAM_ERROR);
	}
	switch(process_end) {
	case(STOP_AT_END_OF_INFILE1):
		endwindow = dz->insams[0]/dz->wanted;
		break;
	case(STOP_AT_END_OF_INFILE2):
		endwindow = dz->insams[2]/dz->wanted;
		break;
	case(STOP_AT_END_OF_SHORTEST_FILE):
		endwindow = min(dz->insams[2]/dz->wanted,dz->insams[0]/dz->wanted);
		break;
	}
	return(endwindow);
}

/***************************** READ_BOTHFILES_FOR_BRIDGE ***********************/

int read_bothfiles_for_bridge(int *windows_in_buf,dataptr dz)
{
	int samps_read;
	int samps_read1, samps_read2;
	if((samps_read1 = fgetfbufEx(dz->bigfbuf,dz->big_fsize,dz->ifd[0],0))<0
	|| (samps_read2 = fgetfbufEx(dz->flbufptr[2],dz->big_fsize,dz->other_file,0))<0) {
		sprintf(errstr,"Sound read error.\n");
		return(SYSTEM_ERROR);
	}
	samps_read = max(samps_read1,samps_read2);
	*windows_in_buf = samps_read/dz->wanted;
	dz->flbufptr[0] = dz->bigfbuf;
	dz->flbufptr[1] = dz->flbufptr[2];
	return(FINISHED);
}

/***************************** DO_SPECBRIDGE *********************************/

int do_specbridge(dataptr dz)
{
	int exit_status;
	int where;

	if(dz->total_windows < dz->iparam[BRG_SWIN])		where = BEFORE;
	else if(dz->total_windows < dz->iparam[BRG_INTPEND])	where = IN;
	else		            							 where = AFTER;
	switch(where) {
	case(BEFORE):	/***  BEFORE START OF INTERPOLATION ***/
		switch(dz->iparam[BRG_STARTIS]) {
		case(1):
			memmove((char *)dz->flbufptr[5],(char *)dz->flbufptr[0],(size_t)(dz->wanted * sizeof(float)));
			break;	/* KEEP FILE1 DATA */
		case(2): 
			memmove((char *)dz->flbufptr[5],(char *)dz->flbufptr[1],(size_t)(dz->wanted * sizeof(float)));	
			break;	/* KEEP FILE2 DATA */
		case(3): 
			if((exit_status = do_interpolation(BEFORE,dz))<0)
				return(exit_status);
			break;	/* INTERP DATA */
		case(0):
			sprintf(errstr,"BRG_STARTIS = 0: Should be impossible!!! do_specbridge()\n");
			return(PROGRAM_ERROR);
		}
		break;
	case(IN):		/********  INTERPOLATION PROPER ********/
		dz->param[BRG_SF2] += dz->param[BRG_FSTEP];
		dz->param[BRG_SA2] += dz->param[BRG_ASTEP];
		if((exit_status = do_interpolation(IN,dz))<0)
			return(exit_status);
		break;
	case(AFTER):	/***  TAIL ***/
		switch(dz->iparam[BRG_TAILIS]) {
		case(1):																		
			memmove((char *)dz->flbufptr[5],(char *)dz->flbufptr[0],(size_t)(dz->wanted * sizeof(float)));
			break;	 /* KEEP FILE1 DATA */
		case(2): 
			memmove((char *)dz->flbufptr[5],(char *)dz->flbufptr[1],(size_t)(dz->wanted * sizeof(float))); 
			break;	 /* KEEP FILE2 DATA */
		case(3): 
			if((exit_status = do_interpolation(AFTER,dz))<0)
				return(exit_status);
			break;	 /* KEEP INTERPOLATED DATA */
		case(0):
			sprintf(errstr,"tailis = 0: Should be impossible!!! do_specbridge()\n");
			return(PROGRAM_ERROR);
		}
		break;
	}
	if((dz->flbufptr[5] += dz->wanted) >= dz->flbufptr[4]) {
		if((exit_status = write_exact_samps(dz->flbufptr[3],dz->big_fsize,dz))<0)
			return(exit_status);
		dz->flbufptr[5] = dz->flbufptr[3];
	}
	return(FINISHED);
}

/*************************** CREATE_TEMPORARY_FILE_FRONTPADDED_WITH_SILENCE ***********************/

int create_temporary_file_frontpadded_with_silence(dataptr dz)
{
	int exit_status;
	int offset_windows,bufwindows, samps_read, windows_read, remain;
// 	if((dz->other_file = sndcreat("temp",-1,SAMP_FLOAT)) < 0) {
//		sprintf(errstr,"Can't create intermediate soundfile 'temp.wav'\n");
//		return(GOAL_FAILED);
//	}
	if((dz->other_file = sndcreat_formatted("temp",-1,SAMP_FLOAT,
			dz->infile->channels,dz->infile->srate,CDP_CREATE_NORMAL)) < 0) {
		sprintf(errstr,"Can't create intermediate soundfile 'temp.wav'\n");
		return(GOAL_FAILED);
	}
	if((exit_status = set_tempfile_properties(dz))<0)
		return(exit_status);
	offset_windows = round(dz->param[BRG_OFFSET]/dz->frametime);
	bufwindows     = dz->big_fsize/dz->wanted;
	while(offset_windows >= bufwindows) {
		memset((char *)dz->flbufptr[3],0,(size_t)(dz->big_fsize * sizeof(float)));
		if((exit_status = 
		write_samps_to_elsewhere(dz->other_file,dz->flbufptr[3],dz->big_fsize,dz))<0)
			return(exit_status);
		offset_windows -= bufwindows;
	}
	if(offset_windows) {
		memset((char *)dz->flbufptr[3],0,(size_t)(offset_windows * dz->wanted * sizeof(float)));
		dz->flbufptr[5] = dz->flbufptr[3] + offset_windows * dz->wanted;
	}
	do {
		if((samps_read = fgetfbufEx(dz->bigfbuf,dz->big_fsize,dz->ifd[1],0)) < 0) {
			sprintf(errstr,"soundfile read failed.\n");
			return(SYSTEM_ERROR);
		}
		dz->flbufptr[0] = dz->bigfbuf;
		if(samps_read > 0) {
			windows_read = samps_read/dz->wanted;
			while(windows_read) {
				if(dz->flbufptr[0] >= dz->flbufptr[2]) {	
					sprintf(errstr,"Error in buffer arithmetic: create_temporary_file_frontpadded_with_silence()\n");
					return(PROGRAM_ERROR);
				}
				memmove((char *)dz->flbufptr[5],(char *)dz->flbufptr[0],(size_t)(dz->wanted * sizeof(float)));
				if((dz->flbufptr[5] += dz->wanted) >= dz->flbufptr[4]) {
					if((exit_status = 
					write_samps_to_elsewhere(dz->other_file,dz->flbufptr[3],dz->big_fsize,dz))<0)
						return(exit_status);
					dz->flbufptr[5] = dz->flbufptr[3];
				}
				dz->flbufptr[0] += dz->wanted;
			 	windows_read--;
			}
		}
	}while(samps_read > 0);
	if((remain = dz->flbufptr[5] - dz->flbufptr[3]) > 0) {
		if((exit_status = write_samps_to_elsewhere(dz->other_file,dz->flbufptr[3],remain,dz))<0)
			return(exit_status);
	}
	dz->insams[2] = dz->total_samps_written;
	dz->total_samps_written = 0;
	dz->flbufptr[0] = dz->bigfbuf;
	dz->flbufptr[5] = dz->flbufptr[3];
	if((sndseekEx(dz->other_file,0L,0))<0) {
		sprintf(errstr,"seek to 0 failed, in create_temporary_file_frontpadded_with_silence()\n");
		return(SYSTEM_ERROR);
	}
	return(FINISHED);
}

/********************* DO_INTERPOLATION **************************/

int do_interpolation(int where,dataptr dz)
{
	int exit_status;
	int n = 0, normalisation_type = NO_NORMALISE;
	double normaliser = 0.0;

	if(dz->mode!= BRG_NO_NORMALISE)	   {		/* RWD need these! */
		if((exit_status = set_normalisation_type(&normalisation_type,where,&normaliser,dz))<0)
			return(exit_status);
	}//RWD
	switch(normalisation_type) {
	case(NO_NORMALISE):
		while(n < dz->wanted) {
			dz->flbufptr[5][n] = (float)((dz->flbufptr[1][n]*dz->param[BRG_SA2]) + (dz->flbufptr[0][n]*(1.0 - dz->param[BRG_SA2])));
			n++;
		}
		break;
	case(NORMALISE_FILE1):
		while(n < dz->wanted) {
			dz->flbufptr[5][n] = (float)((dz->flbufptr[1][n]*dz->param[BRG_SA2]) + (dz->flbufptr[0][n]*(1.0 - dz->param[BRG_SA2])*normaliser));
			n++;
			dz->flbufptr[5][n] = (float)((dz->flbufptr[1][n]*dz->param[BRG_SF2]) + (dz->flbufptr[0][n]*(1.0 - dz->param[BRG_SF2])));
			n++;
		}
		break;
	case(NORMALISE_FILE2):
		while(n < dz->wanted) {
			dz->flbufptr[5][n] = (float)((dz->flbufptr[1][n]*dz->param[BRG_SA2]*normaliser) + (dz->flbufptr[0][n]*(1.0 - dz->param[BRG_SA2])));
			n++;
			dz->flbufptr[5][n] = (float)((dz->flbufptr[1][n]*dz->param[BRG_SF2]) + (dz->flbufptr[0][n]*(1.0 - dz->param[BRG_SF2])));
			n++;
		}
		break;
	default:
		sprintf(errstr,"unknown case in do_interpolation()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/***************************** SET_TEMPFILE_PROPERTIES ******************************/			
 
int set_tempfile_properties(dataptr dz)
{	
	if(sndputprop(dz->other_file,"original sampsize", (char *)&(dz->infile->origstype), sizeof(int)) < 0){
		sprintf(errstr,"Failure to write original sample size. set_tempfile_properties()\n");
		return(PROGRAM_ERROR);
	}
	if(sndputprop(dz->other_file,"original sample rate", (char *)&(dz->infile->origrate), sizeof(int)) < 0){
		sprintf(errstr,"Failure to write original sample rate. set_tempfile_properties()\n");
		return(PROGRAM_ERROR);
	}
	if(sndputprop(dz->other_file,"arate",(char *)&(dz->infile->arate),sizeof(float)) < 0){
		sprintf(errstr,"Failed to write analysis sample rate. set_tempfile_properties()\n");
		return(PROGRAM_ERROR);
	}
	if(sndputprop(dz->other_file,"analwinlen",(char *)&(dz->infile->Mlen),sizeof(int)) < 0){
		sprintf(errstr,"Failure to write analysis window length. set_tempfile_properties()\n");
		return(PROGRAM_ERROR);
	}
	if(sndputprop(dz->other_file,"decfactor",(char *)&(dz->infile->Dfac),sizeof(int)) < 0){
		sprintf(errstr,"Failure to write decimation factor. set_tempfile_properties()\n");
		return(PROGRAM_ERROR);
	}
	if(sndputprop(dz->other_file,"sample rate", (char *)&(dz->infile->srate), sizeof(int)) < 0){
		sprintf(errstr,"Failure to write sample rate. set_tempfile_properties()\n");
		return(PROGRAM_ERROR);
	}
	if(sndputprop(dz->other_file,"channels", (char *)&(dz->infile->channels), sizeof(int)) < 0){
		sprintf(errstr,"Failure to write channel data. set_tempfile_properties()\n");
		return(PROGRAM_ERROR);
	}
//TW can't change sample type after file opened
//	if(sndputprop(dz->other_file,"sample type", (char *)&(dz->infile->stype), sizeof(int)) < 0){
//		sprintf(errstr,"Failure to write sample size. set_tempfile_properties()\n");
//		return(PROGRAM_ERROR);
//	}
	return(FINISHED);
}

/********************* SET_NORMALISATION_TYPE **************************/

int set_normalisation_type(int *normalisation_type,int where,double *normaliser, dataptr dz)
{
	int vc;
	double z1, z2;
	double sum1 = 0.0;
	double sum2 = 0.0;
	for(vc = 0;vc < dz->wanted; vc+=2) {
		sum1 += dz->flbufptr[0][AMPP];
		sum2 += dz->flbufptr[1][AMPP];
	}
	if(sum1 < 0.0 || sum2 < 0.0) {
		sprintf(errstr,"Amplitude anomaly in data: cannot proceed.\n");
		return(DATA_ERROR);
	}
	switch(dz->mode) {
	case(BRG_NORM_TO_MIN):
		if(sum1>sum2) {
			*normalisation_type = NORMALISE_FILE1;		
			if(flteq(sum1,0.0))
				*normaliser = 0.0;
			else 
			*normaliser = sum2/sum1;
		} else {
			*normalisation_type = NORMALISE_FILE2;		
			if(flteq(sum2,0.0))
				*normaliser = 0.0;
			else 
				*normaliser = sum1/sum2;
		}
		break;
	case(BRG_NORM_TO_FILE1):
		*normalisation_type = NORMALISE_FILE2;			
		if(flteq(sum2,0.0))
			*normaliser = 0.0;
		else 
			*normaliser = sum1/sum2;
		break;
	case(BRG_NORM_TO_FILE2):
		*normalisation_type = NORMALISE_FILE1;			
		if(flteq(sum1,0.0))
			*normaliser = 0.0;
		else 
			*normaliser = sum2/sum1;
		break;
	case(BRG_NORM_FROM_1_TO_2):
		switch(where) {
		case(BEFORE):
			*normalisation_type = NORMALISE_FILE2;		
			if(flteq(sum2,0.0))
				*normaliser = 0.0;
			else 
				*normaliser = sum1/sum2;
			break;
		case(IN):
			if(flteq(sum2,0.0))
				*normaliser = 0.0;
			else if(flteq(sum1,0.0))
				*normaliser = 0.0; 
			else {
				z1 = sum1/sum2;
				z2 = sum2/sum1;
				*normaliser = z1 * pow(z2,dz->param[BRG_TFAC]);
			}
			if(dz->param[BRG_TFAC]<=1.0)
				*normalisation_type = NORMALISE_FILE2;
			else
				*normalisation_type = NORMALISE_FILE1;
			dz->param[BRG_TFAC] += dz->param[BRG_TSTEP];
			break;
		case(AFTER):
			*normalisation_type = NORMALISE_FILE1;
			if(flteq(sum1,0.0))
				*normaliser = 0.0; 
			else 
				*normaliser = sum2/sum1;
			break;
		default:
			sprintf(errstr,"Unknown case (A) in set_normalisation_type()\n");
			return(PROGRAM_ERROR);
		}
		break;
	case(BRG_NORM_FROM_2_TO_1):
		switch(where) {
		case(BEFORE):
			*normalisation_type = NORMALISE_FILE1;
			if(flteq(sum1,0.0))
				*normaliser = 0.0;
			else 
				*normaliser = sum2/sum1;
			break;
		case(IN):
			if(flteq(sum1,0.0))
				*normaliser = 0.0;
			else if(flteq(sum2,0.0))
				*normaliser = 0.0; 
			else {
				z1 = sum2/sum1;
				z2 = sum1/sum2;
				*normaliser = z1 * pow(z2,dz->param[BRG_TFAC]);
			}
			if(dz->param[BRG_TFAC]<=1.0)
				*normalisation_type = NORMALISE_FILE1;
			else
				*normalisation_type = NORMALISE_FILE2;
			dz->param[BRG_TFAC] += dz->param[BRG_TSTEP];
			break;
		case(AFTER):
			*normalisation_type = NORMALISE_FILE2;
			if(flteq(sum2,0.0))
				*normaliser = 0.0;
			else 
				*normaliser = sum1/sum2;
			break;
		default:
			sprintf(errstr,"Unknown case (B) in set_normalisation_type()\n");
			return(PROGRAM_ERROR);
		}
		break;
	default:
		sprintf(errstr,"Unknown case (C) in set_normalisation_type()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/***************************** SPECMORPH ***********************/

int specmorph(dataptr dz)
{
#define	FILE0 (0)
#define FILE1 (1)

	int		exit_status;
	int	windows_to_buf0 = 0, windows_to_buf1 = 0, 
			bufwsize		= dz->big_fsize/dz->wanted;
	int	start_of_morph	= min(dz->iparam[MPH_ASTTW],dz->iparam[MPH_FSTTW]);
	int	end_of_morph	= max(dz->iparam[MPH_AENDW],dz->iparam[MPH_FENDW]);
	double	flen			= (double)(dz->iparam[MPH_FENDW] - dz->iparam[MPH_FSTTW]);
	double	alen			= (double)(dz->iparam[MPH_AENDW] - dz->iparam[MPH_ASTTW]);
	int	window_position_in_current_buf_for_file0 = 0,
			window_position_in_outfile = 0, 
			window_position_in_current_buf_for_file1 = -dz->iparam[MPH_STAGW];
	int	total_windows_processed_from_file0 = 0, 
			total_windows_processed_from_file1 = 0;
	int		finished_file0 = 0, 
			finished_file1 = 0;
	int	total_windows_in_file1 = dz->insams[1]/dz->wanted;
	
	do {
		if(!finished_file0) {
			if(total_windows_processed_from_file0 >= end_of_morph)
				finished_file0 = 1;
			else {
				if(window_position_in_current_buf_for_file0 == windows_to_buf0) {
					if((exit_status = read_specific_file_to_specific_buf
					(FILE0,&windows_to_buf0,&dz->flbufptr[0],dz->bigfbuf,dz))<0)
						return(exit_status);
					window_position_in_current_buf_for_file0 = 0;
				}
			}
		}
		if(window_position_in_current_buf_for_file1>=0) {
			if(window_position_in_current_buf_for_file1 >= windows_to_buf1) {
				if((exit_status = read_specific_file_to_specific_buf
				(FILE1,&windows_to_buf1,&dz->flbufptr[1],dz->flbufptr[2],dz))<0)
					return(exit_status);
				window_position_in_current_buf_for_file1 = 0;
			}
			if(total_windows_processed_from_file0 >= start_of_morph) {
				if(!finished_file0) {
					if((exit_status = do_specmorph(alen,flen,dz))<0)
						return(exit_status);
				} else
					memmove((char *)dz->flbufptr[0],(char *)dz->flbufptr[1],(size_t)(dz->wanted * sizeof(float)));	 
			}
			dz->flbufptr[1] += dz->wanted;
			if(++total_windows_processed_from_file1 >= total_windows_in_file1)
				finished_file1 = 1;
		}
		dz->flbufptr[0] += dz->wanted;
		if(++window_position_in_outfile >= bufwsize) {
			if((exit_status = write_exact_samps(dz->bigfbuf,dz->big_fsize,dz))<0)
				return(exit_status);
			window_position_in_outfile = 0;
			dz->flbufptr[0] = dz->bigfbuf;
		}
		if(!finished_file0) {
			total_windows_processed_from_file0++;
			window_position_in_current_buf_for_file0++;
		}
		window_position_in_current_buf_for_file1++;
		dz->total_windows++;
	} while(!finished_file1);

	if((window_position_in_outfile > 0) && (window_position_in_current_buf_for_file0 > 0)) {
		if((exit_status = write_samps(dz->bigfbuf,window_position_in_current_buf_for_file0 * dz->wanted,dz))<0)
		
			return(exit_status);
	}
	return(FINISHED);
}

/***************************** READ_SPECIFIC_FILE_TO_SPECIFIC_BUF ***********************/

int read_specific_file_to_specific_buf(int fileno,int *windows_to_buf,float **fbufptr,float *floatbuf,dataptr dz)
{
	int samps_read;
	if((samps_read = fgetfbufEx(floatbuf,dz->big_fsize,dz->ifd[fileno],0))<=0) {
		if(samps_read<0) {
			sprintf(errstr,"Sound read error.\n");
			return(SYSTEM_ERROR);
		} else {
			sprintf(errstr,"Error in buffer arithmetic in read_specific_file_to_specific_buf()\n");
			return(PROGRAM_ERROR);
		}
	}
	*windows_to_buf = samps_read/dz->wanted;
	*fbufptr  = floatbuf;
	return(FINISHED);
}

/*************************************** DO_SPECMORPH ******************************************/

int do_specmorph(double alen, double flen, dataptr dz)
{
	int exit_status;
	int vc;
	double amp_interpratio, frq_interpratio, diff, incr;

	if(dz->total_windows > dz->iparam[MPH_ASTTW]) {
		if(dz->total_windows < dz->iparam[MPH_AENDW]) {
			amp_interpratio = (double)(dz->total_windows - dz->iparam[MPH_ASTTW])/(double)alen;

			switch(dz->mode) {
			case(MPH_LINE):  
				amp_interpratio = pow(amp_interpratio,dz->param[MPH_AEXP]);				
				break;
			case(MPH_COSIN): 
				if((exit_status = warped_cosin(&amp_interpratio,dz->param[MPH_AEXP],dz))<0)
					return(exit_status);
				break;
			default:
				sprintf(errstr,"Unknown case in do_specmorph()\n");
				return(PROGRAM_ERROR);
			}
			for(vc=0;vc < dz->wanted; vc+=2) {				
				diff = dz->flbufptr[1][AMPP] - dz->flbufptr[0][AMPP];
				incr = diff * amp_interpratio;
				dz->flbufptr[0][AMPP] = (float)(dz->flbufptr[0][AMPP] + incr);
			}
		}
		else {
			for(vc=0;vc < dz->wanted; vc+=2)
				dz->flbufptr[0][AMPP] = dz->flbufptr[1][AMPP];
		}
	}
	if(dz->total_windows > dz->iparam[MPH_FSTTW]) {
		if(dz->total_windows < dz->iparam[MPH_FENDW]) {
			frq_interpratio = (double)(dz->total_windows - dz->iparam[MPH_FSTTW])/flen;

			switch(dz->mode) {
			case(MPH_LINE):  
				frq_interpratio = pow(frq_interpratio,dz->param[MPH_FEXP]);				
				break;
			case(MPH_COSIN): 
				if((exit_status = warped_cosin(&frq_interpratio,dz->param[MPH_FEXP],dz))<0)
					return(exit_status);
				break;
			default:
				sprintf(errstr,"Unknown case in do_specmorph()\n");
				return(PROGRAM_ERROR);
			}
			for(vc=0;vc < dz->wanted; vc+=2) {
				diff = dz->flbufptr[1][FREQ] - dz->flbufptr[0][FREQ];
				incr = diff * frq_interpratio;
				dz->flbufptr[0][FREQ] = (float)(dz->flbufptr[0][FREQ] + incr);
			}
		} else {
			for(vc=0;vc < dz->wanted; vc+=2)
				dz->flbufptr[0][FREQ] = dz->flbufptr[1][FREQ];
		}
	}
	return(FINISHED);
}	

/*************************** DETERMINE_PROCESS_STARTWINDOW ***********************/

void remove_temp_file(dataptr dz)
{
	if(dz->param[BRG_OFFSET] && dz->other_file >= 0) {
		if(sndunlink(dz->other_file) < 0) {
			fprintf(stdout, "WARNING: Can't set temporary sndfile 'temp' for deletion\n");
		}
		if(sndcloseEx(dz->other_file) < 0) {
			fprintf(stdout,"WARNING: Can't close temporary sndfile 'temp'\n");
		}
	}
	dz->other_file = -1;
}

/****************************** WARPED_COSIN ******************************/

int warped_cosin(double *interpratio,double exponent,dataptr dz)
{
	int exit_status;
	if(*interpratio < .5) {
		if((exit_status = time_warp(interpratio,exponent))<0)
		return(exit_status);
	} else if(*interpratio > .5) {
		*interpratio = 1.0 - *interpratio;
		if((exit_status = time_warp(interpratio,exponent))<0)
		return(exit_status);
		*interpratio = 1.0 - *interpratio;					
	}
	if((exit_status = cosin_lookup(interpratio,dz))<0)
		return(exit_status);
	return(FINISHED);
}

/****************************** TIME_WARP ******************************/

int time_warp(double *interpratio,double exponent)
{
	*interpratio = min(*interpratio * 2.0,1.0);
	*interpratio = pow(*interpratio,exponent);
	*interpratio /= 2.0;
	return(FINISHED);
}

/****************************** COSIN_LOOKUP ******************************/

int cosin_lookup(double *interpratio,dataptr dz)
{
	double dtabindex = *interpratio * (double)MPH_COSTABSIZE;
	int tabindex = (int)floor(dtabindex);	/* TRUNCATE */
	double frac  = dtabindex - (double)tabindex;
	double thisval = dz->parray[0][tabindex++]; /* table has wraparound pnt */
	double nextval = dz->parray[0][tabindex];
	double diff = nextval - thisval;
	double step = diff * frac;
	*interpratio = thisval + step;
	return(FINISHED);
}
