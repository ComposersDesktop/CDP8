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
/*
cmdline...
sfedit twixt mode infile(s) outfile switch-times splicelen (segcnt) [-wweight] [-r]\n\n"

SWITCH_TIMES	 = special
SPLICETIME in MS = IS_SPLEN
SWITCH_WEIGHT	 = IS_WEIGHT
FLAG	IS_NOTCYCL


TO SPLICE BETWEEN A SERIES OF INFILES USING A LIST OF GIVEN TIMES

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <structures.h>
#include <cdpmain.h>
#include <tkglobals.h>
#include <pnames.h>
#include <edit.h>
#include <processno.h>
#include <modeno.h>
#include <globcon.h>
#include <logic.h>
#include <filetype.h>
#include <mixxcon.h>
#include <flags.h>
#include <speccon.h>
#include <arrays.h>
#include <special.h>
#include <formants.h>
#include <sfsys.h>
#include <osbind.h>

#if defined unix || defined __GNUC__ 
#define round(x) lround((x))
#endif

static void GetTimes(int fileno,int n,double *lasttime, double *nexttime,dataptr dz);
static int  do_output(int fileno,int n,int limit,double lasttime,double nexttime,
				double splicestep,int *obufend,int *total_outsamps,dataptr dz);
static int Convert_to_samp(double time,dataptr dz);
static void do_startsplice_in_outbuf(float *buf,double splicestep,dataptr dz);
static void do_startof_endsplice_in_outbuf(float *buf,int splicetodo,double splicestep,dataptr dz);
static int 	output_samps(int segno,int firsttime,float *outbuf,int outsamps,int *obufend,dataptr dz);
static int 	flush_outbuf(int segno,int obufend,dataptr dz);
static void randperm(int z,int setlen,dataptr dz);
static void hprefix(int z,int m,int setlen,dataptr dz);
static void hinsert(int z,int m,int t,int setlen,dataptr dz);
static void hshuflup(int z,int k,int setlen,dataptr dz);
static int 	output_cut_sndfile(dataptr dz);
static int 	setup_new_outfile(int n,dataptr dz);
static void	generate_splicelen(dataptr dz);
static int 	Getnextfile(int n,int permcnt,dataptr dz);
static void do_endsplice_in_outbuf(float *buf,double splicestep,dataptr dz);

//TW replaced by global F_SECSIZE
//#define FSECSIZE (256)		   /*RWD*/

/********************************* DO_INTERSPLICE *************************************
 *
 * THE ALGORITHM
 * 
 * A TWO-STAGE PROCESS:
 * 1) We read from an infile into a sector-aligned buffer
 * 2) We read from a point in inbuffer (not ness coinciding with its start(which is a sector boundary))
 * 	     putting in splice at start of read (and, eventually, splice at end of all reads from this infile)
 * 3) WE copy each read to an outbuffer
 * 4) Except on very first use, outbuffer always retains a SPLICELEN-segment of data from the previous read
 * 		ready for splice to be made to next file copied into it.
 * 5) At start of this copy-to-outbuf we ADD the upsplice-part of new file to end downsplice of previous file
 * 		before proceeding to straightforward copying
 */

int do_twixt(dataptr dz)
{
	int exit_status, n = 0, limit, fileno;
	double lasttime=0.0, nexttime=0.0, splicestep;
	int obufend = 0, permcnt = 0, outsamps;

	splicestep = 1.0/(SPLICELEN/dz->infile->channels);

	if(dz->process != TWIXT || dz->mode != TRUE_EDIT)
		permcnt = dz->infilecnt + dz->iparam[IS_WEIGHT] - 1;

	if(dz->process == TWIXT && dz->mode == TRUE_EDIT)
		limit = dz->itemcnt - 1; 
	else {
		switch(dz->mode) {
		case(IN_SEQUENCE): 	
			limit = dz->itemcnt - 1;
			break;
		default:			
			limit = dz->iparam[IS_SEGCNT]; 
			break;	
		}
	}
	dz->tempsize = (int)limit;

	while(n<limit) {
		fileno = Getnextfile(n,permcnt,dz);
		GetTimes(fileno,n,&lasttime,&nexttime,dz);
		if((exit_status = do_output(fileno,n,limit,lasttime,nexttime,splicestep,&obufend,&outsamps,dz))<0)
			return(exit_status);
		n++;
	}
	if((exit_status = flush_outbuf(n-1,obufend,dz))<0)
		return exit_status;
	if(dz->process == TWIXT && dz->mode == TRUE_EDIT) {
		if((exit_status = output_cut_sndfile(dz))<0)
			return exit_status;
	}
	return (FINISHED);
}

/********************************* GETTIMES *************************************/

void GetTimes(int arrayno,int n,double *lasttime, double *nexttime,dataptr dz)
{
	int k, j, lastperm;
	static int init = 1;
	int span = dz->itemcnt-1;

	if(dz->process == TWIXT)	/* only one aray of times, for TWIXT */
		arrayno = 0;
	if(dz->process == TWIXT && dz->mode == TRUE_EDIT) {
		*lasttime = dz->parray[arrayno][n++];
		*nexttime = dz->parray[arrayno][n];
	} else {
		switch(dz->mode) {
		case(IN_SEQUENCE):
			*lasttime = dz->parray[arrayno][n++];
			*nexttime = dz->parray[arrayno][n];
			break;
		case(RAND_REORDER):
			n %= span;
			if(n == 0) {
				if(init) {
					randperm(0,span,dz);
					init = 0;
				} else {
//TW UPDATE
//					lastperm = dz->iparray[arrayno][span-1];
					lastperm = dz->iparray[0][span-1];
					randperm(0,span,dz);
//TW UPDATE
//					if(lastperm == dz->iparray[arrayno][0]) {
//						for(j=0;j < span; j++) {
//							if(dz->iparray[arrayno][j] != lastperm) {
//								dz->iparray[arrayno][0] = dz->iparray[arrayno][j];
//								dz->iparray[arrayno][j] = lastperm;
					if(lastperm == dz->iparray[0][0]) {
						for(j=0;j < span; j++) {
							if(dz->iparray[0][j] != lastperm) {
								dz->iparray[0][0] = dz->iparray[0][j];
								dz->iparray[0][j] = lastperm;
								break;
							}
						}
					}
				}
			}
//TW UPDATE
//			k = dz->iparray[arrayno][n];
			k = dz->iparray[0][n];
			*lasttime = dz->parray[arrayno][k++];
			*nexttime = dz->parray[arrayno][k];
			break;
		case(RAND_SEQUENCE):
			k = (int)floor(drand48() * span);
			*lasttime = dz->parray[arrayno][k++];
			*nexttime = dz->parray[arrayno][k];
			break;
		}
	}
}

/********************************* DO_OUTPUT *************************************/
/*RWD 6:2001: NB permcnt,n unused */
int do_output(int fileno,int n,int limit,double lasttime,double nexttime,double splicestep,
			int *obufend,int *total_outsamps,dataptr dz)
{
	int exit_status;
	static int init;
	int lastsamp = Convert_to_samp(lasttime,dz);
	int nextsamp = Convert_to_samp(nexttime,dz);
	int firsttime = 1;
	int remaining_samps_to_write, startbuf, offset, rotate, max_possible_outsamps, outsamps, startsplice;
	float *inbuf, *outbuf;

	if(nextsamp - lastsamp < SPLICELEN)	/* Possible for final segment */
		return(FINISHED);
	lastsamp = max(0,lastsamp - SPLICELEN);	/* adjust so upsplice starts before start of segment */
	remaining_samps_to_write = nextsamp - lastsamp;
	*total_outsamps = remaining_samps_to_write;
	startbuf = (lastsamp/dz->iparam[IS_SHSECSIZE]) * dz->iparam[IS_SHSECSIZE];
	offset = lastsamp - startbuf;
	rotate = dz->iparam[IS_SHSECSIZE] - offset;

/*
 *	x = offset, start of outbuf
 *	y = SECTOR-minus-x left at end
 *
 *	 	 b_________________________________c
 *	  x |								    |y
 *	|_____________INBUF positioned ___________|
 *	        so y can be copied to its start
 *
 *	1st time, output from b to c = sampbuflen - dz->iparam[IS_SHSECSIZE]
 *	then copy 'y' to before start of inbuf
 *	Other reads, output from a to c. = sampbuflen
 *	then copy 'y' to before start of inbuf
 *
 *  a_____b_________________________________c
 *|y|  x  |
 *|_____sampbuf = standardsize + SECTOR_________|
 *
 */

	inbuf = dz->sampbuf[0] + (dz->iparam[IS_SHSECSIZE] - offset);			
	outbuf = inbuf + offset;
	max_possible_outsamps = dz->buflen - dz->iparam[IS_SHSECSIZE];
	sndseekEx(dz->ifd[fileno],startbuf,0);
	dz->total_samps_read = startbuf;
	do {
		if(n!=0 && (dz->process == TWIXT) && (dz->mode == TRUE_EDIT) && firsttime) {
			if((exit_status = setup_new_outfile(n,dz))<0)
				return(exit_status);
		}
	 	if((dz->ssampsread = fgetfbufEx(inbuf, dz->buflen,dz->ifd[fileno],0)) < 0) {
			sprintf(errstr,"Can't read samples from input soundfile.\n");
			return(SYSTEM_ERROR);
		}
		if(dz->ssampsread == 0)
			break;
 		if(firsttime && !init)
			do_startsplice_in_outbuf(outbuf,splicestep,dz);
		else
			init = 0;
		if(max_possible_outsamps >= remaining_samps_to_write) {
			outsamps = remaining_samps_to_write;
			do_endsplice_in_outbuf(outbuf + outsamps - SPLICELEN,splicestep,dz);
		} else {
			outsamps = max_possible_outsamps;
			 if ((startsplice = remaining_samps_to_write - SPLICELEN) < outsamps)
				do_startof_endsplice_in_outbuf(outbuf + startsplice,outsamps - startsplice,splicestep,dz);
		}
		output_samps(n,firsttime,outbuf,outsamps,obufend,dz);
		remaining_samps_to_write -= outsamps;
		memcpy((char *)dz->sampbuf[0],(char *)(outbuf + outsamps),rotate * sizeof(float));
		outbuf = dz->sampbuf[0];					/* after first output */
		max_possible_outsamps = dz->buflen;
		firsttime = 0;
	} while(remaining_samps_to_write > 0);

	if((dz->process == TWIXT) && (dz->mode == TRUE_EDIT) && (n != limit - 1)) {
		if((exit_status = flush_outbuf(n,*obufend,dz))<0)
			return exit_status;
		*obufend = 0;
		return output_cut_sndfile(dz);
	}
	return(FINISHED);
}

/********************************* CONVERT_TO_SAMP *************************************/

int Convert_to_samp(double time,dataptr dz)
{
	int sampcnt = (int)round(time * dz->infile->srate);
	sampcnt *= dz->infile->channels;
	return(sampcnt);
}

/********************************* DO_STARTSPLICE_IN_OUTBUF *************************************/

void do_startsplice_in_outbuf(float *buf,double splicestep,dataptr dz)
{
	int chans = dz->infile->channels;
	double q, mult = 0.0;
	int k, n = 0;
	while(n < SPLICELEN) {
		mult += splicestep;
		for(k = 0; k < chans; k++) {
			q = *buf * mult;
			*buf++ = (float) /*round*/(q);
			n++;
		}
	}
}

/********************************* DO_ENDSPLICE_IN_OUTBUF *************************************/

void do_endsplice_in_outbuf(float *buf,double splicestep,dataptr dz)
{
	int chans = dz->infile->channels;
	double q, mult = 1.0;
	int k, n = 0;
	while(n < SPLICELEN) {
		mult -= splicestep;
		for(k = 0; k < chans; k++) {
			q = *buf * mult;
			*buf++ = (float) /*round*/(q);
			n++;
		}
	}
}

/********************************* DO_STARTOF_ENDSPLICE_IN_OUTBUF *************************************/

void do_startof_endsplice_in_outbuf(float *buf,int splicetodo,double splicestep,dataptr dz)
{
	int chans = dz->infile->channels;
	double q, mult = 1.0;
	int k, n = 0;
	while(n < splicetodo) {
		mult -= splicestep;
		for(k = 0; k < chans; k++) {
			q = *buf * mult;
			*buf++ = (float) /*round*/(q);
			n++;
		}
	}
}		

/********************************* OUTPUT_SAMPS *************************************/

int output_samps(int segno,int firsttime,float *outbuf,int outsamps,int *obufend,dataptr dz)
{
	/* true length of buf = buflen + SPLICELEN */
	int exit_status;
	int n, samps_to_copy, xs;
	int samps_to_write = dz->buflen ;
	int space_in_outbuf = dz->buflen + SPLICELEN - *obufend;
	float *thisbuf = dz->sampbuf[1] + *obufend;
	static int ob_firsttime = 1;

	if(ob_firsttime) {
		memcpy((char *)thisbuf,(char *)outbuf,outsamps * sizeof(float));
		*obufend = outsamps;
		ob_firsttime = 0;
	} else {
		if(firsttime && (dz->process != TWIXT || dz->mode != TRUE_EDIT)) {
			thisbuf -= SPLICELEN;
			for(n=0; n < SPLICELEN; n++)
				thisbuf[n] = (float)(thisbuf[n] + outbuf[n]);
			thisbuf += SPLICELEN;
			outbuf  += SPLICELEN;
			samps_to_copy = outsamps - SPLICELEN;
		} else
			samps_to_copy = outsamps;
		if((xs = samps_to_copy - space_in_outbuf) < 0) {
			memcpy((char *)thisbuf,(char *)outbuf,samps_to_copy * sizeof(float));
			*obufend += samps_to_copy;
		} else {
			memcpy((char *)thisbuf,(char *)outbuf,space_in_outbuf * sizeof(float));
			samps_to_write = dz->buflen;
			superzargo = segno + 1;
			if((exit_status = write_samps(dz->sampbuf[1],samps_to_write,dz))<0)
				return(exit_status);
			thisbuf = dz->sampbuf[1];
			memcpy((char *)thisbuf,(char *)(thisbuf + dz->buflen),SPLICELEN * sizeof(float));
			memset((char *)(thisbuf+SPLICELEN),0,samps_to_write);
			*obufend = SPLICELEN;
			if(xs > 0) {
				thisbuf += *obufend; 
				outbuf += space_in_outbuf;
				memcpy((char *)thisbuf,(char *)outbuf,xs * sizeof(float));
				*obufend += xs;
			}
		}
	}
	return(FINISHED);
}	
		
/********************************* FLUSH_OUTBUF *************************************/

int flush_outbuf(int segno,int obufend,dataptr dz)
{
	int exit_status;
	int samps_to_write = obufend;
	superzargo = segno+1;
	if(samps_to_write > 0) {
		if((exit_status = write_samps(dz->sampbuf[1],samps_to_write,dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}

/********************************* RANDPERM *************************************/

void randperm(int z,int setlen,dataptr dz)
{
	int n, t;
	for(n=0;n<setlen;n++) {
		t = (int)floor(drand48() * (n+1));
		if(t>=n)
			hprefix(z,n,setlen,dz);
		else
			hinsert(z,n,t,setlen,dz);
	}
}

/***************************** HINSERT **********************************
 *
 * Insert the value m AFTER the T-th element in iparray[].
 */

void hinsert(int z,int m,int t,int setlen,dataptr dz)
{   
	hshuflup(z,t+1,setlen,dz);
	dz->iparray[z][t+1] = m;
}

/***************************** HPREFIX ************************************
 *
 * Insert the value m at start of the permutation iparray[].
 */

void hprefix(int z,int m,int setlen,dataptr dz)
{
	hshuflup(z,0,setlen,dz);
	dz->iparray[z][0] = m;
}

/****************************** HSHUFLUP ***********************************
 *
 * move set members in iparray[] upwards, starting from element k.
 */

void hshuflup(int z,int k,int setlen,dataptr dz)
{   
	int n, *i;
	int y = setlen - 1;
	i = (dz->iparray[z]+y);
	for(n = y;n > k;n--) {
		*i = *(i-1);
		i--;
	}
}

/********************************** OUTPUT_CUT_SNDFILE ********************************/

int output_cut_sndfile(dataptr dz)
{
	int exit_status;
	char filename[64];
	if((exit_status = headwrite(dz->ofd,dz))<0)
		return(exit_status);
	dz->process_type = OTHER_PROCESS;		/* stop process trying to deal with outfile later */
	dz->outfiletype  = NO_OUTPUTFILE;		/* stop process trying to deal with outfile later */
	if(sndcloseEx(dz->ofd) < 0) {
		fprintf(stdout,"WARNING: Can't close output soundfile %s\n",filename);
		fflush(stdout);
	}
	dz->ofd = -1;
	return(FINISHED);
}

/********************************** SETUP_NEW_OUTFILE ********************************/

int setup_new_outfile(int n,dataptr dz)
{
	int exit_status;
	char filename[64];
	strcpy(filename,dz->wordstor[0]);
	insert_new_number_at_filename_end(filename,n,1);

	dz->process_type = UNEQUAL_SNDFILE;	/* allow sndfile to be created */
	dz->outfiletype  = SNDFILE_OUT;		/* allows header to be written  */
	if((exit_status = create_sized_outfile(filename,dz))<0) {
		fprintf(stdout, "WARNING: Cannot create Soundfile %s : %s\n", filename, rsferrstr);
		fflush(stdout);
		dz->process_type = OTHER_PROCESS;
		dz->outfiletype  = NO_OUTPUTFILE;		/* stop process trying to deal with outfile later */
		dz->ofd = -1;
		return(GOAL_FAILED);
	}
	return(FINISHED);
}

/*************************** CREATE_TWIXT_BUFFERS **************************/

int create_twixt_buffers(dataptr dz)
{
	int bigsize;
	size_t bigbufsize;
	int framesize = F_SECSIZE * dz->infile->channels;
	if(dz->sbufptr == 0 || dz->sampbuf==0) {
		sprintf(errstr,"buffer pointers not allocated: create_sndbufs()\n");
		return(PROGRAM_ERROR);
	}
	bigbufsize = (size_t) Malloc(-1);
	bigbufsize -= (SPLICELEN + F_SECSIZE) * sizeof(float);
	bigbufsize /= dz->bufcnt;
	dz->buflen = (int)(bigbufsize/sizeof(float));
	dz->buflen  =  (dz->buflen / framesize)	* framesize;
	if(dz->buflen <= 0)
		dz->buflen = framesize;

	bigsize = (dz->buflen * dz->bufcnt) + SPLICELEN + F_SECSIZE;
	if((dz->bigbuf = (float *)malloc(bigsize * sizeof(float))) == NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
		return(PROGRAM_ERROR);
	}
	dz->sbufptr[0] = dz->sampbuf[0] = dz->bigbuf;
	dz->sbufptr[1] = dz->sampbuf[1] = dz->sampbuf[0] + dz->buflen + dz->iparam[IS_SHSECSIZE];
	dz->sampbuf[2] = dz->sampbuf[1] + dz->buflen + SPLICELEN;
	memset((char *)dz->bigbuf,0,bigsize * sizeof(float));
	return(FINISHED);
}

/*************************** TWIXT_PREPROCESS **************************/

int twixt_preprocess(dataptr dz)		
{
	double *p, last, next = 0.0;
	int n;
	int minsamps = dz->insams[0];
	for(n = 1; n<dz->infilecnt;n++)
		minsamps = min(minsamps,dz->insams[n]);
	dz->duration = (double)(minsamps/dz->infile->channels)/(double)dz->infile->srate;
	if(dz->mode != TRUE_EDIT) {
		if((dz->iparray[1] = (int *)malloc((dz->infilecnt + dz->iparam[IS_WEIGHT] - 1) * sizeof(int)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to allocate file permutation store.\n");
			return(MEMORY_ERROR);
		}
	}
	generate_splicelen(dz);
	p = dz->parray[0];
	if((last = *p++) > dz->duration) {
		sprintf(errstr,"First time in datafile is beyond end of shortest input soundfile (%lf)\n",dz->duration);
		return(DATA_ERROR);
	}
	n = 2;
	while(n <= dz->itemcnt) {
		if((next = *p++) > dz->duration) {
			sprintf(errstr,"Times beyond number %d ( = %lf) in datafile are beyond end of shortest input soundfile (%lf)\n",
			n-1,last,dz->duration);
			return(DATA_ERROR);
		}
		if(next - last <= dz->param[IS_SPLICETIME]) {
			sprintf(errstr,"distant between splicepoints %d & %d (%lf) less than time needed for splices (%lf)\n",
				n,n-1,next-last,dz->param[IS_SPLICETIME]);
			return(DATA_ERROR);
		}
		last = next;
		n++;
	}
	if(dz->duration - next > dz->param[IS_SPLICETIME])  {
		if((dz->parray[0] = (double *)realloc((char *)dz->parray[0],(dz->itemcnt+1) * sizeof(double)))==NULL) {
			sprintf(errstr,"Insufficient memory to store time-switch data.\n");
			return(MEMORY_ERROR);
		}
		dz->parray[0][dz->itemcnt] = dz->duration;
		dz->itemcnt++;
	}
	return(FINISHED);
}

/*************************** SPHINX_PREPROCESS **************************/

int sphinx_preprocess(dataptr dz)		
{
	double *p, last, next;
	int n, k;
	if((dz->iparray[1] = (int *)malloc((dz->infilecnt + dz->iparam[IS_WEIGHT] - 1) * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to allocate file permutation store.\n");
		return(MEMORY_ERROR);
	}
	generate_splicelen(dz);
	for(k=0;k<dz->array_cnt;k++) {
		p = dz->parray[k];
		last = *p++;
		n = 2;
		while(n <= dz->itemcnt) {
			next = *p++;
			if(next - last <= dz->param[IS_SPLICETIME]) {
				sprintf(errstr,"distant between splicepoints %d & %d (%lf) less than time needed for splices (%lf)\n",
					n,n-1,next-last,dz->param[IS_SPLICETIME]);
				return(DATA_ERROR);
			}
			last = next;
			n++;
		}
	}
	return(FINISHED);
}

/*************************** GENERATE_SPLICELEN **************************/

void generate_splicelen(dataptr dz)
{
	double splicetime;
	int j;

	dz->iparam[IS_SHSECSIZE] = F_SECSIZE;
	splicetime = dz->param[IS_SPLEN] * MS_TO_SECS;
	j = round(splicetime * dz->infile->srate);
	j *= dz->infile->channels;
	SPLICELEN = j;
	dz->param[IS_SPLICETIME] = (double)(SPLICELEN/dz->infile->channels)/(double)dz->infile->srate;
}

/*************************** GETNEXTFILE **************************/

int Getnextfile(int n,int permcnt,dataptr dz)
{
	int lastperm, j, k;
	static int init = 1;
	if(dz->process == TWIXT && dz->mode == TRUE_EDIT)
		return 0;
	n %= permcnt;
	if((permcnt == dz->infilecnt) && !dz->vflag[IS_NOTCYCL])	/* with no weighting, and cyclic files specified */
		return n;																				
	if(n == 0) {
		if(init) {
			randperm(1,permcnt,dz);
			for(j=0;j < permcnt;j++) {
				if(dz->iparray[1][j] >= dz->infilecnt)	/* substitute file1 for extra spaces in perm */
					dz->iparray[1][j] = 0;
			}
			if(dz->iparray[1][0] != 0) {
				for(j=1;j < permcnt;j++) {	 			/* Force first file to be first !! */
					if(dz->iparray[1][j] == 0) {
						dz->iparray[1][j] = dz->iparray[1][0];
						dz->iparray[1][0] = 0;
						break;
					}
				}
			}
			if(!dz->vflag[IS_NOTCYCL]) {
				k = 1;
				for(j=1;j < permcnt;j++) {	 		/* Force un-first files to be in order */
					if(dz->iparray[1][j] != 0)
						dz->iparray[1][j] = k++;
				}
			}
			init = 0;
		} else {
			lastperm = dz->iparray[1][permcnt - 1];
			randperm(1,permcnt,dz);
			for(j=0;j < permcnt;j++) {				/* substitute file1 for extra spaces in perm */
				if(dz->iparray[1][j] >= dz->infilecnt)
					dz->iparray[1][j] = 0;
			}										/* avoid repets at perm boundaries */
			if((dz->iparray[1][0] == lastperm) && ((lastperm != 0) || (dz->iparam[IS_WEIGHT] == 1))) {
				for(j=1;j < permcnt;j++) {
					if(dz->iparray[1][j] != lastperm) {
						dz->iparray[1][0] = dz->iparray[1][j];
						dz->iparray[1][j] = lastperm;
						break;
					}
				}
			}
			if(!dz->vflag[IS_NOTCYCL]) {
				k = 1;
				for(j=1;j < permcnt;j++) {	 		/* Force un-first files to be in order */
					if(dz->iparray[1][j] != 0)
						dz->iparray[1][j] = k++;
				}
			}
		}
	}
	return dz->iparray[1][n];
}
