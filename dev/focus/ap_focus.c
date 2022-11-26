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
#include <focus.h>
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
#include <ctype.h>
#include <math.h>
#include <srates.h>

#ifndef cdp_round
extern int cdp_round(double a);
#endif

#if defined unix || defined __GNUC__
#define round(x) lround((x))
#endif

/********************************************************************************************/
/********************************** FORMERLY IN pconsistency.c ******************************/
/********************************************************************************************/

static int 	check_consistency_of_frq_limits_for_focus(dataptr dz);

/********************************************************************************************/
/********************************** FORMERLY IN preprocess.c ********************************/
/********************************************************************************************/

static int  scale_accu_parameters(dataptr dz);
static int  convert_params_for_exag(dataptr dz);
static int  setup_internal_arrays_and_params_for_focus(dataptr dz);
static int  generate_focu_filter_params(dataptr dz);
static int  establish_internal_arrays_for_focus(dataptr dz);
static int  fold_preprocess(dataptr dz);
static int  get_frztimes(dataptr dz);
static int  rationalise_frztimes(dataptr dz);
static int  convert_params_for_step(dataptr dz);

/********************************************************************************************/
/********************************** FORMERLY IN specialin.c *********************************/
/********************************************************************************************/

static int  read_specfreeze_data(char *filename,dataptr dz);
static int  read_specfreeze2_data(char *filename,dataptr dz);
static int  set_timetype(char **first_char_of_string,int strno,int *is_direction_info,dataptr dz);
static int  read_time(char **p,double *dtime,int strno,int linecnt, char *filename);
static int  insert_values_at_time_zero(int *n,dataptr dz);
static int  check_time_sequencing(int n,char *filename,dataptr dz);

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
	case(ACCU):       		dz->extra_bufcnt =  1; dz->bptrcnt = 1; 	is_spec = TRUE;		break;
	case(EXAG):       		dz->extra_bufcnt =  0; dz->bptrcnt = 1; 	is_spec = TRUE;		break;
	case(FOCUS):      		dz->extra_bufcnt =  0; dz->bptrcnt = 4; 	is_spec = TRUE;		break;
	case(FOLD):       		dz->extra_bufcnt =  0; dz->bptrcnt = 1; 	is_spec = TRUE;		break;
	case(FREEZE):     		dz->extra_bufcnt =  1; dz->bptrcnt = 4; 	is_spec = TRUE;		break;	
	case(FREEZE2):     		dz->extra_bufcnt =  0; dz->bptrcnt = 4; 	is_spec = TRUE;		break;	
	case(STEP):       		dz->extra_bufcnt =  1; dz->bptrcnt = 2; 	is_spec = TRUE;		break;
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
	case(ACCU):   	dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(EXAG):   	dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(FOCUS):  	dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(FOLD):   	dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(FREEZE): 	dz->array_cnt = 0; dz->iarray_cnt =1;  dz->larray_cnt =2;  dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(FREEZE2): 	dz->array_cnt = 0; dz->iarray_cnt =1;  dz->larray_cnt =2;  dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(STEP):   	dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
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
	case(ACCU):			setup_process_logic(ANALFILE_ONLY,		  	EQUAL_ANALFILE,		ANALFILE_OUT,	dz);	break;
	case(EXAG):			setup_process_logic(ANALFILE_ONLY,		  	EQUAL_ANALFILE,		ANALFILE_OUT,	dz);	break;
	case(FOCUS):		setup_process_logic(ANALFILE_ONLY,		  	EQUAL_ANALFILE,		ANALFILE_OUT,	dz);	break;
	case(FOLD):			setup_process_logic(ANALFILE_ONLY,		  	EQUAL_ANALFILE,		ANALFILE_OUT,	dz);	break;
	case(FREEZE):		setup_process_logic(ANALFILE_ONLY,		  	EQUAL_ANALFILE,		ANALFILE_OUT,	dz);	break;
	case(FREEZE2):		setup_process_logic(ANALFILE_ONLY,		  	BIG_ANALFILE,		ANALFILE_OUT,	dz);	break;
	case(STEP):			setup_process_logic(ANALFILE_ONLY,		  	EQUAL_ANALFILE,		ANALFILE_OUT,	dz);	break;
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
	mode = 0;
	switch(process) {
	case(ACCU):		  	return(FINISHED);
	case(EXAG):		  	return(FINISHED);
	case(FOCUS):	  	exit_status = set_internalparam_data("ddi",ap);					break;
	case(FOLD):		  	exit_status = set_internalparam_data("iidd",ap);				break;
	case(FREEZE):	  	return(FINISHED);
	case(FREEZE2):	  	return(FINISHED);
	case(STEP):		  	return(FINISHED);
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
//	int exit_status = FINISHED;
	aplptr ap = dz->application;

	switch(ap->special_data) {
	case(FREEZE_DATA):		  	return read_specfreeze_data(str,dz);
	case(FREEZE2_DATA):		  	return read_specfreeze2_data(str,dz);
	default:
		sprintf(errstr,"Unknown special_data type: read_special_data()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);	/* NOTREACHED */
}

/************************** READ_SPECFREEZE_DATA ********************************/

int read_specfreeze_data(char *filename,dataptr dz)
{
	int exit_status;
	int n = 0, OK = 1, is_direction_info = FALSE, linecnt = 0;
	FILE *fp;
	char temp[200], *p;
	double dtime;
	int arraysize = BIGARRAY;
	if((dz->iparray[FRZ_TIMETYPE] = (int  *)malloc(arraysize * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for array of freeze-timetypes.\n");
		return(MEMORY_ERROR);
	}
	if((dz->lparray[FRZ_SEGTIME]  = (int *)malloc(arraysize * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for array of freeze sample times.\n");
		return(MEMORY_ERROR);
	}
	if((fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,"nCannot open file %s to read data.\n",filename);
		return(DATA_ERROR);
	}
	while(OK && fgets(temp,200,fp)) {
		linecnt++;
 		p = temp;
		for(;;) {
			dz->iparray[FRZ_TIMETYPE][n] = FRZ_LIMIT;
			while(isspace(*p))
				p++;
			if(*p == '\n' || *p == '\0')
				break;
			if((exit_status = set_timetype(&p,n,&is_direction_info,dz))<0)
				return(exit_status);
			if((exit_status = read_time(&p,&dtime,n,linecnt,filename))<0)
				return(exit_status);
			dz->lparray[FRZ_SEGTIME][n] = round(dtime/dz->frametime);
			if(n==0) {
				if(dz->lparray[FRZ_SEGTIME][0]!=0) {
					if((exit_status = insert_values_at_time_zero(&n,dz))<0)
						return(exit_status);
				}
			} else if((exit_status = check_time_sequencing(n,filename,dz))<0)
				return(exit_status);
			if(dz->lparray[FRZ_SEGTIME][n] > dz->wlength) {
				dz->lparray[FRZ_SEGTIME][n] = dz->wlength;	 
				n++;
				OK = 0;
				break;
			}	
			if(++n>=arraysize) {
				arraysize += BIGARRAY;
				if((dz->lparray[FRZ_SEGTIME]  = 
				(int *)realloc((char *)dz->lparray[FRZ_SEGTIME], arraysize * sizeof(int)))==NULL) {
					sprintf(errstr,"INSUFFICIENT MEMORY to reallocate freeze sample times.\n");
					return(MEMORY_ERROR);
				}
				if((dz->iparray[FRZ_TIMETYPE] = 
				(int  *)realloc((char *)dz->iparray[FRZ_TIMETYPE],arraysize * sizeof(int)))==NULL) {
					sprintf(errstr,"INSUFFICIENT MEMORY to reallocate freeze timetypes.\n");
					return(MEMORY_ERROR);
				}
			}
		}
	}
	if(!is_direction_info) {
		sprintf(errstr,"No flags given in freeze data\n");
		return(DATA_ERROR);
	}
	if(dz->lparray[FRZ_SEGTIME][n-1]!=dz->wlength) {
		if(++n>=arraysize) {
			if((dz->lparray[FRZ_SEGTIME]  = (int *)realloc((char *)dz->lparray[FRZ_SEGTIME], n * sizeof(int)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY to reallocate freeze sample times.\n");
				return(MEMORY_ERROR);
			}
			if((dz->iparray[FRZ_TIMETYPE] = (int  *)realloc((char *)dz->iparray[FRZ_TIMETYPE],n * sizeof(int)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY to reallocate freeze timetypes.\n");
				return(MEMORY_ERROR);
			}
		}
		dz->lparray[FRZ_SEGTIME][n-1]  = dz->wlength;
		dz->iparray[FRZ_TIMETYPE][n-1] = FRZ_LIMIT;
	} else {
		if((dz->lparray[FRZ_SEGTIME]  = (int *)realloc((char *)dz->lparray[FRZ_SEGTIME], n * sizeof(int)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to reallocate freeze sample times.\n");
			return(MEMORY_ERROR);
		}
		if((dz->iparray[FRZ_TIMETYPE] = (int  *)realloc((char *)dz->iparray[FRZ_TIMETYPE],n * sizeof(int)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to reallocate freeze timetypes.\n");
			return(MEMORY_ERROR);
		}
	}
	dz->itemcnt = n;
	return(FINISHED);
}

/************************** READ_SPECFREEZE2_DATA ********************************/

int read_specfreeze2_data(char *filename,dataptr dz)
{
//	int n = 0, OK = 1, is_location = TRUE;
	int is_location = TRUE;
	FILE *fp;
	char temp[200], *p;
	double dtime;
	int arraysize = BIGARRAY;
 	dz->itemcnt = 0;
	if((dz->lparray[FRZ_FRZTIME]  = (int *)malloc(arraysize * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for array of freeze times.\n");
		return(MEMORY_ERROR);
	}
	if((dz->lparray[FRZ_SEGTIME]  = (int *)malloc(arraysize * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for array of freeze lengths.\n");
		return(MEMORY_ERROR);
	}
	if((fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,"nCannot open file %s to read data.\n",filename);
		return(DATA_ERROR);
	}
	while(fgets(temp,200,fp)) {
 		p = temp;
		while(isspace(*p))
			p++;
		if(*p==ENDOFSTR)
			continue;
		dz->scalefact = dz->duration/dz->frametime;
		while(get_float_from_within_string(&p,&dtime)) {
			dtime /= dz->frametime;
			dz->scalefact += (dtime - 1);
			if(is_location) {
				dz->lparray[FRZ_FRZTIME][dz->itemcnt] = round(dtime);
				if(dz->lparray[FRZ_FRZTIME][dz->itemcnt] < 0
				|| dz->lparray[FRZ_FRZTIME][dz->itemcnt] >= dz->wlength) {
					sprintf(errstr,"Invalid freeze location : %lf\n",dtime);
					return(DATA_ERROR);
				}
			} else {
				dz->lparray[FRZ_SEGTIME][dz->itemcnt] = round(dtime);
				if(dz->lparray[FRZ_SEGTIME][dz->itemcnt] < 0) {
					sprintf(errstr,"Invalid freeze duration : %lf\n",dtime);
					return(DATA_ERROR);
				}
			}
			is_location = !is_location;
			if(is_location) {
				if(++dz->itemcnt >= arraysize) {
					arraysize += BIGARRAY;
					if((dz->lparray[FRZ_FRZTIME]  = 
					(int *)realloc((char *)dz->lparray[FRZ_FRZTIME],arraysize * sizeof(int)))==NULL) {
						sprintf(errstr,"INSUFFICIENT MEMORY to realloc array of freeze times.\n");
						return(MEMORY_ERROR);
					}
					if((dz->lparray[FRZ_SEGTIME]  = 
					(int *)realloc((char *)dz->lparray[FRZ_SEGTIME],arraysize * sizeof(int)))==NULL) {
						sprintf(errstr,"INSUFFICIENT MEMORY to realloc array of freeze lengths.\n");
						return(MEMORY_ERROR);
					}
				}
			}
		}
	}
	if(!is_location) {
		sprintf(errstr,"Freeze times not paired correctly.\n");
		return(DATA_ERROR);
	}
	if((dz->lparray[FRZ_FRZTIME]  = 
	(int *)realloc((char *)dz->lparray[FRZ_FRZTIME],dz->itemcnt * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to realloc array of freeze times.\n");
		return(MEMORY_ERROR);
	}
	if((dz->lparray[FRZ_SEGTIME]  = 
	(int *)realloc((char *)dz->lparray[FRZ_SEGTIME],dz->itemcnt * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to realloc array of freeze lengths.\n");
		return(MEMORY_ERROR);
	}
	return(FINISHED);
}

/************************** SET_TIMETYPE ********************************/

int set_timetype(char **first_char_of_string,int strno,int *is_direction_info,dataptr dz)
{
	char *p = *first_char_of_string;
	switch(*p) {
	case('b'):
		dz->iparray[FRZ_TIMETYPE][strno] = FRZ_BEFORE;
		*is_direction_info = TRUE;
		p++;
		break;
	case('a'):
		dz->iparray[FRZ_TIMETYPE][strno]  = FRZ_AFTER;
		*is_direction_info = TRUE;
		p++;
		break;
	}
	*first_char_of_string = p;
	return(FINISHED);
}

/************************** READ_TIME ********************************/

int read_time(char **p,double *dtime,int strno,int linecnt, char *filename)
{			
	if(**p!='.' && **p!='-' && !isdigit(**p)) {
		sprintf(errstr,"Unknown time flag '%c' on line %d of file %s\n",**p, linecnt,filename);
		return(DATA_ERROR);
	}
	if(sscanf(*p,"%lf",dtime)!=1) {
		sprintf(errstr,"Cannot read time value %d on line %d of file %s.\n",strno+1, linecnt,filename);
		return(DATA_ERROR);
	}
	while(!isspace(**p) && **p!='\n' && **p != '\0')
		(*p)++;
	return(FINISHED);
}

/************************** INSERT_VALUES_AT_TIME_ZERO ********************************/

int insert_values_at_time_zero(int *n,dataptr dz)
{
	if(dz->lparray[FRZ_SEGTIME][0] < 0) {
		dz->lparray[FRZ_SEGTIME][0] = 0;
		*n = 0;
	} else {
		dz->lparray[FRZ_SEGTIME][1]  = dz->lparray[FRZ_SEGTIME][0];
		dz->iparray[FRZ_TIMETYPE][1] = dz->iparray[FRZ_TIMETYPE][0];		
		dz->lparray[FRZ_SEGTIME][0]  = 0;
		dz->iparray[FRZ_TIMETYPE][0] = FRZ_LIMIT;		
		*n = 1;
	}
	return(FINISHED);
}

/************************** CHECK_TIME_SEQUENCING ********************************/

int check_time_sequencing(int n,char *filename,dataptr dz)
{
	if(dz->lparray[FRZ_SEGTIME][n] <= dz->lparray[FRZ_SEGTIME][n-1]) {
		sprintf(errstr,"Time values out of sequence in file %s.\n",filename);
		return(DATA_ERROR);
	}
	return(FINISHED);
}


/********************************************************************************************/
/********************************** FORMERLY IN preprocess.c ********************************/
/********************************************************************************************/

/****************************** PARAM_PREPROCESS *********************************/

int param_preprocess(dataptr dz)	
{
//	int exit_status = FINISHED;

	switch(dz->process) {
	case(ACCU):			return scale_accu_parameters(dz);
	case(EXAG):			return convert_params_for_exag(dz);
	case(FOCUS):		return setup_internal_arrays_and_params_for_focus(dz);
	case(FOLD):			return fold_preprocess(dz);
	case(FREEZE):		return get_frztimes(dz);
	case(FREEZE2):		return(FINISHED);
	case(STEP):			return convert_params_for_step(dz);
	default:
		sprintf(errstr,"PROGRAMMING PROBLEM: Unknown process in param_preprocess()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);	/* NOTREACHED */
}

/************************************* SCALE_ACCU_PARAMETERS *******************/

int scale_accu_parameters(dataptr dz)
{
	int n;
	double *p;
		   		/* decayratio per sec -> decayratio per window */
	if(dz->brksize[ACCU_DINDEX]) {
		p = dz->brk[ACCU_DINDEX] + 1;
		for(n=0;n<dz->brksize[ACCU_DINDEX];n++) {
			*p = exp(log(*p) * dz->frametime);		
			 p += 2;
		}
	} else
		dz->param[ACCU_DINDEX] = exp(log(dz->param[ACCU_DINDEX]) * dz->frametime);

			  	/* glis in 8va per sec -> frqratio per window */
	if(dz->brksize[ACCU_GINDEX]) {
		p = dz->brk[ACCU_GINDEX] + 1;
		for(n=0;n<dz->brksize[ACCU_GINDEX];n++) {
			*p = pow(2.0,*p * dz->frametime);
			 p += 2;
		}
	} else
		dz->param[ACCU_GINDEX] = pow(2.0,dz->param[ACCU_GINDEX] * dz->frametime);
	return(FINISHED);
}

/************ CONVERT_PARAMS_FOR_EXAG *************/

int convert_params_for_exag(dataptr dz)
{
	double *p;
	int n;
	if(dz->brksize[EXAG_EXAG]) {	/* convert to reciprocal of value */
		p = dz->brk[EXAG_EXAG]+1;
		for(n=0;n<dz->brksize[EXAG_EXAG];n++) {
			*p = 1.0/(*p);
			p += 2;
		}
	} else
		dz->param[EXAG_EXAG] = 1.0/dz->param[EXAG_EXAG];
	return(FINISHED);
}

/************ SETUP_INTERNAL_ARRAYS_AND_PARAMS_FOR_FOCUS *************/

int setup_internal_arrays_and_params_for_focus(dataptr dz)
{
	int exit_status;
	dz->itemcnt = dz->iparam[FOCU_PKCNT];
	if(dz->brksize[FOCU_BW]==0)	{
		if((exit_status = generate_focu_filter_params(dz))<0)
			return(exit_status);
	}
	if((exit_status = establish_internal_arrays_for_focus(dz))<0)
		return(exit_status);
	if(dz->vflag[FOCUS_STABLE]) {
		if((exit_status = setup_stability_arrays_and_constants(FOCU_STABL,FOCU_SL1,dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}

/****************************** GENERATE_FOCU_FILTER_PARAMS ***************************/

int generate_focu_filter_params(dataptr dz)
{
	double halfbwidth = dz->param[FOCU_BW]/2.0;
	dz->scalefact = PI/halfbwidth;	/* Convert 8va offset from flt centre-frq to a fraction of the bandwidth X PI */
	dz->param[FOCU_BRATIO_UP] = pow(2.0,halfbwidth);			/* upward transpratio to top of filter band  */
	dz->param[FOCU_BRATIO_DN] = 1.0/dz->param[FOCU_BRATIO_UP];	/* Downward transpratio to bottom of filter band */
	return(FINISHED);
}

/********************** ESTABLISH_INTERNAL_ARRAYS_FOR_FOCUS **********************/

int establish_internal_arrays_for_focus(dataptr dz)
{									
	if((dz->filtpeak = (double *)malloc((dz->itemcnt+1) * sizeof(double)))==NULL) 	 {
		sprintf(errstr,"INSUFFICIENT MEMORY for focus peaks array.\n");
		return(MEMORY_ERROR);		/* stores amps of filter peaks */
	}						
	if((dz->fbandtop = (double *)malloc((dz->itemcnt+1) * sizeof(double)))==NULL)	 {
		sprintf(errstr,"INSUFFICIENT MEMORY for focus band tops array.\n");
		return(MEMORY_ERROR);		/* stores top of fiter bands */
	}							
	if((dz->fbandbot = (double *)malloc((dz->itemcnt+1) * sizeof(double)))==NULL)	 {
		sprintf(errstr,"INSUFFICIENT MEMORY for focus band bottoms array.\n");
		return(MEMORY_ERROR);		/* stores bots of filtbands */
	}							
	if((dz->fsampbuf = (float  *)malloc((dz->clength+1) * sizeof(float)))==NULL)	 {
		sprintf(errstr,"INSUFFICIENT MEMORY for filter contour array.\n");
		return(MEMORY_ERROR);		/* stores filter contour */
	}							
	if((dz->peakno   = (int    *)malloc((dz->itemcnt+1) * sizeof(int)))==NULL)		 {
		sprintf(errstr,"INSUFFICIENT MEMORY for focus peak location array.\n");
		return(MEMORY_ERROR);		/* specenv chans where peaks are */
	}
	memset(dz->peakno,0,dz->itemcnt * sizeof(int));
	/* set_array_safety */
	dz->filtpeak[dz->itemcnt] = 0.0;
    dz->fbandtop[dz->itemcnt] = 0.0;		/* stores top of fiter bands */
    dz->fbandbot[dz->itemcnt] = 0.0;		/* stores bottoms of filtbands */
	dz->fsampbuf[dz->clength] = 0.0f;		/* stores filter contour */
	dz->peakno[dz->itemcnt] = 0;			/* points to specenv chans where peaks are */
	return(FINISHED);
}

/************************** FOLD_PREPROCESS ******************************/

int fold_preprocess(dataptr dz)
{
	if(dz->brksize[FOLD_HIFRQ]==0) {
		dz->iparam[FOLD_HICHAN]   = (int)((dz->param[FOLD_HIFRQ] + dz->halfchwidth)/dz->chwidth); /* TRUNCATE */
		dz->param[FOLD_HICHANTOP] = ((double)dz->iparam[FOLD_HICHAN] * dz->chwidth) + dz->halfchwidth;
		if(dz->iparam[FOLD_HICHAN]!=0)
	   		dz->iparam[FOLD_HICHAN] <<= 1;
	}
	if(dz->brksize[FOLD_LOFRQ]==0) {
		dz->iparam[FOLD_LOCHAN]   = (int)((dz->param[FOLD_LOFRQ] + dz->halfchwidth)/dz->chwidth); /* TRUNCATE */
		if(dz->iparam[FOLD_LOCHAN]>0)
			dz->param[FOLD_LOCHANBOT] = ((double)(dz->iparam[FOLD_LOCHAN]-1) * dz->chwidth) + dz->halfchwidth;
		else
		   dz->param[FOLD_LOCHANBOT] = 0.0;
		if(dz->iparam[FOLD_LOCHAN]!=0)
	    	dz->iparam[FOLD_LOCHAN] <<= 1;
	}
	return(FINISHED);
}

/************************* GET_FRZTIMES ****************************/

int get_frztimes(dataptr dz)
{
	int n;
	if((dz->lparray[FRZ_FRZTIME] = (int *)malloc((dz->itemcnt-1) * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for freezetimes array.\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<dz->itemcnt-1;n++) {
 		if(dz->iparray[FRZ_TIMETYPE][n+1]<FRZ_BEFORE 
			|| dz->iparray[FRZ_TIMETYPE][n+1]>FRZ_AFTER) {
			sprintf(errstr,"Invalid timetype flag in get_frztimes()\n");
			return(PROGRAM_ERROR);
		}
		switch(dz->iparray[FRZ_TIMETYPE][n]) {
		case(FRZ_BEFORE):
		case(FRZ_LIMIT):
			switch(dz->iparray[FRZ_TIMETYPE][n+1]) {
			case(FRZ_LIMIT):
			case(FRZ_AFTER):  dz->lparray[FRZ_FRZTIME][n] = -1;						
				break;
			case(FRZ_BEFORE): dz->lparray[FRZ_FRZTIME][n] = dz->lparray[FRZ_SEGTIME][n+1];
				break;
			}
			break;
		case(FRZ_AFTER):
			switch(dz->iparray[FRZ_TIMETYPE][n+1]) {
			case(FRZ_LIMIT):
			case(FRZ_AFTER):  dz->lparray[FRZ_FRZTIME][n] = dz->lparray[FRZ_SEGTIME][n];	
				break; 			
			case(FRZ_BEFORE):
				sprintf(errstr,"Impossible time sequence in freeze file.\n");
				return(USER_ERROR);
			}
			break;
		default:
			if(n==0) {
				sprintf(errstr,"Invalid timetype flag in get_frztimes()\n");
				return(PROGRAM_ERROR);
			}
		}
	}
	return rationalise_frztimes(dz);
}

/************************** RATIONALISE_FRZTIMES **************************
 *
 * Bear in mind that there are scnt times, icluding the time at the
 * end of the file. This means there are (scnt-1) time-segments to
 * operate on and therefore (scnt-1) frztimes.
 *
 * (1)	If two consecutive time-segments are flagged (frztime<0) to
 *	have no action on them, then we can eliminate the time (and
 *	the related frztime info) of the 2nd window.
 * (2)	Do this by shuffling existing values above this time[n] downwards,
 *	overwriting the existing value (ditto for 'frztime' vals), up as far
 *	end of frztime array,and therefore 1 below top of time array.
 * (3)	Reduce value of time_seg counter.
 * (4)	Move the uppermost time_seg value down.
 * (5)	Replace the absolute values of the time vals, to differences,
 *	which will be the successive numbers of windows to process when
 *	the processing begins.
 * (6)	Hence reduce by 1 the count of time vals stored.
 * (7)	Realloc memory space for time and frztime values.
 */

int rationalise_frztimes(dataptr dz)
{
	int n, m;
	int orig_itemcnt = dz->itemcnt;  
	for(n=1;n<dz->itemcnt-1;n++) {
		if(dz->lparray[FRZ_FRZTIME][n-1]<0 && dz->lparray[FRZ_FRZTIME][n]<0) {/* 1 */
			for(m=n;m<dz->itemcnt-1;m++) {									  
				dz->lparray[FRZ_SEGTIME][n] = dz->lparray[FRZ_SEGTIME][n+1];/* 2 */
				dz->lparray[FRZ_FRZTIME][n] = dz->lparray[FRZ_FRZTIME][n+1];
			}
			dz->itemcnt--;													/* 3 */
			dz->lparray[FRZ_SEGTIME][dz->itemcnt-1] 
				= dz->lparray[FRZ_SEGTIME][dz->itemcnt];/* 4 */
		}
	}
	if(dz->itemcnt != orig_itemcnt) {
		if((dz->lparray[FRZ_SEGTIME] = 
		(int *)realloc((char *)dz->lparray[FRZ_SEGTIME],dz->itemcnt * sizeof(int)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to reallocate freeze segtime array.\n");
			return(MEMORY_ERROR);
		}
		if((dz->lparray[FRZ_FRZTIME] = 
		(int *)realloc((char *)dz->lparray[FRZ_FRZTIME],(dz->itemcnt-1) * sizeof(int)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to reallocate freezetimes array.\n");
			return(MEMORY_ERROR);
		}
	}
	return(FINISHED);
}

/************ CONVERT_PARAMS_FOR_STEP *************/

int convert_params_for_step(dataptr dz)
{
	double *p;
	int n;
	if(dz->brksize[STEP_STEP]==0)
		dz->iparam[STEP_STEP] = round(dz->param[STEP_STEP]/dz->frametime);
	else {
		p = dz->brk[STEP_STEP] + 1;
		for(n=0;n < dz->brksize[STEP_STEP];n++) {
			*p /= dz->frametime;
			p  += 2;
		}
		dz->is_int[STEP_STEP] = 1;
	}	
	return(FINISHED);
}

/********************************************************************************************/
/********************************** FORMERLY IN procspec.c **********************************/
/********************************************************************************************/

/**************************** SPEC_PROCESS_FILE ****************************/

int spec_process_file(dataptr dz)
{	
	dz->total_windows = 0;

	if(dz->process == FREEZE2)
		dz->tempsize = (int)round(dz->scalefact) * dz->wanted;

	display_virtual_time(0L,dz);

	switch(dz->process) {
	case(ACCU):		return outer_loop(dz);
	case(EXAG):		return outer_loop(dz);
	case(FOLD):		return outer_loop(dz);
	
	case(FOCUS): 	return outer_focu_loop(dz);
	case(FREEZE):	return specfreeze(dz);								
	case(FREEZE2):	return specfreeze2(dz);								
	case(STEP):  	return specstep(dz);
	default:
		sprintf(errstr,"Unknown process in spec_process_file()\n");
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
		case(FOCUS):	exit_status = specfocus(peakscore,descnt,least,dz);	break;
		case(FOLD):		exit_status = specfold(dz);							break;
		case(ACCU):		exit_status = specaccu(dz);							break;
		case(EXAG):		exit_status = specexag(&local_zero_set,dz);			break;
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
	int exit_status = FINISHED;
	int vc;
	switch(dz->process) {
	case(EXAG):	  case(FOCUS): case(ACCU):	 
		switch(dz->process) {
		case(ACCU):
			for(vc = 0; vc < dz->wanted; vc += 2)
				dz->windowbuf[0][AMPP] = dz->flbufptr[0][AMPP];
			break;
		case(FOCUS):
		   if(!dz->vflag[FOCUS_STABLE])
		   		break;
			rectify_window(dz->flbufptr[0],dz);
			if((exit_status = extract_specenv(0,0,dz))<0)
				return(exit_status);
 /* put 1st specenv in end of stability store && copy 1st buf to end of bufstore */
			memmove((char *)dz->stable->spec[dz->iparam[FOCU_SL1]],
//TW ERROR IN ORIGINAL CODE
//				(char *)dz->specenvamp,dz->infile->specenvcnt * sizeof(int));
				(char *)dz->specenvamp,dz->infile->specenvcnt * sizeof(float));
			memmove((char *)dz->stable->sbuf[dz->iparam[FOCU_SL1]],
				(char *)dz->flbufptr[0],(size_t)dz->wanted * sizeof(float));
			dz->stable->total_pkcnt[dz->iparam[FOCU_SL1]] = 0; /* 1st window has 0 amp, hence 0 peaks */
	   		break;
		}
		return(TRUE);
	}
	return(FALSE);
}

/****************************** CONSTRUCT_FILTER_ENVELOPE ***************************
 *
 * If we reach the top of the channels, and there is another peak to work on,
 * we must be backtracking to a lower channel. If we are NOT, there is a problem
 * in my logic ... no peak can have its lowest point higher than the frq in the highest
 * channel!! That's why we have used CHECKLOGIC to check this out!!!
 */

int construct_filter_envelope(int pkcnt_here,float *fbuf,dataptr dz)
{
#define OVERSHOOT	(8)	/* No of channels to test beyond current ch, for frqs BELOW current frq */
						/* Guesstimate */
	int exit_status;
	double thisfrq, thisamp, filt_centre_frq;
	int n, m;
	int  baktrak = -1, activate_baktrak = 0, checklogic = 0;
	int  beyond_top, band_incremented;
	int cc , vc;
    for(cc=0; cc<dz->clength; cc++)	/* PRESET FILTER ENVELOPE TO ZERO */ 
   		dz->fsampbuf[cc] = 0.0f;
	n = 0;						/* COUNTER FOR CURRENT PEAK */
	m = 1;						/* COUNTER FOR NEXT PEAK */
	cc = 0;
	vc = 0;
	beyond_top = 0;
	while(cc < dz->clength) {
		switch(dz->process) {
		case(FOCUS): filt_centre_frq = dz->specenvfrq[dz->peakno[n]]; 	break;
		default:
			sprintf(errstr,"Invalid application in construct_filter_envelope()\n");
			return(PROGRAM_ERROR);

		}
		band_incremented = 0;
		thisfrq = fabs(fbuf[FREQ]);
		if(baktrak<0 && (m < pkcnt_here) && (thisfrq > dz->fbandbot[m]))
			baktrak = cc;						/* IF A FRQ IS ENCOUNTERED WHICH IS IN NEXT BAND
												   AND FOR THE FIRST TIME : mark this as channel to baktrak to */
		if(thisfrq < dz->fbandbot[n])	{  			
			if(beyond_top)						/* IF FRQ BELOW BAND: IGNORE IT */
				beyond_top++;					/* If beyond possible band_top, count channels beyond */
		}
		else if(thisfrq < filt_centre_frq) {		/* IF FRQ IN LOWER HALFBAND: generate filter envel */
			if((exit_status = gen_amplitude_in_lo_half_filterband(&thisamp,thisfrq,filt_centre_frq,dz))<0)
				return(exit_status);
			dz->fsampbuf[cc] = (float)max(dz->fsampbuf[cc],thisamp);
			if(beyond_top)
				beyond_top++;				/* If beyond possible band_top, count channels beyond */
		}
		else if(thisfrq < dz->fbandtop[n])	{		 /* IF FRQ IN HIGHER HALFBAND: generate filter envel */
			if((exit_status = gen_amplitude_in_hi_half_filterband(&thisamp,thisfrq,filt_centre_frq,dz))<0)
				return(exit_status);
			dz->fsampbuf[cc] = (float)max(dz->fsampbuf[cc],thisamp);
			if(beyond_top)
				beyond_top++;				/* If beyond possible band_top, count channels beyond */
		}
		else {							/* FREQ is ABOVE TOP OF BAND */
			if(!beyond_top) {			/* IF only just reached possible bandtop among channel frqs */
				if(baktrak < 0)			/* If channel to baktrak to has not been set, set it here */
					baktrak = cc;
			}
			if(++beyond_top>OVERSHOOT){/* IF GONE FAR ENOUGH BEYOND POSSIBLE BAND TOP */
				beyond_top = 0;			/* switch off band_top flag */
				if(++n >= pkcnt_here)	/* MOVE ON TO NEXT BAND  */			
					break;				/* IF REACHED END OF ALL BANDS: stop  */
				m++;
				band_incremented = 1;	/* Note that band has been incremented */
				activate_baktrak = 1;	/* activate the baktraking mechanism */
			}
		}
		if(cc == dz->clength-1) {			/* IF REACHED TOP OF CHANNELS ON THIS PASS */
			checklogic = 1;
			if(!band_incremented) {		/* IF BAND HAS NOT ALREADY BEEN INCREMENTED */
				beyond_top = 0;			/* Ensure band_top flag is switched off */
				if(++n >= pkcnt_here)	/* MOVE ON TO NEXT BAND  */				
					break;				/* IF REACHED END OF ALL BANDS: stop  */
				m++;
				activate_baktrak = 1;	/* activate the baktraking mechanism */
			}
		}
		if(activate_baktrak && baktrak>=0) {/* If necessary, baktrak to channel just inside bottom of next band */
			if(checklogic)
				checklogic++;
			cc = baktrak;
			vc = cc * 2;
			baktrak = -1;				 /* Cancel baktrak value */
			activate_baktrak = 0;		 /* Deactivate baktraking mechanism */
			continue;
		}
		if(checklogic==1) {
			sprintf(errstr,"Problem with peak-scanning logic. construct_filter_envelope()\n");
			return(PROGRAM_ERROR);
		}
		checklogic = 0;
		cc++;							/* Otherwise, move on to next channel */
		vc += 2;
	}
	return filter_band_test(dz);
}

/********************************************************************************************/
/********************************** FORMERLY IN pconsistency.c ******************************/
/********************************************************************************************/

/****************************** CHECK_PARAM_VALIDITY_AND_CONSISTENCY *********************************/

int check_param_validity_and_consistency(dataptr dz)
{
//	int exit_status = FINISHED;
	handle_pitch_zeros(dz);
	switch(dz->process) {
	case(FOCUS):	   return check_consistency_of_frq_limits_for_focus(dz);
	}
	return(FINISHED);
}

/********************** CHECK_CONSISTENCY_OF_FRQ_LIMITS_FOR_FOCUS **********************/

int check_consistency_of_frq_limits_for_focus(dataptr dz)
{
	if(dz->brksize[FOCU_LOFRQ]==0 && dz->brksize[FOCU_HIFRQ]==0) {
		if(flteq(dz->param[FOCU_LOFRQ],dz->param[FOCU_HIFRQ])) {
			sprintf(errstr,"Frequency limits define a zero-width band.\n");
			return(USER_ERROR);
		}
		if(dz->param[FOCU_LOFRQ] > dz->param[FOCU_HIFRQ])
			swap(&dz->param[FOCU_LOFRQ],&dz->param[FOCU_HIFRQ]);
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
	case(ACCU):		case(EXAG):		case(FOLD):		
	case(STEP):		
		return allocate_single_buffer(dz);
	case(FOCUS):
		if(dz->vflag[FOCUS_STABLE])
			return allocate_double_buffer(dz);
		return allocate_single_buffer(dz);

	case(FREEZE): 	
	case(FREEZE2): 	
		return allocate_double_buffer(dz);
	default:
		sprintf(errstr,"Unknown program no. in allocate_large_buffers()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);	/* NOTREACHED */
}
/********************************************************************************************/
/********************************** FORMERLY IN cmdline.c ***********************************/
/********************************************************************************************/

int get_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	if     (!strcmp(prog_identifier_from_cmdline,"accu"))  	   		dz->process = ACCU;	 
	else if(!strcmp(prog_identifier_from_cmdline,"exag"))	   		dz->process = EXAG;
	else if(!strcmp(prog_identifier_from_cmdline,"focus"))	   		dz->process = FOCUS;
	else if(!strcmp(prog_identifier_from_cmdline,"fold"))	   		dz->process = FOLD;
	else if(!strcmp(prog_identifier_from_cmdline,"freeze"))	   		dz->process = FREEZE;
	else if(!strcmp(prog_identifier_from_cmdline,"hold"))	   		dz->process = FREEZE2;
	else if(!strcmp(prog_identifier_from_cmdline,"step"))	   		dz->process = STEP;
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
	"\nFOCUSING OPERATIONS ON A SPECTRAL FILE\n\n"
	"USAGE: focus NAME (mode) infile outfile parameters: \n"
	"\n"
	"where NAME can be any one of\n"
	"\n"
	"accu    exag    focus    fold    freeze    hold     step\n\n"
	"Type 'focus accu' for more info on focus accu..ETC.\n");
	return(USAGE_ONLY);
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
	if(!strcmp(str,"accu")) {
		fprintf(stdout,
		"focus accu infile outfile [-ddecay] [-gglis]\n"
		"\n"
		"SUSTAIN EACH SPECTRAL BAND, UNTIL LOUDER DATA APPEARS IN THAT BAND\n"
		"\n"
		"-d    sutained channel data decays by factor DECAY per sec.\n"
		"      (Possible Range : %lf to 1.0 : Default 1.0)\n"
		"      (Suggested Effective Range : %lf to 0.5)\n"
		"-g    sutained channel data glisses at GLIS 8vas per sec.\n"
		"      (Approx Range : -11.7 to 11.7 : Default 0)\n"
		"\n",ACCU_MIN_DECAY,ACCU_MIN_DECAY);
	} else if(!strcmp(str,"exag")) {
		fprintf(stdout,
		"focus exag infile outfile exaggeration\n"
		"\n"
		"EXAGGERATE SPECTRAL CONTOUR\n"
		"\n"
		"exaggeration >0 will widen troughs: <0 will widen peaks.\n"
		"\n"
		"exaggeration may vary over time.\n"
		"\n");
	} else if(!strcmp(str,"focus")) {
		fprintf(stdout,
		"focus focus infile outfile -fN|-pN [-i] pk bw [-bbt] [-ttp] [-sval]\n"
		"\n"
		"FOCUS SPECTRAL ENERGY ONTO PEAKS IN SPECTRUM\n"
		"\n"
		"-f     extract formant envelope linear frqwise,\n"
		"       using 1 point for every N equally-spaced frequency-channels.\n"
		"-p     extract formant envelope linear pitchwise,\n"
		"       using N equally-spaced pitch-bands per octave.\n"
		"-i   quicksearch for formants (less accurate).\n"
		"pk   (max) number of peaks to find : Range 1 - %d\n"
		"bw   bandwidth of peak-centred filters, in octaves.\n"
		"-b   BT is bottom frequency to start peak search.\n"
		"-t   TP is top frequency to end peak search.\n"
		"-s   Attempt to retain only peaks which are STABLE over time.\n"
		"       Range 2 - %d : default %d : is no. of windows over which peaks averaged.\n"
		"\n"
		"bandwidth,bottom frequency & top frequency may vary over time.\n"
		"\n",MAXPKCNT,MAXSTABIL,DEFAULT_STABILITY);
	} else if(!strcmp(str,"fold")) {
		fprintf(stdout,
		"focus fold infile outfile lofrq hifrq [-x]\n"
		"\n"
		"OCTAVE-TRANSPOSE SPECTRAL COMPONENTS INTO SPECIFIED RANGE\n"
		"\n"
		"lofrq & hifrq specify range into which spectrum is folded,\n"
		"-x            Fuller spectrum.\n"
		"\n"
		"lofrq & hifrq  may vary over time. \n");
	} else if(!strcmp(str,"freeze")) {
		fprintf(stdout,
		"focus freeze mode infile outfile datafile\n\n"
		"FREEZE SPECTRAL CHARACTERISTICS IN SOUND, AT GIVEN TIMES \n\n"
		"Datafile contains times at which spectrum is frozen.\n"
		"These times may be preceded by character MARKERS....\n\n"
		"a   use window here as freezewindow for spectrum AFTER this time.\n"
		"b   use window here as freezewindow for spectrum BEFORE this time.\n\n"
		"Otherwise, times are end\\start of freeze established at one of these markers.\n\n"
		"MODES\n"
		"1   freeze channel amplitudes\n"
		"2   freeze channel frequencies\n"
		"3   freeze channel amplitudes & frequencies\n"
		"\n");
	} else if(!strcmp(str,"hold")) {
		fprintf(stdout,
//TW UPDATE
//		"focus hold mode infile outfile datafile\n\n"
		"focus hold infile outfile datafile\n\n"
		"HOLD SOUND SPECTRUM, AT GIVEN TIMES \n\n"
		"Datafile contains times at which spectrum is held, & hold-duration.\n"
		"These data items must be paired correctly.\n"
		"The process expands each hold window to the duration given,\n"
		"before proceeding to the next window. The output file is therefore longer\n"
		"than the input file.\n"
		"\n");
	} else if(!strcmp(str,"step")) {
		fprintf(stdout,
		"focus step infile outfile timestep\n"
		"\n"
		"STEP-FRAME THROUGH SOUND BY FREEZING SPECTRUM AT REGULAR TIME INTERVALS\n"
		"\n"
		"Once a freeze window is reached, the following windows are given the spectrum\n"
		"of that window, until the freeze end is reached. The output is the same duration\n"
		"as the input.\n"
		"\n"
		"timestep   duration of steps. Must be >= duration of 2 analysis frames.\n"
		"           (Rounded internally to a multiple of analysis-frame time.)\n");
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





