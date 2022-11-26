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



/* May 2011 rebuilt with fixed mainfuncs.c for preserving sample type etc */
#include <stdio.h>
#include <stdlib.h>
#include <structures.h>
#include <cdpmain.h>
#include <tkglobals.h>
#include <pnames.h>
#include <pvoc.h>
#include <processno.h>
#include <modeno.h>
#include <globcon.h>
#include <logic.h>
#include <filetype.h>
#include <mixxcon.h>
#include <speccon.h>
#include <flags.h>
#include <arrays.h>
#include <formants.h>
#include <sfsys.h>
#include <osbind.h>
#include <string.h>
#include <math.h>

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
	case(PVOC_ANAL):			dz->extra_bufcnt = 0;	dz->bufcnt = 0;		break;
	case(PVOC_SYNTH):			dz->extra_bufcnt = 0;	dz->bufcnt = 0;		break;
	case(PVOC_EXTRACT):			dz->extra_bufcnt = 0;	dz->bufcnt = 0;		break;
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
	case(PVOC_ANAL):    dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0; break;
	case(PVOC_SYNTH):   dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0; break;
	case(PVOC_EXTRACT): dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0; break;
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
	case(PVOC_ANAL):	  setup_process_logic(SNDFILES_ONLY,		BIG_ANALFILE,		ANALFILE_OUT,	dz);	break;
	case(PVOC_SYNTH):	  setup_process_logic(ANALFILE_ONLY,		UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(PVOC_EXTRACT):	  setup_process_logic(SNDFILES_ONLY,		UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
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
		case(PVOC_ANAL):	exit_status = set_internalparam_data("000iiiiiiiiiiii",ap);	break;
		case(PVOC_SYNTH):	exit_status = set_internalparam_data("000iiiiiiiiiiii",ap);	break;
		case(PVOC_EXTRACT):	exit_status = set_internalparam_data(   "iiiiiiiiiiii",ap);	break;
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
	switch(dz->process) {
	case(PVOC_ANAL):			
	case(PVOC_SYNTH):			
	case(PVOC_EXTRACT):			
		return pvoc_preprocess(dz);
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
	switch(dz->process) {
	case(PVOC_ANAL):
	case(PVOC_SYNTH):
	case(PVOC_EXTRACT):	return pvoc_process(dz);
	default:
		sprintf(errstr,"Unknown case in process_file()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);	/* NOTREACHED */
}

/********************************************************************************************/
/********************************** FORMERLY IN buffers.c ***********************************/
/********************************************************************************************/

/**************************** ALLOCATE_LARGE_BUFFERS ******************************/

int allocate_large_buffers(dataptr dz)
{
	switch(dz->process) {
	case(PVOC_ANAL):			
	case(PVOC_SYNTH):			
	case(PVOC_EXTRACT):			
		return FINISHED;
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
	if     (!strcmp(prog_identifier_from_cmdline,"anal"))			dz->process = PVOC_ANAL;
	else if(!strcmp(prog_identifier_from_cmdline,"synth"))			dz->process = PVOC_SYNTH;
	else if(!strcmp(prog_identifier_from_cmdline,"extract"))		dz->process = PVOC_EXTRACT;
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
	"USAGE: pvoc NAME (mode) infile outfile (parameters)\n"
	"\n"
	"where NAME can be any one of\n"
	"\n"
	"anal   synth 	extract\n"
	"\n"
	"Type 'pvoc anal'  for more info on pvoc anal option... ETC.\n");
	return(USAGE_ONLY);
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
	if(!strcmp(str,"anal")) {		 
		sprintf(errstr,
	    "CONVERT SOUNDFILE TO SPECTRAL FILE\n\n"
		"USAGE: pvoc anal  mode infile outfile [-cpoints] [-ooverlap]\n\n"
		"MODES ARE....\n"
		"1) STANDARD ANALYSIS\n"
		"2) OUTPUT SPECTRAL ENVELOPE VALS ONLY\n"
		"3) OUTPUT SPECTRAL MAGNITUDE VALS ONLY\n"
		"POINTS   No of analysis points (2-32768 (power of 2)): default 1024\n"
		"         More points give better freq resolution\n"
		"         but worse time-resolution (e.g. rapidly changing spectrum).\n"
		"OVERLAP  Filter overlap factor (1-4): default 3\n");
	} else if(!strcmp(str,"synth")) {		 
		sprintf(errstr,
	    "CONVERT SPECTRAL FILE TO SOUNDFILE\n\n"
		"USAGE: pvoc synth infile outfile\n");
	} else if(!strcmp(str,"extract")) {		 
		sprintf(errstr,
	    "ANALYSE THEN RESYNTHESIZE SOUND WITH VARIOUS OPTIONS\n\n"
		"USAGE: pvoc extract infile outfile\n"
		"       [-cpoints] [-ooverlap] [-ddochans] [-llochan] [-hhichan]\n\n"
		"POINTS   No of analysis points (2-32768 (power of 2)): default 1024\n"
		"         More points give better freq resolution\n"
		"         but worse time-resolution (e.g. rapidly changing spectrum).\n"
		"OVERLAP  Filter overlap factor (1-4): default 3\n"
		"DOCHANS  resynthesize odd (1) or even (2) channels only.\n"
		"LOCHAN   ignore analysis channels below this in resynth (default: 0)\n"
		"HICHAN   ignore analysis channels above this in resynth (dflt: highest channel)\n"
		"LOCHAN and HICHAN refer to channels rather than analysis points.\n"
		"         There is 1 channel for every 2 points.\n"
		"         HICHAN should therefore not be > ANALYSIS POINTS/2\n"
		"         To default to topmost chan, set at ZERO.\n\n"
		"NB If no flags are set, the output sound will be the same as the input.\n");
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

/********************************************************************************************/
/********************************** FORMERLY IN pconsistency.c ******************************/
/********************************************************************************************/

/******************** CHECK_PARAM_VALIDITY_AND_CONSISTENCY (redundant) *************************/

int check_param_validity_and_consistency(dataptr dz)
{
	int exit_status, chans;
	int chancnt, Nchans;
	int M, D, win_overlap;
	float arate;
	int srate;

	switch(dz->process) {
	case(PVOC_ANAL):
		chans = dz->infile->channels;
		srate = dz->infile->srate;
		win_overlap = dz->iparam[PVOC_WINOVLP_INPUT]-1;
		chancnt = dz->iparam[PVOC_CHANS_INPUT];
		chancnt = chancnt + (chancnt%2);
		switch(win_overlap) {
			case 0:	M = 4*chancnt;		break;
			case 1:	M = 2*chancnt;		break;
			case 2: M = chancnt;		break;
			case 3: M = chancnt / 2;	break;
			default:
				sprintf(errstr,"pvoc: Invalid window overlap factor.\n");
				return(PROGRAM_ERROR);
		}
		if((D = (int)(M/PVOC_CONSTANT_A)) == 0){
			fprintf(stdout,"WARNING: Decimation too low: adjusted.\n");
			fflush(stdout);
			D = 1;
		}
		Nchans = chancnt + 2;
		if(dz->mode == ENVEL_ONLY || dz->mode == MAG_ONLY) {
			Nchans /= 2;
			arate = (float)(Nchans * dz->infile->srate)/(float)D;
		} else
			arate = (float)dz->infile->srate/(float)D;

		dz->infile->srate    = (int)arate;
		dz->infile->channels = chancnt + 2;
		if((exit_status = create_sized_outfile(dz->wordstor[0],dz))<0)
			return(exit_status);
		dz->infile->channels = chans;
		dz->infile->srate    = srate;
		break;
	case(PVOC_SYNTH):
		chans = dz->infile->channels;
		srate = dz->infile->srate;
		dz->infile->channels = 1;
		dz->infile->srate = dz->infile->origrate; 
		/*RWD OCT 05 set outfile stype to infile origstype */
        /* RWD Apr 2011 NB: will get changed if -f flag used */
		dz->outfile->stype = dz->infile->origstype;
		if((exit_status = create_sized_outfile(dz->wordstor[0],dz))<0)
			return(exit_status);
		dz->infile->channels = chans;
		dz->infile->srate = srate;
		break;
	}
	return (FINISHED);
}

/***************************** INNER_LOOP (redundant) **************************/

int inner_loop
(int *peakscore,int *descnt,int *in_start_portion,int *least,int *pitchcnt,int windows_in_buf,dataptr dz)
{
	return(FINISHED);
}

