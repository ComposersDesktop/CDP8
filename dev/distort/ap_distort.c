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
#include <math.h>
#include <ctype.h>
#include <structures.h>
#include <cdpmain.h>
#include <tkglobals.h>
#include <pnames.h>
#include <distort.h>
#include <processno.h>
#include <modeno.h>
#include <globcon.h>
#include <logic.h>
#include <filetype.h>
//TW REMOVED (duplicated)
//#include <stdlib.h>
#include <mixxcon.h>
//TW UPDATE
#include <distort1.h>
#include <flags.h>
#include <speccon.h>
#include <arrays.h>
#include <special.h>
#include <formants.h>
#include <sfsys.h>
#include <osbind.h>
#include <string.h>
#include <arrays.h>


/********************************************************************************************/
/********************************** FORMERLY IN buffers.c ***********************************/
/********************************************************************************************/

static int  create_sndbufs_plus_cyclebuf(int cycbuflen_param,dataptr dz);
static int  create_avgbufs(dataptr dz);
//TW UPDATE
static int	create_pulse_buffers(dataptr dz);

/********************************************************************************************/
/********************************** FORMERLY IN pconsistency.c ******************************/
/********************************************************************************************/

static int  check_omit_consistency(dataptr dz);

/********************************************************************************************/
/********************************** FORMERLY IN specialin.c *********************************/
/********************************************************************************************/

static int  get_envfile_data(char *filename,dataptr dz);
//TW UPDATE
static int  get_pulsefile_data(char *filename,dataptr dz);
static int  get_harmonic_data(char *filename,dataptr dz);

/***************************** ESTABLISH_BUFPTRS_AND_EXTRA_BUFFERS **************************/

int establish_bufptrs_and_extra_buffers(dataptr dz)
{
	int exit_status;
//TW AGREED DELETION is_spec VARIABLE
	dz->extra_bufcnt = -1;	/* ENSURE EVERY CASE HAS A PAIR OF ENTRIES !! */
	dz->bptrcnt = 0;
	dz->bufcnt  = 0;
	switch(dz->process) {
	case(DISTORT):	
		switch(dz->mode) {
	    case(DISTORT_EXAGG):	dz->extra_bufcnt = 0;	dz->bufcnt = 3;		break;
		default:				dz->extra_bufcnt = 0;	dz->bufcnt = 2;		break;
		}
		break;
	case(DISTORT_ENV):			dz->extra_bufcnt = 0;	dz->bufcnt = 2;		break;
	case(DISTORT_AVG):			dz->extra_bufcnt = 0;	dz->bufcnt = 4;		break;
	case(DISTORT_OMT):			dz->extra_bufcnt = 0;	dz->bufcnt = 1;		break;
	case(DISTORT_MLT):			dz->extra_bufcnt = 0;	dz->bufcnt = 6;		break;
	case(DISTORT_DIV):			dz->extra_bufcnt = 0;	dz->bufcnt = 4;		break;
	case(DISTORT_HRM):			dz->extra_bufcnt = 0;	dz->bufcnt = 3; 	break;
	case(DISTORT_FRC):			dz->extra_bufcnt = 0;	dz->bufcnt = 3;		break;
	case(DISTORT_REV):			dz->extra_bufcnt = 0;	dz->bufcnt = 2;		break;
	case(DISTORT_SHUF):			dz->extra_bufcnt = 0;	dz->bufcnt = 3;		break;
	case(DISTORT_RPTFL):
	case(DISTORT_RPT2):
	case(DISTORT_RPT):			dz->extra_bufcnt = 0;	dz->bufcnt = 4;		break;
	case(DISTORT_INTP):			dz->extra_bufcnt = 0;	dz->bufcnt = 5;		break;
	case(DISTORT_DEL):			dz->extra_bufcnt = 0;	dz->bufcnt = 3;		break;
	case(DISTORT_RPL):			dz->extra_bufcnt = 0;	dz->bufcnt = 3;		break;
	case(DISTORT_TEL):			dz->extra_bufcnt = 0;	dz->bufcnt = 3;		break;
	case(DISTORT_FLT):			dz->extra_bufcnt = 0;	dz->bufcnt = 3;		break;
	case(DISTORT_INT):
		switch(dz->mode) {
		case(DISTINT_INTRLV):	dz->extra_bufcnt = 0;	dz->bufcnt = 3;		break;
		case(DISTINT_RESIZE):	dz->extra_bufcnt = 0;	dz->bufcnt = 4;		break;
		default:
			sprintf(errstr,"Unknown mode for DISTORT_INT in establish_bufptrs_and_extra_buffers()\n");
			return(PROGRAM_ERROR);
		}
		break;
	case(DISTORT_CYCLECNT):		dz->extra_bufcnt = 0;	dz->bufcnt = 2;		break;
	case(DISTORT_PCH):			dz->extra_bufcnt = 0;	dz->bufcnt = 4;		break;
//TW UPDATE NEW CASES
	case(DISTORT_OVERLOAD):
		switch(dz->mode) {
		case(OVER_NOISE):	dz->extra_bufcnt = 0;	dz->bufcnt = 1;		break;
		case(OVER_SINE):	dz->extra_bufcnt = 0;	dz->bufcnt = 1;		break;
		}
		break;
	case(DISTORT_PULSED):	dz->extra_bufcnt = 0;	dz->bufcnt = 4;		break;

	default:
		sprintf(errstr,"Unknown program type [%d] in establish_bufptrs_and_extra_buffers()\n",dz->process);
		return(PROGRAM_ERROR);
	}

	if(dz->extra_bufcnt < 0) {
		sprintf(errstr,"bufcnts have not been set: establish_bufptrs_and_extra_buffers()\n");
		return(PROGRAM_ERROR);
	}
	if((dz->process==HOUSE_SPEC && dz->mode==HOUSE_CONVERT) || dz->process==INFO_DIFF) {
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
	case(DISTORT): 	  	dz->array_cnt=1; dz->iarray_cnt=0; dz->larray_cnt = 0; dz->ptr_cnt = 0;	dz->fptr_cnt = 0; break;
	case(DISTORT_ENV):	dz->array_cnt=1; dz->iarray_cnt=0; dz->larray_cnt = 0; dz->ptr_cnt = 2;	dz->fptr_cnt = 0; break;
	case(DISTORT_AVG):	dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt = 2; dz->ptr_cnt = 0;	dz->fptr_cnt = 0; break;
	case(DISTORT_OMT):	dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt = 0; dz->ptr_cnt = 0;	dz->fptr_cnt = 0; break;
	case(DISTORT_MLT):	dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt = 0; dz->ptr_cnt = 0;	dz->fptr_cnt = 0; break;
	case(DISTORT_DIV):	dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt = 0; dz->ptr_cnt = 0;	dz->fptr_cnt = 0; break;
	case(DISTORT_HRM):	dz->array_cnt=1; dz->iarray_cnt=1; dz->larray_cnt = 0; dz->ptr_cnt = 0;	dz->fptr_cnt = 0; break;
	case(DISTORT_FRC):	dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt = 2; dz->ptr_cnt = 0;	dz->fptr_cnt = 0; break;
	case(DISTORT_REV):	dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt = 0; dz->ptr_cnt = 0;	dz->fptr_cnt = 0; break;
	case(DISTORT_SHUF):	dz->array_cnt=0; dz->iarray_cnt=1; dz->larray_cnt = 1; dz->ptr_cnt = 0;	dz->fptr_cnt = 0; break;
	case(DISTORT_RPTFL):
	case(DISTORT_RPT2):
	case(DISTORT_RPT):	dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt = 0; dz->ptr_cnt = 0;	dz->fptr_cnt = 0; break;
	case(DISTORT_INTP):	dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt = 0; dz->ptr_cnt = 0;	dz->fptr_cnt = 0; break;
	case(DISTORT_DEL):	dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt = 2; dz->ptr_cnt = 0;	dz->fptr_cnt = 0; break;
	case(DISTORT_RPL):	dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt = 2; dz->ptr_cnt = 0;	dz->fptr_cnt = 0; break;
	case(DISTORT_TEL):	dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt = 2; dz->ptr_cnt = 0;	dz->fptr_cnt = 0; break;
	case(DISTORT_FLT):	dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt = 0; dz->ptr_cnt = 0;	dz->fptr_cnt = 0; break;
	case(DISTORT_INT):	dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt = 0; dz->ptr_cnt = 0;	dz->fptr_cnt = 0; break;
	case(DISTORT_CYCLECNT):
						dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt = 0; dz->ptr_cnt = 0;	dz->fptr_cnt = 0; break;
	case(DISTORT_PCH):	dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt = 0; dz->ptr_cnt = 0;	dz->fptr_cnt = 0; break;
//TW UPDATE NEW CASES
	case(DISTORT_OVERLOAD):
		switch(dz->mode) {
		case(OVER_NOISE):	dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt = 0; dz->ptr_cnt = 0;	dz->fptr_cnt = 0; break;
		case(OVER_SINE):	dz->array_cnt=1; dz->iarray_cnt=0; dz->larray_cnt = 0; dz->ptr_cnt = 0;	dz->fptr_cnt = 0; break;
		}
		break;
	case(DISTORT_PULSED):	dz->array_cnt=3; dz->iarray_cnt=1; dz->larray_cnt = 0; dz->ptr_cnt = 2;	dz->fptr_cnt = 0; break;
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

		/*RWD dz->lfarray shadows lparray  for distort del, etc */
		if((dz->lfarray = (float    **)malloc(dz->larray_cnt * sizeof(float *)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for internal float arrays.\n");
			return(MEMORY_ERROR);
		}

		for(n=0;n<dz->larray_cnt;n++){
			dz->lparray[n] = NULL;
			dz->lfarray[n] = NULL;		/*RWD*/
		}
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
	case(DISTORT):		setup_process_logic(SNDFILES_ONLY,		  	EQUAL_SNDFILE,		SNDFILE_OUT,	dz);	break;
	case(DISTORT_ENV):	setup_process_logic(SNDFILES_ONLY,		  	EQUAL_SNDFILE,		SNDFILE_OUT,	dz);	break;
	case(DISTORT_AVG):	setup_process_logic(SNDFILES_ONLY,		  	UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(DISTORT_OMT):	setup_process_logic(SNDFILES_ONLY,		  	EQUAL_SNDFILE,		SNDFILE_OUT,	dz);	break;
	case(DISTORT_MLT):	setup_process_logic(SNDFILES_ONLY,		  	UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(DISTORT_DIV):	setup_process_logic(SNDFILES_ONLY,		  	UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(DISTORT_HRM):	setup_process_logic(SNDFILES_ONLY,		  	UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(DISTORT_FRC):	setup_process_logic(SNDFILES_ONLY,		  	UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(DISTORT_REV):	setup_process_logic(SNDFILES_ONLY,		  	UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(DISTORT_SHUF):	setup_process_logic(SNDFILES_ONLY,		  	UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(DISTORT_RPTFL):
	case(DISTORT_RPT2):
	case(DISTORT_RPT):	setup_process_logic(SNDFILES_ONLY,		  	UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(DISTORT_INTP):	setup_process_logic(SNDFILES_ONLY,		  	UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(DISTORT_DEL):	setup_process_logic(SNDFILES_ONLY,		  	UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(DISTORT_RPL):	setup_process_logic(SNDFILES_ONLY,		  	UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(DISTORT_TEL):	setup_process_logic(SNDFILES_ONLY,		  	UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(DISTORT_FLT):	setup_process_logic(SNDFILES_ONLY,		  	UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(DISTORT_INT):	setup_process_logic(TWO_SNDFILES,		  	UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(DISTORT_CYCLECNT):	
						setup_process_logic(SNDFILES_ONLY,		  	SCREEN_MESSAGE,		NO_OUTPUTFILE,	dz);	break;
	case(DISTORT_PCH):	setup_process_logic(SNDFILES_ONLY,		  	UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
//TW UPDATE NEW CASES
	case(DISTORT_OVERLOAD):	setup_process_logic(SNDFILES_ONLY,		EQUAL_SNDFILE,		SNDFILE_OUT,	dz);	break;
	case(DISTORT_PULSED):	setup_process_logic(SNDFILES_ONLY,		UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;

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
	case(DISTORT):	  	exit_status = set_internalparam_data("i"    ,ap);				break;
	case(DISTORT_ENV):	exit_status = set_internalparam_data("d"    ,ap);				break;
	case(DISTORT_AVG):	exit_status = set_internalparam_data("i"    ,ap);				break;
	case(DISTORT_OMT):	return(FINISHED);												break;
	case(DISTORT_MLT):	return(FINISHED);												break;
	case(DISTORT_DIV):	return(FINISHED);												break;
	case(DISTORT_HRM):	exit_status = set_internalparam_data("iii"  ,ap);				break;
	case(DISTORT_FRC):	exit_status = set_internalparam_data("i"    ,ap);				break;
	case(DISTORT_REV):	return(FINISHED);												break;
	case(DISTORT_SHUF):	exit_status = set_internalparam_data("ii"   ,ap);				break;
	case(DISTORT_RPTFL):
	case(DISTORT_RPT2):
	case(DISTORT_RPT):	return(FINISHED);												break;
	case(DISTORT_INTP):	return(FINISHED);												break;
	case(DISTORT_DEL):	return(FINISHED);												break;
	case(DISTORT_RPL):	return(FINISHED);												break;
	case(DISTORT_TEL):	return(FINISHED);												break;
	case(DISTORT_FLT):	exit_status = set_internalparam_data("dd"   ,ap);				break;
	case(DISTORT_INT):	return(FINISHED);												break;
	case(DISTORT_CYCLECNT):
						return(FINISHED);												break;
	case(DISTORT_PCH):	return(FINISHED);												break;
//TW NEW CASES
	case(DISTORT_OVERLOAD):	return(FINISHED);											break;
	case(DISTORT_PULSED):	exit_status = set_internalparam_data("ii",ap);				break;
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
//TW CONFIRMED DELETE exit_status = FINISHED;
	aplptr ap = dz->application;

	switch(ap->special_data) {
	case(DISTORT_ENVELOPE):		return get_envfile_data(str,dz);
//TW UPDATE NEW CASE
	case(PULSE_ENVELOPE):		return get_pulsefile_data(str,dz);
	case(HARMONIC_DISTORT):		return get_harmonic_data(str,dz);
	case(SHUFFLE_DATA):			return read_shuffle_data(DISTORTS_DMNCNT,DISTORTS_IMGCNT,DISTORTS_MAP,str,dz);
	default:
		sprintf(errstr,"Unknown special_data type: read_special_data()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);	/* NOTREACHED */
}

/************************ GET_ENVFILE_DATA ***************************/

int get_envfile_data(char *filename,dataptr dz)
{
	int cnt = 0, n;
	double maxtime;
	char temp[200], *p;
	int arraysize = BIGARRAY;
//TW CONFIRMED DELETE aplptr ap = dz->application;
	FILE *fp;
	if((dz->parray[DISTORTE_ENV] = (double *)malloc(arraysize * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for distortion envelope.\n");
		return(MEMORY_ERROR);
	}
	if((fp = fopen(filename,"r"))==NULL) {			/* 2 */
		sprintf(errstr,"Failed to open envelope file %s\n",filename);
		return(DATA_ERROR);
	}
	while(fgets(temp,200,fp)!=NULL) {				/* 3 */
		p = temp;
		while(get_float_from_within_string(&p,&(dz->parray[DISTORTE_ENV][cnt]))) {
			if(cnt==0) {		     /* FORCE ZERO-TIME POINT AT TAB START */
				if(dz->parray[DISTORTE_ENV][cnt]<0.0) {
					sprintf(errstr,"-ve time (%lf) line %d in file %s\n",dz->parray[DISTORTE_ENV][cnt],(cnt/2)+1,filename);
					return(DATA_ERROR);
				}
				if(flteq(dz->parray[DISTORTE_ENV][cnt],0.0)) {
					dz->parray[DISTORTE_ENV][cnt] = 0.0;	/* FORCE 0.0 TIME TO exactly 0.0 */
				} else { 	 								/* Add zero-time values */
					dz->parray[DISTORTE_ENV][2] = dz->parray[DISTORTE_ENV][0];
					dz->parray[DISTORTE_ENV][3] = dz->parray[DISTORTE_ENV][1];
					dz->parray[DISTORTE_ENV][0] = 0.0;
					cnt += 2;
				}
			} else {
				if(EVEN(cnt)) {	/* Time vals */
						/* CHECK TIME SEQUENCING */
					if(dz->parray[DISTORTE_ENV][cnt] <= dz->parray[DISTORTE_ENV][cnt-2]) {
						sprintf(errstr,"Time values out of sequence (%lf : %lf)in envelope file at line %d\n",
						dz->parray[DISTORTE_ENV][cnt-2],dz->parray[DISTORTE_ENV][cnt],(cnt/2)+1);
						return(DATA_ERROR);
					}
				} else {		/* Env values */
					if(dz->parray[DISTORTE_ENV][cnt]<dz->application->min_special 
					|| dz->parray[DISTORTE_ENV][cnt]>dz->application->max_special) {	/* CHECK RANGE */  
						sprintf(errstr,"Invalid envelope value (%lf): line %d file %s\n",
						dz->parray[DISTORTE_ENV][cnt],(cnt/2)+1,filename);
						return(DATA_ERROR);
					}
				}
			}
			if(++cnt >= arraysize) {
				arraysize += BIGARRAY;
				if((dz->parray[DISTORTE_ENV]=
				(double *)realloc((char *)dz->parray[DISTORTE_ENV],arraysize*sizeof(double)))==NULL){
					sprintf(errstr,"INSUFFICIENT MEMORY to reallocate distortion envelope.\n");
					return(MEMORY_ERROR);
				}
			}
		}
	}
	if(ODD(cnt)) {
		sprintf(errstr,"Envelope vals incorrectly paired in file %s\n",filename);
		return(DATA_ERROR);
	}
	if(cnt==0) {
		sprintf(errstr,"No envelope values found in file %s\n",filename);
		return(DATA_ERROR);
	}
	
	if((dz->parray[DISTORTE_ENV]=(double *)realloc(dz->parray[DISTORTE_ENV],cnt * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to reallocate distortion envelope.\n");
		return(MEMORY_ERROR);
	}

	dz->ptr[DISTORTE_ENVEND] = &(dz->parray[DISTORTE_ENV][cnt]);    /* MARK END OF ENVELOPE DATA */

	maxtime = dz->parray[DISTORTE_ENV][cnt-2]; 						/* FIND MAXIMUM TIME IN DATA */
	for(n=2;n<cnt;n +=2)	 					  /* NORMALISE DATA TIMES TO LIE BETWEEN 0 AND 1 */
		dz->parray[DISTORTE_ENV][n] = (float)(dz->parray[DISTORTE_ENV][n]/maxtime);
	dz->parray[DISTORTE_ENV][cnt-2] = 1.0;    				  /* FORCE FINAL TIME TO exactly 1.0 */
	if(fclose(fp)<0) {
		fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
		fflush(stdout);
	}
	return(FINISHED);
}    

//TW UPDATE : NEW FUNCTION
/************************ GET_PULSEFILE_DATA ***************************
 *
 * NB DISTORTE_ENV = 0, and so this function works for DISTORT_PULSED,
 *  putting the data in parray[0]
 */

int get_pulsefile_data(char *filename,dataptr dz)
{
	int cnt = 0, n;
	double maxtime, dummy;
	char temp[200], *p;
	int arraysize = BIGARRAY;
//	aplptr ap = dz->application;
	FILE *fp;
	p = filename;
	if(sloom) {
		if(*p == NUMERICVAL_MARKER) {
			p++;
			if(sscanf(p,"%lf",&dummy)!=1) {
				sprintf(errstr,"Invalid numeric value for pulse envelope.\n");
				return(PROGRAM_ERROR);
			}
			if(dummy!=0.0) {
				sprintf(errstr,"Pulse envelope must be either '0' (for no envelope) or a brkpoint envelope file.\n");
				return(DATA_ERROR);
			}
			if(dz->mode == PULSE_IMP) {
				sprintf(errstr,"Numeric input for pulse envelope should be impossible in this mode.\n");
				return(PROGRAM_ERROR);
			}
			dz->iparam[PULSE_ENVSIZE] = 0;
			return(FINISHED);
		}
	} else {
		if(*p == '0') {
				p++;
			while(*p=='0')
				p++;
			if(*p=='.')
				p++;
			while(*p=='0')
				p++;
			if(*p==ENDOFSTR) {
				if(dz->mode == PULSE_IMP) {
					sprintf(errstr,"You must provide an envelope file for the impulse, for this process\n");
					return(DATA_ERROR);
				}
				dz->iparam[PULSE_ENVSIZE] = 0;
				return(FINISHED);
			}
		}
	}
	if((dz->parray[ORIG_PULSENV] = (double *)malloc(arraysize * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for pulse envelope.\n");
		return(MEMORY_ERROR);
	}
	if((fp = fopen(filename,"r"))==NULL) {			/* 2 */
		sprintf(errstr,"Failed to open envelope file %s\n",filename);
		return(DATA_ERROR);
	}
	while(fgets(temp,200,fp)!=NULL) {				/* 3 */
		p = temp;
		while(get_float_from_within_string(&p,&(dz->parray[ORIG_PULSENV][cnt]))) {
			if(cnt==0) {		     /* FORCE ZERO-TIME POINT AT TAB START */
				if(dz->parray[ORIG_PULSENV][cnt]<0.0) {
					sprintf(errstr,"-ve time (%lf) line %d in file %s\n",dz->parray[ORIG_PULSENV][cnt],(cnt/2)+1,filename);
					return(DATA_ERROR);
				}
				if(flteq(dz->parray[ORIG_PULSENV][cnt],0.0)) {
					dz->parray[ORIG_PULSENV][cnt] = 0.0;	/* FORCE 0.0 TIME TO exactly 0.0 */
				} else { 	 								/* Add zero-time values */
					dz->parray[ORIG_PULSENV][2] = dz->parray[ORIG_PULSENV][0];
					dz->parray[ORIG_PULSENV][3] = dz->parray[ORIG_PULSENV][1];
					dz->parray[ORIG_PULSENV][0] = 0.0;
					cnt += 2;
				}
			} else {
				if(EVEN(cnt)) {	/* Time vals */
						/* CHECK TIME SEQUENCING */
					if(dz->parray[ORIG_PULSENV][cnt] <= dz->parray[ORIG_PULSENV][cnt-2]) {
						sprintf(errstr,"Time values out of sequence (%lf : %lf)in pulse envelope file at line %d\n",
						dz->parray[ORIG_PULSENV][cnt-2],dz->parray[ORIG_PULSENV][cnt],(cnt/2)+1);
						return(DATA_ERROR);
					}
				} else {		/* Env values */
					if(dz->parray[ORIG_PULSENV][cnt]<dz->application->min_special 
					|| dz->parray[ORIG_PULSENV][cnt]>dz->application->max_special) {	/* CHECK RANGE */  
						sprintf(errstr,"Invalid envelope value (%lf): line %d file %s\n",
						dz->parray[ORIG_PULSENV][cnt],(cnt/2)+1,filename);
						return(DATA_ERROR);
					}
				}
			}
			if(++cnt >= arraysize) {
				arraysize += BIGARRAY;
				if((dz->parray[ORIG_PULSENV]=
				(double *)realloc((char *)dz->parray[ORIG_PULSENV],arraysize*sizeof(double)))==NULL){
					sprintf(errstr,"INSUFFICIENT MEMORY to reallocate pulse envelope.\n");
					return(MEMORY_ERROR);
				}
			}
		}
	}
	if(ODD(cnt)) {
		sprintf(errstr,"Pulse envelope vals incorrectly paired in file %s\n",filename);
		return(DATA_ERROR);
	}
	if(cnt==0) {
		sprintf(errstr,"No pulse envelope values found in file %s\n",filename);
		return(DATA_ERROR);
	}
	
	if((dz->parray[ORIG_PULSENV]=(double *)realloc(dz->parray[ORIG_PULSENV],cnt * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to reallocate pulse envelope.\n");
		return(MEMORY_ERROR);
	}


	dz->iparam[PULSE_ENVSIZE] = cnt/2;
	maxtime = dz->parray[ORIG_PULSENV][cnt-2]; 						/* FIND MAXIMUM TIME IN DATA */
	for(n=2;n<cnt;n +=2)	 					  /* NORMALISE DATA TIMES TO LIE BETWEEN 0 AND 1 */
		dz->parray[ORIG_PULSENV][n] = (float)(dz->parray[ORIG_PULSENV][n]/maxtime);
	dz->parray[ORIG_PULSENV][cnt-2] = 1.0;    				  /* FORCE FINAL TIME TO exactly 1.0 */
	if(fclose(fp)<0) {
		fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
		fflush(stdout);
	}
	return(FINISHED);
}    

/************************ GET_HARMONIC_DATA ***************************/

int get_harmonic_data(char *filename,dataptr dz)
{
	int n = 0;
	aplptr ap = dz->application;
	char temp[200], *p;
	int arraysize = BIGARRAY;
	FILE *fp;
	if((fp = fopen(filename,"r"))==NULL) {			/* 2 */
		sprintf(errstr,"Cannot open harmonics file %s\n",filename);
		return(DATA_ERROR);
	}
	if((dz->iparray[DISTORTH_HNO]  = (int *)malloc(arraysize * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store harmonic numbers.\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[DISTORTH_AMP] = (double *)malloc(arraysize * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store harmonic amps.\n");
		return(MEMORY_ERROR);
	}
	while(fgets(temp,200,fp)!=NULL) {				/* 3 */
		p = temp;
		while(isspace(*p)) {
			if(*(++p) == ENDOFSTR)
				break;
		}
		if(*p == ENDOFSTR)		  /* ignore blank lines */
			continue;
		if(sscanf(temp,"%d%lf",&(dz->iparray[DISTORTH_HNO][n]),&(dz->parray[DISTORTH_AMP][n]))!=2) {
			sprintf(errstr,"Cannot read harmonic no/val pair in file %s\n",filename);
			return(DATA_ERROR);
		}
		if(dz->iparray[DISTORTH_HNO][n] < (int)ap->min_special 
		|| dz->iparray[DISTORTH_HNO][n] > (int)ap->max_special) {	/* TW */
			sprintf(errstr,"Error in harmonics file - harmonic number [%d] out of range %.0lf - %.0lf\n",
			dz->iparray[DISTORTH_HNO][n],ap->min_special,ap->max_special);
			return(DATA_ERROR);
		} 
		if(dz->parray[DISTORTH_AMP][n] < (int)ap->min_special2 
		|| dz->parray[DISTORTH_AMP][n] > (int)ap->max_special2) {	/* TW */
			sprintf(errstr,"Error in harmonics file - harmonic amp out of range %lf - %lf\n",ap->min_special2,ap->max_special2);
			return(DATA_ERROR);
		} 
		if(++n >= arraysize) {				/* 5 */
			arraysize += BIGARRAY;
			if((dz->iparray[DISTORTH_HNO]=(int *)realloc(dz->iparray[DISTORTH_HNO],arraysize*sizeof(int)))==NULL){
				sprintf(errstr,"INSUFFICIENT MEMORY to reallocate harmonic numbers.\n");
				return(MEMORY_ERROR);
			}
			if((dz->parray[DISTORTH_AMP]=(double *)realloc(dz->parray[DISTORTH_AMP],arraysize*sizeof(double)))==NULL){
				sprintf(errstr,"INSUFFICIENT MEMORY to reallocate harmonic amps.\n");
				return(MEMORY_ERROR);
			}
		}
	}
	if(fclose(fp)<0) {
		fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
		fflush(stdout);
	}
	if(n==0) {
		sprintf(errstr,"No harmonics found in file\n");
		return(DATA_ERROR);
	}
	if((dz->iparray[DISTORTH_HNO]=(int *)realloc(dz->iparray[DISTORTH_HNO],n * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to reallocate harmonic numbers.\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[DISTORTH_AMP]=(double *)realloc(dz->parray[DISTORTH_AMP],n * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to reallocate harmonic amps.\n");
		return(MEMORY_ERROR);
	}
	dz->iparam[DISTORTH_HCNT] = n;
	return(FINISHED);
}    

/********************************************************************************************/
/********************************** FORMERLY IN preprocess.c ********************************/
/********************************************************************************************/


/****************************** PARAM_PREPROCESS *********************************/

int param_preprocess(dataptr dz)	
{
//TW CONFIRMED DELETE int exit_status = FINISHED;

	switch(dz->process) {
	case(DISTORT):		return distort_preprocess(dz);	
	case(DISTORT_ENV):	return distortenv_preprocess(dz);	
	case(DISTORT_MLT):	return distortmlt_preprocess(dz);
	case(DISTORT_DIV):	return distortdiv_preprocess(dz);
	case(DISTORT_SHUF): return distortshuf_preprocess(dz);
 	case(DISTORT_DEL):	return distortdel_preprocess(dz);
	case(DISTORT_FLT):	return distortflt_preprocess(dz);
	case(DISTORT_AVG):	return distorter_preprocess(DISTORTA_CYCLECNT,DISTORTA_STARTCYC,DISTORTA_CYCLEN,dz);
	case(DISTORT_FRC):	return distorter_preprocess(DISTORTF_SCALE,DISTORTF_STARTCYC,DISTORTF_CYCLEN,dz);
	case(DISTORT_RPL):	return distorter_preprocess(DISTRPL_CYCLECNT,DISTRPL_STARTCYC,DISTRPL_CYCLEVAL,dz);
	case(DISTORT_TEL):	return distorter_preprocess(DISTTEL_CYCLECNT,DISTTEL_STARTCYC,DISTTEL_CYCLEVAL,dz);

	case(DISTORT_HRM):	
		dz->iparam[FOLDOVER_WARNING] = FALSE;	
		return FINISHED;
	case(DISTORT_PCH):	
		initrand48();							
		return FINISHED;

	case(DISTORT_OMT):	case(DISTORT_REV):	case(DISTORT_RPT):		case(DISTORT_RPTFL):
	case(DISTORT_INTP):	case(DISTORT_INT):	case(DISTORT_CYCLECNT):	case(DISTORT_RPT2):
		return FINISHED;
//TW UPDATE NEW CASES
	case(DISTORT_OVERLOAD):
	   	return overload_preprocess(dz);
	case(DISTORT_PULSED):
	   	return preprocess_pulse(dz);

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
	if(dz->process!=DISTORT_CYCLECNT && dz->process!=DISTORT_PULSED)
		display_virtual_time(0L,dz);
	switch(dz->process) {
	case(DISTORT):
		if((exit_status = process_with_swapped_bufs_on_single_half_cycles(dz))<0)
			return(exit_status);
		break;
	case(DISTORT_ENV):
	case(DISTORT_REV):
		if((exit_status = process_with_swapped_bufs_on_full_cycles(dz))<0)
			return(exit_status);
		break;
	case(DISTORT_AVG):
		if((exit_status = 
		process_with_swapped_bufs_on_full_cycles_with_newsize_output_and_skipcycles(dz->sampbuf[2],DISTORTA_SKIPCNT,dz))<0)
			return(exit_status);
		break;
	case(DISTORT_SHUF):
		if((exit_status = 
		process_with_swapped_bufs_on_full_cycles_with_newsize_output_and_skipcycles(dz->sampbuf[2],DISTORTS_SKIPCNT,dz))<0)
			return(exit_status);
		break;
	case(DISTORT_RPTFL):
	case(DISTORT_RPT):
	case(DISTORT_RPT2):
		if((exit_status = 
		process_with_swapped_bufs_on_full_cycles_with_newsize_output_and_skipcycles(dz->sampbuf[2],DISTRPT_SKIPCNT,dz))<0)
			return(exit_status);
		break;
	case(DISTORT_INTP):
		if((exit_status = 
		process_with_swapped_bufs_on_full_cycles_with_newsize_output_and_skipcycles(dz->sampbuf[2],DISTINTP_SKIPCNT,dz))<0)
			return(exit_status);
		break;
	case(DISTORT_DEL):
		if((exit_status = 
		process_with_swapped_bufs_on_full_cycles_with_newsize_output_and_skipcycles(dz->sampbuf[2],DISTDEL_SKIPCNT,dz))<0)
			return(exit_status);
		break;
	case(DISTORT_RPL):
		if((exit_status = 
		process_with_swapped_bufs_on_full_cycles_with_newsize_output_and_skipcycles(dz->sampbuf[2],DISTRPL_SKIPCNT,dz))<0)
			return(exit_status);
		break;
	case(DISTORT_TEL):
		if((exit_status = 
		process_with_swapped_bufs_on_full_cycles_with_newsize_output_and_skipcycles(dz->sampbuf[2],DISTTEL_SKIPCNT,dz))<0)
			return(exit_status);
		break;
	case(DISTORT_FLT):
		if((exit_status = 
		process_with_swapped_bufs_on_full_cycles_with_newsize_output_and_skipcycles(dz->sampbuf[2],DISTFLT_SKIPCNT,dz))<0)
			return(exit_status);
		break;
	case(DISTORT_OMT):
		if((exit_status = process_on_single_buf_with_phase_dependence(dz))<0)
			return(exit_status);
		break;
	case(DISTORT_MLT):
	case(DISTORT_DIV):
		if((exit_status = process_with_swapped_buf_to_swapped_outbufs(dz))<0)
			return(exit_status);
		break;
	case(DISTORT_HRM):
	case(DISTORT_FRC):
		if((exit_status = process_with_swapped_bufs_on_full_cycles_with_optional_prescale(dz))<0)
			return(exit_status);
		break;
	case(DISTORT_INT):
		switch(dz->mode) {
		case(DISTINT_INTRLV):
			if((exit_status = two_infiles_interleave_process(dz))<0)
				return(exit_status);
			break;
		case(DISTINT_RESIZE):
			if((exit_status = two_infiles_resize_process(dz))<0)
				return(exit_status);
			break;
		default:
			sprintf(errstr,"Unknown case in DISTORT_INT mode switch in process_file()\n");
			return(PROGRAM_ERROR);
		}
		break;
	case(DISTORT_CYCLECNT):
		if((exit_status = process_cyclecnt(dz))<0)
			return(exit_status);
		break;
	case(DISTORT_PCH):
		if((exit_status = distort_pitch(dz))<0)
			return(exit_status);
		break;
//TW UPDATE NEW CASES
	case(DISTORT_OVERLOAD):
		if((exit_status = distort_overload(dz))<0)
			return(exit_status);
		break;
	case(DISTORT_PULSED):
		if((exit_status = do_pulsetrain(dz))<0)
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
//TW CONFIRED DELETE int exit_status = FINISHED;
	handle_pitch_zeros(dz);
	switch(dz->process) {
	case(DISTORT_OMT):	return check_omit_consistency(dz); 	
	}
	return(FINISHED);
}

/********************************** CHECK_OMIT_CONSISTENCY **********************************/

int check_omit_consistency(dataptr dz)
{
	int exit_status;
	double brkmax;
	if(dz->brksize[DISTORTO_OMIT]) {
		if((exit_status = get_maxvalue_in_brktable(&brkmax,DISTORTO_OMIT,dz))<0)
			return(exit_status);
		if(round(brkmax) >= dz->iparam[DISTORTO_KEEP]) {
			sprintf(errstr,"A in brkfile > or = B at some point: can't proceed.\n");
			return(USER_ERROR);
		}
	} else {
		if(dz->iparam[DISTORTO_OMIT] >= dz->iparam[DISTORTO_KEEP]) {
			sprintf(errstr,"A > or = B: can't proceed.\n");
			return(USER_ERROR);
		}
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
	case(DISTORT):		case(DISTORT_ENV):	case(DISTORT_OMT):		
	case(DISTORT_MLT):	case(DISTORT_DIV):	case(DISTORT_REV):		
	case(DISTORT_SHUF):	case(DISTORT_RPT):	case(DISTORT_INTP):		case(DISTORT_RPT2):
	case(DISTORT_DEL):	case(DISTORT_RPL):	case(DISTORT_TEL):		
	case(DISTORT_FLT):	case(DISTORT_INT):	case(DISTORT_CYCLECNT):	
	case(DISTORT_PCH):	case(DISTORT_OVERLOAD):	case(DISTORT_RPTFL):
		return create_sndbufs(dz);

	case(DISTORT_AVG):		
		return create_avgbufs(dz);
	case(DISTORT_HRM):		
		return create_sndbufs_plus_cyclebuf(DISTORTH_CYCBUFLEN,dz);
	case(DISTORT_FRC):		
		return create_sndbufs_plus_cyclebuf(DISTORTF_CYCBUFLEN,dz);
//TW UPDATE : NEW CASE
	case(DISTORT_PULSED):		
		return create_pulse_buffers(dz);
	default:
		sprintf(errstr,"Unknown program no. in allocate_large_buffers()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);	/* NOTREACHED */
}

/************************************* CREATE_PULSE_BUFFERS *************************************/

int create_pulse_buffers(dataptr dz)
{
/*  dz->sampbuf[0] = INBUF	has to be extra SECSIZE in length, to allow for rounding samples at end
	dz->sampbuf[1] = OUTBUF
	dz->sampbuf[2] = STOREBUF stores set of wavsets for synth option
	dz->sampbuf[3] = PULSEBUF stores pulse while it's being made
*/
	int exit_status;
	double minfrq, mintrans, k;
	size_t minbufsize, bigbufsize;

	if(dz->brksize[PULSE_FRQ] > 0) {
		if((exit_status = get_minvalue_in_brktable(&minfrq,PULSE_FRQ,dz))<0)
			return(exit_status);
	} else
		minfrq = dz->param[PULSE_FRQ];
	if(dz->param[PULSE_FRQRAND] > 0.0) {				/* if frq could be random-transposed */
		k = dz->param[PULSE_FRQRAND]/SEMITONES_PER_OCTAVE;
		k = pow(2.0,k);														
		minfrq /= k;									/* transpose it down by max amount possible */
	}
	if(dz->brksize[PULSE_TRANSPOS] > 0) {
		if((exit_status = get_minvalue_in_brktable(&mintrans,PULSE_TRANSPOS,dz))<0)
			return(exit_status);
	} else
		mintrans = dz->param[PULSE_TRANSPOS];
	mintrans /= SEMITONES_PER_OCTAVE;
	mintrans = pow(2.0,mintrans);
	minfrq *= mintrans;
	k = (double)(dz->infile->srate/minfrq);
	minbufsize = (int)floor(k + 1);
	bigbufsize = (size_t) Malloc(-1);	/* i.e. grab a big buffer */
	bigbufsize /= sizeof(float);
	bigbufsize = (bigbufsize/(4 * F_SECSIZE)) * 4 * F_SECSIZE;
	bigbufsize /= 4;
	while(bigbufsize <= minbufsize) {
		if((bigbufsize += F_SECSIZE) <= 0) {	/* Numeric overflow */
			sprintf(errstr,"Failed to allocate memory for sound buffers: A.\n");
			return(MEMORY_ERROR);
		}
	}
	dz->buflen = (int) bigbufsize;
	if((dz->bigbuf = (float *)malloc((dz->buflen * 4 * sizeof(float)) + F_SECSIZE))==NULL) {
		sprintf(errstr,"Failed to allocate memory for sound buffers: B.\n");
		return(MEMORY_ERROR);
	}
	dz->sampbuf[0] = dz->bigbuf;
	dz->sampbuf[1] = dz->sampbuf[0] + dz->buflen + F_SECSIZE;
	dz->sampbuf[2] = dz->sampbuf[1] + dz->buflen;
	dz->sampbuf[3] = dz->sampbuf[2] + dz->buflen;
	return(FINISHED);
}

/*************************** CREATE_SNDBUFS_PLUS_CYCLEBUF **************************/

int create_sndbufs_plus_cyclebuf(int cycbuflen_param,dataptr dz)
{
/*
 * RWD this would be 12 samples then... 
 *  #define SAFETY	(24)
*/
#define SAFETY	(48)
	size_t bigbufsize;
	int n;
	int  cyclebuf_size;
	cyclebuf_size = (int)round((double)dz->infile->srate*(double)sizeof(float)*(double)dz->infile->channels*MAXWAVELEN);
	cyclebuf_size += SAFETY;

	dz->iparam[cycbuflen_param]  = (int)(cyclebuf_size/sizeof(float));
	dz->bufcnt--;
	bigbufsize = (size_t) Malloc(-1);
	dz->buflen = (int)(bigbufsize / sizeof(float));
	dz->buflen = (dz->buflen / dz->infile->channels) * dz->infile->channels;
	if((dz->bigbuf = (float *)malloc(sizeof(float) * ((dz->buflen * dz->bufcnt) + dz->iparam[cycbuflen_param]))) == NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<dz->bufcnt;n++)
		dz->sbufptr[n] = dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
	dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);	  /* CYCLEBUF */
	return(FINISHED);
}

/*************************** CREATE_AVGBUFS **************************/
#define FSECSIZE (256)

int create_avgbufs(dataptr dz)
{
	size_t bigbufsize;
	long cyclbuf_size;  	
	cyclbuf_size = round(dz->infile->srate * sizeof(float) * dz->infile->channels * dz->param[DISTORTA_MAXLEN]);
	if((dz->sampbuf[3] = (float *)malloc(cyclbuf_size))==NULL) {	  /* calculation buffer */
		sprintf(errstr,"INSUFFICIENT MEMORY to create cycles buffer.\n");
		return(MEMORY_ERROR);
	}
	dz->iparam[DISTORTA_CYCBUFLEN] = (int)(cyclbuf_size/sizeof(float));
	bigbufsize = (size_t) Malloc(-1);
	dz->buflen = (int)(bigbufsize / sizeof(float));
	dz->buflen /= 3;
	if(dz->buflen==0)
		dz->buflen = FSECSIZE;

	dz->buflen =  (dz->buflen / dz->infile->channels) * dz->infile->channels;

//TW CHANGED
//	if((dz->bigbuf = (float *)malloc((dz->buflen * 3))) == NULL) {
	if((dz->bigbuf = (float *)malloc((dz->buflen * 3 * sizeof(float)))) == NULL) {
		sprintf(errstr, "INSUFFICIENT MEMORY to create sound buffers.\n");
		return(MEMORY_ERROR);
	}
	dz->sampbuf[0] = dz->bigbuf;					/* pair of input bufs */
	dz->sampbuf[1] = dz->sampbuf[0] + dz->buflen;
	dz->sampbuf[2] = dz->sampbuf[1] + dz->buflen;	/* output buffer */
	return(FINISHED);
}

/********************************************************************************************/
/********************************** FORMERLY IN cmdline.c ***********************************/
/********************************************************************************************/

int get_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	if     (!strcmp(prog_identifier_from_cmdline,"reform"))   		dz->process = DISTORT;  
	else if(!strcmp(prog_identifier_from_cmdline,"envel"))    		dz->process = DISTORT_ENV;
	else if(!strcmp(prog_identifier_from_cmdline,"average"))  		dz->process = DISTORT_AVG;
	else if(!strcmp(prog_identifier_from_cmdline,"omit"))  	  		dz->process = DISTORT_OMT;
	else if(!strcmp(prog_identifier_from_cmdline,"multiply")) 		dz->process = DISTORT_MLT;
	else if(!strcmp(prog_identifier_from_cmdline,"divide"))   		dz->process = DISTORT_DIV;
	else if(!strcmp(prog_identifier_from_cmdline,"harmonic")) 		dz->process = DISTORT_HRM;
	else if(!strcmp(prog_identifier_from_cmdline,"fractal"))  		dz->process = DISTORT_FRC;
	else if(!strcmp(prog_identifier_from_cmdline,"reverse"))  		dz->process = DISTORT_REV;
	else if(!strcmp(prog_identifier_from_cmdline,"shuffle"))  		dz->process = DISTORT_SHUF;
	else if(!strcmp(prog_identifier_from_cmdline,"repeat"))   		dz->process = DISTORT_RPT;
	else if(!strcmp(prog_identifier_from_cmdline,"repeat2"))   		dz->process = DISTORT_RPT2;
	else if(!strcmp(prog_identifier_from_cmdline,"replim"))			dz->process = DISTORT_RPTFL;
	else if(!strcmp(prog_identifier_from_cmdline,"interpolate"))   	dz->process = DISTORT_INTP;
	else if(!strcmp(prog_identifier_from_cmdline,"delete"))   		dz->process = DISTORT_DEL;
	else if(!strcmp(prog_identifier_from_cmdline,"replace"))  		dz->process = DISTORT_RPL;
	else if(!strcmp(prog_identifier_from_cmdline,"telescope"))		dz->process = DISTORT_TEL;
	else if(!strcmp(prog_identifier_from_cmdline,"filter"))   		dz->process = DISTORT_FLT;
	else if(!strcmp(prog_identifier_from_cmdline,"interact")) 		dz->process = DISTORT_INT;
	else if(!strcmp(prog_identifier_from_cmdline,"cyclecnt")) 		dz->process = DISTORT_CYCLECNT;
	else if(!strcmp(prog_identifier_from_cmdline,"pitch"))    		dz->process = DISTORT_PCH;
//TW UPDATE NEW CASES
	else if(!strcmp(prog_identifier_from_cmdline,"overload"))    	dz->process = DISTORT_OVERLOAD;
	else if(!strcmp(prog_identifier_from_cmdline,"pulsed"))    		dz->process = DISTORT_PULSED;
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
	"\nUSAGE: distort NAME (mode) infile (infile2) outfile parameters: \n"
	"\n"
	"where NAME can be any one of\n"
	"\n"
	"    DISTORT OPERATIONS ON A SINGLE SOUND FILE\n\n"
   	"average   divide    fractal      multiply  repeat    replim     reverse\n"
	"cyclecnt  envel     harmonic     omit      replace    shuffle\n"
	"delete    filter    interpolate  pitch     reform     telescope\n"
	"overload  pulsed    repeat2"
	"\n"
	"    DISTORT OPERATIONS ON TWO SOUND FILES\n\n"
	"interact\n"
	"\n"
	"Type 'distort reform' for more info on reform option.. ETC.\n\n"
	"DISTORT PROCESSES WORK ONLY ON mono FILES.\n");
	return(USAGE_ONLY);
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
	if(!strcmp(str,"reform")) {		
		sprintf(errstr,
		"USAGE:\n"
		"distort reform 1-7 infile outfile \n"
		"distort reform 8   infile outfile  exaggeration\n\n"
		" MODIFY SHAPE OF 'WAVECYCLES'\n\n"
		"MODES ARE\n"
		"1 Convert to fixed level square_wave\n"
		"2 Convert to square wave\n"
		"3 Convert to fixed level triangular wave\n"
		"4 Convert to triangular wave\n"
		"5 Convert to inverted half_cycles\n"
		"6 Convert to click stream\n"
		"7 Convert to sinusoid\n"
		"8 Exaggerate waveform contour\n\n"
		"EXAGGERATION may vary over time.\n\n"
		"Works on MONO files only\n");
	} else if (!strcmp(str,"envel")) {		
		sprintf(errstr,
		"USAGE:\n"
		"distort envel 1-2 infile outfile         cyclecnt [-ttroughing] [-eexponent]\n"
		"distort envel 3   infile outfile         cyclecnt troughing     [-eexponent]\n"
		"distort envel 4   infile outfile envfile cyclecnt \n\n"
		"IMPOSE ENVELOPE OVER EACH GROUP OF cyclecnt 'WAVECYCLES'\n\n"
		"MODES (Works on MONO files only)-\n"
		"1  rising envelope.\n"
		"2  falling envelope.\n"
		"3  troughed envelope.\n"
		"4  user defined envelope.\n"
		"CYCLECNT  is no of wavecycles under a single envelope.\n"
		"EXPONENT  is exponent for envelope rise or decay:\n"
        "          If OMMITED envelope rise/decay is linear.\n"
		"TROUGHING is trough of envelope (0-1: default 0).\n"
		"ENVFILE   Defines user envelope as time/val(0-1) pairs.\n"
		"          (Time units arbitrary, as envelope stretched to each cycle(set) duration.\n\n"
		"CYCLECNT, TROUGHING and EXPONENT, may vary over time\n\n"
		"Works on MONO files only\n");
	} else if (!strcmp(str,"average")) {		
		sprintf(errstr,
		"USAGE:\n"
		"distort average infile outfile cyclecnt [-mmaxwavelen] [-sskipcycles]\n\n"
		"AVERAGE THE WAVESHAPE OVER N 'WAVECYCLES'\n\n"
		"CYCLECNT   is number of cycles to average over ( > 1)\n"
		"MAXWAVELEN is max permissible wavelength in seconds. (default %.2lf)\n"
		"SKIPCYCLES: (integer) is no. of wavecycles to skip at start of file\n",MAXWAVELEN);
	} else if (!strcmp(str,"omit")) {		
		sprintf(errstr,
		"USAGE:\n"
		"distort omit infile outfile A B\n\n"
		"OMIT A OUT OF EVERY B 'WAVECYCLES' REPLACING THEM BY SILENCE\n\n"
		"A may vary over time: but must always be less than B.\n\n"
		"Works on MONO files only\n");
	} else if (!strcmp(str,"multiply")) {		
		sprintf(errstr,
		"USAGE:\n"
		"distort multiply infile outfile N [-s]\n\n"
		"DISTORTION BY MULTIPLYING 'WAVECYCLE' FREQUENCY.\n\n"
		"N = multiplier (integer only: range 2-16)\n"
    	"-s smoothing: try if glitches appear.\n\n"
		"Works on MONO files only\n");
	} else if (!strcmp(str,"divide")) {		
		sprintf(errstr,
		"USAGE:\n"
		"distort divide infile outfile N [-i]\n\n"
		"DISTORTION BY DIVIDING 'WAVECYCLE' FREQUENCY.\n\n"
		"N = divider (integer only: range 2-16)\n"
		"-i uses waveform interpolation: slower, but cleaner.\n\n"
		"Works on MONO files only\n");
	} else if (!strcmp(str,"harmonic")) {		
		sprintf(errstr,
		"USAGE:\n"
		"distort harmonic infile outfile harmonics-file [-ppre_attenuation]\n\n"
		"HARMONIC DISTORTION BY SUPERIMPOSING 'HARMONICS' ONTO 'WAVECYCLES'.\n\n"
		"HARMONICS-FILE contains harmonic_no/amplitude pairs.\n"
		"    where amplitude of src sound is taken to be 1.0\n"
		"    and harmonics range between %.0lf and %.0lf\n\n"
		"PRE_ATTENUATION is applied to input sound before processing.\n\n"
		"Works on MONO files only\n",MIN_HARMONIC,MAX_HARMONIC);
	} else if (!strcmp(str,"fractal")) {		
		sprintf(errstr,
		"USAGE:\n"
		"distort fractal infile outfile scaling loudness [-ppre_attenuation]\n\n"
		"SUPERIMPOSE MINIATURE COPIES OF SRC 'WAVECYCLES' ONTO THEMSELVES.\n\n"
		"SCALING  = (integer) division of scale of src wave (range: %.0lf to %.2lf*srate)\n"
		"LOUDNESS = loudness of scaled component, relative to src (loudness 1.0).\n\n"
		"PRE_ATTENUATION is applied to input sound before processing.\n\n"
		"scaling and loudness may vary over time\n\n"
		"Works on MONO files only\n",MIN_SCALE,MAXWAVELEN);
	} else if (!strcmp(str,"reverse")) {		
		sprintf(errstr,
		"USAGE:\n"
		"distort reverse infile outfile cyclecnt\n\n"
		"CYCLE_REVERSAL DISTORTION.'WAVECYCLES' REVERSED IN GROUPS.\n\n"
		"CYCLECNT: no. of cycles (>0) in reversed groups.\n\n"
		"cyclecnt may vary in time\n\n"
		"Works on MONO files only\n");
	} else if (!strcmp(str,"shuffle")) {		
		sprintf(errstr,
		"USAGE:\n"
		"distort shuffle infile outfile domain-image [-ccylecnt] [-sskipcycles]\n\n"
		"DISTORTION BY SHUFFLING 'WAVECYCLES'\n\n"
		"DOMAIN  group of letters representing consecutive (groups of) wavecyles.\n"
		"IMAGE   group of letters which is some permutation of the domain.\n"
		"Items from domain may be reordered, ommitted or duplicated.\n"
		"  N.B. DOMAIN and IMAGE must be connected by a DASH (-).\n"
		"CYCLECNT is size of wavecycle-groups to operate on. (default 1)\n"
		"SKIPCYCLES: (integer) is no. of wavecycles to skip at start of file\n\n"
		"Cyclecnt may vary over time.\n\n"
		"Works on MONO files only\n");
	} else if (!strcmp(str,"repeat")) {		
		sprintf(errstr,
		"USAGE:\n"
		"distort repeat infile outfile multiplier [-ccyleccnt] [-sskipcycles]\n\n"
		"TIMESTRETCH FILE BY REPEATING 'WAVECYCLES'\n\n"
		"MULTIPLIER: (integer) is no. of times each wavecycle(grp) repeats.\n"
    	"CYCLECNT:   (integer) is no. wavecycles in repeated groups.\n"
		"SKIPCYCLES: (integer) is no. of wavecycles to skip at start of file\n\n"
		"multiplier and cyclecnt may vary over time.\n\n"
		"Works on MONO files only\n");
	} else if (!strcmp(str,"repeat2")) {		
		sprintf(errstr,
		"USAGE:\n"
		"distort repeat2 infile outfile multiplier [-ccyleccnt] [-sskipcycles]\n\n"
		"REPEATING 'WAVECYCLES' WITHOUT TIME STRETCHING\n\n"
		"MULTIPLIER: (integer) is no. of times each wavecycle(grp) repeats.\n"
    	"CYCLECNT:   (integer) is no. wavecycles in repeated groups.\n"
		"SKIPCYCLES: (integer) is no. of wavecycles to skip at start of file\n\n"
		"multiplier and cyclecnt may vary over time.\n\n"
		"Works on MONO files only\n");
	} else if (!strcmp(str,"replim")) {		
		sprintf(errstr,
		"USAGE:\n"
		"distort replim inf outf multiplier [-ccyleccnt] [-sskipcycles] -f[hilim]\n\n"
		"TIMESTRETCH FILE BY REPEATING 'WAVECYCLES' (BELOW SPECIFIED FRQ)\n\n"
		"MULTIPLIER: (integer) is no. of times each wavecycle(grp) repeats.\n"
    	"CYCLECNT:   (integer) is no. wavecycles in repeated groups.\n"
		"SKIPCYCLES: (integer) is no. of wavecycles to skip at start of file\n"
		"HILIM:      (float)   is frq below which cycles are counted.\n\n"
		"multiplier and cyclecnt may vary over time.\n\n"
		"Works on MONO files only\n");
	} else if (!strcmp(str,"interpolate")) {		
		sprintf(errstr,
		"USAGE:\n"
		"distort interpolate infile outfile multiplier [-sskipcycles]\n\n"
		"TIMESTRETCH FILE BY REPEATING 'WAVECYCLES' & INTERPOLATING BETWEEN THEM.\n\n"
		"MULTIPLIER: (integer) is no. of times each wavecycle repeats.\n"
		"SKIPCYCLES: (integer) is no. of wavecycles to skip at start of file\n\n"
		"multiplier and cyclecnt may vary over time.\n\n"
		"Works on MONO files only\n");
	} else if (!strcmp(str,"delete")) {		
		sprintf(errstr,
		"USAGE:\n"
		"distort delete mode infile outfile cyclecnt [-sskipcycles]\n\n"
		"TIMECONTRACT FILE BY DELETING 'WAVECYCLES'\n\n"
		"MODES:\n"
 		"1:  1 in CYCLECNT wavecycles retained\n"
		"2:  Strongest 1 in CYCLECNT wavecycles retained\n\n"
		"3:  Weakest 1 in CYCLECNT wavecycles deleted\n"
		"SKIPCYCLES: (integer) is no. of wavecycles to skip at start of file\n\n"
		"cyclecnt may vary over time.\n\n"
		"Works on MONO files only\n");
	} else if (!strcmp(str,"replace")) {		
		sprintf(errstr,
		"USAGE:\n"
		"distort replace infile outfile cyclecnt [-sskipcycles]\n\n"
		"STRONGEST 'WAVECYCLE', IN EACH cyclecnt, REPLACES OTHERS\n"
		"SKIPCYCLES: (integer) is no. of wavecycles to skip at start of file\n\n"
		"cyclecnt may vary over time.\n\n"
		"Works on MONO files only\n");
	} else if (!strcmp(str,"telescope")) {		
		sprintf(errstr,
		"USAGE:\n"							
		"distort telescope infile outfile cyclecnt [-sskipcycles] [-a]\n\n"
		"TIMECONTRACT SOUND BY TELESCOPING cyclecnt 'WAVECYCLES' TO 1\n\n"
		"SKIPCYCLES: (integer) is no. of wavecycles to skip at start of file\n"
		"-a          telescope to average cyclelen.(Default: telescope to longest.)\n\n"
		"cyclecnt may vary over time.\n\n"
		"Works on MONO files only\n");
	} else if (!strcmp(str,"filter")) {		
		sprintf(errstr,
		"USAGE:\n"
		"distort filter 1-2 infile outfile freq        [-sskipcycles]\n"
		"distort filter 3   infile outfile freq1 freq2 [-sskipcycles]\n\n"
		"TIMECONTRACT SOUND BY FILTERING OUT 'WAVECYCLES'\n\n"
		"MODES:\n"
		"1:  omit cycles below FREQ\n"
		"2:  omit cycles above FREQ\n"
		"3:  omit cycles below FREQ1 and above FREQ2\n\n"
		"skipcycles: (integer) is no. of wavecycles to skip at start of file\n\n"
		"freq, freq1 and freq2 may vary over time.\n\n"
		"N.B. timevarying freq1, freq2 may not cross each other, nor be equal.\n\n"
		"Works on MONO files only\n");
	} else if (!strcmp(str,"interact")) {		
		sprintf(errstr,
		"USAGE:\n"
		"distort interact mode infile1 infile2 outfile\n\n"
		"TIME-DOMAIN INTERACTION OF SOUNDS.\n\n"
		"MODES:\n"
		"1:  interleave wavecycles from the two infiles.\n"
		"2:  impose wavecycle-lengths of 1st file on wavecycles of 2nd\n\n"
		"Works on MONO files only\n");
	} else if (!strcmp(str,"cyclecnt")) {		
		sprintf(errstr,
		"USAGE:\n"
		"distort cyclecnt infile\n\n"
		"COUNT 'WAVECYCLES' IN SNDFILE.\n\n"
		"Works on MONO files only\n");
	} else if (!strcmp(str,"pitch")) {		
		sprintf(errstr,
 		"USAGE:\n"
		"distort pitch infile outfile octvary [-ccyclelen] [-sskipcycles]\n\n"
		"PITCHWARP 'WAVECYCLES' OF SOUND.\n\n"
     	"octvary:    max possible transposition (up or down) in octaves.\n"
     	"            Range(>0 - %.1lf)\n"
		"            pitch of each wavecycle is varied by a random amount\n"
     	"            within the range octvary 8vas up to octvary 8vas down.\n"
		"cylelen:    Max no. cycles possible between generation of transpos vals(>1)\n"
		"            (Default : %d)\n"
		"            Actual cyclecnt is a random number, less than this maximum.\n"
		"skipcycles: No of cycles to skip at start of file.\n\n"     	
		"cyclelen and octvary may vary over time.\n\n"
		"Works on MONO files only\n",MAXOCTVAR,DEFAULT_RSTEP);
//TW NEW CASES
	} else if (!strcmp(str,"overload")) {		
		sprintf(errstr,
 		"USAGE:\n"
		"distort overload 1 infile outfile clip_level depth\n"
		"OR\n"
		"distort overload 2 infile outfile gate depth freq\n\n"
		"Clip the signal with noise or a (possibly timevarying) waveform.\n\n"
     	"clip_level: Level at which the signal is clipped.\n"
     	"            Range(0 - 1)\n"
		"depth:      Depth of distortion pattern on clipped stretches of signal.\n"
		"            (Range 0 - 1)\n"
		"freq:       Frequency of waveform imposed on clipped stretches of signal.\n\n"     	
		"clip_level, depth and freq may vary over time.\n\n"
		"Works on MONO files only\n");
	} else if (!strcmp(str,"pulsed")) {		
		sprintf(errstr,
 		"USAGE:\n"
		"distort pulsed\n"
		"1 infil outfil env stime dur frq frand trand arand transp tranrand [-s -e]\n"
		"OR\n"
		"2-3 inf outf env stime dur frq frand trand arand cyctime transp tranrand [-s -e]\n"
		"    Impose impulse-train on source (mode 1), OR\n"
		"    Use segment of src as looped content of synthetic impulse-train(modes 2,3).\n"
		"ENV        brkpnt envelope of impulse. Will be scaled to duration needed.\n"
		"STIME      time in src sound where impulses begin.(In mode 3,time as samplecnt)\n"
		"DUR        time for which impulses persist.\n"
		"FRQ        frequency of the impulses, in Hz: range 0.1 to 50\n"
		"FRAND      randomisation of frequency of impulse (in semitones) (range 0-12).\n"
		"TRAND      randomisation of relative time-positions of amp peaks & troughs\n"
		"           from impulse to impulse. (range 0 - 1)\n"
		"ARAND      randomisation of amplitude shape created by peaks & troughs\n"
		"           from impulse to impulse. (range 0 - 1)\n"
		"CYCTIME    duration of wavecycles to grab for sound inside impulses (mode 2)\n"
		"           OR number of wavecycles to grab as sound inside impulses (mode 3)\n"
		"TRANSP     transposition contour of sound inside each impulse. Brkpnt file of\n"
		"           time:semitone-shift pairs,(time will be scaled to impulse dur),\n"
		"TRANRAND   randomisation transp contour from impulse to impulse.(Range 0-1)\n"
		"-s         keep start of src sound, before impulses begin (if any).\n"
		"-e         keep end of src sound, after impulses end (if any)\n"
		"Works on MONO files only\n");
	} else
		sprintf(errstr,"Unknown option '%s'\n",str);
	return(USAGE_ONLY);
}

/******************************** USAGE3 ********************************/

int usage3(char *str1,char *str2)
{
	if(!strcmp(str1,"cyclecnt"))	
		return(CONTINUE);
	else
		sprintf(errstr,"Insufficient parameters on command line.\n");
	return(USAGE_ONLY);
}

/******************************** INNER_LOOP (redundant)  ********************************/

int inner_loop
(int *peakscore,int *descnt,int *in_start_portion,int *least,int *pitchcnt,int windows_in_buf,dataptr dz)
{
	return(FINISHED);
}
