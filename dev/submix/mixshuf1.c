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
#include <tkglobals.h>
#include <globcon.h>
#include <processno.h>
#include <modeno.h>
#include <arrays.h>
#include <filetype.h>
#include <mix.h>
#include <cdpmain.h>

#include <sfsys.h>
#include <osbind.h>
#include <float.h>

static int  get_maxpostdec(int *maxpostdec,dataptr dz);
static int  get_postdec(char *thisword);
static int  get_times_and_timediffs(double *timestor,double *timediff,dataptr dz);

static int  reverse_times(double *timestor,double *timediff,dataptr dz);
static int  freeze_times(double *timestor,double *timediff,dataptr dz);
static int  delete_times(double *timestor,double *timediff,dataptr dz);
static int  altdel_times(double *timestor,double *timediff,dataptr dz);
static int  regular_times(double *timestor,double *timediff,dataptr dz);
static int  arithme_times(double *timestor,double *timediff,dataptr dz);
static int  aritexp_times(double *timestor,double *timediff,dataptr dz);
static int  geometr_times(double *timestor,double *timediff,dataptr dz);
static int  geomexp_times(double *timestor,double *timediff,dataptr dz);
static int  exponen_times(double *timestor,double *timediff,dataptr dz);
static int  expoexp_times(double *timestor,double *timediff,dataptr dz);
static int  spacing_times(double *timestor,double *timediff,dataptr dz);
static int  scatter_times(double *timestor,double *timediff,dataptr dz);
static int  shuffle_times(double *timestor,double *timediff,dataptr dz);
static int  stretch_times(double *timestor,double *timediff,dataptr dz);

static int  put_new_time(int total_words,int postdec,double timeval,dataptr dz);

static int  convert_level(int lineno,int total_words,int *maxwordsize,
						double *llevelstor,double *rlevelstor,int lmaxlevelsize,int rmaxlevelsize,dataptr dz);
static int  get_levels(int lineno,int total_words,
						double *llevelstor,double *rlevelstor,int *lmaxlevelsize,int *rmaxlevelsize,dataptr dz);
static void delete_wordstorage_space_of_all_deleted_lines(int oldwordcnt,char **wordcopy,dataptr dz);

/************************* DO_TIME_MANIP ******************************/

int do_time_manip(dataptr dz)
{
	int    exit_status;
	double *timestor, *timediff;
	int    *maxwordsize = NULL, postdec = 0;
	if((dz->parray[MTW_TIMESTOR] = (double *)malloc(dz->linecnt * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for times store.\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[MTW_TIMEDIFF] = (double *)malloc(dz->linecnt * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for timesteps store.\n");
		return(MEMORY_ERROR);
	}
	timestor = dz->parray[MTW_TIMESTOR];
	timediff = dz->parray[MTW_TIMEDIFF];

	if((exit_status = get_maxpostdec(&postdec,dz))<0)
		return(exit_status);

	if((exit_status = get_times_and_timediffs(timestor,timediff,dz))<0)
		return(exit_status);

	switch(dz->process) {
	case(MIXTWARP):
		switch(dz->mode) {
		case(MTW_TIMESORT):	
			break;
		case(MTW_REVERSE_T):	
			if((exit_status = reverse_times(timestor,timediff,dz))<0)
				return(exit_status);
			break;
		case(MTW_FREEZE_T):		
			if((exit_status = freeze_times(timestor,timediff,dz))<0)
				return(exit_status);
			break;
	  	case(MTW_SCATTER):		
	  		if((exit_status = scatter_times(timestor,timediff,dz))<0)
	  			return(exit_status);
	  		break;
	 	case(MTW_DOMINO):		
	 		if((exit_status = shuffle_times(timestor,timediff,dz))<0)
	 			return(exit_status);
	 		break;
 		case(MTW_ADD_TO_TG):	
 			if((exit_status = spacing_times(timestor,timediff,dz))<0)
 				return(exit_status);
 			break;
		case(MTW_CREATE_TG_1):	
			if((exit_status = regular_times(timestor,timediff,dz))<0)
				return(exit_status);
			break;
 		case(MTW_CREATE_TG_2):	
 			if((exit_status = aritexp_times(timestor,timediff,dz))<0)
 				return(exit_status);
 			break;
		case(MTW_CREATE_TG_3):	
			if((exit_status = geomexp_times(timestor,timediff,dz))<0)
				return(exit_status);
			break;
		case(MTW_CREATE_TG_4):	
			if((exit_status = expoexp_times(timestor,timediff,dz))<0)
				return(exit_status);
			break;
		case(MTW_ENLARGE_TG_1):	
			if((exit_status = stretch_times(timestor,timediff,dz))<0)
				return(exit_status);
			break;
		case(MTW_ENLARGE_TG_2):	
			if((exit_status = arithme_times(timestor,timediff,dz))<0)
				return(exit_status);
			break;
		case(MTW_ENLARGE_TG_3):	
			if((exit_status = geometr_times(timestor,timediff,dz))<0)
				return(exit_status);
			break;
		case(MTW_ENLARGE_TG_4):	
			if((exit_status = exponen_times(timestor,timediff,dz))<0)
				return(exit_status);
			break;
		default:
			sprintf(errstr,"Unknown case in do_time_manip()\n");
			return(PROGRAM_ERROR);
		}
		break;
	case(MIXSHUFL):
		switch(dz->mode) {
		case(MSH_OMIT_ALT):		
			if((exit_status = altdel_times(timestor,timediff,dz))<0)
				return(exit_status);
			break;
		case(MSH_OMIT):			
			if((exit_status = delete_times(timestor,timediff,dz))<0)
				return(exit_status);
			break;
		default:
			sprintf(errstr,"Unknown case in do_time_manip()\n");
			return(PROGRAM_ERROR);
		}
		break;
	default:
		sprintf(errstr,"Unknown case in do_time_manip()\n");
		return(PROGRAM_ERROR);
	}
	if((exit_status = retime_the_lines(timestor,postdec,dz))<0)
		return(exit_status);
	if((exit_status = timesort_mixfile(timestor,dz))<0)
		return(exit_status);
	if((exit_status = get_maxwordsize(&maxwordsize,dz))<0)
		return(exit_status);

	if((exit_status = output_mixfile_lines(maxwordsize,dz))<0)
		return(exit_status);
	free(maxwordsize);
	return(FINISHED);
}

/************************** REVERSE_TIMES **************************/

int reverse_times(double *timestor,double *timediff,dataptr dz)
{
	int n, m, k; 
	int this_linecnt;
	if(dz->iparam[MSH_ENDLINE] - dz->iparam[MSH_STARTLINE] < 2) {
		sprintf(errstr,"CAN'T REVERSE TIME PATTERN: TOO FEW LINES\n");
		return(DATA_ERROR);
	}
	this_linecnt = (dz->iparam[MSH_ENDLINE] - dz->iparam[MSH_STARTLINE]) - 2;
	for(n = 0, k=dz->iparam[MSH_STARTLINE]+1, m = dz->iparam[MSH_ENDLINE] - 2; n<this_linecnt; n++, k++, m--)
		timestor[k] = timestor[k-1] + timediff[m];
	return(FINISHED);
}

/************************** STRETCH_TIMES **************************/

int stretch_times(double *timestor,double *timediff,dataptr dz)
{
	int n; 
	for(n=max(dz->iparam[MSH_STARTLINE],1);n<dz->iparam[MSH_ENDLINE];n++)
		timediff[n-1] *= dz->param[MTW_PARAM];
	for(n=1; n<dz->linecnt; n++) {
		if(DBL_MAX - timestor[n-1] <= timediff[n-1]) {
			sprintf(errstr,"Numbers got too large.\n");
			return(GOAL_FAILED);
		}
		timestor[n] = timestor[n-1] + timediff[n-1];
	}
	return(FINISHED);
}

/************************** FREEZE_TIMES **************************/

int freeze_times(double *timestor,double *timediff,dataptr dz)
{
	int n; 
	for(n=dz->iparam[MSH_STARTLINE]+1;n<dz->iparam[MSH_ENDLINE]-1;n++)
		timediff[n] = timediff[dz->iparam[MSH_STARTLINE]];
	for(n=1; n<dz->linecnt; n++)
		timestor[n] = timestor[n-1] + timediff[n-1];
	return(FINISHED);
}

/************************** DELETE_TIMES **************************/

int delete_times(double *timestor,double *timediff,dataptr dz)
{
	int n, m, k; 
	int del_lines = dz->iparam[MSH_ENDLINE] - dz->iparam[MSH_STARTLINE];
	int new_total_wordcnt = 0, current_word_to_copy, total_wordcnt, oldlinecnt, oldwordcnt;
	char **wordcopy;

	if((wordcopy = (char **)malloc(dz->all_words * sizeof(char *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for wordcopy pointers store.\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<dz->all_words;n++)
		wordcopy[n] = dz->wordstor[n];

	if(dz->iparam[MSH_STARTLINE]==0 && dz->iparam[MSH_ENDLINE] == dz->linecnt) {
		free(wordcopy);
		sprintf(errstr,"Whole file would be deleted!\n");
		return(USER_ERROR);
	}

	for(n=1; n < dz->linecnt; n++) {
		if(timestor[n] < timestor[n-1]) {
			free(wordcopy);
			sprintf(errstr,"mixfile lines are not in correct time-order\n");
			return(USER_ERROR);
		}
	}

	for(n=dz->iparam[MSH_STARTLINE],m = dz->iparam[MSH_ENDLINE]; m < dz->linecnt-1; n++,m++)
		timediff[n] = timediff[m];				/* adjust times: by shuffling down timediffs */
	for(n=0;n<dz->iparam[MSH_STARTLINE];n++)
		new_total_wordcnt += dz->wordcnt[n];
	total_wordcnt =  new_total_wordcnt;
	for(n=dz->iparam[MSH_STARTLINE];n<dz->linecnt;n++)
		total_wordcnt += dz->wordcnt[n];
	current_word_to_copy =  new_total_wordcnt;
	for(n=dz->iparam[MSH_STARTLINE];n<dz->iparam[MSH_ENDLINE];n++)
		current_word_to_copy += dz->wordcnt[n];
	for(n=dz->iparam[MSH_STARTLINE],m = dz->iparam[MSH_ENDLINE]; m < dz->linecnt; n++,m++) {

		for(k=0;k<dz->wordcnt[m];k++)
			dz->wordstor[new_total_wordcnt++] = dz->wordstor[current_word_to_copy++];				
		dz->wordcnt[n] = dz->wordcnt[m];
	}
	oldlinecnt    = dz->linecnt;
	oldwordcnt    = dz->all_words;
	dz->linecnt  -= del_lines;
	dz->all_words = new_total_wordcnt;

	delete_wordstorage_space_of_all_deleted_lines(oldwordcnt,wordcopy,dz);
	free(wordcopy);
	for(n=1; n<dz->linecnt; n++)
		timestor[n] = timestor[n-1] + timediff[n-1];
	return(FINISHED);
}

/************************** ALTDEL_TIMES **************************/

int altdel_times(double *timestor,double *timediff,dataptr dz)
{
	int n, copied_or_deleted_line, k; 
	int new_total_wordcnt = 0, current_word_to_copy, oldlinecnt, oldwordcnt, new_linecnt;
	char **wordcopy;

	if((wordcopy = (char **)malloc(dz->all_words * sizeof(char *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for wordcopy pointers store.\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<dz->all_words;n++)
		wordcopy[n] = dz->wordstor[n];

	copied_or_deleted_line = dz->iparam[MSH_STARTLINE];
	for(n=dz->iparam[MSH_STARTLINE]; n <= dz->iparam[MSH_ENDLINE]; n++) {
		if(++copied_or_deleted_line >= dz->linecnt)			/* SKIP DELETED LINE */
			break;
		timediff[n] = timediff[copied_or_deleted_line];		/* SHUFFLE BACK TIME DIFFERENCES */
		if(++copied_or_deleted_line >= dz->linecnt)
			break;
	}
	while(copied_or_deleted_line < dz->linecnt)				/* RESET TIMEDIFFS OF ANY REMAINING LINES */
		timediff[n++] = timediff[copied_or_deleted_line++];

	for(n=0;n<dz->iparam[MSH_STARTLINE];n++)
		new_total_wordcnt += dz->wordcnt[n];

	current_word_to_copy = new_total_wordcnt;

	new_linecnt = dz->iparam[MSH_STARTLINE];
	copied_or_deleted_line = dz->iparam[MSH_STARTLINE];
	for(n=dz->iparam[MSH_STARTLINE]; n < dz->iparam[MSH_ENDLINE];n++) {
		current_word_to_copy += dz->wordcnt[copied_or_deleted_line];/* SKIP DELETED LINE */
		if(++copied_or_deleted_line >= dz->linecnt)					
			break;
		for(k=0;k<dz->wordcnt[copied_or_deleted_line];k++)			/* COPY RETAINED LINE */
			dz->wordstor[new_total_wordcnt++] = dz->wordstor[current_word_to_copy++];				
		dz->wordcnt[n] = dz->wordcnt[copied_or_deleted_line];		/* RESET NEW LINE-WORDCNT */
		new_linecnt++;												/* COUNT NEW TOTAL LINES  */
		if(++copied_or_deleted_line >= dz->linecnt)
			break;
	}
	while(copied_or_deleted_line<dz->linecnt) {						/* RESET WORDCNTS OF ANY REMAINING LINES */
		dz->wordcnt[n] = dz->wordcnt[copied_or_deleted_line];
		new_linecnt++;												/* KEEP COUNTING NEW TOTAL LINES  */
		copied_or_deleted_line++;
	}
	while(current_word_to_copy<dz->all_words)		  				/* COPY ANY REMAINING LINES */
		dz->wordstor[new_total_wordcnt++] = dz->wordstor[current_word_to_copy++];

	oldlinecnt    = dz->linecnt;
	oldwordcnt    = dz->all_words;
	dz->linecnt   = new_linecnt;
	dz->all_words = new_total_wordcnt;

	delete_wordstorage_space_of_all_deleted_lines(oldwordcnt,wordcopy,dz);
	free(wordcopy);
	for(n=1; n<dz->linecnt; n++)
		timestor[n] = timestor[n-1] + timediff[n-1];
	return(FINISHED);
}

/************************** REGULAR_TIMES **************************/

int regular_times(double *timestor,double *timediff,dataptr dz)
{  int n; 
   for(n=max(dz->iparam[MSH_STARTLINE],1);n<dz->iparam[MSH_ENDLINE];n++)
	timediff[n-1] = dz->param[MTW_PARAM];
    for(n=1; n<dz->linecnt; n++)
      	timestor[n] = timestor[n-1] + timediff[n-1];
	return(FINISHED);
}

/************************** ARITHME_TIMES **************************/

int arithme_times(double *timestor,double *timediff,dataptr dz)
{
	int n;
	double base_factor = dz->param[MTW_PARAM]; 
	for(n=max(dz->iparam[MSH_STARTLINE],1);n<dz->iparam[MSH_ENDLINE];n++) {
		timediff[n-1] += dz->param[MTW_PARAM];
		if(DBL_MAX - dz->param[MTW_PARAM] <= base_factor) {
			sprintf(errstr,"Numbers got too large.\n");
			return(GOAL_FAILED);
		}
		dz->param[MTW_PARAM] += base_factor;
	}
	for(n=1; n<dz->linecnt; n++) {
		if(DBL_MAX - timestor[n-1] <= timediff[n-1]) {
			sprintf(errstr,"Numbers got too large.\n");
			return(GOAL_FAILED);
		}
		timestor[n] = timestor[n-1] + timediff[n-1];
		if(timestor[n]<0.0) {
			sprintf(errstr,"NEGATIVE TIME GENERATED AT LINE %d\n",n+1);
			return(GOAL_FAILED);
		}
	}
	return(FINISHED);
}

/************************** ARITEXP_TIMES **************************/

int aritexp_times(double *timestor,double *timediff,dataptr dz)
{
	int n;
	double value; 
	value = timediff[dz->iparam[MSH_STARTLINE]];
	for(n=dz->iparam[MSH_STARTLINE]+1;n<dz->iparam[MSH_ENDLINE]-1;n++) {
		if(DBL_MAX - value <= dz->param[MTW_PARAM]) {
			sprintf(errstr,"Numbers got too large.\n");
			return(GOAL_FAILED);
		}
		value      += dz->param[MTW_PARAM];
		timediff[n] = value;
	}
	for(n=1; n<dz->linecnt; n++) {
		if(DBL_MAX - timestor[n-1] <= timediff[n-1]) {
			sprintf(errstr,"Numbers got too large.\n");
			return(GOAL_FAILED);
		}
		timestor[n] = timestor[n-1] + timediff[n-1];
		if(timestor[n]<0.0) {
			sprintf(errstr,"NEGATIVE TIME GENERATED AT LINE %d\n",n+1);
			return(GOAL_FAILED);
		}
	}
	return(FINISHED);
}

/************************** GEOMETR_TIMES **************************/

int geometr_times(double *timestor,double *timediff,dataptr dz)
{
	int n; 
	double base_factor = dz->param[MTW_PARAM]; 
	for(n=max(dz->iparam[MSH_STARTLINE],1);n<dz->iparam[MSH_ENDLINE];n++) {
		if(dz->param[MTW_PARAM] > 1.0) {
			if(DBL_MAX/dz->param[MTW_PARAM] <= timediff[n-1]) {
				sprintf(errstr,"Numbers got too large.\n");
				return(GOAL_FAILED);
			}
		}
		timediff[n-1] *= dz->param[MTW_PARAM];
		dz->param[MTW_PARAM] += base_factor;
	}
	for(n=1; n<dz->linecnt; n++) {
		if(DBL_MAX - timestor[n-1] <= timediff[n-1]) {
			sprintf(errstr,"Numbers got too large.\n");
			return(GOAL_FAILED);
		}
		timestor[n] = timestor[n-1] + timediff[n-1];
		if(timestor[n]<0.0) {
			sprintf(errstr,"NEGATIVE TIME GENERATED AT LINE %d\n",n+1);
			return(GOAL_FAILED);
		}
	}
	return(FINISHED);
}

/************************** GEOMEXP_TIMES **************************/

int geomexp_times(double *timestor,double *timediff,dataptr dz)
{
	int n; 
	double value, base_factor = dz->param[MTW_PARAM]; 
	value = timediff[dz->iparam[MSH_STARTLINE]];
	for(n=dz->iparam[MSH_STARTLINE]+1;n<dz->iparam[MSH_ENDLINE]-1;n++) {
		if(dz->param[MTW_PARAM] > 1.0) {
			if(DBL_MAX/(dz->param[MTW_PARAM]) <= value) {
				sprintf(errstr,"Numbers got too large.\n");
				return(GOAL_FAILED);
			}
		}
		timediff[n] = value * dz->param[MTW_PARAM];
		if(DBL_MAX - dz->param[MTW_PARAM] <= base_factor) {
			sprintf(errstr,"Numbers got too large.\n");
			return(GOAL_FAILED);
		}
		dz->param[MTW_PARAM] += base_factor;
	}
	for(n=1; n<dz->linecnt; n++) {
		if(DBL_MAX - timestor[n-1] <= timediff[n-1]) {
			sprintf(errstr,"Numbers got too large.\n");
			return(GOAL_FAILED);
		}
		timestor[n] = timestor[n-1] + timediff[n-1];
		if(timestor[n]<0.0) {
			sprintf(errstr,"NEGATIVE TIME GENERATED AT LINE %d\n",n+1);
			return(GOAL_FAILED);
		}
	}
	return(FINISHED);
}

/************************** EXPONEN_TIMES **************************/

int exponen_times(double *timestor,double *timediff,dataptr dz)
{
	int n;
	double base_factor = dz->param[MTW_PARAM]; 
	for(n=max(dz->iparam[MSH_STARTLINE],1);n<dz->iparam[MSH_ENDLINE];n++) {
		if(DBL_MAX/dz->param[MTW_PARAM] <= timediff[n-1]) {
			sprintf(errstr,"Numbers got too large.\n");
			return(GOAL_FAILED);
		}
		timediff[n-1] *= dz->param[MTW_PARAM];
		dz->param[MTW_PARAM] *= base_factor;
	}
	for(n=1; n<dz->linecnt; n++) {
		if(DBL_MAX - timestor[n-1] <= timediff[n-1]) {
			sprintf(errstr,"Numbers got too large.\n");
			return(GOAL_FAILED);
		}
		timestor[n] = timestor[n-1] + timediff[n-1];
		if(timestor[n]<0.0) {
			sprintf(errstr,"NEGATIVE TIME GENERATED AT LINE %d\n",n+1);
			return(GOAL_FAILED);
		}
	}
	return(FINISHED);
}

/************************** EXPOEXP_TIMES **************************/

int expoexp_times(double *timestor,double *timediff,dataptr dz)
{
	int n; 
	double value, base_factor = dz->param[MTW_PARAM];
	value = timediff[dz->iparam[MSH_STARTLINE]];
	for(n=dz->iparam[MSH_STARTLINE]+1;n<dz->iparam[MSH_ENDLINE]-1;n++) {
		if(DBL_MAX/value <= dz->param[MTW_PARAM]) {
			sprintf(errstr,"Numbers got too large.\n");
			return(GOAL_FAILED);
		}
		
		timediff[n] = value * dz->param[MTW_PARAM];
		if(DBL_MAX/dz->param[MTW_PARAM] <= base_factor) {
			sprintf(errstr,"Numbers got too large.\n");
			return(GOAL_FAILED);
		}
		dz->param[MTW_PARAM] *= base_factor;
	}
	for(n=1; n<dz->linecnt; n++) {
		if(DBL_MAX - timestor[n-1] <= timediff[n-1]) {
			sprintf(errstr,"Numbers got too large.\n");
			return(GOAL_FAILED);
		}
		timestor[n] = timestor[n-1] + timediff[n-1];
		if(timestor[n]<0.0) {
			sprintf(errstr,"NEGATIVE TIME GENERATED AT LINE %d\n",n+1);
			return(GOAL_FAILED);
		}
	}
	return(FINISHED);
}

/************************** SPACING_TIMES **************************/

int spacing_times(double *timestor,double *timediff,dataptr dz)
{   
	int n; 
	for(n = max(dz->iparam[MSH_STARTLINE],1);n<dz->iparam[MSH_ENDLINE];n++)
		timediff[n-1] += dz->param[MTW_PARAM];
	for(n=1; n<dz->linecnt; n++) {
		timestor[n] = timestor[n-1] + timediff[n-1];
		if(timestor[n]<0.0) {
			sprintf(errstr,"NEGATIVE TIME GENERATED AT LINE %d\n",n+1);
			return(GOAL_FAILED);
		}
	}
	return(FINISHED);
}

/************************** SHUFFLE_TIMES **************************/

int shuffle_times(double *timestor,double *timediff,dataptr dz)
{
	int n; 
	n = max(dz->iparam[MSH_STARTLINE],1);
	timediff[n-1] += dz->param[MTW_PARAM];
	for(; n<dz->iparam[MSH_ENDLINE]; n++) {
		timestor[n] = timestor[n-1] + timediff[n-1];
		if(timestor[n]<0.0) {
			sprintf(errstr,"NEGATIVE TIME GENERATED AT LINE %d\n",n+1);
			return(GOAL_FAILED);
		}
	}
	return(FINISHED);
}

/************************** SCATTER_TIMES **************************/

int scatter_times(double *timestor,double *timediff,dataptr dz)
{
	int n; 
	double base_scatter = 0, scatdirection;
	if(dz->iparam[MSH_STARTLINE]==0) {
		base_scatter = (timediff[0]/2.0) * drand48() * dz->param[MTW_PARAM];
		timestor[0] += base_scatter;
	}
	for(n=max(dz->iparam[MSH_STARTLINE],1);n<dz->iparam[MSH_ENDLINE]-1;n++) {
		scatdirection   = (drand48() * 2.0) - 1.0;
		if(scatdirection<0.0)
			timestor[n] += (timediff[n]/2.0) * drand48() * dz->param[MTW_PARAM];
		else
			timestor[n] -= (timediff[n-1]/2.0) * drand48() * dz->param[MTW_PARAM];
	}
	if(dz->iparam[MSH_ENDLINE]==dz->linecnt)
		timestor[dz->linecnt-1] -= (timediff[dz->linecnt-2]/2.0) * drand48() * dz->param[MTW_PARAM];
	else {
		scatdirection   = (drand48() * 2.0) - 1.0;
		if(scatdirection<0.0)
			timestor[dz->iparam[MSH_ENDLINE]-1] += 
			(timediff[dz->iparam[MSH_ENDLINE]]/2.0) * drand48() * dz->param[MTW_PARAM];
		else
			timestor[dz->iparam[MSH_ENDLINE]-1] -= 
			(timediff[dz->iparam[MSH_ENDLINE]-1]/2.0) * drand48() * dz->param[MTW_PARAM];
	}
	if(dz->iparam[MSH_STARTLINE]==0){
		for(n=0; n<dz->linecnt; n++)
			timestor[n] -= base_scatter;
	}
	return(FINISHED);
}

/*************************** GET_TIMES_AND_TIMEDIFFS ***************************/

int get_times_and_timediffs(double *timestor,double *timediff,dataptr dz)
{
	int n, timeloc;
	int total_wordcnt = 0;
	for(n=0;n<dz->linecnt;n++) {
		if(dz->wordcnt[n] < 2) {
			sprintf(errstr,"Problem getting line times: get_times_and_timediffs()\n");
			return(PROGRAM_ERROR);
		}
		timeloc = total_wordcnt + MIX_TIMEPOS;
		if(sscanf(dz->wordstor[timeloc],"%lf",&(timestor[n]))!=1) {
			sprintf(errstr,"Problem reading time: get_times_and_timediffs()\n");
			return(PROGRAM_ERROR);
		}
		total_wordcnt += dz->wordcnt[n];
	}
	for(n=1;n<dz->linecnt;n++)
		timediff[n-1] = timestor[n] - timestor[n-1];
	return(FINISHED);
}

/*************************** retime_the_lines ***************************/

int retime_the_lines(double *timestor,int postdec,dataptr dz)
{
	int exit_status;
	int lineno;
	int total_words = 0;
	for(lineno=0;lineno<dz->linecnt;lineno++) {
		if((exit_status = put_new_time(total_words,postdec,timestor[lineno],dz))<0)
			return(exit_status);
		total_words += dz->wordcnt[lineno];
	}
	return(FINISHED);
}

/************************** PUT_NEW_TIME ***************************/

int put_new_time(int total_words,int postdec,double timeval,dataptr dz)
{
	int wordno, newlen;
	wordno = total_words + MIX_TIMEPOS;
	switch(postdec) {
	case(0):	sprintf(errstr,"%.0lf",timeval);
	case(1):	sprintf(errstr,"%.1lf",timeval);
	case(2):	sprintf(errstr,"%.2lf",timeval);
	case(3):	sprintf(errstr,"%.3lf",timeval);
	case(4):	sprintf(errstr,"%.4lf",timeval);
	default:	sprintf(errstr,"%.5lf",timeval);   /* DEFAULT_DECIMAL_REPRESENTATION */
	}
	if((newlen = (int)strlen(errstr)) > (int)strlen(dz->wordstor[wordno])) {
		if((dz->wordstor[wordno] = (char *)realloc(dz->wordstor[wordno],(newlen+1) * sizeof(char)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for wordstore %d\n",wordno+1);
			return(MEMORY_ERROR);
		}
	}
	strcpy(dz->wordstor[wordno],errstr);
	return(FINISHED);
}

/********************** GET_MAXWORDSIZE ****************************/
 
int get_maxwordsize(int **maxwordsize,dataptr dz)
{
	int total_words = 0, n;
	int m;
	if((*maxwordsize = (int *)malloc(MIX_MAXLINE * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for max wordsize store.\n");
		return(MEMORY_ERROR);
	}
	for(m=0;m<MIX_MAXLINE;m++)
		(*maxwordsize)[m] = 0;

	for(n=0;n<dz->linecnt;n++) {						
		for(m=0;m<dz->wordcnt[n];m++)
			(*maxwordsize)[m] = (int)max((int)(*maxwordsize)[m],(int)strlen(dz->wordstor[total_words+m]));
		total_words += dz->wordcnt[n];											
	}
	return(FINISHED);
}

/********************** GET_MAXPOSTDEC ****************************/
 
int get_maxpostdec(int *maxpostdec,dataptr dz)
{
	int total_words = 0, n, timewordno;
	for(n=0;n<dz->linecnt;n++) {
		timewordno = total_words + MIX_TIMEPOS;						
		*maxpostdec = max(*maxpostdec,get_postdec(dz->wordstor[timewordno]));
		total_words += dz->wordcnt[n];											
	}
	return(FINISHED);
}

/********************** GET_POSTDEC ****************************/
 
int get_postdec(char *thisword)
{
	char *p = thisword;
	int finished = FALSE;
	int post_dec = 0;
	while(*p != '.') {
		if(*p==ENDOFSTR) {
			finished = TRUE;
			break;
		}
		p++;
	}
	if(!finished) {
		p++;
		while(*p != ENDOFSTR) {
			post_dec++;
			p++;
		}
	}
	return post_dec;
}

/************************* MIX_GAIN ******************************/

int mix_gain(dataptr dz)
{
	int    exit_status;
	int   lineno, total_words;
	double *llevelstor, *rlevelstor;
	int    *maxwordsize = NULL, lmaxlevelsize = 0, rmaxlevelsize = 0;
	if((dz->parray[MIX_LLEVELSTOR] = (double *)malloc(dz->linecnt * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for left levels store.\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[MIX_RLEVELSTOR] = (double *)malloc(dz->linecnt * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for right levels store.\n");
		return(MEMORY_ERROR);
	}
	llevelstor = dz->parray[MIX_LLEVELSTOR];
	rlevelstor = dz->parray[MIX_RLEVELSTOR];

	if((exit_status = get_maxwordsize(&maxwordsize,dz))<0)
		return(exit_status);

 	total_words = 0;
	for(lineno=0;lineno<dz->linecnt;lineno++) {
		if((exit_status = get_levels(lineno,total_words,llevelstor,rlevelstor,&lmaxlevelsize,&rmaxlevelsize,dz))<0) {
			free(maxwordsize);
			return(exit_status);
		}
		total_words += dz->wordcnt[lineno];
	}
 	total_words = 0;
	for(lineno=0;lineno<dz->linecnt;lineno++) {
		if((exit_status = convert_level
		(lineno,total_words,maxwordsize,llevelstor,rlevelstor,lmaxlevelsize,rmaxlevelsize,dz))<0) {
			free(maxwordsize);
			return(exit_status);
		}
		total_words += dz->wordcnt[lineno];
	}
	free(maxwordsize);
	return(FINISHED);
}

/********************** GET_LEVELS ****************************/
 
int get_levels(int lineno,int total_words,
double *llevelstor,double *rlevelstor,int *lmaxlevelsize,int *rmaxlevelsize,dataptr dz)
{
	int exit_status;
	int wordcnt = dz->wordcnt[lineno];
	char *thisword; 
	double level;
	int wordno = total_words + MIX_LEVELPOS;

	thisword = dz->wordstor[wordno];
	if((exit_status = get_level(thisword,&level))<0)
		return(exit_status);
	if(lineno >= dz->iparam[MSH_STARTLINE] && lineno < dz->iparam[MSH_ENDLINE])
		llevelstor[lineno] = level * dz->param[MIX_GAIN];
	else
		llevelstor[lineno] = level;
	sprintf(errstr,"%.4lf",level);
	*lmaxlevelsize = max(*lmaxlevelsize,(int)strlen(errstr));

	if(wordcnt <= MIX_RLEVELPOS)
		return(FINISHED);

	wordno = total_words + MIX_RLEVELPOS;
	thisword = dz->wordstor[wordno];
	if((exit_status = get_level(thisword,&level))<0)
		return(exit_status);
	if(lineno >= dz->iparam[MSH_STARTLINE] && lineno <= dz->iparam[MSH_ENDLINE])
		rlevelstor[lineno] = level * dz->param[MIX_GAIN];
	else
		rlevelstor[lineno] = level;
	sprintf(errstr,"%.4lf",level);
	*rmaxlevelsize = max(*rmaxlevelsize,(int)strlen(errstr));
	return(FINISHED);
}

/************************** CONVERT_LEVEL ***************************/

int convert_level(int lineno,int total_words,int *maxwordsize,
				double *llevelstor,double *rlevelstor,int lmaxlevelsize,int rmaxlevelsize,dataptr dz)
{
	int    n, m, spacecnt;
	int   wordno;
	int    wordcnt = dz->wordcnt[lineno];
	for(n=0;n<wordcnt;n++) {
		wordno = total_words + n;
		switch(n) {
		case(MIX_NAMEPOS):
		case(MIX_CHANPOS):
		case(MIX_PANPOS):
		case(MIX_RPANPOS):
		case(MIX_TIMEPOS):
			if((spacecnt = maxwordsize[n] - strlen(dz->wordstor[wordno]))<0) {
				sprintf(errstr,"word alignment error: 1: convert_level()\n");
				return(PROGRAM_ERROR);
			}
			spacecnt++;
			fprintf(dz->fp,"%s",dz->wordstor[wordno]);
			for(m=0;m<spacecnt;m++)
				fprintf(dz->fp," ");
			break;
		case(MIX_LEVELPOS):
			sprintf(errstr,"%.4lf",llevelstor[lineno]);
			if((spacecnt = lmaxlevelsize - strlen(errstr))<0) {
				sprintf(errstr,"word alignment error: 2: convert_level()\n");
				return(PROGRAM_ERROR);
			}
			spacecnt++;
			fprintf(dz->fp,"%s",errstr);
			for(m=0;m<spacecnt;m++)
				fprintf(dz->fp," ");
			break;
		case(MIX_RLEVELPOS):
			sprintf(errstr,"%.4lf",rlevelstor[lineno]);
			if((spacecnt = rmaxlevelsize - strlen(errstr))<0) {
				sprintf(errstr,"word alignment error: 3: convert_level()\n");
				return(PROGRAM_ERROR);
			}
			spacecnt++;
			fprintf(dz->fp,"%s",errstr);
			for(m=0;m<spacecnt;m++)
				fprintf(dz->fp," ");
			break;
		default:
			sprintf(errstr,"Impossible line wordcnt: convert_level()\n");
			return(PROGRAM_ERROR);
		}
	}
	fprintf(dz->fp,"\n");
	return(FINISHED);
}

/*********************** DO_NAME_AND_TIME_REVERSE **************************/

int do_name_and_time_reverse(dataptr dz)
{
	int    exit_status;
	double *timestor, *timediff;
	int    *maxwordsize = NULL, postdec = 0;
	int   *name_index;
	char   *temp;
	int   lineno, newlineno, total_words = 0, half_linecnt;

	if((dz->parray[MTW_TIMESTOR] = (double *)malloc(dz->linecnt * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for times store.\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[MTW_TIMEDIFF] = (double *)malloc(dz->linecnt * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for timegaps store.\n");
		return(MEMORY_ERROR);
	}
	if((name_index = (int *)malloc(dz->linecnt * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for name index store.\n");
		return(PROGRAM_ERROR);
	}

	timestor = dz->parray[MTW_TIMESTOR];
	timediff = dz->parray[MTW_TIMEDIFF];

	if((exit_status = get_maxpostdec(&postdec,dz))<0) {
		free(name_index);
		return(exit_status);
	}
	if((exit_status = get_times_and_timediffs(timestor,timediff,dz))<0) {
		free(name_index);
		return(exit_status);
	}
	if((exit_status = reverse_times(timestor,timediff,dz))<0) {
		free(name_index);
		return(exit_status);
	}
	for(lineno=0;lineno<dz->linecnt;lineno++) {
		name_index[lineno] = total_words;
		total_words += dz->wordcnt[lineno];
	}
	half_linecnt  = dz->iparam[MSH_ENDLINE] - dz->iparam[MSH_STARTLINE];
	half_linecnt  = half_linecnt/2; /* TRUNCATE */
	half_linecnt += dz->iparam[MSH_STARTLINE];

	for(lineno=dz->iparam[MSH_STARTLINE],newlineno=dz->iparam[MSH_ENDLINE]-1;lineno < half_linecnt;lineno++,newlineno--) {
		temp            					= dz->wordstor[name_index[lineno]];
		dz->wordstor[name_index[lineno]] 	= dz->wordstor[name_index[newlineno]];
		dz->wordstor[name_index[newlineno]] = temp;
	}		

	if((exit_status = retime_the_lines(timestor,postdec,dz))<0) {
		free(name_index);
		return(exit_status);
	}
	if((exit_status = get_maxwordsize(&maxwordsize,dz))<0) {
		free(name_index);
		return(exit_status);
	}
	if((exit_status = output_mixfile_lines(maxwordsize,dz))<0) {
		free(maxwordsize);
		free(name_index);
		return(exit_status);
	}
	free(maxwordsize);
	free(name_index);
	return(FINISHED);
}

/*********************** DO_NAME_AND_TIME_FREEZE **************************/

int do_name_and_time_freeze(dataptr dz)
{
	int    exit_status;
	double *timestor, *timediff;
	int    *maxwordsize = NULL, postdec = 0;
	int   *name_index;
	int   lineno, total_words = 0;

	if((dz->parray[MTW_TIMESTOR] = (double *)malloc(dz->linecnt * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for times store.\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[MTW_TIMEDIFF] = (double *)malloc(dz->linecnt * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for timegaps store.\n");
		return(MEMORY_ERROR);
	}
	if((name_index = (int *)malloc(dz->linecnt * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for name index store.\n");
		return(PROGRAM_ERROR);
	}

	timestor = dz->parray[MTW_TIMESTOR];
	timediff = dz->parray[MTW_TIMEDIFF];

	if((exit_status = get_maxpostdec(&postdec,dz))<0) {
		free(name_index);
		return(exit_status);
	}
	if((exit_status = get_times_and_timediffs(timestor,timediff,dz))<0) {
		free(name_index);
		return(exit_status);
	}
	if((exit_status = freeze_times(timestor,timediff,dz))<0) {
		free(name_index);
		return(exit_status);
	}
	for(lineno=0;lineno<dz->linecnt;lineno++) {
		name_index[lineno] = total_words;
		total_words += dz->wordcnt[lineno];
	}

	for(lineno=dz->iparam[MSH_STARTLINE]+1;lineno < dz->iparam[MSH_ENDLINE];lineno++) {
		free(dz->wordstor[name_index[lineno]]);
		dz->wordstor[name_index[lineno]] = dz->wordstor[name_index[dz->iparam[MSH_STARTLINE]]];
	}

	if((exit_status = retime_the_lines(timestor,postdec,dz))<0) {
		free(name_index);
		return(exit_status);
	}
	if((exit_status = get_maxwordsize(&maxwordsize,dz))<0) {
		free(name_index);
		return(exit_status);
	}
	if((exit_status = output_mixfile_lines(maxwordsize,dz))<0) {
		free(maxwordsize);
		free(name_index);
		return(exit_status);
	}
	free(maxwordsize);
	free(name_index);
	return(FINISHED);
}

/*********************** TIMESORT_MIXFILE **************************/

int timesort_mixfile(double *timestor,dataptr dz)
{
	int    n, m; 
	int   lineno, startword, endword;
	double temptime, *timecopy;
	int   *line_wordindex, total_words, running_total_words;
	int    *permm, tempperm, *wordcnt_copy;
	char   **wordcopy;
	total_words = 0;
	for(n=0;n<dz->linecnt;n++)
		total_words += dz->wordcnt[n];
	if((permm = (int *)malloc(dz->linecnt * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for permutation.");
		return(MEMORY_ERROR);
	}
	if((wordcnt_copy = (int *)malloc(dz->linecnt * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for wordcnt copy.");
		return(MEMORY_ERROR);
	}
	if((line_wordindex = (int *)malloc((dz->linecnt + 1) * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for word index.");
		return(MEMORY_ERROR);
	}
	if((timecopy = (double *)malloc(dz->linecnt * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for time copy.");
		return(MEMORY_ERROR);
	}
	if((wordcopy = (char **)malloc(total_words * sizeof(char *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for word copy.");
		return(MEMORY_ERROR);
	}
	for(n=0;n<dz->linecnt;n++)
		permm[n] = n;
	for(n=0;n<dz->linecnt;n++)
		timecopy[n] = timestor[n];
	total_words = 0;
	for(n=0;n<dz->linecnt;n++) {
		line_wordindex[n] = total_words;
		total_words += dz->wordcnt[n];
	}
	line_wordindex[dz->linecnt] = total_words;
	for(n=1;n<dz->linecnt;n++) {
		temptime = timecopy[n];
		tempperm = permm[n];
		m = n-1;
		while(m >= 0 && timecopy[m] > temptime) {
			timecopy[m+1] = timecopy[m];
			permm[m+1]    = permm[m];
			m--;
		}			
		timecopy[m+1] = temptime;
		permm[m+1]    = tempperm;
	}
	running_total_words = 0;
	for(n=0;n<dz->linecnt;n++) {
		lineno = permm[n];
		wordcnt_copy[n] = dz->wordcnt[permm[n]];
		startword = line_wordindex[lineno];
		endword   = line_wordindex[lineno+1];
		for(m=0;m<wordcnt_copy[n];m++) {
			wordcopy[running_total_words] = dz->wordstor[startword+m]; 
			if(++running_total_words > total_words) {
				free(permm);
				free(wordcnt_copy);
				free(line_wordindex);
				free(timecopy);
				free(wordcopy);
				sprintf(errstr,"Accounting error in timesort_mixfile()\n");
				return(PROGRAM_ERROR);
			}
		}
	}
	for(n=0;n<total_words;n++)
		dz->wordstor[n] = wordcopy[n];
	for(n=0;n<dz->linecnt;n++)
		dz->wordcnt[n]  = wordcnt_copy[n];
	free(permm);
	free(wordcnt_copy);
	free(line_wordindex);
	free(timecopy);
	free(wordcopy);
	return(FINISHED);
}

/*********************** DELETE_WORDSTORAGE_SPACE_OF_ALL_DELETED_LINES **************************/

void delete_wordstorage_space_of_all_deleted_lines(int oldwordcnt,char **wordcopy,dataptr dz)
{
	int n, m;
	int is_kept;
	for(m=0;m<oldwordcnt;m++) {
		is_kept = FALSE;
		for(n=0;n<dz->all_words;n++) {
			if(wordcopy[m] == dz->wordstor[n]) {
				is_kept = TRUE;
				break;
			}
		}
		if(!is_kept)
			free(wordcopy[m]);
	}
}

/********************** MIX_SYNC ****************************/
 
int mix_sync(dataptr dz)
{
	int    exit_status;
	int   srate;
	int	   this_namelen, max_namelen = 0, max_timeword = 0, *inchans;												
	int   n;
	double maxfiletime = 0.0, midtime, *timestor;

	if(dz->infile->filetype==MIXFILE)
		return mix_sync_a_mixfile(dz);

	if(dz->all_words==0) {
		sprintf(errstr,"No words in source file.\n");
		return(PROGRAM_ERROR);
	}
	if((dz->parray[MSY_TIMESTOR] = (double *)malloc(dz->linecnt * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for times store.\n");
		return(MEMORY_ERROR);
	}
	timestor = dz->parray[MSY_TIMESTOR];
	if((dz->iparray[MSY_CHANS] = (int *)malloc(dz->linecnt * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for channels store.\n");
		return(MEMORY_ERROR);
	}
	inchans = dz->iparray[MSY_CHANS];
	if((exit_status = establish_file_data_storage_for_mix((int)1,dz))<0)
		return(exit_status);
	for(n=0;n<dz->all_words;n++) {						/* for each sndfile name */
		if((this_namelen = strlen(dz->wordstor[n]))<=0) {
			sprintf(errstr,"filename error: line %d: mix_sync()\n",n+1);
			return(PROGRAM_ERROR);
		}
		max_namelen = max(max_namelen,this_namelen);
		if((exit_status = open_file_and_retrieve_props(n,dz->wordstor[n],&srate,dz))<0)
			return(exit_status);
		inchans[n]  = dz->infile->channels;
		/*RWD NB clac from sampsize */
		timestor[n] = (double)(dz->insams[0]/dz->infile->channels)/(double)dz->infile->srate;
		maxfiletime = max(maxfiletime,timestor[n]);
	}
	midtime = maxfiletime/2.0;
	for(n=0;n<dz->all_words;n++) {
		switch(dz->mode) {
		case(MIX_SYNCEND):	timestor[n] = maxfiletime - timestor[n];		break;
		case(MIX_SYNCMID):	timestor[n] = midtime     - timestor[n]/2.0;	break;
		case(MIX_SYNCSTT):	timestor[n] = 0.0;								break;
		default:
			sprintf(errstr,"Unknown mode in mix_sync()\n");
			return(PROGRAM_ERROR);
		}
		sprintf(errstr,"%.5lf",timestor[n]);
		max_timeword = max(max_timeword,(int)strlen(errstr));
	}							
	for(n=0;n<dz->all_words;n++) {						
		if((exit_status = sync_and_output_mixfile_line
		(n,dz->wordstor[n],max_namelen,max_timeword,timestor[n],1.0,dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}

/********************** MIX_SYNC_A_MIXFILE ****************************/

int mix_sync_a_mixfile(dataptr dz)
{
	int    exit_status;
	int   srate;
	int   n, total_words;
	double maxfiletime = 0.0, midtime, *timestor;
	int    *maxwordsize = NULL, postdec;

	if(dz->linecnt==0) {
		sprintf(errstr,"No words in source file.\n");
		return(PROGRAM_ERROR);
	}
	if((dz->parray[MSY_TIMESTOR] = (double *)malloc(dz->linecnt * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for times store.\n");
		return(MEMORY_ERROR);
	}
	timestor = dz->parray[MSY_TIMESTOR];
	if((exit_status = establish_file_data_storage_for_mix((int)1,dz))<0)
		return(exit_status);
	total_words = 0;
	for(n=0;n<dz->linecnt;n++) {						/* for each sndfile name */
		if((exit_status = open_file_and_retrieve_props(n,dz->wordstor[total_words],&srate,dz))<0)
			return(exit_status);
		/*RWD NB calc from sampsize */
		timestor[n]  = (double)(dz->insams[0]/dz->infile->channels)/(double)dz->infile->srate;
		maxfiletime  = max(maxfiletime,timestor[n]);
		total_words += dz->wordcnt[n];
	}
	midtime = maxfiletime/2.0;
	for(n=0;n<dz->linecnt;n++) {
		switch(dz->mode) {
		case(MIX_SYNCEND):	timestor[n] = maxfiletime - timestor[n];		break;
		case(MIX_SYNCMID):	timestor[n] = midtime     - timestor[n]/2.0;	break;
		case(MIX_SYNCSTT):	timestor[n] = 0.0;								break;
		default:
			sprintf(errstr,"Unknown mode in mix_sync()\n");
			return(PROGRAM_ERROR);
		}
	}							
	postdec = DEFAULT_DECIMAL_REPRESENTATION;
	if((exit_status = retime_the_lines(timestor,postdec,dz))<0)
		return(exit_status);
	if((exit_status = timesort_mixfile(timestor,dz))<0)
		return(exit_status);
	if((exit_status = get_maxwordsize(&maxwordsize,dz))<0)
		return(exit_status);
	if((exit_status = output_mixfile_lines(maxwordsize,dz))<0)
		return(exit_status);
	free(maxwordsize);
	return(FINISHED);
}

/************************* SYNC_AND_OUTPUT_MIXFILE_LINE *******************************/

int sync_and_output_mixfile_line
(int lineno,char *filename,int max_namelen,int max_timeword,double timestor,double gain,dataptr dz)
{
	int space_count, n;
	if((space_count = max_namelen - strlen(filename))<0) {
		sprintf(errstr,"Anomaly 1 in space counting: sync_and_output_mixfile_line()\n");
		return(PROGRAM_ERROR);
	}
	space_count++;
	fprintf(dz->fp,"%s",filename);
	for(n=0;n<space_count;n++)
		fprintf(dz->fp," ");

	sprintf(errstr,"%.5lf",timestor);
	if((space_count = max_timeword - strlen(errstr))<0) {
		sprintf(errstr,"Anomaly 2 in space counting: sync_and_output_mixfile_line()\n");
		return(PROGRAM_ERROR);
	}
	space_count++;
	fprintf(dz->fp,"%s",errstr);
	for(n=0;n<space_count;n++)
		fprintf(dz->fp," ");
	switch(dz->iparray[MSY_CHANS][lineno]) {
	case(MONO):
		if(flteq(gain,1.0))
			fprintf(dz->fp,"  1  1.0  C\n");
		else
			fprintf(dz->fp,"  1  %.5lf  C\n",gain);
		break;
	case(STEREO):	
		if(flteq(gain,1.0))
			fprintf(dz->fp,"  2  1.0  L  1.0  R\n");
		else
			fprintf(dz->fp,"  2  %.5lf  L  %.5lf  R\n",gain,gain);
		break;
	default:
		sprintf(errstr,"Unknown case in create_and_output_mixfile_line()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}
