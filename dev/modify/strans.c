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
#include <memory.h>
#include <structures.h>
#include <tkglobals.h>
#include <globcon.h>
#include <cdpmain.h>
#include <modeno.h>
#include <pnames.h>
#include <arrays.h>
#include <flags.h>
#include <modify.h>

#include <sfsys.h>

#ifdef unix
#define round(x) lround((x))
#else
#define round(x) cdp_round((x))
#endif

#define CALCLIM (0.02)
#define MININC 	(0.002)		/* rwd - black hole avoidance */

static int  create_vtrans_buffers(dataptr dz);
static int  create_strans_buffers(dataptr dz);

static int	read_wrapped_samps_mono(dataptr dz);
static int	read_wrapped_samps_stereo(dataptr dz);

static int	strans_process(dataptr dz);
static int  vtrans_process(dataptr dz);
static int  do_acceleration(dataptr dz);
static int  do_vibrato(dataptr dz);

static int 		vtrans_vtrans(dataptr dz);
static int 		read_samps_for_vtrans(dataptr dz);
static int	 	write_samps_with_intime_display(float *buffer,int samps_to_write,dataptr dz);
static int 		convert_brkpnts(dataptr dz);
static int 		timevents(int *obufcnt,int intime0,int intime1,double insize0,double insize1,dataptr dz);
static double 	cntevents(int dur,double s0,double s1);
static int 		samesize(int *obufcnt,int intime0,double insize0,double insize1,int duration,dataptr dz);
static int 		integral_times(int *obufcnt,int intime0,int isize,int number,dataptr dz);
static int 		putintval(int *obufcnt,int i,dataptr dz);
static int 		unvarying_times(int *obufcnt,int intime0,double size,int number,dataptr dz);
static int 		getetime(int *obufcnt,int t0,int t1,double s0,double s1,int number,dataptr dz);
static double 	refinesize(double hibound,double lobound,double fnumber,int duration,double error,double insize0);
static int 		putval(int *obufcnt,double pos,dataptr dz);
static int 		sizeq(double f1,double f2);
static int 		change_frame(dataptr dz);
static int 	inv_cntevents(int dur,double s0,double s1);
static int 		force_value_at_end_time(int paramno,dataptr dz);
static void 	splice_end(int obufcnt,dataptr dz);
static double 	interp_read_sintable(double *sintab,double sfindex);

/************************* VTRANS_PREPROCESS *****************************/

int vtrans_preprocess(dataptr dz)
{
	double acceltime, tabratio, *p, *pend;
	int n;
	switch(dz->mode) {
	case(MOD_TRANSPOS):
	case(MOD_TRANSPOS_SEMIT):
	case(MOD_TRANSPOS_INFO):
	case(MOD_TRANSPOS_SEMIT_INFO):
	    dz->iparam[UNITSIZE] = dz->insams[0]/dz->infile->channels;
	    dz->param[VTRANS_SR] = (double)dz->infile->srate;
		dz->iparam[TOTAL_UNITS_READ] = 0;
		break;
	case(MOD_ACCEL):
		acceltime = dz->param[ACCEL_GOALTIME] - dz->param[ACCEL_STARTTIME];		
    	if(acceltime < MINTIME_ACCEL) {
    		sprintf(errstr,"time for acceleration (%lf) must be greater than  %.3fsecs\n",acceltime,MINTIME_ACCEL);
			return(DATA_ERROR);
		}
	    dz->param[ACCEL_ACCEL] = pow(dz->param[ACCEL_ACCEL],(1.0/(acceltime *(double)dz->infile->srate)));
		dz->iparam[ACCEL_STARTTIME] = 	/* convert to time-in-samples */
		round(dz->param[ACCEL_STARTTIME] * (double)dz->infile->srate) * dz->infile->channels;
		break;
	case(MOD_VIBRATO):
		tabratio = (double)VIB_TABSIZE/(double)dz->infile->srate;  	/* converts frq to sintable step */
		if(dz->brksize[VIB_FRQ]) {
			p    = dz->brk[VIB_FRQ] + 1;
			pend = dz->brk[VIB_FRQ] + (dz->brksize[VIB_FRQ] * 2);
			while(p < pend) {
				*p *= tabratio;
				p += 2;
			}
		} else
			dz->param[VIB_FRQ] *= tabratio;
		if(dz->brksize[VIB_DEPTH]) {								   /* converts semitones to 8vas */
			p    = dz->brk[VIB_DEPTH] + 1;
			pend = dz->brk[VIB_DEPTH] + (dz->brksize[VIB_DEPTH] * 2);
			while(p < pend) {
				*p *= OCTAVES_PER_SEMITONE;
				p += 2;
			}
		} else
			dz->param[VIB_DEPTH] *= OCTAVES_PER_SEMITONE;
		if((dz->parray[VIB_SINTAB] = (double *)malloc((VIB_TABSIZE + 1) * sizeof(double)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for vibrato sintable.\n");
			return(MEMORY_ERROR);
		}
		tabratio = TWOPI/(double)VIB_TABSIZE;			   /* converts table-step to fraction of 2PI */
		for(n=0;n<VIB_TABSIZE;n++)
			dz->parray[VIB_SINTAB][n] = sin((double)n * tabratio);
		dz->parray[VIB_SINTAB][n] = 0.0;		
		break;
	}
	return(FINISHED);
}

/************************* CREATE_MODSPEED_BUFFERS *****************************/

int create_modspeed_buffers(dataptr dz)
{
	switch(dz->mode) {
	case(MOD_TRANSPOS):
	case(MOD_TRANSPOS_SEMIT):
	case(MOD_TRANSPOS_INFO):
	case(MOD_TRANSPOS_SEMIT_INFO):
		if(dz->brksize[VTRANS_TRANS]==0)
			return create_strans_buffers(dz);
		return create_vtrans_buffers(dz);
	case(MOD_ACCEL):
	case(MOD_VIBRATO):
		return create_strans_buffers(dz);
	default:
		sprintf(errstr,"Buffers not established for this MOD_SPEED mode.\n");
		break;
	}
	return(PROGRAM_ERROR);
}

/************************* CREATE_STRANS_BUFFERS *****************************/
//TW replaced by global
//#define FSECSIZE (256)
int create_strans_buffers(dataptr dz)
{
	size_t bigbufsize;
	int framesize;
	int guardspace = dz->infile->channels * sizeof(float); /* WRAPPED POINT(S) */
	bigbufsize = (size_t)Malloc(-1);
	
	/* RWD belt'n'braces */
	if(dz->infile->channels>STEREO) {
		sprintf(errstr, "File has too many channels.\n");
			return(DATA_ERROR);
	}

	if(dz->infile->channels==STEREO) {
//TW Safety first: stick to existing secsize-convention for now
//6 takes into account STEREO channel-count; no need to realign.
//Will do it completely differently when go to 4+ channels
		framesize = F_SECSIZE * sizeof(float) * 6;	
		if((bigbufsize   = (bigbufsize/framesize) * framesize)<=0)
			bigbufsize   = framesize;
		dz->buflen    = (int)(bigbufsize / 3 / sizeof(float));
		bigbufsize = dz->buflen * 3 * sizeof(float);
		dz->iparam[VTRANS_HBUFSIZE] = dz->buflen/2;
		if((dz->bigbuf = (float *)Malloc(bigbufsize + guardspace)) == 0) {
			sprintf(errstr,"INSUFFICIENT MEMORY FOR SND BUFFERS.\n");
			return(MEMORY_ERROR);
		}
		memset((char *)dz->bigbuf,0,bigbufsize + guardspace);
		/*dz->bigbufsize = dz->bigbufsize/3;*/
		dz->sampbuf[TRUE_LBUF] = dz->bigbuf;
		dz->sampbuf[L_GRDPNT]  = dz->sampbuf[TRUE_LBUF];	/* GUARD POINT */
		dz->sampbuf[LBUF]      = dz->bigbuf + 1;			
		dz->sampbuf[TRUE_RBUF] = dz->sampbuf[LBUF] + dz->iparam[VTRANS_HBUFSIZE];
		dz->sampbuf[R_GRDPNT]  = dz->sampbuf[TRUE_RBUF];
		dz->sampbuf[RBUF] 	   = dz->sampbuf[R_GRDPNT] + 1;			
		dz->sampbuf[INBUF] 	   = dz->sampbuf[RBUF] + dz->iparam[VTRANS_HBUFSIZE];
		dz->sampbuf[OUTBUF]    = dz->sampbuf[INBUF] + dz->buflen;
		dz->sampbuf[OBUFEND]   = dz->sampbuf[OUTBUF] + dz->buflen;
	} else {		
//TW Safety first: stick to existing secsize-convention for now
		framesize = F_SECSIZE * sizeof(float) * 2;
		bigbufsize  = (bigbufsize/framesize) * framesize;
		dz->buflen      = bigbufsize/2/sizeof(float);
		/* RWD probably unnecessary, but may be when we move to nChannels */
		bigbufsize = dz->buflen * 2 * sizeof(float);
		if((dz->bigbuf = (float *)malloc(bigbufsize + guardspace)) == 0) {
			sprintf(errstr, "Can't allocate memory for sound.\n");
			return(MEMORY_ERROR);
		}
		memset((char *)dz->bigbuf,0,bigbufsize + guardspace);
		/*dz->bigbufsize /= 2;*/
		dz->sampbuf[TRUE_LBUF] = dz->bigbuf;
		dz->sampbuf[L_GRDPNT]  = dz->sampbuf[TRUE_LBUF];
		dz->sampbuf[INBUF] = dz->bigbuf + 1;
		dz->sampbuf[OUTBUF] = dz->sampbuf[INBUF] + dz->buflen;
		dz->sampbuf[OBUFEND]   = dz->sampbuf[OUTBUF] + dz->buflen;
	}
	return(FINISHED);
}

/*************************** CREATE_VTRANS_BUFFERS **************************/

int create_vtrans_buffers(dataptr dz)
{
	size_t bigbufsize;
//TW: SAFETY FIRST for now
	int fsecbytesize = F_SECSIZE * sizeof(float);
	bigbufsize = (size_t)Malloc(-1);
	bigbufsize /= dz->bufcnt;
//TW: SAFETY FIRST for now
	bigbufsize = (bigbufsize/fsecbytesize) * fsecbytesize;

	dz->buflen = bigbufsize/sizeof(float);
	dz->iparam[UNIT_BUFLEN] = dz->buflen/dz->infile->channels;
	bigbufsize = dz->buflen * sizeof(float);

	if((dz->bigbuf = (float *)malloc((bigbufsize * dz->bufcnt) + fsecbytesize)) == NULL) {
		sprintf(errstr, "Can't allocate memory for sound.\n");
		return(MEMORY_ERROR);
	}
	dz->sampbuf[INBUF]  = dz->bigbuf;
	dz->sampbuf[OUTBUF] = dz->sampbuf[INBUF] + dz->buflen + F_SECSIZE;
	return(FINISHED);
}

/************************* PROCESS_VARISPEED ******************************/

int process_varispeed(dataptr dz)
{
	switch(dz->mode) {
	case(MOD_ACCEL):	
		return do_acceleration(dz);
	case(MOD_VIBRATO):	
		return do_vibrato(dz);
	case(MOD_TRANSPOS):
	case(MOD_TRANSPOS_SEMIT):
	case(MOD_TRANSPOS_INFO):
	case(MOD_TRANSPOS_SEMIT_INFO):
		if(dz->brksize[VTRANS_TRANS]==0)
			return strans_process(dz);
		return vtrans_process(dz);
	}
	/* RWD 9:2001 need retval*/
	return PROGRAM_ERROR;
}

/************************* STRANS_PROCESS ******************************/

int strans_process(dataptr dz)
{   
	int exit_status;
	int place;
	double flplace = 1.0, fracsamp, step;
//HEREH
	double  interp;
	float *obufptr  = dz->sampbuf[OUTBUF];
	float *obuf     = dz->sampbuf[OUTBUF];
	float *obufend  = dz->sampbuf[OBUFEND];
	float *true_lbuf = dz->sampbuf[TRUE_LBUF];
	float *true_rbuf;
	double place_inc = dz->param[VTRANS_TRANS];
	switch(dz->mode) {
	case(MOD_TRANSPOS_INFO):
		sprintf(errstr,"Output duration = %lf secs\n",dz->duration/dz->param[VTRANS_TRANS]);
		if(!sloom) {
			fprintf(stdout,"INFO: END\n");
			fflush(stdout);
		}
		return(FINISHED);
	case(MOD_TRANSPOS_SEMIT_INFO):
		place_inc = pow(2.0,(place_inc * OCTAVES_PER_SEMITONE));
		sprintf(errstr,"Output duration = %lf secs\n",dz->duration/place_inc);
		if(!sloom) {
			fprintf(stdout,"INFO: END\n");
			fflush(stdout);
		}
		return(FINISHED);
	case(MOD_TRANSPOS_SEMIT):
		place_inc = pow(2.0,(place_inc * OCTAVES_PER_SEMITONE));
		break;
	}
	switch(dz->infile->channels) {
    case(MONO):
		if((exit_status = read_wrapped_samps_mono(dz))<0)
			return(exit_status);
		while(dz->ssampsread > 0) {
			while(flplace < (double)dz->ssampsread) {
				place    = (int)flplace;
				fracsamp = flplace - (double)place;
				step     = true_lbuf[place+1] - true_lbuf[place];
//				interp   = round(fracsamp * step);
				interp   = fracsamp * step;
				*obufptr = (float)(true_lbuf[place] + interp);
				if(++obufptr >= obufend) {
					if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
						return(exit_status);
					obufptr = obuf;
				}
				flplace   += place_inc;
			}
			if((exit_status = read_wrapped_samps_mono(dz))<0)
				return(exit_status);
			flplace -= (double)dz->buflen;
		}
		break;
	case(STEREO):
		true_rbuf = dz->sampbuf[TRUE_RBUF];
		if((exit_status = read_wrapped_samps_stereo(dz))<0)
			return(exit_status);
		while(dz->ssampsread > 0) {					/* 2 */
			while(flplace < (double)dz->ssampsread) {
				place      = (int)flplace;
				fracsamp   = flplace - (double)place;
				step       = true_lbuf[place+1] - true_lbuf[place];
//				interp     = round(fracsamp * step);
				interp     = fracsamp * step;
				*obufptr++ = (float)(true_lbuf[place] + interp);
				step       = true_rbuf[place+1] - true_rbuf[place];
//				interp     = round(fracsamp * step);
				interp     = fracsamp * step;
				*obufptr++ = (float)(true_rbuf[place] + interp);
				if(obufptr >= obufend) {
					if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
						return(exit_status);
					obufptr = obuf;
				}
				flplace += place_inc;
			}
			if((exit_status = read_wrapped_samps_stereo(dz))<0)
				return(exit_status);
			flplace -= (double)dz->iparam[VTRANS_HBUFSIZE];
		}
		break;
	}
    if(obufptr!=obuf)					/* 12 */
		return write_samps(obuf,obufptr - obuf,dz);
	return(FINISHED);
}

/************************** READ_WRAPPED_SAMPS_MONO *****************************/

int read_wrapped_samps_mono(dataptr dz)
{   
	*(dz->sampbuf[L_GRDPNT]) = *(dz->sampbuf[INBUF] + dz->buflen - 1);
    return read_samps(dz->sampbuf[INBUF], dz);
}
								   
/************************* READ_WRAPPED_SAMPS_STEREO *****************************/

int read_wrapped_samps_stereo(dataptr dz)
{
	int exit_status;
	int n, m;
	float *lbuf 	= dz->sampbuf[LBUF];
	float *rbuf 	= dz->sampbuf[RBUF];
	float *l_grdpnt = dz->sampbuf[L_GRDPNT];
	float *r_grdpnt	= dz->sampbuf[R_GRDPNT];
	*l_grdpnt = *(lbuf + dz->iparam[VTRANS_HBUFSIZE] - 1);
	*r_grdpnt = *(rbuf + dz->iparam[VTRANS_HBUFSIZE] - 1);
	if((exit_status = read_samps(dz->sampbuf[INBUF],dz))<0) 
		return(exit_status);
	for(n = 0, m = 0; n < dz->ssampsread; m++, n+=2) {
		lbuf[m] = dz->sampbuf[INBUF][n];
		rbuf[m] = dz->sampbuf[INBUF][n+1];
	}
	dz->ssampsread /= 2; 	/* stereo */
	return(FINISHED);
}

/****************************** VTRANS_PROCESS *************************/

int vtrans_process(dataptr dz)
{
	int exit_status;
	if((exit_status = convert_brkpnts(dz))<0)
		return(exit_status);
	if((exit_status = change_frame(dz))<0)
		return(exit_status);
	if(dz->mode==MOD_TRANSPOS_INFO || dz->mode==MOD_TRANSPOS_SEMIT_INFO)
		return(FINISHED);
    if((exit_status = create_vtrans_buffers(dz))<0)
		return(exit_status);
    return vtrans_vtrans(dz);
}

/****************************** VTRANS_VTRANS *************************/

int vtrans_vtrans(dataptr dz)
{
	int exit_status;
	int n, m;
	double *dbrk = dz->brk[VTRANS_TRANS];
	int obufcnt = 0;
	display_virtual_time(0L,dz);
	if((exit_status = read_samps_for_vtrans(dz))<0)
		return(exit_status);
	for(n=1,m=2;n<dz->brksize[VTRANS_TRANS];n++,m+=2) {
		if((exit_status = 
			timevents(&obufcnt,(int)round(dbrk[m-2]),(int)round(dbrk[m]),dbrk[m-1],dbrk[m+1],dz))!=CONTINUE)
	    	break;
    }
	if(exit_status < 0)
		return(exit_status);
	splice_end(obufcnt,dz);
	return write_samps_with_intime_display(dz->sampbuf[OUTBUF],obufcnt ,dz);
	return FINISHED;
}

/*************************** READ_SAMPS_FOR_VTRANS **************************/

int read_samps_for_vtrans(dataptr dz)
{
//TW MUST READ EXTRA SECTOR to get wrap-around value
	float *buffer = dz->sampbuf[INBUF];
	int samps_read, samps_to_read = dz->buflen + F_SECSIZE;
	dz->iparam[UNITS_RD_PRE_THISBUF] = dz->iparam[TOTAL_UNITS_READ];
	if((samps_read = fgetfbufEx(buffer, samps_to_read,dz->ifd[0],0)) < 0) {
		sprintf(errstr, "Can't read from input soundfile.\n");
		return(SYSTEM_ERROR);
	}
	dz->samps_left -= samps_read;
	if(samps_read == samps_to_read) {
		if((sndseekEx(dz->ifd[0],-(int)F_SECSIZE,1))<0) { /* WE'VE READ EXTRA SECTOR,for wrap-around */
			sprintf(errstr,"sndseekEx() failed.\n");
			return(SYSTEM_ERROR);
		}
		samps_read -= F_SECSIZE;
//TW: correcting an omission in original
		dz->samps_left += F_SECSIZE;
	}
	dz->total_samps_read 		 += samps_read;
	dz->iparam[UNITS_READ]        = samps_read/dz->infile->channels;
	dz->iparam[TOTAL_UNITS_READ] += dz->iparam[UNITS_READ];
	return(FINISHED);
}

/*************************** WRITE_SAMPS_WITH_INTIME_DISPLAY ***********************/

int write_samps_with_intime_display(float *buf,int samps_to_write,dataptr dz)
{   
	int exit_status;
	int samps_written;
// 	if((samps_written = fputfbufEx(buf,samps_to_write,dz->ofd))<0) {
	if(samps_to_write > 0) {
		if((exit_status = write_samps_no_report(buf,samps_to_write,&samps_written,dz))<0) {
			sprintf(errstr, "Can't write to output soundfile.\n");
			return(SYSTEM_ERROR);
		}
		display_virtual_time(dz->total_samps_read,dz);
	}
	return(FINISHED);
}

/************************ CONVERT_BRKPNTS ************************/

int convert_brkpnts(dataptr dz)
{
	int exit_status;
	double *p, *pend;
	if((exit_status= force_value_at_zero_time(VTRANS_TRANS,dz))<0)
		return(exit_status);
	if((exit_status= force_value_at_end_time(VTRANS_TRANS,dz))<0)
		return(exit_status);

	p    = dz->brk[VTRANS_TRANS];
	pend = p + (dz->brksize[VTRANS_TRANS] * 2);

	while(p < pend) {
		*p = (double)round((*p) * dz->param[VTRANS_SR]);	 /* convert to (mono) sample frame */
		p += 2;
	}
	if(dz->mode==MOD_TRANSPOS_SEMIT || dz->mode==MOD_TRANSPOS_SEMIT_INFO) {
		p  = dz->brk[VTRANS_TRANS] + 1;		
		while(p < pend) {
			*p = pow(2.0,(*p) * OCTAVES_PER_SEMITONE);	/* convert to ratios */
			p += 2;
		}
	}
	return(FINISHED);
}

/************************** TIMEVENTS *******************************/
/*
 * 	Generates event positions from startsize and end size
 *
 *	Let start-time be T0 and end time be T1
 *	Let start size be S0 and end size be S1
 *
 *	number of event is given by :-
 *
 *		N = (T1-T0) log S1
 *		    -------    e__
 *		    (S1-S0)     S0
 *
 *	In general this will be non-integral, and we should
 *	round N to an integer, and recalculate S1 by successive 
 *	approximation.
 *
 *	Then positions of events are given by
 *	
 *	    for X = 0 to N		 (S1-S0)
 *					  ----- X
 *	T = (S1T0 - S0T1)   S0(T1 - T0)  (T1-T0)
 *	 s   -----------  + ----------  e 
 *	      (S1 - S0)      (S1 - S0)
 *
 * 	If difference in eventsizes input to the main program is very small
 *
 * This function calculates the number of events within the time-interval,
 * and generates the times of these events.
 */

int timevents(int *obufcnt,int intime0,int intime1,double insize0,double insize1,dataptr dz)
{   
	int exit_status;
	int   number;
	double fnum, fnumber, error, pos;
	double lobound, hibound;
	int duration;
	if(flteq(insize0,0.0) || flteq(insize1,0.0)) {
		sprintf(errstr,"Event size of zero encountered in pair %lf %lf at time %lf\n",
		insize0,insize1,(double)intime0/dz->param[VTRANS_SR]);
		return(DATA_ERROR);
	}
	duration = intime1-intime0;
	if(duration<=0) {
		sprintf(errstr,"Inconsistent input times [ %lf : %lf ] to timevents()\n",
		(double)intime0/dz->param[VTRANS_SR],(double)intime1/dz->param[VTRANS_SR]);
		return(DATA_ERROR);
	}
	if(sizeq(insize1,insize0))				/* 1 */
		return(samesize(obufcnt,intime0,insize0,insize1,duration,dz));

	fnum =cntevents(duration,insize0,insize1);		/* 2 */
	number = round(fnum);
	if(number<2) {					/* 3 */
		pos = (double)(intime0 - dz->iparam[UNITS_RD_PRE_THISBUF]);
		if(pos >= (double)dz->iparam[UNITS_READ]) {
			if(dz->samps_left <= 0)
				return(FINISHED);
			if((exit_status = read_samps_for_vtrans(dz))<0)
				return(exit_status);
			pos -= (double)dz->iparam[UNIT_BUFLEN];
		}
		if((exit_status = putval(obufcnt,pos,dz))<0)
			return(exit_status);
	} else {
		fnumber = (double)number;			/* 4 */
		error   = fabs(fnum - fnumber);
/* HIBOUND is a SMALLER value of SIZE to give BIGGER value of NUM-OF-SEGS */
/* LOBOUND is a LARGER value of SIZE to give SMALLER value of NUM-OF-SEGS */
		lobound = max(insize1,insize0);			/* 5 */
		hibound = min(insize1,insize0);
		if(error > FLTERR)				/* 6 */
			insize1 = refinesize(hibound,lobound,fnumber,duration,error,insize0);
		else
			insize1 = (hibound+lobound)/2;
		return(getetime(obufcnt,intime0,intime1,insize0,insize1,number,dz));
    }							/* 7 */
	return(CONTINUE);
}


/*************************** CNTEVENTS *****************************/

double cntevents(int dur,double s0,double s1)
{   
	double f1 = (double)dur,f2;
	if(sizeq(s1,s0))
		return((f1*2.0)/(s1+s0));
	f2  = s1-s0;
	f1 /= f2;
	f2  = s1/s0;
	f2  = log(f2);
	f1 *= f2;
	return(fabs(f1));
}

/******************************* SAMESIZE(): *******************************
 *
 * get event positions, if eventsize approx same throughout segment.
 *
 *(1)	Get average size, find number of events and round to nearest int.
 *(2)	If too many or zero events, do exception.
 *(3)	Recalculate size, and thence event times.
 */

int samesize(int *obufcnt,int intime0,double insize0,double insize1,int duration,dataptr dz)
{   
	int exit_status;
	int number, isize;
	double size, pos;					/* 1 */
	size     = (insize0+insize1)/2;
	number   = round((double)duration/size);
	if(number==0) {					/* 2 */
		pos = (double)(intime0 - dz->iparam[UNITS_RD_PRE_THISBUF]);
		if(pos >= (double)dz->iparam[UNITS_READ]) {
			if(dz->samps_left <= 0)
				return(FINISHED);
			if((exit_status = read_samps_for_vtrans(dz))<0)
				return(exit_status);
			pos -= (double)dz->iparam[UNIT_BUFLEN];
		}
		if((exit_status = putval(obufcnt,pos,dz))<0)
			return(exit_status);
		return(CONTINUE);
	}
	size = (double)duration/(double)number;
	if(flteq((double)(isize = (int)round(size)),size))
		return integral_times(obufcnt,intime0,isize,number,dz);
	return unvarying_times(obufcnt,intime0,size,number,dz);
}

/************************ INTEGRAL_TIMES ***************************
 *
 * GENERATE vals for equally spaced events, spaced by whole nos. of samps.
 */

int integral_times(int *obufcnt,int intime0,int isize,int number,dataptr dz)
{
	int exit_status;
	int k, pos = intime0 - dz->iparam[UNITS_RD_PRE_THISBUF];
	for(k=0;k<number;k++) {
		if(pos >= dz->iparam[UNITS_READ]) {
			if(dz->samps_left <= 0)
				return(FINISHED);
			if((exit_status = read_samps_for_vtrans(dz))<0)
				return(exit_status);
			pos -= dz->iparam[UNIT_BUFLEN];
		}
		if((exit_status = putintval(obufcnt,pos,dz))<0)
			return(exit_status);
		pos += isize;
	}
	return(CONTINUE);
}

/****************************** PUTINTVAL ******************************/

int putintval(int *obufcnt,int i,dataptr dz)
{  
	int exit_status;
	int n;
	float *buffer = dz->sampbuf[INBUF];
	float *obuf   = dz->sampbuf[OUTBUF];
	i *= dz->infile->channels;
	for(n=0;n<dz->infile->channels;n++) {
		obuf[(*obufcnt)++] = buffer[i];
		if(*obufcnt >= dz->buflen) {
			if((exit_status = write_samps_with_intime_display(obuf,dz->buflen,dz))<0)
				return(exit_status);
			*obufcnt = 0;
		}
		i++;	
	}
	return(FINISHED);
}

/************************ UNVARYING_TIMES ***************************
 *
 * GENERATE vals for equally spaced events.
 */

int unvarying_times(int *obufcnt,int intime0,double size,int number,dataptr dz)
{   
	int exit_status;
	int k; 
	double pos = (double)(intime0 - dz->iparam[UNITS_RD_PRE_THISBUF]);
/******* k = 0 *********/
	for(k=0;k<number;k++) {
/****************/
		if(pos >= (double)dz->iparam[UNITS_READ]) {
			if(dz->samps_left <= 0)
				return(FINISHED);
			if((exit_status = read_samps_for_vtrans(dz))<0)
				return(exit_status);
			pos -= (double)dz->iparam[UNIT_BUFLEN];
		}
		if((exit_status = putval(obufcnt,pos,dz))<0)
			return(exit_status);
		pos += size;
	}
    return(CONTINUE);
}

/******************************* GETETIME(): ********************************
 *
 * Calculate time-positions of events that vary in size between s0 and s1.
 * NB We need to invert the time order, if s1 < s0.
 */

int getetime(int *obufcnt,int t0,int t1,double s0,double s1,int number,dataptr dz)
{   
	int exit_status;
	int n;
	double sdiff = s1-s0, tdiff = (double)(t1-t0), d1, d2, d3, pos;
 /***** TW n=0 NOT 1 ********/
// for(n=1;n<number;n++)   {
	for(n=0;n<number;n++)   {
/***** TW ********/
		d1    = sdiff/tdiff;
		d1   *= (double)n;
		d1    = exp(d1);
		d2    = s0*tdiff;
		d2   /= sdiff;
		d1   *= d2;
		d2    = s1*t0;
		d3    = s0*t1;
		d2   -= d3;
		d2   /= sdiff;
		pos   = d1 + d2;
		pos -= (double)dz->iparam[UNITS_RD_PRE_THISBUF];
		if(pos >= (double)dz->iparam[UNITS_READ]) {
			if(dz->samps_left <= 0)
				return(FINISHED);
			if((exit_status = read_samps_for_vtrans(dz))<0)
				return(exit_status);
			pos -= (double)dz->iparam[UNIT_BUFLEN];
		}
		if((exit_status = putval(obufcnt,pos,dz))<0)
			return(exit_status);
	}
	return(CONTINUE);
}

/***************************** REFINESIZE(): ******************************
 *
 * refine size of final event to reduce error within bounds.
 */

double refinesize(double hibound,double lobound,double fnumber,int duration,double error,double insize0)
{   
	double size, fnum, lasterror;
	double flterr_squared = FLTERR * FLTERR;
	do {
		lasterror = error;
		size  = (hibound+lobound)/2.0;
		fnum  = cntevents(duration,insize0,size);
		error = fabs(fnumber-fnum);
		if(error>FLTERR)  {
			if(fnum<fnumber)
				lobound = size;
			else
				hibound = size;
		}
		if(fabs(lasterror - error) < flterr_squared)
			break;	/* In case error cannot be reduced */
	} while(error > FLTERR);
	return(size);
}

/****************************** PUTVAL ******************************/

int putval(int *obufcnt,double pos,dataptr dz)
{
	int exit_status;
	int i = (int)pos;	/* TRUNCATE */
	double frac = pos - (double)i, diff; 
	float val1, val2;
	float *buffer = dz->sampbuf[INBUF];
	float *obuf   = dz->sampbuf[OUTBUF];
	int chans = dz->infile->channels;
	int n;
	i *= chans;
	for(n=0;n<chans;n++) {
		val1 = buffer[i];
		val2 = buffer[i+chans];
		diff = (double)(val2-val1);
//TW	obuf[(*obufcnt)++] = val1 + (float)/*round*/(frac * diff);
		obuf[(*obufcnt)++] = (float)(val1 + (frac * diff));
		if(*obufcnt >= dz->buflen) {
			if((exit_status = write_samps_with_intime_display(obuf,dz->buflen,dz))<0)
				return(exit_status);
			*obufcnt = 0;
		}
		i++;	
	}
	return(CONTINUE);
}

/**************************** SIZEQ *******************************/

int sizeq(double f1,double f2)
{   
	double upperbnd, lowerbnd;
	upperbnd = f2 + CALCLIM;
	lowerbnd = f2 - CALCLIM;
	if((f1>upperbnd) || (f1<lowerbnd))
		return(FALSE);
	return(TRUE);
}

/********************** CHANGE_FRAME *****************************/

int change_frame(dataptr dz)
{
	int n, m;
	int *newtime;
	double *dbrk = dz->brk[VTRANS_TRANS];
	double lasttime, lastval, thistime, thisval, intime, outtime; 
	if((newtime = (int *)malloc(dz->brksize[VTRANS_TRANS] * 2 * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create transformed brktable.\n");
		return(MEMORY_ERROR);
	}
	newtime[0] = 0;
	lasttime = dbrk[0];
	lastval  = dbrk[1];
	switch(dz->mode) {
	case(MOD_TRANSPOS_INFO):
	case(MOD_TRANSPOS_SEMIT_INFO):
		for(n=1,m=2;n<dz->brksize[VTRANS_TRANS];n++,m+=2) {
			thistime = dbrk[m]; 
			thisval  = dbrk[m+1]; 
			if(dz->vflag[VTRANS_OUTTIMES])
				newtime[n] = newtime[n-1] + inv_cntevents((int)round(thistime - lasttime),lastval,thisval);
			else
				newtime[n] = newtime[n-1] + round(cntevents((int)round(thistime - lasttime),lastval,thisval));
			lasttime = thistime;
			lastval  = thisval;
		}
		print_outmessage_flush("TIME GIVEN\tOCCURS AT OUTTIME\n");
		for(n=0,m=0;n<dz->brksize[VTRANS_TRANS];n++,m+=2) {
			intime  = dbrk[m]/dz->param[VTRANS_SR];
			outtime = (double)newtime[n]/dz->param[VTRANS_SR];
			sprintf(errstr,"%lf\t%lf\n",intime,outtime);
			print_outmessage_flush(errstr);
		}
		if(!sloom) {
			fprintf(stdout,"INFO: END\n");
			fflush(stdout);
		}
		break;
	case(MOD_TRANSPOS):
	case(MOD_TRANSPOS_SEMIT):
		if(dz->vflag[VTRANS_OUTTIMES]) {	/* convert to intimes */
			for(n=1,m=2;n<dz->brksize[VTRANS_TRANS];n++,m+=2) {
				thistime = dbrk[m]; 
				thisval  = dbrk[m+1]; 
				newtime[n] = newtime[n-1] + 
						 inv_cntevents((int)round(thistime - lasttime),lastval,thisval);
				lasttime = thistime;
				lastval  = thisval;
			}
			for(n=1,m=2;n<dz->brksize[VTRANS_TRANS];n++,m+=2)
				dbrk[m] = (double)newtime[n];
		}
		break;
	}
	free(newtime);
	return(FINISHED);
}

/*************************** INV_CNTEVENTS *****************************
 *
 *	no. of event in input (N) corresponding to output (T1-T0) given by :-
 *
 *		N = (T1-T0) log S1
 *		    -------    e__
 *		    (S1-S0)     S0
 *
 *      Hence, 
 *	no. of event in output (T1-T0) corresponding to input (N) given by :-
 *
 *		(S1-S0)N = (T1-T0)
 *		--------
 *		 log S1
 *		    e__
 *		     S0
 *
 *	Except where S1==S0, when (T1-T0) = S * N
 */

int inv_cntevents(int dur,double s0,double s1)
{   
	double ftime;
	int time;
	if(sizeq(s1,s0))
		ftime = (double)dur * s0;
	else
		ftime = ((double)dur * (s1-s0))/log(s1/s0);
	time = round(ftime);
	if(time==0)
		time = 1;
	return(time);
}

/*************************** FORCE_VALUE_AT_END_TIME *****************************/

int force_value_at_end_time(int paramno,dataptr dz)
{
	double duration = (double)dz->iparam[UNITSIZE]/dz->param[VTRANS_SR];
	double *p = dz->brk[paramno] + ((dz->brksize[paramno]-1) * 2);
	double timegap, newgap, step, newstep, timeratio;
	if(dz->brksize[paramno] < 2) {
		sprintf(errstr,"Brktable too small (< 2 pairs).\n");
		return(DATA_ERROR);
	}
	if(flteq(*p,duration)) {
		*p = duration;
		return(FINISHED);
	} else if(*p <  duration) {
		dz->brksize[paramno]++;
		if((dz->brk[paramno] = realloc((char *)dz->brk[paramno],dz->brksize[paramno] * 2 * sizeof(double)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY TO GROW BRKTABLE.\n");
			return(MEMORY_ERROR);
		}
		p = dz->brk[paramno] + ((dz->brksize[paramno]-1) * 2);
		*p = duration;
		*(p+1) = *(p-1);
		return(FINISHED);
	} else {  /* *p >  duration) */
		while(p > dz->brk[paramno]) {
			p -= 2;
			if(flteq(*p,duration)) {
				*p = duration;
				dz->brksize[paramno] = (p - dz->brk[paramno] + 2)/2;
				return(FINISHED);
			} else if(*p < duration) {
				timegap = *(p+2) - *p;
				newgap = duration - *p;
				timeratio = newgap/timegap;
				switch(dz->mode) {
				case(MOD_TRANSPOS_SEMIT):
				case(MOD_TRANSPOS_SEMIT_INFO):
					step = *(p+3) - *(p+1);
					newstep = step * timeratio;
					*(p+3) = *(p+1) + newstep;
					break;
				case(MOD_TRANSPOS):
				case(MOD_TRANSPOS_INFO):
					step    = LOG2(*(p+3)) - LOG2(*(p+1));
					newstep = pow(2.0,step * timeratio);
					*(p+3) = *(p+1) * newstep;
					break;										
				}
				*(p+2) = duration;
				dz->brksize[paramno] = (p - dz->brk[paramno] + 4)/2;
				return(FINISHED);
			}
		}
	}
	sprintf(errstr,"Failed to place brkpnt at snd endtime.\n");
	return(PROGRAM_ERROR);
}

/*************************** SPLICE_END *****************************/

void splice_end(int obufcnt,dataptr dz)		/* A kludge to avoid clicks at end */
{

#define VTRANS_SPLICELEN (5.0)

	float *buf = dz->sampbuf[OUTBUF];
	int chans = dz->infile->channels;
	int splicelen = round(VTRANS_SPLICELEN * MS_TO_SECS * dz->infile->srate) * dz->infile->channels;
	int n, m, k = min(obufcnt,splicelen);
	int startsamp = obufcnt - k;
	double inv_k;
	k /= chans;
	inv_k = 1.0/(double)k;
	for(n = k-1;n >= 0;n--) {
		for(m=0;m<chans;m++) {
			buf[startsamp] = (float)(buf[startsamp] * n * inv_k);
			startsamp++;
		}
	}

}
	
/******************************* DO_ACCELERATION ***************************/

int do_acceleration(dataptr dz)
{	
	int exit_status;
	int place;
	double flplace, fracsamp, step;
//	int OK = 1, interp;
	int OK = 1;
	double interp;
	float *ibuf      = dz->sampbuf[INBUF];
	float *obuf      = dz->sampbuf[OUTBUF];
	float *true_lbuf = dz->sampbuf[TRUE_LBUF];
	float *true_rbuf = dz->sampbuf[TRUE_RBUF];
	float *obufend   = dz->sampbuf[OBUFEND];
	float *obufptr  = obuf;
	double place_inc = 1.0;
	int   startsamp = dz->iparam[ACCEL_STARTTIME];
	int   previous_total_ssampsread = 0;
	double accel     = dz->param[ACCEL_ACCEL];
	dz->total_samps_read = 0;
	display_virtual_time(0L,dz);
	if(dz->infile->channels==STEREO) {
		while(dz->total_samps_read<=startsamp) {
			previous_total_ssampsread = dz->total_samps_read;
			if((exit_status = read_wrapped_samps_stereo(dz))<0)
				return(exit_status);
			if(dz->total_samps_read <= startsamp)
				if((exit_status = write_samps_with_intime_display(ibuf,dz->ssampsread * STEREO,dz))<0)
					return(exit_status);
		}
		startsamp -= previous_total_ssampsread;
		if(startsamp > 0)
			memcpy((char *)obuf,(char *)ibuf,startsamp * sizeof(float));
		obufptr += startsamp;
		startsamp /= 2;	/* STEREO divided into two buffers */
		place = startsamp + 1;
		flplace = place;
		while(dz->ssampsread > place) {
			while(place < dz->ssampsread) {
				fracsamp   = flplace - (double)place;
				step       = true_lbuf[place+1] - true_lbuf[place];
//				interp     = round(fracsamp * step);
				interp     = fracsamp * step;
				*obufptr++ = (float)(true_lbuf[place] + interp);
				step       = true_rbuf[place+1] - true_rbuf[place];
//				interp     = round(fracsamp * step);
				interp     = fracsamp * step;
				*obufptr++ = (float)(true_rbuf[place] + interp);
				if(obufptr >= obufend) {
					if((exit_status = write_samps_with_intime_display(obuf,dz->buflen,dz))<0)
						return(exit_status);
					obufptr = obuf;
				}
				flplace   += place_inc;
				place_inc *= accel;
				if(place_inc <= MININC) {
					fprintf(stdout, "INFO: Acceleration reached black hole! - finishing\n");
					fflush(stdout);
					OK = 0;
					break;
				}
				place = (int)flplace;
			}
			if(!OK) {
				break;
			}
			if(dz->samps_left > 0) {
				flplace -= (double)dz->ssampsread;
				place   -= dz->ssampsread;
				if((exit_status = read_wrapped_samps_stereo(dz))<0)
					return(exit_status);
				if(dz->ssampsread<=0)
					break;
			} else
				break;
		}
	} else {
		while(dz->total_samps_read<=startsamp) {
			previous_total_ssampsread = dz->total_samps_read;
			if((exit_status = read_wrapped_samps_mono(dz))<0)
				return(exit_status);
			if(dz->total_samps_read <= startsamp) {
				if((exit_status = write_samps_with_intime_display(ibuf,dz->ssampsread,dz))<0)
					return(exit_status);
			}
		}
		startsamp -= previous_total_ssampsread;
		if(startsamp > 0)
			memcpy((char *)obuf,(char *)ibuf,startsamp * sizeof(float));
		obufptr += startsamp;
		place = startsamp + 1;
		flplace = place;
		while(dz->ssampsread > place) {
			while(place < dz->ssampsread) {
				fracsamp   = flplace - (double)place;
				step       = true_lbuf[place+1] - true_lbuf[place];
//				interp     = round(fracsamp * step);
				interp     = fracsamp * step;
				*obufptr++ = (float)(true_lbuf[place] + interp);
				if(obufptr >= obufend) {
					if((exit_status = write_samps_with_intime_display(obuf,dz->buflen,dz))<0)
						return(exit_status);
					obufptr = obuf;
				}
				flplace   += place_inc;
				place_inc *= accel;
				if(place_inc <= MININC) {
					fprintf(stdout, "INFO: Acceleration reached black hole! - finishing\n\n");
					fflush(stdout);
					OK = 0;
					break;
				}
				place = (int)flplace;
			}
			if(!OK) {
				break;
			}
			if(dz->samps_left > 0) {
				flplace -= (double)dz->ssampsread;
				place   -= dz->ssampsread;
				if((exit_status = read_wrapped_samps_mono(dz))<0)
					return(exit_status);
				if(dz->ssampsread<=0)
					break;
			} else
				break;
		}
	}
	splice_end(obufptr - obuf,dz);
	return write_samps_with_intime_display(obuf,obufptr - obuf ,dz);  
}

/******************************* DO_VIBRATO ********************************/

int do_vibrato(dataptr dz)
{		
	int    exit_status;
	float  *ibuf      = dz->sampbuf[INBUF];
	float  *obuf      = dz->sampbuf[OUTBUF];
	float  *obufend   = dz->sampbuf[OBUFEND];
	float  *true_lbuf = dz->sampbuf[TRUE_LBUF];
	float  *true_rbuf = NULL;
	double *sintab = dz->parray[VIB_SINTAB];
	int    chans    = dz->infile->channels;
	double sr       = (double)dz->infile->srate;
	double tabsize  = (double)VIB_TABSIZE;
	double bloksize = (double)VIB_BLOKSIZE;
	double inv_bloksize = 1.0/bloksize;
	double thistime = 0.0;
	double time_incr = bloksize/sr;	/* timestep  between param reads */
	int    effective_buflen = dz->buflen/chans;
	int   place   = 0;				/* integer position in sndfile   */
	double flplace = 0.0;			/* float position in sndfile     */
	double incr;					/* incr of position in sndfile   */
	double sfindex = 0.0;			/* float position in sintable    */
	double sinval;					/* current sintable value        */
	double current_depth, lastdepth, depth_incr;
	double current_frq,   lastfrq,   frq_incr;
	double fracsamp;				/* fraction of samptime to interp*/
	double step;					/* step between snd samples      */
//	int    interp;					/* part of sampstep to use   	 */
	double interp;					/* part of sampstep to use   	 */
	int    blokcnt = 0;				/* distance through blokd params */
	float  *obufptr = obuf;
	if(dz->brksize[VIB_FRQ]) {
		if((exit_status = read_value_from_brktable(thistime,VIB_FRQ,dz))<0)
			return(exit_status);
	} else
		frq_incr = 0.0;	
	if(dz->brksize[VIB_DEPTH]) {
		if((exit_status = read_value_from_brktable(thistime,VIB_DEPTH,dz))<0)
			return(exit_status);
	} else
		depth_incr = 0.0;	
	current_frq   = lastfrq   = dz->param[VIB_FRQ];
	current_depth = lastdepth = dz->param[VIB_DEPTH];
	thistime += time_incr;						  /* now time is 1 bloklength ahead */

	switch(chans) {
	case(MONO):
		if((exit_status = read_wrapped_samps_mono(dz))<0)
			return(exit_status);
		obuf[0] = ibuf[0];
		obufptr++;
		break;
	case(STEREO):
		true_rbuf = dz->sampbuf[TRUE_RBUF];
		if((exit_status = read_wrapped_samps_stereo(dz))<0)
			return(exit_status);
		obuf[0] = ibuf[0];
		obufptr++;
		obuf[1] = ibuf[1];
		obufptr++;
		break;
	}

	if((exit_status = read_values_from_all_existing_brktables(thistime,dz))<0)
		return(exit_status);						/* read value 1 bloklength ahead */
	frq_incr   = (dz->param[VIB_FRQ]   - lastfrq)   * inv_bloksize;
	depth_incr = (dz->param[VIB_DEPTH] - lastdepth) * inv_bloksize;
	lastfrq   = dz->param[VIB_FRQ];
	lastdepth = dz->param[VIB_DEPTH];
	blokcnt = VIB_BLOKSIZE - 1;		  /* -1 as we've read 1 (stereo) sample already. */
	thistime += time_incr;						  /* Now time is 2 bloklengths ahead */
								/* but we won't read till another 1 blok has passed. */
	for(;;) {
		if(--blokcnt <= 0) {	/* look at params only every BLOKCNT (stereo)samples */
			if((exit_status = read_values_from_all_existing_brktables(thistime,dz))<0)
				return(exit_status); /* & read vals 1 bloklen ahead,to calc par incr */
			frq_incr   = (dz->param[VIB_FRQ]   - lastfrq)   * inv_bloksize;
			depth_incr = (dz->param[VIB_DEPTH] - lastdepth) * inv_bloksize;
			lastfrq   = dz->param[VIB_FRQ];
			lastdepth = dz->param[VIB_DEPTH];
			blokcnt = VIB_BLOKSIZE;
			thistime += time_incr;
		}
		current_depth += depth_incr;	 	  /* increment params sample by (stereo) sample */
		current_frq   += frq_incr;
		if((sfindex  += current_frq) >= tabsize)
			sfindex -= tabsize;		 		 /* advance in sintable,wrapping around if ness */
		sinval = interp_read_sintable(sintab,sfindex);		/* read sintab by interpolation */
		incr = pow(2.0,sinval * current_depth); /* convert speed & depth from 8vas to ratio */
		if((flplace += incr) >= (double)dz->ssampsread) {	   /* advance through src sound */
			if(dz->samps_left <=0)							    /* if at end of src, finish */
				break;						 /* else if at end of buffer, get more sound in */
			switch(chans) {
			case(MONO):
				if((exit_status = read_wrapped_samps_mono(dz))<0)
					return(exit_status);
				break;
			case(STEREO):
				if((exit_status = read_wrapped_samps_stereo(dz))<0)
					return(exit_status);
				break;
			}
			flplace -= effective_buflen;
		}
		place 	   = (int)flplace; 	/* TRUNCATE */	   /* read sndfile by interpolation */
		fracsamp   = flplace - (double)place;
		step       = true_lbuf[place+1] - true_lbuf[place];
//		interp     = round(fracsamp * step);
		interp     = fracsamp * step;
		*obufptr++ = (float)(true_lbuf[place] + interp);
		if(chans==STEREO) {
			step       = true_rbuf[place+1] - true_rbuf[place];
//			interp     = round(fracsamp * step);
			interp     = fracsamp * step;
			*obufptr++ = (float)(true_rbuf[place] + interp);
		}
		if(obufptr >= obufend) {					 /* if at end of outbuf, write a buffer */
			if((exit_status = write_samps_with_intime_display(obuf,dz->buflen,dz))<0)
				return(exit_status);
			obufptr = obuf;
		}
	}
	if(obufptr < obufend)						 /* if samples remain in outbuf, write them */
		return write_samps_with_intime_display(obuf,obufptr - obuf,dz);
	return(FINISHED);
}

/******************************* INTERP_READ_SINTABLE ********************************/

double interp_read_sintable(double *sintab,double sfindex)
{
	int    sindex0  = (int)sfindex;	/* TRUNCATE */			
	int    sindex1  = sindex0 + 1;
	double frac = sfindex - (double)sindex0;
	double val0 = sintab[sindex0];
	double val1 = sintab[sindex1];
	double valdiff = val1 - val0;
	double valfrac = valdiff * frac;
	return(val0 + valfrac);
}
