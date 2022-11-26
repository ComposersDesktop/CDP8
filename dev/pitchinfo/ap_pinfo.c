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
#include <specpinfo.h>
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
/********************************** FORMERLY IN pconsistency.c ******************************/
/********************************************************************************************/

static int 	check_data_for_pwrite(dataptr dz);
static int	are_pitch_zeros(dataptr dz);

/********************************************************************************************/
/********************************** FORMERLY IN preprocess.c ********************************/
/********************************************************************************************/

static int  phear_preprocess(dataptr dz);
static int  establish_pinfo_internal_param_default_vals(dataptr dz);
static int  setup_internal_params_for_pwrite(dataptr dz);

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
	case(P_INFO):     		dz->extra_bufcnt =  0; dz->bptrcnt = 0; 	is_spec = TRUE;		break;
	case(P_ZEROS):    		dz->extra_bufcnt =  0; dz->bptrcnt = 0; 	is_spec = TRUE;		break;
	case(P_SEE):      		dz->extra_bufcnt =  0; dz->bufcnt = 1;	 	is_spec = FALSE;	break;
	case(P_HEAR):     		dz->extra_bufcnt =  3; dz->bptrcnt = 2; 	is_spec = TRUE;		break;
	case(P_WRITE):    		dz->extra_bufcnt =  0; dz->bptrcnt = 0; 	is_spec = TRUE;		break;
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
	case(P_INFO):   dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(P_ZEROS):  dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(P_SEE):    dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(P_HEAR): 	dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(P_WRITE):  dz->array_cnt =1;  dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
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
	case(P_INFO):	  	setup_process_logic(PITCHFILE_ONLY,		  	SCREEN_MESSAGE,		NO_OUTPUTFILE,	dz);	break;
	case(P_ZEROS):	  	setup_process_logic(PITCHFILE_ONLY,		  	SCREEN_MESSAGE,		NO_OUTPUTFILE,	dz);	break;
	case(P_SEE):	  	setup_process_logic(PITCH_OR_TRANSPOS,	  	PITCH_TO_PSEUDOSND,	SNDFILE_OUT,	dz);	break;
	case(P_HEAR):	  	setup_process_logic(PITCHFILE_ONLY,		  	PITCH_TO_ANAL,		ANALFILE_OUT,	dz);	break;
	case(P_WRITE):	  	setup_process_logic(PITCH_OR_TRANSPOS,	  	TO_TEXTFILE,		TEXTFILE_OUT,	dz);	break;
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
	case(P_INFO): 	  	exit_status = set_internalparam_data("ddddd",ap);				break;
	case(P_ZEROS):    	return(FINISHED);
	case(P_SEE):   	  	return(FINISHED);
	case(P_HEAR): 	  	return(FINISHED);
	case(P_WRITE):    	return(FINISHED);
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
	case(P_HEAR):	    return phear_preprocess(dz);
	case(P_INFO):		return establish_pinfo_internal_param_default_vals(dz);	
	case(P_WRITE):		return setup_internal_params_for_pwrite(dz);

	case(P_ZEROS):	case(P_SEE):		
		return(FINISHED);
	default:
		sprintf(errstr,"PROGRAMMING PROBLEM: Unknown process in param_preprocess()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);	/* NOTREACHED */
}

/************************** PHEAR_PREPROCESS ******************************/

int phear_preprocess(dataptr dz)
{
	int exit_status;
	if((exit_status = establish_testtone_amps(dz))<0)
		return(exit_status);
	return establish_bottom_frqs_of_channels(dz);
}

/********************** ESTABLISH_PINFO_INTERNAL_PARAM_DEFAULT_VALS **********************/

int establish_pinfo_internal_param_default_vals(dataptr dz)
{
	dz->param[PINF_MEAN] = 0.0;
	dz->param[PINF_MAX]  = 0.0;
	dz->param[PINF_MIN]  = dz->nyquist;
	return(FINISHED);
}

/************ SETUP_INTERNAL_PARAMS_FOR_PWRITE *************/

int setup_internal_params_for_pwrite(dataptr dz)
{
	dz->param[PW_DRED] /= 12.0;
	dz->param[PW_DRED] = pow(2.0,dz->param[PW_DRED]);
	dz->is_sharp = dz->param[PW_DRED];
	if(dz->is_sharp==0.0) {
		sprintf(errstr,"param not set in setup_internal_params_for_pwrite()\n");
		return(PROGRAM_ERROR);
	}
	dz->is_flat  = 1.0/dz->is_sharp;
	return(FINISHED);
}

/********************************************************************************************/
/********************************** FORMERLY IN procspec.c **********************************/
/********************************************************************************************/

/**************************** SPEC_PROCESS_FILE ****************************/

int spec_process_file(dataptr dz)
{	
	dz->total_windows = 0;

	switch(dz->process) {
	case(P_INFO):	return specpinfo(dz);
	case(P_ZEROS):	return specpinfo(dz);
	case(P_SEE):	return specpsee(dz);
	case(P_HEAR):	return specphear(dz);
	case(P_WRITE):	return specpwrite(dz);
	default:
		sprintf(errstr,"Unknown process in spec_process_file()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);	/* NOTREACHED */
}

/***************************** INNER_LOOP (redundant) **************************/

int inner_loop
(int *peakscore,int *descnt,int *in_start_portion,int *least,int *pitchcnt,int windows_in_buf,dataptr dz)
{
	return(FINISHED);
}

/********************************************************************************************/
/********************************** FORMERLY IN pconsistency.c ******************************/
/********************************************************************************************/

/****************************** CHECK_PARAM_VALIDITY_AND_CONSISTENCY *********************************/

int check_param_validity_and_consistency(dataptr dz)
{
	int exit_status, chans, stype;
	int srate;
	handle_pitch_zeros(dz);
	switch(dz->process) {
	case(P_WRITE):	   return check_data_for_pwrite(dz);
	case(P_SEE):
		chans = dz->infile->channels;
		srate = dz->infile->srate;
		stype = dz->infile->stype;
		dz->infile->channels = 1;
		dz->infile->srate    = dz->infile->origrate;
		dz->infile->stype = SAMP_SHORT;
		if((exit_status = create_sized_outfile(dz->wordstor[0],dz))<0)
			return(exit_status);
		dz->infile->channels = chans;
		dz->infile->srate    = srate;
		dz->infile->stype    = stype;
		break;
	case(P_HEAR):
		chans = dz->infile->channels;
		dz->infile->channels = dz->infile->origchans;
		if((exit_status = create_sized_outfile(dz->wordstor[0],dz))<0)
			return(exit_status);
		dz->infile->channels = chans;
		break;
	}
	return(FINISHED);
}

/************ CHECK_DATA_FOR_PWRITE *************/

int check_data_for_pwrite(dataptr dz)
{
	if(dz->is_transpos==FALSE && are_pitch_zeros(dz))
		return(DATA_ERROR);
	return(FINISHED);
}

/************************** ARE_PITCH_ZEROS ************************/

int are_pitch_zeros(dataptr dz)
{
	int n;
   	for(n=0;n<dz->wlength;n++) {
	    if(dz->pitches[n] < 0.0) {
 	        sprintf(errstr,"Input file contains unpitched windows: cannot proceed.\n");
	     	return(TRUE);
	    }
	}
	return(FALSE);
}

/********************************************************************************************/
/********************************** FORMERLY IN buffers.c ***********************************/
/********************************************************************************************/

/**************************** ALLOCATE_LARGE_BUFFERS ******************************/

int allocate_large_buffers(dataptr dz)
{
	switch(dz->process) {
	case(P_SEE):
		return create_sndbufs(dz);
	case(P_HEAR):	
		return allocate_single_buffer_plus_extra_pointer(dz);
	case(P_INFO):	case(P_ZEROS):	case(P_WRITE):	
		return(FINISHED);
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
	if     (!strcmp(prog_identifier_from_cmdline,"info"))     		dz->process = P_INFO;
	else if(!strcmp(prog_identifier_from_cmdline,"zeros"))    		dz->process = P_ZEROS;
	else if(!strcmp(prog_identifier_from_cmdline,"see"))  	   		dz->process = P_SEE;
	else if(!strcmp(prog_identifier_from_cmdline,"hear"))     		dz->process = P_HEAR;
	else if(!strcmp(prog_identifier_from_cmdline,"convert"))    	dz->process = P_WRITE;
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
	"\nINFORMATION ON A BINARY PITCH FILE\n\n"
	"USAGE: pitchinfo NAME infile (outfile) parameters: \n"
	"\n"
	"where NAME can be any one of\n"
	"\n"
	"info     zeros     see      hear      convert\n\n"
	"Type 'pitchinfo info' for more info on pitchinfo info..ETC.\n");
	return(USAGE_ONLY);
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
	if(!strcmp(str,"info")) {
		sprintf(errstr,
    	"pitchinfo info pitchfile\n\n"
		"DISPLAY INFORMATION ABOUT PITCHDATA IN PITCHFILE.\n\n"
		"Finds mean pitch, max and min pitch (with timings), as Hz and MIDI.\n"
		"Also displays total range, in octaves and semitones.\n");
	} else if(!strcmp(str,"zeros")) {
		sprintf(errstr,
    	"pitchinfo zeros pitchfile\n\n"
		"SHOWS WHETHER PITCHFILE CONTAINS UNINTERPOLATED ZEROS (UNPITCHED WINDOWS).\n");
	} else if(!strcmp(str,"see")) {
		fprintf(stdout,
		"pitchinfo see 1 pitchfile    outsndfile scalefact\n"
		"pitchinfo see 2 transposfile outsndfile\n\n"
		"CONVERT BINARY PITCHFILE OR TRANSPOSITION-FILE TO PSEUDO-SNDFILE, FOR VIEWING.\n\n"
		"MODES....\n"
		"(1) PITCHFILE is a binary pitchdata file.\n"
		"    SCALEFACT (> 0.0) multiplies pitch vals, for ease of viewing.\n"
		"    Pitch data scaled by (e.g.) 100 can be read directly from 'sndfile'\n"
		"    (Remembering to divide numeric values by 100).\n\n"
		"(2) TRANSPOSFILE is a binary transposition-data file.\n"
		"    Transposition data is automatically scaled to half max range,\n"
		"    And displayed in log format (0 = no transposition, + = up, - = down),\n"
		"    giving a schematic idea ONLY, of transposition data.\n");
	} else if(!strcmp(str,"hear")) {
		sprintf(errstr,
		"pitchinfo hear pitchfile outfile [-ggain]\n\n"
		"CONVERT BINARY PITCHFILE TO ANALYSIS TESTTONE FILE.\n"
		"           (RESYNTHESISE TO HEAR PITCH).\n\n"
		"-g  gain > 0.0 (default: 1.0)\n");
	} else if(!strcmp(str,"convert")) {
		sprintf(errstr,
		"pitchinfo convert pitchfile outtextfile [-dI]\n\n"
		"CONVERT A BINARY PITCH-DATAFILE TO A TIME/FRQ BRKPNT TEXTFILE.\n\n"
		"-d  I = acceptable pitch error in brkpntfile data-reduction. (semitones)\n"
		"    (Range > 0.0 : Default = eighth_tone = %lf)\n",LOG2(EIGHTH_TONE)*SEMITONES_PER_OCTAVE);
	} else
		sprintf(errstr,"Unknown option '%s'\n",str);
	return(USAGE_ONLY);
}

/******************************** USAGE3 ********************************/

int usage3(char *str1,char *str2)
{
	if(!strcmp(str1,"info"))		
		return(CONTINUE);
	else if(!strcmp(str1,"zeros"))
		return(CONTINUE);
	else
		sprintf(errstr,"Insufficient parameters on command line.\n");
	return(USAGE_ONLY);
}


