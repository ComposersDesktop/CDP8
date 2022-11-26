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
#include <structures.h>
#include <tkglobals.h>
#include <globcon.h>
#include <processno.h>
#include <modeno.h>
#include <arrays.h>
#include <envel.h>
#include <cdpmain.h>

#include <sfsys.h>
#include <osbind.h>

static int  create_initial_envelope_params(double infiledur,dataptr dz);
static int  reallocate_initial_envelope_params(dataptr dz);
static int  allocate_initial_envelope_params(dataptr dz);
static int  find_and_test_attack_peak(dataptr dz);
static int  find_attack_point(float *buf,int *attack_point_found,int *attack_point,dataptr dz);
static int  find_attack_point_using_gate(float *buf,int *attack_point_found,int *attack_point,dataptr dz);
static int  attack_point_definitively_found(int *n,float *buf,/*int*/double *attack_level,int *attack_point,
				int previous_total_samps_read,dataptr dz);
static int  level_is_too_high(float *buf,int abs_samppos,dataptr dz);
static int  level_is_too_high_in_thisbuf(float *buf, double *gain, double tail_gainstep,int chans,
				int n,int *too_high,dataptr dz);
static int  go_to_search_start(dataptr dz);
static int  go_to_max_sample(float *buf,int *attack_point_found,int *attack_point,dataptr dz);
static int  find_local_maximum(float *buf,int *attack_point_found,int *attack_point,dataptr dz);
static int  level_is_too_high_in_thisbuf2(float *buf, double *gain, double tail_gainstep,double maxgain,
				int chans,int n,int *sampcnt,int abs_onset_len,int *too_high,dataptr dz);
static int  level_is_too_high_in_onset(float *buf,int abs_samppos,double tail_gainstep,dataptr dz);
static int  restore_buffers_and_file_to_original_condition(float *buf,int abs_samppos,dataptr dz);
static int  level_is_too_high_in_tail(float *buf,int abs_samppos,double tail_gainstep,dataptr dz);
static int  create_tremol_sintable(dataptr dz);
static void convert_tremol_frq_to_log10(dataptr dz);

/***************************** ENVEL_PREPROCESS ******************************/

int envel_preprocess(dataptr dz)
{
	int  exit_status;
	int filestsmplen;
//TW UPDATES
	double *d;
	double maxsplen, minwidth;
	int n;
	double sec = (double)(ENV_FSECSIZE)/(double)(dz->infile->channels * dz->infile->srate);
	switch(dz->process) {
	case(ENV_WARPING):
	case(ENV_RESHAPING):
	case(ENV_REPLOTTING):
		switch(dz->mode) {
		case(ENV_NORMALISE):	case(ENV_REVERSE):		case(ENV_CEILING):
		case(ENV_EXAGGERATING):	case(ENV_ATTENUATING):	case(ENV_FLATTENING):
		case(ENV_TSTRETCHING):	case(ENV_CORRUGATING):	case(ENV_PEAKCNT):
		case(ENV_LIFTING):		case(ENV_GATING):
		case(ENV_DUCKED):		case(ENV_TRIGGERING):
			break;
		case(ENV_LIMITING):
			if(!first_param_not_less_than_second(ENV_LIMIT,ENV_THRESHOLD,dz))
				return(DATA_ERROR);
			break;
		case(ENV_INVERTING):
			if(!first_param_greater_than_second(ENV_MIRROR,ENV_GATE,dz))
				return(DATA_ERROR);
			break;
		case(ENV_EXPANDING):
			if(!first_param_not_less_than_second(ENV_THRESHOLD,ENV_GATE,dz))
				return(DATA_ERROR);
			break;
		default:
			sprintf(errstr,"Unknown case: envwarp,reshape or replot: envelope_preprocess()\n");
			return(PROGRAM_ERROR);
		}
		break;
	case(ENV_CURTAILING):
		switch(dz->mode) {
		case(ENV_START_AND_END):	
		case(ENV_START_AND_DUR):
			switch(dz->iparam[ENV_TIMETYPE]) {
			case(ENV_TIMETYPE_SMPS):
				dz->param[ENV_STARTTIME] = (double)(round(dz->param[ENV_STARTTIME])/dz->infile->channels);
				dz->param[ENV_ENDTIME]   = (double)(round(dz->param[ENV_ENDTIME])/dz->infile->channels);
				/* fall thro */
 			case(ENV_TIMETYPE_STSMPS):
				dz->param[ENV_STARTTIME] = dz->param[ENV_STARTTIME]/(double)dz->infile->srate;
				dz->param[ENV_ENDTIME]   = dz->param[ENV_ENDTIME]/(double)dz->infile->srate;
				break;
 			case(ENV_TIMETYPE_SECS):
				break;
			default:
				sprintf(errstr,"Unknown case: ENV_CURTAILING: envelope_preprocess()\n");
				return(PROGRAM_ERROR);
			}
			if(dz->param[ENV_STARTTIME] < 0.0 || dz->param[ENV_STARTTIME] >= dz->duration) {
				sprintf(errstr,"Start of fade time : out of range.\n");
				return(USER_ERROR);
			}
			if(dz->mode==ENV_START_AND_DUR) {	 /* "ENDTIME" is actually FADE DURATION */
				if(dz->param[ENV_ENDTIME] < FLTERR) {
					sprintf(errstr,"Negative or zero fade duration : impossible.\n");
					return(USER_ERROR);
				}
				dz->param[ENV_ENDTIME] = dz->param[ENV_STARTTIME] + dz->param[ENV_ENDTIME];
				if(dz->param[ENV_ENDTIME] > dz->duration + FLTERR) {
					fprintf(stdout,"WARNING: Fade duration goes beyond file end: curtailing to file end.\n");
					fflush(stdout);
				}
				if(dz->param[ENV_ENDTIME] > dz->duration-FLTERR) {
					dz->param[ENV_ENDTIME] = dz->duration;
				}
			} else {
				if(dz->param[ENV_STARTTIME] + sec >= dz->param[ENV_ENDTIME]) {
					sprintf(errstr,"Fade times too close or reversed.\n");
					return(USER_ERROR);
				}
			 	if(dz->param[ENV_ENDTIME] < 0.0) {
					sprintf(errstr,"End-of-fade time : out of range.\n");
					return(USER_ERROR);
				} else if(dz->param[ENV_ENDTIME] > dz->duration) {
					dz->param[ENV_ENDTIME] = dz->duration;
					fprintf(stdout,"WARNING: End-of-fade beyond file end: defaulting to end of file.\n");
					fflush(stdout);
				}
			}
			break;
		case(ENV_START_ONLY):
			switch(dz->iparam[ENV_TIMETYPE]) {
			case(ENV_TIMETYPE_SMPS):
				dz->param[ENV_STARTTIME] = (double)(round(dz->param[ENV_STARTTIME])/dz->infile->channels);
				/* fall thro */
 			case(ENV_TIMETYPE_STSMPS):
				dz->param[ENV_STARTTIME] = dz->param[ENV_STARTTIME]/(double)dz->infile->srate;
				break;
 			case(ENV_TIMETYPE_SECS):
				break;
			default:
				sprintf(errstr,"Unknown case: ENV_CURTAILING: envelope_preprocess()\n");
				return(PROGRAM_ERROR);
			}
			dz->param[ENV_ENDTIME] = dz->duration;
		}
		dz->itemcnt = 3;
		if((exit_status = create_initial_envelope_params(dz->duration,dz))<0)
			return(exit_status);
		if((exit_status = establish_additional_brktable(dz))<0)
			return(exit_status);
		break;
	case(ENV_DOVETAILING):
		filestsmplen = dz->insams[0]/dz->infile->channels;
		switch(dz->iparam[ENV_TIMETYPE]) {
		case(ENV_TIMETYPE_SMPS):
			dz->param[ENV_STARTTRIM] = 
			(double)round(dz->param[ENV_STARTTRIM])/(double)(dz->infile->channels * dz->infile->srate);
			dz->param[ENV_ENDTRIM]   = 
			(double)round(dz->param[ENV_ENDTRIM])/(double)(dz->infile->channels * dz->infile->srate);
			dz->param[ENV_ENDTRIM]   = dz->duration - dz->param[ENV_ENDTRIM];
			break;
		case(ENV_TIMETYPE_STSMPS):
			dz->param[ENV_STARTTRIM] = (double)round(dz->param[ENV_STARTTRIM])/(double)dz->infile->srate;
			dz->iparam[ENV_ENDTRIM]  = (int)round(dz->param[ENV_ENDTRIM]);
			dz->iparam[ENV_ENDTRIM]  = (int)(filestsmplen - dz->iparam[ENV_ENDTRIM]);
			dz->param[ENV_ENDTRIM]   = (double)dz->iparam[ENV_ENDTRIM]/(double)dz->infile->srate;
			break;
 		case(ENV_TIMETYPE_SECS):
			dz->param[ENV_ENDTRIM]   = dz->duration - dz->param[ENV_ENDTRIM];
			break;
		default:
			sprintf(errstr,"Unknown case: ENV_DOVETAILING: envelope_preprocess()\n");
			return(PROGRAM_ERROR);
		}
		if(dz->param[ENV_ENDTRIM] < 0.0) {
			sprintf(errstr,"End Trim is too large.\n");
			return(USER_ERROR);
		}
		if(dz->param[ENV_STARTTRIM] > dz->duration) {
			sprintf(errstr,"Start Trim is too large.\n");
			return(USER_ERROR);
		}
		if(dz->param[ENV_ENDTRIM] < dz->param[ENV_STARTTRIM]) {
			sprintf(errstr,"Start and End Trims overlap: cannot proceed.\n");
			return(USER_ERROR);
		}
		if(flteq(dz->param[ENV_ENDTRIM],dz->duration) && flteq(dz->param[ENV_STARTTRIM],0.0)) {
			sprintf(errstr,"Start and End Trims are zero: no change in file.\n");
			return(USER_ERROR);
		}
		if(flteq(dz->param[ENV_ENDTRIM],dz->duration) || flteq(dz->param[ENV_STARTTRIM],0.0))
			dz->itemcnt = 3;
		else
			dz->itemcnt = 4;
		if((exit_status = create_initial_envelope_params(dz->duration,dz))<0)
			return(exit_status);
		if((exit_status = establish_additional_brktable(dz))<0)
			return(exit_status);
		break;
	case(ENV_SWELL):
		dz->itemcnt = 3;
		if((exit_status = create_initial_envelope_params(dz->duration,dz))<0)
			return(exit_status);
		if((exit_status = establish_additional_brktable(dz))<0)
			return(exit_status);
		break;
	case(ENV_ATTACK):
		dz->param[ENV_ATK_GATE] *= (double)F_MAXSAMP;		/* RWD actually redundant, but do it anyway */
		if((exit_status = find_and_test_attack_peak(dz))<0)
			return(exit_status);
		dz->param[ENV_ATK_ONSET] *= MS_TO_SECS;
		dz->param[ENV_ATK_TAIL]  *= MS_TO_SECS;
		if(dz->iparam[ENV_ATK_ENVTYPE] == ENVTYPE_EXP)
			dz->iparam[ENV_ATK_ENVTYPE] = ENVTYPE_STEEP;
		dz->itemcnt = 5;
		if((exit_status = create_initial_envelope_params(dz->duration,dz))<0)
			return(exit_status);
		if((exit_status = establish_additional_brktable(dz))<0)
			return(exit_status);
		break;
	case(ENV_CREATE):
		if(dz->mode==ENV_ENVFILE_OUT) {	
			if(dz->extrabrkno>=0) {		  /* If a brktable passed in from framework */
				if(dz->brk == NULL || dz->brk[dz->extrabrkno] == NULL) {	/* clear it */
					sprintf(errstr,"Problem freeing inbrkfile-table: ENV_CREATE: envelope_preprocess()\n");
					return(PROGRAM_ERROR);
				} 
				free(dz->brk[dz->extrabrkno]);
			}								 		 /* allocate temporary brkpnt array */
			if((exit_status = establish_additional_brktable(dz))<0)
				return(exit_status);
		}
		break;	
	case(ENV_EXTRACT):   case(ENV_IMPOSE): 	  	case(ENV_REPLACE):
	case(ENV_ENVTOBRK):  case(ENV_ENVTODBBRK):	case(ENV_BRKTOENV):
	case(ENV_DBBRKTOENV):case(ENV_DBBRKTOBRK):	case(ENV_BRKTODBBRK):
//TW NEW CASE
	case(ENV_PROPOR):
		break;	
	case(ENV_PLUCK):
		if(dz->infile->channels!=MONO) {
			sprintf(errstr,"This process only works with MONO files\n");
			return(USER_ERROR);
		}
		initrand48();
		break;	
	case(ENV_TREMOL):
		if(dz->brksize[ENV_TREM_FRQ] && dz->mode==ENV_TREM_LOG)
			convert_tremol_frq_to_log10(dz);
		return create_tremol_sintable(dz);
		break;	
	case(TIME_GRID):
		dz->tempsize = dz->insams[0] * dz->iparam[GRID_COUNT];
		if(dz->brksize[GRID_SPLEN] > 0) {
			d = dz->brk[GRID_SPLEN] + 1;
			for(n= 0;n < dz->brksize[GRID_SPLEN];n++) {
				*d *= MS_TO_SECS;
				d += 2;
			}
		} else
			dz->param[GRID_SPLEN] *= MS_TO_SECS;
		if(dz->brksize[GRID_WIDTH]) {
			if((exit_status = get_minvalue_in_brktable(&minwidth,GRID_WIDTH,dz))<0)
				return(exit_status);
		} else
			minwidth = dz->param[GRID_WIDTH];
		if((exit_status = get_maxvalue(GRID_SPLEN,&maxsplen,dz))<0)
			return(exit_status);
		if(maxsplen >= minwidth) {
			sprintf(errstr,"Maximum splicelen too large for minimum grid-width\n");
			return(DATA_ERROR);
		}
		break;
	default:
		sprintf(errstr,"Unknown case: envelope_preprocess()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/***************************** CREATE_INITIAL_ENVELOPE_PARAMS ******************************/

int create_initial_envelope_params(double infiledur,dataptr dz)
{
	int exit_status, n, in_itemcnt;
	double attack_start, attack_end;
	if((exit_status = allocate_initial_envelope_params(dz))<0)
		return(exit_status);
	switch(dz->process) {
	case(ENV_CURTAILING):
		dz->parray[ENV_CREATE_INTIME][0]  = 0.0; 
		dz->parray[ENV_CREATE_INLEVEL][0] = 1.0;
		dz->iparray[ENV_SLOPETYPE][0]	  = ENVTYPE_LIN;

		dz->parray[ENV_CREATE_INTIME][1]  = dz->param[ENV_STARTTIME]; 
		dz->parray[ENV_CREATE_INLEVEL][1] = 1.0;
		dz->iparray[ENV_SLOPETYPE][1]	  = ENVTYPE_LIN;

		dz->parray[ENV_CREATE_INTIME][2]  = dz->param[ENV_ENDTIME]; 
		dz->parray[ENV_CREATE_INLEVEL][2] = 0.0;
		switch(dz->mode) {
		case(ENV_START_AND_END):
		case(ENV_START_AND_DUR):
		case(ENV_START_ONLY):
			dz->iparray[ENV_SLOPETYPE][2]	  = dz->iparam[ENV_ENVTYPE];
			break;
		default:
			dz->iparray[ENV_SLOPETYPE][2]	  = ENVTYPE_DBL;
			break;
		}
		break;
	case(ENV_DOVETAILING):
		n = 0;
		if(flteq(dz->param[ENV_STARTTRIM],0.0)) {
			dz->parray[ENV_CREATE_INTIME][n]  = 0.0; 
			dz->parray[ENV_CREATE_INLEVEL][n] = 1.0;
			dz->iparray[ENV_SLOPETYPE][n]	  = ENVTYPE_LIN;
			n++;
		} else {
			dz->parray[ENV_CREATE_INTIME][n]  = 0.0; 
			dz->parray[ENV_CREATE_INLEVEL][n] = 0.0;
			dz->iparray[ENV_SLOPETYPE][n]	  = ENVTYPE_LIN;
			n++;
			dz->parray[ENV_CREATE_INTIME][n]  = dz->param[ENV_STARTTRIM]; 
			dz->parray[ENV_CREATE_INLEVEL][n] = 1.0;
			if(dz->mode==DOVE)
				dz->iparray[ENV_SLOPETYPE][n]	  = dz->iparam[ENV_STARTTYPE];
			else
				dz->iparray[ENV_SLOPETYPE][n]	  = ENVTYPE_DBL;
			n++;
		}
		if(flteq(dz->param[ENV_ENDTRIM],infiledur)) {
			dz->parray[ENV_CREATE_INTIME][n]  = infiledur; 
			dz->parray[ENV_CREATE_INLEVEL][n] = 1.0;
			dz->iparray[ENV_SLOPETYPE][n]	  = ENVTYPE_LIN;
		} else {
			dz->parray[ENV_CREATE_INTIME][n]  = dz->param[ENV_ENDTRIM]; 
			dz->parray[ENV_CREATE_INLEVEL][n] = 1.0;
			dz->iparray[ENV_SLOPETYPE][n]	  = ENVTYPE_LIN;
			n++;
			dz->parray[ENV_CREATE_INTIME][n]  = infiledur; 
			dz->parray[ENV_CREATE_INLEVEL][n] = 0.0;
			if(dz->mode==DOVE)
				dz->iparray[ENV_SLOPETYPE][n]	  = dz->iparam[ENV_ENDTYPE];
			else
				dz->iparray[ENV_SLOPETYPE][n]	  = ENVTYPE_DBL;
		}
		break;
	case(ENV_SWELL):
		dz->parray[ENV_CREATE_INTIME][0]  = 0.0; 
		dz->parray[ENV_CREATE_INLEVEL][0] = 0.0;
		dz->iparray[ENV_SLOPETYPE][0]	  = ENVTYPE_LIN;

		dz->parray[ENV_CREATE_INTIME][1]  = dz->param[ENV_PEAKTIME]; 
		dz->parray[ENV_CREATE_INLEVEL][1] = 1.0;
		dz->iparray[ENV_SLOPETYPE][1]	  = dz->iparam[ENV_ENVTYPE];

		dz->parray[ENV_CREATE_INTIME][2]  = infiledur; 
		dz->parray[ENV_CREATE_INLEVEL][2] = 0.0;
		dz->iparray[ENV_SLOPETYPE][2]	  = dz->iparam[ENV_ENVTYPE];
		break;
	case(ENV_ATTACK):
		in_itemcnt = dz->itemcnt;
		n = 0;
		attack_start = max(0.0,dz->param[ENV_ATK_TIME] - dz->param[ENV_ATK_ONSET]);
		attack_end   = min(infiledur,dz->param[ENV_ATK_TIME] + dz->param[ENV_ATK_TAIL]); 
		if(dz->param[ENV_ATK_TIME] > 0.0) {
			dz->parray[ENV_CREATE_INTIME][n]  = 0.0; 
			dz->parray[ENV_CREATE_INLEVEL][n] = 1.0;
			dz->iparray[ENV_SLOPETYPE][n]	  = ENVTYPE_LIN;
			n++;
			if(attack_start > 0.0) {
				dz->parray[ENV_CREATE_INTIME][n]  = attack_start; 
				dz->parray[ENV_CREATE_INLEVEL][n] = 1.0;
				dz->iparray[ENV_SLOPETYPE][n]	  = ENVTYPE_LIN;
				n++;
			}
			dz->parray[ENV_CREATE_INTIME][n]  = dz->param[ENV_ATK_TIME]; 
			dz->parray[ENV_CREATE_INLEVEL][n] = dz->param[ENV_ATK_GAIN];
			dz->iparray[ENV_SLOPETYPE][n]	  = dz->iparam[ENV_ATK_ENVTYPE];
			n++;
		} else {
			dz->parray[ENV_CREATE_INTIME][n]  = 0.0; 
			dz->parray[ENV_CREATE_INLEVEL][n] = dz->param[ENV_ATK_GAIN];
			dz->iparray[ENV_SLOPETYPE][n]	  = ENVTYPE_LIN;
			n++;
		}
		if(attack_end < infiledur) {
			dz->parray[ENV_CREATE_INTIME][n]  = attack_end; 
			dz->parray[ENV_CREATE_INLEVEL][n] = 1.0;
			dz->iparray[ENV_SLOPETYPE][n]	  = dz->iparam[ENV_ATK_ENVTYPE];
			n++;
			dz->parray[ENV_CREATE_INTIME][n]  = infiledur; 
			dz->parray[ENV_CREATE_INLEVEL][n] = 1.0;
			dz->iparray[ENV_SLOPETYPE][n]	  = ENVTYPE_LIN;
			n++;
		} else {
			dz->parray[ENV_CREATE_INTIME][3]  = infiledur; 
			dz->parray[ENV_CREATE_INLEVEL][3] = 1.0;
			dz->iparray[ENV_SLOPETYPE][3]	  = dz->iparam[ENV_ATK_ENVTYPE];
		}
		dz->itemcnt = n;
		if((dz->itemcnt != in_itemcnt) && (exit_status = reallocate_initial_envelope_params(dz))<0)
			return(exit_status);
		break;
	default:
		sprintf(errstr,"Unknown case in create_initial_envelope_params()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/***************************** REALLOCATE_INITIAL_ENVELOPE_PARAMS ******************************/

int reallocate_initial_envelope_params(dataptr dz)
{
	if((dz->parray[ENV_CREATE_INLEVEL] = (double *)realloc
	(dz->parray[ENV_CREATE_INLEVEL],dz->itemcnt * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to reallocate input level array.\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[ENV_CREATE_INTIME]  = (double *)realloc
	(dz->parray[ENV_CREATE_INTIME],dz->itemcnt * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to reallocate input time array.\n");
		return(MEMORY_ERROR);
	}
	if((dz->iparray[ENV_SLOPETYPE]  = (int *)realloc
	(dz->iparray[ENV_SLOPETYPE],dz->itemcnt * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to reallocate input slopetype array.\n");
		return(MEMORY_ERROR);
	}			
	return(FINISHED);
}

/***************************** ALLOCATE_INITIAL_ENVELOPE_PARAMS ******************************/

int allocate_initial_envelope_params(dataptr dz)
{
	if((dz->parray[ENV_CREATE_INLEVEL] = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for input level array.\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[ENV_CREATE_INTIME]  = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for input time array.\n");
		return(MEMORY_ERROR);
	}
	if((dz->iparray[ENV_SLOPETYPE]  = (int *)malloc(dz->itemcnt * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for input slopetype array.\n");
		return(MEMORY_ERROR);
	}
	return(FINISHED);
}

/****************************** FIND_AND_TEST_ATTACK_PEAK *******************************/

int find_and_test_attack_peak(dataptr dz)
{
	int  exit_status;
	int  chans = dz->infile->channels;
	int  attack_point_found = FALSE;
	int attack_point=0, abs_samppos;
	float *buf = dz->sampbuf[0];
	if((exit_status = find_attack_point(buf,&attack_point_found,&attack_point,dz))<0)
		return(exit_status);
	if (!attack_point_found) {
		sprintf(errstr,"Gate amplitude not exceeded.\n");
		return(GOAL_FAILED);
	}
	if(dz->insams[0] - attack_point < 
	   round((ENV_MIN_ATK_ONSET * MS_TO_SECS) * (double)(dz->infile->srate * chans))) {
		sprintf(errstr,"Attack time too late in file to create a new attack.\n");
		return(GOAL_FAILED);
	}
	abs_samppos = (attack_point/chans) * chans;
	if((exit_status = restore_buffers_and_file_to_original_condition(buf,abs_samppos,dz))<0)
		return(exit_status);
	if(level_is_too_high(buf,abs_samppos,dz)) {
		sprintf(errstr,"The attack may distort with this gain level.\n");
		return(GOAL_FAILED);
	}
	if(dz->mode==ENV_ATK_XTIME)
		dz->param[ENV_ATK_TIME] = dz->param[ENV_ATK_ATTIME];
	else
		dz->param[ENV_ATK_TIME] = (double)(attack_point/chans)/(double)(dz->infile->srate);
	reset_filedata_counters(dz);
	if(sndseekEx(dz->ifd[0],0,0)<0) {
		sprintf(errstr,"sndseekEx failed: find_and_test_attack_peak()\n");
		return(SYSTEM_ERROR);
	}
	return(FINISHED);
}

/****************************** FIND_ATTACK_POINT *******************************/

int find_attack_point(float *buf,int *attack_point_found,int *attack_point,dataptr dz)
{
	int exit_status;
//	int definitely_found = FALSE;
	switch(dz->mode) {
	case(ENV_ATK_GATED):		
		return find_attack_point_using_gate(buf,attack_point_found,attack_point,dz);
	case(ENV_ATK_TIMED):		
		if((exit_status = go_to_search_start(dz))<0)
			return(exit_status);
		if((exit_status = find_local_maximum(buf,attack_point_found,attack_point,dz))<0)
			return(exit_status);
		break;
	case(ENV_ATK_XTIME):
		*attack_point = round(dz->param[ENV_ATK_ATTIME] * (double)dz->infile->srate) * dz->infile->channels;
		*attack_point_found = TRUE;
		break;
	case(ENV_ATK_ATMAX):		
		return go_to_max_sample(buf,attack_point_found,attack_point,dz);
	default:
		sprintf(errstr,"Unknown case in find_attack_point()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/****************************** FIND_ATTACK_POINT_USING_GATE *******************************/

int find_attack_point_using_gate(float *buf,int *attack_point_found,int *attack_point,dataptr dz)
{
	int exit_status;
	int definitely_found = FALSE;
	/*int*/double attack_level=0.0;
	int previous_total_samps_read, n;
	while(!definitely_found && (dz->samps_left > 0)) {
		previous_total_samps_read = dz->total_samps_read;
		if((exit_status = read_samps(buf,dz))<0)
			return(exit_status);
		n = 0;
		while(n<dz->ssampsread) {
			if(!(*attack_point_found)) {
				/*RWD be a little efficient here... */
				double fabsamp = fabs(buf[n]);
				if(/*abs(buf[n])*/fabsamp > dz->param[ENV_ATK_GATE]) {
					attack_level  = /* abs(buf[n])*/fabsamp;
					*attack_point = previous_total_samps_read + n;
					*attack_point_found = TRUE;
				}
			}
			if(*attack_point_found) {
				if((definitely_found = 
				attack_point_definitively_found(&n,buf,&attack_level,attack_point,previous_total_samps_read,dz))==TRUE)
					break;
			} else
				n++;
		}
	}
	return(FINISHED);
}

/****************************** ATTACK_POINT_DEFINITIVELY_FOUND *******************************/

int attack_point_definitively_found
(int *n,float *buf,/*int*/double *attack_level,int *attack_point,int previous_total_samps_read,dataptr dz)
{
	/*int*/double thislevel;
	while(*n < dz->ssampsread) {
		thislevel = fabs(buf[*n]);
		if(thislevel < *attack_level)
			return TRUE;
		else if(thislevel > *attack_level) {
			*attack_level = thislevel;
			*attack_point = previous_total_samps_read + *n;
		}
		(*n)++;
	}
	return(FALSE);
}

/****************************** LEVEL_IS_TOO_HIGH *******************************
 *
 * This is an overestimate of risk!!
 */

int level_is_too_high(float *buf,int abs_samppos,dataptr dz)
{
	int exit_status;
//	int chans = dz->infile->channels;
	int too_high = FALSE;
	double tail_gainstep = dz->param[ENV_ATK_GAIN] - 1.0;
	switch(dz->mode) {
	case(ENV_ATK_ATMAX):
		break;
	case(ENV_ATK_TIMED):
	case(ENV_ATK_XTIME):
		if((exit_status = level_is_too_high_in_onset(buf,abs_samppos,tail_gainstep,dz))<0)
			return(exit_status);
  		if((too_high = exit_status)==TRUE)
			break;
		/* fall thro */
	case(ENV_ATK_GATED):
		if((exit_status = level_is_too_high_in_tail(buf,abs_samppos,tail_gainstep,dz))<0)
			return(exit_status);
		too_high = exit_status;
		break;
	default:
		sprintf(errstr,"Unknown case in level_is_too_high()\n");
		return(PROGRAM_ERROR);
	}
	return(too_high);
}

/****************************** LEVEL_IS_TOO_HIGH_IN_THISBUF *******************************/

int level_is_too_high_in_thisbuf
(float *buf, double *gain, double tail_gainstep,int chans,int n,int *too_high,dataptr dz)
{
	int m;
	double thislevel;
	while(n < dz->ssampsread) {
		for(m=0;m<chans;m++) {
			if((thislevel = (double) fabs(buf[n+m]) * (*gain)) > F_MAXSAMP) {
				*too_high = TRUE;
				return(FINISHED);
			}
		}
		if((*gain -= tail_gainstep)<=1.0)
			return(FINISHED);
		n += chans;
	}
	return(CONTINUE);
}

/****************************** GO_TO_SEARCH_START *******************************/

int go_to_search_start(dataptr dz)
{
	int seccnt;
	int shsecsize = ENV_FSECSIZE;

	double starttime = max(0.0,dz->param[ENV_ATK_ATTIME] - (ENV_ATK_SRCH * MS_TO_SECS));
	int startsamp = round(starttime * (double)dz->infile->srate) * dz->infile->channels;
	if(startsamp >= dz->insams[0]) {
		sprintf(errstr,"Error in sample arithmetic: go_to_search_start()\n");
		return(PROGRAM_ERROR);
	}
	seccnt = startsamp/shsecsize;	/* TRUNCATE */
	startsamp = seccnt * shsecsize;

	if(sndseekEx(dz->ifd[0],startsamp,0)<0) {
		sprintf(errstr,"sndseekEx failed: go_to_search_start()\n");
		return(SYSTEM_ERROR);
	}
	/*dz->total_bytes_read = startbyte;*/
	dz->total_samps_read = startsamp;
	dz->samps_left = dz->insams[0] - startsamp;
	return(FINISHED);
}

/****************************** GO_TO_MAX_SAMPLE *******************************/

int go_to_max_sample(float *buf,int *attack_point_found,int *attack_point,dataptr dz)
{
	int  exit_status;
	int previous_total_samps_read = 0, n;
	/*int*/double  thislevel;
	/*int*/double  attack_level = -1.0;
	*attack_point_found = TRUE;
	while(dz->samps_left > 0) {
		previous_total_samps_read = dz->total_samps_read;
		if((exit_status = read_samps(dz->sampbuf[0],dz))<0)
			return(exit_status);
		n = 0;
		while(n<dz->ssampsread) {
			if((thislevel = fabs(buf[n])) > attack_level) {
				attack_level = thislevel;
				*attack_point = previous_total_samps_read + n;
			}
			n++;
		}
	}
	return(FINISHED);
}

/****************************** FIND_LOCAL_MAXIMUM *******************************/

int find_local_maximum(float *buf,int *attack_point_found,int *attack_point,dataptr dz)
{
	int exit_status;
	/*int*/double thislevel;
	int n, m = 0;
	/*int*/double attack_level = -1.0;
	int finished = FALSE;
	int previous_total_samps_read;
	int search_limit = round((ENV_ATK_SRCH * 2.0) * MS_TO_SECS * (double)dz->infile->srate);
	search_limit *= dz->infile->channels;
	if(dz->samps_left <=0) {
		sprintf(errstr,"Error in buffer arithmetic: find_local_maximum()\n");
		return(PROGRAM_ERROR);
	}
	while(!finished && (dz->samps_left > 0)) {
		previous_total_samps_read = dz->total_samps_read;
		if((exit_status = read_samps(dz->sampbuf[0],dz))<0)
			return(exit_status);
		n = 0;
		while(n<dz->ssampsread) {
			if((thislevel = fabs(buf[n])) > attack_level) {
				attack_level = thislevel;
				*attack_point = previous_total_samps_read + n;
			}
			n++;
			if(++m >= search_limit) {
				finished = TRUE;
				break;
			}
		}
	}
	*attack_point_found = TRUE;
	return(FINISHED);
}

/****************************** LEVEL_IS_TOO_HIGH_IN_THISBUF2 *******************************/

int level_is_too_high_in_thisbuf2(float *buf, double *gain, double tail_gainstep,
			double maxgain,int chans,int n,int *sampcnt,int abs_onset_len,int *too_high,dataptr dz)
{
	int m;
	double thislevel;
	while(!(*too_high) && (n < dz->ssampsread)) {
		for(m=0;m<chans;m++) {
			if((thislevel = (double) fabs(buf[n+m]) * (*gain)) > F_MAXSAMP) {
				*too_high = TRUE;
				break;
			}
		}
		if((*gain += tail_gainstep)>=maxgain)
			*gain = maxgain;
		if((*sampcnt += chans) >= abs_onset_len)
			return(FINISHED);
		n += chans;
		
	}
	return(CONTINUE);
}

/****************************** LEVEL_IS_TOO_HIGH_IN_ONSET *******************************/

int level_is_too_high_in_onset(float *buf,int abs_samppos,double tail_gainstep,dataptr dz)
{
	int  exit_status;
	int  too_high = FALSE;
	int  chans = dz->infile->channels;
	double gain = 1.0;
	int this_bufpos;
	int sampcnt = 0;
	int go_back = round(dz->param[ENV_ATK_ONSET] * MS_TO_SECS * (double)dz->infile->srate) * chans;
	int bakpos = max(0,abs_samppos - go_back);
	int abs_onset_len = abs_samppos - bakpos;
	int onset_len = abs_onset_len/chans;
	int bakset = (bakpos/dz->buflen) * dz->buflen;
	if(sndseekEx(dz->ifd[0],bakset,0)<0) {
		sprintf(errstr,"sndseek failed in level_is_too_high_in_onset()\n");
		return(SYSTEM_ERROR);
	}
	this_bufpos = bakpos%dz->buflen;

	dz->total_samps_read = bakset;
	dz->samps_left       = dz->insams[0] - dz->total_samps_read;

	tail_gainstep /= (double)onset_len;
	while(!too_high && (dz->samps_left > 0)) {
		if((exit_status = read_samps(buf,dz))<0)
			return(exit_status);
		if(dz->ssampsread <= 0) {
			sprintf(errstr,"read anomaly: level_is_too_high_in_onset()\n");
			return(PROGRAM_ERROR);
		}
		if((exit_status = level_is_too_high_in_thisbuf2(buf,&gain,tail_gainstep,
		dz->param[ENV_ATK_GAIN],chans,this_bufpos,&sampcnt,abs_onset_len,&too_high,dz))==FINISHED)
			break;
		this_bufpos = 0;
	}
	if((exit_status = restore_buffers_and_file_to_original_condition(buf,abs_samppos,dz))<0)
		return(exit_status);
	return(too_high);
}

/****************************** RESTORE_BUFFERS_AND_FILE_TO_ORIGINAL_CONDITION *******************************/

int restore_buffers_and_file_to_original_condition(float *buf,int abs_samppos,dataptr dz)
{
	int exit_status;
	int reset = (abs_samppos/dz->buflen) * dz->buflen;
	if(sndseekEx(dz->ifd[0],reset,0)<0) {
		sprintf(errstr,"sndseek failed in restore_buffers_and_file_to_original_condition()\n");
		return(SYSTEM_ERROR);
	}
	if((exit_status = read_samps(buf,dz))<0)
		return(exit_status);
	if(dz->ssampsread <= 0) {
		sprintf(errstr,"read anomaly: restore_buffers_and_file_to_original_condition()\n");
		return(PROGRAM_ERROR);
	}
	dz->total_samps_read = reset + dz->ssampsread;	
	dz->samps_left = dz->insams[0] - dz->total_samps_read;
	return(FINISHED);
}

/****************************** LEVEL_IS_TOO_HIGH_IN_TAIL *******************************/

int level_is_too_high_in_tail(float *buf,int abs_samppos,double tail_gainstep,dataptr dz)
{
	int    exit_status;
	int    too_high = FALSE;
	int	   chans = dz->infile->channels;
	double gain = dz->param[ENV_ATK_GAIN];
	int   tail_len = round((dz->param[ENV_ATK_TAIL] * MS_TO_SECS) * (double)dz->infile->srate);
	int   buf_samppos = abs_samppos % dz->buflen;
	tail_gainstep /= (double)tail_len;
	if((exit_status = level_is_too_high_in_thisbuf
	(buf,&gain,tail_gainstep,chans,buf_samppos,&too_high,dz))==FINISHED)
		return(too_high);
	while(dz->samps_left > 0) {
		if((exit_status = read_samps(dz->sampbuf[0],dz))<0)
			return(exit_status);
		buf_samppos = 0;
		if((exit_status = level_is_too_high_in_thisbuf
		(buf,&gain,tail_gainstep,chans,buf_samppos,&too_high,dz))==FINISHED)
			break;
	}
	return(too_high);
}

/************************** CREATE_TREMOL_SINTABLE ******************/

int create_tremol_sintable(dataptr dz)
{
	int n;
	if((dz->parray[ENV_SINETAB] = (double *)malloc((ENV_TREM_TABSIZE + 1) * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for sine table.\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<ENV_TREM_TABSIZE;n++) {
		dz->parray[ENV_SINETAB][n] = sin(PI * 2.0 * ((double)n/(double)ENV_TREM_TABSIZE));
		dz->parray[ENV_SINETAB][n] += 1.0;
		dz->parray[ENV_SINETAB][n] /= 2.0;
	}
	dz->parray[ENV_SINETAB][n] = 0.5; /* wrap around point */
	return(FINISHED);
}

/************************** CONVERT_TREMOL_FRQ_TO_LOG10 ******************/

void convert_tremol_frq_to_log10(dataptr dz)
{
	double *p, *pend;
	double infiledur = (double)dz->insams[0]/(double)dz->infile->srate;
	double effectively_zero =  1.0/infiledur;
	p = dz->brk[ENV_TREM_FRQ];
	pend = p + (dz->brksize[ENV_TREM_FRQ] * 2);
	p++;
	while(p < pend) {
		if(*p <= effectively_zero)
			*p = effectively_zero;
		*p = log10(*p);
		p += 2;
	}
}
	    
