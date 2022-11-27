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
#include <simple.h>
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

/********************************************************************************************/
/********************************** FORMERLY IN pconsistency.c ******************************/
/********************************************************************************************/

static int 	check_compatibility_of_cut_params(dataptr dz);

/********************************************************************************************/
/********************************** FORMERLY IN preprocess.c ********************************/
/********************************************************************************************/

#define SKIPWDWS 	(4)
static int  setup_internal_params_for_clean(dataptr dz);

/********************************************************************************************/
/************************************ FORMERLY IN aplinit.c *********************************/
/********************************************************************************************/

/***************************** ESTABLISH_BUFPTRS_AND_EXTRA_BUFFERS **************************/

int establish_bufptrs_and_extra_buffers(dataptr dz)
{
	int exit_status;
	int is_spec = FALSE;
	dz->extra_bufcnt = -1;	/* ENSURE EVERY CASE HAS A PAIR OF ENTRIES !! */
	dz->bptrcnt = 0;
	dz->bufcnt  = 0;
	switch(dz->process) {

	case(GAIN):       		dz->extra_bufcnt =  0; dz->bptrcnt = 1; 	is_spec = TRUE;		break;
	case(LIMIT):	  		dz->extra_bufcnt =  0; dz->bptrcnt = 1; 	is_spec = TRUE;		break;
	case(BARE):       		dz->extra_bufcnt =  0; dz->bptrcnt = 1; 	is_spec = TRUE;		break;
	case(CLEAN):      		dz->extra_bufcnt =  0; dz->bptrcnt = 1; 	is_spec = TRUE;		break;
	case(CUT): 	      		dz->extra_bufcnt =  0; dz->bptrcnt = 4; 	is_spec = TRUE;		break;	
	case(GRAB):       		dz->extra_bufcnt =  0; dz->bptrcnt = 1; 	is_spec = TRUE;		break;	
	case(MAGNIFY):    		dz->extra_bufcnt =  1; dz->bptrcnt = 1; 	is_spec = TRUE;		break;	

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

	case(GAIN):   	dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(LIMIT):	dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(BARE):   	dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(CLEAN):  	dz->array_cnt = 0; dz->iarray_cnt =1;  dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(CUT):  	dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(GRAB):  	dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(MAGNIFY):  dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;

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
						 					/* INPUT TYPE			PROCESS TYPE		OUTPUT TYPE */
	case(GAIN):			setup_process_logic(ANALFILE_ONLY,		  	EQUAL_ANALFILE,		ANALFILE_OUT,	dz);	break;
	case(LIMIT):		setup_process_logic(ANALFILE_ONLY,		  	EQUAL_ANALFILE,		ANALFILE_OUT,	dz);	break;
	case(BARE):		 	setup_process_logic(ANAL_WITH_PITCHDATA,  	EQUAL_ANALFILE,		ANALFILE_OUT,	dz);	break;
	case(CLEAN):
		switch(dz->mode) {
		case(COMPARING):setup_process_logic(THREE_ANALFILES,	  	EQUAL_ANALFILE,		ANALFILE_OUT,	dz);	break;
		default:		setup_process_logic(TWO_ANALFILES,		  	EQUAL_ANALFILE,		ANALFILE_OUT,	dz);	break;
		}
		break;
	case(CUT):			setup_process_logic(ANALFILE_ONLY,		  	BIG_ANALFILE,		ANALFILE_OUT,	dz);	break;
	case(GRAB):			setup_process_logic(ANALFILE_ONLY,		  	BIG_ANALFILE,		ANALFILE_OUT,	dz);	break;
	case(MAGNIFY):		setup_process_logic(ANALFILE_ONLY,		  	BIG_ANALFILE,		ANALFILE_OUT,	dz);	break;

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

/********************************************************************************************/
/********************************* FORMERLY IN internal.c **********************************/
/********************************************************************************************/

/****************************** SET_LEGAL_INTERNALPARAM_STRUCTURE *********************************/

int set_legal_internalparam_structure(int process,int mode,aplptr ap)
{
	int exit_status = FINISHED;
	switch(process) {
	case(GAIN):		  	return(FINISHED);
	case(LIMIT):      	return(FINISHED);
	case(BARE):		  	return(FINISHED);
	case(CLEAN):	  	exit_status = set_internalparam_data("i",ap);					break;
	case(CUT):		  	return(FINISHED);
	case(GRAB):	      	return(FINISHED);
	case(MAGNIFY):    	return(FINISHED);
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
	case(CLEAN):		return setup_internal_params_for_clean(dz);

	case(GAIN):	case(LIMIT): case(BARE): case(CUT):	case(GRAB):	case(MAGNIFY):			
		return(FINISHED);
	default:
		sprintf(errstr,"PROGRAMMING PROBLEM: Unknown process in param_preprocess()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);	/* NOTREACHED */
}

/************ SETUP_INTERNAL_PARAMS_FOR_CLEAN *************/

int setup_internal_params_for_clean(dataptr dz)
{
	int cc;
	for(cc=0;cc < dz->clength;cc++)
		dz->amp[cc]   = 0.0F;
	switch(dz->mode) {
	case(FROMTIME):  case(ANYWHERE):
		if((dz->iparam[CL_SKIPW] = round(dz->param[CL_SKIPT]/dz->frametime))<SKIPWDWS)
			dz->iparam[CL_SKIPW] = SKIPWDWS;
		break;
	case(COMPARING):
		if((dz->iparray[CL_MARK] = (int *)malloc(dz->clength * sizeof(int)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for clean marker store.\n");
			return(MEMORY_ERROR);
		}
		for(cc=0;cc < dz->clength;cc++)
			dz->iparray[CL_MARK][cc] = 1;
		break;
	case(FILTERING):
		break;
	default:
		sprintf(errstr,"Unknown case in setup_internal_params_for_clean()\n");
		return(PROGRAM_ERROR);
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

	display_virtual_time(0L,dz);

	switch(dz->process) {
	case(GAIN):		return outer_loop(dz);
	case(LIMIT):	return outer_loop(dz);
	case(BARE):		return outer_loop(dz);

	case(CLEAN):	return specclean(dz);
	case(CUT):	 	return speccut(dz);									
	case(GRAB):		return specgrab_or_magnify(GRAB_FRZTIME,dz);		
	case(MAGNIFY):	return specgrab_or_magnify(MAG_FRZTIME,dz);			
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
		case(BARE):		exit_status = specbare(pitchcnt,dz);	  			break;
		case(GAIN):		exit_status = specgain(dz);	  						break;
		case(LIMIT):	exit_status = speclimit(dz);  						break;
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
	switch(dz->process) {
	case(BARE):  
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
	case(CUT):	   	   return check_compatibility_of_cut_params(dz);
	}
	return(FINISHED);
}

/********************* CHECK_COMPATIBILITY_OF_CUT_PARAMS **********************/

int check_compatibility_of_cut_params(dataptr dz)
{
	int startwindow = (int)(dz->param[CUT_STIME]/dz->frametime);
	int endwindow   = (int)(dz->param[CUT_ETIME]/dz->frametime);
	if(endwindow <= startwindow) {
		sprintf(errstr,"Incompatible start and end times for cut.\n");
		return(USER_ERROR);
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
	case(GAIN):		case(LIMIT):	case(BARE):		
	case(CLEAN):	case(GRAB):		case(MAGNIFY):	
		return allocate_single_buffer(dz);

	case(CUT): 		
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
	if     (!strcmp(prog_identifier_from_cmdline,"gain"))	   		dz->process = GAIN;
	else if(!strcmp(prog_identifier_from_cmdline,"gate"))	   		dz->process = LIMIT;
	else if(!strcmp(prog_identifier_from_cmdline,"bare"))	   		dz->process = BARE;
	else if(!strcmp(prog_identifier_from_cmdline,"clean"))     		dz->process = CLEAN;
	else if(!strcmp(prog_identifier_from_cmdline,"cut"))	   		dz->process = CUT;
	else if(!strcmp(prog_identifier_from_cmdline,"grab"))	   		dz->process = GRAB;
	else if(!strcmp(prog_identifier_from_cmdline,"magnify"))   		dz->process = MAGNIFY;
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
	"\nSIMPLE OPERATIONS ON A SPECTRAL FILE\n\n"
	"USAGE: spec NAME (mode) infile(s) outfile parameters: \n"
	"\n"
	"where NAME can be any one of\n"
	"\n"
	"gain   gate    bare    clean    cut    grab    magnify\n\n"
	"Type 'spec gain' for more info on spec gain.. ETC.\n");
	return(USAGE_ONLY);
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
	if(!strcmp(str,"gain")) {
		sprintf(errstr,
		"spec gain infile outfile gain\n"
		"\n"
		"AMPLIFY OR ATTENUATE THE SPECTRUM\n"
		"\n"
		"gain may vary over time.\n"
		"\n");
	} else if(!strcmp(str,"gate")) {
		sprintf(errstr,
		"spec gate infile outfile threshold\n\n"
		"ELIMINATE CHANNEL DATA BELOW A THRESHOLD AMP\n\n"
		"Threshold may vary over time. Range 0 to 1\n");
	} else if(!strcmp(str,"bare")) {
		sprintf(errstr,
		"spec bare infile pitchfile outfile [-x]\n"
		"\n"
		"ZERO THE DATA IN CHANNELS WHICH DO NOT CONTAIN HARMONICS\n"
		"\n"
		"PITCHFILE must be extracted from your analysis file.\n"
		"          (normally using -z flag to mark any unpitched material).\n"
		"\n"
		"-x        less body in resulting spectrum.\n");
	} else if(!strcmp(str,"clean")) {
		fprintf(stdout,
		"spec clean 1-2 infile nfile       outfile skiptime [-gnoisgain]\n"
		"spec clean 3   infile nfile       outfile freq     [-gnoisgain]\n"
		"spec clean 4   infile nfile gfile outfile          [-gnoisgain]\n\n"
		"REMOVE NOISE FROM PVOC ANALYSIS FILE.\n\n"
		"INFILE,NFILE and GFILE are all pvoc analysis files.\n"
		"NFILE and GFILE should be cut (spec cut) from INFILE\n"
		"to show typical noise (NFILE) and good signal (GFILE).\n"
		"SKIPTIME     (seconds) may be set to time at which\n"
		"             good src. signal level has been established.\n"
		"NOISGAIN     multiplies noiselevels found in NFILE before they are used\n"
		"             for comparison with infile signal: (Default 2).\n"
		"MODES\n"
		"(1) deletes a channel (after skiptime) FROM THE TIME its level falls below\n"
		"    the (noisgain adjusted) maximum level seen for that channel in NFILE.\n"
		"(2) deletes channel (after skiptime) ANYWHERE its level falls below\n"
		"    the (noisgain adjusted) maximum level seen for that channel in NFILE.\n"
		"(3) Deletes channel as in MODE 2 but ONLY for channels of frq > FREQ.\n"
		"(4) deletes channel EVERYWHERE, whose level in GFILE is ALWAYS below\n"
		"    the (noisgain adjusted) maximum level seen for that channel in NFILE.\n");
	} else if(!strcmp(str,"cut")) {
		sprintf(errstr,
		"spec cut infile outfile starttime endtime\n\n"
		"CUT SECTION OUT OF ANALYSIS FILE, BETWEEN STARTTIME & ENDTIME (SECS)\n"
		"\n");
	} else if(!strcmp(str,"grab")) {
		sprintf(errstr,
		"spec grab infile outfile time\n\n"
		"GRAB A SINGLE ANALYSIS WINDOW AT 'TIME'\n\n"
		"A time beyond end of file will grab last window in file.\n"
		"\n");
	} else if(!strcmp(str,"magnify")) {
		sprintf(errstr,
		"spec magnify infile outfile time dur\n\n"
		"MAGNIFY A SINGLE ANALYSIS WINDOW, AT TIME 'TIME', TO DURATION 'DUR'.\n"
		"\n");
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

