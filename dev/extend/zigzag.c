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
#include <extend.h>
#include <cdpmain.h>

#include <sfsys.h>

#include <extend.h>

int 	reverse_it(int incnt,dataptr dz);
void 	do_down_splice(dataptr dz);
int 	add_to_splicebuf(float *iptr,dataptr dz);
int 	find_zzchunk(int **thisstart,int **lastend, int *ziglistend, int *minsamp, dataptr dz);
int 	adjust_buffer(int minsamp, int *oldminsec, int init, dataptr dz);
int 	zig_or_zag(int direction,int *here,int *next,int is_startsplice,int is_endsplice,
				int *outbuf_space,int obufno,int splbufno, dataptr dz);
int 	do_zigzags(int *thisstart,int *lastend,int *outbuf_space,int obufno,int splbufno,dataptr dz);
int 	memcpy_with_check(char *bufptr,char *bufend,char *fromptr,int bytecnt);
int 	setup_splices(int *base,int *last,int *here,int *next,int *after,int *end,
				int *is_startsplice,int *is_endsplice,dataptr dz);
static void do_end_splice(int samps_left,int obufno,dataptr dz);

#define	ZIG		(1)
#define UNKNOWN	(0)
#define ZAG		(-1)

#define NORMAL	(0)
#define REVERSE	(1)

#define SECMARGIN  (256) 


/***************************** ZIGZAG **********************************/

int zigzag(dataptr dz)
{
	int exit_status;
	int *thisstart, *lastend = dz->lparray[ZIGZAG_TIMES], minsamp;
	int oldminsec = 0;
	int init = 1;
    int *ziglistend = dz->lparray[ZIGZAG_TIMES] + dz->itemcnt;
	int outbuf_space = dz->buflen, samps_left;
	int obufno, splbufno;
	switch(dz->process) {
	case(ZIGZAG): 	obufno = 2;	splbufno = 3; break;
	case(LOOP):
	case(SCRAMBLE): obufno = 1;	splbufno = 2; break;
	default:
		sprintf(errstr,"Unknown case in	zigzag()\n");
		return(PROGRAM_ERROR);
	}
	if((exit_status = find_zzchunk(&thisstart,&lastend,ziglistend,&minsamp,dz))!=CONTINUE) {
		if(exit_status == FINISHED)
			exit_status = GOAL_FAILED;
		switch(dz->process) {
		case(ZIGZAG): 	sprintf(errstr,"WARNING: No valid zigzag found\n"); 	break;
		case(LOOP):   	sprintf(errstr,"WARNING: No valid loop found\n"); 	 	break;
		case(SCRAMBLE): sprintf(errstr,"WARNING: No valid scramble found\n"); 	break;
		}
		return(exit_status);
	}
	if((exit_status = adjust_buffer(minsamp,&oldminsec,init,dz))<0)
		return(exit_status);
	init = 0;
	while(lastend < ziglistend) {
		if((exit_status = do_zigzags(thisstart,lastend,&outbuf_space,obufno,splbufno,dz))<0)
			return(exit_status);
		if((exit_status = find_zzchunk(&thisstart,&lastend,ziglistend,&minsamp,dz))!=CONTINUE) {
			if(exit_status==FINISHED)
				break;
			else
				return(exit_status);
		}
		if((exit_status = adjust_buffer(minsamp,&oldminsec,init,dz))<0)
			return(exit_status);
	}
	samps_left = dz->sbufptr[obufno] - dz->sampbuf[obufno];
	do_end_splice(samps_left,obufno,dz);
	if(samps_left > 0)
		return write_samps(dz->sampbuf[obufno],samps_left,dz);
	return FINISHED;
}

/************************* DO_ZIGZAGS ************************************/

int do_zigzags(int *thisstart,int *lastend,int *outbuf_space,int obufno,int splbufno,dataptr dz)
{
	int exit_status;
	int direction;
	int is_startsplice;
	int is_endsplice;
	int *last  = thisstart - 1;
	int *here  = thisstart;
	int *next  = thisstart + 1;
	int *after = thisstart + 2;
	int *base = dz->lparray[ZIGZAG_TIMES];
	int *end  = dz->lparray[ZIGZAG_TIMES] + dz->itemcnt;
	while(next <= lastend) {
		direction = setup_splices(base,last,here,next,after,end,&is_startsplice,&is_endsplice,dz);
 		switch(dz->process) {
		case(ZIGZAG):
			break;				 /* both ZIGS (forward) and ZAGS (backwards) are played */
		case(LOOP):
		case(SCRAMBLE):			 
			if(dz->iparray[ZIGZAG_PLAY][here - base]==TRUE)
				direction = ZIG; /* FLAGS A (forward) PLAYED MOVE IN INFILE */
			else
				direction = ZAG; /* FLAGS A NON-PLAYED MOVE IN INFILE: which might be forward as well as backward */
			break;
		}
		if((exit_status = zig_or_zag(direction,here,next,is_startsplice,is_endsplice,outbuf_space,obufno,splbufno,dz))<0)
			return(exit_status);
		last++;
		here++;
		next++;
		after++;
	}
	return(FINISHED);
}

/****************************** ZIG_OR_ZAG ***********************************/

int zig_or_zag
(int direction,int *here,int *next,int is_startsplice,int is_endsplice,
int *outbuf_space,int obufno, int splbufno, dataptr dz)
{
	int  exit_status;
	int  bufno;
	int incnt  = abs(*next - *here), advance;
	int orig_incnt = incnt;
	int samps_remain;
	float *sbufend = dz->sampbuf[NORMAL] + dz->buflen;

	switch(direction) {
	case(ZIG):
		bufno = NORMAL;
		break;
	case(ZAG):
		switch(dz->process) {
		case(ZIGZAG):
			if((exit_status = reverse_it(incnt,dz))<0)
				return(exit_status);
			bufno = REVERSE;
			break;
		case(LOOP):
		case(SCRAMBLE):
			incnt  = *next - *here;
			if((dz->sbufptr[NORMAL] += incnt) < dz->sampbuf[NORMAL] || dz->sbufptr[NORMAL] >= sbufend) {
				sprintf(errstr,"Error in buffer accounting: zig_or_zag()\n");
				return(PROGRAM_ERROR);
			}
			return(FINISHED);
		default:
			sprintf(errstr,"Unknown case in zig_or_zag()\n");
			return(PROGRAM_ERROR);
		}
		break;
	default:
		sprintf(errstr,"Unknown case in zig_or_zag\n");
		return(PROGRAM_ERROR);
	}
	if(is_startsplice) {
		if((exit_status = add_to_splicebuf(dz->sbufptr[bufno],dz))<0)
			return(exit_status);
	}
	if(*outbuf_space >= incnt) { 	/* ENOUGH SPACE IN OUTBUFFER To DO EVERYTHING */
		if(is_startsplice) {
			if((exit_status = memcpy_with_check
			((char *)dz->sbufptr[obufno],(char *)dz->sampbuf[splbufno],
			       (char *)dz->extrabuf[ZIGZAG_SPLBUF],dz->iparam[ZIG_SPLSAMPS]* sizeof(float)))<0)
				return(exit_status);
			dz->sbufptr[bufno]  += dz->iparam[ZIG_SPLSAMPS];
			dz->sbufptr[obufno] += dz->iparam[ZIG_SPLSAMPS];
			*outbuf_space       -= dz->iparam[ZIG_SPLSAMPS];
			advance = incnt-(2*dz->iparam[ZIG_SPLSAMPS]);
		} else
			advance = incnt - dz->iparam[ZIG_SPLSAMPS];
		if(!is_endsplice)
			advance += dz->iparam[ZIG_SPLSAMPS];
		if((exit_status = memcpy_with_check
		((char *)dz->sbufptr[obufno],(char *)dz->sampbuf[splbufno],(char *)dz->sbufptr[bufno],advance * sizeof(float)))<0)
			return(exit_status);
		dz->sbufptr[obufno] += advance;
		dz->sbufptr[bufno]  += advance;
		*outbuf_space       -= advance;
		if(is_endsplice) {
			memmove((char *)dz->extrabuf[ZIGZAG_SPLBUF],
				(char *)dz->sbufptr[bufno],dz->iparam[ZIG_SPLSAMPS]* sizeof(float));
			do_down_splice(dz);
			dz->sbufptr[bufno] += dz->iparam[ZIG_SPLSAMPS];
		}
		if(*outbuf_space <= 0) {
			if((exit_status = write_samps(dz->sampbuf[obufno],dz->buflen,dz))<0)
				return(exit_status);																	
			dz->sbufptr[obufno] = dz->sampbuf[obufno];															
			*outbuf_space = dz->buflen;
		}
	} else if(is_startsplice && (*outbuf_space <= dz->iparam[ZIG_SPLSAMPS])) {
					/* OUTBUFFER SPACE LESS THAN (START)SPLICELEN */
		samps_remain = dz->iparam[ZIG_SPLSAMPS];
		dz->extrabufptr[ZIGZAG_SPLBUF] = dz->extrabuf[ZIGZAG_SPLBUF];

		if((exit_status = memcpy_with_check
		((char *)dz->sbufptr[obufno],(char *)dz->sampbuf[splbufno],(char *)dz->extrabufptr[ZIGZAG_SPLBUF],(*outbuf_space) * sizeof(float)))<0)
			return(exit_status);
		if((exit_status = write_samps(dz->sampbuf[obufno],dz->buflen,dz))<0)
			return(exit_status);																	
		dz->extrabufptr[ZIGZAG_SPLBUF] += *outbuf_space;
		dz->sbufptr[obufno] = dz->sampbuf[obufno];															
		samps_remain  -= *outbuf_space;								
		*outbuf_space  = dz->buflen;
		if(samps_remain > 0) {
			if((exit_status = memcpy_with_check
			((char *)dz->sbufptr[obufno],(char *)dz->sampbuf[splbufno],(char *)(dz->extrabufptr[ZIGZAG_SPLBUF]),samps_remain * sizeof(float)))<0)
				return(exit_status);
			dz->sbufptr[obufno] += samps_remain;															
			*outbuf_space  -= samps_remain;									
		}
		dz->sbufptr[bufno] += dz->iparam[ZIG_SPLSAMPS];
		advance = incnt - (2 *dz->iparam[ZIG_SPLSAMPS]);
		if(!is_endsplice)
			advance += dz->iparam[ZIG_SPLSAMPS];
		if((exit_status = memcpy_with_check
		((char *)dz->sbufptr[obufno],(char *)dz->sampbuf[splbufno],(char *)dz->sbufptr[bufno],advance * sizeof(float)))<0)
			return(exit_status);
		dz->sbufptr[obufno]     += advance;
		dz->sbufptr[bufno] += advance;
		*outbuf_space 	   -= advance;	
		if(is_endsplice) {
			memmove((char *)dz->extrabuf[ZIGZAG_SPLBUF],
				 (char *)dz->sbufptr[bufno],(size_t)dz->iparam[ZIG_SPLSAMPS]* sizeof(float));
			do_down_splice(dz);
			dz->sbufptr[bufno] += dz->iparam[ZIG_SPLSAMPS];
		}
	} else if(*outbuf_space <= incnt - dz->iparam[ZIG_SPLSAMPS]) {
					/* OUTBUFFER SPACE DOESN'T REACH UP INTO ENDSPLICE */
		if(is_startsplice) {
			if((exit_status = memcpy_with_check
			((char *)dz->sbufptr[obufno],(char *)dz->sampbuf[splbufno],(char *)dz->extrabuf[ZIGZAG_SPLBUF],
			       (int)dz->iparam[ZIG_SPLSAMPS]* sizeof(float)))<0)
				return(exit_status);
			dz->sbufptr[bufno]  += dz->iparam[ZIG_SPLSAMPS];
			dz->sbufptr[obufno] += dz->iparam[ZIG_SPLSAMPS];
			advance = *outbuf_space - dz->iparam[ZIG_SPLSAMPS];
		} else 
			advance = *outbuf_space;
		if((exit_status = memcpy_with_check
		((char *)dz->sbufptr[obufno],(char *)dz->sampbuf[splbufno],(char *)dz->sbufptr[bufno],advance * sizeof(float)))<0)
			return(exit_status);
		if((exit_status = write_samps(dz->sampbuf[obufno],dz->buflen,dz))<0)
			return(exit_status);
		dz->sbufptr[bufno] += advance;
		dz->sbufptr[obufno]      = dz->sampbuf[obufno];
		incnt         -= *outbuf_space;
		*outbuf_space  = dz->buflen;
		advance = incnt - dz->iparam[ZIG_SPLSAMPS];	 
		if(!is_endsplice)
			advance += dz->iparam[ZIG_SPLSAMPS];
		if(advance > 0) {
			if((exit_status = memcpy_with_check
			((char *)dz->sbufptr[obufno],(char *)dz->sampbuf[splbufno],(char *)dz->sbufptr[bufno],advance * sizeof(float)))<0)
				return(exit_status);
			dz->sbufptr[obufno]     += advance;
			dz->sbufptr[bufno] += advance;
			*outbuf_space      -= advance;
		}
		if(is_endsplice) {
			memmove((char *)dz->extrabuf[ZIGZAG_SPLBUF],(char *)dz->sbufptr[bufno],
				(size_t)dz->iparam[ZIG_SPLSAMPS]* sizeof(float));
			do_down_splice(dz);
			dz->sbufptr[bufno] += dz->iparam[ZIG_SPLSAMPS];
		}
	} else {		/* OUTBUFFER SPACE REACHES INTO MIDDLE OF ENDSPLICE */
		if(is_startsplice) {
			if((exit_status = memcpy_with_check
			((char *)dz->sbufptr[obufno],(char *)dz->sampbuf[splbufno],
			      (char *)dz->extrabuf[ZIGZAG_SPLBUF],dz->iparam[ZIG_SPLSAMPS]* sizeof(float)))<0)
				return(exit_status);
			dz->sbufptr[bufno]  += dz->iparam[ZIG_SPLSAMPS];
			dz->sbufptr[obufno] += dz->iparam[ZIG_SPLSAMPS];
			*outbuf_space       -= dz->iparam[ZIG_SPLSAMPS];
			advance = incnt - (2*dz->iparam[ZIG_SPLSAMPS]);
		} else
			advance = incnt - dz->iparam[ZIG_SPLSAMPS];
		if(!is_endsplice) {
			advance += dz->iparam[ZIG_SPLSAMPS];
			if((exit_status = memcpy_with_check
			((char *)dz->sbufptr[obufno],(char *)dz->sampbuf[splbufno],(char *)dz->sbufptr[bufno],(*outbuf_space) * sizeof(float)))<0)
				return(exit_status);
			if((exit_status = write_samps(dz->sampbuf[obufno],dz->buflen,dz))<0)
				return(exit_status);
			dz->sbufptr[bufno] += *outbuf_space;
			dz->sbufptr[obufno] = dz->sampbuf[obufno];
			advance -= *outbuf_space;
			*outbuf_space = dz->buflen;
			if((exit_status = memcpy_with_check
			((char *)dz->sbufptr[obufno],(char *)dz->sampbuf[splbufno],(char *)dz->sbufptr[bufno],advance * sizeof(float)))<0)
				return(exit_status);
			dz->sbufptr[bufno]  += advance;
			dz->sbufptr[obufno] += advance;
			*outbuf_space       -= advance;
		} else {
			if((exit_status = memcpy_with_check
			((char *)dz->sbufptr[obufno],(char *)dz->sampbuf[splbufno],(char *)dz->sbufptr[bufno],advance * sizeof(float)))<0)
				return(exit_status);
			dz->sbufptr[obufno] += advance;
			dz->sbufptr[bufno]  += advance;
			memmove((char *)dz->extrabuf[ZIGZAG_SPLBUF],
				 (char *)dz->sbufptr[bufno],dz->iparam[ZIG_SPLSAMPS]* sizeof(float));
			do_down_splice(dz);
			dz->sbufptr[bufno] += dz->iparam[ZIG_SPLSAMPS];
			*outbuf_space = dz->sampbuf[splbufno] - dz->sbufptr[obufno];
			if(*outbuf_space > dz->iparam[ZIG_SPLSAMPS]) {
				sprintf(errstr,"Error in counting logic: zig_or_zag()\n");
				return(PROGRAM_ERROR);
			}
			if(*outbuf_space <= 0) {
				sprintf(errstr,"Error 2 in counting logic: zig_or_zag()\n");
				return(PROGRAM_ERROR);
			}
		}
	}
	if(direction ==ZAG) {
		dz->sbufptr[0] -= orig_incnt;
		dz->sbufptr[1]  = dz->sampbuf[1];
	}
	return(FINISHED);
}

/* RWD got as far as here...ARRRRGGGGHHHHH!!!!!!!! */

/****************************** ADJUST_BUFFER ******************************/
/* is all this simply to seek to minsamp? */
int adjust_buffer(int minsamp, int *oldminsec, int init, dataptr dz)
{
	int minsec, secchange, sampchange = 0, samp_at_sector_start = 0;
	minsec    = minsamp/SECMARGIN;		/* minimum sector in file that we need, for next read */
	secchange = minsec - *oldminsec;	/* number of sectors we need to move buffer along */
	*oldminsec = minsec;						

	if(init) {				/* On first read */					
		if(secchange<0) {	/* secchange must be >= 0 from start of file */
			sprintf(errstr,"Anomaly in first pass of adjust_buffer()\n");
			return(PROGRAM_ERROR);
		}
		if(secchange) {		/* seek to 1st buffer needed */
			sampchange = secchange * SECMARGIN;
			if(sndseekEx(dz->ifd[0],sampchange,0)<0) {		
				sprintf(errstr,"sndseek: Failed in adjust_buffer()\n");
				return(SYSTEM_ERROR);
			}				/* find sample-number (in file) of first sample in buffer */
			samp_at_sector_start = sampchange;
		}					/* read 1st buffer */
		if(fgetfbufEx(dz->sampbuf[0],dz->buflen,dz->ifd[0],0)<0) {  
			sprintf(errstr,"read() 0 Failed in adjust_buffer()\n");
			return(SYSTEM_ERROR);
		}
		dz->sbufptr[0] = dz->sampbuf[0] + (dz->lparray[ZIGZAG_TIMES][0] - samp_at_sector_start);	
		return(FINISHED);			
	}						/* IF NOT in first read */

//TW UPDATE (Removed below) 
//	if(!secchange) {		/* Data should have been checked to avoid zero zigs or zags */
//		sprintf(errstr,"Search anomaly in adjust_buffer()\n");
//		return(PROGRAM_ERROR);
//	} 					
 
/* sampchange = Change in samp position in file from current buffer start to next buffer start.
 * NB, it should be impossible for this to exceed buffer_size
 * because initial point must be within current buffer,
 * & total length of any segment must be less than buffer_size (controlled by param range-setting) !!
 */

	if(abs(sampchange = secchange * SECMARGIN) > dz->buflen) {
		sprintf(errstr,"Anomaly in sampseek arithmetic: adjust_buffer()\n");
		return(PROGRAM_ERROR);
	}
							/* Seek first to start of current buff */
							/* and then, by the calculated offset, to start of new buf */

	if(sndseekEx(dz->ifd[0],minsec * SECMARGIN,0)<0) {
		sprintf(errstr,"sndseek() 1 Failed in adjust_buffer()\n");
		return(SYSTEM_ERROR);
	}
					/* read the new buffer */
	if(fgetfbufEx(dz->sampbuf[0],dz->buflen,dz->ifd[0],0)<0) {
		sprintf(errstr,"fgetfbufEx() Anomaly in adjust_buffer()\n");
		return(SYSTEM_ERROR);
	}		
	
	/* reposition buffer pointer within NEW buffer */
	dz->sbufptr[0] -= sampchange;
	if(dz->sbufptr[0] < dz->sampbuf[0]) {
		sprintf(errstr,"adjust_buffer(): Input buffer anomaly - pointer off start of buffer.\n");
		return(PROGRAM_ERROR);
	}
	if(dz->sbufptr[0] >= dz->sampbuf[1]) {
		sprintf(errstr,"adjust_buffer(): Input buffer anomaly - pointer off end of buffer.\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/*************************** FIND_ZZCHUNK ************************************/

int find_zzchunk(int **thisstart,int **lastend, int *ziglistend, int *minsamp, dataptr dz)
{
	int *p, min_samp, max_samp, lastmin_samp = 0, lastmax_samp = 0;
	int check;
	int max_smpsize = dz->buflen - SECMARGIN;	
												/*  Max zig-segment permitted, taking into a/c */
 												/*	need to round up to a whole sector AT THE ENDS!! */
 	*thisstart  = *lastend;						/* Set start of current segment to end of last segment */
	p = *thisstart;
						/* WE'RE SEARCHING FOR THE earliest and latest times among successive zigtimes */
	min_samp = *p;								/* so preset both of these to the current first zigtime */
	max_samp = *p;
	p++;
	while(p < ziglistend) {							/* search forward through the zigtimes list */
		check = 0;
		if(*p < min_samp) {						/* if this zigtime earlier than any encountered so far */
			lastmin_samp = min_samp;			/* store last earliest elsewhere, and keep new one */
			min_samp = *p;
			check = 1;	  						/* flag that a new earliest-time has been registered */
		} else if(*p > max_samp) {				/* if this zigtime later than any encountered so far */
			lastmax_samp = max_samp;			/* store last latest elsewhere, and keep new one */	
			max_samp = *p;
			check = 2;							/* flag that a new latest-time has been registered */
		}
		if(check) {									/* If a new earliest or latest time has been found */
			if(max_samp - min_samp > max_smpsize) {	/* If the span between earliest and latest is TOO LARGE */
				if(check==2)						/* restore the previous earliest(or latest) time */
					max_samp = lastmax_samp;
				else	
					min_samp = lastmin_samp;
				break;								/* and break from the loop */
			}										/* (Otherwise next zigtime would be beyond a bufferlength) */
		}
		p++;
	}
	if(max_samp - min_samp < 0) {
		sprintf(errstr,"Anomaly in find_zzchunk()\n");
		return(PROGRAM_ERROR);
	}
	if(max_samp - min_samp == 0)
		return(FINISHED);			
	*minsamp = min_samp;							/* flag position of earliest time, as a samp-count in file */
 				/*	NB... 
  				 *	if we reached end of ziglist (p = ziglistend), lastend = end of list = ziglistend - 1
  				 *	if we dropped out of loop, because last found time was unacceptable, 
  				 * we must baktrak in the the ziglist by 1. Hence...
  				 */
	*lastend = p - 1;

	return(CONTINUE);
}

/********************** ADD_TO_SPLICEBUF *************************/

int add_to_splicebuf(float *iptr,dataptr dz)
{
//TW CHANGED
//	/*int*/float val;
	double val;
	float *sptr = dz->extrabuf[ZIGZAG_SPLBUF];
	int n, m;
	for(n=0;n<dz->iparam[ZIG_SPLICECNT];n++) {
		for(m=0;m<dz->infile->channels;m++) {
			val = *sptr + /*round*/(float)(*iptr * dz->parray[ZIGZAG_SPLICE][n]);
			*sptr  = (float)val;
			sptr++;
			iptr++;
		}
	}
	return(FINISHED);
}

/********************** DO_DOWN_SPLICE *************************/

void do_down_splice(dataptr dz)
{
	/*int*/double val;
	float *sptr = dz->extrabuf[ZIGZAG_SPLBUF];
	int n, m;
	for(n=dz->iparam[ZIG_SPLICECNT]-1;n>=0;n--) {
		for(m = 0;m<dz->infile->channels;m++) {
			val = /*round*/(*sptr * dz->parray[ZIGZAG_SPLICE][n]);
			*sptr = (float)val;
			sptr++;
		}
	}
}

/********************** REVERSE_IT ***************************/

int reverse_it(int incnt,dataptr dz)
{
	int n;
	int k, chans = dz->infile->channels;
	float *s1ptr = dz->sbufptr[0];
	float *s2ptr;
	dz->sbufptr[1] = dz->sampbuf[1];  /* point to stsrt of reversed-segment buffer */
	s2ptr = dz->sbufptr[1];
	incnt /= chans;
	s1ptr -= chans;
	for(n=0;n<incnt;n++) {
		for(k=0;k<chans;k++)
			*s2ptr++ = *s1ptr++;
		s1ptr -= 2 * chans;
	}
	return(FINISHED);
}

/********************** MEMCPY_WITH_CHECK ***************************/

int memcpy_with_check(char *bufptr,char *bufend,char *fromptr,int bytecnt)
{
	if(bufend - bufptr < bytecnt) {
		sprintf(errstr,"Attempted to copy too many bytes to buffer: memcpy_with_check()\n");
		return(PROGRAM_ERROR);
	}
	if(bytecnt < 0) {
		sprintf(errstr,"Attempted to copy -ve number of bytes: memcpy_with_check()\n");
		return(PROGRAM_ERROR);
	}

	memmove(bufptr,fromptr,(size_t)bytecnt);
	return(FINISHED);
}

/********************** SETUP_SPLICES ***************************/

int setup_splices
(int *base,int *last,int *here,int *next,int *after,int *end,int *is_startsplice,int *is_endsplice,dataptr dz)
{
	int predirection   = UNKNOWN;
	int postdirection  = UNKNOWN;
	int direction;
	int place = here - base;
	*is_startsplice = TRUE;
	*is_endsplice   = TRUE;
	if(here > base) {
		if(*last < *here)
			predirection = ZIG;
		else			
			predirection = ZAG;
	}
	if(after < end) {
		if(*next < *after)
			postdirection = ZIG;
		else			
			postdirection = ZAG;
	}
	if(*here < *next)
		direction = ZIG;
	else
		direction = ZAG;
	if(direction == predirection)
		*is_startsplice = FALSE;
	if(direction == postdirection)
		*is_endsplice   = FALSE;
	switch(dz->process) {
	case(LOOP):
	case(SCRAMBLE):
		if(here > base 
		&& dz->iparray[ZIGZAG_PLAY][place]==TRUE && dz->iparray[ZIGZAG_PLAY][place-1]==FALSE)
			*is_startsplice = TRUE;
		if(after < end
		&& dz->iparray[ZIGZAG_PLAY][place]==TRUE && dz->iparray[ZIGZAG_PLAY][place+1]==FALSE)
			*is_endsplice = TRUE;
		break;
	}
	if(after==end && dz->iparam[ZIG_RUNSTOEND])
		*is_endsplice   = FALSE;
	if(here==base && *here == 0)
		*is_startsplice = FALSE;
	return(direction);
}

/********************** DO_END_SPLICE ***************************/

void do_end_splice(int samps_left,int obufno,dataptr dz)
{
//TW CHANGED
//	float *sptr, val;
	float *sptr;
	double val;
	int  n/*, val*/;
	int   m;
	int  splicelen = dz->iparam[ZIG_SPLICECNT] * dz->infile->channels;
	if(samps_left < splicelen) {
		fprintf(stdout,"WARNING: No buffer space to do end splice. Possible click at end.\n");
		fflush(stdout);
	} else {
		sptr = dz->sbufptr[obufno] - splicelen;
		for(n=dz->iparam[ZIG_SPLICECNT]-1;n>=0;n--) {
			for(m = 0;m<dz->infile->channels;m++) {
				val = /*round*/(*sptr * dz->parray[ZIGZAG_SPLICE][n]);
				*sptr = (float)val;
				sptr++;
			}
		}		
	}
}

//TW NEW FUNCTION (converted to float)
/********************** DO_BTOB ***************************/

int do_btob(dataptr dz)
{
	int exit_status,  chans = dz->infile->channels;
	float *ibuf = dz->sampbuf[0], *obuf = dz->sampbuf[1], *sbuf = dz->sampbuf[2];
	int total_samps_got = 0, obufcnt = 0, splicecnt = 0, startsamp, k, n, m;
	double spliceval = 1.0, spliceincr = 1.0/(double)(dz->iparam[BTOB_SPLEN]/chans);
	int samps_to_work_on = dz->insams[0] - dz->iparam[BTOB_CUT] - (dz->iparam[BTOB_SPLEN]/2);
	int bufcnt = dz->insams[0]/dz->buflen;
	int finished = 0;

	if(bufcnt * dz->buflen < dz->insams[0])
		bufcnt++;
	for(k = bufcnt-1; k >=0; k--) {
		if((sndseekEx(dz->ifd[0],k * dz->buflen,0)) < 0) {
			sprintf(errstr,"sndseekEx() 1 Failed in do_btob()\n");
			return(SYSTEM_ERROR);
		}
		if((exit_status = read_samps(ibuf,dz))<0)
			return(exit_status);
		for(n = dz->ssampsread-chans; n>=0;n-=chans) {
			if(obuf==sbuf) {
				for(m=0;m<chans;m++)
					sbuf[splicecnt++] = (float)(ibuf[n+m] * spliceval);
				spliceval -= spliceincr;			
				if(splicecnt >= dz->iparam[BTOB_SPLEN]) {
					finished = 1;
					break;
				}
			} else {			/* if NOT in splice, check whether going into splice */
				for(m=0;m<chans;m++)
					obuf[obufcnt++] = ibuf[n+m];
				if(obufcnt >= dz->buflen) {
				if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
					return(exit_status);
					obufcnt = 0;
				}
				if((total_samps_got += chans) >= samps_to_work_on) {
					obuf= sbuf;
					spliceval -= spliceincr;			
				}
			}
		}
		if(finished)
			break;
	}
	startsamp = dz->iparam[BTOB_CUT] - (dz->iparam[BTOB_SPLEN]/2);
	k = startsamp/dz->buflen;
	if((sndseekEx(dz->ifd[0],k * dz->buflen,0)) < 0) {
		sprintf(errstr,"sndseekEx() 2 Failed in do_btob()\n");
		return(SYSTEM_ERROR);
	}
	startsamp -= k * dz->buflen;
	splicecnt = 0;
	spliceval = 0.0;
	if((exit_status = read_samps(ibuf,dz))<0)
		return(exit_status);
	do {
		for(n = startsamp; n <dz->ssampsread;n+=chans) {
			if(obuf==sbuf) {
				for(m=0;m<chans;m++)
					sbuf[splicecnt+m] = (float)((ibuf[n+m] * spliceval) + sbuf[splicecnt+m]);
				spliceval += spliceincr;
				if((splicecnt += chans) >= dz->iparam[BTOB_SPLEN]) {
					obuf = dz->sampbuf[1];
					for(m=0;m<dz->iparam[BTOB_SPLEN];m++)
						obuf[obufcnt++] = sbuf[m];
					if(obufcnt >= dz->buflen) {
						if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
							return(exit_status);
						obufcnt = 0;
					}
				}
			} else {
				for(m=0;m<chans;m++)
					obuf[obufcnt++] = ibuf[n+m];
				if(obufcnt >= dz->buflen) {
					if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
						return(exit_status);
					obufcnt = 0;
				}
			}
		}
		startsamp = 0;
		if((exit_status = read_samps(ibuf,dz))<0)
			return(exit_status);
	} while(dz->ssampsread > 0);
	if(obufcnt > 0) {
		if((exit_status = write_samps(obuf,obufcnt,dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}

