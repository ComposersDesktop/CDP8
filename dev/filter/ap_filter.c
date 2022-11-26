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



/* floatsams version*/
/*RWD Apr 2011 corrected usage message. Rebuilt with fixed (?) library for extra array element */
#include <stdio.h>
#include <stdlib.h>
#include <structures.h>
#include <cdpmain.h>
#include <tkglobals.h>
#include <pnames.h>
#include <filters.h>
#include <processno.h>
#include <modeno.h>
#include <globcon.h>
#include <logic.h>
#include <filetype.h>
#include <mixxcon.h>
#include <arrays.h>
#include <flags.h>
#include <speccon.h>
#include <arrays.h>
#include <special.h>
#include <filters1.h>
#include <formants.h>
#include <sfsys.h>
#include <osbind.h>
#include <string.h>
#include <math.h>
#include <srates.h>
/*RWD April 2004*/
#include <filtcon.h>

#ifndef cdp_round
extern int cdp_round(double a);
#endif

#if defined unix || defined __GNUC__
#define round(x) lround((x))
#endif

/********************************************************************************************/
/********************************** FORMERLY IN buffers.c ***********************************/
/********************************************************************************************/

static int  create_fltiter_buffer(dataptr dz);

/********************************************************************************************/
/********************************** FORMERLY IN specialin.c *********************************/
/********************************************************************************************/

static int  read_filter_data(char *filename,dataptr dz);
static int  read_time_varying_filter_data(char *filename,dataptr dz);
static int  get_data_from_tvary_infile(char *filename,dataptr dz);
static int  getmaxlinelen(int *maxcnt,FILE *fp);
static int  check_filter_data(int wordcnt_in_line,dataptr dz);
static int  check_seq_and_range_of_filter_data(double *fbrk,int wordcnt_in_line,dataptr dz);

static int	check_seq_and_range_of_filter_data2(double *fbrk,double *hbrk,dataptr dz);
static int	getmaxlinelen2(int *maxcnt,FILE *fp);
static int	read_data_from_t_and_p_vary_infile(char *filename,dataptr dz);

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
	case(EQ):
	case(LPHP):
	case(FSTATVAR):
	case(FLTBANKN):
	case(FLTBANKC):
	case(FLTBANKU):
	case(FLTBANKV):
	case(FLTBANKV2):
	case(FLTSWEEP):
	case(ALLPASS):				dz->extra_bufcnt = 0;	dz->bufcnt = 1;		break;

	case(FLTITER):				dz->extra_bufcnt = 0;	dz->bufcnt = 4;		break;
	case(MAKE_VFILT):			dz->extra_bufcnt = 0;	dz->bufcnt = 0;		break;
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
	case(FLTBANKC):	dz->array_cnt = 2; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0;	dz->fptr_cnt = 0; break;

	case(ALLPASS):	dz->array_cnt = 2; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0;	dz->fptr_cnt = 0; break;

//	case(LPHP):		dz->array_cnt = 11; dz->iarray_cnt= 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0; break;
// TW MULTICHAN 2010
	case(LPHP):		
		dz->array_cnt = 0; dz->iarray_cnt= 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0; 
		break;
	case(EQ):	   	dz->array_cnt = 4; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0;	dz->fptr_cnt = 0; break;
	case(FSTATVAR):	dz->array_cnt = 4; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0;	dz->fptr_cnt = 0; break;
	case(FLTSWEEP):	dz->array_cnt = 4; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0;	dz->fptr_cnt = 0; break;

	case(FLTBANKN):	dz->array_cnt = 19; dz->iarray_cnt= 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0; break;
	case(FLTBANKU):	dz->array_cnt = 19; dz->iarray_cnt= 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0; break;
	case(FLTBANKV):	dz->array_cnt = 19; dz->iarray_cnt= 0; dz->larray_cnt = 1; dz->ptr_cnt = 0; dz->fptr_cnt = 0; break;
	case(FLTBANKV2):dz->array_cnt = 22; dz->iarray_cnt= 0; dz->larray_cnt = 1; dz->ptr_cnt = 0; dz->fptr_cnt = 0; break;
	case(FLTITER):	dz->array_cnt = 19; dz->iarray_cnt= 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0; break;
	case(MAKE_VFILT): dz->array_cnt= 1; dz->iarray_cnt= 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0; break;
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
	case(EQ):			  setup_process_logic(SNDFILES_ONLY,		UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(LPHP):			  setup_process_logic(SNDFILES_ONLY,		UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(FSTATVAR):		  setup_process_logic(SNDFILES_ONLY,		UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(FLTBANKN):		  setup_process_logic(SNDFILES_ONLY,		UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(FLTBANKC):		  setup_process_logic(SNDFILES_ONLY,    	TO_TEXTFILE,		TEXTFILE_OUT,	dz);	break;
	case(FLTBANKU):		  setup_process_logic(SNDFILES_ONLY,		UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(FLTBANKV2):
	case(FLTBANKV):		  setup_process_logic(SNDFILES_ONLY,		UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(FLTITER):		  setup_process_logic(SNDFILES_ONLY,		UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(FLTSWEEP):		  setup_process_logic(SNDFILES_ONLY,		UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(ALLPASS):		  setup_process_logic(SNDFILES_ONLY,		UNEQUAL_SNDFILE,	SNDFILE_OUT,	dz);	break;
	case(MAKE_VFILT):	  setup_process_logic(WORDLIST_ONLY,		TO_TEXTFILE,		NO_OUTPUTFILE,	dz);	break;
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
	case(EQ):  	 	exit_status = set_internalparam_data( "000dddiddd",ap);			break;	   
	case(LPHP):    	exit_status = set_internalparam_data(  "000didi",ap);	  		break;	   
	case(FSTATVAR):	exit_status = set_internalparam_data("0000diiiiididd",ap);		break;
	case(FLTBANKN):	exit_status = set_internalparam_data( "000diiiiid",ap);			break;
	case(FLTBANKC):	exit_status = set_internalparam_data( "000di",ap);				break;
	case(FLTBANKU):	exit_status = set_internalparam_data("0000diiiiid",ap);			break;
	case(FLTBANKV):	exit_status = set_internalparam_data("00iidiiiiididiiid",ap);	break;
	case(FLTBANKV2):exit_status = set_internalparam_data("i000iidiiiiididiiidiidd",ap);	break;
	case(FLTITER):	exit_status = set_internalparam_data(    "diiiiiiiiiiid",ap);	break;
	case(FLTSWEEP): exit_status = set_internalparam_data( "000diiiiiddddddddd",ap);	break;
	case(ALLPASS):	exit_status = set_internalparam_data(  "000diiiiiid",ap);		break;
	case(MAKE_VFILT):	break;
	default:
		sprintf(errstr,"Unknown process in set_legal_internalparam_structure()\n");
		return(PROGRAM_ERROR);
	}
	return(exit_status);		
}

/***************************************************************************************/
/******************************* FORMERLY IN sndlib.c **********************************/
/***************************************************************************************/

/*********************** GET_COEFFS1 *************************/

void get_coeffs1(int n,dataptr dz)
{
	dz->parray[FLT_WW][n]   = 2.0 * PI * dz->parray[FLT_FRQ][n] * dz->param[FLT_INV_SR];
	dz->parray[FLT_COSW][n] = cos(dz->parray[FLT_WW][n]);
	dz->parray[FLT_SINW][n] = sin(dz->parray[FLT_WW][n]);
}

/*********************** GET_COEFFS2 ***************************/

void get_coeffs2(int n,dataptr dz)
{
	double g, r;
	r    = exp( -(dz->parray[FLT_WW][n])/(2.0 * dz->param[FLT_Q]));
	dz->parray[FLT_A][n] = 2.0 * r * dz->parray[FLT_COSW][n];
	dz->parray[FLT_B][n] = -(r) * r;
	g    = 1.0 / ((1.0  + dz->parray[FLT_B][n]) * dz->parray[FLT_SINW][n]);
	dz->parray[FLT_AMPL][n] = dz->parray[FLT_AMP][n]/g;
	if(dz->vflag[FLT_DBLFILT])
		dz->parray[FLT_AMPL][n] /= g;
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
	case(FILTERBANK):				return read_filter_data(str,dz);
	case(TIMEVARYING_FILTERBANK):	return read_time_varying_filter_data(str,dz);
	case(TIMEVARY2_FILTERBANK):		return read_data_from_t_and_p_vary_infile(str,dz);
	default:
		sprintf(errstr,"Unknown special_data type: read_special_data()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);	/* NOTREACHED */
}

/************************** READ_FILTER_DATA ********************************/

int read_filter_data(char *filename,dataptr dz)
{
	int exit_status;
	int n = 0, valcnt;
	char temp[200], *p, *thisword;
	double *frq, *amp;
	int is_frq = TRUE;

	if((dz->fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,"Cannot open datafile %s\n",filename);
		return(DATA_ERROR);
	}
	while(fgets(temp,200,dz->fp)!=NULL) {
		p = temp;
		if(is_an_empty_line_or_a_comment(p))
			continue;
		n++;
	}
	if(n==0) {
		sprintf(errstr,"No data in file %s\n",filename);
		return(DATA_ERROR);
	}
	dz->iparam[FLT_CNT] = n;
	if((exit_status = allocate_filter_frq_amp_arrays(dz->iparam[FLT_CNT],dz))<0)
		return(exit_status);
	frq = dz->parray[FLT_FRQ];
	amp = dz->parray[FLT_AMP];
	if(fseek(dz->fp,0,0)<0) {
		sprintf(errstr,"fseek() failed in read_filter_data()\n");
		return(SYSTEM_ERROR);
	}
	n = 0;
	while(fgets(temp,200,dz->fp)!=NULL) {
		p = temp;
		if(is_an_empty_line_or_a_comment(temp))
			continue;
		if(n >= dz->iparam[FLT_CNT]) {
			sprintf(errstr,"Accounting problem reading Frq & Amp: read_filter_data()\n");
			return(PROGRAM_ERROR);
		}
		valcnt = 0;
		while(get_word_from_string(&p,&thisword)) {
			if(valcnt>=2) {
				sprintf(errstr,"Too many values on line %d: file %s\n",n+1,filename);
				return(DATA_ERROR);
			}
			if(is_frq) {
				if(sscanf(thisword,"%lf",&(frq[n]))!=1) {
					sprintf(errstr,"Problem reading Frq data: line %d: file %s\n",n+1,filename);
					return(DATA_ERROR);
				}
				if(frq[n]<dz->application->min_special || frq[n]>dz->application->max_special) {
					sprintf(errstr,"frq (%.3lf) on line %d out of range (%.1lf to %.1lf):file %s\n",
					frq[n],n+1,dz->application->min_special,dz->application->max_special,filename);
					return(DATA_ERROR);
				}
				if(dz->mode==FLT_MIDI)
					frq[n] = miditohz(frq[n]);
			} else {
				if((exit_status = get_level(thisword,&(amp[n])))<0)
					return(exit_status);
				if(amp[n]<FLT_MINGAIN || amp[n]>FLT_MAXGAIN) {
					sprintf(errstr,"amp (%lf) out of range (%lf to %.1lf) on line %d: file %s\n",
					amp[n],FLT_MINGAIN,FLT_MAXGAIN,n+1,filename);
					return(DATA_ERROR);
				}
			}
			is_frq = !is_frq;
			valcnt++;
		}
		if(valcnt<2) {
			sprintf(errstr,"Not enough values on line %d: file %s\n",n+1,filename);
			return(DATA_ERROR);
		}
		n++;
	}
	if(fclose(dz->fp)<0) {
		fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
		fflush(stdout);
	}
	return(FINISHED);
}

/**************************** ALLOCATE_FILTER_FRQ_AMP_ARRAYS *******************************/

int allocate_filter_frq_amp_arrays(int fltcnt,dataptr dz)
{
//	int chans = dz->infile->channels; 
	/*RWD 9:2001 must have empty arrays */
	if((dz->parray[FLT_AMP] = (double *)calloc(fltcnt * sizeof(double),sizeof(char)))==NULL
	|| (dz->parray[FLT_FRQ] = (double *)calloc(fltcnt * sizeof(double),sizeof(char)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for filter amp and frq arrays.\n");
		return(MEMORY_ERROR);
	}
	return(FINISHED);
}

/************************** READ_TIME_VARYING_FILTER_DATA ********************************/

int read_time_varying_filter_data(char *filename,dataptr dz)
{
	int exit_status;
//	int cnt = 0;
	if((exit_status = get_data_from_tvary_infile(filename,dz))<0)
		return(exit_status);
	if((exit_status = check_filter_data(dz->iparam[FLT_ENTRYCNT],dz))<0)
		return(exit_status);
	return(FINISHED);
}

/**************************** GET_DATA_FROM_TVARY_INFILE *******************************/

int get_data_from_tvary_infile(char *filename,dataptr dz)
{
	int exit_status;
	char *temp, *p, *thisword;
	int maxlinelen, frqcnt;
	int total_wordcnt = 0;
	int  columns_in_this_row, columns_in_row = 0, number_of_rows = 0;
/* NEW CODE */
	int n, m, k, old_wordcnt;
	double filedur = (double)(dz->insams[0]/dz->infile->channels)/(double)(dz->infile->srate);
	double far_time = filedur + FLT_TAIL + 1.0;
/* NEW CODE */

	double val;
	if((dz->fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,"Cannot open datafile %s\n",filename);
		return(DATA_ERROR);
	}
	if((exit_status = getmaxlinelen(&maxlinelen,dz->fp))<0)
		return(exit_status);
	if((fseek(dz->fp,0,0))<0) {
		sprintf(errstr,"Seek failed in get_data_from_tvary_infile()\n");
		return(SYSTEM_ERROR);
	}
	if((temp = (char *)malloc((maxlinelen+2) * sizeof(char)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for temporary line storage.\n");
		return(MEMORY_ERROR);
	}
	while(fgets(temp,maxlinelen,dz->fp)!=NULL) {
		columns_in_this_row = 0;
		if(is_an_empty_line_or_a_comment(temp))
			continue;
		p = temp;
		while(get_word_from_string(&p,&thisword)) { 
			if((exit_status = get_level(thisword,&val))<0) {	/* reads vals or dB vals */
				free(temp);
				return(exit_status);
			}
			if((total_wordcnt==0 && ((dz->parray[FLT_FBRK] = (double *)malloc(sizeof(double)))==NULL))
			|| (dz->parray[FLT_FBRK] = (double *)realloc(dz->parray[FLT_FBRK],(total_wordcnt+1) * sizeof(double)))==NULL) {
				free(temp);
				sprintf(errstr,"INSUFFICIENT MEMORY to reallocate filter brktable.\n");
				return(MEMORY_ERROR);
			}
			dz->parray[FLT_FBRK][total_wordcnt] = val;
			columns_in_this_row++;
			total_wordcnt++;
		}
		if(number_of_rows==0) {
			if((columns_in_row = columns_in_this_row)<3) {
				sprintf(errstr,"Insufficient filter data in row 1 of file %s.\n",filename);
				free(temp);
				return(DATA_ERROR);
			} else if (ODD(columns_in_row - 1)) {
				sprintf(errstr,"Frq and Amp data not paired correctly (or no Time) in row 1 of file %s.\n",filename);
				free(temp);
				return(DATA_ERROR);
			}
		} else if(columns_in_this_row!=columns_in_row) {
			free(temp);
			if(columns_in_this_row < columns_in_row)
				sprintf(errstr,"Not enough entries in row %d of file %s\n",number_of_rows+1,filename);
			else
				sprintf(errstr,"Too many entries in row %d of file %s\n",number_of_rows+1,filename);
			return(DATA_ERROR);
		}
		number_of_rows++;
	}
	dz->iparam[FLT_WORDCNT] = total_wordcnt;
	free(temp);
	if(columns_in_row<3) {
		sprintf(errstr,"Insufficient data in each row, to define filters.\n");
		return(DATA_ERROR);
	}
	dz->iparam[FLT_ENTRYCNT] = columns_in_row;
	frqcnt = columns_in_row - 1;
	if(ODD(frqcnt)) {
		sprintf(errstr,"amplitude and freq data not correctly paired in rows.\n");
		return(DATA_ERROR);
	}
	dz->iparam[FLT_CNT]       = frqcnt/2;
	dz->iparam[FLT_TIMESLOTS] = number_of_rows;
	if(fclose(dz->fp)<0) {
		fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
		fflush(stdout);
	}
/* NEW CODE */
	if(dz->parray[FLT_FBRK][0] !=0.0) {
		if(flteq(dz->parray[FLT_FBRK][0],0.0))
			dz->parray[FLT_FBRK][0] = 0.0;
		else {
			dz->iparam[FLT_TIMESLOTS]++;
			old_wordcnt = dz->iparam[FLT_WORDCNT];
			k = old_wordcnt-1;
			dz->iparam[FLT_WORDCNT] += dz->iparam[FLT_ENTRYCNT];
			m = dz->iparam[FLT_WORDCNT] - 1;
			if((dz->parray[FLT_FBRK] = (double *)realloc(dz->parray[FLT_FBRK],dz->iparam[FLT_WORDCNT] * sizeof(double)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY to reallocate filter brktable.\n");
				return(MEMORY_ERROR);
			}
			for(n=0;n<old_wordcnt;n++,m--,k--)
				dz->parray[FLT_FBRK][m] = dz->parray[FLT_FBRK][k];
			dz->parray[FLT_FBRK][0] = 0.0;
			total_wordcnt = dz->iparam[FLT_WORDCNT];
		}
	}

	if(dz->parray[FLT_FBRK][total_wordcnt - dz->iparam[FLT_ENTRYCNT]] < far_time) {
		dz->iparam[FLT_TIMESLOTS]++;
		m = dz->iparam[FLT_WORDCNT];
		k = m - dz->iparam[FLT_ENTRYCNT];
		dz->iparam[FLT_WORDCNT] += dz->iparam[FLT_ENTRYCNT];
		if((dz->parray[FLT_FBRK] = (double *)realloc(dz->parray[FLT_FBRK],dz->iparam[FLT_WORDCNT] * sizeof(double)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to reallocate filter brktable.\n");
			return(MEMORY_ERROR);
		}
		dz->parray[FLT_FBRK][m] = far_time;
		m++;
		k++;
		for(n=1;n<dz->iparam[FLT_ENTRYCNT];n++,m++,k++)
			dz->parray[FLT_FBRK][m] = dz->parray[FLT_FBRK][k];
	}
/* NEW CODE */
	return(FINISHED);
}

/**************************** GETMAXLINELEN *******************************/

int getmaxlinelen(int *maxcnt,FILE *fp)
{
	int thiscnt = 0;
	char c;
	*maxcnt = 0;
	while((c= (char)fgetc(fp))!=EOF) {
		if(c=='\n' || c == ENDOFSTR) {
			*maxcnt = max(*maxcnt,thiscnt);
			thiscnt = 0;
		} else
			thiscnt++;			
	}
	*maxcnt = (int)max(*maxcnt,thiscnt);
	*maxcnt += 4;	/* NEWLINE, ENDOFSTR and safety!! */
	return(FINISHED);
}			

/**************************** CHECK_FILTER_DATA *******************************/

int check_filter_data(int wordcnt_in_line,dataptr dz)
{
	int    exit_status;	
	int   n, lastfilt, new_total_wordcnt;
	double endtime;
	int    total_wordcnt = dz->iparam[FLT_WORDCNT];
	if(dz->parray[FLT_FBRK][0] < 0.0) {
		sprintf(errstr,"Negative time value (%lf) on line 1.\n",dz->parray[FLT_FBRK][0]);
		return(DATA_ERROR);
	}
	if(flteq(dz->parray[FLT_FBRK][0],0.0))		
		dz->parray[FLT_FBRK][0] = 0.0;			/* FORCE A FILTER SETTING AT TIME ZERO */
	else {				 			
		if((dz->parray[FLT_FBRK] = 
		(double *)realloc(dz->parray[FLT_FBRK],(total_wordcnt+wordcnt_in_line) * sizeof(double)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to reallocate filter brktable.\n");
			return(MEMORY_ERROR);
		}
		for(n=total_wordcnt-1; n>=0; n--)
			dz->parray[FLT_FBRK][n + wordcnt_in_line] = dz->parray[FLT_FBRK][n];
		total_wordcnt += wordcnt_in_line;
		dz->parray[FLT_FBRK][0] = 0.0;
		dz->iparam[FLT_TIMESLOTS]++;
	}
	dz->iparam[FLT_WORDCNT] = total_wordcnt;
	if((exit_status = check_seq_and_range_of_filter_data(dz->parray[FLT_FBRK],wordcnt_in_line,dz))<0)
		return(exit_status);
								/* FORCE A FILTER SETTING AT (BEYOND) END OF FILE */
	lastfilt = total_wordcnt - wordcnt_in_line;
	endtime =  (double)(dz->insams[0]/dz->infile->channels)/(double)dz->infile->srate;
	if(dz->parray[FLT_FBRK][lastfilt] <= endtime) {
		if((dz->parray[FLT_FBRK] = 
		(double *)realloc(dz->parray[FLT_FBRK],(total_wordcnt + wordcnt_in_line) * sizeof(double)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to reallocate filter brktable.\n");
			return(MEMORY_ERROR);
		}
		new_total_wordcnt = total_wordcnt + wordcnt_in_line;
		for(n=total_wordcnt;n<new_total_wordcnt;n++)
			dz->parray[FLT_FBRK][n] = dz->parray[FLT_FBRK][n - wordcnt_in_line];
		dz->parray[FLT_FBRK][total_wordcnt] = endtime + 1.0;
		total_wordcnt = new_total_wordcnt;
		dz->iparam[FLT_TIMESLOTS]++;
	}
	if(dz->iparam[FLT_TIMESLOTS]<2) {
		sprintf(errstr,"Error in timeslot logic: check_filter_data()\n");
		return(PROGRAM_ERROR);
	}
	dz->iparam[FLT_WORDCNT] = total_wordcnt;
	return(FINISHED);
}

/**************************** CHECK_SEQ_AND_RANGE_OF_FILTER_DATA *******************************/

int check_seq_and_range_of_filter_data(double *fbrk,int wordcnt_in_line,dataptr dz)
{
	double lasttime = 0.0;
	int n, m, lineno;
	for(n=1;n<dz->iparam[FLT_WORDCNT];n++) {
		m = n%wordcnt_in_line;
		lineno = (n/wordcnt_in_line)+1;	/* original line-no : ignoring comments */
		if(m==0) {
			if(fbrk[n] <= lasttime) {
				sprintf(errstr,"Time is out of sequence on line %d\n",lineno);
				return(DATA_ERROR);
//NEW AUGUST 2006
//			} else if(fbrk[n] > dz->duration) {
//				fbrk[n] = dz->duration + 1.0;
//				dz->iparam[FLT_WORDCNT] = lineno * wordcnt_in_line;
			}
			lasttime = fbrk[n];
		} else if(ODD(m)) {
			if(fbrk[n]<dz->application->min_special || fbrk[n]>dz->application->max_special) {
				sprintf(errstr,"frq_or_midi value [%.3lf] out of range (%.1f - %.1f) on line %d\n",
				fbrk[n],dz->application->min_special,dz->application->max_special,lineno);
					return(DATA_ERROR);
			}
			if(dz->mode==FLT_MIDI)
				fbrk[n] = miditohz(fbrk[n]);
		} else
			fbrk[n] = max(fbrk[n],MINFILTAMP);	/* Zero filter amp, forced to +ve, but effectively zero */
	}
	return(FINISHED);
}


/********************************************************************************************/
/********************************** FORMERLY IN preprocess.c ********************************/
/********************************************************************************************/

/****************************** PARAM_PREPROCESS *********************************/

int param_preprocess(dataptr dz)	
{
//	int exit_status = FINISHED;

	switch(dz->process) {
	case(EQ):			case(LPHP):			case(FSTATVAR):
	case(FLTBANKN):		case(FLTBANKU):		case(FLTBANKV):
	case(FLTSWEEP):		case(ALLPASS):		case(FLTITER):
	case(FLTBANKC):		case(FLTBANKV2):
		return filter_preprocess(dz);
	case(MAKE_VFILT):	break;
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
//	int exit_status = FINISHED;

	if(dz->process!= FLTBANKC)
		display_virtual_time(0L,dz);

	switch(dz->process) {
	case(LPHP):
	case(EQ):	 		case(FSTATVAR):		case(FLTSWEEP):		case(ALLPASS):
	case(FLTBANKN):		case(FLTBANKC):		case(FLTBANKU):		case(FLTBANKV):		
	case(FLTBANKV2):		
	return filter_process(dz);
	case(FLTITER):			
		return iterating_filter(dz);
	case(MAKE_VFILT):
		return make_vfilt_data(dz);
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
	case(EQ):		case(LPHP):		case(FSTATVAR):		case(FLTBANKN):
	case(FLTBANKU):	case(FLTBANKV):	case(FLTSWEEP):		case(ALLPASS):
	case(FLTITER):	case(FLTBANKC):	case(FLTBANKV2):		
		return filter_pconsistency(dz);
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
	case(EQ):	   		case(LPHP):			case(FSTATVAR):
	case(FLTBANKN):		case(FLTBANKU):		case(FLTBANKV):		case(FLTBANKV2):
	case(FLTSWEEP):		case(ALLPASS):			
		return create_sndbufs(dz);

	case(FLTITER):			
		return create_fltiter_buffer(dz);
	case(FLTBANKC):			
	case(MAKE_VFILT):
		return(FINISHED);
	default:
		sprintf(errstr,"Unknown program no. in allocate_large_buffers()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);	/* NOTREACHED */
}

/*************************** CREATE_FLTITER_BUFFER **************************
 *
 * (1)	Create extra spaces for interpolation guard points at end of infile.
 *
 * (2) 	Allow for any cumulative addition errors in interpolation.
 *
 * (3)	Output buffer must be at least as big as the overflow buffer.
 *	Output buffer must be big enough for the whole of any possible
 *	data overflow (in overflow_size buff) to be copied back into it.
 *	This is because the overflow buffer is ZEROED after such a copy
 *	and if a 2nd copy of the overflow back into the main buffer
 *	were necessary, we would be copying zeroes rather than true data.
 *
 *	Output buffer must be bigger than the maximum possible delay.
 * 	When write_start is beyond end of buffer, we copy the overflow buffer
 *	data back into the true buffer and reset the pointers by subtracting
 *	buflen (length of output buffer). We must be sure that 'write_start'
 *	will then be located WITHIN the true buffer. In the worst possible
 *	case the write_start pointer is at the very end of the true buffer,
 *	and is then delayed to its maximum possible extent, maxdelay_size (Z),
 *	which will be Z samps into the overflow buffer. After the copy-back,
 *	the 'write_start' pointer must lie WITHIN the true buffer.
 *	Hence the true buflen must be >= maximum delay.
 *
 *		true buffer	    			overflow
 *  |-----------------------------|------------------------------------------|
 *			    			 worst^					    ^
 *		    		 possible case^					    ^
 *		             		      ^----->-delayed by maxdelay_size to ->-----^
 *			 					  ^<-restored by -buffer_size into truebuf-<-^
 *  |<-------- BUFFER_SIZE------->
 *
 */

int create_fltiter_buffer(dataptr dz)
{
	int   big_buffer_size;
	size_t bigbufsize;
	double k;
	int  chans = dz->infile->channels;
	int bufminimum, infilesamps;
	dz->iparam[FLT_INFILESPACE] = dz->insams[0];
	if(dz->param[FLT_PSHIFT]>0.0) {
		dz->iparam[FLT_INFILESPACE] += chans;		/* 1 */
		k = pow(2.0,dz->param[FLT_PSHIFT]);
		dz->iparam[FLT_OVFLWSIZE]  = ((round((dz->insams[0]/chans) * k) * chans) + chans);
		dz->iparam[FLT_OVFLWSIZE] += FLT_SAFETY;						/* 2 */
	} else
		dz->iparam[FLT_OVFLWSIZE] = dz->iparam[FLT_INFILESPACE] + FLT_SAFETY;
	infilesamps      = dz->iparam[FLT_INFILESPACE];
	bufminimum       = dz->iparam[FLT_OVFLWSIZE];
	bigbufsize   = (size_t)Malloc(-1);
	bigbufsize /= sizeof(float);
	bigbufsize  -= (dz->iparam[FLT_INFILESPACE] * 2) + dz->iparam[FLT_OVFLWSIZE];
	if(bigbufsize<bufminimum)
		bigbufsize = bufminimum;	
//TW COMMENT: filters don't sfseek: so old buffer protocol can be abandoned

	dz->buflen = (int) bigbufsize;
	big_buffer_size = (dz->iparam[FLT_INFILESPACE] * 2) + dz->buflen + dz->iparam[FLT_OVFLWSIZE];
	if((dz->bigbuf = (float *) malloc((size_t)(big_buffer_size * sizeof(float)))) == NULL) {
		sprintf(errstr, "INSUFFICIENT MEMORY to create sound buffers.\n");
		return(MEMORY_ERROR);
	}
	dz->sampbuf[0]        	  = dz->bigbuf;
	dz->sampbuf[1]        	  = dz->sampbuf[0] + infilesamps;
	dz->sampbuf[FLT_OUTBUF]   = dz->sampbuf[1] + infilesamps;
	dz->sampbuf[FLT_OVFLWBUF] = dz->sampbuf[FLT_OUTBUF] + dz->buflen;
	memset((char *)dz->bigbuf,0,(size_t)(big_buffer_size * sizeof(float)));
	return(FINISHED);
}


/********************************************************************************************/
/********************************** FORMERLY IN cmdline.c ***********************************/
/********************************************************************************************/

int get_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	if     (!strcmp(prog_identifier_from_cmdline,"fixed"))			dz->process = EQ;
	else if(!strcmp(prog_identifier_from_cmdline,"lohi"))			dz->process = LPHP;
	else if(!strcmp(prog_identifier_from_cmdline,"variable"))		dz->process = FSTATVAR;
	else if(!strcmp(prog_identifier_from_cmdline,"bank"))			dz->process = FLTBANKN;
	else if(!strcmp(prog_identifier_from_cmdline,"bankfrqs"))		dz->process = FLTBANKC;
	else if(!strcmp(prog_identifier_from_cmdline,"userbank"))		dz->process = FLTBANKU;
	else if(!strcmp(prog_identifier_from_cmdline,"varibank"))		dz->process = FLTBANKV;
	else if(!strcmp(prog_identifier_from_cmdline,"varibank2"))		dz->process = FLTBANKV2;
	else if(!strcmp(prog_identifier_from_cmdline,"iterated"))		dz->process = FLTITER;
	else if(!strcmp(prog_identifier_from_cmdline,"sweeping"))		dz->process = FLTSWEEP;
	else if(!strcmp(prog_identifier_from_cmdline,"phasing"))		dz->process = ALLPASS;
	else if(!strcmp(prog_identifier_from_cmdline,"vfilters"))		dz->process = MAKE_VFILT;
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
	"USAGE: filter NAME (mode) infile outfile (datafile) parameters\n"
	"\n"
	"where NAME can be any one of\n"
	"\n"
	"fixed      lohi         variable\n"
	"bank       bankfrqs     userbank      varibank      varibank2\n"
	"iterated   sweeping     phasing	   vfilters\n"
	"\n"
	"Type 'filter  userbank'  for more info on filter userbank... ETC.\n");
	return(USAGE_ONLY);
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
	if (!strcmp(str,"fixed")) {		 	/* EQ */
		fprintf(stdout,
		"CUT OR BOOST, ABOVE, BELOW OR AROUND A GIVEN FRQ\n\n"
		"USAGE: filter fixed 1-2  infile outfile        boost/cut freq [-ttail] [-sprescale]\n"
		"OR:    filter fixed 3    infile outfile bwidth boost/cut freq [-ttail] [-sprescale]\n\n"
		"MODES ARE...\n"
		"1) BOOST OR CUT BELOW GIVEN FRQ\n"
		"2) BOOST OR CUT ABOVE GIVEN FRQ\n"
		"3) BOOST OR CUT A BAND CENTERED ON GIVEN FRQ\n\n"
		"BWIDTH     is the filter bandwidth in Hz for Mode 3 only\n"
		"BOOST/CUT  is the boost or cut, in dB\n"
		"FREQ       is the filter frequency in Hz\n"
		"TAIL       decay tail duration\n"
		"PRESCALE   scales the INPUT to the filter.\n");

	} else if (!strcmp(str,"lohi")) {		 /* LOHI */
		fprintf(stdout,
    	"FIXED LOW PASS OR HIGH PASS FILTER.\n\n"
    	"USAGE:\nfilter lohi mode infile outfile attenuation pass-band stop-band [-ttail] [-sprescale]\n\n"
	    "MODES ARE\n"
	    "1) Pass-band and stop-band as freq in Hz.\n"
	    "2) Pass-band and stop-band as (possibly fractional) midi notes.\n\n"
	    "ATTENUATION Gain reduction of filter, in dB. Range(0 to %.0lf)\n"
	    "            Greater attenuation, sharper filter, but longer to calculate.\n"
	    "PASS-BAND   last pitch to be passed by the filter.\n"
	    "STOP-BAND   first pitch to be stopped by the filter.\n"
	    "            If stop-band is above pass-band, this is a lopass filter.\n"
	    "            If stop-band is below pass-band, this is a hipass filter.\n"
		"TAIL       decay tail duration\n"
	    "-s          Prescale input: Avoid overflows: Range(%.3lf to %.1lf)\n",
	    MIN_DB_ON_16_BIT,FLT_MINEQPRESCALE,FLT_MAXEQPRESCALE);

	} else if (!strcmp(str,"variable")) {		 /* FSTATVAR */
		fprintf(stdout,
		"LOPASS, HIGH-PASS, BANDPASS, OR NOTCH FILTER WITH VARIABLE FRQ\n\n"
		"USAGE: filter variable mode infile outfile acuity gain frq [-ttail]\n\n"
		"MODES ARE..\n"
		"1) NOTCH (Band Reject)\n\n"
		"2) BAND-PASS\n"
		"3) LOW-PASS\n"
		"4) HIGH-PASS\n"
	    "ACUITY   acuity of filter: Range(%lf to %.1lf).\n"
	    "         Smaller vals give tighter filter.\n"
		"GAIN     overall gain on output:  Range(%lf to %.1lf)\n"
		"         Rule of thumb:\n"
		"         if acuity = (1/3)to-power-n: gain = (2/3)-to-power-n\n"
	    "FRQ      frq of filter:           Range(%.1lf to srate/2)\n\n"	
		"ACUITY and FRQ can vary over time.\n"
		"TAIL       decay tail duration\n",MIN_ACUITY,MAX_ACUITY,FLT_MINGAIN,FLT_MAXGAIN,FLT_MINFRQ);

	} else if (!strcmp(str,"bank")) {		 /* FLTBANKN */
     	fprintf(stdout,
		"BANK OF FILTERS, WITH TIME-VARIABLE Q\n\n"
		"USAGE: filter bank 1-3 infile outfile Q gain lof hif       [-ttail] [-sscat] [-d]\n"
		"OR:    filter bank 4-6 infile outfile Q gain lof hif param [-ttail] [-sscat] [-d]\n\n"
		"MODES...\n"
		"1)  HARMONIC SERIES over lofrq.\n"
		"2)  ALTERNATE HARMONICS over lofrq.\n"
		"3)  SUBHARMONIC SERIES below hifrq.\n"
		"4)  HARMONIC SERIES WITH LINEAR OFFSET: param = offset in Hz.\n"
		"5)  EQUAL INTERVALS BETWEEN lo & hifrq: param = no. of filters.\n"
		"6)  EQUAL INTERVALS BETWEEN lo & hifrq: param = interval semitone-size.\n\n"
		"Q      Q (tightness) of filters (Range %lf <= Q < %.1lf)\n"
		"GAIN   overall gain (Range %lf to %.1lf).\n"
		"LOF    lofrq limit of filters  (Range: %.0lf to srate/3).\n"
		"HIF    hifrq limit of filters  (Range: lofrq+ to srate/3).\n"
		"TAIL   decay tail duration\n"
		"SCAT   Random scatter of filter frqs (Range 0 to 1: Default 0).\n"
		"-d     Double filtering.\n\n"
		"Q may vary over time.\n",MINQ,MAXQ,FLT_MINGAIN,FLT_MAXGAIN,FLT_MINFRQ);

	} else if (!strcmp(str,"bankfrqs")) {		 /* FLTBANKC */
     	fprintf(stdout,
		"GENERATE A LIST OF FREQUENCIES FOR USE AS A FILTERBANK\n"
		"(ADD AMPLITUDES TO THE TEXTFILE FOR USE WITH 'USERBANK')\n\n"
		"USAGE: filter bankfrqs 1-3 anysndfile outtextfile lof hif\n"
		"OR:    filter bankfrqs 4-6 anysndfile outtextfile lof hif param\n\n"
		"MODES...\n"					   
		"1)  HARMONIC SERIES over lofrq.\n"
		"2)  ALTERNATE HARMONICS over lofrq.\n"
		"3)  SUBHARMONIC SERIES below hifrq.\n"
		"4)  HARMONIC SERIES WITH LINEAR OFFSET: param = offset in Hz.\n"
		"5)  EQUAL INTERVALS BETWEEN lo & hifrq: param = no. of filters.\n"
		"6)  EQUAL INTERVALS BETWEEN lo & hifrq: param = interval semitone-size.\n\n"
		"LOF    lofrq limit of filters  (Range: %.0lf to srate/3).\n"
		"HIF    hifrq limit of filters  (Range: lofrq+ to srate/3).\n"
//		"SCAT   Random scatter of filter frqs (Range 0 to 1: Default 0).\n\n" /* RWD Apr 2011 this param not supported here */
		"NB sampling rate of input soundfile determines frq range\n",FLT_MINFRQ);

	} else if (!strcmp(str,"userbank")) {		 /* FLTBANKU */
      	fprintf(stdout,
	   	"USER-DEFINED FILTERBANK,WITH TIME-VARIABLE Q\n\n"
		"USAGE: filter userbank mode infile outfile datafile Q gain [-ttail] [-d]\n\n"
		"MODES are\n"
		"1)  Filter-pitches as frq in Hz.\n"
		"2)  Filter-pitches as MIDI values.\n\n"
		"DATAFILE: has Pitch & Amp of filters (paired, one pair on each line).\n"
		"          Ranges: Pitch (%.0lfHz to sr/3) Amp (%lf to %.1lf)\n"
		"          Amplitude may also be expressed in decibels.\n"
		"          Comment-lines (starting with ';') may be used.\n\n"
		"Q:        Q (tightness) of filter: Range(%lf to %.1lf)\n"
		"GAIN      overall gain, Range(%lf to %.1lf).\n"
		"TAIL      decay tail duration\n"
		"-d        double filtering.\n\n"
		"Q may vary over time.",FLT_MINFRQ,FLT_MINGAIN,FLT_MAXGAIN,MINQ,MAXQ,FLT_MINGAIN,FLT_MAXGAIN);

	} else if (!strcmp(str,"varibank")) {		 /* FLTBANKV */
		fprintf(stdout,
	   	"USER-DEFINED TIME_VARYING FILTERBANK,WITH TIME-VARIABLE Q\n\n"
		"USAGE:\nfilter varibank mode infile outfile data Q gain [-ttail] [-hhcnt] [-rrolloff] [-d]\n\n"
		"MODES ARE...\n"
		"1)  Enter filter-pitches as frq, in Hz.\n"
		"2)  Enter filter-pitches as MIDI values.\n\n"
		"DATAFILE: has lines of data for filter bands at successive times.\n"
		"          Each line contains the following items\n"
		"                Time:   Pitch1 Amp1   [Pitch2 Amp2    etc....].\n"
		"          Pitch and Amp values must be paired:\n"
		"          any number of pairs can be used in a line,\n"
		"          BUT each line must have SAME number of pairs on it.\n"
		"          (To eliminate a band in any line(s), set its amplitude to 0.0).\n"
		"          Time values (in secs) must be in ascending order (and >=0.0)\n"
		"          Pitch vals may be EITHER frq, OR MIDI, depending on MODE.\n"
		"          Amp values may be numeric, or dB values (e.g. -4.1dB).\n"
		"          Comment-lines may be used: start these with ';'.\n\n"
		"Q         Q (tightness) of filter : Range(%lf to %.1lf).\n"		
		"GAIN:     overall gain: Range: (%lf to %.1lf)\n"					
		"TAIL      decay tail duration\n"
		"HCNT      No of harmonics of each pitch to use: Default 1.\n"
		"          High harmonics of high pitches may be beyond nyquist.\n"
		"          (No-of-pitches times no-of-harmonics determines program speed).\n"
		"ROLLOFF   Level drop (in dB) from one harmonic to next. Range(0 to %.1lf)\n"
		"-d        double filtering.\n\n"
		"Q may also vary over time.\n",MINQ,MAXQ,FLT_MINGAIN, FLT_MAXGAIN,MIN_DB_ON_16_BIT);

	} else if (!strcmp(str,"varibank2")) {		 /* FLTBANKV */
		fprintf(stdout,
	   	"USER-DEFINED TIME_VARYING FILTERBANK,WITH TIME-VARIABLE Q AND PARTIALS\n"
		"USAGE:\nfilter varibank2 mode infil outfil data Q gain [-ttail] [-d]\n"
		"MODES ARE...\n"
		"1)  Enter filter-pitches as frq, in Hz.\n"
		"2)  Enter filter-pitches as MIDI values.\n\n"
		"DATAFILE: has lines of data for filter bands at successive times.\n"
		"          as varibank filter....\n"
		"          Followed by a line with a '#' sign at the start.\n"
		"          Followed by 2nd set of lines, similar to the pitchdata lines\n"
		"          but now the data is a time followed by any number of pairs of\n"
		"          partial number (possibly fractional) + amplitude of partial.\n"
		"          Times for pitch AND for partials data MUST START AT ZERO.\n"
		"Q         Q (tightness) of filter : Range(%lf to %.1lf).\n"		
		"GAIN:     overall gain: Range: (%lf to %.1lf)\n"					
		"TAIL      decay tail duration\n"
		"-d        double filtering.\n\n"
		"Q may also vary over time.\n",MINQ,MAXQ,FLT_MINGAIN,FLT_MAXGAIN);

	} else if (!strcmp(str,"iterated")) {		 /* FLTITER */
		fprintf(stdout,
    	"ITERATE SOUND, WITH CUMULATIVE FILTERING BY A FILTERBANK.\n\n"
    	"USAGE: filter iterated mode infile outfile datafile Q gain delay dur \n"
    	"         [-sprescale] [-rrand] [-ppshift] [-aashift] [-d] [-i] [-e] [-n]\n\n"
		"MODES ARE...\n"
		"1)  Enter filter-pitches as frq, in Hz.\n"
		"2)  Enter filter-pitches as MIDI values.\n\n"
    	"DATAFILE  has Pitch & Amp of filters (paired, one pair on each line).\n"
    	"          Pitch, as Hz or MIDI, Range(%.0lfHz to srate/3)\n"		
    	"          Amp, Range(%lf to %.1lf), can also be entered as dB vals\n\n" 
    	"Q         Q (tightness) of filter: Range(%lf to %.1lf)\n"    
    	"GAIN      overrall gain in filtering process (%lf < gain < %.1lf)\n"
    	"DELAY     (average) delay between iterations (secs).\n"
    	"          Range(>0 to %.0lf)\n"
    	"DUR       (min) duration of output file (>infile duration).\n"
    	"PRESCALE  gain on the INPUT to the filtering process.\n"
    	"          Range (0.0 - 1.0) Default 1.0\n"
    	"          If set to 0.0, Program automatically divides input level by\n"
    	"          the max no. of sound overlays occuring in the iteration process.\n"
    	"RAND      delaytime-randomisation: Range 0 (none) to 1 (max).\n"
    	"PSHIFT    max pitchshift of any segment in (fractions of) semitones (>=0).\n"
    	"ASHIFT    max amplitude reduction of any segment: Range (0.0 - 1.0)\n"
    	"-d        double filtering.\n"
	  	"-i        Turn off Interpolation during pitch shift: (fast but dirty).\n"
    	"-e        Exponential decay added: each seg gets quieter before next enters.\n"
    	"-n        turns OFF normalisation: segs may grow or fall in level quickly.\n",
    	FLT_MINFRQ,FLT_MINGAIN,FLT_MAXGAIN,MINQ, MAXQ, FLT_MINGAIN,FLT_MAXGAIN,FLT_MAXDELAY);

	} else if (!strcmp(str,"sweeping")) {		 /* FLTSWEEP */
		fprintf(stdout,
	    "FILTER WHOSE FOCUS-FREQUENCY SWEEPS OVER A RANGE OF FREQUENCIES\n\n"
		"USAGE:\n"
		"filter sweeping mode infile outfile acuity gain lofrq hifrq sweepfrq [-ttail] [-pphase]\n\n"
		"MODES ARE...\n"
		"1) NOTCH (Band-reject). 3) LOW-PASS\n"
		"2) BAND-PASS            4) HIGH-PASS  \n\n"
	    "ACUITY   Acuity of filter:  Range(%lf to %.1lf)\n"		
	    "         Smaller vals give tighter filter.\n"
		"GAIN     overall gain on output:   Range(%lf to %.1lf)\n"		
		"         Rule of thumb:\n"
		"         if acuity = (1/3)to-power-n: gain = (2/3)-to-power-n\n"
	    "LOFRQ    lowest frq to sweep to:   Range(%.1lf to srate/2)\n"	
	    "HIFRQ    highest frq to sweep to:  Range(%.1lf to srate/2)\n"	
	    "SWEEPFRQ frq of the sweep itself:  Range(0.0 to %.1lf)\n"		
	    "         e.g. to sweep once over range, set sweepfrq to infiledur/2 (default)\n"		
		"         and set phase to 0 (upsweep) or .5(downsweep)\n"
		"TAIL      decay tail duration\n"
		"PHASE    start of sweep: Range(0-1)\n"
		"         vals are: 0 = (lofrq) rising to .5 = (hifrq) falling to 1 = (lofrq).\n"
		"         Default value is .25 (midfrq,rising)\n\n"
		"ACUITY, LOFRQ, HIFRQ and SWEEPFRQ may all vary over time.\n"
		"This filter is most effective in BAND-PASS or LOW-PASS mode, at low ACUITY.\n",
		MIN_ACUITY,MAX_ACUITY,FLT_MINGAIN,FLT_MAXGAIN,FLT_MINFRQ,FLT_MINFRQ,FLT_MAXSWEEP);

	} else if (!strcmp(str,"phasing")) {		 /* ALLPASS */
		fprintf(stdout,
		"PHASESHIFT SOUND, OR PRODUCE 'PHASING' EFFECT\n\n"
		"USAGE: filter phasing mode infile outfile gain delay [-ttail] [-sprescale] [-l]\n\n"
		"MODES ARE..\n"
		"1) ALLPASS FILTER (PHASE-SHIFTED)\n"
		"2) PHASING EFFECT\n\n"
		"GAIN      Range (-1.0 to 1.0)\n"
		"          In MODE 2, phasing effect increases as gain increases from -1\n"
		"          BUT a gain of 1.0 will produce complete phase cancellation\n"
		"          and the output signal will be zero.\n"
		"DELAY     >0.0 MS\n"
		"TAIL      decay tail duration\n"
		"PRESCALE  gain on the INPUT to the filtering process.\n"
    	"          Range (0.0 - 1.0) Default 1.0\n"
		"-l        linear interpolation of changing delay vals (default: logarithmic)\n\n"
		"DELAY may vary over time.\n");
	} else if (!strcmp(str,"vfilters")) {		 /* MAKE VFILTER FIXED-PITCH DATA FILES */
		fprintf(stdout,
		"MAKE FIXED-PITCH DATA-FILES FOR VARIBANK FILTER\n\n"
		"USAGE: filter vfilters inpitchfile outfile\n\n"
		"INPITCHFILE     Textfile, contains a list of MIDI or FRQ pitch values\n"
		"                with one or more values on each line.\n"
		"Each line is converted to a data file for a fixed pitch(es) filter,\n"
		"of the 'varibank' type.\n"
		"Producing outfile0.txt, outfile1.txt etc.\n");
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

/******************************** INNER_LOOP (redundant)  ********************************/

int inner_loop
(int *peakscore,int *descnt,int *in_start_portion,int *least,int *pitchcnt,int windows_in_buf,dataptr dz)
{
	return(FINISHED);
}

/**************************** CHECK_SEQ_AND_RANGE_OF_FILTER_DATA2 *******************************/

int check_seq_and_range_of_filter_data2(double *fbrk,double *hbrk,dataptr dz)
{
	double lasttime = -1.0;
	double maxpartial = dz->infile->srate/(2.0 * FLT_MINFRQ);
	double minpartial = 1.0;
	int n, m, lineno = 0;
	double frqmax  = dz->infile->srate/2.0;
	double midimax = unchecked_hztomidi(frqmax);
	double midimin = unchecked_hztomidi(FLT_MINFRQ);

	int wordcnt_in_line = dz->iparam[FLT_ENTRYCNT];
	for(n=0;n<dz->iparam[FLT_WORDCNT];n++) {
		m = n%wordcnt_in_line;
		if(m==0) {
			lineno++;				/* original line-no : ignoring comments */
			if(fbrk[n] <= lasttime) {
				sprintf(errstr,"Time is out of sequence in pitch data line %d\n",lineno);
				return(DATA_ERROR);
			}
			lasttime = fbrk[n];
		} else if(ODD(m)) {
			if(dz->mode==FLT_MIDI) {
				if(fbrk[n] < midimin || fbrk[n] > midimax) {
					sprintf(errstr,"midi value [%.3lf] out of range (%.1f - %.1f) on line %d\n",
					fbrk[n],midimin,midimax,lineno);
					return(DATA_ERROR);
				}
				fbrk[n] = miditohz(fbrk[n]);
			} else {
				if(fbrk[n] < FLT_MINFRQ || fbrk[n] > frqmax) {
					sprintf(errstr,"frq_or_midi value [%.3lf] out of range (%.1f - %.1f) on line %d\n",
					fbrk[n],FLT_MINFRQ,frqmax,lineno);
					return(DATA_ERROR);
				}
			}
		} else
			fbrk[n] = max(fbrk[n],MINFILTAMP);	/* Zero filter amp, forced to +ve, but effectively zero */
	}
	wordcnt_in_line = dz->iparam[HRM_ENTRYCNT];
	lasttime = -1.0;
	lineno = 0;
	for(n=0;n<dz->iparam[HRM_WORDCNT];n++) {
		m = n%wordcnt_in_line;
		if(m==0) {
			lineno++;
			if(hbrk[n] <= lasttime) {
				sprintf(errstr,"Time is out of sequence in partials data line %d\n",lineno);
				return(DATA_ERROR);
			}
			lasttime = hbrk[n];
		} else if(ODD(m)) {
			if(hbrk[n]<minpartial|| hbrk[n]>maxpartial) {
				sprintf(errstr,"partial value [%.3lf] out of range (%lf - %.1f) on line %d\n",hbrk[n],minpartial,maxpartial,lineno);
				return(DATA_ERROR);
			}
		} else
			hbrk[n] = max(hbrk[n],MINFILTAMP);	/* Zero filter amp, forced to +ve, but effectively zero */
	}
	return(FINISHED);
}

/**************************** READ_DATA_FROM_T_AND_P_VARY_INFILE *******************************/

int read_data_from_t_and_p_vary_infile(char *filename,dataptr dz)
{
	int exit_status;
	char *temp, *p, *thisword;
	int maxlinelen, frqcnt = 0, hcnt;
	int total_wordcnt = 0, number_of_rows = 0, total_number_of_rows = 0,columns_in_this_row,columns_in_row = 0;
	int  harmdata = 0, ishash = 0, thisarray;

	double val;
	if((dz->fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,"Cannot open datafile %s\n",filename);
		return(DATA_ERROR);
	}
	if((exit_status = getmaxlinelen2(&maxlinelen,dz->fp))<0)
		return(exit_status);
	if((fseek(dz->fp,0,0))<0) {
		sprintf(errstr,"Seek failed in get_data_from_t_and_p_vary_infile()\n");
		return(SYSTEM_ERROR);
	}
	if((temp = (char *)malloc((maxlinelen+2) * sizeof(char)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for temporary line storage.\n");
		return(MEMORY_ERROR);
	}
	while(fgets(temp,maxlinelen,dz->fp)!=NULL) {
		columns_in_this_row = 0;
		if(is_an_empty_line_or_a_comment(temp))
			continue;
		p = temp;
		while(get_word_from_string(&p,&thisword)) {
			if(!strncmp(thisword,"#",1)) {
				if(harmdata) {
					sprintf(errstr,"HASH SIGN AT WRONG PLACE IN DATA in LINE %d.\n",total_number_of_rows+1);
					return(DATA_ERROR);
				}
				dz->iparam[FLT_WORDCNT] = total_wordcnt;
				dz->iparam[FLT_ENTRYCNT] = columns_in_row;
				total_wordcnt = 0;
				number_of_rows = 0;
				frqcnt = columns_in_row - 1;
				harmdata = 1;
				ishash = 1;
				break;
			}
			if((exit_status = get_level(thisword,&val))<0) {	/* reads vals */
				free(temp);
				return(exit_status);
			}
			if(harmdata)
				thisarray = FLT_HBRK;
			else
				thisarray = FLT_FBRK;
			if(total_wordcnt==0) {
				if ((dz->parray[thisarray] = (double *)malloc(sizeof(double)))==NULL) {
					sprintf(errstr,"INSUFFICIENT MEMORY to reallocate filter brktables.\n");
					return(MEMORY_ERROR);
				}
				dz->parray[thisarray][total_wordcnt] = val;
			} else {
				if ((dz->parray[thisarray] = (double *)realloc(dz->parray[thisarray],(total_wordcnt+1) * sizeof(double)))==NULL) {
					sprintf(errstr,"INSUFFICIENT MEMORY to reallocate filter brktables.\n");
					return(MEMORY_ERROR);
				}
			}
			dz->parray[thisarray][total_wordcnt] = val;
			columns_in_this_row++;
			total_wordcnt++;
		}
		if(ishash) {
			ishash = 0;
			continue;
		}
		if(number_of_rows==0) {

			if((columns_in_row = columns_in_this_row)<3) {
				sprintf(errstr,"Insufficient data in row 1 of file %s.\n",filename);
				free(temp);
				return(DATA_ERROR);
			} else if (ODD(columns_in_row - 1)) {
				sprintf(errstr,"Value and Amp data not paired correctly (or no Time) in row %d of file %s.\n",total_number_of_rows+1,filename);
				free(temp);
				return(DATA_ERROR);
			}
		} else if(columns_in_this_row!=columns_in_row) {
			free(temp);
			if(columns_in_this_row < columns_in_row)
				sprintf(errstr,"Not enough entries in row %d of file %s\n",total_number_of_rows+1,filename);
			else
				sprintf(errstr,"Too many entries in row %d of file %s\n",total_number_of_rows+1,filename);
			return(DATA_ERROR);
		}
		number_of_rows++;
		total_number_of_rows++;
	}
	dz->iparam[HRM_WORDCNT] = total_wordcnt;
	free(temp);
	if(columns_in_row<3) {
		sprintf(errstr,"Insufficient data in each row, to define filters.\n");
		return(DATA_ERROR);
	}
	dz->iparam[HRM_ENTRYCNT] = columns_in_row;
	hcnt = columns_in_row - 1;
	dz->iparam[FLT_CNT]       = frqcnt/2;
	dz->iparam[FLT_HARMCNT]   = hcnt/2;
	dz->iparam[FLT_CNT] *= dz->iparam[FLT_HARMCNT];
	if(fclose(dz->fp)<0) {
		fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
		fflush(stdout);
	}
	if(dz->parray[FLT_FBRK][0] !=0.0) {
		sprintf(errstr,"FIRST TIME VALUE FOR PITCHES MUST BE ZERO\n");
		return(DATA_ERROR);
	}
	if(dz->parray[FLT_HBRK][0] !=0.0) {
		sprintf(errstr,"FIRST TIME VALUE FOR HARMONICS MUST BE ZERO\n");
		return(DATA_ERROR);
	}
	return check_seq_and_range_of_filter_data2(dz->parray[FLT_FBRK],dz->parray[FLT_HBRK],dz);
}

/**************************** GETMAXLINELEN2 *******************************/

int getmaxlinelen2(int *maxcnt,FILE *fp)
{
	int thiscnt = 0;
	int gothash = 0;
	int gotdata1 = 0, gotdata2 = 0;
	char c;
	*maxcnt = 0;

	while((c= (char)fgetc(fp))!=EOF) {
		if(!gothash) {
			if(c=='#') {
				if(!gotdata1) {
					sprintf(errstr,"FAILED TO GET FIRST SET OF DATA (PITCH INFORMATION) BEFORE HASH SIGN\n");
					return(DATA_ERROR);
				}
				while((c= (char)fgetc(fp))!=EOF) {
					if(c == '\n' || c == ENDOFSTR) {
						gothash = 1;
						thiscnt = 0;
						break;
					}
				}
				if(!gothash)
					break;
			} else {
				if(c=='\n' || c == ENDOFSTR) {
					*maxcnt = max(*maxcnt,thiscnt);
					thiscnt = 0;
					gotdata1 = 1;
				} else {
					thiscnt++;			
				}
			}
		} else {
			if(c=='\n' || c == ENDOFSTR) {
				*maxcnt = max(*maxcnt,thiscnt);
				thiscnt = 0;
				gotdata2 = 1;
			} else {
				thiscnt++;			
			}
		}
	}
	if(!gotdata2) {
		sprintf(errstr,"FAILED TO GET SECOND SET OF DATA (PARTIALS INFORMATION) AFTER HASH SIGN\n");
		return(DATA_ERROR);
	}
	*maxcnt = (int)max(*maxcnt,thiscnt);
	*maxcnt += 4;	/* NEWLINE, ENDOFSTR and safety!! */
	return(FINISHED);
}			

