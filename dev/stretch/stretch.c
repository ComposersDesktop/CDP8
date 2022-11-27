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
#include <stretch.h>
#include <cdpmain.h>
#include <formants.h>
#include <speccon.h>
#include <sfsys.h>
#include <string.h>
#include <stretch.h>

static int  	do_constant_timestretch(int *thatwindow,dataptr dz);
static int  	do_timevariable_timestretch(int *thatwindow,dataptr dz);
static int  	do_timestretching(int *thatwindow,int count,dataptr dz);
static int  	do_timevariable_count(int *outcnt,dataptr dz);
static int  	divide_time_into_equal_increments(int *thatwindow,int startwindow,
					double dur,int count,dataptr dz);
static int  	get_both_vals_from_brktable(double *thistime,double *thisstretch,int brktab_no,dataptr dz);
static int  	timestretch_this_segment(int *thatwindow,int startwindow,
					double thiswdur,double nextwdur,int totaldur,dataptr dz);
static int  	advance_along_input_windows(int wdw_to_advance,int atend,dataptr dz);
static double  	calculate_number_of_output_windows(double startwdur,double endwdur,int totaldur);
static int  	calc_position_output_wdws_relative_to_input_wdws_for_decreasing_stretch
					(int *thatwindow,int startwindow,int count,int totaldur,
					double param0,double param1,double param2,dataptr dz);
static int  	calc_position_output_wdws_relative_to_input_wdws_for_increasing_stretch
					(int *thatwindow,int startwindow,int count,
					double param0,double param1,double param2,dataptr dz);
static double  	calculate_position(int x,double param0,double param1,double param2);
static int  	retrograde_sequence_of_time_intervals(int endtime,int count,double *startptr,dataptr dz);
static int 		stretch_spectrum(double shift,dataptr dz);

/********************************** SPECTSTRETCH **********************************
 *
 * Timestretch spectrum.
 */
 
int spectstretch(dataptr dz)
{
	int exit_status;
	int thatwindow, outcnt = 0;
	int samps_read;

	switch(dz->mode) {
	case(TSTR_NORMAL):
		if((samps_read = fgetfbufEx(dz->flbufptr[0], dz->big_fsize,dz->ifd[0],0)) < 0) {
			sprintf(errstr,"No data found in input soundfile.\n");
			return(DATA_ERROR);
		}
		if(sloom)
			dz->total_samps_read = samps_read;
		dz->flbufptr[3] = dz->flbufptr[0];
		dz->flbufptr[4] = dz->flbufptr[0] + dz->wanted;
		thatwindow = 0;
		if(dz->brksize[TSTR_STRETCH]==0)
			exit_status = do_constant_timestretch(&thatwindow,dz);
		else
			exit_status = do_timevariable_timestretch(&thatwindow,dz);
		if(exit_status<0)
			return(exit_status);
		if((exit_status = do_timestretching(&thatwindow,dz->ptr[TSTR_PBUF] - dz->parray[TSTR_PBUF],dz))<0)
			return(exit_status);
		if(dz->flbufptr[5] - dz->flbufptr[1] > 0)
			return write_samps(dz->flbufptr[1],dz->flbufptr[5] - dz->flbufptr[1],dz);
		return FINISHED;
	case(TSTR_LENGTH):
		if(dz->brksize[TSTR_STRETCH]==0)
			outcnt = round(fabs((double)dz->wlength * dz->param[TSTR_STRETCH]));
		else if((exit_status = do_timevariable_count(&outcnt,dz))<0)
			return(exit_status);
		fprintf(stdout,"INFO: Length of output file will be %.3lf secs.\n",outcnt * dz->frametime);
		fflush(stdout);
		break;
	default:
		sprintf(errstr,"Unknown mode in specstretch()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/******************************* DO_CONSTANT_TIMESTRETCH ****************************
 *
 * Set up array of indices for reading source window values into new
 * windows.
 *
 * (0)	Initialise position-buffer pointer and start window.
 * (1)	Get first breakpoint pair from brkpoint table.
 * (2)	Calculate initial time as whole number of WINDOWS.
 * (3)	If time is stretched, output window is SHRUNK relative to input
 *	window. Hence val = 1/stretch.
 * (4)	Proceed in a loop of all input breakpoint values.
 * (5)	Returns if premature end of file encountered.
 */

int do_constant_timestretch(int *thatwindow,dataptr dz)
{
	double thiswdur, dcount, dur;
	int count;
	int startwindow = 0;
	dz->ptr[TSTR_PBUF] = dz->parray[TSTR_PBUF];
	thiswdur  = 1.0/dz->param[TSTR_STRETCH];					/* 3 */
	dcount = (double)dz->wlength/thiswdur;
	count = round(fabs(dcount));
	dur = (double)dz->wlength/(double)count;
	return divide_time_into_equal_increments(thatwindow,startwindow,dur,count,dz);
}

/******************************* DO_TIMEVARIABLE_TIMESTRETCH ****************************
 *
 * Set up array of indices for reading source window values into new
 * windows.
 *
 * (0)	Initialise position-buffer pointer and start window.
 * (1)	Get first breakpoint pair from brkpoint table.
 * (2)	Calculate initial time as whole number of WINDOWS.
 * (3)	If time is stretched, output window is SHRUNK relative to input
 *		window. Hence val = 1/stretch.
 * (4)	Proceed in a loop of all input breakpoint values.
 * (5)	Returns if brkpnt times ran off end of source soundfile.
 */

int do_timevariable_timestretch(int *thatwindow,dataptr dz)
{
	int exit_status;
	int time0, time1, time2, totaldur;
	double stretch0, stretch1, thiswdur, nextwdur, valdiff, dtime0, dtime1;
	int atend = FALSE;
	int startwindow = 0;												  /* 0 */
	int timestretch_called = FALSE;
	dz->ptr[TSTR_PBUF] = dz->parray[TSTR_PBUF];										
	if((exit_status = get_both_vals_from_brktable(&dtime0,&stretch0,TSTR_STRETCH,dz))<0)	/* 1 */
		return(exit_status);
	if(exit_status == FINISHED) {
		sprintf(errstr,"Not enough data in brkfile & not trapped earlier: do_timevariable_timestretch()\n");
		return(PROGRAM_ERROR); 
	}
	time0 = round(dtime0/dz->frametime);								  /* 2 */
	thiswdur  = 1.0/stretch0;												  /* 3 */
	while((exit_status = get_both_vals_from_brktable(&dtime1,&stretch1,TSTR_STRETCH,dz))==CONTINUE) {/* 4 */
		nextwdur  = 1.0/stretch1;
		if((time1 = round(dtime1/dz->frametime)) > dz->wlength) {
			time2  = time1;												  /* 5 */
			time1 = round(dz->param[TSTR_TOTIME]/dz->frametime);
			valdiff  = nextwdur - thiswdur;
			valdiff *= (double)(time1 - time0)/(double)(time2 - time0);
			nextwdur = thiswdur + valdiff;
			atend = TRUE;
		}
		totaldur = time1 - time0;
		timestretch_called = TRUE;
		if((exit_status = timestretch_this_segment(thatwindow,startwindow,thiswdur,nextwdur,totaldur,dz))<0)
			return(exit_status);
		startwindow += totaldur;
		thiswdur  = nextwdur;
		time0 = time1;
		if(atend)
			break;
	}
	if(timestretch_called == FALSE) {
		sprintf(errstr,"Not enough data in brkfile & not trapped earlier: do_timevariable_timestretch()\n");
		return(PROGRAM_ERROR);
	} 
	return(FINISHED);
}

/*************************** DO_TIMESTRETCHING ********************************
 *
 * Read/interpolate source analysis windows.
 *
 * (1)	initialise the position-buffer pointer. NB we don't use pbufptr as
 *		this is (?) still marking the current position in the output buffer.
 * (2)	For each entry in the position-array.
 * (3)	get the absolute position.
 * (4)	The window to intepolate FROM is this truncated
 *		e.g. position 3.2 translates to window 3.
 * (5)	Is this is a move on from the current window position? How far?
 * (6)	If we're in the final window, set the 'atend' flag.
 * (7)	If we need to move on, call advance-windows and set up a new PAIR of
 *		input windows.
 * (8)	Get Interpolation time-fraction.
 * (9)	Get output values by interpolation between input values.
 * (10) Write a complete out-windowbuf to output file.
 * (11)	Give on-screen user information.
 */

int do_timestretching(int *thatwindow,int count,dataptr dz)
{
	int exit_status;
	double amp0, amp1, phas0, phas1;
	int	 vc;
	int n, thiswindow, step;
	int atend = FALSE;						
	double here, frac;
	double *p = dz->parray[TSTR_PBUF];				/* 1 */   
	for(n=0;n<count;n++) {							/* 2 */
		here = *p++;								/* 3 */
		thiswindow = (int)here; /* TRUNCATE */		/* 4 */
		step = thiswindow - *thatwindow;			/* 5 */
		if(thiswindow == dz->wlength-1)				/* 6 */
			atend = TRUE;
		if(step) {
			if((exit_status = advance_along_input_windows(step,atend,dz))<0)   /* 7 */
				return(exit_status);
		}
		frac = here - (double)thiswindow;			/* 8 */
		for(vc = 0; vc < dz->wanted; vc += 2) {
			amp0  = dz->flbufptr[3][AMPP];			/* 9 */
			phas0 = dz->flbufptr[3][FREQ];
			amp1  = dz->flbufptr[4][AMPP];
			phas1 = dz->flbufptr[4][FREQ];
			dz->flbufptr[5][AMPP]  = (float)(amp0  + ((amp1  - amp0)  * frac));
			dz->flbufptr[5][FREQ]  = (float)(phas0 +	((phas1 - phas0) * frac));
		}
		if((dz->flbufptr[5] += dz->wanted) >= dz->flbufptr[2]) {
			if((exit_status = write_exact_samps(dz->flbufptr[1],dz->big_fsize,dz))<0)
				return(exit_status);
			dz->flbufptr[5] = dz->flbufptr[1];
		}
		*thatwindow = thiswindow;
	}
	return(FINISHED);
}

/******************************* DO_TIMEVARIABLE_COUNT *****************************/

int do_timevariable_count(int *outcnt,dataptr dz)
{
	int exit_status;
	int time0, time1, time2, totaldur;
	double stretch0, stretch1, thiswdur, nextwdur, valdiff, dtime0, dtime1;
	int atend = FALSE;
	int timestretch_called = FALSE;
	if((exit_status = get_both_vals_from_brktable(&dtime0,&stretch0,TSTR_STRETCH,dz))==FINISHED) {
		sprintf(errstr,"Not enough data in brkfile & not trapped earlier: do_timevariable_count()\n");
		return(PROGRAM_ERROR);
	}
	time0 = round(dtime0/dz->frametime);								  /* 2 */
	thiswdur  = 1.0/stretch0;												  /* 3 */
	while((exit_status = get_both_vals_from_brktable(&dtime1,&stretch1,TSTR_STRETCH,dz))==CONTINUE) {/* 4 */
		nextwdur  = 1.0/stretch1;
		if((time1 = round(dtime1/dz->frametime)) > dz->wlength) {
			time2  = time1;												  /* 5 */
			time1 = round(dz->param[TSTR_TOTIME]/dz->frametime);
			valdiff  = nextwdur - thiswdur;
			valdiff *= (double)(time1 - time0)/(double)(time2 - time0);
			nextwdur = thiswdur + valdiff;
			atend = TRUE;
		}
		totaldur = time1 - time0;
		timestretch_called = TRUE;


		*outcnt += round(fabs(calculate_number_of_output_windows(thiswdur,nextwdur,totaldur)));
		thiswdur  = nextwdur;
		time0 = time1;
		if(atend)
			break;
	}
	if(timestretch_called == FALSE) {
		sprintf(errstr,"Not enough data in brkfile & not trapped earlier: do_timevariable_count()\n");
		return(PROGRAM_ERROR);
	} 
	return(FINISHED);
}

/**************************** DIVIDE_TIME_INTO_EQUAL_INCREMENTS *****************************
 *
 * all time intervals are equal. Divide up total time thus.
 */

int divide_time_into_equal_increments(int *thatwindow,int startwindow,double dur,int count,dataptr dz)
{
	int exit_status;
	int n, remnant;
	int end   = dz->ptr[TSTR_PEND] - dz->ptr[TSTR_PBUF];
	int start = 0;
	while(count >= end) {
		for(n = start; n < end; n++)
			*(dz->ptr[TSTR_PBUF])++ = (dur * (double)n) + (double)startwindow;
		if((exit_status = do_timestretching(thatwindow,dz->iparam[TSTR_ARRAYSIZE],dz))<0)	  
			return(exit_status);
		dz->ptr[TSTR_PBUF] = dz->parray[TSTR_PBUF];
		start = end;
		end  += dz->iparam[TSTR_ARRAYSIZE];
	}
	if((remnant = count - start)>0) {
		for(n=start;n<count;n++)
			*(dz->ptr[TSTR_PBUF])++ = (dur * (double)n) + (double)startwindow;
	}
	return(FINISHED);
}

/**************************** GET_BOTH_VALS_FROM_BRKTABLE ****************************/

int get_both_vals_from_brktable(double *thistime,double *thisstretch,int brktab_no,dataptr dz)
{
	double *brkend = dz->brk[brktab_no] + (dz->brksize[brktab_no] * 2);
	if(dz->brkptr[brktab_no]>=brkend)
		return(FINISHED);
	*thistime = *(dz->brkptr[brktab_no])++;
	if(dz->brkptr[brktab_no]>=brkend) {
		sprintf(errstr,"Anomaly in get_both_vals_from_brktable().\n");
		return(PROGRAM_ERROR);
	}
	*thisstretch = *(dz->brkptr[brktab_no])++;
	return(CONTINUE);
}

/************************** TIMESTRETCH_THIS_SEGMENT *****************************
 *
 * Takes a group of input windows, counts number of output windows
 * corresponding to this buffer, and sets up, in pbuff, array(s) of values
 * which are the positions in the input array corresponding to the output
 * array positions.
 * (1)  If there is (almost) no change in duration of segments, calculates
 *       times on simple additive basis.
 * (2)  Otherwise, uses exponential formula.
 * (3)  If NOT passed a negative number (i.e. flagged), the sequence of time
 *      intervals is reversed.
 */

int timestretch_this_segment(int *thatwindow,int startwindow,double thiswdur,double nextwdur,int totaldur,dataptr dz)
{
	int    stretch_decreasing = TRUE;
	int   count;
	double dur, dcount = calculate_number_of_output_windows(thiswdur,nextwdur,totaldur);
	double param0, param1, param2;
	if(dcount < 0.0)
		stretch_decreasing = FALSE; 
	count = round(fabs(dcount));
	if(fabs(nextwdur - thiswdur)<=FLTERR) {            	/* 1 */
		dur = (double)totaldur/(double)count;
		return divide_time_into_equal_increments(thatwindow,startwindow,dur,count,dz);
	}    
	param0  = nextwdur - thiswdur;                      /* 2 */
	param1  = param0/(double)totaldur;
	param2  = thiswdur * (double)totaldur;
	if(stretch_decreasing==TRUE)							 	/* 3 */
		return calc_position_output_wdws_relative_to_input_wdws_for_decreasing_stretch
		(thatwindow,startwindow,count,totaldur,param0,param1,param2,dz);
	else
		return calc_position_output_wdws_relative_to_input_wdws_for_increasing_stretch
		(thatwindow,startwindow,count,param0,param1,param2,dz);
}

/*************************** ADVANCE_ALONG_INPUT_WINDOWS ****************************
 *
 * Advance window frame in input.
 * (1)	If got to end of data... Advanve ONE LESS than the distance
 * 	(wdw_to_advance) from last window-pair.
 * (2)	Else, advance by cnt windows.
 * (3)	If at end of buffer, copy THIS window to start of whole buffer,
 * (4)	And read more data in AFTER that (dz->wanted samps from start).
 * (6)	If this is the last window in the source file, there is no other
 *  	window to interpolate to, so set last window to same as this.
 */

int advance_along_input_windows(int wdw_to_advance,int atend,dataptr dz)			  
{
	int n, count;
	int samps_read;
	if(atend)						/* 1 */
		count = wdw_to_advance-1;
	else							/* 2 */
		count = wdw_to_advance;
	for(n=0;n<count;n++) {
		dz->flbufptr[3] = dz->flbufptr[4];			   	 /* ADVANCE LASTWINDOW TO THISWINDOW */
		if((dz->flbufptr[4] += dz->wanted) > dz->flbufptr[1]) {  /* ADVANCE THISWINDOW TO NEXT */
			memmove((char *)dz->bigfbuf,(char *)dz->flbufptr[3],dz->wanted * sizeof(float));
			dz->flbufptr[3] = dz->bigfbuf;				    /* 3 */
			dz->flbufptr[4] = dz->flbufptr[0];				/* 4 */
			if((samps_read = fgetfbufEx(dz->flbufptr[4],dz->big_fsize,dz->ifd[0],0)) < dz->wanted) {
				if(n <= count-2) {
					sprintf(errstr,"Program miscounted windows: anomaly 1 at EOF? : advance_along_input_windows()\n");
					return(PROGRAM_ERROR);
				}
				if(samps_read < 0) {
					sprintf(errstr,"Program miscounted windows: anomaly 2 at EOF? : advance_along_input_windows()\n");
					return(PROGRAM_ERROR);
				}
				return(FINISHED);
			}
			if(sloom)
				dz->total_samps_read += samps_read;
		}
	}
	if(atend)							/* 6 */
		dz->flbufptr[4] = dz->flbufptr[3];
	return(CONTINUE);
}

/*************************** CALCULATE_NUMBER_OF_OUTPUT_WINDOWS ***************************
 *
 * Given a sequence of events of varying duration, where initial and 
 * final durations are known, together with start and end times of the
 * total sequence, this function calculates the total number of events.
 *
 *	NB
 * (1)	Where the segment duration is increading, log(startwdur/edndur) is -ve,
 *		so the returned count is -ve.
 * (2)	Where segment duration is NOT changing, log(startwdur/endwdur), and hence dcount, would be zero.
 *		This situation is trapped by first checking for (approx) equality of startwdur and endwdur
 *		and calculating the count in simpler manner.
 */

double calculate_number_of_output_windows(double startwdur,double endwdur,int totaldur)
{
	double durdiff, dcount;
	if(flteq((durdiff = endwdur - startwdur),0.0)) {
		dcount = (double)totaldur/endwdur;
		return(dcount);
	}
	dcount  = startwdur/endwdur;
	dcount  = log(dcount);
	dcount *= (double)totaldur/durdiff;
	return(dcount);
}

/********* CALC_POSITION_OUTPUT_WDWS_RELATIVE_TO_INPUT_WDWS_FOR_DECREASING_STRETCH ************
 *
 * THE ARRAY OF positions has to be retrograded, but if we have more than
 * a single output-arrayfull of position values, we must start calculating
 * the output position array from the last input position, then invert those
 * positions so they are first positions, and so on!!!
 * (0)	As the array is to be filled backwards, start calculating positions
 *		from end of input. So end of first pass = end of input (= count).
 * (1)	Find how many locations remain in output-position buffer.
 * (2)	Start of first pass is this no of positions before end.
 * (3)	If start is < 0 this means that all the positions in the current input
 *		pass will fit in the output buffer in its current state, so we skip the
 *		loop and go to (10), Otherwise...
 * (4)	Mark the address in the output buffer where writing-values begins.
 * (5)	Store in output buffer, the positions RELATIVE to start of this segment
 *		of input values (i.e. relative to ZERO).
 * (6)	Retrograde this time-interval set.
 * (7)	Increment these relative values by startwindow, to give absolute
 *		position values.
 * (8)	Do the output, and reinitialise the output buffer pointer to start
 *		of buffer. Reset new input-block end to START of previous block
 *		(we're working backwards).
 * (9)	Decrement start of block by size of output array.
 * (10)	If either we did not enter the loop OR we have some input values
 *		left on leaving the principle loop, calcuate output positions of
 *		these items, and store in the output buffer (which will be flushed
 *		either by next call to timestretch_this_segment() or, at end, by flushbuf()).
 */

int calc_position_output_wdws_relative_to_input_wdws_for_decreasing_stretch
(int *thatwindow,int startwindow,int count,int totaldur,double param0,double param1,double param2,dataptr dz)
{
	int exit_status;
	int n, start, end = count;								/* 0 */
	double *p, *startptr;
	int tofill = dz->ptr[TSTR_PEND] - dz->ptr[TSTR_PBUF];	/* 1 */
	start = count - tofill;									/* 2 */
	while(start>=0) {										/* 3 */
		startptr = dz->ptr[TSTR_PBUF];						/* 4 */
		for(n=start;n<end;n++)			 					/* 5 */
			*(dz->ptr[TSTR_PBUF])++ = calculate_position(n,param0,param1,param2);
															/* 6 */
		if((exit_status = retrograde_sequence_of_time_intervals(totaldur - start,end - start,startptr,dz))<0)
			return(exit_status);
		for(p = startptr;p<dz->ptr[TSTR_PEND];p++)
			*p += (double)startwindow;						/* 7 */
		if((exit_status = do_timestretching(thatwindow,dz->iparam[TSTR_ARRAYSIZE],dz))<0)	
			return(exit_status);
		dz->ptr[TSTR_PBUF] = dz->parray[TSTR_PBUF];			/* 8 */
		end    = start;
		start -= dz->iparam[TSTR_ARRAYSIZE];				/* 9 */
	}
	if(end) {												/* 10 */
		startptr = dz->ptr[TSTR_PBUF];
		for(n=0;n<end;n++)
			*(dz->ptr[TSTR_PBUF])++ = calculate_position(n,param0,param1,param2);
		if((exit_status = retrograde_sequence_of_time_intervals(totaldur,end,startptr,dz))<0)
			return(exit_status);
		for(p = startptr;p<dz->ptr[TSTR_PBUF];p++)
			*p += (double)startwindow;
	}
	return(FINISHED);
}

/********* CALC_POSITION_OUTPUT_WDWS_RELATIVE_TO_INPUT_WDWS_FOR_INCREASING_STRETCH ************
 *
 * Find positions of output samples relative to input samples, buffer 
 * by buffer.
 */
   
int calc_position_output_wdws_relative_to_input_wdws_for_increasing_stretch
(int *thatwindow,int startwindow,int count,double param0,double param1,double param2,dataptr dz)
{
	int exit_status;
	int n;
	int start = 0;
	int end  = dz->ptr[TSTR_PEND] - dz->ptr[TSTR_PBUF];
	while(count>=end) {
		for(n=start;n<end;n++)
			*(dz->ptr[TSTR_PBUF])++ = calculate_position(n,param0,param1,param2) + (double)startwindow;
		if((exit_status = do_timestretching(thatwindow,dz->iparam[TSTR_ARRAYSIZE],dz))<0)
			return(exit_status);
		dz->ptr[TSTR_PBUF] = dz->parray[TSTR_PBUF];				   
		start = end;
		end  += dz->iparam[TSTR_ARRAYSIZE];
	}
	if(count-start) {
		for(n=start;n<count;n++)
			*(dz->ptr[TSTR_PBUF])++ = calculate_position(n,param0,param1,param2) + (double)startwindow;
	}
	return(FINISHED);
}

/*************************** CALCULATE_POSITION ****************************
 *
 * Do the position calculation using the exponential formula.
 */

double calculate_position(int x,double param0,double param1,double param2)
{
	double k;
	k  = param1;
	k *= (double)x;
	k  = exp(k);
	k *= param2;
	k -= param2;
	return(k/(param0));
}

/*************************** RETROGRADE_SEQUENCE_OF_TIME_INTERVALS **************************
 *
 * Accepts a sequence of times, starting in address startptr, 
 * and retrogrades the sequence of time-intervals, storing the
 * results back in startptr onwards.
 */

int retrograde_sequence_of_time_intervals(int endtime,int count,double *startptr,dataptr dz)
{
	double newtime, lasttime, tsize;
	int n;
	double *p  = startptr;
	dz->ptr[TSTR_QBUF] = dz->parray[TSTR_QBUF] + (count-1);
	newtime = endtime;
	for(n=0;n<(count-1);n++) {
		lasttime   = *p++;
		tsize      = *p - lasttime;
		newtime   -= tsize;
		*dz->ptr[TSTR_QBUF]-- = newtime; 
	}
	if(dz->ptr[TSTR_QBUF]!=dz->parray[TSTR_QBUF]) {
		sprintf(errstr,"counting problem: retrograde_sequence_of_time_intervals()\n");
		return(PROGRAM_ERROR);
	}
	*dz->ptr[TSTR_QBUF] = *p; /* put starttime at start of intervals inverted array */
	p  = startptr;
	for(n=0;n<count;n++)
		*p++ = *dz->ptr[TSTR_QBUF]++;
	return(FINISHED);
}

/**************************** SPECSTRETCH ***************************
 *
 * stretch the frq values in spectrum (e.g. harmonic -> inharmonic).
 */

int specstretch(dataptr dz)
{
	int exit_status;
	if(dz->brksize[STR_DEPTH] && flteq(dz->param[STR_DEPTH],0.0))
		return(FINISHED);
	if((exit_status = get_amp_and_frq(dz->flbufptr[0],dz))<0)
		return(exit_status);		
	if(dz->brksize[STR_DEPTH])
		dz->param[STR_NSHIFT] = (dz->param[STR_SL1] * dz->param[STR_DEPTH]) + 1.0;
	if((exit_status = stretch_spectrum(dz->param[STR_NSHIFT],dz))<0)
		return(exit_status);
	if((exit_status = put_amp_and_frq(dz->flbufptr[0],dz))<0)
		return(exit_status);
	return(FINISHED);
}

/************************** STRETCH_SPECTRUM *****************************/

int stretch_spectrum(double shift,dataptr dz)
{
	int    j, k, spectop ,specbot;
	double chshift, ratio, str_part;
	double slone, onels, flone;
	switch(dz->mode) {
	case(STRETCH_ABOVE): 		/* STRETCH SPECTRUM, above fdcno, BY SHIFT */
		if( shift >= 1.0) {  
			slone    = shift - 1.0;
			j        = dz->clength - 1;
			str_part = (double)((dz->clength - 1) - dz->iparam[STR_FDCNO]);
			chshift  = shift;
			k              = round((double)j/chshift);
			while( k >= dz->iparam[STR_FDCNO]) {
				dz->amp[j]  = dz->amp[k];
				dz->freq[j] = (float)chshift*dz->freq[k];
				if(--j <= dz->iparam[STR_FDCNO])
					break;
				ratio   = (double)(j - dz->iparam[STR_FDCNO])/str_part;
				ratio   = pow(ratio,dz->param[STR_EXP]);
				chshift = (ratio * (slone)) + 1.0;
				k       = round((double)j/chshift);
			}
		} else {
			onels = 1.0 - shift;
			spectop  = round((double)(dz->clength-1) * shift);
			str_part = (float)(spectop - dz->iparam[STR_FDCNO]);
			chshift  = 1.0;
			k        = dz->iparam[STR_FDCNO];
			j        = k;
			while( (k <= (dz->clength-1)) && (j <=spectop)) {
				dz->amp[j]  = dz->amp[k];
				dz->freq[j] = (float)chshift * dz->freq[k];
				j++;
				ratio   = (double)(j - dz->iparam[STR_FDCNO])/str_part;
				ratio   = pow(ratio,dz->param[STR_EXP]);
				chshift = 1.0 - (ratio * onels);
				k       = round((double)j/chshift);
			}
		}
		break;
	case(STRETCH_BELOW):		/* STRETCH SPECTRUM, below fdcno, DOWN BY SHIFT */
		flone = dz->iparam[STR_FDCNO] - 1;
		if(shift <= 1.0) {
			onels   = 1.0 - shift;
			j       = 1;
			k       = round((double)j/shift);
			chshift = shift;
			while( k < dz->iparam[STR_FDCNO]) {
				dz->amp[j]  = dz->amp[k];
				dz->freq[j] = (float)chshift*dz->freq[k];
				j++ ;
				ratio   = (double)j/flone;
				ratio   = pow(ratio,dz->param[STR_EXP]);
				chshift = shift + (ratio * onels);
				k       = round((double)j/chshift);
			}
		} else {
			slone    = shift - 1.0;
			j        = dz->iparam[STR_FDCNO] - 1;
			k        = j;
			specbot  = round(shift);
			str_part = flone - (double)specbot;
			chshift  = 1.0;

			while( (k > 0) && (j >= specbot)) {
				dz->amp[j]  = dz->amp[k];
				dz->freq[j] = (float)chshift * dz->freq[k];
				j-- ;
				ratio   = (flone - (double)j)/str_part;
				ratio   = pow(ratio,dz->param[STR_EXP]);
				chshift = (ratio * slone) + 1.0;
				k       = round((double)j/chshift);
			}
		}
		break;
	default:
		sprintf(errstr,"Unknown mode in stretch_spectrum()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

