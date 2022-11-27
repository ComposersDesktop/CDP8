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



/* floatsam version: no changes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <structures.h>
#include <tkglobals.h>
#include <globcon.h>
#include <processno.h>
#include <modeno.h>
#include <arrays.h>
#include <mix.h>
#include <cdpmain.h>

#include <sfsys.h>
#include <osbind.h>

static int  get_times(double *timestor,dataptr dz);
static int  adjust_stereo(int lineno,int total_words,int wordcnt,double param,dataptr dz);
static int  adjust_position(char **thisword,double param,dataptr dz);
static int  treat_short_mono_line(int lineno,int wordno,double param,dataptr dz);
static int  treat_short_stereo_line(int lineno,int wordno,double param,dataptr dz);
static int  investigate_timings(double *duration,double *min_time,dataptr dz);

/************************* MIX_SHUFL ******************************/

int mix_shufl(dataptr dz)
{
	switch(dz->mode) {
	case(MSH_OMIT_ALT):
	case(MSH_OMIT):				return do_time_manip(dz);

	case(MSH_DUPLICATE):
	case(MSH_DUPL_AND_RENAME):	return do_time_and_name_copy(dz);

	case(MSH_SCATTER):			return randomise_names(dz);
	case(MSH_REVERSE_N):		return do_name_reverse(dz);
	case(MSH_FIXED_N):			return do_name_freeze(dz);
	default:
		sprintf(errstr,"Unknown mode in mix_shufl()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);	/* NOTREACHED */
}

/************************* MIX_TIMEWARP ******************************/

int mix_timewarp(dataptr dz)
{
	switch(dz->mode) {
	case(MTW_REVERSE_NT):	return do_name_and_time_reverse(dz);
	case(MTW_FREEZE_NT): 	return do_name_and_time_freeze(dz);

	case(MTW_REVERSE_T):	case(MTW_FREEZE_T):		case(MTW_SCATTER):		case(MTW_DOMINO):
	case(MTW_CREATE_TG_1):	case(MTW_CREATE_TG_2):	case(MTW_CREATE_TG_3):	case(MTW_CREATE_TG_4):
	case(MTW_ENLARGE_TG_1):	case(MTW_ENLARGE_TG_2):	case(MTW_ENLARGE_TG_3):	case(MTW_ENLARGE_TG_4):
	case(MTW_ADD_TO_TG):	case(MTW_TIMESORT):
		return do_time_manip(dz);
	default:
		sprintf(errstr,"Unknown mode in mix_timewarp()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);	/* NOTREACHED */
}

/************************* MIX_SPACEWARP ******************************/

int mix_spacewarp(dataptr dz)
{
	int    exit_status;
	int   n, total_words;
	int    is_even = TRUE;
	double position = 0.0;
	int    *maxwordsize;
	double duration = 0.0, min_time = 0.0, *timestor = NULL;
	double spacerange = 0.0, leftedge = 0.0;
	switch(dz->mode) {
	case(MSW_TWISTALL):
	case(MSW_TWISTONE):
		return mix_twisted(dz);
	case(MSW_LEFTWARDS):
	case(MSW_RIGHTWARDS):
		if((exit_status = investigate_timings(&duration,&min_time,dz))<0)
			return(exit_status);
		timestor = dz->parray[MSW_TIMESTOR];
		/* fall thro */
	case(MSW_RANDOM):
	case(MSW_RANDOM_ALT):
		spacerange = fabs(dz->param[MSW_POS2] - dz->param[MSW_POS1]);
		leftedge   = min(dz->param[MSW_POS1],dz->param[MSW_POS2]);
		break;
	case(MSW_NARROWED):	
	case(MSW_FIXED):	
		break;
	default:
		sprintf(errstr,"Unknown mode in mix_spacewarp()\n");
		return(PROGRAM_ERROR);
	}
	total_words = 0;
	for(n=0;n<dz->iparam[MSH_STARTLINE];n++)
		total_words += dz->wordcnt[n];
	for(n=dz->iparam[MSH_STARTLINE];n<dz->iparam[MSH_ENDLINE];n++) {
		switch(dz->mode) {
		case(MSW_NARROWED):	
			if((exit_status = adjust_stereo(n,total_words,dz->wordcnt[n],dz->param[MSW_NARROWING],dz))<0)
				return(exit_status);
			break;
		case(MSW_FIXED):	
			if((exit_status = adjust_stereo(n,total_words,dz->wordcnt[n],dz->param[MSW_POS1],dz))<0)
				return(exit_status);
			break;
		default:
			switch(dz->mode) {
			case(MSW_RANDOM):
				position  = (drand48() * POSITIONS)/POSITIONS;
				break;
			case(MSW_RANDOM_ALT):
				switch(is_even) {
				case(TRUE):	position = (drand48() * HALF_POSTNS)/POSITIONS; 				break;	/* Force Left */	
				case(FALSE):position = (POSITIONS - (drand48() * HALF_POSTNS))/POSITIONS;	break;	/* Force Right */	
				}
				is_even = !is_even;
				break;
			case(MSW_LEFTWARDS):
//TW UPDATE
				if(duration > FLTERR)
					position = (timestor[n] - min_time)/duration;
				else {
					sprintf(errstr,"Files do not Progress in Time : Hence cannot MOVE Leftwards\n");
					return(USER_ERROR);
				}
				position = 1.0 - position;
				break;
			case(MSW_RIGHTWARDS):
//TW UPDATE
				if(duration > FLTERR)
					position  = (timestor[n] - min_time)/duration;
				else {
					sprintf(errstr,"Files do not Progress in Time : Hence cannot MOVE Rightwards\n");
					return(USER_ERROR);
				}
				break;
			}
			position *= spacerange;
			position += leftedge;
			if((exit_status = adjust_stereo(n,total_words,dz->wordcnt[n],position,dz))<0)
				return(exit_status);
			break;
		}
		total_words += dz->wordcnt[n];
	}
	if((exit_status = get_maxwordsize(&maxwordsize,dz))<0)
		return(exit_status);
	if((exit_status = output_mixfile_lines(maxwordsize,dz))<0)
		return(exit_status);
	free(maxwordsize);
	return(FINISHED);
}

/*************************** GET_TIMES ***************************/

int get_times(double *timestor,dataptr dz)
{
	int n, timeloc;
	int total_wordcnt = 0;
	for(n=0;n<dz->linecnt;n++) {
		if(dz->wordcnt[n] < 2) {
			sprintf(errstr,"PRoblem getting line times: get_times_and_timediffs()\n");
			return(PROGRAM_ERROR);
		}
		timeloc = total_wordcnt + MIX_TIMEPOS;
		if(sscanf(dz->wordstor[timeloc],"%lf",&(timestor[n]))!=1) {
			sprintf(errstr,"Problem reading time: get_times_and_timediffs()\n");
			return(PROGRAM_ERROR);
		}
		total_wordcnt += dz->wordcnt[n];
	}
	return(FINISHED);
}

/************************** ADJUST_STEREO **************************/

int adjust_stereo(int lineno,int total_words,int wordcnt,double param,dataptr dz)
{
	int  exit_status;
	char *thisword;
	int  chans;
	int levelwordno = total_words + MIX_LEVELPOS;
	int lpanwordno  = total_words + MIX_PANPOS;
	int rpanwordno  = total_words + MIX_RPANPOS;
	int chanwordno  = total_words + MIX_CHANPOS;
	switch(wordcnt) {
	case(MIX_MINLINE):
		thisword = dz->wordstor[chanwordno];
		if(sscanf(thisword,"%d",&chans)!=1) {
			sprintf(errstr,"Failed to get channel count: adjust_stereo()\n");
			return(PROGRAM_ERROR);
		}
		switch(chans) {
		case(MONO):				
			switch(dz->mode) {
			case(MSW_NARROWED):	return(FINISHED);	/* mono,   unpanned, files can't be narrowed */
			default:  			return treat_short_mono_line(lineno,levelwordno,param,dz);
			}					/* mono,   unpanned, wordstor has to be extended to take 1 new vals */
			break;				/* stereo, unpanned, wordstor has to be extended to take 3 new vals */
		case(STEREO):			return treat_short_stereo_line(lineno,levelwordno,param,dz);
		default:
			sprintf(errstr,"Invalid channel count: adjust_stereo()\n");
			return(PROGRAM_ERROR);
		}
		break; 
	case(MIX_MAXLINE):
		if((exit_status = adjust_position(&(dz->wordstor[rpanwordno]),param,dz))<0)
			return(exit_status);
		/* fall thro */
	case(MIX_MIDLINE):
		return adjust_position(&(dz->wordstor[lpanwordno]),param,dz);
		break;
	default:
		sprintf(errstr,"Impossible wordcnt: adjust_stereo()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);	/* NOTREACHED */
}

/************************* ADJUST_POSITION ******************************/

int adjust_position(char **thisword,double param,dataptr dz)
{
	double position;
	int    newlen;
	if(dz->mode==MSW_NARROWED) {
		if(!strcmp(*thisword,"C"))
			position = 0.0;
		else if(!strcmp(*thisword,"L"))
			position = -1.0;
		else if(!strcmp(*thisword,"R"))
			position = 1.0;
		else if(sscanf(*thisword,"%lf",&position)!=1) {
			sprintf(errstr,"Failed to get pan value: adjust_position()\n");
			return(PROGRAM_ERROR);
		}
		param *= position;
	}
	sprintf(errstr,"%.4lf",param);	
	newlen = strlen(errstr);
	if(newlen > (int)strlen(*thisword)) {
		if((*thisword = 
		(char *)realloc(*thisword,(newlen+1) * sizeof(char)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to reallocate thisword store.\n");
			return(MEMORY_ERROR);
		}
	}
	strcpy(*thisword,errstr);
	return(FINISHED);
}

/************************* TREAT_SHORT_STEREO_LINE ******************************/

int treat_short_stereo_line(int lineno,int wordno,double param,dataptr dz)
{

#define STEREO_EXTRA_WORDS (3)

	int n, m;
	int  wordlen;
	if((dz->wordstor = (char **)realloc(dz->wordstor,(dz->all_words + STEREO_EXTRA_WORDS) * sizeof(char *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to reallocate word store.\n");
		return(MEMORY_ERROR);
	}
	for(n=0,m=dz->all_words;n<STEREO_EXTRA_WORDS;n++,m++)
		dz->wordstor[m] = NULL;
	dz->all_words += STEREO_EXTRA_WORDS;
	for(n=dz->all_words-STEREO_EXTRA_WORDS-1;n>wordno;n--)
		dz->wordstor[n+STEREO_EXTRA_WORDS] = dz->wordstor[n];
	dz->wordcnt[lineno] += STEREO_EXTRA_WORDS;		 
	for(n=wordno+1;n<=wordno+STEREO_EXTRA_WORDS;n++)
		dz->wordstor[n] = NULL;

	switch(dz->mode) {
	case(MSW_NARROWED):		sprintf(errstr,"%.4lf",-param);		break;
	default:				sprintf(errstr,"%.4lf",param);		break;
	}

	wordlen = strlen(errstr);
	if((dz->wordstor[wordno+1] = (char *)malloc((wordlen+1) * sizeof(char)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to allocate wordstore %d\n",wordno+1+1);
		return(MEMORY_ERROR);
	}	
	strcpy(dz->wordstor[wordno+1],errstr);

	wordlen = strlen(dz->wordstor[wordno]);
	if((dz->wordstor[wordno+2] = (char *)malloc((wordlen+1) * sizeof(char)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to allocate wordstore %d\n",wordno+2+1);
		return(MEMORY_ERROR);
	}	
	strcpy(dz->wordstor[wordno+2],dz->wordstor[wordno]);

	sprintf(errstr,"%.4lf",param);
	wordlen = strlen(errstr);
	if((dz->wordstor[wordno+3] = (char *)malloc((wordlen+1) * sizeof(char)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to allocate wordstore %d\n",wordno+3+1);
		return(MEMORY_ERROR);
	}	
	strcpy(dz->wordstor[wordno+3],errstr);
	return(FINISHED);
}			

/************************* TREAT_SHORT_MONO_LINE ******************************/

int treat_short_mono_line(int lineno,int wordno,double param,dataptr dz)
{

#define MONO_EXTRA_WORDS (1)
	int n, m;
	int  wordlen;
	if((dz->wordstor = (char **)realloc(dz->wordstor,(dz->all_words + MONO_EXTRA_WORDS) * sizeof(char *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for word stores.\n");
		return(MEMORY_ERROR);
	}
	for(n=0,m=dz->all_words;n<MONO_EXTRA_WORDS;n++,m++)
		dz->wordstor[m] = NULL;
	dz->all_words += MONO_EXTRA_WORDS;
	for(n=dz->all_words-MONO_EXTRA_WORDS-1;n>wordno;n--)
		dz->wordstor[n+MONO_EXTRA_WORDS] = dz->wordstor[n];
	dz->wordcnt[lineno] += MONO_EXTRA_WORDS;		 
	dz->wordstor[wordno+1] = NULL;

	sprintf(errstr,"%.4lf",param);
	wordlen = strlen(errstr);
	if((dz->wordstor[wordno+1] = (char *)malloc((wordlen+1) * sizeof(char)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for word store %d\n",wordno+1+1);
		return(MEMORY_ERROR);
	}	
	strcpy(dz->wordstor[wordno+1],errstr);
	return(FINISHED);
}			

/************************* INVESTIGATE_TIMINGS ******************************/

int investigate_timings(double *duration,double *min_time,dataptr dz)
{
	int exit_status;
	int   n;
	double *timestor;
	double max_time;
	if((dz->parray[MSW_TIMESTOR] = (double *)malloc(dz->linecnt * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for times store.\n");
		return(MEMORY_ERROR);
	}
	timestor = dz->parray[MSW_TIMESTOR];
	if((exit_status = get_times(timestor,dz))<0)
		return(exit_status);
	max_time  = timestor[dz->iparam[MSH_STARTLINE]];
	*min_time = timestor[dz->iparam[MSH_STARTLINE]];
	for(n=dz->iparam[MSH_STARTLINE]+1;n<dz->iparam[MSH_ENDLINE];n++) {
		max_time  = max(timestor[n],max_time);
		*min_time = min(timestor[n],*min_time);
	}
	*duration = max_time - *min_time;
	return(FINISHED);
}

//TW UPDATE: NEW FUNCTION
/************************* PANMIX ******************************/

int panmix(dataptr dz)
{
	int newlen, exit_status;
	int baspos = 0, basposnext = 0, n, linelen;		   /*RWD Nov 2003 added init for basposnext */
	int levelwordno, lpanwordno, rpanwordno, chanwordno;
	double time, level;
	char temp[200];
	int *maxwordsize;
	for(n=0;n<dz->linecnt;n++) {
		basposnext += dz->wordcnt[n];
		levelwordno = baspos + MIX_LEVELPOS;
		lpanwordno  = baspos + MIX_PANPOS;
		rpanwordno  = baspos + MIX_RPANPOS;
		chanwordno  = baspos + MIX_CHANPOS;
		time = atof(dz->wordstor[baspos + 1]);
		if(dz->brksize[PAN_PAN]) {
			if((exit_status = read_value_from_brktable(time,PAN_PAN,dz))<0)
				return(exit_status);
		}
		linelen = basposnext - baspos;
		switch(linelen) {
		case(MIX_MINLINE):				/* STEREO CASE : SHORT LINE */
			level = atof(dz->wordstor[levelwordno]);
			level /= 2.0;
			sprintf(errstr,"%.4lf",level);
			strcat(errstr," ");
			sprintf(temp,"%.4lf",dz->param[PAN_PAN]);	
			strcat(temp," ");
			strcat(errstr,temp);
			strcpy(temp,errstr);
			strcat(errstr,temp);
			newlen = strlen(errstr);
			if(newlen > (int)strlen(dz->wordstor[levelwordno])) {
				if((dz->wordstor[levelwordno] = 
				(char *)realloc(dz->wordstor[levelwordno],(newlen+1) * sizeof(char)))==NULL) {
					sprintf(errstr,"INSUFFICIENT MEMORY to reallocate thisword store.\n");
					return(MEMORY_ERROR);
				}
			}
			strcpy(dz->wordstor[levelwordno],errstr);
			break;
		case(MIX_MAXLINE):				/* STEREO CASE : LONG LINE */
			level = atof(dz->wordstor[levelwordno]);
			level += atof(dz->wordstor[levelwordno+2]);
			level /= 4.0;				/* Average level: = (level_left + level-right) / 2.0 */
										/* This average level from BOTH channels is to be put at SAME position */
										/* Therefore, divide by two again */
			sprintf(errstr,"%.4lf",level);
			newlen = strlen(errstr);

			if(newlen > (int)strlen(dz->wordstor[levelwordno])) {
				if((dz->wordstor[levelwordno] = 
				(char *)realloc(dz->wordstor[levelwordno],(newlen+1) * sizeof(char)))==NULL) {
					sprintf(errstr,"INSUFFICIENT MEMORY to reallocate thisword store.\n");
					return(MEMORY_ERROR);
				}
			}
			if(newlen > (int)strlen(dz->wordstor[levelwordno+2])) {
				if((dz->wordstor[levelwordno+2] = 
				(char *)realloc(dz->wordstor[levelwordno+2],(newlen+1) * sizeof(char)))==NULL) {
					sprintf(errstr,"INSUFFICIENT MEMORY to reallocate thisword store.\n");
					return(MEMORY_ERROR);
				}
			}
			strcpy(dz->wordstor[levelwordno],errstr);
			strcpy(dz->wordstor[levelwordno+2],errstr);

			sprintf(errstr,"%.4lf",dz->param[PAN_PAN]);	
			if(newlen > (int)strlen(dz->wordstor[lpanwordno])) {
				if((dz->wordstor[lpanwordno] = 
				(char *)realloc(dz->wordstor[lpanwordno],(newlen+1) * sizeof(char)))==NULL) {
					sprintf(errstr,"INSUFFICIENT MEMORY to reallocate thisword store.\n");
					return(MEMORY_ERROR);
				}
			}
			if(newlen > (int)strlen(dz->wordstor[rpanwordno])) {
				if((dz->wordstor[rpanwordno] = 
				(char *)realloc(dz->wordstor[rpanwordno],(newlen+1) * sizeof(char)))==NULL) {
					sprintf(errstr,"INSUFFICIENT MEMORY to reallocate thisword store.\n");
					return(MEMORY_ERROR);
				}
			}
			strcpy(dz->wordstor[lpanwordno],errstr);
			strcpy(dz->wordstor[rpanwordno],errstr);
			break;
		case(MIX_MIDLINE):				/* MONO CASE */
			sprintf(errstr,"%.4lf",dz->param[PAN_PAN]);	
			newlen = strlen(errstr);
			if(newlen > (int)strlen(dz->wordstor[lpanwordno])) {
				if((dz->wordstor[lpanwordno] = 
				(char *)realloc(dz->wordstor[lpanwordno],(newlen+1) * sizeof(char)))==NULL) {
					sprintf(errstr,"INSUFFICIENT MEMORY to reallocate thisword store.\n");
					return(MEMORY_ERROR);
				}
			}
			strcpy(dz->wordstor[lpanwordno],errstr);
			break;
		}
		baspos = basposnext;
	}
	if((exit_status = get_maxwordsize(&maxwordsize,dz))<0)
		return(exit_status);
	if((exit_status = output_mixfile_lines(maxwordsize,dz))<0)
		return(exit_status);
	free(maxwordsize);
	return(FINISHED);
}

