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
#include <memory.h>
#include <string.h>
#include <structures.h>
#include <tkglobals.h>
#include <pnames.h>
#include <flags.h>
#include <globcon.h>
#include <cdpmain.h>
#include <edit.h>
#include <modicon.h>
#include <processno.h>
#include <modeno.h>
#include <logic.h>
#include <flags.h>
#include <arrays.h>

#include <sfsys.h>

#include <osbind.h>

#ifdef unix
#define round(x) lround((x))
#endif

#define	UPSLOPE		(0)
#define DOWNSLOPE 	(1)

#define	IBUF	(0)
#define	OBUF	(1)
#define	SAVEBUF	(1)
#define	SPLBUF	(2)
#define	SPLBUF2	(3)

#define	BBUF		(0)
#define	BUFEND		(1)
#define	READBUF		(2)
#define	SPLICEBUF	(3)
#define	COPYBUF		(4)
#define	ENDSPLICE_ADDR	(3)

#define CUTOFF_OUTPUT (0)
#define JUST_CONTINUE (1)
#define WRITERESTOF_INPUT (2)

static int do_alternating_splices(int *in_noise,int *splice_cnt,int *edit_position,double splincr,dataptr dz);

#define movmem(from, to, bytes)         memcpy((to), (from), (bytes))

static void setup_startcut_sector_offset(dataptr dz);
static int  create_cut_buffer(dataptr dz);
static int  create_zcut_buffer(dataptr dz);
static int  create_excise_buffers(dataptr dz);
static int  create_insert_buffers(dataptr dz);
static int  establish_working_chunks(dataptr dz);
static void dosplice(float *buf, int samples, int chans, int invert);
static int  check_excise_splices(dataptr dz);
static int find_zero(int end,int allsamps,dataptr dz);
static int  do_excise_finish(int obufleft,dataptr dz);
static void crossplice(dataptr dz);
static int  do_excise_beginning(int *last_total_samps,int *obufleft,int k,dataptr dz);
static int  do_excise_ending(int *last_total_samps,int *obufleft,int *ibufleft,int k,dataptr dz);
static int  write_excise_end(int last_total_samps,int obufleft,int ibufleft,dataptr dz);
static int  copy_excise_chunk(int *last_total_samps,int *obufleft,int *ibufleft,int k,dataptr dz);
static int  gain_insert_buffer(float *thisbuf,int size,double gain);
static void setup_excise_outdisplay_len(dataptr dz);

static int  join_pconsistency(dataptr dz);
static int  create_join_buffer(dataptr dz);
static int  create_join_seq_buffer(dataptr dz);
static int  read_join_splice_remnant(int n,int splice_remnant,int oversamps,dataptr dz);
static void do_join_startsplice(int k,dataptr dz);
static void do_join_initial_splice(int chans,dataptr dz);
static void do_join_endsplice(dataptr dz);
static void housekeep1(int samps_read,int overspill,dataptr dz);
static void housekeep2(int samps_read,dataptr dz);
static int  do_join_write(int n,int splice_remnant,int oversamps,dataptr dz);
static int  reset_join_buffer_params(dataptr dz);

int algo2;

int do_insertsil_many(dataptr dz);
void do_other_splice(float *buf, int splicelen, int start_in_splice, int end_in_splice, int chans, int invert);

void ptr_sort(int end,dataptr dz);
void heavy_scat(int stereo_splicelen,dataptr dz);
void normal_scat(dataptr dz);
int  get_lengths(int stereo_splicelen,dataptr dz);
void do_insitu_splice(int start,int end,int splicecnt,int chans,dataptr dz);
void do_insitu_upsplice(int rel_bufstart,dataptr dz);
//int  create_an_outfile(int cutno, int cutlen, char *outfilename, char *outfilenumber, dataptr dz);
int  create_an_outfile(int cutno, int cutlen, char *outfilename, dataptr dz);
int  close_an_outfile(char *outfilename,dataptr dz);

static int 	do_linear(dataptr dz,double,int,double), do_nonlinear(dataptr dz,double,int,double);
static int	do_fromstart(dataptr dz);
static int  cut_chunk(dataptr dz,double startcut,double endcut,int count);

static void splice_obuf_start(float *obuf,dataptr dz);
static void splice_obuf_end(float *obuf,int *splice_start,int startsamp,int endsamp,dataptr dz);
static int  setup_insert2_overwrite_flag(dataptr dz);

static int silent_end;		

/******************************** EDIT_PCONSISTENCY ********************************/

int edit_pconsistency(dataptr dz)
{
	int exit_status;
	int chans = dz->infile->channels, k;
	double sr = (double)dz->infile->srate;
	int zz, zsecs;
	algo2 = 0;
	silent_end = 0;
	switch(dz->process) {
	case(EDIT_JOIN):
	case(JOIN_SEQ):
	case(JOIN_SEQDYN):
		return join_pconsistency(dz);

	case(EDIT_ZCUT):		 	/* check for stereo file: no splicelen data: REALLY UNNECESSARY */
		if(chans!=MONO) {
			sprintf(errstr,"This process only works for MONO files.\n");
			return(DATA_ERROR);
		}
		break;
	default:					/* establish splicelen constants */
		dz->iparam[CUT_SPLEN]     = round(dz->param[CUT_SPLEN] * MS_TO_SECS * sr) * chans;

		break;
	}
	if(dz->process!=EDIT_EXCISEMANY && dz->process!=INSERTSIL_MANY) {
		switch(dz->mode) {		/* read cutstart info, and convert to samps: info in file for EXCISEMANY  */
		case(EDIT_SECS):
			dz->iparam[CUT_CUT]     = round(dz->param[CUT_CUT] * sr) * chans;
			break;
		case(EDIT_SAMPS):
			dz->iparam[CUT_CUT]     = (dz->iparam[CUT_CUT]/chans) * chans;	/* align to sample-group boundary */
			break;
		case(EDIT_STSAMPS):
			dz->iparam[CUT_CUT]    *= chans;	
			break;
		}
	}
	switch(dz->process) {
	case(EDIT_CUT):				/* read cutend or cutdur info, and convert to samps */
	case(EDIT_ZCUT):
	case(EDIT_EXCISE):
	case(EDIT_INSERTSIL):
		switch(dz->mode) {
		case(EDIT_SECS):
			dz->iparam[CUT_END]     = round(dz->param[CUT_END] * sr) * chans;
			break;
		case(EDIT_SAMPS):
			dz->iparam[CUT_END]     = (dz->iparam[CUT_END]/chans) * chans;	/* align to sample-group boundary */
			break;
		case(EDIT_STSAMPS):
			dz->iparam[CUT_END]    *= chans;	
			break;
		}
		break;
	case(EDIT_INSERT2):
		dz->iparam[CUT_END]     = round(dz->param[CUT_END] * sr) * chans;
		if((exit_status = setup_insert2_overwrite_flag(dz))<0)
			return(exit_status);
		break;
	}
	switch(dz->process) {
	case(EDIT_CUT):		  		/* check consistency cutstart & cutend data */
	case(EDIT_ZCUT):
	case(EDIT_EXCISE):
		  if(dz->iparam[CUT_END] == dz->iparam[CUT_CUT]) {
			sprintf(errstr,"endcut = startcut: No cutting possible.\n");
			return(DATA_ERROR);
		}
		if(dz->iparam[CUT_END] < dz->iparam[CUT_CUT]) {
			fprintf(stdout,"WARNING: end cut before startcut: reversing these times.\n");
			fflush(stdout);
			iiswap(&(dz->iparam[CUT_END]),&(dz->iparam[CUT_CUT]));
		}
	 	break;
	}

	switch(dz->process) {		/* establish cutlen: check mutual consistency of params, for process */
	case(EDIT_INSERTSIL):
		dz->iparam[CUT_LEN] = dz->iparam[CUT_END];
		if(dz->iparam[CUT_SPLEN] * 2 > dz->iparam[CUT_LEN]) {

			sprintf(errstr,"Inserted silence is too short for splices.\n");
			return(GOAL_FAILED);
		}
		if(dz->vflag[INSERT_OVERWRITE] 
		&& (dz->iparam[CUT_CUT] + dz->iparam[CUT_LEN] - dz->iparam[CUT_SPLEN] >= dz->insams[0])) {
			if(dz->vflag[ACCEPT_SILENT_END]) {
				silent_end = 1;
				fprintf(stdout,"WARNING: Insertion will cut sound at end of file: leaving a silent section there.\n");
				fflush(stdout);
			} else {
				if(sloom)
					sprintf(errstr,"Insertion will cut off entire end of file: use EDIT : CUTOUT & KEEP\n");
				else
					sprintf(errstr,"Insertion will cut off entire end of file: use sfedit cut\n");
				return(GOAL_FAILED);

			}
		}
		if(dz->iparam[CUT_CUT] < dz->iparam[CUT_SPLEN]) {
			sprintf(errstr,"Insert time too close to start of file (allowing for splice)\n");
			return(GOAL_FAILED);
		}
		if(dz->iparam[CUT_CUT] >= dz->insams[0] - dz->iparam[CUT_SPLEN]) {
			if(dz->vflag[INSERT_OVERWRITE] || (dz->iparam[CUT_LEN] <= dz->iparam[CUT_SPLEN])) {
				sprintf(errstr,"Insert time beyond end of infile (allowing for splice)\n");
				return(GOAL_FAILED);
			} else {
				dz->tempsize = dz->insams[0] + dz->iparam[CUT_CUT];
				algo2 = 1;
				return(FINISHED);
			}
		}
		if(dz->vflag[INSERT_OVERWRITE])
			dz->tempsize = dz->insams[0];
		else
			dz->tempsize = dz->insams[0] + dz->insams[1] - (dz->iparam[CUT_SPLEN] * 2);
		break;
	case(INSERTSIL_MANY):
		dz->tempsize = dz->insams[0];
		algo2 = 1;
		break;
	case(EDIT_INSERT2):
		if(dz->iparam[CUT_END] >= dz->insams[0] - dz->iparam[CUT_SPLEN]) {
			sprintf(errstr,"Overwrite endtime is beyond end of infile (allowing for splice)\n");
			return(GOAL_FAILED);
		}
		/*fall thro */
	case(EDIT_INSERT):
		if(dz->iparam[CUT_CUT] < dz->iparam[CUT_SPLEN]) {
			sprintf(errstr,"Insert time too close to start of file (allowing for splice)\n");
			return(GOAL_FAILED);
		}
		if(dz->iparam[CUT_CUT] >= dz->insams[0] - dz->iparam[CUT_SPLEN]) {
			sprintf(errstr,"Insert time beyond end of infile (allowing for splice)\n");
			return(GOAL_FAILED);
		}
		if((dz->process == EDIT_INSERT) && dz->vflag[INSERT_OVERWRITE] 
		&& (dz->iparam[CUT_CUT] + dz->insams[1] - dz->iparam[CUT_SPLEN] >= dz->insams[0])) {
			sprintf(errstr,"Insertion will cut off whole end of first file: use cut and splice.\n");
			return(GOAL_FAILED);
		}
		if(dz->iparam[CUT_SPLEN] * 2 > dz->insams[1]) {
			sprintf(errstr,"Inserted file is too short for splices.\n");
			return(GOAL_FAILED);
		}
		if((dz->process == EDIT_INSERT) && dz->vflag[INSERT_OVERWRITE])
			dz->tempsize = dz->insams[0];
		else
			dz->tempsize = dz->insams[0] + dz->insams[1] - (dz->iparam[CUT_SPLEN] * 2);
		break;
	case(EDIT_CUT):
	case(EDIT_EXCISE):
		dz->iparam[CUT_LEN] = dz->iparam[CUT_END] - dz->iparam[CUT_CUT];
		dz->tempsize = dz->iparam[CUT_LEN];		/* EDIT_CUT only : EDIT_EXCISE reset below */
		if((dz->iparam[CUT_SPLEN] * 2) > dz->iparam[CUT_LEN]) {
			sprintf(errstr,"Edited portion is too short for specified splicelen.\n");
			return(GOAL_FAILED);
		}
		break;
	case(EDIT_CUTEND):
		dz->tempsize = dz->iparam[CUT_CUT];
		dz->iparam[CUT_CUT] = dz->insams[0] - dz->iparam[CUT_CUT];		
		if(dz->iparam[CUT_CUT] >= dz->insams[0] - (2 * dz->iparam[CUT_SPLEN])) {
			sprintf(errstr,"Edited portion is too short for specified splicelen.\n");
			return(GOAL_FAILED);
		}
		dz->iparam[CUT_END] = dz->insams[0];		
		dz->iparam[CUT_LEN] = dz->iparam[CUT_END] - dz->iparam[CUT_CUT];
		break;
	case(EDIT_ZCUT):
		if(dz->iparam[CUT_END] > dz->insams[0]) {
			fprintf(stdout,"WARNING: Assumed end of cut [%.2lf] is end of file [%.2lf]\n",
			dz->param[CUT_END],(double)(dz->insams[0]/chans)/sr);
			fflush(stdout);
			dz->iparam[CUT_END] = dz->insams[0];
		}
		dz->tempsize = dz->iparam[CUT_END] - dz->iparam[CUT_CUT];
		if(dz->iparam[CUT_END]== dz->insams[0])
			dz->iparam[CUT_GOES_TO_END] = TRUE;
		else
			dz->iparam[CUT_GOES_TO_END] = FALSE;
		break;
	}

	switch(dz->process) {			/* setup extra arrays or internal variables */
	case(EDIT_EXCISE):
		dz->iparam[EXCISE_CNT] = 1;
		dz->iparam[CUT_NO_END] = FALSE;
		if((dz->lparray[CUT_STTSAMP] = (int *)malloc(sizeof(int)))==NULL
		|| (dz->lparray[CUT_STTSPLI] = (int *)malloc(sizeof(int)))==NULL
		|| (dz->lparray[CUT_ENDSAMP] = (int *)malloc(sizeof(int)))==NULL
		|| (dz->lparray[CUT_ENDSPLI] = (int *)malloc(sizeof(int)))==NULL) {
			sprintf(errstr,"INSUFFICIENT_MEMORY to store excise times.\n");
			return(MEMORY_ERROR);
		}
		dz->lparray[CUT_STTSAMP][0] = dz->iparam[CUT_CUT];
		dz->lparray[CUT_ENDSAMP][0] = dz->iparam[CUT_END];
		/* fall thro */
	case(INSERTSIL_MANY):
	case(EDIT_EXCISEMANY):
		/* It's values were read from a file by special */
		zz = chans * 2;
		zsecs = dz->iparam[CUT_SPLEN]/zz;
		if(zsecs * zz != dz->iparam[CUT_SPLEN]) {
			zsecs++;
		}
		dz->iparam[CUT_SPLEN] = zsecs * zz;
		dz->iparam[CUT_HLFSPLEN] = dz->iparam[CUT_SPLEN]/2;	/* hence hlfsplen is whole no. chans */
		if(dz->infile->channels>MONO) {
			for(k=0;k<dz->iparam[EXCISE_CNT];k++) {			/* align to samp-grp boundaries */
				dz->lparray[CUT_STTSAMP][k] = (dz->lparray[CUT_STTSAMP][k]/chans) * chans;
				dz->lparray[CUT_ENDSAMP][k] = (dz->lparray[CUT_ENDSAMP][k]/chans) * chans;
			}
	    }
		if((exit_status = check_excise_splices(dz))<0)
			return(exit_status);
		setup_excise_outdisplay_len(dz);
		break;
	case(EDIT_CUT):
	case(EDIT_CUTEND):
		setup_startcut_sector_offset(dz);
		break;
	}
	return(FINISHED);
}

/******************************** EDIT_PREPROCESS ********************************/

int edit_preprocess(dataptr dz)
{
	int shsecsize = F_SECSIZE;			  /* RWD want to eliminate this... */

	switch(dz->process) {
	case(EDIT_CUT):
	case(EDIT_CUTEND):
		return establish_working_chunks(dz);
	case(EDIT_INSERT2):
	case(EDIT_INSERT):
	case(EDIT_INSERTSIL):
		if(algo2)
			break;
		dz->iparam[CUT_BUFXS]      = (dz->iparam[CUT_CUT] - dz->iparam[CUT_SPLEN]) % F_SECSIZE;
	 	dz->iparam[CUT_BUFREMNANT] = 
	 	(dz->iparam[CUT_SECSREMAIN] * shsecsize) - (dz->iparam[CUT_BUFXS] + dz->iparam[CUT_SPLEN]);

		break;
	}
 	return(FINISHED);
}

/******************************** SETUP_SECTOR_OFFSET_PARAMS ********************************/

void setup_startcut_sector_offset(dataptr dz)
{
	dz->iparam[CUT_BUFXS]         =  dz->iparam[CUT_CUT] % F_SECSIZE;
 	dz->iparam[CUT_BUFOFFSET]     = F_SECSIZE - dz->iparam[CUT_BUFXS];
}

/******************************** CREATE_EDIT_BUFFERS ********************************/

int create_edit_buffers(dataptr dz)
{
	int exit_status;
	switch(dz->process) {
	case(EDIT_CUT):
	case(EDIT_CUTEND):
	case(RANDCHUNKS):		exit_status = create_cut_buffer(dz);		break;
	case(MANY_ZCUTS):
	case(EDIT_ZCUT):		exit_status = create_zcut_buffer(dz);		break;
	case(EDIT_EXCISE):
	case(EDIT_EXCISEMANY):	exit_status = create_excise_buffers(dz);	break;
	case(EDIT_INSERT):
	case(EDIT_INSERT2):
	case(INSERTSIL_MANY):
	case(EDIT_INSERTSIL):	exit_status = create_insert_buffers(dz);	break;
	case(JOIN_SEQDYN):
	case(JOIN_SEQ):			exit_status = create_join_seq_buffer(dz);	break;
	case(EDIT_JOIN):		exit_status = create_join_buffer(dz);		break;
	default:
		sprintf(errstr,"Unknown process in create_edit_buffers()\n");
		return(PROGRAM_ERROR);
	}
	return(exit_status);
}

/******************************** CREATE_CUT_BUFFER ********************************/

int create_cut_buffer(dataptr dz)
{
	size_t bigbufsize;
    int secsize, sectorsize = dz->infile->channels * F_SECSIZE;
	bigbufsize = (size_t)Malloc(-1);
	bigbufsize /= sizeof(float);
	bigbufsize =  (bigbufsize / sectorsize) * sectorsize;
	if(bigbufsize < dz->iparam[CUT_SPLEN]) {
		bigbufsize = dz->iparam[CUT_SPLEN] + sectorsize;
		secsize = bigbufsize / sectorsize;
		if(secsize * sectorsize != bigbufsize)
			secsize++;
		bigbufsize = secsize * sectorsize;	
	}
	if(bigbufsize <= F_SECSIZE ){		  
		sprintf(errstr, "INSUFFICIENT MEMORY for sound.\n");
		return(MEMORY_ERROR);
	}
	dz->iparam[SMALLBUFSIZ] = bigbufsize - F_SECSIZE;
	if((dz->bigbuf = (float *)Malloc(bigbufsize * sizeof(float))) == 0) {
		sprintf(errstr, "INSUFFICIENT MEMORY for sound.\n");
		return(MEMORY_ERROR);
	}
	dz->buflen = (int) bigbufsize;		/*RWD*/
	return(FINISHED);
}

/******************************** CREATE_ZCUT_BUFFER ********************************/

int create_zcut_buffer(dataptr dz)
{
	/* RWD this may still all be dodgy for n-channel work!*/
	size_t bigbufsize;
    int secextra = F_SECSIZE;
//	secextra = (secextra / dz->infile->channels) * dz->infile->channels;
	if((secextra = (secextra / dz->infile->channels) * dz->infile->channels)<=0) {
		sprintf(errstr,"Can't handle this many channels.\n");
		return(DATA_ERROR);
	}
	bigbufsize = (size_t)Malloc(-1);
	bigbufsize /= sizeof(float);
	bigbufsize = (bigbufsize/F_SECSIZE) * F_SECSIZE;
		
	if((dz->bigbuf = (float *)Malloc((bigbufsize + secextra) * sizeof(float))) == NULL) {
		sprintf(errstr, "INSUFFICIENT MEMORY for sound.\n");
		return(MEMORY_ERROR);
	}
	dz->buflen = (int) bigbufsize;
	dz->sampbuf[OBUF] = dz->sampbuf[IBUF] = dz->bigbuf + secextra;
	return(FINISHED);
}

/******************************** CREATE_EXCISE_BUFFERS ********************************/

int create_excise_buffers(dataptr dz)
{   
	size_t bigbufsize, framesize;
	int splicespace = 2 * dz->iparam[CUT_SPLEN];
//TW : again algo depends on this SECSIZE structuring, at present
//	framesize = dz->infile->channels;
	framesize = dz->infile->channels * F_SECSIZE * 2;
	bigbufsize = (size_t) Malloc(-1) / sizeof(float);
	bigbufsize = (bigbufsize/framesize) * framesize;
	bigbufsize /= 2;
	if(bigbufsize<=0) {
		sprintf(errstr, "INSUFFICIENT MEMORY for sound.\n");
		return(MEMORY_ERROR);
	}
	if((dz->bigbuf = (float *) Malloc(((bigbufsize * 2) + splicespace) * sizeof(float)))==0) {
		sprintf(errstr, "INSUFFICIENT MEMORY for sound.\n");
		return(MEMORY_ERROR);
	}
	dz->buflen   = (int) bigbufsize;
	dz->sbufptr[IBUF]    = dz->sampbuf[IBUF]    = dz->bigbuf;
	dz->sbufptr[OBUF]    = dz->sampbuf[OBUF]    = dz->sampbuf[IBUF]  + dz->buflen;
	dz->sbufptr[SPLBUF]  = dz->sampbuf[SPLBUF]  = dz->sampbuf[OBUF]  + dz->buflen;
	dz->sbufptr[SPLBUF2] = dz->sampbuf[SPLBUF2] = dz->sampbuf[SPLBUF] + dz->iparam[CUT_SPLEN];
	memset((char *)dz->sampbuf[SPLBUF],0,dz->iparam[CUT_SPLEN]* sizeof(float));
	return FINISHED;
}

/******************************** CREATE_INSERT_BUFFERS ********************************/
				 
int create_insert_buffers(dataptr dz)
{
	size_t bigbufsize;
	int splice_space = dz->iparam[CUT_SPLEN] + F_SECSIZE; /* Allow for splice-end not aligning to sector boundary */
	int prelen  =  dz->iparam[CUT_CUT]  - dz->iparam[CUT_SPLEN];
	int prelen2, splicebuf_samplen, insertsize;
	float *cp;
	int framesize = dz->infile->channels * F_SECSIZE * 2;
	bigbufsize = (size_t) Malloc(-1);
	/*RWD*/
	dz->buflen = (bigbufsize / framesize)  * framesize;
	if(algo2) {
		if((dz->bigbuf = (float *)Malloc(dz->buflen * sizeof(float))) == 0) {
			sprintf(errstr, "INSUFFICIENT MEMORY.\n");
			return(MEMORY_ERROR);
		} else {
			return(FINISHED);
		}
	}
	dz->iparam[SMALLBUFSIZ] = /*dz->bigbufsize - SECSIZE;*/ dz->buflen - F_SECSIZE;
	if(dz->iparam[SMALLBUFSIZ] < splice_space) {
		sprintf(errstr,"INSUFFICIENT MEMORY for sound.\n");
		return(MEMORY_ERROR);
	}
	dz->iparam[CUT_BUFCNT] = prelen/dz->buflen;				/* no. of whole buffers in sfile, before splice */
	dz->iparam[CUT_SECCNT] = (prelen%dz->buflen)/F_SECSIZE;	/* no. whole sectors remaining, before splice */
	dz->iparam[CUT_SMPSREMAIN] = 
	dz->iparam[CUT_CUT] - ((dz->iparam[CUT_BUFCNT] * dz->buflen) + (dz->iparam[CUT_SECCNT] * F_SECSIZE)); 
																/* remainder of file */
	dz->iparam[CUT_SECSREMAIN]   = dz->iparam[CUT_SMPSREMAIN]/F_SECSIZE;
	if(dz->iparam[CUT_SECSREMAIN] * F_SECSIZE != dz->iparam[CUT_SMPSREMAIN])
		dz->iparam[CUT_SECSREMAIN]++;					  			/* GROSS number of sectors in remainder of file */
	if(dz->iparam[CUT_SECSREMAIN] * F_SECSIZE > dz->buflen)	{	 
		sprintf(errstr,"INSUFFICIENT MEMORY for sound.\n");
		return(MEMORY_ERROR);
	}
	switch(dz->process) {
	case(EDIT_INSERT):		insertsize = dz->insams[1];			break;	
	case(EDIT_INSERT2):		insertsize = dz->insams[1];			break;	
	case(EDIT_INSERTSIL):	insertsize = dz->iparam[CUT_LEN];	break;	
	default:
		sprintf(errstr,"Unknown process in create_insert_buffers()\n");
		return(PROGRAM_ERROR);
	}
	prelen2 =  insertsize - splice_space;
	dz->iparam[CUT_BUFCNT2]     = prelen2/dz->iparam[SMALLBUFSIZ];	     /* no. whole bufs in insertfile, before splice */
	dz->iparam[CUT_SECCNT2]     = (prelen2%dz->iparam[SMALLBUFSIZ])/F_SECSIZE; /* no. whole secs remaining, before splice */
	dz->iparam[CUT_SMPSREMAIN] =										 /* remainder of file */
	insertsize - ((dz->iparam[CUT_BUFCNT2] * dz->iparam[SMALLBUFSIZ]) + (dz->iparam[CUT_SECCNT2] * F_SECSIZE)); 
	dz->iparam[CUT_SECSREMAIN2]   = dz->iparam[CUT_SMPSREMAIN]/F_SECSIZE;

	if(dz->iparam[CUT_SECSREMAIN2] * F_SECSIZE != dz->iparam[CUT_SMPSREMAIN])
		dz->iparam[CUT_SECSREMAIN2]++;					  			       /* GROSS no. of sectors in remainder of file */
	if((dz->iparam[CUT_SECSREMAIN2] + 1) * F_SECSIZE > dz->buflen) {
		sprintf(errstr,"Buffers too small for splice.\n");
		return(MEMORY_ERROR);
	}

	splicebuf_samplen = (dz->iparam[CUT_SECSREMAIN] + 2) * F_SECSIZE;
	if((cp = dz->bigbuf = (float *)malloc(sizeof(float) * (dz->buflen + (splicebuf_samplen * 2)))) == NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for sound.\n");
		return(MEMORY_ERROR);
	}
	dz->sampbuf[0]       = dz->bigbuf;
	dz->sampbuf[SPLBUF]  = dz->sampbuf[0] + dz->buflen;
	dz->sampbuf[SAVEBUF] = dz->sampbuf[SPLBUF] + (splicebuf_samplen);
	memset((char *)dz->sampbuf[SAVEBUF],0,splicebuf_samplen * sizeof(float));
	memset((char *)dz->sampbuf[SPLBUF], 0,splicebuf_samplen * sizeof(float));
	return(FINISHED);
}

/******************************** ESTABLISH_WORKING_CHUNKS ********************************
 *
 * (2)	SPLICE_SPACE: 	number of sectors definitely containing the endsplice of the cut.
 * (3)	PRELEN: 	  	No of samps in file from end of last complete sector BEFORE cut, to a point
 *					  	definitely before final splice starts.
 * (4)	LOPOFF:			Counts up number of samps to read, from end of preliminary seek to BEFORE endsplice area.
 * (5)	CUT_SECSREMAIN:	Final number of sectors to read, containing all of the final splice.
 */

int establish_working_chunks(dataptr dz)
{
/* OLD **/
	int splice_space = dz->iparam[CUT_SPLEN] + F_SECSIZE; 							/* 2 */
	int prelen = dz->iparam[CUT_LEN] + dz->iparam[CUT_BUFXS] - splice_space; 	/* 3 */
	int lopoff = 0;																	/* 4 */

	int OK = 0;
	while (!OK) {
	/* count number of whole BUFFERS to read */
		dz->iparam[CUT_BUFCNT] = 0;
		if(prelen >= dz->buflen) {	  				    /* 1st attempt to read a whole buffer, reads dz->bigbufsize */
			dz->iparam[CUT_BUFCNT]++;
			prelen -= dz->buflen;
			lopoff += dz->buflen;
			while(prelen >= dz->iparam[SMALLBUFSIZ]) {	/* subsequent attempts read a smbuffsz, allowing for wraparound */
				dz->iparam[CUT_BUFCNT]++;
				prelen -= dz->iparam[SMALLBUFSIZ];
				lopoff += dz->iparam[SMALLBUFSIZ];
			}
		}
		/* count number of whole SECTORS to read */
		dz->iparam[CUT_SECCNT] = 0;
		while(prelen >= F_SECSIZE) {
			dz->iparam[CUT_SECCNT]++;
			prelen -= F_SECSIZE;
			lopoff += F_SECSIZE;
		}																	   /* calculate final no. samples left to read */
		dz->iparam[CUT_SMPSREMAIN] = dz->iparam[CUT_LEN] + dz->iparam[CUT_BUFXS] - lopoff;
		dz->iparam[CUT_SECSREMAIN] = dz->iparam[CUT_SMPSREMAIN] / F_SECSIZE;
		if(dz->iparam[CUT_SECSREMAIN] * F_SECSIZE != dz->iparam[CUT_SMPSREMAIN])					/* 5 */		
			dz->iparam[CUT_SECSREMAIN]++;					  			  /* GROSS number of sectors in remainder of file */
		if(dz->iparam[CUT_SECSREMAIN] * F_SECSIZE > dz->buflen)	{	   /* Possibly unnnecessary check: but being safe */
			dz->iparam[SMALLBUFSIZ] = dz->buflen;
			dz->buflen += F_SECSIZE;
			if((dz->bigbuf = (float *)realloc((char *)dz->bigbuf,dz->buflen * sizeof(float)))==NULL) {
				sprintf(errstr, "INSUFFICIENT MEMORY for endsplice.\n");
				return(MEMORY_ERROR);
			}
		} else
			OK = 1;
	}
	return(FINISHED);
}

/******************************** DO_CUT ********************************/

int do_cut(dataptr dz)
{
	int   exit_status;
	int  ref, cc, to_write = 0, seektest;
	int   first = 1;
	float *bufptr = dz->bigbuf;	  /* CUrrent buffer pointer */
	int  shsecsize = F_SECSIZE, samps_read;
	int is_startcut = TRUE;

 	if(dz->process==EDIT_CUT && dz->iparam[CUT_CUT]==0)
		is_startcut = FALSE;
	/* seek to start of sector before cut */
	if((seektest = sndseekEx(dz->ifd[0], (dz->iparam[CUT_CUT] / F_SECSIZE)  * F_SECSIZE, 0)) 
		!= (dz->iparam[CUT_CUT] / F_SECSIZE)  * F_SECSIZE) {		
		if(seektest<0) {
			sprintf(errstr,"sndseekEx() failed.\n");
			return(SYSTEM_ERROR);
		} else {
			sprintf(errstr,"Start time after end of file.\n");
			return(PROGRAM_ERROR);
		}
	}

	/* ANY WHOLE BUFFERS OF CUT ARE COPIED */

	ref = dz->iparam[SMALLBUFSIZ];
	for(cc = 1; cc <= dz->iparam[CUT_BUFCNT]; cc++)	{
		if(first) {
			first = 0;
			if((samps_read = fgetfbufEx(dz->bigbuf, dz->buflen,dz->ifd[0],0))!=dz->buflen) {
				sprintf(errstr,"Bad sound read 1.\n");
				if(samps_read<0)
					return(SYSTEM_ERROR);
				return(PROGRAM_ERROR);
			}
			bufptr += dz->iparam[CUT_BUFXS];  	/* reset input-pointer to start of cut */

			if(dz->iparam[CUT_SPLEN] > 0 && is_startcut)
				dosplice(bufptr, dz->iparam[CUT_SPLEN], dz->infile->channels, UPSLOPE);

			if((exit_status = write_exact_samps(bufptr, dz->iparam[SMALLBUFSIZ],dz))<0)
				return(exit_status);
			bufptr += ref;	   
			memcpy((char *)dz->bigbuf, (char *)bufptr, 
				dz->iparam[CUT_BUFOFFSET] * sizeof(float));
			bufptr = dz->bigbuf + dz->iparam[CUT_BUFOFFSET];
		} else {
			if((samps_read = fgetfbufEx(bufptr, dz->iparam[SMALLBUFSIZ],dz->ifd[0],0))!=dz->iparam[SMALLBUFSIZ]) {
				sprintf(errstr,"Bad sound read 2.\n");
				if(samps_read<0)
					return(SYSTEM_ERROR);
				return(PROGRAM_ERROR);
			}
			if((exit_status = write_exact_samps(dz->bigbuf, dz->iparam[SMALLBUFSIZ],dz))<0)
				return(exit_status);
			bufptr = dz->bigbuf + ref;
			memcpy((char *)dz->bigbuf, (char *)bufptr, 
				 dz->iparam[CUT_BUFOFFSET] * sizeof(float));
			bufptr = dz->bigbuf + dz->iparam[CUT_BUFOFFSET];
		}
	}
	
	/* ANY (FURTHER) WHOLE SECTORS BEFORE ENDSPLICE, ARE COPIED */
	
	if(dz->iparam[CUT_SECCNT]) {
 		if(first) {
			first = 0;
			if((samps_read  = 
			fgetfbufEx(dz->bigbuf, dz->iparam[CUT_SECCNT] * F_SECSIZE,dz->ifd[0],0))!=dz->iparam[CUT_SECCNT] * F_SECSIZE) {
				sprintf(errstr,"Bad sound read 3.\n");
				if(samps_read<0)
					return(SYSTEM_ERROR);
				return(PROGRAM_ERROR);
			}
			bufptr += dz->iparam[CUT_BUFXS];  	/* reset input-pointer to start of cut */

			if(dz->iparam[CUT_SPLEN]>0 && is_startcut) 		
												/* splice beginning of input */
				dosplice(bufptr, dz->iparam[CUT_SPLEN],dz->infile->channels, UPSLOPE);

			dz->iparam[CUT_SECCNT]--;
			if(dz->iparam[CUT_SECCNT]) {
  				if((exit_status = write_exact_samps(bufptr, dz->iparam[CUT_SECCNT] * F_SECSIZE,dz))<0)
					return(exit_status);
			}
			bufptr += dz->iparam[CUT_SECCNT] * shsecsize;
			memcpy((char *)dz->bigbuf, (char *)bufptr, 
				 dz->iparam[CUT_BUFOFFSET] * sizeof(float));
			bufptr = dz->bigbuf + dz->iparam[CUT_BUFOFFSET];
		} else {
			if((samps_read  = 
			fgetfbufEx(bufptr,dz->iparam[CUT_SECCNT] * F_SECSIZE,dz->ifd[0],0))
			      != dz->iparam[CUT_SECCNT] * F_SECSIZE) {
				sprintf(errstr,"Bad sound read 4.\n");
				if(samps_read<0)
					return(SYSTEM_ERROR);
				return(PROGRAM_ERROR);
			}
			if(dz->iparam[CUT_SECCNT]) {
  				if((exit_status = write_exact_samps(dz->bigbuf,dz->iparam[CUT_SECCNT] * F_SECSIZE,dz))<0)
					return(exit_status);
			}
			bufptr = dz->bigbuf + (dz->iparam[CUT_SECCNT] * shsecsize);
			memcpy((char *)dz->bigbuf, (char *)bufptr, 
				dz->iparam[CUT_BUFOFFSET] * sizeof(float));
			bufptr = dz->bigbuf + dz->iparam[CUT_BUFOFFSET];
		}
	}

	/* DO SECTORS CONTAINING ENDSPLICE OF CUT */

	if(first) {	/* IF THERE ARE NO WHOLE BUFFERS OR SECTORS BEFORE SPLICE STARTS */
		if((samps_read  = fgetfbufEx(dz->bigbuf, dz->iparam[CUT_SECSREMAIN] * F_SECSIZE,dz->ifd[0],0))<=0) {
			sprintf(errstr,"Bad sound read 5.\n");
			if(samps_read<0)
				return(SYSTEM_ERROR);
			return(PROGRAM_ERROR);
		}
		bufptr += dz->iparam[CUT_BUFXS];	  	/* reset input-pointer to start of cut */

		if(dz->iparam[CUT_SPLEN] > 0) { 		/* splice beginning (& end if not CUTEND) of file */
			if(is_startcut)
				dosplice(bufptr, dz->iparam[CUT_SPLEN],dz->infile->channels, UPSLOPE);
			bufptr = dz->bigbuf + dz->iparam[CUT_SMPSREMAIN] - dz->iparam[CUT_SPLEN];	
			if(dz->process==EDIT_CUT)
				dosplice(bufptr, dz->iparam[CUT_SPLEN],dz->infile->channels, DOWNSLOPE);
		}
		bufptr = dz->bigbuf + dz->iparam[CUT_BUFXS];
		if(dz->iparam[CUT_LEN] > 0) {
			if((exit_status = write_samps(bufptr, dz->iparam[CUT_LEN],dz))<0)
				return(exit_status);
		}

	} else {
		if((samps_read  = fgetfbufEx(bufptr, dz->iparam[CUT_SECSREMAIN] * F_SECSIZE,dz->ifd[0],0))<=0) {
			sprintf(errstr,"Bad sound read 6.\n");
			if(samps_read<0)
				return(SYSTEM_ERROR);
			return(PROGRAM_ERROR);
		}
		switch(dz->process) {
		case(EDIT_CUT):
			if(dz->iparam[CUT_SPLEN] > 0) {
				bufptr += dz->iparam[CUT_SMPSREMAIN] - dz->iparam[CUT_SPLEN];
				dosplice(bufptr, dz->iparam[CUT_SPLEN],dz->infile->channels, DOWNSLOPE);		   
			}
			to_write = dz->iparam[CUT_BUFOFFSET] + dz->iparam[CUT_SMPSREMAIN];
			break;
		case(EDIT_CUTEND):
			to_write = dz->iparam[CUT_BUFOFFSET] + samps_read;
			break;
		}			
		if(to_write > 0) {
			if((exit_status = write_samps(dz->bigbuf,to_write,dz))<0)
				return(exit_status);
		}
	}
	return(FINISHED);
}

/******************************** DOSPLICE ********************************
 * 
 * INVERT is flag to invert the direction of splice
 * 			0 = normal, 1 = inverted
 */

void dosplice(float *buf, int samples, int chans, int invert)
{
	double	a1 = 0.0, aincr;
	register int i, j;
	int sampgrps = samples/chans;

	aincr = 1.0/sampgrps;
	if(invert) {
		aincr = -aincr;
		a1 = 1.0 + aincr;
	}
	for(i = 0; i < samples; i+= chans) {
		for(j=0;j<chans;j++) 
			buf[i+j] = (float)((double)buf[i+j] * a1);
		a1 += aincr;
	}
}

/*************************** CHECK_EXCISE_SPLICES ****************************/

int check_excise_splices(dataptr dz)
{
	int k;
    if(dz->lparray[CUT_STTSAMP][0] <= 0) {
		dz->iparam[CUT_NO_STT] = TRUE;
		dz->lparray[CUT_STTSPLI][0] = 0;
	} else {
		dz->iparam[CUT_NO_STT] = FALSE;
		if((dz->lparray[CUT_STTSPLI][0] = dz->lparray[CUT_STTSAMP][0] - dz->iparam[CUT_HLFSPLEN])<0) {
			sprintf(errstr,"initial excise is too close to start of file for splice\n");
			return(DATA_ERROR);
		}
		dz->lparray[CUT_STTSAMP][0]  += dz->iparam[CUT_HLFSPLEN];
	}
	for(k=1;k<dz->iparam[EXCISE_CNT];k++) {
		dz->lparray[CUT_STTSPLI][k] = dz->lparray[CUT_STTSAMP][k] - dz->iparam[CUT_HLFSPLEN];
		dz->lparray[CUT_STTSAMP][k]  += dz->iparam[CUT_HLFSPLEN];
	}
	for(k=0;k<dz->iparam[EXCISE_CNT];k++) {
		dz->lparray[CUT_ENDSPLI][k] = dz->lparray[CUT_ENDSAMP][k] + dz->iparam[CUT_HLFSPLEN];
		dz->lparray[CUT_ENDSAMP][k]  -= dz->iparam[CUT_HLFSPLEN];
		if(dz->lparray[CUT_ENDSAMP][k] < dz->lparray[CUT_STTSAMP][k]) {
			sprintf(errstr,"Excised segment %d too short for splices (shorten splices?)\n",k+1);
			return(DATA_ERROR);
		}
		if(k && (dz->lparray[CUT_STTSPLI][k] < dz->lparray[CUT_ENDSPLI][k-1])) {
			sprintf(errstr,"Retained segment between excise %d & %d is too short for splices.\n",k,k+1);
			return(DATA_ERROR);
		}						 
	}
	if(dz->lparray[CUT_ENDSPLI][k-1] >= dz->insams[0]) {
		dz->iparam[CUT_NO_END] = TRUE;
		if(dz->iparam[CUT_NO_STT]==TRUE && dz->iparam[EXCISE_CNT]==1) {
			sprintf(errstr,"This process would remove the entire file!\n");
			return(DATA_ERROR);
		}
		if(dz->lparray[CUT_ENDSAMP][k-1] <dz->insams[0])
			fprintf(stdout,"INFO: End of final excise = END of FILE (it's closer than SPLICELEN).\n");
			fflush(stdout);
	}
	return(FINISHED);
}

/**************************** DO_ZCUT ********************************/

int do_zcut(dataptr dz)
{   
	int exit_status;
	int start_wr = 0, end_wr, last_end_wr, samps_to_wr, secs_to_wr ,extra, samps_written;
	int shsecsize = F_SECSIZE, last_total_samps;
	if(dz->process != MANY_ZCUTS)
		display_virtual_time(0L,dz);
	do {
		last_total_samps = dz->total_samps_read;
		if((exit_status = read_samps(dz->sampbuf[IBUF],dz))<0)
			return(exit_status);
	} while(dz->total_samps_read < dz->iparam[CUT_CUT]);
	if(dz->iparam[CUT_CUT]) {
		start_wr = dz->iparam[CUT_CUT] - last_total_samps;
		start_wr = find_zero(start_wr,dz->ssampsread,dz);
	}
	if(dz->iparam[CUT_END] <= dz->total_samps_read) {
		end_wr = dz->iparam[CUT_END] - last_total_samps;
		end_wr = find_zero(end_wr,dz->ssampsread,dz);
		if(end_wr - start_wr > 0) {
			if(dz->process == MANY_ZCUTS)
				exit_status = write_samps_no_report((dz->sampbuf[IBUF]+start_wr),(end_wr - start_wr),&samps_written,dz);
			else			
				exit_status = write_samps((dz->sampbuf[IBUF]+start_wr),(end_wr - start_wr),dz);
			if(exit_status < 0)
				return(exit_status);
		}
	} else {
		samps_to_wr = dz->buflen - start_wr;
		secs_to_wr  = samps_to_wr/shsecsize;
		extra	    = samps_to_wr - (secs_to_wr * shsecsize);
		samps_to_wr = secs_to_wr * shsecsize;
		if(samps_to_wr > 0) {
			if(dz->process == MANY_ZCUTS)
				exit_status = write_samps_no_report((dz->sampbuf[IBUF]+start_wr),samps_to_wr,&samps_written,dz);
			else			
				exit_status = write_samps((dz->sampbuf[IBUF]+start_wr),samps_to_wr,dz);
			if(exit_status < 0)
				return(exit_status);
		}
		end_wr      = dz->buflen - extra;
		memcpy((char *)(dz->sampbuf[OBUF] - extra),(char *)(dz->sampbuf[OBUF] + end_wr),extra*sizeof(float));
		dz->sampbuf[OBUF] -= extra;
		for(;;) {
			last_total_samps = dz->total_samps_read;
			if((exit_status = read_samps(dz->sampbuf[IBUF],dz))<0)
				return(exit_status);
			if(dz->total_samps_read < dz->iparam[CUT_END]) {	
				if(dz->process == MANY_ZCUTS)
					exit_status = write_samps_no_report(dz->sampbuf[OBUF],dz->buflen,&samps_written,dz);
				else			
					exit_status = write_samps(dz->sampbuf[OBUF],dz->buflen,dz);
				if(exit_status < 0)
					return(exit_status);
				memcpy((char *)dz->sampbuf[OBUF],(char *)(dz->sampbuf[OBUF] + end_wr),extra*sizeof(float));
			} else {
				last_end_wr = dz->iparam[CUT_END] - last_total_samps;
				if(!dz->iparam[CUT_GOES_TO_END]) {
					last_end_wr = find_zero(last_end_wr+extra,dz->ssampsread+extra,dz);
					if(last_end_wr > 0) {
						if(dz->process == MANY_ZCUTS)
							exit_status = write_samps_no_report(dz->sampbuf[OBUF],last_end_wr,&samps_written,dz);
						else			
							exit_status = write_samps(dz->sampbuf[OBUF],last_end_wr,dz);
						if(exit_status < 0)
							return(exit_status);
					}
				} else {
					if(last_end_wr + extra > 0) {
						if(dz->process == MANY_ZCUTS)
							exit_status = write_samps_no_report(dz->sampbuf[OBUF],(last_end_wr + extra),&samps_written,dz);
						else			
							exit_status = write_samps(dz->sampbuf[OBUF],(last_end_wr + extra),dz);
						if(exit_status < 0)
							return(exit_status);
					}
				}
				break;
			}
		}
	}
	return(FINISHED);
}

/************************ FIND_ZERO **************************/

int find_zero(int end,int allsamps,dataptr dz)
{
	int n = end;
	int m = end;
	int  phase, in_nloop = 0, in_mloop = 0;
	if(smpflteq(dz->sampbuf[OBUF][n],0.0))
		return(end);
	if(dz->sampbuf[OBUF][n]>0.0)
		phase = 1;
	else
		phase = -1;
	switch(phase) {
	case(1):
		while(n>0) {
			in_nloop = 1;
			if(dz->sampbuf[OBUF][n]<0.0) {  
				n++;  
				break;  
			}
			n--;
		}
		if(n==0 && in_nloop)        
			n = dz->buflen; /* FLAG NO ZERO FOUND BACKWRD */
		while(m<allsamps) {
			in_mloop = 1;
			if(dz->sampbuf[OBUF][m]<0.0)  
				break;
			m++;
		}
		if(m==allsamps && in_mloop) 
			m = dz->buflen;	/* FLAG NO ZERO FOUND FORWRD */
		break;
	case(-1):
		while(n>0) {
			in_nloop = 1;
			if(dz->sampbuf[OBUF][n]>0.0) {  
				n++;  
				break;  
			}
			n--;
		}
		if(n==0 && in_nloop) 	    
			n = dz->buflen;
		while(m<allsamps) {
			in_mloop = 1;
			if(dz->sampbuf[OBUF][m]>0.0)  
				break;
			m++;
		}
		if(m==allsamps && in_mloop) 
			m = dz->buflen;
		break;
	}
	if(m==dz->buflen && n==dz->buflen)  /* NO ZERO CROSSING FOUND */
		return(allsamps);
	if((m-end) < (end-n))	/* ELSE RETURN CLOSEST ZERO CROSSING */
		return(m);
	return(n);
}

/****************************** DO_EXCISE *************************/

int do_excise(dataptr dz)
{   
	int exit_status;
	int k;
	int last_total_samps;
	int obufleft, ibufleft;
	dz->total_samps_read = 0;
	display_virtual_time(0L,dz);
	last_total_samps = dz->total_samps_read;
    if((exit_status = read_samps(dz->sampbuf[IBUF],dz))<0)
		return(exit_status);
	obufleft = dz->buflen;
	ibufleft = dz->buflen;
	if((exit_status = copy_excise_chunk(&last_total_samps,&obufleft,&ibufleft,0,dz))<0)
		return(exit_status);
	obufleft = dz->buflen - (dz->lparray[CUT_STTSPLI][0] - last_total_samps);
	ibufleft = dz->buflen - (dz->lparray[CUT_STTSPLI][0] - last_total_samps);
	exit_status = CONTINUE;
	for(k=0;k<dz->iparam[EXCISE_CNT];k++) {
		if(k==0) {
			if(!dz->iparam[CUT_NO_STT])	{
				if((exit_status = do_excise_beginning(&last_total_samps,&obufleft,0,dz))<0)
					return(exit_status);
			}			
		} else {
			if((exit_status = do_excise_beginning(&last_total_samps,&obufleft,k,dz))<0)
				return(exit_status);
		}
    	if(k==dz->iparam[EXCISE_CNT]-1 && dz->iparam[CUT_NO_END])
			exit_status = FINISHED;
 	  	if(exit_status==FINISHED)
    		break;
		if((exit_status = do_excise_ending(&last_total_samps,&obufleft,&ibufleft,k,dz))<0)
			return(exit_status);
	}
  	if(exit_status==FINISHED)
		return do_excise_finish(obufleft,dz);
	return write_excise_end(last_total_samps,obufleft,ibufleft,dz);
}

/*************************** DO_EXCISE_FINISH ***********************/

int do_excise_finish(int obufleft,dataptr dz)
{
	int exit_status;
	int overflow;
	dz->sbufptr[SPLBUF] = dz->sampbuf[SPLBUF];
	if((overflow = dz->iparam[CUT_SPLEN] - obufleft)<=0) {
		memcpy((char *)dz->sbufptr[OBUF],(char *)dz->sbufptr[SPLBUF],dz->iparam[CUT_SPLEN]*sizeof(float));
	dz->sbufptr[OBUF] += dz->iparam[CUT_SPLEN];
	if(dz->sbufptr[OBUF]-dz->sampbuf[OBUF] > 0) {
		if((exit_status = write_samps(dz->sampbuf[OBUF],(dz->sbufptr[OBUF]-dz->sampbuf[OBUF]),dz))<0)
			return(exit_status);
	}
	} else {
		memcpy((char *)dz->sbufptr[OBUF],(char *)dz->sbufptr[SPLBUF],obufleft*sizeof(float));
		if((exit_status = write_samps(dz->sampbuf[OBUF],dz->buflen,dz))<0)
			return(exit_status);
		dz->sbufptr[SPLBUF] += obufleft;
		if(overflow > 0) {
			if((exit_status = write_samps(dz->sbufptr[SPLBUF],overflow,dz))<0)
				return(exit_status);
		}
	}
	return(FINISHED);
}

/**************************** CROSSPLICE ******************************/

void crossplice(dataptr dz)
{   
	double a1, aincr;
	int chans = dz->infile->channels;
	int    sampgrps = dz->iparam[CUT_SPLEN]/chans;
	float  *splptr2 = dz->sampbuf[SPLBUF2]; 
	float  *splptr  = dz->sampbuf[SPLBUF]; 
	int i;
	int j;

	aincr = 1.0/sampgrps;
	a1 = aincr;
	
	for(i = 0; i < dz->iparam[CUT_SPLEN]; i+= chans) {
		for(j=0;j<chans;j++) {
			*splptr2   = (float)((double)(*splptr2) * a1);
			*splptr++ += *splptr2++;
		}
		a1 += aincr;
	}
}

/*********************************** DO_EXCISE_BEGINNING *************************/

int do_excise_beginning(int *last_total_samps,int *obufleft,int k,dataptr dz)
{
	int exit_status = CONTINUE;
	int overflow, remnant, startsplice_in_buf, startsamp_in_buf, samps_to_copy;
	dz->sbufptr[SPLBUF] = dz->sampbuf[SPLBUF];
	startsplice_in_buf  = dz->lparray[CUT_STTSPLI][k] - *last_total_samps;
	dz->sbufptr[IBUF] = dz->sampbuf[IBUF] + startsplice_in_buf;
	*obufleft = dz->sampbuf[SPLBUF] - dz->sbufptr[OBUF];
	startsamp_in_buf  = dz->lparray[CUT_STTSAMP][k] - *last_total_samps;
	if((overflow = startsamp_in_buf - dz->buflen)<=0) {
		memcpy((char *)dz->sbufptr[SPLBUF],(char *)dz->sbufptr[IBUF],
			dz->iparam[CUT_SPLEN] * sizeof(float));
		dz->sbufptr[IBUF] += dz->iparam[CUT_SPLEN];
		if(dz->sbufptr[IBUF] >= dz->sampbuf[OBUF]) {
			if(dz->samps_left<=0)
				exit_status = FINISHED;
			else {
				*last_total_samps = dz->total_samps_read;
			    if((exit_status = read_samps(dz->sampbuf[IBUF],dz))<0)
					return(exit_status);
				dz->sbufptr[IBUF]  = dz->sampbuf[IBUF];
				exit_status = CONTINUE;
			}
		}
	} else {
		remnant = dz->iparam[CUT_SPLEN]-overflow;
		memcpy((char *)dz->sbufptr[SPLBUF],(char *)dz->sbufptr[IBUF],remnant*sizeof(float));
		dz->sbufptr[SPLBUF] += remnant;
		while(overflow > 0) {
			*last_total_samps = dz->total_samps_read;
			if((exit_status = read_samps(dz->sampbuf[IBUF],dz))<0)
				return(exit_status);
			dz->sbufptr[IBUF] = dz->sampbuf[IBUF];
			samps_to_copy = min(overflow,dz->buflen);
			overflow -= samps_to_copy;
			memcpy((char *)dz->sbufptr[SPLBUF],(char *)dz->sbufptr[IBUF],samps_to_copy*sizeof(float));
			dz->sbufptr[IBUF] += samps_to_copy;
			dz->sbufptr[SPLBUF] += samps_to_copy;
			startsamp_in_buf = dz->lparray[CUT_STTSAMP][k] - *last_total_samps;
		}
		exit_status = CONTINUE;
	}
	dosplice(dz->sampbuf[SPLBUF],dz->iparam[CUT_SPLEN],dz->infile->channels,DOWNSLOPE);
	dz->sbufptr[SPLBUF] = dz->sampbuf[SPLBUF];
	return(exit_status);
}


/****************************** DO_EXCISE_ENDING ****************************/

int do_excise_ending(int *last_total_samps,int *obufleft,int *ibufleft,int k,dataptr dz)
{
	int exit_status;
	int overflow, remnant;
	int endsplice_in_buf, endsamp_in_buf;
	int samps_to_copy;

	dz->sbufptr[SPLBUF2] = dz->sampbuf[SPLBUF2];
	while(dz->total_samps_read < dz->lparray[CUT_ENDSAMP][k]) {
		*last_total_samps = dz->total_samps_read;
	    if((exit_status = read_samps(dz->sampbuf[IBUF],dz))<0)
			return(exit_status);
	}
	endsamp_in_buf   = dz->lparray[CUT_ENDSAMP][k]   - *last_total_samps;
	endsplice_in_buf = dz->lparray[CUT_ENDSPLI][k] - *last_total_samps;
	dz->sbufptr[IBUF] = dz->sampbuf[IBUF] + endsamp_in_buf;
	if((overflow = endsplice_in_buf - dz->buflen)<=0) {
		memcpy((char *)dz->sbufptr[SPLBUF2],(char *)dz->sbufptr[IBUF],
			dz->iparam[CUT_SPLEN] * sizeof(float));
		dz->sbufptr[IBUF] += dz->iparam[CUT_SPLEN];
		if(dz->sbufptr[IBUF] >= dz->sampbuf[OBUF]) {
			if(dz->samps_left<=0)
				return(FINISHED);
			else {
				*last_total_samps = dz->total_samps_read;
			    if((exit_status = read_samps(dz->sampbuf[IBUF],dz))<0)
					return(exit_status);
				dz->sbufptr[IBUF]  = dz->sampbuf[IBUF];
			}
		}
	} else {
		remnant = dz->iparam[CUT_SPLEN]-overflow;
		memcpy((char *)dz->sbufptr[SPLBUF2],(char *)dz->sbufptr[IBUF],remnant*sizeof(float));
		dz->sbufptr[SPLBUF2] += remnant;
		while(overflow > 0) {
			*last_total_samps = dz->total_samps_read;
			if((exit_status = read_samps(dz->sampbuf[IBUF],dz))<0)
				return(exit_status);
			dz->sbufptr[IBUF] = dz->sampbuf[IBUF];
			samps_to_copy = min(overflow,dz->buflen);
			overflow -= samps_to_copy;
			memcpy((char *)dz->sbufptr[SPLBUF2],(char *)dz->sbufptr[IBUF],samps_to_copy*sizeof(float));
			dz->sbufptr[IBUF] += overflow;
			dz->sbufptr[SPLBUF2] += samps_to_copy;
		}
	}
	crossplice(dz);
	dz->sbufptr[SPLBUF]  = dz->sampbuf[SPLBUF]; 
	dz->sbufptr[SPLBUF2] = dz->sampbuf[SPLBUF2]; 
	if((overflow = dz->iparam[CUT_SPLEN] - *obufleft)<=0) {
		memcpy((char *)dz->sbufptr[OBUF],(char *)dz->sbufptr[SPLBUF],
			dz->iparam[CUT_SPLEN] * sizeof(float));
		dz->sbufptr[OBUF] += dz->iparam[CUT_SPLEN];
	} else {
		memcpy((char *)dz->sbufptr[OBUF],(char *)dz->sbufptr[SPLBUF],(*obufleft)*sizeof(float));
		dz->sbufptr[SPLBUF] += *obufleft;
		while(overflow > 0) {
			if((exit_status = write_samps(dz->sampbuf[OBUF],dz->buflen,dz))<0)
				return(exit_status);
			dz->sbufptr[OBUF]  = dz->sampbuf[OBUF];
			samps_to_copy = min(overflow,dz->buflen);
			overflow -= samps_to_copy;
			memcpy((char *)dz->sbufptr[OBUF],(char *)dz->sbufptr[SPLBUF],samps_to_copy*sizeof(float));
// MAY 2010
//			dz->sbufptr[OBUF] += overflow;
			dz->sbufptr[OBUF] += samps_to_copy;
			dz->sbufptr[SPLBUF] += samps_to_copy;
		}
	}
	*obufleft = dz->sampbuf[SPLBUF] - dz->sbufptr[OBUF];
	*ibufleft = dz->ssampsread - (dz->sbufptr[IBUF] - dz->sampbuf[IBUF]);
	if(k < dz->iparam[EXCISE_CNT]-1)
		copy_excise_chunk(last_total_samps,obufleft,ibufleft,k+1,dz);
	return(CONTINUE);
}

/************************* WRITE_EXCISE_END *****************************/

int write_excise_end(int last_total_samps,int obufleft,int ibufleft,dataptr dz)
{
	int exit_status;
	int finished = 0, do_it = 0;
	while(!finished) {
		if     (obufleft == ibufleft) do_it = 0; 
		else if(obufleft  > ibufleft) do_it = 1;
		else if(obufleft  < ibufleft) do_it = 2;
		switch(do_it) {
		case(0):	/* EQUAL SPACE IN INPUT & OUTPUT BUFFERS */
			memcpy((char *)dz->sbufptr[OBUF],(char *)dz->sbufptr[IBUF],ibufleft * sizeof(float));
			dz->sbufptr[OBUF] += ibufleft;
			if(dz->sbufptr[OBUF] - dz->sampbuf[OBUF] > 0) {
				if((exit_status = 
				write_samps(dz->sampbuf[OBUF],(dz->sbufptr[OBUF] - dz->sampbuf[OBUF]),dz))<0)
					return(exit_status);
			}
			while(dz->samps_left) {
				last_total_samps = dz->total_samps_read;
				if((exit_status = read_samps(dz->sampbuf[IBUF],dz))<0)
					return(exit_status);
				if(dz->ssampsread > 0) {
					if((exit_status = write_samps(dz->sampbuf[IBUF],dz->ssampsread,dz))<0)
						return(exit_status);
				}
			}
			finished = 1;
			break;
		case(1):	/* MORE SPACE IN OUTPUT BUFFER */
			memcpy((char *)dz->sbufptr[OBUF],(char *)dz->sbufptr[IBUF],ibufleft * sizeof(float));
			dz->sbufptr[OBUF]  += ibufleft;
			obufleft -= ibufleft;
			if(dz->samps_left<=0) {
				if(dz->sbufptr[OBUF]-dz->sampbuf[OBUF] > 0) {
					if((exit_status = 
					write_samps(dz->sampbuf[OBUF],(dz->sbufptr[OBUF]-dz->sampbuf[OBUF]),dz))<0)
						return(exit_status);
				}
				finished = 1;
				break;
			}
			last_total_samps = dz->total_samps_read;
			if((exit_status = read_samps(dz->sampbuf[IBUF],dz))<0)
				return(exit_status);
			dz->sbufptr[IBUF]  = dz->sampbuf[IBUF];
			ibufleft = dz->ssampsread;
			break;
		case(2):	/* MORE SPACE IN INPUT BUFFER */
			memcpy((char *)dz->sbufptr[OBUF],(char *)dz->sbufptr[IBUF],obufleft * sizeof(float));
			dz->sbufptr[IBUF]  += obufleft;
			ibufleft -= obufleft;
			if((exit_status = write_samps(dz->sampbuf[OBUF],dz->buflen,dz))<0)
				return(exit_status);
			dz->sbufptr[OBUF]   = dz->sampbuf[OBUF];
			obufleft  = dz->buflen;
			break;
		}
	}
	return(FINISHED);
}


/************************* COPY_EXCISE_CHUNK *************************/ 

int copy_excise_chunk(int *last_total_samps,int *obufleft,int *ibufleft,int k,dataptr dz)
{
	int exit_status;
	int samps_to_copy = dz->lparray[CUT_STTSPLI][k] - (*last_total_samps + (dz->sbufptr[IBUF] - dz->sampbuf[IBUF]));
	while(samps_to_copy>0) {
		if((*obufleft > *ibufleft) && (samps_to_copy >= *ibufleft)) {
			memcpy((char *)dz->sbufptr[OBUF],(char *)dz->sbufptr[IBUF],(*ibufleft)*sizeof(float));
			samps_to_copy -= *ibufleft;
			*obufleft     -= *ibufleft;
			dz->sbufptr[OBUF]       += *ibufleft;
			*last_total_samps = dz->total_samps_read;
		    if((exit_status = read_samps(dz->sampbuf[IBUF],dz))<0)
				return(exit_status);
			dz->sbufptr[IBUF]   = dz->sampbuf[IBUF];					 
			*ibufleft  = dz->buflen;
		} else if((*ibufleft >= *obufleft) && (samps_to_copy >= *obufleft)) {
 			memcpy((char *)dz->sbufptr[OBUF],(char *)dz->sbufptr[IBUF],(*obufleft)*sizeof(float));
			if((exit_status = write_samps(dz->sampbuf[OBUF],dz->buflen,dz))<0)
				return(exit_status);
			samps_to_copy -= *obufleft;
			*ibufleft      -= *obufleft;
			dz->sbufptr[IBUF]       += *obufleft;
			dz->sbufptr[OBUF]   = dz->sampbuf[OBUF];
			*obufleft  = dz->buflen;
		} else {
			memcpy((char *)dz->sbufptr[OBUF],(char *)dz->sbufptr[IBUF],samps_to_copy*sizeof(float));
			dz->sbufptr[OBUF] += samps_to_copy;
			*obufleft -= samps_to_copy;
			dz->sbufptr[IBUF] += samps_to_copy;
			*ibufleft -= samps_to_copy;
			samps_to_copy = 0;
		}
	}
	return(FINISHED);
}

/******************************** DO_INSERT **********************************/

int do_insert(dataptr dz)
{
	int exit_status;
	int seektest;
	float	*cp;				/* current buffer pointer */
	int	ref, ssampsread;	/* reference value for insert loop */
	int		first = TRUE;		/* first is first read flag */
	int	buffxs2, samps_to_read, sects_to_read, samps_read,n, m, samps_in_buf, remainder, outsize = 0;
// TW 2010
	int	/* secs_to_write, */ samps_to_move, seek_samps = 0, seek_sects, samps_to_write;
	int 	c;
	short	shsecsize = F_SECSIZE;

	dz->total_samps_written = 0;	

	if(algo2) {
		while(dz->total_samps_written < dz->tempsize) {
			memset((char *)dz->bigbuf,0,dz->buflen * sizeof(float));
			if(dz->total_samps_read < dz->insams[0]) {
				if((exit_status = read_samps(dz->bigbuf,dz))<0)
					return(exit_status);
			}
			if((samps_to_write = min(dz->buflen,dz->tempsize - dz->total_samps_written)) > 0) {
				if((exit_status = write_samps(dz->bigbuf,samps_to_write,dz))<0)
					return(exit_status);
			}
		}
		return(FINISHED);
	}

	ref = dz->buflen - F_SECSIZE;

	/* READ FIRST PART OF INFILE, AND GET SPLICE SECTION */
	fprintf(stdout,"INFO: Reading first file\n");
	fflush(stdout);
	for(c = 1; c <= dz->iparam[CUT_BUFCNT]; c++) {	   /* ANY WHOLE BUFFERS COPIED */
		if((samps_read  = fgetfbufEx(dz->sampbuf[0], dz->buflen,dz->ifd[0],0))!=dz->buflen) {
			sprintf(errstr,"Bad sound read 7.\n");
			if(samps_read<0)
				return(SYSTEM_ERROR);
			return(PROGRAM_ERROR);
		}
//		if((samps_written = fputfbufEx(dz->sampbuf[0], dz->buflen,dz->ofd))!=samps_read) {
		if((exit_status = write_samps(dz->sampbuf[0],dz->buflen,dz))< 0)
			return(exit_status);
	}

	if(dz->iparam[CUT_SECCNT]) {	   	/* SPARE SECTORS BEFORE SPLICE */
		samps_to_read = dz->iparam[CUT_SECCNT] * F_SECSIZE;
		if((samps_read  = fgetfbufEx(dz->sampbuf[0],samps_to_read,dz->ifd[0],0))!=samps_to_read) {
			sprintf(errstr,"Bad sound read 8.\n");
			if(samps_read<0)
				return(SYSTEM_ERROR);
			return(PROGRAM_ERROR);
		}
		if(samps_read > 0) {
			if((exit_status =  write_samps(dz->sampbuf[0],samps_read,dz)) < 0)
				return(exit_status);
		}
	}

	if((samps_read  = fgetfbufEx(dz->sampbuf[0], dz->iparam[CUT_SECSREMAIN] * F_SECSIZE,dz->ifd[0],0))<=0) {
		if(samps_read<0) {
			sprintf(errstr,"Bad sound read 9.\n");
			return(SYSTEM_ERROR);
		} else if(dz->iparam[CUT_SECSREMAIN] > 0) {
			sprintf(errstr,"Bad sound read 9.\n");
			return(PROGRAM_ERROR);
		}
	}
	samps_to_move = (dz->iparam[CUT_SECSREMAIN] * F_SECSIZE) - dz->iparam[CUT_BUFXS];
	if(samps_to_move<0) {
		sprintf(errstr,"Arithmetic problem 1\n");
		return(PROGRAM_ERROR);
	}
	if(samps_to_move) {
		memcpy((char *)dz->sampbuf[SPLBUF],(char *)(dz->sampbuf[0] + dz->iparam[CUT_BUFXS]),
			samps_to_move * sizeof(float));
		if(!dz->vflag[INSERT_OVERWRITE])
			memcpy((char *)dz->sampbuf[SAVEBUF],(char *)dz->sampbuf[SPLBUF],
			   samps_to_move * sizeof(float));
	}
	if(dz->iparam[CUT_SPLEN] > 0) {
		dosplice(dz->sampbuf[SPLBUF], dz->iparam[CUT_SPLEN], dz->infile->channels, DOWNSLOPE);
		if(!dz->vflag[INSERT_OVERWRITE])
			dosplice(dz->sampbuf[SAVEBUF], dz->iparam[CUT_SPLEN], dz->infile->channels, UPSLOPE);
	}
	cp = dz->sampbuf[0] + dz->iparam[CUT_BUFXS];
	memset((char *)cp,0,(dz->buflen - dz->iparam[CUT_BUFXS]) * sizeof(float));
	if((dz->process == EDIT_INSERT2) || dz->vflag[INSERT_OVERWRITE]) {
		switch(dz->process) {
		case(EDIT_INSERT2):
			seek_samps = dz->iparam[CUT_END] - (2 * dz->iparam[CUT_SPLEN]);
			break;
		case(EDIT_INSERT):
			seek_samps = dz->iparam[CUT_CUT] + dz->insams[1] 
				- (2 * dz->iparam[CUT_SPLEN]);
			break;
		case(EDIT_INSERTSIL):
			if(silent_end)
				seek_samps = dz->iparam[CUT_CUT] - dz->iparam[CUT_SPLEN];
			else
				seek_samps = dz->iparam[CUT_CUT] 
				     + dz->iparam[CUT_LEN] - (2 * dz->iparam[CUT_SPLEN]);
			break;
		}
		seek_sects = seek_samps / F_SECSIZE;
		if((seektest = sndseekEx(dz->ifd[0],seek_sects * F_SECSIZE,0))!=seek_sects * F_SECSIZE) {
			if(seektest<0) {
				sprintf(errstr,"Seek failed.\n");
				return(SYSTEM_ERROR);
			} else {
				sprintf(errstr,"Seek problem 1 in insert().\n");
				return(PROGRAM_ERROR);
			}
		}
		buffxs2 = 	(seek_samps - (seek_sects * F_SECSIZE));

		samps_to_read = dz->iparam[CUT_SPLEN] + buffxs2 + F_SECSIZE;
		if(((sects_to_read = samps_to_read/F_SECSIZE) * F_SECSIZE)!= samps_to_read)
			sects_to_read++;
		samps_to_read = sects_to_read * F_SECSIZE;
		if((samps_read = fgetfbufEx(dz->sampbuf[SAVEBUF],samps_to_read,dz->ifd[0],0))<=0) {
			sprintf(errstr,"Bad sound read 10.\n");
			if(samps_read<0)
				return(SYSTEM_ERROR);
			return(PROGRAM_ERROR);
		}
		ssampsread = samps_read;
		if(buffxs2) {
			for(n=0,m = buffxs2;n<ssampsread - buffxs2;n++,m++)
				dz->sampbuf[SAVEBUF][n] = dz->sampbuf[SAVEBUF][m];
		}
		dz->iparam[CUT_BUFREMNANT] = ssampsread - (buffxs2 + dz->iparam[CUT_SPLEN]);
		dosplice(dz->sampbuf[SAVEBUF], dz->iparam[CUT_SPLEN], dz->infile->channels, UPSLOPE);
	}											

	switch(dz->process) {
	case(EDIT_INSERT):
	case(EDIT_INSERT2):
		fprintf(stdout,"INFO: Inserting file.\n");
		break;
	case(EDIT_INSERTSIL):
		fprintf(stdout,"INFO: Inserting silence\n");
		break;
	}
	fflush(stdout);
 	for(c = 1; c <= dz->iparam[CUT_BUFCNT2]; c++) {	   /* ANY WHOLE BUFFERS COPIED */	  
		switch(dz->process) {
		case(EDIT_INSERT):
		case(EDIT_INSERT2):
			if((samps_read  = fgetfbufEx(cp, dz->iparam[SMALLBUFSIZ],dz->ifd[1],0))!=dz->iparam[SMALLBUFSIZ]) {
				sprintf(errstr,"Bad sound read 11.\n");
				if(samps_read<0)
					return(SYSTEM_ERROR);
				return(PROGRAM_ERROR);
			}
			if(!flteq(dz->param[INSERT_LEVEL],1.0)) {
			 	if((exit_status = gain_insert_buffer(cp,dz->iparam[SMALLBUFSIZ],dz->param[INSERT_LEVEL]))<0)
					return(exit_status);
			}
			break;
		case(EDIT_INSERTSIL):
			memset((char *)cp,0, dz->iparam[SMALLBUFSIZ] * sizeof(float));
			break;
		}
		if(first) {
			first = FALSE;
			if(dz->iparam[CUT_SPLEN] > 0) {/* splice beginning of input */
				if(dz->process==EDIT_INSERT)
					dosplice(cp, dz->iparam[CUT_SPLEN], dz->infile->channels, UPSLOPE);
				for(n=0;n<dz->iparam[CUT_SPLEN];n++)
					cp[n] += dz->sampbuf[SPLBUF][n];   /* creates cross-splice */
			}
		}
		if(dz->iparam[SMALLBUFSIZ] > 0) {
			if((exit_status = write_samps(dz->sampbuf[0],dz->iparam[SMALLBUFSIZ],dz)) < 0)
				return(exit_status);
		}
		if(dz->iparam[CUT_BUFXS]) {	
			cp = dz->sampbuf[0] + ref;
			movmem((char *)cp, (char *)dz->sampbuf[0], dz->iparam[CUT_BUFXS] * sizeof(float));
			cp = dz->sampbuf[0] + dz->iparam[CUT_BUFXS];
		}
	}

	if(dz->iparam[CUT_SECCNT2]) {	   	/* SPARE SECTORS BEFORE END SPLICE */
		switch(dz->process) {
		case(EDIT_INSERT):
		case(EDIT_INSERT2):
	 		if((samps_read  = 
	 		fgetfbufEx(cp,  dz->iparam[CUT_SECCNT2] * F_SECSIZE,dz->ifd[1],0))!=dz->iparam[CUT_SECCNT2] * F_SECSIZE) {
				sprintf(errstr,"Bad sound read 12.\n");
				if(samps_read<0)
					return(SYSTEM_ERROR);
				return(PROGRAM_ERROR);
			}
			if(!flteq(dz->param[INSERT_LEVEL],1.0)) {
			 	if((exit_status = gain_insert_buffer(cp,dz->iparam[CUT_SECCNT2] * F_SECSIZE,dz->param[INSERT_LEVEL]))<0)
					return(exit_status);
			}
			break;
		case(EDIT_INSERTSIL):
			memset((char *)cp,0,  (dz->iparam[CUT_SECCNT2] * F_SECSIZE) * sizeof(float));
			break;
		}
		if(first) {
			first = FALSE;
			if(dz->iparam[CUT_SPLEN] > 0)  {/* splice beginning of input */
				if(dz->process==EDIT_INSERT)
					dosplice(cp, dz->iparam[CUT_SPLEN], dz->infile->channels, UPSLOPE);
				for(n=0;n<dz->iparam[CUT_SPLEN];n++)
					cp[n] += dz->sampbuf[SPLBUF][n];
			}
		}
		if(dz->iparam[CUT_SECCNT2] > 0) {	
			if((exit_status = write_samps(dz->sampbuf[0],dz->iparam[CUT_SECCNT2] * F_SECSIZE,dz)) < 0)
				return(exit_status);
		}
		if(dz->iparam[CUT_BUFXS]) {	
			cp = dz->sampbuf[0] + (dz->iparam[CUT_SECCNT2] * shsecsize);
			movmem((char *)cp, (char *)dz->sampbuf[0], 
				dz->iparam[CUT_BUFXS] * sizeof(float));
			cp = dz->sampbuf[0] + dz->iparam[CUT_BUFXS];
		}
	}
 	
	switch(dz->process) {
	case(EDIT_INSERT):
	case(EDIT_INSERT2):
		if((samps_read  = 
 		fgetfbufEx(cp, dz->iparam[CUT_SECSREMAIN2] * F_SECSIZE,dz->ifd[1],0))<=0) {
			sprintf(errstr,"Bad sound read 13.\n");
			if(samps_read<0)
				return(SYSTEM_ERROR);
			return(PROGRAM_ERROR);
		}
		if(!flteq(dz->param[INSERT_LEVEL],1.0)) {
		 	if((exit_status = gain_insert_buffer(cp,dz->iparam[CUT_SECSREMAIN2] * F_SECSIZE,dz->param[INSERT_LEVEL]))<0)
				return(exit_status);
		}
		break;
	case(EDIT_INSERTSIL):
		memset((char *)cp,0,(dz->iparam[CUT_SECSREMAIN2] * F_SECSIZE) * sizeof(float));
		break;
	}
	if(first) {	/* NO SECTORS BEFORE END SPLICE STARTS */

		if(dz->iparam[CUT_SPLEN]>0) { /* splice beginning of file */
			if(dz->process==EDIT_INSERT || dz->process==EDIT_INSERT2)
				dosplice(cp, dz->iparam[CUT_SPLEN], dz->infile->channels, UPSLOPE);
			for(n=0;n<dz->iparam[CUT_SPLEN];n++)
				cp[n] += dz->sampbuf[SPLBUF][n];
		}
	}
	cp = dz->sampbuf[0] + dz->iparam[CUT_BUFXS] + dz->iparam[CUT_SMPSREMAIN];
	if(dz->iparam[CUT_SPLEN]>0) {
		cp -= dz->iparam[CUT_SPLEN];
		if(dz->process==EDIT_INSERT || dz->process==EDIT_INSERT2)
			dosplice(cp, dz->iparam[CUT_SPLEN], dz->infile->channels, DOWNSLOPE);
		if(!silent_end) {
			for(n=0;n<dz->iparam[CUT_SPLEN];n++)
				cp[n] += dz->sampbuf[SAVEBUF][n];
		}
		cp += dz->iparam[CUT_SPLEN];
	}
	for(n=0,m=dz->iparam[CUT_SPLEN];n<dz->iparam[CUT_BUFREMNANT];n++,m++) {
		if(silent_end)
//			cp[n] = (short)0;
			cp[n] = (float)0;
		else
			cp[n] = dz->sampbuf[SAVEBUF][m];
	}
	cp += dz->iparam[CUT_BUFREMNANT];
	samps_in_buf = cp - dz->sampbuf[0];
	dz->iparam[CUT_SECCNT] = samps_in_buf/shsecsize;
	dz->iparam[CUT_BUFXS]  = samps_in_buf - (dz->iparam[CUT_SECCNT] * shsecsize);
	if(dz->iparam[CUT_SECCNT] > 0) {
		if((exit_status = write_samps(dz->sampbuf[0],dz->iparam[CUT_SECCNT] * F_SECSIZE,dz)) < 0)
			return(exit_status);
	}
	cp = dz->sampbuf[0] + (dz->iparam[CUT_SECCNT] * shsecsize);
	movmem((char *)cp, (char *)dz->sampbuf[0], 
		dz->iparam[CUT_BUFXS] * sizeof(float));
	cp = dz->sampbuf[0] + dz->iparam[CUT_BUFXS];
	remainder = dz->iparam[CUT_BUFXS];
	
	fprintf(stdout,"INFO: Reading remainder of infile.\n");
	fflush(stdout);
	while((samps_read = fgetfbufEx(cp, dz->iparam[SMALLBUFSIZ],dz->ifd[0],0)) > 0) {
		samps_to_write = min(dz->iparam[SMALLBUFSIZ],samps_read + dz->iparam[CUT_BUFXS]);
		remainder      = samps_read - ((dz->iparam[SMALLBUFSIZ]) - dz->iparam[CUT_BUFXS]);
		if(silent_end)
			memset((char *)dz->sampbuf[0],0,dz->buflen * sizeof(float));
		if(samps_to_write > 0) {
			if((exit_status = write_samps(dz->sampbuf[0],samps_to_write,dz)) < 0)
				return(exit_status);
		}
		if(remainder > 0) {
			cp = dz->sampbuf[0] + ref;
 			movmem((char *)cp, (char *)dz->sampbuf[0], remainder * sizeof(float));
			cp = dz->sampbuf[0] + dz->iparam[CUT_BUFXS];
		}
	}
	if(samps_read<0) {
		sprintf(errstr,"Bad sound read.\n");
		return(SYSTEM_ERROR);
	}
	if(remainder > 0) {
		if((exit_status = write_samps(dz->sampbuf[0],remainder,dz))<=0)
			return(exit_status);
	}				  
	
	if (dz->process == EDIT_INSERT2)
		outsize = dz->insams[0] + dz->insams[1] - dz->iparam[CUT_SPLEN];
	else if(dz->vflag[INSERT_OVERWRITE])
		outsize = dz->insams[0];
	else {
		switch(dz->process) {
		case(EDIT_INSERT):
			outsize = dz->insams[0] + dz->insams[1] - dz->iparam[CUT_SPLEN];
			break;
		case(EDIT_INSERTSIL):
			outsize = dz->insams[0] + dz->iparam[CUT_LEN] 
				- dz->iparam[CUT_SPLEN];
			break;
		}
	}
	dz->total_samps_written = outsize; /* for truncation calculations */
   	return(FINISHED);
}


/******************************** GAIN_INSERT_BUFFER ********************************/ 

int gain_insert_buffer(float *thisbuf,int size,double gain)
{
	int n;
	double d;
	if(gain > 1.0) {
		for(n=0;n < size; n++) {
			if(fabs(d = (double)thisbuf[n] * gain) > (double)F_MAXSAMP) {
				sprintf(errstr,"gain has caused insertfile to distort.\n");
				return(DATA_ERROR);
			}
			thisbuf[n] = (float) /*round*/(d);
		}
	} else {
		for(n=0;n < size; n++)
			thisbuf[n] = (float) /*round*/((double)thisbuf[n] * gain);
	}
	return(FINISHED);
}

/******************************** SETUP_EXCISE_OUTDISPLAY_LEN ********************************/ 

void setup_excise_outdisplay_len(dataptr dz)
{
	int n;
	dz->tempsize = dz->insams[0];
	for(n=0;n<dz->iparam[EXCISE_CNT]-1;n++)
		dz->tempsize -= dz->lparray[CUT_ENDSAMP][n] - dz->lparray[CUT_STTSAMP][n];
	if(dz->iparam[CUT_NO_END])
		dz->tempsize -= dz->insams[0] - dz->lparray[CUT_STTSAMP][n];
	else
		dz->tempsize -= dz->lparray[CUT_ENDSAMP][n] - dz->lparray[CUT_STTSAMP][n];
	if(dz->tempsize<=0)
		dz->tempsize = 1;	/* trap later divides by zero !! */
}

/************************** JOIN_PCONSISTENCY ***************************/

int join_pconsistency(dataptr dz)
{
	int n, m;
	int stsplen;
	double *p, *q;
	double sr = (double)dz->infile->srate;
	int chans = dz->infile->channels;
		 
	stsplen = round(dz->param[CUT_SPLEN] * MS_TO_SECS * sr);
	dz->iparam[CUT_SPLEN]     = stsplen * chans;

	if((dz->parray[SPLICE_UP] = (double *)malloc(stsplen * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICENT MEMORY for first splice table.\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[SPLICE_DN] = (double *)malloc(stsplen * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICENT MEMORY for 2nd splice table.\n");
		return(MEMORY_ERROR);
	}
	p = dz->parray[SPLICE_UP];
	q = dz->parray[SPLICE_DN];
	for(n=0;n<stsplen;n++) {
		*p++ = (double)n/(double)stsplen;    
		*q++ = (double)(stsplen-n)/(double)stsplen;    
	}
	if(dz->insams[0] < 2 * dz->iparam[CUT_SPLEN]) {
		sprintf(errstr,"File 1 too short for specified spliclength.\n");
		return(DATA_ERROR);
	}	
	dz->tempsize = dz->insams[0];
	switch(dz->process) {
	case(EDIT_JOIN):
		dz->tempsize = dz->insams[0];
		for(n=1;n<dz->infilecnt;n++) {
			if(dz->insams[n] < 2 * dz->iparam[CUT_SPLEN]) {
				sprintf(errstr,"File %d too short for specified spliclength.\n",n+1);
				return(DATA_ERROR);
			}	
			dz->tempsize += dz->insams[n] - dz->iparam[CUT_SPLEN];
		}
		break;
	case(JOIN_SEQDYN):
		dz->tempsize = 0;
		for(n=0;n<dz->itemcnt;n++) {
			m = dz->iparray[0][n];
			if(dz->insams[m] < 2 * dz->iparam[CUT_SPLEN]) {
				sprintf(errstr,"File %d too short for specified splicelength.\n",m+1);
				return(DATA_ERROR);
			}	
			dz->tempsize += dz->insams[m] - dz->iparam[CUT_SPLEN];
		}
		break;
	case(JOIN_SEQ):
		dz->tempsize = 0;
		for(n=0;n<dz->itemcnt;n++) {
			if(n >= dz->iparam[MAX_LEN])
				break;
			m = dz->iparray[0][n];
			if(dz->insams[m] < 2 * dz->iparam[CUT_SPLEN]) {
				sprintf(errstr,"File %d too short for specified splicelength.\n",m+1);
				return(DATA_ERROR);
			}	
			dz->tempsize += dz->insams[m] - dz->iparam[CUT_SPLEN];
		}
		break;
	}

	return(FINISHED);
}

/************************** CREATE_JOIN_BUFFER ***************************/

int create_join_buffer(dataptr dz)
{
	int seccnt;
	size_t bigbufsize;
	int framesize = F_SECSIZE * dz->infile->channels;
	int fl_secsize = F_SECSIZE * sizeof(float);
	int splicelen = dz->iparam[CUT_SPLEN] * dz->infile->channels;

	bigbufsize = (size_t) Malloc(-1);
	bigbufsize = (bigbufsize/fl_secsize) * fl_secsize;
	bigbufsize -= fl_secsize;

	dz->buflen = (int)(bigbufsize/sizeof(float));
	dz->buflen = (dz->buflen/framesize) * framesize; 
	if(dz->buflen <= 0)
		dz->buflen = framesize;
	if(splicelen > dz->buflen - F_SECSIZE) {
		dz->buflen = splicelen;
		if(((seccnt = dz->buflen/framesize) * framesize) < dz->buflen)
			seccnt++;
		dz->buflen = seccnt * framesize;
	}
	if(dz->buflen<=0
	|| (dz->bigbuf = (float *)malloc(sizeof(float) * (dz->buflen + F_SECSIZE 
		   + splicelen)))==NULL) {
		sprintf(errstr, "INSUFFICIENT MEMORY for sounds.\n");
		return(MEMORY_ERROR);
	}
	dz->sampbuf[BBUF] = dz->bigbuf;
	dz->sampbuf[BUFEND] = dz->sampbuf[BBUF] + dz->buflen;
	dz->sampbuf[SPLICEBUF] = dz->sampbuf[BUFEND] + F_SECSIZE;
	return(FINISHED);
}

/************************** CREATE_JOIN_SEQ_BUFFER ***************************/

int create_join_seq_buffer(dataptr dz)
{
	int seccnt, OK = 0;
	int last_buflen = 0;
	int framesize = F_SECSIZE * dz->infile->channels;
	int fl_secsize = F_SECSIZE * sizeof(float);
	size_t bigbufsize = (size_t) Malloc(-1);

	bigbufsize = (bigbufsize/fl_secsize) * fl_secsize;
	bigbufsize -= fl_secsize;						/* Leave F_SECSIZE available for malloc */

	dz->buflen = (int)(bigbufsize/sizeof(float));
	dz->buflen = (dz->buflen/framesize) * framesize; 
	if(dz->buflen <= 0)
		dz->buflen = framesize;

	while(!OK) {	
		if(dz->buflen <= last_buflen) {
			sprintf(errstr, "Too many channels to cope with.\n");
			return(MEMORY_ERROR);
	    }
		if(dz->buflen<=0
	    || (dz->bigbuf = (float *)malloc(sizeof(float) * ((dz->buflen * 2) + F_SECSIZE 
		       + dz->iparam[CUT_SPLEN])))==NULL) {
			sprintf(errstr, "INSUFFICIENT MEMORY for sounds.\n");
			return(MEMORY_ERROR);
	    }
		last_buflen = dz->buflen;
	 	dz->sampbuf[COPYBUF] = dz->bigbuf;
	 	dz->bigbuf += dz->buflen;
	 	dz->sampbuf[BBUF] = dz->bigbuf;
	    dz->sampbuf[BUFEND] = dz->sampbuf[BBUF] + dz->buflen;
		dz->sampbuf[SPLICEBUF] = dz->sampbuf[BUFEND] + F_SECSIZE;
	    if(dz->iparam[CUT_SPLEN] <= dz->buflen - F_SECSIZE)	/* splice must fit in buffer */
			OK = 1;
		else {
			dz->buflen = dz->iparam[CUT_SPLEN];
			if(((seccnt = dz->buflen/framesize) * framesize) < dz->buflen)
				seccnt++;
			dz->buflen = seccnt * framesize;
		}
	}
	return(FINISHED);
}

/*************************** DO_JOINING *****************************
 *
 * (A)	Sets special loop condition for zero splice length, see (B).
 * (0)	For every input file.
 * (1)	Set 'samps_to_get' to equal size of file MINUS number of samps
 *	we will use in end-of0-file splice.
 * (2)	Set the total_fsamps_read to zero..
 * (3)	and the number of reads OF THIS FILE to zero.
 * (4)	NORMAL CASE lmt = 0.
 	Read the file. Note the number of samps read. Add this to the total
 *	number of samps read from this file.
 *	If the TOTAL number of samps read, EXCEEDS 'samps_to_get' then we
 *	have read some samps in the END SPLICE REGION. The number of those
 *	samps is 'overspill'.
 * 	  If we have not read any samps in END SPLICE REGION...
 *	  (i.e. we cannot be at the end of the file, and hence we must have
 *	  read a FULL BUFFER!!!).
 * (B)	ZERO SPLICE LENGTH CASE, LMT = -1.
 *	  In this case, an overspill of zero just tells us that the
 *	  entire block has been read .. no special adjustments need
 *	  to be made for splice overlaps, so we can drop out of
 *	  the while loop when we get ZERO overspill in this case.
 *   (5)  If this is the first read of this file, then the START-SPLICE must
 *	  be in the buffer. So do the start-splice IN SITU.
 *   (6)  As the END_SPLICE is not in the buffer, and the buffer is full,
 *	  we can WRITE the whole existing buffer to the output.
 *   (7)  Increment the count of reads OF THIS FILE.
 * (8)  If we have left the read_loop for the current file, with readcnt
 *	remaining at zero, the start of the current file must be in the
 *	current buffer.
 *	We can therefore do the start_splice IN SITU in the current buffer.
 * (9)	Housekeep1
 *	   (a)	finds the address of the start of end_splice
 *	   (b) 	copies whatever part of the splice is in the buffer, into
 *		the splice buffer.
 * (10)	If not all of the splice is in this buffer (splice_remnant>0)...
 *	   (a)	Reads rest of splice directly to splice buffer
 *	   (b)	finds the size and address of any part-sector existing
 *		just prior to the splice-area.
 *	   (c)	Writes the whole buufer up to start of this part-sector..
 *	   (d)	copies part_sector to start of buffer
 *	   (e)  resets readbuf address and size of readbuffer.
 * (11) Otherwise, reset the read-place in buffer, and the (remaining) size
 *	of the read_buffer.
 * (12) On leaving loop, do any splice required at end of file.
 * (13)	Flush buffer.
 */

int do_joining(dataptr dz)
{
	int  exit_status;
	int total_fsamps_read, samps_to_get, samps_read, samps_to_write;
	int	 n, readcnt, lmt = 0;
	int  overspill, splice_remnant;
	dz->iparam[CUT_SMPSREMAIN] = dz->buflen;
	dz->sampbuf[READBUF]       = dz->sampbuf[BBUF];
	if(dz->iparam[CUT_SPLEN]==0)
		lmt = -1;									/* A */
	for(n=0;n<dz->infilecnt;n++) {					/* 0 */
		fprintf(stdout,"INFO: Processing FILE %d\n",n+1);
		fflush(stdout);
		if((samps_to_get = dz->insams[n] - dz->iparam[CUT_SPLEN])<=0) {
			sprintf(errstr,"File %d is too short for the given splicelength\n",n+1);
			return(GOAL_FAILED);
		}
																				/* 1 */
		total_fsamps_read = 0;													/* 2 */
		readcnt = 0;															/* 3 */
		for(;;) {
			if((samps_read = fgetfbufEx(dz->sampbuf[READBUF],
				     dz->iparam[CUT_SMPSREMAIN],dz->ifd[n],0))<0)  {
				sprintf(errstr, "Can't read from input soundfile\n");
				return(SYSTEM_ERROR);
			}
			overspill = (total_fsamps_read += samps_read) - samps_to_get;
			if(overspill > lmt)
				break;
			if(dz->iparam[CUT_SPLEN]>0 && readcnt==0)							/* B */
				do_join_startsplice(n,dz);										/* 5 */
			if((exit_status = write_exact_samps(dz->sampbuf[BBUF],dz->buflen,dz))<0)	
				return(exit_status);											/* 6 */
			housekeep2(samps_read,dz);
			readcnt++;															/* 7 */
		}
		if(dz->iparam[CUT_SPLEN]>0) {
			if(readcnt==0)														/* 8 */
				do_join_startsplice(n,dz);
			housekeep1(samps_read,overspill,dz);								/* 9 */
			if((splice_remnant = dz->iparam[CUT_SPLEN] - overspill)>0)	{	/* 10 */
				if((exit_status = do_join_write(n,splice_remnant,(int)(overspill),dz))<0)
					return(exit_status);
			} else {
				if((exit_status = reset_join_buffer_params(dz))<0)				/* 11 */
					return(exit_status);
			}
		} else {
			dz->sbufptr[ENDSPLICE_ADDR] = dz->sampbuf[READBUF] + samps_read;
			if((exit_status = reset_join_buffer_params(dz))<0)
				return(exit_status);
		}
		if(sndcloseEx(dz->ifd[n]) < 0) {
		    fprintf(stdout, "WARNING: Can't close input soundfile %d\n",n+1);
			fflush(stdout);
		}
		dz->ifd[n] = -1;
	}
	if(dz->iparam[CUT_SPLEN]>0) {
		if(dz->vflag[SPLICE_END])												/* 12 */
			do_join_endsplice(dz);
		memcpy((char *)dz->sampbuf[READBUF],(char *)dz->sampbuf[SPLICEBUF],
			dz->iparam[CUT_SPLEN] * sizeof(float));
	}		
	if((samps_to_write = dz->sampbuf[READBUF] + dz->iparam[CUT_SPLEN] - dz->sampbuf[BBUF])>0)
		return write_samps(dz->sampbuf[BBUF],samps_to_write,dz);				/* 13 */
	return FINISHED;
}

/************************ READ_JOIN_SPLICE_REMNANT ********************************
 *
 * Reads any EXTRA bit of the splice NOT captured in main buffer.
 */

int read_join_splice_remnant(int n,int splice_remnant,int oversamps,dataptr dz)
{
	int toread = splice_remnant/F_SECSIZE, samps_read;
	if((toread * F_SECSIZE) != splice_remnant)
		toread += 1;
	toread *= F_SECSIZE;
     	/* MUST ASK FOR WHOLE NO OF SECTORS : AND THIS MUST BE >= NO OF SAMPS WANTED */
	if((samps_read = fgetfbufEx(dz->sampbuf[SPLICEBUF] + oversamps,toread,dz->ifd[n],0))<0) {
		sprintf(errstr,"Can't read samples for splice from input soundfile.\n");
		return(SYSTEM_ERROR);
	}
	if(samps_read != splice_remnant) {
		sprintf(errstr,"Problem reading part splice-buffer.\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/********************************* DO_JOIN_STARTSPLICE ************************/

void do_join_startsplice(int k,dataptr dz)
{   
	int n;
	int m;
	float *a = dz->sampbuf[READBUF];
	float *b = dz->sampbuf[SPLICEBUF];
	int chans = dz->infile->channels;
	double *p = dz->parray[SPLICE_UP];
	double *q = dz->parray[SPLICE_DN];
    a = dz->sampbuf[READBUF];
	if(k==0)
		do_join_initial_splice(chans,dz);
	else {
		for(n=0;n<dz->iparam[CUT_SPLEN];n+=chans) {
			for(m=0;m<chans;m++) {
				*a= (float) /*round*/((*q * (double)(*b)) + (*p * (double)(*a)));
				a++;
				b++;
			}
			p++;
			q++;
		}
	}
}

/************************** DO_JOIN_INITIAL_SPLICE ********************************/

void do_join_initial_splice(int chans,dataptr dz)
{   
	int n;
	int m;
	float *a = dz->sampbuf[BBUF];
	double *p = dz->parray[SPLICE_UP];
	if(dz->vflag[SPLICE_START]) {
		for(n=0;n<dz->iparam[CUT_SPLEN];n+=chans) {
			for(m=0;m<chans;m++) {
				*a = (float) /*round*/(*p  * (double)(*a));
				a++;
			}
			p++;
		}
	}
}

/*********************************** DO_JOIN_ENDSPLICE **************************
 *
 * Put a fade on data in splice_buffer.
 */

void do_join_endsplice(dataptr dz)
{   
	int n;
	int m, chans = dz->infile->channels;
	float *b = dz->sampbuf[SPLICEBUF];
    double *q = dz->parray[SPLICE_DN];
    for(n=0;n<dz->iparam[CUT_SPLEN];n+=chans) {
		for(m=0;m<chans;m++) {
			*b = (float) /*round*/(*q  * (double)(*b));
			b++;
		}
		q++;
    }
}

/************************* HOUSEKEEP1 *******************************
 *
 * (1)	'body' is the number of samps of the current file, in the
 *	current buffer, NOT COUNTING the splice portion.
 * (2)  ENDSPLICE_ADDR, is offset from the address where we
 * 	last read into this buffer, by 'body'.
 * (3)	Copy whatever part of the end-splice region is in this buffer
 *	into the splice-buffer.
 */

void housekeep1(int samps_read,int overspill,dataptr dz)
{   
	int body = (samps_read - overspill);										/* 1 */
	dz->sbufptr[ENDSPLICE_ADDR] = dz->sampbuf[READBUF] + body;								/* 2 */
	memcpy((char *)dz->sampbuf[SPLICEBUF],(char *)dz->sbufptr[ENDSPLICE_ADDR],overspill * sizeof(float));	/* 3 */
}

/************************* HOUSEKEEP2 *******************************
 *
 * (1)	Find end of final write into this buffer.
 * (2)	Find size of part-sector written beyond ACTUAL (read) buffer.
 * (3)	Copy part-sector to start of ACTUAL buffer.
 * (4) 	Reset start of readbuf to end of this part-sector.
 * (5) 	Set size of readbuffer to full-size (we have an extra sector for
 *	overrun at end!!).
 */

void housekeep2(int samps_read,dataptr dz)
{   
	float *write_end = dz->sampbuf[READBUF] + samps_read;							/* 1 */
    int  part_sector_size = write_end - dz->sampbuf[BUFEND];										/* 2 */
    memcpy((char *)dz->sampbuf[BBUF],(char *)dz->sampbuf[BUFEND],part_sector_size * sizeof(float));	/* 3 */
    dz->sampbuf[READBUF]       = dz->sampbuf[BBUF] + part_sector_size;								/* 4 */
    dz->iparam[CUT_SMPSREMAIN] = dz->buflen;													/* 5 */
}

/******************************* DO_JOIN_WRITE **********************************
 *
 * (1)	Read the unread PART of the splice directly into the end of the
 *	splice_buffer.
 * (2)	The distance of the start of this end-splice from the start of 
 *	the ACTUAL buffer is given by (ENDSPLICE_ADDR - buffer).
 * (3)	This distance in SECTORS (truncated).
 * (4)	This truncated distance in samps.
 * (5)	The size of any part_sector preceding the splice-start.
 * (6)	The address of this part_sector.
 * (7)	Write the existing buffer, up as far as the last complete sector
 *	before the splice sector begins.
 * (8)	Copy any incomplete sector (prior to the splice) to the start
 *	of the buffer.
 * (9)	Set the reading-in point (readbuf) to the end of that part_sector.
 * (10)	Set the space remaining in the buffer (dz->iparam[CUT_SAMPREMAIN]) to the
 *	total buffersize (bear in mind that there is a guard SECSIZE
 *	of address space at the end, to allow for the part_sectors
 *	inserted at the start of the buffer).
 */

int do_join_write(int n,int splice_remnant,int oversamps,dataptr dz)
{
	int exit_status;
	int shsecsize = F_SECSIZE;			   	
    int samps_from_buf_start,secdist,sector_sampdist,part_sector_size;
    float *part_sector_addr;
	if((exit_status = read_join_splice_remnant(n,splice_remnant,oversamps,dz))<0)	
		return(exit_status);												/* 1 */
    samps_from_buf_start = dz->sbufptr[ENDSPLICE_ADDR] - dz->sampbuf[BBUF];	/* 2 */
    secdist  	     	 = samps_from_buf_start/shsecsize;					/* 3 */
    sector_sampdist	  	 = secdist * shsecsize;								/* 4 */
    part_sector_size     = (samps_from_buf_start - sector_sampdist);		/* 5 */
    part_sector_addr     = dz->sampbuf[BBUF] + sector_sampdist;				/* 6 */
	if(sector_sampdist) {
    	if((exit_status = write_samps(dz->sampbuf[BBUF],sector_sampdist,dz))<0)
    		return(exit_status);											/* 7 */
	}
	if(part_sector_size && (dz->sampbuf[BBUF]!=part_sector_addr))			/* 8 */
		memcpy((char *)dz->sampbuf[BBUF],(char *)part_sector_addr,part_sector_size * sizeof(float));		
    dz->sampbuf[READBUF]   = dz->sampbuf[BBUF] + (part_sector_size);		/* 9 */
// SEPT 2010
	dz->iparam[CUT_SMPSREMAIN] = dz->buflen;							/* 10 */
	return(FINISHED);
}


/*********************** RESET_JOIN_BUFFER_PARAMS ****************************
 *
 * (1)	Set the next read-in address to be at the START of the
 *	current file's end-splice area. Note that the end-splice
 *	samps have been saved to the splice-buffer. The start-splice
 *	samps of the new file will be written over the end-splice values
 *	and the splice then created IN SITU.
 * (2)	Check how many complete sectors remain in the ACTUAL buffer.
 * (3)	Convert this to a sampcount.
 * (4)	Unless there was an exact number of sectors, add one extra sector
 *	of samps. This ensures that the reads will write to the end of OR
 *	beyond the end of the ACTUAL buffer, so that WRITES will be
 *	valid. This is safe because we have allowed an extra sector
 *	at the end of the buffer.
 */

int reset_join_buffer_params(dataptr dz)
{   
	int shsecsize = F_SECSIZE;
	int bufcheck;
    dz->sampbuf[READBUF]  = dz->sbufptr[ENDSPLICE_ADDR];						/* 1 */
    if((bufcheck = dz->sampbuf[BUFEND] - dz->sbufptr[ENDSPLICE_ADDR])==0) {		/* 2 */
		dz->iparam[CUT_SMPSREMAIN]  =  dz->buflen;
		dz->sbufptr[ENDSPLICE_ADDR] = dz->sampbuf[BBUF];
		dz->sampbuf[READBUF]        = dz->sampbuf[BBUF];
	} else {
		dz->iparam[CUT_SMPSREMAIN] =  bufcheck/shsecsize;
			dz->iparam[CUT_SMPSREMAIN] *= F_SECSIZE;								/* 3 */
		if(bufcheck != dz->iparam[CUT_SMPSREMAIN])		
			dz->iparam[CUT_SMPSREMAIN] += F_SECSIZE;								/* 4 */
		if(dz->iparam[CUT_SMPSREMAIN] < F_SECSIZE) {								/* 5 */
			sprintf(errstr,"Zero buffer sectorsize: impossible!?: reset_join_buffer_params()\n");
			return(PROGRAM_ERROR);
		}
	}
	return(FINISHED);
}


/******************************** DO_INSERTSIL_MANY **********************************/

int do_insertsil_many(dataptr dz)
{
	int exit_status;
	int last_total_samps_read, endsamp, startsamp, inbuf_location;
	int chans = dz->infile->channels;
	int here = 0;
	int here_in_splice = 0;
	int in_downsplice = 0;
	int in_upsplice = 0;
	int in_zero = 0;
	int splicelen = dz->iparam[CUT_SPLEN];
	dz->total_samps_read = 0;	

	while(dz->total_samps_read < dz->insams[0]) {
		last_total_samps_read = dz->total_samps_read;
		if((exit_status = read_samps(dz->bigbuf,dz))<0)
			return(exit_status);
		if (here >= dz->iparam[EXCISE_CNT]) {
			if(dz->ssampsread > 0) {
				if((exit_status = write_samps(dz->bigbuf,dz->ssampsread,dz))<0)
		   			return(exit_status);											/* 7 */
			}
			continue;
		}
		if (in_downsplice) {
			endsamp = dz->lparray[CUT_STTSAMP][here] - last_total_samps_read;
			if(endsamp <= dz->buflen) {
				do_other_splice(dz->bigbuf,splicelen,here_in_splice,splicelen,chans,1);
				in_downsplice  = 0;
				here_in_splice = 0;
			} else {
				do_other_splice(dz->bigbuf,splicelen,here_in_splice,here_in_splice + dz->buflen,chans,1);
				here_in_splice += dz->buflen;
				if(dz->ssampsread > 0) {
		    		if((exit_status = write_samps(dz->bigbuf,dz->ssampsread,dz))<0)
		    			return(exit_status);											/* 7 */
				}
				continue;
			}
		} else if (in_zero) {
			endsamp = dz->lparray[CUT_ENDSAMP][here] - last_total_samps_read;
			if(endsamp <= dz->buflen) {
				memset((char *)dz->bigbuf,0,endsamp * sizeof(float));
				in_zero = 0;
			} else {

				memset((char *)dz->bigbuf,0,dz->buflen * sizeof(float));
				if(dz->ssampsread > 0) {
					if((exit_status = write_samps(dz->bigbuf,dz->ssampsread,dz))<0)
		    			return(exit_status);											/* 7 */
				}
				continue;
			}
		} else if (in_upsplice) {
			endsamp = dz->lparray[CUT_ENDSPLI][here] - last_total_samps_read;
			if(endsamp <= dz->buflen) {
				do_other_splice(dz->bigbuf,splicelen,here_in_splice,splicelen,chans,0);
				in_upsplice  = 0;
				here_in_splice = 0;
				here++;				/* To next set of values */
			} else {
				do_other_splice(dz->bigbuf,splicelen,here_in_splice,here_in_splice + dz->buflen,chans,0);
				here_in_splice += dz->buflen;
				if(dz->ssampsread > 0) {
		    		if((exit_status = write_samps(dz->bigbuf,dz->ssampsread,dz))<0)
		    			return(exit_status);											/* 7 */
				}
				continue;
			}
		}
		while(here < dz->iparam[EXCISE_CNT]) {
			if(dz->lparray[CUT_STTSPLI][here] >= dz->total_samps_read) {
				break;
			}
			inbuf_location =  dz->lparray[CUT_STTSPLI][here] - last_total_samps_read;
			if(inbuf_location >= 0 && inbuf_location < dz->ssampsread) {
				startsamp = dz->lparray[CUT_STTSPLI][here] % dz->buflen;
				if(dz->lparray[CUT_STTSAMP][here] < dz->total_samps_read) {
					endsamp = dz->lparray[CUT_STTSAMP][here] % dz->buflen;
					do_other_splice((dz->bigbuf + startsamp),splicelen,0,splicelen,chans,1);
					here_in_splice = 0;
					in_downsplice = 0;
				} else {
					endsamp = dz->buflen;
					here_in_splice = endsamp - startsamp;
					do_other_splice((dz->bigbuf + startsamp),splicelen,0,here_in_splice,chans,1);
					in_downsplice = 1;
					break;
				}
			}
			inbuf_location =  dz->lparray[CUT_STTSAMP][here] - last_total_samps_read;
			if(inbuf_location >= 0 && inbuf_location < dz->ssampsread) {
				startsamp = dz->lparray[CUT_STTSAMP][here] % dz->buflen;
				if(dz->lparray[CUT_ENDSAMP][here] < dz->total_samps_read) {
					endsamp = dz->lparray[CUT_ENDSAMP][here] % dz->buflen;
					memset((char *)(dz->bigbuf + startsamp),0,(endsamp - startsamp) * sizeof(float));
					in_zero = 0;
				} else {
					endsamp = dz->buflen;
					memset((char *)(dz->bigbuf + startsamp),0,(endsamp - startsamp) * sizeof(float));
					in_zero = 1;
					break;
				}
			}
			inbuf_location =  dz->lparray[CUT_ENDSAMP][here] - last_total_samps_read;
			if(inbuf_location >= 0 && inbuf_location < dz->ssampsread) {
				startsamp = dz->lparray[CUT_ENDSAMP][here] % dz->buflen;
				if(dz->lparray[CUT_ENDSPLI][here] < dz->total_samps_read) {
					endsamp = dz->lparray[CUT_ENDSPLI][here] % dz->buflen;
					do_other_splice((dz->bigbuf + startsamp),splicelen,0,splicelen,chans,0);
					here_in_splice = 0;
					in_upsplice = 0;
				} else {
					endsamp = dz->buflen;
					here_in_splice = endsamp - startsamp;
					do_other_splice((dz->bigbuf + startsamp),splicelen,0,here_in_splice,chans,0);
					in_upsplice = 1;
					break;
				}
			}
			here++;
		}
 		if(dz->ssampsread > 0) {
	   		if((exit_status = write_samps(dz->bigbuf,dz->ssampsread,dz))<0)
    			return(exit_status);											/* 7 */
		}
	}
	return(FINISHED);
}

void do_other_splice(float *buf, int splicelen, int start_in_splice, int end_in_splice, int chans, int invert)
{
	double	a1 = 0.0, aincr;
	register int i, j, k;
	int sampgrps = splicelen/chans;
	aincr = 1.0/sampgrps;
	if(invert) {
		aincr = -aincr;
		a1 = 1.0 + aincr;
	}
	a1 += (aincr * start_in_splice);
	for(i = 0, k = start_in_splice; k < end_in_splice; i+= chans, k += chans) {
		for(j=0;j<chans;j++) 
			buf[i+j] = (float)((double)buf[i+j] * a1);
		a1 += aincr;
	}
}


/**************************** DO_RANDCUTS ****************************/

int do_randcuts(dataptr dz)
{
//	int bigbufsize;
	int exit_status, namelen, cutendno;
	float *obuf;
	char *outfilename;
	int rel_bufend, rel_bufstart, abs_bufend, abs_bufstart, /*origbufsize*/origbuflen;
	int rel_cutend, last_cut, next_cut, cutlen;
	int chans = dz->infile->channels;
	int stereo_splicelen = SHRED_SPLICELEN * chans;
	int part_splice, splice_start, samps_to_write;

	if((exit_status = get_lengths(stereo_splicelen,dz))<0)
		return(PROGRAM_ERROR);

	namelen = strlen(dz->wordstor[0]);		
	if(sloom)
		namelen--;								/* Drop the 0 at end of name */
//FEB 2010 TW
	if((outfilename = (char *)malloc((namelen + 6 + 10) * sizeof(char)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for outfilename.\n");
		return(MEMORY_ERROR);
	}				
	strcpy(outfilename,dz->wordstor[0]);
//FEB 2010 TW
	if(!sloom)
		insert_separator_on_sndfile_name(outfilename,2);
	cutendno = 0;
	obuf = dz->sampbuf[0];
	rel_bufend = dz->buflen;	
	abs_bufend = dz->buflen;	
	rel_bufstart = 0;

	/*origbufsize = dz->bigbufsize;	*/		/*	READ A DOUBLE BUFFER */
	origbuflen = dz->buflen;
	dz->buflen *= 2;
	if((exit_status = read_samps(dz->sampbuf[0],dz)) < 0) {
		sprintf(errstr,"Failed (1) to read input file.\n");
		/*RWD need this here.. */
		free(outfilename);

		return(PROGRAM_ERROR);
	}
	dz->buflen = origbuflen;
	
	cutendno = 0;
	next_cut = 0;
 	abs_bufstart = 0;
	part_splice = 0;

	while(cutendno < dz->iparam[RC_CHCNT]) {
		last_cut = next_cut;
		next_cut = dz->lparray[0][cutendno];
		cutlen = next_cut - last_cut;
/* FIX: MAR 2003 */
		strcpy(outfilename,dz->wordstor[0]);
//FEB 2010 TW
		if(!sloom)
			insert_separator_on_sndfile_name(outfilename,2);
		if((exit_status = create_an_outfile(cutendno,cutlen,outfilename,dz))<0)  {
			free(outfilename);	/*RWD should be done here */
			return(exit_status);
		}
		if(cutendno > 0)
			do_insitu_upsplice(rel_bufstart,dz);
		while(next_cut >= abs_bufend) {	/* ?? >= OR > ?? */
			part_splice = 0;
			if(cutendno != dz->iparam[RC_CHCNT]-1) {
				if((part_splice = stereo_splicelen - (next_cut - abs_bufend)) > 0) {
					splice_start = rel_bufend - part_splice;
					do_insitu_splice(splice_start,rel_bufend,0,chans,dz);
				} else {
					part_splice = 0;
				}
			}
			if((exit_status = write_samps(obuf,dz->buflen,dz)) < 0)	{
				/* RWD and here..... */
				free(outfilename);
				return(exit_status);
			}
			memcpy((char *)dz->sampbuf[0],(char *)dz->sampbuf[1],dz->buflen * sizeof(float));
			abs_bufend += dz->buflen;
			cutlen -= dz->buflen;
			if((exit_status = read_samps(dz->sampbuf[1],dz)) < 0) {
				/*RWD and here.... */
				free(outfilename);
				return(exit_status);
			}
		}
		rel_cutend = cutlen + rel_bufstart;
		if(cutendno != dz->iparam[RC_CHCNT]-1) {
			if(part_splice > 0) {
				part_splice /= dz->infile->channels;
				do_insitu_splice(rel_bufstart,rel_cutend,part_splice,chans,dz);
			} else {
				do_insitu_splice(rel_cutend - stereo_splicelen,rel_cutend,0,chans,dz);
			}
		}
		if((samps_to_write = rel_cutend - rel_bufstart) > 0) {
			if((exit_status = write_samps(obuf,samps_to_write,dz)) < 0)	 {
				/*RWD and here.... */
				free(outfilename);
				return(exit_status);
			}
		}
		if((exit_status = close_an_outfile(outfilename,dz)) < 0) {
			/*RWD and here.... */
			free(outfilename);
			return(exit_status);
		}
		abs_bufstart = next_cut;
		abs_bufend   = abs_bufstart + dz->buflen;

		if(rel_cutend >= dz->buflen) {
  			memcpy((char *)dz->sampbuf[0],(char *)dz->sampbuf[1],dz->buflen * sizeof(float));
			if((exit_status = read_samps(dz->sampbuf[1],dz)) < 0) {
				sprintf(errstr,"Failed (2) to read input file.\n");
				/*RWD and here.... */
				free(outfilename);
				return(PROGRAM_ERROR);
			}
			rel_cutend -= dz->buflen;
		}
		rel_bufstart = rel_cutend;
		rel_bufend   = rel_bufstart + dz->buflen;
		obuf = dz->sampbuf[0] + rel_bufstart;
		cutendno++;
	}
	/*RWD and finally here! */
	free(outfilename);
	return(FINISHED);
}

/**************************** CLOSE_AN_OUTFILE ****************************/

int close_an_outfile(char *outfilename,dataptr dz)
{
	int exit_status;
	dz->process_type = UNEQUAL_SNDFILE;		/* allows header to be written  */
	dz->outfiletype  = SNDFILE_OUT;			/* allows header to be written  */
	if((exit_status = complete_output(dz))<0) {	/* ensures file is truncated */
		/*free(outfilename);*/
		return(exit_status);
	}
	dz->process_type = OTHER_PROCESS;		/* restore true status */
	dz->outfiletype  = NO_OUTPUTFILE;		/* restore true status */
	if((exit_status = reset_peak_finder(dz))<0)
		return(exit_status);
	if(sndcloseEx(dz->ofd) < 0) {
		fprintf(stdout,"WARNING: Can't close output soundfile %s\n",outfilename);
		fflush(stdout);
	}
	/*free(outfilename);	*/		/*RWD: dangerous! */
	dz->ofd = -1;
	return(FINISHED);
}

/**************************** CREATE_AN_OUTFILE ****************************/

int create_an_outfile(int cutno, int cutlen, char *outfilename, dataptr dz)
{
	int exit_status;
	int orig_infilesize;

	insert_new_number_at_filename_end(outfilename,cutno,1);
	dz->process_type = EQUAL_SNDFILE;	/* allow sndfile to be created */
	orig_infilesize = dz->insams[0];
	
	fprintf(stdout,"INFO: creating file	%s\n",outfilename);
	fflush(stdout);
	if((exit_status = create_sized_outfile(outfilename,dz))<0) {
		sprintf(errstr, "Failed to open file %s\n",outfilename);
		/*free(outfilename);*/
		dz->process_type = OTHER_PROCESS;
		dz->insams[0] = orig_infilesize;
		dz->ofd = -1;
		return(GOAL_FAILED);
	}
	dz->process_type = OTHER_PROCESS;
	dz->insams[0] = orig_infilesize;
	reset_filedata_counters(dz);
	return(FINISHED);
}

/**************************** DO_INSITU_UPSPLICE ****************************/

void do_insitu_upsplice(int rel_bufstart,dataptr dz)
{
	float *samp = dz->sampbuf[0] + rel_bufstart;
	double val;
	int chans = dz->infile->channels, i, j;
	for(i=0; i < SHRED_SPLICELEN; i++) {
		for(j=0;j<chans;j++) {
			val = (*samp) * (double)i/SHRED_SPLICELEN;
			*samp = (float)val;
			samp++;
		}
	}
}

/**************************** DO_INSITU_SPLICE ****************************/

void do_insitu_splice(int start,int end,int splicecnt,int chans, dataptr dz)
{
	float *samp = dz->sampbuf[0] + start;
	double val;
	int i;
	int j, k = SHRED_SPLICELEN - splicecnt;
	for(i= start; i < end; i+= chans) {
		k--;
		for(j=0;j<chans;j++) {
			val = (*samp) * (double)k/SHRED_SPLICELEN;
			*samp = (float)val;
			samp++;
		}
	}
}

/**************************** GET_LENGTHS ****************************/

int get_lengths(int stereo_splicelen,dataptr dz)
{   
	int m, n;

	for(m=0,n=1;n< dz->iparam[RC_CHCNT];n++,m++)
		dz->lparray[0][m] = dz->iparam[RC_UNITLEN] * n;
	dz->lparray[0][m] = dz->insams[0];

	if(dz->param >= 0) {
		if(!dz->iparam[RC_SCAT])	
			normal_scat(dz);											/* 1 */
		else	
			heavy_scat(stereo_splicelen,dz);
	}
	return(FINISHED);
}

/**************************** NORMAL_SCAT ****************************/

void normal_scat(dataptr dz)
{   
	double this_scatter;
	int n;
	int chunkscat, total_len = dz->iparam[RC_UNITLEN];	/* 1 */
	int cnt_less_one = dz->iparam[RC_CHCNT] - 1;

	for(n=0;n<cnt_less_one;n++) {			/* 2 */
		this_scatter  = (drand48() - 0.5) * dz->param[RC_SCAT];
		chunkscat = (int)(this_scatter * dz->iparam[RC_UNITLEN]);
		if(ODD(chunkscat))   
			chunkscat--;		/* 3 */
		dz->lparray[0][n]= total_len + chunkscat;
		total_len  += dz->iparam[RC_UNITLEN];			/* 4 */
	}
	dz->lparray[0][n] = dz->insams[0];
}

/*********************** HEAVY_SCAT ***************************
 *
 * (1)	Start at the chunk (this=1) AFTER the first (which can't be moved).
 * (2) 	STARTPTR marks the start of the chunk GROUP (and will be advanced
 *	by RANGE, which is length of chunk-group).
 * (3)	The loop will generate a set of positions for the chunks in
 *	a chunk-group. In the first chunkgroup the position of the
 *	first chunk (start of file) can't be moved, so loop starts at
 *	(first=) 1. Subsequemt loop passes start at 0.
 * (4)	For eveery chunk-group.
 * (5)	  Set the index of the first chunk in this group (start) to the
 *	  current index (this).
 * (6)	  For every member of this chunk-group.
 * (7)	    Generate a random-position within the chunk-grp's range
 *	    and check it is not too close ( < SPLICELEN) to the others.
 *	      Set a checking flag (OK).
 * (8)	      Generate a position within the range, and after the startptr.
 * (9)		Compare it with all previously generated positions in this
 *		chunk-grp AND with last position of previous chunk-group!!
 * 		If it's closer than SPLICELEN, set OK = 0, drop out of
		checking loop and generate another position instead.
 * (10)	      If the position is OK, drop out of position generating loop..
 * (11)	    Advance to next chunk in this group.
 * (12)	  Once all this group is done, advance the group startpoint by RANGE.
 * (13)	  After FIRST grp, all positions can by varied, so set the initial
 *	  loop counter to (first=)0.
 * (14)	If there are chunks left over (endscat!=0)..
 *	  Follow the same procedure for chunks in end group, using the
 *	  alternative variables, endscat and endrange.
 */

void heavy_scat(int stereo_splicelen,dataptr dz)
{   
	int	thiss = 1, that, start, n, m, OK;						/* 1 */
	int startptr = 0;											/* 2 */
	int first = 1;												/* 3 */
	int *chunkptr = dz->lparray[0];
	chunkptr[0] = 0;
	for(n=0;n<dz->iparam[RC_SCATGRPCNT];n++) {					/* 4 */
		start = thiss;											/* 5 */
		for(m=first;m<dz->iparam[RC_SCAT];m++) {				/* 6 */
			do {												/* 7 */
				OK = 1;					
				chunkptr[thiss] = (int)(drand48()*dz->iparam[RC_RANGE]); 	/* TRUNCATE (?)*/
				chunkptr[thiss] += startptr; 					/* 8 */
				if(ODD(chunkptr[thiss]))   
					chunkptr[thiss]--;	
				for(that=start-1; that<thiss; that++) {
					if(abs(chunkptr[thiss] - chunkptr[that])<stereo_splicelen * 2) {
						OK = 0;									/* 9 */
						break;
					}
				}
			} while(!OK);										/* 10 */
			thiss++;											/* 11 */
		}
		startptr += dz->iparam[RC_RANGE];						/* 12 */
		first = 0;												/* 13 */
	}
	if(dz->iparam[RC_ENDSCAT]) {								/* 14 */
		start = thiss;
		for(m=0;m<dz->iparam[RC_ENDSCAT];m++) {
			do {
				OK = 1;
				chunkptr[thiss] = (int)(drand48() * dz->iparam[RC_ENDRANGE]);	  /* TRUNCATE (?) */
				chunkptr[thiss] += startptr;
				if(ODD(chunkptr[thiss]))   
					chunkptr[thiss]--;	
				for(that=start-1; that<thiss; that++) {
					if(abs(chunkptr[thiss] - chunkptr[that])<stereo_splicelen * 2) {
						OK = 0;
						break;
					}
				}
			} while(!OK);
			thiss++;
		}
	}
	ptr_sort(thiss,dz);
}

/************************** PTR_SORT ***************************/

void ptr_sort(int end,dataptr dz)
{   
	int i,j;
	int a;
	int *chunkptr = dz->lparray[0];
	for(j=1;j<end;j++) {
		a = chunkptr[j];
		i = j-1;
		while(i >= 0 && chunkptr[i] > a) {
			chunkptr[i+1]=chunkptr[i];
			i--;
		}
		chunkptr[i+1] = a;
	}
	for(i=0, j=1; j < dz->iparam[RC_CHCNT]; i++, j++)
		chunkptr[i] = chunkptr[j];
	chunkptr[dz->iparam[RC_CHCNT]-1] = dz->insams[0];
}    

/************************** DO_RANDCHUNKS ***************************/

int do_randchunks(dataptr dz)
{   
	double range, multiplier, total, addrange;
    int rangecnt;

    initrand48();
 	if(dz->param[MINCHUNK] > dz->param[MAXCHUNK])
		swap(&dz->param[MINCHUNK],&dz->param[MAXCHUNK]);
    range = dz->duration - dz->param[MINCHUNK];
    if(range > dz->param[MAXCHUNK])
		range = dz->param[MAXCHUNK];
    if(dz->vflag[FROMSTART])
		return do_fromstart(dz);
    if(dz->vflag[LINEAR]) {
    	if((rangecnt = (int)(range/MINOUTDUR))<1) {
		    fprintf(stdout,"INFO: incompatible source duration and MINIMUM PERMITTED DUR %.2lf\n",MINOUTDUR);
		    return(GOAL_FAILED);
		}
		if(rangecnt < dz->iparam[CHUNKCNT])
		    addrange = (range - dz->param[MINCHUNK])/dz->iparam[CHUNKCNT];
	    else
			addrange = (range - dz->param[MINCHUNK])/rangecnt;
		return do_linear(dz,range,rangecnt,addrange);
    } else {
    	multiplier = 1.5;
    	rangecnt = dz->iparam[CHUNKCNT];
    	while(dz->iparam[CHUNKCNT] < rangecnt) {
		    rangecnt = 1;
		    total    = dz->param[MINCHUNK];
		    addrange = MINOUTDUR;
		    multiplier += .5;
    	    while(total < range) {
		    	total += addrange;
		    	addrange *= multiplier;
		    	rangecnt++;
            }
		    rangecnt--;
		}
		return do_nonlinear(dz,range,rangecnt,multiplier);
    }
}

/************************** DO_LINEAR ***************************/

int do_linear(dataptr dz,double range,int rangecnt,double addrange)
{   int m = 0, n, count, exit_status;
	double mindur = dz->param[MINCHUNK];
    double thisrange = dz->param[MINCHUNK], totalrange, k, thislength;
    thislength = thisrange;
	count = dz->iparam[CHUNKCNT];

    for(n=0;n<count;n++) {
        if(m >= rangecnt || thisrange >= range) {
		    m = 0;
		    thislength = thisrange = mindur;
		}
		totalrange = dz->duration - thisrange;
	    k = drand48() * totalrange;
		if((exit_status = cut_chunk(dz,k,k+thislength,n))<0)
			return(exit_status);
		m++;
    	thisrange += addrange;
		thislength = thisrange - (addrange * drand48() * 0.5);
    }
	return(FINISHED);
}

/************************** DO_NONLINEAR ***************************/

int do_nonlinear(dataptr dz,double range,int rangecnt,double multiplier)
{   int m = 0, n, count, exit_status;
	double mindur = dz->param[MINCHUNK];
    double thislength, thisrange = dz->param[MINCHUNK];
    double addrange  = dz->param[MINCHUNK], totalrange, k;
    thislength = thisrange;
	count = dz->iparam[CHUNKCNT];
    for(n=0;n<count;n++) {
        if(m >= rangecnt || thisrange >= range) {
			m = 0;
	   		thislength = thisrange  = mindur;
	   		addrange   = mindur;
		}
		totalrange = dz->duration - thisrange;
     	k = drand48() * totalrange;
		if((exit_status = cut_chunk(dz,k,k+thislength,n))<0)
			return(exit_status);
		m++;
    	thisrange += addrange;
    	thislength = thisrange - (addrange * drand48() * 0.5);
		addrange  *= multiplier;
    }
	return(FINISHED);
}

/************************** DO_FROMSTART ***************************/

int do_fromstart(dataptr dz)
{   
	int n, count,exit_status;
	double mindur = dz->param[MINCHUNK], maxdur = dz->param[MAXCHUNK];
    double k = 0.0, range = dz->param[MAXCHUNK] - dz->param[MINCHUNK], outdur, step;
	count = dz->iparam[CHUNKCNT];
    switch(dz->vflag[LINEAR]) {
    case(0):
    	for(n=0;n<count;n++) {
    	    k = drand48() * range;
			if((exit_status = cut_chunk(dz,0.0,k+mindur,n))<0)
				return(exit_status);
    	}
		break;
    case(1):
    	if((exit_status = cut_chunk(dz,0.0,mindur,0))<0)
			return(exit_status);
		outdur = mindur;
		step   = range/(double)(count-2);
		count--;
    	for(n=1;n<count;n++) {
    	    k = drand48() * step;
		    if((exit_status = cut_chunk(dz,0.0,outdur + k,n))<0)
				return(exit_status);
    	    outdur += step;
    	}
    	if((exit_status = cut_chunk(dz,0.0,maxdur,n))<0)
			return(exit_status);
		break;
    default:
		fprintf(stderr,"\nImpossible!!! invalid value of 'linear'");
		break;
    }
	return(FINISHED);
}

/************************** CUT_CHUNK ***************************/

int cut_chunk(dataptr dz,double startcut,double endcut,int count)
{
	int exit_status;
	int origprocess = dz->process, origmode = dz->mode, namelen, numlen = 1;
	char *outfilename;
	char *p, *q, *r;
	if(!sloom) {
		namelen = strlen(dz->wordstor[0]);
		q = dz->wordstor[0];
		r = dz->wordstor[0] + namelen;
		p = r - 1;
		/*RWD 6:2001 added trap for ':' */
		while((*p != '\\') && (*p != '/') && (*p != ':')) {
			p--	;
			if(p < dz->wordstor[0])
				break;
		}
		if(p > dz->wordstor[0]) {
			p++;
			while(p <= r)
				*q++ = *p++;
		}
	}
	dz->process = EDIT_CUT;
	dz->mode = EDIT_SECS;
	dz->param[CUT_CUT] = startcut;	
	dz->param[CUT_END] = endcut;
	dz->param[CUT_SPLEN] = EDIT_SPLICELEN;
	namelen = strlen(dz->wordstor[0]);
	namelen--;
	if(count > 99)
		numlen = 3;
	else if(count > 9)
		numlen = 2;
	reset_filedata_counters(dz);
	if((outfilename = (char *)malloc((namelen + numlen + 1) * sizeof(char)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for outfilename.\n");
		return(MEMORY_ERROR);
	}				
    strcpy(outfilename,dz->wordstor[0]);
	insert_new_number_at_filename_end(outfilename,count,1);

	dz->process_type = UNEQUAL_SNDFILE;		/* allow sndfile to be created */
	dz->outfiletype  = SNDFILE_OUT;			/* allows header to be written  */
	if((exit_status = create_sized_outfile(outfilename,dz))<0)
		return(exit_status);
	if((exit_status = edit_pconsistency(dz))<0)
		return exit_status;
	if((exit_status = edit_preprocess(dz))<0)
		return exit_status;
	if((exit_status = do_cut(dz))<0)
		return exit_status;
	if((exit_status = complete_output(dz))<0)
		return(exit_status);
	if((exit_status = reset_peak_finder(dz))<0)
		return(exit_status);
	if(sndcloseEx(dz->ofd) < 0) {
		fprintf(stdout,"WARNING: Can't close output soundfile %s\n",outfilename);
		fflush(stdout);
	}
	dz->process_type = OTHER_PROCESS;
	dz->outfiletype  = NO_OUTPUTFILE;		/* restore true status */
	free(outfilename);
	dz->ofd = -1;
	dz->process = origprocess;
	dz->mode = origmode;
	return(FINISHED);
}

/************************** CUT_MANY ***************************/

int cut_many(dataptr dz)
{
	float *ibuf, *obuf;
	char filename[64];
	int k, startsplice, endsplice;
	int startcut, endcut, samps_left, startseek, offset;
	int outcnt = 0, n = 0, splice_start, samps_to_write, samps_written;
	int exit_status;
	int last_fileout = (dz->itemcnt/2) - 1;

	while(n < dz->itemcnt) {
		if(outcnt > 0) {
			strcpy(filename,dz->wordstor[0]);
			if(sloom)
				insert_new_number_at_filename_end(filename,outcnt,1);
			else
				insert_new_number_at_filename_end(filename,outcnt+1,0);
			if((exit_status = create_sized_outfile(filename,dz))<0) {
				sprintf(errstr, "ERROR: Soundfile %s already exists: cannot proceed further\n", filename);
				return(exit_status);
			}
		}
		startcut = dz->lparray[0][n++];
		endcut   = dz->lparray[0][n++];
		samps_left = endcut - startcut;
		startsplice = (samps_left - dz->iparam[CM_SPLICE_TOTSAMPS]) % dz->buflen;
		startseek = (startcut/F_SECSIZE) * F_SECSIZE;
		offset = startcut - startseek;
		sndseekEx(dz->ifd[0],startseek,0);
		ibuf = dz->bigbuf; 
		obuf = ibuf + offset;
		dz->buflen += F_SECSIZE;
		if((exit_status = read_samps(ibuf,dz))<0)
			return(SYSTEM_ERROR);
		dz->buflen -= F_SECSIZE;
		ibuf += F_SECSIZE; 
		k = 0;
		splice_start = 0;
		dz->total_samps_written = 0;
		do {
			samps_to_write = min(samps_left,dz->buflen); 
			if(k == 0)								
				splice_obuf_start(obuf,dz);
			if(samps_left < dz->buflen + dz->iparam[CM_SPLICE_TOTSAMPS]) {
				if(splice_start == 0)	{
					if((endsplice = startsplice + dz->iparam[CM_SPLICE_TOTSAMPS]) <= dz->buflen) {
						splice_obuf_end(obuf,&splice_start,startsplice,endsplice,dz);
					} else {
						splice_obuf_end(obuf,&splice_start,startsplice,dz->buflen,dz);
					}
				} else {
					splice_obuf_end(obuf,&splice_start,0,samps_left,dz);
				}
			}
			if(samps_to_write > 0) {
				if((exit_status = write_samps_no_report(obuf,samps_to_write,&samps_written,dz))<0)
					return(exit_status);
			}
			if((samps_left -= samps_to_write) >  0) {
				memcpy((char *)obuf,(char *)(obuf + dz->buflen),(F_SECSIZE - offset) * sizeof(float));
				if((exit_status = read_samps(ibuf,dz))<0)
					return(exit_status);
			}
			k++;
		} while(samps_left > 0);
		if(outcnt != last_fileout) {
			if((exit_status = headwrite(dz->ofd,dz))<0)
				return(exit_status);
			if((exit_status = reset_peak_finder(dz))<0)
				return(exit_status);
			if(sndcloseEx(dz->ofd)<0) {
				sprintf(errstr,"Failed to close output soundfile.\n");
				return(SYSTEM_ERROR);
			}
			dz->ofd = -1;
		}
		outcnt++;
		display_virtual_time(outcnt,dz);
	}
	return(FINISHED);
}

/************************** SPLICE_OBUF_START ***************************/

void splice_obuf_start(float *obuf,dataptr dz)
{
	int n, m;
	double aincr = dz->param[CM_SPLICEINCR];
	double mult = 0;
	int chans = dz->infile->channels;
	int splicelen = dz->iparam[CM_SPLICESAMPS] * dz->infile->channels;
	for(n = 0;n < splicelen; n+=chans) {
		for(m = 0;m < chans; m++)
			obuf[n+m] = (float)(obuf[n+m] * mult);
		mult += aincr;
	}
}

/************************** SPLICE_OBUF_END ***************************/

void splice_obuf_end(float *obuf,int *splice_start,int startsamp,int endsamp,dataptr dz)
{
	int n, m;
	double aincr = dz->param[CM_SPLICEINCR];
	double mult = 1.0 - (aincr * (*splice_start + 1));
	int chans = dz->infile->channels;				   
	int splicelen = (endsamp - startsamp)/chans;
	for(n = startsamp;n < endsamp; n+=chans) {
		for(m = 0;m < chans; m++)
			obuf[n+m] = (float)(obuf[n+m] * mult);
		mult -= aincr;
	}
	*splice_start += splicelen;
}

/********************************** DO_NOISE_SUPPRESSION **********************************
 *
 * Creating envelope to splice out non_pitched material based on wavset-len,
 * and eliminating any bits of tone which are too short
 */

int do_noise_suppression(dataptr dz)
{
	float *ibuf = dz->sampbuf[0];
	int getnextbuf = 0, phase, isnoise = -1, doincr, noisefirst = 0, spliceafter;
	int in_noise = 0, in_splice = 0, OK, exit_status;
	int j, k, n = 0, m, nextpoint, splice_cnt, edit_position, next_edit_position;
	int twosplices;
	int brkpntcnt = 0, last_total_samps_read = 0, lastzcross = 0;
	double splincr;
	int thispos, lastpos = 0;

	splincr = 1.0/(double)dz->iparam[NOISE_SPLEN];

	if((exit_status = read_samps(ibuf,dz))<0)
		return(exit_status);
	else if(dz->ssampsread <= 0) {
		sprintf(errstr,"Failed to read sound from input file\n");
		return(DATA_ERROR);
	}

	/* ESTABLISH INITIAL PHASE OF SIGNAL */

	do {
		if(dz->ssampsread <= 0)
			break;
		n = 0;
		getnextbuf = 0;
		while(smpflteq(ibuf[n],0.0)) {
			if(++n >= dz->ssampsread) {
				getnextbuf = 1;
				break;
			}
		}
		if(!getnextbuf)
			break;
		last_total_samps_read = dz->total_samps_read;
	} while((exit_status = read_samps(ibuf,dz))>=0);
	
	if(exit_status < 0)
		return(exit_status);
	if(getnextbuf) {
		sprintf(errstr,"Failed to find any wavecycles in file.\n");
		return(DATA_ERROR);
	}	
	if(ibuf[n] > 0.0)
		phase = 0;
	else
		phase = 1;
	
	/* LOCATE NOISE: non-NOISE SWITCHES */

	for(;;) {	/* READ INPUT BUFFERS LOOP */

		for(;;) {	/* LOCATE WAVECYCLE IN CURRENT BUFFER, LOOP */

			/* FIND A WAVECYCLE */
			
			if(phase == 0) {
				while(ibuf[n] > 0.0) {
					if(++n >= dz->ssampsread) {
						getnextbuf = 1;
						break;
					}
				}
				if(getnextbuf)
					break;
				while(ibuf[n] <= 0.0) {
					if(++n >= dz->ssampsread) {
						getnextbuf = 1;
						break;
					}
				}
				if(getnextbuf)
					break;
			} else {
				while(ibuf[n] <= 0.0) {
					if(++n >= dz->ssampsread) {
						getnextbuf = 1;
						break;
					}
				}
				if(getnextbuf)
					break;
				while(ibuf[n] > 0.0) {
					if(++n >= dz->ssampsread) {
						getnextbuf = 1;
						break;
					}
				}
				if(getnextbuf)
					break;
			}
			thispos = last_total_samps_read + n;
			doincr  = 0;

			/* MEASURE WAVE-CYCLE LENGTH, AND TEST FOR NOISE */
			/* IF SIGNAL SWITCHES FROM NOISE  to NOT-NOISE or vice versa, STORE THAT POSITION */
			
			if(thispos - lastzcross < dz->iparam[NOISE_MINFRQ]) {
				if(isnoise < 0)				/* if at signal start ... */
					noisefirst = 1;			/* flag signal starts with noise - (don't store position [i.e. count brkpnts] yet) */
				else if(isnoise == 0) {		/* elseif switched into noise ... */
					if(brkpntcnt == 0) {
						if(thispos > dz->iparam[MIN_TONELEN])	/* if enough tone present */
							doincr = 1;			/* ... --- keep point (i.e. count brkpnt & incr brkpnt pointer) */
						else
							noisefirst = 1;
					} else {
						if(thispos - lastpos > dz->iparam[MIN_TONELEN])
							doincr = 1;			/* ... --- keep point (i.e. count brkpnt & incr brkpnt pointer) */
						else {					/* if NOT ENOUGH tone present */
							brkpntcnt--;		/* eliminate the previous breakpoint, which started the tone segment */
							thispos = lastpos;
						}
					}
				}
				isnoise = 1;
			} else {
				if(isnoise < 0)				/* if at signal start ... */
					noisefirst = 0;			/* flag signal doesn't start with noise - (don't store position [i.e. count brkpnts] yet) */
				else if(isnoise == 1) {		/* elseif switched out of noise ... */
					if(brkpntcnt == 0) {
						if(thispos > dz->iparam[MIN_NOISLEN])
							doincr = 1;
						else
							noisefirst = 0;
					} else {					/* if enough noise present ... */
						if(thispos - lastpos > dz->iparam[MIN_NOISLEN])
							doincr = 1;			/* ....--- keep point (i.e. count brkpnt & incr brkpnt pointer) */
						else {					/* if NOT ENOUGH noise present */
							brkpntcnt--;		/* eliminate the previous breakpoint, which started the noise segment */
							thispos = lastpos;
						}
					}
				}
				isnoise = 0;
			}										/* store position of last waveset end... */
			lastzcross = thispos;	/* regardless of whether position is stored in brkpnt array */

			if(doincr) {					/* if point retained in brkpoint array, move along array */
				lastpos = thispos;
				brkpntcnt++;
			}
		}		/* READ NEXT BUFFER */
		last_total_samps_read = dz->total_samps_read;
		if((exit_status = read_samps(ibuf,dz)) < 0)
			return(exit_status);
		else if(dz->ssampsread <= 0)
			break;
		n = 0;
		getnextbuf = 0;
	}
//TW ADDED TEST BEFORE MALLOCS ATTEMPTED: JULY 2006
	if(brkpntcnt<=1) {
		if(isnoise) {
			sprintf(errstr,"Sound is all noise.");
			return(GOAL_FAILED);
		} else {
			sprintf(errstr,"Sound has no noise.");
			return(GOAL_FAILED);
		}
	}
//TW late july :TO AVOID realooc PROBLEMS, GRAB THE DOUBLE WADGE OF MEMORY HERE
	if((dz->lparray[0] = (int *)malloc(brkpntcnt * 2 * sizeof(int)))==NULL) {
		sprintf(errstr,"Insufficient memory for edit points.");
		return(MEMORY_ERROR);
	}
	sndseekEx(dz->ifd[0],0,0);
//TW RESET VARIABLE BELOW
	last_total_samps_read = 0;
	reset_filedata_counters(dz);
	brkpntcnt = 0;
	if((exit_status = read_samps(ibuf,dz))<0)
		return(exit_status);
	else if(dz->ssampsread <= 0) {
		sprintf(errstr,"Failed to read sound from input file\n");
		return(DATA_ERROR);
	}
	do {
		if(dz->ssampsread <= 0)
			break;
		n = 0;
		getnextbuf = 0;
		while(smpflteq(ibuf[n],0.0)) {
			if(++n >= dz->ssampsread) {
				getnextbuf = 1;
				break;
			}
		}
		if(!getnextbuf)
			break;
		last_total_samps_read = dz->total_samps_read;
	} while((exit_status = read_samps(ibuf,dz))>=0);
	
	if(exit_status < 0)
		return(exit_status);
	if(getnextbuf) {
		sprintf(errstr,"Failed to find any wavecycles in file.\n");
		return(DATA_ERROR);
	}	
	if(ibuf[n] > 0.0)
		phase = 0;
	else
		phase = 1;
	
	/* LOCATE NOISE: non-NOISE SWITCHES */
	
	for(;;) {	/* READ INPUT BUFFERS LOOP */

		for(;;) {	/* LOCATE WAVECYCLE IN CURRENT BUFFER, LOOP */

			/* FIND A WAVECYCLE */
			
			if(phase == 0) {
				while(ibuf[n] > 0.0) {
					if(++n >= dz->ssampsread) {
						getnextbuf = 1;
						break;
					}
				}
				if(getnextbuf)
					break;
				while(ibuf[n] <= 0.0) {
					if(++n >= dz->ssampsread) {
						getnextbuf = 1;
						break;
					}
				}
				if(getnextbuf)
					break;
			} else {
				while(ibuf[n] <= 0.0) {
					if(++n >= dz->ssampsread) {
						getnextbuf = 1;
						break;
					}
				}
				if(getnextbuf)
					break;
				while(ibuf[n] > 0.0) {
					if(++n >= dz->ssampsread) {
						getnextbuf = 1;
						break;
					}
				}
				if(getnextbuf)
					break;
			}
			dz->lparray[0][brkpntcnt] = last_total_samps_read + n;
			doincr  = 0;

			/* MEASURE WAVE-CYCLE LENGTH, AND TEST FOR NOISE */
			/* IF SIGNAL SWITCHES FROM NOISE  to NOT-NOISE or vice versa, STORE THAT POSITION */
			
			if(dz->lparray[0][brkpntcnt] - lastzcross < dz->iparam[NOISE_MINFRQ]) {
				if(isnoise < 0)				/* if at signal start ... */
					noisefirst = 1;			/* flag signal starts with noise - (don't store position [i.e. count brkpnts] yet) */
				else if(isnoise == 0) {		/* elseif switched into noise ... */
					if(brkpntcnt == 0) {
						if(dz->lparray[0][brkpntcnt] > dz->iparam[MIN_TONELEN])	/* if enough tone present */
							doincr = 1;			/* ... --- keep point (i.e. count brkpnt & incr brkpnt pointer) */
						else
							noisefirst = 1;
					} else {
						if(dz->lparray[0][brkpntcnt] - dz->lparray[0][brkpntcnt-1] > dz->iparam[MIN_TONELEN])
							doincr = 1;			/* ... --- keep point (i.e. count brkpnt & incr brkpnt pointer) */
						else					/* if NOT ENOUGH tone present */
							brkpntcnt--;		/* eliminate the previous breakpoint, which started the tone segment */
					}
				}
				isnoise = 1;
			} else {
				if(isnoise < 0)				/* if at signal start ... */
					noisefirst = 0;			/* flag signal doesn't start with noise - (don't store position [i.e. count brkpnts] yet) */
				else if(isnoise == 1) {		/* elseif switched out of noise ... */
					if(brkpntcnt == 0) {
						if(dz->lparray[0][brkpntcnt] > dz->iparam[MIN_NOISLEN])
							doincr = 1;
						else
							noisefirst = 0;
					} else {					/* if enough noise present ... */
						if(dz->lparray[0][brkpntcnt] - dz->lparray[0][brkpntcnt-1] > dz->iparam[MIN_NOISLEN])
							doincr = 1;			/* ....--- keep point (i.e. count brkpnt & incr brkpnt pointer) */
						else					/* if NOT ENOUGH noise present */
							brkpntcnt--;		/* eliminate the previous breakpoint, which started the noise segment */
					}
				}
				isnoise = 0;
			}										/* store position of last waveset end... */
			lastzcross = dz->lparray[0][brkpntcnt];	/* regardless of whether position is stored in brkpnt array */

			if(doincr)							/* if point retained in brkpoint array, move along array */
				++brkpntcnt;
		}		/* READ NEXT BUFFER */
		last_total_samps_read = dz->total_samps_read;
		if((exit_status = read_samps(ibuf,dz))<0)
			return(exit_status);
		else if(dz->ssampsread <= 0)
			break;
		n = 0;
		getnextbuf = 0;
	}

	/* CHECK THAT ANY NOISE : non-NOISE SWITCHES FOUND */

	if(brkpntcnt<=1) {
		if(isnoise) {
			sprintf(errstr,"Sound is all noise.");
			return(GOAL_FAILED);
		} else {
			sprintf(errstr,"Sound has no noise.");
			return(GOAL_FAILED);
		}
	}

	/* ELIMINATE ANY TONE-SEGS SHORTER THAN 2 SPLICELENS */

	twosplices = (dz->iparam[NOISE_SPLEN] * 2);
	if(noisefirst) 
		k = 1;
	else
		k = 2;
	for(n=k;n<brkpntcnt;n++) {
		if(dz->lparray[0][n] - dz->lparray[0][n-1] <= twosplices) {
			for(j=n+1,m=n-1;j<brkpntcnt;j++,m++)
				dz->lparray[0][m] =dz->lparray[0][j];
			brkpntcnt -= 2;
			n -= 2;
		}
	}

	/* ADD SPLICE STARTS AND ENDS TO ARRAY of SWITCH-POINTS */

//TW: late july : AVOID realloc PROBLEMS ... EXTRA SPACE INCLUDED IM INITIAL malloc INSTERAD OF ADDING IT HERE
//	if((dz->lparray[0] = (int *)realloc(dz->lparray[0],brkpntcnt * 2 * sizeof(int)))==NULL) {
//		sprintf(errstr,"No more memory for splice points.");
//		return(MEMORY_ERROR);
//	}
	
	if((noisefirst && EVEN(brkpntcnt)) || (!noisefirst && ODD(brkpntcnt)))		/* ends with noise */
		spliceafter = 0;
	else																		/* ends with tone */
		spliceafter = 1;
	for(n = brkpntcnt-1; n >= 0; n--) {
		m = n * 2;
		if(spliceafter) {
			dz->lparray[0][m]   = dz->lparray[0][n];
			dz->lparray[0][m+1] = dz->lparray[0][n] + dz->iparam[NOISE_SPLEN];
		} else {
			dz->lparray[0][m+1] = dz->lparray[0][n];
			dz->lparray[0][m]   = dz->lparray[0][n] - dz->iparam[NOISE_SPLEN];
		}
		spliceafter = !spliceafter;
	}
	brkpntcnt *= 2;

	/* ELIMINATE BAD POINTS AT START OR END */
											/* as original [noise to non-noise ot vice versa] pts MUST lie within sound..... */
	if(dz->lparray[0][0] < 0) {				/* 1st splice can start BEFORE snd start only if it's a 'splicebefore' */
											/* from tone->noise at start of sound, and that tone is too short */
		for(j=0,n=2;n < brkpntcnt;n++,j++)	/* eliminate it by eliminating opening splice */
			dz->lparray[0][j] = dz->lparray[0][n];
		brkpntcnt -= 2;						/* AND indicating sounds now starts with noise */
		noisefirst = 1;
	}										/* final splice can end AFTER sound end only if it's a 'spliceafter' */
											/* from noise->tone at end of sound, and that tone is too short */
											/* so, by eliminating last splice, we eliminate that tone */
	if(dz->lparray[0][brkpntcnt-1] > dz->insams[0])
		brkpntcnt -= 2;						

	if(brkpntcnt<=2) {
		if(isnoise) {
			sprintf(errstr,"Sound is all noise.");
			return(GOAL_FAILED);
		} else {
			sprintf(errstr,"Sound has no noise.");
			return(GOAL_FAILED);
		}
	}

	/* RETAIN SPLICE STARTS ONLY */
	
	for(n=1,m=2;m <brkpntcnt;n++, m+=2)
		dz->lparray[0][n] = dz->lparray[0][m];
	brkpntcnt = n;

	/* NOW EDIT SOUND */
	sndseekEx(dz->ifd[0],0,0);
	dz->total_samps_read = 0;
	display_virtual_time(0,dz);

	in_splice = 0;
	splice_cnt = 0;
	if(dz->vflag[GET_NOISE])
		noisefirst = !noisefirst;
	
	if(noisefirst)
		in_noise = 1;
	else
		in_noise = 0;

	k = -1;		
	OK = JUST_CONTINUE;

	/* BUFFER READING:WRITING LOOP */
	
	while(OK == JUST_CONTINUE) {
		last_total_samps_read = dz->total_samps_read;
		if((exit_status = read_samps(ibuf,dz))<0)
			return(exit_status);		/* reads first, or subsequent buffer */
		else if(dz->ssampsread <= 0) {
			OK = CUTOFF_OUTPUT;			/* if infile exhausted, flag no more output file here */
			break;						/* which means we can break from read loop WITHOUT writing more data */
		}
		edit_position  = 0;				/* point to start of buffer, earliest position where edits can take effect */
		if(in_splice) {					/* if editing process has begun before start of this buffer */
										/* .... and we're in midst of a splice at start of buffer, complete splice */
										/* (guaranteed to be completed within current buffer, due to bufsize checks) */
			in_splice = do_alternating_splices(&in_noise,&splice_cnt,&edit_position,splincr,dz);
			k++;
		}

	/* EDITING OF CURRENT BUFFER, LOOP */
	
		for(;;) {
			if(in_splice)				/* If a splice fails to finish during treatment of current buffer ...  */
				break;					/* .... we must be at end of buffer, so break from buffer-editing loop */

			if(++k >= brkpntcnt) {		/* if there are no more editpoints to read ... */
				if(in_noise) {						/* this indicates final seg is noise  */
					dz->ssampsread = edit_position;	/* so we can curtail outfile to START of this final [edited out] noise seg */
					OK = CUTOFF_OUTPUT;
				} else							/* otherwise, final segment is retainable signal */
					OK = WRITERESTOF_INPUT;		/* so flag that we want to write the rest of input to output */
				break;						/* in BOTH cases, exit from the buffer-editing loop */

			} else						/* else, get next edit point */
				nextpoint = dz->lparray[0][k];	
										/* if next edit point lies within current buffer */
			if(nextpoint < dz->total_samps_read) {
				next_edit_position = nextpoint - last_total_samps_read;	/* get position of edit-point WITHIN buffer */
				if(in_noise)				/* remove noise by zeroing signal from previous edit_end to current edit_start */
					memset((char *)(ibuf + edit_position),0,(next_edit_position - edit_position) * sizeof(float));	
				edit_position = next_edit_position;		/* reset edit_position */
				splice_cnt = 0;				/* reset the splice-counter to zero, for new splice */
											/* do-splice, automatically resetting 'in_noise' flag if splice completed */
											/* ...but setting in_splice flag to TRUE if splice NOT completed */
				in_splice = do_alternating_splices(&in_noise,&splice_cnt,&edit_position,splincr,dz);
			} else {					/* next edit point lies within current buffer */
				k--;					/* restore counter when buffer switches */
				break;					/* so, exit buffer-editing loop and go write-output and read next buffer */
			}
		}								
									/* EXITED  buffer-editing loop */
									/* if still in noise, remove noise at end of buffer  */
		if((edit_position < dz->ssampsread) && in_noise)
			memset((char *)(ibuf + edit_position),0,(dz->ssampsread - edit_position) * sizeof(float));
									/* write the edited buffer */
		if(dz->ssampsread > 0) {
			if((exit_status = write_samps(ibuf,dz->ssampsread,dz))< 0)
				return(exit_status);
		}							/* go [round loop] to read next buffer */
	}

	if(OK==WRITERESTOF_INPUT) {
		if((exit_status = read_samps(ibuf,dz))<0)
			return(exit_status);
		while(dz->ssampsread > 0) {
			if((exit_status = write_samps(ibuf,dz->ssampsread,dz))< 0)
				return(exit_status);
			if((exit_status = read_samps(ibuf,dz))<0)
				return(exit_status);
		}
	}
	return(FINISHED);
}

/********************************** DO_ALTERNATING_SPLICES **********************************/

int do_alternating_splices(int *in_noise,int *splice_cnt,int *edit_position,double splincr,dataptr dz)
{
	double newval, spliceval = (double)(*splice_cnt)/(double)dz->iparam[NOISE_SPLEN];
	double this_splincr = splincr;
	
	if(!(*in_noise)) {
		spliceval = 1.0 - spliceval;
		this_splincr = -this_splincr;
	}
	while(*splice_cnt < dz->iparam[NOISE_SPLEN]) {
		newval = dz->sampbuf[0][*edit_position] * spliceval;
		dz->sampbuf[0][*edit_position] = (float)newval;
		(*edit_position)++;
		(*splice_cnt)++;
		if(*edit_position >= dz->ssampsread)
			break;
		spliceval += this_splincr;
	}
	if(*splice_cnt >= dz->iparam[NOISE_SPLEN]) {
		(*in_noise) = !(*in_noise);
		return(0);
	}
	return(1);
}

/********************************** SETUP_INSERT2_OVERWRITE_FLAG **********************************/

int setup_insert2_overwrite_flag(dataptr dz)
{
	int flags_needed = INSERT_OVERWRITE + 1;
	if(dz->application->vflag_cnt <= INSERT_OVERWRITE) {
		if(dz->application->vflag_cnt <=0) {
			if((dz->vflag  = (char *)malloc(flags_needed * sizeof(char)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY: vflag store,\n");
				return(MEMORY_ERROR);
			}
		} else {
			if((dz->vflag  = (char *)realloc((char *)dz->vflag,flags_needed * sizeof(char)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY: vflag store,\n");
				return(MEMORY_ERROR);
			}
		}
	}
	dz->vflag[INSERT_OVERWRITE] = 1;
	return FINISHED;
}

/*************************** DO_PATTERNED_JOINING *****************************
 *
 * Exactly as do_joining, except for commented  stuff.
 */

int do_patterned_joining(dataptr dz)
{
	int  exit_status;
	int total_fsamps_read, samps_to_get, samps_read, samps_to_write;
	int	 n, m, k, nextfile, readcnt, lmt = 0;
	int  overspill, splice_remnant;
	int	 tempfile = -1;
	char tempfilename[200];
	int thisifd;
	unsigned int this_size, hold_samps;

	strcpy(tempfilename,dz->wordstor[0]);
	strcat(tempfilename,"_");
	insert_new_number_at_filename_end(tempfilename,dz->infilecnt,0);	/* create name for temp file */

	dz->iparam[CUT_SMPSREMAIN] = dz->buflen;
	dz->sampbuf[READBUF]       = dz->sampbuf[BBUF];
	if(dz->iparam[CUT_SPLEN]==0)
		lmt = -1;

	for(m=0;m < dz->itemcnt;m++) {					/* get next file in pattern */
		if(dz->process == JOIN_SEQ && m >= dz->iparam[MAX_LEN])
			break;
		n = dz->iparray[0][m];
		if(n >= dz->infilecnt) {
			sndseekEx(dz->other_file,0,0);
			fprintf(stdout,"INFO: Processing FILE %d\n",tempfile+1);
		} else {										/* reset selected file to start */
			sndseekEx(dz->ifd[n],0,0);					
			fprintf(stdout,"INFO: Processing FILE %d\n",n+1);
		}
		fflush(stdout);
		if(m < dz->itemcnt - 1)						/* check the succeeding file in the pattern */
			nextfile = dz->iparray[0][m+1];
		else 
			nextfile = -1;
		if(n == nextfile) {							/* if same file is used contiguously */
			if(tempfile != n) {						/* if the temporary file is not the same as this one */
				if(tempfile != -1) {
					char pfname[_MAX_PATH];
					strcpy(pfname, snd_getfilename(dz->other_file));
					sndcloseEx(dz->other_file);
					if(remove(/*outfilename*/pfname)<0) {
						fprintf(stdout,"WARNING: Tempfile %s not removed: ",tempfilename);
						fflush(stdout);
					}
					dz->other_file = -1;
				}									/* create a temporary file */
				if((dz->other_file=sndcreat_formatted(tempfilename,dz->insams[n],SAMP_FLOAT,
				dz->infile->channels,dz->infile->srate,CDP_CREATE_NORMAL))<0) {
					sprintf(errstr,"Cannot open temporary file %s: %s\n", tempfilename,sferrstr());
					return(SYSTEM_ERROR);
				}									/* copy current file into temporary file */
				do {
					if((samps_read  = fgetfbufEx(dz->sampbuf[COPYBUF], dz->buflen,dz->ifd[n],0))<0) {
						sprintf(errstr,"Sound read error.\n");
						return(SYSTEM_ERROR);
					}
					if(samps_read > 0) {
						hold_samps = dz->total_samps_written;
						if((exit_status = 
							write_samps_to_elsewhere(dz->other_file,dz->sampbuf[COPYBUF],samps_read,dz))<0) {
							sprintf(errstr, "Can't write copied samples to temporary soundfile:  (is hard-disk full?).\n");
							return(SYSTEM_ERROR);
						}
						dz->total_samps_written = hold_samps;
					}
				} while (samps_read > 0);			
				memset((char *)dz->sampbuf[COPYBUF],0,dz->buflen * sizeof(float));
				sndseekEx(dz->ifd[n],0,0);				/* reset copy buffer (safety only), and current file position */
				tempfile = n;						/* remember which file's data is in the temporary file */			
			}
			dz->iparray[0][m+1] = dz->infilecnt;	/* set up next-file-to-use as the temporary file */
		}
		if(n >= dz->infilecnt)						/* if temporary file in use */
			this_size = dz->insams[tempfile];		/* find its size by reference to file copied from */
		else										/* else find size of current normal file */
			this_size = dz->insams[n];
		if((samps_to_get = this_size - dz->iparam[CUT_SPLEN])<=0) {
			sprintf(errstr,"File %d is too short for the given splicelength\n",n+1);
			return(GOAL_FAILED);
		}
		total_fsamps_read = 0;
		readcnt = 0;
		for(;;) {
			if(n >= dz->infilecnt)					/* if temporary file in use, get its filepointer */
				thisifd = dz->other_file;
			else									/* else get filepointer of current file */
				thisifd = dz->ifd[n];
			if((samps_read = fgetfbufEx(dz->sampbuf[READBUF],dz->iparam[CUT_SMPSREMAIN],thisifd,0))<0) {
				sprintf(errstr, "Can't read samples from input soundfile\n");
				return(SYSTEM_ERROR);
			}
			if(dz->process == JOIN_SEQDYN) {
				for(k = 0;k < samps_read; k++)
					dz->sampbuf[READBUF][k] = (float)(dz->sampbuf[READBUF][k] * dz->parray[2][m]);
			}
			overspill = (total_fsamps_read += samps_read) - samps_to_get;
			if(overspill > lmt)
				break;
			if(dz->iparam[CUT_SPLEN]>0 && readcnt==0)
				do_join_startsplice(n,dz);
			if((exit_status = write_exact_samps(dz->sampbuf[BBUF],dz->buflen,dz))<0)	
				return(exit_status);
			housekeep2(samps_read,dz);
			readcnt++;
		}
		if(dz->iparam[CUT_SPLEN]>0) {
			if(readcnt==0)
				do_join_startsplice(n,dz);
			housekeep1(samps_read,overspill,dz);
			if((splice_remnant = dz->iparam[CUT_SPLEN] - overspill)>0)	{
				if((exit_status = do_join_write(n,splice_remnant,overspill,dz))<0)
					return(exit_status);
			} else {
				if((exit_status = reset_join_buffer_params(dz))<0)
					return(exit_status);
			}
		} else {
			dz->sbufptr[ENDSPLICE_ADDR] = dz->sampbuf[READBUF] + samps_read;
			if((exit_status = reset_join_buffer_params(dz))<0)
				return(exit_status);
		}
	}
	if(dz->iparam[CUT_SPLEN]>0) {
		if(dz->vflag[SPLICE_END])
			do_join_endsplice(dz);
		memcpy((char *)dz->sampbuf[READBUF],(char *)dz->sampbuf[SPLICEBUF],dz->iparam[CUT_SPLEN] * sizeof(float));
	}		
	samps_to_write = dz->sampbuf[READBUF] + dz->iparam[CUT_SPLEN] - dz->sampbuf[BBUF];
	if(samps_to_write > 0)
		exit_status = write_samps(dz->sampbuf[BBUF],samps_to_write,dz);
	else
		exit_status = FINISHED;
	if(tempfile != -1) {			
		char pfname[_MAX_PATH];
		strcpy(pfname, snd_getfilename(dz->other_file));
		sndcloseEx(dz->other_file);
		if(remove(/*outfilename*/pfname)<0) {
			fprintf(stdout,"WARNING: Tempfile %s not removed: ",tempfilename);
			fflush(stdout);
		}
		dz->other_file = -1;
	}
	return exit_status;
}

/*************************** DO_MANY_ZCUTS *****************************/

int do_many_zcuts(dataptr dz) {
	int exit_status, outcnt, last_fileout = 0;
	char filename[400];
	int n, m;
	outcnt = 0;
	for(n=0,m=0;m < dz->itemcnt;n++,m+=2) {
		if((outcnt > 0) && (last_fileout != outcnt)) {
			strcpy(filename,dz->wordstor[0]);
			if(sloom)
				insert_new_number_at_filename_end(filename,outcnt,1);
			else
				insert_new_number_at_filename_end(filename,outcnt+1,0);
			if((exit_status = create_sized_outfile(filename,dz))<0) {
				sprintf(errstr, "ERROR: Soundfile %s already exists: cannot proceed further\n", filename);
				fflush(stdout);
			}
		}
		dz->iparam[CUT_CUT] = (int)dz->lparray[0][m];
		dz->iparam[CUT_END] = (int)dz->lparray[0][m+1];
		sndseekEx(dz->ifd[0],0,0);
		dz->total_samps_read = 0;
		memset((char *)dz->sampbuf[OBUF],0,dz->buflen * sizeof(float));
		memset((char *)dz->sampbuf[IBUF],0,dz->buflen * sizeof(float));
		if((exit_status = do_zcut(dz)) < 0) {
			fprintf(stdout,"INFO: FAILED TO MAKE EDIT %d\n",n+1);
			fflush(stdout);
			continue;
		} else {
			if((exit_status = headwrite(dz->ofd,dz))<0)
				return(exit_status);
			if((exit_status = reset_peak_finder(dz))<0)
				return(exit_status);
			if(sndcloseEx(dz->ofd)<0) {
				sprintf(errstr,"Failed to close output soundfile.\n");
				return(SYSTEM_ERROR);
			}
			dz->ofd = -1;
			last_fileout = outcnt;
			outcnt++;
			display_virtual_time(outcnt,dz);
		}
	}
	if(outcnt == 0) {
		sprintf(errstr,"NO EDITS MADE");
		return(GOAL_FAILED);
	}
	return FINISHED;
}
	
