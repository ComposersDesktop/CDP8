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
#include <string.h>
#include <structures.h>
#include <processno.h>
#include <tkglobals.h>
#include <mix.h>
#include <globcon.h>
#include <cdpmain.h>
#include <filetype.h>

#include <sfsys.h>

#ifdef unix
#define round(x) lround((x))
#endif

static int establish_action_value_storage_for_mix(dataptr dz);
static int open_file_and_get_props(int filecnt,char *filename,int *srate,int chans,dataptr dz);
static int allocate_actions(int filecnt,dataptr dz);
static int assign_stereo_sense(int chans,double lpan,double rpan,double llevel,double rlevel,dataptr dz);
static int unmark_freeable_bufs(int **bufsflag,int *bufsflagcnt,int startpos,int this,int longbitsize,dataptr dz);
static int get_free_buf(int **bufsflag,int *bufsflagcnt,int longbitsize,int *thisbuf,dataptr dz);
#ifdef NOTDEF
static int assign_scaling(int here,double llevel,double rlevel,double lpan,double rpan,
			int *llscale,int *rrscale,int *lrscale,int *rlscale,dataptr dz);
#else
static int d_assign_scaling(int here,double llevel,double rlevel,double lpan,double rpan,
double *llscale,double *rrscale,double *lrscale,double *rlscale,dataptr dz);
#endif
static int check_right_level(char *str,double *rlevel,int filecnt);
static int check_right_pan(char *str,double *pan,int filecnt);
static int check_left_pan(char *str,double *pan,int filecnt);
static int check_left_level(char *str,double *level,int filecnt);
static int unmark_buf(int **bufsflag,int *bufsflagcnt,int thisbuf,int longbitsize);
static int mark_buf(int **bufsflag,int *bufsflagcnt,int thisbuf,int longbitsize,dataptr dz);
static int reallocate_bufsflag(int z,int **bufsflag,int *bufsflagcnt);

/********************** SET_UP_MIX ****************************/
 
int set_up_mix(dataptr dz)
{
	int exit_status;
	double time, llevel, lpan, rlevel, rpan;
	int    chans;
	int   startpos, endpos, n;
	int *bufsflag = NULL;
	int bufsflagcnt = 0;
	double filetime, eventend;
	int longbitsize = sizeof(int) * CHARBITSIZE;
	int srate=0, end_of_mix = -1;
	int mix_end_set   = FALSE;
	int mix_end_specified = FALSE;
	int mix_start_set = FALSE;
	char *filename;
	int filecnt = 0;												
	int total_words = 0;

	if(dz->param[MIX_START] >= dz->param[MIX_END]) {
		sprintf(errstr,"Mix starts after it ends.\n");
		return(USER_ERROR);
	}
	dz->iparam[MIX_STRTPOS] 		  = 0;
	dz->iparam[MIX_STRTPOS_IN_ACTION] = 0;
	dz->iparam[MIX_TOTAL_ACTCNT]      = 0;
	if((exit_status = establish_file_data_storage_for_mix(dz->linecnt,dz))<0)
		return(exit_status);
	if((exit_status = establish_action_value_storage_for_mix(dz))<0)
		return(exit_status);
	for(n=0;n<dz->linecnt;n++) {						/* for each mixfile line */
		filename = dz->wordstor[total_words];			/* get the filename */
		if(strlen(filename)<=0) {
			sprintf(errstr,"filename error: line %d: parse_mixfile_for_data()\n",filecnt+1);
			return(PROGRAM_ERROR);
		}
		if((exit_status = get_mixdata_in_line			
		(dz->wordcnt[n],dz->wordstor,total_words,&time,&chans,&llevel,&lpan,&rlevel,&rpan,filecnt,dz))<0)
			return(exit_status);
		total_words += dz->wordcnt[n];
		if(time >= dz->param[MIX_END])					/* If line starts after specified mix end, ignore */
			continue;
		if((exit_status = finalise_and_check_mixdata_in_line(dz->wordcnt[n],chans,llevel,&lpan,&rlevel,&rpan))<0)
			return(exit_status);
		if((exit_status = open_file_and_get_props(filecnt,filename,&srate,chans,dz))<0)
			return(exit_status);
		filetime = (double)(dz->insams[filecnt]/chans)/(double)srate;
		if((eventend = time + filetime) < dz->param[MIX_START])	/* If line ends before specified mix start, ignore */
			continue;
		if(!mix_start_set) {								/* if mix start param not set yet, set it */
			dz->iparam[MIX_STRTPOS]           = (int)round(dz->param[MIX_START] * (double)(srate * dz->out_chans));
			dz->iparam[MIX_STRTPOS_IN_ACTION] = dz->iparam[MIX_STRTPOS];
			mix_start_set = TRUE;
		}
		if(!mix_end_set) {									/* if mix end param not set yet, set it */
			if(!flteq(dz->param[MIX_END],dz->application->default_val[MIX_END])) {	 /* if mixend param given by user */
				end_of_mix = round(dz->param[MIX_END] * (double)srate) * dz->out_chans;
				if(end_of_mix < 0) {
					sprintf(errstr,"Error in end_of_mix logic: parse_mixfile_for_data()\n");
					return(PROGRAM_ERROR);
				}
				mix_end_specified = TRUE;
			}
			mix_end_set = TRUE;
		}
		if((exit_status = allocate_actions(filecnt,dz))<0)	/* assign values to mix ON and OFF actions */
			return(exit_status);
		startpos = round(time * (double)srate) * chans;		/* startposition, in samples, of this file */
		dz->valstor[filecnt]->stereo  = assign_stereo_sense(chans,lpan,rpan,llevel,rlevel,dz);
		if(dz->valstor[filecnt]->stereo == MONO_TO_STEREO
		|| dz->valstor[filecnt]->stereo == MONO_TO_CENTRE
		|| dz->valstor[filecnt]->stereo == MONO_TO_LEFT
		|| dz->valstor[filecnt]->stereo == MONO_TO_RIGHT) {
			startpos             *= STEREO;					/* startposition, in output samples, of this file */
			dz->insams[filecnt]  *= STEREO;					/* EFFECTIVE sample-dur of file: it's converted to stereo */
		}
		if((exit_status = unmark_freeable_bufs				/* finds buffers which are not (any longer) in use */
		(&bufsflag,&bufsflagcnt,startpos,dz->iparam[MIX_TOTAL_ACTCNT],longbitsize,dz))<0)
			return(exit_status);
		dz->valstor[filecnt]->ifd  	= dz->ifd[filecnt];
		if((exit_status = get_free_buf(&bufsflag,&bufsflagcnt,longbitsize,&(dz->valstor[filecnt]->bufno),dz))<0) 	
			return(exit_status);	  						/* Allocate a buffer which is not currently in use */	
		if((exit_status = d_assign_scaling(dz->iparam[MIX_TOTAL_ACTCNT],llevel,rlevel,lpan,rpan,
		&(dz->valstor[filecnt]->llscale),&(dz->valstor[filecnt]->rrscale),
		&(dz->valstor[filecnt]->lrscale),&(dz->valstor[filecnt]->rlscale),dz))<0)
			return(exit_status);							/* Assign the input sound amplitude scaling */
		endpos =  startpos + dz->insams[filecnt];			/* Find end-of-current-file in output stream */
		if(mix_end_specified && (end_of_mix < endpos)) {	/* If file ends aftert mix ends */
			endpos  = end_of_mix;						 	/* curtail (effective) length */
			dz->insams[filecnt] = end_of_mix - startpos;
		}
		dz->valstor[filecnt]->samplen = dz->insams[filecnt]; 			/* store (effective) length */
		dz->act[dz->iparam[MIX_TOTAL_ACTCNT]++]->position = startpos;	/* store outputstream-position of mix start */
		dz->act[dz->iparam[MIX_TOTAL_ACTCNT]++]->position = endpos;		/* store outputstream-position of mix end   */
		filecnt++;											/* count the ACTUALLY USED lines */
	}
	if(!mix_end_set || filecnt==0) {
		sprintf(errstr,"No mixfile line is active within the time limits specified.\n");
		return(DATA_ERROR);
	}
	if(bufsflagcnt)
		free(bufsflag);
    dz->bufcnt++;		/* bufcnt is number assigned to highest assigned buf, COUNTING FROM ZERO */
						/* Hence, actual bufcnt is 1 more than this */

	dz->infile->channels = dz->out_chans;	/* output channels(evenutally derived from dz->infile->channels) */
//TW SET UP AFTER OUTCHANS KNOWN
	if(dz->process!=MIXMAX) {
		if((exit_status = create_sized_outfile(dz->outfilename,dz))<0)
			return(exit_status);
	}
	return(FINISHED);						/* is highest number of channels encountered in infiles */
}

/**************************** ESTABLISH_FILE_DATA_STORAGE_FOR_MIX ********************************/

int establish_file_data_storage_for_mix(int filecnt,dataptr dz)
{
	int n;
	if(dz->insams!=NULL)		
		free(dz->insams);	 	/* in TK insams[0] also used in parse accounting */
	if(dz->ifd!=NULL)
		free(dz->ifd);
	if((dz->insams = (int *)malloc(filecnt * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to allocate infile samplesize array.\n");
		return(MEMORY_ERROR);
	}
	if((dz->ifd = (int  *)malloc(filecnt * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to allocate infile pointers array.\n");		   	
		return(MEMORY_ERROR);
	}
	for(n=0;n<filecnt;n++)
		dz->ifd[n] = -1;
	return(FINISHED);
}

/**************************** ESTABLISH_ACTION_VALUE_STORAGE_FOR_MIX ********************************/

int establish_action_value_storage_for_mix(dataptr dz)
{
	int n;
	if((dz->valstor  = (actvalptr *)malloc(dz->linecnt * sizeof(actvalptr)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for mix action values store.\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<dz->linecnt;n++) {
		dz->valstor[n] = NULL;		
		if((dz->valstor[n]=(actvalptr)malloc(sizeof(struct actval)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for mix action value store %d\n",n+1);				
			return(MEMORY_ERROR);
		}
	}
	return(FINISHED);
}

/**************************** GET_MIXDATA_IN_LINE ********************************/

int get_mixdata_in_line(int wordcnt,char **wordstor,int total_words,double *time,int *chans,
double *llevel,double *lpan,double *rlevel,double *rpan,int filecnt,dataptr dz)
{
	int exit_status;
	switch(wordcnt) {
	case(MIX_MAXLINE):
		if((exit_status = check_right_level(wordstor[total_words+MIX_RLEVELPOS],rlevel,filecnt))<0)	/* 6d */
			return(exit_status);
		if((exit_status = check_right_pan(wordstor[total_words+MIX_RPANPOS],rpan,filecnt))<0)
			return(exit_status);
		/* fall thro */
	case(MIX_MIDLINE):
		if((exit_status = check_left_pan(wordstor[total_words+MIX_PANPOS],lpan,filecnt))<0)		/* 6c */
			return(exit_status);
		/* fall thro */
	case(MIX_MINLINE):
		if(sscanf(dz->wordstor[total_words+1],"%lf",time)!=1
		|| sscanf(dz->wordstor[total_words+2],"%d",chans)!=1) {
			sprintf(errstr,"Error scanning data: get_mixdata_in_line()\n");
			return(PROGRAM_ERROR);
		}
		if((exit_status = check_left_level(wordstor[total_words+3],llevel,filecnt))<0)
			return(exit_status);
		break;
	default:
		sprintf(errstr,"Illegal line length: get_mixdata_in_line()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/**************************** FINALISE_AND_CHECK_MIXDATA_IN_LINE ********************************/

int  finalise_and_check_mixdata_in_line(int wordcnt,int chans,double llevel,
					double *lpan,double *rlevel,double *rpan)
{
	switch(wordcnt) {
	case(MIX_MINLINE):
		switch(chans) {
		case(1): 
			*lpan = 0.0;						/* 6a */
			break;
		case(2): 
			*rlevel = llevel;				/* 6b */
			*lpan   = -1.0;
			*rpan   = 1.0;
		}
		break;
	case(MIX_MIDLINE):
		if(chans!=1) {
			sprintf(errstr,"Error parsing data: finalise_and_check_mixdata_in_line()\n");
			return(PROGRAM_ERROR);
		}
		break;
	case(MIX_MAXLINE):
		if(chans!=2) {
			sprintf(errstr,"Error parsing data: finalise_and_check_mixdata_in_line()\n");
			return(PROGRAM_ERROR);
		}
		break;
	}
	return(FINISHED);
}


/************************* OPEN_FILE_AND_GET_PROPS *******************************/

int open_file_and_get_props(int filecnt,char *filename,int *srate,int chans,dataptr dz)
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
	if((dz->ifd[filecnt] = sndopenEx(filename,0,CDP_OPEN_RDONLY)) < 0) {
		sprintf(errstr,"Failed to open sndfile %s: %s\n",filename,sferrstr());
		return(SYSTEM_ERROR);
	}
	if((exit_status = readhead(ifp,dz->ifd[filecnt],filename,&maxamp,&maxloc,&maxrep,getmax,getmaxinfo))<0)
		return(exit_status);
	copy_to_fileptr(ifp,dz->infile);
	if(dz->infile->filetype!=SNDFILE) {
		sprintf(errstr,"Non soundfile encountered [%s]: open_file_and_get_props()\n",filename);
		return(PROGRAM_ERROR);
	}
	if(dz->infile->channels!=chans) {
		sprintf(errstr,"Incorrect channels found [%s]: open_file_and_get_props()\n",filename);
		return(PROGRAM_ERROR);
	}
	if(filecnt==0)
		*srate = dz->infile->srate;
	else if(dz->infile->srate != *srate) {
		sprintf(errstr,"incompatible srates: [file %s] open_file_and_get_props()\n",filename);
		return(PROGRAM_ERROR);
	}
	if((dz->insams[filecnt] = sndsizeEx(dz->ifd[filecnt]))<0) {	    			
		sprintf(errstr, "Can't read size of input file %s: open_file_and_get_props()\n",filename);
		return(PROGRAM_ERROR);
	}
	if(dz->insams[filecnt] <=0) {
		sprintf(errstr, "Zero size for input file %s: open_file_and_get_props()\n",filename);
		return(PROGRAM_ERROR);
	}			
	return(FINISHED);
}

/*********************** ALLOCATE_ACTIONS *************************/

int allocate_actions(int filecnt,dataptr dz)
{
	int new_total = dz->iparam[MIX_TOTAL_ACTCNT] + 2;
	if(dz->iparam[MIX_TOTAL_ACTCNT]==0) {
		if((dz->act = (actptr *)malloc(new_total * sizeof(actptr)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for mix actions store.\n");
			return(MEMORY_ERROR);
		}
	} else {
		if((dz->act=(actptr *)realloc(dz->act,new_total * sizeof(actptr)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY ro reallocate mix actions store.\n");
			return(MEMORY_ERROR);
		}
	}
	if((dz->act[dz->iparam[MIX_TOTAL_ACTCNT]] = (actptr)malloc(sizeof(struct action)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for final mix action store.\n");
		return(MEMORY_ERROR);
	}
	dz->act[dz->iparam[MIX_TOTAL_ACTCNT]]->val    = dz->valstor[filecnt];
	dz->act[dz->iparam[MIX_TOTAL_ACTCNT]]->role   = MIX_ACTION_ON;
	if((dz->act[dz->iparam[MIX_TOTAL_ACTCNT]+1] = (actptr)malloc(sizeof(struct action)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for further mix action store.\n");
		return(MEMORY_ERROR);
	}
	dz->act[dz->iparam[MIX_TOTAL_ACTCNT]+1]->val  = dz->valstor[filecnt];
	dz->act[dz->iparam[MIX_TOTAL_ACTCNT]+1]->role = MIX_ACTION_OFF;
	return(FINISHED);
}

/********************* ASSIGN_STEREO_SENSE **************************/

int assign_stereo_sense(int chans,double lpan,double rpan,double llevel,double rlevel,dataptr dz)
{
	switch(chans) {
	case(MONO):
		if(dz->out_chans==MONO)		return(MONO);
		else if(lpan <= -1.0)	 	return(MONO_TO_LEFT);
		else if(lpan >= 1.0)		return(MONO_TO_RIGHT);
		else if(flteq(lpan,0.0))	return(MONO_TO_CENTRE);
		else						return(MONO_TO_STEREO);
		break;
	case(STEREO):
		if(flteq(rlevel,0.0)) {
			if(lpan <= -1.0)		return(STEREOLEFT_TO_LEFT);
			else if(lpan >= 1.0)	return(STEREOLEFT_TO_RIGHT);
			else					return(STEREOLEFT_PANNED);
		}
		else if(flteq(llevel,0.0)) {
			if(rpan <= -1.0)		return(STEREORIGHT_TO_LEFT);
			else if(rpan >= 1.0)	return(STEREORIGHT_TO_RIGHT);
			else					return(STEREORIGHT_PANNED);
		} else if(lpan <= -1.0 &&  rpan >= 1.0)								return(STEREO);
		else if(lpan >= 1.0 && rpan <= -1.0)								return(STEREO_MIRROR);
		else if(flteq(lpan,0.0) && flteq(rpan,0.0) && flteq(rlevel,llevel))	return(STEREO_TO_CENTRE);
		else if((lpan <= -1.0) && flteq(rpan,lpan) && flteq(rlevel,llevel))	return(STEREO_TO_LEFT);
		else if((rpan >= 1.0)  && flteq(rpan,lpan) && flteq(rlevel,llevel))	return(STEREO_TO_RIGHT);
		break;
	}
	return(STEREO_PANNED);
}

/************************* UNMARK_FREEABLE_BUFS ***********************
 *
 * (1)	If a buffer has been switched off BEFORE now, then it is
 *		available for use!! unmark it!!
 * (2)	If a buffer is subsequently turned back on, this catches it!!
 *		A buffer can ONLY be turned back on LATER (!), and is hence
 *		LATER in this list EVEN though it is not yet fully time-sorted!!  (1998 ???)
 */

int unmark_freeable_bufs(int **bufsflag,int *bufsflagcnt,int startpos,int this,int longbitsize,dataptr dz)
{
	int exit_status;
	int n;
	for(n=0;n<this;n++) {
		switch(dz->act[n]->role) {
		case(MIX_ACTION_ON):				/* 2 */
			if((exit_status = mark_buf(bufsflag,bufsflagcnt,dz->act[n]->val->bufno,longbitsize,dz))<0)
				return(exit_status);
			break;
		case(MIX_ACTION_OFF):				/* 1 */
			if(dz->act[n]->position < startpos)	{
				if((exit_status = unmark_buf(bufsflag,bufsflagcnt,dz->act[n]->val->bufno,longbitsize))<0)
					return(exit_status);
			}
			break;
		}
	}
	return(FINISHED);
}

/************************** GET_FREE_BUF ***************************
 *
 * Get the FIRST available free buffer.
 * (1)	Going through each int.
 * (2)	Set the mask to start of int.
 * (3)	For each position in the int..
 * (4)	If that byte is NOT set, break, thisbuf counts which byte
 *	and therefore which buffer it is.
 * (5)	Advance the mask.
 * (6)	Advance the buffer counter.
 * (7)	Set the appropriate bit for this buffer, return buffer no.
 */

int get_free_buf(int **bufsflag,int *bufsflagcnt,int longbitsize,int *thisbuf,dataptr dz)
{
	int exit_status=0;
	int y=0, z=0, got_it = 0;
	int mask=0;
	*thisbuf = 0;
	for(z=0;z<*bufsflagcnt;z++) {			/* 1 */
		mask = 1;	    					/* 2 */
		for(y=0;y<longbitsize;y++) {		/* 3 */
			if(!(mask & (*bufsflag)[z])) {	/* 4 */
				got_it = 1;
				break;
			}
			mask <<= 1;						/* 5 */
			(*thisbuf)++;					/* 6 */
		}
		if(got_it)
			break;
	}
	if((exit_status = mark_buf(bufsflag,bufsflagcnt,*thisbuf,longbitsize,dz))<0)	/* 7 */
		return(exit_status);
	return(FINISHED);
}

/*************************** ASSIGN_SCALING ***************************/
/*RWD gulp! */
#ifdef NOTDEF

int assign_scaling(int here,double llevel,double rlevel,double lpan,double rpan,
int *llscale,int *rrscale,int *lrscale,int *rlscale,dataptr dz)
{
	switch(dz->act[here]->val->stereo) {
	case(MONO):
		if(lpan < -1.0 - FLTERR)		*llscale = round(llevel * TWO_POW_15/(-lpan));
		else if(lpan > 1.0 + FLTERR)	*llscale = round(llevel * TWO_POW_15/lpan);
		else							*llscale = round(llevel * TWO_POW_15);
		break;
	case(MONO_TO_CENTRE):
	case(STEREO_TO_CENTRE):				*llscale = round(llevel * TWO_POW_15);
		break;
	case(MONO_TO_LEFT):
	case(STEREO_TO_LEFT):
	case(STEREOLEFT_TO_LEFT):
		if(lpan<(-1. - FLTERR))			*llscale = round(llevel * TWO_POW_15/(-lpan));
		else							*llscale = round(llevel * TWO_POW_15);
		break;
	case(STEREORIGHT_TO_LEFT):
		if(lpan<(-1. - FLTERR))			*llscale = round(rlevel * TWO_POW_15/(-lpan));
		else							*llscale = round(rlevel * TWO_POW_15);
		break;
	case(MONO_TO_RIGHT):
	case(STEREOLEFT_TO_RIGHT):
		if(lpan>(1. + FLTERR))			*rrscale = round(llevel * TWO_POW_15/(lpan));
		else 							*rrscale = round(llevel * TWO_POW_15);
		break;
	case(STEREO_TO_RIGHT):
	case(STEREORIGHT_TO_RIGHT):
		if(rpan>(1. + FLTERR))			*rrscale = round(rlevel * TWO_POW_15/rpan);
		else							*rrscale = round(rlevel * TWO_POW_15);
		break;
	case(STEREO):
		if(lpan < -1.0 - FLTERR)		*llscale = round(llevel * TWO_POW_15/(-lpan));
		else							*llscale = round(llevel * TWO_POW_15);

		if(rpan > 1.0 + FLTERR)			*rrscale = round(rlevel * TWO_POW_15/rpan);
		else							*rrscale = round(rlevel * TWO_POW_15);
		break;
	case(STEREO_MIRROR):
		if(lpan > 1.0 + FLTERR)			*llscale = round(llevel * TWO_POW_15/(lpan));
		else							*llscale = round(llevel * TWO_POW_15);

		if(rpan < -1.0 - FLTERR)		*rrscale = round(rlevel * TWO_POW_15/(-rpan));
		else							*rrscale = round(rlevel * TWO_POW_15);
		break;
	case(MONO_TO_STEREO):
	case(STEREOLEFT_PANNED):
		if(lpan < -1.0 - FLTERR) {	/* VERY LEFT */
			*llscale = round(llevel * TWO_POW_15/(-lpan));
			*rrscale = 0;
		} else if(lpan < 0.0) {		/* LEFTWARDS */
			*llscale = round(llevel * TWO_POW_15);
			*rrscale = round(llevel * (1.0+lpan) * TWO_POW_15);
		} else if(lpan <= 1.0) {	/* RIGHTWARDS */
			*llscale = round(llevel * (1.0-lpan) * TWO_POW_15);
			*rrscale = round(llevel * TWO_POW_15);
		} else {					/* VERY RIGHT */
			*llscale = 0;
			*rrscale = round(llevel * TWO_POW_15/(lpan));
		}
		break;
	case(STEREORIGHT_PANNED):
		if(rpan < -1.0) {			/* VERY LEFT */
			*llscale = round(rlevel * TWO_POW_15/(-rpan));
			*rrscale = 0;
		} else if(rpan < 0.0) {		/* LEFTWARDS */
			*llscale = round(rlevel * TWO_POW_15);
			*rrscale = round(rlevel * (1.0+rpan) * TWO_POW_15);
		} else if(rpan <= 1.0) {	/* RIGHTWARDS */
			*llscale = round(rlevel * (1.0-rpan) * TWO_POW_15);
			*rrscale = round(rlevel * TWO_POW_15);
		} else {					/* VERY RIGHT */
			*llscale = 0;
			*rrscale = round(rlevel * TWO_POW_15/rpan);
		}
		break;
	case(STEREO_PANNED):	
		/* LEFT STEREO INPUT PANNED ... */
		if(lpan < -1.0) {			/* VERY LEFT */
			*llscale = round(llevel * TWO_POW_15/(-lpan));
			*lrscale = 0;
		} else if(lpan < 0.0) {		/* LEFTWARDS */
			*llscale = round(llevel * TWO_POW_15);
			*lrscale = round(llevel * (1.0+lpan) * TWO_POW_15);
		} else if(lpan <= 1.0) {	/* RIGHTWARDS */
			*llscale = round(llevel * (1.0-lpan) * TWO_POW_15);
			*lrscale = round(llevel * TWO_POW_15);
		} else {					/* VERY RIGHT */
			*llscale = 0;
			*lrscale = round(llevel * TWO_POW_15/(lpan));
		}
		/* RIGHT STEREO INPUT PANNED ... */
		if(rpan < -1.0) {			/* VERY LEFT */
			*rlscale = round(rlevel * TWO_POW_15/(-rpan));
			*rrscale = 0;
		} else if(rpan < 0.0) {		/* LEFTWARDS */
			*rlscale = round(rlevel * TWO_POW_15);
			*rrscale = round(rlevel * (1.0+rpan) * TWO_POW_15);
		} else if(rpan <= 1.0) {	/* RIGHTWARDS */
			*rlscale = round(rlevel * (1.0-rpan) * TWO_POW_15);
			*rrscale = round(rlevel * TWO_POW_15);
		} else {					/* VERY RIGHT */
			*rlscale = 0;
			*rrscale = round(rlevel * TWO_POW_15/(rpan));
		}
		break;
	default:
		sprintf(errstr,"Unknown case in assign_scaling()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

#else

int d_assign_scaling(int here,double llevel,double rlevel,double lpan,double rpan,
double *llscale,double *rrscale,double *lrscale,double *rlscale,dataptr dz)
{
	switch(dz->act[here]->val->stereo) {
	case(MONO):
		if(lpan < -1.0 - FLTERR)		*llscale = llevel/(-lpan);
		else if(lpan > 1.0 + FLTERR)	*llscale = llevel/lpan;
		else							*llscale = llevel;
		break;
	case(MONO_TO_CENTRE):
	case(STEREO_TO_CENTRE):				*llscale = llevel;
		break;
	case(MONO_TO_LEFT):
	case(STEREO_TO_LEFT):
	case(STEREOLEFT_TO_LEFT):
		if(lpan<(-1. - FLTERR))			*llscale = llevel/(-lpan);
		else							*llscale = llevel;
		break;
	case(STEREORIGHT_TO_LEFT):
		if(lpan<(-1. - FLTERR))			*llscale = rlevel/(-lpan);
		else							*llscale = rlevel;
		break;
	case(MONO_TO_RIGHT):
	case(STEREOLEFT_TO_RIGHT):
		if(lpan>(1. + FLTERR))			*rrscale = llevel/lpan;
		else 							*rrscale = llevel;
		break;
	case(STEREO_TO_RIGHT):
	case(STEREORIGHT_TO_RIGHT):
		if(rpan>(1. + FLTERR))			*rrscale = rlevel/rpan;
		else							*rrscale = rlevel;
		break;
	case(STEREO):
		if(lpan < -1.0 - FLTERR)		*llscale = llevel/(-lpan);
		else							*llscale = llevel;

		if(rpan > 1.0 + FLTERR)			*rrscale = rlevel/rpan;
		else							*rrscale = rlevel;
		break;
	case(STEREO_MIRROR):
		if(lpan > 1.0 + FLTERR)			*llscale = llevel/lpan;
		else							*llscale = llevel;

		if(rpan < -1.0 - FLTERR)		*rrscale = rlevel/(-rpan);
		else							*rrscale = rlevel;
		break;
	case(MONO_TO_STEREO):
	case(STEREOLEFT_PANNED):
		if(lpan < -1.0 - FLTERR) {	/* VERY LEFT */
			*llscale = llevel/(-lpan);
			*rrscale = 0.0;
		} else if(lpan < 0.0) {		/* LEFTWARDS */
			*llscale = llevel;
			*rrscale = llevel * (1.0+lpan);
		} else if(lpan <= 1.0) {	/* RIGHTWARDS */
			*llscale = llevel * (1.0-lpan);
			*rrscale = llevel;
		} else {					/* VERY RIGHT */
			*llscale = 0.0;
			*rrscale = llevel/lpan;
		}
		break;
	case(STEREORIGHT_PANNED):
		if(rpan < -1.0) {			/* VERY LEFT */
			*llscale = rlevel/(-rpan);
			*rrscale = 0.0;
		} else if(rpan < 0.0) {		/* LEFTWARDS */
			*llscale = rlevel;
			*rrscale = rlevel * (1.0+rpan);
		} else if(rpan <= 1.0) {	/* RIGHTWARDS */
			*llscale = rlevel * (1.0-rpan);
			*rrscale = rlevel;
		} else {					/* VERY RIGHT */
			*llscale = 0.0;
			*rrscale = rlevel/rpan;
		}
		break;
	case(STEREO_PANNED):	
		/* LEFT STEREO INPUT PANNED ... */
		if(lpan < -1.0) {			/* VERY LEFT */
			*llscale = llevel /(-lpan);
			*lrscale = 0.0;
		} else if(lpan < 0.0) {		/* LEFTWARDS */
			*llscale = llevel;
			*lrscale = llevel * (1.0+lpan);
		} else if(lpan <= 1.0) {	/* RIGHTWARDS */
			*llscale = llevel * (1.0-lpan);
			*lrscale = llevel;
		} else {					/* VERY RIGHT */
			*llscale = 0.0;
			*lrscale = llevel/lpan;
		}
		/* RIGHT STEREO INPUT PANNED ... */
		if(rpan < -1.0) {			/* VERY LEFT */
			*rlscale = rlevel/(-rpan);
			*rrscale = 0.0;
		} else if(rpan < 0.0) {		/* LEFTWARDS */
			*rlscale = rlevel;
			*rrscale = rlevel * (1.0+rpan);
		} else if(rpan <= 1.0) {	/* RIGHTWARDS */
			*rlscale = rlevel * (1.0-rpan);
			*rrscale = rlevel ;
		} else {					/* VERY RIGHT */
			*rlscale = 0.0;
			*rrscale = rlevel/rpan;
		}
		break;
	default:
		sprintf(errstr,"Unknown case in assign_scaling()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

#endif



/************************** CHECK_RIGHT_LEVEL ************************/

int check_right_level(char *str,double *rlevel,int filecnt)
{
	if(is_dB(str)) {
		if(!get_leveldb(str,rlevel)) {
			sprintf(errstr,"Error1 scanning chan2 level: line %d\n",filecnt+1);
			return(PROGRAM_ERROR);
		}		
	} else {
		if(sscanf(str,"%lf",rlevel)!=1) {
			sprintf(errstr,"Error2 scanning chan2 level: line %d\n",filecnt+1);
			return(PROGRAM_ERROR);
		}
	}
	if(*rlevel < 0.0) {
		sprintf(errstr,"Error3 scanning chan2 level: line %d\n",filecnt+1);
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/*********************** CHECK_RIGHT_PAN **********************/

int check_right_pan(char *str,double *pan,int filecnt)
{
	switch(str[0]) {
	case('L'): *pan = -1.0; break;
	case('R'): *pan = 1.0; break;
	case('C'): *pan = 0.5; break;
	default:
		if(sscanf(str,"%lf",pan)!=1) {
			sprintf(errstr,"Error1 scanning chan2 pan: line %d\n",filecnt+1);
			return(PROGRAM_ERROR);
		}
		if(*pan < MINPAN || *pan > MAXPAN)  {
			sprintf(errstr,"Error2 scanning chan2 pan: line %d\n",filecnt+1);
			return(PROGRAM_ERROR);
		}
		break;
	}
	return(FINISHED);
}

/************************** CHECK_LEFT_PAN ************************/

int check_left_pan(char *str,double *pan,int filecnt)
{
	switch(str[0]) {
	case('L'): *pan = -1.0; break;
	case('R'): *pan = 1.0; break;
	case('C'): *pan = 0.0; break;
	default:
		if(sscanf(str,"%lf",pan)!=1) {
			sprintf(errstr,"Error1 scanning (chan1) pan: line %d\n",filecnt+1);
			return(PROGRAM_ERROR);
		}
		if(*pan < MINPAN || *pan > MAXPAN)  {
			sprintf(errstr,"Error2 scanning (chan1) pan: line %d\n",filecnt+1);
			return(PROGRAM_ERROR);
		}
	}
	return(FINISHED);
}

/*************** CHECK_LEFT_LEVEL ******************/

int check_left_level(char *str,double *level,int filecnt)
{
	if(is_dB(str)) {
		if(!get_leveldb(str,level)) {
			sprintf(errstr,"Error1 scanning (chan1) level: line %d\n",filecnt+1);
			return(PROGRAM_ERROR);
		}		
	} else {
		if(sscanf(str,"%lf",level)!=1) {
			sprintf(errstr,"Error2 scanning (chan1) level: line %d\n",filecnt+1);
			return(PROGRAM_ERROR);
		}
	}
	if(*level < 0.0) {
		sprintf(errstr,"Error3 scanning (chan1) level: line %d\n",filecnt+1);
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/************************** UNMARK_BUF ***************************/

int unmark_buf(int **bufsflag,int *bufsflagcnt,int thisbuf,int longbitsize)
{
	int exit_status;
	int mask = 1;	    
	int z = thisbuf/longbitsize;
/* 1998 --> */
	if(z >= *bufsflagcnt) {
		if((exit_status = reallocate_bufsflag(z,bufsflag,bufsflagcnt))<0)
			return(exit_status);
	}
/* <-- 1998 */
	thisbuf -= (z * longbitsize);
	mask <<= thisbuf;
	mask = ~mask;
	(*bufsflag)[z] &= mask;
	return(FINISHED);
}

/************************** MARK_BUF ***************************
 *
 * (1)	Which int is the appropriate byte in.
 * (2)	What is it's index into this int.
 * (3)	Move mask along the int.
 * (4)	Set bit.
 * (5)	Keep note of max number of bufs in use.
 */

int mark_buf(int **bufsflag,int *bufsflagcnt,int thisbuf,int longbitsize,dataptr dz)
{
	int exit_status;
	int mask = 1;    
	int z = thisbuf/longbitsize;	/* 1 */
/* 1998 --> */
	if(z >= *bufsflagcnt) {
		if((exit_status = reallocate_bufsflag(z,bufsflag,bufsflagcnt))<0)
			return(exit_status);
	}
/* <-- 1998 */
	if(thisbuf > dz->bufcnt)
		dz->bufcnt = thisbuf;
	thisbuf -= (z * longbitsize);	/* 2 */
	mask <<= thisbuf;				/* 3 */
	(*bufsflag)[z] |= mask;			/* 4 */
	return(FINISHED);
}

/**************************** REALLOCATE_BUFSFLAG ********************************/

int reallocate_bufsflag(int z,int **bufsflag,int *bufsflagcnt)
{
	int n;
	if(*bufsflagcnt==0) 
	//	*bufsflag = (int *) malloc((z+1) * sizeof(int));
	    *bufsflag = (int *) calloc((z+1),sizeof(int));  //RWD 2013 needs init values
	else if((*bufsflag = (int *)realloc(*bufsflag,(z+1) * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for buffer flags store.\n");
		return(MEMORY_ERROR);
	}
	for(n=*bufsflagcnt;n<z;n++) 
		*bufsflag[n] = 0;		
	*bufsflagcnt = z+1;
	return(FINISHED);
}

/**************************** MIX_MODEL ********************************/

int mix_model(dataptr dz)
{
	int n, m, k;
	int total_words = 0,filestart;
	char temp[200];
	filestart = dz->itemcnt;
	for(n=0,k=filestart;n<dz->linecnt;n++,k++) {
		for(m = 0;m < dz->wordcnt[n];m++) {
			if(m==0)
				strcpy(temp,dz->wordstor[k]);
			else {
				strcat(temp," ");
				strcat(temp,dz->wordstor[total_words]);
			}
			total_words++;
		}
		strcat(temp,"\n");
		if(fputs(temp,dz->fp)<0) {
			sprintf(errstr,"Failed to print new mixfile line %d to output file.\n",n+1);
			return(SYSTEM_ERROR);
		}
	}
	return FINISHED;
}
