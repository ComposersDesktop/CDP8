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
/* RWD 14 Dec 2020 fix typos in usage msgs */


/* floatsam version */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <structures.h>
#include <cdpmain.h>
#include <tkglobals.h>
#include <pnames.h>
#include <extend.h>
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
#include <srates.h>
#include <graicon.h>
#include <limits.h>
#include <float.h>

#if defined unix || defined __GNUC__
#define round(x) lround((x))
#endif

#ifndef HUGE
#define HUGE 3.40282347e+38F
#endif

static int read_sequence(char *filename,dataptr dz);
static int read_sequence2(char *filename,dataptr dz);
static int sequencer_preprocess(dataptr dz);
static int sequencer2_preprocess(dataptr dz);
static int create_btob_bufs(dataptr dz);
static int check_btob_consistency(dataptr dz);
static int create_doublets_bufs(dataptr dz);
static int doublets_preprocess(dataptr dz);
#define SAMPLE_T short

/********************************************************************************************/
/********************************** FORMERLY IN pconsistency.c ******************************/
/********************************************************************************************/

static int  check_zigzag_consistency(dataptr dz);
static int  check_loop_consistency(dataptr dz);
static int  calc_params(int *repetitions,int *outsamps,int filelen,int lsfield,
				int startsamp,int stepsamps,int loopsamps);
static int  check_scramble_consistency(dataptr dz);
static int  check_drunk_consistency(dataptr dz);
static int  cloktik_vals_OK(double lo_limit,dataptr dz);

static int read_atstream(char *filename,dataptr dz);
static int create_accbufs(dataptr dz);
static int create_seqbufs(dataptr dz);
static int create_seqbufs2(dataptr dz);

/********************************************************************************************/
/********************************** FORMERLY IN specialin.c *********************************/
/********************************************************************************************/

static int  read_ziginfo(char *filename,dataptr dz);
/*static int getmaxlong(void);*/

/***************************************************************************************/
/****************************** FORMERLY IN aplinit.c **********************************/
/***************************************************************************************/

/***************************** ESTABLISH_BUFPTRS_AND_EXTRA_BUFFERS **************************/

int establish_bufptrs_and_extra_buffers(dataptr dz)
{
	/*int is_spec = FALSE;*/
	dz->extra_bufcnt = -1;	/* ENSURE EVERY CASE HAS A PAIR OF ENTRIES !! */
	dz->bptrcnt = 0;
	dz->bufcnt  = 0;
	switch(dz->process) {
	case(ZIGZAG):				dz->extra_bufcnt = 1;	dz->bufcnt = 3;		break;
	case(LOOP):					dz->extra_bufcnt = 1;	dz->bufcnt = 2;		break;
	case(SCRAMBLE):				dz->extra_bufcnt = 1;	dz->bufcnt = 2;		break;
	case(ITERATE):				dz->extra_bufcnt = 0;	dz->bufcnt = 3;		break;
	case(ITERATE_EXTEND):		dz->extra_bufcnt = 0;	dz->bufcnt = 4;		break;
	case(DRUNKWALK):			dz->extra_bufcnt = 0;	dz->bufcnt = 3;		break;
	case(ACC_STREAM):			dz->extra_bufcnt = 1;	dz->bufcnt = 1;		break;
	case(SEQUENCER2):			dz->extra_bufcnt = 0;	dz->bufcnt = 0;		break;
	case(SEQUENCER):			dz->extra_bufcnt = 0;	dz->bufcnt = 4;		break;
	case(BAKTOBAK):				dz->extra_bufcnt = 0;	dz->bufcnt = 3;		break;
	case(DOUBLETS):				dz->extra_bufcnt = 0;	dz->bufcnt = 3;		break;
	default:
		sprintf(errstr,"Unknown program type [%d] in establish_bufptrs_and_extra_buffers()\n",dz->process);
		return(PROGRAM_ERROR);
	}

	if(dz->extra_bufcnt < 0) {
		sprintf(errstr,"bufcnts have not been set: establish_bufptrs_and_extra_buffers()\n");
		return(PROGRAM_ERROR);
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
	case(ZIGZAG):	dz->array_cnt = 1; dz->iarray_cnt = 0; dz->larray_cnt = 1; dz->ptr_cnt = 0;	dz->fptr_cnt = 0; break;
	case(LOOP):		dz->array_cnt = 1; dz->iarray_cnt = 1; dz->larray_cnt = 1; dz->ptr_cnt = 0;	dz->fptr_cnt = 0; break;
	case(SCRAMBLE):	dz->array_cnt = 1; dz->iarray_cnt = 2; dz->larray_cnt = 3; dz->ptr_cnt = 0;	dz->fptr_cnt = 0; break;
	case(ITERATE_EXTEND):
	case(ITERATE):	dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0;	dz->fptr_cnt = 0; break;
	case(DRUNKWALK): dz->array_cnt = 1; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0;	dz->fptr_cnt = 0; break;
	case(ACC_STREAM): dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 1; dz->ptr_cnt = 0;	dz->fptr_cnt = 0; break;
	case(SEQUENCER2): dz->array_cnt = 2; dz->iarray_cnt = 0; dz->larray_cnt = 1; dz->ptr_cnt = 0;	dz->fptr_cnt = 0; break;
	case(SEQUENCER): dz->array_cnt = 2; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0;	dz->fptr_cnt = 0; break;
	case(BAKTOBAK): dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0;	dz->fptr_cnt = 0; break;
	case(DOUBLETS): dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0;	dz->fptr_cnt = 0; break;
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
	case(ZIGZAG):		setup_process_logic(SNDFILES_ONLY,		  	UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(LOOP):  		setup_process_logic(SNDFILES_ONLY,		  	UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(SCRAMBLE):		setup_process_logic(SNDFILES_ONLY,		  	UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(ITERATE_EXTEND):
	case(ITERATE):		setup_process_logic(SNDFILES_ONLY,		  	UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(DRUNKWALK):	setup_process_logic(SNDFILES_ONLY,		  	UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(ACC_STREAM):	setup_process_logic(SNDFILES_ONLY,		  	UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(SEQUENCER):	setup_process_logic(SNDFILES_ONLY,		  	UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(SEQUENCER2):	setup_process_logic(MANY_SNDFILES,		  	UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(BAKTOBAK):		setup_process_logic(SNDFILES_ONLY,		  	UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(DOUBLETS):		setup_process_logic(SNDFILES_ONLY,		  	UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
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
	case(ZIGZAG):
	switch(mode) {
		case(ZIGZAG_SELF):
				      	exit_status = set_internalparam_data("iiii"  ,ap);			break;
		case(ZIGZAG_USER):	/* 1st 2,dummy variables for non-used maxzig & seed */	
		   			  	exit_status = set_internalparam_data("diiiii",ap);			break;
		default:
			sprintf(errstr,"Unknown mode for zigzag: set_legal_internalparam_structure()\n");
			return(PROGRAM_ERROR);
		}
		break;
	case(LOOP):      	exit_status = set_internalparam_data("iiiii" ,ap);			break;
	case(SCRAMBLE):    	exit_status = set_internalparam_data("iiiiii",ap);			break;
	case(ITERATE_EXTEND):
	case(ITERATE):    	exit_status = set_internalparam_data("diiii", ap);			break;
	case(DRUNKWALK):    exit_status = set_internalparam_data("iiiiiii",ap);			break;
	case(ACC_STREAM):	exit_status = set_internalparam_data("",ap);				break;
	case(SEQUENCER2):
	case(SEQUENCER):	exit_status = set_internalparam_data("",ap);				break;
	case(BAKTOBAK):		exit_status = set_internalparam_data("",ap);				break;
	case(DOUBLETS):		exit_status = set_internalparam_data("i",ap);				break;
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
	/*int exit_status = FINISHED;*/
	aplptr ap = dz->application;

	switch(ap->special_data) {
	case(ZIGDATA):				return read_ziginfo(str,dz);
	case(ATTACK_STREAM):		return read_atstream(str,dz);
	case(SEQUENCER_VALUES):		return read_sequence(str,dz);
	case(SEQUENCER2_VALUES):	return read_sequence2(str,dz);
	default:
		sprintf(errstr,"Unknown special_data type: read_special_data()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);	/* NOTREACHED */
}

/***************************** READ_ZIGINFO ***************************/

int read_ziginfo(char *filename,dataptr dz)
{
	FILE   *fp;
	double  p;
	char  temp[200], *q;
	int  arraysize = BIGARRAY, sampcnt;
	int maxlong = /*getmaxlong()*/0x7fffffff;
	dz->itemcnt = 0;
	if((fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,"Can't open text file %s to read.\n",filename);
		return(DATA_ERROR);
	}
	if((dz->lparray[ZIGZAG_TIMES] = (int *)malloc(arraysize * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store zigzag times.\n");
		return(MEMORY_ERROR);
	}
	while(fgets(temp,200,fp)!=NULL) {
		q = temp;
		if(*q == ';')	//	Allow comments in file
			continue;
		while(get_float_from_within_string(&q,&p)){
			if(p < 0.0) {
				sprintf(errstr,"Invalid zigzag time, less than zero\n");
				return(DATA_ERROR);
			}
			if((sampcnt = round(p * (double)dz->infile->srate) * dz->infile->channels) < 0) /* overflow */
				dz->lparray[ZIGZAG_TIMES][dz->itemcnt] = (maxlong/dz->infile->channels) * dz->infile->channels;
			else
				dz->lparray[ZIGZAG_TIMES][dz->itemcnt] = sampcnt;
 			if(++dz->itemcnt >= arraysize) {
				arraysize += BIGARRAY;
				if((dz->lparray[ZIGZAG_TIMES] = 
				(int *)realloc((char *)dz->lparray[ZIGZAG_TIMES],arraysize*sizeof(int)))==NULL) {
					sprintf(errstr,"INSUFFICIENT MEMORY to reallocate zigzag times.\n");
					return(MEMORY_ERROR);
				}
			}
		}
	}	    
	if(!dz->itemcnt) {
		sprintf(errstr,"No data in file %s\n",filename);
		return(DATA_ERROR);
	}
	if((dz->lparray[ZIGZAG_TIMES] =
	(int *)realloc((char *)dz->lparray[ZIGZAG_TIMES],dz->itemcnt * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to reallocate zigzag times.\n");
		return(MEMORY_ERROR);
	}
	if(fclose(fp)<0) {
		fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
		fflush(stdout);
	}
	if(dz->itemcnt < 2) {
		sprintf(errstr,"Not enough zig information found in file %s\n",filename);
		return(DATA_ERROR);
	}
	return(FINISHED);
}

/***************************** READ_ATSTREAM ***************************/

int read_atstream(char *filename,dataptr dz)
{
	FILE   *fp;
	double  p;
	char  temp[200], *q;
	int  arraysize = BIGARRAY, sampcnt;
	aplptr ap = dz->application;
	dz->itemcnt = 0;
	ap->min_special *= (double)(dz->infile->srate * dz->infile->channels);
	ap->max_special *= (double)(dz->infile->srate * dz->infile->channels);
	if((fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,"Can't open text file %s to read.\n",filename);
		return(DATA_ERROR);
	}
	if((dz->lparray[0] = (int *)malloc(arraysize * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store entry times.\n");
		return(MEMORY_ERROR);
	}
	while(fgets(temp,200,fp)!=NULL) {
		q = temp;
		if(*q == ';')	//	Allow comments in file
			continue;
		while(get_float_from_within_string(&q,&p)){
			if(p < 0.0) {
				sprintf(errstr,"Invalid entry time, less than zero\n");
				return(DATA_ERROR);
			}
			sampcnt = round(p * (double)dz->infile->srate) * dz->infile->channels;
			if (sampcnt  < ap->min_special || sampcnt > ap->max_special) {
				sprintf(errstr,"Entry time value (%lf) out of range.\n",p);
				return(MEMORY_ERROR);
			}
			dz->lparray[0][dz->itemcnt] = sampcnt;
 			if(++dz->itemcnt >= arraysize) {
				arraysize += BIGARRAY;
				if((dz->lparray[0] = 
				(int *)realloc((char *)dz->lparray[0],arraysize*sizeof(int)))==NULL) {
					sprintf(errstr,"INSUFFICIENT MEMORY to reallocate entry times.\n");
					return(MEMORY_ERROR);
				}
			}
		}
	}	    
	if(!dz->itemcnt) {
		sprintf(errstr,"No data in file %s\n",filename);
		return(DATA_ERROR);
	}
	if((dz->lparray[0] =
	(int *)realloc((char *)dz->lparray[0],dz->itemcnt * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to reallocate entry times.\n");
		return(MEMORY_ERROR);
	}
	if(fclose(fp)<0) {
		fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
		fflush(stdout);
	}
	return(FINISHED);
}

/***** GONE TO TK ************************ GETMAXLONG ***************************/
#ifdef NOTDEF
int getmaxlong(void)
{
	int k, maxlong = sizeof(int) * CHARBITSIZE;	/* bitsize, say 32 			 */
	maxlong--;										/* say 31 					 */
	k = 1;
	k <<= maxlong-1;								/* 2^30 					 */
	maxlong = k;									
	maxlong += (k-1);								/* 2^30 + 2^30 -1 = 2^31 - 1 */
	return(maxlong);
}
#endif
/********************************************************************************************/
/********************************** FORMERLY IN preprocess.c ********************************/
/********************************************************************************************/


/****************************** PARAM_PREPROCESS *********************************/

int param_preprocess(dataptr dz)	
{
	switch(dz->process) {
	case(ZIGZAG):		return zigzag_preprocess(dz);
	case(LOOP):			return loop_preprocess(dz);
	case(SCRAMBLE):		return scramble_preprocess(dz);
	case(ITERATE_EXTEND):
	case(ITERATE):		return iterate_preprocess(dz);
	case(DRUNKWALK):   	return drunk_preprocess(dz);
	case(ACC_STREAM):   return FINISHED;
	case(SEQUENCER):    return sequencer_preprocess(dz);
	case(SEQUENCER2):   return sequencer2_preprocess(dz);
	case(BAKTOBAK):     return FINISHED;
	case(DOUBLETS):     return doublets_preprocess(dz);
	default:
		sprintf(errstr,"PROGRAMMING PROBLEM: Unknown process in param_preprocess()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);	/* NOTREACHED */
}


/********************************************************************************************/
/********************************** FORMERLY IN procgrou.c **********************************/
/********************************************************************************************/

/**************************** GROUCHO_PROCESS_FILE ****************************/

int groucho_process_file(dataptr dz)   /* FUNCTIONS FOUND IN PROCESS.C */
{	
	int exit_status = FINISHED;

	switch(dz->process) {
	case(ZIGZAG):
	case(LOOP):
	case(SCRAMBLE):
		if((exit_status = zigzag(dz))<0)
			return(exit_status);
		break;
	case(ITERATE_EXTEND):
	case(ITERATE):
		if((exit_status = do_iteration(dz))<0)
			return(exit_status);
		break;
	case(DRUNKWALK):
		if((exit_status = do_drunken_walk(dz))<0)
			return(exit_status);
		break;
	case(ACC_STREAM):
		if((exit_status = accent_stream(dz))<0)
			return(exit_status);
		break;
	case(SEQUENCER):
		if((exit_status = do_sequence(dz))<0)
			return(exit_status);
		break;
	case(SEQUENCER2):
		if((exit_status = do_sequence2(dz))<0)
			return(exit_status);
		break;
	case(BAKTOBAK):
		if((exit_status = do_btob(dz))<0)
			return(exit_status);
		break;
	case(DOUBLETS):
		if((exit_status = do_doubling(dz))<0)
			return(exit_status);
		break;
	default:
		sprintf(errstr,"Unknown case in process_file()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/********************************************************************************************/
/********************************** FORMERLY IN pconsistency.c ******************************/
/********************************************************************************************/

/****************************** CHECK_PARAM_VALIDITY_AND_CONSISTENCY *********************************/

int check_param_validity_and_consistency(dataptr dz)
{
	/*int exit_status = FINISHED;*/
	handle_pitch_zeros(dz);
	switch(dz->process) {
	case(ZIGZAG):		return check_zigzag_consistency(dz);
	case(LOOP):			return check_loop_consistency(dz);
	case(SCRAMBLE):		return check_scramble_consistency(dz);
	case(DRUNKWALK):	return check_drunk_consistency(dz);
	case(BAKTOBAK):		return check_btob_consistency(dz);
	}
	return(FINISHED);
}

/********************************** CHECK_ZIGZAG_CONSISTENCY **********************************/

int check_zigzag_consistency(dataptr dz)
{
	double diff;
	if(dz->mode==ZIGZAG_SELF) {
		if(dz->param[ZIGZAG_MAX] <= dz->param[ZIGZAG_MIN]) {
			sprintf(errstr,"maximum zig duration <= minimum zig duration\n");
			return(DATA_ERROR);
		}
		if(dz->param[ZIGZAG_MIN] < (((dz->param[ZIGZAG_SPLEN] * 2) + ZIG_MIN_UNSPLICED) * MS_TO_SECS)) {
			sprintf(errstr,"minimum ziglength must be > %.3lf: cannot proceed\n",
			((dz->param[ZIGZAG_SPLEN] * 2) + ZIG_MIN_UNSPLICED) * MS_TO_SECS);
			return(DATA_ERROR);
		}
		diff = dz->param[ZIGZAG_END] - dz->param[ZIGZAG_START];
		if(diff<=0.0) {
			sprintf(errstr,"Zig start and end times incompatible.\n");
			return(DATA_ERROR);
		}
		if(round(diff/dz->param[ZIGZAG_MIN])<1) {
			sprintf(errstr,"Zigzagging sector too short for zig-zag minlength specified.\n");
			return(DATA_ERROR);
		}
	}
	return(FINISHED);
}

/********************************** CHECK_LOOP_CONSISTENCY **********************************/

int check_loop_consistency(dataptr dz)
{
	int exit_status;
	int outsamps, loopsamps, lsfield, end_partloop = 0, filelen_needed;
	int filelen = dz->insams[0];
	int stepsamps = dz->infile->channels * (int) (dz->infile->srate * dz->param[LOOP_STEP] * MS_TO_SECS);
	int startsamp = dz->infile->channels * (int) (dz->infile->srate * dz->param[LOOP_START]);
	loopsamps = dz->infile->channels * round((double)dz->infile->srate * dz->param[LOOP_LEN] * MS_TO_SECS);
	lsfield   = dz->infile->channels * round((double)dz->infile->srate * dz->param[LOOP_SRCHF] * MS_TO_SECS);	/* default to 0 */
	if(dz->mode==LOOP_ALL) {
		if((exit_status = calc_params((int *)&dz->iparam[LOOP_REPETS],&outsamps,filelen,lsfield,startsamp,stepsamps,loopsamps))<0)
			return(exit_status);
	} else {
		switch(dz->mode) {
		case(LOOP_RPTS):	
			dz->iparam[ZIG_RUNSTOEND] = TRUE;
			outsamps = ((dz->iparam[LOOP_REPETS] * loopsamps)/dz->infile->channels) * dz->infile->channels;
			end_partloop = 0;
			break;
		case(LOOP_OUTLEN):	
			outsamps = dz->infile->channels * round(dz->infile->srate * dz->param[LOOP_OUTDUR]);
			outsamps -= startsamp + lsfield;
			if((dz->iparam[LOOP_REPETS] = (int)(outsamps / loopsamps))<=0) {
				sprintf(errstr,"Infile too short to do this looping operation.\n");
				return(GOAL_FAILED);
			}
			end_partloop = outsamps - (dz->iparam[LOOP_REPETS] * loopsamps);
			break;
		}
		filelen_needed = startsamp + lsfield + (dz->iparam[LOOP_REPETS] * stepsamps) + end_partloop;
		if(filelen > filelen_needed)
			dz->iparam[ZIG_RUNSTOEND] = FALSE;
		else {
			if((exit_status = calc_params((int *)&dz->iparam[LOOP_REPETS],&outsamps,filelen,lsfield,startsamp,stepsamps,loopsamps))<0)
				return(exit_status);
			switch(dz->mode) {
			case(LOOP_RPTS):
				fprintf(stdout,"WARNING: filelength too short for job : Curtailing to %d repetitions\n",
				dz->iparam[LOOP_REPETS]); 
				break;
			case(LOOP_OUTLEN):
				sprintf(errstr,"WARNING: filelength too short for job: Curtailing to ");
				if(lsfield>0)
					strcat(errstr,"approx ");
				fprintf(stdout,"%s%lf secs\n",errstr,
					(double)(outsamps + startsamp)/(double)(dz->infile->srate * dz->infile->channels)); 
				break;
			}
			fflush(stdout);
			dz->iparam[ZIG_RUNSTOEND] = TRUE;
		}		
	}
 	return(FINISHED);
}

/********************************** CALC_PARAMS **********************************/

int calc_params(int *repetitions,int *outsamps,int filelen,int lsfield,int startsamp,int stepsamps,int loopsamps)
{										 	
	int  part_repet = 0;
	int end_partloop;
	int loopable_len = filelen - lsfield - startsamp;
	if((loopable_len -= loopsamps)<0) { 	/* subtract final complete loop */
		if(lsfield > 0)
			sprintf(errstr,"Infile too short to attempt this looping operation.\n");
		else
			sprintf(errstr,"Infile too short to do this looping operation.\n");
		return(GOAL_FAILED);
	}
	*repetitions = loopable_len/stepsamps;	/* TRUNCATE: no of complete steps in remaining filelen */
	if((end_partloop = loopable_len - ((*repetitions) * stepsamps))>0)
		part_repet = 1;						/* Final curtailed repetition which hits end of data */
	(*repetitions)++;						/* add in final loop, subtracted at start */
	if((*repetitions) + part_repet<=1) {
		sprintf(errstr,"Infile too short to do this looping operation.\n");
		return(GOAL_FAILED);
	}
	*outsamps = ((*repetitions) * loopsamps) + end_partloop;
	return(FINISHED);
}

/********************************** CHECK_SCRAMBLE_CONSISTENCY **********************************/

int check_scramble_consistency(dataptr dz)
{
	int chunkcnt;
	switch(dz->mode) {
	case(SCRAMBLE_RAND):
		if(dz->param[SCRAMBLE_MAX] < dz->param[SCRAMBLE_MIN]) {
			sprintf(errstr,"maxseglen is less than minseglen\n");
			return(USER_ERROR);
		}
		if(dz->param[SCRAMBLE_SPLEN] * MS_TO_SECS * 2.0 > dz->param[SCRAMBLE_MIN]) {
			sprintf(errstr,"Splicelen is too long for minseglen\n");
			return(USER_ERROR);
		}
		if(dz->param[SCRAMBLE_DUR] < dz->param[SCRAMBLE_MAX]) {
			sprintf(errstr,"Output duration is less than maxseglen\n");
			return(USER_ERROR);
		}
		break;
	case(SCRAMBLE_SHRED):
		chunkcnt =  /* truncate */
		dz->insams[0]/round(dz->param[SCRAMBLE_LEN] * dz->infile->srate * dz->infile->channels);
		if(dz->param[SCRAMBLE_SCAT] > (double)chunkcnt) {
			sprintf(errstr,"scatter cannot be greater than infilesize/chunklen (rounded DOWN).\n");
			return(USER_ERROR);
		}
		break;
	default:
		sprintf(errstr,"Unknown case: check_scramble_consistency()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/***************************** CHECK_DRUNK_CONSISTENCY *************************/

int check_drunk_consistency(dataptr dz)
{
	int exit_status;
	dz->iparam[DRNK_SPLICELEN] = (int)round(dz->param[DRNK_SPLICELEN] * MS_TO_SECS * (double)dz->infile->srate);

	if((exit_status = convert_time_and_vals_to_samplecnts(DRNK_CLOKTIK,dz))<0)
		return(exit_status);
	if(!cloktik_vals_OK((double)(dz->iparam[DRNK_SPLICELEN] * 2),dz)) {
		sprintf(errstr,"(minimum) clock value <= splicelen * 2 [%.1lf MS]: cannot proceed.\n",
		dz->param[DRNK_SPLICELEN]*2.0);
		return(DATA_ERROR);
	}
	return(FINISHED);
}

/************************* CLOKTIK_VALS_OK *********************/

int cloktik_vals_OK(double lo_limit,dataptr dz)
{
	double *p, *pend;
	if(dz->brksize[DRNK_CLOKTIK]) {
		p    = dz->brk[DRNK_CLOKTIK] + 1;
		pend = dz->brk[DRNK_CLOKTIK] + (dz->brksize[DRNK_CLOKTIK] * 2);
		while(p < pend) {
			if(*p <= lo_limit)
				return(FALSE);
			p += 2;
		}
	} else {
		if((double)dz->iparam[DRNK_CLOKTIK] <= lo_limit)
			return(FALSE);
	}		
	return(TRUE);
}

/********************************************************************************************/
/********************************** FORMERLY IN buffers.c ***********************************/
/********************************************************************************************/

/**************************** ALLOCATE_LARGE_BUFFERS ******************************/

int allocate_large_buffers(dataptr dz)
{
	switch(dz->process) {
	case(ZIGZAG):		case(LOOP):			case(SCRAMBLE):			
		return create_sndbufs(dz);

	case(ITERATE):		case(DRUNKWALK):	case(ITERATE_EXTEND):		
		return FINISHED;	/* buffers allocated after param-preprocessing */
	case(ACC_STREAM):		
		return create_accbufs(dz);
	case(SEQUENCER):		
		return create_seqbufs(dz);
	case(SEQUENCER2):		
		return create_seqbufs2(dz);
	case(BAKTOBAK):		
		return create_btob_bufs(dz);
	case(DOUBLETS):		
		return create_doublets_bufs(dz);
	default:
		sprintf(errstr,"Unknown program no. in allocate_large_buffers()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);	/* NOTREACHED */
}

/*************************** CREATE_ITERBUFS **************************
 *
 * (1)	Create extra spaces for interpolation guard points at end of infile.
 *
 * (2) 	Allow for any cumulative addition errors in interpolation.
 *
 * (3)	Output buffer must be at least as big as the overflow buffer.
 *	Output buffer must be big enough for the whole of any possible
 *	data overflow (in overflow_size buff) to be copied back into it.
 *	This is because the overflow buffer is ZEROED after such a copy
 *	and if a 2nd copy of the overflow back into the main buffer
 *	were necessary , we would be copying zeroes rather than true data.
 *
 *
 *		true buffer	    			overflow
 *  |-----------------------------|------------------------------------------|
 *			    worst			  ^					    					 ^
 *		    possible case		  ^					    					 ^
 *		                 	      ^----->-delayed by maxdelay_size to ->-----^
 *			         			  ^<-restored by -buffer_size into truebuf-<-^
 *  |<-------- BUFFER_SIZE------->
 *
 */

int create_iterbufs(double maxpscat,dataptr dz)
{
	size_t bigbufsize;
    int seccnt;
	double k;
	int extra_space, infile_space, orig_infile_space = 0, big_buffer_size;
	int overflowsize /*, seccnt*/;

	int framesize = F_SECSIZE * sizeof(float) * dz->infile->channels;
/* MULTICHAN 2009 --> */
	int chans = dz->infile->channels;
/* <-- MULTICHAN 2009 */

	size_t bigchunk, min_bufsize;
	if(dz->process == ITERATE) {
		infile_space = dz->insams[0];
		if(dz->vflag[IS_ITER_PSCAT]) {
			infile_space += dz->infile->channels;			/* 1 */
			k = pow(2.0,maxpscat * OCTAVES_PER_SEMITONE);
			overflowsize = (round((double)(dz->insams[0]/chans) * k) * chans) + 1;
			overflowsize += ITER_SAFETY; 						/* 2 */	
		} else
			overflowsize = dz->insams[0];
		if((seccnt = infile_space/F_SECSIZE) * F_SECSIZE < infile_space)
			seccnt++;
		infile_space = F_SECSIZE * seccnt;
	} else {	//	dz->process = ITERATE_EXTEND;
		dz->rampbrksize = dz->iparam[CHUNKEND] - dz->iparam[CHUNKSTART];
		orig_infile_space = dz->rampbrksize;
		infile_space = orig_infile_space;
		if(dz->param[ITER_PSCAT] > 0.0) {
			infile_space += dz->infile->channels;			/* 1 */
			if((seccnt = infile_space/F_SECSIZE) * F_SECSIZE < infile_space)
				seccnt++;
			infile_space = F_SECSIZE * seccnt;
			k = pow(2.0,maxpscat * OCTAVES_PER_SEMITONE);
			overflowsize = (round((double)(orig_infile_space/chans) * k) * chans) + 1;
			overflowsize += ITER_SAFETY; 						/* 2 */	
		} else
			overflowsize = orig_infile_space;
	}

	extra_space = infile_space + overflowsize;
	if(dz->process == ITERATE_EXTEND)
		extra_space += infile_space;
	min_bufsize = (extra_space * sizeof(float)) + framesize;
	if(dz->process == ITERATE) {
		bigchunk = (size_t)Malloc(-1);
		if(bigchunk < min_bufsize)
			bigbufsize = framesize;
		else {
			bigbufsize = bigchunk - extra_space*sizeof(float);
			bigbufsize = (bigbufsize/framesize) * framesize;
		}
		dz->buflen     = (int)(bigbufsize/sizeof(float));
		big_buffer_size = dz->buflen + extra_space;
	} else {
		dz->buflen = max(ITERATE_EXTEND_BUFSIZE,infile_space);
		big_buffer_size = (dz->buflen * 2) + overflowsize + infile_space;
	}
	if((dz->bigbuf = (float *)Malloc(big_buffer_size * sizeof(float)))==NULL) {
		sprintf(errstr, "INSUFFICIENT MEMORY to create sound buffers.\n");
		return(MEMORY_ERROR);
	}
	dz->sbufptr[0] = dz->sampbuf[0] = dz->bigbuf;
	if(dz->process == ITERATE) {
		dz->sbufptr[1] = dz->sampbuf[1] = dz->sampbuf[0] + infile_space;
		dz->sbufptr[2] = dz->sampbuf[2] = dz->sampbuf[1] + dz->buflen;
		dz->sbufptr[3] = dz->sampbuf[3] = dz->sampbuf[2] + overflowsize;
		memset((char *)dz->sampbuf[0],0,(size_t)(infile_space * sizeof(float)));
		memset((char *)dz->sampbuf[1],0,(size_t)(dz->buflen * sizeof(float)));
		memset((char *)dz->sampbuf[2],0,(size_t)(overflowsize * sizeof(float)));
	} else {
		dz->sbufptr[1] = dz->sampbuf[1] = dz->sampbuf[0] + dz->buflen;
		dz->sbufptr[2] = dz->sampbuf[2] = dz->sampbuf[1] + dz->buflen;
		dz->sbufptr[3] = dz->sampbuf[3] = dz->sampbuf[2] + overflowsize;
		memset((char *)dz->sampbuf[0],0,(size_t)(dz->buflen * sizeof(float)));
		memset((char *)dz->sampbuf[1],0,(size_t)(dz->buflen * sizeof(float)));
		memset((char *)dz->sampbuf[2],0,(size_t)(overflowsize * sizeof(float)));
		memset((char *)dz->sampbuf[3],0,(size_t)infile_space * sizeof(float));
	}
	return(FINISHED);
}

/*************************** CREATE_SEQBUFS **************************
 *
 * (0)	 Buffer for input
 * (1-2) Buffers for output, both >= largest transpos size of input
 * (3) 	 Buffer for transpos, >= largest transpos size of input.
 */

int create_seqbufs(dataptr dz)
{
	double *d, mintranspos;
	int transposbufsize, seccnt, infile_space = dz->insams[0] + dz->infile->channels, total_bufsize;
	unsigned int limit;
	int bigbufsize;
/* MULTICHAN 2009 --> */
	int chans = dz->infile->channels;
/* <-- MULTICHAN 2009 */
	d = dz->parray[0] + 1;
	mintranspos = HUGE;
	while(d < dz->parray[1]) {
		if(*d < mintranspos)
			mintranspos = *d;
		d += 3;
	}
	limit = (unsigned int)floor((INT_MAX - 64) * mintranspos);
	if((unsigned int)(dz->insams[0] * sizeof(float)) >= limit) {
		sprintf(errstr, "INSUFFICIENT MEMORY to create sound buffers (A).\n");
		return(MEMORY_ERROR);
	}
/* MULTICHAN 2009 --> */
//	transposbufsize = (int)round(dz->insams[0]/mintranspos) + 64; /* 64 = safety margin */
	transposbufsize = ((int)round((dz->insams[0]/chans)/mintranspos) * chans) + 64; /* 64 = safety margin */
	if((seccnt = infile_space/F_SECSIZE) * F_SECSIZE < infile_space)
		seccnt++;
	if((infile_space = F_SECSIZE * seccnt) < 0) {
		sprintf(errstr, "INSUFFICIENT MEMORY to create sound buffers (B).\n");
		return(MEMORY_ERROR);
	}
	if((seccnt = transposbufsize/F_SECSIZE) * F_SECSIZE < transposbufsize)
		seccnt++;
	if((transposbufsize = F_SECSIZE * seccnt) < 0) {
		sprintf(errstr, "INSUFFICIENT MEMORY to create sound buffers (C).\n");
		return(MEMORY_ERROR);
	}
	bigbufsize = transposbufsize * sizeof(float);	
	if((unsigned int)bigbufsize >= (INT_MAX - (infile_space * sizeof(float)))/3) {
		sprintf(errstr, "INSUFFICIENT MEMORY to create sound buffers (D).\n");
		return(MEMORY_ERROR);
	}
	total_bufsize = (infile_space * sizeof(float)) + (3 * bigbufsize);

	if((dz->bigbuf = (float *)Malloc(total_bufsize))==NULL) {
		sprintf(errstr, "INSUFFICIENT MEMORY to create sound buffers (E).\n");
		return(MEMORY_ERROR);
	}
	dz->buflen     = bigbufsize/sizeof(float);
	dz->sbufptr[0] = dz->sampbuf[0] = dz->bigbuf;
	dz->sbufptr[1] = dz->sampbuf[1] = dz->sampbuf[0] + infile_space;
	dz->sbufptr[2] = dz->sampbuf[2] = dz->sampbuf[1] + dz->buflen;
	dz->sbufptr[3] = dz->sampbuf[3] = dz->sampbuf[2] + dz->buflen;
	memset((char *)dz->sampbuf[0],0,(size_t)infile_space * sizeof(float));
	memset((char *)dz->sampbuf[1],0,(size_t)dz->buflen * sizeof(float));
	memset((char *)dz->sampbuf[2],0,(size_t)dz->buflen * sizeof(float));
	memset((char *)dz->sampbuf[3],0,(size_t)dz->buflen * sizeof(float));
	return(FINISHED);
}

/*************************** CREATE_SEQBUFS2 **************************
 *
 * (0)	 Buffer for input
 * (1-2) Buffers for output, both >= largest transpos size of input
 * (3) 	 Buffer for transpos, >= largest transpos size of input.
 */
 
int create_seqbufs2(dataptr dz)
{
	double *d, mintranspos;
	int transposbufsize, seccnt;
	int total_memory_space = 0;
	int maxlen, k;
	int n;
/* MULTICHAN 2009 --> */
	int chans = dz->infile->channels;
/* <-- MULTICHAN 2009 */

	if((dz->sampbuf = (float **)malloc(sizeof(float *) * (dz->infilecnt + 4)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffers.\n");
		return(MEMORY_ERROR);
	}
	if((dz->sbufptr = (float **)malloc(sizeof(float *) * (dz->infilecnt + 4)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffer pointers.\n");
		return(MEMORY_ERROR);
	}
	for(n = 0;n <(dz->infilecnt + 3); n++)
		dz->sampbuf[n] = dz->sbufptr[n] = (float *)0;
	dz->sampbuf[n] = (float *)0;

	if((dz->lparray[0] = (int *)malloc((dz->infilecnt+1) * sizeof(int)))==NULL) {
		sprintf(errstr, "INSUFFICIENT MEMORY to start creation of sound buffer.\n");
		return(MEMORY_ERROR);
	}
	d = dz->parray[0] + 2;
	mintranspos = DBL_MAX;
	while(d < dz->parray[1]) {
		if(*d < mintranspos)
			mintranspos = *d;
		d += 5;
	}
	maxlen = 0;
	for(n=0;n<dz->infilecnt;n++) {
		if(dz->insams[n] > maxlen)
			maxlen = dz->insams[n];
		if((seccnt = dz->insams[n]/F_SECSIZE) * F_SECSIZE < dz->insams[n])
			seccnt++;
		if((k = F_SECSIZE * seccnt) < 0) {
			sprintf(errstr, "INSUFFICIENT MEMORY to create sound buffer for infile %d.\n",n+1);
			return(MEMORY_ERROR);
		}
		total_memory_space += k;
		if(n > 0)  {
			if(total_memory_space < 0) {
				sprintf(errstr, "INSUFFICIENT MEMORY to create sound buffers for infiles 1 - %d.\n",n+1);
				return(MEMORY_ERROR);
			}
		}
		dz->lparray[0][n] = k;
	}

/* MULTICHAN 2009 --> */
//	transposbufsize = (int)ceil((double)maxlen/(double)mintranspos) + 64; /* 64 = safety margin */
	transposbufsize = ((int)ceil((double)(maxlen/chans)/(double)mintranspos) * chans) + 64; /* 64 = safety margin */
	if((seccnt = transposbufsize/F_SECSIZE) * F_SECSIZE < transposbufsize)
		seccnt++;
	if((transposbufsize = F_SECSIZE * seccnt) < 0) {
		sprintf(errstr, "INSUFFICIENT MEMORY to create transposition buffer.\n");
		return(MEMORY_ERROR);
	}
	dz->lparray[0][n] = transposbufsize;
	total_memory_space += transposbufsize;	/* outputbuf */
	if(total_memory_space < 0) {
		sprintf(errstr, "INSUFFICIENT MEMORY to create output buffer.\n");
		return(MEMORY_ERROR);
	}
	total_memory_space += transposbufsize;	/* overflowbuf */
	if(total_memory_space < 0) {
		sprintf(errstr, "INSUFFICIENT MEMORY to create overflow buffer.\n");
		return(MEMORY_ERROR);
	}
	total_memory_space += transposbufsize;	/* transposbuf */
	if(total_memory_space < 0) {
		sprintf(errstr, "INSUFFICIENT MEMORY to create transposition buffer.\n");
		return(MEMORY_ERROR);
	}
	if((dz->bigbuf = (float *)Malloc(total_memory_space * sizeof(float)))==NULL) {
		sprintf(errstr, "INSUFFICIENT MEMORY to create sound buffers (E).\n");
		return(MEMORY_ERROR);
	}
	dz->sbufptr[0] = dz->sampbuf[0] = dz->bigbuf;
	for(n=1;n<=dz->infilecnt;n++)
		dz->sbufptr[n] = dz->sampbuf[n] = dz->sampbuf[n-1] + (dz->lparray[0][n-1]);
	dz->sbufptr[n] = dz->sampbuf[n] = dz->sampbuf[n-1] + (transposbufsize);
	n++;
	dz->sbufptr[n] = dz->sampbuf[n] = dz->sampbuf[n-1] + (transposbufsize);
	memset((char *)dz->bigbuf,0,(size_t)total_memory_space * sizeof(float));
	return(FINISHED);
}

/*************************** CREATE_BUFFER **************************/

int create_drunk_buffers(dataptr dz)
{
	size_t bigbufsize;
    int seccnt;
	int exit_status;
	int maxoverflow/*, seccnt*/;
	size_t bigchunk, min_bufsize;
	double maxclok, maxrand;
	int chans = dz->infile->channels;
	int standardbuf_cnt, n;
//TW old buffer protocol for now + allow channel rounding
	int framesize = F_SECSIZE * sizeof(float) * dz->infile->channels;

	if((exit_status = get_maxvalue(DRNK_CLOKTIK,&maxclok,dz))<0)
		return(exit_status);
	if((exit_status = get_maxvalue(DRNK_CLOKRND,&maxrand,dz))<0)
		return(exit_status);
	maxoverflow  = round((double)maxclok * (1.0 + maxrand)) * chans;
	maxoverflow += 2 * (dz->iparam[DRNK_SPLICELEN] * chans);

//TW retaining old buffer-protocol for now
	maxoverflow += F_SECSIZE;	/* safety */
	if(((seccnt = maxoverflow/F_SECSIZE)*F_SECSIZE)!=maxoverflow) {
		seccnt++;
		maxoverflow = seccnt * F_SECSIZE;
	}
	standardbuf_cnt = dz->bufcnt - 1;
//TW CHANGED
	min_bufsize = standardbuf_cnt * framesize;
	bigchunk = (size_t) Malloc(-1);
	if(bigchunk < min_bufsize)
		bigbufsize = framesize;
	else {
		bigchunk /= standardbuf_cnt;
//TW retaining old buffer-protocol for now + channel rounding, for RWD
		bigbufsize = (bigchunk/framesize) * framesize;
	}
	dz->buflen = (int) (bigbufsize/sizeof(float));

	if((dz->bigbuf = (float *)malloc((size_t)(((dz->buflen * standardbuf_cnt) + maxoverflow) * sizeof(float)))) == NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<standardbuf_cnt;n++)
		dz->sbufptr[n] = dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
	dz->sampbuf[standardbuf_cnt] = dz->bigbuf + (dz->buflen * standardbuf_cnt);
	dz->sampbuf[dz->bufcnt]      = dz->sampbuf[standardbuf_cnt] + maxoverflow;
	return(FINISHED);
}

/*************************** CREATE_DOUBLETS_BUFS **************************/

int create_doublets_bufs(dataptr dz)
{
	int shsecsize = F_SECSIZE * sizeof(float);
	int n, exit_status;
	double brkmax;
	size_t segbuflen, bigbufsize;
	if(dz->sbufptr == 0 || dz->sampbuf==0) {
		sprintf(errstr,"buffer pointers not allocated: create_sndbufs()\n");
		return(PROGRAM_ERROR);
	}
	if(dz->brksize[SEG_DUR] > 0) {
		if((exit_status = get_maxvalue_in_brktable(&brkmax,SEG_DUR,dz))<0)
			return(exit_status);
	} else
		brkmax = dz->param[SEG_DUR];
	segbuflen = round((brkmax * dz->infile->srate) + 1.0) * dz->infile->channels * sizeof(float);

	bigbufsize = (size_t) Malloc(-1) - segbuflen;
	bigbufsize /= (dz->bufcnt - 1);
	if((bigbufsize  = (bigbufsize/shsecsize) * shsecsize)<=0)
		bigbufsize  = shsecsize;
	dz->buflen = bigbufsize/sizeof(float);
	if((dz->bigbuf = (float *)malloc((size_t)((bigbufsize * (dz->bufcnt - 1)) + segbuflen))) == NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
		return(PROGRAM_ERROR);
	}
	for(n=0;n<dz->bufcnt;n++)
		dz->sbufptr[n] = dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
	dz->iparam[SEGLEN] = segbuflen/sizeof(float);
	dz->sampbuf[n] = dz->bigbuf + (dz->buflen * (dz->bufcnt -1)) + dz->iparam[SEGLEN];
	return(FINISHED);
}

/********************************************************************************************/
/********************************** FORMERLY IN cmdline.c ***********************************/
/********************************************************************************************/

int get_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	if     (!strcmp(prog_identifier_from_cmdline,"zigzag"))   		dz->process = ZIGZAG;
	else if(!strcmp(prog_identifier_from_cmdline,"loop"))     		dz->process = LOOP;
	else if(!strcmp(prog_identifier_from_cmdline,"scramble")) 		dz->process = SCRAMBLE;
	else if(!strcmp(prog_identifier_from_cmdline,"iterate"))  		dz->process = ITERATE;
	else if(!strcmp(prog_identifier_from_cmdline,"freeze"))  		dz->process = ITERATE_EXTEND;
	else if(!strcmp(prog_identifier_from_cmdline,"drunk"))    		dz->process = DRUNKWALK;
	else if(!strcmp(prog_identifier_from_cmdline,"sequence"))    	dz->process = SEQUENCER;
	else if(!strcmp(prog_identifier_from_cmdline,"sequence2"))    	dz->process = SEQUENCER2;
	else if(!strcmp(prog_identifier_from_cmdline,"baktobak"))    	dz->process = BAKTOBAK;
	else if(!strcmp(prog_identifier_from_cmdline,"doublets"))    	dz->process = DOUBLETS;
	else if(!strcmp(prog_identifier_from_cmdline,"repetitions"))    dz->process = ACC_STREAM;
	else {
		sprintf(errstr,"Unknown program identification string '%s'\n",prog_identifier_from_cmdline);
		return(USAGE_ONLY);
	}
	return FINISHED;
}

/********************************************************************************************/
/********************************** FORMERLY IN usage.c *************************************/
/********************************************************************************************/

/******************************** USAGE1 ********************************/

int usage1(void)
{
	sprintf(errstr,
	"\nUSAGE: extend NAME (mode) infile outfile parameters: \n"
	"\n"
	"where NAME can be any one of\n"
	"\n"
  	"zigzag loop iterate freeze scramble drunk sequence sequence2 baktobak doublets\n"
  	"repetitions\n"
	"\n"
	"Type 'extend zigzag' for more info on zigzag option.. ETC.\n");
	return(USAGE_ONLY);
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
	if(!strcmp(str,"zigzag")) {		
	    fprintf(stdout,
		"USAGE: extend zigzag 1 infile outfile start end dur minzig\n"
		"\t[-ssplicelen] [-mmaxzig] [-rseed]\n\n"
		"OR:    extend zigzag 2 infile outfile timefile [-ssplicelen]\n\n"
	    "READ BACK AND FORTH INSIDE SOUNDFILE.\n\n"
		"MODES\n"
		"1:	random zigzags: starts at file start, ends at file end.\n"
		"2: zigzagging follows times supplied by user.\n"
		"start:     together with...\n"
		"end:       define interval in which times zigzag.\n"
		"dur:       is total duration of output sound required.\n"
		"minzig:    is min acceptable time between successive zigzag timepoints.\n"
		"splicelen: in MILLIsecs (Default 25ms).\n"
		"maxzig:    is max acceptable time between successive zigzag timepoints\n"
		"seed:      number to generate a replicable random sequence. (>0)\n"
		"           entering same number on next program run, generates same sequence.\n"
		"           (Default: (0) random sequence is different every time).\n"
		"timefile:  text file containing sequence of times to zigzag between.\n"
		"           Each step-between-times must be > (3 * splicelen).\n"
		"           zigsteps moving in the same (time-)direction will be concatenated.\n");
	} else if(!strcmp(str,"loop")) {		
	    fprintf(stdout,
		"USAGE:\n"
		"extend loop 1 infile outfile     start len    step  [-wsplen] [-sscat] [-b]\n"
		"extend loop 2 infile outfile dur start len [-lstep] [-wsplen] [-sscat] [-b]\n"
		"extend loop 3 infile outfile cnt start len [-lstep] [-wsplen] [-sscat] [-b]\n\n"
	    "LOOP INSIDE SOUNDFILE.\n\n"
		"MODES\n"
		"1: Loop advances in soundfile until soundfile is exhausted.\n"
		"2: Specify outfile duration (shortened if looping reaches end of infile).\n"
		"3: Specify number of loop repeats (reduced if looping reaches end of infile).\n\n"
		"start:   time in infile at which looping process begins.\n"
		"dur:     duration of outfile required.\n"
		"cnt:     number of loop repeats required.\n"
		"len:     length of looped segment (MILLIsecs).\n"
		"step:    advance in infile from one loopstart to next (MILLIsecs).\n"
		"         NB: Can be ZERO in modes 2 and 3, but NOT in mode 1.\n"
		"splen:   length of splice in MILLIsecs (Default 25ms).\n"
		"scat:    make step advance irregular, within timeframe given by 'scat'.\n"
		"b:       play from beginning of infile (even if looping doesn't begin there).\n");
	} else if(!strcmp(str,"scramble")) {		
	    fprintf(stdout,
		"USAGE:\n"
		"extend scramble 1 infile outfile minseglen maxseglen outdur\n"
		"                               [-wsplen] [-sseed] [-b] [-e]\n"
		"extend scramble 2 infile outfile seglen    scatter   outdur\n"
		"                             [-wsplen] [-sseed] [-b] [-e]\n\n"
		"MODES:-\n"
	    "1) CUT RANDOM CHUNKS FROM FILE, AND SPLICE END TO END.\n"
	    "1) CUT FILE INTO RANDOM CHUNKS AND REARRANGE. REPEAT DIFFERENTLY..ETC\n\n"
		"minseglen: minimum chunksize to cut.\n"
		"maxseglen: maximum chunksize to cut. (> minseglen)\n"
		"seglen:    average chunksize to cut.\n"
		"scatter:   Randomisation of chunk lengths. (>= 0)\n"
		"           (cannot be greater than infilesize/chunklen (rounded DOWN))\n"
		"outdur:    duration of outfile required (> maxseglen).\n"
		"splen:     length of splice in MILLIsecs (Default 25ms).\n"
		"seed:      the same seed-number will produce identical output on rerun\n"
		"           (Default: (0) random sequence is different every time).\n"
		"b:         force start of outfile to be beginning of infile.\n"
		"e:         force end of outfile to be end of infile.\n");
	} else if(!strcmp(str,"iterate")) {		
	    fprintf(stdout,
		"ITERATE AN INPUT SOUND IN A FLUID MANNER\n\n"
		"USAGE: extend iterate 1 infil outfil outduration\n"
		"     [-ddelay] [-rrand] [-ppshift] [-aampcut] [-ffade] [-ggain] [-sseed]\n"
		"OR:    extend iterate 2 infil outfil repetitions\n"
		"     [-ddelay] [-rrand] [-ppshift] [-aampcut] [-ffade] [-ggain] [-sseed]\n\n"
		"delay   (average) delay between iterations. Default: infile duration.\n"
		"rand    delaytime-randomisation: Range 0 - 1: Default 0\n"
		"pshift  max of random pitchshift of each iter: Range 0 - %.0lf semitones\n"
		"        e.g.  2.5 =  2.5 semitones up or down.\n"
		"ampcut  max of random amp-reduction on each iter: Range 0-1: default 0\n"
		"fade    (average) amplitude fade between iters (Range 0 - 1: default 0)\n"
		"gain    Overall Gain: Range 0 - 1:\n"
		"        special val 0 (default), gives best guess for no distortion.\n"
		"seed	 the same seed-number will produce identical output on rerun,\n"
		"        (Default: (0) random sequence is different every time).\n",ITER_MAXPSHIFT);
	} else if(!strcmp(str,"freeze")) {		
	    fprintf(stdout,
		"FREEZE A SEGMENT OF A SOUND BY ITERATION IN A FLUID MANNER\n\n"
		"USAGE: extend freeze 1 infil outfil outduration\n"
		"     delay rand pshift ampcut start_of_freeze end gain [-sseed]\n"
		"OR:    extend freeze 2 infil outfil repetitions\n"
		"     delay rand pshift ampcut starttime_of_freeze endtime gain [-sseed]\n"
		"delay   (average) delay between iterations: <= length of frozen segment.\n"
		"rand    delaytime-randomisation: Range 0 - 1\n"
		"pshift  max of random pitchshift of each iter: Range 0 - %.0lf semitones\n"
		"        e.g.  2.5 =  2.5 semitones up or down.\n"
		"ampcut  max of random amp-reduction on each iter: Range 0-1\n"
		"start_of_freeze    Time where frozen segment begins in original sound.\n"
		"end                Time where frozen segment ends in original sound.\n"
		"gain				Adjustment to gain of frozen segment (range 0.25 to 4).\n"
		"seed	 the same seed-number will produce identical output on rerun,\n"
		"        (Default: (0) random sequence is different every time).\n",ITER_MAXPSHIFT);
	} else if(!strcmp(str,"drunk")) {		
	    fprintf(stdout,
		"SPLICE SEGMENTS OF SRCFILE END-TO-END: START-TIMES (IN SRCFILE) OF SEGS,CHOSEN\n"
		"BY 'DRUNKEN-WALK' THROUGH SRCFILE:   IN MODE 2, SRCFILE PLAYS SOBERLY AT HOLDS\n\n"
		"USAGES:\n"
		"extend drunk 1 infile outfile outdur locus ambitus step clock\n"
        "       [-ssplicelen] [-cclokrand] [-ooverlap] [-rseed]\n\n"
		"extend drunk 2 infile outfile outdur locus ambitus step clock mindrnk maxdrnk\n"
		"       [-ssplicelen] [-cclokrand] [-ooverlap] [-rseed] [-llosober] [-hhisober]\n\n"
		"OUTDUR    Total (minimum) duration of output file (secs)\n"
		"LOCUS     Time in src around which drunkwalk happens.(secs) CAN MOVE THRO SRC.\n"
		"AMBITUS   (half)width of region from within which sound-segments read.(secs).\n"
		"STEP      Max length of (random) step between segment-reads (>%.3lf secs).\n"
        "          (always falls WITHIN ambitus: automatically adjusted where too large)\n"
		"CLOCK     Time between segment reads = seg duration (> SPLICELEN * 2).(secs).\n"
		"MINDRNK   Min no. cloktiks between sober plays (1 - %.0lf: Default %.0lf)\n"
		"MAXDRNK   Max no. cloktiks between sober plays (1 - %.0lf: Default %.0lf)\n"
		"SPLICELEN in MILLIsecs (default %.0lfms).\n"
		"CLOKRAND  Randomisation of clock-ticks. (Range : 0-1: default 0)\n"
		"OVERLAP   Mutual overlap of segments in output. (Range 0 to %.4lf: default 0)\n"
		"SEED      Any set value gives REPRODUCIBLE output.\n"
		"LOSOBER   Min duration of sober plays (secs) (Range: >0 - infiledur+)\n"
		"          If >= infiledur (default): sober plays all go to end of src\n"
		"HISOBER   Max duration of sober plays (secs) (Range: >0 - infiledur+)\n\n"
		"all params EXCEPT outdur, splicelen and seed, may vary through time.\n",
		DRNK_GRAIN,MAX_DRNKTIK,DEFAULT_MIN_DRNKTIK,MAX_DRNKTIK,DEFAULT_MAX_DRNKTIK,DRNK_SPLICE,DRNK_MAX_OVERLAP);
	} else if(!strcmp(str,"sequence")) {		
	    fprintf(stdout,
    	"PRODUCE SEQUENCE OF INPUT SOUND PLAYED AT TRANSPOSITIONS & TIMES SPECIFIED.\n"
		"USAGE:  extend sequence infile outfile sequence-file attenuation\n\n"
		"SEQUENCE-FILE  contains output-time, semitone-transposition, loudness triples.\n"
		"               (one for each event in the sequence)\n"
		"               where loudness is a loudness multiplier\n"
		"ATTENUATION    overall attenuation on source, should output overload.\n");
	} else if(!strcmp(str,"sequence2")) {		
	    fprintf(stdout,
    	"PRODUCE SEQUENCE OF SEVERAL SOUNDS PLAYED AT TRANSPOSITIONS & TIMES SPECIFIED.\n"
		"USAGE:  extend sequence2 inf1 inf2 [inf3....] outfile sequence-file attenuation [-ssplice]\n"
		"        (Input files must all have same number of channels).\n"
		"SEQUENCE-FILE  1st line has (possibly notional) midi-pitch of each input snd.\n"
		"               All other lines have 5 values...\n"
		"               input-sound-number, output-time, midi-pitch, loudness, duration\n"
		"               (one for each event in the sequence)\n"
		"               Loudness is a loudness multiplier (value 0-1).\n"
		"               Duration val can curtail length of event, but cannot extend it.\n"
		"               (Max event length = (transposed) duration of sound chosen).\n"
		"ATTENUATION    overall attenuation on output.\n"
		"SPLICE         length, in mS, of splice to cutoff samples at correct duration, if necessary.\n");
	} else if(!strcmp(str,"baktobak")) {		
	    fprintf(stdout,
    	"JOIN A TIME-REVERSED COPY OF THE SOUND, TO A NORMAL COPY, IN THAT ORDER.\n"
		"USAGE:  extend baktobak infile outfile join_time splice-length\n\n"
		"JOIN TIME     Time in src file where join-cut is to be made.\n"
		"SPLICE LENGTH (MS) Length of the splice, in milliseconds.\n");

	} else if(!strcmp(str,"doublets")) {		
	    fprintf(stdout,
    	"SPLICE SOURCE INTO SEGMENTS, REPEATING EACH.\n"
		"USAGE:  extend doublets infile outfile segdur repets [-s]\n\n"
		"SEGDUR       Duration of segments (can vary through time).\n"
		"REPETS       Number of times each segment repeated.\n"
		"-s           Outfile tries to stay 'synced' to infile.\n");
	} else if(!strcmp(str,"repetitions")) {		
	    fprintf(stdout,
    	"REPEAT SOURCE AT GIVEN TIMES.\n"
		"USAGE:  extend repetitions infile outfile timesfile level\n\n"
		"TIMESFILE   Textfile of times (in secs) at which source plays.\n"
		"LEVEL       Level of output (range 0-1): can vary over time.\n");
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

/******************************** INNER_LOOP (redundant)  ********************************/

int inner_loop
(int *peakscore,int *descnt,int *in_start_portion,int *least,int *pitchcnt,int windows_in_buf,dataptr dz)
{
	return(FINISHED);
}

/******************************** CREATE_ACCBUFS ********************************/

int create_accbufs(dataptr dz)
{
	size_t bigbufsize;
	int /*seccnt,*/ effective_size;
#ifdef NOTDEF
	if(((seccnt = dz->infilesize[0]/SECSIZE) * SECSIZE) < dz->infilesize[0])
		seccnt++;
	effective_size = seccnt * SECSIZE;
#else
	effective_size = dz->insams[0];
#endif
	if(dz->sbufptr == 0 || dz->sampbuf==0 || dz->extrabuf==0) {
		sprintf(errstr,"buffer pointers not allocated: create_accbufs()\n");
		return(PROGRAM_ERROR);
	}
	if((dz->extrabuf[0] = (float *)malloc((size_t)(effective_size * sizeof(float)))) == NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store input sound: Process works with short sounds only.\n");
		return(PROGRAM_ERROR);
	}
	bigbufsize = (size_t) Malloc(-1);
#ifdef NOTDEF
	if((dz->bigbufsize  = (dz->bigbufsize/SECSIZE) * SECSIZE)<=0)
		dz->bigbufsize  = SECSIZE;
#endif
	dz->buflen = (int)(bigbufsize/sizeof(float));

	/*RWD*/
	dz->buflen = (dz->buflen / dz->infile->channels) * dz->infile->channels;

	if((dz->bigbuf = (float *)malloc((size_t)(sizeof(float)*(dz->buflen + effective_size)))) == NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
		return(PROGRAM_ERROR);
	}
	dz->sbufptr[0] = dz->sampbuf[0] = dz->bigbuf;
	return(FINISHED);
}

//TW NEW FUNCTIONS
/********************************* READ_SEQUENCE ***************************/

#define IS_SEQ_TIME (0)
#define IS_SEQ_PICH (1)
#define IS_SEQ_LEVL (2)

int read_sequence(char *filename,dataptr dz)
{
	FILE *fp;
	char temp[200], *p;
	double val;
	int itemcnt = 0;
	int data_type = 0;
//	double max_transpos = GR_MAX_TRANSPOS * SEMITONES_PER_OCTAVE;
	double last_time = -1.0;
	if((fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,"Cannot open file %s to read data.\n",filename);
		return(DATA_ERROR);
	}
	while(fgets(temp,200,fp)!=NULL) {
		p = temp;
		if(*p == ';')	//	Allow comments in file
			continue;
		while(get_float_from_within_string(&p,&val)) {
			switch(data_type) {
			case(IS_SEQ_TIME):
				if(last_time < 0.0) {
					if(val < 0.0) {
						sprintf(errstr,"First Time (%lf) less than zero\n",val);
						return(DATA_ERROR);
					}
				} else {
					if(val < last_time) {
						sprintf(errstr,"Times do not increase at (%lf): OR data not grouped as time-transpos-level\n",val);
						return(DATA_ERROR);
					}
				}
				last_time = val;
				break;
			case(IS_SEQ_PICH):
				if(val < dz->application->min_special || val > dz->application->max_special) {
					sprintf(errstr,"Pitch-ratio (%lf) out of range (%lf - %lf): OR data not grouped as time-transpos-level\n",
					val,dz->application->min_special,dz->application->max_special);
					return(DATA_ERROR);
				}
				val = pow(2.0,val/SEMITONES_PER_OCTAVE);
				break;
			case(IS_SEQ_LEVL):
				if(val < 0.0) {
					sprintf(errstr,"Level (%lf) is less than zero: OR data not grouped as time-transpos-level\n",val);
					return(DATA_ERROR);
				}
				break;
			}
			data_type++;
			data_type = data_type % 3;
			if(!itemcnt)
				dz->parray[0] = (double *)malloc((itemcnt+1)*sizeof(double));
			else
				dz->parray[0] =(double *)realloc((char *)dz->parray[0],(itemcnt+1)*sizeof(double));
			dz->parray[0][itemcnt] = val;
			itemcnt++;
		}
	}
	if(itemcnt==0) {
		sprintf(errstr,"No data in file '%s'.\n",filename);
		return(DATA_ERROR);
	}
	if((itemcnt/3)*3 != itemcnt) {
		sprintf(errstr,"Time, Pitch and Level values not grouped correctly.\n");
		return(DATA_ERROR);
	}
	dz->parray[1] = dz->parray[0] + itemcnt;
	if(fclose(fp)<0) {
		fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
		fflush(stdout);
	}
	return(FINISHED);
}

/********************************* SEQUENCER_PREPROCESS ***************************/

int sequencer_preprocess(dataptr dz)
{
	int chans = dz->infile->channels;
	double last_time     = *(dz->parray[1] - 3);
	double last_transpos = *(dz->parray[1] - 2);
	dz->tempsize =  (int)round(last_time * dz->infile->srate) * chans;
	dz->tempsize += (int)round((dz->insams[0]/chans)/last_transpos) * chans;
	return(FINISHED);
}

/********************************* CREATE_BTOB_BUFS ***************************/

int create_btob_bufs(dataptr dz)
{
	int seccnt;
	size_t total_bufsize, bigbufsize;
	int fsecbytesize = F_SECSIZE * sizeof(float);
	bigbufsize = (size_t) Malloc(-1);
	seccnt = bigbufsize/(2 * fsecbytesize);
	if(seccnt * (2 *fsecbytesize) < bigbufsize)
		seccnt++;
	bigbufsize = seccnt * fsecbytesize;
	dz->buflen = (int)(bigbufsize/sizeof(float));
	total_bufsize = ((dz->buflen * 2) + dz->iparam[BTOB_SPLEN]) * sizeof(float);
	if((dz->bigbuf = (float *)Malloc(total_bufsize)) == NULL) {
		sprintf(errstr,"Insufficient memory for sound buffers.\n");
		return(MEMORY_ERROR);
	}
	dz->sampbuf[0] = dz->bigbuf;	
	dz->sampbuf[1] = dz->sampbuf[0] + dz->buflen;
	dz->sampbuf[2] = dz->sampbuf[1] + dz->buflen;
	memset((char *)dz->bigbuf,0,total_bufsize);
	return(FINISHED);
}

/********************************* CHECK_BTOB_CONSISTENCY ***************************/

int check_btob_consistency(dataptr dz)
{
	dz->param[BTOB_SPLEN] *= MS_TO_SECS;
	if(dz->param[BTOB_SPLEN]/2.0 >= dz->param[BTOB_CUT]) {
		sprintf(errstr,"Cut point (%lf secs)is too near to start of file for the splicelength (%lf secs) demanded.\n",
			dz->param[BTOB_CUT],dz->param[BTOB_SPLEN]);
		return(DATA_ERROR);
	}
	if(!EVEN(dz->iparam[BTOB_SPLEN] = (int)round(dz->param[BTOB_SPLEN] * dz->infile->srate)))
		dz->iparam[BTOB_SPLEN]--;
	dz->iparam[BTOB_SPLEN] *= dz->infile->channels;
	dz->iparam[BTOB_CUT] = (int)round(dz->param[BTOB_CUT] * dz->infile->srate) * dz->infile->channels;
	dz->tempsize = ((dz->insams[0] - dz->iparam[BTOB_CUT]) * 2) + (dz->iparam[BTOB_SPLEN]/2);
	return(FINISHED);
}

/********************************* DOUBLETS_PREPROCESS ***************************/

int doublets_preprocess(dataptr dz)
{
	int file_samplen;
	double duration;
	file_samplen = dz->insams[0]/dz->infile->channels;
	duration= (double)file_samplen/(double)dz->infile->srate;
	if(duration < SPLICEDUR * 2) {
		sprintf(errstr,"Input file is too short for this process.\n");
		return(DATA_ERROR);
	}
	return FINISHED;
}

/********************************* READ_SEQUENCE2 ***************************/

#define IS_SEQ2_INS  (0)
#define IS_SEQ2_TIME (1)
#define IS_SEQ2_PICH (2)
#define IS_SEQ2_LEVL (3)
#define IS_SEQ2_DUR  (4)

int read_sequence2(char *filename,dataptr dz)
{
	FILE *fp;
	char temp[200], *p;
	double val;
	double *inpitches;
	int ival = 0;
	int itemcnt = 0, gotpitches = 0;
	int arraysize = 0;
	int data_type = 0, linecnt;
	double last_time = -1.0;
	double splicelen = 2.0 * MS_TO_SECS;
	if((fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,"Cannot open file %s to read data.\n",filename);
		return(DATA_ERROR);
	}
	while(fgets(temp,200,fp)!=NULL) {
		p = temp;
		if(*p == ';')	//	Allow comments in file
			continue;
		while(get_float_from_within_string(&p,&val)) {
			arraysize++;
		}
	}
	if(arraysize==0) {
		sprintf(errstr,"No data in file '%s'.\n",filename);
		return(DATA_ERROR);
	}
	arraysize -= dz->infilecnt;
	if((arraysize/5)*5 != arraysize) {
		sprintf(errstr,"Sound-number, Time, Pitch, Level, Duration vals not grouped correctly: or sound-pitches listed incorrectly\n");
		return(DATA_ERROR);
	}
	if((inpitches = (double *)malloc((dz->infilecnt)*sizeof(double)))==NULL) {
		sprintf(errstr,"Cannot allocate memory to store input sound pitches.\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[0] = (double *)malloc((arraysize)*sizeof(double)))==NULL) {
		sprintf(errstr,"Cnnot allocate memory to store sequence data\n");
		return(MEMORY_ERROR);
	}
	fseek(fp,0,0);
	linecnt = 1;
	gotpitches = 0;
	while(fgets(temp,200,fp)!=NULL) {
		p = temp;
		if(*p == ';')	//	Allow comments in file
			continue;
		while(get_float_from_within_string(&p,&val)) {
			if(gotpitches < dz->infilecnt) {			/* GET PITCHES OF INPUT SOUND */
				if(val < dz->application->min_special || val > dz->application->max_special) {
					sprintf(errstr,"Pitch of sound %d (%lf) on first line of sequence data is out of range\n",
					gotpitches+1,val/*, dz->application->min_special,dz->application->max_special*/);
					return(DATA_ERROR);
				}
				inpitches[gotpitches++] = miditohz(val);
				continue;
			}
			switch(data_type) {
			case(IS_SEQ2_INS):
				ival = (int)round(val);
				if(val != (double)ival || ival < 1 || ival > dz->infilecnt) {
					sprintf(errstr,"Instrument number (1st item) at line %d does  not correspond to any input file\n",linecnt);
					return(DATA_ERROR);
				}
				ival--;
				val = (double)ival;
				break;
			case(IS_SEQ2_TIME):
				if(last_time < 0.0) {
					if(val < 0.0) {
						sprintf(errstr,"First Time (%lf) less than zero\n",val);
						return(DATA_ERROR);
					}
				} else {
					if(val < last_time) {
						sprintf(errstr,"Times do not increase at (%lf) AT LINE %d: OR data not grouped as time-transpos-level\n",val,linecnt);
						return(DATA_ERROR);
					}
				}
				last_time = val;
				break;
			case(IS_SEQ2_PICH):
				if(val < dz->application->min_special || val > dz->application->max_special) {
					sprintf(errstr,"Pitch (%lf) (2nd item) on line %d out of range (%lf - %lf): OR data not grouped as sound-time-pitch-level\n",
					val,linecnt, dz->application->min_special,dz->application->max_special);
					return(DATA_ERROR);
				}
				val = miditohz(val);
				val = val/inpitches[ival];	/* convert pitch to transposition */
				break;
			case(IS_SEQ2_LEVL):
				if(val < 0.0) {
					sprintf(errstr,"Level (%lf) is less than zero at line %d: OR data not grouped as sound-time-pitch-level\n",val,linecnt);
					return(DATA_ERROR);
				}
				break;
			case(IS_SEQ2_DUR):
				if(val <= splicelen) {
					sprintf(errstr,"Duration (%lf) at line %d is too short for splicelen (2 ms)\n",val,linecnt);
					return(DATA_ERROR);
				}
				break;
			}
			data_type++;
			data_type = data_type % 5;
			dz->parray[0][itemcnt] = val;
			itemcnt++;
		}
		linecnt++;
	}
	dz->parray[1] = dz->parray[0] + itemcnt;
	if(fclose(fp)<0) {
		fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
		fflush(stdout);
	}
	return(FINISHED);
}

/********************************* SEQUENCER2_PREPROCESS ***************************/

int sequencer2_preprocess(dataptr dz)
{
	int chans = dz->infile->channels;
	int    last_ins      = (int)round(*(dz->parray[1] - 5));
	double last_time     = *(dz->parray[1] - 4);
	double last_transpos = *(dz->parray[1] - 3);
	int   last_dur      = (int)round(*(dz->parray[1] - 1) * dz->infile->srate) * chans;
	int lastdur = (int)round((dz->insams[last_ins]/chans)/last_transpos) * chans;
	lastdur = min(lastdur,last_dur);
	dz->tempsize =  (int)round(last_time * dz->infile->srate) * chans;
	dz->tempsize += lastdur;
	return(FINISHED);
}

