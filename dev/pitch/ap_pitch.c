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
#include <structures.h>
#include <cdpmain.h>
#include <tkglobals.h>
#include <pnames.h>
#include <pitch.h>
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
#include <string.h>
#include <math.h>
#include <srates.h>

#ifndef HUGE
#define HUGE 3.40282347e+38F
#endif

/********************************************************************************************/
/********************************** FORMERLY IN pconsistency.c ******************************/
/********************************************************************************************/

static int 	check_viability_and_compatibility_of_shiftp_params(dataptr dz);
static int  sub_check_shiftp(dataptr dz);

/********************************************************************************************/
/********************************** FORMERLY IN preprocess.c ********************************/
/********************************************************************************************/

#define SCALEFUDGE  (3.0)
static int  tune_preprocess(dataptr dz);
static int  convert_params_for_tune(dataptr dz);
static int  pick_preprocess(dataptr dz);
static int  adjust_parameters_for_specpick(dataptr dz);
static int  mark_chosen_channels_in_bitflags(dataptr dz);
static int  chan_containing_partial(int *botpchan,double *lastchtop,int *chan,double partial,dataptr dz);

/********************************************************************************************/
/********************************** FORMERLY IN specialin.c *********************************/
/********************************************************************************************/

static int  generate_template_frqs(char *str,int datatype,dataptr dz);
static int  get_input_frqs(char *str,int datatype,double **fundamentals, double **harmonics,int *infrqcnt,dataptr dz);
static int  generate_template(double *fundamentals, double *harmonics, int infrqcnt, dataptr dz);
static int  getnextfrq(double *minfrq,double *fundamentals,double *harmonics,int infrqcnt);
static int  chordget(char *str,dataptr dz);

/***************************************************************************************/
/****************************** FORMERLY IN aplinit.c **********************************/
/***************************************************************************************/

/***************************** ESTABLISH_BUFPTRS_AND_EXTRA_BUFFERS **************************/

int establish_bufptrs_and_extra_buffers(dataptr dz)
{
	int exit_status;
	int is_spec = FALSE;
	dz->extra_bufcnt = -1;	/* ENSURE EVERY CASE HAS A PAIR OF ENTRIES !! */
	dz->bptrcnt = 0;
	dz->bufcnt  = 0;
	switch(dz->process) {
	case(ALT):        		dz->extra_bufcnt =  0; dz->bptrcnt = 1; 	is_spec = TRUE;		break;
	case(OCT):        		dz->extra_bufcnt =  0; dz->bptrcnt = 1; 	is_spec = TRUE;		break;
	case(SHIFTP):     		dz->extra_bufcnt =  0; dz->bptrcnt = 1; 	is_spec = TRUE;		break;
	case(TUNE):       		dz->extra_bufcnt =  0; dz->bptrcnt = 1; 	is_spec = TRUE;		break;
	case(PICK):       		dz->extra_bufcnt =  0; dz->bptrcnt = 1; 	is_spec = TRUE;		break;
	case(MULTRANS):   		dz->extra_bufcnt =  1; dz->bptrcnt = 1; 	is_spec = TRUE;		break;
	case(CHORD):      		dz->extra_bufcnt =  1; dz->bptrcnt = 1; 	is_spec = TRUE;		break;
	default:
		sprintf(errstr,"Unknown program type [%d] in establish_bufptrs_and_extra_buffers()\n",dz->process);
		return(PROGRAM_ERROR);
	}

	if(dz->extra_bufcnt < 0) {
		sprintf(errstr,"bufcnts have not been set: establish_bufptrs_and_extra_buffers()\n");
		return(PROGRAM_ERROR);
	}
	if(is_spec)
		return establish_spec_bufptrs_and_extra_buffers(dz);
	else if((dz->process==HOUSE_SPEC && dz->mode==HOUSE_CONVERT) || dz->process==INFO_DIFF) {
		if((exit_status = establish_spec_bufptrs_and_extra_buffers(dz))<0)
			return(exit_status);
	}
	return establish_groucho_bufptrs_and_extra_buffers(dz);
}

/***************************** SETUP_INTERNAL_ARRAYS_AND_ARRAY_POINTERS **************************/

int setup_internal_arrays_and_array_pointers(dataptr dz)
{
	int n;		 
	dz->ptr_cnt    = -1;		/* base constructor...process */
	dz->array_cnt  = -1;
	dz->iarray_cnt = -1;
	dz->larray_cnt = -1;
	switch(dz->process) {
	case(ALT):    	dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(OCT):    	dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(SHIFTP): 	dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(TUNE):   	dz->array_cnt = 0; dz->iarray_cnt =1;  dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(PICK):   	dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt =1;  dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(MULTRANS): dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(CHORD):  	dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	}

/*** WARNING ***
ANY APPLICATION DEALING WITH A NUMLIST INPUT: MUST establish AT LEAST 1 double array: i.e. dz->array_cnt = at least 1
**** WARNING ***/


	if(dz->array_cnt < 0 || dz->iarray_cnt < 0 || dz->larray_cnt < 0 || dz->ptr_cnt < 0 || dz->fptr_cnt < 0) {
		sprintf(errstr,"array_cnt not set in setup_internal_arrays_and_array_pointers()\n");	   
		return(PROGRAM_ERROR);
	}

	if(dz->array_cnt > 0) {  
		if((dz->parray  = (double **)malloc(dz->array_cnt * sizeof(double *)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for internal double arrays.\n");
			return(MEMORY_ERROR);
		}
		for(n=0;n<dz->array_cnt;n++)
			dz->parray[n] = NULL;
	}
	if(dz->iarray_cnt > 0) {
		if((dz->iparray = (int     **)malloc(dz->iarray_cnt * sizeof(int *)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for internal int arrays.\n");
			return(MEMORY_ERROR);
		}
		for(n=0;n<dz->iarray_cnt;n++)
			dz->iparray[n] = NULL;
	}
	if(dz->larray_cnt > 0) {	  
		if((dz->lparray = (int    **)malloc(dz->larray_cnt * sizeof(int *)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for internal long arrays.\n");
			return(MEMORY_ERROR);
		}
		for(n=0;n<dz->larray_cnt;n++)
			dz->lparray[n] = NULL;
	}
	if(dz->ptr_cnt > 0)   {  	  
		if((dz->ptr    	= (double  **)malloc(dz->ptr_cnt  * sizeof(double *)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for internal pointer arrays.\n");
			return(MEMORY_ERROR);
		}
		for(n=0;n<dz->ptr_cnt;n++)
			dz->ptr[n] = NULL;
	}
	if(dz->fptr_cnt > 0)   {  	  
		if((dz->fptr = (float  **)malloc(dz->fptr_cnt * sizeof(float *)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for internal float-pointer arrays.\n");
			return(MEMORY_ERROR);
		}
		for(n=0;n<dz->fptr_cnt;n++)
			dz->fptr[n] = NULL;
	}
	return(FINISHED);
}

/****************************** ASSIGN_PROCESS_LOGIC *********************************/

int assign_process_logic(dataptr dz)
{						 
	switch(dz->process) {
	case(ALT):			setup_process_logic(ANAL_WITH_PITCHDATA,  	EQUAL_ANALFILE,		ANALFILE_OUT,	dz);	break;
	case(OCT):			setup_process_logic(ANAL_WITH_PITCHDATA,  	EQUAL_ANALFILE,		ANALFILE_OUT,	dz);	break;
	case(SHIFTP):		setup_process_logic(ANALFILE_ONLY,		  	EQUAL_ANALFILE,		ANALFILE_OUT,	dz);	break;
	case(TUNE):			setup_process_logic(ANALFILE_ONLY,		  	EQUAL_ANALFILE,		ANALFILE_OUT,	dz);	break;
	case(PICK):			setup_process_logic(ANALFILE_ONLY,		  	EQUAL_ANALFILE,		ANALFILE_OUT,	dz);	break;
	case(MULTRANS):		setup_process_logic(ANALFILE_ONLY,		  	EQUAL_ANALFILE,		ANALFILE_OUT,	dz);	break;
	case(CHORD):		setup_process_logic(ANALFILE_ONLY,		  	EQUAL_ANALFILE,		ANALFILE_OUT,	dz);	break;
	default:
		sprintf(errstr,"Unknown process: assign_process_logic()\n");
		return(PROGRAM_ERROR);
		break;
	}
	if(dz->has_otherfile) {
		switch(dz->input_data_type) {
		case(ALL_FILES):
		case(TWO_SNDFILES):
		case(SNDFILE_AND_ENVFILE):
		case(SNDFILE_AND_BRKFILE):
		case(SNDFILE_AND_UNRANGED_BRKFILE):
		case(SNDFILE_AND_DB_BRKFILE):
			break;
		case(MANY_SNDFILES):
			if(dz->process==INFO_TIMELIST)
				break;
			/* fall thro */
		default:
			sprintf(errstr,"Most processes accepting files with different properties\n"
						   "can only take 2 sound infiles.\n");
			return(PROGRAM_ERROR);
		}
	}
	return(FINISHED);
}

/***************************** SET_LEGAL_INFILE_STRUCTURE **************************
 *
 * Allows 2nd infile to have different props to first infile.
 */

void set_legal_infile_structure(dataptr dz)
{
	switch(dz->process) {
	default:
		dz->has_otherfile = FALSE;
		break;
	}
}

/***************************************************************************************/
/****************************** FORMERLY IN internal.c *********************************/
/***************************************************************************************/

/****************************** SET_LEGAL_INTERNALPARAM_STRUCTURE *********************************/

int set_legal_internalparam_structure(int process,int mode,aplptr ap)
{
	int exit_status = FINISHED;

	switch(process) {
	case(ALT):		  	return(FINISHED);
	case(OCT):		  	return(FINISHED);
	case(SHIFTP):	  	exit_status = set_internalparam_data("idddddd",ap);				break;
	case(TUNE):		  	return(FINISHED);
	case(PICK):		  	exit_status = set_internalparam_data("ii",ap);					break;
	case(MULTRANS):	  	return(FINISHED);
	case(CHORD):	  	return(FINISHED);
	default:
		sprintf(errstr,"Unknown process in set_legal_internalparam_structure()\n");
		return(PROGRAM_ERROR);
	}
	return(exit_status);		
}

/********************************************************************************************/
/********************************** FORMERLY IN specialin.c *********************************/
/********************************************************************************************/

/********************** READ_SPECIAL_DATA ************************/

int read_special_data(char *str,dataptr dz)	   
{
	aplptr ap = dz->application;

	switch(ap->special_data) {
	case(FRQ_OR_FRQSET):
	case(PITCH_OR_PITCHSET):	return generate_template_frqs(str,(int)ap->special_data,dz);

	case(SEMIT_TRANSPOS_SET): 	return chordget(str,dz);
	default:
		sprintf(errstr,"Unknown special_data type: read_special_data()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);	/* NOTREACHED */
}

/**************************** GENERATE_TEMPLATE_FRQS ****************************/

int generate_template_frqs(char *str,int datatype,dataptr dz) /* 'datatype' has to be  PITCH_OR_PITCHSET or FRQ_OR_FRQSET */
{
 	double *fundamentals, *harmonics;
	int infrqcnt;
	int exit_status;
	if((exit_status = get_input_frqs(str,datatype,&fundamentals,&harmonics,&infrqcnt,dz))<0)
		return(exit_status);
	if((exit_status = generate_template(fundamentals,harmonics,infrqcnt,dz))<0)
		return(exit_status);
	free(fundamentals);
	free(harmonics);
	return(FINISHED);
}

/**************************** GET_INPUT_FRQS ****************************/

int get_input_frqs(char *str,int datatype,double **fundamentals, double **harmonics,int *infrqcnt,dataptr dz)
{		
	double dummy = 0.0, mididummy = 0.0;
	FILE *fp;
	int n, is_numeric = 0;
	int arraysize = BIGARRAY;
	char temp[200], *p;

	*infrqcnt = 0;

	if(!sloom) {
//TW NEW CONVENTION ON numeric filenames
		if(!value_is_numeric(str) && file_has_invalid_startchar(str)) {
    		sprintf(errstr,"Filename has invalid start-character [%s]\n",str);
			return(USER_ERROR);
		}
		if(value_is_numeric(str)) {
			if(sscanf(str,"%lf",&dummy)!=1) {
				sprintf(errstr,"Invalid frq or pitch data: get_input_frqs()\n");
				return(DATA_ERROR);
			}
			is_numeric = 1;
		}
	} else {
		if(str[0]==NUMERICVAL_MARKER) {
			str++;
			if(strlen(str)<=0 || sscanf(str,"%lf",&dummy)!=1) {
				sprintf(errstr,"Invalid frq or pitch data: get_input_frqs()\n");
				return(DATA_ERROR);
			}
			is_numeric = 1;
		}
	}
	if(is_numeric) {
		if(datatype == PITCH_OR_PITCHSET) {
			mididummy = dummy;
			dummy = miditohz(dummy);
		}
		if(dummy <= 0.0 || dummy >= dz->nyquist) {
			if(datatype == PITCH_OR_PITCHSET) {
				sprintf(errstr,"Input midi value %lf in file %s is outside frq range (>0 - %.0lf[nyquist])\n",
				mididummy,str,dz->nyquist);
			} else {
				sprintf(errstr,"Input frq value %lf in file %s is outside frq range (>0 - %.0lf[nyquist])\n",
				dummy,str,dz->nyquist);
			}
			return(USER_ERROR);
		}
		if((*fundamentals = (double *)malloc(sizeof(double)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to store fundamentals.\n");
			return(MEMORY_ERROR);
		}
		(*fundamentals)[(*infrqcnt)++] = (float)dummy;
	} else {
		if((fp = fopen(str,"r"))==NULL) {
			sprintf(errstr,"Cannot open file %s to read template.\n",str);
			return(DATA_ERROR);
		}
		if((*fundamentals = (double *)malloc(arraysize * sizeof(double)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to store fundamentals.\n");
			return(MEMORY_ERROR);
		}
		while(fgets(temp,200,fp)!=NULL) {
			p = temp;
			while(get_float_from_within_string(&p,&dummy)) {
				switch(datatype) {
				case(PITCH_OR_PITCHSET):
					mididummy = dummy;
					dummy = miditohz(dummy);
					/* fall thro */
				case(FRQ_OR_FRQSET):
					if(dummy <= 0.0 || dummy >= dz->nyquist) {
						if(datatype == PITCH_OR_PITCHSET) {
							sprintf(errstr,"Input midi value %lf in file %s is outside frq range (>0 - %.0lf[nyquist])\n",
							mididummy,str,dz->nyquist);
						} else {
							sprintf(errstr,"Input frq value %lf in file %s is outside frq range (>0 - %.0lf[nyquist])\n",
							dummy,str,dz->nyquist);
						}
						return(USER_ERROR);
					}
					break;
				}
				(*fundamentals)[*infrqcnt] = (float)dummy;
				if(++(*infrqcnt)>=arraysize) {
					arraysize += BIGARRAY;
					if((*fundamentals = (double *)realloc((char *)(*fundamentals),arraysize*sizeof(double)))==NULL) {
						sprintf(errstr,"INSUFFICIENT MEMORY to reallocate fundamentals store.\n");
						return(MEMORY_ERROR);
					}
				}
			}
		}
		if(*infrqcnt==0) {
			sprintf(errstr,"No data found in frq template file %s\n",str);
			return(DATA_ERROR);
		}
		if((*fundamentals = (double *)realloc((char *)(*fundamentals),(*infrqcnt) * sizeof(double)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to reallocate fundamentals store.\n");
			return(MEMORY_ERROR);
		}
		if(fclose(fp)<0) {
			fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",str);
			fflush(stdout);
		}
	}
	if((*harmonics = (double *)malloc((*infrqcnt) * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store harmonics.\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<(*infrqcnt);n++)
		(*harmonics)[n] = (*fundamentals)[n];
	return(FINISHED);
}
    
/**************************** GENERATE_TEMPLATE *************************/

int generate_template(double *fundamentals, double *harmonics, int infrqcnt, dataptr dz)
{
	int exit_status;
	int n;
	double lastchtop, thischtop, thisfrq;
	for(n=0;n<infrqcnt;n++) {
		if(fundamentals[n]>=dz->nyquist) {
			sprintf(errstr,"Input frq %d = %lf >= nyquist[%.0lf].\n",n+1,fundamentals[n],dz->nyquist);
			return(DATA_ERROR);
		}
	}
	lastchtop = 0.0;
	thischtop = dz->chwidth/2.0;
	if((exit_status = getnextfrq(&thisfrq,fundamentals,harmonics,infrqcnt))<0)
		return(exit_status);
	if((exit_status = float_array(&(dz->frq_template),dz->clength))<0)
		return(exit_status);
	for(n=0;n<dz->clength;n++) {
		while(thisfrq < lastchtop) {
			if((exit_status = getnextfrq(&thisfrq,fundamentals,harmonics,infrqcnt))<0)
				return(exit_status);
		}
		if(thisfrq > thischtop)
			dz->frq_template[n] = (float)(-1.0);
		else
			dz->frq_template[n] = (float)thisfrq;
		lastchtop = thischtop;
		if((thischtop += dz->chwidth) > dz->nyquist)
			thischtop = dz->nyquist;
	}
	return(FINISHED);
}

/***************************** GETNEXTFRQ *****************************/

int getnextfrq(double *minfrq,double *fundamentals,double *harmonics,int infrqcnt)
{
	int minptr = 0, n;
	*minfrq = HUGE;
	for(n=0;n<infrqcnt;n++) {
		if(harmonics[n] < *minfrq) {		/* FIND CURRENT MIN FRQ IN ARRAY harmonics[] */
			*minfrq = harmonics[n];
			minptr   = n;
		}
	}
	harmonics[minptr] += fundamentals[minptr];	/* INCREMENT MIN VALUE TO NEXT APPROPRIATE HARMONIC */
	return(FINISHED);
}

/*************************** CHORDGET ******************************/

int chordget(char *str,dataptr dz)
{
	FILE *fp;
	double val;
	int arraysize = BIGARRAY;
	char temp[200], *q;
	if((dz->transpos = (float *)malloc(arraysize * sizeof(float)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for transposition array.\n");
		return(MEMORY_ERROR);
	}
	if(!sloom) {
//TW NEW CONVENTION on numeric filenames
		if(!value_is_numeric(str) && file_has_invalid_startchar(str)) {
    		sprintf(errstr,"Cannot read parameter value [%s]\n",str);
			return(USER_ERROR);
		}
//TW REVISED Dec 2002
		if(value_is_numeric(str)) {
			sprintf(errstr,"This process takes chord-data in a FILE only.\n");
			return(DATA_ERROR);
		}
		if((fp = fopen(str,"r"))==NULL) {
			sprintf(errstr,"Can't open text file %s to read.\n",str);
			return(DATA_ERROR);
		}
	} else {
		if(str[0] == NUMERICVAL_MARKER) {
//TW THis Check should be redundant: trapped by Sound Loom
			sprintf(errstr,"This process takes chord-data in a FILE only.\n");
			return(DATA_ERROR);
		} else if((fp = fopen(str,"r"))==NULL) {
			sprintf(errstr,"Can't open text file %s to read.\n",str);
			return(DATA_ERROR);
		}
	}
	while(fgets(temp,200,fp)==temp) {
		q = temp;
		while(get_float_from_within_string(&q,&val)) {
			val *= OCTAVES_PER_SEMITONE;	/* CONVERT SEMITONES TO OCTAVES */
			val = pow(2.0,val);				/* CONVERT OCTAVES TO FRQ RATIO */
			if(val <= MIN_TRANSPOS || val > MAX_TRANSPOS) {
				sprintf(errstr,"transposition out of range (frq ratio %lf to %lf)\n",
				LOG2(MIN_TRANSPOS) * SEMITONES_PER_OCTAVE,LOG2(MAX_TRANSPOS) * SEMITONES_PER_OCTAVE);
				return(DATA_ERROR);
			}
			dz->transpos[dz->itemcnt] = (float)val;
			if(++dz->itemcnt >= arraysize) {
				arraysize += BIGARRAY;
				if((dz->transpos = (float *)realloc((char *)dz->transpos,arraysize * sizeof(float)))==NULL) {
					sprintf(errstr,"INSUFFICIENT MEMORY to reallocate transposition array.\n");
					return(MEMORY_ERROR);
				}
			}
		}
	}	    
	if(dz->itemcnt == 0) {
		sprintf(errstr,"No data in file %s\n",str);
		return(DATA_ERROR);
	}
	if((dz->transpos = (float *)realloc((char *)dz->transpos,dz->itemcnt * sizeof(float)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to reallocate transposition array.\n");
		return(MEMORY_ERROR);
	}
	if(fclose(fp)<0) {
		fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",str);
		fflush(stdout);
	}
	return(FINISHED);
}
/********************************************************************************************/
/********************************** FORMERLY IN preprocess.c ********************************/
/********************************************************************************************/

/****************************** PARAM_PREPROCESS *********************************/

int param_preprocess(dataptr dz)	
{
	int check;

	switch(dz->process) {
	case(OCT):			return initialise_specenv(&check,dz);
	case(SHIFTP):		return adjust_params_and_setup_internal_params_for_shiftp(dz);
	case(TUNE):			return tune_preprocess(dz);
	case(PICK):			return pick_preprocess(dz);

  	case(ALT):	case(MULTRANS): case(CHORD): 		
  		return(FINISHED);
	default:
		sprintf(errstr,"PROGRAMMING PROBLEM: Unknown process in param_preprocess()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);	/* NOTREACHED */
}

/************ ADJUST_PARAMS_AND_SETUP_INTERNAL_PARAMS_FOR_SHIFTP *************/

int adjust_params_and_setup_internal_params_for_shiftp(dataptr dz)
{
	int exit_status;
	double frq;
	frq = dz->param[SHIFTP_FFRQ]  + dz->chwidth/2.0;
	dz->iparam[SHIFTP_FDCNO] = (int)floor(frq/dz->chwidth);
	dz->param[SHIFTP_S1L1] = dz->param[SHIFTP_SHF1] - 1.0;
 	dz->param[SHIFTP_1LS1] = 1.0 - dz->param[SHIFTP_SHF1];
	if(dz->mode == P_SHFT_UP_AND_DN) {
		dz->param[SHIFTP_S2L1] = dz->param[SHIFTP_SHF2] - 1.0;
		dz->param[SHIFTP_1LS2] = 1.0 - dz->param[SHIFTP_SHF2];
	}
	else {	   
		dz->param[SHIFTP_S2L1] = 0.0;
		dz->param[SHIFTP_1LS2] = 0.0;
	}
	dz->param[SHIFTP_NS1] = dz->param[SHIFTP_SHF1];
	if(dz->mode == P_SHFT_UP_AND_DN)
		dz->param[SHIFTP_NS2] = dz->param[SHIFTP_SHF2];
	else						 
		dz->param[SHIFTP_NS2] =  0.0;
	if(dz->vflag[SHP_IS_DEPTH] && dz->brksize[SHIFTP_DEPTH]==0) {
		if((exit_status = reset_shiftps_according_to_depth_value(dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}

/************************ RESET_SHIFTPS_ACCORDING_TO_DEPTH_VALUE ****************************/

int reset_shiftps_according_to_depth_value(dataptr dz)
{
	switch(dz->mode) {
	case(6):
  		if(dz->param[SHIFTP_SHF2] > 1.0)
  			dz->param[SHIFTP_NS2] = (dz->param[SHIFTP_S2L1] * dz->param[SHIFTP_DEPTH]) + 1.0;
		else
			dz->param[SHIFTP_NS2] = 1.0 - (dz->param[SHIFTP_1LS2] * dz->param[SHIFTP_DEPTH]);
	default:
		if(dz->param[SHIFTP_SHF1] > 1.0)
			dz->param[SHIFTP_NS1]  = (dz->param[SHIFTP_S1L1] * dz->param[SHIFTP_DEPTH]) + 1.0;
		else
			dz->param[SHIFTP_NS1]  = 1.0 - (dz->param[SHIFTP_1LS1] * dz->param[SHIFTP_DEPTH]);
	}
	return(FINISHED);
}

/************************** TUNE_PREPROCESS ******************************/

int tune_preprocess(dataptr dz)
{
	int exit_status;
	if((exit_status = convert_params_for_tune(dz))<0)
		return(exit_status);
	if((dz->iparray[TUNE_LOUD] = (int *)malloc(dz->wanted * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for loudness array.\n");
		return(MEMORY_ERROR);
	}
	return setup_ring(dz);
}

/************ CONVERT_PARAMS_FOR_TUNE *************/

int convert_params_for_tune(dataptr dz)
{
	double *p;
	int n;
	if(dz->brksize[TUNE_CLAR]==0)
		dz->param[TUNE_CLAR] = 1.0 - dz->param[TUNE_CLAR];
	else {
		p = dz->brk[TUNE_CLAR] + 1;
		for(n = 0;n<dz->brksize[TUNE_CLAR];n++) {
			*p = 1.0 - *p;
			p += 2;
		}
	}
	return(FINISHED);
}

/************************** PICK_PREPROCESS ******************************/

int pick_preprocess(dataptr dz)
{
	int exit_status;
	if((exit_status = adjust_parameters_for_specpick(dz))<0)
		return(exit_status);
	if((exit_status = setup_internal_bitflags(PICK_BFLG,PICK_LONGPOW2,PICK_DIVMASK,dz))<0)
		return(exit_status);
	return mark_chosen_channels_in_bitflags(dz);
}

/************ ADJUST_PARAMETERS_FOR_SPECPICK *************/

int adjust_parameters_for_specpick(dataptr dz)
{
	/* Make perceived clarity grow linearly with entered val: PICK */

	double *p;
	int n;
	if(dz->brksize[PICK_CLAR] == 0) {
		dz->param[PICK_CLAR] = 1.0 - dz->param[PICK_CLAR];
		dz->param[PICK_CLAR] = pow(dz->param[PICK_CLAR],SCALEFUDGE);
	} else {
		p = dz->brk[PICK_CLAR] + 1;
		for(n=0; n < dz->brksize[PICK_CLAR]; n++) {
			*p = 1.0 - *p;
			*p = pow(*p,SCALEFUDGE);
			p += 2;
		}
	}
	return(FINISHED);
}

/***************************** MARK_CHOSEN_CHANNELS_IN_BITFLAGS *************/

int mark_chosen_channels_in_bitflags(dataptr dz)
{
	int exit_status;
	int n=1, chan;
	int mask;
	int botpchan = 0;
	double lastchtop = dz->chwidth/2.0;
	double partial = dz->param[PICK_FUND];
	do {
		if((exit_status = chan_containing_partial(&botpchan,&lastchtop,&chan,partial,dz))<0) {
			sprintf(errstr,"Problem with partial->channel calculation: mark_chosen_channels_in_bitflags()\n");
			return(PROGRAM_ERROR);
		}
		mask = 1;		  /* set bitflag */
		dz->lparray[PICK_BFLG][(chan>>dz->iparam[PICK_LONGPOW2])] 
			|= (mask <<= (chan & dz->iparam[PICK_DIVMASK]));
		switch(dz->mode) {
		case(PIK_HARMS):  		   n++;   partial  =  dz->param[PICK_FUND] * (double)n;	    					break;
		case(PIK_OCTS):    	 	   		  partial  =  partial * 2.0;	    									break;
		case(PIK_ODD_HARMS):  	   n+=2;  partial  =  dz->param[PICK_FUND] * (double)n;	    					break;
		case(PIK_LINEAR):	       		  partial +=  dz->param[PICK_LIN];    									break;
		case(PIK_DISPLACED_HARMS): n++;   partial  = (dz->param[PICK_FUND] 
										* (double)n) + dz->param[PICK_LIN];  
			break;
		default:
			sprintf(errstr,"Programing Problem: Unknown mode in mark_chosen_channels_in_bitflags()\n");
			return(PROGRAM_ERROR);
		}
	} while(partial < dz->nyquist);
	return(FINISHED);
}

/************************** CHAN_CONTAINING_PARTIAL *************************/

int chan_containing_partial(int *botpchan,double *lastchtop,int *chan,double partial,dataptr dz)
{
	int n;
	double chtop;
	chtop = *lastchtop;
	for(n=*botpchan; n<=dz->clength ;n++) {
		if(partial < chtop) {
			*botpchan = n;
			*lastchtop = chtop;
			*chan = n;
			return(FINISHED);
		}
		chtop += dz->chwidth;
	}
	return(GOAL_FAILED);
}

/********************************************************************************************/
/********************************** FORMERLY IN procspec.c **********************************/
/********************************************************************************************/

/**************************** SPEC_PROCESS_FILE ****************************/

int spec_process_file(dataptr dz)
{	
	dz->total_windows = 0;

	display_virtual_time(0L,dz);

	switch(dz->process) {
	case(ALT):		return outer_loop(dz);
	case(OCT):		return outer_loop(dz);
	case(SHIFTP):	return outer_loop(dz);
	case(TUNE):		return outer_loop(dz);
	case(PICK):		return outer_loop(dz);
	case(MULTRANS):	return outer_loop(dz);
	case(CHORD):	return outer_loop(dz);
	default:
		sprintf(errstr,"Unknown process in procspec()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);	/* NOTREACHED */
}

/**************************** INNER_LOOP ****************************/

int inner_loop
(int *peakscore,int *descnt,int *in_start_portion,int *least,int *pitchcnt,int windows_in_buf,dataptr dz)
{
	int exit_status;
	int local_zero_set = FALSE;
	int wc;
   	for(wc=0; wc<windows_in_buf; wc++) {
		if(dz->total_windows==0) {
			if((exit_status = skip_or_special_operation_on_window_zero(dz))<0)
				return(exit_status);
			if(exit_status==TRUE) {
				dz->flbufptr[0] += dz->wanted;
				dz->total_windows++;
				dz->time = (float)(dz->time + dz->frametime);
				continue;
			}
		}
		if((exit_status = read_values_from_all_existing_brktables((double)dz->time,dz))<0)
			return(exit_status);

		switch(dz->process) {
		case(ALT):		exit_status = specalt(pitchcnt,dz);	  				break;		
		case(OCT):		exit_status = specoct(dz);	  						break;
		case(CHORD):	exit_status = specchord(dz);  						break;
		case(MULTRANS):	exit_status = specchord2(dz); 						break;
		case(PICK):		exit_status = specpick(dz);	  						break;
		case(SHIFTP):	exit_status = specshiftp(dz); 						break;
		case(TUNE):		exit_status = spectune(dz);	  						break;
		default:
			sprintf(errstr,"unknown process in inner_loop()\n");
			return(PROGRAM_ERROR);
		}
		if(exit_status<0)
			return(exit_status);
		dz->flbufptr[0] += dz->wanted;
		dz->total_windows++;
		dz->time = (float)(dz->time + dz->frametime);
	}
	if(!dz->zeroset && local_zero_set==TRUE) {
		fprintf(stdout,"WARNING: Zero-amp spectral window(s) encountered: orig window(s) substituted.\n"); 
		fflush(stdout);
		dz->zeroset = TRUE;
	}
	return(FINISHED);
}

/***************** SKIP_OR_SPECIAL_OPERATION_ON_WINDOW_ZERO ************/

int skip_or_special_operation_on_window_zero(dataptr dz)
{
	int vc;
	switch(dz->process) {
	case(CHORD):   case(OCT):   case(MULTRANS):
		switch(dz->process) {
		case(CHORD):
		case(MULTRANS):
			for(vc = 0; vc < dz->wanted; vc += 2)
				dz->windowbuf[0][FREQ] = dz->flbufptr[0][FREQ];
			break;
		}
		return(TRUE);
	}
	return(FALSE);
}

/********************************************************************************************/
/********************************** FORMERLY IN pconsistency.c ******************************/
/********************************************************************************************/

/****************************** CHECK_PARAM_VALIDITY_AND_CONSISTENCY *********************************/

int check_param_validity_and_consistency(dataptr dz)
{
	handle_pitch_zeros(dz);
	switch(dz->process) {
	case(SHIFTP):	   return check_viability_and_compatibility_of_shiftp_params(dz);
	}
	return(FINISHED);
}

/***************** CHECK_VIABILITY_AND_COMPATIBILITY_OF_SHIFTP_PARAMS ******************/

int check_viability_and_compatibility_of_shiftp_params(dataptr dz)
{
	int exit_status;
	double thistime = 0.0;
	if(dz->vflag[SHP_IS_DEPTH]) {
		if((exit_status = check_depth_vals(SHIFTP_DEPTH,dz))<0)
			return(exit_status);
	}
	if(dz->brksize[SHIFTP_FFRQ] || dz->brksize[SHIFTP_SHF1] || dz->brksize[SHIFTP_SHF2]) {
		while(thistime < dz->duration + dz->frametime) {
			if((exit_status = read_values_from_all_existing_brktables(thistime,dz))<0)
				return(exit_status);
			if((exit_status = sub_check_shiftp(dz))<0)
				return(exit_status);
			thistime += dz->frametime;
		}
		return(FINISHED);
	}
	return sub_check_shiftp(dz);
}

/*********************** SUB_CHECK_SHIFTP ***********************/

int sub_check_shiftp(dataptr dz)
{
	double frq = dz->param[SHIFTP_FFRQ] + dz->chwidth/2.0;
	int fdcno = (int)floor(frq/dz->chwidth);
	switch(dz->mode) {
	case(P_SHFT_UP_AND_DN):
		dz->param[SHIFTP_SHF1] *= OCTAVES_PER_SEMITONE;
		dz->param[SHIFTP_SHF1] = pow(2.0,dz->param[SHIFTP_SHF1]);
		dz->param[SHIFTP_SHF2] *= OCTAVES_PER_SEMITONE;
		dz->param[SHIFTP_SHF2] = pow(2.0,dz->param[SHIFTP_SHF2]);

		if(dz->param[SHIFTP_SHF1] > 1.0
		&& round((double)(dz->clength-1)/dz->param[SHIFTP_SHF1]) < fdcno) {
			sprintf(errstr,"Shift above frq split is too great to work.\n");
			return(DATA_ERROR);
		}
		if(dz->param[SHIFTP_SHF1] <= 1.0
		&& round((double)(dz->clength-1)*dz->param[SHIFTP_SHF1]) <= fdcno) {
			sprintf(errstr,"Shift above frq split is too great to work.\n");
			return(DATA_ERROR);
		}
		if(dz->param[SHIFTP_SHF2] > dz->param[SHIFTP_FFRQ]/SPEC_MINFRQ) {
			sprintf(errstr,"Shift below frq split is too great to work.\n");
			return(DATA_ERROR);
		}
		if(dz->param[SHIFTP_SHF2] <= 1.0
		&& (1.0/dz->param[SHIFTP_SHF2]) >= dz->param[SHIFTP_FFRQ]/SPEC_MINFRQ) {
			sprintf(errstr,"Shift below frq split is too great to work.\n");
			return(DATA_ERROR);
		}
		break;
	case(P_SHFT_DN):
		dz->param[SHIFTP_SHF1] *= OCTAVES_PER_SEMITONE;
		dz->param[SHIFTP_SHF1] = pow(2.0,dz->param[SHIFTP_SHF1]);
		if(dz->param[SHIFTP_SHF1] > 1.0
		&& round((double)(dz->clength-1)/dz->param[SHIFTP_SHF1]) < fdcno) {
			sprintf(errstr,"Shift incompatible with frqsplit.\n");
			return(DATA_ERROR);
		}
		if(dz->param[SHIFTP_SHF1] <= 1.0
		&& round((double)(dz->clength-1)*dz->param[SHIFTP_SHF1]) < fdcno) {
			sprintf(errstr,"Shift incompatible with frqsplit.\n");
			return(DATA_ERROR);
		}
		break;
	case(P_SHFT_UP):
		dz->param[SHIFTP_SHF1] *= OCTAVES_PER_SEMITONE;
		dz->param[SHIFTP_SHF1] = pow(2.0,dz->param[SHIFTP_SHF1]);
		if(dz->param[SHIFTP_SHF1] > 1.0
		&& round((double)(dz->clength-1)/dz->param[SHIFTP_SHF1]) < fdcno) {
			sprintf(errstr,"Shift incompatible with frqsplit.\n");
			return(DATA_ERROR);
		}
		if(dz->param[SHIFTP_SHF1] <= 1.0
		&& round((double)(dz->clength-1)*dz->param[SHIFTP_SHF1]) <= fdcno) {
			sprintf(errstr,"Shift incompatible with frqsplit.\n");
			return(DATA_ERROR);
		}
		break;
	}
	return(FINISHED);
}

/********************************************************************************************/
/********************************** FORMERLY IN buffers.c ***********************************/
/********************************************************************************************/

/**************************** ALLOCATE_LARGE_BUFFERS ******************************/

int allocate_large_buffers(dataptr dz)
{
	switch(dz->process) {
	case(ALT):		case(OCT):		case(SHIFTP):	
	case(TUNE):		case(PICK):		case(MULTRANS):	
	case(CHORD):
		return allocate_single_buffer(dz);
	}
	sprintf(errstr,"Unknown program no. in allocate_large_buffers()\n");
	return(PROGRAM_ERROR);
}

/********************************************************************************************/
/********************************** FORMERLY IN cmdline.c ***********************************/
/********************************************************************************************/

int get_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	if     (!strcmp(prog_identifier_from_cmdline,"altharms"))	   	dz->process = ALT;
	else if(!strcmp(prog_identifier_from_cmdline,"octmove"))	   	dz->process = OCT;
	else if(!strcmp(prog_identifier_from_cmdline,"transp"))    	    dz->process = SHIFTP;
	else if(!strcmp(prog_identifier_from_cmdline,"tune"))	   		dz->process = TUNE;
	else if(!strcmp(prog_identifier_from_cmdline,"pick"))	   		dz->process = PICK;
	else if(!strcmp(prog_identifier_from_cmdline,"chord"))    		dz->process = MULTRANS;
	else if(!strcmp(prog_identifier_from_cmdline,"chordf"))	   		dz->process = CHORD;
	else {
		sprintf(errstr,"Unknown program identification string '%s'\n",prog_identifier_from_cmdline);
		return(USAGE_ONLY);
	}
//TW UPDATE
	return(FINISHED);
}

/********************************************************************************************/
/********************************** FORMERLY IN usage.c *************************************/
/********************************************************************************************/

/******************************** USAGE1 ********************************/

int usage1(void)
{
	sprintf(errstr,
	"\nPITCH OPERATIONS ON A SPECTRAL FILE\n\n"
	"USAGE: pitch NAME (mode) infile outfile parameters: \n"
	"\n"
	"where NAME can be any one of\n"
	"\n"
	"altharms    octmove    transp    tune    pick    chordf    chord\n\n"
	"Type 'pitch altharms' for more info on pitch altharms..ETC.\n");
	return(USAGE_ONLY);
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
	if(!strcmp(str,"altharms")) {
		fprintf(stdout,
		"pitch altharms mode infile pitchfile outfile [-x]\n"
		"\n"
		"DELETE ALTERNATE HARMONICS\n"
		"\n"
		"MODES :-\n"
		"1   delete odd harmonics.\n"
		"    Usually produces octave up transposition with no formant change.\n"
		"2   delete even harmonics.\n"
		"-x  alternative spectral reconstruction.\n\n"
		"PITCHFILE must be derived from infile\n");
	} else if(!strcmp(str,"octmove")) {
		fprintf(stdout,
		"pitch octmove 1-2 infile pitchfile outfile [-i] transposition\n"
		"pitch octmove 3   infile pitchfile outfile [-i] transposition bassboost\n"
		"\n"
		"OCTAVE TRANSPOSE WITHOUT FORMANT SHIFT\n"
		"\n"
		"MODES :-\n"
		"1   transpose up.\n"
		"2   transpose down.\n"
		"3   transpose down, with bass-reinforcement.\n"
		"\n"
		"PITCHFILE must be derived from infile.\n"
		"transposition an integer transposition-ratio: 2 is 8va, 3 is 12th etc.\n"
		"              as in harmonic series.\n"
		"bassboost     bass reinforcement: values >=0.0\n"
		"-i            quicksearch for formants (less accurate).\n"
		"\n"
		"bassboost may vary over time.\n");
	} else if(!strcmp(str,"transp")) {	  /* SHIFTP */
		fprintf(stdout,
		"pitch transp 1-3 infile outfile frq_split                     [-ddepth]\n"
		"pitch transp 4-5 infile outfile frq_split transpos            [-ddepth]\n"
		"pitch transp 6   infile outfile frq_split transpos1 transpos2 [-ddepth]\n"
		"\n"
		"SHIFT PITCH OF (PART OF) SPECTRUM\n"
		"\n"
		"MODES :-\n"
		"1   Octave transpose up, above freq_split.\n"
		"2   Octave transpose down, below freq_split.\n"
		"3   Octave transpose up and down.\n"
		"4   Pitch  transpose up, above freq_split.\n"
		"5   Pitch  transpose down, below freq_split.\n"
		"6   Pitch  transpose up and down.\n"
		"\n"
		"frq_split is frequency above or below which transposition takes place.\n"
		"transpos  is transposition above or below freq_split.\n"
		"transpos1 is transposition above freq_split. (semitones)\n"
		"transpos2 is transposition below freq_split. (semitones)\n"
		"depth     transposition effect on source (from 0(no effect) to 1(full effect))\n"
		"\n"
		"depth,frq_split,transpos1 & transpos2 can vary over time.\n");
	} else if(!strcmp(str,"tune")) {
		fprintf(stdout,
		"pitch tune mode infile outfile pitch_template\n"
		"               [-ffocus] [-cclarity] [-ttrace] [-bbcut] \n"
		"\n"
		"REPLACE SPECTRAL FRQS BY HARMONICS OF SPECIFIED PITCH(ES)\n"
		"\n"
		"MODES :-\n"
		"1   enter pitch_template data as frq (in Hz).\n"
		"2   enter pitch_template data as (possibly fractional) MIDI values.\n"
		"\n"
		"pitch_template   a value, or a textfile containing values (as hz or midi).\n"
		"\n"
		"-f  FOCUS determines degree of focusing of partial pitches onto template.\n"
	    "    (range 0-1: val or brkpnt file: default 1).\n"
		"-c  CLARITY determines degree to which non-template partials are suppressed.\n"
	    "    (range 0-1: val or brkpnt file: default 0).\n"
		"-t  TRACE specifies no. of (window_by_window) most prominent channels\n"
		"    to be replaced by template frqs.\n"
		"-b  Ignore frqs below BCUT, Bass cutoff frq.\n"
		"\n"
		"All parameters may vary over time.\n");
	} else if(!strcmp(str,"pick")) {
		fprintf(stdout,
		"pitch pick 1-3 infile outfile fundamental         [-cclarity]\n"
		"pitch pick 4-5 infile outfile fundamental frqstep [-cclarity]\n"
		"\n"
		"ONLY RETAIN CHANNELS WHICH MIGHT HOLD SPECIFIED PARTIALS\n"
		"\n"
		"MODES :-\n"
		"1   Harmonic Series.\n"
		"2   Octaves.\n"
		"3   Odd partials of harmonic series only.\n"
		"4   Partials are successive linear steps (each of frqstep) from 'fundamental'.\n"
		"5   Add linear displacement (frqstep) to harmonic partials over fundamental.\n"
		"\n"
		"fundamental  Fundamental frequency of harmonic series, (or of calculation).\n"
		"frqstep      Frequency step to be added to another frequency.\n"
		"clarity      extent to which data in other channels is suppressed.\n"
	    "             Range 0-1: Default 1.\n"
		"\n"
		"clarity may vary over time.\n");
	} else if(!strcmp(str,"chordf")) {
		fprintf(stdout,
		"pitch chordf infile outfile -fN|-pN [-i] transpose_file [-bbot] [-ttop] [-x]\n"
		"\n"
		"TRANSPOSED VERSIONS OF SPECTRUM SUPERIMPOSED WITHIN EXISTING SPECTRAL ENVELOPE\n"
		"\n"
		"-f   extract spectral envelope linear-freqwise, using N chans per point.\n"
		"-p   extract spectral envelope linear-pitchwise, using N bands per octave.\n"
		"     (recommeded value 1.0)\n"
		"-i   quicksearch for formants (less accurate).\n"
		"\n"
		"transpose_file is file of (possibly fractional) semitone transposition values.\n"
		"\n"
		"-b   BOT = bottom frq, below which data is filtered out.\n"
		"-t   TOP = top frq, above which data is filtered out.\n"
		"-x   Fuller spectrum.\n"
		"\n"
		"top frq and bottom frq may vary over time.\n"
		"\n");
	} else if(!strcmp(str,"chord")) {
		fprintf(stdout,
		"pitch chord infile outfile transpose_file [-bbot] [-ttop] [-x]\n"
		"\n"
		"TRANSPOSED VERSIONS OF SOUND SUPERIMPOSED ON ORIGINAL\n"
		"\n"
		"transpose_file is file of (possibly fractional) semitone transposition values.\n"
		"\n"
		"-t   TOP = top frq, above which data is filtered out.\n"
		"-b   BOT = bottom frq, below which data is filtered out.\n"
		"-x   Fuller spectrum.\n"
		"\n"
		"top frq and bottom frq may vary over time.\n"
		"\n");
	} else
		fprintf(stdout,"Unknown option '%s'\n",str);
	return(USAGE_ONLY);
}

/******************************** USAGE3 ********************************/

int usage3(char *str1,char *str2)
{
	sprintf(errstr,"Insufficient parameters on command line.\n");
	return(USAGE_ONLY);
}

