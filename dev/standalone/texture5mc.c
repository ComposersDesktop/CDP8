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
    License aint with the CDP System; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
 *
 */



/* floatsam version */
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>			/*RWD*/
#include <structures.h>
#include <tkglobals.h>
#include <globcon.h>
#include <processno.h>
#include <modeno.h>
#include <arrays.h>
#include <texture.h>
#include <cdpmain.h>
#include <osbind.h>
#include <standalone.h>

#include <sfsys.h>
#ifdef _DEBUG
#include <assert.h>
#endif

#if defined unix || defined __GNUC__
#define round(x) lround((x))
#endif
#ifndef HUGE
#define HUGE 3.40282347e+38F
#endif

#define permcnt rampbrksize

static int setup_splice(int st_splicecnt,double **splicebuf);
static int generate_origdur_and_frq(double **origdur,double **origfrq,dataptr dz);
static int init_soundout_set(sndoutmchptr *sndout);
static int replace_tset_by_soundout_set(noteptr thisnote,sndoutmchptr *sndout,
			double *origdur,double *origfrq,double  mindur,int st_splicecnt,dataptr dz);
static int convert_tsetnote_to_sndout_item_multichan(noteptr thisnote,sndoutmchptr sndout,
			double *origdur,double *origfrq,double mindur,int st_splicecnt,int *warning_set,dataptr dz);
static int generate_previous_sndout(sndoutmchptr *sndout);
static int get_thisdur(double *thisdur,noteptr thisnote,int thisins,
			double *origdur,double inv_trnsp,double mindur,dataptr dz);
static int get_stereospace_compensation(double position,double *compensate);

static int do_mix(sndoutmchptr sndout,double *splicebuf,dataptr dz);
static int check_sequencing(sndoutmchptr sndout);
static int add_samples_to_outbuf_from_inbuf(int *out_of_insamples,int * max_samp_written,
			sndoutmchptr sndout,double *splicebuf,int splicelen,/*int*/float *lbuf,int obuflen,dataptr dz);
static void unlink_sndoutptr_at_start_of_list(sndoutmchptr sndout);
static void unlink_sndoutptr(sndoutmchptr sndout);
static double get_interpd_value(unsigned int here,double ibufpos,sndoutmchptr sndout);
static int  free_first_sndout(sndoutmchptr *sndout);

//TW UPDATE 2002: to make texture from stereo sources (updated to flotsams)
static int add_stereo_samples_to_outbuf_from_inbuf
(int *out_of_insamples,int *max_samp_written,sndoutmchptr sndout,double *splicebuf,int splicelen,float *lbuffer,int obuflen,dataptr dz);
static double get_interpd_value_stereo(unsigned int here,double ibufpos,double *chanval2,sndoutmchptr sndout);
static int chanperm(int lastperm,dataptr dz);
static void permute_chunks(dataptr dz);
static void insert(int n,int t,dataptr dz);
static void prefix(int n,dataptr dz);
static void shuflup(int k,dataptr dz);


/**************************** PRODUCE_TEXTURE_SOUND *****************************/

int produce_texture_sound(dataptr dz)
{
	int exit_status;
	/*insamptr  *insound = dz->tex->insnd;*/
	motifptr  tset;
	sndoutmchptr sndout;
	noteptr   thisnote;
	double *origdur, *origfrq, *splicebuf;
	double mindur;
	int st_splicecnt;
	tset     = dz->tex->timeset;
	thisnote = tset->firstnote;
	mindur = (dz->frametime + TEXTURE_SAFETY) * MS_TO_SECS;
	st_splicecnt = round((dz->frametime * MS_TO_SECS) * dz->infile->srate);
	if((exit_status = setup_splice(st_splicecnt,&splicebuf))<0)
		return(exit_status);
	if((exit_status = generate_origdur_and_frq(&origdur,&origfrq,dz))<0)
		return(exit_status);
	if((exit_status = init_soundout_set(&sndout))<0)
		return(exit_status);
	if((exit_status = replace_tset_by_soundout_set(thisnote,&sndout,origdur,origfrq,mindur,st_splicecnt,dz))<0)
		return(exit_status);
	free(origdur);
	free(origfrq);
	return do_mix(sndout,splicebuf,dz);
} 

/************************** SETUP_SPLICE ************************/

int setup_splice(int st_splicecnt,double **splicebuf)
{
	int n, m;
	if((*splicebuf = (double *)malloc(st_splicecnt * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for splice buffer.\n");
		return(MEMORY_ERROR);
	}
	for(m=0,n=st_splicecnt-1; n>=0; n--,m++)
		(*splicebuf)[m] = (double)n/(double)st_splicecnt;
	return(FINISHED);
}

/************************** GENERATE_ORIGDUR_AND_FRQ ************************/

int generate_origdur_and_frq(double **origdur,double **origfrq,dataptr dz)
{
	int n;
	if((*origdur = (double *)malloc(dz->infilecnt * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store original durations.\n");
		return(MEMORY_ERROR);
	}
	if((*origfrq = (double *)malloc(dz->infilecnt * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store original frqs.\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n <dz->infilecnt;n++) {
//TW UPDATE: Now allowing stereo sources to be used to make texture
		(*origdur)[n] = (double)dz->insams[n]/(double)dz->infile->channels/(double)dz->infile->srate;
		(*origfrq)[n] = miditohz(((dz->tex->insnd)[n])->pitch);
	}
	return(FINISHED);
}

/************************** INIT_SOUNDOUT_SET ************************/

int init_soundout_set(sndoutmchptr *sndout)
{
	if((*sndout = (sndoutmchptr)malloc(sizeof(struct soundoutmchan)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for soundoutmchan pointers.\n");
		return(MEMORY_ERROR);
	}
	(*sndout)->last = (sndoutmchptr)0;
	(*sndout)->next = (sndoutmchptr)0;
	return(FINISHED);
}

/************************** REPLACE_TSET_BY_SOUNDOUT_SET ************************/

int replace_tset_by_soundout_set
(noteptr thisnote,sndoutmchptr *sndout,double *origdur,double *origfrq,double  mindur,int st_splicecnt,dataptr dz)
{
	int exit_status;
	int warning_set = FALSE;
	while((thisnote->next)!=(noteptr)0)	/* go to end of tset */
		thisnote = thisnote->next;
	dz->permcnt = dz->iparam[TEXTURE_OUTCHANS];
	while(thisnote!=(noteptr)0) {
		if((exit_status = convert_tsetnote_to_sndout_item_multichan
		(thisnote,*sndout,origdur,origfrq,mindur,st_splicecnt,&warning_set,dz))<0)
			return(exit_status);
		if((exit_status = generate_previous_sndout(sndout))<0)
			return(exit_status);
		if(thisnote->last!=NULL) {
			thisnote = thisnote->last;		/* free tset from end backwards */
			free(thisnote->next);	
		} else {
			free(thisnote);
			if((exit_status = free_first_sndout(sndout))<0)	
				return(exit_status);
			thisnote = NULL;
		}
	}
	return(FINISHED);
}

/************************** CONVERT_TSETNOTE_TO_SNDOUT_ITEM ************************/

//TW "STEREO" defined in globcon.h
#define MAXGAIN (1.0)

int convert_tsetnote_to_sndout_item_multichan
(noteptr thisnote,sndoutmchptr sndout,double *origdur,double *origfrq,double mindur,int st_splicecnt,int *warning_set,dataptr dz)
{
	int exit_status, special_case = 0;
	int thisins, outchanbase, lastperm = dz->iparam[TEXTURE_OUTCHANS] - 1; 
	unsigned int insampcnt, st_osampcnt;
	double stereopos = 0.0, indur;
	double thisfrq,thisdur,trnspstep,inv_trnsp,thisamp,thispos,compensate = 1.0;
	double *pos = NULL;
	int *lmost = NULL;
	if(dz->vflag[4]) {
		lmost  = dz->iparray[dz->iarray_cnt-1];
		pos = dz->parray[dz->array_cnt-1];
	}
	thisins   = thisnote->instr;
    thisfrq   = miditohz((double)thisnote->pitch);
	trnspstep = thisfrq/origfrq[thisins];
	inv_trnsp = 1.0/trnspstep;
	if((exit_status = get_thisdur(&thisdur,thisnote,thisins,origdur,inv_trnsp,mindur,dz))<0)
		return(exit_status);
	indur       = thisdur * trnspstep;
	insampcnt   = min(dz->insams[thisins]/dz->infile->channels,round(indur * (double)dz->infile->srate));		
	st_osampcnt = round((double)insampcnt * inv_trnsp);		
	if(st_osampcnt <= (unsigned int)st_splicecnt) {
		sprintf(errstr,"Error in samplecount calculations: convert_tsetnote_to_sndout_item_multichan()\n");
		return(PROGRAM_ERROR);
	}
	thisamp   = thisnote->amp/(double)MIDITOP;
	thispos   = thisnote->spacepos;
	if(dz->vflag[4]) {
		sndout->lchan = lmost[thisins]; 
		sndout->rchan = sndout->lchan + 2;
		stereopos = pos[thisins];
	} else if((dz->brksize[TEXTURE_SPRD] == 0) && (dz->param[TEXTURE_SPRD] == 0.0)) {
		special_case = 1;
	/* If spread param is set to 1.0 (max) and not a brkfile, uses a perm of no of loudspeakers */
	/* so each event is AT a different loudspeaker, and never between */
		if(dz->infile->channels == 2) {		//	In stereo case, pan between permuted chans
			outchanbase = chanperm(lastperm,dz);
			thispos = outchanbase;
			sndout->lchan = (int)floor(thispos);
			outchanbase = chanperm(lastperm,dz);
			thispos = outchanbase;
			sndout->rchan = (int)floor(thispos);
		} else {							//	In mono case, perm positions in lspkrs (no inter-lspkr positions)
			outchanbase = chanperm(lastperm,dz);
			thispos = outchanbase;
			sndout->lchan = (int)floor(thispos);
			sndout->rchan = (sndout->lchan + 1);
		}
	} else {
		sndout->lchan = (int)floor(thispos);
		sndout->rchan = (sndout->lchan + 1);
	}
	sndout->lchan %= dz->iparam[TEXTURE_OUTCHANS];
	sndout->rchan %= dz->iparam[TEXTURE_OUTCHANS];
	if(!special_case) {
		if(!dz->vflag[4])
			stereopos = thispos - (double)sndout->lchan;
		if((exit_status = get_stereospace_compensation(stereopos,&compensate))<0)
			return(exit_status);
	}
    if(thisamp > MAXGAIN) {
#ifdef _DEBUG
		assert(thisamp <= MAXGAIN);
#endif
		if(!(*warning_set)) {
			fprintf(stdout,"WARNING: one or more events exceed max level. Adjusted.\n");
			fflush(stdout);
			*warning_set = TRUE;
		}
		thisamp = MAXGAIN;
    }
	sndout->inbuf       = ((dz->tex->insnd)[thisins])->buffer;
	sndout->ibufcnt     = insampcnt;
	sndout->st_sstttime = round(thisnote->ntime * dz->infile->srate);
	sndout->st_sendtime = sndout->st_sstttime + st_osampcnt; /* Redundant variable: might need if func revised */
	sndout->st_splicpos = sndout->st_sendtime - st_splicecnt;
 	sndout->ibufpos 	= 0.0;
	sndout->step		= trnspstep;
//TW UPDATE: in stereo-input case, only 1 gain value used, as no spatialisation takes place
	if(dz->infile->channels==STEREO) {
		sndout->lgain 		= thisamp;
	} else if(special_case) {
		sndout->lgain 		= thisamp;
		sndout->rgain 		= 0.0;
	} else {
		sndout->lgain 		= thisamp * (1.0 - stereopos) * compensate;
		sndout->rgain 		= thisamp * stereopos * compensate;
	}
	return(FINISHED);
}

/************************** GENERATE_PREVIOUS_SNDOUT ************************/

int generate_previous_sndout(sndoutmchptr *sndout)
{
	sndoutmchptr new;
	if((new = (sndoutmchptr)malloc(sizeof(struct soundoutmchan)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for new soundoutmchan pointers.\n");
		return(MEMORY_ERROR);
	}
	new->last     = (sndoutmchptr)0;
	new->next     = *sndout;
	(*sndout)->last = new;
	*sndout = new;
	return(FINISHED);
}

/************************** FREE_FIRST_SNDOUT ************************/

int free_first_sndout(sndoutmchptr *sndout)
{
	if((*sndout = (*sndout)->next)==NULL) {
		sprintf(errstr,"Problem in free_first_sndout()\n");
		return(PROGRAM_ERROR);
	}
	free((*sndout)->last);
	(*sndout)->last = NULL;
	return(FINISHED);
}

/************************** GET_THISDUR ************************/

int get_thisdur
(double *thisdur,noteptr thisnote,int thisins,double *origdur,double inv_trnsp,double mindur,dataptr dz)
{
	double transposed_dur = origdur[thisins] * inv_trnsp;
	*thisdur   = thisnote->dur;
	if(*thisdur>transposed_dur)			
	    *thisdur = transposed_dur;
	else if(dz->vflag[TEX_IGNORE_DUR])
		*thisdur = transposed_dur;
    if(*thisdur<mindur) {
		sprintf(errstr,
		"too short dur generated (%lf : mindur = %lf): get_thisdur()\n",*thisdur,mindur);
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/************************** GET_STEREOSPACE_COMPENSATION ************************
 *
 * 1) 	comp(ensation) is a factor which compensates for the apparent
 *		loss of perceived amplitude as a sound moves out of the absolute
 *		left or absolute right position (sound from a single loudspeaker)
 *		by simple linear interpolation.	
 *		At first we define comp to vary LINERALY from 0.0 at L or R,
 *		to 1.0 in centre.
 * 2)	NONLIN makes this varation nonlinear, but smooth at the centre, and
 *		still between 0.0 and 1.0
 * 3)	As a signal of fixed perceived level moves from left to right (or v.v.)
 *		we wish to boost the amplitude from each loudspeaker somewhat,
 *		using a curve of the shape now defined.
 *		The maximum possible amplitude is 1.0. To define how much we should
 *		boost amplitude at any point on the L-R axis, we need to define
 *		a ratio (MAXBOOST) with which to multiply the signal.
 *		Multiplying comp-curve by this, defines a curve of amplitude-boosting values.
 * 4) 	1.0 is now added to this curve, to give a multiplier of amplitude at
 *		each point along the left-right axis.
 *		And MAXBOOST is subtracted, so that the level 
 *		from each loudspeaker when at maximum boost cannot exceed
 *		MAXGAIN (1.0).
 */

#define NONLIN   (0.5)
#define MAXBOOST (0.25)

int get_stereospace_compensation(double position,double *compensate)
{
	double comp = 1.0 - (fabs((position * (double)2.0) - (double)1.0)); 	/* 9a */
    comp = pow(comp,NONLIN);			/* 9b */
    comp *= MAXBOOST;					/* 9c */
    comp += (1.0 - MAXBOOST);			/* 9d */
	*compensate = comp;
	return(FINISHED);
}

/************************** DO_MIX ************************
 *
 * 1)	Position, in multichan-samples, of start-of-splice in this-sndout-struct =  
 *			sndout->st_splicpos
 *		Position in actual-samples of ditto = 
 *			 sndout->st_splicpos * ochans
 *		Position relative to start of current buffer = 
 *			(sndout->st_splicpos * ochans) - (obufcnt * obuflen);
 *		Same position expressed relative to start-of-splice (so that value 0 = start_of_splice) = 
 *			-((sndout->st_splicpos * ochans) - (obufcnt * obuflen));
 */

int do_mix(sndoutmchptr sndout,double *splicebuf,dataptr dz)
{
	int exit_status;
	int out_of_insamples, ochans = dz->iparam[TEXTURE_OUTCHANS];
	/*unsigned long totaltime = 0;*/
	int max_samp_written, obuflen;
	unsigned int this_sampend, obufcnt = 0;
	int splicelen = round((dz->frametime * MS_TO_SECS) * dz->infile->srate) * ochans;
	sndoutmchptr startsnd = sndout, endsnd = sndout;
	/*int*/float *lbuf;
//TW UPDATE handles stereo input files
	int is_stereo = 0;
	if(dz->infile->channels == STEREO)
		is_stereo = 1;
//TW MOVED from texprepro
	dz->infile->channels = ochans;	/* for output time calculations */
	if((exit_status = create_sized_outfile(dz->outfilename,dz))<0)
		return(exit_status);
	obuflen = dz->buflen;
	if(is_stereo)
		obuflen /= 2;
	obuflen *= dz->iparam[TEXTURE_OUTCHANS];
	if((lbuf = (float *)malloc(obuflen * sizeof(float)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for mixing buffers.\n");
		return(MEMORY_ERROR);
	}
	dz->sbufptr[0] = dz->sampbuf[0];
/* <--SAFETY CHECK */
	if((exit_status = check_sequencing(sndout))<0)
		return(exit_status);
/* SAFETY CHECK--> */
	while(startsnd != NULL) {										/* until we reach end of event list */
		this_sampend = (obufcnt+1) * obuflen;		/* get absolute samplecnt of end of current buf */
													 /* look for NEW events starting during this outbuf */
		if(sndout!=NULL) {
			while(sndout->st_sstttime * ochans < this_sampend) {	 
				sndout->obufpos  = (sndout->st_sstttime * ochans) % obuflen;/* obufptr for new event */
				sndout->st_splicpos = (sndout->st_splicpos * ochans)-(obufcnt * obuflen);/* NOTE1 abv*/
				sndout->st_splicpos = -(sndout->st_splicpos);							/* NOTE 1 above */
				sndout = sndout->next;
				endsnd = sndout;					  /* update the endmarker of the active events list */
				if(endsnd == NULL)										/* if all events finished,break */
					break;
			}
		}
		sndout = startsnd;										 /* set start to 1st event still active */

		memset((char *)lbuf,0,obuflen * sizeof(float));					     /* empty output buffer */
		if(sndout == endsnd) {					    /* If there are NO active events during this buffer */
			if(endsnd != NULL)					  		 		 /* and we are not at end of event list */
				max_samp_written = obuflen;						  /* setup to write an empty buffer */
			else {
				sprintf(errstr,"Error in loop logic: do_mix()\n");
				return(PROGRAM_ERROR);
			}
		} else
			max_samp_written = 0;
		while(sndout!= endsnd) {									      /* look in all active buffers */
			out_of_insamples = FALSE;

//TW UPDATE: handles stereo input
			if(is_stereo) {
				if((exit_status = add_stereo_samples_to_outbuf_from_inbuf
				(&out_of_insamples,&max_samp_written,sndout,splicebuf,splicelen,lbuf,obuflen,dz))<0)
					return(exit_status);
			} else {
				if((exit_status = add_samples_to_outbuf_from_inbuf
				(&out_of_insamples,&max_samp_written,sndout,splicebuf,splicelen,lbuf,obuflen,dz))<0)
					return(exit_status);
			}
			if(out_of_insamples) { 			   					   /* if inbuf exhausted for THIS event */

				if(sndout->next==NULL) {								   	   	   /* if at end of list */
					if(sndout->last==NULL) { 		   /* if this is definitively the last active event */
						 free(sndout);									  					 /* free it */	   
						 startsnd = NULL;			 /* and set ptr to NULL, so we dropout of outerloop */
						 break;
					} else {					 			 /* BUT if some previous event still active */
						sndout = sndout->last;							/* unlink final event from list */
					 	free(sndout->next);
						sndout->next = NULL;
						sndout = sndout->next;							  /* move sndout to end of list */
					}
				} else if(sndout==startsnd) {					 /* if this is 1st active event in list */
						sndout   = sndout->next;						   	   /* proceed to next event */
						startsnd = sndout;				/* move start of active list to this next event */
						unlink_sndoutptr_at_start_of_list(sndout->last);
				} else { 								/* else if this is NOT 1st active event in list */
						sndout   = sndout->next;							   /* proceed to next event */
						unlink_sndoutptr(sndout->last);/* unlink the exhausted event from (active) list */
				}
			} else {							/* else there is data remaining in inbuf for THIS event */
				sndout->obufpos = 0;			   		      /* so reset obufpos to start of next obuf */
				sndout->st_splicpos += obuflen;/* reset relativeposition splice-start & nextbufstart */
				sndout = sndout->next;						  			   /* and proceed to next event */
			}
		}
		if(startsnd!=NULL)				  /* If we're not at the end of ALL events, write a full buffer */
			max_samp_written = obuflen;

		if(max_samp_written > 0) {
			dz->process = TEX_MCHAN;	//	Forces correct display in Loom
			if((exit_status = write_samps(lbuf,max_samp_written,dz))<0)
				return(exit_status);
			dz->process = SIMPLE_TEX;
		}
		obufcnt++;
	}
	if(dz->param[TEX_MAXOUT] > F_MAXSAMP) {
		fprintf(stdout,"WARNING: OVERLOAD: suggest attenuation by < %lf\n",
			(((double)F_MAXSAMP/(double)dz->param[TEX_MAXOUT])) * dz->param[TEXTURE_ATTEN]);
		fflush(stdout);
	}
	return(FINISHED);
}

/************************** CHECK_SEQUENCING ************************/

int check_sequencing(sndoutmchptr sndout)
{
	unsigned int lasttime = sndout->st_sstttime;
	unsigned int thistime;
	while(sndout->next != (sndoutmchptr)0) {
		sndout = sndout->next;
		if((thistime = sndout->st_sstttime) < lasttime) {
			sprintf(errstr,"Sequencing anomaly in sndout list: check_sequencing()\n");
			return(PROGRAM_ERROR);
		}
		lasttime = thistime;
	} 
	return(FINISHED);
}

/************************** ADD_SAMPLES_TO_OUTBUF_FROM_INBUF ************************/

int add_samples_to_outbuf_from_inbuf
(int *out_of_insamples,int *max_samp_written,sndoutmchptr sndout,double *splicebuf,int splicelen,float *lbuffer,int obuflen,dataptr dz)
{
	unsigned int thisopos = sndout->obufpos;
	unsigned int here;
	double hereval = 0.0;
	float *lbuf    = lbuffer + thisopos;
	float *lbufend = lbuffer + obuflen;
	int splicpos  = sndout->st_splicpos + thisopos;
	float outval, thismaxoutval;
	int max_set = FALSE, chan;
	while(lbuf < lbufend) {
		if((here = (unsigned int) sndout->ibufpos) >= sndout->ibufcnt) {	/* TRUNCATE : get current pos in inbuf */
			*out_of_insamples = TRUE;
			*max_samp_written = max(*max_samp_written,lbuf - lbuffer);
/* SAFETY CHECK--> */
			if(splicpos < splicelen) {
				sprintf(errstr,"BUM endsplice: add_samples_to_outbuf_from_inbuf()\n"); 
				return(PROGRAM_ERROR);
			}
/* <--SAFETY CHECK */

			max_set = TRUE;
			break;
		}
		hereval = get_interpd_value(here,sndout->ibufpos,sndout);
/* SAFETY FOR ARITHMETIC ROUNDING ERROS CALCULATING START OF SPLICE --> */
		if(splicpos >= splicelen)
			hereval = 0.0;
/* <--SAFETY */

		if(splicpos >= 0)
			hereval *= splicebuf[splicpos/dz->iparam[TEXTURE_OUTCHANS]];
		splicpos += dz->iparam[TEXTURE_OUTCHANS];
		for(chan = 0;chan < dz->iparam[TEXTURE_OUTCHANS];chan ++) {
			if(chan == sndout->lchan) {
				outval =(float)( *lbuf + (hereval * sndout->lgain ));
				if((thismaxoutval = (float) fabs(outval)) > dz->param[TEX_MAXOUT])
					dz->param[TEX_MAXOUT] = thismaxoutval;
				*lbuf  = outval;
			} else if(chan == sndout->rchan) {
				outval = (float)(*lbuf + (hereval * sndout->rgain));
				if((thismaxoutval = (float) fabs(outval)) > dz->param[TEX_MAXOUT])
					dz->param[TEX_MAXOUT] =  thismaxoutval;
				*lbuf = outval;
			}
			lbuf++;
		}
		sndout->ibufpos += sndout->step;
	}
	if(!max_set)
		*max_samp_written = obuflen;
	return(FINISHED);
}

//TW UPDATE: NEW FUNCTIONfor stereo input (updated for flotsams)
/************************** ADD_STEREO_SAMPLES_TO_OUTBUF_FROM_INBUF ************************/

int add_stereo_samples_to_outbuf_from_inbuf
(int *out_of_insamples,int *max_samp_written,sndoutmchptr sndout,double *splicebuf,int splicelen,float *lbuffer,int obuflen,dataptr dz)
{
	unsigned int thisopos = sndout->obufpos;
	unsigned int here;
	double chanval1, chanval2;
	float *lbuf    = lbuffer + thisopos;
	float *lbufend = lbuffer + obuflen;
	int splicpos  = sndout->st_splicpos + thisopos;
	float outval, thismaxoutval;
	int max_set = FALSE,  chan;
	while(lbuf < lbufend) {
		if((here = (unsigned int)sndout->ibufpos)>=sndout->ibufcnt) {	/* TRUNCATE : get current pos in inbuf */
			*out_of_insamples = TRUE;
			*max_samp_written = max(*max_samp_written,lbuf - lbuffer);
			if(splicpos < splicelen) {
				sprintf(errstr,"BUM endsplice: add_samples_to_outbuf_from_inbuf()\n"); 
				return(PROGRAM_ERROR);
			}
			max_set = TRUE;
			break;
		}
		chanval1 = get_interpd_value_stereo(here,sndout->ibufpos,&chanval2,sndout);
	/* SAFETY FOR ARITHMETIC ROUNDING ERROS CALCULATING START OF SPLICE --> */
		if(splicpos >= splicelen) {
			chanval1 = 0.0;
			chanval2 = 0.0;
		}
	/* <--SAFETY */
		if(splicpos >= 0) {
			chanval1 *= splicebuf[splicpos/dz->iparam[TEXTURE_OUTCHANS]];
			chanval2 *= splicebuf[splicpos/dz->iparam[TEXTURE_OUTCHANS]];
		}
		splicpos += dz->iparam[TEXTURE_OUTCHANS];
		for(chan = 0;chan < dz->iparam[TEXTURE_OUTCHANS];chan++) {
			if(chan == sndout->lchan) {
				outval = (float)(*lbuf + (chanval1 * sndout->lgain));
				if((thismaxoutval = (float)fabs(outval))>dz->param[TEX_MAXOUT])
					dz->param[TEX_MAXOUT] = thismaxoutval;
				*lbuf  = outval;
			} else if(chan == sndout->rchan) {
				outval = (float)(*lbuf + (chanval2 * sndout->lgain));	/* only one gain val for stereo inputs: kept in lgain */
				if((thismaxoutval = (float)fabs(outval))>dz->param[TEX_MAXOUT])
					dz->param[TEX_MAXOUT] = thismaxoutval;
				*lbuf = outval;
			}
			lbuf++;
		}
		sndout->ibufpos += sndout->step;
	}
	if(!max_set)
		*max_samp_written = obuflen;
	return(FINISHED);
}

/************************** GET_INTERPD_VALUE ************************/

double get_interpd_value(unsigned int here,double ibufpos,sndoutmchptr sndout)
{
	unsigned int next = here+1; 		/* NB  all inbufs have a wraparound (0val) point at end */
	double frac     = ibufpos - (double)here;
	double hereval  = (double)(sndout->inbuf[here]);
	double nextval  = (double)(sndout->inbuf[next]);
	double diff     = nextval - hereval;
	diff    *= frac;
	return(hereval+diff);
}

//TW UPDATE NEW FUNCTION for stereo input
/************************** GET_INTERPD_VALUE_STEREO ************************/

double get_interpd_value_stereo(unsigned int here,double ibufpos,double *chanval2,sndoutmchptr sndout)
{
	unsigned int next = here+1; 		/* NB  all inbufs have a wraparound (0val) point at end */
	double frac     = ibufpos - (double)here;
	double hereval, nextval, diff;
	double chanval1;
	unsigned int sthere = here * 2, stnext = next * 2;
	hereval  = (double)(sndout->inbuf[sthere]);
	nextval  = (double)(sndout->inbuf[stnext]);
	diff     = (nextval - hereval) * frac;
	chanval1 = hereval+diff;
	sthere++;
	stnext++;
	hereval  = (double)(sndout->inbuf[sthere]);
	nextval  = (double)(sndout->inbuf[stnext]);
	diff     = (nextval - hereval) * frac;
	*chanval2 = hereval+diff;
	return(chanval1);
}

/************************** UNLINK_SNDOUTPTR_AT_START_OF_LIST ************************/

void unlink_sndoutptr_at_start_of_list(sndoutmchptr sndout)
{
	sndout->next->last = (sndoutmchptr)0;
	free(sndout);
}

/************************** UNLINK_SNDOUTPTR ************************/

void unlink_sndoutptr(sndoutmchptr sndout)
{
	sndout->next->last = sndout->last;
	sndout->last->next = sndout->next;
	free(sndout);
}

/************************** CHANPERM ************************/

int chanperm(int lastperm,dataptr dz)
{
	int endperm;
	dz->permcnt++;
	if(dz->permcnt >= dz->iparam[TEXTURE_OUTCHANS]) {
		endperm = dz->iparray[6][lastperm];
		do {
			permute_chunks(dz);
		} while (endperm == dz->iparray[6][0]);
		dz->permcnt = 0;
	}
	return dz->iparray[6][dz->permcnt];
}

/*************************** PERMUTE_CHUNKS ***************************/

void permute_chunks(dataptr dz)
{   
	int n, t;
	for(n=0;n<dz->iparam[TEXTURE_OUTCHANS];n++) {
		t = (int)(drand48() * (double)(n+1));	 /* TRUNCATE */
		if(t==n)
			prefix(n,dz);
		else
			insert(n,t,dz);

	}
}

/****************************** INSERT ****************************/

void insert(int n,int t,dataptr dz)
{   
	shuflup(t+1,dz);
	dz->iparray[6][t+1] = n;
}

/****************************** PREFIX ****************************/

void prefix(int n,dataptr dz)
{   
	shuflup(0,dz);
	dz->iparray[6][0] = n;
}

/****************************** SHUFLUP ****************************/

void shuflup(int k,dataptr dz)
{   
	int n;
	for(n = dz->iparam[TEXTURE_OUTCHANS] - 1; n > k; n--)
		dz->iparray[6][n] = dz->iparray[6][n-1];
}

