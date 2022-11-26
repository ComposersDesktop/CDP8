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
#include <math.h>
#include <float.h>
#include <string.h>
#include <structures.h>
#include <tkglobals.h>
#include <globcon.h>
#include <processno.h>
#include <modeno.h>
#include <arrays.h>
#include <envel.h>
#include <cdpmain.h>

#include <sfsys.h>


#define MARGIN 	(16)

#define IS_CLICK   (1.0/(3.0 * MS_TO_SECS)) /* from zero to full amplitude in 3ms */
                                   		 /* anything faster is a 'click'!!   */

#define MINSPAN (16)					 /* spanning points for creating exponential env */
#define	ATTEN	(3.33333333)			 /* Attenuation for creating exponential env */
#define	ATTENATOR	(1.7)			 	 /* Attenuation for creating exponential env */
#define	STEEP_ATTEN	(2.0)			 	 /* For ATTACK */

static int   realloc_envcreate_tables(int tabsize,dataptr dz);
static int   envnorm(float *env,float *envend);
static int   envceiling(float *env,float *envend);
static int   envduck(float *env,float *envend,dataptr dz);
static int   envrevers(float *env,float *envend);
static int   envexagg(float *env,float *envend,dataptr dz);
static int   envatten(float *env,float *envend,dataptr dz);	
static int   envlift(float *env,float *envend,dataptr dz);	
static int   envtstretch(float **env,float **envend,dataptr dz);
static int   envflatn(float *env,float *envend,dataptr dz);	
static int   envgate(float *env,float *envend,dataptr dz);	
static int   envinvert(float *env,float *envend,dataptr dz);
static int   envexpand(float *env,float *envend,dataptr dz);
static int   envlimit(float *env,float *envend,dataptr dz);	
static int   envkorrug(float *env,float *envend,dataptr dz);
static int   envpeakcnt(float *env,float *envend,dataptr dz);
static int   envtrig(float **env,float **envend,dataptr dz);	
static int	 envshortn(float **env,float **envend,double timestretch);
static int 	 envlenthn(float **env,float **envend,double timestretch);
static float envintpl(double here,double winbase,double base,double next);
static void  brkdeglitch(double *,double **);
static int   gen_trigger_envelope(double **brk, double **brkend,dataptr dz);
static int   gen_reapplied_trigger_envelope(double **brk, double **brkend,dataptr dz);
static int   gatefilt(float *p,float *envend,double gate,int smoothing);
static int   do_simple_gating(float *env,float *envend,dataptr dz);
static int   do_smoothed_gating(float *env,float *envend,int smoothing,dataptr dz);
static void  do_env_invert(float *p,double upratio,double dnratio,dataptr dz);
static void  do_expanding(float *p,float *envend,int *gating,double squeeze,int smoothing,dataptr dz);
static void  digout_trough(float *env,float *envend,float *shadowenv,int del,float *q);
static int   ispeak(float *env,float *envend,float *q,int width);
static int   istrough(float *env,float *envend,float *q,int width);
static int   enlarge_brktable(double **brk,double **brkend, double **q,int *arraysize);
static void  convert_to_on_triggers(double now,double timestep,double rise,int *triggered,double **q);
static int   round_off_brktable(double *q,double **brk,double **brkend,double envlen);
static int   locate_and_size_triggers(float *env,float *envend,double **brk,double **brkend,dataptr dz);
static int   convert_other_brktable_to_envtable(double *brk,double *brkend,float **env,float **envend,dataptr dz);
static float read_a_val_from_brktable(double *brk,double *brkend,double thistime);

/*************************** ENVELOPE_WARP ************************/
 
int envelope_warp(float **env,float **envend,dataptr dz)
{
	int exit_status;
	if(flteq((double)dz->outfile->window_size,0.0)) {
		sprintf(errstr,"dz->outfile->wsize not set: envelope_warp()\n");
		return(PROGRAM_ERROR);
	}
	if(dz->process!=ENV_WARPING && dz->process!=ENV_REPLOTTING && dz->process!=ENV_RESHAPING) {
		sprintf(errstr,"this function not valid with this process: envelope_warp()\n");
		return(PROGRAM_ERROR);
	}
	switch(dz->mode) {
	case(ENV_NORMALISE): 	exit_status = envnorm(*env,*envend);		break;
	case(ENV_REVERSE):   	exit_status = envrevers(*env,*envend);		break;
	case(ENV_EXAGGERATING):	exit_status = envexagg(*env,*envend,dz);	break;
	case(ENV_ATTENUATING): 	exit_status = envatten(*env,*envend,dz);	break;
	case(ENV_LIFTING):	   	exit_status = envlift(*env,*envend,dz);		break;
	case(ENV_TSTRETCHING): 	exit_status = envtstretch(env,envend,dz);	break;
	case(ENV_FLATTENING):  	exit_status = envflatn(*env,*envend,dz);	break;
	case(ENV_GATING):	   	exit_status = envgate(*env,*envend,dz);		break;
	case(ENV_INVERTING):   	exit_status = envinvert(*env,*envend,dz);	break;
	case(ENV_EXPANDING):   	exit_status = envexpand(*env,*envend,dz);	break;
	case(ENV_LIMITING):	  	exit_status = envlimit(*env,*envend,dz);	break;
	case(ENV_CORRUGATING): 	exit_status = envkorrug(*env,*envend,dz);	break;
	case(ENV_TRIGGERING):  	exit_status = envtrig(env,envend,dz);		break;
	case(ENV_CEILING):  	exit_status = envceiling(*env,*envend);		break;
	case(ENV_DUCKED):  		exit_status = envduck(*env,*envend,dz);		break;
	case(ENV_PEAKCNT):  	exit_status = envpeakcnt(*env,*envend,dz);	break;
	default:
		sprintf(errstr,"Unknown case in	envelope_warp()\n");
		return(PROGRAM_ERROR);
	}
	return(exit_status);
}

/*************************** ENVNORM ************************/
 
int envnorm(float *env,float *envend)
{											  
	float *p = env;
	double convertor, maxval = 0.0;
	while(p < envend) {
		maxval = max(*p,maxval);
		p++;
	}
	if(flteq(maxval,0.0)) {
		sprintf(errstr,"Envelope level is effectively zero: cannot normalise.\n");
		return(DATA_ERROR);
	}
	p = env;
	convertor = 1.0/maxval;
	while(p < envend) {
		*p = (float)((*p) * convertor);
		*p = (float)min(*p,1.0);	/* SAFETY */
		p++;
	}
	return(FINISHED);
}

/*************************** ENVCEILING ************************/
 
int envceiling(float *env,float *envend)
{											  
	float *p = env;
	double maxval = 0.0;
	while(p < envend) {
		maxval = max(*p,maxval);
		p++;
	}
	p = env;
	while(p < envend) {
		*p = (float)maxval;
		p++;
	}
	return(FINISHED);
}

/*************************** ENVDUCK ************************/
 
int envduck(float *env,float *envend,dataptr dz)
{											  
	int exit_status;
	float *p = env;
	double thistime = 0.0;
	double timestep = dz->outfile->window_size * MS_TO_SECS;
	switch(dz->process) {
	case(ENV_WARPING):
		while(p < envend) {
			if((exit_status = read_values_from_all_existing_brktables(thistime,dz))<0)
				return(exit_status);
			if(*p > dz->param[ENV_THRESHOLD])
				*p = (float)dz->param[ENV_GATE];
			thistime += timestep;
			p++;
		}
		break;
	case(ENV_RESHAPING):
	case(ENV_REPLOTTING):
		while(p < envend) {
			if((exit_status = read_values_from_all_existing_brktables(thistime,dz))<0)
				return(exit_status);
			if(*p > dz->param[ENV_THRESHOLD])
				*p = (float)dz->param[ENV_GATE];
			else
				*p = 1.0f; 	
			thistime += timestep;
			p++;
		}
		break;
	default:
		sprintf(errstr,"Unknown case in envduck()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/*************************** ENVREVERS ************************/
 
int envrevers(float *env,float *envend)
{											  
	float *p = env, *q = envend - 1;
	float temp;
	while(q > p) {
		temp = *p;
		*p   = *q;
		*q   = temp;
		p++;
		q--;
	}
	return(FINISHED);
}

/*************************** ENVEXAGG ************************/

int envexagg(float *env,float *envend,dataptr dz)
{											  
	int exit_status;
	float  *p = env;
	double thistime = 0.0;
	double timestep = dz->outfile->window_size * MS_TO_SECS;
	if(dz->brk[ENV_EXAG]) {
		while(p<envend) {
			if((exit_status = read_value_from_brktable(thistime,ENV_EXAG,dz))<0)
				return(exit_status);
			*p = (float)pow((double)*p,dz->param[ENV_EXAG]);
			p++;
			thistime += timestep;
		}
	} else {
		while(p<envend) {
			*p = (float)pow((double)*p,dz->param[ENV_EXAG]);
			p++;
		}
	}
	return(FINISHED);
}

/*************************** ENVATTEN *************************/

int envatten(float *env,float *envend,dataptr dz)
{											  
	int exit_status;
	float  *p = env;
	double thistime = 0.0;
	double timestep = dz->outfile->window_size * MS_TO_SECS;
	if(dz->brk[ENV_ATTEN]) {
		while(p<envend) {
			if((exit_status = read_value_from_brktable(thistime,ENV_ATTEN,dz))<0)
				return(exit_status);
			*p = (float)(*p * dz->param[ENV_ATTEN]);
			p++;
			thistime += timestep;
		}
	} else {
		while(p<envend) {
			*p = (float)(*p * dz->param[ENV_ATTEN]);
			p++;
		}
	}
	return(FINISHED);
}

/*************************** ENVLIFT ************************/

int envlift(float *env,float *envend,dataptr dz)
{											  
	int exit_status;
	float *p = env;
	int saturated = 0;
	double thistime = 0.0;
	double timestep = dz->outfile->window_size * MS_TO_SECS;
	if(dz->brk[ENV_ATTEN]) {
		while(p<envend) {
			if((exit_status = read_value_from_brktable(thistime,ENV_LIFT,dz))<0)
				return(exit_status);
			if((*p = (float)(*p + dz->param[ENV_LIFT]))> 1.0) {
				*p = 1.0f;
				saturated++;
			}
			p++;
			thistime += timestep;
		}
	} else {
		while(p<envend) {
			if((*p = (float)(*p + dz->param[ENV_LIFT]))> 1.0) {
				*p = 1.0f;
				saturated++;
			}
			p++;
		}
	}
	if(saturated) {
		fprintf(stdout,"WARNING: Envelope was clipped, but will not distort sounds.\n");
		fflush(stdout);
	}
	return(FINISHED);
}

/**************************** ENVTSTRETCH **************************/

int envtstretch(float **env,float **envend,dataptr dz)
{
	int exit_status;
	double timestretch = dz->param[ENV_TSTRETCH];
	if(dz->brksize[ENV_TSTRETCH]) {
		sprintf(errstr,"Invalid use of brktable: envtstretch()\n");
		return(PROGRAM_ERROR);
	}
	if(timestretch>1.0) {
		if((exit_status = envlenthn(env,envend,timestretch))<0)
			return(exit_status);
	}
	if(timestretch<1.0) {
		if((exit_status = envshortn(env,envend,timestretch))<0)
			return(exit_status);
	}
	return(FINISHED);
}

/**************************** ENVSHORTN **************************/

int envshortn(float **env,float **envend,double timestretch)
{
	int   oldenvlen, newenvlen;
	int    n, m = 0;
	float *nuenv, *p;
	double here, skip, base, winbase, next;
	skip   = 1.0/timestretch;
	oldenvlen = *envend - *env; 

	newenvlen = (int)(((double)oldenvlen * timestretch) + 1.0);  /* rescale & round up */
	newenvlen += MARGIN;
	if((nuenv = (float *)malloc(newenvlen*sizeof(float)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create new envelope array.\n");
		return(MEMORY_ERROR);
	}
	p = nuenv;
	*p++ = **env;
	for(;;) {
		if(++m > newenvlen) {
			sprintf(errstr,"New envelope array bound exceeded in envshortn()\n");
			return(PROGRAM_ERROR);
		}
		here = skip * (double)(m);
		if(here>=(double)(oldenvlen-1)) {
			*p = *(p-1);					   
			newenvlen = p - nuenv;
			free(*env);
			if((*env = (float *)realloc(nuenv,newenvlen*sizeof(float)))==NULL) {
				sprintf(errstr,"envshortn(): 2\n");
				return(MEMORY_ERROR);
			}
			*envend = *env + newenvlen;
			break;
		}
		winbase = floor(here);
		n		= round(winbase);
		base  = (double)*((*env) + n);
		if(n > oldenvlen-1)
			next = base;
		else
			next = (double)*((*env) + n + 1);
		*p++ = envintpl(here,winbase,base,next);
	}
	return(FINISHED);
}


/**************************** ENVLENTHN ***************************/

int envlenthn(float **env,float **envend,double timestretch)
{
	int   oldenvlen, newenvlen;
	int    n = 0, m = 0;
	double here = 0.0, skip;
	float  *nuenv, *p;
	double base, winbase, next;
	skip   = 1.0/timestretch;
	oldenvlen = *envend - *env;
	newenvlen = (int)(((double)oldenvlen * timestretch) + 1.0);
	newenvlen += MARGIN;
	if((nuenv = (float *)malloc(newenvlen * sizeof(float)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create new envelope array.\n");
		return(MEMORY_ERROR);
	}
	p = nuenv;
	next = (double)**env;
	for(;;) {
		base = next;
		winbase = (double)n;
		if(++n > oldenvlen-1) {
			if(!m) {
				sprintf(errstr,"Anomaly in envlenthn()\n");
				return(PROGRAM_ERROR);
			}
			newenvlen = p - nuenv;
			free(*env);
			if((*env = (float *)realloc(nuenv,newenvlen*sizeof(float)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY to reallocate new envelope array.\n");
				return(MEMORY_ERROR);
			}
			*envend = *env + newenvlen;
			break;
		}
		next = (double)*((*env) + n);
		while(here <= (double)n) {
			*p++ = envintpl(here,winbase,base,next);
			here = skip * (double)(++m);
			if(m >= newenvlen) {
				sprintf(errstr,"envlenthn(): New array bounds exceeded\n");
				return(PROGRAM_ERROR);
			}
		}
	}
	return(FINISHED);
}

/*************************** ENVINTPL ****************************/

float envintpl(double here,double winbase,double base,double next)
{
	double tratio = here - winbase;
	double valdif = next - base;
	double val    = base + (tratio * valdif); 
	return((float)val);
}

/*************************** ENVFLATN *************************/

int envflatn(float *env, float *envend, dataptr dz)
{
	int exit_status;
	float *p, *q, *z, *nuenv;
	int envlen = envend - env, ppos, qpos;
	int backovflow, fowdovflow, thisback, n;
	double thistime = 0.0;
	double d, sum;
	double timestep = dz->outfile->window_size * MS_TO_SECS;
	int  maxflatn;
	if(dz->brksize[ENV_FLATN]) {
		if((exit_status = get_maxvalue_in_brktable(&d,ENV_FLATN,dz))<0)
			return(exit_status);
		maxflatn = round(d);
	} else 
		maxflatn = dz->iparam[ENV_FLATN];
	if(maxflatn >= envlen) {
		sprintf(errstr,"Flattening param too large for infile.\n");
		return(DATA_ERROR);
	}
	if((nuenv=(float *)malloc((envlen + maxflatn)*sizeof(float)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create new envelope array.\n");
		return(MEMORY_ERROR);
	}
	backovflow  = maxflatn/2;
	fowdovflow  = maxflatn - backovflow - 1;
	p = env;
	q = nuenv;
	for(n=0;n<backovflow;n++)
		*q++ = *p;
	while(p < envend)
		*q++ = *p++;
	p--;
	for(n=0;n<fowdovflow;n++)
		*q++ = *p;
	p = env;
	q = nuenv;
	if(dz->brksize[ENV_FLATN]) {
		while(p < envend) {
			if((exit_status = read_value_from_brktable(thistime,ENV_EXAG,dz))<0)
				return(exit_status);
			thisback  = dz->iparam[ENV_FLATN]/2;
			ppos = p - env;
			qpos = ppos + backovflow - thisback;
			z = nuenv + qpos;
			sum = 0.0;
			for(n=0;n<dz->iparam[ENV_FLATN];n++)
				sum += *z++;
			sum /= (double)dz->iparam[ENV_FLATN];
			*p++ = (float)sum;
			thistime += timestep;
		}
	} else {
		while(p < envend) {
			z  = q++;
			sum = 0.0;
			for(n=0;n<dz->iparam[ENV_FLATN];n++)
				sum += *z++;
			sum /= (double)dz->iparam[ENV_FLATN];
			*p++ = (float)sum;
		}
	}
	free(nuenv);
	return(FINISHED);
}

/********************************* ENVGATE ****************************/

int envgate(float *env,float *envend,dataptr dz)
{
	int exit_status;
	int smoothing  = dz->iparam[ENV_SMOOTH];
	if(smoothing==0)
		exit_status = do_simple_gating(env,envend,dz);
	else
		exit_status = do_smoothed_gating(env,envend,smoothing,dz);
	return(exit_status);
}

/********************************* DO_SIMPLE_GATING ****************************/

int do_simple_gating(float *env,float *envend,dataptr dz)
{
	int exit_status;
	double thistime = 0.0;
	float *p = env;
	double timestep = dz->outfile->window_size * MS_TO_SECS;
	if(dz->brksize[ENV_GATE]) {
		while(p<envend) {
			if((exit_status = read_value_from_brktable(thistime,ENV_GATE,dz))<0)
				return(exit_status);
			if(*p < dz->param[ENV_GATE])
				*p = 0.0f;
			p++;
			thistime += timestep;
		}
	} else {
		while(p<envend) {
			if(*p < dz->param[ENV_GATE])
				*p = 0.0f;
			p++;
		}
	}
	return(FINISHED);
}

/********************************* DO_SMOOTHED_GATING ****************************/

int do_smoothed_gating(float *env,float *envend,int smoothing,dataptr dz)
{
	int exit_status;
	double thistime = 0.0;
	int    gating = TRUE;
	float  *p = env;
	double timestep = dz->outfile->window_size * MS_TO_SECS;
	if(dz->brksize[ENV_GATE]) {
		while(p<envend) {
			if((exit_status = read_value_from_brktable(thistime,ENV_GATE,dz))<0)
				return(exit_status);
			if((*p >= dz->param[ENV_GATE])                                          /* 1 */
			&& (!gating || (gating && !gatefilt(p,envend,dz->param[ENV_GATE],smoothing)))) {
				gating = FALSE;
			} else {
				*p = 0.0f;                                           /* 2 */
				gating = TRUE;
			}
			p++;
			thistime += timestep;
		}
	} else {
		while(p<envend) {
			if((*p >= dz->param[ENV_GATE])                                          /* 1 */
			&& (!gating || (gating && !gatefilt(p,envend,dz->param[ENV_GATE],smoothing)))) {
				gating = FALSE;
			} else {
				*p = 0.0f;                                           /* 2 */
				gating = TRUE;
			}
			p++;
		}
	}
	return(FINISHED);
}

/****************************** GATEFILT *************************/
 
int gatefilt(float *p,float *envend,double gate,int smoothing)
{
	int this_smoothfactor, windows_remain;
	int n;
	double average = 0.0;
	float  *q;

	if((windows_remain = envend - p) >= smoothing)
		this_smoothfactor = smoothing;
	else
		this_smoothfactor = windows_remain;
	q = p;
	for(n=0;n<this_smoothfactor;n++)
		average += *q++;
	average /= (double)this_smoothfactor;
	if(average<=gate)
		return(TRUE);
	return(FALSE);
}

/*************************** ENVINVERT *************************/

int envinvert(float *env,float *envend,dataptr dz)
{
#define NO_BRK		(0)
#define G_BRK		(1)
#define M_BRK		(2)
#define G_AND_M_BRK	(3)

	int exit_status;
	float  *p;
	double hirange = 0.0, lorange = 0.0, upratio = 0.0, dnratio = 0.0;
	double thistime = 0.0;
	int    thistype;
	double timestep = dz->outfile->window_size * MS_TO_SECS;
	if(dz->brksize[ENV_GATE]) {
		if(dz->brksize[ENV_MIRROR])
			thistype = G_AND_M_BRK;
		else {
			thistype = G_BRK;
			hirange =  1.0 - dz->param[ENV_MIRROR];	
		}
	} else {
		if(dz->brksize[ENV_MIRROR])
			thistype = M_BRK;
		else {
			thistype = NO_BRK;
			hirange = 1.0 - dz->param[ENV_MIRROR];	
			lorange = dz->param[ENV_MIRROR] - dz->param[ENV_GATE];
			upratio = hirange/lorange;
			dnratio = lorange/hirange;
		}
	}
	p = env;
	if(thistype == NO_BRK) {
	 	while(p<envend) {
			do_env_invert(p,upratio,dnratio,dz);
			p++;
		}
	} else {
		while(p<envend) {
			switch(thistype) {
			case(G_BRK):	
				if((exit_status = read_value_from_brktable(thistime,ENV_GATE,dz))<0)
					return(exit_status);
				lorange = dz->param[ENV_MIRROR] - dz->param[ENV_GATE];
				break;
			case(M_BRK):	
				if((exit_status = read_value_from_brktable(thistime,ENV_MIRROR,dz))<0)
					return(exit_status);
 				hirange = 1.0 - dz->param[ENV_MIRROR];	
				lorange = dz->param[ENV_MIRROR] - dz->param[ENV_GATE];
				break;
			case(G_AND_M_BRK):	
				if((exit_status = read_value_from_brktable(thistime,ENV_GATE,dz))<0)
					return(exit_status);
				if((exit_status = read_value_from_brktable(thistime,ENV_MIRROR,dz))<0)
					return(exit_status);
 				hirange = 1.0 - dz->param[ENV_MIRROR];	
				lorange = dz->param[ENV_MIRROR] - dz->param[ENV_GATE];
				break;
			}
			upratio = hirange/lorange;
			dnratio = lorange/hirange;
			do_env_invert(p,upratio,dnratio,dz);
			thistime += timestep;
			p++;
		}
	}
	return(FINISHED);
}

/*************************** DO_ENV_INVERT *************************/

void do_env_invert(float *p,double upratio,double dnratio,dataptr dz)
{
	if(*p<dz->param[ENV_GATE]) {
		*p = 0.0f;
	} else {
		if(*p<=dz->param[ENV_THRESHOLD])
			*p = (float)(((dz->param[ENV_THRESHOLD] - *p) * upratio) + dz->param[ENV_THRESHOLD]);
		else
			*p = (float)(dz->param[ENV_THRESHOLD] - ((*p - dz->param[ENV_THRESHOLD]) * dnratio)); 
	}
}

/**************************** ENVFEXPAND ******************************/

int envexpand(float *env,float *envend,dataptr dz)
{
#define NO_BRK		(0)
#define G_BRK		(1)
#define T_BRK		(2)
#define G_AND_T_BRK	(3)

	int exit_status;
	float  *p;
	double toprange = 0.0, newrange = 0.0, squeeze = 0.0;
	double thistime = 0.0;
	int    gating = TRUE;
	int	   smoothing = dz->iparam[ENV_SMOOTH];
	int    thistype;
	double timestep = dz->outfile->window_size * MS_TO_SECS;
	if(dz->brksize[ENV_GATE]) {
		if(dz->brksize[ENV_THRESHOLD])
			thistype = G_AND_T_BRK;
		else {
			thistype = G_BRK;
			newrange = 1.0 - dz->param[ENV_THRESHOLD];	
		}
	} else {
		toprange = 1.0 - dz->param[ENV_GATE];
		if(dz->brksize[ENV_THRESHOLD])
			thistype = T_BRK;
		else {
			thistype = NO_BRK;
			newrange = 1.0 - dz->param[ENV_THRESHOLD];	
			squeeze  = newrange/toprange;
		}
	}
	p = env;
	if(thistype == NO_BRK) {
		while(p<envend) {
			do_expanding(p,envend,&gating,squeeze,smoothing,dz);
			p++;
		}
	} else {
		while(p<envend) {
			switch(thistype) {
			case(G_AND_T_BRK):	
				if((exit_status = read_value_from_brktable(thistime,ENV_GATE,dz))<0)
					return(exit_status);
				toprange = 1.0 - dz->param[ENV_GATE];
				/* fall thro */
			case(T_BRK):	
				if((exit_status = read_value_from_brktable(thistime,ENV_THRESHOLD,dz))<0)
					return(exit_status);
				newrange = 1.0 - dz->param[ENV_THRESHOLD];	
				break;
			case(G_BRK):	
				if((exit_status = read_value_from_brktable(thistime,ENV_GATE,dz))<0)
					return(exit_status);
				toprange = 1.0 - dz->param[ENV_GATE];
				break;
			}
			squeeze  = newrange/toprange;
			do_expanding(p,envend,&gating,squeeze,smoothing,dz);
			thistime += timestep;
			p++;
		}
	}
	return(FINISHED);
}

/**************************** DO_EXPANDING ****************************/

void do_expanding(float *p,float *envend,int *gating,double squeeze,int smoothing,dataptr dz)
{
	if(smoothing) {
		if((*p > dz->param[ENV_GATE])
		&& (*gating==FALSE || ((*gating==TRUE) && !gatefilt(p,envend,dz->param[ENV_GATE],smoothing)))) {
			*p = (float)(1.0 - ((1.0 - *p) * squeeze));
			*gating = FALSE;
		} else {
			*p = 0.0f;
			*gating = TRUE;
		}
	} else {
		if(*p > dz->param[ENV_GATE])
			*p = (float)(1.0 - ((1.0 - *p) * squeeze));
		else
			*p = 0.0f;
	}		
}

/**************************** ENVLIMIT ******************************/

int envlimit(float *env,float *envend,dataptr dz)
{

#define NO_BRK		(0)
#define TH_BRK		(1)
#define L_BRK		(2)
#define T_AND_L_BRK	(3)

	int exit_status;
	float  *p;
	double x, toprange = 0.0, newrange, squeeze = 0.0;
	double thistime = 0.0;
	int    thistype;
	double timestep = dz->outfile->window_size * MS_TO_SECS;
	if(dz->brksize[ENV_THRESHOLD]) {
		if(dz->brksize[ENV_LIMIT])
			thistype = T_AND_L_BRK;
		else
			thistype = TH_BRK;
	} else {
		toprange = 1.0 - dz->param[ENV_THRESHOLD];
		if(dz->brksize[ENV_LIMIT])
			thistype = L_BRK;
		else {
			thistype = NO_BRK;
			newrange = dz->param[ENV_LIMIT] - dz->param[ENV_THRESHOLD];
			squeeze  = newrange/toprange;
		}
	}
	p = env;
	if(thistype == NO_BRK) {
		while(p<envend) {
			if(*p > dz->param[ENV_THRESHOLD]) {
				x = (*p - dz->param[ENV_THRESHOLD]) * squeeze;
				*p = (float)(dz->param[ENV_THRESHOLD] + x);
			}
			p++;
		}
	} else {
		while(p<envend) {
			switch(thistype) {
			case(L_BRK):	
				if((exit_status = read_value_from_brktable(thistime,ENV_LIMIT,dz))<0)
					return(exit_status);
				newrange = dz->param[ENV_LIMIT] - dz->param[ENV_THRESHOLD];
				squeeze  = newrange/toprange;
				break;
			case(T_AND_L_BRK):	
				if((exit_status = read_value_from_brktable(thistime,ENV_LIMIT,dz))<0)
					return(exit_status);
				/* fall thro */
			case(TH_BRK):	
				if((exit_status = read_value_from_brktable(thistime,ENV_THRESHOLD,dz))<0)
					return(exit_status);
				toprange = 1.0 - dz->param[ENV_THRESHOLD];
				newrange = dz->param[ENV_LIMIT] - dz->param[ENV_THRESHOLD];
				squeeze  = newrange/toprange;
				break;
			}
			if(*p > dz->param[ENV_THRESHOLD]) {
				x = (*p - dz->param[ENV_THRESHOLD]) * squeeze;
				*p = (float)(dz->param[ENV_THRESHOLD] + x);
			}
			thistime += timestep;
			p++;
		}
	}
	return(FINISHED);
}

 /*************************** ENVKORRUG *************************/
 
int envkorrug(float *env,float *envend,dataptr dz)
{
	int exit_status;
 	int upwards;
	double thistime = 0.0;
 	float *p, *q;
	double timestep = dz->outfile->window_size * MS_TO_SECS;
 	float *shadowenv;
	size_t bytelen = (envend - env) * sizeof(float);
	if((shadowenv = (float *)malloc(bytelen))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create shadow envelope array.\n");
		return(MEMORY_ERROR);
	}
	memset((char *)shadowenv,0,(size_t)bytelen);
 	p = env+1;
 	q = env;    
 	if (*p > *q)
 		upwards = TRUE;
 	else
 		upwards = FALSE;
 	while(p < envend) {
		if((exit_status = read_values_from_all_existing_brktables(thistime,dz))<0)
			return(exit_status);
 		if(upwards) {
 			if(*p <= *q) {
 				if(ispeak(env,envend,q,(int)dz->iparam[ENV_PKSRCHWIDTH]))
					*(shadowenv + (q - env)) = 1.0f;	
 				upwards = FALSE;
 			}
 		} else {
 			if(*p > *q)
 				upwards = TRUE;
 		}
		thistime += timestep;
 		p++;
 		q++;
 	}

	thistime = 0.0;
 	p = env+1;
 	q = env;    
 	while(p < envend) {
		if((exit_status = read_values_from_all_existing_brktables(thistime,dz))<0)
			return(exit_status);
 		if(upwards) {
 			if(*p <= *q) {
 				upwards = FALSE;
 			}
 		} else {
 			if(*p > *q) {
 				if(istrough(env,envend,q,(int)dz->iparam[ENV_PKSRCHWIDTH]))
 					digout_trough(env,envend,shadowenv,(int)dz->iparam[ENV_TROFDEL],q);
 				upwards = TRUE;
 			}
 		}
		thistime += timestep;
 		p++;
 		q++;
 	}
	if(dz->process==ENV_WARPING) {
 		p = env;
 		while(p < envend) {
	 		if(*p>0.0)
 				*p = 1.0f;	/* NB: NB: Therefore don't use REPLACE with corrugation, when WARPING !! */
 			p++;
 		}
	}
 	free(shadowenv);
 	return(FINISHED);
 }
 
/*************************** ISTROUGH **********************************/
 
int istrough(float *env,float *envend,float *q,int width)
{
 	int up, down;
 	float *upq, *downq, *r;
 	if(width<2)
 		return(1);
 	down = up = width/2;   
 	if(EVEN(width))
 		down = up - 1;		/* set search p[arams above and below q */
 	downq = q - down;
 	upq   = q + up;
 	upq   = min(envend-1,upq);	/* allow for ends of envelope table */
 	downq = max(env,downq);
 	for(r = downq; r<=upq; r++) {
 		if(*q > *r)
 			return(FALSE);		
 	}
 	return(TRUE);			/* if r is minimum of all in trough, return 1 */
 }
 
/*************************** ISPEAK **********************************/
 
int ispeak(float *env,float *envend,float *q,int width)
{
 	int up, down;
 	float *upq, *downq, *r;
 	if(width<2)
 		return(TRUE);
 	down = up = width/2;   
 	if(EVEN(width))
 		down = up - 1;		/* set search params above and below q */
 	downq = q - down;
 	upq   = q + up;
 	upq   = min(envend-1,upq);	/* allow for ends of envelope table */
 	downq = max(env,downq);
 	for(r = downq; r<=upq; r++) {
 		if(*q < *r)
 			return(FALSE);		
 	}
 	return(TRUE);			/* if r is maximum of all in peak, return 1 */
 }
 
 /***************************** DIGOUT_TROUGH *********************************/
 
void digout_trough(float *env,float *envend,float *shadowenv,int del,float *q)
{
 	float *r, *upq, *downq;
 	int	down, up, herepos, lopos, hipos, thispos;
 	if(del<2) {
 		*q = 0.0f;	/* set single-sector trough to 0.0 */
 		return;
 	}
 	down = up = del/2;   
 	if(EVEN(del))
 		down = up - 1;				  /* set trof_delete limits above and below q */
 	downq = q - down;
 	upq   = q + up;
 	upq   = min(envend-1,upq); 				  /* allow for ends of envelope table */
 	downq = max(env,downq);

	herepos = q-env;
	lopos   = downq - env;				  /* find relative positions in env-table */
	hipos   = upq - env;
								/* search downwards for peaks set in shadow table */
 	for(r = shadowenv + herepos; r>= shadowenv + lopos; r--) {
 		if(*r>FLTERR) {
			thispos = (r+1) - shadowenv;
			downq   = env + thispos;/* & adjust downward delete-limit accordingly */
 			break;									   /* to avoid deleting peaks */
 		}
 	}							  /* search upwards for peaks set in shadow table */
 	for(r = shadowenv + herepos; r<=shadowenv + hipos; r++) {
 		if(*r>FLTERR) {		
			thispos = (r-1) - shadowenv;
			upq   = env + thispos;	/* and adjust upward delete-limit accordingly */
 			break;									   /* to avoid deleting peaks */
 		}
 	}
 	for(r = downq; r<=upq; r++)						 /* delete the defined trough */
		 *r = 0.0f;
 }

/**************************** ENVTRIG *********************************/

int envtrig(float **env,float **envend,dataptr dz)	/* NOTE THIS USES POINTERS TO env AND envend */
{
	int exit_status;
	double *brk, *brkend;
	if((exit_status = locate_and_size_triggers(*env,*envend,&brk,&brkend,dz))<0)
		return(exit_status);
	switch(dz->process) {
	case(ENV_REPLOTTING):
	case(ENV_RESHAPING):
		exit_status = gen_trigger_envelope(&brk,&brkend,dz);
		break;
	case(ENV_WARPING):
		exit_status = gen_reapplied_trigger_envelope(&brk,&brkend,dz);
		break;
	}		
	if(exit_status < 0)
		return(exit_status);
	return convert_other_brktable_to_envtable(brk,brkend,env,envend,dz);
}

/**************************** LOCATE_AND_SIZE_TRIGGERS *******************************/

int locate_and_size_triggers(float *env,float *envend,double **brk,double **brkend,dataptr dz)
{
	int exit_status;
	float *p = env, *end;
	double  *q, rise, mean, now;
	double timestep = dz->outfile->window_size * MS_TO_SECS;
	double envdur;
	int   n, scan, arraysize = BIGARRAY;
	int triggered = FALSE, k;
	double maxrise, thistime = 0.0;
	int look_ahead = 4;
	if((*brk = (double *)malloc(arraysize * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create trigger brktable.\n");
		return(MEMORY_ERROR);
	}
	q = *brk;
	*brkend = *brk + arraysize;
	if((scan = (int)(dz->param[ENV_TRIGDUR]/timestep))<=0) {
		sprintf(errstr,"scansize too small for envelope windows.\n");
		return(GOAL_FAILED);
	}
	end = envend - scan - 1;
	*q++ = 0.0;				
	*q++ = 0.0;             		/*Store an initial brkpnt value of zero */
	n = scan;
	while(p<end) {
		if((exit_status = read_values_from_all_existing_brktables(thistime,dz))<0)
			return(exit_status);
		maxrise = -DBL_MAX;
		for(k=1;k<=scan;k++) {
			rise = *(p+k) - *p;  	/* deduce max and min rise[fall] of envelope across scantime */
			maxrise = max(rise,maxrise);
		}
		rise = maxrise;
		mean = *p + (rise/2.0);		/* calc mean level of rising portion */
		if(mean>dz->param[ENV_GATE] 	  /* deduce mean rate of rise[fall] and compare with triggering rate */
		&& fabs(rise)>=dz->param[ENV_TRIGRISE]) {/* if triggering criteria met */
			if(q+look_ahead >= *brkend) {							
				if((exit_status = enlarge_brktable(brk,brkend,&q,&arraysize))<0)
					return(exit_status);
			}
			now = n * timestep;
			convert_to_on_triggers(now,timestep,rise,&triggered,&q);
			p += scan;                               
			n += scan;
			thistime += timestep * scan;
		} else {                                     
			triggered = FALSE;
			p++;
			n++;
			thistime += timestep;
		}
	}
	envdur = (envend - env) * timestep; /* if end of new brktable not at orig end of envel.. extend */
	if((exit_status = round_off_brktable(q,brk,brkend,envdur))<0)
		return(exit_status);
	return(FINISHED);
}

/******************************* GEN_TRIGGER_ENVELOPE ******************************/

int gen_trigger_envelope(double **brk, double **brkend,dataptr dz)
{
	int exit_status;
	int 	 n, brklen;
	double   next_trigtime, this_trigtime, nuhere, last_outval = 0.0, trigval;
	double   *q, *nubrkptr, *nubrk, *nubrkend;
	double   *trigptr, *trigbrk, *trigbrkend;
	int arraysize = BIGARRAY;
	trigptr = trigbrk = *brk;
	trigbrkend = *brkend;
	if((nubrk = (double *)malloc(arraysize * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create new envelope array.\n");
		return(MEMORY_ERROR);
	}
	nubrkend = nubrk + arraysize;
	nubrkptr = nubrk;
	while(trigptr < trigbrkend) {
		this_trigtime = *trigptr++;
		trigval       = *trigptr++;
		if(trigptr < trigbrkend)
			next_trigtime = *trigptr;
		else
			next_trigtime = DBL_MAX;
		if(flteq(trigval,0.0)) {		/* if the trigger table value is 0.0 put Zero in new table */
			*nubrkptr++ = this_trigtime;
			*nubrkptr++ = 0.0;
		} else {						/* else */
			if(trigval < last_outval)	/* If the new trigger value is less than current value held, */
				trigval = last_outval;	/* set trig val to held value, (else, keep the new value).   */
			q = dz->rampbrk;			/* start copying values from ramp table into new brkpnt table, */
			n = 0;						/* until ramptable ends, OR we reach next brkpnt in trigger table */
			while(n < dz->rampbrksize && (nuhere = this_trigtime + *q++)<next_trigtime) {  
				*nubrkptr++ = nuhere;
				last_outval = trigval  * (*q++);	/* Each rampval is scaled by size of current trigger */
				*nubrkptr++ = last_outval;
				n++;
			}
		}
		if(nubrkptr>=(nubrkend-2)) {
			if((exit_status = enlarge_brktable(&nubrk,&nubrkend,&nubrkptr,&arraysize))<0)
				return(exit_status);
		}
	}
	brkdeglitch(nubrk,&nubrkptr);		/* get rid of too fast amp rises or falls */
	brklen = nubrkptr - nubrk;
	free((char *)(*brk));
	if((*brk=(double *)realloc((char *)nubrk,brklen * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to reallocate new envelope array.\n");
		return(MEMORY_ERROR);
	}
	if((brklen/2) * 2 != brklen) {
		sprintf(errstr,"Anomalous breaktable size in gen_trigger_envelope()\n");
		return(PROGRAM_ERROR);
	}
	*brkend = *brk + brklen;
	return(FINISHED);
}

/**************************** BRKDEGLITCH ****************************
 * Check a breakpoint table for sudden glitches, and smooth them.
 *
 * (1) If the brktable has very sudden rise or fall, current time-point is moved forward a little.
 * (2) If, as a result last time-points moved beyond original endtime, readjust to orig duration.
 */

void brkdeglitch(double *brk,double **brkend)
{
	double *p = brk, *here, *next;
	double rise, risetime, slope, newtime;
	double thistime, thisval, timeshift, timeratio;
	double lasttime = *p++;
	double lastval  = *p++;
	double endtime  = *((*brkend)-2);
	double timediff, valdiff, valstep;
	double click = IS_CLICK;
//	int cnt = 0;
	while(p < *brkend) {
		thistime = *p++;
		thisval  = *p++;
		rise =fabs(thisval-lastval);
		if((slope = rise/(thistime-lasttime))>click) {	/* If the brktable has very sudden rise or fall */
			risetime = rise/click;
			here = p-2;
			next = p;
			newtime   = lasttime + risetime;			/* current time-point is moved forward a little */
			timeshift = newtime - thistime;
			while(next < *brkend) {
				*here += timeshift;
				if(*here >= *next) {
					here += 2;
					next += 2;								
				} else
					break;
			}
			if(next >= *brkend)
				*here += timeshift;
			thistime = newtime;
		}
		lasttime = thistime;
		lastval  = thisval;
	}
	p = *brkend - 2;			  
	if(flteq((double)*p,endtime)) {
		*p = endtime;
		*brkend = p + 2;
	} else {
		while(*p > endtime + FLTERR)	  /* if, as as a result of deglitches, endtime of table is moved forward */
			p -= 2;						  /* Find the last point prior to the true endtime */
		if(flteq((double)*p,endtime)) {	  /* and establish the correct time .. */
			*p = endtime;
			*brkend = p + 2;
		} else {		   				  /* ... or the correct value, for that true endtime */
			lasttime = *p;
			lastval  = *(p+1);
			thistime = *(p+2);
			thisval  = *(p+3);
			timediff = thistime - lasttime;
			valdiff  = thisval - lastval;
			timeratio= (endtime - lasttime)/timediff;
			valstep  = valdiff * timeratio;
			thisval  = lastval + valstep;
			p += 2;
			*p++ = endtime;
			*p++ = thisval;
			*brkend = p;
		}
	}
	return;
}
    
/********************************* ENVREPLACE *************************
 *
 *  NEEDED FOR all WARP, except KORRUGATION and TRIGGER
 */

int envreplace(float *env,float **envend,float *origenv,float *origend)
{
	float *p, *q;
	double gate = MIN_FRACTION_OF_LEVEL; 	/* minimum non-zero value of envelope */
	double maxval = 0.0;
	p = origenv;
	q = env;
	if((origend - origenv) < (*envend - env)) {
		while(p<origend) {
			if(*p <= gate) {	/* if original envelope touches zero */
				if(*q <=gate || q==env)	/* if new envelope touches zero */
					*q = 0.0f;			/* OR start_of_file, set to zero */
				else			/* else, retain previous value of envelope */
					*q = *(q-1);
			} else
				*q = (float)((*q)/(*p));
			maxval = max(*q,maxval);
			q++;
			p++;
		} 
	} else {
		while(q<*envend) {
			if(*p <= gate) {	/* if original envelope touches zero */
				if(*q <=gate || q==env)	/* if new envelope touches zero */
					*q = 0.0f;			/* OR start_of_file, set to zero */
				else			/* else, retain previous value of envelope */
					*q = *(q-1);
			} else
				*q = (float)((*q)/(*p));
			maxval = max(*q,maxval);
			q++;
			p++;
		}
	}
	*envend = q;
	if(maxval<=gate) {
		sprintf(errstr,"new envelope is effectively zero.\n");
		return(GOAL_FAILED);
	}
	return(FINISHED);
}

/****************************** CONVERT_OTHER_BRKTABLE_TO_ENVTABLE *********************************/

int convert_other_brktable_to_envtable(double *brk,double *brkend,float **env,float **envend,dataptr dz)
{
	int   n, envlen;
	float  *q;
	double here;
	double timestep = dz->outfile->window_size * MS_TO_SECS;
//	double inc = timestep;
	double duration = *(brkend-2);
	if(timestep <= 0.0) {
		sprintf(errstr,"No window_size set: convert_other_brktable_to_envtable()\n");
		return(PROGRAM_ERROR);
	}
	if(timestep < ENV_MIN_WSIZE * MS_TO_SECS) {
		sprintf(errstr,"Invalid window_size: convert_other_brktable_to_envtable()\n");
		return(PROGRAM_ERROR);
	}
	if(duration < timestep) {
		sprintf(errstr,"Brktable duration less than window_size: Cannot proceed.\n");
		return(DATA_ERROR);
	}
	envlen = (int)((duration/timestep) + 1.0);  /* round up */
	envlen += MARGIN;                  		/* allow for errors in float-calculation */
	if(brk==NULL) {
		sprintf(errstr,"No existing brkpnt table. convert_brktable_to_envtable()\n");
		return(PROGRAM_ERROR);
	}
	if(*env != NULL)
		free(*env);
	if((*env = (float *)malloc(envlen * sizeof(float)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create new envelope array.\n");
		return(MEMORY_ERROR);
	}
	q = *env;
	n = 0;
	while(n<envlen) {
		if((here = (double)n * timestep)>(duration+FLTERR))
			break;
		*q = read_a_val_from_brktable(brk,brkend,here);
		q++;
		n++;
	}
	envlen = n;
	if((*env = (float *)realloc(*env,envlen * sizeof(float)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to reallocate new envelope array.\n");
		return(MEMORY_ERROR);
	}
	*envend = *env + envlen;
	return(FINISHED);
}

/**************************** READ_A_VAL_FROM_BRKTABLE *****************************/

float read_a_val_from_brktable(double *brk,double *brkend,double thistime)
{
    float  thisval;
    double *p = brk, val;
    double hival, loval, hiind, loind;
	double firstval = *(brk+1);
	double lastval  = *(brkend-1);
	double lasttime = *(brkend-2);
	if(thistime <= 0.0)
		thisval = (float)firstval;
	else if(thistime >= lasttime)
		thisval = (float)lastval;
	else { 
		while(*(p)<thistime)
			p += 2;
		hival  = *(p+1);
		hiind  = *p;
		loval  = *(p-1);
		loind  = *(p-2);
		val    = (thistime - loind)/(hiind - loind);
		val   *= (hival - loval);
		val   += loval;
		thisval  = (float)val;
	}
    return(thisval);
}

/**************************** ENLARGE_BRKTABLE *****************************/

int enlarge_brktable(double **brk,double **brkend, double **q,int *arraysize)
{
	int offset = *q - *brk;
	*arraysize += BIGARRAY;
	if((*brk = (double *)realloc((*brk),(*arraysize) * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to enlarge brktable.\n");
		return(MEMORY_ERROR);
	}
	*brkend = *brk + *arraysize;
	*q = *brk + offset;
	return(FINISHED);
}

/**************************** ROUND_OFF_BRKTABLE *****************************/

int round_off_brktable(double *q,double **brk,double **brkend,double envdur)
{
	int brksize;
	int vals_written = q - *brk;
	double *endadr;
	double *startadr = *brk;
	double *nextadr  = *brk + 2;
	double starttime = *startadr, nexttime = *nextadr;
	(brksize = vals_written/2);
	if((brksize = vals_written/2) * 2 != vals_written) {
		sprintf(errstr,"Data pairing anomaly in round_off_brktable()\n");
		return(PROGRAM_ERROR);
	}
	if(brksize > 1) {
		if(flteq(nexttime,starttime)) {	/* Junk any redundant initial zero */
			memmove((char *)startadr,(char *)nextadr,(brksize-1)*2*sizeof(double));
			brksize--;
		}
	}
	endadr = *brk + (brksize * 2);
	if(*(endadr - 2) != envdur) {	  	/* IF lasttime != length of envelope */
		if(flteq(*(endadr - 2),envdur))	/* If approx equal, reset to equal */
			*(endadr - 2) = envdur;
		else {							/* Else generate an endtime time-val pair */
			brksize++;
			if((*brk=(double *)realloc(*brk,(brksize*2)*sizeof(double)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY to reallocate brktable.\n");
				return(MEMORY_ERROR);
			}
			endadr = *brk + (brksize * 2);
			*(endadr-2) = envdur;		/* true end time gets */
			*(endadr-1) = 0.0;			/* 0 endvalue		  */
		}
	}
	if((*brk=(double *)realloc(*brk,(brksize*2)*sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to reallocate brktable.\n");
		return(MEMORY_ERROR);
	}
	*brkend = *brk + (brksize * 2);	
	return(FINISHED);
}

/**************************** CONVERT_TO_ON_TRIGGERS *****************************/

void convert_to_on_triggers(double now,double timestep,double rise,int *triggered,double **q)
{
	double *p = *q;
	if(*triggered==FALSE) { 				/* If not already RISING-triggered */
		if(!flteq(*(p-2),now-timestep)) {	/* If previous time-value was not too close to this one */
			*p++ = now - timestep;			/* Write pre-time and 0.0 for start of rise */
			*p++ = 0.0;
		}
		*p++ = now;	                    	/* Write time and rise-value for end of rise */
		*p++ = rise;                    
		*triggered = TRUE; 					/* Flag trigrise */
	} else {                          		/* If already RISING-triggered  */
		*(p-4)  = now - timestep;			/* readjust start-time of existing rise, from current time */
		*(p-2)  = now;						/* readjust end-time of existing rise, from current time */
		*(p-1) += rise;						/* readjust total rise value */
	}
	*q = p;
}

/******************************* GEN_REAPPLIED_TRIGGER_ENVELOPE ******************************/

int gen_reapplied_trigger_envelope(double **brk, double **brkend,dataptr dz)
{
	int exit_status;
	int 	 n, brklen;
//	double   next_trigtime, this_trigtime, nuhere, last_outval = 0.0, trigval;
	double   next_trigtime, this_trigtime, nuhere, trigval;
	double   *q, *nubrkptr, *nubrk, *nubrkend;
	double   *trigptr, *trigbrk, *trigbrkend;
	int arraysize = BIGARRAY;
	trigptr = trigbrk = *brk;
	trigbrkend = *brkend;
	if((nubrk = (double *)malloc(arraysize * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create new envelope table.\n");
		return(MEMORY_ERROR);
	}
	nubrkend = nubrk + arraysize;
	nubrkptr = nubrk;
	while(trigptr < trigbrkend) {
		this_trigtime = *trigptr++;
		trigval       = *trigptr++;
		if(trigptr < trigbrkend)
			next_trigtime = *trigptr;
		else
			next_trigtime = DBL_MAX;
		if(flteq(trigval,0.0)) {		/* if the trigger table value is 0.0 put Zero in new table */
			*nubrkptr++ = this_trigtime;
			*nubrkptr++ = 0.0;
		} else {						/* else */
			q = dz->rampbrk;			/* start copying values from ramp table into new brkpnt table, */
			n = 0;						/* until ramptable ends, OR we reach next brkpnt in trigger table */
			while(n < dz->rampbrksize && (nuhere = this_trigtime + *q++)<next_trigtime) {  
				*nubrkptr++ = nuhere;
				*nubrkptr++ = *q++;
				n++;
			}
		}
		if(nubrkptr>=(nubrkend-2)) {
			if((exit_status = enlarge_brktable(&nubrk,&nubrkend,&nubrkptr,&arraysize))<0)
				return(exit_status);
		}
	}
	brkdeglitch(nubrk,&nubrkptr);		/* get rid of too fast amp rises or falls */
	brklen = nubrkptr - nubrk;
	free((char *)(*brk));
	if((*brk=(double *)realloc((char *)nubrk,brklen * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to reallocate new envelope table.\n");
		return(MEMORY_ERROR);
	}
	if((brklen/2) * 2 != brklen) {
		sprintf(errstr,"Anomalous breaktable size in gen_reapplied_trigger_envelope()\n");
		return(PROGRAM_ERROR);
	}
	*brkend = *brk + brklen;
	return(FINISHED);
}

/***************************** REALLOC_ENVCREATE_TABLES ****************************/

int realloc_envcreate_tables(int tabsize,dataptr dz)
{
	if((dz->parray[ENV_CREATE_TIME] = 
	(double *)realloc(dz->parray[ENV_CREATE_TIME],tabsize * sizeof(double)))==NULL
	|| (dz->parray[ENV_CREATE_LEVL] = 
	(double *)realloc(dz->parray[ENV_CREATE_LEVL],tabsize * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to reallocate level array.\n");
		return(MEMORY_ERROR);
	}
	return(FINISHED);
}

/***************************** CREATE_ENVELOPE ****************************
 *
 * (1)	time_quantum has a minimum value, as this function can create a
 *		brktable for conversion to an envfile.
 * 1a)	Except in the case of ENV_ATTACK, which has very acute risetime!!
 */

int create_envelope(int *cnt,dataptr dz)
{
	int exit_status;
	int falling = FALSE;
	int n,m, k, base, newcnt;
	double time_quantum, timestep, levelstep, attenator = 0.0;
	double thisduration, ratio;
//	double atten = 1.0/ATTEN;
	if((dz->parray[ENV_CREATE_TIME] =  (double *)malloc(sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create times array.\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[ENV_CREATE_LEVL] = (double *)malloc(sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create levels array.\n");
		return(MEMORY_ERROR);
	}
	dz->parray[ENV_CREATE_TIME][0] = dz->parray[ENV_CREATE_INTIME][0];
	dz->parray[ENV_CREATE_LEVL][0] = dz->parray[ENV_CREATE_INLEVEL][0];
	*cnt = 1;
	for(n=1;n<dz->itemcnt;n++) {
		thisduration = dz->parray[ENV_CREATE_INTIME][n] - dz->parray[ENV_CREATE_INTIME][n-1];
//TW UPDATE
		if(dz->process == ENV_ATTACK || dz->process == ENV_DOVETAILING || dz->process == ENV_CURTAILING)
			time_quantum = max(thisduration/(double)MINSPAN,2.0/dz->infile->srate);  	/* 1a */
		else
			time_quantum = max(thisduration/(double)MINSPAN,ENV_MIN_WSIZE * MS_TO_SECS);  /* 1 */
		if((levelstep = dz->parray[ENV_CREATE_INLEVEL][n] - dz->parray[ENV_CREATE_INLEVEL][n-1]) <= 0.0)
				falling = TRUE;
			else
				falling = FALSE;
		switch(dz->iparray[ENV_SLOPETYPE][n]) {
		case(ENVTYPE_LIN):
			(*cnt)++;
			if((exit_status = realloc_envcreate_tables(*cnt,dz))<0)
				return(exit_status);
			dz->parray[ENV_CREATE_TIME][*cnt - 1] = dz->parray[ENV_CREATE_INTIME][n];
			dz->parray[ENV_CREATE_LEVL][*cnt - 1] = dz->parray[ENV_CREATE_INLEVEL][n];
			break;
		case(ENVTYPE_STEEP):
		case(ENVTYPE_EXP):
		case(ENVTYPE_DBL):
			switch(dz->iparray[ENV_SLOPETYPE][n]) {
			case(ENVTYPE_STEEP):	attenator = STEEP_ATTEN;	break;
			case(ENVTYPE_EXP):		attenator = ATTENATOR;		break;
			case(ENVTYPE_DBL):		attenator = ATTENATOR * 2;	break;
			}
			base = *cnt;
			newcnt = (int)(thisduration/time_quantum);	/* TRUNCATE */
			/* RWD 4:2002 NB: div/zero bug here when newcnt resolves to zero */
			/* should this be flagged as a user error? */
			if(newcnt==0)
				newcnt = 1;
			
			timestep = thisduration/(double)newcnt;
			*cnt += newcnt;
			if((exit_status = realloc_envcreate_tables(*cnt,dz))<0)
				return(exit_status);
			for(m = 1,k = base;k < *cnt;k++,m++) {
				dz->parray[ENV_CREATE_TIME][k] = dz->parray[ENV_CREATE_INTIME][n-1] + (m * timestep);
				if(falling) {
					ratio = 1.0 - ((m * timestep)/thisduration);
					ratio = max(ratio,0.0);
					ratio = pow(ratio,attenator);
					dz->parray[ENV_CREATE_LEVL][k] = fabs(levelstep) * ratio;
					dz->parray[ENV_CREATE_LEVL][k] += dz->parray[ENV_CREATE_INLEVEL][n];
				}
				else {
					ratio = (m * timestep)/thisduration;
					ratio = pow(ratio,attenator);
					ratio = min(ratio,1.0);
					dz->parray[ENV_CREATE_LEVL][k] = fabs(levelstep) * ratio;
					dz->parray[ENV_CREATE_LEVL][k] += dz->parray[ENV_CREATE_INLEVEL][n-1];
				}
			}
			dz->parray[ENV_CREATE_TIME][*cnt-1] = dz->parray[ENV_CREATE_INTIME][n];
			break;
		default:
			sprintf(errstr,"Unknown case in create_envelope()\n");
			return(PROGRAM_ERROR);			
		}
	}
	return(FINISHED);
}

/******************************** ENVELOPE_TREMOL **********************************/

int envelope_tremol(dataptr dz)
{
	int exit_status;
	int m;
	int n, k, st_sampsread, st_samps_processed;
	double thistime, losin, hisin, frac, val;
	double fsinpos = 0.0;
	int	   sinpos;
	int    chans = dz->infile->channels;
	double inverse_srate = 1.0/(double)dz->infile->srate;
	double tabsize_over_srate = (double)ENV_TREM_TABSIZE/(double)dz->infile->srate;
	float *buf = dz->sampbuf[0];
	double *sintab = dz->parray[ENV_SINETAB];
	while(dz->samps_left > 0) {
		st_samps_processed = dz->total_samps_read/chans;
		if((exit_status = read_samps(buf,dz))<0)
			return(exit_status);
		st_sampsread = dz->ssampsread/chans; 
		for(n=0; n < st_sampsread; n++) {
			k = n * chans;
			thistime = (double)st_samps_processed * inverse_srate;
			if((exit_status = read_values_from_all_existing_brktables(thistime,dz))<0)
				return(exit_status);
			if(dz->brksize[ENV_TREM_FRQ] && dz->mode==ENV_TREM_LOG)
				dz->param[ENV_TREM_FRQ] = pow(10,dz->param[ENV_TREM_FRQ]);
			sinpos	 = (int)fsinpos;	/* TRUNCATE */
			frac     = fsinpos - (double)sinpos;
			losin    = sintab[sinpos];
			hisin    = sintab[sinpos+1];
			val		 = losin + ((hisin - losin) * frac);	/* interpd (normalised) sintable value */
			val     *= dz->param[ENV_TREM_DEPTH];			/* adjusted for tremolo depth */
			val     += (1.0 - dz->param[ENV_TREM_DEPTH]);	/* subtract from max: tremol superimposed on existing level */
			val     *= dz->param[ENV_TREM_AMP];				/* multiply by overall signal gain (<= 1.0) */
			fsinpos += (dz->param[ENV_TREM_FRQ] * tabsize_over_srate);	/* advance sintable float-pointer */
			fsinpos  = fmod(fsinpos,(double)ENV_TREM_TABSIZE);			/* wrap at table-size */
			for(m=0;m<chans;m++)
				buf[k+m]   = (float)/*round*/(buf[k+m] * val);
			st_samps_processed++;
		}
		if(dz->ssampsread > 0) {
			if((exit_status = write_samps(buf,dz->ssampsread,dz))<0)
				return(exit_status);
		}
	}
	return(FINISHED);
}


/*************************** ENVPEAKCNT *************************/
 
int envpeakcnt(float *env,float *envend,dataptr dz)
{
	int exit_status;
 	int upwards, peakkcnt = 0;
	double thistime = 0.0;
 	float *p, *q;
	double timestep = dz->outfile->window_size * MS_TO_SECS;
 	float *shadowenv;
	size_t bytelen = (envend - env) * sizeof(float);
	if((shadowenv = (float *)malloc(bytelen))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create shadow envelope array.\n");
		return(MEMORY_ERROR);
	}
	memset((char *)shadowenv,0,(size_t)bytelen);
 	p = env+1;
 	q = env;    
 	if (*p > *q)
 		upwards = TRUE;
 	else
 		upwards = FALSE;
 	while(p < envend) {
		if((exit_status = read_values_from_all_existing_brktables(thistime,dz))<0)
			return(exit_status);
 		if(upwards) {
 			if(*p <= *q) {
 				if(ispeak(env,envend,q,(int)dz->iparam[ENV_PKSRCHWIDTH]))
					peakkcnt++;
 				upwards = FALSE;
 			}
 		} else {
 			if(*p > *q)
 				upwards = TRUE;
 		}
		thistime += timestep;
 		p++;
 		q++;
 	}
	fprintf(stdout,"INFO: Number of peaks = %d\n",peakkcnt);
	fflush(stdout);
 	return(FINISHED);
}
 
