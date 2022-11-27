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
#include <string.h>
#include <structures.h>
#include <tkglobals.h>
#include <globcon.h>
#include <processno.h>
#include <modeno.h>
#include <filetype.h>
#include <arrays.h>
#include <mix.h>
#include <cdpmain.h>

#include <sfsys.h>
#ifdef unix
#define round lround
#endif

static int  getenvpeak(int n,int samps_per_second,
			int windows_per_sector,int sectors_per_window,int window_size,dataptr dz);
static int  gettime_of_maxsamp(int k, /*int*/float *env,/*int*/float *envend,int *envpos,int *peak_sampno,int window_size);
static int  create_envelope_arrays(int n,int numsects,int windows_per_sector,int sectors_per_window,
			int **envpos,/*int*/float **env,dataptr dz);
static int  count_sectors_to_scan(int *numsects,int lineno,int *skipsamps,int samps_per_second,dataptr dz);
static int  get_envelope(int skipsamps,int **envpos,/*int*/float **env,int nbuff,int nsec, int window_size,dataptr dz);
static void readenv_in_buffer(float *buf,int samps_to_process,int window_size,int **envpos,/*int*/float **env,int powflag);
static void getmaxsamp_in_window(float *buf,int startsamp,int sample_cnt,int *envpos, /*int*/float *env,int powflag);
static int  rescale_the_lines(double atten,dataptr dz);
static int  attenuate_line(int total_words,double atten,int wordcnt,dataptr dz);
static int  output_a_new_syncatt_file(double *timestor,double gain,dataptr dz);
static int  output_a_syncatt_file(double *timestor,double gain,dataptr dz);
static int  sum_maximum_samples(int n,double *total_maxval,dataptr dz);
static int  get_filesearch_data(dataptr dz);
static int  check_syncatt_window_factor(dataptr dz);

/******************************** SYNCHRONISE_MIX_ATTACK ******************************/
/* RWD crude hack to avoid SECSIZE abusage! */
/* NB dz->lparray relaced with dz->lfarray for MSY_ENVEL amp stuff*/

//TW WINSECSIZE replaced by global F_SECSIZE
int synchronise_mix_attack(dataptr dz)
{
	int    exit_status;
	double total_maxval = 0.0, attenuation = 1.0, *timestor;
	int   sectors_per_window, n,  max_atk_time = 0, sampblok, window_size;
	int    windows_per_sector, typefactor, inchans;
	int   total_words = 0, srate =0, samps_per_second;	
	int    textfile_filetype = dz->infile->filetype;
	for(n=0;n<dz->linecnt;n++) {
		windows_per_sector = 0;
		if((exit_status = open_file_retrieve_props_open(n,dz->wordstor[total_words],&srate,dz))<0)
			return(exit_status);
		inchans = dz->iparray[MSY_CHANS][n];
		samps_per_second = srate * inchans;
		typefactor       = dz->iparam[MSY_SRFAC] * inchans;
		sampblok         = (F_SECSIZE  * typefactor)/dz->iparam[MSY_WFAC];
		
		if((sectors_per_window = sampblok/F_SECSIZE)<=0)
			windows_per_sector = F_SECSIZE/sampblok;
		window_size = sampblok;

		if((exit_status = getenvpeak(n,samps_per_second,windows_per_sector,sectors_per_window,window_size,dz))<0)
			return(exit_status);
		if((exit_status = sum_maximum_samples(n,&total_maxval,dz))<0)	/* read maxsample and add to running sum */
			return(exit_status);
		dz->lparray[MSY_PEAKSAMP][n] /= inchans;	 					/* convert to 'stereo'-sample count: TRUNCATES */
		max_atk_time = max(max_atk_time,dz->lparray[MSY_PEAKSAMP][n]);	/* find attack time which is latest */
		total_words += dz->wordcnt[n];
		if(sndcloseEx(dz->ifd[0])<0) {
			sprintf(errstr, "Failed to close input file %s: line %d: synchronise_mix_attack()\n",
			dz->wordstor[total_words],n+1);
			return(SYSTEM_ERROR);
		}
		dz->ifd[0] = -1;
	}
	if(total_maxval > (double)F_MAXSAMP)									/* calculate required attenuation */
		attenuation = (double)F_MAXSAMP/total_maxval;

	if((dz->parray[MSY_TIMESTOR] = (double *)malloc(dz->linecnt * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store times.\n");
		return(MEMORY_ERROR);
	}
	timestor = dz->parray[MSY_TIMESTOR];
	for(n=0;n<dz->linecnt;n++)											/* calculate sndfile offsets */
		timestor[n] = (double)(max_atk_time - dz->lparray[MSY_PEAKSAMP][n])/(double)srate;

	dz->infile->filetype = textfile_filetype;
	switch(dz->infile->filetype) {
	case(SNDLIST):
	case(SYNCLIST):
		return output_a_new_syncatt_file(timestor,attenuation,dz);
	case(MIXFILE):
		return output_a_syncatt_file(timestor,attenuation,dz);
	default:
		sprintf(errstr,"Unknown case in synchronise_mix_attack()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);	/* NOTREACHED */
}

/* RWD NB window_size is in samps */
/* dz->lparray etc now floats */
/******************************** GETENVPEAK ******************************/

int getenvpeak(int lineno,int samps_per_second,int windows_per_sector,int sectors_per_window,int window_size,dataptr dz)
{   
	int exit_status;
	int peak_sampno;
	int skipsamps, numsects, searchsize, nbuff, nsec;
	int  *envpos;
	/*int*/float *env, *envend;
	if((exit_status = count_sectors_to_scan(&numsects,lineno,&skipsamps,samps_per_second,dz))<0)
		return(exit_status);
	if((exit_status = create_envelope_arrays(lineno,numsects,windows_per_sector,sectors_per_window,&envpos,&env,dz))<0)
		return(exit_status);
	searchsize = numsects * F_SECSIZE;
    nbuff      = searchsize/dz->buflen;	  			/* no. of whole buffs to srch   */
    nsec       = (searchsize % dz->buflen)/F_SECSIZE;	/* no. of further secs to srch  */
	if((exit_status = get_envelope(skipsamps,&envpos,&env,nbuff,nsec,window_size,dz))<0)
		return(exit_status);
    envend   = env;
    env      = dz->lfarray[MSY_ENVEL];
	envpos   = dz->iparray[MSY_ENVPOS];
	if((exit_status = gettime_of_maxsamp(lineno,env,envend,envpos,&peak_sampno,window_size))<0)
		return(exit_status);
	dz->lparray[MSY_PEAKSAMP][lineno] = peak_sampno + skipsamps;
	return(FINISHED);
}

/***************************** GETTIME_OF_MAXSAMP ***********************************/

int gettime_of_maxsamp(int lineno, /*int*/float *env,/*int*/float *envend,int *envpos,int *peak_sampno,int window_size)
{
	float peak = 0.0f, *thismax = env, *loudest_place = NULL;
	
	int  blokno;
	while(thismax < envend) {
		if(*thismax > peak) {
			peak = *thismax;
			loudest_place = thismax;
		}
		thismax++;
	}
	if(loudest_place==NULL) {
		fprintf(stdout,"WARNING: No peak found in file %d: Syncing to file start.\n",lineno+1);
		fflush(stdout);
		*peak_sampno = 0;
		return(FINISHED);
	}
	blokno       = loudest_place - env;           								/* to nearest shrtblok */
	*peak_sampno = (blokno * window_size) + envpos[blokno]; 					/* to nearest sample   */
	return(FINISHED);
}

/********************** CREATE_ENVELOPE_ARRAYS ***********************/

int create_envelope_arrays
(int lineno,int numsects,int windows_per_sector,int sectors_per_window,int **envpos,/*int*/float **env,dataptr dz)
{
	int arraysize;
	if(windows_per_sector > 0) {
		arraysize = numsects * windows_per_sector;
		arraysize += windows_per_sector;  /* Allow for a short buf at end */
	} else {
		arraysize = numsects/sectors_per_window;
		arraysize++;		           /* round up + allow ditto */
	}
	if(lineno==0) {
		if((dz->iparray[MSY_ENVPOS] = (int *)malloc(arraysize * sizeof(int)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to store envelope positions.\n");
			return(MEMORY_ERROR);
		}
		if((dz->lfarray[MSY_ENVEL] = (/*int*/float *)malloc(arraysize * sizeof(/*int*/float)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to store envelope.\n");
			return(MEMORY_ERROR);
		}
	} else {
		free(dz->iparray[MSY_ENVPOS]);
		free(dz->lparray[MSY_ENVEL]);
		dz->iparray[MSY_ENVPOS] = NULL;
		dz->lparray[MSY_ENVEL]  = NULL;
		if((dz->iparray[MSY_ENVPOS] = (int *)realloc(dz->iparray[MSY_ENVPOS],arraysize * sizeof(int)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to reallocate envelope positions.\n");
			return(MEMORY_ERROR);
		}
		if((dz->lfarray[MSY_ENVEL] = (/*int*/float *)realloc(dz->lfarray[MSY_ENVEL],arraysize * sizeof(/*int*/float)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to reallocate envelope.\n");
			return(MEMORY_ERROR);
		}
	}
	*envpos = dz->iparray[MSY_ENVPOS];
	*env    = dz->lfarray[MSY_ENVEL];
	return(FINISHED);
}

/****************************** COUNT_SECTORS_TO_SCAN *******************************/

int count_sectors_to_scan(int *numsects,int lineno,int *skipsamps,int samps_per_second,dataptr dz)
{
	int endsamps, endsecs, skipsecs;
	double *start = dz->parray[MSY_STARTSRCH];
	double *end   = dz->parray[MSY_ENDSRCH];
	int shsecsize = F_SECSIZE;
	skipsecs   = 0;
	/* RWD what is this doing? */
	*skipsamps = round(start[lineno] * (double)samps_per_second);
//TW SAFETY for the moment: matches up with secsize rounding  below
// and secsize rounding in envelope-windowing and search
	skipsecs   = *skipsamps/shsecsize;		/* TRUNCATE */
	*skipsamps = skipsecs * shsecsize;   

	if(end[lineno] < 0.0)						/* -1.0 flags ENDOFSNDFILE */
		endsamps = dz->lparray[MSY_SAMPSIZE][lineno];
	else
		endsamps = round(end[lineno] * samps_per_second);
	endsecs  = endsamps/shsecsize;
	if(endsecs * shsecsize < endsamps)		/* CEILING */
		endsecs++;
	*numsects = max((int)1,endsecs - skipsecs);
	return(FINISHED);
}

/************************** GET_ENVELOPE ***************************/

int get_envelope(int skipsamps,int **envpos,/*int*/float **env,int nbuff,int nsec, int window_size,dataptr dz)
{
	int  exit_status;
	int n, shrtstoget;
	int shsecsize = F_SECSIZE;
	float *buf = dz->sampbuf[0];
	int powflag = dz->vflag[MSY_POWMETHOD];
	if(sndseekEx(dz->ifd[0],skipsamps,0)<0) {
		sprintf(errstr,"seek() failed in get_envelope()\n");
		return(SYSTEM_ERROR);
	}
	if(nbuff > 0) {
		for(n = 0; n < nbuff-1; n++)	{			/* 1ST PASS : WHOLE BUFFERS */
			if((exit_status = read_samps(dz->bigbuf,dz))<0)
				return(exit_status);
			if(dz->ssampsread < dz->buflen) {
				sprintf(errstr,"Error in buffer accounting: get_envelope()\n");
				return(PROGRAM_ERROR);
			}
			readenv_in_buffer(buf,dz->buflen,window_size,envpos,env,powflag);
		}
												/* 2ND PASS : LAST WHOLE BUFFERS */
		if((exit_status = read_samps(dz->bigbuf,dz))<0)
			return(exit_status);
		if(nsec > 0 && dz->ssampsread < dz->buflen) {
			sprintf(errstr,"Error in buffer accounting: get_envelope()\n");
			return(PROGRAM_ERROR);
		}
		readenv_in_buffer(buf,min(dz->ssampsread,dz->buflen),window_size,envpos,env,powflag);
	}

	if(nsec) {									/* 3RD PASS : REMAINING SECTORS */
		shrtstoget = nsec * shsecsize;
		if((exit_status = read_samps(dz->bigbuf,dz))<0)
			return(exit_status);
		readenv_in_buffer(buf,min(shrtstoget,dz->ssampsread),window_size,envpos,env,powflag);
	}
	return(FINISHED);
}

/************************* READENV_IN_BUFFER ******************************/

void readenv_in_buffer(float *buf,int samps_to_process,int window_size,int **envpos,/*int*/float **env,int powflag)
{
	int  startsamp = 0;
	int  *epos;
	float *ev;
	epos  = *envpos;
	ev    = *env;
	while(samps_to_process >= window_size) {
		getmaxsamp_in_window(buf,startsamp,window_size,epos,ev,powflag);
		epos++;
		ev++;
		startsamp 		 += window_size;
		samps_to_process -= window_size;
	}
	if(samps_to_process) {	/* Handle any final short buffer */
		getmaxsamp_in_window(buf,startsamp,samps_to_process,epos,ev,powflag);
		epos++;
		ev++;
	}
	*envpos = epos;
	*env    = ev;
}

/*************************** GETMAXSAMP_IN_WINDOW ********************************/

void getmaxsamp_in_window(float *buf,int startsamp,int sample_cnt,int *envpos, /*int*/float *env,int powflag)
{
	int   i, endsamp = startsamp + sample_cnt, maxpos = startsamp;
	double rms_power = 0.0;
	double maxval = 0, val;
	

	switch(powflag) {			
	case(TRUE):
		for(i = startsamp; i<endsamp; i++) {
			val = fabs(buf[i]);
			rms_power += (double)(val * val);
			if(val > maxval) {
				maxval = val;
				maxpos = i;
			}
		}
		rms_power /= (double)sample_cnt;
		rms_power  = sqrt(rms_power);
		*env       = (float) rms_power;
		break;
	case(FALSE):
		for(i = startsamp; i<endsamp; i++) {
			val = fabs(buf[i]);
			if(val > maxval) {
				maxval = val;
				maxpos = i;
			}
		}
		*env = (float)maxval;
		break;
	}
	*envpos = (int)(maxpos - startsamp);
}

/*************************** RESCALE_THE_LINES ***************************/

int rescale_the_lines(double atten,dataptr dz)
{
	int exit_status;
	int lineno;
	int total_words = 0;
	for(lineno=0;lineno<dz->linecnt;lineno++) {
		if((exit_status = attenuate_line(total_words,atten,dz->wordcnt[lineno],dz))<0)
			return(exit_status);
		total_words += dz->wordcnt[lineno];
	}
	return(FINISHED);
}

/************************** ATTENUATE_LINE ***************************/

int attenuate_line(int total_words,double atten,int wordcnt,dataptr dz)
{
	int    exit_status;
	int   wordno, newlen;
	double level;
	wordno = total_words + MIX_LEVELPOS;
	if((exit_status = get_level(dz->wordstor[wordno],&level))<0)
		return(exit_status);
	sprintf(errstr,"%.5lf",level * atten);
	if((newlen = (int)strlen(errstr)) > (int)strlen(dz->wordstor[wordno])) {
		if((dz->wordstor[wordno] = (char *)realloc(dz->wordstor[wordno],(newlen+1) * sizeof(char)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for wordstore %d\n",wordno+1);
			return(MEMORY_ERROR);
		}
	}
	strcpy(dz->wordstor[wordno],errstr);
	if(wordcnt <= MIX_MIDLINE)
		return(FINISHED);

	wordno = total_words + MIX_RLEVELPOS;
	if((exit_status = get_level(dz->wordstor[wordno],&level))<0)
		return(exit_status);
	sprintf(errstr,"%.5lf",level * atten);
	if((newlen = (int)strlen(errstr)) > (int)strlen(dz->wordstor[wordno])) {
		if((dz->wordstor[wordno] = (char *)realloc(dz->wordstor[wordno],(newlen+1) * sizeof(char)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for wordstore %d\n",wordno+1);
			return(MEMORY_ERROR);
		}
	}
	strcpy(dz->wordstor[wordno],errstr);
	return(FINISHED);
}

/************************** OUTPUT_A_NEW_SYNCATT_FILE ***************************/

int output_a_new_syncatt_file(double *timestor,double gain,dataptr dz)
{
	int  exit_status;
	int  max_timeword = 0;
	int  max_namelen  = 0;
	int total_words  = 0, n;
	for(n=0;n<dz->linecnt;n++) {
		max_namelen = max(max_namelen,(int)strlen(dz->wordstor[total_words]));
		sprintf(errstr,"%.5lf",timestor[n]);
		max_timeword = max(max_timeword,(int)strlen(errstr));
		total_words += dz->wordcnt[n];
	}							
	total_words = 0;
	for(n=0;n<dz->linecnt;n++) {						
		if((exit_status = sync_and_output_mixfile_line
		(n,dz->wordstor[total_words],max_namelen,max_timeword,timestor[n],gain,dz))<0)
			return(exit_status);
		total_words += dz->wordcnt[n];
	}
	return(FINISHED);
}

/************************** OUTPUT_A_SYNCATT_FILE ***************************/

int output_a_syncatt_file(double *timestor,double gain,dataptr dz)
{
	int exit_status;
	int *maxwordsize;
	int postdec = DEFAULT_DECIMAL_REPRESENTATION;
	if((exit_status = retime_the_lines(timestor,postdec,dz))<0)
		return(exit_status);
	if(gain < 1.0) {
		if((exit_status = rescale_the_lines(gain,dz))<0)
			return(exit_status);
	}
	if((exit_status = timesort_mixfile(timestor,dz))<0)
		return(exit_status);
	if((exit_status = get_maxwordsize(&maxwordsize,dz))<0)
		return(exit_status);
	if((exit_status = output_mixfile_lines(maxwordsize,dz))<0)
		return(exit_status);
	free(maxwordsize);
	return(FINISHED);
}

/************************** SUM_MAXIMUM_SAMPLES ***************************/
/* RWD: IS THIS ROUNDING FOR SFSEEK, OR ROUNDING FOR WINDOW SECTORS????? */

int sum_maximum_samples(int n,double *total_maxval,dataptr dz)
{
	int   exit_status;
	float *buf = dz->sampbuf[0];
	int  shsecsize = F_SECSIZE;
	int  peak_sampno = dz->lparray[MSY_PEAKSAMP][n];
	int  sectorstart_of_maxsamp, sampstep;

	sectorstart_of_maxsamp = (peak_sampno/shsecsize) * shsecsize;	 /* TRUNCATE */
	sampstep  = peak_sampno - sectorstart_of_maxsamp;
	if(sndseekEx(dz->ifd[0],sectorstart_of_maxsamp,0)<0) {
		sprintf(errstr,"soundfile seek failed in sum_maximum_samples()\n");
		return(SYSTEM_ERROR);
	}
	if((exit_status = read_samps(buf,dz))<0)
		return(exit_status);
	if(sampstep >= dz->ssampsread) {
		sprintf(errstr,"Buffer accounting anomaly: sum_maximum_samples()\n");
		return(PROGRAM_ERROR);
	}
	*total_maxval +=  fabs(buf[sampstep]);
	return(FINISHED);
}

/************************* GET_FILESEARCH_DATA ************************/

int get_filesearch_data(dataptr dz)
{   
	int    exit_status;
	int   srate = 0, total_words, n;
	int    *inchans;
	int   shsecsize = F_SECSIZE, *samplen;
	double filedur;
	double minsyncscan;
	double *start, *end;
	int    textfile_filetype = dz->infile->filetype;

	if(dz->linecnt > SF_MAXFILES) {
		sprintf(errstr,"Maximum number of sndfiles [%d] exceeded.\n",SF_MAXFILES);
		return(USER_ERROR);
	} else if(dz->linecnt <= 0) {
		sprintf(errstr,"No data in sync file.\n");
		return(USER_ERROR);
	}
	if((dz->parray[MSY_STARTSRCH] = (double *)malloc(dz->linecnt * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store startsearch array.\n");
		return(MEMORY_ERROR);
	}
	start = dz->parray[MSY_STARTSRCH];
	if((dz->parray[MSY_ENDSRCH] = (double *)malloc(dz->linecnt * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store endsearch array.\n");
		return(MEMORY_ERROR);
	}
	end = dz->parray[MSY_ENDSRCH];
	if((dz->lparray[MSY_SAMPSIZE] = (int *)malloc(dz->linecnt * sizeof(int)))==NULL) {
		sprintf(errstr,"gINSUFFICIENT MEMORY to store sizes in samples.\n");
		return(MEMORY_ERROR);
	}
	samplen = dz->lparray[MSY_SAMPSIZE];
	if((dz->iparray[MSY_CHANS] = (int *)malloc(dz->linecnt * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store channel info.\n");
		return(MEMORY_ERROR);
	}
	inchans = dz->iparray[MSY_CHANS];
	if((dz->lparray[MSY_PEAKSAMP] = (int *)malloc(dz->linecnt * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store peak information.\n");
		return(MEMORY_ERROR);
	}
	if((exit_status = establish_file_data_storage_for_mix((int)1,dz))<0)
		return(exit_status);
	total_words = 0;
	for(n=0;n<dz->linecnt;n++) {
		if((exit_status = open_file_and_retrieve_props(n,dz->wordstor[total_words],&srate,dz))<0)
			return(exit_status);
		samplen[n]	= dz->insams[0];
		inchans[n]	= dz->infile->channels;
//REVISION TW July, 2004
//		filedur = (double)(samplen[n]/inchans[n])/(double)srate;
		total_words += dz->wordcnt[n];
	}

	if(srate > SAMPLE_RATE_DIVIDE)  dz->iparam[MSY_SRFAC]=2;
	else							dz->iparam[MSY_SRFAC]=1;

	minsyncscan = (double)((shsecsize * dz->iparam[MSY_SRFAC])/MAX_WINFAC)/(double)srate;

	total_words = 0;
	for(n=0;n<dz->linecnt;n++) {
		switch(dz->wordcnt[n]) {
		case(3):
			if(dz->infile->filetype==MIXFILE) {
				sprintf(errstr,"Anomalous line length [%d] in mixfile\n",dz->wordcnt[n]);
				return(PROGRAM_ERROR);
			}
			if(sscanf(dz->wordstor[total_words+1],"%lf",&(start[n]))!=1) {
				sprintf(errstr,"Failed to read starttime: line %d: get_filesearch_data()\n",n+1);
				return(PROGRAM_ERROR);
			}
			if(sscanf(dz->wordstor[total_words+2],"%lf",&(end[n]))!=1) {
				sprintf(errstr,"Failed to read endtime: line %d: get_filesearch_data()\n",n+1);
				return(PROGRAM_ERROR);
			}
			if((start[n] < 0.0) || (end[n] < 0.0) || (start[n] + minsyncscan >= end[n])) {
				sprintf(errstr,"Impossible or incompatible searchtimes [%.5lf to %.5lf]: line %d.\n",
				start[n],end[n],n+1);
				return(USER_ERROR);
			}
//REVISION TW July, 2004
			filedur = (double)(samplen[n]/inchans[n])/(double)srate;
			if(start[n] >= filedur - minsyncscan) {
				sprintf(errstr,"starttime on line %d is beyond effective file end.\n",n+1);
				return(DATA_ERROR);
			}
			if(end[n] >= filedur)
				end[n] = -1.0;	/* flags END_OF_SNDFILE */
			break;
		default:
			start[n] = 0.0;
			end[n]   = -1.0;	/* flags END_OF_SNDFILE */
			break;
		}
		total_words += dz->wordcnt[n];
	}
	dz->infile->filetype = textfile_filetype;
	return(FINISHED);
}

/***************************** SYNCATT_PRESETS **************************/

int syncatt_presets(dataptr dz)
{
	int exit_status;
	if(!check_syncatt_window_factor(dz))
		return(USER_ERROR);
	if((exit_status= get_filesearch_data(dz))<0)
		return(exit_status);
	return(FINISHED);
}

/***************************** CHECK_SYNCATT_WINDOW_FACTOR **************************/

int check_syncatt_window_factor(dataptr dz)
{
	int valid_value = MIN_WINFAC;
	while(valid_value <= MAX_WINFAC) {
		if(dz->iparam[MSY_WFAC]==valid_value)
			return(TRUE);
		valid_value *= 2;
	}
	return(FALSE);
}
