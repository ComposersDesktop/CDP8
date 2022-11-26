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
//TW UPDATE
#include <processno.h>
#include <modeno.h>
#include <arrays.h>
#include <flags.h>
#include <combine.h>
#include <cdpmain.h>
#include <formants.h>
#include <speccon.h>
#include <sfsys.h>
#include <osbind.h>
#include <string.h>
#include <combine.h>

static int  setup_channel_mifdrqs(dataptr dz);
static int  check_formant_buffer(dataptr dz);
static int  preset_channels(dataptr dz);
static int  write_pitched_window(double fundamental,double *amptotal,dataptr dz);
//TW UPDATES + AVOID WARNING
static int  write_unpitched_window(double *amptotal,dataptr dz);
static int  write_silent_window(dataptr dz);

static int  normalise_in_file(double normaliser,dataptr dz);
static int  do_specmax(dataptr dz);
static int  if_loud_enough_to_go_in_list_put_in_list(float *ampstore,int *locstore,float val,int location,dataptr dz);
static int  generate_mean_values(int *loc1, int *loc2,dataptr dz);
static int  pichwise_mean_of_frqs(double *outfrq,double frq1,double frq2);
//TW UPDATE
static int read_envel_from_envfile(float **env,float **envend,int *envlen,int fileno,dataptr dz);

/****************************** SPECMAKE ****************************
 *
 * Construct an analfile, from pitchfile and formantdata only.
 */

int specmake(dataptr dz)
{
#define MAXANALAMP	(1.0)

	int exit_status;
	double normaliser, amptotal, thisfrq, maxamptotal = 0.0;

//TW UPDATE
	float *env = NULL, *envend;
	int envlen;

	if(dz->process==MAKE2) {
		if((exit_status = read_envel_from_envfile(&env,&envend,&envlen,2,dz))<0)
			return(exit_status);
		dz->wlength = min(dz->wlength,envlen);
	}

	dz->wanted      = dz->infile->origchans;	/* to reconstruct an analfile !! */

	if((exit_status = setup_formant_params(dz->ifd[1],dz))<0)
		return(exit_status);
	dz->flbufptr[1] = dz->flbufptr[2] + dz->descriptor_samps;
	if((exit_status = setup_channel_mifdrqs(dz))<0)
		return(exit_status);
	if((exit_status = check_formant_buffer(dz))<0)
		return(exit_status);
	dz->flbufptr[0] = dz->bigfbuf;
 	while(dz->total_windows < dz->wlength){
		amptotal = 0.0;
		if((exit_status = preset_channels(dz))<0)
			return(exit_status);
		if((thisfrq = dz->pitches[dz->total_windows])>0.0) {
			if((exit_status = write_pitched_window(thisfrq,&amptotal,dz))<0)
				return(exit_status);
//TW UPDATE
		} else if(flteq(thisfrq,NOT_SOUND)) {
			amptotal = 0.0;
			if((exit_status = write_silent_window(dz))<0)
				return(exit_status);
		} else {
//TW AVOID WARNING
			if((exit_status = write_unpitched_window(&amptotal,dz))<0)
				return(exit_status);
			if(dz->process==MAKE2) 
				normalise(env[dz->total_windows],amptotal,dz);
		}

		if(amptotal > maxamptotal)
			maxamptotal = amptotal;
		if((dz->flbufptr[0] += dz->wanted) >= dz->flbufptr[2]) {
			if((exit_status = write_exact_samps(dz->bigfbuf,dz->buflen,dz))<0)
				return(exit_status);
			dz->flbufptr[0] = dz->bigfbuf;
		}
		if((exit_status = move_along_formant_buffer(dz))<0)
			return(exit_status);
		if(exit_status!=CONTINUE)
			break;
		dz->total_windows++;
	}
	if(dz->flbufptr[0] != dz->bigfbuf) {
		if((exit_status = write_samps(dz->bigfbuf,(dz->flbufptr[0] - dz->bigfbuf),dz))<0)
			return(exit_status);
	}
//TW UPDATE
	if(dz->process!=MAKE2) {
		if(maxamptotal > MAXANALAMP)
			normaliser = MAXANALAMP/maxamptotal;
		else
			normaliser = 1.0;
		if((exit_status = normalise_in_file(normaliser,dz))<0)
			return(exit_status);
	}
	dz->specenvamp = NULL;	/* prevent SUPERFREE from attempting to free this pointer */
	return(FINISHED);
}

/*************************** SETUP_CHANNEL_MIFDRQS ********************/

int setup_channel_mifdrqs(dataptr dz)
{
	int cc;
	double thisfrq = 0.0;
	for(cc=0;cc<dz->clength;cc++) {
		dz->freq[cc] = (float)thisfrq;	/* Set up midfrqs of PVOC channels */
		thisfrq += dz->chwidth;
	}
	return(FINISHED);
}

/*************************** CHECK_FORMANT_BUFFER ********************/

int check_formant_buffer(dataptr dz)
{
	if(dz->flbufptr[1] >= dz->flbufptr[3]) {
		if(fgetfbufEx(dz->flbufptr[2],dz->buflen2,dz->ifd[1],0)<0) {
			sprintf(errstr,"data miscount in check_formant_buffer()\n");
			return(SYSTEM_ERROR);
		}
		dz->flbufptr[1] = dz->flbufptr[2];
	}
	return(FINISHED);
}

/*************************** PRESET_CHANNELS ********************/

int preset_channels(dataptr dz)
{
	int vc, cc;
	for(cc = 0, vc = 0; cc < dz->clength; cc++, vc+=2) {
		dz->flbufptr[0][AMPP] = 0.0f;
		dz->flbufptr[0][FREQ] = dz->freq[cc];
	}
	return(FINISHED);
}

/*************************** WRITE_PITCHED_WINDOW ********************/

int write_pitched_window(double fundamental,double *amptotal,dataptr dz)
{
	int exit_status;
	double thisamp, thisfrq = fundamental;
	int n = 1, vc, cc;
	dz->specenvamp = dz->flbufptr[1];
	while(thisfrq < dz->nyquist) {
		if((exit_status = get_channel_corresponding_to_frq(&cc,thisfrq,dz))<0)
			return(exit_status);
		vc = cc * 2;
		if((exit_status = getspecenvamp(&thisamp,thisfrq,0,dz))<0)
			return(exit_status);
		if(thisamp  > dz->flbufptr[0][vc]) {
			*amptotal -= dz->flbufptr[0][vc];
			dz->flbufptr[0][AMPP] = (float)thisamp;
			dz->flbufptr[0][FREQ] = (float)thisfrq;
			*amptotal += thisamp;
		}
		n++;
		thisfrq = fundamental * (double)n;
	}
	return(FINISHED);
}

/********************************** NORMALISE_IN_FILE ***********************************/

int normalise_in_file(double normaliser,dataptr dz)
{
	int exit_status;
	int vc;
	int bufcnt = 0;
	int total_samps_to_write = dz->total_samps_written;
	int samps_to_write;
	float *bufend;
//TW NEW MECHANISM writes renormalised vals from original file created (tempfile) into true outfile
// true outfile gets the name the user originally input
//seek eliminated
	if(sndcloseEx(dz->ifd[0]) < 0) {
		sprintf(errstr, "WARNING: Can't close input soundfile\n");
		return(SYSTEM_ERROR);
	}
	dz->ifd[0] = -1;
	if(sndcloseEx(dz->ofd) < 0) {
		sprintf(errstr, "WARNING: Can't close output soundfile\n");
		return(SYSTEM_ERROR);
	}
	dz->ofd = -1;
	if((dz->ifd[0] = sndopenEx(dz->outfilename,0,CDP_OPEN_RDWR)) < 0) {	   /*RWD Nov 2003, was RDONLY */
		sprintf(errstr,"Failure to reopen file %s for renormalisation.\n",dz->outfilename);
		return(SYSTEM_ERROR);
	}
	sndseekEx(dz->ifd[0],0,0);
	if((exit_status = create_sized_outfile(dz->wordstor[0],dz))<0) {
		sprintf(errstr,"Failure to create file %s for renormalisation.\n",dz->wordstor[0]);
		return(exit_status);
	}
	if((exit_status = reset_peak_finder(dz)) < 0)
		return exit_status;
	dz->total_samps_written = 0;
	fprintf(stdout,"INFO: Normalising.\n");
	fflush(stdout);
	while(total_samps_to_write>0) {
		if(fgetfbufEx(dz->bigfbuf,dz->buflen,dz->ifd[0],0)<0) {
			sprintf(errstr,"Sound read error.\n");
			close_and_delete_tempfile(dz->outfilename,dz);
			return(SYSTEM_ERROR);
		}
		dz->flbufptr[0]  = dz->bigfbuf;
		samps_to_write  = min(dz->buflen,total_samps_to_write);
		bufend = dz->flbufptr[0] + samps_to_write;
		while(dz->flbufptr[0] < bufend) {
			for(vc = 0;vc < dz->wanted; vc+=2)
				dz->flbufptr[0][AMPP] = (float)(dz->flbufptr[0][AMPP] * normaliser);
			dz->flbufptr[0] += dz->wanted;
		}
		if(samps_to_write>=dz->buflen) {
			if((exit_status = write_exact_samps(dz->bigfbuf,samps_to_write,dz))<0) {
				close_and_delete_tempfile(dz->outfilename,dz);
				return(exit_status);
			}
		} else if(samps_to_write > 0) {
			if((exit_status = write_samps(dz->bigfbuf,samps_to_write,dz))<0) {
				close_and_delete_tempfile(dz->outfilename,dz);
				return(exit_status);
			}
		}
		total_samps_to_write -= samps_to_write;
		bufcnt++;
	}	
	close_and_delete_tempfile(dz->outfilename,dz);
	return(FINISHED);
}

/*************************** SPECSUM ***********************/

int specsum(dataptr dz)
{
	int vc;
	switch(dz->vflag[SUM_IS_CROSS]) {
	case(TRUE):
		for(vc = 0; vc < dz->wanted; vc += 2)
			dz->flbufptr[0][vc] = (float)(dz->flbufptr[0][vc] + (dz->param[SUM_CROSS] * dz->flbufptr[1][vc]));
		break;
	case(FALSE):
		for(vc = 0; vc < dz->wanted; vc += 2)
			dz->flbufptr[0][vc] = (float)(dz->flbufptr[0][vc] + dz->flbufptr[1][vc]);
		break;
	default:
		sprintf(errstr,"unknown case in specsum()\n");
		return(PROGRAM_ERROR);				 
	}
	return(FINISHED);
}

/*************************** SPECDIFF ***********************/

int specdiff(dataptr dz)
{
	int vc;
	switch(dz->vflag[DIFF_SUBZERO]) {
	case(FALSE):
		switch(dz->vflag[DIFF_IS_CROSS]) {
		case(FALSE):
			for(vc = 0; vc < dz->wanted; vc += 2)
				dz->flbufptr[0][vc] = (float)max(0.0,dz->flbufptr[0][vc] - dz->flbufptr[1][vc]);
			break;
		case(TRUE):
			for(vc = 0; vc < dz->wanted; vc += 2)
				dz->flbufptr[0][vc] = (float)max(0.0,dz->flbufptr[0][vc] - (dz->flbufptr[1][vc] * dz->param[DIFF_CROSS]));
			break;
		}
		break;
	case(TRUE):
		switch(dz->vflag[DIFF_IS_CROSS]) {
		case(FALSE):
			for(vc = 0; vc < dz->wanted; vc += 2)
				dz->flbufptr[0][vc] = (float)(dz->flbufptr[0][vc] - dz->flbufptr[1][vc]);
			break;
		case(TRUE):
			for(vc = 0; vc < dz->wanted; vc += 2)
				dz->flbufptr[0][vc] = (float)(dz->flbufptr[0][vc] - (dz->flbufptr[1][vc] * dz->param[DIFF_CROSS]));
			break;
		}
		break;
	}
	return(FINISHED);
}

/**************************** SPECLEAF ****************************/

int specleaf(dataptr dz)
{
	int  exit_status;
	int  n;
	int samps_read, minsams = dz->insams[0];
	for(n=1;n<dz->infilecnt;n++)
		minsams = min(minsams,dz->insams[n]);
	dz->wlength = minsams/dz->wanted;
	while(dz->total_windows < dz->wlength) {
		for(n = 0;n < dz->infilecnt;n++) {
			if((samps_read = fgetfbufEx(dz->bigfbuf,dz->buflen,dz->ifd[n],0))<=0) {
				if(samps_read<0) {
					sprintf(errstr,"Sound read error.\n");
					return(SYSTEM_ERROR);
				} else {
					sprintf(errstr,"Accounting problem: specleaf()\n");
					return(PROGRAM_ERROR);
				}
			}
			if((exit_status = write_samps(dz->bigfbuf,dz->buflen,dz))<0)
				return(exit_status);
			if((dz->total_windows += dz->iparam[LEAF_SIZE]) >= dz->wlength)
				break;
		}
	}
	return(FINISHED);
}

/***************************** SPECMAX ***********************
 *
 * NB file0 is always the largest.
 */

int specmax(dataptr dz)
{
	int exit_status;
	int n, m, windows_in_buf, windows_read, samps_read, samps_to_write;
	int windows_to_process = dz->wlength; 
	while(windows_to_process) {
		if((samps_read = fgetfbufEx(dz->bigfbuf,dz->buflen,dz->ifd[0],0))<=0) {
			if(samps_read<0) {
				sprintf(errstr,"Sound read error.\n");
				return(SYSTEM_ERROR);
			} else {
				sprintf(errstr,"Buffer accounting problem: specmax()\n");
				return(PROGRAM_ERROR);
			}
		}
		windows_read = samps_read/dz->wanted;
		samps_to_write = samps_read;
		for(n=1;n<dz->infilecnt;n++) {
			if((samps_read = fgetfbufEx(dz->flbufptr[2],dz->buflen,dz->ifd[n],0))>0) {
				windows_in_buf = samps_read/dz->wanted;
				dz->flbufptr[0] = dz->bigfbuf;
				dz->flbufptr[1] = dz->flbufptr[2];
				for(m=0;m<windows_in_buf;m++) {
					if((exit_status = do_specmax(dz))<0)
						return(exit_status);
					dz->flbufptr[0] += dz->wanted;
					dz->flbufptr[1] += dz->wanted;
				}
			}
		}
		if(samps_to_write > 0) {
			if((exit_status = write_samps(dz->bigfbuf,samps_to_write,dz))<0)
				return(exit_status);
		}
		windows_to_process -= windows_read;
	}
	return(FINISHED);
}

/****************** DO_SPECMAX *********************/

int do_specmax(dataptr dz)
{
	int vc;

	for(vc=0;vc<dz->wanted;vc+=2) {
		if(dz->flbufptr[1][AMPP] > dz->flbufptr[0][AMPP]) {
			dz->flbufptr[0][AMPP] = dz->flbufptr[1][AMPP];
			dz->flbufptr[0][FREQ] = dz->flbufptr[1][FREQ];
		}
	}
	return(FINISHED);
}

/****************** SPECMEAN *********************/

int specmean(dataptr dz)
{
	int exit_status;
	int vc, cc;
	int *loc1 = dz->iparray[MEAN_LOC1];
	int *loc2 = dz->iparray[MEAN_LOC2];
	/* SUBZERO ALL LOUDEST-VALS TO BE USED IN THIS PASS */
	rectify_window(dz->flbufptr[0],dz);
	rectify_window(dz->flbufptr[1],dz);
	for(cc = 0; cc < dz->clength; cc++) {
		dz->windowbuf[1][cc] = -1.0f;
		dz->windowbuf[2][cc] = -1.0f;
	}
	if(dz->vflag[MEAN_ZEROED]) {	/* zero out-of-range channels */
		for(vc = 0; vc < dz->iparam[MEAN_BOT];vc += 2)
			dz->flbufptr[0][AMPP] = 0.0f;
		for(vc = dz->iparam[MEAN_TOP]; vc < dz->wanted; vc += 2)
			dz->flbufptr[0][AMPP] = 0.0f;
		return(FINISHED);
	}
	for(vc = dz->iparam[MEAN_BOT]; vc < dz->iparam[MEAN_TOP]; vc += 2) {
		if((exit_status = if_loud_enough_to_go_in_list_put_in_list(dz->windowbuf[1],loc1,dz->flbufptr[0][vc],vc,dz))<0)
			return(exit_status);
		if((exit_status = if_loud_enough_to_go_in_list_put_in_list(dz->windowbuf[2],loc2,dz->flbufptr[1][vc],vc,dz))<0)
			return(exit_status);
		dz->windowbuf[0][AMPP] = 0.0f;	/* initialise each output to zero amp */
		dz->windowbuf[0][FREQ] = dz->flbufptr[0][FREQ];
	}
	return generate_mean_values(loc1,loc2,dz);	   /* SWITCH AMPLITUDES */
}

/****************************** IF_LOUD_ENOUGH_TO_GO_IN_LIST_PUT_IN_LIST ******************************/

int if_loud_enough_to_go_in_list_put_in_list(float *ampstore,int *locstore,float val,int location,dataptr dz)
{
	int n = 0;
	if(val < 0.0)				/* eliminate any subzero amplitudes */
		return(FINISHED);
	while(ampstore[n] >= val) {	/* Move along until stored vals are less than input val */
		if(++n >= dz->clength)
			return(FINISHED);
	}
	if(n < dz->clength-1) {		/* If ness, shuffle existing vals */
		memmove((char *)(ampstore + n),(char *)(ampstore + n + 1),(dz->clength - n - 1) * sizeof(float));
		memmove((char *)(locstore + n),(char *)(locstore + n + 1),(dz->clength - n - 1) * sizeof(int));
	}
	ampstore[n] = val;			/* Store new value */
	locstore[n] = location;		/* store its location in original buffer */
	return(FINISHED);
}
	

/*************************** GENERATE_MEAN_VALUES ****************************/

int generate_mean_values(int *loc1, int *loc2,dataptr dz)
{
	int exit_status;
	double newfrq, newamp;
	int j, n, a, b;
	n = 0;
	while(n < dz->clength && dz->windowbuf[1][n] >= 0.0 && dz->windowbuf[2][n] >= 0.0) {
		a = loc1[n];
		b = loc2[n];
		switch(dz->mode) {
		case(MEAN_AMP_AND_PICH):
			newamp = (dz->flbufptr[0][a] + dz->flbufptr[1][b])/2.0;
			a++;	
			b++;
			if((exit_status = pichwise_mean_of_frqs
			(&newfrq,fabs((double)dz->flbufptr[0][a]),fabs((double)dz->flbufptr[1][b])))<0)
				return(exit_status);
			break;
		case(MEAN_AMP_AND_FRQ):
			newamp = (dz->flbufptr[0][a] + dz->flbufptr[1][b])/2.0;
			a++;
			b++;
			newfrq = (fabs(dz->flbufptr[0][a]) + fabs(dz->flbufptr[1][b]))/2.0;
			break;
		case(AMP1_MEAN_PICH):
			newamp = dz->flbufptr[0][a];
			a++;
			b++;
			if((exit_status = pichwise_mean_of_frqs
			(&newfrq,fabs((double)dz->flbufptr[0][a]),fabs((double)dz->flbufptr[1][b])))<0)
				return(exit_status);
			break;
		case(AMP1_MEAN_FRQ):
			newamp = dz->flbufptr[0][a];
			a++;
			b++;
			newfrq = (fabs(dz->flbufptr[0][a]) + fabs(dz->flbufptr[1][b]))/2.0;
			break;
		case(AMP2_MEAN_PICH):
			newamp = dz->flbufptr[1][b];
			a++;
			b++;
			if((exit_status = pichwise_mean_of_frqs
			(&newfrq,fabs((double)dz->flbufptr[0][a]),fabs((double)dz->flbufptr[1][b])))<0)
				return(exit_status);
			break;
		case(AMP2_MEAN_FRQ):
			newamp = dz->flbufptr[1][b];
			a++;
			b++;
			newfrq = (fabs(dz->flbufptr[0][a]) + fabs(dz->flbufptr[1][b]))/2.0;
			break;
		case(MAXAMP_MEAN_PICH):
			newamp = max(dz->flbufptr[0][a],dz->flbufptr[1][b]);
			a++;
			b++;
			if((exit_status = pichwise_mean_of_frqs
			(&newfrq,fabs((double)dz->flbufptr[0][a]),fabs((double)dz->flbufptr[1][b])))<0)
				return(exit_status);
			break;
		case(MAXAMP_MEAN_FRQ):
			newamp = max(dz->flbufptr[0][a],dz->flbufptr[1][b]);
			a++;
			b++;
			newfrq = (fabs(dz->flbufptr[0][a]) + fabs(dz->flbufptr[1][b]))/2.0;
			break;
		default:
			sprintf(errstr,"unknown program mode in generate_mean_values()\n");
			return(PROGRAM_ERROR);
		}		
		j = (int)((newfrq + dz->halfchwidth)/dz->chwidth); /* TRUNCATE */ /*FIND APPROP CHAN */

		if(j < dz->infile->channels) {
			j *= 2;

			if(newamp > dz->windowbuf[0][j]) {
				dz->windowbuf[0][j++] = (float)newamp;
				dz->windowbuf[0][j]   = (float)newfrq;
			}
		}
		n++;
	}
	for(j = dz->iparam[MEAN_BOT]; j < dz->iparam[MEAN_TOP]; j++) 
		dz->flbufptr[0][j] = dz->windowbuf[0][j];
	return(FINISHED);
}

/****************************** PICHWISE_MEAN_OF_FRQS ******************************/

int pichwise_mean_of_frqs(double *outfrq,double frq1,double frq2)
{
	double half_of_interval_in_octaves;
	if(frq1 == 0.0)	 {
		*outfrq = frq2;
		return(FINISHED);
	}
	if(frq2 == 0.0)	 {
		*outfrq = frq1;
		return(FINISHED);
	}
	half_of_interval_in_octaves = (LOG2(frq2/frq1))/2.0;
	*outfrq = pow(2.0,half_of_interval_in_octaves) * frq1;
	return(FINISHED);
}

/************************* SPECCROSS ***************************/

int speccross(dataptr dz)
{
	int vc;
	double valdiff;
	switch(dz->vflag[CROSS_INTP]) {
	case(CROS_FULL):
		for(vc = 0; vc < dz->wanted; vc += 2)
			dz->flbufptr[0][vc] = dz->flbufptr[1][vc];
		break;
	case(CROS_INTERP):
		for(vc = 0; vc < dz->wanted; vc += 2) {
			valdiff            = dz->flbufptr[1][vc] - dz->flbufptr[0][vc];
			valdiff           *= dz->param[CROS_INTP];
			dz->flbufptr[0][vc] = (float)(dz->flbufptr[0][vc] + valdiff);
		}
		break;
	default:
		sprintf(errstr,"Unknown flag value in speccross()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

//TW NEW UPDATE CODE BELOW
/*************************** WRITE_UNPITCHED_WINDOW ********************/

#define NOISEBASEX	(0.5)

//TW AVOID WARNING
int write_unpitched_window(double *amptotal,dataptr dz)
{
	int exit_status;
	double thisamp, basefrq = 0.0, thisfrq;
	double rand_offset;
	double noisrange = 1.0 - NOISEBASEX;
	int n = 1, vc, cc;
	int ccnt =  dz->wanted/2;
	n = 1;
	while(n <= ccnt) {
		rand_offset = (drand48() - (.5)) * dz->chwidth;
		thisfrq = basefrq + rand_offset;
		if(thisfrq < 0.0 || thisfrq > dz->nyquist)	
			continue;
		if((exit_status = get_channel_corresponding_to_frq(&cc,thisfrq,dz))<0)
			return(exit_status);
		vc = cc * 2;
		if((exit_status = getspecenvamp(&thisamp,thisfrq,0,dz))<0)
			return(exit_status);
		if(thisamp  > dz->flbufptr[0][vc]) {
			*amptotal -= dz->flbufptr[0][vc];
			dz->flbufptr[0][AMPP] = (float)(thisamp * (drand48() * noisrange));
			dz->flbufptr[0][FREQ] = (float)thisfrq;
			*amptotal += thisamp;
		}
		basefrq += dz->chwidth;
		n++;
	}
	return(FINISHED);
}

/*************************** WRITE_SILENT_WINDOW ********************/

int write_silent_window(dataptr dz)
{
	int vc, cc;
	double thisfrq = 0.0;
	for(cc=0, vc =0; cc < dz->clength; cc++, vc += 2) {
		dz->flbufptr[0][AMPP] = (float)0.0;
		dz->flbufptr[0][FREQ] = (float)thisfrq;
		thisfrq += dz->chwidth;
	}
	return(FINISHED);
}

/*************************** READ_ENVEL_FROM_ENVFILE **************************/

int read_envel_from_envfile(float **env,float **envend,int *envlen,int fileno,dataptr dz)
{
	int samps_to_read, secs_to_read;
	
	*envlen = dz->insams[fileno];
	if(((secs_to_read = dz->insams[fileno]/F_SECSIZE)*F_SECSIZE)!=dz->insams[fileno])
		secs_to_read++;
	samps_to_read = secs_to_read * F_SECSIZE;	
	if((*env = (float *)malloc((size_t)(samps_to_read * sizeof(float))))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for envelope array.\n");
		return(MEMORY_ERROR);
	}
	if(fgetfbufEx(*env,samps_to_read,dz->ifd[fileno],0) < 0) {
		sprintf(errstr,"fgetfbufEx() Anomaly in read_envel_from_envfile()\n");
		return(SYSTEM_ERROR);
	}		
	if((*env = (float *)realloc(*env,(size_t)(dz->insams[fileno] * sizeof(float))))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to reallocate envelope array.\n");
		return(MEMORY_ERROR);
	}
	*envend = *env + *envlen;
	return(FINISHED);
}


