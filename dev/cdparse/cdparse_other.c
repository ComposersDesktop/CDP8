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



/* floats version */
/* RECEIVES FROM TK
 *
 *	Name of input file.
 *
 * SENDS TO TK
 *
 * (1)	Various file constants, which TK needs to STORE, to pass back at subsequent stage.
 */

/**** FEB 1999 MODS 
 *
 *	Force cdparams() output to be printed to stdout on a single line, with items separated by
 *	single spaces, forming a valid TK list
 *
 *	Recognise Float-sndfiles.
 */

#include <stdio.h>
#include <osbind.h>
#include <math.h>
#include <float.h>
#include <cdplib.h>
#include <structures.h>
#include <cdpstate.h>
#include <tkglobals.h>
#include <filetype.h>
#include <globcon.h>
#include <headread.h>
#include <speccon.h>
#include <processno.h>
#include <stdlib.h>
#include <sfsys.h>
#include <ctype.h>
#include <limits.h>
#include <string.h>


/* NEW CODE June 1st 2000 */

#ifndef CDP97
#define CDP97				//RWD.6.98 need this!
#endif
#ifdef _DEBUG
#include <assert.h>
#endif
void maxsamp(int ifd,double *maxamp,double *maxloc,int *maxrep);
/* END OF NEW CODE */

#define	POSSIBLE				(TRUE)

char	errstr[400];

static void initialise_fileptr(infileptr inputfile);
static int  check_for_data_in_file(char *filename,infileptr inputfile);
static int  set_wlength_and_check_data(char *filename,int ifd,infileptr inputfile);
static int  check_pitch_or_transpos_binary_data(infileptr inputfile,char *filename,int ifd);
static int  megaparsec(char *filename,FILE *fp,infileptr inputfile);
static int  initial_parse_textfile(char *filename,FILE *fp,int *linelist,
							int *has_comments,int *dB_notation,int *total_wordcnt,
							int *numcnt,int *linecnt,int *max_line_word_cnt,int *min_line_word_cnt);
static void initial_textfile_check(int dB_notation,int *sndlist,int *mixlist,int *brklist,int *numlist,
							int numcnt,int total_wordcnt,int max_line_word_cnt,int in_line_word_cnt);
static int  valid_start_of_mixfile_line(int k,int *chans,int lineno,char **wordstor,int *srate,
							int *mixlist,infileptr inputfile,char *filename,double *dur);
#ifdef IS_CDPARSE
static int  is_valid_end_of_mixfile_line(int j,int chans,int lineno,char **wordstor,int *wordcnt);
#else
static int  is_valid_end_of_mixfile_line(int j,int chans,int lineno,char **wordstor,int *wordcnt,char *filename);
#endif
static int  confirm_true_stereo(int *chans,int *is_left,int *is_right,int *is_central,char *panword);
static int  is_a_possible_mixfile(char **wordstor,int *wordcnt,int linecnt,
			int *mixlist,int *srate,infileptr inputfile, char *filename,double *dur);
static int  OK_level(char *str);
static int  is_a_dB_value(char *str);
static int  get_leveldb_frame(char *str,double *val);
static int  is_a_valid_sndfile_for_mixfile(char *filename,char *sndfilename,int *srate,
			int chans,int linecnt,infileptr inputfile,double time,double *dur);
static int  check_for_brklist_and_count_numbers(char **wordstor,int *wordcnt,
			int linecnt,int *brklist,infileptr inputfile,char *filename);
static int  check_for_a_linelist(int line_word_cnt,int *linelist,char **wordstor,infileptr inputfile);
static int  store_wordlist_without_comments
				(FILE *fp,char *temp,int total_wordcnt,char **wordstor,int *wordcnt,infileptr inputfile);
/* NEW CODE */
static int  assign_type(int sndlist,int numlist,int brklist,int mixlist,int synclist,int linelist,
			int extended_brkfile,infileptr inputfile);
/* NEW CODE */
static int  check_for_sndlist(char **wordstor,int *wordcnt,int linecnt,int *sndlist);
static int  check_for_a_synclist(int *synclist,char **wordstor,int *wordcnt,infileptr inputfile,char *filename);
static int  is_a_valid_sndfile_for_sync(char *filename,char *sndfilename,int *srate,int linecnt,infileptr inputfile);
static int  set_sndlist_flags(infileptr inputfile,char **wordstor);
static int	assign_brkfile_type(infileptr inputfile,int linelist);

#ifdef NO_DUPLICATE_SNDFILES
#ifdef IS_CDPARSE
static int  duplicated_filenames(int linecnt,char **wordstor,int *wordcnt);
#else
static int  duplicated_filenames(int linecnt,char **wordstor,int *wordcnt,char *filename);
#endif
#endif

static int  check_for_an_extended_brkfile
			(int *extended_brkfile,char **wordstor,int *wordcnt,int linecnt,int total_words,infileptr inputfile);
static int  extract_value_pair
			(int *n,double *time, double *val,double *maxval,double *minval,double *lasttime, char **wordstor);
static void utol(char *str);

extern int smpflteq(double a,double b);

int do_exit(int exit_status);

/* RWD TODO: use sndsystem */
#ifdef IS_CDPARSE
static int nuheadread(infileptr inputfile,int ifd,char *filename,double *maxamp,double *maxloc, int *maxrep,int getmax);
#endif
static int parse_a_textfile(char *filename,infileptr inputfile);

static int copy_to_infileptr(infileptr i, fileptr f);

/***************************** MAIN **************************/

#ifdef IS_CDPARSE

extern int sloom = 1;
extern int sloombatch = 0;

int main(int argc,char *argv[])
{
	char *filename;
	infileptr inputfile;
	char temp[2000], *p;	/*** Added FEB 1999 *****/
	int getmaxinfo = 1;
#else

int cdparse(char *filename,infileptr inputfile)
{
	int getmaxinfo = 0;

#endif

	int  exit_status;
	int  ifd;
 	double maxamp = 0.0, maxloc = -1.0;
 	int maxrep = 0;
	int getmax = 0;
#ifdef IS_CDPARSE

/* NEW CODE : June 1st 2000 */
	if(argc!=3) {
		sprintf(errstr,"Incorrect call to cdparse()\n");
		exit_status = FAILED;
		return do_exit(exit_status);
	}
	if(sscanf(argv[2],"%d",&getmax)!=1) {
		sprintf(errstr,"Incorrect call to cdparse()\n");
		exit_status = FAILED;
		return do_exit(exit_status);
	}
	filename = argv[1];

	if(file_has_invalid_startchar(filename)) {
		sprintf(errstr,	"Filename %s has invalid starting character(s)\n",filename);
		exit_status = FAILED;
		return do_exit(exit_status);
	}
	if(file_has_invalid_extension(filename)) {
		sprintf(errstr,	"Filename %s has invalid extension for CDP work\n",filename);
		exit_status = FAILED;
		return do_exit(exit_status);
	}
	if((inputfile = (infileptr)malloc(sizeof(struct filedata)))==NULL) {
		sprintf(errstr,"Insufficient memory for parsing file %s.\n",filename);
		exit_status = MEMORY_ERROR;
		return do_exit(exit_status);
	}
#endif

	initialise_fileptr(inputfile);
	if((ifd = sndopenEx(filename,0,CDP_OPEN_RDONLY)) >= 0) {
		if((exit_status = readhead(inputfile,ifd,filename,&maxamp,&maxloc,&maxrep,getmax,getmaxinfo))<0) {
			if(sndcloseEx(ifd)<0) {
				sprintf(errstr,	"Failed to close file %s\n",filename);
				exit_status = SYSTEM_ERROR;
				return do_exit(exit_status);
			}
			return do_exit(exit_status);
		}
/* TEST */
fprintf(stdout,"INFO: 3333 inputfile->filetype = %d\n",inputfile->filetype);
fflush(stdout);
/* TEST */
		if(getmaxinfo && getmax && (maxrep < 0)) {	/* if maxsamp required & not in header, get it by searching file */
//TW only get maxsamp if it's a soundfile
			if(inputfile->filetype == SNDFILE) {
				maxsamp(ifd,&maxamp,&maxloc,&maxrep);		  /*RWD TODO */
				sndseekEx(ifd,0,0);		   /* RWD TODO */
			}
		}
		if((inputfile->insams = sndsizeEx(ifd))<0) {	    			
			sprintf(errstr, "Can't read size of input file %s\n",filename);
			exit_status = DATA_ERROR;
			if(sndcloseEx(ifd)<0) {
				sprintf(errstr,	"Failed to close file %s: %s\n",filename,sferrstr());
				exit_status = SYSTEM_ERROR;
			}
			return do_exit(exit_status);
		}
		if((exit_status = check_for_data_in_file(filename,inputfile))<0) {
			if(sndcloseEx(ifd)<0) {
				sprintf(errstr,	"Failed to close file %s\n",filename);
				exit_status = SYSTEM_ERROR;
			}
			return do_exit(exit_status);
		}
		switch(inputfile->filetype) {
		case(ANALFILE):
		case(PITCHFILE):
		case(TRANSPOSFILE):
		case(FORMANTFILE):
			inputfile->nyquist   = inputfile->origrate/2.0;
			inputfile->frametime = (float)(1.0/inputfile->arate);
			/*inputfile->insams    = inputfile->infilesize/sizeof(float);	*/
			if((exit_status = set_wlength_and_check_data(filename,ifd,inputfile))<0) {
				if(sndcloseEx(ifd)<0) {
					sprintf(errstr,	"Failed to close file %s\n",filename);
					exit_status = SYSTEM_ERROR;
				}
				return do_exit(exit_status);
			}
			inputfile->duration  = inputfile->wlength * inputfile->frametime;
			if(inputfile->duration <= FLTERR) {
				sprintf(errstr,	"file %s has ZERO duration : ignoring it\n",filename);
				exit_status = DATA_ERROR;
				return do_exit(exit_status);
			}
			break;
		case(ENVFILE):	
			if(sndgetprop(ifd,"window size",(char *)&inputfile->window_size,sizeof(float)) < 0) {
				sprintf(errstr,	"Cannot read window size for file %s\n",filename);
				exit_status = DATA_ERROR;
				return do_exit(exit_status);
			}
			inputfile->duration  = inputfile->window_size * inputfile->insams * MS_TO_SECS;
			if(inputfile->duration <= FLTERR) {
				sprintf(errstr,	"file %s has zero duration : ignoring it\n",filename);
				exit_status = DATA_ERROR;
				return do_exit(exit_status);
			}
			break;
		case(SNDFILE):
		case(FLT_SNDFILE):		   /*RWD 2001 added - needed? */
			/*inputfile->insams    = inputfile->infilesize/sizeof(short);	*/
			inputfile->nyquist   = (double)inputfile->srate/2.0;
			inputfile->duration  = (double)(inputfile->insams/inputfile->channels)/(double)(inputfile->srate);
			if(inputfile->duration <= FLTERR) {
				sprintf(errstr,	"file %s has zero duration : ignoring it\n",filename);
				exit_status = DATA_ERROR;
				return do_exit(exit_status);
			}
			//RWD.7.99 need to reset this to FLT_SNDFILE if stype==FLOAT ?
			break;
		default:
			exit_status = DATA_ERROR;
			sprintf(errstr,"Unknown type of file: file %s\n",filename);
			if(sndcloseEx(ifd)<0) {
				sprintf(errstr,	"Failed to close file %s\n",filename);
				exit_status = SYSTEM_ERROR;
			}
			return do_exit(exit_status);
		}
		if(sndcloseEx(ifd)<0) {
			sprintf(errstr,	"Failed to close file %s\n",filename);
			exit_status = SYSTEM_ERROR;
			return do_exit(exit_status);
		}
	} else {
		if((exit_status = parse_a_textfile(filename,inputfile)) < 0)
			return(exit_status);

	}

#ifdef IS_CDPARSE

	p = temp;										  
	sprintf(p,"%d ",inputfile->filetype);			/* 0 */	
	p = temp + strlen(temp);
//TW CORRECTED, FOR COMPATIBILITY WITH tkinput: first value OUTPUT is NOW in samples (it was, infilesize)
//	sprintf(p,"%ld ",  inputfile->infilesize		  /* 1 */	p = temp + strlen(temp);
	sprintf(p,"%ld ",  inputfile->insams);			  /* 1 */	p = temp + strlen(temp);
	sprintf(p,"%ld ",  inputfile->insams);			  /* 2 */	p = temp + strlen(temp);
	sprintf(p,"%ld ",  inputfile->srate);			  /* 3 */	p = temp + strlen(temp);
	sprintf(p,"%ld ",  inputfile->channels);		  /* 4 */	p = temp + strlen(temp);
	sprintf(p,"%ld ",  inputfile->wanted);			  /* 5 */	p = temp + strlen(temp);
	sprintf(p,"%ld ",  inputfile->wlength);			  /* 6 */	p = temp + strlen(temp);
	sprintf(p,"%ld ",  inputfile->linecnt);			  /* 7 */	p = temp + strlen(temp);
	sprintf(p,"%f ",   inputfile->arate);			  /* 8 */	p = temp + strlen(temp);
	sprintf(p,"%.12f ",inputfile->frametime);		  /* 9 */	p = temp + strlen(temp);
	sprintf(p,"%lf ",  inputfile->nyquist);			  /* 10 */	p = temp + strlen(temp);
	sprintf(p,"%lf ",  inputfile->duration);		  /* 11 */	p = temp + strlen(temp);
	sprintf(p,"%ld ",  inputfile->stype);			  /* 12 */	p = temp + strlen(temp);
	sprintf(p,"%ld ",  inputfile->origstype);		  /* 13 */	p = temp + strlen(temp);
	sprintf(p,"%ld ",  inputfile->origrate);		  /* 14 */	p = temp + strlen(temp);
	sprintf(p,"%d ",   inputfile->Mlen);			  /* 15 */	p = temp + strlen(temp);
	sprintf(p,"%d ",   inputfile->Dfac);			  /* 16 */	p = temp + strlen(temp);
	sprintf(p,"%ld ",  inputfile->origchans);		  /* 17 */	p = temp + strlen(temp);
	sprintf(p,"%d ",   inputfile->specenvcnt);		  /* 18 */	p = temp + strlen(temp);
	sprintf(p,"%d ",   inputfile->out_chans);		  /* 19 */	p = temp + strlen(temp);
//TW REVISED
//	sprintf(p,"%ld ",  inputfile->descriptor_bytes);  /* 20 *	p = temp + strlen(temp);   /*RWD TODO : ??? */
	sprintf(p,"%ld ",  inputfile->descriptor_samps);  /* 20 */	p = temp + strlen(temp);
	sprintf(p,"%d ",   inputfile->is_transpos);		  /* 21 */	p = temp + strlen(temp);
	sprintf(p,"%d ",   inputfile->could_be_transpos); /* 22 */	p = temp + strlen(temp);
	sprintf(p,"%d ",   inputfile->could_be_pitch);	  /* 23 */	p = temp + strlen(temp);
	sprintf(p,"%d ",   inputfile->different_srates);  /* 24 */	p = temp + strlen(temp);
	sprintf(p,"%d ",   inputfile->duplicate_snds);	  /* 25 */	p = temp + strlen(temp);
	sprintf(p,"%ld ",  inputfile->brksize);			  /* 26 */	p = temp + strlen(temp);
	sprintf(p,"%ld ",  inputfile->numsize);			  /* 27 */	p = temp + strlen(temp);
	sprintf(p,"%ld ",  inputfile->all_words);		  /* 28 */	p = temp + strlen(temp);
	sprintf(p,"%f ",   inputfile->window_size);		  /* 29 */	p = temp + strlen(temp);
	sprintf(p,"%lf ",  inputfile->minbrk);			  /* 30 */	p = temp + strlen(temp);
	sprintf(p,"%lf ",  inputfile->maxbrk);			  /* 31 */	p = temp + strlen(temp);
	sprintf(p,"%lf ",  inputfile->minnum);			  /* 32 */	p = temp + strlen(temp);
	sprintf(p,"%lf ", inputfile->maxnum);			  /* 33 */	p = temp + strlen(temp); 
//TW maxsamp is now float value
	sprintf(p,"%lf ", maxamp);				/* 34 value of maxsamp */	p = temp + strlen(temp);
	sprintf(p,"%ld ", (int)round(maxloc));		/* 35 location maxsamp */	p = temp + strlen(temp);
	sprintf(p,"%ld\n", maxrep);					/* 36 no of repets, [old usage, -1 flagged no maxval] */
	fprintf(stdout,"%s",temp);
	fprintf(stdout,"END\n");
	fflush(stdout);
	return(SUCCEEDED);
#else
	return(FINISHED);
#endif
}

/********************************** CHECK_FOR_DATA_IN_FILE ************************************/

int check_for_data_in_file(char *filename,infileptr inputfile)
{
	switch(inputfile->filetype) {
	case(FORMANTFILE):
		/*if(inputfile->infilesize <= (int)(DESCRIPTOR_DATA_BLOKS * inputfile->specenvcnt * sizeof(float))) */
		if(inputfile->insams <= (int)(DESCRIPTOR_DATA_BLOKS * inputfile->specenvcnt)) {
			sprintf(errstr, "FILE %s contains no data.\n",filename);
			return(DATA_ERROR);
		}
		break;
	default:
		if(inputfile->insams <= 0L) {
			sprintf(errstr, "FILE %s contains no data.\n",filename);
			return(DATA_ERROR);
		}
		break;
	}
	return(FINISHED);
}

/************************* INITIALISE_FILEPTR *************************/

void initialise_fileptr(infileptr inputfile)
{
	inputfile->filetype		= 0;

	/*inputfile->infilesize		= 0;*/
	inputfile->insams 			= 0;

	inputfile->srate 			= 0;
	inputfile->channels 		= 0;
	inputfile->stype 			= 0;
	inputfile->origstype		= 0;
	inputfile->origrate 		= 0;
	inputfile->Mlen 			= 0;
	inputfile->Dfac 			= 0;
	inputfile->origchans 		= 0;
	inputfile->specenvcnt		= 0;
	inputfile->wanted			= 0;
	inputfile->wlength			= 0;
	inputfile->out_chans		= 0;	
	inputfile->descriptor_samps = 0;

	inputfile->is_transpos			= FALSE;
	inputfile->could_be_transpos	= FALSE;
	inputfile->could_be_pitch		= FALSE;
	inputfile->different_srates		= FALSE;
	inputfile->duplicate_snds		= FALSE;

	inputfile->brksize 		= 0;
	inputfile->numsize 		= 0;

	inputfile->linecnt		= 0;
	inputfile->all_words	= 0;

	inputfile->arate		= 0.0f;
	inputfile->frametime 	= 0.0f;
	inputfile->window_size 	= 0.0f;
	inputfile->nyquist	 	= 0.0;
	inputfile->duration		= 0.0;
	inputfile->minbrk		= 0.0;
	inputfile->maxbrk		= 0.0;
	inputfile->minnum		= 0.0;
	inputfile->maxnum		= 0.0;
}

/***************************** SET_WLENGTH_AND_CHECK_DATA *******************************/

int set_wlength_and_check_data(char *filename,int ifd,infileptr inputfile)
{
	int exit_status;
	switch(inputfile->filetype) {
	case(ANALFILE):
		inputfile->wanted  = inputfile->channels;
		inputfile->wlength = inputfile->insams/inputfile->wanted;
		break;
	case(PITCHFILE):
	case(TRANSPOSFILE):
		inputfile->wlength = inputfile->insams;
		inputfile->wanted  = 1;	/* notional: to avoid error calls with bad command lines */
		if((exit_status = check_pitch_or_transpos_binary_data(inputfile,filename,ifd))<0)
			return(exit_status);
		break;
	case(FORMANTFILE):
		inputfile->wlength          = (inputfile->insams/inputfile->specenvcnt) - DESCRIPTOR_DATA_BLOKS;
		inputfile->descriptor_samps = inputfile->specenvcnt * DESCRIPTOR_DATA_BLOKS;
		inputfile->wanted           = inputfile->origchans;	/* temporary val for initialise_specenv */
														/* working val set in ...chunklen...()  */
	    break;
	default:
		sprintf(errstr,"Invalid case: set_wlength_and_check_data()\n",inputfile->filetype);
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/********************* CHECK_PITCH_OR_TRANSPOS_BINARY_DATA *************************/

int check_pitch_or_transpos_binary_data(infileptr inputfile,char *filename,int ifd)
{
	int samps_read;
	int samps_to_read = inputfile->insams;  
	int n;
	float *thisdata;
	
	if((thisdata = (float *)malloc((size_t)(samps_to_read * sizeof(float))))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for pitch_or_transpos_binary_data of file %s\n",filename);
		return(MEMORY_ERROR);
	}
	samps_read = fgetfbufEx(thisdata, samps_to_read,ifd,0);
	if(samps_read!= inputfile->insams) {
		/*RWD.6.98 lets discriminate!*/
		if(samps_read < 0){
			sprintf(errstr,"Failed to read infile data.\n");
			return SYSTEM_ERROR;
		}
		sprintf(errstr,"Problem reading binary pitch or transposition data in file %s.\n",filename);
		return(PROGRAM_ERROR);
	}
	switch(inputfile->filetype) {
	case(PITCHFILE):
		for(n=0;n<inputfile->insams;n++) {

//TW BRACKETED OUT: NEW CODE DEALS WITH UNPITCHED, AND SILENT WINDOWS		
/*
			if(thisdata[n] < FLTERR) {
				sprintf(errstr,"File %s contains unpitched windows: cannot proceed.\n",filename);
				return(DATA_ERROR);
			}
*/
			if(thisdata[n] >= inputfile->nyquist) {
				sprintf(errstr,"File %s contains pitches beyond nyquist: cannot proceed.\n",filename);
				return(DATA_ERROR);
			}
		}
		break;
	case(TRANSPOSFILE):
		for(n=0;n<inputfile->insams;n++) {
			if(thisdata[n] < MIN_TRANSPOS || thisdata[n] >= MAX_TRANSPOS) {
				sprintf(errstr,"Transposition data item %d (%f):file %s: out of range (%lf to %lf)\n",				
				n+1,thisdata[n],filename,MIN_TRANSPOS,MAX_TRANSPOS);
				return(DATA_ERROR);
			}
		}
		inputfile->is_transpos = TRUE;
		break;
	}
	free(thisdata);
	return(FINISHED);
}

/************************************* MEGAPARSEC ******************************/

int megaparsec(char *filename,FILE *fp,infileptr inputfile)
{
	int  exit_status;
/* NEW CODE */	
	int	 extended_brkfile = POSSIBLE;
/* NEW CODE */	
	int  sndlist      = POSSIBLE;
	int  numlist      = POSSIBLE;
	int  brklist      = POSSIBLE;
	int  mixlist      = POSSIBLE;
	int  linelist     = POSSIBLE;
	int  synclist	  = POSSIBLE;
	int  has_comments = FALSE;
	int  dB_notation  = FALSE;
	char temp[200];
	char **wordstor;
	int  *wordcnt;
	int numcnt = 0, linecnt = 0, total_wordcnt = 0, max_line_word_cnt = 0, min_line_word_cnt, n, srate;
	double dur = 0.0;

	if((exit_status = initial_parse_textfile(filename,fp,&linelist,&has_comments,&dB_notation,
	&total_wordcnt,&numcnt,&linecnt,&max_line_word_cnt,&min_line_word_cnt))<0)
		return(exit_status);
	initial_textfile_check(dB_notation,&sndlist,&mixlist,&brklist,
	&numlist,numcnt,total_wordcnt,max_line_word_cnt,min_line_word_cnt);

	if((wordstor = (char **)malloc(total_wordcnt * sizeof(char *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for wordstor: file %s\n",filename);
		return(MEMORY_ERROR);
	}
	for(n=0;n<total_wordcnt;n++)		/* initialise, for testing and safe freeing */
		wordstor[n] = NULL;

	inputfile->all_words = total_wordcnt;

	if((wordcnt  = (int *)malloc(linecnt * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for wordcnt: file %s\n",filename);
		return(MEMORY_ERROR);
	}
	for(n=0;n<linecnt;n++)		/* initialise, for later testing */
		wordcnt[n] = 0;

	if(fseek(fp,0,0)<0) {
		sprintf(errstr,"fseek failed in megaparsec(): 1\n");
		return(PROGRAM_ERROR);
	}

	if((exit_status = store_wordlist_without_comments(fp,temp,inputfile->all_words,wordstor,wordcnt,inputfile))<0)
		return(exit_status);

	if(sndlist==POSSIBLE) {
		if((exit_status = check_for_sndlist(wordstor,wordcnt,linecnt,&sndlist))<0)
			return(exit_status);
	} else if(numlist==POSSIBLE) {
		if((exit_status = check_for_brklist_and_count_numbers(wordstor,wordcnt,linecnt,&brklist,inputfile,filename))<0)
			return(exit_status);
	}
	if(sndlist==POSSIBLE || numlist==POSSIBLE)		
		mixlist = FALSE;

	if(mixlist==POSSIBLE) {
		if((exit_status = is_a_possible_mixfile(wordstor,wordcnt,linecnt,&mixlist,&srate,inputfile,filename,&dur))<0)
			return(exit_status);
		if(mixlist==FALSE)
			inputfile->out_chans = 0;	
		else {
			inputfile->srate = srate;
			inputfile->duration = dur;
			synclist = FALSE;
			brklist  = FALSE;
			sndlist  = FALSE;
			extended_brkfile = FALSE;
		}
	}

	if(linelist==POSSIBLE) {
		if((exit_status = check_for_a_linelist(max_line_word_cnt,&linelist,wordstor,inputfile))<0)
			return(exit_status);
	}

	if(sndlist==POSSIBLE) {
		if((exit_status = set_sndlist_flags(inputfile,wordstor))<0)
			return(exit_status);
		if(inputfile->different_srates
		
#ifdef NO_DUPLICATE_SNDFILES
		|| inputfile->duplicate_snds
#endif		

		)
			synclist = FALSE;
	} else if(synclist==POSSIBLE) {
		if((exit_status = check_for_a_synclist(&synclist,wordstor,wordcnt,inputfile,filename))<0)
			return(exit_status);
	}
//TW REMOVED COMMENTS
	if(extended_brkfile==POSSIBLE) {
		if((exit_status = 
		check_for_an_extended_brkfile(&extended_brkfile,wordstor,wordcnt,linecnt,total_wordcnt,inputfile))<0)
			return(exit_status);
	}
	if((inputfile->filetype = assign_type(sndlist,numlist,brklist,mixlist,synclist,linelist,extended_brkfile,inputfile))<0)
		return(inputfile->filetype);
	return(FINISHED);
}

/************************************* INITIAL_PARSE_TEXTFILE ******************************/

int initial_parse_textfile					
(char *filename,FILE *fp,int *linelist,int *has_comments,int *dB_notation,
int *total_wordcnt,int *numcnt,int *linecnt,int *max_line_word_cnt,int *min_line_word_cnt)
{
	char temp[200], *p, *q, c;
	double val;
	int line_num_cnt = 0;
	int last_line_word_cnt = 0, last_line_num_cnt = 0;
	int line_word_cnt = 0;
	*max_line_word_cnt = 0;
	*min_line_word_cnt = LONG_MAX;

	while((c = (char)fgetc(fp))!=EOF) {
		if(!isalnum(c) && !ispunct(c) && !isspace(c)) {
			sprintf(errstr,"%s is not a valid CDP file\n",filename);
			return(DATA_ERROR);
		}
	}
	if(fseek(fp,0,0) != 0) {
		sprintf(errstr,"Cannot rewind to start of file %s\n",filename);
		return(DATA_ERROR);
	}


	while(fgets(temp,200,fp)!=NULL) {
		p = temp;
		if(is_a_comment(p)) {
			*has_comments = TRUE;
			continue;
		} else if(is_an_empty_line(p))
			continue;
		line_word_cnt = 0;
		line_num_cnt  = 0;
		while(get_word_from_string(&p,&q)) {
			line_word_cnt++;
			if(is_a_dB_value(q)) {
				*dB_notation = TRUE;
				(*numcnt)++;
				line_num_cnt++;
			} else if(get_float_from_within_string(&q,&val)) {
				(*numcnt)++;
				line_num_cnt++;
			}
		}
		if(*linecnt) {
		 	if(line_word_cnt != last_line_word_cnt
		 	|| line_num_cnt != last_line_num_cnt)
				*linelist = FALSE;
		}
		*max_line_word_cnt = max(*max_line_word_cnt,line_word_cnt);
		*min_line_word_cnt = min(*min_line_word_cnt,line_word_cnt);
		last_line_word_cnt = line_word_cnt;
		last_line_num_cnt  = line_num_cnt;
		*total_wordcnt	  += line_word_cnt;
		(*linecnt)++;
	}
	if(*total_wordcnt==0) {
		sprintf(errstr,"Cannot get any data from file %s\n",filename);
		return(DATA_ERROR);
	}
	return(FINISHED);
}

/************************************* INITIAL_TEXTFILE_CHECK ******************************/

void initial_textfile_check(int dB_notation,int *sndlist,int *mixlist,int *brklist,int *numlist,
							int numcnt,int wordcnt,int max_line_word_cnt,int min_line_word_cnt)
{
	if((numcnt < wordcnt) || dB_notation) {
		*numlist  = FALSE;
		*brklist  = FALSE;
	}
	if(numcnt == wordcnt) {
		*mixlist  = FALSE;
		*sndlist  = FALSE;
	}
	if(ODD(numcnt))
		*brklist = FALSE;

	if(numcnt>0)
		*sndlist  = FALSE;
	else
		*mixlist = FALSE;

	if(min_line_word_cnt<MIX_MINLINE || max_line_word_cnt>MIX_MAXLINE)
		*mixlist  = FALSE;

}

/************************************* VALID_START_OF_MIXFILE_LINE ******************************/
#ifdef IS_CDPARSE
int valid_start_of_mixfile_line
(int k,int *chans,int lineno,char **wordstor,int *srate,int *mixlist,infileptr inputfile,char *filename,double *dur)
{
	int exit_status;
	double time;
	char *sndfilename;
	sndfilename = wordstor[k++];
	if(sscanf(wordstor[k++],"%lf",&time)!=1) {
		*mixlist = FALSE;	
		return(FINISHED);
	}
	if(time<0.0) {
		*mixlist = FALSE;
		return(FINISHED);
	}
	if(sscanf(wordstor[k++],"%d",chans)!=1)	{
		*mixlist = FALSE;
		return(FINISHED);
	}
	if(*chans < MONO || *chans > STEREO) {
		*mixlist = FALSE;
		return(FINISHED);
	}

	if((exit_status = is_a_valid_sndfile_for_mixfile(filename,sndfilename,srate,*chans,lineno,inputfile,time,dur))<0)
		return(exit_status);
	if(exit_status == FALSE) {
		*mixlist = FALSE;
		return(FINISHED);
	}
	if(!OK_level(wordstor[k])) {		    			/* must be numeric, and witihin range */
		*mixlist = FALSE;
		return(FINISHED);
	}
	return(FINISHED);
}
#else
int valid_start_of_mixfile_line
(int k,int *chans,int lineno,char **wordstor,int *srate,int *mixlist,infileptr inputfile,char *filename,double *dur)
{
	int exit_status;
	double time;
	char *sndfilename;
	sndfilename = wordstor[k++];
	if(sscanf(wordstor[k++],"%lf",&time)!=1) {
		if(lineno>0) {
			fprintf(stdout,"WARNING: If %s is a mixfile: Invalid or missing time val on line %d\n",filename,lineno+1);
			fflush(stdout);
		}
		*mixlist = FALSE;
		return(FINISHED);
	}
	if(time<0.0) {
		if(lineno>0) {
			fprintf(stdout,"WARNING: If %s is a mixfile: starttime less than zero [%lf]: line %d\n",
			filename,time,lineno+1);
			fflush(stdout);
		}
		*mixlist = FALSE;
		return(FINISHED);
	}
	if(sscanf(wordstor[k++],"%d",chans)!=1)	{
		if(lineno>0) {
			fprintf(stdout,"WARNING: If %s is a mixfile: Invalid or missing channel val on line %d\n",
			filename,lineno+1);
			fflush(stdout);
		}
		*mixlist = FALSE;
		return(FINISHED);
	}
	if(*chans < MONO || *chans > STEREO) {
		if(lineno>0) {
			fprintf(stdout,"WARNING: If %s is a mixfile: Invalid channel-cnt on line %d\n",filename,lineno+1);
			fflush(stdout);
		}
		*mixlist = FALSE;
		return(FINISHED);
	}

	if((exit_status = is_a_valid_sndfile_for_mixfile(filename,sndfilename,srate,*chans,lineno,inputfile,time,dur))<0)
		return(exit_status);
	if(exit_status == FALSE) {
		*mixlist = FALSE;
		return(FINISHED);
	}
	if(!OK_level(wordstor[k])) {		    			/* must be numeric, and witihin range */
		if(lineno>0) {
			fprintf(stdout,"WARNING: If %s is a mixfile: Invalid [channel 1] level val on line %d\n",
			filename,lineno+1);
			fflush(stdout);
		}
		*mixlist = FALSE;
		return(FINISHED);
	}
	return(FINISHED);
}
#endif

/************************************* IS_VALID_END_OF_MIXFILE_LINE ******************************/

#ifdef IS_CDPARSE
int is_valid_end_of_mixfile_line(int j,int chans,int lineno,char **wordstor,int *wordcnt)
{
	double pan;
//TW UPDATE: AVOID WARNING
	int got_numeric_panval = sscanf(wordstor[j],"%lf",&pan);
	if(strcmp(wordstor[j],"C") && strcmp(wordstor[j],"L") && strcmp(wordstor[j],"R") 
	&& got_numeric_panval!=1)
		return(FALSE);										
	if(got_numeric_panval && (pan < MINPAN || pan >MAXPAN))
		return(FALSE);										
	if(chans==MONO) {
		if(wordcnt[lineno]>MIX_MIDLINE) {
			return(FALSE);
		} 
		return(TRUE);
	} else {
		 if(wordcnt[lineno]<MIX_MAXLINE) {
			return(FALSE);
		}
	}
	j++;
	if(!OK_level(wordstor[j++]))				/* level must be numeric or dB & in range  */
		return(FALSE);
	got_numeric_panval = 0;
//TW UPDATE: AVOID WARNING
	got_numeric_panval = sscanf(wordstor[j],"%lf",&pan);
	if(strcmp(wordstor[j],"C") && strcmp(wordstor[j],"L") && strcmp(wordstor[j],"R") 
	&& (got_numeric_panval !=1))	/* pan must be C,R,L or numeric */
		return(FALSE);
	if(got_numeric_panval && (pan < MINPAN || pan > MAXPAN)) {
		return(FALSE);										
	}
	return(TRUE);
}

#else

int is_valid_end_of_mixfile_line(int j,int chans,int lineno,char **wordstor,int *wordcnt,char *filename)
{
	double pan;
//TW UPDATE: AVOID WARNING
	int got_numeric_panval = sscanf(wordstor[j],"%lf",&pan);
	if(strcmp(wordstor[j],"C") && strcmp(wordstor[j],"L") && strcmp(wordstor[j],"R") 
	&& got_numeric_panval!=1) {
		if(lineno>0) {
			fprintf(stdout,"WARNING: If %s is a mixfile: Invalid (1st) pan val, line %d\n",filename,lineno+1);
			fflush(stdout);
		}
		return(FALSE);										
	}
	if(got_numeric_panval && (pan < MINPAN || pan >MAXPAN))  {
		if(lineno>0) {
			fprintf(stdout,"WARNING: If %s is a mixfile: Invalid (1st) pan val [%lf], line %d\n",
			filename,pan,lineno+1);
			fflush(stdout);
		}
		return(FALSE);										
	}

	if(chans==MONO) {
		if(wordcnt[lineno]>MIX_MIDLINE) {
			if(lineno>0) {
				fprintf(stdout,"WARNING: If %s is a mixfile: Too many words on line %d\n",filename,lineno+1);
				fflush(stdout);
			}
			return(FALSE);
		} 
		return(TRUE);
	} else {
		 if(wordcnt[lineno]<MIX_MAXLINE) {
			if(lineno>0) {
				fprintf(stdout,"WARNING: If %s is a mixfile: Too few entries for stereo file on line %d\n",
				filename,lineno+1);
				fflush(stdout);
			}
			return(FALSE);
		}
	}
	j++;
	if(!OK_level(wordstor[j++])) {				/* level must be numeric or dB & in range  */
		if(lineno>0) {
			fprintf(stdout,"WARNING: If %s is a mixfile: Missing level for chan2 on line %d\n",filename,lineno+1);
			fflush(stdout);
		}
		return(FALSE);
	}
	got_numeric_panval = 0;
//TW UPDATE: AVOID WARNING
	got_numeric_panval = sscanf(wordstor[j],"%lf",&pan);
	if(strcmp(wordstor[j],"C") && strcmp(wordstor[j],"L") && strcmp(wordstor[j],"R") 
	&& (got_numeric_panval !=1)) {	/* pan must be C,R,L or numeric */
		if(lineno>0) {
			fprintf(stdout,"WARNING: If %s is a mixfile: Missing chan2 pan val on line %d\n",
			filename,lineno+1);
			fflush(stdout);
		}
		return(FALSE);
	}
	if(got_numeric_panval && (pan < MINPAN || pan > MAXPAN)) {
		if(lineno>0) {
			fprintf(stdout,"WARNING: If %s is a mixfile: Invalid 2nd pan val [%lf], line %d\n",
			filename,pan,lineno+1);
			fflush(stdout);
		}
		return(FALSE);										
	}
	return(TRUE);
}
#endif
/************************************* CONFIRM_TRUE_STEREO ******************************/

int confirm_true_stereo(int *chans,int *is_left,int *is_right,int *is_central,char *panword)
{
	double pan;
	if(!strcmp(panword,"L")) {
		*is_left = TRUE;			/* panned hard Left */
		*chans = MONO;
	} else if(!strcmp(panword,"R")) {
		*is_right = TRUE;		/* panned hard Right */
		*chans = MONO;
	} else if(strcmp(panword,"C")) {
		if(sscanf(panword,"%lf",&pan)!=1) {
			sprintf(errstr,"Parsing error 2: confirm_true_stereo()\n");
			return(PROGRAM_ERROR);
		}
		if(pan <= -1.0) {
			*is_left = TRUE;		/* panned hard Left */
			*chans  = MONO;
		} else if(pan >= 1.0) {
			*is_right = TRUE;		/* panned hard Right */
			*chans  = MONO;
		}
// TW REMOVED COMMENTS
		  else
		  	*chans = STEREO;
	}
	  else
	  	*is_central = TRUE;
	return(FINISHED);
}

/**************** IS_A_POSSIBLE_MIXFILE **********************/

int is_a_possible_mixfile(char **wordstor,int *wordcnt,int linecnt,int *mixlist,
							int *srate,infileptr inputfile,char *filename,double *dur)
{
	int exit_status;
	int is_left = FALSE, is_right = FALSE, is_central = FALSE;
	int /*cnt = 0,*/ chans;
	int k = 0, j, lineno;

	if(linecnt > SF_MAXFILES-1) {
#ifndef IS_CDPARSE
		fprintf(stdout,"WARNING: If %s is a mixfile: cannot mix more than %d files in one mix\n",
		filename,SF_MAXFILES-1);
		fflush(stdout);
#endif
		*mixlist  = FALSE;
		return(FINISHED);
	}
	for(lineno=0;lineno<linecnt;lineno++) {
		if((exit_status = valid_start_of_mixfile_line(k,&chans,lineno,wordstor,srate,mixlist,inputfile,filename,dur))<0)
			return(exit_status);
		if(*mixlist==FALSE) 
			return(FINISHED);
		if(wordcnt[lineno]>MIX_MINLINE) {
			j = k + MIX_MINLINE;
#ifdef IS_CDPARSE
			if(!is_valid_end_of_mixfile_line(j,chans,lineno,wordstor,wordcnt)) {
#else
			if(!is_valid_end_of_mixfile_line(j,chans,lineno,wordstor,wordcnt,filename)) {
#endif
				*mixlist = FALSE;
				return(FINISHED);
			}
			if(wordcnt[lineno]==MIX_MIDLINE && chans==MONO) {
				if((exit_status = confirm_true_stereo(&chans,&is_left,&is_right,&is_central,wordstor[j]))<0)
					return(exit_status);
			}
		} else
			is_central = TRUE;
		inputfile->out_chans = max(inputfile->out_chans,chans);
		k += wordcnt[lineno];
	}

#ifdef NO_DUPLICATE_SNDFILES
#ifdef IS_CDPARSE
	if(duplicated_filenames(linecnt,wordstor,wordcnt)) 
#else
	if(duplicated_filenames(linecnt,wordstor,wordcnt,filename)) 
#endif
	{
		*mixlist = FALSE;
		return(FINISHED);
	}
#endif

	if(inputfile->out_chans==MONO) {
		if((is_left  && is_right)
		|| (is_left  && is_central)
		|| (is_right && is_central))
			inputfile->out_chans = STEREO;
	}
	if(inputfile->out_chans < MONO) {
		sprintf(errstr,"Impossible out_chans value\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/******************* DUPLICATED_FILENAMES **************************/
#ifdef IS_CDPARSE

int duplicated_filenames(int linecnt,char **wordstor,int *wordcnt)
{
	int n, m;
	int j, k = 0;
	for(n=0;n<linecnt-1;n++) {
		j = k;		
		for(m=n+1;m<linecnt;m++) {
			j += wordcnt[m-1];			
			if(!strcmp(wordstor[k],wordstor[j]))
				return(TRUE);
		}
		k += wordcnt[n];
	}
	return(FALSE);
}
#else
int duplicated_filenames(int linecnt,char **wordstor,int *wordcnt,char *filename)
{
	int n, m;
	int j, k = 0;
	for(n=0;n<linecnt-1;n++) {
		j = k;		
		for(m=n+1;m<linecnt;m++) {
			j += wordcnt[m-1];			
			if(!strcmp(wordstor[k],wordstor[j])) {
#ifndef IS_CDPARSE
				fprintf(stdout,"WARNING: If %s is a mix or syncfile: filename duplicates found (not allowed).\n",filename);
				fflush(stdout);
#endif
				return(TRUE);
			}
		}
		k += wordcnt[n];
	}
	return(FALSE);
}
#endif

/*************** OK_LEVEL ******************/

int OK_level(char *str)
{
	int exit_status = TRUE;
	double level;
	if(is_a_dB_value(str))
		exit_status = get_leveldb_frame(str,&level);
	else {
		if(sscanf(str,"%lf",&level)!=1)
			exit_status = FALSE;
	}
	if(exit_status==FALSE || level < 0.0)
		return(FALSE);
	return(TRUE);
}

/************************************* IS_A_DB_VALUE ******************************/

int is_a_dB_value(char *str)
{
	int decimal_point_cnt = 0;
	int has_digits = FALSE;
	double val;
	char *p, *end_of_number;
	p = str + strlen(str) - 2;
	if(strcmp(p,"dB") && strcmp(p,"DB") && strcmp(p,"db"))
		return(FALSE);
	if(p==str)
		return(FALSE);
	end_of_number = p;
	p = str;
	switch(*p) {
	case('-'):						break;
	case('.'): decimal_point_cnt=1;	break;
	default:
		if(!isdigit(*p))
			return(FALSE);
		has_digits = TRUE;
		break;
	}
	p++;		
	while(p!=end_of_number) {
		if(isdigit(*p))
			has_digits = TRUE;
		else if(*p == '.') {
			if(++decimal_point_cnt>1)
				return(FALSE);
		} else
			return(FALSE);
		p++;
	}
	if(!has_digits || sscanf(str,"%lf",&val)!=1)
		return(FALSE);
	return(TRUE);
}

/**************************** GET_LEVELDB_FRAME ********************************/

int get_leveldb_frame(char *str,double *val)
{
	int is_neg = 0;
	if(sscanf(str,"%lf",val)!=1)
		return(FALSE);
	*val = max(*val,MIN_DB_ON_16_BIT);
	if(flteq(*val,0.0)) {
		*val = 1.0;
		return(TRUE);
	}
	if(*val<0.0) {
		*val = -(*val);
		is_neg = 1;
	}
	*val /= 20.0;
	*val = pow(10.0,*val);
	if(is_neg)
		*val = 1.0/(*val);
	return(TRUE);
}

/***************************** IS_A_VALID_SNDFILE_FOR_MIXFILE **************************/

int is_a_valid_sndfile_for_mixfile
(char *filename,char *sndfilename,int *srate,int chans,int linecnt,infileptr inputfile,double time,double *dur)
{
	int exit_status;
	/*int minsize = 0L;*/
	double thisdur;
	int ifd;
 	double maxamp, maxloc;
 	int maxrep;
#ifdef IS_CDPARSE
	int getmaxinfo = 1;
#else
	int getmaxinfo = 0;
#endif
	char *p = strrchr(sndfilename, '.');
	if(p == NULL) {
#ifndef IS_CDPARSE
		fprintf(stdout,"WARNING: If %s is a mixfile: uses %s which has no sndfile extension\n",filename,sndfilename);
		fflush(stdout);
		return(FALSE);
#endif
	}
	p++;
#ifdef NOTDEF
	/*RWD 4:2002 this is silly! sfsys accepts aiff files, so cdparse certainly can! */
	if(_stricmp(p,(getenv("CDP_SOUND_EXT")))) {
#ifndef IS_CDPARSE
		fprintf(stdout,"WARNING: If %s is a mixfile: uses %s which has an invalid sndfile extension :  %s is not %s\n",filename,sndfilename,p,getenv("CDP_SOUND_EXT"));
		fflush(stdout);
		return(FALSE);
#endif
	}
#endif
	if((ifd = sndopenEx(sndfilename,0,CDP_OPEN_RDONLY)) < 0) {
#ifndef IS_CDPARSE
		fprintf(stdout,"WARNING: If %s is a mixfile: uses %s which is not a sndfile\n",filename,sndfilename);
		fflush(stdout);
#endif
		return(FALSE);
	}
	if((exit_status = readhead(inputfile,ifd,sndfilename,&maxamp,&maxloc,&maxrep,0,getmaxinfo))<0) {     /* READ THE FILE HEADER */
		if(sndcloseEx(ifd)<0)
			sprintf(errstr,"Failed to close file %s when checking file %s\n",sndfilename,filename);
		return(exit_status);
	}
	exit_status = TRUE;
	if(inputfile->filetype!=SNDFILE) {
#ifndef IS_CDPARSE
		fprintf(stdout,"WARNING: If %s is a mixfile: uses %s which is not a sndfile\n",filename,sndfilename);
		fflush(stdout);
#endif
		exit_status=FALSE;
	} else if(inputfile->channels != chans) {
		if(chans==MONO)
#ifndef IS_CDPARSE
			fprintf(stdout,"WARNING: If %s is a mixfile: %s is not a mono sndfile\n",filename,sndfilename);
		else if(chans==STEREO)		/*RWD 4:2001 added test */
			fprintf(stdout,"WARNING: If %s is a mixfile: %s is not a stereo sndfile\n",filename,sndfilename);
		fflush(stdout);
#endif
		exit_status=FALSE;
	} else if((inputfile->insams = sndsizeEx(ifd))<0) {	    			/* FIND SIZE OF FILE */
		sprintf(errstr, "Can't read size of input file %s: used in file %s\n",sndfilename,filename);
		exit_status=PROGRAM_ERROR;
	} else if(inputfile->insams <= 0L) {
#ifndef IS_CDPARSE
		fprintf(stdout,"WARNING: If %s is a mixfile: file %s contains no data.\n",filename,sndfilename);
		fflush(stdout);
#endif
		exit_status=FALSE;
	} else if (linecnt==0) {
		*srate = inputfile->srate;
	} else if(inputfile->srate != *srate) {
#ifndef IS_CDPARSE
		fprintf(stdout,"WARNING: If %s is a mixfile: Incompatible sample-rate in file %s\n",filename,sndfilename);
		fflush(stdout);
#endif
		exit_status=FALSE;
	}
#ifdef NOTDEF			/* RWD 4:2001 */
	switch(inputfile->filetype) {
	case(SNDFILE):		inputfile->insams = inputfile->infilesize/sizeof(short);	break;
	case(FLT_SNDFILE):	inputfile->insams = inputfile->infilesize/sizeof(float);	break;
	}
#endif
#ifdef _DEBUG
	assert(inputfile->insams > 0);
#endif
	thisdur = time + (inputfile->insams/inputfile->channels/(double)inputfile->srate);
	*dur = max(*dur,thisdur);

//	inputfile->infilesize	= 0;
	inputfile->insams 		= 0;
	inputfile->filetype		= 0;
	inputfile->srate 		= 0;
	inputfile->channels		= 0;
	inputfile->stype 		= 0;
	if(sndcloseEx(ifd)<0) {
		sprintf(errstr,"Failed to close file %s while checking file %s\n",sndfilename,filename);
		exit_status=SYSTEM_ERROR;
	}
	return(exit_status);
}

/*************************** CHECK_FOR_BRKLIST_AND_STORE_NUMBERS ***********************/

int check_for_brklist_and_count_numbers
(char **wordstor,int *wordcnt,int linecnt,int *brklist,infileptr inputfile,char *filename)
{
	double p, lasttime = 0.0;
	int istime = TRUE;
	int lineno, word_in_line, dcount = 0, k = 0;
	for(lineno=0;lineno<linecnt;lineno++) {
		for(word_in_line = 0;word_in_line < wordcnt[lineno];word_in_line++) {
			if(sscanf(wordstor[k],"%lf",&p)!=1) {
				sprintf(errstr,"scan anomaly: check_for_brklist_and_count_numbers()\n");
				return(PROGRAM_ERROR);
			}
			if(istime) {
				if(*brklist==POSSIBLE) {
					if(k==0) {
						if(p < 0.0)
							*brklist = FALSE;
					} else if(p <= lasttime)
						*brklist = FALSE;
					lasttime = p;
				}
				if(k==0) {
					inputfile->maxnum = p;
					inputfile->minnum = p;
				} else {
					inputfile->maxnum = max(p,inputfile->maxnum);
					inputfile->minnum = min(p,inputfile->minnum);
				}
			} else {
				if(k==1) {
					inputfile->maxbrk = p;
					inputfile->minbrk = p;
				} else {
					inputfile->maxbrk = max(p,inputfile->maxbrk);
					inputfile->minbrk = min(p,inputfile->minbrk);
				}								
				inputfile->maxnum = max(p,inputfile->maxnum);
				inputfile->minnum = min(p,inputfile->minnum);
			}
			istime = !istime;
			k++;
		}
	}	    
	if(k == 0) {
		sprintf(errstr,"No numeric data found in file %s\n",filename);
		return(DATA_ERROR);
	}
	if(*brklist==POSSIBLE) {
		if(k < 4)		   		/*	Must be at least 2 point in a brkfile */
			*brklist = FALSE;
	}
	if(*brklist==POSSIBLE) {
		if(((dcount = k/2) * 2) != k)
			*brklist = FALSE;
	}
	if(*brklist==POSSIBLE) {
		inputfile->brksize  = dcount;
		inputfile->duration = lasttime;
	}
	else
		inputfile->brksize = 0;
	inputfile->numsize = k;
	return(FINISHED);
}


/*************************** CHECK_FOR_A_LINELIST ***********************/

#define OTHER   	(0)
#define	NUMERICAL	(1)
#define DBLIKE		(2)

int check_for_a_linelist(int line_word_cnt,int *linelist,char **wordstor,infileptr inputfile)
{
	int linecnt = 0, n, m;
	int  total_wordcnt = 0;
//TW AGREED REMOVAL of 'OK'
	int  k;
	double val;
	char *wordtype[2], *p;
	if((wordtype[0] = (char *)malloc(line_word_cnt * sizeof(char)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for wordtype[0]\n");
		return(MEMORY_ERROR);
	}
	if((wordtype[1] = (char *)malloc(line_word_cnt * sizeof(char)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for wordtype[1]\n");
		free(wordtype[0]);
		return(MEMORY_ERROR);
	}
	for(n=0;n<inputfile->linecnt;n++) {
		if(n==0)	k = 0;
		else		k = 1;
		for(m=0;m<line_word_cnt;m++) {
			p = wordstor[total_wordcnt];
			if(is_a_dB_value(wordstor[total_wordcnt]))
				wordtype[k][m] = (char)DBLIKE;
			else if(get_float_from_within_string(&p,&val))
				wordtype[k][m] = (char)NUMERICAL;
			else
				wordtype[k][m] = (char)OTHER;
		  	total_wordcnt++;
		}
		if(linecnt>0) {
			for(m=0;m<line_word_cnt;m++) {
				if(wordtype[0][m] != wordtype[1][m]) {
					*linelist = FALSE;
					break;
				}
			}
		}
		if(!(*linelist))
			break;
		linecnt++;
	}			
	free(wordtype[0]);
	free(wordtype[1]);
	return(FINISHED);
}

/***************************** CHECK_FOR_SNDLIST **************************/

int check_for_sndlist(char **wordstor,int *wordcnt,int linecnt,int *sndlist)
{
	char *filename;
//TW AGREED VARIABLE REMOVED total_wordcnt
	int ifd;
	int k = 0, lineno, word_in_line;
	//RWD.9.98
	SFPROPS props;


	for(lineno=0;lineno<linecnt;lineno++) {
		for(word_in_line=0;word_in_line<wordcnt[lineno];word_in_line++) {
			filename = wordstor[k];
			if((ifd = sndopenEx(filename,0,CDP_OPEN_RDONLY)) < 0) {
				*sndlist = FALSE;		
				return(FINISHED);
			} 
			//RWD added 9.98
/* TEST THIS !!! */
			if(!snd_headread(ifd,&props)){
				sprintf(errstr,"Cannot read soundfile header: %s\n",filename);
				sndcloseEx(ifd);
				return(SYSTEM_ERROR);
			}
			if(!(props.type==wt_wave)){
				*sndlist = FALSE;
				sprintf(errstr,"soundfile %s is not a wave file\n",filename);
				sndcloseEx(ifd);
				return FINISHED;
			}
			if(sndcloseEx(ifd)<0) {
				sprintf(errstr,"Cannot close sndfile %s: check_for_sndlist()\n",filename);
				return(SYSTEM_ERROR);
			}

			k++;
		}
	}
	return(FINISHED);
}

/***************************** STORE_WORDLIST_WITHOUT_COMMENTS **************************/

int store_wordlist_without_comments(FILE *fp,char *temp,int total_wordcnt,char **wordstor,int *wordcnt,infileptr inputfile)
{
	char *p	, *q;
	int this_wordcnt = 0;
	int wordcnt_in_line;
	int linecnt = 0;
	while(fgets(temp,200,fp)!=NULL) {
		p = temp;
		if(is_an_empty_line_or_a_comment(p))
			continue;
		wordcnt_in_line = 0;
		while(get_word_from_string(&p,&q)) {
			if((wordstor[this_wordcnt] = (char *)malloc((strlen(q) + 1) * sizeof(char)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY for word %ld\n",this_wordcnt+1);
				return(MEMORY_ERROR);
			}
			strcpy(wordstor[this_wordcnt],q);
			if(++this_wordcnt > total_wordcnt) {
				sprintf(errstr,"Error in word accounting: store_wordlist_without_comments()\n");
				return(PROGRAM_ERROR);
			}
			wordcnt_in_line++;
		}
		wordcnt[linecnt] = wordcnt_in_line;
		linecnt++;
	}
	inputfile->linecnt = linecnt;
	return(FINISHED);
}

/***************************** ASSIGN_TYPE **************************/

//TW REMOVED OLD COMMENT LINES
int assign_type(int sndlist,int numlist,int brklist,int mixlist,int synclist,int linelist,int extended_brkfile,infileptr inputfile)
{
//TW AGREED WARNING SHOULD BE MOVED
	if(sndlist==POSSIBLE) {
		if(synclist==POSSIBLE) {
			if(linelist==POSSIBLE)
				return SNDLIST_OR_SYNCLIST_LINELIST_OR_WORDLIST;
			else
				return SNDLIST_OR_SYNCLIST_OR_WORDLIST;
		} else {
			if(linelist==POSSIBLE)
				return SNDLIST_OR_LINELIST_OR_WORDLIST;
			else
				return SNDLIST_OR_WORDLIST;
		}
	} else if(numlist==POSSIBLE) {
		if(brklist==POSSIBLE)
			return assign_brkfile_type(inputfile,linelist);
		else {
			if(linelist==POSSIBLE)
				return NUMLIST_OR_LINELIST_OR_WORDLIST;
			else
				return NUMLIST_OR_WORDLIST;
		}
	} else if(mixlist==POSSIBLE) {
		if(linelist==POSSIBLE)
			return MIXLIST_OR_LINELIST_OR_WORDLIST;
		else
			return MIXLIST_OR_WORDLIST;
	} else if(synclist==POSSIBLE) {
		if(linelist==POSSIBLE)
			return SYNCLIST_OR_LINELIST_OR_WORDLIST;
		else
			return SYNCLIST_OR_WORDLIST;
	} else if(linelist==POSSIBLE)
		return LINELIST_OR_WORDLIST;
	//RWD.8.98 do this test here!
	if(extended_brkfile==POSSIBLE) {		
		fprintf(stderr,"WARNING:Extended format brkfiles not yet supported by commandline programs.\n");	
	}
	return WORDLIST;
}

/****************************************** CHECK_FOR_A_SYNCLIST ****************************************/

int check_for_a_synclist(int *synclist,char **wordstor,int *wordcnt,infileptr inputfile,char *filename)
{
	int  exit_status;
	int n;
	int total_words = 0;
	int srate = 0;
	double starttime, endtime;
	for(n=0;n<inputfile->linecnt;n++) {	 /* Must be 1 or 3 words on each line */
		if(wordcnt[n]!=1 && wordcnt[n]!=3) {
			*synclist = FALSE;
			return(FINISHED);
		}
	}
	for(n=0;n<inputfile->linecnt;n++) {	/* Every line must start with a sndfilename */
		if((exit_status = is_a_valid_sndfile_for_sync(filename,wordstor[total_words],&srate,n,inputfile))<0)
			return(exit_status);
		if(exit_status == FALSE) {
			*synclist = FALSE;
			return(FINISHED);
		}
		total_words += wordcnt[n];
	}
	total_words = 0;
	for(n=0;n<inputfile->linecnt;n++) {	/* Any other values must be numbers */
		if(wordcnt[n] > 1) {
			if(sscanf(wordstor[total_words+1],"%lf\n",&starttime)!=1) {
				*synclist = FALSE;
				return(FINISHED);
			}
			else if(sscanf(wordstor[total_words+2],"%lf\n",&endtime)!=1) {
				*synclist = FALSE;
				return(FINISHED);
			} else if(starttime <  0.0 || endtime < 0.0) {
#ifndef IS_CDPARSE
				fprintf(stdout,"WARNING: If %s is a synclist, it contains negative times\n",filename);
				fflush(stdout);
#endif
				*synclist = FALSE;
				return(FINISHED);
			} else if(starttime >  endtime) {
#ifndef IS_CDPARSE
				fprintf(stdout,"WARNING: If %s is a synclist, starttime > endtime in line %d\n",filename,n+1);
				fflush(stdout);
#endif
				*synclist = FALSE;
				return(FINISHED);
			}				
		}
		total_words += wordcnt[n];
	}

#ifdef NO_DUPLICATE_SNDFILES
#ifdef CDPARSE
	if(duplicated_filenames(inputfile->linecnt,wordstor,wordcnt))
#else
	if(duplicated_filenames(inputfile->linecnt,wordstor,wordcnt,filename))
#endif
		return(FALSE);
#endif
	inputfile->srate = srate;
	return(FINISHED);
}

/***************************** IS_A_VALID_SNDFILE_FOR_SYNC **************************/

int is_a_valid_sndfile_for_sync(char *filename,char *sndfilename,int *srate,int linecnt,infileptr inputfile)
{
	int exit_status;
//TW AGREED REMOVAL minsize
	int ifd;
 	double maxamp, maxloc;
 	int maxrep;
#ifdef IS_CDPARSE
	int getmaxinfo = 1;
#else
	int getmaxinfo = 0;
#endif
	if((ifd = sndopenEx(sndfilename,0,CDP_OPEN_RDONLY)) < 0)
		return(FALSE);
	if((exit_status = readhead(inputfile,ifd,sndfilename,&maxamp,&maxloc,&maxrep,0,getmaxinfo))<0) {	/* READ THE FILE HEADER */
		if(sndcloseEx(ifd)<0) {
			sprintf(errstr,"Failed to close sndfile %s while checking file %s\n",sndfilename,filename);
			exit_status = SYSTEM_ERROR;
		}
		return(exit_status);
	}
	exit_status = TRUE;

	if(inputfile->filetype!=SNDFILE) {
#ifndef IS_CDPARSE
		fprintf(stdout,"WARNING: If %s is a syncfile: %s is not a sndfile.\n",filename,sndfilename);
		fflush(stdout);
#endif
		exit_status=FALSE;
	} else if((inputfile->insams = sndsizeEx(ifd))<0) {	    			/* FIND SIZE OF FILE */
		sprintf(errstr, "Can't read size of input file %s: while checking file %s\n",sndfilename,filename);
		exit_status = SYSTEM_ERROR;			/*RWD 4.2001 was PROGRAM_ERROR */
	} else if(inputfile->insams == 0L)		/*RWD 4.2001 was <= */
		exit_status=FALSE;
	else if (linecnt==0)
		*srate = inputfile->srate;
	else if(inputfile->srate != *srate) {
#ifndef IS_CDPARSE
		fprintf(stdout,"WARNING: If %s is a syncfile: Incompatible sample-rate in file %s\n",filename,sndfilename);
		fflush(stdout);
#endif
		exit_status=FALSE;
	}
//	inputfile->infilesize	= 0;
	inputfile->insams 		= 0;
	inputfile->filetype		= 0;
	inputfile->srate 		= 0;
	inputfile->channels 	= 0;
	inputfile->stype 		= 0;
	if(sndcloseEx(ifd)<0) {
		sprintf(errstr,"Failed to close sndfile %s while testing file %s\n",sndfilename,filename);
		return(SYSTEM_ERROR);
	}
	return(exit_status);
}

/***************************** SET_SNDLIST_FLAGS **************************/

int set_sndlist_flags(infileptr inputfile,char **wordstor)
{
	int  exit_status;
	int srate = 0, wordcnt, m;
	char *filename;
	int  ifd;
 	double maxamp, maxloc;
 	int maxrep;
#ifdef IS_CDPARSE
	int getmaxinfo = 1;
#else
	int getmaxinfo = 0;
#endif
	for(wordcnt=0;wordcnt<inputfile->all_words;wordcnt++) {
		filename = wordstor[wordcnt]; 
		if(inputfile->different_srates == FALSE) {
			if((ifd = sndopenEx(filename,0,CDP_OPEN_RDONLY)) < 0) {
#ifdef _DEBUG
				sprintf(errstr,"Anomaly reopening sndfile %s: set_sndfilelist_flags()\n",filename);
#else
				sprintf(errstr,"Anomaly reopening sndfile %s: %s\n",filename,rsferrstr);
#endif
				return(SYSTEM_ERROR);
			}
			if((exit_status = readhead(inputfile,ifd,filename,&maxamp,&maxloc,&maxrep,0,getmaxinfo))<0) {	/* READ THE FILE HEADER */
				sprintf(errstr,"Anomaly Rereading header of sndfile %s: set_sndfilelist_flags()\n",filename);
				if(sndcloseEx(ifd) < 0)
					sprintf(errstr, "Can't close soundfile %s: set_sndlist_flags()\n",filename);
				return(SYSTEM_ERROR);
			}
			if(wordcnt==0)
				srate = inputfile->srate;
			else if(inputfile->srate != srate) {
				inputfile->different_srates = TRUE;
				inputfile->srate 			 = 0;
			}
//			inputfile->infilesize	= 0;
			inputfile->insams 		= 0;
			inputfile->filetype		= 0;
			inputfile->channels 	= 0;
			inputfile->stype 		= 0;
			if(sndcloseEx(ifd) < 0) {
				sprintf(errstr, "Can't close soundfile %s: set_sndlist_flags()\n",filename);
				return(SYSTEM_ERROR);
			}
		}
	}
	for(wordcnt=0;wordcnt<inputfile->all_words;wordcnt++) {
		for(m=wordcnt+1;m<inputfile->all_words;m++) {
			if(!strcmp(wordstor[wordcnt],wordstor[m])) {
				inputfile->duplicate_snds = TRUE;
				break;
			}
		}
		if(inputfile->duplicate_snds)
			break;
	}
	return(FINISHED);
}

/****************************************** ASSIGN_BRKFILE_TYPE ****************************************/

int	assign_brkfile_type(infileptr inputfile,int linelist)
{
	int normalised_brkfile = FALSE;
	int dB_brkfile         = FALSE;
	int pitch_brkfile 	   = FALSE;
	int transpos_brkfile   = FALSE;
	int positive_brkfile   = FALSE;

	if(inputfile->minbrk >= 0.0)
		positive_brkfile = TRUE;
	if(inputfile->maxbrk <= 1.0 && inputfile->minbrk >= 0.0)
		normalised_brkfile = TRUE;
	else if (inputfile->maxbrk <= DEFAULT_NYQUIST && inputfile->minbrk >= -1.0) {
		inputfile->could_be_pitch = TRUE;
		pitch_brkfile = TRUE;
	}
	if(inputfile->maxbrk <= 0.0 && inputfile->minbrk >= MIN_DB_ON_16_BIT)
		dB_brkfile = TRUE;
	if(inputfile->maxbrk <= MAX_TRANSPOS && inputfile->minbrk >= MIN_TRANSPOS) {
		inputfile->could_be_transpos = TRUE;
		transpos_brkfile = TRUE;
	}

	if(transpos_brkfile) {
		if(normalised_brkfile) {
			if(linelist==POSSIBLE)
				return TRANSPOS_OR_NORMD_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST;
			else
				return TRANSPOS_OR_NORMD_BRKFILE_OR_NUMLIST_OR_WORDLIST;
		} else if(pitch_brkfile) {
			if(linelist==POSSIBLE)
				return TRANSPOS_OR_PITCH_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST;
			else
				return TRANSPOS_OR_PITCH_BRKFILE_OR_NUMLIST_OR_WORDLIST;
		} else {
			if(linelist==POSSIBLE)
				return TRANSPOS_OR_UNRANGED_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST;
			else
				return TRANSPOS_OR_UNRANGED_BRKFILE_OR_NUMLIST_OR_WORDLIST;
		}	
	} else {
		if(normalised_brkfile) {
			if(linelist==POSSIBLE)
				return NORMD_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST;
			else
				return NORMD_BRKFILE_OR_NUMLIST_OR_WORDLIST;
		} else if(dB_brkfile) {
			if(linelist==POSSIBLE)
				return DB_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST;
			else
				return DB_BRKFILE_OR_NUMLIST_OR_WORDLIST;
		} else if(pitch_brkfile) {
			if(linelist==POSSIBLE) {
				if(positive_brkfile)
					return PITCH_POSITIVE_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST;
				else
					return PITCH_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST;
			} else {
				if(positive_brkfile)
					return PITCH_POSITIVE_BRKFILE_OR_NUMLIST_OR_WORDLIST;
				else
					return PITCH_BRKFILE_OR_NUMLIST_OR_WORDLIST;
			}
		} else if(positive_brkfile) {
			if(linelist==POSSIBLE)
				return POSITIVE_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST;
			else
				return POSITIVE_BRKFILE_OR_NUMLIST_OR_WORDLIST;
		} else {
			if(linelist==POSSIBLE)
				return UNRANGED_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST;
			else
				return UNRANGED_BRKFILE_OR_NUMLIST_OR_WORDLIST;
		}	
	}
}

#define MIN_BRKSLOPE	(1.0)
#define MAX_BRKSLOPE	(6.0)

/************************************ CHECK_FOR_AN_EXTENDED_BRKFILE *********************************/

int check_for_an_extended_brkfile
(int *extended_brkfile,char **wordstor,int *wordcnt,int linecnt,int total_words,infileptr inputfile)
{
	int wordno = 0, lineno;
	int is_extended_format = FALSE;
	double lasttime = -1.0, time, val;
	double maxval = -(double)(MAXRANGE+1);
	double minval = (double)(MAXRANGE+1);
	if(wordcnt[0]!=2) {
		*extended_brkfile = FALSE;
		return(FINISHED);
	}
	if(!extract_value_pair(&wordno,&time,&val,&maxval,&minval,&lasttime,wordstor)) {
		*extended_brkfile = FALSE;
		return(FINISHED);
	}
	if(time!=0.0) {
		*extended_brkfile = FALSE;
		return(FINISHED);
	}
	for(lineno=1;lineno<linecnt;lineno++) {
		if(wordno+2 > total_words) {
			*extended_brkfile = FALSE;
			return(FINISHED);
		} 
		if(wordcnt[lineno]<2 || wordcnt[lineno]>4) {
			*extended_brkfile = FALSE;
			return(FINISHED);
		}
		if(!extract_value_pair(&wordno,&time,&val,&maxval,&minval,&lasttime,wordstor)) {
			*extended_brkfile = FALSE;
			return(FINISHED);
		}
		if(wordcnt[lineno]>2) {
			if(wordno >=total_words) {
				sprintf(errstr,"Failure in word-counting logic 2: check_for_an_extended_brkfile()\n");
				return(PROGRAM_ERROR);
			} 
			utol(wordstor[wordno]);
			if(strcmp(wordstor[wordno],"lin") 
			&& strcmp(wordstor[wordno],"exp") 
			&& strcmp(wordstor[wordno],"log")) {
				*extended_brkfile = FALSE;
				return(FINISHED);
			}
			wordno++;
			is_extended_format = TRUE;
		}
		if(wordcnt[lineno]>3) {
			if(wordno >=total_words) {
				sprintf(errstr,"Failure in word-counting logic 3: check_for_an_extended_brkfile()\n");
				return(PROGRAM_ERROR);
			} 
			if(sscanf(wordstor[wordno],"%lf",&val)!=1
			|| val < MIN_BRKSLOPE || val > MAX_BRKSLOPE) {
				*extended_brkfile = FALSE;
				return(FINISHED);
			}
			wordno++;
		}
	}
	if(is_extended_format) {
		inputfile->maxbrk = maxval;
		inputfile->minbrk = minval;
	} else
		*extended_brkfile = FALSE;
	return(FINISHED);
}

/****************************************** EXTRACT_VALUE_PAIR ****************************************/

int extract_value_pair(int *n,double *time, double *val,double *maxval,double *minval, double *lasttime, char **wordstor)
{
	if(sscanf(wordstor[*n],"%lf",time)!=1 || sscanf(wordstor[(*n)+1],"%lf",val)!=1)
		return(FALSE);
//TW
//	if(*time <= *lasttime || *val > (double)MAXSAMP || *val < -(double)MAXSAMP)
	if(*time <= *lasttime || *val > (double)MAXRANGE || *val < -(double)MAXRANGE)
		return(FALSE);
	*lasttime = *time;
	*n += 2;
	*maxval = max(*maxval,*val);
	*minval = min(*minval,*val);
	return(TRUE);
}

/****************************************** UTOL ****************************************/

#define UPPER_TO_LOWER_CASE (32)

void utol(char *str)
{
	int n, k = strlen(str);
	for(n=0;n<k;n++) {
		if(str[n] >= 'A' && str[n] <= 'Z')
			str[n] += UPPER_TO_LOWER_CASE;
	}
}
			
/* NEW CODE : June 1st : 2000 */

/****************************************** MAXSAMP ****************************************/
//TW Rewritten: floating point data only is retained:
//This version cannot write maxsamp info to header
void maxsamp(int ifd,double *maxamp,double *maxloc,int *maxrep)
{
	int got, totalsamps = 0, samps, i, maxxloc;
	double maxdamp = 0.0;
	float *bigbuf, *fbuf;
	int repeats = 0;			/* counts how many times the max repeats 	*/
	int bufsize;

	*maxrep = -1;	/* flags that maxamp and maxloc are not found */
	*maxamp = 0.0;
	*maxloc = 0.0;

	if((bufsize = (int)(long) Malloc(-1)) < sizeof(float))
		return;
	/*bufsize = (bufsize/SECSIZE)*SECSIZE;	*/	/* round to sector boundary */
	bufsize /= sizeof(float);
	if((bigbuf = (float *)malloc(bufsize * sizeof(float)))==NULL)
		return;
	fbuf = (float *)bigbuf;

	while((got = fgetfbufEx(bigbuf,bufsize,ifd,0)) > 0 ) {
		samps      = got;
		for( i=0 ; i<samps ; i++ ) {
			if(smpflteq(fbuf[i],maxdamp) || smpflteq(-fbuf[i],maxdamp))
				repeats++;
			else if(fbuf[i] >= 0.0) {
				if(fbuf[i] > maxdamp) {
					maxdamp = fbuf[i];
					*maxloc = (double)(totalsamps + i);
					repeats = 1;
				}
			} else {
				if (-fbuf[i] > maxdamp) {
					maxdamp = -fbuf[i];
					*maxloc = (double)(totalsamps + i);
					repeats = 1;
				}
			}
		}
		totalsamps += got;
	} 
	if( got < 0 ) {
		free(bigbuf);
		return;
	}
	if(totalsamps > 0) {
		if(repeats <= 0) {
			Mfree(bigbuf);
			return;
		}
		*maxamp = maxdamp;			/* return info to SoundLoom */
		maxxloc = (int)round(*maxloc);
		*maxrep = repeats;
	}
	Mfree(bigbuf);
}

/************************************** DO_EXIT ************************************/

int do_exit(int exit_status) {
#ifdef IS_CDPARSE
 	if(exit_status == DATA_ERROR)
		fprintf(stdout,"WARNING: %s",errstr);	/* Sound Loom responds to message headers */ 
	else 										/* and differently to different message headers */
		fprintf(stdout,"ERROR: %s",errstr); 	/* but ignores the return value */
	fflush(stdout);  							
#endif
	return(exit_status);						/* cmdline responds to value of 'exit_status' */
}


/***************************** PARSE_A_TEXTFILE *****************************/

int parse_a_textfile(char *filename,infileptr inputfile)  
{
	int exit_status;
	FILE *fp;

	if((fp = fopen(filename,"r"))==NULL) {								
		sprintf(errstr,	"Can't open file %s to read data.\n",filename);
		exit_status = DATA_ERROR;
		return do_exit(exit_status);
	}
	if(file_has_reserved_extension(filename)) {
		sprintf(errstr,	"File %s either has unredable header, or is a textfile with a CDP reserved extension.\n",filename);
//TW ADDED: ESSENTIAL TO CLOSE FILE or NOTIFY SOUND LOOM OF FAILURE TO CLOSE: for Sound Loom INTEGRITY
		if(fclose(fp)<0) {
			sprintf(errstr,	"Failed to close file %s\n",filename);
			exit_status = SYSTEM_ERROR;
			return do_exit(exit_status);
		}
		exit_status = DATA_ERROR;
		return do_exit(exit_status);
	}
	if((exit_status = megaparsec(filename,fp,inputfile))<0)	{				
		if(fclose(fp)<0) {
			sprintf(errstr,	"Failed to close file %s\n",filename);
			exit_status = SYSTEM_ERROR;
		}
		return do_exit(exit_status);
	}
	if(fclose(fp)<0) {
		sprintf(errstr,	"Failed to close file %s\n",filename);
		exit_status = SYSTEM_ERROR;
		return do_exit(exit_status);
	}
	return FINISHED;
}

