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
#include <combine.h>
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
#include <srates.h>

/********************************************************************************************/
/********************************** FORMERLY IN preprocess.c ********************************/
/********************************************************************************************/

static int  force_file_zero_to_be_largest_file(dataptr dz);
static int  mean_preprocess(dataptr dz);
static void integer_swap(int *a,int *b);
static int allocate_specleaf_buffer(dataptr dz);

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
//TW NEW CASE
	case(MAKE2):

	case(MAKE):	      		dz->extra_bufcnt =  0; dz->bptrcnt = 4; 	is_spec = TRUE;		break;
	case(SUM):	      		dz->extra_bufcnt =  0; dz->bptrcnt = 4; 	is_spec = TRUE;		break;
	case(DIFF):	      		dz->extra_bufcnt =  0; dz->bptrcnt = 4; 	is_spec = TRUE;		break;
	case(LEAF):       		dz->extra_bufcnt =  0; dz->bptrcnt = 4; 	is_spec = TRUE;		break;
	case(MAX):	      		dz->extra_bufcnt =  0; dz->bptrcnt = 4; 	is_spec = TRUE;		break;
	case(MEAN):       		dz->extra_bufcnt =  3; dz->bptrcnt = 4; 	is_spec = TRUE;		break;	/* TW August 19 */
	case(CROSS):      		dz->extra_bufcnt =  0; dz->bptrcnt = 4; 	is_spec = TRUE;		break;
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
	dz->ptr_cnt    = -1;		//base constructor...process
	dz->array_cnt  = -1;
	dz->iarray_cnt = -1;
	dz->larray_cnt = -1;
	switch(dz->process) {
//TW NEW CASE
	case(MAKE2):
	case(MAKE):    	dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(SUM):	  	dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(DIFF):	  	dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(LEAF):	  	dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(MAX): 	  	dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(MEAN):   	dz->array_cnt = 0; dz->iarray_cnt = 2; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(CROSS):  	dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
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
	case(MAKE):			setup_process_logic(PITCH_AND_FORMANTS,   	PITCH_TO_ANAL,		ANALFILE_OUT,	dz);	break;
//TW NEW CASE
	case(MAKE2):		setup_process_logic(PFE,   					PITCH_TO_ANAL,		ANALFILE_OUT,	dz);	break;
	case(SUM):			setup_process_logic(TWO_ANALFILES,		  	MAX_ANALFILE,		ANALFILE_OUT,	dz);	break;
	case(DIFF):		  	setup_process_logic(TWO_ANALFILES,		  	EQUAL_ANALFILE,		ANALFILE_OUT,	dz);	break;
	case(LEAF):		  	setup_process_logic(MANY_ANALFILES,		  	MIN_ANALFILE,		ANALFILE_OUT,	dz);	break;
	case(MAX):		  	setup_process_logic(MANY_ANALFILES,		  	MAX_ANALFILE,		ANALFILE_OUT,	dz);	break;
	case(MEAN):		  	setup_process_logic(TWO_ANALFILES,		  	MIN_ANALFILE,		ANALFILE_OUT,	dz);	break;
	case(CROSS):	  	setup_process_logic(TWO_ANALFILES,		  	MIN_ANALFILE,		ANALFILE_OUT,	dz);	break;
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
//TW NEW CASE
	case(MAKE2):
	case(MAKE):       	return(FINISHED);
	case(SUM):		  	return(FINISHED);
	case(DIFF):		  	return(FINISHED);
	case(LEAF):		  	return(FINISHED);
	case(MAX):		  	return(FINISHED);
	case(MEAN):		  	exit_status = set_internalparam_data("ii",ap);					break;
	case(CROSS):	  	return(FINISHED);
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
//TW AVOIDS WARNING
	//str = str;

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
	case(MAX):			return force_file_zero_to_be_largest_file(dz);
	case(MEAN):			return mean_preprocess(dz);

//TW NEW CASE
	case(MAKE2):
	case(MAKE):	case(SUM):	case(DIFF):	case(LEAF):	case(CROSS):		
		return(FINISHED);
	default:
		sprintf(errstr,"PROGRAMMING PROBLEM: Unknown process in param_preprocess()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);	/* NOTREACHED */
}

/********************** FORCE_FILE_ZERO_TO_BE_LARGEST_FILE **********************/

int force_file_zero_to_be_largest_file(dataptr dz)
{
	int maxsize = dz->insams[0];
	int maxfile = 0, n, k;
	for(n=1;n<dz->infilecnt;n++) {
		if(dz->insams[n] > maxsize) {
			maxsize = dz->insams[n];
			maxfile = n;
		}
	}
	if(n!=0) {		/* Force file zero to be largest file */
		k = dz->ifd[0];
		dz->ifd[0] = dz->ifd[maxfile];
		dz->ifd[maxfile] = k;
		k = dz->insams[0];
		dz->insams[0] = dz->insams[maxfile];
		dz->insams[maxfile] = k;
	}
	return(FINISHED);
}

/************************** MEAN_PREPROCESS ******************************/

int mean_preprocess(dataptr dz)
{
	int exit_status;
	if(dz->zeroset) {
		sprintf(errstr,"zeroset flag set: Needed unset for later use: mean_preprocess()\n");
		return(PROGRAM_ERROR);
	}
	if((dz->iparray[MEAN_LOC1] = (int *)malloc(dz->clength * sizeof(int)))==NULL
	|| (dz->iparray[MEAN_LOC2] = (int *)malloc(dz->clength * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for mean location arrays.\n");
		return(MEMORY_ERROR);
	}
	if((exit_status = get_channel_corresponding_to_frq(&(dz->iparam[MEAN_BOT]),dz->param[MEAN_LOF],dz))<0)
		return(exit_status);
	if((exit_status = get_channel_corresponding_to_frq(&(dz->iparam[MEAN_TOP]),dz->param[MEAN_HIF],dz))<0)
		return(exit_status);
	if(dz->iparam[MEAN_TOP] < dz->iparam[MEAN_BOT])
		integer_swap(&(dz->iparam[MEAN_TOP]),&(dz->iparam[MEAN_BOT]));
	if(dz->iparam[MEAN_TOP] - dz->iparam[MEAN_BOT] + 1 < dz->iparam[MEAN_CHAN])
		/* Reducing chans parameter to fit into frq limits. */
		dz->iparam[MEAN_CHAN] = dz->iparam[MEAN_TOP] - dz->iparam[MEAN_BOT] + 1;
	dz->iparam[MEAN_TOP] *= 2; /* CONVERT FROM CHANNEL-NO TO FLOAT-NO IN BUFFER */
	dz->iparam[MEAN_BOT] *= 2;
	dz->iparam[MEAN_TOP] += 2; /* INCLUSIVE LOOP LIMIT */
	dz->iparam[MEAN_TOP] = min(dz->iparam[MEAN_TOP],dz->wanted);
	return(FINISHED);
}

/***************************** INTEGER_SWAP **************************/

void integer_swap(int *a,int *b)
{
	int k;
	k  = *a;
	*a = *b;
	*b = k;
}

/***************************** INNER_LOOP (redundant) **************************/

int inner_loop
(int *peakscore,int *descnt,int *in_start_portion,int *least,int *pitchcnt,int windows_in_buf,dataptr dz)
{
	return(FINISHED);
}

/***************** SKIP_OR_SPECIAL_OPERATION_ON_WINDOW_ZERO ************/

int skip_or_special_operation_on_window_zero(dataptr dz)
{
	return(FALSE);
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
//TW NEW CASE
	case(MAKE2):
	case(MAKE):		return specmake(dz);		   
	case(SUM):		return outer_twofileinput_loop(dz);
	case(DIFF):		return outer_twofileinput_loop(dz);
	case(LEAF):		return specleaf(dz);
	case(MAX):		return specmax(dz);
	case(MEAN):		return outer_twofileinput_loop(dz);
	case(CROSS):	return outer_twofileinput_loop(dz);
	default:
		sprintf(errstr,"Unknown process in procspec()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);	/* NOTREACHED */
}

/**************************** OUTER_TWOFILEINPUT_LOOP ***************************/

int outer_twofileinput_loop(dataptr dz)
{
	int exit_status;
	int windows_to_process, windows_in_buf, samps_read, samps_to_write, wc, got;
	int file_to_keep = 0, finished = 0, stop_at_end_of_shortest_file = 0;
	int stop_at_end_of_process = 0;
	dz->time = 0.0f;
	switch(dz->process) {
	case(CROSS):
	case(MEAN):
		windows_to_process = min(dz->insams[0],dz->insams[1])/dz->wanted;
		stop_at_end_of_shortest_file = 1;
		break;
	case(SUM):
		windows_to_process = min(dz->insams[0],dz->insams[1])/dz->wanted;
		file_to_keep = get_longer_file(dz);
		break;
	case(DIFF):
		windows_to_process = min(dz->insams[0],dz->insams[1])/dz->wanted;
		if(dz->insams[0] > dz->insams[1])
			file_to_keep = 1;
		break;
	default:
		sprintf(errstr,"unknown case in outer_twofileinput_loop()\n");
		return(PROGRAM_ERROR);
	}
   	while(!finished) {

		if((exit_status = read_both_files(&windows_in_buf,&got,dz))<0)
			return(exit_status);
		samps_to_write = got;
 		for(wc=0; wc<windows_in_buf; wc++) {
			if(dz->total_windows==0) {
				if((exit_status = skip_or_special_operation_on_window_zero(dz))<0)
				 	return(exit_status);
				if(exit_status==TRUE) {
					if((exit_status = advance_one_2fileinput_window(dz))<0)
						return(exit_status);
					continue;
				}
			}
			if((exit_status = read_values_from_all_existing_brktables((double)dz->time,dz))<0)
				return(exit_status);
			switch(dz->process) {
			case(CROSS):  
				if((exit_status = speccross(dz))<0)
					return(exit_status);
				break;
			case(DIFF):	  
				if((exit_status = specdiff(dz))<0)
					return(exit_status);
				break;
			case(MEAN):   
				if((exit_status = specmean(dz))<0)
					return(exit_status);
				break;
			case(SUM):	  
				if((exit_status = specsum(dz))<0)
					return(exit_status);
				break;
			default:
				sprintf(errstr,"unknown process in outer_twofileinput_loop()\n");
				return(PROGRAM_ERROR);
			}
			if((exit_status = advance_one_2fileinput_window(dz))<0)
				return(exit_status);
			if(dz->total_windows >= windows_to_process) {
				if((exit_status = keep_excess_samps_from_correct_file(&samps_to_write,file_to_keep,got,wc,dz))<0)
					return(exit_status);
				finished = 1;
				break;
			}
		}
		if(samps_to_write > 0) { 
			if((exit_status = write_samps(dz->bigfbuf,samps_to_write,dz))<0)
				return(exit_status);
		}
	}
	if(!stop_at_end_of_process && !stop_at_end_of_shortest_file) {
		if((exit_status = read_either_file(&samps_read,file_to_keep,dz))<0)
			return(exit_status);
		while(samps_read > 0) {
			if((exit_status = write_samps(dz->bigfbuf,samps_read,dz))<0)
				return(exit_status);
			if((exit_status = read_either_file(&samps_read,file_to_keep,dz))<0)
				return(exit_status);
		}
	}
	return(FINISHED);
}

/********************************************************************************************/
/********************************** FORMERLY IN pconsistency.c ******************************/
/********************************************************************************************/

/****************************** CHECK_PARAM_VALIDITY_AND_CONSISTENCY *********************************/

int check_param_validity_and_consistency(dataptr dz)
{
	return(FINISHED);
}

/********************************************************************************************/
/********************************** FORMERLY IN buffers.c ***********************************/
/********************************************************************************************/

/**************************** ALLOCATE_LARGE_BUFFERS ******************************/

int allocate_large_buffers(dataptr dz)
{
	switch(dz->process) {
	case(SUM):		case(DIFF):
	case(MAX):		case(MEAN):		case(CROSS):	
		return allocate_double_buffer(dz);
	case(LEAF):		
		return allocate_specleaf_buffer(dz);
	case(MAKE):		
//TW NEW CASE
	case(MAKE2):		
		return allocate_analdata_plus_formantdata_buffer(dz);
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
	if     (!strcmp(prog_identifier_from_cmdline,"make"))  	   		dz->process = MAKE;
//TW NEW CASE
	else if(!strcmp(prog_identifier_from_cmdline,"make2"))  	   	dz->process = MAKE2;
	else if(!strcmp(prog_identifier_from_cmdline,"sum"))  	   		dz->process = SUM;
	else if(!strcmp(prog_identifier_from_cmdline,"diff"))  	   		dz->process = DIFF;
	else if(!strcmp(prog_identifier_from_cmdline,"interleave"))  	dz->process = LEAF;
	else if(!strcmp(prog_identifier_from_cmdline,"max"))  	   		dz->process = MAX;
	else if(!strcmp(prog_identifier_from_cmdline,"mean"))  	   		dz->process = MEAN;
	else if(!strcmp(prog_identifier_from_cmdline,"cross"))     		dz->process = CROSS;
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
	"\nCOMBINATION OF SPECTRAL FILES\n\n"
	"USAGE: combine NAME (mode) infile (infile2...) outfile parameters: \n"
	"\n"
	"where NAME can be any one of\n"
	"\n"
//TW ADDED NEW CASE
	"make    make2    sum    diff    interleave    max    mean    cross\n\n"
	"Type 'combine make' for more info on combine make..ETC.\n");

	return(USAGE_ONLY);
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
	if(!strcmp(str,"make")) {
		sprintf(errstr,
		"combine make pitchfile formantfile outfile\n\n"
		"GENERATE SPECTRUM FROM PITCH & FORMANT DATA ONLY.\n\n"
		"pitchfile   is a binary pitchdata file\n" 
		"formantfile is a binary formantdata file\n" 
		"outfile     is an analysis file which can be resynthesized with PVOC\n"
		"\n"
//TW temporary cmdline restriction
		"** Do not use output filenames which end in '1'\n");
//TW NEW CASE
	} else if(!strcmp(str,"make2")) {
		sprintf(errstr,
		"combine make2 pitchfile formantfile envfile outfile\n\n"
		"GENERATE SPECTRUM FROM PITCH, FORMANT AND ENVELOPE DATA ONLY.\n\n"
		"pitchfile   is a binary pitchdata file\n" 
		"formantfile is a binary formantdata file\n" 
		"envfile     is a binary envelope file derived from an ANALYSIS file\n" 
		"outfile     is an analysis file which can be resynthesized with PVOC\n");

} else if(!strcmp(str,"sum")) {
		sprintf(errstr,
		"combine sum infile infile2 outfile [-ccrossover]\n\n"
		"FIND SUM OF TWO SPECTRA.\n\n"
		"-c  CROSSOVER is amount of 2nd spectrum added to 1st (range 0 to 1)\n\n"
		"crossover may vary over time.\n");
	} else if(!strcmp(str,"diff")) {
		sprintf(errstr,
		"combine diff infile infile2 outfile [-ccrossover] [-a]\n\n"
		"FIND DIFFERENCE OF TWO SPECTRA.\n\n"
		"-c  CROSSOVER is amount of 2nd spectrum subtracted from 1st (range 0 to 1)\n"
		"-a  retains any subzero amplitudes produced (Default: sets these to zero).\n\n"
		"crossover may vary over time.\n");
	} else if(!strcmp(str,"interleave")) {
		sprintf(errstr,
		"combine interleave infile infile2 [infile3 ....] outfile leafsize\n\n"
		"INTERLEAVE WINDOWS FROM INFILES, LEAFSIZE WINDOWS PER LEAF.\n");
	} else if(!strcmp(str,"max")) {
		sprintf(errstr,
		"combine max infile infile2 [infile3 ....] outfile\n\n"
		"IN EACH ANALYSIS CHANNEL, IN EACH WINDOW, TAKE MAX VAL AMONGST INPUT FILES.\n");
	} else if(!strcmp(str,"mean")) {
		sprintf(errstr,
		"combine mean mode infile infile2 outfile [-llofrq] [-hhifrq] [-cchans] [-z]\n\n"
		"GENERATE SPECTRAL 'MEAN' OF 2 SOUNDS.\n\n"
		"LOFRQ 	is low freq limit of channels to look at.\n"
		"HIFRQ 	is high freq limit of channels to look at.\n"
		"CHANS	no. of significant channels to compare..Default: All within range.\n"
		"-z   	Zeroes channels OUTSIDE frq range specified.\n\n"
		"MODES are...\n"
		"(1) mean channel amp of 2 files :  mean of two pitches\n"
		"(2) mean channel amp of 2 files :  mean of two frqs\n"
		"(3) channel amp from file1      :  mean of two pitches\n"
		"(4) channel amp from file1      :  mean of two frqs\n"
		"(5) channel amp from file2      :  mean of two pitches\n"
		"(6) channel amp from file2      :  mean of two frqs\n"
		"(7) max channel amp of 2 files  :  mean of two pitches\n"
		"(8) max channel amp of 2 files  :  mean of two frqs\n");
	} else if(!strcmp(str,"cross")) {
		sprintf(errstr,
		"combine cross infile infile2 outfile [-iinterp]\n\n"
		"REPLACE SPECTRAL AMPLITUDES OF 1st FILE WITH THOSE OF 2nd.\n\n"
		"-i	INTERP is degree of replacement.\n\n"
		"    interp may vary over time.\n");
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

/**************************** ALLOCATE_SPECLEAF_BUFFER ******************************/

int allocate_specleaf_buffer(dataptr dz)
{
	unsigned int buffersize;
	if(dz->bptrcnt <= 0) {
		sprintf(errstr,"bufptr not established in allocate_specleaf_buffer()\n");
		return(PROGRAM_ERROR);
	}
	buffersize = dz->wanted * dz->iparam[LEAF_SIZE];
	dz->buflen = buffersize;
	buffersize += 1;
	if((dz->bigfbuf	= (float*) malloc(buffersize * sizeof(float)))==NULL) {  
		sprintf(errstr,"INSUFFICIENT MEMORY for sound buffers.\n");
		return(MEMORY_ERROR);
	}
	dz->big_fsize = dz->buflen;
	dz->bigfbuf[dz->big_fsize] = 0.0f;	/* safety value */
	return(FINISHED);
}

