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
#include <morph.h>
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
/********************************** FORMERLY IN buffers.c ***********************************/
/********************************************************************************************/

int  allocate_triple_buffer_for_bridge(dataptr dz);

/********************************************************************************************/
/********************************** FORMERLY IN pconsistency.c ******************************/
/********************************************************************************************/

static int 	check_compatibility_of_bridge_params(dataptr dz);
static int	check_consistency_of_morph_params(dataptr dz);

/********************************************************************************************/
/********************************** FORMERLY IN preprocess.c ********************************/
/********************************************************************************************/

static int  setup_internal_params_and_arrays_for_glide(dataptr dz);
static int  setup_internal_params_for_bridge(dataptr dz);
static int  establish_interplen_and_if_head_or_tail_to_the_interp(int *wlen,dataptr dz);
static int  morph_preprocess(dataptr dz);
static int  make_morph_costable(dataptr dz);
static int	setup_internal_morph_params(dataptr dz);

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
	case(GLIDE):      		dz->extra_bufcnt =  2; dz->bptrcnt = 2; 	break;
	case(BRIDGE):     		dz->extra_bufcnt =  0; dz->bptrcnt = 6; 	break;
	case(MORPH):      		dz->extra_bufcnt =  0; dz->bptrcnt = 4; 	break;	
	default:
		sprintf(errstr,"Unknown program type [%d] in establish_bufptrs_and_extra_buffers()\n",dz->process);
		return(PROGRAM_ERROR);
	}

	if(dz->extra_bufcnt < 0) {
		sprintf(errstr,"bufcnts have not been set: establish_bufptrs_and_extra_buffers()\n");
		return(PROGRAM_ERROR);
	}
	return establish_spec_bufptrs_and_extra_buffers(dz);
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
	case(GLIDE):  	dz->array_cnt =1;  dz->iarray_cnt =1;  dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(BRIDGE): 	dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(MORPH):  	dz->array_cnt =1;  dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
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
	case(GLIDE):	  	setup_process_logic(TWO_ANALFILES,		  	BIG_ANALFILE,		ANALFILE_OUT,	dz);	break;
	case(BRIDGE):	  	setup_process_logic(TWO_ANALFILES,		  	BIG_ANALFILE,		ANALFILE_OUT,	dz);	break;
	case(MORPH):	  	setup_process_logic(TWO_ANALFILES,		  	BIG_ANALFILE,		ANALFILE_OUT,	dz);	break;
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

/****************************** FORMERLY IN internal.c *********************************/
/***************************************************************************************/

/****************************** SET_LEGAL_INTERNALPARAM_STRUCTURE *********************************/

int set_legal_internalparam_structure(int process,int mode,aplptr ap)
{
	int exit_status = FINISHED;
	switch(process) {
	case(GLIDE):	  	return(FINISHED);
	case(BRIDGE):	  	exit_status = set_internalparam_data("ddddiiii",ap);			break;
	case(MORPH):	  	exit_status = set_internalparam_data("iiiii",ap);				break;
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
	case(GLIDE):		return setup_internal_params_and_arrays_for_glide(dz);
	case(BRIDGE):		return setup_internal_params_for_bridge(dz);
	case(MORPH):		return morph_preprocess(dz);
	default:
		sprintf(errstr,"PROGRAMMING PROBLEM: Unknown process in param_preprocess()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);	/* NOTREACHED */
}

/************ SETUP_INTERNAL_PARAMS_AND_ARRAYS_FOR_GLIDE *************/

int setup_internal_params_and_arrays_for_glide(dataptr dz)
{
	dz->wlength    = round(dz->param[GLIDE_DUR]/dz->frametime);
	if((dz->parray[GLIDE_INF]   = (double *)malloc(dz->clength * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for glide array.\n");
		return(MEMORY_ERROR);
	}
	if((dz->iparray[GLIDE_ZERO] = (int    *)malloc(dz->clength * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for glide zero array.\n");
		return(MEMORY_ERROR);
	}
	return(FINISHED);
}

/************ SETUP_INTERNAL_PARAMS_FOR_BRIDGE *************/

int setup_internal_params_for_bridge(dataptr dz)
{
	dz->param[BRG_TFAC]     = 0.0;			
	dz->iparam[BRG_SWIN]    = 0;
	dz->iparam[BRG_INTPEND] = round(dz->param[BRG_ETIME]/dz->frametime);
	dz->iparam[BRG_STARTIS] = 0;
	dz->iparam[BRG_TAILIS]  = 0;
	return establish_interplen_and_if_head_or_tail_to_the_interp(&(dz->wlength),dz);
}

/************** ESTABLISH_INTERPLEN_AND_IF_HEAD_OR_TAIL_TO_THE_INTERP **********/

int establish_interplen_and_if_head_or_tail_to_the_interp(int *wlen,dataptr dz)
{
	int interplen;
	int endwindow1  = dz->insams[0]/dz->wanted;
	int endwindow2  = dz->insams[1]/dz->wanted + round(dz->param[BRG_OFFSET]/dz->frametime);
	int endwindow   = min(endwindow1,endwindow2);
	if(endwindow > dz->iparam[BRG_INTPEND])  	/* If interp ends before snd end */
		dz->iparam[BRG_TAILIS] = 3;				/* establish a tail */

	if(flteq(dz->param[BRG_EA2],1.0) 			/* if intp ends in pure 2nd snd */ 
	&& flteq(dz->param[BRG_EF2],1.0)  
	&& dz->iparam[BRG_INTPEND] <= endwindow1) {	/* after 1st sound has ended */
		dz->iparam[BRG_TAILIS] = 2;  			/* keep end of FILE2 */
		if(endwindow2 > endwindow)
			endwindow = endwindow2;
	}
	if(flteq(dz->param[BRG_EA2],0.0)  			/* if intp ends in pure 1st snd */
	&& flteq(dz->param[BRG_EF2],0.0)
	&& dz->iparam[BRG_INTPEND] <= endwindow2) {	/* after 2nd sound has ended */
		dz->iparam[BRG_TAILIS] = 1;   			/* keep end of FILE1 */
		if(endwindow1 > endwindow)
			endwindow = endwindow1;
	}
	if(dz->param[BRG_STIME] > 0.0) {
		dz->iparam[BRG_STARTIS] = 3;
		dz->iparam[BRG_SWIN] = round(dz->param[BRG_STIME]/dz->frametime);
		if(flteq(dz->param[BRG_SA2],0.0) && flteq(dz->param[BRG_SF2],0.0))
			/* Start with snd1 before interp */
			dz->iparam[BRG_STARTIS] = 1;
		if(flteq(dz->param[BRG_SA2],1.0) && flteq(dz->param[BRG_SF2],1.0))
			/* Start with snd2 before interp */
			dz->iparam[BRG_STARTIS] = 2;
	}
	if((interplen = dz->iparam[BRG_INTPEND] - dz->iparam[BRG_SWIN])<0) {
		sprintf(errstr,"Starttime beyond end of one soundfile, or beyond endtime.\n");
		return(USER_ERROR);
	}
	dz->param[BRG_FSTEP] = (dz->param[BRG_EF2] 
		- dz->param[BRG_SF2])/(double)interplen; /* FRQ INTERPOLATION STEP */
	dz->param[BRG_ASTEP] = (dz->param[BRG_EA2] 
		- dz->param[BRG_SA2])/(double)interplen; /* AMP INTERPOLATION STEP */
	dz->param[BRG_TSTEP] = 2.0 * (1.0/(double)interplen);
	*wlen = endwindow;
	return(FINISHED);
}

/************************** MORPH_PREPROCESS ******************************/

int morph_preprocess(dataptr dz)
{
	int exit_status;
	if(dz->mode==MPH_COSIN)	{
		if((exit_status = make_morph_costable(dz))<0)
			return(exit_status);
	}
	return setup_internal_morph_params(dz);
}

/************************ MAKE_MORPH_COSTABLE ***********************/

int make_morph_costable(dataptr dz)
{
	double d, d2;
	int n;
	d = PI/(double)MPH_COSTABSIZE;
	if((dz->parray[MPH_COS] = (double *)malloc((MPH_COSTABSIZE + 1) * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for morph cosin table.\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<MPH_COSTABSIZE;n++) {
		d2 = cos((double)n * d);
		d2 += 1.0;
		d2 /= 2.0;
		d2 = 1.0 - d2;
		d2 = min(d2,1.0);	/* trap calc errors */
		d2 = max(d2,0.0);
		dz->parray[MPH_COS][n] = d2;
	}
	dz->parray[MPH_COS][n] = 1.0;	/* wrap-around point */
	return(FINISHED);
}

/********************** SETUP_INTERNAL_MORPH_PARAMS **********************/

int	setup_internal_morph_params(dataptr dz)
{	
	dz->iparam[MPH_AENDW] = round(dz->param[MPH_AEND]/dz->frametime);
	dz->iparam[MPH_ASTTW] = round(dz->param[MPH_ASTT]/dz->frametime);
	dz->iparam[MPH_FENDW] = round(dz->param[MPH_FEND]/dz->frametime);
	dz->iparam[MPH_FSTTW] = round(dz->param[MPH_FSTT]/dz->frametime);
	dz->iparam[MPH_STAGW] = round(dz->param[MPH_STAG]/dz->frametime);
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
	case(GLIDE):	return specglide(dz);
	case(BRIDGE):	return specbridge(dz);
	case(MORPH):	return specmorph(dz);
	default:
		sprintf(errstr,"Unknown process in procspec()\n");
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
	handle_pitch_zeros(dz);
	switch(dz->process) {
	case(BRIDGE):	   return check_compatibility_of_bridge_params(dz);
	case(MORPH):	   return check_consistency_of_morph_params(dz);
	}
	return(FINISHED);
}

/************** CHECK_COMPATIBILITY_OF_BRIDGE_PARAMS **********/

int check_compatibility_of_bridge_params(dataptr dz)
{
	double bridge_time;
	double infile0_dur = (dz->insams[0]/dz->wanted) * dz->frametime;
	double infile1_dur = (dz->insams[1]/dz->wanted) * dz->frametime;

	if(dz->param[BRG_OFFSET] >= infile0_dur - dz->frametime) {
		sprintf(errstr,"Offset incompatible with 1st file duration.\n");
		return(USER_ERROR);
	}
	if(!dz->vflag[IS_BRG_START])
		dz->param[BRG_STIME] = dz->param[BRG_OFFSET];
	if(!dz->vflag[IS_BRG_END])
		dz->param[BRG_ETIME] = min(infile0_dur,dz->param[BRG_OFFSET] + infile1_dur);
	bridge_time = dz->param[BRG_ETIME] - dz->param[BRG_STIME];

	if(bridge_time < dz->frametime) {
		sprintf(errstr,"Starttime and endtime incompatible.\n");
		return(USER_ERROR);
	}
	if(dz->param[BRG_OFFSET] > dz->param[BRG_STIME]) {
		sprintf(errstr,"time_offset beyond bridge starttime.\n");
		return(USER_ERROR);
	}
	if(dz->param[BRG_ETIME] > dz->param[BRG_OFFSET] + infile1_dur) {
		sprintf(errstr,"bridge endtime beyond end of 2nd file.\n");
		return(USER_ERROR);
	}
	return(FINISHED);
}

/********************** CHECK_CONSISTENCY_OF_MORPH_PARAMS **********************/

int check_consistency_of_morph_params(dataptr dz)
{
	double duration0  = (dz->insams[0]/dz->wanted) * dz->frametime;
	double duration1 = ((dz->insams[1]/dz->wanted) * dz->frametime)
		+ dz->param[MPH_STAG];
	if(dz->param[MPH_ASTT] < dz->param[MPH_STAG]
	|| dz->param[MPH_FSTT] < dz->param[MPH_STAG]) {
		sprintf(errstr,
			"start of amp or frq interpolation is set before entry of 2nd soundfile.\n");
		return(DATA_ERROR);
	}
	if(dz->param[MPH_AEND] > duration0 || dz->param[MPH_AEND] > duration1
	|| dz->param[MPH_FEND] > duration0 || dz->param[MPH_FEND] > duration1) {
		sprintf(errstr,
			"end of amp or frq interpolation is beyond end of one of soundfiles.\n");
		return(DATA_ERROR);
	}
	if(dz->param[MPH_AEND] <= dz->param[MPH_ASTT]) {
		sprintf(errstr,"amp interpolation starttime is after (or equal to) its endtime.\n");
		return(USER_ERROR);
	}
	if(dz->param[MPH_FEND] <= dz->param[MPH_FSTT]) {
		sprintf(errstr,"frq interpolation starttime is after (or equal to) its endtime.\n");
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
	case(GLIDE):	return allocate_single_buffer_plus_extra_pointer(dz);
	case(BRIDGE):	return allocate_triple_buffer_for_bridge(dz);
	case(MORPH):	return allocate_double_buffer(dz);
	}
	sprintf(errstr,"Unknown program no. in allocate_large_buffers()\n");
	return(PROGRAM_ERROR);
}

/**************************** ALLOCATE_TRIPLE_BUFFER_FOR_BRIDGE ******************************/

int allocate_triple_buffer_for_bridge(dataptr dz)
{
	unsigned int buffersize;
	if(dz->bptrcnt < 5) {
		sprintf(errstr,"Insufficient bufptrs established in allocate_triple_buffer_for_bridge()\n");
		return(PROGRAM_ERROR);
	}
//TW REVISED: , as buffers no longer have to be in sector lengths
	buffersize = dz->wanted * BUF_MULTIPLIER;
	if((dz->bigfbuf = (float*)malloc((size_t)(buffersize * 3 * sizeof(float))))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for sound buffers.\n");
		return(MEMORY_ERROR);
	}
	dz->big_fsize   = buffersize;
//TW ADDED
	dz->buflen   = buffersize;
	dz->flbufptr[2]  = dz->bigfbuf + dz->big_fsize;
	dz->flbufptr[3]  = dz->flbufptr[2] + dz->big_fsize;
	dz->flbufptr[4]  = dz->flbufptr[3] + dz->big_fsize;	/* flbufptr[3-4] are static ptrs */
	/* NB MOVING POINTERS NOT ESTABLISHED HERE !! */
	return(FINISHED);
}

/********************************************************************************************/
/********************************** FORMERLY IN cmdline.c ***********************************/
/********************************************************************************************/

int get_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	if     (!strcmp(prog_identifier_from_cmdline,"glide"))     		dz->process = GLIDE;
	else if(!strcmp(prog_identifier_from_cmdline,"bridge"))    		dz->process = BRIDGE;
	else if(!strcmp(prog_identifier_from_cmdline,"morph"))     		dz->process = MORPH;
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
	"\nMORPHING BETWEEN SPECTRAL FILES\n\n"
	"USAGE: morph NAME (mode) infile infile2 outfile parameters: \n"
	"\n"
	"where NAME can be any one of\n"
	"\n"
	"glide     bridge     morph\n\n"
	"Type 'morph glide' for more info on morph glide..ETC.\n");
	return(USAGE_ONLY);
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
	if(!strcmp(str,"glide")) {
		fprintf(stdout,
		"morph glide infile infile2 outfile duration\n\n"
		"INTERPOLATE, LINEARLY, BETWEEN 2 SINGLE ANALYSIS WINDOWS\n"
		"              EXTRACTED WITH spec grab.\n\n"
		"INFILE1, INFILE2  are single-window analysis files.\n"
		"DURATION          is duration of output sound required.\n");
	} else if(!strcmp(str,"bridge")) {
		fprintf(stdout,
		"morph bridge mode infile1 infile2 outfile\n"
		"       [-aoffset] [-bsf2] [-csa2] [-def2] [-eea2] [-fstart] [-gend]\n\n"
		"      MAKE A BRIDGING-INTERPOLATION BETWEEN TWO SOUND SPECTRA\n"
		"BY INTERPOLATING BETWEEN 2 TIME-SPECIFIED WINDOWS IN THE 2 INFILES.\n\n"
		"OFFSET time infile2 starts, relative to start of file1: (>=0)  default: 0.0\n"
		"SF2    fraction of 2nd sound's frq interpolated at START.(default 0)\n"
		"SA2    fraction of 2nd sound's amp interpolated at START.(default 0)\n"
		"...if an OFFSET is used, when SF2 or SA2 are set NON-zero,\n"
		"...outsound starts from point where 2nd sound enters.\n"
		"EF2    fraction of 2nd sound's frq interpolated at END.(default 1)\n"
		"EA2    fraction of 2nd sound's amp interpolated at END.(default 1)\n\n"
		"...if EF2 or EA2 are set < 1.0 , outsound ends at end of first sound to end.\n"
		"START  time in infile1, of startwindow for interp, (secs): default: 0.0\n"
		"END    time in infile1 of endwindow of interp: default: end_of_file\n"
		"...if START and END are not specified\n"
		"...interp runs from OFFSET to end of 1st file to end.\n"
		"MODES....\n"
		"(1) output level is direct result of interpolation.\n"
		"(2) output level follows moment to moment minimum of the 2 infile amplitudes.\n"
		"(3) output level follows moment to moment amplitude of infile1.\n"
		"(4) output level follows moment to moment amplitude of infile2.\n"
		"(5) output level moves, through interp, from that of file1 to that of file2.\n"
		"(6) output level moves, through interp, from that of file2 to that of file1.\n");
	} else if(!strcmp(str,"morph")) {
		fprintf(stdout,
		"morph morph mode infile infile2 outfile as ae fs fe expa expf [-sstagger]\n\n"
		"MORPH ONE SPECTRUM INTO ANOTHER.\n\n"
		"as   is time(secs) when amplitude-interpolation starts.\n"
		"ae   is time(secs) when amplitude-interpolation ends.\n"
		"fs   is time(secs) when frequency-interpolation starts.\n"
		"fe   is time(secs) when frequency-interpolation ends.\n"
		"expa is exponent of amplitude interpolation.\n"
		"expf is exponent of frequency interpolation.\n"
		"stagger is time-delay of entry of 2nd file (defalut 0.0).\n\n"
		"MODES..\n"
		"(1) interpolate linearly (exp 1)\n"
		"    or over a curve of increasing (exp >1) or decreasing (exp <1) slope.\n"
		"(2) interpolate over a cosinusoidal spline.\n");
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

