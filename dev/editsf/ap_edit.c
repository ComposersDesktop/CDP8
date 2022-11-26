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



/* floatsams version */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <structures.h>
#include <cdpmain.h>
#include <tkglobals.h>
#include <pnames.h>
#include <edit.h>
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
#include <ctype.h>

#include <srates.h>

#ifdef unix
#define round(x) lround((x))
#endif

int randcuts_pconsistency(dataptr dz);
int syllabs_pconsistency(dataptr dz);
int get_syllab_times(char *filename,dataptr dz);
int get_joinseq(char *filename,dataptr dz);
int get_joindynseq(char *filename,dataptr dz);

/********************************************************************************************/
/********************************** FORMERLY IN specialin.c *********************************/
/********************************************************************************************/

static int  get_excise_times_from_file(char *filename,dataptr dz);
static int get_many_cuts_times(char *filename,dataptr dz);
static int preprocess_cutmany(dataptr dz);
static int create_cutmany_sndbuf(dataptr dz);


/***************************************************************************************/
/****************************** FORMERLY IN aplinit.c **********************************/
/***************************************************************************************/

/***************************** ESTABLISH_BUFPTRS_AND_EXTRA_BUFFERS **************************/

int establish_bufptrs_and_extra_buffers(dataptr dz)
{
	dz->extra_bufcnt = -1;	/* ENSURE EVERY CASE HAS A PAIR OF ENTRIES !! */
	dz->bptrcnt = 0;
	dz->bufcnt  = 0;
	switch(dz->process) {
	case(EDIT_CUT):
	case(EDIT_CUTEND):
	case(INSERTSIL_MANY):		dz->extra_bufcnt = 0;	dz->bufcnt = 0;		break;

	case(MANY_ZCUTS):
	case(EDIT_ZCUT):			dz->extra_bufcnt = 0;	dz->bufcnt = 2;		break;

	case(EDIT_EXCISE):			
	case(EDIT_EXCISEMANY):		dz->extra_bufcnt = 0;	dz->bufcnt = 4;		break;

	case(EDIT_INSERT):
	case(EDIT_INSERT2):
	case(EDIT_INSERTSIL):		dz->extra_bufcnt = 0;	dz->bufcnt = 3;		break;
	case(JOIN_SEQDYN):
	case(JOIN_SEQ):				dz->extra_bufcnt = 0;	dz->bufcnt = 5;		break;
	case(EDIT_JOIN):			dz->extra_bufcnt = 0;	dz->bufcnt = 4;		break;
	case(RANDCUTS):				dz->extra_bufcnt = 0;	dz->bufcnt = 2;		break;
	case(RANDCHUNKS):			dz->extra_bufcnt = 0;	dz->bufcnt = 0;		break;
	case(TWIXT):				dz->extra_bufcnt = 0;	dz->bufcnt = 2;		break;
	case(SPHINX):				dz->extra_bufcnt = 0;	dz->bufcnt = 2;		break;
	case(SYLLABS):
	case(EDIT_CUTMANY):			dz->extra_bufcnt = 0;	dz->bufcnt = 1;		break;
	case(NOISE_SUPRESS):		dz->extra_bufcnt = 0;	dz->bufcnt = 1;		break;
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
	case(EDIT_CUT):		   dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt=0; dz->ptr_cnt= 0; dz->fptr_cnt = 0; break;
	case(EDIT_CUTEND):	   dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt=0; dz->ptr_cnt= 0; dz->fptr_cnt = 0; break;
	case(MANY_ZCUTS):	   dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt=1; dz->ptr_cnt= 0; dz->fptr_cnt = 0; break;
	case(EDIT_ZCUT):	   dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt=0; dz->ptr_cnt= 0; dz->fptr_cnt = 0; break;
	case(EDIT_EXCISE):	   dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt=4; dz->ptr_cnt= 0; dz->fptr_cnt = 0; break;
	case(EDIT_EXCISEMANY): dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt=4; dz->ptr_cnt= 0; dz->fptr_cnt = 0; break;
	case(EDIT_INSERT2):
	case(EDIT_INSERT):	   dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt=0; dz->ptr_cnt= 0; dz->fptr_cnt = 0; break;
	case(EDIT_INSERTSIL):  dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt=0; dz->ptr_cnt= 0; dz->fptr_cnt = 0; break;
	case(INSERTSIL_MANY):  dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt=4; dz->ptr_cnt= 0; dz->fptr_cnt = 0; break;
	case(JOIN_SEQDYN):	   dz->array_cnt=3; dz->iarray_cnt=1; dz->larray_cnt=0; dz->ptr_cnt= 0; dz->fptr_cnt = 0; break;
	case(JOIN_SEQ):		   dz->array_cnt=2; dz->iarray_cnt=1; dz->larray_cnt=0; dz->ptr_cnt= 0; dz->fptr_cnt = 0; break;
	case(EDIT_JOIN):  	   dz->array_cnt=2; dz->iarray_cnt=0; dz->larray_cnt=0; dz->ptr_cnt= 0; dz->fptr_cnt = 0; break;
	case(RANDCUTS):  	   dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt=1; dz->ptr_cnt= 0; dz->fptr_cnt = 0; break;
	case(RANDCHUNKS):  	   dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt=0; dz->ptr_cnt= 0; dz->fptr_cnt = 0; break;
	case(TWIXT):  	   	   dz->array_cnt=1; dz->iarray_cnt=2; dz->larray_cnt=0; dz->ptr_cnt= 0; dz->fptr_cnt = 0; break;
	case(SPHINX):  	   	   dz->array_cnt=1; dz->iarray_cnt=2; dz->larray_cnt=0; dz->ptr_cnt= 0; dz->fptr_cnt = 0; break;
	case(NOISE_SUPRESS):  dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt=1; dz->ptr_cnt= 0; dz->fptr_cnt = 0; break;
	case(SYLLABS):
	case(EDIT_CUTMANY):	   dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt=1; dz->ptr_cnt= 0; dz->fptr_cnt = 0; break;
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
	case(EDIT_CUT):			setup_process_logic(SNDFILES_ONLY,		UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(EDIT_CUTEND):		setup_process_logic(SNDFILES_ONLY,		UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(MANY_ZCUTS):
	case(EDIT_ZCUT):		setup_process_logic(SNDFILES_ONLY,		UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(EDIT_EXCISE):		setup_process_logic(SNDFILES_ONLY,		UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(EDIT_EXCISEMANY):	setup_process_logic(SNDFILES_ONLY,		UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(EDIT_INSERT2):
	case(EDIT_INSERT):		setup_process_logic(TWO_SNDFILES,		UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(EDIT_INSERTSIL):	setup_process_logic(SNDFILES_ONLY,		UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(INSERTSIL_MANY):	setup_process_logic(SNDFILES_ONLY,		EQUAL_SNDFILE,		SNDFILE_OUT,	dz);	break;
	case(JOIN_SEQDYN):
	case(JOIN_SEQ):
	case(EDIT_JOIN):		setup_process_logic(MANY_SNDFILES,		UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(RANDCUTS):			setup_process_logic(SNDFILES_ONLY,		OTHER_PROCESS,		NO_OUTPUTFILE,	dz);	break;
	case(RANDCHUNKS):		setup_process_logic(SNDFILES_ONLY,		OTHER_PROCESS,		NO_OUTPUTFILE,	dz);	break;
	case(TWIXT):			setup_process_logic(MANY_SNDFILES,		UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(SPHINX):			setup_process_logic(MANY_SNDFILES,		UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(NOISE_SUPRESS):	setup_process_logic(SNDFILES_ONLY,		UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(SYLLABS):
	case(EDIT_CUTMANY):		setup_process_logic(SNDFILES_ONLY,		UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;

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
	case(EDIT_CUT):			exit_status = set_internalparam_data( "0iiiiiiiiiiiiiiiiiiiiiii",ap); break;
	case(EDIT_CUTEND):	    exit_status = set_internalparam_data( "0iiiiiiiiiiiiiiiiiiiiiii",ap); break;
	case(MANY_ZCUTS):	    exit_status=set_internalparam_data("ii00iiiiiiiiiiiiiiiiiiiiiii",ap); break;
	case(EDIT_ZCUT):	    exit_status = set_internalparam_data("00iiiiiiiiiiiiiiiiiiiiiii",ap); break;
	case(EDIT_EXCISE):	    exit_status = set_internalparam_data( "0iiiiiiiiiiiiiiiiiiiiiii",ap); break;
	case(EDIT_EXCISEMANY):  exit_status = set_internalparam_data( "0iiiiiiiiiiiiiiiiiiiiiii",ap); break;
	case(INSERTSIL_MANY):   exit_status = set_internalparam_data( "0iiiiiiiiiiiiiiiiiiiiiii",ap); break;
	case(EDIT_INSERT2):
	case(EDIT_INSERT):	    exit_status = set_internalparam_data(  "iiiiiiiiiiiiiiiiiiiiiii",ap); break;
	case(EDIT_INSERTSIL):   exit_status = set_internalparam_data( "0iiiiiiiiiiiiiiiiiiiiiii",ap); break;
	case(JOIN_SEQDYN):
	case(JOIN_SEQ):
	case(EDIT_JOIN):   		exit_status = set_internalparam_data( "0iiiiiiiiiiiiiiiiiiiiiii",ap); break;
	case(RANDCUTS):   		exit_status = set_internalparam_data( "iiiiii",ap); break;
	case(RANDCHUNKS):   	exit_status = set_internalparam_data( "0iiiiiiiiiiiiiiiiiiiiiii",ap); break;
	case(TWIXT):
		switch(mode) {
		case(TRUE_EDIT):	exit_status = set_internalparam_data( "0di",ap); break;
		default:   			exit_status = set_internalparam_data(  "di",ap); break;
		}
		break;
	case(SPHINX):			exit_status = set_internalparam_data(  "di",ap); break;
	case(NOISE_SUPRESS):	exit_status = set_internalparam_data(  "",ap); break;
	case(SYLLABS):		/* this sets up too many internal params in this case, but doesn't matter */
	case(EDIT_CUTMANY):		exit_status = set_internalparam_data(  "iid",ap); break;

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
	case(EXCISE_TIMES):				return get_excise_times_from_file(str,dz);
	case(SWITCH_TIMES):				return get_switchtime_data(str,dz);
	case(MANY_SWITCH_TIMES):		return get_multi_switchtime_data(str,dz);
	case(MANYCUTS):					return get_many_cuts_times(str,dz);
	case(SYLLTIMES):				return get_syllab_times(str,dz);
	case(JOINSEQ):					return get_joinseq(str,dz);
	case(JOINSEQDYN):				return get_joindynseq(str,dz);
	default:
		sprintf(errstr,"Unknown special_data type: read_special_data()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);	/* NOTREACHED */
}

/************************* GET_EXCISE_TIMES_FROM_FILE *************************/ 

int get_excise_times_from_file(char *filename,dataptr dz)
{
	int is_start = 1, k = 0;
	char *q, *p, temp[200];
	double dummy;
	int chans = dz->infile->channels; 
	double sr = (double)dz->infile->srate;
	FILE *fp;
	if((fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,"Cannot open textfile %s\n",filename);
		return(DATA_ERROR);
	}
	dz->iparam[CUT_NO_END] = FALSE;
	if((dz->lparray[CUT_STTSAMP] = (int *)malloc(sizeof(int)))==NULL
	|| (dz->lparray[CUT_STTSPLI] = (int *)malloc(sizeof(int)))==NULL
	|| (dz->lparray[CUT_ENDSAMP] = (int *)malloc(sizeof(int)))==NULL
	|| (dz->lparray[CUT_ENDSPLI] = (int *)malloc(sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT_MEMORY to store excise times.\n");
		return(MEMORY_ERROR);
	}
	while(fgets(temp,200,fp)!=NULL) {
		p = temp;
		while(get_word_from_string(&p,&q)) {
			if(dz->iparam[CUT_NO_END] == TRUE) {
				sprintf(errstr,"End of excise %d is beyond end of file: cannot proceed.\n",k);
				return(DATA_ERROR);
			}
			if(is_start) {
				if(k > 0) {
					if((dz->lparray[CUT_STTSAMP] = 
					(int *)realloc((char *)(dz->lparray[CUT_STTSAMP]),(k+1) * sizeof(int)))==NULL
					|| (dz->lparray[CUT_STTSPLI] = 
					(int *)realloc((char *)(dz->lparray[CUT_STTSPLI]),(k+1) * sizeof(int)))==NULL
					|| (dz->lparray[CUT_ENDSAMP] = 
					(int *)realloc((char *)(dz->lparray[CUT_ENDSAMP]),(k+1) * sizeof(int)))==NULL
					|| (dz->lparray[CUT_ENDSPLI] = 
					(int *)realloc((char *)(dz->lparray[CUT_ENDSPLI]),(k+1) * sizeof(int)))==NULL) {
						sprintf(errstr,"INSUFFICIENT_MEMORY to store more excise times.\n");
						return(MEMORY_ERROR);
					}
				}
				switch(dz->mode) {
				case(EDIT_SAMPS):
	   				if(sscanf(q,"%d",&(dz->lparray[CUT_STTSAMP][k]))!=1) {
						sprintf(errstr,"Cannot read samplecnt %d\n",k+1);
						return(DATA_ERROR);
	   				}
					break;
				case(EDIT_STSAMPS):
	   				if(sscanf(q,"%d",&(dz->lparray[CUT_STTSAMP][k]))!=1) {
						sprintf(errstr,"Cannot read samplecnt %d\n",k+1);
						return(DATA_ERROR);
	   				}
					dz->lparray[CUT_STTSAMP][k] *= chans;
					break;
				case(EDIT_SECS):
	   				if(sscanf(q,"%lf",&dummy)!=1) {
						sprintf(errstr,"Cannot read starttime %d\n",k+1);
						return(DATA_ERROR);
					}
					dz->lparray[CUT_STTSAMP][k] = round(dummy * sr) * chans;
					break;
				}
			    if(dz->lparray[CUT_STTSAMP][k] < 0 || dz->lparray[CUT_STTSAMP][k] >= dz->insams[0]) {
					sprintf(errstr,"excise starttime %d is outside filelen.\n",k+1);
					return(DATA_ERROR);
			    }
				if(k>0 && dz->lparray[CUT_STTSAMP][k] <= dz->lparray[CUT_ENDSAMP][k-1]) {
					sprintf(errstr,"Start and end of excised portions %d & %d overlap.\n",k,k+1);
					return(DATA_ERROR);
				}							
			} else {
				switch(dz->mode) {
				case(EDIT_SAMPS):
		    		if(sscanf(q,"%d",&(dz->lparray[CUT_ENDSAMP][k]))!=1) {
						sprintf(errstr,"Cannot read end samplecnt %d\n",k+1);
						return(DATA_ERROR);
		    		}
					break;
				case(EDIT_STSAMPS):
		    		if(sscanf(q,"%d",&(dz->lparray[CUT_ENDSAMP][k]))!=1) {
						sprintf(errstr,"Cannot read end samplecnt %d\n",k+1);
						return(DATA_ERROR);
		    		}
					dz->lparray[CUT_ENDSAMP][k] *= chans;
					break;
				case(EDIT_SECS):
		    		if(sscanf(q,"%lf",&dummy)!=1) {
						sprintf(errstr,"Cannot read endtime %d\n",k+1);
						return(DATA_ERROR);
		    		}
					dz->lparray[CUT_ENDSAMP][k] = round(dummy * sr) * chans;
					break;
				}
			    if(dz->lparray[CUT_STTSAMP][k] >= dz->lparray[CUT_ENDSAMP][k]) {
					sprintf(errstr,"starttime <= endtime for excise %d\n",k+1);
					return(DATA_ERROR);
		    	}
				if(dz->lparray[CUT_ENDSAMP][k] >= dz->insams[0]) {
					dz->iparam[CUT_NO_END] = TRUE;
				}
			}
			is_start = !is_start;
			if(is_start)
				k++;
		}
	}
	if(!is_start) {
		sprintf(errstr,"Start and End times not paired correctly in textfile %s\n",filename);
		return(DATA_ERROR);
	}
	if(k==0) {
		sprintf(errstr,"No values found in textfile %s\n",filename);
		return(DATA_ERROR);
	}
	dz->iparam[EXCISE_CNT] = k;
	return(FINISHED);
}

/**************************** GET_SWITCHTIME_DATA ****************************/

int get_switchtime_data(char *filename,dataptr dz)
{
	FILE *fp;
	int n = 0, curtailed = 0;
	int arraysize = BIGARRAY;
	char temp[2000], *p;
	double lasttime = 0.0, dummy;
	double duration1 = (double)dz->insams[0]/(double)(dz->infile->channels * dz->infile->srate);
	double duration2 = (double)dz->insams[1]/(double)(dz->infile->channels * dz->infile->srate);
	double mindur = min(duration1,duration2);
	if((fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,"Cannot open file %s to read time-switch data.\n",filename);
		return(DATA_ERROR);
	}
	if((dz->parray[0] = (double *)malloc(arraysize * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store time-switch data.\n");
		return(MEMORY_ERROR);
	}

	while(fgets(temp,2000,fp)!=NULL) {
		p = temp;
		if(*p == ';')
			continue;
		while(get_float_from_within_string(&p,&dummy)) {
			if(n==0)
				lasttime = dummy;
			else if(dummy <= lasttime) {
				sprintf(errstr,"Times %d (%lf) and %d (%lf) are not in ascending order\n",n,lasttime,n+1,dummy);
				return(DATA_ERROR);
			}
			if(dummy < 0.0) {
				sprintf(errstr,"Times %lf does not lie within the file (duration %lf).\n",dummy,dz->duration);
				return(DATA_ERROR);
			} else if(dummy > mindur) {
				fprintf(stdout,"WARNING: Time %lf does not lie within both files (smallest duration %lf).\n",dummy,mindur);
				fprintf(stdout,"WARNING: Curtailing output.\n");
				fflush(stdout);
				curtailed = 1;
				break;
			}
			dz->parray[0][n] = dummy;
			if(++n >= arraysize) {
				arraysize += BIGARRAY;
				if((dz->parray[0] = (double *)realloc((char *)dz->parray[0],arraysize*sizeof(double)))==NULL) {
					sprintf(errstr,"INSUFFICIENT MEMORY to reallocate switch-times store.\n");
					return(MEMORY_ERROR);
				}
			}
		}
		if(curtailed)
			break;
	}
	if(n==0) {
		sprintf(errstr,"No valid data found in switch-times data file %s\n",filename);
		return(DATA_ERROR);
	}
	if(n < 2) {
		sprintf(errstr,"Insufficient valid data (only 1 value) found in switch-times data file %s\n",filename);
		return(DATA_ERROR);
	}
	if((dz->parray[0] = (double *)realloc((char *)dz->parray[0],n * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to reallocate switch-times store.\n");
		return(MEMORY_ERROR);
	}
	if(dz->mode == RAND_REORDER) {
		if((dz->iparray[0] = (int *)malloc((n-1) * sizeof(int)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to allocate switch-times permutation store.\n");
			return(MEMORY_ERROR);
		}
	}
	dz->itemcnt = n;
	if(fclose(fp)<0) {
		fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
		fflush(stdout);
	}
	return(FINISHED);
}
    
/**************************** GET_MULTI_SWITCHTIME_DATA ****************************/

int get_multi_switchtime_data(char *filename,dataptr dz)
{
	FILE *fp;
	int m, n = 0, curtailed = 0;
	int arraysize = BIGARRAY;
	char temp[2000], *p;
	double *lasttime, *thistime, *duration, dummy;

	if((fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,"Cannot open file %s to read time-switch data.\n",filename);
		return(DATA_ERROR);
	}
	if((dz->parray = (double **)realloc((char *)dz->parray,dz->infilecnt * sizeof(double *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store time-switch data.\n");
		return(MEMORY_ERROR);
	}
	if((duration = (double *)malloc(dz->infilecnt * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store file durations.\n");
		return(MEMORY_ERROR);
	}
	for(m=0; m<dz->infilecnt;m++) {
		duration[m] = (double)(dz->insams[m]/dz->infile->channels)/(double)(dz->infile->srate);
		if((dz->parray[m] = (double *)malloc(arraysize * sizeof(double)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to store time-switch data.\n");
			return(MEMORY_ERROR);
		}
	}
	if((lasttime = (double *)malloc(dz->infilecnt * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store time-check data.\n");
		return(MEMORY_ERROR);
	}
	if((thistime = (double *)malloc(dz->infilecnt * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store time data.\n");
		return(MEMORY_ERROR);
	}
	while(fgets(temp,2000,fp)!=NULL) {
		p = temp;
		if(*p == ';')
			continue;
		m = 0;
		if(n==0) {
			while(get_float_from_within_string(&p,&dummy)) {
				if(m >= dz->infilecnt) {
					sprintf(errstr,"Too many entries found in row %d.\n",n+1);
					return(DATA_ERROR);
				}
				if(dummy < 0.0 || dummy > duration[m]) {
					sprintf(errstr,"First time %lf for file %d does not lie within the file (duration %lf).\n",dummy,m+1,duration[m]);
					return(DATA_ERROR);
				}
				lasttime[m] = dummy;
				dz->parray[m][n] = lasttime[m];
				m++;
			}				
			p = temp;
			if(m==0)	 	/* ignore blank lines */
				continue;
			if(m!=dz->infilecnt) {
				sprintf(errstr,"Number of data-items per line (%d) does not tally with number of infiles (%d).\n",
				m,dz->infilecnt);
				return(DATA_ERROR);
			}
		} else {
			while(get_float_from_within_string(&p,&dummy)) {
				if(m >= dz->infilecnt) {
					sprintf(errstr,"Too many entries found in row %d.\n",n+1);
					return(DATA_ERROR);
				}
				thistime[m++] = dummy;
			}
			if(m==0)	 	/* ignore blank lines */
				continue;
			if(m != dz->infilecnt) {
				sprintf(errstr,"Too few entries found in row %d.\n",n+1);
				return(DATA_ERROR);
			}
			for(m = 0;m < dz->infilecnt; m++) {
				if(thistime[m] <= lasttime[m]) {
					sprintf(errstr,"Times %d (%lf) and %d (%lf) are not in ascending order in column %d\n",
					n,lasttime[m],n+1,thistime[m],m+1);
					return(DATA_ERROR);
				}
				if(thistime[m] < 0.0 || thistime[m] > duration[m]) {
					fprintf(stdout,"WARNING: Time %lf for file %d does not lie within the file (duration %lf).\n",
					thistime[m],m+1,duration[m]);
					fprintf(stdout,"WARNING: Curtailing output.\n");
					fflush(stdout);
					curtailed = 1;
					break;
				}
				dz->parray[m][n] = thistime[m];
			}
		}
		if(curtailed)
			break;
		if(++n >= arraysize) {
			arraysize += BIGARRAY;
			for(m=0; m<dz->infilecnt;m++) {
				if((dz->parray[m] = (double *)realloc((char *)dz->parray[m],arraysize*sizeof(double)))==NULL) {
					sprintf(errstr,"INSUFFICIENT MEMORY to reallocate switch-times stores.\n");
					return(MEMORY_ERROR);
				}
			}
		}
	}
	if(n==0) {
		sprintf(errstr,"No valid data found in switch-times data file %s\n",filename);
		return(DATA_ERROR);
	}
	if(n < 2) {
		sprintf(errstr,"Insufficient valid data (only 1 value) found in switch-times data file %s\n",filename);
		return(DATA_ERROR);
	}
	for(m=0; m<dz->infilecnt;m++) {
		if((dz->parray[m] = (double *)realloc((char *)dz->parray[m],n * sizeof(double)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to reallocate switch-times stores.\n");
			return(MEMORY_ERROR);
		}
	}
	if(dz->mode == RAND_REORDER) {
		if((dz->iparray[0] = (int *)malloc((n-1) * sizeof(int)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to allocate switch-times permutation store.\n");
			return(MEMORY_ERROR);
		}
	}
	dz->itemcnt = n;
	if(fclose(fp)<0) {
		fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
		fflush(stdout);
	}
	free(thistime);
	free(lasttime);
	free(duration);
	return(FINISHED);
}
    
/**************************** PREPROCESS_CUTMANY ****************************/

int preprocess_cutmany(dataptr dz)
{
	int n;
	dz->param[CM_SPLICEINCR]   = 1.0/(double)dz->iparam[CM_SPLICESAMPS];

	for(n=0; n < dz->itemcnt; n+=2) {
		if(dz->lparray[0][n+1] - dz->lparray[0][n] <= dz->iparam[CM_SPLICE_TOTSAMPS] * 2) {
			sprintf(errstr,"Time pair %d is not possible with soundfile and splice length being used.\n",
			(n+2)/2);
			return(GOAL_FAILED);
		}
	}
	return(FINISHED);
}

/**************************** CREATE_CUTMANY_SNDBUF ****************************/

int create_cutmany_sndbuf(dataptr dz)
{
	int splicespace, splicesects;
    size_t bigbufsize;
	int k, framesize;
	framesize = dz->infile->channels * F_SECSIZE;

	dz->iparam[CM_SPLICESAMPS] = (int)round(dz->param[CM_SPLICELEN] * MS_TO_SECS * dz->infile->srate);
	dz->param[CM_SPLICEINCR]   = 1.0/(double)dz->iparam[CM_SPLICESAMPS];
	dz->iparam[CM_SPLICE_TOTSAMPS] = dz->iparam[CM_SPLICESAMPS] * dz->infile->channels; 

	splicespace = (dz->iparam[CM_SPLICE_TOTSAMPS] * 2);
	splicesects = splicespace/framesize;
	if(splicespace > splicesects * framesize)
		splicesects++;
	splicespace = splicesects * framesize;

	bigbufsize = (size_t)Malloc(-1);
	dz->buflen = (int)(bigbufsize/sizeof(float));
	k = dz->buflen % framesize;					/* truncate to multiple of framesize */
	dz->buflen -= k;
	if(dz->buflen <= 0)
		dz->buflen = framesize;
	dz->buflen = max(dz->buflen,splicespace);
	bigbufsize = dz->buflen * sizeof(float);
	if((dz->bigbuf	= (float *)malloc((dz->buflen * sizeof(float)) + F_SECSIZE))==NULL) {  
		sprintf(errstr,"INSUFFICIENT MEMORY for sound buffer.\n");
		return(MEMORY_ERROR);
	}
	return(FINISHED);
}

/**************************** GET_MANY_CUTS_TIMES ****************************/

int get_many_cuts_times(char *filename,dataptr dz)
{
	FILE *fp;
	int n = 0, OK;
	int arraysize = BIGARRAY, /* byteval */sampval = 0;
	char temp[200], *p;
	double  dummy;

	if((fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,"Cannot open file %s to read cut-times data.\n",filename);
		return(DATA_ERROR);
	}
	if((dz->lparray[0] = (int *)malloc(arraysize * sizeof(int *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store time-switch data.\n");
		return(MEMORY_ERROR);
	}
	OK = 1;
	while(fgets(temp,200,fp)!=NULL) {
		p = temp;
		while(isspace(*p))
			p++;
		if(*p == ';')
			continue;
		while(get_float_from_within_string(&p,&dummy)) {
			if(dummy < 0.0) {
				sprintf(errstr,"Cut-segment time before zero in file %s\n",filename);
				return(DATA_ERROR);
			}
			switch(dz->mode) {
			case(EDIT_SECS): 
				sampval = (int)round(dummy * dz->infile->srate);
				sampval *= dz->infile->channels;
				break;
			case(EDIT_SAMPS): 
				sampval = (int)round(dummy);
				break;
			case(EDIT_STSAMPS): 
				sampval = (int)round(dummy) * dz->infile->channels;
				break;
			}
			if(ODD(n)) {
				if(sampval <= dz->lparray[0][n-1]) {
					sprintf(errstr,"Time pair %d in file %s is not in the correct order.\n",(n+1)/2,filename);
					return(DATA_ERROR);
				}
				dz->lparray[0][n] = min(sampval,dz->insams[0]);
			} else {
				if(sampval >= dz->insams[0]) {
					fprintf(stdout,"WARNING: Some cut points are beyond the end of the input file.\n");
					fflush(stdout);
					OK = 0;
					break;
				}
				dz->lparray[0][n] = sampval;
			}
			if(++n >= arraysize) {
				arraysize += BIGARRAY;
				if((dz->lparray[0] = (int *)realloc((char *)dz->lparray[0],arraysize*sizeof(int)))==NULL) {
					sprintf(errstr,"INSUFFICIENT MEMORY to reallocate cut-times store.\n");
					return(MEMORY_ERROR);
				}
			}
		}
		if(!OK)
			break;
	}
	if(n<=0) {
		sprintf(errstr,"No data found in data file %s\n",filename);
		return(DATA_ERROR);
	}
	if(ODD(n)) {
		sprintf(errstr,"cut times incorrectly paired in data file %s\n",filename);
		return(DATA_ERROR);
	}
	if((dz->lparray[0] = (int *)realloc((char *)dz->lparray[0],n * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to reallocate cut-times stores.\n");
		return(MEMORY_ERROR);
	}
	dz->itemcnt = n;
	if(fclose(fp)<0) {
		fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
		fflush(stdout);
	}
	dz->tempsize = dz->itemcnt/2;
	return(FINISHED);
}

/********************************************************************************************/
/********************************** FORMERLY IN preprocess.c ********************************/
/********************************************************************************************/


/****************************** PARAM_PREPROCESS *********************************/

int param_preprocess(dataptr dz)	
{
	switch(dz->process) {
	case(EDIT_CUT):
	case(EDIT_CUTEND):
	case(EDIT_INSERT):
	case(EDIT_INSERT2):
	case(EDIT_INSERTSIL):
		return edit_preprocess(dz);
	case(MANY_ZCUTS):
	case(EDIT_ZCUT):
	case(EDIT_EXCISE):
	case(EDIT_EXCISEMANY):
	case(JOIN_SEQ):
	case(JOIN_SEQDYN):
	case(EDIT_JOIN):
	case(INSERTSIL_MANY):
	case(RANDCUTS):
	case(RANDCHUNKS):
	case(TWIXT):
	case(SPHINX):
		break;
	case(EDIT_CUTMANY):
		return preprocess_cutmany(dz);
	case(NOISE_SUPRESS):
		dz->iparam[NOISE_SPLEN] = (int)round(dz->param[NOISE_SPLEN] * MS_TO_SECS * (double)dz->infile->srate);
		dz->iparam[NOISE_MINFRQ] = (int)round((double)dz->infile->srate/dz->param[NOISE_MINFRQ]);
		dz->iparam[MIN_NOISLEN] = (int)round(dz->param[MIN_NOISLEN] * MS_TO_SECS * (double)dz->infile->srate);
		dz->iparam[MIN_TONELEN] = (int)round(dz->param[MIN_TONELEN] * MS_TO_SECS * (double)dz->infile->srate);
		if(dz->iparam[NOISE_SPLEN] >= dz->buflen) {
			sprintf(errstr,"Splices are too long for the internal sound buffers.\n");
			return(GOAL_FAILED);
		}
		break;

	default:
		sprintf(errstr,"PROGRAMMING PROBLEM: Unknown process in param_preprocess()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/********************************************************************************************/
/********************************** FORMERLY IN procgrou.c **********************************/
/********************************************************************************************/

/**************************** GROUCHO_PROCESS_FILE ****************************/

int groucho_process_file(dataptr dz)   /* FUNCTIONS FOUND IN PROCESS.C */
{	
	switch(dz->process) {
	case(EDIT_CUT):
	case(EDIT_CUTEND):		
		return do_cut(dz);
	case(MANY_ZCUTS):		
		return do_many_zcuts(dz);
	case(EDIT_ZCUT):		
		return do_zcut(dz);
	case(EDIT_EXCISE):
	case(EDIT_EXCISEMANY):	
		return do_excise(dz);
	case(EDIT_INSERT):
	case(EDIT_INSERT2):
	case(EDIT_INSERTSIL):	
		return do_insert(dz);
	case(INSERTSIL_MANY):	
		return do_insertsil_many(dz);
	case(EDIT_JOIN):		
		return do_joining(dz);
	case(JOIN_SEQ):		
	case(JOIN_SEQDYN):		
		return do_patterned_joining(dz);
	case(RANDCUTS):		
		return do_randcuts(dz);
	case(RANDCHUNKS):		
		return do_randchunks(dz);
	case(TWIXT):		
	case(SPHINX):		
		return do_twixt(dz);
	case(EDIT_CUTMANY):
		return cut_many(dz);
	case(NOISE_SUPRESS):		
		return do_noise_suppression(dz);

	default:
		sprintf(errstr,"Unknown case in process_file()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);	/* NOTREACHED */
}

/********************************************************************************************/
/********************************** FORMERLY IN pconsistency.c ******************************/
/********************************************************************************************/

/****************************** CHECK_PARAM_VALIDITY_AND_CONSISTENCY ****************************/

int check_param_validity_and_consistency(dataptr dz)
{
	handle_pitch_zeros(dz);
	switch(dz->process) {
	case(EDIT_CUT):
	case(EDIT_CUTEND):
	case(EDIT_ZCUT):
	case(EDIT_EXCISE):
	case(EDIT_EXCISEMANY):
	case(EDIT_INSERT):
	case(EDIT_INSERT2):
	case(EDIT_INSERTSIL):
	case(INSERTSIL_MANY):
	case(EDIT_JOIN):
	case(JOIN_SEQ):	
	case(JOIN_SEQDYN):
		return edit_pconsistency(dz);
	case(RANDCUTS):
		return randcuts_pconsistency(dz);
	case(SYLLABS):
		return syllabs_pconsistency(dz);
	case(RANDCHUNKS):
	case(NOISE_SUPRESS):
	case(EDIT_CUTMANY):
	case(MANY_ZCUTS):
		break;
	case(TWIXT):
		return twixt_preprocess(dz);
	case(SPHINX):
		return sphinx_preprocess(dz);
	}
	return(FINISHED);
}

/**************************** RANDCUTS_PCONSISTENCY ****************************/

int randcuts_pconsistency(dataptr dz)
{
	int chans = dz->infile->channels;
	int excess, endunit_len;
	double duration = (double)(dz->insams[0]/chans)/(double)dz->infile->srate;
    initrand48();
	dz->iparam[RC_CHCNT] = round(duration/dz->param[RC_CHLEN]);
	if(dz->param[RC_SCAT] > (double)dz->iparam[RC_CHCNT]) {
		sprintf(errstr,"Scatter value cannot be greater than infileduration/chunklength.\n");
		return(DATA_ERROR);
	}    
	dz->iparam[RC_UNITLEN] = (int)round(dz->insams[0]/dz->iparam[RC_CHCNT]);
/* 2009 MULTICHANNEL --> */
	excess = dz->iparam[RC_UNITLEN] % chans;
	dz->iparam[RC_UNITLEN] -= excess;
/* <-- 2009 MULTICHANNEL */
/* OLD
	if(ODD(dz->iparam[RC_UNITLEN]))   
		dz->iparam[RC_UNITLEN]--;
*/
	excess = dz->insams[0] - (dz->iparam[RC_UNITLEN] * dz->iparam[RC_CHCNT]);
	endunit_len = dz->iparam[RC_UNITLEN] + excess;
	if((dz->lparray[0] = (int *)malloc(dz->iparam[RC_CHCNT] * sizeof(int))) ==NULL) {
		sprintf(errstr,"Insufficient space to store block sizes\n");
		return(MEMORY_ERROR);
	}
	if(dz->param[RC_SCAT] > 1.0) {
		dz->iparam[RC_SCAT] = round(dz->param[RC_SCAT]);
		dz->iparam[RC_SCATGRPCNT] = (int)(dz->iparam[RC_CHCNT]/dz->iparam[RC_SCAT]);
		dz->iparam[RC_ENDSCAT]    = (int)(dz->iparam[RC_CHCNT] - (dz->iparam[RC_SCATGRPCNT] * dz->iparam[RC_SCAT]));
		dz->iparam[RC_RANGE]      = dz->iparam[RC_UNITLEN] * dz->iparam[RC_SCAT];
		dz->iparam[RC_ENDRANGE]   = ((dz->iparam[RC_ENDSCAT]-1) * dz->iparam[RC_UNITLEN]) + endunit_len;
	} else if(dz->param[RC_SCAT] > 0.0)
		dz->iparam[RC_SCAT] = 0;		
	else
		dz->iparam[RC_SCAT] = -1;		
	return(FINISHED);
}

/********************************************************************************************/
/********************************** FORMERLY IN buffers.c ***********************************/
/********************************************************************************************/

/**************************** ALLOCATE_LARGE_BUFFERS ******************************/

int allocate_large_buffers(dataptr dz)
{
	switch(dz->process) {
	case(EDIT_CUT):
	case(EDIT_CUTEND):
	case(MANY_ZCUTS):
	case(EDIT_ZCUT):
	case(EDIT_EXCISE):
	case(EDIT_EXCISEMANY):
	case(EDIT_INSERT):
	case(EDIT_INSERT2):
	case(EDIT_INSERTSIL):
	case(INSERTSIL_MANY):
	case(EDIT_JOIN):
	case(JOIN_SEQ):
	case(JOIN_SEQDYN):
	case(RANDCHUNKS):
		return create_edit_buffers(dz);
	case(RANDCUTS):
	case(NOISE_SUPRESS):
		return create_sndbufs(dz);
	case(EDIT_CUTMANY):
		return create_cutmany_sndbuf(dz);

	case(TWIXT):
	case(SPHINX):
		return create_twixt_buffers(dz);
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
	if     (!strcmp(prog_identifier_from_cmdline,"cut"))			dz->process = EDIT_CUT;
	else if(!strcmp(prog_identifier_from_cmdline,"cutend"))			dz->process = EDIT_CUTEND;
	else if(!strcmp(prog_identifier_from_cmdline,"zcut"))			dz->process = EDIT_ZCUT;
	else if(!strcmp(prog_identifier_from_cmdline,"zcuts"))			dz->process = MANY_ZCUTS;
	else if(!strcmp(prog_identifier_from_cmdline,"excise"))			dz->process = EDIT_EXCISE;
	else if(!strcmp(prog_identifier_from_cmdline,"excises"))		dz->process = EDIT_EXCISEMANY;
	else if(!strcmp(prog_identifier_from_cmdline,"insert"))			dz->process = EDIT_INSERT;
	else if(!strcmp(prog_identifier_from_cmdline,"replace"))		dz->process = EDIT_INSERT2;
	else if(!strcmp(prog_identifier_from_cmdline,"insil"))			dz->process = EDIT_INSERTSIL;
	else if(!strcmp(prog_identifier_from_cmdline,"join"))			dz->process = EDIT_JOIN;
	else if(!strcmp(prog_identifier_from_cmdline,"masks"))			dz->process = INSERTSIL_MANY;
	else if(!strcmp(prog_identifier_from_cmdline,"randcuts"))		dz->process = RANDCUTS;
	else if(!strcmp(prog_identifier_from_cmdline,"randchunks"))		dz->process = RANDCHUNKS;
	else if(!strcmp(prog_identifier_from_cmdline,"twixt"))			dz->process = TWIXT;
	else if(!strcmp(prog_identifier_from_cmdline,"sphinx"))			dz->process = SPHINX;
	else if(!strcmp(prog_identifier_from_cmdline,"noisecut"))		dz->process = NOISE_SUPRESS;
	else if(!strcmp(prog_identifier_from_cmdline,"cutmany"))		dz->process = EDIT_CUTMANY;
	else if(!strcmp(prog_identifier_from_cmdline,"syllables"))		dz->process = SYLLABS;
	else if(!strcmp(prog_identifier_from_cmdline,"joinseq"))		dz->process = JOIN_SEQ;
	else if(!strcmp(prog_identifier_from_cmdline,"joindyn"))		dz->process = JOIN_SEQDYN;
	else {
		sprintf(errstr,"Unknown program identification string '%s'\n",prog_identifier_from_cmdline);
		return(USAGE_ONLY);
	}
	return(FINISHED);
}

/********************************************************************************************/
/********************************** FORMERLY IN usage.c *************************************/
/********************************************************************************************/

/******************************** USAGE1 ********************************/

int usage1(void)
{
	sprintf(errstr,
	"USAGE: sfedit NAME (mode) infile(s) outfile (parameters)\n"
	"\n"
	"where NAME can be any one of\n"
	"\n"
	"cut    cutend   zcut   zcuts   excise   excises   insert   replace\n"
	"insil  join     masks     randcuts   randchunks    cutmany\n"
	"noisecut   syllables   joinseq    joindyn  twixt  sphinx  noisecut\n"
	"\n"
	"Type 'sfedit excise'  for more info on sfedit excise option... ETC.\n");
	return(USAGE_ONLY);
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
	if(!strcmp(str,"cut")) {
	    sprintf(errstr,
		"CUT & KEEP A SEGMENT OF A SOUND.\n\n"
		"USAGE: sfedit cut mode infile outfile start end [-wsplice]\n\n"
		"MODES ARE\n"
		"1) Time in seconds.\n"
		"2) Time as sample count (rounded to multiples of channel-cnt).\n"
		"3) Time as grouped-sample count (e.g. 3 = 3 stereo-pairs).\n\n"
		"START  is time in infile where segment to keep begins.\n"
		"END    is time in infile where segment to keep ends.\n"
		"SPLICE  splice window in milliseconds (default: %.0lf)\n",EDIT_SPLICELEN);
	} else if(!strcmp(str,"cutmany")) {
	    sprintf(errstr,
		"CUT & KEEP SEVERAL SEGMENTS OF A SOUND.\n\n"
		"USAGE: sfedit cutmany mode infile outfile cuttimes splice\n\n"
		"MODES ARE\n"
		"1) Time in seconds.\n"
		"2) Time as sample count (rounded to multiples of channel-cnt).\n"
		"3) Time as grouped-sample count (e.g. 3 = 3 stereo-pairs).\n\n"
		"CUTTIMES is a file of time-pairs of the start and end of each segment\n"
		"SPLICE  splice window in milliseconds\n"/*,EDIT_SPLICELEN*/);
	} else if(!strcmp(str,"syllables")) {
	    sprintf(errstr,
		"CUT TEXT INTO ITS CONTIGUOUS SYLLABLES.\n\n"
		"USAGE: sfedit syllables mode infil outfil cuttimes dovetail splice [-p]\n\n"
		"MODES ARE\n"
		"1) Time in seconds.\n"
		"2) Time as sample count (rounded to multiples of channel-cnt).\n"
		"3) Time as grouped-sample count (e.g. 3 = 3 stereo-pairs).\n\n"
		"CUTTIMES is starttime of syllables, and end time of last syllable.\n"
		"DOVETAIL is time in milliseconds to allow for syllable overlap.\n"
		"SPLICE  splice window in milliseconds.\n"
		"-p   Forces process to cut PAIRS of syllables.\n");
	} else if(!strcmp(str,"cutend")) {
	    sprintf(errstr,
		"CUT & KEEP END PORTION OF A SOUND.\n\n"
		"USAGE: sfedit cutend mode infile outfile length [-wsplice]\n\n"
		"MODES ARE\n"
		"1) Time in seconds.\n"
		"2) Time as sample count (rounded to multiples of channel-cnt).\n"
		"3) Time as grouped-sample count (e.g. 3 = 3 stereo-pairs).\n\n"
		"LENGTH is length of sound to keep, ending at end of input sound.\n"
		"SPLICE  splice window in milliseconds (default: %.0lf)\n",EDIT_SPLICELEN);
	} else if(!strcmp(str,"zcut")) {
	    sprintf(errstr,
		"CUT & KEEP SEG OF MONO SOUND, CUTTING AT ZERO-CROSSINGS (NO SPLICES).\n\n"
    	"USAGE: sfedit zcut mode infile outfile start end\n\n"
		"MODES ARE\n"
		"1) Time in seconds.\n"
		"2) Time as sample count.\n"
		"START  is (approx) time in infile where segment to keep begins.\n"
		"END    is (approx) time in infile where segment to keep ends.\n");
	} else if(!strcmp(str,"zcuts")) {
	    sprintf(errstr,
		"CUT & KEEP SEGS OF MONO SOUND, CUTTING AT ZERO-CROSSINGS (NO SPLICES).\n\n"
    	"USAGE: sfedit zcuts mode infile outfile cuttimes\n\n"
		"MODES ARE\n"
		"1) Time in seconds.\n"
		"2) Time as sample count.\n"
		"CUTTIMES is a file of time-pairs of the start and end of each segment\n");
	} else if(!strcmp(str,"excise")) {
	    sprintf(errstr,
		"DISCARD SPECIFIED CHUNK OF SOUND, CLOSING UP THE GAP.\n\n"
		"USAGE: sfedit excise mode infile outfile start end [-wsplice]\n\n"
		"MODES ARE\n"
		"1) Time in seconds.\n"
		"2) Time as sample count (rounded to multiples of channel-cnt).\n"
    	"START   starttime of excision.\n"
    	"END     endtime of excision.\n"
		"SPLICE  splice window in milliseconds (default: %.0lf)\n",EDIT_SPLICELEN);
	} else if(!strcmp(str,"excises")) {
	    sprintf(errstr,
		"DISCARD SPECIFIED CHUNKS OF A SOUND, CLOSING UP THE GAPS.\n\n"
    	"USAGE: sfedit excises mode infile outfile excisefile [-wsplice]\n\n"
		"MODES ARE\n"
		"1) Time in seconds.\n"
		"2) Time as sample count (rounded to multiples of channel-cnt).\n"
		"3) Time as grouped-sample count (e.g. 3 = 3 stereo-pairs).\n\n"
		"EXCISEFILE is a texfile with (paired) start & end times of chunks\n"
		"           to be removed. These must be in increasing time order.\n"
		"SPLICE  splice window in milliseconds (default: %.0lf)\n",EDIT_SPLICELEN);
	} else if(!strcmp(str,"insert")) {
		sprintf(errstr,
		"INSERT A 2nd SOUND INTO AN EXISTING SOUND.\n\n"
		"USAGE: sfedit insert mode infile insert outfile time [-wsplice] [-llevel] [-o]\n\n"
		"MODES ARE\n"
		"1) Time in seconds.\n"
		"2) Time as sample count (rounded to multiples of channel-cnt).\n"
		"3) Time as grouped-sample count (e.g. 3 = 3 stereo-pairs).\n\n"
		"TIME    at which 2nd file to be inserted in 1st file.\n"
		"SPLICE  splice window in milliseconds (default: %.0lf)\n"
		"LEVEL   gain multiplier on inserted file. (default 1.0).\n"
		"-o      overwrite equivalent duration of original file with inserted file.\n"
		"        (default .. insertion pushes infile apart).\n",EDIT_SPLICELEN);
	} else if(!strcmp(str,"replace")) {
		sprintf(errstr,
		"INSERT A 2nd SOUND INTO AN EXISTING SOUND, REPLACING PART OF ORIGINAL SOUND.\n\n"
		"USAGE: sfedit replace mode infile insert outfile time endtime [-wsplice] [-llevel]\n\n"
		"MODES ARE\n"
		"1) Time in seconds.\n"
		"2) Time as sample count (rounded to multiples of channel-cnt).\n"
		"3) Time as grouped-sample count (e.g. 3 = 3 stereo-pairs).\n\n"
		"TIME    at which 2nd file to be inserted in 1st file.\n"
		"ENDTIME endtime of segment in original file to be replaced.\n"
		"SPLICE  splice window in milliseconds (default: %.0lf)\n"
		"LEVEL   gain multiplier on inserted file. (default 1.0).\n",EDIT_SPLICELEN);
	} else if(!strcmp(str,"insil")) {
		sprintf(errstr,
		"INSERT SILENCE INTO AN EXISTING SOUND.\n\n"
		"USAGE: sfedit insil mode infile outfile time duration [-wsplice] [-o] [-s]\n\n"
		"MODES ARE\n"
		"1) Time in seconds.\n"
		"2) Time as sample count (rounded to multiples of channel-cnt).\n"
		"3) Time as grouped-sample count (e.g. 3 = 3 stereo-pairs).\n\n"
		"TIME     at which silence to be inserted in infile.\n"
		"DURATION of inserted silence.\n"
		"SPLICE   splice window in milliseconds (default: %.0lf)\n"
		"-o       overwrites original file with the silence.\n"
		"         (default .. silence pushes infile apart).\n"
		"-s       retains any silence written over file end.\n"
		"         (default .. rejects silence added at file end).\n",EDIT_SPLICELEN);
	} else if(!strcmp(str,"join")) {
    	sprintf(errstr,
    	"JOIN FILES TOGETHER, ONE AFTER ANOTHER.\n\n"
		"USAGE: sfedit join infile1 [infile2 infile3....] outfile [-wsplice] [-b] [-e] \n\n"
    	"SPLICE duration of splices, in MS (default 15)\n"
    	"-b     splices start of first file\n"
    	"-e     splices end of last file\n");
	} else if(!strcmp(str,"joinseq")) {
    	sprintf(errstr,
    	"JOIN FILES TOGETHER, ONE AFTER ANOTHER, IN A PATTERN\n\n"
		"USAGE: sfedit join infile1 [infile2 ....] outfile pattern [-wsplice] [-mmaxlen] [-b] [-e] \n\n"
    	"PATTERN file contains sequence of numbers = sequence of input files to use\n"
    	"SPLICE duration of splices, in MS (default 15)\n"
    	"MAXLEN maximum number of items in pattern to use\n"
    	"-b     splices start of first file\n"
    	"-e     splices end of last file\n");
	} else if(!strcmp(str,"joindyn")) {
    	sprintf(errstr,
    	"JOIN FILES TOGETHER, ONE AFTER ANOTHER, IN A PATTERN, WITH LOUDNESS PATTERN\n\n"
		"USAGE: sfedit join inf1 [inf2...] outf pattern [-wsplice] [-b] [-e] \n\n"
    	"PATTERN  file of sequence of numbers = sequence of input files to use,\n"
    	"         each paired with a gain value for that pattern item\n"
    	"SPLICE duration of splices, in MS (default 15)\n"
    	"-b     splices start of first file\n"
    	"-e     splices end of last file\n");
	} else if(!strcmp(str,"masks")) {
	    sprintf(errstr,
		"MASK SPECIFIED CHUNKS OF A SOUND, WITH SILENCE.\n\n"
    	"USAGE: sfedit masks mode infile outfile excisefile [-wsplice]\n\n"
		"MODES ARE\n"
		"1) Time in seconds.\n"
		"2) Time as sample count (rounded to multiples of channel-cnt).\n"
		"3) Time as grouped-sample count (e.g. 3 = 3 stereo-pairs).\n\n"
		"EXCISEFILE is a texfile with (paired) start & end times of chunks\n"
		"           to be masked. These must be in increasing time order.\n"
		"SPLICE  splice window in milliseconds (default: %.0lf)\n",EDIT_SPLICELEN);
	} else if(!strcmp(str,"randcuts")) {
	    sprintf(errstr,
		"CUT FILE INTO PIECES WITH CUTS AT RANDOM TIMES.\n\n"
    	"USAGE: sfedit randcuts infile average-chunklen scattering\n\n"
		"Names of outfiles will be infilename truncated by 1 character,\n"
		"      with a number added, starting from zero.\n"
		"average-chunklen is average length of chunks cut.\n"
		"scattering       is variation in length of cuts : range 0-8\n");
	} else if(!strcmp(str,"randchunks")) {
	    sprintf(errstr,
		"CUT CHUNKS FROM FILE, RANDOMLY.\n\n"
    	"USAGE:\n"
    	"sfedit randchunks infile chunkcnt minchunk [-mmaxchunk] [-l] [-s]\n\n"
		"Names of outfiles will be 'infilename' truncated by 1 character,\n"
		"      with a number added, starting from zero.\n"
		"Chunkcnt is number of chunks to cut.\n"
		"Minchunk is minimum length of chunks.\n\n"
		"-m    maxchunk is maximum length of chunks.\n"
		"-l    chunks chosen are evenly distributed over file\n"
		"      (default: random distribution)\n"
		"-s    all chunks start at beginning of file\n");
	} else if(!strcmp(str,"twixt")) {
	    sprintf(errstr,
		"SWITCH BETWEEN SEVERAL FILES, TO MAKE A NEW SOUND.\n\n"
    	"USAGE:\n"
    	"sfedit twixt mode infile(s) outfile switch-times splicelen (segcnt) [-wweight] [-r]\n\n"
		"MODES ARE\n"
		"1) In Sequence    Imagine all files are running in parallel on a multitrack.\n"
		"                  Switch from one to another at switch-times.\n"
		"2) Permuted       ..similarly, but time-segment order randomly permuted.\n"
		"3) Random Choice  ..similarly, but chose any time-seg, at random, as next segment.\n\n"
		"4) Edit only      Cut file-1 (only) to chunks defined by switch-times,\n"
		"                  and output chunks as separate files.\n"
		"SWITCH-TIMES is a texfile with times which divide every file into segments.\n"
		"             Times must be in ascending order.\n"
		"             (Include time zero if you want to use segments at start of files).\n"
		"SPLICELEN    is duration of splices, in milliseconds\n"
		"SEGCNT       is number of segments to use in output (only with modes 2 & 3)\n"
		"-wWEIGHT     if flag set, file-1 occurs WEIGHT times more often than other files.\n"
		"-r           if flag set, order of files used is randomly permuted.\n");
	} else if(!strcmp(str,"sphinx")) {
	    sprintf(errstr,
		"SWITCH BETWEEN SEVERAL FILES, WITH DIFFERENT SWITCH TIMES, TO MAKE NEW SOUND.\n\n"
    	"USAGE:\n"
    	"sfedit sphinx mode infile(s) outfile switch-times splicelen (segcnt) [-wweight -r]\n\n"
		"MODES ARE\n"
		"1) In Sequence    Imagine all files are running in parallel on a multitrack.\n"
		"                  Switch from one to another at switch-times, where\n"
		"                  Nth switch-time in one file, corresponds to Nth in another file.\n"
		"                  but these are not necessarily the same absolute time.\n"
		"2) Permuted       ..similarly, but time-segment order randomly permuted.\n"
		"3) Random Choice  ..similarly, but chose any time-seg, at random, as next segment.\n\n"
		"SWITCH-TIMES texfile with times which divide each file into segments.\n"
		"             There must be one column of times for each input file,\n"
		"             and the same number of entries in each column.\n"
		"             Times, in each column, must be in ascending order.\n"
		"             (Include time zero if you want to use a segment at start of a file).\n"
		"SPLICELEN    is duration of splices, in milliseconds\n"
		"SEGCNT       is number of segments to use in output (only with modes 2 & 3)\n"
		"-wWEIGHT     if flag set, 1st file occurs WEIGHT times more often than other files.\n"
		"-r           if flag set, order of files used is randomly permuted.\n");
	} else if(!strcmp(str,"noisecut")) {
	    sprintf(errstr,
		"SUPPRESS NOISE IN A (MONO) SOUND FILE: REPLACE BY SILENCE.\n\n"
    	"USAGE: sfedit noisecut infile outfile splicelen noisfrq maxnoise mintone [-n]\n\n"
		"SPLICELEN    duration of splices, in MS\n"
		"NOISEFRQ     frequency above which signal regarded as noise (try 6000 Hz)\n"
		"MAXNOISE     max duration of any noise segments permitted to remain (NOT replaced)\n"
		"MINTONE      min duration of any non-noise segments to be retained.\n"
		"-n           retain noise rather than non-noise\n");
	} else
		sprintf(errstr,"Unknown option '%s'\n",str);
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

/******************************** SYLLABS_CONSISTENCY ********************************
 *
 * Converts the syllables data into EDIT_CUTMANY data
 */

int syllabs_pconsistency(dataptr dz)
{
	int orig_cnt = dz->itemcnt, new_cnt = dz->itemcnt, n, k;
	int end, stt;
	int dovesamps = round(dz->param[SYLLAB_DOVETAIL] * MS_TO_SECS * (double)dz->infile->srate);
	dovesamps *= dz->infile->channels;
	if(dz->vflag[SYLLAB_PAIRS]) {
		if(dz->itemcnt < 3) {
			sprintf(errstr,"INSUFFICIENT DATA to cut syllable pairs (need at least 3 times).\n");
			return(MEMORY_ERROR);
		}
		new_cnt -= 2;
		new_cnt *= 2;
		if((dz->lparray[0] = (int *)realloc((char *)dz->lparray[0],(orig_cnt + new_cnt) * sizeof(int)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to reallocate cut-times stores.\n");
			return(MEMORY_ERROR);
		}
		for(n=orig_cnt-3,k = (orig_cnt + new_cnt) - 2;n>=0;n--,k-=2) {
			end = min(dz->insams[0],dz->lparray[0][n+2] + dovesamps);
			dz->lparray[0][k+1] = end;
			stt = max(0,dz->lparray[0][n] - dovesamps);
			dz->lparray[0][k] = stt;
		}
		for(n=0,k = orig_cnt;n < new_cnt;n++,k++)
			dz->lparray[0][n] = dz->lparray[0][k];
	} else {
		new_cnt--;
		new_cnt *= 2;
		if((dz->lparray[0] = (int *)realloc((char *)dz->lparray[0],new_cnt * sizeof(int)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to reallocate cut-times stores.\n");
			return(MEMORY_ERROR);
		}
		for(n=orig_cnt-2,k = new_cnt - 2;n>=0;n--,k-=2) {
			end = min(dz->insams[0],dz->lparray[0][n+1] + dovesamps);
			dz->lparray[0][k+1] = end;
			stt = max(0,dz->lparray[0][n] - dovesamps);
			dz->lparray[0][k] = stt;
		}		
	}
	dz->itemcnt = new_cnt;
	dz->tempsize = dz->itemcnt/2;
	dz->process = EDIT_CUTMANY;
	return(FINISHED);
}

/******************************** GET_SYLLAB_TIMES ********************************/

int get_syllab_times(char *filename,dataptr dz)
{
	FILE *fp;
	int n = 0;
	int arraysize = BIGARRAY, sampval = 0;
	char temp[200], *p;
	double  dummy;

	if((fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,"Cannot open file %s to read cut-times data.\n",filename);
		return(DATA_ERROR);
	}
	if((dz->lparray[0] = (int *)malloc(arraysize * sizeof(int *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store time-switch data.\n");
		return(MEMORY_ERROR);
	}
		
	while(fgets(temp,200,fp)!=NULL) {
		p = temp;
		while(isspace(*p))
			p++;
		if(*p == ';')
			continue;

		while(get_float_from_within_string(&p,&dummy)) {
			if(dummy < 0.0) {
				sprintf(errstr,"Cut-segment time before zero in file %s\n",filename);
				return(DATA_ERROR);
			}
			switch(dz->mode) {
			case(EDIT_SECS): 
				sampval = (int)round(dummy * dz->infile->srate);
				sampval *= dz->infile->channels;
				break;
			case(EDIT_SAMPS): 
				sampval = (int)round(dummy);
				break;
			case(EDIT_STSAMPS): 
				sampval = (int)round(dummy) * dz->infile->channels;
				break;
			}
			if(n>0) {
				dz->lparray[0][n] = min(sampval,dz->insams[0]);
				if(sampval <= dz->lparray[0][n-1]) {
					sprintf(errstr,"Time %d in file %s is not in the correct order.\n",(n+1)/2,filename);
					return(DATA_ERROR);
				}
			} else 
				dz->lparray[0][n] = max(sampval,0);
			if(dz->lparray[0][n] >= dz->insams[0])
				break;
			if(++n >= arraysize) {
				arraysize += BIGARRAY;
				if((dz->lparray[0] = (int *)realloc((char *)dz->lparray[0],arraysize*sizeof(int)))==NULL) {
					sprintf(errstr,"INSUFFICIENT MEMORY to reallocate cut-times store.\n");
					return(MEMORY_ERROR);
				}
			}
		}
	}
	if(n<2) {
		sprintf(errstr,"Insufficient data found in data file %s (at least 2 times required)\n",filename);
		return(DATA_ERROR);
	}
	dz->itemcnt = n;
	if(fclose(fp)<0) {
		fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
		fflush(stdout);
	}
	return(FINISHED);
}

/******************************** GET_JOINSEQ ********************************/

int get_joinseq(char *filename,dataptr dz)
{
	FILE *fp;
	int n = 0, k;
	int arraysize = BIGARRAY;
	char temp[200], *p;
	double  dummy;

	if((fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,"Cannot open file %s to read joining seqence.\n",filename);
		return(DATA_ERROR);
	}
	if((dz->iparray[0] = (int *)malloc(arraysize * sizeof(int *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store joining seqence data.\n");
		return(MEMORY_ERROR);
	}
		
	while(fgets(temp,200,fp)!=NULL) {
		p = temp;
		while(isspace(*p))
			p++;
		if(*p == ';')
			continue;

		while(get_float_from_within_string(&p,&dummy)) {
			k = round(dummy);
			if(k < 1 || k > dz->infilecnt) {
				sprintf(errstr,"Number '%d' in sequence data does not correspond to any input soundfile\n",k);
				return(DATA_ERROR);
			}
			k--;
			dz->iparray[0][n] = k;
			if(++n >= arraysize) {
				arraysize += BIGARRAY;
				if((dz->iparray[0] = (int *)realloc((char *)dz->iparray[0],arraysize*sizeof(int)))==NULL) {
					sprintf(errstr,"INSUFFICIENT MEMORY to reallocate joining sequence data.\n");
					return(MEMORY_ERROR);
				}
			}
		}
	}
	if(n<2) {
		sprintf(errstr,"Insufficient data found in data file %s (at least 2 file numbers reuqired)\n",filename);
		return(DATA_ERROR);
	}
	dz->itemcnt = n;
	if(fclose(fp)<0) {
		fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
		fflush(stdout);
	}
	return(FINISHED);
}

/******************************** GET_JOINDYNSEQ ********************************/

int get_joindynseq(char *filename,dataptr dz)
{
	FILE *fp;
	int n = 0, k, is_loudness;
	int arraysize = BIGARRAY;
	char temp[200], *p;
	double  dummy, maxloudness;

	if((fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,"Cannot open file %s to read joining seqence.\n",filename);
		return(DATA_ERROR);
	}
	if((dz->iparray[0] = (int *)malloc(arraysize * sizeof(int *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store joining seqence data.\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[2] = (double *)malloc(arraysize * sizeof(double *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store joining seqence data.\n");
		return(MEMORY_ERROR);
	}
	is_loudness = 0;
	maxloudness = 0.0;
	while(fgets(temp,200,fp)!=NULL) {
		p = temp;
		while(isspace(*p))
			p++;
		if(*p == ';')
			continue;

		while(get_float_from_within_string(&p,&dummy)) {
			if(is_loudness) {
				if(dummy < 0.0) {
					sprintf(errstr,"Number '%lf' in loudness data is invalid (negative).\n",dummy);
					return(DATA_ERROR);
				} else if(dummy > maxloudness)
					maxloudness = dummy;
				dz->parray[2][n] = dummy;
			} else {
				k = round(dummy);
				if(k < 1 || k > dz->infilecnt) {
					sprintf(errstr,"Number '%d' in sequence data does not correspond to any input soundfile\n",k);
					return(DATA_ERROR);
				}
				k--;
				dz->iparray[0][n] = k;
			}
			if(is_loudness) {
				if(++n >= arraysize) {
					arraysize += BIGARRAY;
					if((dz->iparray[0] = (int *)realloc((char *)dz->iparray[0],arraysize*sizeof(int)))==NULL) {
						sprintf(errstr,"INSUFFICIENT MEMORY to reallocate joining sequence data.\n");
						return(MEMORY_ERROR);
					}
					if((dz->parray[2] = (double *)realloc((char *)dz->parray[2],arraysize*sizeof(double)))==NULL) {
						sprintf(errstr,"INSUFFICIENT MEMORY to reallocate joining sequence loudness data.\n");
						return(MEMORY_ERROR);
					}
				}
			}
			is_loudness = !is_loudness;
		}
	}
	if(is_loudness) {
		sprintf(errstr,"Data incorrectly paired in file %s\n",filename);
		return(DATA_ERROR);
	}
	if(n<2) {
		sprintf(errstr,"Insufficient data found in data file %s (at least 2 file numbers reuqired)\n",filename);
		return(DATA_ERROR);
	}
	dz->itemcnt = n;
	for(n=0;n<dz->itemcnt;n++)		/* NORMALISE LOUDNESS VALS */
		dz->parray[2][n] /= maxloudness;
	if(fclose(fp)<0) {
		fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
		fflush(stdout);
	}
	return(FINISHED);
}
