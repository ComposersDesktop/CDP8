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



/* floatsam version TW */
#include <stdio.h>
#include <stdlib.h>
#include <structures.h>
#include <cdpmain.h>
#include <tkglobals.h>
#include <pnames.h>
#include <highlight.h>
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
#include <vowels2.h>

#include <sfsys.h>
#include <osbind.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <srates.h>

#if defined unix || defined __GNUC__
#define round(x) lround((x))
#endif

/********************************************************************************************/
/********************************** FORMERLY IN pconsistency.c ******************************/
/********************************************************************************************/

static int	check_arpe_parameters_are_compatible(dataptr dz);
static int	check_validity_of_trace_vals_for_bltr(dataptr dz);

/********************************************************************************************/
/********************************** FORMERLY IN procspec.c **********************************/
/********************************************************************************************/

static int  create_and_initialise_array_of_loudest_channel_pointers(int arraysize,dataptr dz);
static int  make_room_in_loudchans_list_by_moving_later_items_down_a_place(int size_of_list,
				int index_into_list,dataptr dz);
static int  prepare_to_do_trace(double *thistime,dataptr dz);
static int  if_loud_enough_store_in_ring(int cc,int vc,dataptr dz);
static int	 create_trace(dataptr dz);

/********************************************************************************************/
/********************************** FORMERLY IN preprocess.c ********************************/
/********************************************************************************************/

static int  arpe_preprocess(dataptr dz);
static int  set_arpegtones_sustain_type(dataptr dz);
static int  gen_arpe_waveform_table(dataptr dz);
static int  setup_internal_arpe_params_and_preprocess_params(dataptr dz);
static int  fix_inverted_frqrange_in_arpe(dataptr dz) ;
static int  establish_arpe_halfband_param(dataptr dz);
static int  do_log_conversions_for_specfilt(dataptr dz);
static int  convert_brkpnt_table_vals_to_log(int brktableno,dataptr dz);

/********************************************************************************************/
/********************************** FORMERLY IN specialin.c *********************************/
/********************************************************************************************/

static int  generate_filter_data(char *str,int datatype,dataptr dz);
static int  get_filt_params(char *str,int datatype,double **filtwidth,dataptr dz);
static int  check_filtfrq(double val, double minval, double maxval);
static int  check_filtbw(double val, double minval, double maxval);
static int  sort_peaks_by_frq(double *filtwidth,dataptr dz);
static int  setup_filter_param_arrays(dataptr dz);
static int  generate_filter_params(double *filtwidth,dataptr dz);
static int  read_specsplit_data(char *str,dataptr dz);
static int  sort_frqbands_into_ascending_order(dataptr dz);
static void swap_frqbands(int n,int m,dataptr dz);

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
	case(FILT):       		dz->extra_bufcnt =  0; dz->bptrcnt = 2; 	is_spec = TRUE;		break;
//TW NEW CASE
	case(VFILT): 			dz->extra_bufcnt =  0; dz->bptrcnt = 2; 	is_spec = TRUE;		break;

	case(GREQ):       		dz->extra_bufcnt =  0; dz->bptrcnt = 1; 	is_spec = TRUE;		break;
	case(SPLIT):      		dz->extra_bufcnt =  0; dz->bptrcnt = 1; 	is_spec = TRUE;		break;
	case(ARPE):       		dz->extra_bufcnt =  1; dz->bptrcnt = 1; 	is_spec = TRUE;		break;
	case(PLUCK):      		dz->extra_bufcnt =  0; dz->bptrcnt = 1; 	is_spec = TRUE;		break;
	case(S_TRACE):    		dz->extra_bufcnt =  0; dz->bptrcnt = 1; 	is_spec = TRUE;		break;
	case(BLTR):       		dz->extra_bufcnt =  0; dz->bptrcnt = 4; 	is_spec = TRUE;		break;
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
	case(FILT):   	dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
//TW NEW CASE
	case(VFILT):  	dz->array_cnt = 1; dz->iarray_cnt = 1; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;

	case(GREQ):   	dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(SPLIT):  	dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(ARPE):   	dz->array_cnt =1;  dz->iarray_cnt =1;  dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(PLUCK):  	dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt =1;  dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(S_TRACE):  dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
	case(BLTR):   	dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;	break;
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
	case(FILT):			setup_process_logic(ANALFILE_ONLY,		  	EQUAL_ANALFILE,		ANALFILE_OUT,	dz);	break;
//TW NEW CASE
	case(VFILT):		setup_process_logic(ANALFILE_ONLY,		  	EQUAL_ANALFILE,		ANALFILE_OUT,	dz);	break;

	case(GREQ):			setup_process_logic(ANALFILE_ONLY,		  	EQUAL_ANALFILE,		ANALFILE_OUT,	dz);	break;
	case(SPLIT):		setup_process_logic(ANALFILE_ONLY,		  	EQUAL_ANALFILE,		ANALFILE_OUT,	dz);	break;
	case(ARPE):			setup_process_logic(ANALFILE_ONLY,		  	EQUAL_ANALFILE,		ANALFILE_OUT,	dz);	break;
	case(PLUCK):		setup_process_logic(ANALFILE_ONLY,		  	EQUAL_ANALFILE,		ANALFILE_OUT,	dz);	break;
	case(S_TRACE):		setup_process_logic(ANALFILE_ONLY,		  	EQUAL_ANALFILE,		ANALFILE_OUT,	dz);	break;
	case(BLTR):			setup_process_logic(ANALFILE_ONLY,		  	EQUAL_ANALFILE,		ANALFILE_OUT,	dz);	break;
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
	case(FILT):	 	  	return(FINISHED);
//TW NEW CASE
	case(VFILT):		return(FINISHED);

	case(GREQ):	      	return(FINISHED);
	case(SPLIT):	  	return(FINISHED);
	case(ARPE):
		switch(mode) {
		case(ON): case(BOOST): case(ABOVE_BOOST): case(BELOW_BOOST):
					  	exit_status = set_internalparam_data(  "ddid",ap);				break;
		default:	  	exit_status = set_internalparam_data("00ddid",ap);				break;
		}
		break;
	case(PLUCK): 	  	exit_status = set_internalparam_data("ii",ap);					break;
	case(S_TRACE):	  	return(FINISHED);
	case(BLTR):		  	exit_status = set_internalparam_data("ii",ap);					break;
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
	int exit_status = FINISHED;
	aplptr ap = dz->application;

	switch(ap->special_data) {
	case(FILTER_FRQS):
	case(FILTER_BWS_AND_FRQS):	return generate_filter_data(str,(int)ap->special_data,dz);
	case(SPECSPLI_DATA):	  	return read_specsplit_data(str,dz);
//TW NEW CASE
	case(PITCH_VOWELS):
		if((exit_status = get_the_vowels(str,&dz->parray[0],&dz->iparray[0],&dz->itemcnt,dz)) < 0)
			return(exit_status);
		break;

	default:
		sprintf(errstr,"Unknown special_data type: read_special_data()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);	/* NOTREACHED */
}

/************************************* GENERATE_FILTER_DATA	**************************************/

int generate_filter_data(char *str,int datatype,dataptr dz)
{
	int exit_status;
	double *filtwidth;
	if((exit_status = get_filt_params(str,datatype,&filtwidth,dz))<0)
		return(exit_status);
	if((exit_status = sort_peaks_by_frq(filtwidth,dz))<0)
		return(exit_status);
	if((exit_status = setup_filter_param_arrays(dz))<0)
		return(exit_status);
	if((exit_status = generate_filter_params(filtwidth,dz))<0)
		return(exit_status);
 	free(filtwidth);
	return(FINISHED);
}

/********************* GET_FILT_PARAMS ************************/

int get_filt_params(char *str,int datatype,double **filtwidth,dataptr dz)
{
	int exit_status;
	char  *p, temp[200];
	int    gotpair = TRUE, firstime = TRUE;
	int   arraysize = BIGARRAY;
	double val, bwidth = 0.0, fpeak = 0.0;
	aplptr ap = dz->application;
	FILE *fp;
	if((fp = fopen(str,"r"))==NULL) {
		sprintf(errstr,"Cannot open datafile '%s'.\n",str);
		return(DATA_ERROR);
	}																									 	
	if((dz->filtpeak    = (double *)malloc(arraysize * sizeof(double)))==NULL){	/* stores amplitude of filter peaks */ 
		sprintf(errstr,"INSUFFICIENT MEMORY to store filter peaks.\n");
		return(MEMORY_ERROR);
	}
	if((*filtwidth   = (double *)malloc(arraysize * sizeof(double)))==NULL)	{	/* stores bndwidths of filter peaks */ 
		sprintf(errstr,"INSUFFICIENT MEMORY to store filter peakwidths.\n");
		return(MEMORY_ERROR);
	}
	while(fgets(temp,200,fp)) {
		p = temp;
		while(get_float_from_within_string(&p,&val)) {
			switch(datatype) {
			case(FILTER_FRQS):
				if(firstime) {
					if((exit_status = check_filtbw(val,ap->min_special,ap->max_special))<0)
						return(exit_status);
					bwidth = val;
					gotpair  = FALSE;
					firstime = FALSE;
				} else {
					if((exit_status = check_filtfrq(val,ap->min_special2,ap->max_special2))<0)
						return(exit_status);
					fpeak = val;
					gotpair = TRUE;
				}
				break;
			case(FILTER_BWS_AND_FRQS):
				if(gotpair) {
					if((exit_status = check_filtfrq(val,ap->min_special,ap->max_special))<0)
						return(exit_status);
					fpeak  = val;
				} else {
					if((exit_status = check_filtbw(val,ap->min_special2,ap->max_special2))<0)
						return(exit_status);
					bwidth = val;
				}
				gotpair = !gotpair;
				break;
			default:
				sprintf(errstr,"Programming error in get_filt_params()\n");
				return(PROGRAM_ERROR);
			}
			if(gotpair) {
				if(++dz->itemcnt >= arraysize) {
					arraysize += BIGARRAY;
					if((dz->filtpeak = (double *)realloc((char *)dz->filtpeak, arraysize * sizeof(double)))==NULL) {
						sprintf(errstr,"INSUFFICIENT MEMORY to reallocate filter peaks store.\n");
						return(MEMORY_ERROR);
					}
					if((*filtwidth   = (double *)realloc((char *)(*filtwidth),arraysize * sizeof(double)))==NULL) {
						sprintf(errstr,"INSUFFICIENT MEMORY to reallocate filter width store.\n");
						return(MEMORY_ERROR);
					}
				}
				(*filtwidth)[dz->itemcnt-1] = bwidth;
				dz->filtpeak[dz->itemcnt-1]  = fpeak;	
			}
		}
	}
	if(!gotpair) {
		sprintf(errstr,"Filter data not correctly paired in file '%s'.\n",str);
		return(DATA_ERROR);
	}
	if(dz->itemcnt==0) {
		sprintf(errstr,"No filter data found in file '%s'.\n",str);
		return(DATA_ERROR);
	}
	if(fclose(fp)<0) {
		fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",str);
		fflush(stdout);
	}
	return(FINISHED);
}

/************************* CHECK_FILTFRQ *****************************/

int check_filtfrq(double val, double minval, double maxval)
{
	if(val < minval || val > maxval) {
		sprintf(errstr,"Frequency value (%lf) out of range (%lf to %.1lf) in filter data file\n",val,minval,maxval);
		return(DATA_ERROR);
	}
	return(FINISHED);
}

/************************* CHECK_FILTBW *****************************/

int check_filtbw(double val, double minval, double maxval)
{
	if(val < minval || val > maxval) {
		sprintf(errstr,"Bandwidth value (%lf) out of range (%lf to %.1lf) in filter data file\n",val,minval,maxval);
		return(DATA_ERROR);
	}
	return(FINISHED);
}

/****************************** SORT_PEAKS_BY_FRQ ***************************/

int sort_peaks_by_frq(double *filtwidth,dataptr dz)		 /* Sort peaks in ascending frequency order */
{	
	int m, n;
	double k;
	for(n=0;n<dz->itemcnt-1;n++) {
		for(m=n+1;m<dz->itemcnt;m++) {
			if(dz->filtpeak[n] > dz->filtpeak[m]) {
				k         	    = dz->filtpeak[m];
				dz->filtpeak[m] = dz->filtpeak[n];
				filtwidth[n]    = k;
				k         		= filtwidth[m];
				filtwidth[m]    = filtwidth[n];
				filtwidth[n]    = k;
			}
		}
	}
	return(FINISHED);
}

/****************************** SETUP_FILTER_PARAM_ARRAYS ***************************/
																					
int setup_filter_param_arrays(dataptr dz)											
{																					
	if((dz->fbandtop   = (double *)malloc((dz->itemcnt+1) * sizeof(double)))==NULL)	 { /* stores top of fiter bands */
		sprintf(errstr,"INSUFFICIENT MEMORY to store filterband tops.\n");
		return(MEMORY_ERROR);													
	}
	if((dz->fbandbot   = (double *)malloc((dz->itemcnt+1) * sizeof(double)))==NULL)	 { /* stores bottoms of filtbands */
		sprintf(errstr,"INSUFFICIENT MEMORY to store filterband bottoms.\n");
		return(MEMORY_ERROR);													
	}
	if((dz->fsampbuf   = (float  *)malloc((dz->clength+1) * sizeof(float) ))==NULL)	 { /* stores filter contour */
		sprintf(errstr,"INSUFFICIENT MEMORY for filter buffer.\n");
		return(MEMORY_ERROR);
	}
	dz->filtpeak[dz->itemcnt] = 0.0;
    dz->fbandtop[dz->itemcnt] = 0.0;		/* stores top of fiter bands */
    dz->fbandbot[dz->itemcnt] = 0.0;		/* stores bottoms of filtbands */
	dz->fsampbuf[dz->clength] = 0.0f;		/* stores filter contour */
	memset(dz->fsampbuf,0,dz->clength * sizeof(float));	/* initialise filter buf */
	return(FINISHED);
}


/****************************** GENERATE_FILTER_PARAMS ***************************/

int generate_filter_params(double *filtwidth,dataptr dz)
{
	int n;
	double bratio_up, bratio_dn, halfbwidth;
	for(n=0;n<dz->itemcnt;n++) {		  /* ESTABLISH BOTTOM AND TOP OF FILTER BANDS */
		halfbwidth = filtwidth[n]/2.0;
		dz->scalefact = PI/halfbwidth;	 /* Convert an octave offset from filter centre-frq
									    to a fraction of the bandwidth X PI */
		bratio_up = pow(2.0,halfbwidth); /* upward transposition ratio to top of filter band  */
		bratio_dn = 1.0/bratio_up;	 	 /* Downward transposition ratio to bottom of filter band */
		dz->fbandbot[n] = dz->filtpeak[n] * bratio_dn;
		dz->fbandtop[n] = min(dz->filtpeak[n] * bratio_up,dz->nyquist);
	}
	return(FINISHED);
}

/******************************* READ_SPECSPLIT_DATA ***************************
 *
 * Read input data from data file and store in structures.
 */

int read_specsplit_data(char *str,dataptr dz)
{
	char temp[200], *p, *q;
	double otheramp, d1;
	int mask, n, flags_set = 0, linecnt = 0;
	int arraysize = BIGARRAY;
	bandptr bb;
	FILE *fp;
	if((fp = fopen(str,"r"))==NULL) {
  		sprintf(errstr,"Cannot open data file %s\n", str);
		return(DATA_ERROR);
	}
	if((dz->band = (bandptr *)malloc(arraysize * sizeof(bandptr)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store bands.\n");
		return(MEMORY_ERROR);
	}
	while(fgets(temp,200,fp)!=NULL) {
		if((dz->band[dz->itemcnt]=(bandptr)malloc(sizeof(struct bandd)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to store band %d\n",dz->itemcnt+1);
			return(MEMORY_ERROR);
		}
		bb = dz->band[dz->itemcnt];
		p = temp;
		while(isspace(*p))
			p++;
		if(*p==NEWLINE || *p==ENDOFSTR)
			continue;
		linecnt++;
		if(!get_float_from_within_string(&p,&(bb->bfrqlo))) {
			sprintf(errstr,"Cannot read lower frequency in line %d in datafile\n",linecnt);
			return(DATA_ERROR);
		}
		if(bb->bfrqlo<0.0 || bb->bfrqlo>dz->nyquist) {
			sprintf(errstr,"freq %.2lf out of range (>0 to %lf[nyquist]) in line %d in datafile\n",
			bb->bfrqlo,dz->nyquist,linecnt);
			return(DATA_ERROR);
		}
		if(!get_float_from_within_string(&p,&(bb->bfrqhi))) {
			sprintf(errstr,"Cannot read higher frequency in line %d in datafile\n",linecnt);
			return(DATA_ERROR);
		}
		if(bb->bfrqhi<0.0) {
			sprintf(errstr,"freq %.2lf out of range (>0) in line %d in datafile\n",bb->bfrqhi,linecnt);
			return(DATA_ERROR);
		}
		if(bb->bfrqhi>dz->nyquist)
			bb->bfrqhi = dz->nyquist;
		if(flteq(bb->bfrqlo,bb->bfrqhi)) {
			sprintf(errstr,"Cannot proceed, band range zero in line %d in datafile\n",linecnt);
			return(DATA_ERROR);
		}
		if((bb->bfrqdif=bb->bfrqhi - bb->bfrqlo)<0.0){
			d1        = bb->bfrqlo;
			bb->bfrqlo = bb->bfrqhi;
			bb->bfrqhi = d1;
			bb->bfrqdif = -(bb->bfrqdif);
		}
		if(get_word_from_string(&p,&q)==FALSE) {
			sprintf(errstr,"Cannot read action flag in line %d in datafile\n",linecnt);
			return(DATA_ERROR);
		}
		mask  = 1;
		bb->bdoflag   = 0;
		bb->badditive = 0;
		for(n=0;n<4;n++) {
			if(q[n] <'0' || q[n] > '1') {
				sprintf(errstr,"Invalid flag in line %d in datafile\n",linecnt);
				return(DATA_ERROR);
			}
			if(q[n]=='1') {
				flags_set++;
				bb->bdoflag |= mask;
				switch(n) {
				case(0):
					if(!get_float_from_within_string(&p,&(bb->bamp))) {
						sprintf(errstr,"Cannot read amplitude 1 in in line %d in datafile\n",linecnt);
						return(DATA_ERROR);
					}
					break;
				case(1):
					if(!(bb->bdoflag & DO_AMPLITUDE_CHANGE)) {
						sprintf(errstr,"Cannot have amp2 when no amp1 in line %d in datafile\n",linecnt);
						return(DATA_ERROR);
					}
					if(!get_float_from_within_string(&p,&otheramp)) {		 
						sprintf(errstr,"Cannot read amplitude 1 in line %d in datafile\n",linecnt);
						return(DATA_ERROR);
					}
// FEB 2006
//					if(otheramp<bb->bamp)
//						swap(&(bb->bamp),&otheramp);
					bb->bampdif = otheramp - bb->bamp;
					break;
				case(2):
					while(isspace(*p))
						p++;
					if(*p=='+') {
						bb->badditive = 1;
						p++;
					}
 					if(!get_float_from_within_string(&p,&(bb->btrans))) {
						sprintf(errstr,"Cannot read transposition in line %d in datafile\n",linecnt);
						return(DATA_ERROR);
					}
					break;
				case(3):
					if(!(bb->bdoflag & DO_TRANSPOSITION)) {
						sprintf(errstr,"Cannot add_in partials without first transposing: line %d in datafile\n",linecnt);
						return(DATA_ERROR);
					}
					break;
				}
			}
			mask <<= 1;
		}
		if(flags_set==0) {
			sprintf(errstr,"Zero bitflag in line %d in datafile\n",linecnt);
			return(DATA_ERROR);
		}
		if(++dz->itemcnt >= arraysize) {
			arraysize += BIGARRAY;
			if((dz->band=(bandptr *)realloc((char *)dz->band,arraysize*sizeof(bandptr)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY to reallocate band stores.\n");
				return(MEMORY_ERROR);
			}
		}
	}
	if((dz->band=(bandptr *)realloc((char *)dz->band,(dz->itemcnt+1)*sizeof(bandptr)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to reallocate band stores.\n");
		return(MEMORY_ERROR);
	}
	return sort_frqbands_into_ascending_order(dz);
}

/************************ SORT_FRQBANDS_INTO_ASCENDING_ORDER ********************************
 *
 * Sort bands into frequency ascending order.
 */

int sort_frqbands_into_ascending_order(dataptr dz)
{
	double f1, f2;
	int penult = dz->itemcnt-1, n, m;
	for(n=0;n<penult;n++) {
		f1 = dz->band[n]->bfrqlo;
		for(m=n+1;m<dz->itemcnt;m++) {
			if((f2 = dz->band[m]->bfrqlo)<f1)
				swap_frqbands(n,m,dz);
		}
	}
	for(n=0;n<penult;n++) {
		if(dz->band[n]->bfrqhi > dz->band[n+1]->bfrqlo) {
			sprintf(errstr,"Frequency bands %d and %d in data file overlap\n%d  %lf..%lf\n%d  %lf..%lf\n",
			n+1,n+2,n+1,dz->band[n]->bfrqlo,dz->band[n]->bfrqhi,n+2,dz->band[n+1]->bfrqlo,dz->band[n+1]->bfrqhi);
			return(DATA_ERROR);
		}
	}
	return(FINISHED);
}

/************************ SWAP_FRQBANDS ********************************
 *
 * Swap two bands, using spare band 'banscnt' as temporary store.
 */

void swap_frqbands(int n,int m,dataptr dz)
{   bandptr temp;

    temp        = dz->band[n];
    dz->band[n] = dz->band[m];
    dz->band[m] = temp;
}

/********************************************************************************************/
/********************************** FORMERLY IN preprocess.c ********************************/
/********************************************************************************************/

/****************************** PARAM_PREPROCESS *********************************/

int param_preprocess(dataptr dz)	
{
	switch(dz->process) {
	case(ARPE):			return arpe_preprocess(dz);
	case(FILT):			return do_log_conversions_for_specfilt(dz);
	case(PLUCK):		return setup_internal_bitflags(PLUK_BFLG,PLUK_LONGPOW2,PLUK_DIVMASK,dz);
	case(S_TRACE):		return setup_ring(dz);

 	case(GREQ):	
	case(SPLIT):	
	case(BLTR):			
//TW NEW CASE
	case(VFILT):

		return(FINISHED);
	default:
		sprintf(errstr,"PROGRAMMING PROBLEM: Unknown process in param_preprocess()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);	/* NOTREACHED */
}

/************************** ARPE_PREPROCESS ******************************/

int arpe_preprocess(dataptr dz)
{
	int exit_status;
	if((exit_status = gen_arpe_waveform_table(dz))<0)
		return(exit_status);
	if((exit_status = setup_internal_arpe_params_and_preprocess_params(dz))<0)
		return(exit_status);
	dz->iparam[ARPE_SUSFLAG] = set_arpegtones_sustain_type(dz);
	return(FINISHED);
}

/****************************** SET_ARPEGTONES_SUSTAIN_TYPE ***********************
 *
 * this functions establishes the following flag modes for susflag
 * AP_NORMAL										(0)
 * AP_SUSTAIN_PITCH									(1)
 * AP_LIMIT_SUSTAIN									(2)
 * AP_SUSTAIN_PITCH_AND_LIMIT_SUSTAIN				(3)
 * AP_NONLIN										(4)
 * AP_NONLIN_AND_SUSTAIN_PITCH						(5)
 * AP_NONLIN_AND_LIMIT_SUSTAIN						(6)
 * AP_NONLIN_AND_SUSTAIN_PITCH_AND_LIMIT_SUSTAIN	(7)
*/
int set_arpegtones_sustain_type(dataptr dz)
{
	int susflag = 0;
	if(dz->mode==ON || dz->mode==BOOST || dz->mode==ABOVE_BOOST || dz->mode==BELOW_BOOST) {
		if(!dz->vflag[ARPE_TRKFRQ])	  	susflag |= 1;	/* Forces pitch-sustain */
		if(!dz->vflag[ARPE_KEEP_SUST]) 	susflag |= 2;	/* Limits length of sustains */
		if(dz->vflag[ARPE_IS_NONLIN])   susflag |= 4;	/* forces nonlinear mode */
	}
	return(susflag);
}

/**************************** GEN_ARPE_WAVEFORM_TABLE *******************************/

int gen_arpe_waveform_table(dataptr dz)
{
	int n, m, halftab = ARPE_TABSIZE/2;
	double anglestep = (PI * 2.0)/ARPE_TABSIZE;
	if((dz->parray[ARPE_TAB] = (double *)malloc(ARPE_TABSIZE * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for arpeg table.\n");
		return(MEMORY_ERROR);
	}
	switch(dz->iparam[ARPE_WTYPE]) {
	case(SIN):
		for(n=0;n<ARPE_TABSIZE;n++) {
			dz->parray[ARPE_TAB][n]  = sin((PI * 1.5) + ((double)n*anglestep));
			dz->parray[ARPE_TAB][n] += 1.0;
			dz->parray[ARPE_TAB][n] /= 2.0;
		}
		break;
	case(SAW):
		for(n=0;n<halftab;n++)
			dz->parray[ARPE_TAB][n]  = (double)n/(double)halftab;
		m = halftab;
		for(n=0;n<halftab;n++)
			dz->parray[ARPE_TAB][m++]  = 1.0 - dz->parray[ARPE_TAB][n];
		break;
	case(DOWNRAMP):
		for(n=0;n<ARPE_TABSIZE;n++)
			dz->parray[ARPE_TAB][n]  = 1.0 - (double)n/(double)ARPE_TABSIZE;
		break;
	case(UPRAMP):
		for(n=0;n<ARPE_TABSIZE;n++)
			dz->parray[ARPE_TAB][n]  = (double)n/(double)ARPE_TABSIZE;
		break;
	default:
		sprintf(errstr,"Programing Problem in gen_arpe_waveform_table()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/********************** SETUP_INTERNAL_ARPE_PARAMS_AND_PREPROCESS_PARAMS *********/

int setup_internal_arpe_params_and_preprocess_params(dataptr dz)
{
	int exit_status;
   	dz->param[ARPE_WAVETABPOS] = dz->param[ARPE_PHASE] * (double)ARPE_TABSIZE;
	if((dz->iparray[ARPE_KEEP] = (int *)malloc(dz->clength * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for arpe keep array.\n");
		return(MEMORY_ERROR);
	}
	if(dz->brksize[ARPE_LOFRQ]==0 && dz->brksize[ARPE_HIFRQ]==0) {
		if((exit_status = fix_inverted_frqrange_in_arpe(dz))<0)
			return(exit_status);
	}
	if((exit_status = establish_arpe_halfband_param(dz))<0)
		return(exit_status);
	if(dz->brksize[ARPE_AMPL]==0 && dz->brksize[ARPE_SUST]==0)
		dz->param[ARPE_AMPL]  /= (double)dz->iparam[ARPE_SUST];
    if(dz->brksize[ARPE_ARPFRQ]) {
    	if((exit_status = read_value_from_brktable(0.0,ARPE_ARPFRQ,dz))<0)
			return(exit_status);
		dz->param[ARPE_LASTARPFRQ] = dz->param[ARPE_ARPFRQ];
	}
	return(FINISHED);
}

/****************************** FIX_INVERTED_FRQRANGE_IN_ARPE ********************/

int fix_inverted_frqrange_in_arpe(dataptr dz) 
{			   /* cure inverted freq range */
	double dummy;
	if(dz->param[ARPE_LOFRQ] >  dz->param[ARPE_HIFRQ]) {
		dummy = dz->param[ARPE_LOFRQ];
		dz->param[ARPE_LOFRQ] =	dz->param[ARPE_HIFRQ];
		dz->param[ARPE_HIFRQ] = dummy;
	}
	return(FINISHED);
}

/********************** ESTABLISH_ARPE_HALFBAND_PARAM **********************/

int establish_arpe_halfband_param(dataptr dz)
{
	double *p;
	int n;
	if(dz->brksize[ARPE_HBAND]==0)
		dz->param[ARPE_HBAND] /=  2.0;
	else {
		p = dz->brk[ARPE_HBAND] + 1;
		for(n=0;n<dz->brksize[ARPE_HBAND];n++) {
			*p /=  2.0;
			p  += 2;
		}
	} 
	return(FINISHED);
}

/************ DO_LOG_CONVERSIONS_FOR_SPECFILT *************/

int do_log_conversions_for_specfilt(dataptr dz)	   	/* convert brktables to log vals, for log-interp */
{			
	int exit_status;
	if((exit_status = convert_brkpnt_table_vals_to_log(FILT_QQ,dz))<0)
		return(exit_status);
	if((exit_status = convert_brkpnt_table_vals_to_log(FILT_FRQ1,dz))<0)
		return(exit_status);
	return convert_brkpnt_table_vals_to_log(FILT_FRQ2,dz);
}

/******************************* CONVERT_BRKPNT_TABLE_VALS_TO_LOG ****************/

int convert_brkpnt_table_vals_to_log(int brktableno,dataptr dz)
{
	int n;
	double *p;
	if(dz->brksize[brktableno]) { 	/* convert vals in brkfile to log, so interp in brkfile is log */
		p = dz->brk[brktableno]+1;	
		for(n=0;n<dz->brksize[brktableno];n++) {	/* (convert back to exp on special_read) */
			if(*p <=0.0) {
				sprintf(errstr,"Error in freq or Q table: 0.0 val: convert_brkpnt_table_vals_to_log()\n");
				return(PROGRAM_ERROR);
			}
			*p = log(*p);
			p += 2;
		}
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
	case(FILT):		return outer_loop(dz);
//TW NEW CASE
	case(VFILT):	return vowel_filter(dz);

	case(GREQ):		return outer_loop(dz);
	case(SPLIT):	return outer_loop(dz);
	case(ARPE):		return outer_loop(dz);
	case(PLUCK):	return outer_loop(dz);
	case(S_TRACE):	return outer_loop(dz);
	case(BLTR):		return spec_blur_and_bltr(dz);
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
		case(FILT):		exit_status = specfilt(dz);	  						break;
		case(GREQ):		exit_status = specgreq(dz);	  						break;
		case(SPLIT): 	exit_status = specsplit(dz);  						break;
		case(ARPE):		exit_status = specarpe(in_start_portion,dz);		break;
		case(PLUCK):	exit_status = specpluck(dz);  						break;
		case(S_TRACE):	exit_status = spectrace(dz);  						break;
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
//	int exit_status = FINISHED;
	switch(dz->process) {
	case(GREQ):   case(PLUCK): 
		return(TRUE);
	}
	return(FALSE);
}

/*********************************** SPEC_BLUR_AND_BLTR ***********************************/

int spec_blur_and_bltr(dataptr dz)
{
	int exit_status;
	int 	blurcnt = 0, blurfactor = dz->iparam[BLUR_BLURF], w_to_buf, wc;
  	int	last_total_windows = 0;
	float 	*ampdif, *freqdif;
	dz->time = 0.0f;
	if((exit_status = float_array(&ampdif,dz->clength))<0)
		return(exit_status);
	if((exit_status = float_array(&freqdif,dz->clength))<0)
		return(exit_status);
	if(dz->process==BLTR
	&& (exit_status = create_and_initialise_array_of_loudest_channel_pointers(dz->iparam[BLTR_MAXTRACE],dz))<0)
		return(exit_status);
	dz->flbufptr[1] = dz->flbufptr[2];
	if((exit_status = read_samps(dz->bigfbuf, dz)) < 0)
		return exit_status;
	while(dz->ssampsread > 0) {
    	dz->flbufptr[0] = dz->bigfbuf;
    	w_to_buf       = dz->ssampsread/dz->wanted;  
    	for(wc=0; wc<w_to_buf; wc++) {
			if(blurcnt==0) {
				if(dz->total_windows > 0) {
 					if((exit_status = do_the_bltr(&last_total_windows,ampdif,freqdif,blurfactor,dz))<0)
						return(exit_status);
				}
 				if(dz->brksize[BLUR_BLURF]) {
 					if((exit_status = read_value_from_brktable((double)dz->time,BLUR_BLURF,dz))<0)
						return(exit_status);
				}
				blurfactor = dz->iparam[BLUR_BLURF];
				if(dz->total_windows + blurfactor >= dz->wlength)	  /* number of windows to skip */
					blurfactor = dz->wlength - dz->total_windows - 1; /* must not exceed total no of windows */

				blurcnt = blurfactor;

					/* SEPARATE THE AMP AND FREQ DATA IN (NEXT) INITIAL WINDOW */
				if((exit_status = get_amp_and_frq(dz->flbufptr[0],dz))<0)
					return(exit_status);
			}
			blurcnt--;
			dz->flbufptr[0] += dz->wanted;
			dz->total_windows++;
			dz->time = (float)(dz->time + dz->frametime);
		}
		if((exit_status = read_samps(dz->bigfbuf, dz)) < 0)
			return exit_status;
	}
	if((exit_status = put_amp_and_frq(dz->flbufptr[1],dz))<0) /* transfer last window to output buffer */
		return(exit_status);		
	dz->flbufptr[1] += dz->wanted;
	return write_exact_samps(dz->flbufptr[2],(dz->flbufptr[1] - dz->flbufptr[2]),dz);
}

/****************************** DO_THE_BLTR ***************************/

int do_the_bltr(int *last_total_windows,float *ampdif,float *freqdif,int blurfactor,dataptr dz)
{
	int exit_status = FINISHED;
	int thiswindow, j;
	int cc, vc;
	double thistime = *last_total_windows * dz->frametime;

	for( cc = 0 ,vc = 0; cc < dz->clength; cc++, vc += 2){
		ampdif[cc]   = dz->flbufptr[0][AMPP] - dz->amp[cc];
		ampdif[cc]  /= (float)blurfactor;
		freqdif[cc]  = dz->flbufptr[0][FREQ] - dz->freq[cc];
		freqdif[cc] /= (float)blurfactor;
	}
	for(thiswindow = *last_total_windows,j=0;thiswindow<dz->total_windows;thiswindow++,j++) {
		if(dz->process == BLTR) {
			if((exit_status = prepare_to_do_trace(&thistime,dz))<0)
			return(exit_status);
		}
		for(cc = 0, vc = 0; cc < dz->clength; cc++, vc += 2){  /* BLUR amp AND freq */
			dz->flbufptr[1][AMPP] = dz->amp[cc]  + ((float)j * ampdif[cc]);
			dz->flbufptr[1][FREQ] = dz->freq[cc] + ((float)j * freqdif[cc]);
			if(dz->process == BLTR) {	/* COMPARE amp WITH LOUDEST VALS, AND STORE IF NESS */
				if((exit_status = if_loud_enough_store_in_ring(cc,vc,dz))<0)
					return(exit_status);	
			}
		}
		if(dz->process == BLTR) {
			if((exit_status = create_trace(dz))<0)
				return(exit_status);
		}
		dz->flbufptr[1] += dz->wanted;
		if(dz->flbufptr[1] >= dz->flbufptr[3]) {
			if((exit_status = write_exact_samps(dz->flbufptr[2],dz->buflen,dz))<0)
				return(exit_status);
			dz->flbufptr[1] = dz->flbufptr[2];
		}
	}

	*last_total_windows = dz->total_windows;
	return(FINISHED);
}

/****************************** CONSTRUCT_FILTER_ENVELOPE (HILITE) ***************************
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
		case(GREQ):	 filt_centre_frq = dz->filtpeak[n];					break;
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

/************* CREATE_AND_INITIALISE_ARRAY_OF_LOUDEST_CHANNEL_POINTERS ************
 *
 * Create &initialise an array of k loudest-channel-pointers.
 */

int create_and_initialise_array_of_loudest_channel_pointers(int arraysize,dataptr dz)
{
	int n;
    if((dz->loudest = (chsptr)malloc(arraysize * (sizeof(struct chanstore))))==NULL) {
		sprintf(errstr,"create_and_initialise_array_of_loudest_channel_pointers()\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<arraysize;n++) {
		(dz->loudest+n)->chan = 0;
		(dz->loudest+n)->amp  = 0.0F;
	}
	return(FINISHED);
}

/****************************** PREPARE_TO_DO_TRACE ***************************/

int prepare_to_do_trace(double *thistime,dataptr dz)
{
	int exit_status;
	int n;
	if(dz->brksize[BLTR_TRACE]) {
		if((exit_status = read_value_from_brktable(*thistime,BLTR_TRACE,dz))<0)
			return(exit_status);

		*thistime += dz->frametime;
	}
	for(n=0;n<dz->iparam[BLTR_TRACE];n++)
		(dz->loudest+n)->amp = 0.0F;	 /* ZERO LOUDEST VALS */
	return(FINISHED);
}

/****************************** IF_LOUD_ENOUGH_STORE_IN_RING ***************************/

int if_loud_enough_store_in_ring(int cc,int vc,dataptr dz)
{
	int exit_status;
	int n;
	for(n=0;n<dz->iparam[BLTR_TRACE];n++) {
		if(dz->flbufptr[1][AMPP] > (dz->loudest+n)->amp) {
			if((exit_status = make_room_in_loudchans_list_by_moving_later_items_down_a_place(dz->iparam[BLTR_TRACE],n,dz))<0)
				return(exit_status);
			(dz->loudest+n)->chan = cc;
			(dz->loudest+n)->amp  = dz->flbufptr[1][AMPP];
			break;
		}
	}	/* RE-INITIALISE CHANNEL AMP TO ZERO */
	dz->flbufptr[1][AMPP]  = 0.0F;   
	return(FINISHED);
}

/******************************** CREATE_TRACE *****************************
 *
 * Insert true amplitude in loudest channels only.
 *
 * NB array positions of amp-vals in output_buf are *2 true channel position
 */

int	create_trace(dataptr dz)
{
	int n;				
	for(n=0;n<dz->iparam[BLTR_TRACE];n++)
		dz->flbufptr[1][((dz->loudest+n)->chan)*2] = (dz->loudest+n)->amp;
	return(FINISHED);
}

/************* MAKE_ROOM_IN_LOUDCHANS_LIST_BY_MOVING_LATER_ITEMS_DOWN_A_PLACE *************
 *
 * Shuffle the items in the list of loudest channels, down from k towards
 * end.
 */

int make_room_in_loudchans_list_by_moving_later_items_down_a_place(int size_of_list,int index_into_list,dataptr dz)
{
	int n;
	if(index_into_list==(size_of_list-1))
		return(FINISHED);
	for(n=(size_of_list-1);n>index_into_list;n--) {
		(dz->loudest+n)->chan = (dz->loudest+n-1)->chan;
		(dz->loudest+n)->amp  = (dz->loudest+n-1)->amp;
	}
	return(FINISHED);
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
	case(ARPE):		   return check_arpe_parameters_are_compatible(dz);
	case(BLTR):	   	   return check_validity_of_trace_vals_for_bltr(dz);
	}
	return(FINISHED);
}

/*********************** CHECK_ARPE_PARAMETERS_ARE_COMPATIBLE ***********************/

int check_arpe_parameters_are_compatible(dataptr dz)
{
	if(dz->mode==ABOVE_BOOST || dz->mode==ONCE_ABOVE) {
		if((dz->iparam[ARPE_WTYPE]==SIN 
			|| dz->iparam[ARPE_WTYPE]==SAW) 
			&& dz->param[ARPE_PHASE] < 0.5) {
			sprintf(errstr,
				"startphase of SIN or SAW must be > 0.5 with ABOVE_BOOST & ONCE_ABOVE\n");
			return(USER_ERROR);
		}
	}
	return(FINISHED);
}

/********************* CHECK_VALIDITY_OF_TRACE_VALS_FOR_BLTR **********************/

int check_validity_of_trace_vals_for_bltr(dataptr dz)
{
	int exit_status;
	double brkmax;
	if(dz->brksize[BLTR_TRACE]==0)
		dz->iparam[BLTR_MAXTRACE] = dz->iparam[BLTR_TRACE];
	else { 
		if((exit_status = get_maxvalue_in_brktable(&brkmax,BLTR_TRACE,dz))<0)
			return(exit_status);
		dz->iparam[BLTR_MAXTRACE] = round(brkmax);
	}
	if(dz->iparam[BLTR_MAXTRACE]<=0) {
		sprintf(errstr,"Failed to find trace value above zero,\n");
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
	case(FILT):		case(GREQ):		case(SPLIT):	
	case(ARPE):		case(PLUCK):	case(S_TRACE):	
		return allocate_single_buffer(dz);
//TW NEW CASE
 	case(VFILT):
		return allocate_single_buffer_plus_extra_pointer(dz);

	case(BLTR):		
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
	if     (!strcmp(prog_identifier_from_cmdline,"filter"))	   		dz->process = FILT;
//TW NEW CASE
	else if(!strcmp(prog_identifier_from_cmdline,"vowels"))	   		dz->process = VFILT;

	else if(!strcmp(prog_identifier_from_cmdline,"greq"))	   		dz->process = GREQ;
	else if(!strcmp(prog_identifier_from_cmdline,"band"))	   		dz->process = SPLIT;
	else if(!strcmp(prog_identifier_from_cmdline,"arpeg"))	   		dz->process = ARPE;
	else if(!strcmp(prog_identifier_from_cmdline,"pluck"))	   		dz->process = PLUCK;
	else if(!strcmp(prog_identifier_from_cmdline,"trace"))	   		dz->process = S_TRACE;
	else if(!strcmp(prog_identifier_from_cmdline,"bltr"))	   		dz->process = BLTR;
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
	"\nHIGHLIGHTING OPERATIONS ON A SPECTRAL FILE\n\n"
	"USAGE: hilite NAME (mode) infile outfile parameters: \n"
	"\n"
	"where NAME can be any one of\n"
	"\n"
	"filter   greq    band    arpeg    pluck	trace    bltr\n\n"
//TW NEW CASE
	"vowels\n\n"

	"Type 'hilite filter' for more info on hilite filter..ETC.\n");
	return(USAGE_ONLY);
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
	if(!strcmp(str,"filter")) {
		fprintf(stdout,
		"hilite filter 1-4   infile outfile frq1 Q \n"
		"hilite filter 5-6   infile outfile frq1 Q gain\n"
		"hilite filter 7-10  infile outfile frq1 frq2 Q\n"
		"hilite filter 11-12 infile outfile frq1 frq2 Q gain\n"
		"\n"
		"FILTER THE SPECTRUM\n"
		"\n"
		"MODES:-\n"
		"1    high pass filter \n"
		"2    high pass filter (normalised output) \n"
		"3    low pass filter \n"
		"4    low pass filter (normalised output) \n"
		"5    high pass filter with gain \n"
		"6    low pass filter with gain \n"
		"7    band pass filter \n"
		"8    band pass filter (normalised output) \n"
		"9    notch filter \n"
		"10   notch filter (normalised output) \n"
		"11   band pass filter with gain \n"
		"12   notch filter with gain \n"
		"\n"
		"frq1            filter cutoff frq.\n"
		"frq1 with frq2  limits of filter band.\n"
		"Q               width of filter skirts, in Hz (Range: >0).\n"
		"gain            amplification of resulting sound.\n"
		"\n"
		"frq1, frq2 and Q may vary over time.\n");
//TW NEW CASE
	} else if(!strcmp(str,"vowels")) {
		fprintf(stdout,
		"hilite vowels infile outfile vowelfile halfwidth steepness range threshold\n"
		"\n"
		"VOWELS......file containing paired times & vowels, where vowels can be....\n"
		"ee: as in 'heat'  i:  as in 'hit'    e:  as in 'pet'  ai: as in 'hate'\n"
		"a:  as in 'pat'   ar: as in 'heart'  o:  as in 'hot'  or: as in 'taught'\n"
		"oa: as in 'boat'  u:  as in 'hood'   oo: as in 'mood'\n"
		"xx: as in Southern English 'hub'     x:  neutral vowel in 'herb' or 'the'\n"
		"\n"
		"Times must start at zero, and increase.\n"
		"\n"
		"HALFWIDTH ....half-width of formant peaks as fraction of formant centre-frq.\n"
		"              (Range 0.01 - 10)\n"
		"STEEPNESS ....steepness of formant peaks. (Range 0.1 - 10)\n"
		"RANGE ........the peaks stand above the signal floor. The range of the peaks\n"
		"..............is therefore a part of the total range of the signal.\n"
		"..............Range = ratio (max) peakheight to (max) totalrange.(vals 0-1)\n"
		"THRESHOLD ....spectral window's level is compared with vowel-envelope level.\n"
		"..............If it exceeds a certain proportion of that level,\n"
		"..............it is forced to the vowel-envelope level.\n"
		"..............(Otherwise it remains where it is).\n"
		"..............threshold defines this proportion.(Range 0-1)\n"
		"good defaults might be %lf, %d, %lf, 0.5\n",V_HWIDTH,CURVIT,PEAK_RANGE);
	} else if(!strcmp(str,"greq")) {
		fprintf(stdout,
		"hilite greq mode infile outfile filtfile [-r]\n"
		"\n"
		"GRAPHIC EQ ON SPECTRUM\n"
		"\n"
		"MODES :-\n"
		"1   single bandwidth for all filter bands.\n"
		"    FILTFILE has ONE bandwidth (octaves)\n"
		"    followed by centre frqs of all filter bands (Hz).\n\n"
		"2   separate bandwidths for each filter band.\n"
		"    FILTFILE has a pair of values for each filter band.\n"
		"    These are centre frq of the band (Hz) && bandwidth (octaves).\n"
		"\n"
		"-r  Band reject (notch) filter: Default is a bandpass filter.\n");
	} else if(!strcmp(str,"band")) {	/* SPLIT */
		fprintf(stdout,
		"hilite band infile outfile datafile\n"
		"\n"
		"SPLIT SPECTRUM INTO BANDS & PROCESS THESE INDIVIDUALLY\n"
		" \n"
		"Datafile contains a number of lines with the following info..\n"
		"\n"
		"lofrq   hifrq   bitflag   [amp1   amp2   [+]transpose]\n"
		"\n"
	   	"bitflag has 4 bits (e.g. '0101' or '1000')\n"
	   	"bit 1 set: amplitude change to band: put amp multiplier 'amp1' in line.\n"
	   	"bit 2 set: amplitude ramp: put 2nd amp multiplier 'amp2' in line.\n"
	   	"           (spectral band will change in amplitude from amp1 to amp2).\n"
	   	"bit 3 set: partials transpose: prog expects frq multiplier 'transpose' in line.\n"
	   	"           OR, if val preceeded by '+',val is frq in Hz ADDED to frq in band.\n"
	   	"bit 4 set: transposed partials ADDED to original spectrum (default: replace)\n\n"
	    "amp1, amp2 &\\or transpose MUST be present, as required by the bitflag options.\n");
	} else if(!strcmp(str,"arpeg")) {
		fprintf(stdout,
		"hilite arpeg 1-4 infile outfile wave rate [-pU] [-lX] [-hY] [-bZ] [-aA]\n"
		"                                              [-Nk] [-sS] [-T]  [-K]\n\n"
//TW JULY 2006
		"hilite arpeg 5-6 infile outfile wave rate [-pU] [-lX] [-hY] [-aA]\n\n"
		"hilite arpeg 7-8 infile outfile wave rate [-pU] [-lX] [-hY] [-bZ] [-aA]\n\n"
		"MODES:-                ARPEGGIATE THE SPECTRUM\n"
		"1 ON...........Play components inside arpeggiated band ONLY.\n"
		"2 BOOST........Amplify snds in band. Others play unamplified.\n"
		"3 BELOW_BOOST..INITIALLY Play components in & below band ONLY.\n"
		"               THEN amplify snds in band. Others play unamplified.\n"
		"               (NOT with downramp).\n"
		"4 ABOVE_BOOST..INITIALLY Play components in & above band ONLY.\n"
		"               THEN amplify snds in band. Others play unamplified.\n"
		"              (NOT with upramp: with sin/saw startphase>0.5)\n"
		"5 BELOW........Play components in & below arpeggiated band ONLY.\n"
		"6 ABOVE........Play components in & above arpeggiated band ONLY.\n"
		"7 ONCE_BELOW...INITIALLY Play components in and below band ONLY.\n"
		"               THEN play whole sound as normal.(NOT with downramp).\n"
		"8 ONCE_ABOVE...INITIALLY Play components in and above arpeggiated band ONLY.\n"
		"               THEN play whole sound as normal.\n"
		"               (NOT with upramp: with sin/saw startphase>0.5)\n"
		"wave  1 downramp : 2 sin : 3 saw : 4 upramp\n"
		"rate  sweeps per second.\n"
		"-p    U = start_phase: range 0-1: (limited range for some cases).\n"
		"-l    X = lowest  freq arpeg sweeps down to: default 0\n"
		"-h    Y = highest freq arpeg sweeps up to:   default nyquist\n"
		"-b    Z = bandwidth of sweep band (in Hz): default = nyquist/channel_cnt\n"
		"-a    A = amplification of arpegtones : default 10.0\n"
		"-N    k = Nonlinear decay arpegtones. >1 faster, <1 slower. Must be >0.\n"
		"-s    S = No. of windows over which arpegtones sustained: Default 3\n"
		"-T    In sustains, TRACK changing frq of src (default : retain start frq)\n"
		"-K    Let sustains run to zero before new arpegtone attack accepted\n"
		"     (Default:Re-attack once sustains fall below current input level)\n"
		"\n"
		"all parameters may vary over time, except for wavetype and startphase.\n");
	} else if(!strcmp(str,"pluck")) {
		fprintf(stdout,
		"hilite pluck infile outfile gain\n"
		"\n"
		"EMPHASISE SPECTRAL CHANGES (USE e.g. WITH spec arpeg)\n"
		"\n"
		"gain   amplitude gain applied to newly prominent spectral components.\n"
		"\n"
		"gain may vary over time.\n");
	} else if(!strcmp(str,"trace")) {
		fprintf(stdout,
		"hilite trace 1 infile outfile N\n"
		"hilite trace 2 infile outfile N lofrq       [-r]\n"
		"hilite trace 3 infile outfile N       hifrq [-r]\n"
		"hilite trace 4 infile outfile N lofrq hifrq [-r]\n"
		"\n"
		"RETAIN THE N LOUDEST PARTIALS ONLY (ON A WINDOW-BY-WINDOW BASIS)\n"
		"\n"
		"MODES :-    \n"
		"1  Select loudest spectral components.\n"
		"2  Select loudest from above lofrq: Reject all spectral data below lofrq.\n"
		"3  Select loudest from below hifrq: Reject all spectral data above hifrq.\n"
		"4  Select loudest from between lofrq and hifrq: Reject data outside.\n\n"
		"-r If trace index > no of chans in fltband (defined by lofrq\\hifrq)\n"
		"   RETAIN loudest chans OUTSIDE fltband.\n"
		"   (Default: always omit chans outside fltband)\n"
		"\n"
		"N   is number of spectral components to retain.\n"
		"\n"
		"N, lofrq and hifrq may vary over time.\n");
	} else if(!strcmp(str,"bltr")) {
		fprintf(stdout,
		"hilite bltr infile outfile blurring tracing \n"
		"\n"
		"TIME-AVERAGE, AND TRACE, THE SPECTRUM\n"
		"\n"
		"blurring   is number of windows over which to average the spectrum.\n"
		"tracing    is number of (loudest) channels to retain, in each window.\n"
		"\n"
		"blurring AND tracing may vary over time. \n");
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

