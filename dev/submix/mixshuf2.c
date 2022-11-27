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



/* floatsam version : no changes */
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


static int  check_chans(int lineno,int oldwordpos,dataptr dz);
static void insert_word(char **permm,int permlen,int *name_index,int m,int t,dataptr dz);
static void prefix_word(char **perm,int permlen,int *name_index,int m,dataptr dz);
static void shuflup_words(char **perm,int permlen,int k);
static int  output_mixfile_line(char **wordstor,int total_words,int wordcnt,int *maxwordsize,dataptr dz);

/************************* DO_TIME_AND_NAME_COPY ************************/

int do_time_and_name_copy(dataptr dz)
{
	int  exit_status;
	int  extra_words = 0;
	int  *maxwordsize;
	int total_words = 0, new_words = 0, words_above = 0, moveindex;
	int new_total_words, oldwordpos, orig_oldwordpos, newwordpos;
	int  n, lineno, wordno, newlines, lines_above, oldlinecnt;

	if(dz->mode==MSH_DUPL_AND_RENAME) {	   /* space for the new sndfilename must bhave been allocated */
		if(dz->extra_word<0) {
			sprintf(errstr,"extra_word not accounted for: do_time_and_name_copy()\n");
			return(PROGRAM_ERROR);
		}
		extra_words = 1;
	}

	newlines    = dz->iparam[MSH_ENDLINE] - dz->iparam[MSH_STARTLINE];
	lines_above = dz->linecnt - dz->iparam[MSH_ENDLINE];

	for(n=0;n<dz->linecnt;n++)
		total_words += dz->wordcnt[n];

	for(n=dz->iparam[MSH_ENDLINE];n<dz->linecnt;n++)
		words_above += dz->wordcnt[n];

	for(n=dz->iparam[MSH_STARTLINE];n<dz->iparam[MSH_ENDLINE];n++)
		new_words += dz->wordcnt[n];

	new_total_words = total_words + new_words;

	if((dz->wordstor = (char **)realloc(dz->wordstor,(new_total_words+extra_words) * sizeof(char *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for word store.\n");
		return(MEMORY_ERROR);								/* alloc space for all new words */
	}
	for(n=total_words+extra_words; n < new_total_words+extra_words;n++)
		dz->wordstor[n] = NULL;

	oldlinecnt = dz->linecnt;
	dz->linecnt += newlines;

	if((dz->wordcnt = (int *)realloc(dz->wordcnt,dz->linecnt * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for word count store.\n");
		return(MEMORY_ERROR);								/* alloc space for all new line wordcnts */
	}
	for(n=oldlinecnt; n < dz->linecnt;n++)
		dz->wordcnt[n] = 0;

	dz->all_words  += new_words;							/* update total wordcnt: good housekeeping */
			
	oldwordpos      = total_words + extra_words     - 1;
	newwordpos      = new_total_words + extra_words - 1;
												/* 'MOVE' WORDS BEYOND THE ACTED-ON LINES + ANY EXTRA_WORD */
	for(n=0;n<(words_above+extra_words);n++) {	
		dz->wordstor[newwordpos] = dz->wordstor[oldwordpos];
		newwordpos--;
		oldwordpos--;
		if(oldwordpos<0) {
			sprintf(errstr,"Accounting problem 0: do_time_and_name_copy()\n");
			return(PROGRAM_ERROR);
		}
	}
	for(n=oldlinecnt;n<dz->linecnt;n++)			/* Put correct wordcnt in place for these moved lines */
		dz->wordcnt[n] = dz->wordcnt[n - newlines];
 
 	if(dz->mode==MSH_DUPL_AND_RENAME)  			/* adjust address where new sndfilename is now found */
		dz->extra_word += new_words;

	oldwordpos = 0;								/* GO TO THE END OF THE WORDS TO DUPLICATE */
	for(lineno=0;lineno<dz->iparam[MSH_ENDLINE];lineno++)
		oldwordpos += dz->wordcnt[lineno];
	oldwordpos--;
	newwordpos = oldwordpos + new_words; 		/* MOVING BACKWARDS, DUPLICATE THE ACTED-ON LINES */

	for(lineno = dz->iparam[MSH_ENDLINE]-1,moveindex = newlines;lineno>=dz->iparam[MSH_STARTLINE];lineno--,moveindex--) {

		orig_oldwordpos = oldwordpos;			/* CREATE WORDS IN A DUPLICATE LINE */
		for(wordno=dz->wordcnt[lineno]-1;wordno>=0;wordno--) {		
												/* CHECKING THE CHANNEL If process DEMANDS IT */
  			if(wordno==MIX_CHANPOS && dz->mode==MSH_DUPL_AND_RENAME && dz->vflag[MSH_NOCHECK]==FALSE) {
				if((exit_status = check_chans(lineno,oldwordpos,dz))<0)
					return(exit_status);
			}									/* SUBSTITUTING NEW NAME: IF process DEMANDS IT */
  			if(wordno==MIX_NAMEPOS && dz->mode==MSH_DUPL_AND_RENAME)
				dz->wordstor[newwordpos] = dz->wordstor[dz->extra_word];
			else
				dz->wordstor[newwordpos] = dz->wordstor[oldwordpos];
			oldwordpos--;
			newwordpos--;
			if(lineno > 0 && wordno > 0) {
				if(oldwordpos<0) {
					sprintf(errstr,"Accounting problem 1: do_time_and_name_copy()\n");
					return(PROGRAM_ERROR);
				}
			}
		}
		dz->wordcnt[lineno+moveindex] = dz->wordcnt[lineno];

		oldwordpos = orig_oldwordpos;			/* THEN ALSO MOVE WORDS OF ORIGINAL LINE */
		if(newwordpos < oldwordpos) {
			sprintf(errstr,"Accounting problem 2: do_time_and_name_copy()\n");
			return(PROGRAM_ERROR);
		}
		for(wordno=dz->wordcnt[lineno]-1;wordno>=0;wordno--) {
			dz->wordstor[newwordpos] = dz->wordstor[oldwordpos];
			oldwordpos--;
			newwordpos--;
		}
		if(moveindex-1 < 0) {
			sprintf(errstr,"Accounting problem 2a: do_time_and_name_copy()\n");
			return(PROGRAM_ERROR);
		}
		dz->wordcnt[lineno+moveindex-1] = dz->wordcnt[lineno];
	}
											
	if((exit_status = get_maxwordsize(&maxwordsize,dz))<0)
		return(exit_status);
	if((exit_status = output_mixfile_lines(maxwordsize,dz))<0)
		return(exit_status);
	free(maxwordsize);
	return(FINISHED);
}

/************************* OUTPUT_MIXFILE_LINES ******************************/

int output_mixfile_lines(int *maxwordsize,dataptr dz)
{
	int exit_status;
	int n;
	int total_words = 0;
	for(n=0;n<dz->linecnt;n++) {						
		if((exit_status = output_mixfile_line(dz->wordstor,total_words,dz->wordcnt[n],maxwordsize,dz))<0)
			return(exit_status);
		total_words += dz->wordcnt[n];
	}
	return(FINISHED);
}

/************************* CHECK_CHANS ******************************/

int check_chans(int lineno,int oldwordpos,dataptr dz)
{
	int chans;
	if(sscanf(dz->wordstor[oldwordpos],"%d",&chans)!=1) {
		sprintf(errstr,"Failed to read channel value:line %d: check_chans()\n",lineno+1);
		return(PROGRAM_ERROR);
	}
	if(chans!=dz->otherfile->channels) {
		sprintf(errstr,"New named file has incompatible channel cnt: line %d: check_chans()\n",lineno+1);
		return(DATA_ERROR);
	}
	return(FINISHED);
}

/************************* DO_NAME_REVERSE ******************************/

int do_name_reverse(dataptr dz)
{
	int exit_status;
	int  *maxwordsize;
	int *name_index;
	int half_linecnt;
	char *temp;
	int lineno, newlineno, total_words = 0;

	if((name_index = (int *)malloc(dz->linecnt * sizeof(int)))==NULL) {
		sprintf(errstr,"do_name_reverse()\n");
		return(PROGRAM_ERROR);
	}
	if((exit_status = get_maxwordsize(&maxwordsize,dz))<0) {
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
	if((exit_status = output_mixfile_lines(maxwordsize,dz))<0) {
		free(maxwordsize);
		free(name_index);
		return(exit_status);
	}
	free(maxwordsize);
	free(name_index);
	return(FINISHED);
}

/************************* DO_NAME_FREEZE ******************************/

int do_name_freeze(dataptr dz)
{
	int exit_status;
	int  *maxwordsize;
	int *name_index;
	int lineno, total_words = 0;

	if((name_index = (int *)malloc(dz->linecnt * sizeof(int)))==NULL) {
		sprintf(errstr,"do_name_reverse()\n");
		return(PROGRAM_ERROR);
	}
	if((exit_status = get_maxwordsize(&maxwordsize,dz))<0) {
		free(name_index);
		return(exit_status);
	}	
	for(lineno=0;lineno<dz->linecnt;lineno++) {
		name_index[lineno] = total_words;
		total_words += dz->wordcnt[lineno];
	}

	for(lineno=dz->iparam[MSH_STARTLINE];lineno < dz->iparam[MSH_ENDLINE];lineno++)
		dz->wordstor[name_index[lineno]] = dz->wordstor[name_index[dz->iparam[MSH_STARTLINE]]];

	if((exit_status = output_mixfile_lines(maxwordsize,dz))<0) {
		free(name_index);
		free(maxwordsize);
		return(exit_status);
	}
	free(name_index);
	free(maxwordsize);
	return(FINISHED);
}

/************************** RANDOMISE_NAMES **************************/

int randomise_names(dataptr dz)
{
	int  exit_status;
	int  *maxwordsize;
	int *name_index;
	int lineno, total_words = 0;
	int n, m, t; 
	int permlen = dz->iparam[MSH_ENDLINE] - dz->iparam[MSH_STARTLINE];
	char **permm;

	if((name_index = (int *)malloc(dz->linecnt * sizeof(int)))==NULL) {
		sprintf(errstr,"randomise_names(): 1\n");
		return(PROGRAM_ERROR);
	}
	if((permm = (char **)malloc(permlen * sizeof(char *)))==NULL) {
		free(name_index);
		sprintf(errstr,"randomise_names(): 2\n");
		return(PROGRAM_ERROR);
	}
	if((exit_status = get_maxwordsize(&maxwordsize,dz))<0) {
		free(name_index);
		free(permm);
		return(exit_status);
	}	
	for(lineno=0;lineno<dz->linecnt;lineno++) {
		name_index[lineno] = total_words;
		total_words += dz->wordcnt[lineno];
	}
	for(n=0;n<permlen;n++) {
		t = (int)(drand48() * (double)(n+1));
		if(t==n)
			prefix_word(permm,permlen,name_index,n+dz->iparam[MSH_STARTLINE],dz);
		else
			insert_word(permm,permlen,name_index,n+dz->iparam[MSH_STARTLINE],t,dz);
	}
	for(n=0,m=dz->iparam[MSH_STARTLINE];n<permlen;n++,m++)
		dz->wordstor[name_index[m]] = permm[n];
	if((exit_status = output_mixfile_lines(maxwordsize,dz))<0) {
		free(name_index);
		free(permm);
		free(maxwordsize);
		return(exit_status);
	}
	free(name_index);
	free(permm);
	free(maxwordsize);
	return(FINISHED);
}

/****************************** INSERT_WORD ****************************/

void insert_word(char **permm,int permlen,int *name_index,int m,int t,dataptr dz)
{   
	shuflup_words(permm,permlen,t+1);
	permm[t+1] = dz->wordstor[name_index[m]];
}

/****************************** PREFIX_WORD ****************************/

void prefix_word(char **permm,int permlen,int *name_index,int m,dataptr dz)
{
	shuflup_words(permm,permlen,0);
	permm[0] = dz->wordstor[name_index[m]];
}

/****************************** SHUFLUP_WORDS ****************************/

void shuflup_words(char **permm,int permlen,int k)
{
	int n;
	char **i;
	int z = permlen - 1;
	i = &(permm[z]);
	for(n = z; n > k; n--) {
		*i = *(i-1);
		i--;
	}
}

/************************** OUTPUT_MIXFILE_LINE ***************************/

int output_mixfile_line(char **wordstor,int total_words,int wordcnt,int *maxwordsize,dataptr dz)
{
	int n, m, spacecnt;
	int wordno;
	for(n = 0;n<wordcnt;n++) {
		wordno = total_words + n;
		if((spacecnt = maxwordsize[n] - strlen(wordstor[wordno]))<0) {
			sprintf(errstr,"Word alignment error: output_mixfile_line()\n");
			return(PROGRAM_ERROR);
		}
		spacecnt++;
		fprintf(dz->fp,"%s",wordstor[wordno]);
		for(m=0;m<spacecnt;m++)
			fprintf(dz->fp," ");
	}
	fprintf(dz->fp,"\n");
	return(FINISHED);
}
