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



/* floatsam version */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <structures.h>
#include <cdpmain.h>
#include <tkglobals.h>
#include <pnames.h>
#include <mix.h>
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
#include <mix1.h>

//TW UPDATE
#include <ctype.h>

#ifdef unix
#define round(x) lround((x))
#endif

/********************************************************************************************/
/********************************** FORMERLY IN buffers.c ***********************************/
/********************************************************************************************/

#define MAX_SYNCATT_CHANS	(STEREO)

static int  create_mix_buffers(dataptr dz);
static int  create_syncatt_buffer(dataptr dz);
static int  create_mixtwo_buffer(dataptr dz);
//TW UPDATE
static int create_mixmany_buffer(dataptr dz);

/********************************************************************************************/
/********************************** FORMERLY IN pconsistency.c ******************************/
/********************************************************************************************/

static int  mixtwo_sndprops_consistency(dataptr dz);
static int  get_filesearch_data(dataptr dz);
static int  syncatt_presets(dataptr dz);
static int  check_syncatt_window_factor(dataptr dz);
static int  mixtwarp_consistency(dataptr dz);
static int  mixswarp_consistency(dataptr dz);
static int  mix_consistency(dataptr dz);
static int  mix_atten(dataptr dz);
static int  adjust_levels(int lineno,int total_words,dataptr dz);

//TW UPDATES
static int cyc_preprop(dataptr dz);
static int create_cycinbi_buffer(dataptr dz);
static int invalid_data_items(char *str);
static int read_mixgrid_file(char *str,dataptr dz);
static int read_and_scale_balance_data(char *filename,dataptr dz);
static int addtomix_consistency(dataptr dz);
static int mixmodel_consistency(dataptr dz);
static int read_inbetween_ratios(char *filename,dataptr dz);

/***************************************************************************************/
/****************************** FORMERLY IN aplinit.c **********************************/
/***************************************************************************************/

/***************************** ESTABLISH_BUFPTRS_AND_EXTRA_BUFFERS **************************/

int establish_bufptrs_and_extra_buffers(dataptr dz)
{
//	int is_spec = FALSE;
	dz->extra_bufcnt = -1;	/* ENSURE EVERY CASE HAS A PAIR OF ENTRIES !! */
	dz->bptrcnt = 0;
	dz->bufcnt  = 0;
	switch(dz->process) {
	case(MIX):
	case(MIXTEST):
	case(MIXMAX):
	case(MIXFORMAT):
	case(MIXDUMMY):
	case(MIXTWARP):
	case(MIXSWARP):	   	   
//TW UPDATE
	case(MIX_PAN):	   	   
	case(MIXGAIN):	   	   
	case(MIXSHUFL):	   	   
	case(MIXSYNC):
//TW UPDATES
	case(MIX_ON_GRID):
	case(AUTOMIX):
	case(ADDTOMIX):
	case(MIX_MODEL):
	case(MIX_AT_STEP):	   	   
								dz->extra_bufcnt = 0;	dz->bufcnt = 0;		break;
//TW UPDATE
	case(MIXMANY):				dz->extra_bufcnt = 0;	dz->bufcnt = 3;		break;
	case(MIXSYNCATT):			dz->extra_bufcnt = 0;	dz->bufcnt = 1;		break;
	case(MIXTWO):
	case(MIXBALANCE):			dz->extra_bufcnt = 0;	dz->bufcnt = 4;		break;
	case(MIXCROSS):				dz->extra_bufcnt = 0;	dz->bufcnt = 2;		break;
	case(MIXINTERL):			dz->extra_bufcnt = 0;	dz->bufcnt = 1 + MAX_MI_OUTCHANS;	break;
	case(CYCINBETWEEN):
	case(MIXINBETWEEN):			dz->extra_bufcnt = 0;	dz->bufcnt = 3;		break;
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
	case(MIX):
	case(MIXTWO):
	case(MIXBALANCE):
	case(MIXTEST):
	case(MIXMAX):
	case(MIXFORMAT):
	case(MIXDUMMY):
	case(MIXINTERL):
	case(MIXMANY):
	case(MIXINBETWEEN):
	case(ADDTOMIX):
	case(MIX_MODEL):
	case(MIX_AT_STEP):	   	   
					dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0;	dz->fptr_cnt = 0; break;
	case(CYCINBETWEEN):
					dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 4; dz->ptr_cnt = 0;	dz->fptr_cnt = 0; break;

	case(MIX_ON_GRID):
	case(AUTOMIX):
	case(MIXSWARP):	   	   
	case(MIX_PAN):	   	   
					dz->array_cnt = 1; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0;	dz->fptr_cnt = 0; break;
	case(MIXGAIN):	   	   
	case(MIXTWARP):
	case(MIXSHUFL):	   	   
					dz->array_cnt = 2; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0;	dz->fptr_cnt = 0; break;

	case(MIXSYNC):	dz->array_cnt = 1; dz->iarray_cnt = 1; dz->larray_cnt = 0; dz->ptr_cnt = 0;	dz->fptr_cnt = 0; break;
	case(MIXSYNCATT): dz->array_cnt=3; dz->iarray_cnt = 2; dz->larray_cnt = 3; dz->ptr_cnt = 0;	dz->fptr_cnt = 0; break;
	case(MIXCROSS):	dz->array_cnt = 1; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0;	dz->fptr_cnt = 0; break;
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
	/* RWD all now floats */
	if(dz->larray_cnt > 0) {	  
		if((dz->lparray = (int    **)malloc(dz->larray_cnt * sizeof(int *)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for internal long arrays.\n");
			return(MEMORY_ERROR);
		}
		/* RWD 4:2002 lparray shadowed by lfarray for submix syncatt */

		if((dz->lfarray = (float    **)malloc(dz->larray_cnt * sizeof(float *)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for internal float arrays.\n");
			return(MEMORY_ERROR);
		}		
		for(n=0;n<dz->larray_cnt;n++){
			dz->lparray[n] = NULL;
			dz->lfarray[n] = NULL;
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
//TW FIXED
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
	case(MIX):	           setup_process_logic(MIXFILES_ONLY,	 	UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(MIXTWO):
	case(MIXBALANCE):	   setup_process_logic(TWO_SNDFILES,	 	UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(MIXCROSS):		   setup_process_logic(TWO_SNDFILES,	 	UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(MIXINTERL):	   setup_process_logic(MANY_SNDFILES,	 	UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(CYCINBETWEEN):
	case(MIXINBETWEEN):	   setup_process_logic(TWO_SNDFILES,	 	OTHER_PROCESS,		NO_OUTPUTFILE,	dz);	break;
	case(MIXTEST):		   setup_process_logic(MIXFILES_ONLY,	 	SCREEN_MESSAGE,		NO_OUTPUTFILE,	dz);	break;
	case(MIXFORMAT):	   setup_process_logic(NO_FILE_AT_ALL,	 	SCREEN_MESSAGE,		NO_OUTPUTFILE,	dz);	break;
	case(MIXDUMMY):	   	   setup_process_logic(MANY_SNDFILES,	 	TO_TEXTFILE,		TEXTFILE_OUT,	dz);	break;
//TW UPDATES
	case(ADDTOMIX):	   setup_process_logic(ANY_NUMBER_OF_ANY_FILES, TO_TEXTFILE,		TEXTFILE_OUT,	dz);	break;
	case(MIX_MODEL):   setup_process_logic(ANY_NUMBER_OF_ANY_FILES, TO_TEXTFILE,		TEXTFILE_OUT,	dz);	break;
	case(MIXMANY):	   	   setup_process_logic(MANY_SNDFILES,	 	UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(MIX_ON_GRID):	   setup_process_logic(MANY_SNDFILES,	 	TO_TEXTFILE,		TEXTFILE_OUT,	dz);	break;
	case(AUTOMIX):	       setup_process_logic(MANY_SNDFILES,	 	UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;

	case(MIXSYNC):	   	   setup_process_logic(SND_OR_MIXLIST_ONLY,	TO_TEXTFILE,		TEXTFILE_OUT,	dz);	break;
	case(MIXSYNCATT): setup_process_logic(SND_SYNC_OR_MIXLIST_ONLY, TO_TEXTFILE,		TEXTFILE_OUT,	dz);	break;
	case(MIXTWARP):	   	   setup_process_logic(MIXFILES_ONLY,	 	TO_TEXTFILE,		TEXTFILE_OUT,	dz);	break;
	case(MIXSWARP):	   	   setup_process_logic(MIXFILES_ONLY,	 	TO_TEXTFILE,		TEXTFILE_OUT,	dz);	break;
//TW UPDATES
	case(MIX_PAN):	   	   setup_process_logic(MIXFILES_ONLY,	 	TO_TEXTFILE,		TEXTFILE_OUT,	dz);	break;
	case(MIX_AT_STEP):	   setup_process_logic(MANY_SNDFILES,	 	TO_TEXTFILE,		TEXTFILE_OUT,	dz);	break;

	case(MIXGAIN):	   	   setup_process_logic(MIXFILES_ONLY,	 	TO_TEXTFILE,		TEXTFILE_OUT,	dz);	break;
	case(MIXSHUFL):	   	   setup_process_logic(MIXFILES_ONLY,	 	TO_TEXTFILE,		TEXTFILE_OUT,	dz);	break;
	case(MIXMAX):
		switch(dz->mode) {
		case(MIX_LEVEL_ONLY):	  setup_process_logic(MIXFILES_ONLY,SCREEN_MESSAGE,		NO_OUTPUTFILE,	dz);	break;
		case(MIX_CLIPS_ONLY):	  setup_process_logic(MIXFILES_ONLY,TO_TEXTFILE,		TEXTFILE_OUT,	dz);	break;
		case(MIX_LEVEL_AND_CLIPS):setup_process_logic(MIXFILES_ONLY,TO_TEXTFILE,		TEXTFILE_OUT,	dz);	break;
		default:
			sprintf(errstr,"Unknown mode for MIXMAX in assign_process_logic()\n");
			return(PROGRAM_ERROR);
		}
		break;			   
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
	case(MIXSHUFL):
		if(dz->mode==MSH_DUPL_AND_RENAME)
			dz->has_otherfile = TRUE;
		else 
			dz->has_otherfile = FALSE;
		break;
	case(MIXBALANCE):
	case(MIXTWO):	  		/* normal file comparison ignores stereo-mono conflict */
	case(MIXCROSS):			/* But most of these progs need consistent channel-count in input snds. */
	case(MIXINTERL):		/* Sending header data  to 'otherfile' */
	case(MIXINBETWEEN):		/* allows comparison to be made in a distinct function. */
	case(CYCINBETWEEN):
	case(AUTOMIX):
		dz->has_otherfile = TRUE;
		break;
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
	case(MIX):			exit_status = set_internalparam_data("iii",ap);				break;
	case(MIXTWO):		exit_status = set_internalparam_data("dd",ap);				break;
	case(MIXBALANCE):	exit_status = set_internalparam_data("",ap);				break;
	case(MIXCROSS):	    
		switch(mode) {
		case(MCLIN):	exit_status = set_internalparam_data("0iid",ap);				break;
		case(MCCOS):	exit_status = set_internalparam_data( "iid",ap);				break;	
		}
		break;
	case(MIXINTERL):    exit_status = set_internalparam_data("",ap);					break;
	case(CYCINBETWEEN):
	case(MIXINBETWEEN): exit_status = set_internalparam_data("",ap);					break;
	case(MIXTEST):		exit_status = set_internalparam_data("",ap);					break;
	case(MIXMAX):		exit_status = set_internalparam_data("iii",ap);				break;
	case(MIXFORMAT):	exit_status = set_internalparam_data("",ap);					break;
	case(MIXMANY):	
	case(MIX_ON_GRID):	
	case(AUTOMIX):	
	case(ADDTOMIX):	
	case(MIX_MODEL):
	case(MIX_AT_STEP):	

	case(MIXDUMMY):		exit_status = set_internalparam_data("",ap);					break;
	case(MIXSYNC):		exit_status = set_internalparam_data("",ap);					break;
	case(MIXSYNCATT):	exit_status = set_internalparam_data("i",ap);					break;
	case(MIXTWARP):		exit_status = set_internalparam_data("",ap);					break;
	case(MIXSWARP):		exit_status = set_internalparam_data("",ap);					break;
//TW UPDATE
	case(MIX_PAN):		exit_status = set_internalparam_data("",ap);					break;
	case(MIXGAIN):		exit_status = set_internalparam_data("",ap);					break;
	case(MIXSHUFL):		exit_status = set_internalparam_data("",ap);					break;
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
	case(SNDFILENAME):				return read_new_filename(str,dz);
//TW UPDATES
	case(GRIDDED_MIX):				return read_mixgrid_file(str,dz);
	case(AUTO_MIX):					return read_and_scale_balance_data(str,dz);
	case(INBTWN_RATIOS):
		if(dz->mode == INBI_RATIO)
			return read_inbetween_ratios(str,dz);
		break;
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
	int exit_status = FINISHED;

	switch(dz->process) {
	case(MIX):	
	case(MIXMAX):		return mix_preprocess(dz);

	case(MIXTWO):		
		if((exit_status = create_sized_outfile(dz->outfilename,dz))<0)
				return(exit_status);
		if((exit_status = mixtwo_preprocess(dz))<0)
			return(exit_status);
		return FINISHED;
	case(MIXCROSS):		return mixcross_preprocess(dz);
	case(MIXINBETWEEN):	return get_inbetween_ratios(dz);
	case(CYCINBETWEEN):	return cyc_preprop(dz);
	case(MIXTEST):			case(MIXFORMAT):		case(MIXDUMMY):
	case(MIXSYNC):			case(MIXSYNCATT):
	case(MIXMANY):			case(MIX_ON_GRID):		case(AUTOMIX):	
	case(ADDTOMIX):			case(MIX_AT_STEP):		case(MIX_MODEL):	

		return(FINISHED);
	case(MIXBALANCE):
		if((dz->infile->channels > 2 ||  dz->otherfile->channels > 2) && (dz->otherfile->channels != dz->infile->channels)) {
			sprintf(errstr,"With multichannel files, both files must have same number of channels.\n");
			return(DATA_ERROR);
		}
		break;
	case(MIXINTERL):
		if(dz->infilecnt > MAX_MI_OUTCHANS) {
			sprintf(errstr,"Too many infiles for interpolation program. Max %d\n",MAX_MI_OUTCHANS);
			return(USER_ERROR);
		}

		/* RWD 4:2002  now we can open outfile with corect params! */
		dz->infile->channels = dz->infilecnt;	/* ARRGH! */
		if((exit_status = create_sized_outfile(dz->outfilename,dz))<0)
				return(exit_status);


		return(FINISHED);
	case(MIXTWARP):
		initrand48();
		switch(dz->mode) {
		case(MTW_TIMESORT):							break;
		default:	dz->iparam[MSH_STARTLINE]--;	break;
		}
		return(FINISHED);
	case(MIXSWARP):
		initrand48();
		switch(dz->mode) {
		case(MSW_TWISTALL):	break;
		case(MSW_TWISTONE): dz->iparam[MSW_TWLINE]--;		break;
		default:			dz->iparam[MSH_STARTLINE]--;	break;
		}
		return(FINISHED);
	case(MIXGAIN):
		dz->iparam[MSH_STARTLINE]--;
		return(FINISHED);
	case(MIXSHUFL):
		initrand48();
		if((exit_status = check_new_filename(dz->wordstor[dz->extra_word],dz))<0)
			return(exit_status);
		dz->iparam[MSH_STARTLINE]--;
		return(FINISHED);
//TW UPDATE
	case(MIX_PAN):
		break;
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
	double normalisation;

	switch(dz->process) {
	case(MIX):			case(MIXCROSS):
	case(MIXINTERL):	case(MIXINBETWEEN):	case(MIXMAX):	case(CYCINBETWEEN):
		display_virtual_time(0L,dz);
		break;
	}
	switch(dz->process) {
	case(MIX):		   	return mmix(dz);
	case(MIXTWO):
		display_virtual_time(0L,dz);
	   	return mixtwo(dz);
	case(MIXBALANCE):	return cross_stitch(dz);
	case(MIXCROSS):	   	return mix_cross(dz);
	case(MIXINTERL):   	return mix_interl(dz);
	case(CYCINBETWEEN):
	case(MIXINBETWEEN):	return mix_inbetween(dz);
	case(MIXMAX):	  	return mix_level_check(&normalisation,dz);
	case(MIXTEST):	  	return mix_syntax_check(dz);

	case(MIXFORMAT):  	
		if(!sloom)
			return usage2("fileformat");
		return (FINISHED);	
//TW UPDATES
	case(MIXMANY):
	   	return mixmany(dz);
	case(AUTOMIX):		return do_automix(dz);
	case(ADDTOMIX):		return addtomix(dz);
	case(MIX_MODEL):	return mix_model(dz);
	case(MIX_ON_GRID):	
	case(MIX_AT_STEP):	

	case(MIXDUMMY):   	return create_mixdummy(dz);

	case(MIXSYNC):	  	return mix_sync(dz);
	case(MIXSYNCATT): 	return synchronise_mix_attack(dz);

	case(MIXTWARP):	  	return mix_timewarp(dz);
	case(MIXSWARP):	  	return mix_spacewarp(dz);
//TW UPDATE
	case(MIX_PAN):	  	return panmix(dz);
	case(MIXGAIN):	  	return mix_gain(dz);
	case(MIXSHUFL):	  	return mix_shufl(dz);
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
	int exit_status = FINISHED;
	handle_pitch_zeros(dz);
	switch(dz->process) {
	case(MIX):
		if(!(flteq(dz->param[MIX_ATTEN],1.0))) {
			if((exit_status = mix_atten(dz)) <0)
				return(exit_status);
		}
		/* fall thro */
	case(MIXMAX):		return set_up_mix(dz);

	case(MIXTWO):
	case(MIXBALANCE):
	case(MIXCROSS):
	case(MIXINTERL):
	case(MIXINBETWEEN):	return mixtwo_sndprops_consistency(dz);

	case(MIXSYNCATT):	return syncatt_presets(dz);
	case(MIXTWARP):		return mixtwarp_consistency(dz);
	case(MIXSWARP):		return mixswarp_consistency(dz);

	case(MIXGAIN):
	case(MIXSHUFL):		return mix_consistency(dz);
	case(MIX_MODEL):	return mixmodel_consistency(dz);
	case(ADDTOMIX):		return addtomix_consistency(dz);
	case(AUTOMIX):		/* bufer pointers established once infilecnt is known */
		dz->bufcnt = dz->infilecnt + 1;
		return establish_groucho_bufptrs_and_extra_buffers(dz);
	}
	return(FINISHED);
}

/******************************** MIXTWO_SNDPROPS_CONSISTENCY ********************************/
/*RWD comment: for a large source file this is a heavy burden; we need inline buffer conversion rather than
 a temporary file */
/* RWD: this is mono/stereo only - where is that tested for? */
int mixtwo_sndprops_consistency(dataptr dz)
{
	double infiledur2, infiledur, sr = (double)dz->infile->srate; 

	int n, m, k, exit_status;
	int tempfd;
	float *tempbuf;
	int samps_read, samps_written;	
//TW just to make it clearer
	int shsecsize = F_SECSIZE;
//	int dbl_shsecsize = shsecsize * 2;
	int outsize_stereo;

	if(dz->infile->srate!=dz->otherfile->srate) {
		sprintf(errstr,"Different sample-rates in input files: can't proceed.\n");
		return(USER_ERROR);
	}
	switch(dz->process) {
	case(MIXTWO): 		
		if(dz->infile->channels > 2) {
			if(dz->infile->channels != dz->otherfile->channels) {
				sprintf(errstr,"With multichannel files, both files must have same number of channels.\n");
				return(DATA_ERROR);
			}
			dz->outchans = dz->infile->channels;
		} else  {
			if(dz->infile->channels!=dz->otherfile->channels) {
				fprintf(stdout,"INFO: Files have different channel count. Converting to stereo.\n");
				fflush(stdout);
				if((tempbuf = (float *)malloc(/*SECSIZE*/shsecsize * 2 * sizeof(float)))==NULL) {
					sprintf(errstr,"Cannot create temporary buffer for mono to stereo conversion.\n");
					return(MEMORY_ERROR);
				}
				if(dz->infile->channels == 1) {
					is_converted_to_stereo = 0;
					outsize_stereo = dz->insams[0] * 2;
				} else {
					is_converted_to_stereo = 1;
					outsize_stereo = dz->insams[1] * 2;
				}
				if(sloom) {
					if((tempfd = sndcreat_formatted("#temp",outsize_stereo,dz->infile->stype,
							STEREO,dz->infile->srate,CDP_CREATE_NORMAL)) < 0) {
						is_converted_to_stereo = -1;
						sprintf(errstr,"Cannot open output file '#temp'\n");
						return(DATA_ERROR);
					}
				} else if((tempfd = sndcreat_formatted("_temp",outsize_stereo,dz->infile->stype,
						STEREO,dz->infile->srate,CDP_CREATE_NORMAL)) < 0) {
					is_converted_to_stereo = -1;
					sprintf(errstr,"Cannot open output file '_temp': %s\n",sferrstr());
					return(DATA_ERROR);
				}
				while((samps_read  = fgetfbufEx(tempbuf, shsecsize,dz->ifd[is_converted_to_stereo],0))>0) {
					int d_samps_read = samps_read * 2;
					for(m = d_samps_read - 2,k = d_samps_read - 1, 
							n = samps_read - 1;
								n >=0; n--,m-=2,k-=2) {
						tempbuf[m] = tempbuf[n];						
						tempbuf[k] = tempbuf[n];						
					}
					if((samps_written = fputfbufEx(tempbuf,/*SECSIZE*/samps_read * 2,tempfd))<0) {
						sprintf(errstr,"Can't write to output temporary soundfile: (is hard-disk full?).\n");
						return(SYSTEM_ERROR);
					}
					if(samps_written != samps_read * 2) {
						sprintf(errstr,"Error in data accounting while converting mono file to stereo.\n");
						return(SYSTEM_ERROR);
					}
				}
				dz->infile->channels = 2;

				if(sndcloseEx(tempfd) < 0) {
					sprintf(errstr,"Failed to close temporary stereo soundfile.\n");
					return(SYSTEM_ERROR);
				}
				if(sndcloseEx(dz->ifd[is_converted_to_stereo]) < 0) {
					sprintf(errstr,"Failed to close input soundfile, at conversion to stereo.\n");
					return(SYSTEM_ERROR);
				}
				dz->ifd[is_converted_to_stereo] = -1;
				if(sloom) {
					if((dz->ifd[is_converted_to_stereo] = sndopenEx("#temp",0,CDP_OPEN_RDONLY)) < 0) {
						sprintf(errstr,"Failed to reopen temporary file '#temp' for output.\n");
						return(SYSTEM_ERROR);
					}
				} else {
					if((dz->ifd[is_converted_to_stereo] = sndopenEx("_temp",0,CDP_OPEN_RDONLY)) < 0) {
						sprintf(errstr,"Failed to reopen temporary file '#temp' for output.\n");
						return(SYSTEM_ERROR);
					}
				}
				sndseekEx(dz->ifd[is_converted_to_stereo],0,0);
				dz->insams[is_converted_to_stereo] *= 2;
			}
		}
		if((exit_status = reset_peak_finder(dz))<0)
			return(exit_status);
		break;
	case(MIXBALANCE):
		break;
	default:	
		if(dz->infile->channels!=dz->otherfile->channels) {
			sprintf(errstr,"Different no. of channels in input files: can't proceed.\n");
			return(USER_ERROR);
		}
		break;
	}
	if(dz->otherfile->filetype!=SNDFILE) {
		sprintf(errstr,"2nd file is not a soundfile: can't proceed.\n");
		return(USER_ERROR);
	}
	infiledur  =(double)(dz->insams[0]/dz->infile->channels)/sr; 
	infiledur2 =(double)(dz->insams[1]/dz->infile->channels)/sr; 

	switch(dz->process) {
	case(MIXTWO):		
		if(dz->param[MIX_SKIP] >= infiledur2) {
			sprintf(errstr,"SKIP INTO 2ND FILE exceeds length of that file: cannot proceed\n");
			return(DATA_ERROR);
		}		
		break;
	case(MIXBALANCE):
		break;
	case(MIXCROSS):
		if(dz->param[MCR_BEGIN] >= infiledur2 + dz->param[MCR_STAGGER]) {
			sprintf(errstr,"CROSSFADE START is after end of 2nd sndfile: cannot proceed\n");
			return(DATA_ERROR);
		}
		if(dz->param[MCR_END] > infiledur + infiledur2) {
			sprintf(errstr,"CROSSFADE END is beyond end of sndfiles: cannot proceed\n");
			return(DATA_ERROR);
		}
		break;
	}
	return(FINISHED);
}

/************************* GET_FILESEARCH_DATA ************************/

int get_filesearch_data(dataptr dz)
{   
	int    exit_status;
	int   srate = 0, total_words, n;
	int    *inchans;
	/* RWD: sets a scan window only; crude, but we keep it for now */
//TW better to use global
	int   shsecsize = F_SECSIZE, *samplen;
	double filedur;
	double minsyncscan;
	double *start, *end;
	int    textfile_filetype = dz->infile->filetype;

	if(dz->linecnt > SF_MAXFILES) {
		sprintf(errstr,"Maximum number of sndfiles [%d] exceeded.\n",SF_MAXFILES);
		return(USER_ERROR);
	} else if(dz->linecnt <= 0) {
		sprintf(errstr,"No lines found in sync data file.\n");
		return(USER_ERROR);
	}
	if((dz->parray[MSY_STARTSRCH] = (double *)malloc(dz->linecnt * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for sync startsearch array.\n");
		return(MEMORY_ERROR);
	}
	start = dz->parray[MSY_STARTSRCH];
	if((dz->parray[MSY_ENDSRCH] = (double *)malloc(dz->linecnt * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for sync endsearch array.\n");
		return(MEMORY_ERROR);
	}
	end = dz->parray[MSY_ENDSRCH];
	if((dz->lparray[MSY_SAMPSIZE] = (int *)malloc(dz->linecnt * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for sync sampsizes array.\n");
		return(MEMORY_ERROR);
	}
	samplen = dz->lparray[MSY_SAMPSIZE];
	if((dz->iparray[MSY_CHANS] = (int *)malloc(dz->linecnt * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for sync channels array.\n");
		return(MEMORY_ERROR);
	}
	inchans = dz->iparray[MSY_CHANS];
	if((dz->lparray[MSY_PEAKSAMP] = (int *)malloc(dz->linecnt * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for sync peaks array.\n");
		return(MEMORY_ERROR);
	}
	if((exit_status = establish_file_data_storage_for_mix((int)1,dz))<0)
		return(exit_status);
	total_words = 0;
	for(n=0;n<dz->linecnt;n++) {
		if((exit_status = open_file_and_retrieve_props(n,dz->wordstor[total_words],&srate,dz))<0)
			return(exit_status);
		samplen[n]	= dz->insams[0];
		inchans[n]	= dz->infile->channels;
		total_words += dz->wordcnt[n];
	}

	if(srate > SAMPLE_RATE_DIVIDE)  dz->iparam[MSY_SRFAC]=2;
	else							dz->iparam[MSY_SRFAC]=1;

	minsyncscan = (double)((shsecsize * dz->iparam[MSY_SRFAC])/MAX_WINFAC)/(double)srate;

	total_words = 0;
	for(n=0;n<dz->linecnt;n++) {
		switch(dz->wordcnt[n]) {
		case(3):
			if(dz->infile->filetype==MIXFILE) {
				sprintf(errstr,"Anomalous line length [%d] in mixfile\n",dz->wordcnt[n]);
				return(PROGRAM_ERROR);
			}
			if(sscanf(dz->wordstor[total_words+1],"%lf",&(start[n]))!=1) {
				sprintf(errstr,"Failed to read starttime: line %d: get_filesearch_data()\n",n+1);
				return(PROGRAM_ERROR);
			}
			if(sscanf(dz->wordstor[total_words+2],"%lf",&(end[n]))!=1) {
				sprintf(errstr,"Failed to read endtime: line %d: get_filesearch_data()\n",n+1);
				return(PROGRAM_ERROR);
			}
			if((start[n] < 0.0) || (end[n] < 0.0) || (start[n] + minsyncscan >= end[n])) {
				sprintf(errstr,"Impossible or incompatible searchtimes [%.5lf to %.5lf]: line %d.\n",
				start[n],end[n],n+1);
				return(USER_ERROR);
			}
// TW MOVED July 2004
			filedur = (double)(samplen[n]/inchans[n])/(double)srate;
			if(start[n] >= filedur - minsyncscan) {
				sprintf(errstr,"starttime on line %d is beyond effective file end.\n",n+1);
				return(DATA_ERROR);
			}
			if(end[n] >= filedur)
				end[n] = -1.0;	/* flags END_OF_SNDFILE */
			break;
		default:
			start[n] = 0.0;
			end[n]   = -1.0;	/* flags END_OF_SNDFILE */
			break;
		}
		total_words += dz->wordcnt[n];
	}
	dz->infile->filetype = textfile_filetype;
	return(FINISHED);
}

/************************* OPEN_FILE_AND_RETRIEVE_PROPS *******************************/

int open_file_and_retrieve_props(int filecnt,char *filename,int *srate,dataptr dz)
{
	int exit_status;
	if((exit_status = open_file_retrieve_props_open(filecnt,filename,srate,dz))<0)
		return(exit_status);
	if(sndcloseEx(dz->ifd[0])<0) {
		sprintf(errstr, "Failed to close input file %s: line %d: open_file_and_retrieve_props()\n",filename,filecnt+1);
		return(SYSTEM_ERROR);
	}
	dz->ifd[0] = -1;
	return(FINISHED);
}

/************************* OPEN_FILE_RETRIEVE_PROPS_OPEN *******************************/

int open_file_retrieve_props_open(int filecnt,char *filename,int *srate,dataptr dz)
{
	int exit_status;
	double maxamp, maxloc;
	int maxrep;
	int getmax = 0, getmaxinfo = 0;
	infileptr ifp;
	if((ifp = (infileptr)malloc(sizeof(struct filedata)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store data on file %s\n",filename);
		return(MEMORY_ERROR);
	}
	/* OK to use sndopenEx? */
	if((dz->ifd[0] = sndopenEx(filename,0,CDP_OPEN_RDONLY)) < 0) {
		sprintf(errstr,"Failed to open sndfile %s: line %d: open_file_retrieve_props_open()\n",filename,filecnt+1);
		return(SYSTEM_ERROR);
	}
	/* this use revised version that recognizes floatsam sndfile */
	if((exit_status = readhead(ifp,dz->ifd[0],filename,&maxamp,&maxloc,&maxrep,getmax,getmaxinfo))<0)
		return(exit_status);
	copy_to_fileptr(ifp,dz->infile);
	if(dz->infile->filetype!=SNDFILE) {
		sprintf(errstr,"%s is not a soundfile: line %d: open_file_retrieve_props_open()\n",filename,filecnt+1);
		return(PROGRAM_ERROR);
	}
	if(filecnt==0)
		*srate = dz->infile->srate;
	else if(dz->infile->srate != *srate) {
		sprintf(errstr,"incompatible srate: file %s: line %d: open_file_retrieve_props_open()\n",filename,filecnt+1);
		return(DATA_ERROR);
	}
	if((dz->insams[0] = sndsizeEx(dz->ifd[0]))<0) {	    			
		sprintf(errstr, "Can't read size of input file %s: line %d: open_file_retrieve_props_open()\n",filename,filecnt+1);
		return(PROGRAM_ERROR);
	}
	if(dz->insams[0] <=0) {
		sprintf(errstr, "Zero size for input file %s: line %d: open_file_retrieve_props_open()\n",filename,filecnt+1);
		return(DATA_ERROR);
	}			
	return(FINISHED);
}

/***************************** SYNCATT_PRESETS **************************/

int syncatt_presets(dataptr dz)
{
	int exit_status;
	if(!check_syncatt_window_factor(dz))
			return(USER_ERROR);
	if((exit_status= get_filesearch_data(dz))<0)
		return(exit_status);
	return(FINISHED);
}

/***************************** CHECK_SYNCATT_WINDOW_FACTOR **************************/

int check_syncatt_window_factor(dataptr dz)
{
	int valid_value = MIN_WINFAC;
	while(valid_value <= MAX_WINFAC) {
		if(dz->iparam[MSY_WFAC]==valid_value)
			return(TRUE);
		valid_value *= 2;
	}
	return(FALSE);
}

/****************************** MIXTWARP_CONSISTENCY *********************************/

int mixtwarp_consistency(dataptr dz)
{
	switch(dz->mode) {
	case(MTW_TIMESORT):	
		break;
	default:
		if(dz->iparam[MSH_ENDLINE] < dz->iparam[MSH_STARTLINE]) {
			sprintf(errstr,"Start and endline values incompatible.\n");
			return(USER_ERROR);
		}
	}
	return(FINISHED);
}

/****************************** MIXSWARP_CONSISTENCY *********************************/

int mixswarp_consistency(dataptr dz)
{
	if(dz->mode!=MSW_TWISTALL && dz->mode!=MSW_TWISTONE) {
		if(dz->iparam[MSH_ENDLINE] < dz->iparam[MSH_STARTLINE]) {
			sprintf(errstr,"Start and endline values incompatible.\n");
			return(USER_ERROR);
		}
	}
	return(FINISHED);
}

/****************************** MIX_CONSISTENCY *********************************/

int mix_consistency(dataptr dz)
{
	if(dz->iparam[MSH_ENDLINE] < dz->iparam[MSH_STARTLINE]) {
		sprintf(errstr,"Start and endline values incompatible.\n");
		return(USER_ERROR);
	}
	return(FINISHED);
}

//TW UPDATE: NEW FUNCTION
/****************************** ADDTOMIX_CONSISTENCY *********************************/

int addtomix_consistency(dataptr dz)
{
	if(dz->infile->filetype != MIXFILE) {
		sprintf(errstr,"The FIRST FILE must be a MIXFILE.\n");
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
	case(MIX):			case(MIXMAX):			
		return create_mix_buffers(dz);

	case(MIXTWO):
	case(MIXBALANCE):
		return create_mixtwo_buffer(dz);

	case(MIXINTERL):
		dz->bufcnt = 1 + dz->infilecnt;
		return create_sndbufs(dz);
	case(MIXCROSS):		case(MIXINBETWEEN):		
	case(AUTOMIX):
		return create_sndbufs(dz);
	case(CYCINBETWEEN):			
		return create_cycinbi_buffer(dz);
	case(MIXMANY):			
		return create_mixmany_buffer(dz);

	case(MIXSYNCATT):		
		return create_syncatt_buffer(dz);

	case(MIXFORMAT):	case(MIXDUMMY):		case(MIXTEST):
	case(MIXTWARP):		case(MIXSWARP):		case(MIXGAIN):
	case(MIXSHUFL):		case(MIXSYNC):		case(MIX_MODEL):
	case(MIX_ON_GRID):	case(ADDTOMIX):		case(MIX_PAN):
	case(MIX_AT_STEP):	

		return(FINISHED);
	default:
		sprintf(errstr,"Unknown program no. in allocate_large_buffers()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);	/* NOTREACHED */
}

/*************************** CREATE_MIX_BUFFERS **************************
 *
 * (1)	Basic buffersize needs an extra factor of 2 because MONO_TO_STEREO conversion
 *		reads 1/2 a buffer into dz->sampbuf[STEREOBUF], and that 1/2 a buf needs to be
 *		a multiple of F_SECSIZE samples!!
 * (2)	As well as the input buffers, need an output buffer, 
 		and a stereo-buffer for panned-stereo.
 * (3)	Initialise space left in outbuffer to all of it.
 */

int create_mix_buffers(dataptr dz)
{
	int exit_status;
	size_t bigbufsize;
	size_t bloksize = SECSIZE * 2;  			/* 1 */
	/*RWD NB longsize_in_floats was longsize_in_shorts...*/
//TW MIXMAX CALCULATIONS ARE DONE IN FLOATS, SO BUFFER-OF-LONGS NOT NEEDED
	switch(dz->process) {
	case(MIX):
		dz->bufcnt += 2;					/* allow for panned_stereo_buffer and output buffer */
		break;
	case(MIXMAX):
//TW AMPLITUDE SUM NOW IN FLOATS
		dz->bufcnt += 2;					
		break;
	default:
		sprintf(errstr,"Unknown case: create_mix_buffers()\n");
		return(PROGRAM_ERROR);
	}
	bloksize = dz->bufcnt * bloksize;
	bigbufsize = (size_t)Malloc(-1);
	if((bigbufsize = (bigbufsize/bloksize) * bloksize) <= 0)
		bigbufsize = bloksize;
	if((dz->bigbuf = (float *)Malloc(bigbufsize)) == NULL) {
		sprintf(errstr, "INSUFFICIENT MEMORY to create sound buffers.\n");
		return(MEMORY_ERROR);
	}

	dz->buflen =(int)( bigbufsize/sizeof(float)/dz->bufcnt);	 		/* length of floats buffers */
//TW NO ADJUSTMENTS NEEDED FOR BUFFER-0F-LONGS
	if((exit_status = establish_groucho_bufptrs_and_extra_buffers(dz))<0)
		return(exit_status);
	dz->bufcnt -= 2;							 /* bufcnt is now a count of input buffers only */
	dz->sampbuf[OBUFMIX] = dz->bigbuf;
//TW NO ADJUSTMENTS NEEDED FOR BUFFER-0F-LONGS
	dz->sampbuf[STEREOBUF] = dz->bigbuf + dz->buflen;

	dz->sampbuf[IBUFMIX] = dz->sampbuf[STEREOBUF] + dz->buflen;
	return(FINISHED);
}

/*************************** CREATE_SYNCATT_BUFFER **************************/

int create_syncatt_buffer(dataptr dz)
{
	size_t bigbufsize;
	long n, bufactor = dz->iparam[MSY_SRFAC] * MAX_SYNCATT_CHANS;
	size_t fsecbytesize = F_SECSIZE * sizeof(float);
	bigbufsize = (size_t)Malloc(-1);
//TW SAFER
	if((bigbufsize  = (bigbufsize/(fsecbytesize*bufactor)) * (fsecbytesize*bufactor))<=0) {
		bigbufsize  = fsecbytesize*bufactor;
	}
	dz->buflen = (int)(bigbufsize/sizeof(float));
	if((dz->bigbuf = (float *)malloc((size_t)(dz->buflen * sizeof(float)))) == NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<dz->bufcnt;n++)
		dz->sbufptr[n] = dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
	dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
	return(FINISHED);
}

/*************************** CREATE_MIXTWO_BUFFER **************************/

int create_mixtwo_buffer(dataptr dz)
{
	size_t bigbufsize;
	int n, bufactor;
	int samptodouble = sizeof(double)/sizeof(float);
	size_t fsecbytesize = F_SECSIZE * sizeof(float);
	bigbufsize = (size_t) Malloc(-1);
	if(dz->process == MIXBALANCE) {
// MULTICHAN 2009 --> 
		if(dz->infile->channels > 2)
			dz->bufcnt = dz->infile->channels * 2;
// <-- MULTICHAN 2009
		if((bigbufsize  /= dz->bufcnt) <= 0) {
			sprintf(errstr,"INSUFFICIENT MEMORY for sound buffers.\n");
			return(MEMORY_ERROR);
		}
		if((bigbufsize = ((bigbufsize/fsecbytesize * dz->bufcnt) * fsecbytesize * dz->bufcnt)) <= 0) {
			bigbufsize = fsecbytesize * dz->bufcnt;
		}
		if((dz->bigbuf = (float *)malloc((size_t)(bigbufsize * dz->bufcnt))) == NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
			return(MEMORY_ERROR);

		}
		dz->buflen = (int)(bigbufsize/sizeof(float));
		dz->sampbuf[0] = dz->bigbuf;
// MULTICHAN 2009 --> 
		if(dz->infile->channels > 2)
			dz->sampbuf[1] = dz->sampbuf[0] + (dz->buflen * dz->infile->channels);
// <-- MULTICHAN 2009
		else if(dz->infile->channels == 1)
			dz->sampbuf[1] = dz->sampbuf[0] + dz->buflen;
		else
			dz->sampbuf[1] = dz->sampbuf[0] + (dz->buflen * 2);
	} else {
// MULTICHAN 2009 --> 
		if(dz->infile->channels > 2) {
			dz->bufcnt = 3 + samptodouble;
			bufactor  = dz->bufcnt * dz->infile->channels;
// <-- MULTICHAN 2009
		} else
			bufactor = (dz->bufcnt - 1) + samptodouble;
		if((bigbufsize  = (bigbufsize/(fsecbytesize*bufactor)) * (fsecbytesize*bufactor))<=0)
			bigbufsize  = fsecbytesize*bufactor;
		if((dz->bigbuf = (float *) malloc(bigbufsize) ) == NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
			return(MEMORY_ERROR);
		}
// MULTICHAN 2009 --> 
		if(dz->infile->channels > 2)
			bufactor = dz->bufcnt;
// <-- MULTICHAN 2009
		bigbufsize /= bufactor;
		dz->buflen = (int)(bigbufsize/sizeof(float));
		for(n=0;n<dz->bufcnt;n++)
			dz->sbufptr[n] = dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
		dz->sampbuf[n] = dz->bigbuf + (dz->buflen * bufactor);
	}
	return(FINISHED);
}

// TW NEW FUNCTION: ADJUSTED FOR floats
/*************************** CREATE_MIXMANY_BUFFER **************************/

int create_mixmany_buffer(dataptr dz)
{
	size_t n, bufactor, bigbufsize;
	size_t samptodouble = sizeof(double)/sizeof(float);
	size_t fsecbytesize = F_SECSIZE * sizeof(float);
	bigbufsize = (long)Malloc(-1);
	bufactor = dz->bufcnt + samptodouble;
	if((bigbufsize  = (bigbufsize/(fsecbytesize*bufactor)) * (fsecbytesize*bufactor))<=0) {
		bigbufsize  = fsecbytesize*bufactor;
	}
	if((dz->bigbuf = (float *)malloc(bigbufsize)) == NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
		return(MEMORY_ERROR);
	}
	bigbufsize /= bufactor;
	dz->buflen = (int)(bigbufsize/sizeof(float));
	for(n=0;n<dz->bufcnt;n++)
		dz->sbufptr[n] = dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
	dz->sampbuf[n] = dz->bigbuf + (dz->buflen * bufactor);
	return(FINISHED);
}

/*************************** CREATE_CYCINBI_BUFFER **************************/

int create_cycinbi_buffer(dataptr dz)
{
	int n;
	/*RWD April 2004 */ 
	int insize_0,insize_1, bigbufsize;
	insize_0 = dz->insams[0] * sizeof(float);
	insize_1 = dz->insams[1] * sizeof(float);
 /*	int bigbufsize = max(dz->infilesize[0], dz->infilesize[1]) + F_SECSIZE; */
	bigbufsize = max(insize_0, insize_1) + F_SECSIZE; 
	bigbufsize = (bigbufsize/F_SECSIZE) * F_SECSIZE * dz->bufcnt;
	if((dz->bigbuf = (float *)malloc((size_t)(bigbufsize))) == NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
		return(MEMORY_ERROR);
	}
	bigbufsize /= dz->bufcnt;
	dz->buflen = bigbufsize/sizeof(float);
	for(n=0;n<dz->bufcnt;n++)
		dz->sbufptr[n] = dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
	dz->sampbuf[n] = dz->bigbuf + (dz->buflen * dz->bufcnt);
	return(FINISHED);
}

/********************************************************************************************/
/********************************** FORMERLY IN cmdline.c ***********************************/
/********************************************************************************************/

int get_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	if     (!strcmp(prog_identifier_from_cmdline,"mix"))			dz->process = MIX;
	else if(!strcmp(prog_identifier_from_cmdline,"test"))			dz->process = MIXTEST;
	else if(!strcmp(prog_identifier_from_cmdline,"getlevel"))		dz->process = MIXMAX;
	else if(!strcmp(prog_identifier_from_cmdline,"fileformat"))		dz->process = MIXFORMAT;
	else if(!strcmp(prog_identifier_from_cmdline,"dummy"))			dz->process = MIXDUMMY;
	else if(!strcmp(prog_identifier_from_cmdline,"timewarp"))		dz->process = MIXTWARP;
	else if(!strcmp(prog_identifier_from_cmdline,"spacewarp"))		dz->process = MIXSWARP;
	else if(!strcmp(prog_identifier_from_cmdline,"attenuate"))		dz->process = MIXGAIN;
	else if(!strcmp(prog_identifier_from_cmdline,"shuffle"))		dz->process = MIXSHUFL;
	else if(!strcmp(prog_identifier_from_cmdline,"sync"))			dz->process = MIXSYNC;
	else if(!strcmp(prog_identifier_from_cmdline,"syncattack"))		dz->process = MIXSYNCATT;
	else if(!strcmp(prog_identifier_from_cmdline,"merge"))			dz->process = MIXTWO;
	else if(!strcmp(prog_identifier_from_cmdline,"balance"))		dz->process = MIXBALANCE;
	else if(!strcmp(prog_identifier_from_cmdline,"crossfade"))		dz->process = MIXCROSS;
	else if(!strcmp(prog_identifier_from_cmdline,"interleave"))		dz->process = MIXINTERL;
	else if(!strcmp(prog_identifier_from_cmdline,"inbetween"))		dz->process = MIXINBETWEEN;
//TW UPDATES
	else if(!strcmp(prog_identifier_from_cmdline,"mergemany"))		dz->process = MIXMANY;
	else if(!strcmp(prog_identifier_from_cmdline,"ongrid"))			dz->process = MIX_ON_GRID;
	else if(!strcmp(prog_identifier_from_cmdline,"atstep"))			dz->process = MIX_AT_STEP;
	else if(!strcmp(prog_identifier_from_cmdline,"faders"))			dz->process = AUTOMIX;
	else if(!strcmp(prog_identifier_from_cmdline,"addtomix"))		dz->process = ADDTOMIX;
	else if(!strcmp(prog_identifier_from_cmdline,"pan"))			dz->process = MIX_PAN;
	else if(!strcmp(prog_identifier_from_cmdline,"model"))			dz->process = MIX_MODEL;
	else if(!strcmp(prog_identifier_from_cmdline,"inbetween2"))		dz->process = CYCINBETWEEN;
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
	"USAGE: submix NAME (mode) infiles     outfile(s) [parameters]:\n"
	"OR:    submix NAME (mode) mixdatafile outfile    [parameters]:\n"
	"\n"
	"where NAME can be any one of\n"
	"\n"
	"merge      balance     crossfade   interleave   inbetween   inbetween2\n"
	"mix        getlevel      attenuate\n"
	"shuffle    timewarp      spacewarp      sync       syncattack\n"
	"test       fileformat    dummy     addtomix     atstep\n"
	"ongrid     faders      mergemany   pan          model\n"
	"\n"
	"Type 'submix  mix'  for more info on submix  mix.\n"
	"Type 'submix  fileformat'  for more info on mixfile fileformat... ETC\n");
	return(USAGE_ONLY);
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
	if(!strcmp(str,"mix")) {		
	    sprintf(errstr,
	    "MIX SOUNDS AS INSTRUCTED IN A MIXFILE\n\n"
    	"USAGE:	submix mix mixfile outsndfile [-sSTART] [-eEND] [-gATTENUATION] [-a]\n\n"
    	"START       gives starttime START (to start mixing later than zero).\n"
    	"END         gives output endtime END (to stop mix before its true end).\n"
    	"ATTENUATION reduces level of entire mix (range >0-1).\n\n"
		"Note that the START and END params are intended for mix TESTING purposes only.\n"
		"If you want to keep output from such a testmix, you should TOPNTAIL it.\n\n"
    	"-a    alternative mix algorithm, slightly slower,\n"
    	"      but may avoid clipping in special circumstances.\n");
	} else if(!strcmp(str,"test")) {		
	    sprintf(errstr,
	    "TEST THE SYNTAX OF A MIXFILE\n\n"
    	"USAGE:	submix test mixfile\n\n");
	} else if(!strcmp(str,"getlevel")) {		
	    sprintf(errstr,
	    "  TEST THE MAXIMUM LEVEL OF A MIX, DEFINED IN A MIXFILE\n"
	    "AND SUGGEST A GAIN FACTOR TO AVOID OVERLOAD, IF NECESSARY\n\n"
    	"USAGE: submix getlevel 1   mixfile             [-sSTART] [-eEND]\n"
    	"USAGE: submix getlevel 2-3 mixfile outtextfile [-sSTART] [-eEND]\n\n"
    	"MODES...\n"
    	"1) finds maximum level of mix.\n"
    	"2) finds locations of clipping in mix.\n"
    	"3) finds locations of clipping, and maxlevel, in mix.\n"
      	"START gives starttime START (to start mixing later than zero).\n"
    	"END   gives output endtime END (to stop mix before its true end).\n"
  	     "OUTTEXTFILE  stores clipping locations (& maxlevel, in mode 3).\n\n"
    	"You can alter the overall level of a mix with 'submix attenuate'.\n");
	} else if(!strcmp(str,"fileformat")) {		
	sprintf(errstr,
	"MIXFILES CONSIST OF LINES WITH ONE OF THE FOLLOWING FORMATS\n"
	"sndname starttime_in_mix  chans  level\n"
	"sndname starttime_in_mix  1      level       pan\n"
	"sndname starttime_in_mix  2      left_level  left_pan  right_level  right_pan\n\n"
	"SNDNAME is name of a MONO or STEREO sndfile: ALL MUST BE OF SAME SAMPLING RATE.\n"
	"CHANS   is number of channels in this soundfile (1 or 2 ONLY)\n"
	"LEVEL   is loudness, as number (1 = unity gain) or dB (0dB = unity gain)\n"
	"        Mono AND stereo files MAY have a SINGLE level parameter (NO pan data).\n"
	"        In this case, MONO files in STEREO mixes are panned centrally.\n"
	"        OTHERWISE....\n"
	"        MONO files   must have 1 level & 1 pan parameter (ONLY).\n"
	"        STEREO files MUST have 2 level & 2 pan params, 1 for each channel.\n"
	"PAN     is spatial positioning of file (or file channel) in output mix.\n"
	"        -1 HArd Left : 0 Centre : 1 Hard Right\n"
	"        < -1 hard left & attenuated : > 1 hard right & attenuated.\n"
	"ALSO....\n"
	"1) The mixfile list need NOT be in starttime order.\n"
	"2) Silence at start of mix IGNORED.(splice on afterwards if needed).\n"
	"3) With exclusively mono inputs, with NO pan information, OR \n"
	"   when ALL panned hard left, or ALL hard right, output is MONO.\n"
	"   All other situations produce stereo output.\n"
	"4) TAKE CARE WHEN PANNING BOTH CHANNELS OF A STEREO FILE.\n"
	"   The channel contributions sum, so e.g. if both channels are\n"
	"   panned to same position without attenuation, overload possible.\n"
	"5) You may test for maximum level in your mix output with submix GETLEVEL.\n"
	"   Reduce the mixfile level, if necessary, using 'submix attenuate'.\n"
	"6) You may put comment lines in mixfiles : Start such line with a ';'\n"
	"   Blank lines are ignored.\n");
	} else if(!strcmp(str,"dummy")) {		
	    sprintf(errstr,
	    "CONVERT A LIST OF SNDFILES INTO A BASIC MIXFILE (FOR EDITING)\n\n"
    	"USAGE: submix dummy mode infile1 infile2 [infile3..] mixfile\n"
		"mode 1 - all files start at time zero.\n"
		"mode 2 - each file starts where previous file ends.\n"
		"mode 3 - (mono) files, first to left, remainder to right.\n");
//TW UPDATES
	} else if(!strcmp(str,"ongrid")) {		
	    sprintf(errstr,
	    "CONVERT LISTED SNDFILES TO BASIC MIXFILE ON TIMED GRID (FOR EDITING)\n\n"
    	"USAGE: submix ongrid infil1 infil2 [infil3..] outmixfile gridfile\n\n"
 		"GRIDFILE a list of times (one for each input sound) of sounds in mix\n"
		"         OR a list with some times preceded by 'x' (no space after 'x')\n"
		"         where 'x' marks a time to actually use \n"
		"         (other grid times being ignored).\n\n"
		"Numbers, or gridpoint names, may follow times (on same line ONLY)\n\n"
		"IF NO SOUND USED AT TIME ZERO, mix will skip to first snd actually used.\n"
		"To avoid this, use a SILENT FILE at time zero.\n");
	} else if(!strcmp(str,"atstep")) {		
	    sprintf(errstr,
	    "CONVERT LISTED SNDFILES TO BASIC MIXFILE WITH FIXED TIMESTEP BETWEEN ENTRIES\n\n"
  	   	"USAGE: submix atstep infile1 infile2 [infile3..] outmixfile step\n\n"
		"STEP is time, in seconds, between entry of each file.\n\n");
	} else if(!strcmp(str,"faders")) {		
	    sprintf(errstr,
		"MIX SEVERAL MONO OR STEREO FILES USING TIME-CHANGING BALANCE-FUNCTION\n\n"
    	"USAGE: submix faders inf1 inf2 [inf3..] outf balance-data envelope-data\n\n"
    	"(NB do not mix mono and stereo in the input)\n"
		"BALANCE-DATA  is a list of value sets.\n"
		"              Each consisting of a time, followed by the\n"
		"              RELATIVE level of each file in the mix, at that time.\n\n"
		"              Whatever set of numbers is used, THESE ARE SCALED\n"
		"              so the total set of levels used ON ANY LINE adds up to 1.0\n"
		"              (This means that if all signals are at max level\n"
		"              the output will never exceed max level)\n\n"
		"              You can't use this data to vary the OVERALL level.\n"
		"              and, in particular, the values cannot ALL be zero.\n\n"
		"ENVELOPE-DATA is the loudness envelope to apply to the TOTAL sound.\n"
		"              and this (also) can vary over time.\n");

	} else if(!strcmp(str,"timewarp")) {		
		sprintf(errstr,
		"TIMEWARP THE DATA IN A MIXFILE\n\n"
		"USAGE:  submix timewarp 1    inmixfile outmixfile\n"
		"OR:     submix timewarp 2-5  inmixfile outmixfile   [-sstartline] [-eendline]\n"
		"OR:     submix timewarp 6-16 inmixfile outmixfile Q [-sstartline] [-eendline]\n\n"
		"STARTLINE: line at which warping begins (default: 1st in file)\n"
		"ENDLINE  : line at which warping ends  (default: last in file).\n"
		"MODES ARE....\n"
		"1)  SORT INTO TIME ORDER.\n"
		"2)  REVERSE TIMING PATTERN:  e.g. rit. of sound entries becomes an accel.\n"
		"3)  REVERSE TIMING PATTERN & ORDER OF FILENAMES.\n"
		"4)  FREEZE TIMEGAPS          between sounds, at FIRST timegap value.\n"
		"5)  FREEZE TIMEGAPS & NAMES  ditto, and all files take firstfile name.\n"
		"6)  SCATTER ENTRY TIMES      about orig vals. Q is scattering: Range(0-1).\n"
		"7)  SHUFFLE UP ENTRY TIMES   shuffle times in file forward by time Q secs.\n"
		"8)  ADD TO TIMEGAPS          add fixed val Q secs, to timegaps between sounds.\n"
		"9)  CREATE FIXED TIMEGAPS 1  between all sounds,timegap = Q secs\n"
		"10) CREATE FIXED TIMEGAPS 2  startval+Q,startval+2Q  etc\n"
		"11) CREATE FIXED TIMEGAPS 3  startval*Q startval*2Q etc\n"
		"12) CREATE FIXED TIMEGAPS 4  startval*Q     startval*Q*Q    etc\n"
		"13) ENLARGE TIMEGAPS 1       multiply them by Q.\n"
		"14) ENLARGE TIMEGAPS 2       by +Q, +2Q,+3Q  etc\n"
		"15) ENLARGE TIMEGAPS 3       by *Q *2Q *3Q\n"
		"16) ENLARGE TIMEGAPS 4       by *Q, *Q*Q, *Q*Q*Q  etc. (CARE!!)\n\n");
	} else if(!strcmp(str,"shuffle")) {		
		sprintf(errstr,
		"SHUFFLE THE DATA IN A MIXFILE\n\n"
		"USAGE: submix shuffle 1-6 inmixfile outmixfile         [-sstartl] [-eendl]\n"
		"OR:    submix shuffle 7   inmixfile outmixfile newname [-sstartl] [-eendl] [-x]\n\n"
		"STARTL: line at which shuffling begins (default: 1st in file)\n"
		"ENDL  : line at which shuffling ends  (default: last in file).\n"
		"MODES ARE....\n"
		"1)  DUPLICATE EACH LINE.\n"
		"2)  REVERSE ORDER OF FILENAMES.\n"
		"3)  SCATTER ORDER OF FILENAMES.\n"
		"4)  REPLACE SOUNDS IN SELECTED LINES WITH SOUND IN STARTLINE.\n"
		"5)  OMIT LINES           (closing up timegaps appropriately)\n"
		"6)  OMIT ALTERNATE LINES (closing up timegaps appropriately)\n"
		"                         In modes 5 & 6 mix must be in correct time-order.\n"
		"                         mixfiles can be time-ordered using timewarp mode 1\n"
		"7)  DUPLICATE AND RENAME: duplicate each line with new sound, newname.\n\n"
		"                         Program checks 'newname' is compatible sndfile, BUT,\n"
		"-x  flag TURNS OFF 'newname' checking in mode 7.\n");
	} else if(!strcmp(str,"attenuate")) {		
		sprintf(errstr,
		"ALTER THE OVERALL LEVEL OF A MIXFILE.\n\n"
		"USAGE:  submix attenuate inmixfile outmixfile gainval [-sstartline] [-eendline]\n\n"
		"GAINVAL must be > 0.0\n"
		"STARTLINE: line at which attenuation begins (default: 1st in file)\n"
		"ENDLINE  : line at which attenuation ends  (default: last in file).\n"
		"You can test the overall level of a mix with 'submix getlevel'\n");
	} else if(!strcmp(str,"spacewarp")) {		
		sprintf(errstr,
		"ALTER THE SPATIAL DISTRIBUTION OF A MIXFILE.\n\n"
		"USAGE: submix spacewarp 1-2 inmixfile outmixfile Q     [-sstartl] [-eendl]\n"
		"OR:    submix spacewarp 4-6 inmixfile outmixfile Q1 Q2 [-sstartl] [-eendl]\n"
		"OR:    submix spacewarp 7   inmixfile outmixfile\n"
		"OR:    submix spacewarp 8   inmixfile outmixfile Q\n"
		"STARTL: line at which warping begins (default: 1st in file)\n"
		"ENDL  : line at which warping ends  (default: last in file).\n"
		"MODES ARE....\n"
		"1) SOUNDS TO SAME POSITION           Q is position. (stereo files become mono)\n"
		"2) NARROW SPATIAL SPREAD             Q is a +ve number < 1.0\n"
		"3) SEQUENCE POSITIONS LEFTWARDS    over range Q1-Q2 (stereo files become mono)\n"
		"4) SEQUENCE POSITIONS RIGHTWARDS   over range Q1-Q2 (stereo files become mono)\n"
		"5) RANDOM-SCATTER POSITIONS      within range Q1-Q2 (stereo files become mono)\n"
		"6) RANDOM, BUT ALTERNATE TO L/R OF THE CENTRE OF THE SPATIAL RANGE SPECIFIED.\n"
		"                                        range Q1-Q2 (stereo files become mono)\n"
		"7) INVERT STEREO IN ALTERNATE LINES OF MIXFILE: (Use to avoid clipping).\n"
		"8) INVERT STEREO IN SPECIFIED LINE OF MIXFILE   Q is line number.\n");
	} else if(!strcmp(str,"sync")) {		
		sprintf(errstr,
		"SYNCHRONISE SOUNDFILES IN A MIXFILE,\n"
		"OR GENERATE SUCH A MIXFILE FROM A LIST OF SNDFILES.\n\n"
		"USAGE: submix sync mode intextfile outmixfile\n\n"
		"INTEXTFILE is list of sndfiles OR an existing mixfile.\n\n"
		"MODES ARE....\n"
		"1) SYNC SNDFILE MIDTIMES.\n"		
		"2) SYNC SNDFILE ENDTIMES.\n");
	} else if(!strcmp(str,"syncattack")) {		
		sprintf(errstr,
		"SYNCHRONISE ATTACKS OF SOUNDFILES, IN A MIXFILE,\n"
		"OR GENERATE SUCH A MIXFILE FROM A LIST OF SNDFILES.\n\n"
		"USAGE: submix syncattack intextfile outmixfile  [-wdiv] [-p]\n\n"
		"INTEXTFILE is a list of sndfiles OR an existing mixfile.\n"		
		"           with a sndfilelist (only), each sndname MAY be followed by 2 times,\n"
		"           which limit the search area for the sound's attack.\n"
		"-w         DIV is factor shortening window which scans for the attack.\n"
   		"           It can be 2,4,8,16, or 32 ONLY.\n"
   		"-p         Program finds peak-power segment, before locating its max sample.\n"
   		"           Default: program looks purely for maxsample.\n\n"
		"The program estimates output levels required to prevent clipping,\n"
		"But estimate may be over-cautiously low: adjust with 'submix attenuate'.\n");
	} else if(!strcmp(str,"merge")) {		
		sprintf(errstr,
		"QUICK MIX OF 2 SNDFILES (Mono or Stereo only).\n\n"
		"USAGE: submix merge sndfile1 sndfile2 outfile\n"
		"          [-sstagger] [-jskip] [-kskew] [-bstart] [-eend]\n"
		"\n"
		"STAGGER ...2nd file enters, 'stagger' secs after first.\n"
		"SKIP ......skip by 'skip' secs into the 2nd file, before starting to mix.\n"
		"SKEW.......1st sound has 'skew' times more gain than 2nd.\n"
		"START .....Start the mix at the time specified.\n"
		"END........Stop the mix at the time specified.\n"
		"Stagger and Skip are approximated to within about one hundredth of a second.\n");
//TW UPDATE
	} else if(!strcmp(str,"mergemany")) {		
		sprintf(errstr,
		"QUICK MIX OF SEVERAL SNDFILES (WITH SAME NUMBER OF CHANNELS).\n\n"
		"USAGE: submix mergemany sndfile1 sndfile2 [sndfile3 ....] outfile\n");
	} else if(!strcmp(str,"balance")) {		
		sprintf(errstr,
		"MIX BETWEEN 2 SNDFILES USING A BALANCE FUNCTION.\n\n"
		"USAGE: submix balance sndfile1 sndfile2 outfile\n"
		"                            [-kbalance] [-bstart] [-eend]\n\n"
		"(files may or may not have different number of channels).\n"
		"\n"
		"BALANCE ...Describes the relative level of the two sounds. (Range 0-1)\n"
		"......File 1 level is multiplied by the balance function.\n"
		"......File 2 level is multiplied by the inverse of the balance function.\n"
		"Balance may vary over time.\n\n"
		"START .....Start the mix at the time specified.\n"
		"END........Stop the mix at the time specified.\n");	
	} else if(!strcmp(str,"crossfade")) {		
		sprintf(errstr,
		"QUICK CROSSFADE BETWEEN SNDFILES (WITH SAME NUMBER OF CHANNELS).\n\n"
		"USAGE: submix crossfade 1 sndfile1 sndfile2 outfile\n\t\t[-sSTAGGER] [-bBEGIN] [-eEND]\n"
		"OR:    submix crossfade 2 sndfile1 sndfile2 outfile\n\t\t[-sSTAGGER] [-bBEGIN] [-eEND] [-pPOWFAC]\n\n"
		"MODES ARE...\n"
		"1) Linear crossfade.\n"
		"2) Cosinusiodal crossfade\n\n"
		"Crossfade is from sndfile1 towards sndfile2.\n\n"
	    "STAGGER   2nd file starts 'stagger' secs. after 1st. (Default: 0)\n"
		"BEGIN     crossfade starts at BEGIN secs. (>Stagger) (Default: 0)\n"
		"END       crossfade ends at END secs. (>Begin) (Default: end of shortest file)\n"
		"          If crossfade ends before end of file2, remainder of file2 plays on.\n"
	    "POWFAC    crossfade skew.\n"
		"          if powfac = 1, cosinusoidal-crossfade is normal.\n"
		"          in range %.2lf - 1, cosin-fade begins rapidly then slows.\n"
		"          in range 1 - %.0lf, cosin-fade begins slowly, then speeds up.\n\n"
 		"Stagger approximated in c. hundredths of a sec. For more precise stagger, \n" 
		"splice silence to start of sndfile2 & use stagger 0 (or use 'submix mix').\n\n"
		"(Only spectral morphing will create true morph between 2 sounds).\n",MIN_MCR_POWFAC,MAX_MCR_POWFAC);
	} else if(!strcmp(str,"interleave")) {		
		sprintf(errstr,
		"INTERLEAVE MONO FILES TO MAKE MULTICHANNEL OUTFILE.\n\n"
		"USAGE: submix interleave sndfile1 sndfile2 [sndfile3 sndfile4] outfile\n\n"
		"MAx number of channels in output is %d\n"
		"First sndfile goes to left channel of stereo, (or channel 1 of 4), etc.\n",MAX_MI_OUTCHANS);
	} else if(!strcmp(str,"inbetween")) {		
		sprintf(errstr,
		"GENERATE A SET OF SOUNDS IN-BETWEEN THE 2 INPUT SOUNDS\n"
		"      THROUGH WEIGHTED MIXES OF THE INPUT SOUNDS,\n"
		"         FROM MOSTLY SOUND1 TO MOSTLY SOUND2.\n\n"
		"USAGE: submix inbetween 1 infile1  infile2  outname  count\n"
		"OR:    submix inbetween 2 infile1  infile2  outname  ratios\n\n"
		"OUTNAME  is your GENERIC name for the output sndfiles.\n"
		"         New soundfiles will be called outname001 outname002 etc\n\n"
		"COUNT    If a single number (COUNT) is given to the program,\n"
		"         it generates amplitude ratios for the new sounds, automatically,\n"
		"         and COUNT is the number of inbetween outfiles to produce.\n\n"
		"RATIOS   A ratio, or a list of ratios in a textfile. These are\n"
		"         the level of file2 RELATIVE to file 1,for each new outfile,\n"
		"         as FRACTIONS (Range 0-1)\n"
		"         There must be an even number of values and in ascending order.\n\n"
		"Ensure NONE of the files to be created already exists!!\n");
	} else if(!strcmp(str,"inbetween2")) {		
		sprintf(errstr,
		"GENERATE A SET OF SOUNDS IN-BETWEEN THE 2 INPUT SOUNDS\n"
		"      THROUGH INTERPOLATION PEGGED TO ZERO_CROSSINGS.\n\n"
		"USAGE: submix inbetween2 infile1  infile2  outname  count  cutoff\n"
		"OUTNAME  is your GENERIC name for the output sndfiles.\n"
		"         New soundfiles will be called outname001 outname002 etc\n\n"
		"COUNT    is the number of inbetween outfiles to produce.\n\n"
		"CUTOFF   Is the frquency above which 'cycles' are ignored: \n"
		"         usually noise, they are incorporated into other cycles.\n\n"
		"Ensure NONE of the files to be created already exists!!\n");
	} else if(!strcmp(str,"addtomix")) {		
		sprintf(errstr,
		"ADD SOUNDFILES (at max level and time end-of-file) TO EXISTING MIXFILE.\n\n"
		"USAGE: submix addtomix mixfile sndfile1 [sndfile2 ........] outfile\n");
	} else if(!strcmp(str,"model")) {		
		sprintf(errstr,
		"REPLACE SOUNDFILES IN EXISTING MIXFILE.\n\n"
		"USAGE: submix model mixfile sndfile1 [sndfile2 ........] outfile\n");
	} else if(!strcmp(str,"pan")) {		
		sprintf(errstr,
		"PAN A MIXFILE.\n\n"
		"USAGE: submix pan inmixfile outmixfile pan\n\n"
		"PAN may vary over time\n"
		"    in this case, soundfiles in mix are POSITIONED differently\n"
		"    depending on time they begin, and value in PAN file at that time.\n"
		"    BUT the sounds THEMSELVES will not be panned (in the final mixdown)\n"); 
	} else
		sprintf(errstr,"Unknown option '%s'\n",str);
	return(USAGE_ONLY);
}

/******************************** USAGE3 ********************************/

int usage3(char *str1,char *str2)
{	
	if(!strcmp(str1,"test"))		
		return(CONTINUE);
	else if(!strcmp(str1,"fileformat"))	{
		sprintf(errstr,"Too many parameters on command line.\n");
		return(USAGE_ONLY);
	} else
		sprintf(errstr,"Insufficient parameters on command line.\n");
	return(USAGE_ONLY);
}

/******************************** INNER_LOOP (redundant)  ********************************/

int inner_loop
(int *peakscore,int *descnt,int *in_start_portion,int *least,int *pitchcnt,int windows_in_buf,dataptr dz)
{
	return(FINISHED);
}


/************************* MIX_ATTEN ******************************/

int mix_atten(dataptr dz)
{
	int    exit_status;
	int   lineno, total_words;

 	total_words = 0;
	for(lineno=0;lineno<dz->linecnt;lineno++) {
		if((exit_status = adjust_levels(lineno,total_words,dz))<0) {
			return(exit_status);
		}
		total_words += dz->wordcnt[lineno];
	}
	return(FINISHED);
}

/********************** ADJUST_LEVELS ****************************/
 
int adjust_levels(int lineno,int total_words,dataptr dz)
{
	int exit_status;
	int wordcnt = dz->wordcnt[lineno], len;
	char *thisword, temp[200]; 
	double level;
	int wordno = total_words + MIX_LEVELPOS;

	thisword = dz->wordstor[wordno];
	if((exit_status = get_level(thisword,&level))<0)
		return(exit_status);
	level *= dz->param[MIX_ATTEN];
	sprintf(temp,"%lf",level);
	if((len = (int)strlen(temp)) > (int)strlen(thisword)) {
		if((dz->wordstor[wordno] = (char *)realloc((char *)dz->wordstor[wordno],(len+1)))==NULL)
			return(MEMORY_ERROR);
	}	
	strcpy(dz->wordstor[wordno],temp);
	if(wordcnt <= MIX_RLEVELPOS)
		return(FINISHED);

	wordno = total_words + MIX_RLEVELPOS;
	thisword = dz->wordstor[wordno];
	if((exit_status = get_level(thisword,&level))<0)
		return(exit_status);
	level *= dz->param[MIX_ATTEN];
	sprintf(temp,"%lf",level);
	if((len = (int)strlen(temp)) > (int)strlen(thisword)) {
		if((dz->wordstor[wordno] = (char *)realloc((char *)dz->wordstor[wordno],(len+1)))==NULL)
			return(MEMORY_ERROR);
	}	
	strcpy(dz->wordstor[wordno],temp);
	return(FINISHED);
}

//TW UPDATE: NEW FUNCTIONS
/********************** READ_MIXGRID_FILE ****************************/
 
int read_mixgrid_file(char *str,dataptr dz)
{
	int linecnt = 0, timecnt = 0, is_marked = 0;
	char temp[200], *q, *p;
	double time, last_time = 0.0;
	int arraysize = BIGARRAY;
	FILE *fp;

	if((fp = fopen(str,"r"))==NULL) {
		sprintf(errstr,"Failed to open file %s to read the grid data\n",str);
		return(DATA_ERROR);
	}
	if((dz->parray[0] = (double *)malloc(arraysize * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for grid data.\n");
		return(MEMORY_ERROR);
	}
	while(fgets(temp,200,fp)==temp) {
		q = temp;
		linecnt++;
		if(!get_word_from_string(&q,&p))
			continue;
		if(*p == 'x') {
			p++;
			if(!is_marked) {
				timecnt = 0;
				is_marked = 1;
			}
		} else if(is_marked)
			continue;
		if(sscanf(p,"%lf",&time)!=1) {
			sprintf(errstr,"Failed to find a time at line %d\n",linecnt);
			return(DATA_ERROR);
		}
		if(time < 0.0) {
			sprintf(errstr,"Invalid timevalue (%lf) at line %d\n",time,linecnt);
			return(DATA_ERROR);
		}
		if(timecnt>0) {
			if(time < last_time) {
				if(is_marked)
					sprintf(errstr,"Marked times not in ascending order at line %d time %lf\n",linecnt,time);
				else
					sprintf(errstr,"Times not in ascending order at line %d time %lf\n",linecnt,time);
				return(DATA_ERROR);
			}
		}
		last_time = time;
		dz->parray[0][timecnt] = time;
		if(++timecnt >= arraysize) {
			arraysize += BIGARRAY;
			if((dz->parray[0] = (double *)realloc((char *)dz->parray[0],arraysize * sizeof(double)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY for grid data.\n");
				return(MEMORY_ERROR);
			}
		}
	}						
	if(timecnt != dz->infilecnt) {
		if(timecnt < dz->infilecnt) {
			if(is_marked)
				sprintf(errstr,"Insufficient times marked (%d) for the number of input files (%d).\n",
				timecnt,dz->infilecnt);
			else
				sprintf(errstr,"Insufficient times listed (%d) for the number of input files (%d).\n",
				timecnt,dz->infilecnt);
			return(DATA_ERROR);
		} else {
			if(is_marked)
				fprintf(stdout,"WARNING: Too many times marked (%d) for the number of input files (%d).\n",
				timecnt,dz->infilecnt);
			else
				fprintf(stdout,"WARNING: Too many times listed (%d) for the number of input files (%d).\n",
				timecnt,dz->infilecnt);
			fflush(stdout);
		}
	}
	if((dz->parray[0] = (double *)realloc((char *)dz->parray[0],dz->infilecnt * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for reallocate storage for grid data.\n");
		return(MEMORY_ERROR);
	}
	if(!flteq(dz->parray[0][0],0.0)) {
		fprintf(stdout,"WARNING: First time used is NOT AT ZERO :  Mix will start at start of 1st file\n");
		fprintf(stdout,"WARNING: and will be curtailed by %.4lf secs at start.\n",dz->parray[0][0]);
		fflush(stdout);
	}
	return(FINISHED);
}

/*************************** READ_AND_SCALE_BALANCE_DATA *******************************/

int read_and_scale_balance_data(char *filename,dataptr dz)
{
	FILE *fp;
	double *p, sum, errtime = 1.0/dz->infile->srate, lasttime = 0.0;
	int datalen = dz->infilecnt + 1;
	int arraysize = BIGARRAY;
	char temp[200], *q;
	int n = 0, m;
	if((fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,	"Can't open textfile %s to read data.\n",filename);
		return(DATA_ERROR);
	}
	if((dz->parray[0] = (double *)malloc(arraysize * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for data.\n");
		return(MEMORY_ERROR);
	}
	p = dz->parray[0];
	while(fgets(temp,200,fp)==temp) {
		q = temp;
		while(*q != ENDOFSTR) {
			if(!isspace(*q))
				break;
			q++;
		}
		if(*q == ';')
			continue;
		if(invalid_data_items(temp))
			return(DATA_ERROR);
		while(get_float_from_within_string(&q,p)) {
			if(n%datalen==0)	{ /* time value */
				if(n==0) {
					lasttime = *p;
					if(*p != 0.0) {
						sprintf(errstr,"Data in datafile %s must begin at time zero\n",filename);
						return(DATA_ERROR);
					}
				} else {
					if (*p < lasttime + errtime) {
						sprintf(errstr,"Times %lf and %lf too close in datafile %s\n",*p,lasttime,filename);
						return(DATA_ERROR);
					}
				}
			} else if(*p < 0.0) {
				sprintf(errstr,"Negative gain values not permitted in datafile %s\n",filename);
				return(DATA_ERROR);
			}
			p++;
			if(++n >= arraysize) {
				arraysize += BIGARRAY;
				if((dz->parray[0] = (double *)realloc((char *)dz->parray[0],arraysize * sizeof(double)))==NULL) {
					sprintf(errstr,"INSUFFICIENT MEMORY to reallocate data table.\n");
					return(MEMORY_ERROR);
				}
				p = dz->parray[0] + n;		
			}
		}
		if(n%datalen == 0) {
			sum = 0.0;
			for(m=1;m <= dz->infilecnt;m++)
				sum += *(p-m);
			if(sum <= 0.0) {
				sprintf(errstr,"levels are zero at line %d\n",(n/datalen)+1);
				return(DATA_ERROR);
			}
			sum = 1.0/sum;
			for(m=1;m <= dz->infilecnt;m++)
				*(p-m) *= sum;	/* scale all gain values */
		}
	}
	if(n == 0) {
		sprintf(errstr,"No data in textdata file %s\n",filename);
		return(DATA_ERROR);
	}
	if(n%datalen != 0) {
		sprintf(errstr,"Data in textdata file %s is not grouped correctly\n",filename);
		return(DATA_ERROR);
	}
	if((dz->parray[0] = (double *)realloc((char *)dz->parray[0],n * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to reallocate data table.\n");
		return(MEMORY_ERROR);
	}
	if(fclose(fp)<0) {
		fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
		fflush(stdout);
	}
	dz->itemcnt = n;
	p  = dz->parray[0];
	return(FINISHED);
}

/****************************** MIXMODEL_CONSISTENCY *********************************/

int mixmodel_consistency(dataptr dz)
{
	infileptr fpq;
	int exit_status;
	int n, m, sndfilecnt, totalwords = 0, thischans, filestart;
	double maxamp, maxloc;
	int maxrep;
	int getmax = 0, getmaxinfo = 0;
	if(dz->infile->filetype != MIXFILE) {
		sprintf(errstr,"The FIRST FILE must be a MIXFILE.\n");
		return(USER_ERROR);
	}
	for(n=0;n<dz->linecnt;n++)
		totalwords += dz->wordcnt[n];
	filestart = totalwords;
	dz->itemcnt = filestart;
	totalwords = 0;
	if((fpq = (infileptr)malloc(sizeof(struct filedata)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store header data of soundfiles.\n");
		return(MEMORY_ERROR);
	}
	for(n=0,m=filestart,sndfilecnt=1;n<dz->linecnt;n++,m++,sndfilecnt++) {
		if(sndfilecnt >= dz->infilecnt) {
			sprintf(errstr,"Insufficient soundfiles entered\n");
			return(DATA_ERROR);
		}
		if((sscanf(dz->wordstor[totalwords + 2],"%d",&thischans)) != 1) {
			sprintf(errstr,"Failed to get channel count in line %d of mixfile\n",m);
			return(DATA_ERROR);
		}
		if((exit_status = readhead(fpq,dz->ifd[sndfilecnt],dz->wordstor[m],&maxamp,&maxloc,&maxrep,getmax,getmaxinfo))<0)
			return(exit_status);
		if(fpq->channels != thischans) {
			sprintf(errstr,"channel count in line %d of mixfile incompatible with entered file %s\n",m,dz->wordstor[m]);
			return(DATA_ERROR);
		}
		totalwords += dz->wordcnt[n];
	}
	if(sndfilecnt != dz->infilecnt) {
		sprintf(errstr,"Too many soundfiles entered\n");
		return(DATA_ERROR);
	}
	return FINISHED;
}

/************** READ_INBETWEEN_RATIOS ***********/

int read_inbetween_ratios(char *str,dataptr dz)
{
	char temp[200], *p;
	double q;
	int n;
	int arraysize = BIGARRAY;
	FILE *fp;

	if((fp = fopen(str,"r"))==NULL) {
		sprintf(errstr,"Failed to open file %s to read the inbetween ratios\n",str);
		return(DATA_ERROR);
	}
	if((dz->brk = (double **)malloc(sizeof(double *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for grid data.\n");
		return(MEMORY_ERROR);
	}
	if((dz->no_brk = (char *)malloc(sizeof(char)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for grid data.\n");
		return(MEMORY_ERROR);
	}
	if((dz->iparam = (int *)malloc(sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for grid data.\n");
		return(MEMORY_ERROR);
	}
	if((dz->brk[0] = (double *)malloc(arraysize * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for grid data.\n");
		return(MEMORY_ERROR);
	}
	n = 0;
	while(fgets(temp,200,fp)==temp) {
		p = temp;
		while(get_float_from_within_string(&p,&q)) {
			if(n > MAXBETWEEN) {
				sprintf(errstr,"Too many inbetween-ratios in file: maximum %d\n",MAXBETWEEN);
				return(USER_ERROR);
			}
			if(n >= arraysize) {
				arraysize += BIGARRAY;
				if((dz->brk[0] = (double *)realloc((char *)dz->brk[0],arraysize * sizeof(double)))==NULL) {
					sprintf(errstr,"INSUFFICIENT MEMORY for inbetween ratios.\n");
					return(MEMORY_ERROR);
				}
			}
			if(q < 0.0 || q > 1.0) {
				sprintf(errstr,"Inbetween value (%lf) out of range (0-1) at item %d\n",q,n+1);
				return(DATA_ERROR);
			}
			dz->brk[0][n] = q;
			n++;
		}
	}						
	if((dz->brk[0] = (double *)realloc((char *)dz->brk[0],n * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for reallocate storage for grid data.\n");
		return(MEMORY_ERROR);
	}
	dz->no_brk[INBETW] = FALSE;
	dz->iparam[INBETW] = n;
	return(FINISHED);
}

/**************************** CYC_PREPROP *************************/

int cyc_preprop(dataptr dz)
{
	int n0, n1, lastn0, lastn1, c0, c1, cyclecnt;
	int OK;
	float *ibuf0 = dz->sampbuf[0], *ibuf1 = dz->sampbuf[1];
	int *cycs0, *cycs1, *lens0, *lens1;
	int n;
	int hifrq_cut = (int)round((double)dz->infile->srate/dz->param[BTWN_HFRQ]);

	if((dz->ssampsread = fgetfbufEx(ibuf0,dz->buflen,dz->ifd[0],0)) <= 0) {
		sprintf(errstr,"Failed to read data: Infile 1\n");
		return(SYSTEM_ERROR);
	}
	if((dz->ssampsread = fgetfbufEx(ibuf1,dz->buflen,dz->ifd[1],0)) <= 0) {
		sprintf(errstr,"Failed to read data: Infile 2\n");
		return(SYSTEM_ERROR);
	}
	n0 = 0;
	while(ibuf0[n0] == 0) {
		if(++n0 >= dz->insams[0]) {
			sprintf(errstr,"No signal in first soundfile\n");
			return(DATA_ERROR);
		}
	}
	if(ibuf0[n0] < 0) {		/* force positive phase */
		for(n=0;n<dz->insams[0];n++)
			ibuf0[n] = (float)(-ibuf0[n]);
	}
	n1 = 0;
	while(ibuf1[n1] == 0) {
		if(++n1 >= dz->insams[1]) {
			sprintf(errstr,"No signal in second soundfile\n");
			return(DATA_ERROR);
		}
	}
	if(ibuf1[n1] < 0) {	/* force positive phase */
		for(n=0;n<dz->insams[1];n++)
			ibuf1[n] = (float)(-ibuf1[n]);
	}
	n0 = 0;
	n1 = 0;
	OK = 1;
	c0 = 0;
	c1 = 0;
	while(OK) {				/* This loop counts wavesets, with upper frq limit */
		lastn0 = n0;
		c0++;
		lastn1 = n1;
		c1++;
		while(OK) {
			while(ibuf0[n0] >= 0) {
				if(++n0 >= dz->insams[0]) {
					OK = 0;
					break;
				}
			}
			if(OK) {
				while(ibuf0[n0] < 0) {
					if(++n0 >= dz->insams[0]) {
						OK = 0;
						break;
					}
				}
			}
			if(!OK || (n0 - lastn0 > hifrq_cut))
				break;
		}
		while(OK) {
			while(ibuf1[n1] >= 0) {
				if(++n1 >= dz->insams[1]) {
					OK = 0;
					break;
				}
			}
			if(OK) {
				while(ibuf1[n1] < 0) {
					if(++n1 >= dz->insams[1]) {
						OK = 0;
						break;
					}
				}
			}
			if(!OK  || (n1 - lastn1 > hifrq_cut))
				break;
		}
	}

	if(c1 != c0) {
		sprintf(errstr,"Unequal cycle counts\n");
		return(PROGRAM_ERROR);
	}
	cyclecnt = c0;
	if((dz->lparray[0] = malloc(cyclecnt * sizeof(int)))==NULL) {
		sprintf(errstr,"Insufficient memory to store cycle positions for sound 1\n");
		return(MEMORY_ERROR);
	}
	if((dz->lparray[1] = malloc(cyclecnt * sizeof(int)))==NULL) {
		sprintf(errstr,"Insufficient memory to store cycle positions for sound 2\n");
		return(MEMORY_ERROR);
	}
	if((dz->lparray[2] = malloc((cyclecnt - 1) * sizeof(int)))==NULL) {
		sprintf(errstr,"Insufficient memory to store cycle lengths for sound 1\n");
		return(MEMORY_ERROR);
	}
	if((dz->lparray[3] = malloc((cyclecnt -1) * sizeof(int)))==NULL) {
		sprintf(errstr,"Insufficient memory to store cycle lengths for sound 2\n");
		return(MEMORY_ERROR);
	}
	cycs0 = dz->lparray[0];
	cycs1 = dz->lparray[1];
	lens0 = dz->lparray[2];
	lens1 = dz->lparray[3];

	n0 = 0;
	n1 = 0;
	c0 = 0;
	c1 = 0;
	OK = 1;
	while(OK) {			/* This loop stores the zero-crossing positions */
		cycs0[c0++] = n0;
		cycs1[c1++] = n1;
		lastn0 = n0;
		lastn1 = n1;
		if(c0 >= cyclecnt)
			OK = 0;
		else {
			while(OK) {
				while(ibuf0[n0] >= 0)
					n0++;
				while(ibuf0[n0] < 0)
					n0++;
				if(n0 - lastn0 > hifrq_cut)
					break;
			}
			while(OK) {
				while(ibuf1[n1] >= 0)
					n1++;
				while(ibuf1[n1] < 0)
					n1++;
				if(n1 - lastn1 > hifrq_cut)
					break;
			}
		}
	}
	for(n = 1;n < cyclecnt; n++) {
		lens0[n-1] = cycs0[n] - cycs0[n-1];
		lens1[n-1] = cycs1[n] - cycs1[n-1];
	}
	dz->itemcnt = cyclecnt;
	return FINISHED;
}

int invalid_data_items(char *str) 
{
	char *p = str;
	while(*p != ENDOFSTR) {
		if(!(isspace(*p) || isdigit(*p) || *p =='.' || *p == '-')) {
			sprintf(errstr,"Invalid (non-numeric) data in balance function.\n");
			return 1;
		}
		p++;
	}
	return 0;
}
