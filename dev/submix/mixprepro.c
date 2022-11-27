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
#include <limits.h>
#include <structures.h>
#include <tkglobals.h>
#include <globcon.h>
#include <filetype.h>
#include <modeno.h>
#include <mix.h>
#include <sfsys.h>

#ifdef unix
#define round(x) lround((x))
#endif

static int  do_refill_acts(dataptr dz);
static void sort_actions(dataptr dz);
static int  init_inbufs(dataptr dz);
static int  gen_mcr_table(dataptr dz);

/************************** MIX_PREPROCESS ******************/

int mix_preprocess(dataptr dz)
{
	int exit_status;
	if((exit_status = do_refill_acts(dz))<0)						
		return(exit_status);
	sort_actions(dz);			
	if(dz->iparam[MIX_STRTPOS] > 0)	   		/* Allow for mix not starting at zero time */
		dz->iparam[MIX_STRTPOS_IN_ACTION] = (int)max(dz->iparam[MIX_STRTPOS] - dz->act[0]->position,0L);  
/* NEW 2000 */
	/* RWD now in samps */
	dz->tempsize = (dz->act[dz->iparam[MIX_TOTAL_ACTCNT]-1]->position - dz->iparam[MIX_STRTPOS_IN_ACTION]);
/* NEW 2000 */
	return init_inbufs(dz);		
}

/*************************** DO_REFILL_ACTS **************************/

int do_refill_acts(dataptr dz)
{
	int n;
	int samps_used, actcnt = dz->iparam[MIX_TOTAL_ACTCNT];
	int arraysize = actcnt + BIGARRAY;							/* Generate more space for further actions. */
	if((dz->act=(actptr *)realloc(dz->act,arraysize * sizeof(actptr)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to construct buffer-fill action pointers.\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<dz->iparam[MIX_TOTAL_ACTCNT];n+=2) {				/* actions paired as ON/OFF, so we look only at ON bufs */
		if(dz->act[n]->val->samplen > dz->buflen) {				/* If more data in (action's) file than fits in 1 buffer */
			samps_used = 0;		
			while((samps_used += dz->buflen) < dz->act[n]->val->samplen) {
				if((dz->act[actcnt] = (actptr)malloc(sizeof(struct action)))==NULL) {
					sprintf(errstr,"INSUFFICIENT MEMORY to construct buffer-fill actions.\n");
					return(MEMORY_ERROR);
				}
				dz->act[actcnt]->val      = dz->act[n]->val;	/* Create a new action, using same vals as the original */
				dz->act[actcnt]->position = dz->act[n]->position + samps_used;/* Positioned 1 further buflen later, and */
				dz->act[actcnt]->role     = MIX_ACTION_REFILL;	/* with role REFILL */
				if(++actcnt >= arraysize) {
					arraysize += BIGARRAY;
					if((dz->act=(actptr *)realloc(dz->act,arraysize * sizeof(actptr)))==NULL) {
						sprintf(errstr,"INSUFFICIENT MEMORY to reallocate buffer-fill action pointers.\n");
						return(MEMORY_ERROR);
					}
				}
			}
		}
	}
	if((dz->act=(actptr *)realloc(dz->act,actcnt * sizeof(actptr)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to reallocate buffer-fill actions.\n");
		return(MEMORY_ERROR);
	}
	dz->iparam[MIX_TOTAL_ACTCNT] = (int)actcnt;
	return(FINISHED);
}

/************************* SORT_ACTIONS ****************************/

void sort_actions(dataptr dz)
{
	actptr temp;   
	int n, m;
	for(n=0;n<dz->iparam[MIX_TOTAL_ACTCNT]-1;n++) {
		for(m=n+1;m<dz->iparam[MIX_TOTAL_ACTCNT];m++) {
			if(dz->act[m]->position < dz->act[n]->position) {
				temp   = dz->act[n];
				dz->act[n] = dz->act[m];	
				dz->act[m] = temp;
			}
		}
	}
}

/****************************** INIT_INBUFS *********************************
 *
 * (1)	Create the space for buffers within each bufitem. NB enough space
 * 		for ALL these buffers must be allocated in the initial creation
 * 		of 'inbuf' in create_buffers.
 * (2)	Create the pointers to point to start and current_position in ACTIVE bufs.
 */

int init_inbufs(dataptr dz)
{
	int n;
	float *thisbuf = dz->sampbuf[IBUFMIX];
	dz->buflist = (mixbufptr *)malloc(dz->bufcnt * sizeof(mixbufptr));
	for(n=0;n<dz->bufcnt;n++) {
		dz->buflist[n] = NULL;
		if((dz->buflist[n] = (mixbufptr)malloc(sizeof(struct bufitem)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for buffer list.\n");
			return(MEMORY_ERROR);
		}
		dz->buflist[n]->status = MIX_ACTION_OFF;
		dz->buflist[n]->buf    = thisbuf;
		dz->buflist[n]->here   = dz->buflist[n]->buf;
		thisbuf += dz->buflen;
	}
	if((dz->activebuf = (int *)malloc(dz->bufcnt * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for active buffer list.\n");
		return(MEMORY_ERROR);
	}
	if((dz->activebuf_ptr = (float **)malloc(dz->bufcnt * sizeof(float *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for active buffer pointer list.\n");
		return(MEMORY_ERROR);
	}
	return(FINISHED);
}

/****************************** MIXTWO_PREPROCESS ************************************/

int mixtwo_preprocess(dataptr dz)
{
	int shsecsize = F_SECSIZE * dz->infile->channels;
	double k;
	dz->iparam[MIX_STAGGER] = round(dz->param[MIX_STAGGER] * (double)dz->infile->srate) * dz->infile->channels;
	dz->iparam[MIX_STAGGER] = (dz->iparam[MIX_STAGGER]/shsecsize) * shsecsize;
	dz->iparam[MIX_STAGGER] = min(dz->iparam[MIX_STAGGER],dz->insams[0] - dz->infile->channels);

	dz->iparam[MIX_SKIP] = round(dz->param[MIX_SKIP] * (double)dz->infile->srate) * dz->infile->channels;
	dz->iparam[MIX_SKIP] = (dz->iparam[MIX_SKIP]/shsecsize) * shsecsize;
	dz->iparam[MIX_SKIP] = min(dz->iparam[MIX_SKIP],dz->insams[1] - dz->infile->channels);
	
	if(((k = (dz->param[MIX_STTA] * (double)dz->infile->srate))  * (double)dz->infile->channels) >= (double)LONG_MAX)
		dz->iparam[MIX_STTA] = INT_MAX;
	else
		dz->iparam[MIX_STTA] = round(dz->param[MIX_STTA] * (double)dz->infile->srate) * dz->infile->channels;
	dz->iparam[MIX_STTA] = (dz->iparam[MIX_STTA]/shsecsize) * shsecsize;
	dz->param[MIX2_GAIN1] = dz->param[MIX_SKEW]/(dz->param[MIX_SKEW] + 1.0);
	dz->param[MIX2_GAIN2] = 1.0 - dz->param[MIX2_GAIN1];

	if(((k = (dz->param[MIX_DURA] * (double)dz->infile->srate))  * (double)dz->infile->channels) >= (double)LONG_MAX)
		dz->iparam[MIX_DURA] = dz->insams[0] + dz->insams[1];
	else
		dz->iparam[MIX_DURA] = round(k) * dz->infile->channels;

	if(dz->iparam[MIX_DURA] <= dz->iparam[MIX_STAGGER]) {
		sprintf(errstr,"Mix cuts off before 2nd file enters\n");
		return(DATA_ERROR);
	}
	if(dz->iparam[MIX_STTA] >= dz->insams[0]) {
		sprintf(errstr,"Mix does not start until first file has ended.\n");
		return(DATA_ERROR);
	}
	if(dz->iparam[MIX_DURA] <= dz->iparam[MIX_STTA]) {
		sprintf(errstr,"Mix end is before mix start.\n");
		return(DATA_ERROR);
	}
	/* RWD all now in samps */
	dz->tempsize = max((dz->insams[1]-dz->iparam[MIX_SKIP]+dz->iparam[MIX_STAGGER]),dz->insams[0]);
	dz->tempsize = min(dz->tempsize,dz->iparam[MIX_DURA]);
	dz->tempsize -= (dz->iparam[MIX_STTA]);

	return(FINISHED);
}

/********************** MIXCROSS_PREPROCESS **********************/

int mixcross_preprocess(dataptr dz)
{
	int exit_status;
	int crosfact, maxend;

		   	/* CONVERT TO SAMPLES */
	dz->iparam[MCR_BEGIN]   = round(dz->param[MCR_BEGIN]   * dz->infile->srate) * dz->infile->channels;
		
		   	/* CONVERT TO SAMPLES AND APPROXIMATE TO SECTOR BOUNDARY */
	dz->iparam[MCR_STAGGER] = round(dz->param[MCR_STAGGER] * dz->infile->srate) * dz->infile->channels;
//TW SAFE ??
//#ifdef NOTDEF
//	dz->iparam[MCR_STAGGER] = (dz->iparam[MCR_STAGGER]/SECSIZE) * SECSIZE;
//#endif
	maxend = min(dz->insams[0],dz->iparam[MCR_STAGGER] + dz->insams[1]);

	if(dz->param[MCR_END] > 0.0) {		/* i.e. HAS BEEN SET */
		dz->iparam[MCR_END] = round(dz->param[MCR_END] * dz->infile->srate) * dz->infile->channels;
		dz->iparam[MCR_END] = min(dz->iparam[MCR_END],maxend);	/* END may be curtailed by stagger's rounding */
	} else
		dz->iparam[MCR_END] = maxend;

			/* TEST CROSSFADE PARAMS FOR CONSISTENCY */
	if(dz->iparam[MCR_BEGIN] < dz->iparam[MCR_STAGGER]) {
		sprintf(errstr,"Crossfade begins before end of stagger: Impossible.\n");
		return(USER_ERROR);
	}
	if(dz->iparam[MCR_BEGIN] >=  dz->iparam[MCR_STAGGER] + dz->insams[1]) {
		sprintf(errstr,"Crossfade begins after end of 2nd file: Impossible.\n");
		return(USER_ERROR);
	}
	if((crosfact = dz->iparam[MCR_END] - dz->iparam[MCR_BEGIN])<=0) {
		sprintf(errstr,"Crossfade length is zero or negative: Impossible.\n");
		return(USER_ERROR);
	}
			/* SET UP LENGTH OF CROSSFADE, AND THE DIVIDE FACTOR FOR INDEXING THIS */
	dz->param[MCR_CROSFACT] = 1.0/(double)crosfact;

			/* SET CONTOUR TYPE */
	if(dz->mode==MCLIN)
		dz->iparam[MCR_CONTOUR] = MCR_LINEAR;
	else if(flteq(dz->param[MCR_POWFAC],1.0))
		dz->iparam[MCR_CONTOUR] = MCR_COSIN;
	else
		dz->iparam[MCR_CONTOUR] = MCR_SKEWED;

			/* FOR COSIN CONTOUR TYPE : MAKE COSTABLE */
	if(dz->iparam[MCR_CONTOUR]==MCR_SKEWED || dz->iparam[MCR_CONTOUR]==MCR_COSIN) {
		if((exit_status = gen_mcr_table(dz))<0)
			return(exit_status);
	}
			/* SET CROSS-FADE INDEX OT ZERO */
	dz->iparam[MCR_INDEX]   = 0;
	return(FINISHED);
}

/*** GONE TO TK *********************** GEN_MCR_TABLE **********************/

int gen_mcr_table(dataptr dz)
{
	int n;
	double *costable;
	if((dz->parray[MCR_COSTABLE] = (double *)malloc((MCR_TABLEN+1)*	sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for cosine table.\n");
		return(MEMORY_ERROR);
	}
	costable = dz->parray[MCR_COSTABLE];
	for(n=0;n<MCR_TABLEN;n++) {
		costable[n]  = ((double)n/(double)(MCR_TABLEN)) * PI;
		costable[n]  = cos(costable[n]);
		costable[n] += 1.0;
		costable[n] /= 2.0;
		costable[n]  = 1.0 - costable[n];
		costable[n]  = max(0.0,costable[n]);
		costable[n]  = min(costable[n],1.0);
	}
	costable[n]  = 1.0;
	return(FINISHED);
}

/******************************** GET_INBETWEEN_RATIOS ***************************/

int get_inbetween_ratios(dataptr dz)
{
	double *p, scaler;
	int n;
	switch(dz->mode) {
	case(INBI_RATIO):	/* now read as special data */
		break;
	case(INBI_COUNT):	/* GENERATE RATIOS AUTOMATICALLY IN A NEWLY MALLOCD brkpnt TABLE */
		dz->iparam[INBETW] = round(dz->param[INBETW]);
		if((dz->brk[INBETW] = (double *)malloc(dz->iparam[INBETW] * sizeof(double)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for inbetween values store.\n");
			return(MEMORY_ERROR);
		}
		dz->no_brk[INBETW] = FALSE;
		p = dz->brk[INBETW];
		scaler = 1.0/(double)(dz->iparam[INBETW]+1);
		for(n=1;n<=dz->iparam[INBETW];n++)
			*p++ = (double)n * scaler;
		break;
	default:
		sprintf(errstr,"Unknown mode in get_inbetween_ratios()\n");
		return(PROGRAM_ERROR);
	}			
	return(FINISHED);
}

/************** CHECK_NEW_FILENAME ***********/

int check_new_filename(char *filename,dataptr dz)
{
	int exit_status;
	double maxamp, maxloc;
	int maxrep;
	int getmax = 0, getmaxinfo = 0;
	infileptr ifp;
	if((ifp = (infileptr)malloc(sizeof(struct filedata)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store data on files.\n");
		return(MEMORY_ERROR);
	}
	if(dz->mode!=MSH_DUPL_AND_RENAME || dz->vflag[MSH_NOCHECK])
		return(FINISHED);
	if(dz->infile->srate==0) {
		sprintf(errstr,"srate not set: check_new_filename()\n");
		return(PROGRAM_ERROR);
	}
	if(dz->ifd!=NULL)  /* set up by default in main() */
		free(dz->ifd);
	if((exit_status = establish_file_data_storage_for_mix((int)1,dz))<0)
		return(exit_status);

	if((dz->ifd[0] = sndopenEx(filename,0,CDP_OPEN_RDONLY)) < 0) {
		sprintf(errstr,"Failed to open sndfile %s: check_new_filename()\n",filename);
		return(DATA_ERROR);
	}
	if((exit_status = readhead(ifp,dz->ifd[0],filename,&maxamp,&maxloc,&maxrep,getmax,getmaxinfo))<0)
		return(exit_status);
	copy_to_fileptr(ifp,dz->otherfile);
	if(dz->otherfile->filetype!=SNDFILE) {
		sprintf(errstr,"%s is not a soundfile: check_new_filename()\n",filename);
		return(DATA_ERROR);
	}
	if(dz->otherfile->srate != dz->infile->srate) {
		sprintf(errstr,"Incompatible srate: check_new_filename()\n");
		return(DATA_ERROR);
	}
	return(FINISHED);
}
