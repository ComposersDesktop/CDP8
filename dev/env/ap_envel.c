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
#include <string.h>
#include <math.h>
#include <structures.h>
#include <cdpmain.h>
#include <tkglobals.h>
#include <pnames.h>
#include <envel.h>
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

//#ifdef unix
#define round(x) lround((x))
//#endif
/********************************************************************************************/
/********************************** FORMERLY IN buffers.c ***********************************/
/********************************************************************************************/

static int  do_special_brkpnt_envelling_buffer(dataptr dz);
static int  create_pluck_buffers(dataptr dz);

/********************************************************************************************/
/********************************** FORMERLY IN pconsistency.c ******************************/
/********************************************************************************************/

static int  setup_envel_windowsize(dataptr dz);
static double  get_outfile_wsize_in_msecs(int envwindow_sampsize,dataptr dz);
static void inject_dbl_parameter(int paramno,dataptr dz);
static int  setup_endofsndfile_value_if_needed(dataptr dz);
static int  create_envsyn_buf(dataptr dz);
/********************************************************************************************/
/********************************** FORMERLY IN specialin.c *********************************/
/********************************************************************************************/

static int  read_env_create_file(char *str,dataptr dz);
static int  read_createfile_level(char *q,int cnt,dataptr dz);
static int  reallocate_create_table_memory(int thissize,dataptr dz);
static int  read_envsyn_file(char *str,dataptr dz);
/***************************************************************************************/
/************************************ NEW FUNCTIONS ************************************/
/***************************************************************************************/

//TW UPDATE
static int scale_envelope_data(dataptr dz);

/***************************** ESTABLISH_BUFPTRS_AND_EXTRA_BUFFERS **************************/

int establish_bufptrs_and_extra_buffers(dataptr dz)
{
//  int is_spec = FALSE;
    dz->extra_bufcnt = -1;  /* ENSURE EVERY CASE HAS A PAIR OF ENTRIES !! */
    dz->bptrcnt = 0;
    dz->bufcnt  = 0;
    switch(dz->process) {
    case(ENV_EXTRACT):
    case(ENV_WARPING):
    case(ENV_REPLACE):
        dz->extra_bufcnt = 0;   dz->bufcnt = 1;     break;
    case(ENV_IMPOSE):
        switch(dz->mode) {
        case(ENV_DB_BRKFILE_IN):
        case(ENV_BRKFILE_IN):   
            dz->extra_bufcnt = 0;   dz->bufcnt = 3;     break;
        default:                
            dz->extra_bufcnt = 0;   dz->bufcnt = 1;     break;
        }
        break;
//TW NEW CASE
    case(ENV_PROPOR):
        dz->extra_bufcnt = 0;   dz->bufcnt = 3;     break;
    case(ENV_DOVETAILING):
    case(ENV_CURTAILING):
    case(ENV_SWELL):
    case(ENV_ATTACK):
        dz->extra_bufcnt = 0;   dz->bufcnt = 3;     break;
    case(ENV_PLUCK):
        dz->extra_bufcnt = 0;   dz->bufcnt = 5;     break;
    case(ENV_TREMOL):
//TW NEW CASE
    case(TIME_GRID):
        dz->extra_bufcnt = 0;   dz->bufcnt = 1;     break;
    case(ENVSYN):
        dz->extra_bufcnt =  0;  dz->bptrcnt = 1;    break;
    case(ENV_ENVTOBRK):
    case(ENV_ENVTODBBRK):
    case(ENV_BRKTOENV):
    case(ENV_CREATE):
    case(ENV_RESHAPING):
    case(ENV_REPLOTTING):
    case(ENV_DBBRKTOENV):
    case(ENV_DBBRKTOBRK):
    case(ENV_BRKTODBBRK):
        dz->extra_bufcnt = 0;   dz->bufcnt = 0;     break;
    default:
        sprintf(errstr,"Unknown program type [%d] in establish_bufptrs_and_extra_buffers()\n",dz->process);
        return(PROGRAM_ERROR);
    }

    if(dz->extra_bufcnt < 0) {
        sprintf(errstr,"bufcnts have not been set: establish_bufptrs_and_extra_buffers()\n");
        return(PROGRAM_ERROR);
    }
    if(dz->process == ENVSYN)
        return establish_spec_bufptrs_and_extra_buffers(dz);
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
    case(ENV_CREATE):
    case(ENV_DOVETAILING):
    case(ENV_CURTAILING):
    case(ENV_SWELL):
    case(ENV_ATTACK):
        dz->array_cnt = 4; dz->iarray_cnt = 1; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0; break;
    case(ENV_TREMOL):
        dz->array_cnt = 1; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0; break;
    case(ENV_EXTRACT):
    case(ENV_IMPOSE):
//TW NEW CASE
    case(ENV_PROPOR):
    case(ENV_REPLACE):
    case(ENV_ENVTOBRK):
    case(ENV_ENVTODBBRK):
    case(ENV_BRKTOENV):
    case(ENV_DBBRKTOENV):
    case(ENV_DBBRKTOBRK):
    case(ENV_BRKTODBBRK):
    case(ENV_WARPING):
    case(ENV_RESHAPING):
    case(ENV_REPLOTTING):
    case(ENV_PLUCK):
        dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0; break;
//TW NEW CASE
    case(TIME_GRID):
        dz->array_cnt = 2; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0; break;
    case(ENVSYN):
        dz->array_cnt = 1; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 2; dz->fptr_cnt = 0; break;
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
//TW FIXED
//      if((dz->fptr = (float  **)malloc(dz->fptr_cnt * sizeof(int *)))==NULL) {
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
    case(ENV_DOVETAILING): setup_process_logic(SNDFILES_ONLY,       EQUAL_SNDFILE,      SNDFILE_OUT,    dz);    break;
    case(ENV_CURTAILING):  setup_process_logic(SNDFILES_ONLY,       UNEQUAL_SNDFILE,    SNDFILE_OUT,    dz);    break;
    case(ENV_SWELL):       setup_process_logic(SNDFILES_ONLY,       EQUAL_SNDFILE,      SNDFILE_OUT,    dz);    break;
    case(ENV_ATTACK):      setup_process_logic(SNDFILES_ONLY,       EQUAL_SNDFILE,      SNDFILE_OUT,    dz);    break;
    case(ENV_ENVTOBRK):    setup_process_logic(ENVFILES_ONLY,       TO_TEXTFILE,        BRKFILE_OUT,    dz);    break;
    case(ENV_ENVTODBBRK):  setup_process_logic(ENVFILES_ONLY,       TO_TEXTFILE,        BRKFILE_OUT,    dz);    break;
    case(ENV_BRKTOENV):    setup_process_logic(BRKFILES_ONLY,       CREATE_ENVFILE,     ENVFILE_OUT,    dz);    break;
    case(ENV_WARPING):     setup_process_logic(SNDFILES_ONLY,       EQUAL_SNDFILE,      SNDFILE_OUT,    dz);    break;
    case(ENV_RESHAPING):
        switch(dz->mode) {
        case(ENV_TSTRETCHING):
                           setup_process_logic(ENVFILES_ONLY,       UNEQUAL_ENVFILE,    ENVFILE_OUT,    dz);    break;
        default:
                           setup_process_logic(ENVFILES_ONLY,       EQUAL_ENVFILE,      ENVFILE_OUT,    dz);    break;
        }
        break;
    case(ENV_REPLOTTING):  setup_process_logic(BRKFILES_ONLY,       TO_TEXTFILE,        BRKFILE_OUT,    dz);    break;
    case(ENV_DBBRKTOENV):  setup_process_logic(DB_BRKFILES_ONLY,    CREATE_ENVFILE,     ENVFILE_OUT,    dz);    break;
    case(ENV_DBBRKTOBRK):  setup_process_logic(DB_BRKFILES_ONLY,    TO_TEXTFILE,        BRKFILE_OUT,    dz);    break;
    case(ENV_BRKTODBBRK):  setup_process_logic(BRKFILES_ONLY,       TO_TEXTFILE,        BRKFILE_OUT,    dz);    break;
    case(ENV_CREATE):
        switch(dz->mode) {
        case(ENV_ENVFILE_OUT):
                           setup_process_logic(NO_FILE_AT_ALL,      CREATE_ENVFILE,     ENVFILE_OUT,    dz);    break;

        case(ENV_BRKFILE_OUT):
                           setup_process_logic(NO_FILE_AT_ALL,      TO_TEXTFILE,        BRKFILE_OUT,    dz);    break;
        default:
            sprintf(errstr,"Unknown case for ENV_CREATE in assign_process_logic()\n");
            return(PROGRAM_ERROR);
        }
        break;
    case(ENV_EXTRACT):
        switch(dz->mode) {
        case(ENV_ENVFILE_OUT):
                           setup_process_logic(SNDFILES_ONLY,       EXTRACT_ENVFILE,    ENVFILE_OUT,    dz);    break;

        case(ENV_BRKFILE_OUT):
                           setup_process_logic(SNDFILES_ONLY,       TO_TEXTFILE,        BRKFILE_OUT,    dz);    break;
        default:
            sprintf(errstr,"Unknown case for ENV_EXTRACT in assign_process_logic()\n");
            return(PROGRAM_ERROR);
        }
        break;
//TW NEW CASE
    case(ENV_PROPOR):
       setup_process_logic(SNDFILE_AND_UNRANGED_BRKFILE,    UNEQUAL_SNDFILE,    SNDFILE_OUT,    dz);
       break;

    case(ENV_IMPOSE):
        switch(dz->mode) {
        case(ENV_ENVFILE_IN):
                           setup_process_logic(SNDFILE_AND_ENVFILE, EQUAL_SNDFILE,      SNDFILE_OUT,    dz);    break;
        case(ENV_SNDFILE_IN):
                           setup_process_logic(TWO_SNDFILES,        EQUAL_SNDFILE,      SNDFILE_OUT,    dz);    break;
        case(ENV_BRKFILE_IN):
                           setup_process_logic(SNDFILE_AND_UNRANGED_BRKFILE,    UNEQUAL_SNDFILE,    SNDFILE_OUT,    dz);    break;
        case(ENV_DB_BRKFILE_IN):
                         setup_process_logic(SNDFILE_AND_DB_BRKFILE,UNEQUAL_SNDFILE,    SNDFILE_OUT,    dz);    break;
        default:
            sprintf(errstr,"Unknown case for ENV_IMPOSE in assign_process_logic()\n");
            return(PROGRAM_ERROR);
        }
        break;
    case(ENV_REPLACE):
        switch(dz->mode) {
        case(ENV_ENVFILE_IN):
                           setup_process_logic(SNDFILE_AND_ENVFILE, EQUAL_SNDFILE,      SNDFILE_OUT,    dz);    break;
        case(ENV_SNDFILE_IN):
                           setup_process_logic(TWO_SNDFILES,        EQUAL_SNDFILE,      SNDFILE_OUT,    dz);    break;
        case(ENV_BRKFILE_IN):
                           setup_process_logic(SNDFILE_AND_BRKFILE, UNEQUAL_SNDFILE,    SNDFILE_OUT,    dz);    break;
        case(ENV_DB_BRKFILE_IN):
                         setup_process_logic(SNDFILE_AND_DB_BRKFILE,UNEQUAL_SNDFILE,    SNDFILE_OUT,    dz);    break;
        default:
            sprintf(errstr,"Unknown mode for ENV_REPLACE in assign_process_logic()\n");
            return(PROGRAM_ERROR);
        }
        break;
    case(ENV_PLUCK):       setup_process_logic(SNDFILES_ONLY,       UNEQUAL_SNDFILE,    SNDFILE_OUT,    dz);    break;
    case(ENV_TREMOL):      setup_process_logic(SNDFILES_ONLY,       EQUAL_SNDFILE,      SNDFILE_OUT,    dz);    break;
//TW NEW CASE
    case(TIME_GRID):       setup_process_logic(SNDFILES_ONLY,       EQUAL_SNDFILE,      SNDFILE_OUT,    dz);    break;
    case(ENVSYN):          setup_process_logic(NO_FILE_AT_ALL,      CREATE_ENVFILE,     ENVFILE_OUT,    dz);    break;
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
    case(ENV_IMPOSE):
//TW NEW CASE
    case(ENV_PROPOR):
    case(ENV_REPLACE):
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
                        /* DANGER: if programs with >1 options added!! */
                        /* comment: these programs have NO options.  1st internal param is a dummy to ensure */
                        /* ENV_SAMP_WSIZE is always dz->iparam[5]      */
    case(ENV_CREATE):       case(ENV_WARPING):      case(ENV_RESHAPING):
    case(ENV_IMPOSE):       case(ENV_REPLACE):      case(ENV_BRKTOENV):             
    case(ENV_DBBRKTOENV):   case(ENV_DBBRKTOBRK):   case(ENV_BRKTODBBRK):
//TW + NEW CASE
    case(ENV_SWELL):        case(ENV_PROPOR):
                        exit_status = set_internalparam_data("0i",ap);              break;
    case(ENV_EXTRACT):                  
        switch(mode) {
        case(ENV_ENVFILE_OUT):      /*DANGER: as above */
                        exit_status = set_internalparam_data("0i",ap);              break;
        case(ENV_BRKFILE_OUT):      /* as below */ 
                        exit_status = set_internalparam_data("i",ap);                   break;
        }
        break;

 /* comment: these programs have 1 option. No dummy internal param needed. */
    case(ENV_REPLOTTING):
    case(ENV_DOVETAILING):          
    case(ENV_CURTAILING):           
    case(ENV_ENVTOBRK):
    case(ENV_ENVTODBBRK):
                        exit_status = set_internalparam_data("i",ap);                   break;

 /* commrnt: these progs work differently: no problems here!! */
    case(ENV_ATTACK):   exit_status = set_internalparam_data("id",ap);              break;
    case(ENV_PLUCK):    exit_status = set_internalparam_data("ii",ap);              break;
    case(ENV_TREMOL):   exit_status = set_internalparam_data("",ap);                    break;
//TW NEW CASE
    case(TIME_GRID):    exit_status = set_internalparam_data("",ap);                break;
    case(ENVSYN):       exit_status = set_internalparam_data("",ap);                break;
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
//  int exit_status = FINISHED;
    aplptr ap = dz->application;

    switch(ap->special_data) {
    case(ENV_TRIGGER_RAMP):     return read_env_ramp_brk(str,dz);
    case(ENV_CREATEFILE):       return read_env_create_file(str,dz);
    case(ENVSYN_ENVELOPE):      return read_envsyn_file(str,dz);
    default:
        sprintf(errstr,"Unknown special_data type: read_special_data()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);   /* NOTREACHED */
}

/************************* READ_ENV_RAMP_BRK *****************************/

int read_env_ramp_brk(char *filename,dataptr dz)
{
//  double maxval = 1.0;
//  double minval = 0.0;
    int arraysize = BIGARRAY;
    double *p, lasttime = 0.0;
    int  istime = TRUE;
    int n = 0, final_size;
    char temp[200], *q;
    FILE *fp;
    if((fp = fopen(filename,"r"))==NULL) {          
        sprintf(errstr,"Failed to open envelope ramp file %s\n",filename);
        return(DATA_ERROR);
    }
    if((dz->rampbrk = (double *)malloc(arraysize * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create ramp array.\n");
        return(MEMORY_ERROR);
    }
    p = dz->rampbrk;
    while(fgets(temp,200,fp)!=NULL) {    /* READ AND TEST BRKPNT VALS */
        q = temp;
        while(get_float_from_within_string(&q,p)) {
            if(istime) {
                if(p!=dz->rampbrk) {
                    if(*p <= lasttime) {
                        sprintf(errstr,"Times (%lf & %lf) in ramp brkpntfile %s are not in increasing order.\n",
                        lasttime,*p,filename);
                        return(DATA_ERROR);
                    }
                }
                lasttime = *p;
            } else {
                if(*p < 0.0 || *p > 1.0) {
                    sprintf(errstr,"Brkpntfile value (%lf) out of range (0 - 1)\n",*p);
                    return(DATA_ERROR);
                }
            }
            istime = !istime;
            p++;
            if(++n >= arraysize) {
                arraysize += BIGARRAY;
                if((dz->rampbrk = (double *)realloc((char *)(dz->rampbrk),arraysize * sizeof(double)))==NULL) {
                    sprintf(errstr,"INSUFFICIENT MEMORY to reallocate ramp array.\n");
                    return(MEMORY_ERROR);
                }
                p = dz->rampbrk + n;        
            }
        }
    }       
    if(n < 2) {
        sprintf(errstr,"No significant data in ramp brkpnt file %s\n",filename);
        return(DATA_ERROR);
    }
    if(ODD(n)) {
        sprintf(errstr,"Data not paired correctly in ramp brkpntfile %s\n",filename);
        return(DATA_ERROR);
    }
    final_size = n;
    if(dz->rampbrk[0] != 0.0) { /* Force base-time to zero */
        for(n=0;n<final_size;n+=2)
            dz->rampbrk[n] -= dz->rampbrk[0];           
    }
    if((dz->rampbrk = (double *)realloc((char *)(dz->rampbrk),final_size * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to reallocate ramp array.\n");
        return(MEMORY_ERROR);
    }
    dz->rampbrksize = final_size/2;
    return(FINISHED);
}

/************************* READ_ENV_CREATE_FILE *****************************/

int read_env_create_file(char *str,dataptr dz)
{
    int exit_status;
    int cnt  = 0;
    double lasttime = 0.0;
    int gotlevel = TRUE;
    char temp[200], *q, *p;
    int arraysize = BIGARRAY;
    FILE *fp;
    if((fp = fopen(str,"r"))==NULL) {           
        sprintf(errstr,"Cannot open envelope create file %s\n",str);
        return(DATA_ERROR);
    }
    if((dz->parray[ENV_CREATE_INLEVEL] = (double *)malloc(arraysize * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to make level data array.\n");
        return(MEMORY_ERROR);
    }
    if((dz->parray[ENV_CREATE_INTIME]  = (double *)malloc(arraysize * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to make time data array.\n");
        return(MEMORY_ERROR);
    }
    if((dz->iparray[ENV_SLOPETYPE]  = (int *)malloc(arraysize * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to make slope data array.\n");
        return(MEMORY_ERROR);
    }
    while(fgets(temp,200,fp)!=NULL) {    /* READ AND TEST BRKPNT VALS */
        q = temp;
        while(get_word_from_string(&q,&p)) {
            if(gotlevel) {
                if(*p=='e') {
                    sprintf(errstr,"Time and (exponential) level out of sequence in createfile.\n");
                    return(DATA_ERROR);
                }
                if(sscanf(p,"%lf",&dz->parray[ENV_CREATE_INTIME][cnt])!=1) {
                    sprintf(errstr,"Cannot read time %d\n",cnt);
                    return(PROGRAM_ERROR);
                }
                if(dz->parray[ENV_CREATE_INTIME][cnt] < dz->application->min_special
                || dz->parray[ENV_CREATE_INTIME][cnt] > dz->application->max_special) {
                    sprintf(errstr,"Envelope time out of range %lf - %lf.\n",
                    dz->application->min_special,dz->application->max_special);
                    return(DATA_ERROR);
                }
                if((cnt > 0)
                && (dz->parray[ENV_CREATE_INTIME][cnt] <= lasttime + (2.0 * ENV_MIN_WSIZE * MS_TO_SECS))) {
                    sprintf(errstr,"timegap %lf - %lf is too small.\n",
                    dz->parray[ENV_CREATE_INTIME][cnt],dz->parray[ENV_CREATE_INTIME][cnt-1]);
                    return(DATA_ERROR);
                }
                lasttime = dz->parray[ENV_CREATE_INTIME][cnt];
                gotlevel = FALSE;
            } else {
                switch(*p) {
                case('e'):  dz->iparray[ENV_SLOPETYPE][cnt] = ENVTYPE_EXP;  p++;    break;
                default:    dz->iparray[ENV_SLOPETYPE][cnt] = ENVTYPE_LIN;          break;
                }
                if((exit_status = read_createfile_level(p,cnt,dz))<0)
                    return(exit_status);
                if(++cnt >= arraysize) {
                    arraysize += BIGARRAY;
                    if((exit_status = reallocate_create_table_memory(arraysize,dz))<0)
                        return(exit_status);
                }
                gotlevel = TRUE;
            }
        }
    }
    if(!gotlevel) {
        sprintf(errstr,"Levels and Times not correctly paired: read_env_create_file()\n");
        return(DATA_ERROR); 
    }
    if(cnt<=0) {
        sprintf(errstr,"Failed to read any data from file %s.\n",str);
        return(DATA_ERROR);
    }
    if((exit_status = reallocate_create_table_memory(cnt,dz))<0)
        return(exit_status);
    dz->itemcnt = cnt;
    return(FINISHED);
}

/**************************** READ_CREATEFILE_LEVEL ********************************/

int read_createfile_level(char *q,int cnt,dataptr dz)
{
    int exit_status;
    int is_a_dB_val = is_dB(q);
    if(sscanf(q,"%lf",&(dz->parray[ENV_CREATE_INLEVEL][cnt]))!=1) {
        sprintf(errstr,"Cannot read level %d: read_createfile_level()\n",cnt);
        return(PROGRAM_ERROR);
    }
    if(is_a_dB_val) {
        if((exit_status = convert_dB_at_or_below_zero_to_gain(&(dz->parray[ENV_CREATE_INLEVEL][cnt])))<0)
            return(exit_status);
    } else if(dz->parray[ENV_CREATE_INLEVEL][cnt] < dz->application->min_special2 
           || dz->parray[ENV_CREATE_INLEVEL][cnt] > dz->application->max_special2) {
        sprintf(errstr,"Level value %lf out of range\n",dz->parray[ENV_CREATE_INLEVEL][cnt]);
        return(DATA_ERROR);
    }
    if(flteq(dz->parray[ENV_CREATE_INLEVEL][cnt],dz->parray[ENV_CREATE_INLEVEL][cnt-1]))   
        dz->iparray[ENV_SLOPETYPE][cnt] = ENVTYPE_LIN;      /* Force linear interp, for equal levels */
    return(FINISHED);
}

/************** REALLOCATE_CREATE_TABLE_MEMORY ***********/

int reallocate_create_table_memory(int thissize,dataptr dz)
{
    if((dz->parray[ENV_CREATE_INLEVEL]= (double *)realloc(dz->parray[ENV_CREATE_INLEVEL],thissize * sizeof(double)))==NULL
    || (dz->parray[ENV_CREATE_INTIME] = (double *)realloc(dz->parray[ENV_CREATE_INTIME],thissize * sizeof(double)))==NULL
    || (dz->iparray[ENV_SLOPETYPE]    = (int *)realloc(dz->iparray[ENV_SLOPETYPE],thissize * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to reallocate envelope arrays.\n");
        return(MEMORY_ERROR);
    }
    return(FINISHED);
}

/********************************************************************************************/
/********************************** FORMERLY IN preprocess.c ********************************/
/********************************************************************************************/

/****************************** PARAM_PREPROCESS *********************************/

int param_preprocess(dataptr dz)    
{
//  int exit_status = FINISHED;

    switch(dz->process) {
    case(ENV_CREATE):       case(ENV_EXTRACT):      case(ENV_IMPOSE):    
    case(ENV_REPLACE):      case(ENV_ENVTOBRK):     case(ENV_ENVTODBBRK):
    case(ENV_BRKTOENV):     case(ENV_DBBRKTOENV):   case(ENV_DBBRKTOBRK):   
    case(ENV_BRKTODBBRK):   case(ENV_WARPING):      case(ENV_RESHAPING):    
    case(ENV_REPLOTTING):   case(ENV_DOVETAILING):  case(ENV_CURTAILING):
    case(ENV_SWELL):        case(ENV_ATTACK):       case(ENV_PLUCK):    
//TW + NEW CASES
    case(ENV_TREMOL):       case(ENV_PROPOR):       case(TIME_GRID):
        return envel_preprocess(dz);
    case(ENVSYN):
        break;
    default:
        sprintf(errstr,"PROGRAMMING PROBLEM: Unknown process in param_preprocess()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);   /* NOTREACHED */
}


/********************************************************************************************/
/********************************** FORMERLY IN procgrou.c **********************************/
/********************************************************************************************/

/**************************** GROUCHO_PROCESS_FILE ****************************/

int groucho_process_file(dataptr dz)   /* FUNCTIONS FOUND IN PROCESS.C */
{   
    int exit_status = FINISHED;

    switch(dz->process) {
    case(ENV_EXTRACT):      case(ENV_IMPOSE):       case(ENV_REPLACE):
    case(ENV_WARPING):      case(ENV_RESHAPING):    case(ENV_REPLOTTING):
    case(ENV_DOVETAILING):  case(ENV_CURTAILING):   case(ENV_SWELL):        
    case(ENV_ATTACK):       case(ENV_PLUCK):        case(ENV_TREMOL):
//TW NEW CASES
    case(ENV_PROPOR):       case(TIME_GRID):
        display_virtual_time(0L,dz);
        break;
    }
    switch(dz->process) {
    case(ENV_CREATE):       case(ENV_EXTRACT):      case(ENV_IMPOSE):     case(ENV_REPLACE):
    case(ENV_ENVTOBRK):     case(ENV_ENVTODBBRK):
    case(ENV_BRKTOENV):     case(ENV_DBBRKTOENV):   case(ENV_DBBRKTOBRK): case(ENV_BRKTODBBRK):
    case(ENV_WARPING):      case(ENV_RESHAPING):    case(ENV_REPLOTTING):
    case(ENV_DOVETAILING):  case(ENV_CURTAILING):
    case(ENV_SWELL):        case(ENV_ATTACK):       case(ENV_PLUCK):      case(ENV_TREMOL):
//TW NEW CASE
    case(ENV_PROPOR):
        if((exit_status = process_envelope(dz))<0)
            return(exit_status);
        break;
//TW NEW CASE
    case(TIME_GRID):
        if((exit_status = do_grids(dz))<0)
            return(exit_status);
        break;
    case(ENVSYN):
        if((exit_status = envsyn(dz))<0)
            return(exit_status);
        break;
    default:
        sprintf(errstr,"Unknown case in process_file()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
}

/********************************************************************************************/
/********************************** FORMERLY IN pconsistency.c ******************************/
/********************************************************************************************/

/****************************** CHECK_PARAM_VALIDITY_AND_CONSISTENCY *********************************/

int check_param_validity_and_consistency(dataptr dz)
{
    int exit_status = FINISHED, chans = 0;
    int srate = 0;
//TW UPDATE
//  handle_pitch_zeros(dz);
    switch(dz->process) {
    case(ENV_BRKTOENV):     
    case(ENV_DBBRKTOENV):   
        if((exit_status = setup_envel_windowsize(dz))<0)
            return(exit_status);
        dz->infile->srate = round(SECS_TO_MS/dz->outfile->window_size);
        dz->infile->channels = 1;
        return create_sized_outfile(dz->outfilename,dz);
    case(ENV_CREATE):
        if((exit_status = setup_envel_windowsize(dz))<0)
            return(exit_status);
        if(dz->mode == ENV_ENVFILE_OUT) {
            dz->infile->srate = round(SECS_TO_MS/dz->outfile->window_size);
            dz->infile->channels = 1;
        }
        return create_sized_outfile(dz->wordstor[0],dz);
    case(ENV_EXTRACT):      
        if((exit_status = setup_envel_windowsize(dz))<0)
            return(exit_status);
        if(dz->mode == ENV_ENVFILE_OUT) {
            chans = dz->infile->channels;
            srate = dz->infile->srate;
            dz->infile->channels = 1;
            dz->infile->srate    = round(SECS_TO_MS/dz->outfile->window_size);
        }
        if((exit_status = create_sized_outfile(dz->wordstor[0],dz))<0)
            return(exit_status);
        if(dz->mode == ENV_ENVFILE_OUT) {
            dz->infile->channels = chans;
            dz->infile->srate    = srate;
        }
        break;
//TW + NEW CASE
    case(ENV_IMPOSE):       case(ENV_REPLACE):      case(ENV_PROPOR):
    case(ENV_ENVTOBRK):     case(ENV_ENVTODBBRK):   case(ENV_DBBRKTOBRK):   
    case(ENV_BRKTODBBRK):   case(ENV_WARPING):      case(ENV_RESHAPING):    
    case(ENV_REPLOTTING):   case(ENV_DOVETAILING):  case(ENV_CURTAILING):   
    case(ENV_SWELL):        case(ENV_ATTACK):
        return setup_envel_windowsize(dz);
    case(ENVSYN):
        if((exit_status = setup_envel_windowsize(dz))<0)
            return(exit_status);
        dz->infile->srate = round((double)SECS_TO_MS/(double)dz->outfile->window_size);
        dz->infile->channels = 1;
        if((exit_status = create_sized_outfile(dz->wordstor[0],dz))<0)
            return(exit_status);
        break;
    }
    return(FINISHED);
}

/************************* SETUP_ENVEL_WINDOWSIZE *********************
 *
 * This operation performed here because ENV_SAMP_WSIZE needed for buffer_definition.
 */

int setup_envel_windowsize(dataptr dz)
{
    int exit_status;
    switch(dz->process) {
    case(ENV_DBBRKTOBRK):
    case(ENV_BRKTODBBRK):
        break;
    case(ENV_CREATE):
        if(dz->mode==ENV_ENVFILE_OUT)
            dz->outfile->window_size = (float)dz->param[ENV_WSIZE];     /* 0 buffers */
        break;
    case(ENV_REPLOTTING):
    case(ENVSYN):
        dz->outfile->window_size = (float)dz->param[ENV_WSIZE];     /* 0 buffers */ 
        break;
    case(ENV_BRKTOENV):                                         /* 0 buffers */
    case(ENV_DBBRKTOENV):
        dz->outfile->window_size = (float)dz->param[ENV_WSIZE];
        break;
    case(ENV_ENVTOBRK):                                         /* 0 buffers */
    case(ENV_ENVTODBBRK):
        inject_dbl_parameter(ENV_WSIZE,dz);
        dz->param[ENV_WSIZE] = (double)dz->infile->window_size;         
        break;
    case(ENV_RESHAPING):                                        /* 0 buffers */
        inject_dbl_parameter(ENV_WSIZE,dz);
        dz->param[ENV_WSIZE] = (double)dz->infile->window_size;
        if((exit_status = generate_samp_windowsize(dz->infile,dz)) < 0)
            return(exit_status);
        dz->outfile->window_size = dz->infile->window_size;
        break;
    case(ENV_EXTRACT):
        if((exit_status = generate_samp_windowsize(dz->infile,dz)) < 0)
            return(exit_status);
        dz->outfile->window_size = 
            (float)get_outfile_wsize_in_msecs((int)dz->iparam[ENV_SAMP_WSIZE],dz);
        break;
    case(ENV_WARPING):
        if((exit_status = generate_samp_windowsize(dz->infile,dz))<0)
            return(exit_status);
        dz->outfile->window_size = 
            (float)get_outfile_wsize_in_msecs((int)dz->iparam[ENV_SAMP_WSIZE],dz);
        break;
//TW NEW CASE
    case(ENV_PROPOR):
        if(dz->extrabrkno < 0) {
            sprintf(errstr,"extrabrkno not established: setup_envel_windowsize()\n");
            return(PROGRAM_ERROR);
        }
        if(dz->brksize[dz->extrabrkno] < 2) {
            sprintf(errstr,"Brktable must have at least 2 value-pairs.\n");
            return(DATA_ERROR);
        }
        if((exit_status = scale_envelope_data(dz))<0)
            return(exit_status);
        dz->iparam[ENV_SAMP_WSIZE] = 0; /* forces default size onto buffers for */
        break;                          /* non-standard brkpnt-applied envelling */

    case(ENV_IMPOSE):
        switch(dz->mode) {
        case(ENV_ENVFILE_IN):
            inject_dbl_parameter(ENV_WSIZE,dz);         
            dz->param[ENV_WSIZE] = (double)dz->otherfile->window_size;  /* wsize is derived from 2nd input file */
            return generate_samp_windowsize(dz->infile,dz);
        case(ENV_SNDFILE_IN):/* wsize is a user input parameter. Apply it to 'otherfile' as that ENV extracted 1st, */ 
            return generate_samp_windowsize(dz->otherfile,dz); /* while 2nd file may have different srate or chans. */  
        case(ENV_BRKFILE_IN):
        case(ENV_DB_BRKFILE_IN):
            if(dz->extrabrkno < 0) {
                sprintf(errstr,"extrabrkno not established: setup_envel_windowsize()\n");
                return(PROGRAM_ERROR);
            }
            if(dz->brksize[dz->extrabrkno] < 2) {
                sprintf(errstr,"Brktable must have at least 2 value-pairs.\n");
                return(DATA_ERROR);
            }
            if((exit_status = setup_endofsndfile_value_if_needed(dz))<0)
                return(exit_status);
            dz->iparam[ENV_SAMP_WSIZE] = 0; /* forces default size onto buffers for */
            break;                          /* non-standard brkpnt-applied envelling */
        default:
            sprintf(errstr,"Unknown case for ENV_IMPOSE in setup_envel_windowsize()\n");
            return(PROGRAM_ERROR);
        }
        break;                                          
    case(ENV_REPLACE):
        switch(dz->mode) {
        case(ENV_ENVFILE_IN):
            inject_dbl_parameter(ENV_WSIZE,dz);         
            dz->param[ENV_WSIZE] = (double)dz->otherfile->window_size;  /* wsize is derived from 2nd input file */
            return generate_samp_windowsize(dz->infile,dz);
        case(ENV_SNDFILE_IN):  /* wsize is a user input parameter: Apply to 'otherfile' as that ENV extracted 1st, */
            return generate_samp_windowsize(dz->otherfile,dz); /* while 2nd file may have different srate or chans */   
        case(ENV_BRKFILE_IN):
        case(ENV_DB_BRKFILE_IN):
            if(dz->extrabrkno < 0) {
                sprintf(errstr,"extrabrkno not established: setup_envel_windowsize()\n");
                return(PROGRAM_ERROR);
            }
//DELETED AUGUST 2005
//          inject_dbl_parameter(ENV_WSIZE,dz);                 /* envelling done in standard way */
//          dz->param[ENV_WSIZE] = ENV_DEFAULT_WSIZE;           /* wsize is set to a default value */
            return generate_samp_windowsize(dz->infile,dz);
        default:
            sprintf(errstr,"Unknown case for ENV_REPLACE in setup_envel_windowsize()\n");
            return(PROGRAM_ERROR);
        }                                                       
        break;                                          
    case(ENV_CURTAILING):
    case(ENV_DOVETAILING):
    case(ENV_SWELL):
    case(ENV_ATTACK):
        dz->iparam[ENV_WSIZE] = 0;      /* forces default size onto buffers */
        break;
    default:
        sprintf(errstr,"Unknown case in setup_envel_windowsize()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
}

/**************************** GENERATE_SAMP_WINDOWSIZE [GENERATE_BLOK] ************************/

int generate_samp_windowsize(fileptr thisfile,dataptr dz)
{
    int unadjusted_envwindow_sampsize, j, k, chansecsize;
    int  channels = thisfile->channels;
    int srate = thisfile->srate;

    chansecsize = (int)(ENV_FSECSIZE * channels);

    unadjusted_envwindow_sampsize = round(dz->param[ENV_WSIZE] * MS_TO_SECS * (double)srate) * channels;
    if(unadjusted_envwindow_sampsize < chansecsize) {
        k = chansecsize;
        while(unadjusted_envwindow_sampsize<k)
            k /= 2;
        j = k * 2;
        if(j - unadjusted_envwindow_sampsize > unadjusted_envwindow_sampsize - k)
            dz->iparam[ENV_SAMP_WSIZE] = (int)k;
        else
            dz->iparam[ENV_SAMP_WSIZE] = (int)j;
    } else if(unadjusted_envwindow_sampsize >= chansecsize) {
        k = round((double)unadjusted_envwindow_sampsize/(double)chansecsize);
        dz->iparam[ENV_SAMP_WSIZE] = (int)(chansecsize * k);
    }
    return(FINISHED);
}

/************************* GET_OUTFILE_WSIZE_IN_MSECS *********************/

double get_outfile_wsize_in_msecs(int envwindow_sampsize,dataptr dz)
{
    double size = (double)(envwindow_sampsize/dz->infile->channels)/(double)dz->infile->srate;
    size *= SECS_TO_MS;
    return(size);
}

/******************************** INJECT_DBL_PARAMETER *************************/

void inject_dbl_parameter(int paramno,dataptr dz)
{
    dz->is_active[paramno] = (char)1; 
    dz->is_int[paramno]    = (char)0;
    dz->no_brk[paramno]    = (char)1;
}

/***************************** SETUP_ENDOFSNDFILE_VALUE_IF_NEEDED **************************
 *
 *  If envtable longer than sound, curtail to length of sndfile.
 *  Prevents ridiculously long interpolation vals occuring at end of envfile.
 */

int setup_endofsndfile_value_if_needed(dataptr dz)
{
    int paramno = dz->extrabrkno;
    double lasttime, lastval, nexttime, nextval, timediff, valdiff, timeratio, endval;
    double endtime;
    double infiledur = (double)(dz->insams[0]/dz->infile->channels)/(double)(dz->infile->srate);
    double *startaddr = dz->brk[paramno];
    double *endaddr   = dz->brk[paramno] + ((dz->brksize[paramno] - 1) * 2);
    if(*startaddr >= infiledur - FLTERR) {
        sprintf(errstr,"Envelope starts effectively beyond end of sndfile: can't proceed.\n");
        return(DATA_ERROR);
    }
    if((endtime = *endaddr) > infiledur + FLTERR) {
        while(endaddr > startaddr) {
            endtime = *endaddr;
            if(endtime > infiledur+FLTERR)
                endaddr -= 2;
            else
                break;
        }
        lasttime  = *endaddr;
        lastval   = *(endaddr + 1);
        endaddr  += 2;
        nexttime  = *endaddr;
        nextval   = *(endaddr + 1);
        timediff  = nexttime - lasttime;
        timeratio = (infiledur - lasttime)/timediff;
        valdiff   = nextval - lastval;
        endval    = (valdiff * timeratio) + lastval;
        *endaddr  = infiledur;
        *(endaddr+1) = endval;
        dz->brksize[paramno] = ((endaddr - startaddr)/2) + 1;
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
    case(ENV_DOVETAILING):  case(ENV_CURTAILING):   case(ENV_EXTRACT):      
    case(ENV_WARPING):      case(ENV_IMPOSE):       case(ENV_REPLACE):
    case(ENV_SWELL):        case(ENV_ATTACK):
//TW NEW CASE
    case(ENV_PROPOR):
        return create_sndbufs_for_envel(dz);

    case(ENV_PLUCK):        
        return create_pluck_buffers(dz);
    case(ENV_TREMOL):       
//TW NEW CASE
    case(TIME_GRID):        
        return create_sndbufs(dz);

    case(ENV_ENVTOBRK):     case(ENV_ENVTODBBRK):   case(ENV_BRKTOENV):     
    case(ENV_DBBRKTOENV):   case(ENV_DBBRKTOBRK):   case(ENV_BRKTODBBRK):
    case(ENV_CREATE):       case(ENV_RESHAPING):    case(ENV_REPLOTTING):   
        return(FINISHED);
    case(ENVSYN):
        return create_envsyn_buf(dz);
    default:
        sprintf(errstr,"Unknown program no. in allocate_large_buffers()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);   /* NOTREACHED */
}

/*************************** CREATE_SNDBUFS_FOR_ENVEL **************************
 *
 * 1)   some processes use NO sndfile-buffers: ENV_CREATE: ENV_REPLOTTING: ENV_CONVERTBRK: ENV_CONVERTENV 
 * 2)   some processes have no ENV_SAMP_WSIZE: default set to ZERO, DOVETAILING: CURTAILING. 
 * 3)   Also, ENV_SAMP_WSIZE may be LESS than SECSIZE, but if so,
 *      will always be a simple fraction thereof (see generate_samp_windowsize()).
 */

int create_sndbufs_for_envel(dataptr dz)
{
    size_t bigbufsize;
    int n;
    int bufactor;
    if(dz->bufcnt==0)                       /* Processes with no soundfiles */
        return(FINISHED);   

    if(dz->iparam[ENV_SAMP_WSIZE]==0) {     /* Processes which apply brkfiles directly to snd */
        if(dz->process != ENV_CURTAILING 
        && dz->process != ENV_DOVETAILING 
        && dz->process != ENV_SWELL
        && dz->process != ENV_ATTACK
//TW NEW CASE
        && dz->process != ENV_PROPOR
        && !(dz->process == ENV_IMPOSE && (dz->mode == ENV_BRKFILE_IN || dz->mode == ENV_DB_BRKFILE_IN))) {
            sprintf(errstr,"Error in setting ENV_SAMP_WSIZE prior to create_sndbufs_for_envel()\n");
            return(PROGRAM_ERROR);
        }
        if(dz->bufcnt!=3) {
            sprintf(errstr,"Insufficient pointers: create_sndbufs_for_envel()\n");
            return(PROGRAM_ERROR);
        }
        return do_special_brkpnt_envelling_buffer(dz);
    }

    /* All other cases */
    bufactor = max((int)dz->iparam[ENV_SAMP_WSIZE],ENV_FSECSIZE);   /* 2 */ /* 3 */

    if(dz->sbufptr == 0 || dz->sampbuf == 0) {
        sprintf(errstr,"buffer pointers not allocated: create_sndbufs_for_envel()\n");
        return(PROGRAM_ERROR);
    }
    bigbufsize = (size_t) Malloc(-1);
    bigbufsize /= dz->bufcnt;

    dz->buflen = (int)(bigbufsize/sizeof(float));

    if((dz->buflen  = (dz->buflen/bufactor) * bufactor)<=0) {
        dz->buflen  = bufactor;
    }

    bigbufsize = dz->buflen * sizeof(float);

    if((dz->bigbuf = (float *)malloc((bigbufsize * dz->bufcnt))) == NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
        return(MEMORY_ERROR);
    }
    for(n=0;n<dz->bufcnt;n++)
        dz->sbufptr[n] = dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
    dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
    return(FINISHED);
}

/*************************** DO_SPECIAL_BRKPNT_ENVELLING_BUFFER **************************/

int do_special_brkpnt_envelling_buffer(dataptr dz)
{
    size_t bigbufsize;
    size_t fsecsizebytes = ENV_FSECSIZE * sizeof(float);
    size_t extended_buffer_size = (size_t) Malloc(-1);
    
    extended_buffer_size = (extended_buffer_size/fsecsizebytes) * fsecsizebytes;
    bigbufsize = extended_buffer_size - fsecsizebytes;  /* overflow sector */
    dz->buflen = (int)(bigbufsize/sizeof(float));
    if((dz->bigbuf = (float *)malloc(extended_buffer_size)) == NULL) {
        sprintf(errstr, "INSUFFICIENT MEMORY to create sound buffer.\n");
        return(MEMORY_ERROR);
    }
    dz->sbufptr[0] = dz->sampbuf[0] = dz->bigbuf;           /* INITIAL POSITION OF buffer start */
    return(FINISHED);
}

/*************************** CREATE_PLUCK_BUFFERS ***************************/

int create_pluck_buffers(dataptr dz)
{   
    size_t bigbufsize;
    int sec_cnt=0, extrasamps;
    int obuf_offset, ibuf_offset;
    int pluklen = dz->iparam[ENV_PLK_WAVELEN] * dz->iparam[ENV_PLK_CYCLEN];
    int shsecsize = ENV_FSECSIZE;
    int fsecsizebytes = ENV_FSECSIZE * sizeof(float);

    if((extrasamps = max(0,pluklen - dz->iparam[ENV_PLK_ENDSAMP])) > 0) {
        if(((sec_cnt = extrasamps/ENV_FSECSIZE) * ENV_FSECSIZE)!=extrasamps)
            sec_cnt++;
        extrasamps = sec_cnt * ENV_FSECSIZE;
    } 
    bigbufsize = (size_t)Malloc(-1);
    bigbufsize = (bigbufsize/fsecsizebytes) * fsecsizebytes;

    /*RWD*/
    dz->buflen = (int)(bigbufsize / sizeof(float));
    if(dz->buflen <= 0) {
        sprintf(errstr,"INSUFFICIENT MEMORY for sound buffers.\n");
        return(MEMORY_ERROR);
    }
    dz->buflen += extrasamps;

    if((dz->bigbuf = (float *)Malloc(dz->buflen * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
        return(MEMORY_ERROR);
    }
    dz->sampbuf[PLK_BUFEND] = dz->bigbuf + dz->buflen;

    dz->sampbuf[PLK_INITBUF] = dz->bigbuf + extrasamps;
    if((dz->sampbuf[PLK_PLUKEND] = dz->sampbuf[PLK_INITBUF] + dz->iparam[ENV_PLK_ENDSAMP]) >dz->sampbuf[PLK_BUFEND]) {
        sprintf(errstr,"INSUFFICIENT MEMORY: Pluck too long for buffers.\n");
        return(MEMORY_ERROR);
    }   
    if((dz->sampbuf[PLK_OUTBUF]  = dz->sampbuf[PLK_PLUKEND] - pluklen) < dz->bigbuf) {
        sprintf(errstr,"Problem in buffer arithmetic: create_pluck_buffers(): 1\n");
        return(PROGRAM_ERROR);
    } 

    obuf_offset = (dz->sampbuf[PLK_OUTBUF] - dz->bigbuf);

    if(((sec_cnt = obuf_offset/ENV_FSECSIZE) * ENV_FSECSIZE)!=obuf_offset)
        sec_cnt++;
    ibuf_offset = sec_cnt * ENV_FSECSIZE;
    dz->sampbuf[PLK_INBUF] = dz->bigbuf + (ibuf_offset);

    sec_cnt = (dz->sampbuf[PLK_BUFEND] - dz->sampbuf[PLK_OUTBUF]) / shsecsize;
    dz->iparam[ENV_PLK_OBUFLEN] = sec_cnt * shsecsize;

    if(dz->sampbuf[PLK_INBUF] + dz->iparam[ENV_PLK_OBUFLEN] != dz->sampbuf[PLK_BUFEND]) {
        sprintf(errstr,"Problem in buffer arithmetic: create_pluck_buffers(): 2\n");
        return(PROGRAM_ERROR);
    } 

    if((dz->iparam[ENV_PLK_OWRAPLEN]  = dz->sampbuf[PLK_INBUF] - dz->sampbuf[PLK_OUTBUF])<0) {
        sprintf(errstr,"Problem in buffer arithmetic: create_pluck_buffers(): 3\n");
        return(PROGRAM_ERROR);
    } 
//TW : THIS IS CORRECT: original code was incorrect
    if(dz->iparam[ENV_PLK_OWRAPLEN]  >= ENV_FSECSIZE) {
        sprintf(errstr,"Problem in buffer arithmetic: create_pluck_buffers(): 4\n");
        return(PROGRAM_ERROR);
    } 

    dz->sampbuf[PLK_OBUFWRAP] = dz->sampbuf[PLK_OUTBUF] + dz->iparam[ENV_PLK_OBUFLEN];

    if(dz->sampbuf[PLK_BUFEND] - dz->sampbuf[PLK_OBUFWRAP] != dz->iparam[ENV_PLK_OWRAPLEN]) {
        sprintf(errstr,"Problem in buffer arithmetic: create_pluck_buffers(): 5\n");
        return(PROGRAM_ERROR);
    } 
    memset((char *)dz->bigbuf,0,dz->buflen * sizeof(float));
    return(FINISHED);
}

/********************************************************************************************/
/********************************** FORMERLY IN cmdline.c ***********************************/
/********************************************************************************************/

int get_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
    if     (!strcmp(prog_identifier_from_cmdline,"create"))         dz->process = ENV_CREATE;
    else if(!strcmp(prog_identifier_from_cmdline,"extract"))        dz->process = ENV_EXTRACT;
    else if(!strcmp(prog_identifier_from_cmdline,"impose"))         dz->process = ENV_IMPOSE;
    else if(!strcmp(prog_identifier_from_cmdline,"replace"))        dz->process = ENV_REPLACE;
    else if(!strcmp(prog_identifier_from_cmdline,"brktoenv"))       dz->process = ENV_BRKTOENV;
    else if(!strcmp(prog_identifier_from_cmdline,"envtobrk"))       dz->process = ENV_ENVTOBRK;
    else if(!strcmp(prog_identifier_from_cmdline,"envtodb"))        dz->process = ENV_ENVTODBBRK;
    else if(!strcmp(prog_identifier_from_cmdline,"warp"))           dz->process = ENV_WARPING;
    else if(!strcmp(prog_identifier_from_cmdline,"reshape"))        dz->process = ENV_RESHAPING;
    else if(!strcmp(prog_identifier_from_cmdline,"replot"))         dz->process = ENV_REPLOTTING;
    else if(!strcmp(prog_identifier_from_cmdline,"dovetail"))       dz->process = ENV_DOVETAILING;
    else if(!strcmp(prog_identifier_from_cmdline,"curtail"))        dz->process = ENV_CURTAILING;
    else if(!strcmp(prog_identifier_from_cmdline,"dbtoenv"))        dz->process = ENV_DBBRKTOENV;
    else if(!strcmp(prog_identifier_from_cmdline,"dbtogain"))       dz->process = ENV_DBBRKTOBRK;
    else if(!strcmp(prog_identifier_from_cmdline,"gaintodb"))       dz->process = ENV_BRKTODBBRK;
    else if(!strcmp(prog_identifier_from_cmdline,"swell"))          dz->process = ENV_SWELL;
    else if(!strcmp(prog_identifier_from_cmdline,"attack"))         dz->process = ENV_ATTACK;
    else if(!strcmp(prog_identifier_from_cmdline,"pluck"))          dz->process = ENV_PLUCK;
    else if(!strcmp(prog_identifier_from_cmdline,"tremolo"))        dz->process = ENV_TREMOL;
//TW NEW CASES
    else if(!strcmp(prog_identifier_from_cmdline,"scaled"))         dz->process = ENV_PROPOR;
    else if(!strcmp(prog_identifier_from_cmdline,"timegrid"))       dz->process = TIME_GRID;
    else if(!strcmp(prog_identifier_from_cmdline,"cyclic"))         dz->process = ENVSYN;
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
    fprintf(stdout,
    "\nUSAGE: envel NAME (mode) (infile) (infile2) outfile parameters:\n"
    "\n"
    "where NAME can be any one of\n"
    "\n"
    "    ENVELOPE GENERATION OPERATIONS\n"
    "create      cyclic\n"
    "\n"
    "    ENVELOPE OPERATION ON A SNDFILE\n"
    "extract     impose      replace      warp\n"
    "tremolo     swell       attack       pluck\n"
//TW NEW CASES
    "dovetail    curtail     scaled       timegrid\n"
    "\n"
    "    ENVELOPE OPERATION ON AN ENVELOPE-FILE\n"
    "reshape\n"
    "envtobrk    envtodb\n"
    "\n"
    "    ENVELOPE OPERATION ON A BRKPNT (TEXT) FILE\n\n"
    "replot\n"
    "dbtogain    gaintodb\n"
    "brktoenv    dbtoenv\n"
    "\n"
    "Type 'envel warp' for more info on envel warp.. ETC.\n");
    return(USAGE_ONLY);
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
    if(!strcmp(str,"create"))   {       
        fprintf(stdout,
        "CREATE AN ENVELOPE.\n\n"
        "USAGE: envel create 1 envfile createfile  wsize\n"
        "OR:    envel create 2 brkfile createfile\n\n"
        "MODES..\n" 
        "1) creates a BINARY envelope file:\n"
        "   If you specify starttime > 0,vals from 0 to starttime hold your startlevel.\n"
        "2) creates a (TEXT) BRKPNT file:   File starts at time you specify.\n\n"
        "WSIZE      window_size (in MS) of envelope to be created.\n"    
        "           Range (%.0lf - filelen)\n"   
        "CREATEFILE is a textfile with the following format:\n"
        "           time [e]level time [e]level ......\n"
        "where time-level pairs can be repeated as often as desired.\n"
        "Level is a number between 0 and 1, or a dB value between %.0lfdB and 0dB\n"
        "(you must write 'dB' if you want dB).\n"
        "If preceded by an 'e', envelope rises[falls] exponentially to that level.\n"
        "Default is linear rise[fall].\n\n"
        "Times are in seconds, must increase through the file,\n"
        "and be separated by at least %.3lf secs\n",
        ENV_MIN_WSIZE,MIN_DB_ON_16_BIT,2.0 * ENV_MIN_WSIZE * MS_TO_SECS);
    } else if(!strcmp(str,"extract"))   {       
        fprintf(stdout,
        "EXTRACT ENVELOPE FROM AN INPUT SOUNDFILE.\n\n"
        "USAGE: envel extract  1  infile outenvfile  wsize\n"
        "OR:    envel extract  2  infile outbrkfile  wsize  [-ddatareduce]\n\n"
        "MODE 1 extracts a binary envelope file:\n"
        "MODE 2 extracts a (text) brkpnt file.\n\n"
        "WSIZE      window_size (in MS) for scanning envelope: Range (%.0lf - filelen)\n"
        "DATAREDUCE variable determines quantity v. accuracy of data written to brkfile\n"
        "           Range(0 - 1)\n",ENV_MIN_WSIZE);
    } else if(!strcmp(str,"impose"))    {       
        fprintf(stdout,
        "IMPOSE AN ENVELOPE ON AN INPUT SOUNDFILE.\n\n"
        "USAGE: envel impose  1  input_sndfile imposed-sndfile    outsndfile    wsize\n"
        "USAGE: envel impose  2  input_sndfile imposed-envfile    outsndfile\n"
        "USAGE: envel impose  3  input_sndfile imposed-brkfile    outsndfile\n"
        "USAGE: envel impose  4  input_sndfile imposed-brkfile-dB outsndfile\n\n"
        "MODE 1 imposes an envelope extracted from another sndfile.\n"
        "MODE 2 imposes an envelope from a binary envelope file.\n"
        "MODE 3 imposes an envelope from a (text) brkpnt file: val range (0 - 1).\n"
        "MODE 4 imposes an envelope from a (text) brkpnt file with dB vals (%.0lf to 0).\n\n"
        "WSIZE      window_size (in MS) for scanning envelope: Range (%.0lf - filelen)\n\n"
        "In MODES 1 & 2, the whole sndfile is enveloped.\n"
        "In MODES 3 & 4, brkpnt may start (and end) at any time in file,\n"
        "                effectively editing it. Must have at least 2 brkpnt pairs.\n",
        MIN_DB_ON_16_BIT,ENV_MIN_WSIZE);
    } else if(!strcmp(str,"replace"))   {       
        fprintf(stdout,
        "REPLACE THE EXISTING ENVELOPE OF AN INPUT SOUNDFILE\n"
        "            WITH A DIFFERENT ENVELOPE.\n\n"
        "USAGE: envel replace 1 input_sndfile replacing-sndfile    outsndfile wsize\n"
        "USAGE: envel replace 2 input_sndfile replacing-envfile    outsndfile\n"
        "USAGE: envel replace 3 input_sndfile replacing-brkfile    outsndfile\n"
        "USAGE: envel replace 4 input_sndfile replacing-brkfile-dB outsndfile\n\n"
        "MODES:\n"
        "1 replaces envelope with new one extracted from another sndfile.\n"
        "2 replaces envelope with new one from a binary envelope file.\n"
        "3 replaces envelope with new one from (text) brkpnt file: valrange 0-1.\n"
        "4 replaces envelope with new one from (text) brkpnt file in dB (-96 to 0).\n\n"
        "In all cases, the entire sndfile is enveloped\n\n"
        "WSIZE      window_size (in MS) for scanning envelope: Range (%.0lf - filelen)\n\n"
        "MODE 1 is especially useful for restoring the amplitude contour of a sound\n"
        "after filtering with time-varying Q-value.\n",ENV_MIN_WSIZE);
    } else if(!strcmp(str,"brktoenv"))  {       
        fprintf(stdout,
        "CONVERT A (TEXT) BRKPNT ENVELOPE TO A BINARY ENVELOPE FILE.\n\n"
        "USAGE: envel brktoenv inbrkfile outenvfile  wsize\n\n"
        "WSIZE      window_size (in MS) for scanning envelope: Range (%.0lf - filelen)\n\n"
        "If brkpnt starttime > zero, new envelope will start from zero,\n"
        "holding the brktabel startval as far as first brktable time.\n",ENV_MIN_WSIZE);
    } else if(!strcmp(str,"envtobrk"))  {       
        fprintf(stdout,
        "CONVERT A BINARY ENVELOPE FILE TO A (TEXT) BRKPNT ENVELOPE.\n\n"
        "USAGE: envel envtobrk inenvfile outbrkfile  [-ddatareduce]\n\n"
        "DATAREDUCE variable determines quantity v. accuracy of data written to brkfile.\n"
        "           Range(0 - 1)\n");
    } else if(!strcmp(str,"envtodb"))   {       
        fprintf(stdout,
        "CONVERT A BINARY ENVELOPE FILE TO A (TEXT) BRKPNT ENVELOPE WITH dB VALUES.\n\n"
        "USAGE: envel envtodb inenvfile outbrkfile  [-ddatareduce]\n\n"
        "DATAREDUCE variable determines quantity v. accuracy of data written to brkfile.\n"
        "           Range(0 - 1)\n");
    } else if(!strcmp(str,"dovetail")){     
        fprintf(stdout,
            "DOVETAIL SNDFILE BY ENVELOPING THE START AND END OF IT.\n\n"
            "USAGE:\nenvel dovetail 1 infile outfile infadedur outfadedur intype outtype [-ttimes]\n"
            "OR:   \nenvel dovetail 2 infile outfile infadedur outfadedur [-ttimes]\n\n"
            "In mode 2, the dovetail slopes are doubly exponential (steeper).\n\n"
            "INFADE-DUR  is duration of start-of-file fade-in.\n"
            "OUTFADE-DUR is duration of end-of-file fade-out.\n"
            "            Infade-dur and Outfade-dur must not overlap each another.\n"
//TW UPDATES
            "INTYPE      0 for linear fade, 1 (default) for exponential fade, at start.\n"
            "OUTTYPE     0 for linear fade, 1 (default) for exponential fade, at end.\n"
            "-t          times for fade durations are given in the units....\n"
            "            seconds (-t0) samples (-t1), or grouped-samples (-t2)\n");
    } else if(!strcmp(str,"curtail")){      
         fprintf(stdout,
            "CURTAIL SNDFILE BY FADING TO ZERO AT SOME TIME WITHIN IT.\n\n"
            "USAGE: envel curtail 1 sndfile outfile fadestart fadeend  envtype [-ttimes]\n"
            "OR:    envel curtail 2 sndfile outfile fadestart fade-dur envtype [-ttimes]\n"
            "OR:    envel curtail 3 sndfile outfile fadestart          envtype [-ttimes]\n\n"
            "OR:    envel curtail 4 sndfile outfile fadestart fadeend          [-ttimes]\n"
            "OR:    envel curtail 5 sndfile outfile fadestart fade-dur         [-ttimes]\n"
            "OR:    envel curtail 6 sndfile outfile fadestart                  [-ttimes]\n\n"
            "MODES 4-6 produce doubly exponential (steeper) editing slope.\n\n"
            "FADESTART is start-time of fade.\n"
            "FADEEND   is end-time of fade. (In MODE 3, assumed to be endtime of origsnd).\n"
            "FADE-DUR  is duration of fade-out.\n"
            "ENVTYPE   0 for linear fade, 1 (default) for exponential fade.\n"
            "-t        TIMES for fade start, end or duration are given in the units....\n"
            "          seconds (-t0) samples (-t1), or grouped-samples (-t2)\n");       
    } else if(!strcmp(str,"warp")) {        
        fprintf(stdout,
        "WARP THE ENVELOPE OF AN SOUNDFILE\n\n"
        "envel warp 1-12  sndfile outsndfile          wsize various_params\n"
        "envel warp 13    sndfile outsndfile rampfile wsize various_params\n"
        "envel warp 14-15 sndfile outsndfile          wsize various_params\n\n"
        "where MODE NUMBERS stand for are....\n"
        "1 NORMALISE       5 LIFT            9  INVERT       13 TRIGGER\n"  
        "2 REVERSE         6 TIMESTRETCH     10 LIMIT        14 CEILING\n"
        "3 EXAGGERATE      7 FLATTEN         11 CORRUGATE    15 DUCKED\n"
        "4 ATTENUATE       8 GATE            12 EXPAND \n\n"
        "WSIZE is duration of enveloping window, in MS.\n\n"
        "for more info on MODES, and their 'various_params', type e.g.:\n"
        "      'envel warp normalise' for more info on 'normalise' option... etc\n");
    } else if(!strcmp(str,"reshape")) {     
        fprintf(stdout,
        "WARP THE ENVELOPE IN A BINARY ENVELOPE FILE\n\n"
        "envel reshape 1-12  envfile outenvfile          various_params\n"
        "envel reshape 13    envfile outenvfile rampfile various_params\n"
        "envel reshape 14-15 envfile outenvfile          various_params\n\n"
        "where MODE NUMBERS stand for are....\n"
        "1 NORMALISE       5 LIFT            9  INVERT       13 TRIGGER\n"  
        "2 REVERSE         6 TIMESTRETCH     10 LIMIT        14 CEILING\n"     
        "3 EXAGGERATE      7 FLATTEN         11 CORRUGATE    15 DUCKED\n"
        "4 ATTENUATE       8 GATE            12 EXPAND \n\n"
        "for more info on MODES, and their 'various_params', type e.g.:\n"
        "      'envel reshape normalise' for more info on 'normalise' option... etc\n");
    } else if(!strcmp(str,"replot")) {      
        fprintf(stdout,
        "WARP THE ENVELOPE IN A (TEXT) BRKPNT ENVELOPE FILE\n\n"
        "envel replot 1-12  brkfile outbrkfile          wsize various_params [-dreduce]\n"
        "envel replot 13    brkfile outbrkfile rampfile wsize various_params [-dreduce]\n"
        "envel replot 14-15 brkfile outbrkfile          wsize various_params [-dreduce]\n\n"
        "where MODE NUMBERS stand for are....\n"
        "1 NORMALISE       5 LIFT            9  INVERT       13 TRIGGER\n"  
        "2 REVERSE         6 TIMESTRETCH     10 LIMIT        14 CEILING\n"
        "3 EXAGGERATE      7 FLATTEN         11 CORRUGATE    15 DUCKED\n"
        "4 ATTENUATE       8 GATE            12 EXPAND \n\n"
        "WSIZE   is duration of enveloping window, in MS.\n"
        "REDUCE  forces interpolation of data in brkpnt outfile\n"
        "        to reduce unnecessary data output: Range(0-1).\n\n"
        "for more info on MODES, and their 'various_params', type e.g.:\n"
        "      'envel replot normalise' for more info on 'normalise' option... etc\n"
        "NB: TRIGGER works better using reshape or warp.\n");
    } else if(!strcmp(str,"dbtoenv")) {     
        fprintf(stdout,
        "CONVERT A (TEXT) BRKPNT FILE WITH VALS IN dB TO AN ENVELOPE FILE\n\n"
        "envel dbtoenv db_brkfile outenvfile  wsize\n\n"
        "WSIZE is duration of enveloping window, in MS.\n");
    } else if(!strcmp(str,"dbtogain")) {        
        fprintf(stdout,
        "CONVERT A (TEXT) BRKPNT FILE WITH dB VALS TO GAIN VALS\n\n"
        "envel dbtogain db_brkfile outbrkfile\n");
    } else if(!strcmp(str,"gaintodb")) {        
        fprintf(stdout,
        "CONVERT A (TEXT) BRKPNT FILE WITH GAIN VALS TO dB VALS\n\n"
        "envel gaintodb brkfile out_db_brkfile\n");
    } else if(!strcmp(str,"swell")) {       
        fprintf(stdout,
        "CAUSE SOUND TO FADE IN TO AND OUT FROM A PEAK MOMENT\n\n"
        "envel swell infile outfile peaktime peaktype\n\n"
        "PEAKTYPE: 0 linear: 1 (default) exponential\n");
    } else if(!strcmp(str,"attack")) {      
        fprintf(stdout,
        "EMPHASIZE THE ATTACK OF A SOUND\n\n"
        "envel attack 1 infile outfile gate gain onset decay [-tenvtype]\n"
        "envel attack 2 infile outfile time gain onset decay [-tenvtype]\n"
        "envel attack 3 infile outfile time gain onset decay [-tenvtype]\n"
        "envel attack 4 infile outfile      gain onset decay [-tenvtype]\n\n"
        "MODES ARE:\n"
        "1) Set attack point where snd level first exceeds gate-level.\n"
        "2) attack point at max level around your approx-time (+- %.0lf MS)\n"
        "3) attack point at your exact-time.\n"
        "4) attack point at maxlevel in sndfile.\n\n"
        "GAIN:    Amplification of attack point\n"
        "GATE:    Level for attack point to be recognised: Range(0 - 1)\n"
        "TIME:    Time (approx or exact) of attack point,in secs.\n"
        "ONSET:   Attack onset duration (MS): Range(%.0lf to %.0lf)\n"
        "DECAY:   Attack decay duration (MS): Range(%.0lf to <infiledur)\n"
        "ENVTYPE: 0 linear: 1 exponential(default).\n",
        ENV_ATK_SRCH,ENV_MIN_ATK_ONSET,ENV_MAX_ATK_ONSET,ENV_MIN_ATK_ONSET);
    } else if(!strcmp(str,"pluck")) {       
        fprintf(stdout,
        "PLUCK START OF SOUND (MONO FILES ONLY)\n\n"
        "envel pluck infile outfile startsamp wavelen [-aatkcycles] [-ddecayrate]\n\n"
        "STARTSAMP  Sample, in src sound, at which pluck will END:\n"
        "           must be a sample AT A ZERO_CROSSING...\n"
        "WAVELEN    no of (absolute) samples in pluck wavelen: \n"
        "           should be same as src_signal's wavelen immediately after STARTSAMP\n"
        "ATKCYCLES  no of wavecycles in pluck-attack : Range(%.0lf - %.0lf) (Default %.0lf)\n"
        "DECAYRATE  rate of decay of the pluck_attack: Range(%.0lf - %.0lf) (Default %.0lf)\n",
        ENV_PLK_CYCLEN_MIN,ENV_PLK_CYCLEN_MAX,ENV_PLK_CYCLEN_DEFAULT,
        ENV_PLK_DECAY_MIN,ENV_PLK_DECAY_MAX,ENV_PLK_DECAY_DEFAULT);
    } else if(!strcmp(str,"tremolo")) {     
        fprintf(stdout,
        "TREMOLO A SOUND\n\n"
        "USAGE: envel tremolo mode infile outfile frq depth gain\n\n"
        "MODES:\n"
        "1) Interpolate linearly between frqs in any frq brktable (default).\n"
        "2) Interpolate logarithmically (like pitch). (Care with zero frqs).\n\n"
        "FRQ          frequency of tremolo (0 - %.0lf) \n"
        "DEPTH        depth of tremolo: Range(0 to 1: default %.3lf)\n"
        "GAIN         Overall signal gain, or envelope: Range(0 to 1: default 1)\n\n"
        "Frq, Depth and Gain may vary over time.\n",ENV_TREM_MAXFRQ,ENV_TREM_DEFAULT_DEPTH);
//TW NEW CASES
    } else if(!strcmp(str,"scaled")) {      
        fprintf(stdout,
        "IMPOSE AN ENVELOPE ON AN INPUT SOUNDFILE, SCALING IT TIMEWISE TO SOUND'S DURATION.\n\n"
        "USAGE: envel  scaled  input_sndfile  imposed-brkfile  outsndfile\n\n"
        "Must have at least 2 brkpnt pairs.\n");
    } else if(!strcmp(str,"timegrid")) {        
        fprintf(stdout,
        "PARTITION SOUNDFILE INTO STAGGERED GRIDS.\n"
        "EACH GRID IS SEQUENCE OF WINDOWS FROM SOURCE, AT THEIR ORIGINAL TIMES, SEPARATED BY SILENCE.\n\n"
        "USAGE: envel timegrid input_sndfile  generic_outsndfile_name gridcnt gridwidth splicelen\n\n"
        "GRIDCNT      Number of grids (and hence output files).\n"
        "GRIDWIDTH    Duration of Grid windows, in seconds.\n"
        "SPLICELEN    Splice length, in mS\n\n"
        "gridwidth and splicelen may vary over time.\n");
    } else if(!strcmp(str,"cyclic")) {      
        fprintf(stdout,
        "CREATE A SEQUENCE OF REPEATED ENVELOPES, IN A BINARY ENVELOPE FILE.\n\n"
        "USAGE: envel cyclic 1-3 outf       wsize total-dur cell-dur phase trough env-exp\n"
        "OR:    envel cyclic 4 outf userenv wsize total-dur cell-dur phase\n\n"
        "MODES:\n"
        "(1) Rising. (2) Falling. (3) Troughed (falls then rises)  (4) User-defined\n\n"
        "WSIZE       Envelope window size (ms) : Resolution of envelope generated.\n"
        "TOTAL-DUR   Duration of output file.\n"
        "CELL-DUR    Duration of individual (repeated) units: can vary over time.\n"
        "PHASE       Where in cell-envelope to begin output: 0=start, 1=end of cell.\n"
        "TROUGH      Lowest point of envelope cell (Range 0 -1): can vary over time.\n"
        "ENV-EXP     Shape of env: 1 linear: <1 steep at top, > 1 steep at bottom.\n\n"
        "USERENV     textfile of time/val(0-1) pairs defining env (time units arbitrary)\n"
        "            as the envelope is stretched to each unit duration.\n");
    } else
        fprintf(stdout,"Unknown option '%s'\n",str);

    return(USAGE_ONLY);
}

/******************************** USAGE3 ********************************/

int usage3(char *str1,char *str2)
{
    if((!strcmp(str1,"warp")) || (!strcmp(str1,"reshape")) || (!strcmp(str1,"replot"))) {
        if(!strcmp(str2,"normalise")) {
            fprintf(stdout,
            "NORMALISE envelope. Expand so that highest env point is max possible.\n\n"
            "No other extra parameters needed.\n");
        } else if(!strcmp(str2,"reverse")) {
            fprintf(stdout,
            "TIME-REVERSE envelope.\n\n"
            "No other extra parameters needed.\n");
        } else if(!strcmp(str2,"ceiling")) {
            fprintf(stdout,
            "Force envelope up to its MAXIMUM LEVEL, EVERYWHERE.\n"
            "No other extra parameters needed.\n");
        } else if(!strcmp(str2,"ducked")) {
            fprintf(stdout,
            "CREATE DUCKING ENVELOPE.\n\n"
            "2 other parameters.\n"
            "    GATE:      Range(0 - 1)\n"
            "    THRESHOLD: Range(0 - 1)\n"
            "With Envelope Warping:\n"
            "When input envelope exceeds threshold, output envelope is reduced\n"
            "to GATE level. Elsewhere it is unchanged.\n\n"
            "With Envelope Reshaping or Replotting:\n"
            "When input envelope exceeds threshold, output envelope takes\n"
            "GATE level. Elsewhere it gives unity gain.\n\n"
            "This can be used to create an envelope to apply to another sound\n"
            "which will be mixed with sound from which original envelope extracted.\n"
            "Gate and Threshold may vary through time.\n");
        } else if(!strcmp(str2,"exaggerate")) {
            fprintf(stdout,
            "EXAGGERATE envelope contour.\n"
            "1 other parameter :\n"
            "               EXAGGERATE: Range > 0.0\n"
            "              <1, Low vals boosted: >1, High vals boosted.\n"
            "              Value 1, gives no change.\n\n"
            "Exagg may vary over time\n");
        } else if(!strcmp(str2,"attenuate")) {
            fprintf(stdout,
            "ATTENUATE an envelope.\n"
            "1 other parameter : ATTENUATION : Range (0-1)\n\n"
            "Atten may vary over time\n");
        } else if(!strcmp(str2,"lift")) {
            fprintf(stdout,
            "LIFT envelope by fixed amount.\n"
            "1 other parameter: LIFT: Range (0 - 1)\n\n"
            "Lift may vary over time\n");
        } else if(!strcmp(str2,"timestretch")) {
            fprintf(stdout,
            "TIMESTRETCH an envelope.\n"
            "1 other parameter: TIMESTRETCH: Range > 0.0\n"
            "Note that timestretch < 1.0 shrinks the envelope.\n");
        } else if(!strcmp(str2,"flatten")) {
            fprintf(stdout,
            "FLATTEN envelope contour.\n"
            "1 other parameter: FLATTEN: Range (1 - %ld windows)\n\n"
            "Flatten may vary over time\n",round(MAX_ENV_FLATN));
        } else if(!strcmp(str2,"gate")) {
            fprintf(stdout,
            "GATE envelope. Levels less than gate are set to zero.\n"
            "2 other parameters:\n"
            "                GATE:      Range (0 - 1)\n"
            "                SMOOTHING: Range (0 - %ld windows)\n"
            "SMOOTHING excises low-level segments of less than SMOOTHING windows.\n"
            "          value zero turns off smoothing effect.\n\n"
            "Gate may vary over time\n",round(MAX_ENV_SMOOTH));
        } else if(!strcmp(str2,"invert")) {
            fprintf(stdout,
            "INVERT envelope contour.\n"
            "2 other parameters.    GATE:   Range (0 - <MIRROR)\n"
            "                       MIRROR: Range (>GATE - 1)\n"
            "Levels below GATE-level are set to zero. All other levels,\n"
            "above & below MIRROR, are inverted to other side of mirror.\n\n"
            "Gate and Mirror may vary over time\n");
        } else if(!strcmp(str2,"limit")) {
            fprintf(stdout,
            "LIMIT the envelope.\n"
            "2 other parameters   LIMIT:     Range (THRESHOLD - 1)\n"
            "                     THRESHOLD: Range (0 - LIMIT)\n"
            "Levels above THRESHOLD are squeezed downwards so maxamp becomes LIMIT.\n\n"
            "Limit and Threshold may vary over time\n");
        } else if(!strcmp(str2,"corrugate")) {
            fprintf(stdout,
            "CORRUGATE the envelope.  Take all troughs in the envelope to zero.\n"
            " 2 other parameters:\n"
            "    TROFDEL:         number of windows to set to zero, per trough.\n"
            "                     Range(1 to <PEAK_SEPARATION).\n"
            "    PEAK_SEPARATION: Range(2 - %d): min windows between peaks.\n\n"
            "Trofdel and Peak_separation may vary over time.\n",MAX_PEAK_SEPARATION);
        } else if(!strcmp(str2,"expand")) {
            fprintf(stdout,
            "EXPAND the envelope level.\n"
            "3 other parameters.\n"
            "    GATE:      Range(0 - THRESHOLD)\n"
            "    THRESHOLD: Range(GATE - 1)\n"
            "               Levels below GATE set to 0. Other levels squeezed upwards\n"
            "               so minimum level becomes THRESHOLD.\n\n"
            "    SMOOTHING: excises low-level segments, < SMOOTHING windows in length.\n"
            "               Range(0 - %ld) : 0 turns off smoothing effect.\n\n"
            "Gate and Threshold may vary over time\n",round(MAX_ENV_SMOOTH));
        } else if(!strcmp(str2,"trigger")) {
            fprintf(stdout,
            "Create a new envelope of sudden ON bursts, \n"
            "TRIGGERED by the rate of rise of the current envelope.\n"
            "4 other parameters\n"
            "   RAMPFILE: is your new brkpnt envelope for the (triggered) bursts.\n"
            "   GATE:     Range(0 - 1): to trigger, average-level must be > gate.\n"
            "   RISE:     Minimum loudness-step to cause triggering. Range(>0 - 1)\n"
            "   DUR:      max duration (in MS) of min-loudness step, to trigger.\n"
            "             Must be  >= ENVELOPE_WINDOW duration.\n"
            "Gate may vary over time\n");
        } else
            fprintf(stdout,"UNKNOWN MODE\n");
    } else
        fprintf(stdout,"Insufficient parameters on command line.\n");
    return(USAGE_ONLY);
}

/******************************** INNER_LOOP (redundant)  ********************************/

int inner_loop
(int *peakscore,int *descnt,int *in_start_portion,int *least,int *pitchcnt,int windows_in_buf,dataptr dz)
{
//TW PREVENT WARNINGS
#if 0
    peakscore = peakscore;
    descnt = descnt;
    in_start_portion = in_start_portion;
    least = least;
    pitchcnt = pitchcnt;
    windows_in_buf = windows_in_buf;
    dz = dz;
#endif
    return(FINISHED);
}

//TW UPDATE : NEW FUNCTION
/******************************** SCALE_ENVELOPE_DATA ********************************/

int scale_envelope_data(dataptr dz)
{
    int n = 0;
    double scaler;
    double *ebrk = dz->brk[dz->extrabrkno];
    int lastindx = (dz->brksize[dz->extrabrkno] - 1) * 2;
    double firsttime = ebrk[0];
    double lasttime  = ebrk[lastindx];
    double timerange = lasttime - firsttime;
    if(timerange <= 0.0) {
        sprintf(errstr,"Time range in envelope file is too small, or inverted.\n");
        return(DATA_ERROR);
    }
    if(firsttime > 0.0) {
        while(n <=lastindx) {
            ebrk[n] -= firsttime;
            n += 2;
        }
    }
    if(dz->duration <= 0.0) {
        sprintf(errstr,"Duration of input file is anomalous.\n");
        return(DATA_ERROR);
    }
    scaler = dz->duration/ebrk[lastindx];
    n = 0;
    while(n <= lastindx) {
        ebrk[n] *= scaler;
        n += 2;
    }
    return(FINISHED);
}

/******************************** READ_ENVSYN_FILE ********************************/

int  read_envsyn_file(char *str,dataptr dz)
{
    int cnt = 0, n;
    double maxtime;
    char temp[200], *p;
    int arraysize = BIGARRAY;
    
    FILE *fp;
    if((dz->parray[ENVSYN_ENV] = (double *)malloc(arraysize * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for user envelope.\n");
        return(MEMORY_ERROR);
    }
    if((fp = fopen(str,"r"))==NULL) {           /* 2 */
        sprintf(errstr,"Failed to open envelope file %s\n",str);
        return(DATA_ERROR);
    }
    while(fgets(temp,200,fp)!=NULL) {               /* 3 */
        p = temp;
        while(get_float_from_within_string(&p,&(dz->parray[ENVSYN_ENV][cnt]))) {
            if(cnt==0) {             /* FORCE ZERO-TIME POINT AT TAB START */
                if(dz->parray[ENVSYN_ENV][cnt]<0.0) {
                    sprintf(errstr,"-ve time (%lf) line %d in file %s\n",dz->parray[ENVSYN_ENV][cnt],(cnt/2)+1,str);
                    return(DATA_ERROR);
                }
                if(flteq(dz->parray[ENVSYN_ENV][cnt],0.0)) {
                    dz->parray[ENVSYN_ENV][cnt] = 0.0;  /* FORCE 0.0 TIME TO exactly 0.0 */
                } else {                                    /* Add zero-time values */
                    dz->parray[ENVSYN_ENV][2] = dz->parray[ENVSYN_ENV][0];
                    dz->parray[ENVSYN_ENV][3] = dz->parray[ENVSYN_ENV][1];
                    dz->parray[ENVSYN_ENV][0] = 0.0;
                    cnt += 2;
                }
            } else {
                if(EVEN(cnt)) { /* Time vals */
                        /* CHECK TIME SEQUENCING */
                    if(dz->parray[ENVSYN_ENV][cnt] <= dz->parray[ENVSYN_ENV][cnt-2]) {
                        sprintf(errstr,"Time values out of sequence (%lf : %lf)in envelope file at line %d\n",
                        dz->parray[ENVSYN_ENV][cnt-2],dz->parray[ENVSYN_ENV][cnt],(cnt/2)+1);
                        return(DATA_ERROR);
                    }
                } else {        /* Env values */
                    if(dz->parray[ENVSYN_ENV][cnt]<dz->application->min_special 
                    || dz->parray[ENVSYN_ENV][cnt]>dz->application->max_special) {  /* CHECK RANGE */  
                        sprintf(errstr,"Invalid envelope value (%lf): line %d file %s\n",
                        dz->parray[ENVSYN_ENV][cnt],(cnt/2)+1,str);
                        return(DATA_ERROR);
                    }
                }
            }
            if(++cnt >= arraysize) {
                arraysize += BIGARRAY;
                if((dz->parray[ENVSYN_ENV]=
                (double *)realloc((char *)dz->parray[ENVSYN_ENV],arraysize*sizeof(double)))==NULL){
                    sprintf(errstr,"INSUFFICIENT MEMORY to reallocate user envelope.\n");
                    return(MEMORY_ERROR);
                }
            }
        }
    }
    if(ODD(cnt)) {
        sprintf(errstr,"Envelope vals incorrectly paired in file %s\n",str);
        return(DATA_ERROR);
    }
    if(cnt==0) {
        sprintf(errstr,"No envelope values found in file %s\n",str);
        return(DATA_ERROR);
    }
    
    if((dz->parray[ENVSYN_ENV]=(double *)realloc(dz->parray[ENVSYN_ENV],cnt * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to reallocate user envelope.\n");
        return(MEMORY_ERROR);
    }

    dz->ptr[ENVSYN_ENVEND] = &(dz->parray[ENVSYN_ENV][cnt]);    /* MARK END OF ENVELOPE DATA */

    maxtime = dz->parray[ENVSYN_ENV][cnt-2];                        /* FIND MAXIMUM TIME IN DATA */
    for(n=2;n<cnt;n +=2)                          /* NORMALISE DATA TIMES TO LIE BETWEEN 0 AND 1 */
        dz->parray[ENVSYN_ENV][n] = (float)(dz->parray[ENVSYN_ENV][n]/maxtime);
    dz->parray[ENVSYN_ENV][cnt-2] = 1.0;                      /* FORCE FINAL TIME TO exactly 1.0 */
    if(fclose(fp)<0) {
        fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",str);
        fflush(stdout);
    }
    return(FINISHED);
}    

/******************************** CREATE_ENVSYN_BUF ********************************/
/*RWD April 2004: NB bigbufsize strictly a local var now */
int create_envsyn_buf(dataptr dz)
{
    size_t bigbufsize;

    bigbufsize = (size_t) Malloc(-1);
    bigbufsize = (bigbufsize/SECSIZE) * SECSIZE;
    if(bigbufsize <= 0) {
        sprintf(errstr,"INSUFFICIENT MEMORY for envelope data buffers.\n");
        return(MEMORY_ERROR);
    }
    if((dz->bigfbuf = (float *)Malloc(bigbufsize))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create envelope data buffer.\n");
        return(MEMORY_ERROR);
    }
    dz->buflen      = (int)(bigbufsize/sizeof(float));
    dz->flbufptr[0] = dz->bigfbuf;
    return(FINISHED);
}
