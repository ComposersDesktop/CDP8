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
#include <string.h>				/*RWD Nov 2003 */
#include <structures.h>
#include <tkglobals.h>
#include <globcon.h>
#include <modify.h>
#include <cdpmain.h>
#include <modeno.h>
#include <arrays.h>
#include <filetype.h>
#include <sfsys.h>
//TW UPDATE
#include <osbind.h>

#if defined unix || defined __GNUC__
#define round(x) lround((x))
#endif

//TW UPDATES
#define SMALLARRAY	20
#define	ONE_dB		1.22018

#define BSIZE		(128)
#define ROOT2		(1.4142136)
#define SIGNAL_TO_LEFT  (0)
#define SIGNAL_TO_RIGHT (1)

#define	NARROW_IT	(1)
#define	MONO_IT		(0)
#define	MIRROR_IT	(-1)

static long newpar(int *brkindex,double *par,double *parincr,long *total_sams,dataptr dz);
static void pancalc(double position,double *leftgain,double *rightgain);
static void do_mirror(float *buffer,dataptr dz);
static void do_narrow(double narrow,double one_less_narrow,dataptr dz);
static void do_mono(dataptr dz);

static int more_space(int size_step,long *arraysize,double **brk);
static int get_sinval(double *f0,double *a0,double time,double *phase,double phase_quantum,double **p,dataptr dz);

//TW UPDATE: NEW FUNCTIONS
static int insert_point_in_envelope(int *last_postk,double basetime,int thisenv,int *cnt,
		double *last_posttime,double *last_postlevel,int *last_insertk,double *last_inserttime,double *last_insertlevel,
		double envtime,double envlevel,dataptr dz);

/* SHUDDER */
static int apply_brkpnt_envelope(dataptr dz);
static int  init_brkpnts
	(long *sampno,double *starttime,double *gain,double **endbrk,double **nextbrk,
	 double *nextgain,double *gain_step,double *gain_incr,long *nextbrk_sampno,int paramno,int brksize,dataptr dz);
static int  advance_brkpnts
	(double starttime,double *gain,double *endbrk,double **nextbrk,double *nextgain,
	 double *gain_step,double *gain_incr,long *nextbrk_sampno,dataptr dz);
static int  simple_envel_processing
	(long *samps_left_to_process,double starttime,double *endbrk,double **nextbrk,double *nextgain,
	 long *sampno,double *gain,double *gain_step,double *gain_incr,long *nextbrk_sampno,int chan,float *maxobuf,dataptr dz);
static int  do_mono_envelope
	(long samp_cnt,double starttime,double *endbrk,double **nextbrk,double *nextgain,
	 long *sampno,double *gain,double *gain_step,double *gain_incr,long *nextbrk_sampno,float *maxobuf,dataptr dz);
static int  read_ssamps(long samps_to_read,long *samps_left_to_process,int chan,dataptr dz);
static int  do_simple_read(long *samps_left_to_process,long buflen,int chan,dataptr dz);
static int other_read_samps(long samps_to_read,int fileid,int bufno,dataptr dz);
static void unlink_temp_files(int type,int ofd0, int ofd1,dataptr dz);

/************************************ MODSPACE_PCONSISTENCY *******************************/

int modspace_pconsistency(dataptr dz) 
{
	switch(dz->mode) {
	case(MOD_PAN):
		if(dz->infile->channels!=MONO) {
			sprintf(errstr,"ERROR: PAN only works with MONO input files.\n");
			return(DATA_ERROR);
		}
		break;
	case(MOD_MIRROR):
		if(dz->infile->channels!=STEREO) {
			sprintf(errstr,"ERROR: MIRROR only works with STEREO input files.\n");
			return(DATA_ERROR);
		}
		break;
	case(MOD_MIRRORPAN):
		if(dz->infile->filetype!=UNRANGED_BRKFILE) {
			sprintf(errstr,"ERROR: MIRROR PAN only works with PAN textdata files.\n");
			return(DATA_ERROR);
		}
		if(dz->brksize[dz->extrabrkno] < 2) {
			sprintf(errstr,"ERROR: MIRROR PAN table too short [%d pairs < 2].\n",dz->brksize[dz->extrabrkno]);
			return(DATA_ERROR);
		}
		break;
	case(MOD_NARROW):
		if(dz->infile->channels!=STEREO) {
			sprintf(errstr,"ERROR: NARROW only works with STEREO input files.\n");
			return(DATA_ERROR);
		}
		break;
	}
	return(FINISHED);
}

/************************************ MODSPACE_PREPROCESS *******************************/

int modspace_preprocess(dataptr dz)
{
	int exit_status;
	if(dz->mode == MOD_PAN && dz->brksize[PAN_PAN]) {
		if((exit_status = force_value_at_zero_time(PAN_PAN,dz))<0)
			return(exit_status);
	}

	if(dz->mode==MOD_PAN){
		/* create stereo outfile here! */
		/* RWD 4:2002  now we can open outfile with corect params! */
		dz->infile->channels = STEREO;	/* ARRGH! */
		if((exit_status = create_sized_outfile(dz->outfilename,dz))<0)
			return(exit_status);
	}


	return(FINISHED);
}

/************************************ DOPAN *******************************/

int dopan(dataptr dz)
{
	int exit_status;
	int 	i;
	int		brkindex = 1;
	long	block = 0, sams = 0, total_sams = 0;
	float  *inbuf = dz->sampbuf[0], *bufptr;
	double	leftgain,rightgain;
	double	lcoef = 0.0, rcoef = 0.0;			
	double	par = 0.0, parincr = 0.0;
/*	double	fdist = 1.0;*/		/* fdist is for listener distance: not used yet */

	if(!dz->brksize[PAN_PAN]) {
		pancalc(dz->param[PAN_PAN],&leftgain,&rightgain);
		lcoef = leftgain  * dz->param[PAN_PRESCALE];
		rcoef = rightgain * dz->param[PAN_PRESCALE];
	}
	display_virtual_time(0L,dz);
	dz->infile->channels = STEREO;	/* affects output only */
	do {
		if((exit_status = read_samps(inbuf,dz))<0) {
			sprintf(errstr,"Failed to read data from sndfile.\n");
			return(DATA_ERROR);
		}
		bufptr = dz->sampbuf[1];	/* reset output buffer pointer */

		for (i = 0 ; i < dz->ssampsread; i++ ) {
			if(dz->brksize[PAN_PAN]) {
				if(sams-- <= 0)
					sams = newpar(&brkindex,&par,&parincr,&total_sams,dz);
				if(block-- <= 0) {
					pancalc(par,&leftgain,&rightgain);
					lcoef = leftgain  * dz->param[PAN_PRESCALE];
					rcoef = rightgain * dz->param[PAN_PRESCALE];
					block += BSIZE;
					par += parincr;
	    		}
			}
			*bufptr++ = (float) /*round*/(lcoef * inbuf[i]);
			*bufptr++ = (float) /*round*/(rcoef * inbuf[i]);
		}
		if(dz->ssampsread > 0) {
			if((exit_status = write_samps(dz->sampbuf[1],dz->ssampsread * STEREO,dz))<0)
				return(exit_status);
		}
	} while(dz->samps_left > 0);
	return(FINISHED);
}

/************************************ MIRRORING *******************************/

int mirroring(dataptr dz)
{  
	int exit_status;
	float *inbuf = dz->sampbuf[0];
	do {
		if((exit_status = read_samps(inbuf,dz))<0) {
			sprintf(errstr,"Failed to read data from sndfile.\n");
			return(DATA_ERROR);
		}
		do_mirror(inbuf,dz);
		if(dz->ssampsread > 0) {
			if((exit_status = write_exact_samps(dz->sampbuf[0],dz->ssampsread,dz))<0)
				return(exit_status);
		}
	} while(dz->samps_left > 0);
	return(FINISHED);
}

/******************************** MIRROR_PANFILE ********************************/

void mirror_panfile(dataptr dz)
{
	int    panbrk = dz->extrabrkno;
	long   n;
	double *panbrktab  = dz->brk[panbrk];
	long   panbrksize = dz->brksize[panbrk];
	double *p = panbrktab + 1;
	for(n=0;n<panbrksize;n++) {
		*p = -(*p);
		p += 2;
	}		
	p = panbrktab;
	for(n=0;n<panbrksize;n++) {
		fprintf(dz->fp,"%lf\t%lf\n",*p,*(p+1));
		p += 2;
	}
	return;
}

/***************************** NARROW_SOUND ***************************/

int narrow_sound(dataptr dz)
{   
	int exit_status;
	int narrowing = NARROW_IT;
	double narrow = dz->param[NARROW], one_less_narrow;
	float *buffer = dz->sampbuf[0];
	display_virtual_time(0L,dz);
	if(flteq(narrow,1.0)) {
		sprintf(errstr, "WARNING: Narrowing of 1.0 has no effect on input file.\n");
		return(DATA_ERROR);
   	}
	if(flteq(narrow,-1.0)) 		narrowing = MIRROR_IT;
   	else if(flteq(narrow,0.0))	narrowing = MONO_IT;
   	narrow = dz->param[NARROW] + 1.0;
   	narrow /= 2.0;
   	one_less_narrow = narrow;
   	narrow  = 1.0 - narrow;	
	while(dz->samps_left != 0) {
		if((exit_status = read_samps(buffer,dz))<0) {
			sprintf(errstr,"Failed to read data from sndfile.\n");
			return(PROGRAM_ERROR);
		}
		switch(narrowing) {
		case(NARROW_IT):	do_narrow(narrow,one_less_narrow,dz);	break;
		case(MIRROR_IT):	do_mirror(buffer,dz);					break;
		case(MONO_IT):		do_mono(dz);							break;
		}
		if(dz->ssampsread > 0) {
			if((exit_status= write_exact_samps(buffer,dz->ssampsread,dz))<0)
				return(exit_status);
		}
    }
	return(FINISHED);
}

/************************************ NEWPAR *******************************/

long newpar(int *brkindex,double *par,double *parincr,long *total_sams,dataptr dz)
{
	double diff, steps, nextval;
	double *thisbrk = dz->brk[PAN_PAN];
	long here;
	long sams;
	if(*brkindex < dz->brksize[PAN_PAN]) {
		here  = (*brkindex) * 2;
		sams  = round(thisbrk[here] * dz->infile->srate) - *total_sams;
		*par  = thisbrk[here-1];
		nextval = thisbrk[++here];
		diff  = nextval - (*par);
		steps = (double)sams/(double)BSIZE;	
		*parincr = diff/steps;
		(*brkindex)++;
	} else {
		*parincr = 0.0;
		sams = dz->insams[0] - *total_sams;
	}
	*total_sams += sams;
	return(sams);	
}

/************************************ PANCALC *******************************/

void pancalc(double position,double *leftgain,double *rightgain)
{
	int dirflag;
	double temp;
	double relpos;
	double reldist, invsquare;

	if(position < 0.0)
		dirflag = SIGNAL_TO_LEFT;		/* signal on left */
	else
		dirflag = SIGNAL_TO_RIGHT;

	if(position < 0) 
		relpos = -position;
	else 
		relpos = position;
	if(relpos <= 1.0){		/* between the speakers */
		temp = 1.0 + (relpos * relpos);
		reldist = ROOT2 / sqrt(temp);
		temp = (position + 1.0) / 2.0;
		*rightgain = temp * reldist;
		*leftgain = (1.0 - temp ) * reldist;
	} else {				/* outside the speakers */
		temp = (relpos * relpos) + 1.0;
		reldist  = sqrt(temp) / ROOT2;   /* relative distance to source */
		invsquare = 1.0 / (reldist * reldist);
		if(dirflag == SIGNAL_TO_LEFT){
			*leftgain = invsquare;
			*rightgain = 0.0;
		} else {   /* SIGNAL_TO_RIGHT */
			*rightgain = invsquare;
			*leftgain = 0;
		}
	}
}

/************************************ DO_MIRROR *******************************/

void do_mirror(float *buffer,dataptr dz)
{
	register long i;
	float store;
	for(i = 0; i < dz->ssampsread; i += 2) {
		store       = buffer[i];
		buffer[i]   = buffer[i+1];
		buffer[i+1] = store;
	}
}

/******************************* DO_NARROW *************************/

void do_narrow(double narrow,double one_less_narrow,dataptr dz)
{
	register long i;
	double new_l, new_r, add_to_l, add_to_r;
	float *buffer = dz->sampbuf[0];
	for(i = 0; i < dz->ssampsread; i+=2)  {
//TW CHANGED (casts removed)
		add_to_r    = buffer[i]   * narrow;
		add_to_l    = buffer[i+1] * narrow;
		new_l       = buffer[i]   * one_less_narrow;
		new_r       = buffer[i+1] * one_less_narrow;
		new_l      += add_to_l;
		new_r      += add_to_r;
		buffer[i]   = (float) /*round*/(new_l);
		buffer[i+1] = (float) /*round*/(new_r);
    }
}

/******************************* DO_MONO *************************/

void do_mono(dataptr dz)
{   
	register long  i, j;
	register double temp;
	float *buffer =dz->sampbuf[0];
	for(i = 0; i < dz->ssampsread; i+=2)  {
		j = i+1;
		temp      = (buffer[i] + buffer[j]) /*/2.0*/ * 0.5;
		buffer[i] = (float)/*round*/(temp);
		buffer[j] = (float)/*round*/(temp);
	}
}

/****************************** GENERATE_SINTABLE *******************************/

int generate_sintable(dataptr dz)
{
	int exit_status;
	double *p, f0, a0, phase_quantum, time, phase;
	long arraysize = BIGARRAY, cnt;

	if((dz->parray[SIN_TABLE] = (double *)malloc(arraysize * sizeof(double)))==NULL) {
		sprintf(errstr,"Out of memory for breakpoint table.\n");
		return(MEMORY_ERROR);
	}
	p = dz->parray[SIN_TABLE];
	phase = (dz->param[SIN_PHASE]/360.0) * TWOPI;
//TW UPDATE
//	*p++ = 0.0;
//	*p++ = phase;
//	cnt = 2;
	time = 0.0;
	if(dz->brksize[SIN_FRQ]) {
		if((exit_status = read_value_from_brktable(time,SIN_FRQ,dz))<0)
			return(exit_status);
	}
//TW UPDATE
	dz->param[SIN_FRQ] = 1.0/dz->param[SIN_FRQ];

	if(dz->brksize[SIN_AMP]) {
		if((exit_status = read_value_from_brktable(time,SIN_AMP,dz))<0)
			return(exit_status);
	}		
	f0 = dz->param[SIN_FRQ];
	a0 = dz->param[SIN_AMP];

	phase_quantum = dz->param[SIN_QUANT] * TWOPI;
//TW UPDATES
	if((exit_status = get_sinval(&f0,&a0,time,&phase,phase_quantum,&p,dz))<0)
		return exit_status;
	cnt = 2;

	for(time = dz->param[SIN_QUANT]; time <= dz->param[SIN_DUR]; time += dz->param[SIN_QUANT]) {
		if((cnt += 2) > arraysize) {
			if((exit_status = more_space(BIGARRAY,&arraysize,&(dz->parray[SIN_TABLE])))<0)
				return(exit_status);
			p = dz->parray[SIN_TABLE] + cnt - 2;
		}
		if((exit_status = get_sinval(&f0,&a0,time,&phase,phase_quantum,&p,dz))<0)
			return exit_status;
	}		
	if(!flteq(*(p-2),dz->param[SIN_DUR])) {
		if((cnt += 2) > arraysize) {
			arraysize = cnt;
			if((exit_status = more_space(2,&arraysize,&(dz->parray[SIN_TABLE])))<0)
				return(exit_status);
			p = dz->parray[SIN_TABLE] + cnt - 2;
		}
		if((exit_status = get_sinval(&f0,&a0,dz->param[SIN_DUR],&phase,phase_quantum,&p,dz))<0)
			return exit_status;
	} else {
		*(p-2) = dz->param[SIN_DUR];
	}
	if(cnt < arraysize) {
		if((dz->parray[SIN_TABLE] = (double *)realloc((char *)(dz->parray[SIN_TABLE]),cnt * sizeof(double)))==NULL) {
			sprintf(errstr,"Memory error on shrinking breaktable to size.\n");
			return(MEMORY_ERROR);
		}
	}
	cnt /= 2;
	if((exit_status = write_brkfile(dz->fp,cnt,SIN_TABLE,dz))<0)
		return(exit_status);
	return(FINISHED);
}

/****************************** GET_SINVAL *******************************/

int get_sinval(double *f0,double *a0,double time,double *phase,double phase_quantum,double **p,dataptr dz)
{
	int exit_status;
	double frq, amp, val;
	if(dz->brksize[SIN_FRQ]) {
		if((exit_status = read_value_from_brktable(time,SIN_FRQ,dz))<0)
			return(exit_status);
//TW UPDATE
		dz->param[SIN_FRQ] = 1.0/dz->param[SIN_FRQ];

		frq = (dz->param[SIN_FRQ] + *f0) /*/2.0 */ * 0.5;
	} else {
		frq = *f0;
	}
	if(dz->brksize[SIN_AMP]) {
		if((exit_status = read_value_from_brktable(time,SIN_AMP,dz))<0)
			return(exit_status);
		amp = (dz->param[SIN_AMP] + *a0) /* /2.0*/ * 0.5;
	} else {
		amp = *a0;
	}
//TW UPDATE (moved line)
	val = sin(*phase) * amp;
	*phase += frq * phase_quantum;
	*phase = fmod(*phase,TWOPI);
	**p = time;
	(*p)++;
	**p = val;
	(*p)++;
	*f0 = dz->param[SIN_FRQ];
	*a0 = dz->param[SIN_AMP];
	return(FINISHED);
}

/****************************** MORE_SPACE *******************************/

int more_space(int size_step,long *arraysize,double **brk)
{
	*arraysize += size_step;
	if((*brk = (double *)realloc((char *)(*brk),(*arraysize) * sizeof(double)))==NULL) {
		sprintf(errstr,"Out of memory for breakpoint table.\n");
		return(MEMORY_ERROR);
	}
	return(FINISHED);
}

//TW NEW FUNCTIONS (updated for flotsams)
/************************************ SCALEDPAN_PREPROCESS *******************************/

int scaledpan_preprocess(dataptr dz)
{
	int exit_status, n, len;
	double endtime, scaling;
	double *thisbrk;
	if((len = (dz->brksize[PAN_PAN] * 2)) <= 0) {
		fprintf(stdout,"WARNING: Scaled pan is no different to ordinary panning, if you do not use a brkpoint file.\n");
		fflush(stdout);
		return(FINISHED);
	}
	if((exit_status = force_value_at_zero_time(PAN_PAN,dz))<0)
		return(exit_status);
	thisbrk = dz->brk[PAN_PAN];
	endtime = thisbrk[len - 2];
	scaling = dz->duration/endtime;
	for(n=0;n<len;n+=2)
		thisbrk[n] *= scaling;
	/* create stereo outfile here! */
	dz->infile->channels = STEREO;	/* ARRGH! */
	return create_sized_outfile(dz->outfilename,dz);
}

/************************************* DO_SHUDDER ***********************************/

int do_shudder(dataptr dz)
{
	int left, exit_status;
	long cnt = 0, cnt2 = 0, timecnt, n, m, j, levelcnt;
	int cnt0, cnt1;
	long arraysz[2] = {SMALLARRAY,SMALLARRAY};
	double time, nexttime, lasttime, pos, leftgain, rightgain, depth, maxlevel, width, lastrealtime = -1.0;
	double envtime, envlevel, lgain, rgain, scat, realtime, levell, levelr, one_minus_depth, safety = 1.0/ROOT2;
	double last_posttime[2], last_postlevel[2], last_inserttime[2], last_insertlevel[2], basetime[2];
	int last_insertk[2], last_postk[2];

	double ee[] = {-7,-5, -3, -1, 1,  3,  5,  7};
	double ff[] = {0, 0.2,0.8,1.0,0.8,0.2,0.1,0};

	dz->tempsize = dz->insams[0] * 2;
	initrand48();
	for(n=0;n<8;n++)
		ee[n] *= 1.0/14.0;

	if((dz->parray[SHUD_TIMES] = (double *)malloc(arraysz[0] * sizeof(double)))==NULL) {
		sprintf(errstr,"Out of Memory While calculating Shudder Times\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[SHUD_LEVEL] = (double *)malloc(arraysz[1] * sizeof(double)))==NULL) {
		sprintf(errstr,"Out of Memory While calculating Shudder Levels\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[SHUD_WIDTH] = (double *)malloc(arraysz[0] * sizeof(double)))==NULL) {
		sprintf(errstr,"Out of Memory While calculating Shudder Widths\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[SHUD_DEPTH] = (double *)malloc(arraysz[0] * sizeof(double)))==NULL) {
		sprintf(errstr,"Out of Memory While calculating Shudder Widths\n");
		return(MEMORY_ERROR);
	}
	if((drand48() * 2.0) - 1.0 <= 0.0)
		left = 1;
	else
		left = -1;
								/* Get TIME, and WIDTH of initial shudder */
	time = dz->param[SHUD_STARTTIME];
	if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
		return(exit_status);
	width = ((dz->param[SHUD_MAXWIDTH] - dz->param[SHUD_MINWIDTH]) * drand48()) + dz->param[SHUD_MINWIDTH];
	dz->parray[SHUD_WIDTH][cnt] = width;
	time = max(time,width/2.0);
	dz->parray[SHUD_TIMES][cnt] = time;

				/* From STEREO-POSITION and DEPTH, calculate L and R levels */
	
	pos = dz->param[SHUD_SPREAD] * drand48() * left;
	pancalc(pos,&leftgain,&rightgain);
	
	depth = ((dz->param[SHUD_MAXDEPTH] - dz->param[SHUD_MINDEPTH]) * drand48()) + dz->param[SHUD_MINDEPTH];
					/* SHUDDER is added to existing level */
					/*... and will be automatically scaled by existing level when imposed on sound */
	dz->parray[SHUD_DEPTH][cnt] = 1.0 - depth;
	lgain = depth * leftgain;
	rgain = depth * rightgain;
	maxlevel = max(lgain,rgain);
	dz->parray[SHUD_LEVEL][cnt2++] = lgain;
	dz->parray[SHUD_LEVEL][cnt2++] = rgain;
	left = -left;
	cnt++;
				/* get, time , width and L & R level for all shudders */
	while(time < dz->duration) {
		lasttime = time;
		time = time + (1/dz->param[SHUD_FRQ]);	/* use frq from previous time, to calculate where next time is */
		if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
			return(exit_status);
		nexttime = time + (1/dz->param[SHUD_FRQ]);
		scat = ((drand48() * 2.0) - 1.0) * dz->param[SHUD_SCAT];
		if(scat >= 0)
			realtime = ((nexttime - time) * scat) + time;
		else
			realtime = time - ((time - lasttime) * scat);
		width = ((dz->param[SHUD_MAXWIDTH] - dz->param[SHUD_MINWIDTH]) * drand48()) + dz->param[SHUD_MINWIDTH];
		if((realtime <= (width/2.0) + FLTERR) || (realtime <= lastrealtime))
			continue;
		lastrealtime = realtime;
		dz->parray[SHUD_TIMES][cnt] = realtime;
		pos = dz->param[SHUD_SPREAD] * drand48() * left; 
		left = -left;
		pancalc(pos,&leftgain,&rightgain);
		depth = ((dz->param[SHUD_MAXDEPTH] - dz->param[SHUD_MINDEPTH]) * drand48()) + dz->param[SHUD_MINDEPTH];
		dz->parray[SHUD_DEPTH][cnt] = 1.0 - depth;
		lgain = depth * leftgain;
		rgain = depth * rightgain;
		maxlevel = max(maxlevel,lgain);
		maxlevel = max(maxlevel,rgain);
		dz->parray[SHUD_LEVEL][cnt2++] = lgain;
		dz->parray[SHUD_LEVEL][cnt2++] = rgain;
		dz->parray[SHUD_WIDTH][cnt] = width;
		if (++cnt >= arraysz[0]) {
			arraysz[0] += SMALLARRAY;
			if((dz->parray[SHUD_TIMES] = (double *)realloc((char *)dz->parray[SHUD_TIMES],arraysz[0] * sizeof(double)))==NULL) {
				sprintf(errstr,"Out of Memory While calculating Shudder Times\n");
				return(MEMORY_ERROR);
			}
			if((dz->parray[SHUD_WIDTH] = (double *)realloc((char *)dz->parray[SHUD_WIDTH],arraysz[0] * sizeof(double)))==NULL) {
				sprintf(errstr,"Out of Memory While calculating Shudder Widths\n");
				return(MEMORY_ERROR);
			}
			if((dz->parray[SHUD_DEPTH] = (double *)realloc((char *)dz->parray[SHUD_DEPTH],arraysz[0] * sizeof(double)))==NULL) {
				sprintf(errstr,"Out of Memory While calculating Shudder Widths\n");
				return(MEMORY_ERROR);
			}
		}
		if (cnt2 >= arraysz[1]) {
			arraysz[1] += SMALLARRAY;
			if((dz->parray[SHUD_LEVEL] = (double *)realloc((char *)dz->parray[SHUD_LEVEL],arraysz[1] * sizeof(double)))==NULL) {
				sprintf(errstr,"Out of Memory While calculating Shudder Levels\n");
				return(MEMORY_ERROR);
			}
		}
	}
	if((dz->parray[SHUD_TIMES] = (double *)realloc((char *)dz->parray[SHUD_TIMES],cnt * sizeof(double)))==NULL) {
		sprintf(errstr,"Out of Memory While calculating Shudder Times\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[SHUD_WIDTH] = (double *)realloc((char *)dz->parray[SHUD_WIDTH],cnt * sizeof(double)))==NULL) {
		sprintf(errstr,"Out of Memory While calculating Shudder Widths\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[SHUD_DEPTH] = (double *)realloc((char *)dz->parray[SHUD_DEPTH],cnt * sizeof(double)))==NULL) {
		sprintf(errstr,"Out of Memory While calculating Shudder Widths\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[SHUD_LEVEL] = (double *)realloc((char *)dz->parray[SHUD_LEVEL],cnt2 * sizeof(double)))==NULL) {
		sprintf(errstr,"Out of Memory While calculating Shudder Levels\n");
		return(MEMORY_ERROR);
	}
	/* NORMALISE LEVELS */
	for(m=0;m<cnt2;m++)
		dz->parray[SHUD_LEVEL][m] /= maxlevel;

	if((dz->parray[SHUD_ENV0] = (double *)malloc(((cnt * 16) + 4) * sizeof(double)))==NULL) {
		sprintf(errstr,"Out of Memory While calculating Shudder Envelopes\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[SHUD_ENV1] = (double *)malloc(((cnt * 16) + 4) * sizeof(double)))==NULL) {
		sprintf(errstr,"Out of Memory While calculating Shudder Envelopes\n");
		return(MEMORY_ERROR);
	}
	timecnt = 0;
	levelcnt = 0;
	cnt0 = 0;
	cnt1 = 0;
	basetime[0] = -1.0;
	basetime[1] = -1.0;
	arraysz[0] = SMALLARRAY;
	arraysz[1] = SMALLARRAY;
	while(timecnt< cnt) {
		time  = dz->parray[SHUD_TIMES][timecnt];	
		width = dz->parray[SHUD_WIDTH][timecnt];	
		one_minus_depth = dz->parray[SHUD_DEPTH][timecnt];
		levell = dz->parray[SHUD_LEVEL][levelcnt];
		levelr = dz->parray[SHUD_LEVEL][levelcnt+1];
		last_postk[0] = -1;
		last_postk[1] = -1;
		for(j=0;j<8;j++) {
			envtime = time + (ee[j] * width);
			envlevel = ((levell * ff[j]) + one_minus_depth) * safety;
					/* safety compensates for centre boost given by pan-calculation */
			insert_point_in_envelope(&(last_postk[0]),basetime[0],SHUD_ENV0,&cnt0,&(last_posttime[0]),&(last_postlevel[0]),
				&(last_insertk[0]),&(last_inserttime[0]),&(last_insertlevel[0]),envtime,envlevel,dz);
			envlevel = (levelr * ff[j]) + one_minus_depth;
			insert_point_in_envelope(&(last_postk[1]),basetime[1],SHUD_ENV1,&cnt1,&(last_posttime[1]),&(last_postlevel[1]),
				&(last_insertk[1]),&(last_inserttime[1]),&(last_insertlevel[1]),envtime,envlevel,dz);
		
		}
		basetime[0] = dz->parray[SHUD_ENV0][cnt0-2];	/* last envelope time in array */
		basetime[1] = dz->parray[SHUD_ENV1][cnt1-2];	/* last envelope time in array */

		timecnt++;
		levelcnt+=2;
	}
	if(dz->parray[SHUD_ENV0][0] <= FLTERR) {			/* Force points at ZERO and at DURATION+ */
		dz->parray[SHUD_ENV0][0] = 0.0;
		dz->parray[SHUD_ENV1][0] = 0.0;
	} else {
		for(n=cnt0-1;n>=0;n--)
			dz->parray[SHUD_ENV0][n+2] = dz->parray[SHUD_ENV0][n];
		for(n=cnt1-1;n>=0;n--)
			dz->parray[SHUD_ENV1][n+2] = dz->parray[SHUD_ENV1][n];
		dz->parray[SHUD_ENV0][0] = 0.0;
		dz->parray[SHUD_ENV0][1] = dz->parray[SHUD_ENV0][3];
		dz->parray[SHUD_ENV1][0] = 0.0;
		dz->parray[SHUD_ENV1][1] = dz->parray[SHUD_ENV1][3];
		cnt0 += 2;
		cnt1 += 2;
	}
	if(dz->parray[SHUD_ENV0][cnt0 - 2] < dz->duration - FLTERR) {
		dz->parray[SHUD_ENV0][cnt0] = dz->duration + FLTERR;
		dz->parray[SHUD_ENV0][cnt0 + 1] = dz->parray[SHUD_ENV0][cnt0 - 1];
		dz->parray[SHUD_ENV1][cnt1] = dz->duration + FLTERR;
		dz->parray[SHUD_ENV1][cnt1 + 1] = dz->parray[SHUD_ENV1][cnt1 - 1];
		cnt0 += 2;
		cnt1 += 2;
	}
	if((dz->parray[SHUD_ENV0] = (double *)realloc((char *)dz->parray[SHUD_ENV0],cnt0 * sizeof(double)))==NULL) {
		sprintf(errstr,"Out of Memory While calculating Shudder Envelopes\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[SHUD_ENV1] = (double *)realloc((char *)dz->parray[SHUD_ENV1],cnt1 * sizeof(double)))==NULL) {
		sprintf(errstr,"Out of Memory While calculating Shudder Envelopes\n");
		return(MEMORY_ERROR);
	}
	dz->itemcnt = cnt0;			/* used to store counts */
	dz->extrabrkno = cnt1;		/* used to store counts */
	if((exit_status = apply_brkpnt_envelope(dz))<0)
		return(exit_status);
	return(FINISHED);
}

/************************************* INSERT_POINT_IN_ENVELOPE ***********************************/

int insert_point_in_envelope(int *last_postk,double basetime,int thisenv,int *cnt,
		double *last_posttime,double *last_postlevel,int *last_insertk,double *last_inserttime,double *last_insertlevel,
		double envtime,double envlevel,dataptr dz)
{
	int k, j, last_pk;
	double pretime, posttime, prelevel, postlevel, timestep, frac, levelhere, last_ptime, last_plevel;

	if(envtime <= basetime) {
		k = *cnt - 2;
		while (dz->parray[thisenv][k] > envtime)
			k -= 2;
		pretime   = dz->parray[thisenv][k];
		prelevel  = dz->parray[thisenv][k+1];
		k += 2;
		posttime  = dz->parray[thisenv][k];
		postlevel = dz->parray[thisenv][k+1];
		timestep = posttime - pretime;
		frac      = (envtime - pretime)/timestep;
		levelhere = ((postlevel - prelevel) * frac) + prelevel;
		if(envlevel > levelhere) {
			for(j = *cnt-1;j >= k; j--)				/* SHUFFLUP DATA ABOVE */
				dz->parray[thisenv][j+2] = dz->parray[thisenv][j];
			dz->parray[thisenv][k]   = envtime;	/* INSERT NEW POINT */
			dz->parray[thisenv][k+1] = envlevel;
			*cnt += 2;
			if(*last_postk >= 0) {					/* IF POINTS HAVE ALREADY BEEN INSERTED */
				timestep = dz->parray[thisenv][k] - dz->parray[thisenv][*last_insertk];
				last_pk     = *last_postk;
				last_ptime  = *last_posttime;		/* CHECK POINTS INTERVENING BETWEEN THIS AND LAST INSERT */
				last_plevel = *last_postlevel;
				while(envtime > last_ptime) {
					frac = (last_ptime - *last_inserttime)/timestep;
					levelhere = ((envlevel - *last_insertlevel) * frac) + *last_insertlevel;
					if(levelhere > last_plevel)
						dz->parray[thisenv][last_pk+1] = levelhere;
					last_pk += 2;
					last_ptime  = dz->parray[thisenv][last_pk];
					last_plevel = dz->parray[thisenv][last_pk+1];
				}
			}
			*last_insertk = k;						/* REMEMBER TIMES AND LEVELS OF LAST INSERT, AND POST-INSERT */
			*last_inserttime  = dz->parray[thisenv][k];
			*last_insertlevel = dz->parray[thisenv][k+1];
			k +=2;
			*last_postk = k;							
			*last_posttime  = dz->parray[thisenv][k];
			*last_postlevel = dz->parray[thisenv][k+1];
		} else {
			*last_postk = -1;
		}
	} else {
		dz->parray[thisenv][(*cnt)++]   = envtime;
		dz->parray[thisenv][(*cnt)++]   = envlevel;
		*last_postk = -1;
	}
	return(FINISHED);
}

/******************************* APPLY_BRKPNT_ENVELOPE *******************************/

int apply_brkpnt_envelope(dataptr dz)
{
	int exit_status, ofd0, ofd1;
	long startsamp = 0, samps_left_to_process = dz->insams[0], sampout_cnt;
	long sampno, nextbrk_sampno;
	double starttime, gain, nextgain, gain_step, gain_incr, multiplier0 = 0.0, multiplier1 = 0.0;
	double *endbrk, *nextbrk; 
	long total_samps_to_read, n, j, k,samps_to_read_here;
	int chan;
	float maxobuf0, maxobuf1;
	char *outfilename;

	if(sloom) {
		if((outfilename = (char *)malloc((strlen(dz->wordstor[0])+1) * sizeof(char)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for outfilename.\n");
			return(MEMORY_ERROR);
		}				
	} else {
		if((outfilename = (char *)malloc((strlen(dz->outfilename)+1) * sizeof(char)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for outfilename.\n");
			return(MEMORY_ERROR);
		}				
	}
										/* ENVELOPE CHANNEL 1 */
	chan = 0;
	maxobuf0 = 0.0f;
	if((exit_status = do_simple_read(&samps_left_to_process,dz->buflen,chan,dz))<0)
		return(exit_status);								/* setup simple buffering option */
	if((exit_status = init_brkpnts
	(&sampno,&starttime,&gain,&endbrk,&nextbrk,&nextgain,&gain_step,&gain_incr,&nextbrk_sampno,SHUD_ENV0,dz->itemcnt,dz))<0)
		return(exit_status);
	if((exit_status = simple_envel_processing(&samps_left_to_process,starttime,endbrk,&nextbrk,&nextgain,
		&sampno,&gain,&gain_step,&gain_incr,&nextbrk_sampno,chan,&maxobuf0,dz))<0)
		return(exit_status);
										/* STORE CHANNEL 1 OUTFILE */
	ofd0 = dz->ofd;
										/* CREATE CHANNEL 2 OUTFILE */
	if(sloom) {
		strcpy(outfilename,dz->wordstor[0]);
		insert_new_number_at_filename_end(outfilename,2,1);
	} else {
		strcpy(outfilename,dz->outfilename);
		insert_new_number_at_filename_end(outfilename,2,1);
	}
	sampout_cnt = dz->total_samps_written;

	if((exit_status = create_sized_outfile(outfilename,dz))<0) {
		dz->ofd = ofd0;
		return(SYSTEM_ERROR);
	}
	dz->total_samps_written = sampout_cnt;
										/* RESET INFILE TO START */
	sndseekEx(dz->ifd[0],0,0);
	startsamp = 0;
	samps_left_to_process = dz->insams[0];

										  /* ENVELOPE CHANNEL 2 */
	chan = 1;
	maxobuf1 = 0.0f;
	if((exit_status = do_simple_read(&samps_left_to_process,dz->buflen,chan,dz))<0)
		return(exit_status);								/* setup simple buffering option */
	if((exit_status = init_brkpnts
	(&sampno,&starttime,&gain,&endbrk,&nextbrk,&nextgain,&gain_step,&gain_incr,&nextbrk_sampno,SHUD_ENV1,dz->extrabrkno,dz))<0)
		return(exit_status);
	if((exit_status = simple_envel_processing(&samps_left_to_process,starttime,endbrk,&nextbrk,&nextgain,
		&sampno,&gain,&gain_step,&gain_incr,&nextbrk_sampno,chan,&maxobuf1,dz))<0)
		return(exit_status);
										/* STORE CHANNEL 2 OUTFILE */
	ofd1 = dz->ofd;
										 /* CREATE STEREO OUTFILE */
	strcpy(outfilename,dz->wordstor[0]);
	if(sloom)
		insert_new_number_at_filename_end(outfilename,0,1);
	else
		outfilename[strlen(outfilename) -9] = ENDOFSTR;

	sampout_cnt = dz->total_samps_written;
	if((exit_status = create_sized_outfile(outfilename,dz))<0) {
		unlink_temp_files(1,ofd0,ofd1,dz);
		return(SYSTEM_ERROR);
	}
	if((exit_status = reset_peak_finder(dz))<0)
		return(exit_status);
	dz->total_samps_written = sampout_cnt;
									/* RESET BOTH CHANNEL-FILES TO START */
	sndseekEx(ofd0,0,0);
	sndseekEx(ofd1,0,0);
	total_samps_to_read = dz->insams[0]/2;

								/* MERGE SEPARATE CHANNELS BACK INTO STEREO */
	if(dz->vflag[SHUD_BALANCE]) {
		if(((maxobuf0 > maxobuf1) && (maxobuf0/maxobuf1 < ONE_dB)) 
		|| (maxobuf1/maxobuf0 < ONE_dB))
			dz->vflag[0] = 0;
		else if(maxobuf0 > maxobuf1) {
			multiplier1 = maxobuf0/maxobuf1;
		} else {
			multiplier1 = -1.0;
			multiplier0 = maxobuf1/maxobuf0;
		}
	}
	while(total_samps_to_read > 0) {
		samps_to_read_here = min(total_samps_to_read,dz->buflen);
		if((exit_status = other_read_samps(samps_to_read_here,ofd0,0,dz))<0) {
			unlink_temp_files(2,ofd0,ofd1,dz);
			return(exit_status);
		}
		if((exit_status = other_read_samps(samps_to_read_here,ofd1,1,dz))<0) {
			unlink_temp_files(2,ofd0,ofd1,dz);
			return(exit_status);
		}
		if(dz->vflag[SHUD_BALANCE]) {
			if(multiplier1 > 0.0) {
				for(n=0,j=0,k=1;n<dz->ssampsread;n++,j+=2,k+=2) {
					dz->sampbuf[2][j] = dz->sampbuf[0][n];
					dz->sampbuf[2][k] = (float)(dz->sampbuf[1][n] * multiplier1);
				}
			} else {
				for(n=0,j=0,k=1;n<dz->ssampsread;n++,j+=2,k+=2) {
					dz->sampbuf[2][j] = (float)(dz->sampbuf[0][n] * multiplier0);
					dz->sampbuf[2][k] = dz->sampbuf[1][n];
				}
			}
		} else {
			for(n=0,j=0,k=1;n<dz->ssampsread;n++,j+=2,k+=2) {
				dz->sampbuf[2][j] = dz->sampbuf[0][n];
				dz->sampbuf[2][k] = dz->sampbuf[1][n];
			}
		}
		if(dz->ssampsread > 0) {
			if((exit_status = write_samps(dz->sampbuf[2],dz->ssampsread * 2,dz))<0) {
				unlink_temp_files(2,ofd0,ofd1,dz);
				return(exit_status);
			}
		}
		total_samps_to_read -= dz->ssampsread;
	}
										/* DELETE TEMPORARY OUTFILES */
	unlink_temp_files(2,ofd0,ofd1,dz);
	return FINISHED;
}

/**************************** INIT_BRKPNTS ********************************/

int init_brkpnts(long *sampno,double *starttime,double *gain,double **endbrk,double **nextbrk,
double *nextgain,double *gain_step,double *gain_incr,long *nextbrk_sampno,int paramno,int brksize,dataptr dz)
{
	double nexttime, time_from_start;
	*endbrk    = dz->parray[paramno] + ((brksize-1) * 2);
	*sampno    = 0;
	*starttime = dz->parray[paramno][0];
	*gain      = dz->parray[paramno][1];
	*nextbrk   = dz->parray[paramno] + 2;
	nexttime   = **nextbrk;
	*nextgain  = *((*nextbrk) + 1);
	*gain_step = *nextgain - *gain;
	time_from_start = nexttime - *starttime;
	if((*nextbrk_sampno  = (long)round(time_from_start * (double)dz->infile->srate))<0) {
		sprintf(errstr,"Impossible brkpoint time: (%.2lf secs)\n",time_from_start);
		return(PROGRAM_ERROR);
	}
	*gain_incr = (*gain_step)/(double)(*nextbrk_sampno);
	return(FINISHED);
}

/**************************** ADVANCE_BRKPNTS ******************************/

int advance_brkpnts(double starttime,double *gain,double *endbrk,double **nextbrk,
double *nextgain,double *gain_step,double *gain_incr,long *nextbrk_sampno,dataptr dz)
{
	double  nexttime, time_from_start;
	long   lastbrk_sampno, sampdist;
	if(*nextbrk!=endbrk) {
		*nextbrk   += 2;
		*gain      = *nextgain;
		nexttime   = **nextbrk;
		*nextgain  = *((*nextbrk) + 1);
		*gain_step = *nextgain - *gain;
		lastbrk_sampno = *nextbrk_sampno;
		time_from_start = nexttime - starttime;
		if((*nextbrk_sampno  = (long)round(time_from_start * dz->infile->srate))<0) {
			sprintf(errstr,"Impossible brkpoint time: (%.2lf secs)\n",time_from_start);
			return(PROGRAM_ERROR);
		}
		sampdist   = *nextbrk_sampno - lastbrk_sampno;
		*gain_incr = (*gain_step)/(double)sampdist;
	}
	return(FINISHED);
}

/*********************** SIMPLE_ENVEL_PROCESSING *********************************/

int simple_envel_processing
(long *samps_left_to_process,double starttime,double *endbrk,double **nextbrk,double *nextgain,
long *sampno,double *gain,double *gain_step,double *gain_incr,long *nextbrk_sampno,int chan,float *maxobuf,dataptr dz)
{
	int exit_status;
	while(*samps_left_to_process > 0) {
		if((exit_status = do_mono_envelope
		(dz->buflen,starttime,endbrk,nextbrk,nextgain,sampno,gain,gain_step,gain_incr,nextbrk_sampno,maxobuf,dz))<0)
			return(exit_status);
		if((exit_status = write_samps(dz->sampbuf[1],dz->buflen,dz))<0)
			return(exit_status);
		if((exit_status = do_simple_read(samps_left_to_process,dz->buflen,chan,dz))<0)
			return(exit_status);
	}
	if((exit_status = do_mono_envelope
	(dz->ssampsread,starttime,endbrk,nextbrk,nextgain,sampno,gain,gain_step,gain_incr,nextbrk_sampno,maxobuf,dz))<0)
		return(exit_status);
	if(dz->ssampsread > 0)
		return write_samps(dz->sampbuf[1],dz->ssampsread,dz);
	return FINISHED;
}

/************************* DO_MONO_ENVELOPE *********************************/

int do_mono_envelope
(long sampcnt,double starttime,double *endbrk,double **nextbrk,double *nextgain,
long *sampno,double *gain,double *gain_step,double *gain_incr,long *nextbrk_sampno,float *maxobuf,dataptr dz)
{
	int    exit_status;
	long   n;
	long   endsampno, this_endsamp, this_sampcnt;
	float  *ibuf = dz->sampbuf[0];
	float  *obuf = dz->sampbuf[1];
	float  *obufend = obuf + dz->buflen;
	int    quit = FALSE, change;
	double this_gain      = *gain;
	double this_gain_incr = *gain_incr;
	double this_gain_step = *gain_step;
	endsampno = *sampno + sampcnt;				/* 1 */
	if(sampcnt > dz->buflen) {
		sprintf(errstr,"Buffering anomaly: do_mono_envelope()\n");
		return(PROGRAM_ERROR);
	}
	while(quit==FALSE) {						
		if(*nextbrk_sampno <= endsampno) {		/* 2 */
			change = TRUE;
			this_endsamp = *nextbrk_sampno;		/* 3 */
			if(*nextbrk_sampno==endsampno)		
				quit = TRUE;					/* 4 */
		} else {
			change = FALSE;						/* 5 */
			this_endsamp = endsampno;
			quit = TRUE;
		}
		this_sampcnt = this_endsamp - *sampno;	/* 6 */
		if(obuf + this_sampcnt > obufend) {
			sprintf(errstr,"array overrun: do_mono_envelope()\n");
			return(PROGRAM_ERROR);
		}
		if(flteq(this_gain,1.0) && flteq(this_gain_step,0.0)) {
			obuf += this_sampcnt;				
			ibuf += this_sampcnt;				/* no modification of sound */
		}
		else {
			
			for(n=0;n<this_sampcnt;n++) {		/* 7 */
				this_gain += this_gain_incr;
				*obuf = (float)(*ibuf * this_gain);
				*maxobuf = max(*maxobuf,*obuf);
				obuf++;
				ibuf++;
			}
		}
		*sampno += this_sampcnt;				/* 8 */
		if(change) {							/* 9 */
			if((exit_status = advance_brkpnts
			(starttime,gain,endbrk,nextbrk,nextgain,gain_step,gain_incr,nextbrk_sampno,dz))<0)
				return(exit_status);
			this_gain = *gain;
			this_gain_incr = *gain_incr;
			this_gain_step = *gain_step;
		}
	}
	*gain = this_gain;							/* 10 */
	return(FINISHED);
}

/************************* READ_SSAMPS ******************************/

int read_ssamps(long samps_to_read,long *samps_left_to_process,int chan,dataptr dz)
{
	long n, m;
	if((dz->ssampsread = fgetfbufEx(dz->sampbuf[2],samps_to_read,dz->ifd[0],0))<0) {
		sprintf(errstr,"Can't read samps from input soundfile: read_ssamps()\n");
		return(SYSTEM_ERROR);
	}
	if(dz->ssampsread!=samps_to_read) {
		sprintf(errstr,"Error in buffering arithmetic: read_ssamps()\n");
		return(PROGRAM_ERROR);
	}
	*samps_left_to_process -= dz->ssampsread;
	dz->ssampsread /= 2;
	for(n=0,m=chan;n<dz->ssampsread;n++,m+=2)
		dz->sampbuf[0][n] = dz->sampbuf[2][m];
	return(FINISHED);
}

/******************************** DO_SIMPLE_READ **********************************/

int do_simple_read(long *samps_left_to_process,long buflen,int chan,dataptr dz)
{
	int exit_status;
	if(*samps_left_to_process < buflen * 2)
		exit_status = read_ssamps(*samps_left_to_process,samps_left_to_process,chan,dz);
	else
		exit_status = read_ssamps(buflen * 2,samps_left_to_process,chan,dz);
	return(exit_status);
}

/**************************** OTHER_READ_SAMPS *****************************/

int other_read_samps(long samps_to_read,int fileid,int bufno,dataptr dz)
{
	if((dz->ssampsread = fgetfbufEx(dz->sampbuf[bufno],samps_to_read,fileid,0)) < 0) {
		sprintf(errstr,"Can't read samps from input soundfile: other_read_samps()\n");
		return(SYSTEM_ERROR);
	}
	if(dz->ssampsread < samps_to_read) {
		sprintf(errstr,"Error in buffering arithmetic: other_read_samps()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}
							  
/**************************** UNLINK_TEMP_FILES *****************************/

void unlink_temp_files(int type,int ofd0, int ofd1,dataptr dz)
{
	switch(type) {
	case(1):
		if(sndunlink(ofd1) < 0)
			fprintf(stdout, "ERROR: Can't set temporary output soundfile for deletion.\n");
		if(sndcloseEx(ofd1) < 0)
			fprintf(stdout, "WARNING: Can't close temporary output soundfile.\n");
		else
			ofd1 = -1;
		dz->ofd = ofd0;		/* closed at finish() */
		break;
	case(2):
		if(sndunlink(ofd0) < 0)
			fprintf(stdout, "ERROR: Can't set 1st temporary output soundfile for deletion.\n");
		if(sndcloseEx(ofd0) < 0)
			fprintf(stdout, "WARNING: Can't close 1st temporary output soundfile.\n");
		else
			ofd0 = -1;
		if(sndunlink(ofd1) < 0)
			fprintf(stdout, "ERROR: Can't set 2nd temporary output soundfile for deletion.\n");
		if(sndcloseEx(ofd1) < 0)
			fprintf(stdout, "WARNING: Can't close 2nd temporary output soundfile.\n");
		else
			ofd1 = -1;
		break;
	}
}

/************************************ FINDPAN *******************************/

int findpan(dataptr dz)
{  
	int exit_status;
	long n, m, k, samppos, secpos;
	double panpos;
	float *inbuf = dz->sampbuf[0];
	float maxL = 0.0, maxR = 0.0;
	if(dz->param[PAN_PAN] > 0.0) {		/* Assesses One Sector at given time */
		samppos =  round(dz->param[PAN_PAN] * dz->infile->srate) * dz->infile->channels;
		secpos = samppos;
		if (secpos >= dz->insams[0]) {
			if((secpos -= F_SECSIZE) < 0)
				secpos = 0;
		}
		secpos = (secpos/F_SECSIZE) * F_SECSIZE;
		sndseekEx(dz->ifd[0],secpos,0);
		if((exit_status = read_samps(inbuf,dz))<0) {
			sprintf(errstr,"Failed to read data from sndfile.\n");
			return(DATA_ERROR);
		}
		k = min(dz->ssampsread,F_SECSIZE);
		for(n=0,m=1;n<k;n+=2,m+=2) {
			maxL = (float)max(fabs(inbuf[n]),maxL);
			maxR = (float)max(fabs(inbuf[m]),maxR);
		}
	} else {							/* Assesses whole file */
		do {
			if((exit_status = read_samps(inbuf,dz))<0) {
				sprintf(errstr,"Failed to read data from sndfile.\n");
				return(DATA_ERROR);
			}
			for(n=0,m=1;n<dz->ssampsread;n+=2,m+=2) {
				maxL = (float)max(fabs(inbuf[n]),maxL);
				maxR = (float)max(fabs(inbuf[m]),maxR);
			}
		} while(dz->samps_left > 0);
	}
	if(maxL < SMP_FLTERR && maxR < SMP_FLTERR)
		fprintf(stdout,"INFO: levels are ZERO\n");
	else {				
		panpos = maxR/(maxR + maxL);
		panpos = (panpos * 2.0) - 1.0;
		fprintf(stdout,"INFO: levels LEFT %.3lf RIGHT %.3lf : Panning position is %.2lf\n",maxL,maxR,panpos);
	}
	fflush(stdout);
	return(FINISHED);
}


