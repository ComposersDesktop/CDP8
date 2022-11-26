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



/* floatsam version*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <structures.h>
#include <tkglobals.h>
#include <cdpmain.h>
#include <modify.h>
#include <modeno.h>
#include <globcon.h>
#include <modify.h>

#include <sfsys.h>

 /* RWD added do_clip, peak (later, sort out nChans PEAK stuff... */
static void gain(long samples,double multiplier,double maxnoclip,long *numclipped,
				 int do_clip,double *peak,dataptr dz);
static int find_max(double *maxfound,dataptr dz);
static int  find_maximum(int infile,long *maxloc,long *maxrep,double *maxsamp,dataptr dz);
static void	get_max(double *,long,long,long *,long *,dataptr dz);
static int  balance(double mratio,dataptr dz);
//TW replaced by peakchunk
//static int	write_maxsamp_properties(double,long,long,dataptr dz);
static int  do_balance(dataptr dz);
static int  gain_process(dataptr dz);
static int  get_normalisation_gain(dataptr dz);
static void do_normalise(dataptr dz);
static void do_phase_invert(dataptr dz);
static void find_maxsamp(double *maxfound,dataptr dz);
static int  do_multifile_loudness(dataptr dz);


#define	MAX_DBGAIN	 (90)

//TW UPDATE: further updated for floatsams and new-style clipping detection
int gainenv(long samples,long *last_total,double maxnoclip,long *numclipped,int do_clip,double *peak,dataptr dz);

/************************** LOUDNESS_PROCESS *******************************/

int loudness_process(dataptr dz)
{
	long n, m;
	double maxtime,intime, ratio;  /*RWD April 2004 needed to add these  */ 
	switch(dz->mode) {
	case(LOUD_PROPOR):
	case(LOUD_DB_PROPOR):
		maxtime = dz->brk[LOUD_GAIN][(dz->brksize[LOUD_GAIN] - 1) * 2];
		intime = (double)(dz->insams[0]/dz->infile->channels)/(double)dz->infile->srate;
		ratio = intime/maxtime;
		for(n=0,m = 0;n < dz->brksize[LOUD_GAIN];n++,m+=2)
			dz->brk[LOUD_GAIN][m] = dz->brk[LOUD_GAIN][m] * ratio;
		dz->mode = LOUDNESS_GAIN;
		break;
	case(LOUDNESS_DBGAIN):
		if(dz->brksize[LOUD_GAIN]) {
			for(n=0,m=1;n < dz->brksize[LOUD_GAIN];n++,m+=2)
				dz->brk[LOUD_GAIN][m] = dbtogain(dz->brk[LOUD_GAIN][m]);
		} else {
			dz->param[LOUD_GAIN] = dbtogain(dz->param[LOUD_GAIN]);
		}
		break;
	}
	switch(dz->mode) {
	case(LOUDNESS_LOUDEST):
	case(LOUDNESS_EQUALISE): 	return do_multifile_loudness(dz);
	case(LOUDNESS_BALANCE): 	return do_balance(dz);
	default:				 	return gain_process(dz);
	}
	return(FINISHED);	/* NOTREACHED */
}

/************************** GAIN_PROCESS *******************************/

int gain_process(dataptr dz)
{
	int exit_status;
	double maxnoclip = F_MAXSAMP, peakval = 0.0;
//TW UPDATE
	long numclipped = 0, last_total = 0;
	int clipsamps = 1;
	float *buffer = dz->sampbuf[0];

	if(dz->true_outfile_stype == SAMP_FLOAT)
		if(dz->clip_floatsams == 0)
			clipsamps = 0;

	switch(dz->mode) {
	case(LOUDNESS_NORM):
	case(LOUDNESS_SET):
		if((exit_status = get_normalisation_gain(dz))!=CONTINUE)
			return(exit_status);
		break;
	}
	display_virtual_time(0,dz);
	while(dz->samps_left > 0) {
		if((exit_status = read_samps(buffer,dz))<0) {
			sprintf(errstr,"Cannot read data from sndfile.\n");
			return(PROGRAM_ERROR);
		}
		switch(dz->mode) {
		case(LOUDNESS_GAIN):
		case(LOUDNESS_DBGAIN):
//TW UPDATE >>
			if(dz->brksize[LOUD_GAIN])
				gainenv(dz->ssampsread,&last_total,maxnoclip,&numclipped,clipsamps,&peakval,dz);
			else if(dz->param[LOUD_GAIN] <= FLTERR) {
				sprintf(errstr,"With gain of %lf the soundfile will be reduced to SILENCE!\n",dz->param[LOUD_GAIN]);
				return(DATA_ERROR);
			} else
//<< TW UPDATE
				gain(dz->ssampsread,dz->param[LOUD_GAIN],maxnoclip,&numclipped,clipsamps,&peakval,dz);
			break;
		case(LOUDNESS_NORM):
		case(LOUDNESS_SET):
			do_normalise(dz);
			break;
		case(LOUDNESS_PHASE):
			do_phase_invert(dz);
			break;
		}
		if(dz->ssampsread > 0) {
			if((exit_status = write_exact_samps(buffer,dz->ssampsread,dz))<0)
				return(exit_status);
		}
	}
	if(clipsamps && (numclipped > 0))
		sprintf(errstr, "WARNING: %ld samples were clipped.\n", numclipped);
	dz->peak_fval = peakval;
	/*will get written to header automatically*/
	return(FINISHED);
}

/************************** GAIN *******************************/

void gain(long samples,double multiplier,double maxnoclip,long *numclipped,
		  int do_clip,double *peak,dataptr dz)
{
	long i;            
	double s, buf_peak = *peak;
	float *buffer = dz->sampbuf[0];
	if(multiplier < 0.0) 
		multiplier = -multiplier;
	for(i = 0; i < samples; i++) {
		s = buffer[i] * multiplier;
		if(s >= 0) {
			if(do_clip && (s > maxnoclip)) {
				(*numclipped)++;
				s = F_MAXSAMP;
			}
			buf_peak = max(buf_peak,s);
		} else {
			if(do_clip && (-s > maxnoclip)) {
				(*numclipped)++;
				s = -F_MAXSAMP;
			}
			buf_peak = max(buf_peak,-s);
		}
		buffer[i] = (float) s;
		*peak = buf_peak;
	}
	
}

/**************************** DO_NORMALISE ***************************/
/* RWD NB my old floatsam version added a peak_val arg */
void do_normalise(dataptr dz)
{   
	register long i;

	float *buffer = dz->sampbuf[0];
	float dgain = (float) dz->param[LOUD_GAIN];
    for(i = 0; i < dz->ssampsread; i++) {
//TW	float sampl;
//TW   	sampl  = buffer[i] * dgain;
//TW   	buffer[i] =  sampl;
//TW must cast to float anyway, as multiply gives double
	   	buffer[i] =  (float)(buffer[i] * dgain);
    }
}

/**************************** DO_PHASE_INVERT ***************************/

void do_phase_invert(dataptr dz)
{   register long i;
	double samplong;
	float *buffer = dz->sampbuf[0];
    for(i = 0; i < dz->ssampsread; i++) {
    	samplong  = -buffer[i];
		samplong = min(samplong,(double)F_MAXSAMP);
    	buffer[i] = (float)(samplong);
    }
}

/****************************** FIND_MAX *************************/

int find_max(double *maxfound,dataptr dz)
{
	int exit_status;
	float *buffer = dz->sampbuf[0];
	display_virtual_time(0L,dz);
	while(dz->samps_left != 0) {
		if((exit_status = read_samps(buffer,dz))<0) {
			sprintf(errstr,"Cannot read data from sndfile.\n");
			return(PROGRAM_ERROR);
		}
		find_maxsamp(maxfound,dz);
		display_virtual_time(dz->total_samps_read,dz);
    }
	/*RWD NB this is a weird step, specifically for this program - balances to
	 * MAXSAMP if sfile is actually higher than that.
	 * must delete next line for a generic version!
	 */
	*maxfound = min(*maxfound,(double)F_MAXSAMP); /* e.g. found -32768: substitute 32767 */
	return(FINISHED);
}

/**************************** MAXSAMP ***************************/

void find_maxsamp(double *maxfound,dataptr dz)
{   register long i;
    double k;				 
	float *buffer = dz->sampbuf[0];
    for(i = 0; i < dz->ssampsread; i++) {
	if((k = fabs(buffer[i])) > *maxfound)
	    *maxfound = k;
    }
	return;
}

/**************************** GET_NORMALISATION_GAIN ***************************/

int get_normalisation_gain(dataptr dz)
{
	int exit_status;
	double maxfound = 0.0;
	double loud_level = dz->param[LOUD_LEVEL] * (double)F_MAXSAMP;
	
	print_outmessage_flush("Finding maximum amplitude.\n");
    if((exit_status = find_max(&maxfound,dz))<0)
		return(exit_status);
    if(flteq(maxfound,loud_level)) {
		sprintf(errstr,"File is already at the specified level.\n");
		return(GOAL_FAILED);
    }
    if((maxfound > loud_level) && dz->mode == LOUDNESS_NORM) {
		sprintf(errstr,"File is already above the specified level.\n");
		return(GOAL_FAILED);
    }

    dz->param[LOUD_GAIN]  = loud_level/maxfound;

	display_virtual_time(0L,dz);
	sprintf(errstr,"Normalising with Gain Factor = %lf\n", dz->param[LOUD_GAIN]);
	print_outmessage_flush(errstr);
	if(sndseekEx(dz->ifd[0],0L,0)<0) {
		sprintf(errstr,"sndseek() failed.\n");
		return(SYSTEM_ERROR);
    }
	reset_filedata_counters(dz);
	return(CONTINUE);
}

/***************************** DO_BALANCE ************************/

int do_balance(dataptr dz)
{
	int exit_status;
	long maxloc1, maxloc;
	long maxrep1, maxrep;
	double max1, max2;
	double mratio;
	display_virtual_time(0,dz);
	print_outmessage_flush("Getting max level from file 1.\n");
	if((exit_status = find_maximum(dz->ifd[0],&maxloc,&maxrep,&max1,dz))<0)
		return(exit_status);
	if(max1 <= 0) {
		sprintf(errstr,"First file has zero amplitude: can't proceed.\n");
		return(GOAL_FAILED);
	}
	maxloc1 = maxloc;
	maxrep1 = maxrep;
	reset_filedata_counters(dz);
	dz->samps_left = dz->insams[1];
	display_virtual_time(0,dz);
	print_outmessage_flush("Getting max level from file 2.\n");
	if((exit_status = find_maximum(dz->ifd[1],&maxloc,&maxrep,&max2,dz))<0)
		return(exit_status);
	mratio = max2/max1;
	if(sndseekEx(dz->ifd[0],0,0)<0) {
		sprintf(errstr,"sndseek failed.\n");
		return(SYSTEM_ERROR);
	}
	reset_filedata_counters(dz);
	display_virtual_time(0,dz);
	sprintf(errstr,"Adjusting gain of 1st infile by %lf\n",mratio);
	print_outmessage_flush(errstr);
	if((exit_status = balance(mratio,dz))< 0)	
		return(exit_status);
//TW No longer required: takes place in PEAK procedure
//	if((exit_status = write_maxsamp_properties(max2,maxloc1,maxrep1,dz))<0)
//		return(exit_status);
	return(FINISHED);
}

/***************************** BALANCE ************************/

int balance(double mratio,dataptr dz)
{
	int exit_status;
	long /*samps_read,*/ ssampsread, n;
	/*double maxsamp = 0;*/
	float *buffer = dz->sampbuf[0];
	while((ssampsread = fgetfbufEx(buffer, dz->buflen,dz->ifd[0],0)) > 0) {		
		for(n=0;n<ssampsread;n++)
			buffer[n] = (float) /*round*/((double)buffer[n] * mratio);
		if((exit_status = write_exact_samps(buffer,ssampsread,dz))<0)
			return(exit_status);
	}
	if(ssampsread<0) {
		sprintf(errstr,"Sound read error.\n");
		return(SYSTEM_ERROR);
	}
	return(FINISHED);
}

/***************************** FIND_MAXIMUM ************************/

int find_maximum(int infile,long *maxloc,long *maxrep,double *maxsamp,dataptr dz)
{
	long  ssampsread, last_total_ssampsread = 0, total_samps_read = 0;
	float *buffer = dz->sampbuf[0];
	*maxsamp = 0;
	while((ssampsread = fgetfbufEx(buffer, dz->buflen,infile,0)) > 0) {
		total_samps_read += ssampsread;		
		get_max(maxsamp,ssampsread,last_total_ssampsread,maxloc,maxrep,dz);
		last_total_ssampsread += ssampsread;
		display_virtual_time(total_samps_read,dz);
	}
	if(ssampsread<0) {
		sprintf(errstr,"Sound read error.\n");
		return(SYSTEM_ERROR);
	}
	return(FINISHED);
}

/***************************** GET_MAX ************************/

void get_max(double *maxsamp,long ssampsread,long last_total_ssampsread, long *maxloc, long *maxrep,dataptr dz)
{
	register long n;
	float thisamp;
	float *buffer = dz->sampbuf[0];
	for(n=0;n<ssampsread;n++) {
		if((thisamp = (float)fabs(buffer[n])) > *maxsamp) {
			*maxsamp = thisamp;
			*maxloc  = last_total_ssampsread + n;
			*maxrep = 1;
		} else if(flteq(thisamp,*maxsamp)) {
			(*maxrep)++;
		}
	}
	return;
}

/********************** WRITE_MAXSAMP_PROPERTIES ***************************/
//
//int write_maxsamp_properties(double max1,long maxloc,long maxrep,dataptr dz)
//{
//
//	float maxpfamp = (float)max1;
//
//	if(maxloc<0 || maxrep<0) {
//		sprintf(errstr,"Failed to establish location and/or repetition-count of max-sample\n");
//		return(GOAL_FAILED);
//	}
	/* TODO: scrap all this for the PEAK chunk instead! */
//* don't store in 'maxamp' as this is (long) in old files: 
//	if(sndputprop(dz->ofd, "maxpfamp", (char *)&maxpfamp, sizeof(float)) < 0) {
//		sprintf(errstr,"Can't write new max-sample to sndfile.\n");
//		return(PROGRAM_ERROR);
//	}
//	if(sndputprop(dz->ofd, "maxloc", (char *)&maxloc, sizeof(long)) < 0) {
//		sprintf(errstr,"Can't write location of max-sample to sndfile.\n");
//		return(PROGRAM_ERROR);
//	}
//	if(sndputprop(dz->ofd, "maxrep", (char *)&maxrep, sizeof(long)) < 0) {
//		sprintf(errstr,"Can't write repeat-count of max-sample to sndfile.\n");
//		return(PROGRAM_ERROR);
//	}
//	return(FINISHED);
//}

/********************** DO_MULTIFILE_LOUDNESS ***************************/

int do_multifile_loudness(dataptr dz)
{
	int exit_status;
	double *greater, greatest = 0.0, maxfound;
	/*RWD*/
	double dbamp;
	int clipsamps = 1;
	double peak_val = 0.0;

	int orig_ifd = dz->ifd[0];
 //TW REVISED Dec 2002
//	int n, do_gain, namelen, maxno = 0;
	int n, do_gain, maxno = 0;
//TW REVISED Dec 2002
//	char outfilename[256], *outfilenumber;	   /* RWD fname was only [64] */
	char outfilename[256];
	long numclipped = 0;
	double this_gain = 1.0, maxnoclip = 1.0;

	if((greater = (double *)malloc(dz->infilecnt * sizeof(double)))==NULL) {
		sprintf(errstr,"Insufficient memory to store file maxima\n");
		return(MEMORY_ERROR);
	}

	if(dz->true_outfile_stype == SAMP_FLOAT)
		if(dz->clip_floatsams == 0)
			clipsamps = 0;

	fprintf(stdout,"INFO: Finding loudest file\n");
	fflush(stdout);
	for(n=0;n<dz->infilecnt;n++) {
		maxfound = 0.0;
		dz->ifd[0] = dz->ifd[n];
		dz->total_samps_written = 0;
		dz->samps_left = dz->insams[n];
		dz->total_samps_read = 0;		
	    if((exit_status = find_max(&maxfound,dz))<0) {
			dz->ifd[0] = orig_ifd;
			return(exit_status);
		}
		if(maxfound > greatest) {
			greatest = maxfound;
			maxno = n;
		}
		greater[n] = maxfound/F_MAXSAMP;
	}
	greatest /= F_MAXSAMP;
	dz->ifd[0] = orig_ifd;
	dbamp = 20 * log10(greatest);  /*RWD*/
	if(dz->mode==LOUDNESS_LOUDEST) {
		fprintf(stdout,"INFO: loudest file is %s with level %lf (%.3f dB)\n",dz->wordstor[maxno],greatest,dbamp);
		fflush(stdout);
		return(FINISHED);
	}
	fprintf(stdout,"INFO: Equalising levels\n");
	fflush(stdout);
	for(n=0;n<dz->infilecnt;n++) {
		dz->ssampsread = 0;
		dz->total_samps_read = 0;
		dz->ifd[0] = dz->ifd[n];
		if(sndseekEx(dz->ifd[0],0L,0)<0) {
			sprintf(errstr,"sndseekEx() failed.\n");
			return(SYSTEM_ERROR);
	    }
		dz->total_samps_written = 0;
		dz->samps_left = dz->insams[n];
		dz->total_samps_read = 0;		
		if(!flteq(greatest,greater[n])) {
			this_gain = greatest/greater[n];
			maxnoclip  = fabs((double)F_MAXSAMP/this_gain);
			do_gain = 1;
		} else {
			do_gain = 0;
		}
		if(n>0) {
			strcpy(outfilename,dz->wordstor[dz->infilecnt]);
			/* RWD 9:2001 the -1 is unsafe if outname is only one char! */
//TW This is a sloom requirement,as standard sloom temp outfile name is 'cdptest0'
// and temporary-file housekeeping requires files to be named cdptestN in numeric order of N
// However, we can do it differently for cmdline case....
			if(sloom)
				insert_new_number_at_filename_end(outfilename,n,1);
			else
				insert_new_number_at_filename_end(outfilename,n,0);
			if((exit_status = create_sized_outfile(outfilename,dz))<0) {
				sprintf(errstr,"Cannot open output file %s\n", outfilename);
				dz->ifd[0] = orig_ifd;
				return(DATA_ERROR);
			}
		}
		while(dz->samps_left > 0) {
			if((exit_status = read_samps(dz->sampbuf[0],dz))<0) {
				sprintf(errstr,"Cannot read data from sndfile %d\n",n+1);
				dz->ifd[0] = orig_ifd;
				return(PROGRAM_ERROR);
			}
			if(do_gain)
				gain(dz->ssampsread,this_gain,maxnoclip,&numclipped,clipsamps,&peak_val,dz);
			if(dz->ssampsread > 0) {
				if((exit_status = write_samps(dz->sampbuf[0],dz->ssampsread,dz))<0) {
					dz->ifd[0] = orig_ifd;
					return(exit_status);
				}
			}
		}
		if(n < dz->infilecnt - 1) {
			if((exit_status = headwrite(dz->ofd,dz))<0) {
				dz->ifd[0] = orig_ifd;
				return(exit_status);
			}
			if((exit_status = reset_peak_finder(dz))<0)
				return(exit_status);
			if(sndcloseEx(dz->ofd) < 0) {
				fprintf(stdout,"WARNING: Can't close output soundfile %s\n",outfilename);
				fflush(stdout);
			}
			dz->ofd = -1;
		}
	}
	dz->ifd[0] = orig_ifd;
	/*RWD*/
	if(clipsamps && (numclipped > 0))
		sprintf(errstr, "WARNING: %ld samples were clipped.\n", numclipped);
	dz->peak_fval = peak_val;
	/*will get written to header automatically*/
	return(FINISHED);
}


//TW NEW FUNCTION, updated for floatsams and new-style clipping detection
/********************** GAINENV ***************************/

int gainenv(long samples,long *last_total,double maxnoclip,long *numclipped,int do_clip,double *peak,dataptr dz)
{
	int exit_status;
	register long i;            
	double s, buf_peak = *peak;
	double timeincr = 1.0/(double)dz->infile->srate;
	float *buffer = dz->sampbuf[0];
	double time = (double)(*last_total/dz->infile->channels)/(double)dz->infile->srate;

	for(i = 0; i < samples; i++) {
		if(i % dz->infile->channels == 0) {
			if((exit_status = read_value_from_brktable(time,LOUD_GAIN,dz))<0)
				return(exit_status);
			time += timeincr;
		}
		s = buffer[i];
		s *= dz->param[LOUD_GAIN];
		if(s >= 0) {
			if(do_clip && (s > maxnoclip)) {
				(*numclipped)++;
				s = 1.0;
			}
			buf_peak = max(buf_peak,s);
		} else {
			if(do_clip && (s < -maxnoclip)) {
				(*numclipped)++;
				s = -1.0;
			}
			buf_peak = max(buf_peak,-s);
		}
		buffer[i] = (float)s;
	}
	*peak = buf_peak;
	*last_total = dz->total_samps_read;
	return(FINISHED);
}

