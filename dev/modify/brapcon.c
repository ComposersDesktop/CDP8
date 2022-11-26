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



/* floatsam version*/
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <structures.h>
#include <tkglobals.h>
#include <globcon.h>
#include <processno.h>
#include <modeno.h>
#include <arrays.h>
#include <modify.h>

#include <sfsys.h>
#include <osbind.h>

#if defined unix || defined __GNUC__
#define round(x) lround((x))
#endif

#define SAFETY   ((long)4096)
#define OSAFETY  ((long)256)
static int  initialise_unused_brassage_vflags(dataptr dz);
static void initialise_user_inaccessible_params(dataptr dz);
static void param_check_and_convert(int *zerovel,dataptr dz);
static void convert_pitch(int paramno,dataptr dz);
static int  initialise_data(dataptr dz);
static int  set_internal_flags(dataptr dz);
static void initialise_channel_configuration(dataptr dz);
static int  granula_setup(dataptr dz);
static int  check_for_zeroes(int paramno,dataptr dz);
static void convert_value_to_int(int paramno,double convertor,dataptr dz);
static int  make_splice_tables(dataptr dz);
static void set_ranges(dataptr dz);
static int  calc_overflows(dataptr dz);
static void set_this_range(int rangeno,int hino, int lono, int flagno, dataptr dz);
static void set_this_drange(int rangeno,int hino, int lono, int flagno, dataptr dz);
static int  calc_max_scatter(int mgi,long *max_scatlen, dataptr dz);
static void adjust_overflows(long mscat, dataptr dz);
#ifdef MULTICHAN
static int check_spatialisation_data(dataptr dz);
#endif

/********************************* GRANULA_PCONSISTENCY *************************************/

int granula_pconsistency(dataptr dz)
{
	int exit_status;
	int zerovel      = FALSE;
	int total_flags  = SFLAGCNT+FLAGCNT;
/*	int total_params = dz->application->max_param_cnt + dz->application->option_cnt;*/
	int n;
	long min_infilesize = LONG_MAX;

	initrand48();

	if(dz->application->vflag_cnt < GRS_MAX_VFLAGS
	&&(exit_status = initialise_unused_brassage_vflags(dz))<0) 
		return(exit_status);	/* Create flags inaccessible to user in certain modes */

	initialise_user_inaccessible_params(dz);

	switch(dz->process) {	/* infilesize counted in mono samples OR STEREO sample-pairs */
	case(BRASSAGE):
    	dz->iparam[ORIG_SMPSIZE] = dz->insams[0]/dz->infile->channels;
		break;					 
	case(SAUSAGE):
		for(n=0;n<dz->infilecnt;n++)	
			min_infilesize = min(min_infilesize,dz->insams[n]);
		if(min_infilesize == LONG_MAX) {
			sprintf(errstr,"Problem estimating min infilesize.\n");
			return(PROGRAM_ERROR);
		}
    	dz->iparam[ORIG_SMPSIZE] = min_infilesize/dz->infile->channels;
		break;					 
	}
	if((dz->iparray[GRS_FLAGS]=(int *)malloc(total_flags * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICEINT MEMORY for brassage internal flags.\n");
		return(MEMORY_ERROR);		/* establish internal flagging */
	}
	for(n=0;n<total_flags;n++)
		dz->iparray[GRS_FLAGS][n] = 0;

	param_check_and_convert(&zerovel,dz);

	if(sloom) {
		if(zerovel) {
		 	if(dz->mode == GRS_TIMESTRETCH) {
				fprintf(stdout,"INFO: Infinite time-stretch (zero squeeze) found : Creating file of length 2.0 secs\n");
			 	fflush(stdout);
				dz->param[GRS_OUTLEN] = 2.0;
/* NEW Mar 24: 2002*/
				dz->brksize[GRS_OUTLEN] = 0;
				convert_value_to_int(GRS_OUTLEN,(double)dz->infile->srate,dz);
			} else if(dz->param[GRS_OUTLEN] == 0.0) {
				sprintf(errstr,	"Infinite time-stretch (zero squeeze) found : Outfile length must be specified.\n");
				return(USER_ERROR);
			}
		}
	} else {
		if(zerovel && dz->param[GRS_OUTLEN]==0.0) {
			sprintf(errstr,	"Zero VELOCITY found: Outfile length must be specified.\n");
			return(USER_ERROR);
		}
	}

#ifdef MULTICHAN
	dz->out_chans = 2;
	if (dz->iparam[GRS_CHAN_TO_XTRACT] < 0) {	/* setup multichan output */
		dz->out_chans = -dz->iparam[GRS_CHAN_TO_XTRACT];	
		dz->iparam[GRS_CHAN_TO_XTRACT] = 0;		/* Force input to mix to mono (if not mono already) */
	}
	if((exit_status = check_spatialisation_data(dz))<0)
		return(exit_status);
#endif
	if(dz->infile->channels ==MONO)
		dz->iparam[GRS_CHAN_TO_XTRACT] = 0;		/* even if user has set it, in error */

	convert_pitch(GRS_PITCH,dz);
	convert_pitch(GRS_HPITCH,dz);

	dz->iparam[GRS_ARRAYSIZE] = BIGARRAY;
	if((exit_status = initialise_data(dz))<0) 			/* before setup_environment */
		return(exit_status);
	if((exit_status = set_internal_flags(dz))<0)
		return(exit_status);
	initialise_channel_configuration(dz);
#ifndef MULTICHAN
	dz->infile->channels = dz->iparam[GRS_OUTCHANS];	/* set channel count for OUTPUT */
#else
	dz->outfile->channels = dz->iparam[GRS_OUTCHANS];	/* preset channel count for OUTPUT */
#endif

	if((exit_status = granula_setup(dz))<0)
		return(exit_status);							/* has to be done before buffers.c */
	if(dz->process==SAUSAGE) {
		dz->bufcnt += (dz->infilecnt * 2);
		if((dz->sampbuf = (float **)realloc((char *)dz->sampbuf,dz->bufcnt * sizeof(float *)))==NULL
		|| (dz->sbufptr = (float **)realloc((char *)dz->sbufptr,dz->bufcnt * sizeof(float *)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to reallocate space for sausage.\n");
			return(MEMORY_ERROR);
		}
	}
	return(FINISHED);
}

/************************* INITIALISE_UNUSED_BRASSAGE_VFLAGS *************************/

int initialise_unused_brassage_vflags(dataptr dz)
{
	int n;
	if(dz->vflag==NULL) {
		if((dz->vflag  = (char *)malloc(GRS_MAX_VFLAGS * sizeof(char)))==NULL) {
			sprintf(errstr,"INSUFFICEINT MEMORY for brassage flags.\n");
			return(MEMORY_ERROR);
		}
	} else {
		if((dz->vflag  = (char *)realloc(dz->vflag,GRS_MAX_VFLAGS * sizeof(char)))==NULL) {
			sprintf(errstr,"INSUFFICEINT MEMORY to reallocate brassage flags.\n");
			return(MEMORY_ERROR);
		}
	}
	for(n=dz->application->vflag_cnt;n<GRS_MAX_VFLAGS;n++) 
		dz->vflag[n]  = FALSE;
	return(FINISHED);
}

/************************* INITIALISE_USER_INACCESSIBLE_PARAMS *************************/

void initialise_user_inaccessible_params(dataptr dz)
{
	switch(dz->mode) {
	case(GRS_PITCHSHIFT):	
	case(GRS_TIMESTRETCH):		/* Initialise search param, inaccessible to user in these modes */
	case(GRS_GRANULATE):
	 	dz->param[GRS_SRCHRANGE] = 0.0;		/* NOT STRICTLY NECESSARY: as should not be used */
		/* fall thro */
	case(GRS_REVERB):			/* Initialise scatter param, inaccessible to user in these modes */
	case(GRS_SCRAMBLE):		
	    dz->param[GRS_SCATTER]  = GRS_DEFAULT_SCATTER;
		break;
	}
}

/************************** PARAM_CHECK_AND_CONVERT ************************************/

void param_check_and_convert(int *zerovel,dataptr dz)
{
	double sr = (double)dz->infile->srate;
	int paramno, total_params = dz->application->max_param_cnt + dz->application->option_cnt;
	for(paramno = 0;paramno < total_params; paramno++) {
		switch(paramno) {
		case(GRS_VELOCITY):
		case(GRS_HVELOCITY):
			if(*zerovel == FALSE)
				*zerovel = check_for_zeroes(paramno,dz);
			break;	
		case(GRS_AMP):
		case(GRS_HAMP):
			/* not required for floats */
			/*convert_value_to_int(paramno,TWO_POW_15,dz);*/
			break;
		case(GRS_OUTLEN):
			convert_value_to_int(paramno,sr,dz);
			break;
		case(GRS_SRCHRANGE):
		case(GRS_GRAINSIZE):
		case(GRS_HGRAINSIZE):
		case(GRS_BSPLICE):
		case(GRS_HBSPLICE):
		case(GRS_ESPLICE):
		case(GRS_HESPLICE):
			convert_value_to_int(paramno,MS_TO_SECS * sr,dz);
			break;						 
		}
	}
}

/************************** CONVERT_PITCH ************************************/

void convert_pitch(int paramno,dataptr dz)
{
	double *p, *end;
	if(dz->brksize[paramno]) {
		p   = dz->brk[paramno] + 1;		
		end = dz->brk[paramno] + (dz->brksize[paramno] * 2);
		while(p < end) {
			*p *= OCTAVES_PER_SEMITONE;
			p += 2;
		}
	} else
		dz->param[paramno] = dz->param[paramno] * OCTAVES_PER_SEMITONE;
}

/******************************* INITIALISE_DATA **************************/

int initialise_data(dataptr dz)
{
	/* NEED TO BE INITALISED (??) AS THEY ARE INTERNAL */
	dz->param[GRS_VRANGE] 		= 0.0;
	dz->param[GRS_DRANGE] 		= 0.0;
	dz->param[GRS_PRANGE]		= 0.0;
	dz->param[GRS_SPRANGE]		= 0.0;
	dz->iparam[GRS_ARANGE] 		= 0;
	dz->iparam[GRS_GRANGE] 		= 0;
	dz->iparam[GRS_BRANGE]		= 0;
	dz->iparam[GRS_ERANGE]		= 0;
	dz->iparam[GRS_LONGS_BUFLEN]= 0;
	dz->iparam[GRS_BUF_XS] 		= 0;
	dz->iparam[GRS_LBUF_XS] 	= 0;
	dz->iparam[GRS_BUF_SMPXS] 	= 0;
	dz->iparam[GRS_LBUF_SMPXS] 	= 0;
	dz->iparam[GRS_GLBUF_SMPXS] = 0;
	dz->iparam[SAMPS_IN_INBUF]  = 0;

	dz->iparam[GRS_IS_BTAB]		= FALSE;
	dz->iparam[GRS_IS_ETAB]		= FALSE;
																		
    if((dz->parray[GRS_NORMFACT] = (double *)malloc(dz->iparam[GRS_ARRAYSIZE] * sizeof(double)))==NULL) {
		sprintf(errstr,"initialise_data()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/************************** SET_INTERNAL_FLAGS ************************************/

int set_internal_flags(dataptr dz)
{
	if(dz->process==SAUSAGE) {
		if(dz->brksize[GRS_HVELOCITY])  dz->iparray[GRS_FLAGS][G_VELOCITY_FLAG]  |= G_VARIABLE_HI_BOUND;
		else 							dz->iparray[GRS_FLAGS][G_VELOCITY_FLAG]  |= G_HI_BOUND;

		if(dz->brksize[GRS_HDENSITY])   dz->iparray[GRS_FLAGS][G_DENSITY_FLAG]   |= G_VARIABLE_HI_BOUND;
		else 							dz->iparray[GRS_FLAGS][G_DENSITY_FLAG]   |= G_HI_BOUND;

		if(dz->brksize[GRS_HGRAINSIZE]) dz->iparray[GRS_FLAGS][G_GRAINSIZE_FLAG] |= G_VARIABLE_HI_BOUND;
		else  							dz->iparray[GRS_FLAGS][G_GRAINSIZE_FLAG] |= G_HI_BOUND;

		if(dz->brksize[GRS_HPITCH])  	dz->iparray[GRS_FLAGS][G_PITCH_FLAG]     |= G_VARIABLE_HI_BOUND;
		else 							dz->iparray[GRS_FLAGS][G_PITCH_FLAG]     |= G_HI_BOUND;

		if(dz->brksize[GRS_HAMP])  		dz->iparray[GRS_FLAGS][G_AMP_FLAG]       |= G_VARIABLE_HI_BOUND;
		else 							dz->iparray[GRS_FLAGS][G_AMP_FLAG]       |= G_HI_BOUND;

		if(dz->brksize[GRS_HSPACE])  	dz->iparray[GRS_FLAGS][G_SPACE_FLAG]     |= G_VARIABLE_HI_BOUND;
		else 							dz->iparray[GRS_FLAGS][G_SPACE_FLAG]     |= G_HI_BOUND;

		if(dz->brksize[GRS_HBSPLICE])  	dz->iparray[GRS_FLAGS][G_BSPLICE_FLAG]   |= G_VARIABLE_HI_BOUND;
		else 							dz->iparray[GRS_FLAGS][G_BSPLICE_FLAG]   |= G_HI_BOUND;

		if(dz->brksize[GRS_HESPLICE])  	dz->iparray[GRS_FLAGS][G_ESPLICE_FLAG]   |= G_VARIABLE_HI_BOUND;
		else 							dz->iparray[GRS_FLAGS][G_ESPLICE_FLAG]   |= G_HI_BOUND;

		if(dz->brksize[GRS_VELOCITY])	dz->iparray[GRS_FLAGS][G_VELOCITY_FLAG]  |= G_VARIABLE_VAL;
		else							dz->iparray[GRS_FLAGS][G_VELOCITY_FLAG]  |= G_FIXED_VAL;

		if(dz->brksize[GRS_DENSITY])	dz->iparray[GRS_FLAGS][G_DENSITY_FLAG]   |= G_VARIABLE_VAL;
		else							dz->iparray[GRS_FLAGS][G_DENSITY_FLAG]   |= G_FIXED_VAL;

		if(dz->brksize[GRS_GRAINSIZE])	dz->iparray[GRS_FLAGS][G_GRAINSIZE_FLAG] |= G_VARIABLE_VAL;
		else							dz->iparray[GRS_FLAGS][G_GRAINSIZE_FLAG] |= G_FIXED_VAL;

		if(dz->brksize[GRS_PITCH])		dz->iparray[GRS_FLAGS][G_PITCH_FLAG]     |= G_VARIABLE_VAL;
		else							dz->iparray[GRS_FLAGS][G_PITCH_FLAG]     |= G_FIXED_VAL;

		if(dz->brksize[GRS_AMP])		dz->iparray[GRS_FLAGS][G_AMP_FLAG]       |= G_VARIABLE_VAL;
		else							dz->iparray[GRS_FLAGS][G_AMP_FLAG]       |= G_FIXED_VAL;

		if(dz->brksize[GRS_SPACE])		dz->iparray[GRS_FLAGS][G_SPACE_FLAG]     |= G_VARIABLE_VAL;
		else							dz->iparray[GRS_FLAGS][G_SPACE_FLAG]     |= G_FIXED_VAL;

		if(dz->brksize[GRS_BSPLICE])	dz->iparray[GRS_FLAGS][G_BSPLICE_FLAG]   |= G_VARIABLE_VAL;
		else							dz->iparray[GRS_FLAGS][G_BSPLICE_FLAG]   |= G_FIXED_VAL;

		if(dz->brksize[GRS_ESPLICE])	dz->iparray[GRS_FLAGS][G_ESPLICE_FLAG]   |= G_VARIABLE_VAL;
		else							dz->iparray[GRS_FLAGS][G_ESPLICE_FLAG]   |= G_FIXED_VAL;

		if(dz->brksize[GRS_SRCHRANGE])	dz->iparray[GRS_FLAGS][G_RANGE_FLAG]     |= G_VARIABLE_VAL;
		else							dz->iparray[GRS_FLAGS][G_RANGE_FLAG]     |= G_FIXED_VAL;

		if(dz->brksize[GRS_SCATTER])	dz->iparray[GRS_FLAGS][G_SCATTER_FLAG]   |= G_VARIABLE_VAL;
		else							dz->iparray[GRS_FLAGS][G_SCATTER_FLAG]   |= G_FIXED_VAL;

		return(FINISHED);
	}

	switch(dz->mode) {
	case(GRS_FULL_MONTY):
		if(dz->brksize[GRS_HVELOCITY])  dz->iparray[GRS_FLAGS][G_VELOCITY_FLAG]  |= G_VARIABLE_HI_BOUND;
		else 							dz->iparray[GRS_FLAGS][G_VELOCITY_FLAG]  |= G_HI_BOUND;

		if(dz->brksize[GRS_HDENSITY])   dz->iparray[GRS_FLAGS][G_DENSITY_FLAG]   |= G_VARIABLE_HI_BOUND;
		else 							dz->iparray[GRS_FLAGS][G_DENSITY_FLAG]   |= G_HI_BOUND;

		if(dz->brksize[GRS_HGRAINSIZE]) dz->iparray[GRS_FLAGS][G_GRAINSIZE_FLAG] |= G_VARIABLE_HI_BOUND;
		else  							dz->iparray[GRS_FLAGS][G_GRAINSIZE_FLAG] |= G_HI_BOUND;

		if(dz->brksize[GRS_HPITCH])  	dz->iparray[GRS_FLAGS][G_PITCH_FLAG]     |= G_VARIABLE_HI_BOUND;
		else 							dz->iparray[GRS_FLAGS][G_PITCH_FLAG]     |= G_HI_BOUND;

		if(dz->brksize[GRS_HAMP])  		dz->iparray[GRS_FLAGS][G_AMP_FLAG]       |= G_VARIABLE_HI_BOUND;
		else 							dz->iparray[GRS_FLAGS][G_AMP_FLAG]       |= G_HI_BOUND;

		if(dz->brksize[GRS_HSPACE])  	dz->iparray[GRS_FLAGS][G_SPACE_FLAG]     |= G_VARIABLE_HI_BOUND;
		else 							dz->iparray[GRS_FLAGS][G_SPACE_FLAG]     |= G_HI_BOUND;

		if(dz->brksize[GRS_HBSPLICE])  	dz->iparray[GRS_FLAGS][G_BSPLICE_FLAG]   |= G_VARIABLE_HI_BOUND;
		else 							dz->iparray[GRS_FLAGS][G_BSPLICE_FLAG]   |= G_HI_BOUND;

		if(dz->brksize[GRS_HESPLICE])  	dz->iparray[GRS_FLAGS][G_ESPLICE_FLAG]   |= G_VARIABLE_HI_BOUND;
		else 							dz->iparray[GRS_FLAGS][G_ESPLICE_FLAG]   |= G_HI_BOUND;
		/* fall thro */
	case(GRS_BRASSAGE):
		if(dz->brksize[GRS_VELOCITY])	dz->iparray[GRS_FLAGS][G_VELOCITY_FLAG]  |= G_VARIABLE_VAL;
		else							dz->iparray[GRS_FLAGS][G_VELOCITY_FLAG]  |= G_FIXED_VAL;

		if(dz->brksize[GRS_DENSITY])	dz->iparray[GRS_FLAGS][G_DENSITY_FLAG]   |= G_VARIABLE_VAL;
		else							dz->iparray[GRS_FLAGS][G_DENSITY_FLAG]   |= G_FIXED_VAL;

		if(dz->brksize[GRS_GRAINSIZE])	dz->iparray[GRS_FLAGS][G_GRAINSIZE_FLAG] |= G_VARIABLE_VAL;
		else							dz->iparray[GRS_FLAGS][G_GRAINSIZE_FLAG] |= G_FIXED_VAL;

		if(dz->brksize[GRS_PITCH])		dz->iparray[GRS_FLAGS][G_PITCH_FLAG]     |= G_VARIABLE_VAL;
		else							dz->iparray[GRS_FLAGS][G_PITCH_FLAG]     |= G_FIXED_VAL;

		if(dz->brksize[GRS_AMP])		dz->iparray[GRS_FLAGS][G_AMP_FLAG]       |= G_VARIABLE_VAL;
		else							dz->iparray[GRS_FLAGS][G_AMP_FLAG]       |= G_FIXED_VAL;

		if(dz->brksize[GRS_SPACE])		dz->iparray[GRS_FLAGS][G_SPACE_FLAG]     |= G_VARIABLE_VAL;
		else							dz->iparray[GRS_FLAGS][G_SPACE_FLAG]     |= G_FIXED_VAL;

		if(dz->brksize[GRS_BSPLICE])	dz->iparray[GRS_FLAGS][G_BSPLICE_FLAG]   |= G_VARIABLE_VAL;
		else							dz->iparray[GRS_FLAGS][G_BSPLICE_FLAG]   |= G_FIXED_VAL;

		if(dz->brksize[GRS_ESPLICE])	dz->iparray[GRS_FLAGS][G_ESPLICE_FLAG]   |= G_VARIABLE_VAL;
		else							dz->iparray[GRS_FLAGS][G_ESPLICE_FLAG]   |= G_FIXED_VAL;

		if(dz->brksize[GRS_SRCHRANGE])	dz->iparray[GRS_FLAGS][G_RANGE_FLAG]     |= G_VARIABLE_VAL;
		else							dz->iparray[GRS_FLAGS][G_RANGE_FLAG]     |= G_FIXED_VAL;

		if(dz->brksize[GRS_SCATTER])	dz->iparray[GRS_FLAGS][G_SCATTER_FLAG]   |= G_VARIABLE_VAL;
		else							dz->iparray[GRS_FLAGS][G_SCATTER_FLAG]   |= G_FIXED_VAL;

		break;
	case(GRS_PITCHSHIFT):
		if(dz->brksize[GRS_PITCH])		dz->iparray[GRS_FLAGS][G_PITCH_FLAG]     |= G_VARIABLE_VAL;
		else							dz->iparray[GRS_FLAGS][G_PITCH_FLAG]     |= G_FIXED_VAL;
		dz->iparray[GRS_FLAGS][G_SCATTER_FLAG] |= G_FIXED_VAL;
		break;
	case(GRS_TIMESTRETCH):
		if(dz->brksize[GRS_VELOCITY])   dz->iparray[GRS_FLAGS][G_VELOCITY_FLAG]  |= G_VARIABLE_VAL;
		else						    dz->iparray[GRS_FLAGS][G_VELOCITY_FLAG]  |= G_FIXED_VAL;
		dz->iparray[GRS_FLAGS][G_SCATTER_FLAG] |= G_FIXED_VAL;
		break;
	case(GRS_GRANULATE):
		if(dz->brksize[GRS_DENSITY])	dz->iparray[GRS_FLAGS][G_DENSITY_FLAG]   |= G_VARIABLE_VAL;
		else							dz->iparray[GRS_FLAGS][G_DENSITY_FLAG]   |= G_FIXED_VAL;
		dz->iparray[GRS_FLAGS][G_SCATTER_FLAG] |= G_FIXED_VAL;
		break;
	case(GRS_SCRAMBLE):
		if(dz->brksize[GRS_SRCHRANGE])
			dz->iparray[GRS_FLAGS][G_RANGE_FLAG]     |= G_VARIABLE_VAL;
		else
			dz->iparray[GRS_FLAGS][G_RANGE_FLAG]     |= G_FIXED_VAL;
		dz->iparray[GRS_FLAGS][G_SCATTER_FLAG] |= G_FIXED_VAL;

		/* RWD Nov 2003 */
		if(dz->brksize[GRS_GRAINSIZE])	
			dz->iparray[GRS_FLAGS][G_GRAINSIZE_FLAG] |= G_VARIABLE_VAL;
		else
			dz->iparray[GRS_FLAGS][G_GRAINSIZE_FLAG] |= G_FIXED_VAL;


	 	break;
	case(GRS_REVERB):
		if(dz->brksize[GRS_DENSITY])	dz->iparray[GRS_FLAGS][G_DENSITY_FLAG]   |= G_VARIABLE_VAL;
		else							dz->iparray[GRS_FLAGS][G_DENSITY_FLAG]   |= G_FIXED_VAL;

		if(dz->brksize[GRS_PITCH])		dz->iparray[GRS_FLAGS][G_PITCH_FLAG]     |= G_VARIABLE_VAL;
		else							dz->iparray[GRS_FLAGS][G_PITCH_FLAG]     |= G_FIXED_VAL;

		if(dz->brksize[GRS_AMP])		dz->iparray[GRS_FLAGS][G_AMP_FLAG]       |= G_VARIABLE_VAL;
		else							dz->iparray[GRS_FLAGS][G_AMP_FLAG]       |= G_FIXED_VAL;

		if(dz->brksize[GRS_SRCHRANGE])	dz->iparray[GRS_FLAGS][G_RANGE_FLAG]     |= G_VARIABLE_VAL;
		else							dz->iparray[GRS_FLAGS][G_RANGE_FLAG]     |= G_FIXED_VAL;

		dz->iparray[GRS_FLAGS][G_SCATTER_FLAG] |= G_FIXED_VAL;
		dz->iparray[GRS_FLAGS][G_SPACE_FLAG]   |= G_HI_BOUND;
		dz->iparray[GRS_FLAGS][G_SPACE_FLAG]   |= G_FIXED_VAL;

	 	break;
	default:
		sprintf(errstr,"Unknown mode in set_internal_flags()\n");
		return(PROGRAM_ERROR);
	}
#ifdef MULTICHAN
	switch(dz->mode) {
	case(GRS_FULL_MONTY):
		if(dz->param[GRS_HSPACE] == 0.0 && dz->param[GRS_SPACE] == 0)
			dz->iparray[GRS_FLAGS][G_SPACE_FLAG]  = 0;		
		break;
	case(GRS_BRASSAGE):
		if(dz->param[GRS_SPACE] == 0)
			dz->iparray[GRS_FLAGS][G_SPACE_FLAG]  = 0;		
		break;
	}
#endif
	return(FINISHED);
}

/**************************** INITIALISE_CHANNEL_CONFIGURATION *******************************/

#ifndef MULTICHAN

void initialise_channel_configuration(dataptr dz)
{
	if(dz->infile->channels == STEREO) {
		dz->iparam[GRS_CHANNELS] = TRUE;	 /** added MAY 1998 **/
		if(dz->iparray[GRS_FLAGS][G_SPACE_FLAG]) {
			dz->iparam[GRS_CHAN_TO_XTRACT] 	= BOTH_CHANNELS;	/* force stereo input to be mixed to mono */
			dz->iparam[GRS_INCHANS]  		= MONO;   	/* input mixed to mono */
			dz->iparam[GRS_OUTCHANS] 		= STEREO;	/* output spatialised to stereo */
		} else if(dz->iparam[GRS_CHAN_TO_XTRACT]) {		/* specifies which stereo channel to select */
			dz->iparam[GRS_INCHANS]  		= MONO;		/* input is single channel of stereo */
			dz->iparam[GRS_OUTCHANS] 		= MONO;		/* not spatialised */
		} else {
			dz->iparam[GRS_INCHANS]  		= STEREO;	/* input is stereo */
			dz->iparam[GRS_OUTCHANS] 		= STEREO;	/* output is stereo */
		}
	} else {
		dz->iparam[GRS_CHANNELS] = FALSE;
		dz->iparam[GRS_INCHANS]  			= MONO;		/* input is mono */
		if(dz->iparray[GRS_FLAGS][G_SPACE_FLAG])
			dz->iparam[GRS_OUTCHANS] 		= STEREO;	/* output spatialised to stereo */ 
		else
			dz->iparam[GRS_OUTCHANS] 		= MONO;		/* not spatialised */
	}
}

#else

void initialise_channel_configuration(dataptr dz)
{
	if(dz->infile->channels > MONO) {
		dz->iparam[GRS_CHANNELS] = TRUE;
		if(dz->iparam[GRS_CHAN_TO_XTRACT]) {				/* specifies which multichan channel to select */
			dz->iparam[GRS_INCHANS]  		= MONO;			/* input is single channel of multichan, grain is mono */
			if(dz->iparray[GRS_FLAGS][G_SPACE_FLAG])
				dz->iparam[GRS_OUTCHANS] 	= dz->out_chans;	/* output spatialised to multichan */
			else
				dz->iparam[GRS_OUTCHANS] 	= MONO;			/* not spatialised */
			
		} else if(dz->iparray[GRS_FLAGS][G_SPACE_FLAG]) {	/* if spatialising, but NOT extracting a specific channel */
			dz->iparam[GRS_CHAN_TO_XTRACT] 	= ALL_CHANNELS;	/* force multichan input to be mixed to mono */
			dz->iparam[GRS_INCHANS]  		= MONO;   		/* input mixed to mono, grain is mono */
			dz->iparam[GRS_OUTCHANS] 		= dz->out_chans;/* output spatialised to multichan */
		} else {											
			dz->iparam[GRS_INCHANS]  		= dz->infile->channels;	/* Otherwise, grain is multichan */
			dz->iparam[GRS_OUTCHANS] 		= dz->infile->channels;	/* output is same-multichan */
		}
	} else {
		dz->iparam[GRS_CHANNELS] = FALSE;
		dz->iparam[GRS_INCHANS]  			= MONO;			/* input is mono, grain is mono */
		if(dz->iparray[GRS_FLAGS][G_SPACE_FLAG])
			dz->iparam[GRS_OUTCHANS] 		= dz->out_chans;/* output spatialised to multichan */
		else
			dz->iparam[GRS_OUTCHANS] 		= MONO;			/* not spatialised, remains mono */
	}
}

#endif

/*************************** GRANULA_SETUP ***********************/

#ifndef MULTICHAN

int granula_setup(dataptr dz)
{
	int exit_status;
	
	if((exit_status = make_splice_tables(dz))<0)
		return(exit_status);
	set_ranges(dz);
	if((exit_status = calc_overflows(dz))<0)
		return(exit_status);

	dz->iparam[GRS_BUF_SMPXS]   *= dz->iparam[GRS_INCHANS];
	dz->iparam[GRS_BUF_XS]      *= dz->iparam[GRS_INCHANS];
	dz->iparam[GRS_GLBUF_SMPXS] *= dz->iparam[GRS_INCHANS];
	dz->iparam[GRS_LBUF_SMPXS]  *= dz->iparam[GRS_OUTCHANS];		 /* SEPT 1996 */
	dz->iparam[GRS_LBUF_XS]     *= dz->iparam[GRS_OUTCHANS];
	return(FINISHED);
}

#else

int granula_setup(dataptr dz)
{
	int exit_status;
	
	if((exit_status = make_splice_tables(dz))<0)
		return(exit_status);
	set_ranges(dz);
	if((exit_status = calc_overflows(dz))<0)
		return(exit_status);

	dz->iparam[GRS_BUF_SMPXS]   *= dz->iparam[GRS_INCHANS];		//	extra space in INPUT buffer (must accept all input channels)
//	dz->iparam[GRS_BUF_XS]      *= dz->iparam[GRS_INCHANS];		//	 TW this is redundant
	dz->iparam[GRS_GLBUF_SMPXS] *= dz->iparam[GRS_INCHANS];		//	extra space in the GRAIN buffer is either mono 
																//	(where channel is extracted, or all channels mixed to mono) 
																//	or it is multichannel: GRS_INCHANS knows this
	dz->iparam[GRS_LBUF_SMPXS]  *= dz->iparam[GRS_OUTCHANS];	//	The output grain can be multichan via spatialisation, or via using multichan input
																//	or multichan if multichan input goes direct to multichan output
//	dz->iparam[GRS_LBUF_XS]     *= dz->iparam[GRS_OUTCHANS];	//	TW : this is redundant
	return(FINISHED);
}

#endif

/************************** CHECK_FOR_ZEROES ************************************/

int check_for_zeroes(int paramno,dataptr dz)
{
	double *p, *end;
	if(dz->brksize[paramno]) {
		p   = dz->brk[paramno] + 1;		
		end = dz->brk[paramno] + (dz->brksize[paramno] * 2);
		while(p < end) {
			if(flteq(*p,0.0))
				return TRUE;
			p++;
		}
	} else  {
		if(flteq(dz->param[paramno],0.0))
			return TRUE;
	}
	return (FALSE);
}

/************************** CONVERT_VALUE_TO_INT ************************************/

void convert_value_to_int(int paramno,double convertor,dataptr dz)
{
	double *p, *end;
	if(dz->brksize[paramno]) {
		p   = dz->brk[paramno] + 1;		
		end = dz->brk[paramno] + (dz->brksize[paramno] * 2);
		while(p < end) {
			*p = (double)round(*p * convertor);
			p += 2;
		}
		dz->is_int[paramno] = TRUE;
	} else
		dz->iparam[paramno] = round(dz->param[paramno] * convertor);
}

/************************ MAKE_SPLICE_TABLES ******************************/

int make_splice_tables(dataptr dz)
{						 	/* rwd: changed to eliminate f/p division */
	long n;					/* plus quasi-exponential option */
	double dif,val,lastval;
	double length, newsum,lastsum,twodif;
	double *local_btabptr;	/* better safe than sorry! */
	double *local_etabptr;

	if(dz->iparray[GRS_FLAGS][G_BSPLICE_FLAG]<=1) {
		if(dz->iparam[GRS_BSPLICE]==0) {
			dz->iparam[GRS_IS_BTAB] = TRUE;	/* even though it's empty */
			return(FINISHED);
		}
		if((dz->parray[GRS_BSPLICETAB] = (double *)malloc(dz->iparam[GRS_BSPLICE] * sizeof(double)))==NULL) {
			sprintf(errstr,"make_splice_tables(): 1\n");
			return(PROGRAM_ERROR);
		}
		local_btabptr = dz->parray[GRS_BSPLICETAB];
		val = 0.0;
		length = (double)dz->iparam[GRS_BSPLICE];
		if(!dz->vflag[GRS_EXPON]) {
			dif = 1.0/length;
			lastval = dif;
			*local_btabptr++ = val;
			*local_btabptr++ = lastval;
			for(n=2;n<dz->iparam[GRS_BSPLICE];n++) {
				val = lastval + dif;
				lastval = val;
				*local_btabptr++ = val;
			}
		} else {			/* do quasi-exponential splice */
			dif = 1.0/(length*length);
			twodif = dif*2.0;
			lastsum = 0.0;
			lastval = dif;
			*local_btabptr++ = val;
			*local_btabptr++ = lastval;
			for(n=2;n<dz->iparam[GRS_BSPLICE];n++) {
				newsum = lastsum + twodif;
				val = lastval + newsum + dif;
				*local_btabptr++ = val;
				lastval = val;
				lastsum = newsum;
			}
		}   
		dz->iparam[GRS_IS_BTAB] = TRUE;
	}
	if(dz->iparray[GRS_FLAGS][G_ESPLICE_FLAG]<=1) {
		if(dz->iparam[GRS_ESPLICE]==0) {
			dz->iparam[GRS_IS_ETAB] = TRUE;	/* even thogh it's empty! */
			return(FINISHED);
		}
		if((dz->parray[GRS_ESPLICETAB] = (double *)malloc(dz->iparam[GRS_ESPLICE] * sizeof(double)))==NULL) {
			sprintf(errstr,"make_splice_tables(): 2\n");
			return(PROGRAM_ERROR);
		}
		local_etabptr = dz->parray[GRS_ESPLICETAB] + dz->iparam[GRS_ESPLICE];
		val = 0.0;
		length = (double)dz->iparam[GRS_ESPLICE];
		if(!dz->vflag[GRS_EXPON]) {
			dif = 1.0/length;
			lastsum = dif;
			*--local_etabptr = val;
			*--local_etabptr = lastsum;
			for(n=dz->iparam[GRS_ESPLICE]-3;n>=0;n--){
				val = lastsum + dif;
				lastsum = val;	
				*--local_etabptr = val;
			}
		} else {
			dif = 1.0/(length*length);
			twodif = dif*2.0;
			lastsum = 0.0;
			lastval = dif;
			*--local_etabptr = val;
			*--local_etabptr = lastval;
			for(n=dz->iparam[GRS_ESPLICE]-3;n>=0;n--) {
				newsum = lastsum + twodif;
				val = lastval + newsum + dif;
				*--local_etabptr = val;
				lastval = val;
				lastsum = newsum;
			}
		}   
		dz->iparam[GRS_IS_ETAB] = TRUE;
    }
	return(FINISHED);
}

/************************* SET_RANGES *****************************/

void set_ranges(dataptr dz)
{
	int n;
	for(n=0;n<SFLAGCNT;n++) {		
		if(dz->iparray[GRS_FLAGS][n]==RANGED) {
			switch(n) {
			case(G_VELOCITY_FLAG):
				set_this_drange(GRS_VRANGE,GRS_HVELOCITY,GRS_VELOCITY,G_VELOCITY_FLAG,dz);
				break;
			case(G_DENSITY_FLAG):
				set_this_drange(GRS_DRANGE,GRS_HDENSITY,GRS_DENSITY,G_DENSITY_FLAG,dz);
				break;
			case(G_GRAINSIZE_FLAG):
				set_this_range(GRS_GRANGE,GRS_HGRAINSIZE,GRS_GRAINSIZE,G_GRAINSIZE_FLAG,dz);
 				break;
			case(G_BSPLICE_FLAG):
				set_this_range(GRS_BRANGE,GRS_HBSPLICE,GRS_BSPLICE,G_BSPLICE_FLAG,dz);
				break;
			case(G_ESPLICE_FLAG):
				set_this_range(GRS_ERANGE,GRS_HESPLICE,GRS_ESPLICE,G_ESPLICE_FLAG,dz);
				break;
			case(G_PITCH_FLAG):
				set_this_drange(GRS_PRANGE,GRS_HPITCH,GRS_PITCH,G_PITCH_FLAG,dz);
				break;
			case(G_AMP_FLAG):
				/*set_this_range*/set_this_drange(GRS_ARANGE,GRS_HAMP,GRS_AMP,G_AMP_FLAG,dz);
				break;
			case(G_SPACE_FLAG):
				set_this_drange(GRS_SPRANGE,GRS_HSPACE,GRS_SPACE,G_SPACE_FLAG,dz);
				break;
			}
		}
	}
}

/************************ CALC_OVERFLOWS *************************
 *
 * We will attempt to put data into output buffer until we reach its 'end'.
 * But out grain will extend GRAINSIZE beyond this> So we must have
 * an overflow area on the buffer equal to the maximum grainsize.
 *
 * On the input side, we will attempt to read from input buffer until we
 * get to its end, BUT, grainsize extends beyond this. Also, to create
 * an output of GRAINSIZE we need to use an input size of 
 * (grainsize*transposition). So overflow on input buffer = maximum val
 * of (grainsize*transposition).
 * Lbuf_smpxs  = overflow in Lbuf
 * gLbuf_smpxs = MAXIMUM GRAINSIZE
 * buf_smpxs   = overflow in inbuf
 */


int calc_overflows(dataptr dz)	/* DO THIS AFTER THE CONVERSION OF PITCH from SEMITONES */
{
	int exit_status;
	long max_scatter;
	double mt = 1.0;	/* MAXIMUM TRANSPOSITION */
	double mg = 1.0;	/* MAXIMUM GRAINSIZE     */
	double max1, max2;
	int mgi;
	switch(dz->iparray[GRS_FLAGS][G_PITCH_FLAG]) {
	case(NOT_SET):   
		mt = 1.0;
		break;
	case(FIXED):     
		mt = dz->param[GRS_PITCH];   		  								
		break;
	case(VARIABLE):
		if((exit_status = get_maxvalue_in_brktable(&max1,GRS_PITCH,dz))<0)
			return(exit_status);
	  	mt = max1;
	  	break;
	case(RANGED):    
		mt = max(dz->param[GRS_HPITCH],dz->param[GRS_PITCH]);		   		
		break;
	case(RANGE_VLO):
		if((exit_status = get_maxvalue_in_brktable(&max1,GRS_PITCH,dz))<0)
			return(exit_status);
		mt = max(max1,dz->param[GRS_HPITCH]);  											
		break;
	case(RANGE_VHI): 
		if((exit_status = get_maxvalue_in_brktable(&max2,GRS_HPITCH,dz))<0)
			return(exit_status);
		mt = max(max2,dz->param[GRS_PITCH]);  											
		break;
	case(RANGE_VHILO):
		if((exit_status = get_maxvalue_in_brktable(&max1,GRS_PITCH,dz))<0)
			return(exit_status);
		if((exit_status = get_maxvalue_in_brktable(&max2,GRS_HPITCH,dz))<0)
			return(exit_status);
		mt = max(max1,max2);		
		break;
	}
	mt = pow(2.0,mt);	/* CONVERT OCTAVES TO TRANSPOSITION RATIO */

	switch(dz->iparray[GRS_FLAGS][G_GRAINSIZE_FLAG]) {
	case(NOT_SET):
	case(FIXED):     
		mg = (double)dz->iparam[GRS_GRAINSIZE];			       	   						
	   	break;
	case(VARIABLE):  
		if((exit_status = get_maxvalue_in_brktable(&max1,GRS_GRAINSIZE,dz))<0)
			return(exit_status);
		mg = max1;		  					
		break;
	case(RANGED):    
// BUG: OCtober 2009
		mg = max(dz->iparam[GRS_HGRAINSIZE],dz->iparam[GRS_GRAINSIZE]);  			       							
		break;
	case(RANGE_VLO): 
		if((exit_status = get_maxvalue_in_brktable(&max1,GRS_GRAINSIZE,dz))<0)
			return(exit_status);
		mg = max(max1,(double)dz->iparam[GRS_HGRAINSIZE]);		
		break;
	case(RANGE_VHI): 
		if((exit_status = get_maxvalue_in_brktable(&max2,GRS_HGRAINSIZE,dz))<0)
			return(exit_status);
		mg = max(max2,(double)dz->iparam[GRS_GRAINSIZE]);		
		break;
	case(RANGE_VHILO):
		if((exit_status = get_maxvalue_in_brktable(&max1,GRS_GRAINSIZE,dz))<0)
			return(exit_status);
		if((exit_status = get_maxvalue_in_brktable(&max2,GRS_HGRAINSIZE,dz))<0)
			return(exit_status);
		mg = max(max1,max2);  
		break;
	}
	mgi = round(mg)+1;
	dz->iparam[GRS_GLBUF_SMPXS] = mgi;						/* Overflow in outbuf = MAX-grainsize */
	dz->iparam[GRS_BUF_SMPXS]   = round((double)mgi * mt);	/* Overflow in inbuf  = MAX-grainsize * transpos */
	if((exit_status = calc_max_scatter(mgi,&max_scatter,dz))<0)
		return(exit_status);
	adjust_overflows(max_scatter,dz);
	return(FINISHED);
}

/************************ SET_THIS_RANGE ******************************/

void set_this_range(int rangeno,int hino, int lono, int flagno, dataptr dz)
{
	dz->iparam[rangeno]  = dz->iparam[hino] - dz->iparam[lono];
	if(dz->iparam[rangeno]==0)
		dz->iparray[GRS_FLAGS][flagno] = FIXED;
}

/************************ SET_THIS_DRANGE ******************************/

void set_this_drange(int rangeno,int hino, int lono, int flagno, dataptr dz)
{
	dz->param[rangeno]  = dz->param[hino] - dz->param[lono];
	if(flteq(dz->param[rangeno],0.0))
		dz->iparray[GRS_FLAGS][flagno] = FIXED;
}

/******************************** CALC_MAX_SCATTER ****************************/

int calc_max_scatter(int mgi,long *max_scatlen, dataptr dz)		 /* mgi = MAX GRAIN SIZE */
{

#define GRS_ROUNDUP	(0.9999)

	int exit_status;
	long os = 0;
	double sc = 0.0;
	double min1, min2, minn/*, sr = (double)dz->infile->srate*/;
	int k = dz->iparray[GRS_FLAGS][G_DENSITY_FLAG];	
	switch(k) {
	case(NOT_SET):
	case(FIXED):
	case(RANGED):      
		os = (long)(((double)mgi/dz->param[GRS_DENSITY]) + GRS_ROUNDUP);	/* ROUND UP */						
		break;
	case(VARIABLE):    
		if((exit_status = get_minvalue_in_brktable(&min1,GRS_DENSITY,dz))<0)
			return(exit_status);
		os = (long)(((double)mgi/min1) + GRS_ROUNDUP);		  						
		break;
	case(RANGE_VLO):   
		if((exit_status = get_minvalue_in_brktable(&min1,GRS_DENSITY,dz))<0)
			return(exit_status);
		minn = min(min1,dz->param[GRS_HDENSITY]);			
		os = (long)(((double)mgi/minn) + GRS_ROUNDUP); 	
		break;
	case(RANGE_VHI):   
		if((exit_status = get_minvalue_in_brktable(&min2,GRS_HDENSITY,dz))<0)
			return(exit_status);
		minn = min(min2,dz->param[GRS_DENSITY]);			
		os = (long)(((double)mgi/minn) + GRS_ROUNDUP); 	
		break;
	case(RANGE_VHILO): 
		if((exit_status = get_minvalue_in_brktable(&min1,GRS_DENSITY,dz))<0)
			return(exit_status);
		if((exit_status = get_minvalue_in_brktable(&min2,GRS_HDENSITY,dz))<0)
			return(exit_status);
		minn = min(min1,min2);			
		os = (long)(((double)mgi/minn) + GRS_ROUNDUP); 	
		break;
	}
	switch(dz->iparray[GRS_FLAGS][G_SCATTER_FLAG]) {
	case(NOT_SET):
	case(FIXED):     
		sc = dz->param[GRS_SCATTER];   		   	   		
		break;
	case(VARIABLE):  
		if((exit_status = get_maxvalue_in_brktable(&sc,GRS_SCATTER,dz))<0)
			return(exit_status);
		break;
	}
	*max_scatlen = (long)(((double)os * sc) + GRS_ROUNDUP); /* ROUND UP */
	return(FINISHED);
}

/************************ ADJUST_OVERFLOWS ***************************
 * GRS_LBUF_SMPXS		   = overflow in calculation buffer Lbuf 
 * iparam[GRS_GLBUF_SMPXS] = MAXIMUM GRAINSIZE 
 * GRS_BUF_SMPXS		   = overflow in inbuf
 */
 /* RW NB we eliminate almost everything! */
void adjust_overflows(long mscat, dataptr dz)
{
	dz->iparam[GRS_BUF_SMPXS]   += SAFETY;							/* ADD SAFETY MARGINS !! */
	dz->iparam[GRS_GLBUF_SMPXS] += OSAFETY;
	dz->iparam[GRS_LBUF_SMPXS]   = dz->iparam[GRS_GLBUF_SMPXS] + mscat;
}

#ifdef MULTICHAN

/************************** CHECK_SPATIALISATION_DATA ************************************/

int check_spatialisation_data(dataptr dz)
{
	int exit_status;
	double maxspace, maxspaceh = 0.0;

	if(dz->process==SAUSAGE) {
		if(dz->brksize[GRS_HSPACE]) {
			if((exit_status = get_maxvalue_in_brktable(&maxspaceh,GRS_HSPACE,dz))<0)
				return(exit_status);
		} else
			maxspaceh = dz->param[GRS_HSPACE];
		if(dz->brksize[GRS_SPACE]) {
			if((exit_status = get_maxvalue_in_brktable(&maxspace,GRS_SPACE,dz))<0)
				return(exit_status);
		} else
			maxspace = dz->param[GRS_SPACE];
		maxspace = max(maxspace,maxspaceh);
	} else {
		switch(dz->mode) {
		case(GRS_FULL_MONTY):
			if(dz->brksize[GRS_HSPACE]) {
				if((exit_status = get_maxvalue_in_brktable(&maxspaceh,GRS_HSPACE,dz))<0)
					return(exit_status);
			} else
				maxspaceh = dz->param[GRS_HSPACE];
			/* fall thro */
		case(GRS_BRASSAGE):
			if(dz->brksize[GRS_SPACE]) {
				if((exit_status = get_maxvalue_in_brktable(&maxspace,GRS_SPACE,dz))<0)
					return(exit_status);
			} else
				maxspace = dz->param[GRS_SPACE];
			break;
		default:
			return FINISHED;
		}
		if(dz->mode == GRS_FULL_MONTY)
			maxspace = max(maxspace,maxspaceh);
	}
	if(dz->out_chans == 2)  {
		if(maxspace > 1.0) {
			sprintf(errstr,"SPATIAL DATA MAX VALUE (%lf) INCOMPATIBLE WITH OUTPUT CHANNEL-CNT (STEREO) (data range for stereo 0-1)\n",maxspace);
			return(DATA_ERROR);
		}
	} else {
		if(maxspace > dz->out_chans) {
			sprintf(errstr,"SPATIAL DATA MAX VALUE (%lf) INCOMPATIBLE WITH OUTPUT CHANNEL CNT (dz->out_chans) SPECIFIED\n",maxspace);
			return(DATA_ERROR);
		}
	}
	return(FINISHED);
}

#endif
