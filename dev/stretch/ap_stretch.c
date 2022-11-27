/*
 * Copyright (c) 1983-2020 Trevor Wishart and Composers Desktop Project Ltd
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
#include <stretch.h>
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

static int  allocate_tstretch_buffer(dataptr dz);

/********************************************************************************************/
/********************************** FORMERLY IN pconsistency.c ******************************/
/********************************************************************************************/

static int  check_viability_and_compatibility_of_stretch_params(dataptr dz);
static int  check_for_enough_tstretch_brkpnt_vals(dataptr dz);

/********************************************************************************************/
/********************************** FORMERLY IN preprocess.c ********************************/
/********************************************************************************************/

#define POSMIN      (200)   /* Minimum size for positions array in spec tstretch */
static int  set_internal_stretch_params(dataptr dz);
static int  setup_internal_params_for_tstretch(dataptr dz);

/***************************************************************************************/
/****************************** FORMERLY IN aplinit.c **********************************/
/***************************************************************************************/

/***************************** ESTABLISH_BUFPTRS_AND_EXTRA_BUFFERS **************************/

int establish_bufptrs_and_extra_buffers(dataptr dz)
{
    int exit_status;
    int is_spec = FALSE;
    dz->extra_bufcnt = -1;  /* ENSURE EVERY CASE HAS A PAIR OF ENTRIES !! */
    dz->bptrcnt = 0;
    dz->bufcnt  = 0;
    switch(dz->process) {
    case(STRETCH):          dz->extra_bufcnt =  0; dz->bptrcnt = 1;     is_spec = TRUE;     break;
    case(TSTRETCH):         dz->extra_bufcnt =  0; dz->bptrcnt = 6;     is_spec = TRUE;     break;
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
    dz->ptr_cnt    = -1;        /* base constructor...process */
    dz->array_cnt  = -1;
    dz->iarray_cnt = -1;
    dz->larray_cnt = -1;
    switch(dz->process) {
    case(STRETCH):  dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;   break;
    case(TSTRETCH): dz->array_cnt =2;  dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt =4;  dz->fptr_cnt = 0;   break;
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
        if((dz->ptr     = (double  **)malloc(dz->ptr_cnt  * sizeof(double *)))==NULL) {
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
    case(STRETCH):      setup_process_logic(ANALFILE_ONLY,          EQUAL_ANALFILE,     ANALFILE_OUT,   dz);    break;
    case(TSTRETCH):
        switch(dz->mode) {
        case(TSTR_NORMAL):setup_process_logic(ANALFILE_ONLY,        BIG_ANALFILE,       ANALFILE_OUT,   dz);    break;
        case(TSTR_LENGTH):setup_process_logic(ANALFILE_ONLY,        SCREEN_MESSAGE,     NO_OUTPUTFILE,  dz);    break;
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
    case(STRETCH):      exit_status = set_internalparam_data("idd",ap);                 break;
    case(TSTRETCH):     exit_status = set_internalparam_data("di",ap);                  break;
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
    return(FINISHED);   /* NOTREACHED */
}

/********************************************************************************************/
/********************************** FORMERLY IN preprocess.c ********************************/
/********************************************************************************************/

/****************************** PARAM_PREPROCESS *********************************/

int param_preprocess(dataptr dz)
{
    switch(dz->process) {
    case(STRETCH):      return set_internal_stretch_params(dz);
    case(TSTRETCH):     return setup_internal_params_for_tstretch(dz);
    default:
        sprintf(errstr,"PROGRAMMING PROBLEM: Unknown process in param_preprocess()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);   /* NOTREACHED */
}

/************ SET_INTERNAL_STRETCH_PARAMS *************/

int set_internal_stretch_params(dataptr dz)
{
    dz->param[STR_FFRQ]  += dz->chwidth/2.0;    /* set up FDCNO */
    dz->iparam[STR_FDCNO] = (int)floor(dz->param[STR_FFRQ]/dz->chwidth);
    if(dz->vflag[STR_IS_DEPTH]) {
        dz->param[STR_SL1] = dz->param[STR_SHIFT] - 1.0;
        if(dz->brksize[STR_DEPTH]==0)
            dz->param[STR_NSHIFT] = (dz->param[STR_SL1] * dz->param[STR_DEPTH]) + 1.0;
    } else
        dz->param[STR_NSHIFT] = dz->param[STR_SHIFT];
    return(FINISHED);
}

/************ SETUP_INTERNAL_PARAMS_FOR_TSTRETCH *************/

int setup_internal_params_for_tstretch(dataptr dz)
{
    int exit_status;
    if(dz->brksize[TSTR_STRETCH] && (exit_status = force_value_at_zero_time(TSTR_STRETCH,dz))<0)
        return(exit_status);
    dz->param[TSTR_TOTIME] = (double)dz->wlength * dz->frametime;
    /* dur of orig sound source */

    dz->iparam[TSTR_ARRAYSIZE]  = POSMIN;
    if((dz->parray[TSTR_PBUF]
        = (double *)malloc(dz->iparam[TSTR_ARRAYSIZE] * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for timestretch array.\n");
        return(MEMORY_ERROR);
    }
    dz->ptr[TSTR_PEND]  = dz->parray[TSTR_PBUF] + dz->iparam[TSTR_ARRAYSIZE];
    if((dz->parray[TSTR_QBUF]  
        = (double *)malloc(dz->iparam[TSTR_ARRAYSIZE] * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for 2nd timestretch array.\n");
        return(MEMORY_ERROR);
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

    if(!(dz->process==TSTRETCH && dz->mode==TSTR_LENGTH))
        display_virtual_time(0L,dz);

    switch(dz->process) {
    case(STRETCH):  return outer_loop(dz);
    case(TSTRETCH): return spectstretch(dz);
    default:
        sprintf(errstr,"Unknown process in procspec()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);   /* NOTREACHED */
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
        case(STRETCH):  exit_status = specstretch(dz);                      break;
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
    case(STRETCH):     return check_viability_and_compatibility_of_stretch_params(dz);
    case(TSTRETCH):    return check_for_enough_tstretch_brkpnt_vals(dz);
    }
    return(FINISHED);
}

/*************** CHECK_VIABILITY_AND_COMPATIBILITY_OF_STRETCH_PARAMS **********/

int check_viability_and_compatibility_of_stretch_params(dataptr dz)
{
    int exit_status;
    /* set up FDCNO for tests */
    double  frq = dz->param[STR_FFRQ] + dz->chwidth/2.0;
    int fdcno = (int)floor(frq/dz->chwidth);
    if(flteq(dz->param[STR_SHIFT],1.0)) {
        sprintf(errstr,"Shift = 1. No change. Use your original source.\n");
        return(DATA_ERROR);
    }
    if(dz->vflag[STR_IS_DEPTH]) {
        if((exit_status = check_depth_vals(STR_DEPTH,dz)) <0)
            return(exit_status);
    }
    switch(dz->mode) {
    case(STRETCH_ABOVE):
        if(dz->param[STR_SHIFT] > 1.0
        && round((double)(dz->clength-1)/dz->param[STR_SHIFT]) < fdcno) {
            sprintf(errstr,"Maxstretch incompatible with frq_split.\n");
            return(DATA_ERROR);
        }
        if(dz->param[STR_SHIFT] <= 1.0
        && round((double)(dz->clength-1)*dz->param[STR_SHIFT]) < fdcno) {
            sprintf(errstr,"Maxstretch incompatible with frq_split.\n");
            return(DATA_ERROR);
        }
        break;
    case(STRETCH_BELOW):
        if(dz->param[STR_SHIFT] >= (double)fdcno) {
            sprintf(errstr,"Maxstretch incompatible with frq_split.\n");
            return(DATA_ERROR);
        }
        if(dz->param[STR_SHIFT] <= 1.0
        && 1.0/dz->param[STR_SHIFT] >= (double)fdcno) {
            sprintf(errstr,"Maxstretch incompatible with frq_split.\n");
            return(DATA_ERROR);
        }
        break;
    default:
        sprintf(errstr,"Unknown spec stretch mode in check_stretch_params()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
}

/********************* CHECK_FOR_ENOUGH_TSTRETCH_BRKPNT_VALS **********************/

int check_for_enough_tstretch_brkpnt_vals(dataptr dz)
{
    if(dz->brksize[TSTR_STRETCH] && dz->brksize[TSTR_STRETCH] < 2) {
        sprintf(errstr,"Not enough data in tsretch data file.\n");
        return(DATA_ERROR);
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
    case(STRETCH):
        return allocate_single_buffer(dz);
    case(TSTRETCH):
        if(dz->mode==TSTR_NORMAL)
            return allocate_tstretch_buffer(dz);
        return(FINISHED);
    default:
        sprintf(errstr,"Unknown program no. in allocate_large_buffers()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);   /* NOTREACHED */
}

/*************************** ALLOCATE_TSTRETCH_BUFFER ****************************/

int allocate_tstretch_buffer(dataptr dz)
{
    unsigned int buffersize;
    if(dz->bptrcnt < 6) {
        sprintf(errstr,"Insufficient bufptrs established in allocate_tstretch_buffer()\n");
        return(PROGRAM_ERROR);
    }
//TW REVISED: multiplle of LCM no longer required
    buffersize = dz->wanted * BUF_MULTIPLIER;
    dz->buflen = buffersize;
    buffersize *= 2;
    buffersize += dz->wanted;
    if((dz->bigfbuf = (float *)malloc((size_t)(buffersize * sizeof(float))))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for sound buffers.\n");
        return(MEMORY_ERROR);
    }
    dz->big_fsize = dz->buflen;
    dz->flbufptr[0] = dz->bigfbuf + dz->wanted;         /* inbuf */
    dz->flbufptr[1] = dz->flbufptr[0] + dz->big_fsize;  /* outbuf & inbufend */
    dz->flbufptr[2] = dz->flbufptr[1] + dz->big_fsize;  /* outbufend */

    dz->flbufptr[3] = dz->flbufptr[0];                  /* 1st inbuf pointer */
    dz->flbufptr[4] = dz->flbufptr[0] + dz->wanted;     /* 2nd inbuf pointer */
    dz->flbufptr[5] = dz->flbufptr[1];                  /* outbuf ptr */
    return(FINISHED);
}

/********************************************************************************************/
/********************************** FORMERLY IN cmdline.c ***********************************/
/********************************************************************************************/

int get_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
    if     (!strcmp(prog_identifier_from_cmdline,"spectrum"))       dz->process = STRETCH;
    else if(!strcmp(prog_identifier_from_cmdline,"time"))           dz->process = TSTRETCH;
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
    "\nSTRETCHING A SPECTRAL FILE\n\n"
    "USAGE: stretch NAME (mode) infile outfile parameters: \n"
    "\n"
    "where NAME can be any one of\n"
    "spectrum      time\n\n"
    "Type 'stretch spectrum' for more info on stretch spectrum..ETC.\n");
    return(USAGE_ONLY);
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
    if(!strcmp(str,"spectrum")) {     /* STRETCH */
        fprintf(stdout,
        "stretch spectrum mode infile outfile frq_divide maxstretch exponent [-ddepth]\n"
        "\n"
        "STRETCH THE FREQUENCIES IN THE SPECTRUM\n"
        "\n"
        "MODES:-\n"
        "1     Stretch above the frq_divide.\n"
        "2     Stretch below the frq_divide.\n"
        "\n"
        "frq_divide is the frq above or below which spectral stretching takes place.\n"
        "maxstretch is the transposition ratio of the topmost spectral components.\n"
        "exponent   specifies the type of stretching required.  (> 0)\n"
        "depth      stretch effect on source (from 0 (no effect) to 1 (full effect))\n\n"
        "depth can vary over time.\n");
    } else if(!strcmp(str,"time")) {    /* TSTRETCH */
        sprintf(errstr,
        "stretch time 1 infile outfile timestretch\n"
        "stretch time 2 infile timestretch\n"
        "\n"
        "TIME-STRETCHING OF INFILE.\n"
        "In mode 2, program calculates length of output, only."
        "\n"
        "Timestretch may itself vary over time.\n");
    } else
        sprintf(errstr,"Unknown option '%s'\n",str);
    return(USAGE_ONLY);
}

/******************************** USAGE3 ********************************/

int usage3(char *str1,char *str2)
{
//TW AVOID WARNINGS
//    str1 = str1;
//    str2 = str2;

    sprintf(errstr,"Insufficient parameters on command line.\n");
    return(USAGE_ONLY);
}

