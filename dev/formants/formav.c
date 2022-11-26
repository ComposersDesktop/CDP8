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



// (2) Do need an entry in  standalone ....

#include <stdio.h>
#include <stdlib.h>
#include <structures.h>
#include <filetype.h>
#include <cdpmain.h>
#include <logic.h>
#include <sfsys.h>

char errstr[2400];

static int local_print_messages_and_close_sndfiles(int exit_status,int is_launched,int ofd,int *ifd,int infilecnt);
static int local_read_samps(float *bbuf,int wanted,int *total_samps_read, int *ssampsread,int n,int *ifd, int totalsize);
static int reset_file_to_start_and_pointers_to_after_descriptor_blocks
			(int *ifd,int *windows_in_buf,int n,int wanted,int specenvcnt,float *bigfbuf,float **flbufptr,
			int *total_samps_read,int totalsize);
static int check_the_formant_data(int *windows_in_buf,float *specenvpch,float *specenvtop,float *bigfbuf,float **flbufptr,char **argv,
								infileptr ifp,int specenvcnt,int wanted,int *total_samps_read,int totalsize,int n,int *ifd);
static int initialise_the_formant_data(int *windows_in_buf,float *specenvpch,float *specenvtop,float *bigfbuf,float **flbufptr,
								infileptr ifp,int specenvcnt,int wanted,int *total_samps_read,int totalsize,int n,int *ifd);
static void display_sloom_time(int samps_sent,int totalsize);
static int local_readhead(infileptr inputfile,int ifd,char *filename,int getmax,int needmaxinfo);
static void local_splice_multiline_string(char *str,char *prefix);
static int flteq(double f1,double f2);
static int formant_headwrite(int ofd,infileptr ifp);

#define FAILED	(-1)
#define SUCCEEDED	(0)
#define PBAR_LENGTH	(256)

#define CONTINUE      (1)
#define FINISHED      (0)
#define	GOAL_FAILED	  (-1)	/* program succeeds, but users goal not achieved: e.g. find pitch */
#define USER_ERROR    (-2)	/* program fails because wrong data, no data etc given by user */
#define	DATA_ERROR	  (-3)	/* Data is unsuitable, incorrect or missing */
#define	MEMORY_ERROR  (-4)	/* program fails because ran out of, or mismanaged, memory */
#define	SYSTEM_ERROR  (-5)  /* failure to write to or truncate outfile: usually means H/D is full */
#define PROGRAM_ERROR (-6)	/* program fails because the programming is naff */
#define	USAGE_ONLY	  (-7)	/* program interrogated for usage_message only */	
#define	TK_USAGE	  (-8)	/* program interrogated by TK for usage_message only */	
#define	BAD_INFILECNT (-9)	/* Bad infilecnt sent from TK at program testing stage */

#ifndef FALSE
#define FALSE   (0)
#endif

#ifndef TRUE
#define TRUE	(1)
#endif

#define DESCRIPTOR_DATA_BLOKS 	(2)
#define	ENDOFSTR			 ('\0')
#define FLTERR 		 	(0.000002)

/**************************************** MAIN *********************************************/

// cmdline formav formfil1 formfil2 [formfil3 ....] outformfil dur

int main(int argc,char *argv[])
{
	int exit_status, n, m, k, infilecnt;
	int is_launched = FALSE;
	int *ifd = NULL, ofd = -1;
	int specenvcnt;
	int wanted, windows_in_buf,outfltcnt,outwindowcnt;
	infileptr ifp,ifp2;
	float *bigfbuf, *bigbufend, *flbufptr, *oflbufptr, *specenvpch, *specenvtop;
	int total_windows = 0, ssampsread, min_inwindowsize, totalsize, size, total_samps_read;
	double dur;
	
	if(sflinit("cdp")){
		sfperror("cdp: initialisation\n");
		return(FAILED);
	}
	if(argc < 5) {
		fprintf(stdout,"ERROR: Insufficient arguments on commandline.\n");
		fflush(stdout);
		return(FAILED);
	}
	if(sscanf(argv[argc-1],"%lf",&dur)!=1) {
		fprintf(stdout,"ERROR: Cannot read output duration from command line.\n");
		fflush(stdout);
		return(FAILED);
	}
	if (dur < 0.2) {
		fprintf(stdout,"ERROR: Output duration (%lf) too short (min 0.2 secs).\n",dur);
		fflush(stdout);
		return(FAILED);
	}
	if((ifp = (infileptr)malloc(sizeof(struct filedata)))==NULL) {
		fprintf(stdout,"ERROR: INSUFFICIENT MEMORY to store data on infiles (1)\n");
		fflush(stdout);
		return(FAILED);
	}
	if((ifp2 = (infileptr)malloc(sizeof(struct filedata)))==NULL) {
		fprintf(stdout,"ERROR: INSUFFICIENT MEMORY to store data on infiles (2)\n");
		fflush(stdout);
		return(FAILED);
	}
	if((ifd = (int *)malloc((argc-3) * sizeof(int))) == NULL) {
		fprintf(stdout,"ERROR: Insufficient memory.\n");
		fflush(stdout);
		return(FAILED);
	}
	for(n=1;n<argc-2;n++) {
		m = n-1;
		if((ifd[m] = sndopenEx(argv[n],0,CDP_OPEN_RDONLY)) < 0) {
			sprintf(errstr,"FAILED TO OPEN FILE %s\n",argv[n]);
			exit_status =	DATA_ERROR;
			local_print_messages_and_close_sndfiles(exit_status,is_launched,ofd,ifd,m);
			return(FAILED);
		}
	}
	infilecnt = m+1;

	for(n=0;n<infilecnt;n++) {	
		if((exit_status = local_readhead(ifp,ifd[n],argv[n+1],0,0))<0) {
			exit_status =	SYSTEM_ERROR;
			local_print_messages_and_close_sndfiles(exit_status,is_launched,ofd,ifd,infilecnt);
			return(FAILED);
		}
		if(n==0) {
			if(ifp->filetype != FORMANTFILE) {
				sprintf(errstr,"First input file is not a formant file.\n");
				exit_status = DATA_ERROR;
				local_print_messages_and_close_sndfiles(exit_status,is_launched,ofd,ifd,infilecnt);
				return(FAILED);
			}
			ifp2->filetype		= ifp->filetype;
			ifp2->srate 	   	= ifp->srate;	   		
			ifp2->stype 	    = ifp->stype;
			ifp2->origstype  	= ifp->origstype;
			ifp2->origrate   	= ifp->origrate;
			ifp2->channels   	= ifp->channels;
			ifp2->origchans  	= ifp->origchans;
			ifp2->specenvcnt 	= ifp->specenvcnt;
			ifp2->Mlen 	   		= ifp->Mlen;
			ifp2->Dfac 	   		= ifp->Dfac;
			ifp2->arate 	   	= ifp->arate;
			ifp2->descriptor_samps = ifp->descriptor_samps;
		} else {
			if(ifp2->filetype	!= ifp->filetype
			|| ifp2->srate 		!= ifp->srate
			|| ifp2->stype 		!= ifp->stype
			|| ifp2->origstype  != ifp->origstype
			|| ifp2->origrate   != ifp->origrate
			|| ifp2->channels   != ifp->channels
			|| ifp2->origchans  != ifp->origchans
			|| ifp2->specenvcnt != ifp->specenvcnt
			|| ifp2->Mlen 	   	!= ifp->Mlen
			|| ifp2->Dfac 	   	!= ifp->Dfac
			|| ifp2->arate 		!= ifp->arate
			|| ifp2->descriptor_samps != ifp->descriptor_samps) {
				sprintf(errstr,"Input files have incompatible properties.\n");
				exit_status = DATA_ERROR;
				local_print_messages_and_close_sndfiles(exit_status,is_launched,ofd,ifd,infilecnt);
				return(FAILED);
			}
		}
	}
	wanted = ifp->specenvcnt * 8;
	specenvcnt = ifp->specenvcnt;
	min_inwindowsize = 10000000;
	totalsize = 0;
	for(n=0;n<infilecnt;n++) {
		size = sndsizeEx(ifd[n]);
		if(n==0)
			min_inwindowsize = size;
		else if(size < min_inwindowsize)
			min_inwindowsize = size;
		totalsize += size;
	}
	if(min_inwindowsize - (specenvcnt * 2) <= 0) {
		sprintf(errstr,"No significant data in at least one of input files\n");
		local_print_messages_and_close_sndfiles(exit_status,is_launched,ofd,ifd,infilecnt);
		return(FAILED);
	}
	outfltcnt = ((int)round(dur / ifp->frametime) * specenvcnt) + 2;
	if((ofd = sndcreat_formatted(argv[argc-1],outfltcnt,SAMP_FLOAT,
			ifp->channels,ifp->srate,CDP_CREATE_NORMAL)) < 0) {
		sprintf(errstr,"Cannot open output file %s\n", argv[argc-1]);
		exit_status = SYSTEM_ERROR;
		local_print_messages_and_close_sndfiles(exit_status,is_launched,ofd,ifd,infilecnt);
		return(FAILED);
	}
	outwindowcnt = (outfltcnt/specenvcnt) - 2;
	is_launched = 1;
	if((specenvpch = (float *)malloc(specenvcnt * sizeof(float))) == NULL) {
		sprintf(errstr,"Insufficient memory to store formant descriptor data 1.\n");
		exit_status = MEMORY_ERROR;
		local_print_messages_and_close_sndfiles(exit_status,is_launched,ofd,ifd,infilecnt);
		return(FAILED);
	}
	if((specenvtop = (float *)malloc(specenvcnt * sizeof(float))) == NULL) {
		sprintf(errstr,"Insufficient memory to store formant descriptor data 2.\n");
		exit_status = MEMORY_ERROR;
		local_print_messages_and_close_sndfiles(exit_status,is_launched,ofd,ifd,infilecnt);
		return(FAILED);
	}
	if((oflbufptr = (float *)malloc(specenvcnt * sizeof(float))) == NULL) {
		sprintf(errstr,"Insufficient memory to store output formant data.\n");
		exit_status = MEMORY_ERROR;
		local_print_messages_and_close_sndfiles(exit_status,is_launched,ofd,ifd,infilecnt);
		return(FAILED);
	}
	memset((char *)oflbufptr,0,specenvcnt * sizeof(float));
	if((bigfbuf = (float *)malloc(wanted * sizeof(float))) == NULL) {
		sprintf(errstr,"Insufficient memory to store input formant data.\n");
		exit_status = MEMORY_ERROR;
		local_print_messages_and_close_sndfiles(exit_status,is_launched,ofd,ifd,infilecnt);
		return(FAILED);
	}
	bigbufend = bigfbuf + wanted;
	memset((char *)bigfbuf,0,wanted * sizeof(float));
	total_samps_read = 0;
	for(n=0;n<infilecnt;n++) {
		if(n==0) {
			if((exit_status = initialise_the_formant_data(&windows_in_buf,specenvpch,specenvtop,bigfbuf,
			&flbufptr,ifp,specenvcnt,wanted,&total_samps_read,totalsize,n,ifd))<0) {
				local_print_messages_and_close_sndfiles(exit_status,is_launched,ofd,ifd,infilecnt);
				return(FAILED);
			}
		} else {
			if((exit_status = check_the_formant_data(&windows_in_buf,specenvpch,specenvtop,bigfbuf,
				&flbufptr,argv,ifp,specenvcnt,wanted,&total_samps_read,totalsize,n,ifd))<0) {
				local_print_messages_and_close_sndfiles(exit_status,is_launched,ofd,ifd,infilecnt);
				return(FAILED);
			}
		}
		m = 0;
		while(m < windows_in_buf) {
			for(k = 0;k<specenvcnt;k++)
				oflbufptr[k] += flbufptr[k];
			flbufptr += specenvcnt;
			total_windows++;
			m++;
		}
		for (;;) {
			if((exit_status = local_read_samps(bigfbuf,wanted,&total_samps_read,&ssampsread,n,ifd,totalsize)) < 0)
				return(exit_status);
			if(ssampsread <= 0)
				break;
			windows_in_buf = ssampsread/specenvcnt;
			flbufptr = bigfbuf;
			m = 0;
			while(m < windows_in_buf) {
				for(k = 0;k<specenvcnt;k++)
					oflbufptr[k] += flbufptr[k];
				flbufptr += specenvcnt;
				total_windows++;
				m++;
			}
		}
		if(sndcloseEx(ifd[n]) < 0) {
			fprintf(stdout,"WARNING: Failed to close file %s\n",argv[n+1]);
			fflush(stdout);
		}
		ifd[n] = -1;
	}
	ifd = NULL;
	for(k = 0;k<specenvcnt;k++)
		oflbufptr[k] = (float) (oflbufptr[k]/(double)total_windows);
	if(fputfbufEx(specenvpch,specenvcnt,ofd)<=0) {
		sprintf(errstr,"Can't write to output soundfile: %s\n",sferrstr());
		return(SYSTEM_ERROR);
	}
	if(fputfbufEx(specenvtop,specenvcnt,ofd)<=0) {
		sprintf(errstr,"Can't write to output soundfile: %s\n",sferrstr());
		return(SYSTEM_ERROR);
	}
	for(n=0;n<outwindowcnt;n++) {
		if(fputfbufEx(oflbufptr,specenvcnt,ofd)<=0) {
			sprintf(errstr,"Can't write to output soundfile: %s\n",sferrstr());
			return(SYSTEM_ERROR);
		}
	}
	if(sndcloseEx(ofd) < 0) {
		sprintf(errstr,"ERROR: Failed to close output file %s\n",argv[argc-1]);
		exit_status = SYSTEM_ERROR;
		local_print_messages_and_close_sndfiles(exit_status,is_launched,ofd,ifd,infilecnt);
		return(FAILED);
	}
	if((exit_status = formant_headwrite(ofd,ifp))<0) {
		local_print_messages_and_close_sndfiles(exit_status,is_launched,ofd,ifd,infilecnt);
		return(FAILED);
	}
	local_print_messages_and_close_sndfiles(exit_status,is_launched,ofd,ifd,infilecnt);
	return(SUCCEEDED);
}

/***************************** INITIALISE_THE_FORMANT_DATA *************************/

int initialise_the_formant_data(int *windows_in_buf,float *specenvpch,float *specenvtop,float *bigfbuf,float **flbufptr,
								infileptr ifp,int specenvcnt,int wanted,int *total_samps_read,int totalsize,int n,int *ifd)
{
	int exit_status;
	int ssampsread;
	int fsamps_to_read = ifp->descriptor_samps/DESCRIPTOR_DATA_BLOKS;
	if((exit_status = local_read_samps(bigfbuf,wanted,total_samps_read,&ssampsread,n,ifd,totalsize)) < 0)
		return(exit_status);
	if(ssampsread < ifp->descriptor_samps) {
		sprintf(errstr,"No significant data in formantfile.\n");
		return(DATA_ERROR);
	}
	*flbufptr = bigfbuf;
	memcpy((char *)specenvpch,(char *)(*flbufptr),(size_t)(fsamps_to_read * sizeof(float)));
	*flbufptr += specenvcnt;
	memcpy((char *)specenvtop,(char *)(*flbufptr),(size_t)(fsamps_to_read * sizeof(float)));
	*flbufptr = bigfbuf + (DESCRIPTOR_DATA_BLOKS * specenvcnt);
	*windows_in_buf = ssampsread/specenvcnt;    
	if((*windows_in_buf -= DESCRIPTOR_DATA_BLOKS) <= 0) {
		sprintf(errstr,"Buffers too SMALL for formant data.\n");
		return(PROGRAM_ERROR);
	}
	return reset_file_to_start_and_pointers_to_after_descriptor_blocks
	(ifd,windows_in_buf,n,wanted,specenvcnt,bigfbuf,flbufptr,total_samps_read,totalsize);
}

/***************************** CHECK_THE_FORMANT_DATA *************************/

int check_the_formant_data(int *windows_in_buf,float *specenvpch,float *specenvtop,float *bigfbuf,float **flbufptr,char **argv,
								infileptr ifp,int specenvcnt,int wanted,int *total_samps_read,int totalsize,int n,int *ifd)
{
	int exit_status, k;
	int ssampsread;
	int fsamps_to_read = ifp->descriptor_samps/DESCRIPTOR_DATA_BLOKS;
	if((exit_status = local_read_samps(bigfbuf,wanted,total_samps_read,&ssampsread,n,ifd,totalsize)) < 0)
		return(exit_status);
	if(ssampsread < ifp->descriptor_samps) {
		sprintf(errstr,"No significant data in formantfile.\n");
		return(DATA_ERROR);
	}
	*flbufptr = bigfbuf;
	for(k=0;k<specenvcnt;k++) {
		if(!flteq(specenvpch[k],(*flbufptr)[k])) {
			sprintf(errstr,"Formant file %s has a different format to the others\n",argv[n+1]);
			return(DATA_ERROR);
		}
	}
	*flbufptr += specenvcnt;
	for(k=0;k<specenvcnt;k++) {
		if(!flteq(specenvtop[k],(*flbufptr)[k])) {
			sprintf(errstr,"Formant file %s has a different format to the others\n",argv[n+1]);
			return(DATA_ERROR);
		}
	}
	*flbufptr = bigfbuf + (DESCRIPTOR_DATA_BLOKS * specenvcnt);
	*windows_in_buf = ssampsread/specenvcnt;    
	if((*windows_in_buf -= DESCRIPTOR_DATA_BLOKS) <= 0) {
		sprintf(errstr,"Buffers too SMALL for formant data.\n");
		return(PROGRAM_ERROR);
	}
	return reset_file_to_start_and_pointers_to_after_descriptor_blocks
	(ifd,windows_in_buf,n,wanted,specenvcnt,bigfbuf,flbufptr,total_samps_read,totalsize);
}

/*********************** RESET_FILE_TO_START_AND_POINTERS_TO_AFTER_DESCRIPTOR_BLOCKS **********************/

int reset_file_to_start_and_pointers_to_after_descriptor_blocks
(int *ifd,int *windows_in_buf,int n,int wanted,int specenvcnt,float *bigfbuf,float **flbufptr,int *total_samps_read,int totalsize)
{
	int exit_status;
	int ssampsread;
	if(sndseekEx(ifd[n],0L,0)<0) { /* RESET FILE POINTER TO START OF DATA */
		sprintf(errstr,"seek failed in reset_file_to_start_and_pointers_to_after_descriptor_blocks()\n");
		return(SYSTEM_ERROR);
	}
	if((exit_status = local_read_samps(bigfbuf,wanted,total_samps_read,&ssampsread,n,ifd,totalsize)) < 0) {
		sprintf(errstr,"Read problem after rewind in reset_file_to_start_and_pointers_to_after_descriptor_blocks()\n");
		return(SYSTEM_ERROR);
	}
	*total_samps_read -= ssampsread;	/* allow for the backtracking invloved in reading descriptor blocks */
	*windows_in_buf = ssampsread/specenvcnt;    
	if((*windows_in_buf -= DESCRIPTOR_DATA_BLOKS) < 0) {
		sprintf(errstr,"Buffers too SMALL in reset_file_to_start_and_pointers_to_after_descriptor_blocks()\n");
		return(PROGRAM_ERROR);
	}
	(*flbufptr) = bigfbuf + (DESCRIPTOR_DATA_BLOKS * specenvcnt);
	return(FINISHED);
}

/*********************** LOCAL_READ_SAMPS **********************/

int local_read_samps(float *bbuf,int wanted,int *total_samps_read, int *ssampsread,int n,int *ifd,int totalsize)
{
	if((*ssampsread = fgetfbufEx(bbuf, wanted,ifd[n],0)) < 0) {
		sprintf(errstr,"Can't read samples from input soundfile.\n");
		return(SYSTEM_ERROR);
	}
	*total_samps_read += *ssampsread;
	display_sloom_time(*total_samps_read,totalsize);

	return(FINISHED);
}


/*********************** DISPLAY_SLOOM_TIME **********************/

void display_sloom_time(int samps_sent,int totalsize) {
	double float_time   = min(1.0,(double)samps_sent/(double)totalsize);
	int display_time = round(float_time * PBAR_LENGTH);
	fprintf(stdout,"TIME: %d\n",display_time);
	fflush(stdout);
}

/****************************** LOCAL_PRINT_MESSAGES_AND_CLOSE_SNDFILES ******************************/

int	local_print_messages_and_close_sndfiles(int exit_status,int is_launched,int ofd,int *ifd,int infilecnt)
{
	int n;							/* OUTPUT ERROR MESSAGES */
	switch(exit_status) {
	case(FINISHED):			break;
	case(PROGRAM_ERROR):	
		fprintf(stdout,"ERROR: INTERNAL ERROR: (Bug?)\n");
		local_splice_multiline_string(errstr,"ERROR:");
		break;
	case(SYSTEM_ERROR):		
		fprintf(stdout,"ERROR: SYSTEM ERROR\n");
		local_splice_multiline_string(errstr,"ERROR:");
		break;
	case(MEMORY_ERROR):		
		fprintf(stdout,"ERROR: MEMORY ERROR\n");
		local_splice_multiline_string(errstr,"ERROR:");
		break;
	case(USER_ERROR):		
		fprintf(stdout,"ERROR: DATA OR RANGE ERROR\n");
		local_splice_multiline_string(errstr,"ERROR:");
		break;
	case(DATA_ERROR):		
		fprintf(stdout,"ERROR: INVALID DATA\n");
		local_splice_multiline_string(errstr,"ERROR:");
		break;
	case(GOAL_FAILED):		
		fprintf(stdout,"ERROR: CANNOT ACHIEVE TASK:\n");
		local_splice_multiline_string(errstr,"ERROR:");
		break;
	case(USAGE_ONLY):
		fprintf(stdout,"ERROR: PROGRAM ERROR: usage messages should not be called.\n");
		fflush(stdout);
		break;
	case(TK_USAGE):			
		exit_status = FINISHED;
		break;
	default:
		fprintf(stdout,"ERROR: INTERNAL ERROR: (Bug?)\n");
		fprintf(stdout,"ERROR: Unknown case in print_messages_and_close_sndfiles)\n");
		exit_status = PROGRAM_ERROR;
		break;
	}
	if(ofd >= 0 && (exit_status!=FINISHED || !is_launched) && sndunlink(ofd) < 0)
		fprintf(stdout, "ERROR: Can't set output soundfile for deletion.\n");
//	if((ofd >= 0) && dz->needpeaks){
//		if(sndputpeaks(dz->ofd,dz->outchans,dz->outpeaks)) {
//			fprintf(stdout,"WARNING: failed to write PEAK data\n");
//			fflush(stdout);
//		}
//	}
	if(ofd >= 0  && sndcloseEx(ofd) < 0) {
		fprintf(stdout, "WARNING: Can't close output sf-soundfile : %s\n",sferrstr());
		fflush(stdout);
	}
	if(ifd != NULL && infilecnt >= 0) {
		for(n=0;n<infilecnt;n++) {
		  /* ALL OTHER CASES */
			if(ifd[n] >= 0 && sndcloseEx(ifd[n]) < 0)
				fprintf(stdout, "WARNING: Can't close input sf-soundfile %d.\n",n+1);
		}
	}
	sffinish();
	fprintf(stdout,"END:");
	fflush(stdout);
	if(exit_status != FINISHED) {
		return(FAILED);
	}
	return(SUCCEEDED);
}

int local_readhead(infileptr inputfile,int ifd,char *filename,int getmax,int needmaxinfo)
{
	SFPROPS props = {0};
	int isenv = 0;
	int os;
	
	if(!snd_headread(ifd,&props)) {
		fprintf(stdout,"INFO: Failure to read properties, in %s\n",filename);
		fflush(stdout);
		return(DATA_ERROR);
	}
	inputfile->srate = props.srate;
	inputfile->channels = props.chans;

	switch(props.samptype) {
	case(SHORT16):	inputfile->stype = SAMP_SHORT;	break;
	case(FLOAT32):	inputfile->stype = SAMP_FLOAT;	break;
	case(SHORT8):	inputfile->stype = SAMP_BYTE;	break;
	default:	/* remaining symbols have same int value */	break;
	}
//TW TEMPORARY SUBSTITUTION
	inputfile->filetype = SNDFILE;
	if(sndgetprop(ifd,"is an envelope",(char *) &isenv,sizeof(int)) >= 0)
		inputfile->filetype = ENVFILE;
	else if(sndgetprop(ifd,"is a formant file",(char *) &isenv,sizeof(int)) >= 0)
		inputfile->filetype = FORMANTFILE;
	else if(sndgetprop(ifd,"is a pitch file",(char *) &isenv,sizeof(int)) >= 0)
		inputfile->filetype = PITCHFILE;
	else if(sndgetprop(ifd,"is a transpos file",(char *) &isenv,sizeof(int)) >= 0)
		inputfile->filetype = TRANSPOSFILE;
	else if(sndgetprop(ifd,"original sampsize",(char *) &os,sizeof(int)) > 0)
		inputfile->filetype = ANALFILE;

	inputfile->specenvcnt = props.specenvcnt;
	if(inputfile->channels != 1) {
		sprintf(errstr,"Channel count not equal to 1 in %s: readhead()\n"
		"Implies failure to write correct header in another program.\n",
		filename);
		return(PROGRAM_ERROR);
	}
	if(sndgetprop(ifd,"orig channels",(char *)&(inputfile->origchans),sizeof(int)) < 0) {
		fprintf(stdout,"Cannot read original channels count: %s\n",filename);
		fflush(stdout);
		return(DATA_ERROR);
	}
	if(sndgetprop(ifd,"original sampsize",(char *)&(inputfile->origstype),sizeof(int)) < 0) {
		fprintf(stdout,"Cannot read original sample type: %s\n",filename);
		fflush(stdout);
		return(DATA_ERROR);
	}
	if(sndgetprop(ifd,"original sample rate",(char *)&(inputfile->origrate),sizeof(int)) < 0) {
		fprintf(stdout,"Cannot read original sample rate: %s\n",filename);
		fflush(stdout);
		return(DATA_ERROR);
	}
	if(sndgetprop(ifd,"arate",(char *)&(inputfile->arate),sizeof(float)) < 0) {
		fprintf(stdout,"Cannot read analysis rate: %s\n",filename);
		fflush(stdout);
		return(DATA_ERROR);
	}
	if(sndgetprop(ifd,"analwinlen",(char *)&(inputfile->Mlen),sizeof(int)) < 0) {
		fprintf(stdout,"Cannot read analysis window length: %s\n",filename);
		fflush(stdout);
		return(DATA_ERROR);
	}
	if(sndgetprop(ifd,"decfactor",(char *)&(inputfile->Dfac),sizeof(int)) < 0) {
		fprintf(stdout,"Cannot read original decimation factor: %s\n",filename);
		fflush(stdout);
		return(DATA_ERROR);
	}
	return(FINISHED);
}

/****************************** LOCAL_SPLICE_MULTILINE_STRING ******************************/

void local_splice_multiline_string(char *str,char *prefix)
{
	char *p, *q, c;
	p = str;
	q = str;
	while(*q != ENDOFSTR) {
		while(*p != '\n' && *p != ENDOFSTR)
			p++;
		c = *p;
		*p = ENDOFSTR;
		fprintf(stdout,"%s %s\n",prefix,q);
		*p = c;
		if(*p == '\n')
			 p++;
		while(*p == '\n') {
			fprintf(stdout,"%s \n",prefix);
			p++;
		}
		q = p;
		p++;
	}
}

/**************************** FLTEQ *******************************/

int flteq(double f1,double f2)
{
	double upperbnd, lowerbnd;
	upperbnd = f2 + FLTERR;		
	lowerbnd = f2 - FLTERR;		
	if((f1>upperbnd) || (f1<lowerbnd))
		return(FALSE);
	return(TRUE);
}

/***************************** FORMANT_HEADWRITE *******************************/

int formant_headwrite(int ofd,infileptr ifp)
{	
	int  property_marker = 1;

    if(sndputprop(ofd,"specenvcnt", (char *)&(ifp->specenvcnt), sizeof(int)) < 0){
		sprintf(errstr,"Failure to write specenvcnt: headwrite()\n");
		return(PROGRAM_ERROR);
	}
	if(sndputprop(ofd,"orig channels", (char *)&(ifp->origchans), sizeof(int)) < 0){
		sprintf(errstr,"Failure to write original channel data: headwrite()\n");
		return(PROGRAM_ERROR);
	}
	if(sndputprop(ofd,"original sampsize", (char *)&(ifp->origstype), sizeof(int)) < 0){
		sprintf(errstr,"Failure to write original sample size. headwrite()\n");
		return(PROGRAM_ERROR);
	}
	if(sndputprop(ofd,"original sample rate", (char *)&(ifp->origrate), sizeof(int)) < 0){
		sprintf(errstr,"Failure to write original sample rate. headwrite()\n");
		return(PROGRAM_ERROR);
	}
	if(sndputprop(ofd,"arate",(char *)&(ifp->arate),sizeof(float)) < 0){
		sprintf(errstr,"Failed to write analysis sample rate. headwrite()\n");
		return(PROGRAM_ERROR);
	}
	if(sndputprop(ofd,"analwinlen",(char *)&(ifp->Mlen),sizeof(int)) < 0){
		sprintf(errstr,"Failure to write analysis window length. headwrite()\n");
		return(PROGRAM_ERROR);
	}
	if(sndputprop(ofd,"decfactor",(char *)&(ifp->Dfac),sizeof(int)) < 0){
		sprintf(errstr,"Failure to write decimation factor. headwrite()\n");
		return(PROGRAM_ERROR);
	}
	if(sndputprop(ofd,"is a formant file", (char *)&property_marker, sizeof(int)) < 0){
		sprintf(errstr,"Failure to write formant property: headwrite()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

