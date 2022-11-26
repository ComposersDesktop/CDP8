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
/* RWD 14-20: removed usage refs to 'dump' and 'recover' */


/* floatsam version TW */
#include <stdio.h>
#include <stdlib.h>
#include <structures.h>
#include <cdpmain.h>
#include <tkglobals.h>
#include <pnames.h>
#include <house.h>
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
#include <flags.h>
//TW UPDATE
#include "sfdump.h"

#if defined unix || defined __GNUC__
#define round(x) lround((x))
#endif

static int batchexpand_consistency(dataptr dz);
static int read_batchexpand_params(char *filename,dataptr dz);
static int create_bakup_sndbufs(dataptr dz);

/***************************************************************************************/
/****************************** FORMERLY IN aplinit.c **********************************/
/***************************************************************************************/

/***************************** ESTABLISH_BUFPTRS_AND_EXTRA_BUFFERS **************************/

int establish_bufptrs_and_extra_buffers(dataptr dz)
{
	int exit_status;
//	int is_spec = FALSE;
	dz->extra_bufcnt = -1;	/* ENSURE EVERY CASE HAS A PAIR OF ENTRIES !! */
	dz->bptrcnt = 0;
	dz->bufcnt  = 0;
	switch(dz->process) {
	case(HOUSE_COPY):
		switch(dz->mode) {
		case(COPYSF):			dz->extra_bufcnt = 0;	dz->bufcnt = 1;		break;
		case(DUPL):				dz->extra_bufcnt = 0;	dz->bufcnt = 1;		break;
		default:
			sprintf(errstr,"Unknown mode for HOUSE_COPY in establish_bufptrs_and_extra_buffers()\n");
			return(PROGRAM_ERROR);
		}
		break;
	case(HOUSE_DEL):			dz->extra_bufcnt = 0;	dz->bufcnt = 0;							break;
	case(HOUSE_CHANS):
		switch(dz->mode) {
		case(HOUSE_CHANNEL):
		case(HOUSE_CHANNELS):	dz->extra_bufcnt = 0;	dz->bufcnt = MAX_SNDFILE_OUTCHANS;		break;
		case(HOUSE_ZCHANNEL):	dz->extra_bufcnt = 0;	dz->bufcnt = MAX_SNDFILE_OUTCHANS;		break;
		case(STOM):				dz->extra_bufcnt = 0;	dz->bufcnt = MAX_SNDFILE_OUTCHANS;		break;
		default:				dz->extra_bufcnt = 0;	dz->bufcnt = 2;							break;
		}
		break;
	case(HOUSE_BUNDLE):			dz->extra_bufcnt = 0;	dz->bufcnt = 0;							break;
	case(HOUSE_SORT):			dz->extra_bufcnt = 0;	dz->bufcnt = 0;							break;
	case(HOUSE_SPEC):
		switch(dz->mode) {
		case(HOUSE_RESAMPLE):	dz->extra_bufcnt = 0;	dz->bufcnt = 5;							break;
		case(HOUSE_CONVERT):	dz->extra_bufcnt = 0;	dz->bufcnt = 1;	dz->bptrcnt = 1;		break;
		case(HOUSE_REPROP):		dz->extra_bufcnt = 0;	dz->bufcnt = 1;							break;
		}
		break;
	case(HOUSE_EXTRACT):
		switch(dz->mode) {
		case(HOUSE_CUTGATE):		 dz->extra_bufcnt = 0;	dz->bufcnt = 2;						break;
		case(HOUSE_CUTGATE_PREVIEW): dz->extra_bufcnt = 0;	dz->bufcnt = 2;						break;
		case(HOUSE_TOPNTAIL):		 dz->extra_bufcnt = 0;	dz->bufcnt = 2;						break;
		case(HOUSE_RECTIFY):		 dz->extra_bufcnt = 0;	dz->bufcnt = 1;						break;
		case(HOUSE_BYHAND):		 	 dz->extra_bufcnt = 0;	dz->bufcnt = 1;						break;
		case(HOUSE_ONSETS):		 	 dz->extra_bufcnt = 0;	dz->bufcnt = 2;						break;
		default:
			sprintf(errstr,"Unknown case for HOUSE_EXTRACT in establish_bufptrs_and_extra_buffers()\n");
			return(PROGRAM_ERROR);
		}
		break;
	case(TOPNTAIL_CLICKS):		dz->extra_bufcnt = 0;	dz->bufcnt = 2;							break;
	case(HOUSE_BAKUP):			
	case(HOUSE_GATE):			dz->extra_bufcnt = 0;	dz->bufcnt = 2;							break;
	case(HOUSE_GATE2):			dz->extra_bufcnt = 0;	dz->bufcnt = 1;							break;
	case(BATCH_EXPAND):
	case(HOUSE_DISK):			dz->extra_bufcnt = 0;	dz->bufcnt = 0;							break;
	default:
		sprintf(errstr,"Unknown program type [%d] in establish_bufptrs_and_extra_buffers()\n",dz->process);
		return(PROGRAM_ERROR);
	}

	if(dz->extra_bufcnt < 0) {
		sprintf(errstr,"bufcnts have not been set: establish_bufptrs_and_extra_buffers()\n");
		return(PROGRAM_ERROR);
	}
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
	case(HOUSE_COPY):	   dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt=0; dz->ptr_cnt= 0; dz->fptr_cnt = 0; break;
	case(HOUSE_DEL):       dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt=0; dz->ptr_cnt= 0; dz->fptr_cnt = 0; break;
	case(HOUSE_CHANS):     dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt=0; dz->ptr_cnt= 0; dz->fptr_cnt = 0; break;
	case(HOUSE_BUNDLE):	   dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt=0; dz->ptr_cnt= 0; dz->fptr_cnt = 0; break;
	case(HOUSE_SORT):	   dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt=0; dz->ptr_cnt= 0; dz->fptr_cnt = 0; break;
	case(HOUSE_SPEC):
		switch(dz->mode) {
		case(HOUSE_RESAMPLE): dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt=0; dz->ptr_cnt=6; dz->fptr_cnt=0; break;
		default:			  dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt=0; dz->ptr_cnt=0; dz->fptr_cnt=0; break;
		}
		break;		
	case(HOUSE_EXTRACT):
		switch(dz->mode) {
		case(HOUSE_CUTGATE): dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt=2; dz->ptr_cnt=0; dz->fptr_cnt= 0; break;
		case(HOUSE_ONSETS):  dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt=1; dz->ptr_cnt=0; dz->fptr_cnt= 0; break;
		case(HOUSE_BYHAND):  dz->array_cnt=2; dz->iarray_cnt=0; dz->larray_cnt=0; dz->ptr_cnt=0; dz->fptr_cnt= 0; break;
		default:		   dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt=0; dz->ptr_cnt= 0; dz->fptr_cnt = 0; break;
		}
		break;
	case(TOPNTAIL_CLICKS): dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt=0; dz->ptr_cnt= 0; dz->fptr_cnt = 0; break;
	case(HOUSE_BAKUP):	   dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt=0; dz->ptr_cnt= 0; dz->fptr_cnt = 0; break;
	case(HOUSE_GATE):	   dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt=0; dz->ptr_cnt= 0; dz->fptr_cnt = 0; break;
	case(HOUSE_GATE2):	   dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt=1; dz->ptr_cnt= 0; dz->fptr_cnt = 0; break;
	case(BATCH_EXPAND):
	case(HOUSE_DISK):	   dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt=0; dz->ptr_cnt= 0; dz->fptr_cnt = 0; break;
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
	case(HOUSE_COPY):
		switch(dz->mode) {
		case(COPYSF):		setup_process_logic(ALL_FILES,			EQUAL_SNDFILE,		SNDFILE_OUT,	dz);	break;
		case(DUPL):
			if(!sloom)
				setup_process_logic(SNDFILES_ONLY,		OTHER_PROCESS,		NO_OUTPUTFILE,	dz);
			else
				setup_process_logic(ALL_FILES,			EQUAL_SNDFILE,		SNDFILE_OUT,	dz);
			break;
		default:
			sprintf(errstr,"Unknown mode for HOUSE_COPY in assign_process_logic()\n");
			return(PROGRAM_ERROR);
		}
		break;
	case(HOUSE_DEL):		setup_process_logic(NO_FILE_AT_ALL,		OTHER_PROCESS,		NO_OUTPUTFILE,	dz);	break;
	case(HOUSE_CHANS):		
		switch(dz->mode) {
		case(HOUSE_CHANNEL):	setup_process_logic(SNDFILES_ONLY,	UNEQUAL_SNDFILE,	NO_OUTPUTFILE,	dz);	break;
		case(HOUSE_CHANNELS):	setup_process_logic(SNDFILES_ONLY,	UNEQUAL_SNDFILE,	NO_OUTPUTFILE,	dz);	break;
		case(HOUSE_ZCHANNEL):	setup_process_logic(SNDFILES_ONLY,	UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
		case(STOM):				setup_process_logic(SNDFILES_ONLY,	UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
		case(MTOS):				setup_process_logic(SNDFILES_ONLY,	UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
		default:
			sprintf(errstr,"Unknown mode for HOUSE_CHANS in assign_process_logic()\n");
			return(PROGRAM_ERROR);
		}
		break;
	case(HOUSE_BUNDLE):			setup_process_logic(ANY_NUMBER_OF_ANY_FILES,TO_TEXTFILE,		TEXTFILE_OUT,	dz);	break;
	case(HOUSE_SORT):			setup_process_logic(WORDLIST_ONLY,			OTHER_PROCESS,		NO_OUTPUTFILE,	dz);	break;
	case(HOUSE_SPEC):			
		switch(dz->mode) {
		case(HOUSE_REPROP):		setup_process_logic(SNDFILES_ONLY,	EQUAL_SNDFILE,		SNDFILE_OUT,	dz);	break;
		default:				setup_process_logic(SNDFILES_ONLY,	UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
		}
		break;
	case(HOUSE_EXTRACT):
		switch(dz->mode) {
		case(HOUSE_CUTGATE):		 setup_process_logic(SNDFILES_ONLY,	OTHER_PROCESS,	 NO_OUTPUTFILE,	dz);	break;
		case(HOUSE_ONSETS):			 setup_process_logic(SNDFILES_ONLY,	TO_TEXTFILE,	 TEXTFILE_OUT,	dz);	break;
		case(HOUSE_CUTGATE_PREVIEW): setup_process_logic(SNDFILES_ONLY, UNEQUAL_SNDFILE, SNDFILE_OUT,	dz);	break;
		case(HOUSE_TOPNTAIL):		 setup_process_logic(SNDFILES_ONLY,	UNEQUAL_SNDFILE, SNDFILE_OUT,	dz);	break;
		case(HOUSE_RECTIFY):		 setup_process_logic(SNDFILES_ONLY,	EQUAL_SNDFILE,	 SNDFILE_OUT,	dz);	break;
		case(HOUSE_BYHAND):		 	 setup_process_logic(SNDFILES_ONLY,	EQUAL_SNDFILE,	 SNDFILE_OUT,	dz);	break;
		}
		break;
	case(TOPNTAIL_CLICKS):  setup_process_logic(SNDFILES_ONLY,	UNEQUAL_SNDFILE, SNDFILE_OUT,	dz);			break;
	case(HOUSE_BAKUP):		setup_process_logic(ONE_OR_MANY_SNDFILES,	UNEQUAL_SNDFILE, SNDFILE_OUT,	dz);	break;
	case(HOUSE_GATE):		setup_process_logic(SNDFILES_ONLY,			OTHER_PROCESS,	 NO_OUTPUTFILE,	dz);	break;
	case(HOUSE_GATE2):		setup_process_logic(SNDFILES_ONLY,			EQUAL_SNDFILE,	 SNDFILE_OUT,	dz);	break;
	case(HOUSE_DISK):			
		setup_process_logic(ALL_FILES,		SCREEN_MESSAGE,		NO_OUTPUTFILE,	dz);	
		break;
	case(BATCH_EXPAND):	    setup_process_logic(ANY_NUMBER_OF_ANY_FILES, TO_TEXTFILE,	TEXTFILE_OUT,	dz);	break;
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
	case(HOUSE_COPY):   	exit_status = set_internalparam_data("",ap); 		break;
	case(HOUSE_DEL):   		exit_status = set_internalparam_data("",ap); 		break;
	case(HOUSE_CHANS):   	exit_status = set_internalparam_data("",ap); 		break;
	case(HOUSE_BUNDLE):		exit_status = set_internalparam_data("",ap); 		break;
	case(HOUSE_SORT):   	exit_status = set_internalparam_data("i",ap); 		break;
	case(HOUSE_SPEC):
		switch(mode) {
		case(HOUSE_RESAMPLE):exit_status = set_internalparam_data("00iiii",ap);	break;
		default:			 exit_status = set_internalparam_data("",ap); 		break;
		}
		break;															 /*w	  sp*/
	case(HOUSE_EXTRACT):   												 /*i	  lc*/
		switch(mode) {													 /*n	  nt*/
		case(HOUSE_CUTGATE): exit_status = set_internalparam_data(         "iiiiiii",ap);	break;
		case(HOUSE_ONSETS):  exit_status = set_internalparam_data(         "iiiiiii",ap);	break;
case(HOUSE_CUTGATE_PREVIEW): exit_status = set_internalparam_data("00000000iiiiiii",ap);	break;
	   case(HOUSE_TOPNTAIL): exit_status = set_internalparam_data("ii",ap);					break;
		case(HOUSE_RECTIFY): exit_status = set_internalparam_data("",ap);					break;
		case(HOUSE_BYHAND):  exit_status = set_internalparam_data("",ap);					break;
		}
		break;
	case(TOPNTAIL_CLICKS):	exit_status = set_internalparam_data("",ap);			break;
	case(HOUSE_BAKUP):   	
	case(HOUSE_GATE):   	
	case(HOUSE_GATE2):
	case(BATCH_EXPAND):
	case(HOUSE_DISK):   	exit_status = set_internalparam_data("",ap); 					break;
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
	case(SNDFILENAME):				return read_new_filename(str,dz);
	case(BATCH):					return read_batchexpand_params(str,dz);
	case(BY_HAND):	
// TW ADDED : ensures back-compatibility, but blocks essentially 16-bit (& now redundant) process
		sprintf(errstr,"THIS PROCESS IS NO LONGER AVAILABLE.");
		return(GOAL_FAILED);
	default:
		sprintf(errstr,"Unknown special_data type: read_special_data()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);	/* NOTREACHED */
}

/********************************************************************************************/
/********************************** FORMERLY IN preprocess.c ********************************/
/********************************************************************************************/

/****************************** PARAM_PREPROCESS *********************************/

int param_preprocess(dataptr dz)	
{
//	int exit_status = FINISHED;

	switch(dz->process) {
	case(HOUSE_COPY):
	case(HOUSE_DEL):
	case(HOUSE_BUNDLE):
	case(HOUSE_EXTRACT):
	case(TOPNTAIL_CLICKS):
	case(HOUSE_BAKUP):
	case(HOUSE_GATE):
	case(HOUSE_DISK):
	case(BATCH_EXPAND):
		break;
	case(HOUSE_CHANS):
		switch(dz->mode) {
		case(HOUSE_CHANNELS):
		case(STOM):
		case(MTOS):
			break;
		default:
			if(dz->iparam[CHAN_NO] > dz->infile->channels) {
				sprintf(errstr,"There is no channel %d in the input file\n",dz->iparam[CHAN_NO]);
				return(DATA_ERROR);
			}
			break;
		}
		break;
	case(HOUSE_GATE2):
		dz->iparam[GATE2_DUR]    = max(1,(int)round(dz->param[GATE2_DUR] * MS_TO_SECS * (double)dz->infile->srate));
		dz->iparam[GATE2_ZEROS]  = max(1,(int)round(dz->param[GATE2_ZEROS] * MS_TO_SECS * (double)dz->infile->srate));
		dz->iparam[GATE2_SPLEN]  = max(1,(int)round(dz->param[GATE2_SPLEN] * MS_TO_SECS * (double)dz->infile->srate));
		dz->iparam[GATE2_FILT]   = max(1,(int)round(dz->param[GATE2_FILT] * MS_TO_SECS * (double)dz->infile->srate));
		if(dz->iparam[GATE2_SPLEN] * 2 > dz->iparam[GATE2_ZEROS]) {
			sprintf(errstr,"Splicelength must be no more than half as long as minimum duration of 'silence' around glitch.\n");
			return(DATA_ERROR);
		}
		break;
	case(HOUSE_SPEC): return FINISHED;
	case(HOUSE_SORT): return sort_preprocess(dz);
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
//	int exit_status = FINISHED;

	switch(dz->process) {
	case(HOUSE_COPY):		return do_duplicates(dz);
	case(HOUSE_DEL):		return do_deletes(dz);
	case(HOUSE_CHANS):		return do_channels(dz);
	case(HOUSE_BUNDLE):		return do_bundle(dz);
	case(HOUSE_SORT):		return do_file_sorting(dz);
	case(HOUSE_SPEC):		return do_respecification(dz);
	case(TOPNTAIL_CLICKS):
	case(HOUSE_EXTRACT):	return process_clean(dz);
//TW UPDATES (replacing bracketed-out code - DUMP & RECOVER have now been abandoned)
	case(HOUSE_GATE):		return house_gate(dz);
	case(HOUSE_GATE2):		return house_gate2(dz);
	case(HOUSE_BAKUP):		return house_bakup(dz);

	case(HOUSE_DISK):		return check_available_diskspace(dz);
	case(BATCH_EXPAND):		return batch_expand(dz);
	default:
		sprintf(errstr,"Unknown case in process_file()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);	/* NOTREACHED */
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
	case(TOPNTAIL_CLICKS):
	case(HOUSE_EXTRACT):	return pconsistency_clean(dz);
	case(BATCH_EXPAND):		return batchexpand_consistency(dz);
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
	case(HOUSE_COPY):	 
		if(!(dz->mode == COPYSF && dz->infile->filetype == WORDLIST))
			return create_sndbufs(dz);
		break;
	case(HOUSE_CHANS):	 return create_sndbufs(dz);
	case(HOUSE_SPEC):	 return create_respec_buffers(dz);
//TW UPDATES (replacing bracketed-out code - DUMP & RECOVER have now been abandoned)
	case(HOUSE_BAKUP):	 return create_bakup_sndbufs(dz);
	case(HOUSE_GATE2):
	case(HOUSE_GATE): 	 return create_sndbufs(dz); 

	case(HOUSE_EXTRACT): 
		switch(dz->mode) {
		case(HOUSE_RECTIFY):
		case(HOUSE_BYHAND):	return create_sndbufs(dz);
		default:			return create_clean_buffers(dz);
		}
		break;
	case(TOPNTAIL_CLICKS): return create_clean_buffers(dz);
	case(HOUSE_BUNDLE):
	case(HOUSE_DEL):
	case(HOUSE_SORT):
	case(HOUSE_DISK):
	case(BATCH_EXPAND):
		break;
	default:
		sprintf(errstr,"Unknown program no. in allocate_large_buffers()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/********************************************************************************************/
/********************************** FORMERLY IN cmdline.c ***********************************/
/********************************************************************************************/

int get_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	if     (!strcmp(prog_identifier_from_cmdline,"copy"))			dz->process = HOUSE_COPY;
	else if(!strcmp(prog_identifier_from_cmdline,"remove"))			dz->process = HOUSE_DEL;
	else if(!strcmp(prog_identifier_from_cmdline,"chans"))			dz->process = HOUSE_CHANS;
	else if(!strcmp(prog_identifier_from_cmdline,"bundle"))			dz->process = HOUSE_BUNDLE;
	else if(!strcmp(prog_identifier_from_cmdline,"sort"))			dz->process = HOUSE_SORT;
	else if(!strcmp(prog_identifier_from_cmdline,"respec"))			dz->process = HOUSE_SPEC;
	else if(!strcmp(prog_identifier_from_cmdline,"extract"))		dz->process = HOUSE_EXTRACT;
//TW UPDATES (replacing bracketed-out code - DUMP & RECOVER have now been abandoned)
	else if(!strcmp(prog_identifier_from_cmdline,"bakup"))			dz->process = HOUSE_BAKUP;
	else if(!strcmp(prog_identifier_from_cmdline,"gate"))			dz->process = HOUSE_GATE;

	else if(!strcmp(prog_identifier_from_cmdline,"disk"))			dz->process = HOUSE_DISK;
	else if(!strcmp(prog_identifier_from_cmdline,"batchexpand"))	dz->process = BATCH_EXPAND;
	else if(!strcmp(prog_identifier_from_cmdline,"endclicks"))		dz->process = TOPNTAIL_CLICKS;
	else if(!strcmp(prog_identifier_from_cmdline,"deglitch"))		dz->process = HOUSE_GATE2;
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
	"USAGE: housekeep NAME (mode) infile(s) (outfile) (parameters)\n"
	"\n"
	"where NAME can be any one of\n"
	"\n"
	"extract  copy   remove   chans   respec   bundle   sort   disk\n"
//TW UPDATES
	"bakup    gate    batchexpand    endclicks    deglitch\n"

	"\n"
	"Type 'housekeep chans'  for more info on housekeep chans option... ETC.\n");
	return(USAGE_ONLY);
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
	if(!strcmp(str,"copy")) {
    	fprintf(stdout,
		"MAKE COPIES OF THE INFILE\n\n"
		"USAGE: housekeep copy 1 infile outfile\n"
		"OR:    housekeep copy 2 infile count [-i]\n"
		"MODES ARE\n"
		"1) COPY ONCE\n" 
		"2) COPY MANY\n"
		"         Produces N copies of infile X, with names X_001,X_002...\n"
		"         COUNT is number of duplicates to produce.\n"
		"     -i  ignore existing duplicates (don't overwrite them).\n"
		"         default, process halts on discovering a pre-existing file.\n");

	} else if(!strcmp(str,"remove")) {
    	fprintf(stdout,
		"REMOVE EXISTING COPIES OF A FILE \n\n"
		"USAGE: housekeep remove filename [-a]\n\n"
		"         Deletes any copies of filename X, having names X_001,X_002...\n"
		"         No checks are made that these ARE COPIES of file X!!\n"
		"     -a  Program checks all names in numbered sequence.\n"
		"         In standard case, once a numbered file is missing,\n"
		"         program checks for %d more named files before halting.\n"
		"         Setting -a flag forces program to search for all possible\n"
		"         duplicate filenames. This may take some time.\n",COPYDEL_OVERMAX);

	} else if(!strcmp(str,"chans")) {
    	fprintf(stdout,
		"EXTRACT OR CONVERT CHANNELS OF SOUNDFILE.\n\n"
		"USAGE: housekeep chans 1 infile channo\n"
		"OR:    housekeep chans 2 infile \n"
		"OR:    housekeep chans 3 infile outfile channo\n"
		"OR:    housekeep chans 4 infile outfile [-p]\n"
		"OR:    housekeep chans 5 infile outfile\n\n"
		"MODES ARE\n"
		"1) EXTRACT A CHANNEL\n"
		"         channo is channel to extract\n" 
		"         outfile is named inname_c1 (for channel 1) etc.\n"
		"2) EXTRACT ALL CHANNELS\n"
		"         outfiles are named inname_c1 (for channel 1) etc.\n"
		"3) ZERO ONE CHANNEL\n"
		"         channo is channel to zero\n" 
		"         mono file goes to just one side of stereo outfile.\n"
		"         multichannel files have one channel zeroed out.\n"
		"4) MIX DOWN TO MONO\n"
		"         -p inverts phase of 2nd stereo channel before mixing.\n" 
		"         (phase inversion has no effect with non-stereo files).\n" 
		"5) MONO TO STEREO\n"
		"         Creates a 2-channel equivalent of mono infile.\n"); 
	} else if(!strcmp(str,"bundle")) {
    	fprintf(stdout,
		"LIST FILENAMES IN TEXTFILE FOR SORTING, OR MIXDUMMY.\n\n"
		"USAGE: housekeep bundle mode infile [infile2....] outtextfile\n\n"
		"MODES ARE\n"
		"1) BUNDLE ALL ENTERED FILES\n"
		"2) BUNDLE ALL NON-TEXT FILES ENTERED\n"
		"3) BUNDLE ALL NON-TEXT FILES OF SAME TYPE AS FIRST NON-TEXT FILE\n"
		"           e.g. all sndfiles, or all analysis files....\n"
		"4) AS (3), BUT ONLY FILES WITH SAME PROPERTIES\n"
		"5) AS (4), BUT IF FILE1 IS SNDFILE, FILES WITH SAME CHAN COUNT ONLY\n");
	} else if(!strcmp(str,"sort")) {
    	fprintf(stdout,
		"SORT FILES LISTED IN A TEXTFILE.\n\n"
		"USAGE: housekeep sort 1   listfile\n"
		"OR:    housekeep sort 2-3 listfile small large step [-l]\n"
		"OR:    housekeep sort 4   listfile [-l]\n"
		"OR:    housekeep sort 5   listfile\n"
		"LISTFILE is a textfile, a list of names of files to be sorted.\n\n"
		"MODES ARE\n"
		"1) SORT BY FILETYPE\n"
		"      Sorts files into different types.\n"
		"      Lists appropriate filenames in textfiles with extensions.. \n"
		"      .mot  mono sndfiles           .stt  stereo sndfiles\n"
		"      .qtt  4-channel sndfiles      .ant  analysis files\n"
		"      .pct  binary pitchdata files  .fot  binary formant files\n"
		"      .ent  binary envelope files   .trt  binary transposition files\n"
		"      .ott  any other files\n"
		"2) SORT BY SRATE\n"
		"      Sorts any soundfiles to different sampling rates.\n"
		"      Lists appropriate filenames in textfiles with extensions.. \n"
		"      .24  for 24000            .44  for 44100              ETC\n"
		"3) SORT BY DURATION\n"
		"      Sorts any soundfiles to different lengths.\n"
		"      If the listfile is 'list' or 'list.xxx' sorted file is 'list.len'.\n"
		"      SMALL is max size of smallest files. (secs)\n"
		"      LARGE is min size of largest files. (secs)\n"
		"      STEP  is size-steps between file types. (secs)\n"
		"      -l causes file durations not to be written to outfile.\n"
		"4) SORT BY LOG DURATION\n"
		"      The same, except STEP is duration ratio between file types.\n"
		"      -l causes file durations not to be written to outfile.\n"
		"5) SORT INTO DURATION ORDER\n"
		"      Sorts any sndfiles into duration order.\n"
		"      If the listfile is 'list' or 'list.xxx' sorted file is 'list.len'.\n"
		"      -l causes file durations not to be written to outfile.\n"
		"6) FIND ROGUES\n"
		"      Sort out any non- or invalid soundfiles.\n");
	} else if(!strcmp(str,"respec")) {
    	fprintf(stdout, 
    	"ALTER THE SPECIFICATION OF A SOUNDFILE.\n\n"
    	"USAGE: housekeep respec 1 infile outfile new_samplerate\n"
		"OR:    housekeep respec 2 infile outfile\n"
		"OR:    housekeep respec 3 infile outfile [-ssrate] [-cchannels]\n"
		"MODES ARE\n\n"
		"1) RESAMPLE  at some different sampling rate.\n"
		"           New sampling rate must be one of...\n"
		"           96000,88200,48000,24000,44100,22050,32000,16000\n\n"
		"2) CONVERT FROM INTEGER TO FLOAT SAMPLES, OR VICE VERSA\n\n"
		"3) CHANGE PROPERTIES OF SOUND: (USE WITH CAUTION!!)\n"
        "      SRATE is the new sample rate to impose.\n"
		"           NB: this does NOT RESAMPLE the data.\n"
		"           Simply causes original data to be read at different srate.\n"
		"           Sound has same no of samples, but different duration,\n"
		"           and will appear to be transposed in pitch.\n"
        "      CHANNELS is the new channel count to impose.\n"
		"           NB: this does NOT RECHANNEL the data.\n"
		"           Hence e.g. a stereo file will appear twice as long.\n"
		);
	} else if(!strcmp(str,"extract")) {
//TW further reformatted: fits on one screen
		fprintf(stdout,
		"USAGE: housekeep extract \n"
		"       1 inf [-ggate] [-sS] [-eE] [-tT] [-hH] [-bB] [-iI] [-lL] [-wW] [-n]\n"
    	"OR:    2 inf outf             OR: 3 inf outf [-ggate] [-ssplice] [-b] [-e]\n"
    	"OR:    4 inf outf shift       OR: 5 inf outf valsfile\n"
    	"OR:    6 inf gate endgate threshold baktrak initlevel minsize gatewin\n"
		"MODE 1) CUT OUT & KEEP SIGNIFICANT EVENTS FROM INPUT SNDFILE.\n"
		"-g  GATE level (G) above which sounds accepted : Range:0-1 (default 0).\n"
		"-s  SPLICE LENGTH in mS (default 15mS)\n"
		"-e  END-CUTOFF level (E) below which END of sound cut.(If 0, defaults to GATE).\n"
		"-t  THRESHOLD (T) If segment level never exceeds threshold,not kept(default 0)\n"
		"-h  HOLD sound till S sectors BEFORE START of next segment.(default 0)\n"
		"-b  KEEP B sects prior to gate-on, if level there > I (see below). Max B is %d.\n"
		"-i  INITIAL level (I). Use with -b flag.\n"
		"-l  Min LENGTH of events to keep (secs).\n"
		"-w  GATE_WINDOW Gates off only if level < gate for W+1 sectors. (default 0).\n"
		"-n  STOP if NAME of EXISTING sndfile generated. Default: ignore and continue.\n"
		"MODE 2) PREVIEW: make envel as 'sound'. View to get params for CUT OUT & KEEP.\n"
		"MODE 3) TOP AND TAIL: REMOVE LOW LEVEL SIGNAL FROM START & END OF SOUND.\n"
		" GATE   level ABOVE which signal accepted : (Range 0-1 : default 0).\n"
		" SPLICE length in mS (default 15mS) : -b Don't trim start : -e Don't trim end.\n"
		"MODE 4) RECTIFY: SHIFT ENTIRE SIGNAL TO ELIMINATE DC DRIFT.\n"
		"MODE 5) MODIFY 'BY HAND'. This process is no longer available.\n"
		"MODE 6) GET ONSET TIMES:gate,end-cutoff,thresh,keep,init-lvl,minlen,gate-windw.\n",CUTGATE_MAXBAKTRAK);
	} else if(!strcmp(str,"endclicks")) {
		fprintf(stdout,
		"REMOVE CLICKS FROM START OR END OF FILE.\n\n"
		"USAGE: housekeep endclicks infile outfile gate splicelen [-b] [-e]\n"
		"GATE   level ABOVE which signal accepted : (Range 0-1).\n"
		"SPLICE length in mS\n"
		"-b     Trim start\n"
		"-e     Trim end.\n");
	} else if(!strcmp(str,"bakup")) {
		fprintf(stdout,
		"CONCATENATE SOUNDFILES IN ONE BAKUP FILE, WITH SILENCES BETWEEN\n\n"
     	"USAGE: housekeep bakup infile1 [infile2 ...] outfile\n\n");
	} else if(!strcmp(str,"gate")) {
		fprintf(stdout,
		"CUT FILE AT ZERO AMPLITUDE POINTS.\n\n"
		"USAGE: housekeep gate infile outfile [-zzerocnt]\n"
  		"\n"
		"zerocnt: number of consecutive zero samples (per channel)\n"
		"         to indicate a silent gap in sound, where it can be cut.\n");

	} else if(!strcmp(str,"disk")) {
    	fprintf(stdout,
		"DISPLAY AVAILABLE SPACE ON DISK.\n\n"
		"USAGE: housekeep disk anyinfile\n\n"
		"If a sndfile is used, its samplerate will govern space calculations.\n");
	} else if(!strcmp(str,"batchexpand")) {
    	fprintf(stdout,
		"EXPAND AN EXISTING BATCHFILE.\n\n"
		"USAGE: housekeep batchexpand mode batch snd1 [snd2 ...] datafile ic oc pc\n\n"
		"BATCH      a batchfile, which uses same kind of param in one of its columns.\n"
		"SND1,2...  any number of soundfiles\n"
		"IC  is column in batchfile containing input file.\n"
		"OC  is column in batchfile containing  output file.\n"
		"PC  is column in batchfile containing parameter to be replaced.\n"
		"DATAFILE  of param values where 1st param applies to 1st sndfile, 2nd param to 2nd.\n"
		"New batchfile created, modelled on original........\n"
		"IN MODE 1:\n"
		"a) Existing batchfile has only 1 input file.\n"
		"b) Same sequence of operations applied to each in-sndfile using each new param.\n"
		"(So a 3 line file, with 3 sound inputs, becomes a 9-line file)\n"
		"IN MODE 2:\n"
		"a) Existing batchfile can operate on different soundfiles.\n"
		"Soundfiles in orig batchfile replaced by chosen files, and using new params.\n"
		"(So a 3 line file, with 3 sound inputs, becomes a 3-line file)\n");
	} else if(!strcmp(str,"deglitch")) {
    	fprintf(stdout,
		"ATTEMPT TO DEGLITCH A SOUND.\n\n"
		"USAGE: housekeep deglitch inf outf glitch sil thresh splice window [-s]\n\n"
		"GLITCH maximum duration (ms) of any glitches to find.\n"
		"SIL    minimum duration (ms) of 'silence' on either side of glitch.\n"
		"THRESH maximum level of 'silence' on either side of glitch.\n"
		"SPLICE splicelength (ms) to cutout glitch. Must be < half SILDUR)\n"
		"       with THRESH 0 use SPLICE 0. Otherwise SPLICE 0 makes clicks.\n"
		"WINDOW windowlen (ms) in which to search for glitches, & 'silence'.\n"
		"        very short windows may mistake parts of waveform for 'silence'.\n"
		"        Use larger windows to see larger features.\n"
		"-s     See details of (possible) glitches found.\n");
	} else
		fprintf(stdout,"Unknown option '%s'\n",str);
	return(USAGE_ONLY);
}

/******************************** USAGE3 ********************************/

int usage3(char *str1,char *str2)
{
	if(!strcmp(str1,"remove"))		
		return(CONTINUE);
	else if(!strcmp(str1,"disk"))		
		return(CONTINUE);
//TW NEW CASE
	else if(!strcmp(str1,"gate"))		
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

/******************************** READ_BATCHEXPAND_PARAMS ********************************/

int read_batchexpand_params(char *filename,dataptr dz)
{
	FILE *fp;
	int cnt = 0;
	char temp[200], *p, *q;

	if((fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,	"Can't open file %s to read data.\n",filename);
		return(DATA_ERROR);
	}
	while(fgets(temp,200,fp)!=NULL) {
		p = temp;
		while(get_word_from_string(&p,&q)) {
			if((dz->wordstor[dz->all_words] = (char *)malloc((strlen(q) + 1) * sizeof(char)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY for wordstor %d\n",dz->all_words+1);
				return(MEMORY_ERROR);
			}
			strcpy(dz->wordstor[dz->all_words],q);
			dz->all_words++;
			cnt++;
			if(cnt > dz->itemcnt) {
				sprintf(errstr,"NUMBER OF DATA PARAMETERS is greater than NUMBER OF INPUT SOUNDFILES\n");
				return(DATA_ERROR);
			}
		}
	}			
	if(cnt < dz->itemcnt) {
		sprintf(errstr,"NUMBER OF DATA PARAMETERS is less than NUMBER OF INPUT SOUNDFILES\n");
		return(DATA_ERROR);
	}
	return(FINISHED);
}

//TW UPDATE : NEW FUNCTION (modified for flotsams)
/*************************** CREATE_BAKUP_SNDBUFS **************************/

int create_bakup_sndbufs(dataptr dz)
{
	int n;
	int insert_samps, insert_secs;
	size_t bigbufsize;
    int framesize = F_SECSIZE * dz->infile->channels;
	
	insert_samps = dz->infile->srate * dz->infile->channels;	/* 1 second in samps */
	insert_samps = (int)round(BAKUP_GAP * insert_samps);
	if((insert_secs = insert_samps/framesize) * framesize != insert_samps)
		insert_secs++;
	insert_samps = insert_secs * framesize;
	if(dz->sbufptr == 0 || dz->sampbuf == 0) {
		sprintf(errstr,"buffer pointers not allocated: create_bakup_sndbufs()\n");
		return(PROGRAM_ERROR);
	}
	bigbufsize = (size_t) Malloc(-1);
	bigbufsize /= dz->bufcnt;
	dz->buflen = (int)(bigbufsize/sizeof(float));
	if((dz->buflen = (dz->buflen/framesize) * framesize)<=insert_samps) {
		dz->buflen  = insert_samps;
	}
	if((dz->bigbuf = (float *)malloc((size_t)(dz->buflen * sizeof(float) * dz->bufcnt))) == NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
		return(PROGRAM_ERROR);
	}
	for(n=0;n<dz->bufcnt;n++)
		dz->sbufptr[n] = dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
	dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
	return(FINISHED);
}

/****************************** BATCHEXPAND_CONSISTENCY *********************************/

int batchexpand_consistency(dataptr dz)
{
	int n, entrycnt, basecnt;
	char paramval[200], infname[200];
	if (dz->iparam[BE_INFILE] >= dz->iparam[BE_OUTFILE]) {
		sprintf(errstr,"Infile and Outfile column numbers are inconsistent.\n");
		return(DATA_ERROR);
	}
	if (dz->iparam[BE_OUTFILE] >= dz->iparam[BE_PARAM]) {
		sprintf(errstr,"Outfile and Param column numbers are inconsistent.\n");
		return(DATA_ERROR);
	}
	entrycnt = dz->wordcnt[0];
	if(entrycnt < 5) {
		sprintf(errstr,"First file is not a batchfile.\n");
		return(DATA_ERROR);
	}
	if(entrycnt < dz->iparam[BE_INFILE]) {
		sprintf(errstr,"Batchfile has insufficient columns to correspond to Infile column number.\n");
		return(DATA_ERROR);
	}
	if(entrycnt < dz->iparam[BE_OUTFILE]) {
		sprintf(errstr,"Batchfile has insufficient columns to correspond to Outfile column number.\n");
		return(DATA_ERROR);
	}
	if(entrycnt < dz->iparam[BE_PARAM]) {
		sprintf(errstr,"Batchfile has insufficient columns to correspond to Parameter column number.\n");
		return(DATA_ERROR);
	}
	dz->iparam[BE_INFILE]--;
	dz->iparam[BE_OUTFILE]--;
	dz->iparam[BE_PARAM]--;
	if(dz->mode==ONE_PARAM) {
		strcpy(paramval,dz->wordstor[BE_PARAM]);
		strcpy(infname,dz->wordstor[BE_INFILE]);
	}
	basecnt = entrycnt;
	for(n = 1; n < dz->linecnt; n++) {
		if(entrycnt != dz->wordcnt[n]) {
			sprintf(errstr,"First file is not a batchfile.\n");
			return(DATA_ERROR);
		}
		if(dz->mode==ONE_PARAM && (strcmp(paramval,dz->wordstor[BE_PARAM + basecnt]))) {
			sprintf(errstr,"Specified batchfile parameter is not the same in all lines of the original batchfile\n");
			return(DATA_ERROR);
		}
		if(dz->mode==ONE_PARAM && (strcmp(infname,dz->wordstor[BE_INFILE + basecnt]))) {
			sprintf(errstr,"Infile is not the same in all lines of the original batchfile\n");
			return(DATA_ERROR);
		}
		basecnt += entrycnt;
	}
	return(FINISHED);
}

/******************************** READ_INDIVIDUAL_SAMPLE_VALUES ********************************/

int read_individual_sample_values(char *filename,dataptr dz)
{
	FILE *fp;
	int cnt = 0;
	char temp[200], *p;
	double val;
	double lasttime = 0.0, *la;

	if((fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,	"Can't open file %s to read data.\n",filename);
		return(DATA_ERROR);
	}
	if((la = (double *)malloc(sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT_MEMORY to store sample value information.\n");
		return(MEMORY_ERROR);
	}
	while(fgets(temp,200,fp)!=NULL) {
		p = temp;
		while(get_float_from_within_string(&p,&val)) {
			if(cnt > 0) {
				if((la = (double*)realloc((char *)la,(cnt+1) * sizeof(double)))==NULL) {
					sprintf(errstr,"INSUFFICIENT_MEMORY to store sample value information.\n");
					return(MEMORY_ERROR);
				}
			}
			la[cnt] = (double)val;
			if(EVEN(cnt)) {
				if(cnt == 0)
					lasttime = la[cnt];
				else if(la[cnt] <= lasttime) {
					sprintf(errstr,"Sample positions not in ascending order.\n");
					return(DATA_ERROR);
				}
			} else {
				if((la[cnt] < (double)MINSAMP) || (la[cnt] > (double)MAXSAMP)) {
					sprintf(errstr,"Sample value %lf is out of range (%lf to %lf).\n",
					la[cnt],(double)MINSAMP,(double)MAXSAMP);
					return(DATA_ERROR);
				}
			}
			cnt++;
		}
	}
	if(ODD(cnt)) {
		sprintf(errstr,"Data values not paired correctly.\n");
		return(DATA_ERROR);
	}
	dz->parray[0] = la;
	dz->parray[1] = la + cnt;
	return(FINISHED);
}
