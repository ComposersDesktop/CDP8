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

static int  test_mixdata_in_line(int wordcnt,char **wordstor,int total_words,int *chans,int lineno,dataptr dz);
static int  test_right_level(char *str,int lineno);
static int  test_right_pan(char *str,int lineno);
static int  test_left_level(char *str,int lineno);
static int  test_left_pan(char *str,int lineno);
static int  open_file_and_test_props(int filecnt,char *filename,int *srate,int chans,int lineno,dataptr dz);

static int  syntax_check_mix(dataptr dz);
//TW UPDATED FUNCTION
static int  create_and_output_mixfile_line(int infilno,int max_namelen,double *running_total,dataptr dz);

static int  output_mixfile_line(char **wordstor,int *maxwordsize,int start_word_no,int words_in_line,dataptr dz);
static int  twist_mixfile_line(char **wordstor,int *maxwordsize,int start_word_no,int words_in_line,dataptr dz);
static int  output_aligned_word(char *thisword,int maxthiswordsize,dataptr dz);
static int  mirror_panning(char *thisword, int maxthiswordsize,dataptr dz);

/*************************** MIX_SYNTAX_CHECK *************************/

int mix_syntax_check(dataptr dz)
{
	int exit_status;
	if((exit_status = syntax_check_mix(dz))<0)
		return(exit_status);
	if(exit_status==CONTINUE)
		sprintf(errstr,"MIX SYNTAX IS CORRECT.\n");
	return(FINISHED);
}

/********************** SYNTAX_CHECK_MIX ****************************/
 
int syntax_check_mix(dataptr dz)
{
	int    exit_status;
	int    chans;
	int   srate=0;
	char   *filename;
	int    filecnt = 0;												
	int   total_words = 0, n;
	if(dz->infile->filetype == MIXFILE)
		return(CONTINUE);
	if((exit_status = establish_file_data_storage_for_mix((int)1,dz))<0)
		return(exit_status);
	for(n=0;n<dz->linecnt;n++) {						/* for each mixfile line */
		if(dz->wordstor[total_words][0]==';') {
			total_words += dz->wordcnt[n];
			continue;	
		}	
		filename = dz->wordstor[total_words];			/* get the filename */
		if(strlen(filename)<=0) {
			sprintf(errstr,"filename error: line %d\n",dz->linecnt+1);
			return(FINISHED);
		}
		if((exit_status = test_mixdata_in_line(dz->wordcnt[n],dz->wordstor,total_words,&chans,n,dz))!=CONTINUE)
			return(FINISHED);
		total_words += dz->wordcnt[n];
		if((exit_status = open_file_and_test_props(filecnt,filename,&srate,chans,n,dz))<0)
			return(exit_status);
		if(exit_status!=CONTINUE)
			return(FINISHED);
		filecnt++;											/* count the ACTUALLY USED lines: ignore comments */
	}
	if(filecnt==0) {
		sprintf(errstr,"No active mixfile lines.\n");
		return(FINISHED);
	}
	return(CONTINUE);
}

/**************************** TEST_MIXDATA_IN_LINE ********************************/

int test_mixdata_in_line(int wordcnt,char **wordstor,int total_words,int *chans,int lineno,dataptr dz)
{
	int exit_status;
	double time;
	switch(wordcnt) {
	case(MIX_MAXLINE):
		if((exit_status = test_right_level(wordstor[total_words+MIX_RLEVELPOS],lineno))!=CONTINUE)	/* 6d */
			return(FINISHED);
		if((exit_status = test_right_pan(wordstor[total_words+MIX_RPANPOS],lineno))!=CONTINUE)
			return(FINISHED);
		/* fall thro */
	case(MIX_MIDLINE):
		if((exit_status = test_left_pan(wordstor[total_words+MIX_PANPOS],lineno))!=CONTINUE)		/* 6c */
			return(FINISHED);
		/* fall thro */
	case(MIX_MINLINE):
		if(sscanf(dz->wordstor[total_words+MIX_TIMEPOS],"%lf",&time)!=1)  {
			sprintf(errstr,"Cannot read time data: line %d\n",lineno+1);
			return(FINISHED);
		}
		if(time < 0.0) {
			sprintf(errstr,"time value less than 0.0 time: line %d\n",lineno+1);
			return(FINISHED);
		}
		if(sscanf(dz->wordstor[total_words+MIX_CHANPOS],"%d",chans)!=1) {
			sprintf(errstr,"Cannot read channel data: line %d\n",lineno+1);
			return(FINISHED);
		}
		if(*chans < MONO || *chans > STEREO) {
			sprintf(errstr,"channel value out of range: line %d\n",lineno+1);
			return(FINISHED);
		}
		if((exit_status = test_left_level(wordstor[total_words+MIX_LEVELPOS],lineno))!=CONTINUE)
			return(FINISHED);
		break;
	default:
		sprintf(errstr,"Illegal mixfile line-length: line %d\n",lineno+1);
		return(FINISHED);
	}
	if(wordcnt==MIX_MIDLINE && *chans!=MONO) {
		sprintf(errstr,"Invalid channel count, or line length: line %d\n",lineno+1);
		return(FINISHED);
	}
	if(wordcnt==MIX_MAXLINE && *chans!=STEREO) {
		sprintf(errstr,"Invalid channel count, or line too long: line %d\n",lineno+1);
		return(FINISHED);
	}
	return(CONTINUE);
}

/************************** TEST_RIGHT_LEVEL ************************/

int test_right_level(char *str,int lineno)
{
	double rlevel;
	if(is_dB(str)) {
		if(!get_leveldb(str,&rlevel)) {
			sprintf(errstr,"Error in chan2 level: line %d\n",lineno+1);
			return(FINISHED);
		}		
	} else {
		if(sscanf(str,"%lf",&rlevel)!=1) {
			sprintf(errstr,"Error2 in chan2 level: line %d\n",lineno+1);
			return(FINISHED);
		}
	}
	if(rlevel < 0.0) {
		sprintf(errstr,"chan2 level out of range: line %d\n",lineno+1);
		return(FINISHED);
	}
	return(CONTINUE);
}

/*********************** TEST_RIGHT_PAN **********************/

int test_right_pan(char *str,int lineno)
{
	double pan;
	switch(str[0]) {
	case('L'): 
	case('R'): 
	case('C'): return(CONTINUE);
	default:
		if(sscanf(str,"%lf",&pan)!=1) {
			sprintf(errstr,"Error in chan2 pan: line %d\n",lineno+1);
			return(FINISHED);
		}
		if(pan < MINPAN || pan > MAXPAN)  {
			sprintf(errstr,"chan2 pan out of range: line %d\n",lineno+1);
			return(FINISHED);
		}
		break;
	}
	return(CONTINUE);
}

/*************** TEST_LEFT_LEVEL ******************/

int test_left_level(char *str,int lineno)
{
	double level;
	if(is_dB(str)) {
		if(!get_leveldb(str,&level)) {
			sprintf(errstr,"Error in (chan1) level: line %d\n",lineno+1);
			return(FINISHED);
		}		
	} else {
		if(sscanf(str,"%lf",&level)!=1) {
			sprintf(errstr,"Error in (chan1) level: line %d\n",lineno+1);
			return(FINISHED);
		}
	}
	if(level < 0.0) {
		sprintf(errstr,"(chan1) level out of range: line %d\n",lineno+1);
		return(FINISHED);
	}
	return(CONTINUE);
}

/************************** TEST_LEFT_PAN ************************/

int test_left_pan(char *str,int lineno)
{
	double pan;
	switch(str[0]) {
	case('L'): 
	case('R'): 
	case('C'): return(CONTINUE);
	default:
		if(sscanf(str,"%lf",&pan)!=1) {
			sprintf(errstr,"Error in (chan1) pan: line %d\n",lineno+1);
			return(FINISHED);
		}
		if(pan < MINPAN || pan > MAXPAN)  {
			sprintf(errstr,"(chan1) pan out of range: line %d\n",lineno+1);
			return(FINISHED);
		}
	}
	return(CONTINUE);
}

/************************* OPEN_FILE_AND_TEST_PROPS *******************************/

int open_file_and_test_props(int filecnt,char *filename,int *srate,int chans,int lineno,dataptr dz)
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
	if((dz->ifd[0] = sndopenEx(filename,0,CDP_OPEN_RDONLY)) < 0) {
		sprintf(errstr,"Failed to open sndfile %s: line %d\n",filename,lineno+1);
		return(FINISHED);
	}
	if((exit_status = readhead(ifp,dz->ifd[0],filename,&maxamp,&maxloc,&maxrep,getmax,getmaxinfo))<0)
		return(exit_status);
	copy_to_fileptr(ifp,dz->infile);
	if(dz->infile->filetype!=SNDFILE) {
		sprintf(errstr,"%s is not a soundfile: line %d\n",filename,lineno+1);
		return(FINISHED);
	}
	if(dz->infile->channels!=chans) {
		sprintf(errstr,"channel-cnt in file '%s' different to channel value given: line %d\n",filename,lineno+1);
		return(FINISHED);
	}
	if(filecnt==0)
		*srate = dz->infile->srate;
	else if(dz->infile->srate != *srate) {
		sprintf(errstr,"incompatible srate: file %s: line %d\n",filename,lineno+1);
		return(FINISHED);
	}
	if((dz->insams[0] = sndsizeEx(dz->ifd[0]))<0) {	    			
		sprintf(errstr, "Can't read size of input file %s: line %d\n",filename,lineno+1);
		return(PROGRAM_ERROR);
	}
	if(dz->insams[0] <=0) {
		sprintf(errstr, "Zero size for input file %s: line %d\n",filename,lineno+1);
		return(FINISHED);
	}			
	if(sndcloseEx(dz->ifd[0])<0) {
		sprintf(errstr, "Failed to close input file %s: line %d\n",filename,lineno+1);
		return(SYSTEM_ERROR);
	}
	dz->ifd[0] = -1;
	return(CONTINUE);
}

/********************** CREATE_MIXDUMMY ****************************/
 
int create_mixdummy(dataptr dz)
{
	int    exit_status;
	int   srate;
	int	   this_namelen, max_namelen = 0;												
	int   n;
	double running_total = 0.0;
	sndcloseEx(dz->ifd[0]);
	dz->ifd[0] = -1;
	if(dz->all_words==0) {
		sprintf(errstr,"No words in source file.\n");
		return(PROGRAM_ERROR);
	}
	dz->infilecnt = 1;		/* program now utilises only 1 file at a time : needs data storage for only 1 file */
	if((exit_status = establish_file_data_storage_for_mix((int)1,dz))<0)
		return(exit_status);
	for(n=0;n<dz->all_words;n++) {						/* for each sndfile name */
		if((this_namelen = strlen(dz->wordstor[n]))<=0) {
			sprintf(errstr,"filename error: line %d\n",dz->linecnt+1);
			return(PROGRAM_ERROR);
		}
		max_namelen = max(max_namelen,this_namelen);
	}
	for(n=0;n<dz->all_words;n++) {						/* for each sndfile name */
		if((exit_status = open_file_and_retrieve_props(n,dz->wordstor[n],&srate,dz))<0)
			return(exit_status);
		if(dz->mode == 2 && dz->infile->channels != 1) {
			sprintf(errstr,"This process only works with mono input files.\n");
			return(DATA_ERROR);
		}
		if((exit_status = create_and_output_mixfile_line(n,max_namelen,&running_total,dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}

/************************* CREATE_AND_OUTPUT_MIXFILE_LINE *******************************/

//TW FUNCTION UPDATED (including flotsam conversion)
int create_and_output_mixfile_line(int infilno,int max_namelen,double *running_total,dataptr dz)
{
	int space_count, n;
	double filetime = 0.0; /* default value : MIXDUMMY , mode MD_TOGETHER */

	if((space_count = max_namelen - strlen(dz->wordstor[infilno]))<0) {
		sprintf(errstr,"Anomaly in space counting: create_and_output_mixfile_line()\n");
		return(PROGRAM_ERROR);
	}
	space_count++;
	fprintf(dz->fp,"%s",dz->wordstor[infilno]);
	for(n=0;n<space_count;n++)
		fprintf(dz->fp," ");
	switch(dz->process) {
	case(MIXDUMMY):
		if(dz->mode == MD_FOLLOW) {
			filetime = *running_total;
			*running_total += (double)(dz->insams[0]/dz->infile->channels)/(double)dz->infile->srate;
		}
		break;			
	case(MIX_ON_GRID):		
		filetime = dz->parray[0][infilno];	
		break;
	case(MIX_AT_STEP):		
			filetime = *running_total;
			*running_total += dz->param[MIX_STEP];
		break;
	}
	switch(dz->infile->channels) {
	case(MONO):
		if(dz->process == MIXDUMMY && dz->mode == 2) {
			if(infilno == 0)
				fprintf(dz->fp,"%.4lf  1  1.0  L\n",filetime);
			else
				fprintf(dz->fp,"%.4lf  1  1.0  R\n",filetime);
		} else
			fprintf(dz->fp,"%.4lf  1  1.0  C\n",filetime);
		break;
	case(STEREO):	fprintf(dz->fp,"%.4lf  2  1.0  L  1.0  R\n",filetime);	break;
	default:
		sprintf(errstr,"Invalid number of input channels for one of mix input files (Must be mono or stereo).\n");
		return(DATA_ERROR);
	}
	return(FINISHED);
}

/********************** OUTPUT_MIXFILE_LINE ****************************/
 
int output_mixfile_line(char **wordstor,int *maxwordsize,int start_word_no,int words_in_line,dataptr dz)
{
	int space_count, n, m;
	int wordno;
	for(n = 0;n < words_in_line; n++) {
		wordno = start_word_no + n;
		fprintf(dz->fp,"%s",wordstor[wordno]);
		if((space_count = maxwordsize[n] - strlen(wordstor[wordno]))<0) {
			sprintf(errstr,"Error in space_count: output_mixfile_line()\n");
			return(PROGRAM_ERROR);
		}
		space_count++;
		for(m=0; m < space_count;m++)
			fprintf(dz->fp," ");
	}
	fprintf(dz->fp,"\n");
	return(FINISHED);
}

/********************** MIX_TWISTED ****************************/
 
int mix_twisted(dataptr dz)
{
	int    exit_status;
	double time, llevel, lpan, rlevel, rpan;
	int    chans, m;
	int   n;
	char   *filename;
	int    filecnt = 0;												
	int   total_words = 0, initial_total_words;
	int    *maxwordsize;
	if((maxwordsize = (int *)malloc(MIX_MAXLINE * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store maximum word sizes.\n");
		return(MEMORY_ERROR);
	}
	for(m=0;m<MIX_MAXLINE;m++)
		maxwordsize[m] = 0;

	for(n=0;n<dz->linecnt;n++) {						
		filename = dz->wordstor[total_words];			
		if(strlen(filename)<=0) {
			free(maxwordsize);
			sprintf(errstr,"filename error: line %d: mix_twisted()\n",filecnt+1);
			return(PROGRAM_ERROR);
		}
		if((exit_status = get_mixdata_in_line			
		(dz->wordcnt[n],dz->wordstor,total_words,&time,&chans,&llevel,&lpan,&rlevel,&rpan,filecnt,dz))<0) {
			free(maxwordsize);
			return(exit_status);
		}
		initial_total_words = total_words;
		for(m=0;m<dz->wordcnt[n];m++)
			maxwordsize[m] = (int)max((int)maxwordsize[m],(int)strlen(dz->wordstor[initial_total_words+m]));
		total_words += dz->wordcnt[n];
		if((exit_status = finalise_and_check_mixdata_in_line(dz->wordcnt[n],chans,llevel,&lpan,&rlevel,&rpan))<0) {
			free(maxwordsize);
			return(exit_status);
		}
		filecnt++;											
	}
	maxwordsize[MIX_PANPOS]   = (int)max((int)maxwordsize[MIX_PANPOS],(int)3);	/* allow pan to be converted text to number */
	maxwordsize[MIX_RPANPOS]  = (int)max((int)maxwordsize[MIX_RPANPOS],(int)3);
	maxwordsize[MIX_PANPOS]++;											/* allow for pan to become negative */
	maxwordsize[MIX_RPANPOS]++;

	total_words = 0;
	filecnt = 0;
	for(n=0;n<dz->linecnt;n++) {						
		filename = dz->wordstor[total_words];			
		initial_total_words = total_words;
		total_words += dz->wordcnt[n];
		switch(dz->mode) {
		case(MSW_TWISTALL):
			if(ODD(filecnt)) {
				if((exit_status = twist_mixfile_line(dz->wordstor,maxwordsize,initial_total_words,dz->wordcnt[n],dz))<0) {
					free(maxwordsize);
					return(exit_status);
				}
			} else {
				if((exit_status = output_mixfile_line(dz->wordstor,maxwordsize,initial_total_words,dz->wordcnt[n],dz))<0) {
					free(maxwordsize);
					return(exit_status);
				}
			}
			break;
		case(MSW_TWISTONE):
			if(filecnt == dz->iparam[MSW_TWLINE]) {
 				if((exit_status = twist_mixfile_line(dz->wordstor,maxwordsize,initial_total_words,dz->wordcnt[n],dz))<0) {
					free(maxwordsize);
					return(exit_status);
				}
			} else {
				if((exit_status = output_mixfile_line(dz->wordstor,maxwordsize,initial_total_words,dz->wordcnt[n],dz))<0) {
					free(maxwordsize);
					return(exit_status);
				}
			}
			break;
		default:
			sprintf(errstr,"Unknown mode in mix_twisted()\n");
			return(PROGRAM_ERROR);
		}
		filecnt++;											
	}

	free(maxwordsize);
	return(FINISHED);
}

/********************** TWIST_MIXFILE_LINE ****************************/
 
int twist_mixfile_line(char **wordstor,int *maxwordsize,int start_word_no,int words_in_line,dataptr dz)
{
	int exit_status;
	int n;
	int wordno;
	for(n = 0;n < words_in_line; n++) {
		wordno = n + start_word_no;
		switch(n) {
		case(MIX_NAMEPOS):
		case(MIX_TIMEPOS):
		case(MIX_CHANPOS):
		case(MIX_LEVELPOS):
		case(MIX_RLEVELPOS):
			if((exit_status = output_aligned_word(wordstor[wordno],maxwordsize[n],dz))<0)
				return(exit_status);
			break;
		case(MIX_PANPOS):
		case(MIX_RPANPOS):
			if((exit_status = mirror_panning(wordstor[wordno],maxwordsize[n],dz))<0)
				return(exit_status);
			break;
		default:
			sprintf(errstr,"Impossible word no: twist_mixfile_line()\n");
			return(PROGRAM_ERROR);
		}
	}   	
	fprintf(dz->fp,"\n");
	return(FINISHED);
}

/********************** MIRROR_PANNING ****************************/
 
int mirror_panning(char *thisword, int maxthiswordsize,dataptr dz)
{
	int exit_status;
	double pan;
	int was_neg = FALSE, newlen, leading_zero_cnt = 0;
	char *p;
	if(!strcmp(thisword,"C"))
		return output_aligned_word(thisword,maxthiswordsize,dz);
	else if(!strcmp(thisword,"L")) 
		return output_aligned_word("R",maxthiswordsize,dz);
	else if(!strcmp(thisword,"R"))
		return output_aligned_word("L",maxthiswordsize,dz);
	else if(sscanf(thisword,"%lf",&pan)!=1) {
		sprintf(errstr,"Failed to get pan value 1: mirror_panning()\n");
		return(PROGRAM_ERROR);
	}
	if(flteq(pan,0.0)) {
		if((exit_status = output_aligned_word(thisword,maxthiswordsize,dz))<0)
		return(exit_status);
	} else {
		if(pan < 0.0)
			was_neg = TRUE;
		pan = -pan;
		sprintf(errstr,"%lf",pan);
		newlen = strlen(thisword);
		leading_zero_cnt = 0;
		p = thisword;
		if(was_neg)
			p = thisword+1;
		while(*p=='0') {
			leading_zero_cnt++;
			p++;
		}
		if(*p=='.') {
			if(leading_zero_cnt==0)
				newlen++;							/* Allow for leading 0 being added */
			else
				newlen -= leading_zero_cnt - 1;		/* Allow for xs leading zeroes being taken away */
		}
		if(was_neg)		newlen--;		/* Allow for decimal point being removed */
		else			newlen++;		/* Allow for decimal point being added */
		
		*(errstr + newlen) = ENDOFSTR;
		if((exit_status = output_aligned_word(errstr,maxthiswordsize,dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}

/********************** OUTPUT_ALIGNED_WORD ****************************/
 
int output_aligned_word(char *thisword,int maxthiswordsize,dataptr dz)
{
	int space_count, n;
	fprintf(dz->fp,"%s",thisword);
	if((space_count = maxthiswordsize - strlen(thisword))<0) {
		sprintf(errstr,"Error in space_count: output_aligned_word()\n");
		return(PROGRAM_ERROR);
	}
	space_count++;
	for(n=0; n < space_count;n++)
		fprintf(dz->fp," ");
	return(FINISHED);
}

//TW UPDATE: NEW FUNCTION
/********************** ADDTOMIX ****************************/
 
int addtomix(dataptr dz)
{
	int    exit_status;
	int	   total_wordcnt, *chans;												
	int    n, m, sndfiles_in = dz->infilecnt - 1;
	char	temp[200], temp2[200];
	infileptr fq;
	double maxamp, maxloc;
	int maxrep;
	int getmax = 0, getmaxinfo = 0;
	if(sndfiles_in <= 0) {
		sprintf(errstr,"No new sound files remembered.\n");
		return(PROGRAM_ERROR);
	}
	if(dz->linecnt <= 0) {
		sprintf(errstr,"None of original mixdata remembered.\n");
		return(PROGRAM_ERROR);
	}
	if((fq = (infileptr)malloc(sizeof(struct filedata)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store data on files.\n");
		return(MEMORY_ERROR);
	}
	if ((chans = (int *)malloc(sndfiles_in * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store channel count information.\n");
		return(MEMORY_ERROR);
	}
	for(n = sndfiles_in,m = dz->all_words - 1; n>=1; n--,m--) {
		if((exit_status = readhead(fq,dz->ifd[n],dz->wordstor[m],&maxamp,&maxloc,&maxrep,getmax,getmaxinfo))<0) {
			sprintf(errstr,"Cannot read header of soundfile %s\n",dz->wordstor[m]);
			return(USER_ERROR);
		}
		if(((chans[n-1] = fq->channels) < 1) || (chans[n-1] > 2)) {
			sprintf(errstr,"Soundfile %s has wrong number of channels [%d] for this process.\n",dz->wordstor[m],chans[n-1]);
			return(DATA_ERROR);
		}
    }
	
	total_wordcnt = 0;
	for(n=0;n< dz->linecnt;n++) {
		for(m=0;m<dz->wordcnt[n];m++) {
			if(m==0)
				strcpy(temp,dz->wordstor[total_wordcnt++]);
			else {
				strcat(temp," ");
				strcat(temp,dz->wordstor[total_wordcnt++]);
			}
			if(total_wordcnt >= dz->all_words) {
				sprintf(errstr,"Word Count error.\n");
				return(PROGRAM_ERROR);
			}
		}
		strcat(temp,"\n");
		if(fputs(temp,dz->fp)<0) {
			sprintf(errstr,"Failed to write mixfile line %d to output file.\n",n+1);
			return(SYSTEM_ERROR);
		}
	}
	m = 0;
	while(total_wordcnt < dz->all_words) {
		strcpy(temp,dz->wordstor[total_wordcnt]);
		sprintf(temp2,"%lf",dz->duration);
		strcat(temp," ");
		strcat(temp,temp2);

		if(chans[m] == 1)
			strcat(temp,"  1 1.0 C\n");
		else
			strcat(temp," 2 1.0 L 1.0 R\n");
		if(fputs(temp,dz->fp)<0) {
			sprintf(errstr,"Failed to write mixfile line %d to output file.\n",n+1);
			return(SYSTEM_ERROR);
		}
		if(++total_wordcnt > dz->all_words) {
			sprintf(errstr,"Word Count error.\n");
			return(PROGRAM_ERROR);
		}
		m++;
		n++;
	}
	return(FINISHED);
}

