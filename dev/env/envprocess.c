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



/* floatsam version */
#include <stdio.h>
#include <stdlib.h>
#include <structures.h>
#include <tkglobals.h>
#include <globcon.h>
#include <processno.h>
#include <modeno.h>
#include <arrays.h>
#include <envel.h>
#include <cdpmain.h>

#include <sfsys.h>
#include <envlcon.h>

#define MARGIN   	  (16)
#define BASE_REDUCE   ENV_DEFAULT_DATAREDUCE

static int  read_envel_from_envfile(float **env,float **envend,int fileno,dataptr dz);
static int  envcopy(float *envel1,float *envend1,float **envel2,float **envend2);
static int  convert_envelope_to_brkpnt_table(int envcnt,double window_size,double infiledur,dataptr dz);
static int  convert_brktable_to_envtable(double window_size,float **env,float **envend,dataptr dz);
static int  write_brkpnt_data_to_file(dataptr dz);
static int  write_envel_to_envfile(float *env,float *envend,dataptr dz);
static double *datareduce(double *q,double datareduction,int *bcnt,int paramno,dataptr dz);
static int  rejig_buffering(dataptr dz);
static int  adjust_last_val_of_brk(double *brk,double *brkend,int *datacnt,double endtime);
static int  store_brkpnt_envelope(int cnt,dataptr dz);
static int  read_value_from_brktable_from_brkpntfile(double thistime,float *thisval,dataptr dz);
static int  convert_brkvals_db_to_gain(int paramno,dataptr dz);
static int  convert_brkvals_gain_to_dB(int paramno,dataptr dz);
static int  convert_normalised_gain_val_to_dB(double *val);

/************************** PROCESS_ENVELOPE ************************/

int process_envelope(dataptr dz)
{
	int exit_status;
	int bufcnt, envcnt, cnt, other_envcnt;
	switch(dz->process) {
	case(ENV_CURTAILING):
	case(ENV_DOVETAILING):
	case(ENV_SWELL):
	case(ENV_ATTACK):
		if((exit_status = create_envelope(&cnt,dz))<0)
			return(exit_status);
		if((exit_status = store_brkpnt_envelope(cnt,dz))<0)
			return(exit_status);
		if((exit_status = apply_brkpnt_envelope(dz))<0)
			return(exit_status);
		break;
	case(ENV_CREATE):
		if((exit_status = create_envelope(&cnt,dz))<0)
			return(exit_status);
		if((exit_status = store_brkpnt_envelope(cnt,dz))<0)
			return(exit_status);
		switch(dz->mode) {
		case(ENV_ENVFILE_OUT):
			if((exit_status = convert_brktable_to_envtable(dz->param[ENV_WSIZE] * MS_TO_SECS,&dz->env,&dz->envend,dz))<0)
				return(exit_status);
			if((exit_status = write_envel_to_envfile(dz->env,dz->envend,dz))<0)
				return(exit_status);
			break;
		case(ENV_BRKFILE_OUT):
			if((exit_status = write_brkpnt_data_to_file(dz))<0)
				return(exit_status);
			break;
		default:
			sprintf(errstr,"Unknown case: ENV_CREATE in process_envelope()\n");
			return(PROGRAM_ERROR);
		}
		break;	
	case(ENV_EXTRACT):
		if((exit_status = extract_env_from_sndfile(&bufcnt,&envcnt,&dz->env,&dz->envend,0,dz))<0)
			return(exit_status);
		switch(dz->mode) {
		case(ENV_ENVFILE_OUT):
			if((exit_status = write_envel_to_envfile(dz->env,dz->envend,dz))<0)
				return(exit_status);
			break;
		case(ENV_BRKFILE_OUT):
 			if((exit_status = convert_envelope_to_brkpnt_table(envcnt,dz->outfile->window_size * MS_TO_SECS,dz->duration,dz))<0)
				return(exit_status);
			if((exit_status = write_brkpnt_data_to_file(dz))<0)
				return(exit_status);
			break;
		default:
			sprintf(errstr,"Unknown case: ENV_EXTRACT in process_envelope()\n");
			return(PROGRAM_ERROR);
		}
		break;
	case(ENV_ENVTOBRK):
	case(ENV_ENVTODBBRK):
		if((exit_status = read_envel_from_envfile(&dz->env,&dz->envend,0,dz))<0)
			return(exit_status);
		envcnt = dz->envend - dz->env;
		if((exit_status = convert_envelope_to_brkpnt_table(envcnt,dz->infile->window_size * MS_TO_SECS,dz->duration,dz))<0)
			return(exit_status);
		if(dz->process==ENV_ENVTODBBRK) {
			if((exit_status = convert_brkvals_gain_to_dB(dz->extrabrkno,dz))<0)
				return(exit_status);
		}
		if((exit_status = write_brkpnt_data_to_file(dz))<0)
			return(exit_status);
		break;
	case(ENV_BRKTODBBRK):
		if((exit_status = convert_brkvals_gain_to_dB(dz->extrabrkno,dz))<0)
			return(exit_status);
		if((exit_status = write_brkpnt_data_to_file(dz))<0)
			return(exit_status);								  
		break;
	case(ENV_DBBRKTOBRK):
		if((exit_status = convert_brkvals_db_to_gain(dz->extrabrkno,dz))<0)
			return(exit_status);
		if((exit_status = write_brkpnt_data_to_file(dz))<0)
			return(exit_status);
		break;
	case(ENV_DBBRKTOENV):
		if((exit_status = convert_brkvals_db_to_gain(dz->extrabrkno,dz))<0)
			return(exit_status);
		/* fall thro */
	case(ENV_BRKTOENV):
		if((exit_status = convert_brktable_to_envtable(dz->param[ENV_WSIZE] * MS_TO_SECS,&dz->env,&dz->envend,dz))<0)
			return(exit_status);
		if((exit_status = write_envel_to_envfile(dz->env,dz->envend,dz))<0)
			return(exit_status);
		break;
	case(ENV_WARPING):

		if(sloom) {
			fprintf(stdout,"INFO: Extracting envelope from input soundfile.\n");
			fflush(stdout);
		}
		if((exit_status = extract_env_from_sndfile(&bufcnt,&envcnt,&dz->origenv,&dz->origend,0,dz))<0)
			return(exit_status);
		if((exit_status = envcopy(dz->origenv,dz->origend,&dz->env,&dz->envend))<0)
			return(exit_status);
		if((exit_status = envelope_warp(&dz->env,&dz->envend,dz))<0)
			return(exit_status);
		switch(dz->mode) {
		case(ENV_CORRUGATING): 	/* generated envelope applied directly to source */
		case(ENV_TRIGGERING):	/* generated envelope applied directly to source */
		case(ENV_PEAKCNT):
			break;
		default:
			if((exit_status = envreplace(dz->env,&dz->envend,dz->origenv,dz->origend))<0)
				return(exit_status);
			break;
		}				
		if(dz->mode == ENV_PEAKCNT)
			return(FINISHED);
		if((exit_status = impose_envel_on_sndfile(dz->env,envcnt,bufcnt,0,dz))<0)
			return(exit_status);
		break;
	case(ENV_RESHAPING):
		if((exit_status = read_envel_from_envfile(&dz->env,&dz->envend,0,dz))<0)
			return(exit_status);
		if((exit_status = envelope_warp(&dz->env,&dz->envend,dz))<0)
			return(exit_status);
		if((exit_status = write_envel_to_envfile(dz->env,dz->envend,dz))<0)
			return(exit_status);
		break;
	case(ENV_REPLOTTING):
		if((exit_status = convert_brktable_to_envtable(dz->param[ENV_WSIZE] * MS_TO_SECS,&dz->env,&dz->envend,dz))<0)
			return(exit_status);
		if((exit_status = envelope_warp(&dz->env,&dz->envend,dz))<0)
			return(exit_status);
		envcnt = dz->envend - dz->env;
		if((exit_status = convert_envelope_to_brkpnt_table(envcnt,dz->param[ENV_WSIZE] * MS_TO_SECS,dz->duration,dz))<0)
			return(exit_status);
		if((exit_status = write_brkpnt_data_to_file(dz))<0)
			return(exit_status);
		break;
//TW NEW CASE
	case(ENV_PROPOR):
		return apply_brkpnt_envelope(dz);
	case(ENV_IMPOSE):
		switch(dz->mode) {
		case(ENV_BRKFILE_IN):
		case(ENV_DB_BRKFILE_IN):
			return apply_brkpnt_envelope(dz);
		case(ENV_ENVFILE_IN):
			exit_status = read_envel_from_envfile(&dz->env,&dz->envend,1,dz);
			break;
		case(ENV_SNDFILE_IN):

			if(sloom) {
				fprintf(stdout,"INFO: Extracting envelope from input soundfile.\n");
				fflush(stdout);
			}
			if((exit_status = extract_env_from_sndfile(&bufcnt,&envcnt,&dz->env,&dz->envend,1,dz))<0)
				return(exit_status);
			exit_status = rejig_buffering(dz);	/* deals with file2 having different srate or channels */
			reset_filedata_counters(dz);
			break;
		default:
			sprintf(errstr,"Unknown case for ENV_IMPOSE: in process_envelope()\n");
			return(PROGRAM_ERROR);
		}
		if(exit_status<0)
			return(exit_status);
		bufcnt = buffers_in_sndfile(/*dz->bigbufsize*/dz->buflen,0,dz);
		envcnt = dz->envend - dz->env;
		if((exit_status = impose_envel_on_sndfile(dz->env,envcnt,bufcnt,0,dz))<0)
			return(exit_status);
		break;
	case(ENV_REPLACE):
		switch(dz->mode) {
		case(ENV_ENVFILE_IN):
			exit_status = read_envel_from_envfile(&dz->env,&dz->envend,1,dz);
			if(sloom) {
				fprintf(stdout,"INFO: Extracting envelope from input soundfile.\n");
				fflush(stdout);
			}
			break;
		case(ENV_BRKFILE_IN):
		case(ENV_DB_BRKFILE_IN):
//			exit_status = convert_brktable_to_envtable(dz->param[ENV_WSIZE] * MS_TO_SECS,&dz->env,&dz->envend,dz);
			exit_status = convert_brktable_to_envtable
			((double)(dz->iparam[ENV_SAMP_WSIZE]/dz->infile->channels)/(double)dz->infile->srate,&dz->env,&dz->envend,dz);
			if(sloom) {
				fprintf(stdout,"INFO: Extracting envelope from input soundfile.\n");
				fflush(stdout);
			}
			break;
		case(ENV_SNDFILE_IN):
			if(sloom) {
				fprintf(stdout,"INFO: Extracting envelope from 1st input soundfile.\n");
				fflush(stdout);
			}
			if((exit_status = extract_env_from_sndfile(&bufcnt,&envcnt,&dz->env,&dz->envend,1,dz))<0)
				return(exit_status);
			exit_status = rejig_buffering(dz);	/* deals with file2 having different srate or channels */
			reset_filedata_counters(dz);
			break;
		default:
			sprintf(errstr,"Unknown case for ENV_REPLACE: in process_envelope()\n");
			return(PROGRAM_ERROR);
		}
		if(exit_status<0)
			return(exit_status);
		envcnt = dz->envend - dz->env;

		if(sloom) {
			fprintf(stdout,"INFO: Extracting envelope from 2nd input soundfile.\n");
			fflush(stdout);
		}
		if((exit_status = extract_env_from_sndfile(&bufcnt,&other_envcnt,&dz->origenv,&dz->origend,0,dz))<0)
			return(exit_status);
		if((exit_status = envreplace(dz->env,&dz->envend,dz->origenv,dz->origend))<0)
			return(exit_status);
		if((exit_status = impose_envel_on_sndfile(dz->env,envcnt,bufcnt,0,dz))<0)
			return(exit_status);
		break;
	case(ENV_PLUCK):
		if((exit_status = envelope_pluck(dz))<0)
			return(exit_status);
		break;
	case(ENV_TREMOL):
		if((exit_status = envelope_tremol(dz))<0)
			return(exit_status);
		break;
	default:
		sprintf(errstr,"Unknown case in process_envelope()\n");
		return(PROGRAM_ERROR);
	}
	return FINISHED;
}

/**************************** REJIG_BUFFERING ******************************
 *
 * recreates buffering to deal with 1ST file, once 2ND file has been dealt with!!
 */

int rejig_buffering(dataptr dz)
{
	int exit_status;
	if(dz->otherfile->srate    != dz->infile->srate
	|| dz->otherfile->channels != dz->infile->channels) {
		if((exit_status = generate_samp_windowsize(dz->infile,dz)) < 0)
			return(exit_status);			
		free(dz->bigbuf);
		if((exit_status = create_sndbufs_for_envel(dz)) < 0)
			return(exit_status);
	}
	return(FINISHED);
}

/*************************** READ_ENVEL_FROM_ENVFILE **************************/

int read_envel_from_envfile(float **env,float **envend,int fileno,dataptr dz)
{
	int samps_read /*, samps_to_read, secs_to_read*/;
	int envlen = dz->insams[fileno];
#ifdef NOTDEF
	if(((secs_to_read = dz->infilesize[fileno]/SECSIZE)*SECSIZE)!=dz->infilesize[fileno])
		secs_to_read++;
	bytes_to_read = secs_to_read * SECSIZE;	
#endif
	if((*env = (float *)malloc(/*bytes_to_read*/envlen * sizeof(float)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for envelope array.\n");
		return(MEMORY_ERROR);
	}
	if((samps_read = fgetfbufEx(*env, envlen,dz->ifd[fileno],0)) < 0) {
		sprintf(errstr,"Can't read samples from input envelfile\n");
		return(SYSTEM_ERROR);
	}
#ifdef NOTDEF
	if((*env = (float *)realloc(*env,dz->insams[fileno]))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to reallocate envelope array.\n");
		return(MEMORY_ERROR);
	}
#endif
	*envend = *env + envlen;
	return(FINISHED);
}

/*************************** ENVCOPY [ENVMOVE] *****************************
 * moves envelope from array env1 to array pointed to by env2.
 * returns the end+1 address of env2 array.
 */

int envcopy(float *envel1,float *envend1,float **envel2,float **envend2)
{
	float *p, *q;
	int tabsize = envend1 - envel1;
	if(*envel2 != NULL)
		free(*envel2);
	if((*envel2 = (float *)malloc(tabsize * sizeof(float)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to copy envelope array.\n");
		return(MEMORY_ERROR);
	}
	p = envel1;
	q = *envel2;
	while(p<envend1)
		*q++ = *p++;
	*envend2 = q;
	return(FINISHED);
}

/****************************** CONVERT_ENVELOPE_TO_BRKPNT_TABLE [ENVTOBRK]*********************************
 *
 * window_size is in seconds.
 */

int convert_envelope_to_brkpnt_table(int envcnt,double window_size,double infiledur,dataptr dz)
{
	int exit_status;
	double *q;
	float  *p = dz->env;
	double inc = window_size, lasttime, thisreduce = BASE_REDUCE;
	int bcnt, n, final_size;
	int paramno = dz->extrabrkno;
	int add_last_val = FALSE;
	int orig_bcnt;

	if(window_size <= 0.0) {
		sprintf(errstr,"No window_size set: convert_envelope_to_brkpnt_table()\n");
		return(PROGRAM_ERROR);
	}
	if(paramno < 0) {
		sprintf(errstr,"extrabuf not set up: convert_envelope_to_brkpnt_table()\n");
		return(PROGRAM_ERROR);
	}
	if(dz->brk[paramno]!=NULL)
		free(dz->brk[paramno]);
	if((dz->brk[paramno] = (double *)malloc(envcnt * sizeof(double) * 2))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to convert envelope to brkpnt table.\n");
		return(MEMORY_ERROR);
	}

	q = dz->brk[paramno];
	for(n=0;n<envcnt;n++) {
		*q++ = (double)n * inc;
		*q++ = (double)*p++;
	}
	bcnt = envcnt * 2;
	while(bcnt >= 6 && thisreduce < dz->param[ENV_DATAREDUCE]) {	/* RWD improvement --> */
		q = dz->brk[paramno] + 4;								/* incremental datareduction */
		orig_bcnt = bcnt;
		for(n=4;n<orig_bcnt;n+=2) {									
			q += 2;												
			q = datareduce(q,thisreduce,&bcnt,paramno,dz);						
		}															
		thisreduce += BASE_REDUCE;								
	}															
	if(bcnt >= 6) {												
		q = dz->brk[paramno] + 4;								
		orig_bcnt = bcnt;
		for(n=4;n<orig_bcnt;n+=2) {									
			q += 2;												
			q = datareduce(q,dz->param[ENV_DATAREDUCE],&bcnt,paramno,dz);		
		}														
	}
	n = bcnt;
	if(ODD(n)) {
		sprintf(errstr,"convert_envelope_to_brkpnt_table(): Problem of event pairing.\n");
		return(PROGRAM_ERROR);
	}
	lasttime = *(q-2);
	final_size = n;
	if(flteq(lasttime,infiledur)) {
		*(q-2) = infiledur;
		final_size = n;
	} else if(lasttime < infiledur) {
		final_size = n+2;
		add_last_val = TRUE;
	} else {
 		if(q-4 >= dz->brk[paramno]) {
			if((exit_status = adjust_last_val_of_brk(dz->brk[paramno],q,&final_size,infiledur))<0)
				return(exit_status);
		}
	} 				
	if((dz->brk[paramno] = (double *)realloc(dz->brk[paramno],final_size * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to reallocate new brkpnt table.\n");
		return(MEMORY_ERROR);
	}
	if(add_last_val) {
		*(dz->brk[paramno] + final_size - 2) = infiledur;
		*(dz->brk[paramno] + final_size - 1) = *(dz->brk[paramno] + final_size - 3);
	}
	dz->brkptr[paramno]  = dz->brk[paramno];
	dz->brksize[paramno] = final_size/2;
	return(FINISHED);
}
    
/***************************** DATAREDUCE **********************
 * Reduce data on passing from envelope to brkpnt representation.
 *
 * Take last 3 points, and if middle point has (approx) same value as
 * a point derived by interpolating between first and last points, then
 * ommit midpoint from brkpnt representation.
 */

double *datareduce(double *q,double datareduction,int *bcnt,int paramno,dataptr dz)
{
	double *p   = q-6, *midpair = q-4, *endpair = q-2, *tabend;
	double startime = *p++;
	double startval = sqrt(*p++);	/* RWD improvement: in power case, take sqrt */
	double midtime  = *p++;
	double midval   = sqrt(*p++);
	double endtime  = *p++;
	double endval   = sqrt(*p);
	double valrange     = endval - startval;
	double midtimeratio = (midtime-startime)/(endtime-startime);
	double guessval     = (valrange * midtimeratio) + startval;
	if(fabs(guessval - midval) < datareduction) {
		tabend = dz->brk[paramno] + (*bcnt);
		while (endpair < tabend) {		
			*midpair++ = *endpair++;
		}
		(*bcnt) -= 2;
		q -= 2;
	}
	return(q);
}

/****************************** CONVERT_BRKTABLE_TO_ENVTABLE [BRKTOENV] *********************************
 * convert brkpnt table to envelope table 
 *
 * Assume envelope space has already beem mallocd().
 *
 * window_size is in SECS.
 */

int convert_brktable_to_envtable(double window_size,float **env,float **envend,dataptr dz)
{
	int exit_status;
	int   n, envlen;
	float  *q;
	double here = 0.0;
	double inc = window_size;
	int    paramno = dz->extrabrkno;
	double duration;

	if(paramno < 0) {
		sprintf(errstr,"extrabuf not set up: convert_brktable_to_envtable()\n");
		return(PROGRAM_ERROR);
	}
	duration = *(dz->brk[paramno] + ((dz->brksize[paramno]-1)*2));
	if(window_size <= 0.0) {
		sprintf(errstr,"No window_size set: convert_brktable_to_envtable()\n");
		return(PROGRAM_ERROR);
	}
	if(inc < ENV_MIN_WSIZE * MS_TO_SECS) {
		sprintf(errstr,"Invalid window_size: convert_brktable_to_envtable()\n");
		return(PROGRAM_ERROR);
	}
	if(duration < inc) {
		sprintf(errstr,"Brktable duration less than window_size: Cannot proceed.\n");
		return(DATA_ERROR);
	}
	envlen = (int)((duration/inc) + 1.0);  /* round up */
	envlen += MARGIN;                  		/* allow for errors in float-calculation */
	if(dz->brk[paramno]==NULL) {
		sprintf(errstr,"No existing brkpnt table. convert_brktable_to_envtable()\n");
		return(PROGRAM_ERROR);
	}
	if(dz->brksize[paramno]<2) {
		sprintf(errstr,"Brkpnt table length invalid: convert_brktable_to_envtable()\n");
		return(PROGRAM_ERROR);
	}
	if(*env != NULL)
		free(*env);
	if((*env = (float *)malloc(envlen * sizeof(float)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for envelope array.\n");
		return(MEMORY_ERROR);
	}
	q = *env;
	n = 0;
	while(n<envlen) {
		if((here = (double)n * inc)>(duration+FLTERR))
			break;
		if((exit_status = read_value_from_brktable_from_brkpntfile(here,q,dz))<0)
			return(exit_status);
		q++;
		n++;
	}
	if(here<duration) {
		sprintf(errstr,"Not all of brkpnt table transferred: convert_brktable_to_envtable()\n");
		return(PROGRAM_ERROR);
	}
	envlen = n;
	if((*env = (float *)realloc(*env,envlen * sizeof(float)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to reallocate envelope array.\n");
		return(MEMORY_ERROR);
	}
	*envend = *env + envlen;
	return(FINISHED);
}

/*************************** WRITE_BRKPNT_DATA_TO_FILE [FPUT_BRK] *****************************/

int write_brkpnt_data_to_file(dataptr dz)
{   
	double *p;
	int n, cnt;
	int paramno = dz->extrabrkno;
	if(paramno < 0) {
		sprintf(errstr,"extrabrkno not set: write_brkpnt_data_to_file()\n");
		return(PROGRAM_ERROR);
	}
	p = dz->brk[paramno];
	cnt = dz->brksize[paramno];
	for(n=0;n<cnt;n++) {
		if((fprintf(dz->fp,"%lf\t%lf\n",*p,*(p+1)))<2) {
			sprintf(errstr,"fput_brk(): Problem writing brkpnt data to file.\n");
			return(PROGRAM_ERROR);
		}
		p += 2;
	}
/*
	if(fclose(dz->fp)<0)  {
		fprintf(stdout,"WARNING: Failure to close brkpnt data output file.\n");
		fflush(stdout);
	}
*/
	return(FINISHED);
}

/*************************** WRITE_ENVEL_TO_ENVFILE ***************************/

int write_envel_to_envfile(float *env,float *envend,dataptr dz)
{
	int envlen = envend - env;
	int samps_to_write = envlen  /* * sizeof(float)*/;
	int exit_status;
#ifdef NOTDEF
	int pseudo_bytes_to_write;
	int secs_to_write = bytes_to_write/SECSIZE;
	int bytes_written;
	if((secs_to_write * SECSIZE)!=bytes_to_write)
		secs_to_write++;
	pseudo_bytes_to_write = secs_to_write * SECSIZE;
#else
	int samps_written;
#endif
	if(samps_to_write > 0) {
		if((exit_status = write_samps_no_report(env,samps_to_write,&samps_written,dz))<0)
			return(SYSTEM_ERROR);
		else if(samps_written  == 0) {
			sprintf(errstr,"No data written to output envelfile.\n");
			return(SYSTEM_ERROR);
		}
	}
	return(FINISHED);
}

/*************************** ADJUST_LAST_VAL_OF_BRK ***************************/

int adjust_last_val_of_brk(double *brk,double *brkend,int *datacnt,double endtime)
{
	double *q = brkend - 4;
	double penulttime, penultval, lasttime, lastval, timeratio, valdiff;
	while(*q > endtime+FLTERR) {
		q -= 2;
		if(q < brk) {
			sprintf(errstr,"Cannot adjust last brkval: adjust_last_val_of_brk()\n");
			return(PROGRAM_ERROR);
		}
	}
	if(flteq(*q,endtime)) {
		*q = endtime;
		q += 2;
		*datacnt = (q - brk)/2;
	} else {
		penulttime   = *(q);
		penultval    = *(q+1);
		lasttime     = *(q+2);
		lastval      = *(q+3);
		timeratio    = (endtime - penulttime)/(lasttime - penulttime);
		valdiff      = timeratio * (lastval - penultval);
		*(q+2) = lasttime;
		*(q+3) = penultval + valdiff;
		q += 4;
		*datacnt = (q - brk)/2;
	}
	return(FINISHED);
}

/***************************** STORE_BRKPNT_ENVELOPE *****************************/

int store_brkpnt_envelope(int cnt,dataptr dz)
{
	int paramno = dz->extrabrkno;
	int n;
	double *p;																			  
	if(paramno < 0) {
		sprintf(errstr,"extrabrkno not established: store_brkpnt_envelope()\n");
		return(PROGRAM_ERROR);
	}
	if(dz->brk[paramno]!=NULL) {
		sprintf(errstr,"extrabrktable already exists: store_brkpnt_envelope()\n");
		return(PROGRAM_ERROR);
	}
	if((dz->brk[paramno] = (double *)malloc(cnt * 2 * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for brkpnt envelope array.\n");
		return(MEMORY_ERROR);
	}
	p = dz->brk[paramno];
	for(n=0;n<cnt;n++) {
		*p++ = dz->parray[ENV_CREATE_TIME][n];
		*p++ = dz->parray[ENV_CREATE_LEVL][n];
	}
	dz->brksize[paramno] = cnt;
	dz->brkptr[paramno]  = dz->brk[paramno];
	free(dz->parray[ENV_CREATE_TIME]);
	free(dz->parray[ENV_CREATE_LEVL]);
	dz->parray[ENV_CREATE_TIME] = (double *)0;
	dz->parray[ENV_CREATE_LEVL] = (double *)0;
	return(FINISHED);
}

/**************************** READ_VALUE_FROM_BRKTABLE_FROM_BRKPNTFILE *****************************/

int read_value_from_brktable_from_brkpntfile(double thistime,float *thisval,dataptr dz)
{
    double *endpair, *p, val;
    double hival, loval, hiind, loind;
	int paramno = dz->extrabrkno;
    if(!dz->brkinit[paramno]) {
		dz->brkptr[paramno]   = dz->brk[paramno];
		dz->firstval[paramno] = *(dz->brk[paramno]+1);
		endpair                 = dz->brk[paramno] + ((dz->brksize[paramno]-1)*2);
		dz->lastind[paramno]  = *endpair;
		dz->lastval[paramno]  = *(endpair+1);
 		dz->brkinit[paramno] = 1;
    }
	p = dz->brkptr[paramno];
	if(thistime <= *(dz->brk[paramno])) {
		*thisval = (float)dz->firstval[paramno];
		return(FINISHED);
	} else if(thistime >= dz->lastind[paramno]) {
		*thisval = (float)dz->lastval[paramno];
		return(FINISHED);
	} 
	if(thistime > *p) {
		while(*p < thistime)
			p += 2;
	} else {
		while(*p >= thistime)
			p -= 2;
		p += 2;
	}
	hival  = *(p+1);
	hiind  = *p;
	loval  = *(p-1);
	loind  = *(p-2);
	val    = (thistime - loind)/(hiind - loind);
	val   *= (hival - loval);
	val   += loval;
	*thisval  = (float)val;
 	dz->brkptr[paramno] = p;
    return(FINISHED);
}

/******************************** CONVERT_BRKVALS_DB_TO_GAIN *************************/

int convert_brkvals_db_to_gain(int paramno,dataptr dz)
{
	int exit_status;
	double *p = dz->brk[paramno];
	double *pend = p + (dz->brksize[paramno] * 2);
	p++;
	while(p < pend) {
		if((exit_status = convert_dB_at_or_below_zero_to_gain(p))<0)
			return(exit_status);
		p += 2;
	}
	return(FINISHED);
}

/******************************** CONVERT_BRKVALS_GAIN_TO_DB *************************/

int convert_brkvals_gain_to_dB(int paramno,dataptr dz)
{
	int exit_status;
	double *p = dz->brk[paramno];
	double *pend = p + (dz->brksize[paramno] * 2);
	p++;
	while(p < pend) {
		if((exit_status = convert_normalised_gain_val_to_dB(p))<0)
			return(exit_status);
		p += 2;
	}
	return(FINISHED);
}

/**************************** CONVERT_NORMALISED_GAIN_VAL_TO_DB ********************************/

int convert_normalised_gain_val_to_dB(double *val)
{
	double thisval = *val;
	double mingain = 1.0/pow(10,(-MIN_DB_ON_16_BIT/20.0));
	if(thisval > 1.0 || thisval < 0.0) {
		sprintf(errstr,"convert_normalised_gain_vals_to_dB() received non-normalised gain\n");
		return(PROGRAM_ERROR);
	}
	if(thisval <= mingain)
		*val = MIN_DB_ON_16_BIT;
	else {
		thisval  = 1.0/thisval;
		thisval  = log10(thisval);
		thisval *= 20.0;
		thisval  = -thisval;
		*val = thisval;
	}
	return(FINISHED);
}
